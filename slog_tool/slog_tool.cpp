/*** xrkmonitor license ***

   Copyright (c) 2019 by rockdeng

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.


   字符云监控(xrkmonitor) 开源版 (c) 2019 by rockdeng
   使用授权协议： apache license 2.0
   当前版本：v1.0

   云版本主页：http://xrkmonitor.com

   云版本为开源版提供永久免费告警通道支持，告警通道支持短信、邮件、
   微信等多种方式，欢迎使用

   模块 slog_tool 功能:
        slog_tool 为工具模块，用于查看共享内存中的数据

		例如：./slog_tool show app  将输出出所有应用的共享内存数据到控制台

****/

#include <libmysqlwrapped.h>
#include <myparam_comm.h>
#include <errno.h>
#include <Memcache.h>
#include "top_include_comm.h"
#include "memcache.pb.h"
#include "mt_attr.h"

using namespace comm;

#define CONFIG_FILE "./slog_tool.conf"

typedef struct
{
	SLogConfig *pShmConfig;
	SLogAppInfo *pAppInfo;
	int iLoginShmKey;
	int iConfigId;
}CONFIG;
CONFIG stConfig;
CSupperLog slog;

int GetUserSessionInfo(FloginInfo *psess, user::UserSessionInfo & user)
{
	int *piLen = (int*)psess->sReserved;
	char *pbuf = (char*)(psess->sReserved+4);

	user.Clear();
	if(*piLen > 0 && !user.ParseFromArray(pbuf, *piLen))
	{
		return SLOG_ERROR_LINE;
	}
	return 0;
}

static void ShowLoginUser(int iUserId=0)
{
	FloginList *pshmLoginList = NULL;
	int iRet = 0;
	if((iRet=GetShm2((void**)(&pshmLoginList), stConfig.iLoginShmKey, sizeof(FloginList), 0666)))
	{
		printf("Get login Shm failed , shmkey:%d, size:%d\n", 
			stConfig.iLoginShmKey, (int)sizeof(FloginList));
		return ;
	}

	uint32_t dwCurTime = time(NULL);
	for(int j=0; j < FLOGIN_SESSION_NODE_COUNT; j++)
	{
		if(pshmLoginList->stLoginList[j].iUserId == 0
			|| dwCurTime > pshmLoginList->stLoginList[j].dwLastAccessTime
				+ pshmLoginList->stLoginList[j].iLoginExpireTime
			|| dwCurTime > pshmLoginList->stLoginList[j].dwLoginTime+LOGIN_MAX_EXPIRE_TIME)
			continue;
		if(iUserId != 0 && iUserId != pshmLoginList->stLoginList[j].iUserId)
			continue;
		printf("---- login user info:%d ---- \n", j);
		pshmLoginList->stLoginList[j].Show();

		user::UserSessionInfo sess;
		GetUserSessionInfo(pshmLoginList->stLoginList+j, sess);
		printf("\n\tsess:%s\n", sess.ShortDebugString().c_str());
	}
}

const char * GetTodayTableName()
{
	static char szTableName[32];
	time_t tmNow = time(NULL); 
	struct tm curr = *localtime(&tmNow);
	if(curr.tm_year > 50)
		snprintf(szTableName, sizeof(szTableName), "attr_%04d%02d%02d",
			curr.tm_year+1900, curr.tm_mon+1, curr.tm_mday);
	else
		snprintf(szTableName, sizeof(szTableName), "attr_%04d%02d%02d",
			curr.tm_year+2000, curr.tm_mon+1, curr.tm_mday);
	return szTableName;
}

void ShowHelp()
{
	printf("cmd as followed: \n");
	printf("\t show memcache set key val\n");
	printf("\t show memcache get key \n");
	printf("\t show memcache machine_attr machine_id\n");
	printf("\t show memcache machine_attr_val attr_id machine_id\n");
	printf("\t show str_report_info attr_id machine_id\n");
	printf("\t show warninfo [warn_id|0]\n");
	printf("\t show view [view_id|0]\n");
	printf("\t show vmemval idx\n");
	printf("\t show vmem [shmkey len idx]\n");
	printf("\t show [ips2d ipd2s] [ip digit]\n");
	printf("\t show sysconfig [attr | app | client | logconfig | machine | view | remoteloginfo] \n");
	printf("\t show server [server id] \n");
	printf("\t show config [config id] \n");
	printf("\t show attrtype [0|attrtype id] \n");
	printf("\t show attrtype_tree  \n");
	printf("\t show attr [attr id] \n");
	printf("\t show app [app id] \n");
	printf("\t show loginUser [user id] \n");
	printf("\t show ctable \n");
	printf("\t show log \n");
	printf("\t show machine [machine id] \n");
	printf("\t show rtable \n");
	printf("\t show warnconfig [warn type id - attrid] \n");
	printf("\t show machview [machine id] \n");
	printf("\t show attrview [attr id] \n");
	printf("\t show varconfig \n");
	printf("\t show attrreport [machineid attrid]\n");
	printf("\t show applogfile [appid] \n");
	printf("\t show attrlist \n");
	printf("\t show mailshm\n");
	printf("\t show ipinfo ip\n");
}

void ShowTableInfo(int argc, char *argv[])
{
	if(argc < 3 || !strcmp(argv[2], "help")) {
		ShowHelp();
		return;
	}

	if(strcmp(argv[1], "show"))
	{
		ShowHelp();
		return;
	}

	const char *ptabName = argv[2];
	printf("\n\n\nshow table :%s info ----------------------------- \n", ptabName);

	if(!strcmp(ptabName, "view")) {
		if(argc < 4) {
			printf("\n use xxx show view [view_id|0]\n");
			return;
		}
		int iView = atoi(argv[3]);
		slog.ShowViewInfo(iView);
		return;
	}
	else if(!strcmp(ptabName, "memcache")) 
	{
		if(slog.InitMemcache() < 0) 
		{   
			printf("\n InitMemcache failed\n");
			return ;
		}   

		if(argc < 4) {
			printf("\n use xxx show memcache [item_name] [item_para]\n");
			printf("\n use xxx show memcache set/get key val\n");
			return;
		}
		const char *pitem = argv[3];
		if(!strcmp(pitem, "set")) {
			if(argc < 6) {
				printf("\n use xxx show memcache set key val\n");
				return;
			}
			slog.memcache.SetKey("test-%s", argv[4]);
			if(slog.memcache.SetValue(argv[5]) != 0) {
				printf("memcache set val:%s failed \n", argv[5]);
				return;
			}
			printf("memcache set key:%s,val:%s ok\n", argv[4], argv[5]);
			return;
		}
		else if(!strcmp(pitem, "get")) {
			if(argc < 5) {
				printf("\n use xxx show memcache set key val\n");
				return;
			}
			slog.memcache.SetKey("test-%s", argv[4]);
			const char *pval = NULL;
			if((pval=slog.memcache.GetValue()) == NULL) {
				printf("get memcache key:%s value failed\n", argv[4]);
				return;
			}
			printf("get val:%s\n", pval);
			return;
		}
		else if(!strcmp(pitem, "machine_attr")) {
			if(argc < 5) {
				printf("use xxx show memcache machine_attr machine_id\n");
				return;
			}
			slog.memcache.SetKey("machine-attr-%s-%s-monitor", GetTodayTableName(), argv[4]);
			int *pattr_list = (int*)slog.memcache.GetValue();
			int iLen = slog.memcache.GetDataLen();
			if(iLen > 0 && pattr_list != NULL) {
				printf("get memcache value ok, key:%s\n", slog.memcache.GetKey());
				for(int i=0; iLen >= (int)sizeof(int); iLen -= (int)sizeof(int), i++)
				{
					printf("attr:%d- %d\n", i, ntohl(pattr_list[i]));
				}
			}
			else {
				printf("get memcache value failed|%d|%p, key:%s\n", iLen, pattr_list, slog.memcache.GetKey());
				return;
			}
		}
		else if(!strcmp(pitem, "machine_attr_val")) {
			if(argc < 6) {
				printf("use xxx show memcache machine_attr_val attr_id machine_id\n");
				return;
			}
			slog.memcache.SetKey("machine-attr-val-%s-%s-%s-monitor", GetTodayTableName(), argv[4], argv[5]);

			comm::MonitorMemcache memInfo;
			if(slog.memcache.GetMonitorMemcache(memInfo) >= 0 && memInfo.machine_attr_day_val().attr_val_size() > 0)
			{
				printf("get memcache value ok, key:%s\nval:\n\t%s\n", slog.memcache.GetKey(), memInfo.DebugString().c_str());
			}
			else {
				printf("get memcache value failed, key:%s\n", slog.memcache.GetKey());
				return;
			}
		}
		return ;
	}

	if(!strcmp(ptabName, "str_report_info")) {
		if(argc < 5) {
			printf("use show str_report_info attr_id machine_id\n");
			return;
		}
		TStrAttrReportInfo *pstrShm = slog.GetStrAttrShmInfo(atoi(argv[3]), atoi(argv[4]), NULL);
		if(pstrShm == NULL) {
			printf("not find str attr:%d, machine:%d info\n", atoi(argv[3]), atoi(argv[4]));
			return;
		}
		pstrShm->Show();
		printf("\n");
		StrAttrNodeValShmInfo *pstrAttrShm = slog.GetStrAttrNodeValShm(false);
		if(pstrAttrShm == NULL) {
			printf("GetStrAttrNodeValShm failed\n");
			return;
		}
		pstrAttrShm->Show();
		printf("\n");
		int idx = pstrShm->iReportIdx;
		for(int i=0; i < pstrShm->bStrCount; i++)
		{
			pstrAttrShm->stInfo[idx].Show();
			idx = pstrAttrShm->stInfo[idx].iNextStrAttr;
		}
	}


	if(!strcmp(ptabName, "vmemval")) {
		if(argc < 4) {
			ShowHelp();
			return;
		}
		int idx = atoi(argv[3]);
		const char *ptmp = MtReport_GetFromVmem_Local(idx);
		printf("get vmem info, idx:%d, val:%s\n", idx, ptmp);
	}
	else if(!strcmp(ptabName, "vmem")) 
	{
		int iShmKey = VMEM_DEF_SHMKEY;
		if(argc >= 4)
			iShmKey = atoi(argv[3]);
		int iShowLen = 0;
		if(argc >= 5)
			iShowLen = atoi(argv[4]);
		int iShowArryIdx = -1;
		if(argc >= 6)
			iShowArryIdx = atoi(argv[5]);
		int iRet = MtReport_InitVmem_ByFlag(0666, iShmKey);
		if(iRet >= 0)
			printf("init vmem ok shmkey:%d\n", iShmKey);
		else {
			printf("init vmem failed(def key:%d), key:%d\n", VMEM_DEF_SHMKEY, iShmKey);
			return;
		}
		MtReport_show_shm(iShowLen, iShowArryIdx);
		return;
	}

	if(!strcmp(ptabName, "ips2d")) {
		if(argc < 4) {
			ShowHelp();
			return;
		}
		uint32_t ip = inet_addr(argv[3]);
		printf("%u - %#x\n", ip, ip);
		return;
	}

	if(!strcmp(ptabName, "ipd2s")) {
		if(argc < 4) {
			ShowHelp();
			return;
		}
		uint32_t ip = strtoul(argv[3], NULL, 10);
		printf("%s\n", ipv4_addr_str(ip));
		return;
	}

	if(!strcmp(ptabName, "varconfig")) {
		slog.ShowVarConfig();
		return;
	}

	if(!strcmp(ptabName, "sysconfig")) {
		if(argc >= 4)
			slog.ShowSystemConfig(argv[3]);
		else
			slog.ShowSystemConfig();
		return;
	}

	if(!strcmp(ptabName, "server")) {
		if(argc >= 4)
			slog.ShowServerInfo(strtoul(argv[3], NULL, 10));
		else
			slog.ShowServerInfo();
		return;
	}

	if(!strcmp(ptabName, "config")) {
		if(argc >= 4)
			slog.ShowSlogConfig(atoi(argv[3]));
		else
			slog.ShowSlogConfig();
		return;
	}

	if(!strcmp(ptabName, "attrtype_tree")) {
		if(argc < 4) {
			printf("use xx show attrtype [0|attrtype id]\n");
			return;
		}
		MmapUserAttrTypeTree stTypeTree;
		char sTmp[2048] = {0};
		int iLen = (int)sizeof(sTmp);
		int iVmemIdx = 0;

		MtSystemConfig *psysConfig = slog.GetSystemCfg();
		iVmemIdx = psysConfig->iAttrTypeTreeVmemIdx;
		if(iVmemIdx > 0) {
			const char *pbuf = MtReport_GetFromVmemZeroCp(iVmemIdx, sTmp, &iLen);
			if(pbuf != NULL && iLen > 0) {
				if(!stTypeTree.ParseFromArray(pbuf, iLen)) {
					printf("parse attr type tree for failed, vmemidx:%d, len:%d\n", iVmemIdx, iLen);
				}
				else {
					printf("get attr type tree ok, vmemidx:%d, len:%d, tree:\n%s\n", 
						iVmemIdx, iLen, stTypeTree.DebugString().c_str());
				}
			}
			else {
				printf("get attr type tree failed, vmemidx:%d, len:%d\n", iVmemIdx, iLen);
			}
		}
		else {
			printf("have no attr type \n");
		}
		return;
	}

	if(!strcmp(ptabName, "attrtype")) {
		if(slog.InitAttrTypeList() < 0)
		{
			printf("init mt_attr_type shm failed !\n");
			return ;
		}

		if(argc >= 4)
			slog.ShowAttrTypeList(atoi(argv[3]));
		else
			slog.ShowAttrTypeList();
		return;
	}

	if(!strcmp(ptabName, "warninfo")) {
		if(argc >= 4)
			slog.ShowWarnInfo(strtoul(argv[3], NULL, 10));
		else
			slog.ShowWarnInfo();
		return;
	}

	if(!strcmp(ptabName, "attr")) {
		if(argc >= 4)
			slog.ShowAttrList(strtoul(argv[3], NULL, 10));
		else
			slog.ShowAttrList();
		return;
	}

	if(!strcmp(ptabName, "app")) {
		if(argc >= 4)
			slog.ShowAppShmLog(strtoul(argv[3], NULL, 10));
		else
			slog.ShowAppShmLog();
		return;
	}

	if(!strcmp(ptabName, "loginUser")) {
		if(argc >= 4)
			ShowLoginUser(strtoul(argv[3], NULL, 10));
		else
			ShowLoginUser();
		return;
	}

	if(!strcmp(ptabName, "log")) {
		slog.ShowShmLogInfo();
		return;
	}

	if(!strcmp(ptabName, "machine")) {
		if(argc >= 4)
			slog.ShowMachineList(strtoul(argv[3], NULL, 10));
		else
			slog.ShowMachineList();
		return;
	}

	if(!strcmp(ptabName, "warnconfig")) {
		if(slog.InitwarnConfig() < 0)
		{
			printf("init InitwarnConfig info shm failed !\n");
			return;
		}

		if(argc >= 5) {
			int iWarnId = atoi(argv[3]);
			int iAttrId = atoi(argv[4]);
			TWarnConfig *pConfig = slog.GetWarnConfigInfo(iWarnId, iAttrId, NULL);
			if(pConfig)
			{
				printf("warn config info -- \n");
				pConfig->Show();
			}
			else
				printf("not find warn config\n");
		}
		else
		{
			printf("invalid argument ! -- show [warn type id] attrid \n");
		}
	}

	if(!strcmp(ptabName, "attrreport")) {
		if(slog.InitWarnAttrList() < 0)
		{
			printf("InitWarnAttrList failed\n");
			return;
		}
		if(argc >= 5) {
			int iMachineId = atoi(argv[3]);
			int iAttrId = atoi(argv[4]);
			TWarnAttrReportInfo *pAttrShm = slog.GetWarnAttrInfo(iAttrId, iMachineId, NULL);
			if(pAttrShm)
			{
				printf("attr report info -- \n");
				pAttrShm->Show();
			}
			else
				printf("not find report\n");
		}
		else
		{
			printf("invalid argument ! -- show attrreport machineId attrId \n");
		}
	}

	if(!strcmp(ptabName, "machview")) {
		if(argc >= 4) {
			int iMachineId = atoi(argv[3]);
			TMachineViewConfigInfo *pInfo = slog.GetMachineViewInfo(iMachineId, NULL);
			if(pInfo != NULL)
				pInfo->Show();
		}
		else {
			printf("invalid argument ! -- show machview machineId\n");
		}
	}

	if(!strcmp(ptabName, "ipinfo")) {
		if(argc < 4) {
			printf("\n use xxx show ipinfo ip\n");
			return;
		}
		uint32_t ip = ntohl( inet_addr(argv[3]) );
		TIpInfo *pInfo = slog.GetIpInfo( ip );
		if(pInfo != NULL)
			pInfo->Show();
		else {
			TIpInfoShm *pshm =  slog.GetIpInfoShm();
			if(pshm != NULL) {
				printf("not find ip:%s, :%u, count:%d, first info\n", argv[3], ip, pshm->iCount);
				pshm->ips[0].Show();
			}
			else {
				printf("ipinfo may by not load from file !\n");
			}
		}
	}

	if(!strcmp(ptabName, "mailshm")) {
		TCommSendMailInfoShm *pshm = slog.InitMailInfoShm();
		if(pshm != NULL)
			pshm->Show();
		else
			printf("get mail shm failed !\n");
	}

	if(!strcmp(ptabName, "attrview")) {
		if(argc > 3) {
			int id = atoi(argv[3]);
			TAttrViewConfigInfo *pInfo = slog.GetAttrViewInfo(id, NULL);
			if(pInfo != NULL)
				pInfo->Show();
		}
		else {
			printf("invalid argument ! -- show attrview attrId\n");
		}
	}

	if(!strcmp(ptabName, "applogfile")) {
		if(argc > 3) {
			int id = atoi(argv[3]);
			CSLogSearch log;
			if(log.Init() < 0 || log.SetAppId(id) < 0)
			{
				printf("logsearch init failed, id:%d\n", id);
				return;
			}
			SLogFile *pfile = log.GetLogFileShm();
			if(pfile) 
			{
				pfile->Show();
				printf("total :%d slog files ------- \n", pfile->wLogFileCount);
				for(int i=0; i < pfile->wLogFileCount; i++)
				{
					printf("file:%d:%s records:%d time:%" PRIu64 "-%" PRIu64 "-%" PRIu64 "\n", i,
							pfile->stFiles[i].szAbsFileName,
							pfile->stFiles[i].stFileHead.iLogRecordsWrite,
							pfile->stFiles[i].qwLogTimeStart,
							pfile->stFiles[i].stFileHead.qwLogTimeStart,
							pfile->stFiles[i].stFileHead.qwLogTimeEnd);
				}
				return;
			}
		}
		else {
			printf("invalid argument ! -- show applogfile appId [user]\n");
		}
	}
}

int Init(const char *pFile = NULL)
{
	const char *pConfFile = NULL;
	if(pFile != NULL)
		pConfFile = pFile;
	else
		pConfFile = CONFIG_FILE;

	int32_t iRet = 0, iLogToStd = 0;
	if((iRet=LoadConfig(pConfFile,
		"SLOG_LOG_TO_STD", CFG_INT, &iLogToStd, 0,
		"SLOG_CONFIG_ID", CFG_INT, &stConfig.iConfigId, 0,
		"FLOGIN_SHM_KEY", CFG_INT, &stConfig.iLoginShmKey, FLOGIN_SESSION_HASH_SHM_KEY,
		(void*)NULL)) < 0)
	{   
		printf("LoadConfig:%s failed ! ret:%d\n", pConfFile, iRet);
		return SLOG_ERROR_LINE;
	} 

	if((iRet=slog.InitConfigByFile(pConfFile)) >= 0 && stConfig.iConfigId != 0)
	{
		if(slog.GetSlogConfig(stConfig.iConfigId) != NULL)
		{
			if(slog.Init(NULL) < 0)
				return SLOG_ERROR_LINE;
		}
		else if(slog.InitForUseLocalLog(pConfFile) < 0)
		{
			return SLOG_ERROR_LINE;
		}
	}
	else if(slog.InitForUseLocalLog(pConfFile) < 0)
	{
		return SLOG_ERROR_LINE;
	}

	stConfig.pShmConfig = slog.GetSlogConfig();
	stConfig.pAppInfo = slog.GetAppInfo();
	if(stConfig.pShmConfig == NULL || stConfig.pAppInfo == NULL) 
	{
		FATAL_LOG("get pShmConfig or pAppInfo failed !");
		return SLOG_ERROR_LINE;
	}

	if(slog.InitAttrList() < 0)
	{
		FATAL_LOG("init mt_attr shm failed !");
		return SLOG_ERROR_LINE;
	}

	if(slog.InitStrAttrHash() < 0)
	{
		ERR_LOG("InitStrAttrHash failed");
		return SLOG_ERROR_LINE;
	}

	if(slog.InitMachineList() < 0)
	{
		ERR_LOG("init machine list shm failed !");
		return SLOG_ERROR_LINE;
	}

	if(slog.InitMachineViewConfig() < 0)
	{
		ERR_LOG("init InitwarnConfig info shm failed !");
		return SLOG_ERROR_LINE;
	}

	if(slog.InitAttrViewConfig() < 0)
	{
		ERR_LOG("init InitwarnConfig info shm failed !");
		return SLOG_ERROR_LINE;
	}

	MtReport_InitVmem();
	return 0;
}

int main(int argc, char *argv[])
{
	if(argc >= 2 && !strcmp(argv[1], "run_test")) {
		return 0;
	}
	if(argc >= 4 && !strcmp(argv[1], "show") && (!strcmp(argv[2], "ips2d") || !strcmp(argv[2], "ipd2s")))
	{
		ShowTableInfo(argc, argv);
		return 0;
	}

	if(Init(NULL) < 0)
	{
		printf("init failed !\n");
		return -1;
	}
	ShowTableInfo(argc, argv);
	return 0;
}

