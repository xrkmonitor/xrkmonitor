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

   云版本主页：http://xrkmonitor.com

   云版本为开源版提供永久免费告警通道支持，告警通道支持短信、邮件、
   微信等多种方式，欢迎使用

   模块 slog_config 功能:
        将 mysql 中的配置拉取到本地共享内存中，通过 mysql 的触发器实现
        高性能低成本的配置变化监控，在配置变化时无须扫表实现精准的配置
        记录读取。

*/


#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS 1
#endif

#include <libmysqlwrapped.h>
#include <myparam_comm.h>
#include <errno.h>
#include <set>
#include "top_include_comm.h"
#include "memcache.pb.h"

using namespace comm;

#define CONFIG_FILE "./slog_config.conf"
#define DELAY_RECORD_CHECK_TIMEOUT 60

#define RECORD_STATUS_USE 0
#define RECORD_STATUS_DELETE 1

#define READ_TABLE_FLAG_flogin_user 2 
#define READ_TABLE_FLAG_mt_app_info 4
#define READ_TABLE_FLAG_mt_attr 8
#define READ_TABLE_FLAG_mt_attr_type 16
#define READ_TABLE_FLAG_mt_log_config 64
#define READ_TABLE_FLAG_mt_machine 128
#define READ_TABLE_FLAG_mt_module_info 512 
#define READ_TABLE_FLAG_mt_server 1024 
#define READ_TABLE_FLAG_mt_view 8192 
#define READ_TABLE_FLAG_mt_view_battr 16384 
#define READ_TABLE_FLAG_mt_view_bmach 32768
#define READ_TABLE_FLAG_mt_warn_config 65536 
#define READ_TABLE_FLAG_mt_warn_info 131072
#define READ_TABLE_FLAG_test_key_list 262144

typedef struct
{
	char szDbHost[32];
	char szUserName[32];
	char szPass[32];
	char szDbName[32];
	int iDbPort;
	char szLocalIp[20];
	uint32_t dwDbHostIp;
	char szLogFile[256];
	char szAccessKey[33];
	int iDelRecordTime;

	int iLoadIp;

	// iDeleteDbRecord, iDeleteMonitorRecords 需要配置为 1 时只能在一台机器上配置
	// 删除状态的数据，是否执行物理删除 1 执行，0 不执行
	int iDeleteNoUseRecord; 
	int iDeleteDbRecord;
	// 监控配置变化的记录是否执行物理删除 1 执行， 0 不执行
	int iDeleteMonitorRecords;

	SLogConfig *pShmConfig;
	SLogAppInfo *pAppInfo;
	MtSystemConfig *psysConfig;
	bool bUpdateAll;
	int iLoginShmKey;
	int iCheckDataPerTime;

	uint32_t dwModuleDelaySetTime;
	std::vector<int> stModuleDelay;
	FloginList *pshmLoginList;
	int iNeedStrAttrShm;
	StrAttrNodeValShmInfo *pStrAttrShm;
	Database *pdb;
	Query *qu_info;

	char szPluginCheckStr[16];
}CONFIG;

CONFIG stConfig;
CSupperLog slog;

int int32Cmp(const void *a, const void *b)
{
	return *(int*)a - *(int*)b;
}

int GetReadTableFlagByStr(const char *pstr)
{
	int iFlag = 0x7fffffff;
	if(strstr(pstr, "mt_attr_type"))
		iFlag = clear_bit(iFlag, READ_TABLE_FLAG_mt_attr_type);
	if(strstr(pstr, "mt_log_config"))
		iFlag = clear_bit(iFlag, READ_TABLE_FLAG_mt_log_config);
	if(strstr(pstr, "mt_machine"))
		iFlag = clear_bit(iFlag, READ_TABLE_FLAG_mt_machine);
	if(strstr(pstr, "mt_module_info"))
		iFlag = clear_bit(iFlag, READ_TABLE_FLAG_mt_module_info);
	if(strstr(pstr, "mt_server"))
		iFlag = clear_bit(iFlag, READ_TABLE_FLAG_mt_server);
	if(strstr(pstr, "mt_view"))
		iFlag = clear_bit(iFlag, READ_TABLE_FLAG_mt_view);
	if(strstr(pstr, "mt_view_battr"))
		iFlag = clear_bit(iFlag, READ_TABLE_FLAG_mt_view_battr);
	if(strstr(pstr, "mt_view_bmach"))
		iFlag = clear_bit(iFlag, READ_TABLE_FLAG_mt_view_bmach);
	if(strstr(pstr, "mt_warn_config"))
		iFlag = clear_bit(iFlag, READ_TABLE_FLAG_mt_warn_config);
	if(strstr(pstr, "mt_warn_info"))
		iFlag = clear_bit(iFlag, READ_TABLE_FLAG_mt_warn_info);
	if(strstr(pstr, "test_key_list"))
		iFlag = clear_bit(iFlag, READ_TABLE_FLAG_test_key_list);
	return iFlag;
}

int CheckIpInfo()
{
	TIpInfoShm * pInfoShm = slog.GetIpInfoShm();
	if(pInfoShm == NULL)
	{
		ERR_LOG("GetIpInfoShm failed");
		return SLOG_ERROR_LINE;
	}

	TIpInfo *pInfoPrev = NULL;
	for(int i = 0; i < pInfoShm->iCount; i++) {
		if(pInfoPrev == NULL) {
			pInfoPrev = pInfoShm->ips+i;
			continue;
		}
		if(pInfoPrev->dwStart > pInfoPrev->dwEnd) {
			WARN_LOG("ipinfo check failed:%d, %u > %u", i, pInfoPrev->dwStart, pInfoPrev->dwEnd);
			return SLOG_ERROR_LINE;
		}
		if(pInfoPrev->dwEnd >= pInfoShm->ips[i].dwStart) {
			WARN_LOG("ipinfo check failed:%d, %u >= %u", i, pInfoPrev->dwEnd, pInfoShm->ips[i].dwStart);
			return SLOG_ERROR_LINE;
		}
		pInfoPrev = pInfoShm->ips+i;
	}
	return 0;
}

int32_t ReadIpInfo(const char *pip_info_file, bool bReset=false)
{
	TIpInfoShm * pInfoShm = slog.GetIpInfoShm();
	if(pInfoShm == NULL)
	{
		ERR_LOG("GetIpInfoShm failed");
		return SLOG_ERROR_LINE;
	}
	if(bReset) {
		pInfoShm->iCount = 0;
	}

	FILE *fp = fopen(pip_info_file, "rb");
	if(fp == NULL) {
		ERR_LOG("open ipinfo file:%s failed, msg:%s", pip_info_file, strerror(errno));
		return SLOG_ERROR_LINE;
	}

	char sMagicStr[128] = {0};
	int iReadLen = 0;
	iReadLen = fread(sMagicStr, 1, 128, fp);
	if(iReadLen != 128 || strcmp(sMagicStr, IPINFO_FILE_MAGIC_STR)) {
		ERR_LOG("read file:%s check magic failed:(%d, %s, %s)", 
			pip_info_file, iReadLen,  sMagicStr, IPINFO_FILE_MAGIC_STR);
		fclose(fp);
		return SLOG_ERROR_LINE;
	}

	TIpInfoInFile sFileIpinfo;
	uint32_t start = 0, end = 0, dwIpCounts = 0;
	const char *ptmp = NULL, *ptmp2 = NULL;
	TIpInfo *pInfo = NULL;
	int iIpNewCount = 0;

	while((iReadLen=fread(&sFileIpinfo, 1, sizeof(sFileIpinfo), fp)) == (int)sizeof(sFileIpinfo))
	{
		start = strtoul(sFileIpinfo.sStart, NULL, 10);
		end = strtoul(sFileIpinfo.sEnd, NULL, 10);
		dwIpCounts += end-start+1;

#define SAVE_IP_INFO(pf, pflag, vidx) \
		ptmp = sFileIpinfo.pf; \
		if(ptmp[0] != '\0' && strlen(ptmp) < sizeof(pInfo->pf)) \
			strcpy(pInfo->pf, ptmp); \
		else { \
			pInfo->bSaveFlag |= pflag; \
			pInfo->vidx = MtReport_SaveToVmem(ptmp, strlen(ptmp)+1); \
		}

		if(pInfoShm->iCount == 0) {
			pInfo = pInfoShm->ips + iIpNewCount;
			memset(pInfo, 0, sizeof(*pInfo));
			pInfo->dwStart = start;
			pInfo->dwEnd = end;
			SAVE_IP_INFO(sprov, IPINFO_FLAG_PROV_VMEM, iProvVmemIdx);
			SAVE_IP_INFO(scity, IPINFO_FLAG_CITY_VMEM, iCityVmemIdx);
			SAVE_IP_INFO(sowner, IPINFO_FLAG_OWNER_VMEM, iOwnerVmemIdx);
			iIpNewCount++;
			if(iIpNewCount >= IPINFO_HASH_NODE_COUNT) {
				WARN_LOG("need more buffer to save ip info, read:%d", iIpNewCount);
				break;
			}
			continue;
		}

		pInfo = slog.GetIpInfo(start, IpInfoInitCmp);
		if(pInfo == NULL) {
			if(pInfoShm->iCount >= IPINFO_HASH_NODE_COUNT) {
				WARN_LOG("need more buffer to save ip info");
				break;
			}
			pInfo = pInfoShm->ips + pInfoShm->iCount;
			memset(pInfo, 0, sizeof(*pInfo));
			pInfo->dwStart = start;
			pInfo->dwEnd = end;
			SAVE_IP_INFO(sprov, IPINFO_FLAG_PROV_VMEM, iProvVmemIdx);
			SAVE_IP_INFO(scity, IPINFO_FLAG_CITY_VMEM, iCityVmemIdx);
			SAVE_IP_INFO(sowner, IPINFO_FLAG_OWNER_VMEM, iOwnerVmemIdx);
			pInfoShm->iCount++;
			qsort(pInfoShm->ips, pInfoShm->iCount, MYSIZEOF(TIpInfo), IpInfoInitCmp);
		}
		else {
#define CHECK_IP_UPDATE_INFO(pf, pflag, vidx) \
		ptmp = sFileIpinfo.pf; \
		if(pInfo->bSaveFlag & pflag) \
			ptmp2 = MtReport_GetFromVmem_Local(pInfo->vidx); \
		else \
			ptmp2 = pInfo->pf; \
		if(ptmp2 == NULL || (ptmp2 != NULL && strcmp(ptmp, ptmp))) { \
			if(pInfo->bSaveFlag & pflag) { \
				MtReport_FreeVmem(pInfo->vidx); \
				pInfo->bSaveFlag = clear_bit(pInfo->bSaveFlag, pflag); \
			} \
			if(ptmp != NULL && ptmp[0] != '\0' && strlen(ptmp) < sizeof(pInfo->pf)) \
				strcpy(pInfo->pf, ptmp); \
			else { \
				pInfo->bSaveFlag |= pflag; \
				pInfo->vidx = MtReport_SaveToVmem(ptmp, strlen(ptmp)+1); \
			} \
		}
			if(pInfo->dwEnd != end)
				pInfo->dwEnd = end;
			CHECK_IP_UPDATE_INFO(sprov, IPINFO_FLAG_PROV_VMEM, iProvVmemIdx);
			CHECK_IP_UPDATE_INFO(scity, IPINFO_FLAG_CITY_VMEM, iCityVmemIdx);
			CHECK_IP_UPDATE_INFO(sowner, IPINFO_FLAG_OWNER_VMEM, iOwnerVmemIdx);
#undef CHECK_IP_UPDATE_INFO 
		}
	}
#undef SAVE_IP_INFO
	fclose(fp);

	if(iIpNewCount > 0) {
		pInfoShm->iCount = iIpNewCount;
		qsort(pInfoShm->ips, iIpNewCount, MYSIZEOF(TIpInfo), IpInfoInitCmp);
	}

	if(pInfoShm->iCount > 0) {
		INFO_LOG("read ipinfo count:%d, ips:%u, first:%u-%u, last:%u-%u", 
			pInfoShm->iCount,  dwIpCounts,
			pInfoShm->ips[0].dwStart, pInfoShm->ips[0].dwEnd,
			pInfoShm->ips[pInfoShm->iCount-1].dwStart, pInfoShm->ips[pInfoShm->iCount-1].dwEnd);
	}
	return 0;
}


/*
   *
   * 首次运行时将日志调成本地的日志,并输出全部日志可以定位启动问题
   * SLOG_OUT_TYPE 3, SLOG_TYPE all, SLOG_SET_TEST 1
   *
   */
int Init(const char *pFile = NULL)
{
	const char *pConfFile = NULL;
	if(pFile != NULL)
		pConfFile = pFile;
	else
		pConfFile = CONFIG_FILE;

	int32_t iRet = 0, iLogToStd = 0, iLogWriteSpeed = 100, iSysDaemon = 0;
	int32_t  iConfigId = 0, iVmemShmKey = 0, iLoginShmKey = 0;
	if((iRet=LoadConfig(pConfFile,
		// 插件一键部署校验码，如需设置，请注意使用字母数字组合，某些特殊字符会导致参数解析错误
		"PLUGIN_INSTALL_CHECKSTR", CFG_STRING, stConfig.szPluginCheckStr, "", MYSIZEOF(stConfig.szPluginCheckStr),
		"MYSQL_SERVER", CFG_STRING, stConfig.szDbHost, "127.0.0.1", MYSIZEOF(stConfig.szDbHost),
		"MYSQL_USER", CFG_STRING, stConfig.szUserName, "mtreport", MYSIZEOF(stConfig.szUserName),
		"MYSQL_PASS", CFG_STRING, stConfig.szPass, "mtreport875", MYSIZEOF(stConfig.szPass),
		"MYSQL_DATABASE", CFG_STRING, stConfig.szDbName, "mtreport_db", MYSIZEOF(stConfig.szDbName),
		"MYSQL_DB_PORT", CFG_INT, &stConfig.iDbPort, 3306,
		"FLOGIN_SHM_KEY", CFG_INT, &iLoginShmKey, FLOGIN_SESSION_HASH_SHM_KEY,
		"LOCAL_IP", CFG_STRING, stConfig.szLocalIp, "", MYSIZEOF(stConfig.szLocalIp),
		"SLOG_LOG_FILE", CFG_STRING, stConfig.szLogFile, "./slog_config.log", MYSIZEOF(stConfig.szLogFile),
		"SLOG_LOG_TO_STD", CFG_INT, &iLogToStd, 0,
		"LOCAL_DELETE_NO_USE_IN_SQL", CFG_INT, &stConfig.iDeleteNoUseRecord, 0,
		"DELETE_DB_RECORD", CFG_INT, &stConfig.iDeleteDbRecord, 0,
		"DELETE_MONITOR_RECORDS", CFG_INT, &stConfig.iDeleteMonitorRecords, 0,
		"SLOG_LOG_SPEED", CFG_INT, &iLogWriteSpeed, 100, 
		"SLOG_CONFIG_ID", CFG_INT, &iConfigId, 0,
		"AGENT_ACCESS_KEY", CFG_STRING, stConfig.szAccessKey, "232k8s8d8f20@#@%@#$@skdfj2351%^", MYSIZEOF(stConfig.szAccessKey),
		"FLOGIN_SHM_KEY", CFG_INT, &stConfig.iLoginShmKey, FLOGIN_SESSION_HASH_SHM_KEY,
		"VMEM_SHM_KEY", CFG_INT, &iVmemShmKey, VMEM_DEF_SHMKEY,
		"CHECK_DATA_PER_TIME", CFG_INT, &stConfig.iCheckDataPerTime, 1,
		"NEED_STR_ATTR_SHM", CFG_INT, &stConfig.iNeedStrAttrShm, 1,
		"DELETE_DB_RECORD_TIME", CFG_INT, &stConfig.iDelRecordTime, 24*60*60,
		"SYSTEM_FLAG_DAEMON", CFG_INT, &iSysDaemon, 0,
		"LOAD_IPINFO", CFG_INT, &stConfig.iLoadIp, 0,
		(void*)NULL)) < 0)
	{   
		ERR_LOG("LoadConfig:%s failed ! ret:%d", pConfFile, iRet);
		return SLOG_ERROR_LINE;
	} 

	// 物理删除数据库记录需要间隔一定时间，否则在分布式部署环境可能出现某些机器未读取就被删除
	if(stConfig.iDelRecordTime < 30*60) 
		stConfig.iDelRecordTime = 30*60;

	if(iVmemShmKey > 0) {
		iRet = MtReport_InitVmem_ByFlag(0666|IPC_CREAT, iVmemShmKey);
		if(iRet < 0) {
			ERR_LOG("Init vmem buf failed, key:%d, ret:%d", iVmemShmKey, iRet);
			return SLOG_ERROR_LINE;
		}
	}

	// 这里要先调用, attach 上共享内存，以便后面读取数据库 -- 重要
	if((iRet=slog.InitConfigByFile(pConfFile, true)) >= 0 && iConfigId != 0)
	{
		if(slog.GetSlogConfig(iConfigId) != NULL)
		{
			if(slog.Init() < 0)
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
	stConfig.psysConfig = slog.GetSystemCfg();
	if(stConfig.pShmConfig == NULL || stConfig.pAppInfo == NULL || stConfig.psysConfig == NULL) 
	{
		FATAL_LOG("get pShmConfig, psysConfig or pAppInfo failed !");
		return SLOG_ERROR_LINE;
	}

	if(iSysDaemon)
		stConfig.psysConfig->dwSystemFlag = set_bit(stConfig.psysConfig->dwSystemFlag, SYSTEM_FLAG_DAEMON);
	else
		stConfig.psysConfig->dwSystemFlag = clear_bit(stConfig.psysConfig->dwSystemFlag, SYSTEM_FLAG_DAEMON);

	if(strcmp(stConfig.psysConfig->szAgentAccessKey, stConfig.szAccessKey))
	{
		INFO_LOG("change agent access key to :%s", stConfig.szAccessKey);
		strncpy(stConfig.psysConfig->szAgentAccessKey, stConfig.szAccessKey, sizeof(stConfig.psysConfig->szAgentAccessKey));
	}

	if(stConfig.szLocalIp[0] == '\0' || INADDR_NONE == inet_addr(stConfig.szLocalIp))
		GetCustLocalIP(stConfig.szLocalIp);
	if(stConfig.szLocalIp[0] == '\0' || INADDR_NONE == inet_addr(stConfig.szLocalIp))
	{
		ERR_LOG("get local ip failed, use LOCAL_IP to set !");
		return SLOG_ERROR_LINE;
	}

	if(slog.InitAttrTypeList() < 0)
	{
		FATAL_LOG("init mt_attr_type shm failed !");
		return SLOG_ERROR_LINE;
	}

	if(slog.InitAttrList() < 0)
	{
		FATAL_LOG("init mt_attr shm failed !");
		return SLOG_ERROR_LINE;
	}

	if(slog.InitMachineList() < 0)
	{
		ERR_LOG("init machine list shm failed !");
		return SLOG_ERROR_LINE;
	}

	if(slog.InitwarnConfig() < 0)
	{
		ERR_LOG("init InitwarnConfig info shm failed !");
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

	if(slog.InitWarnInfo() < 0)
	{
		ERR_LOG("init InitwarnInfo shm failed !");
		return SLOG_ERROR_LINE;
	}

	if(slog.InitMtClientInfo() < 0)
	{
		ERR_LOG("init InitMtClientInfo shm failed !");
		return SLOG_ERROR_LINE;
	}

	if(GetShm2((void**)(&stConfig.pshmLoginList), iLoginShmKey, MYSIZEOF(FloginList), 0666|IPC_CREAT) < 0)
	{
		ERR_LOG("init pshmLoginList info shm failed , key:%d !", iLoginShmKey);
		return SLOG_ERROR_LINE;
	}

	if(stConfig.iNeedStrAttrShm) {
		stConfig.pStrAttrShm = slog.GetStrAttrNodeValShm(true);
		if(stConfig.pStrAttrShm == NULL) {
			ERR_LOG("GetStrAttrNodeValShm failed, key:%d, size:%u",
				STR_ATTR_NODE_VAL_SHM_DEF_KEY, MYSIZEOF(StrAttrNodeValShmInfo));
			return SLOG_ERROR_LINE;
		}
	}
	else
		stConfig.pStrAttrShm = slog.GetStrAttrNodeValShm(false);

	INFO_LOG("database info server:%s, user:%s, database:%s:%d local:%s, del monitor records:%d",
		stConfig.szDbHost, stConfig.szUserName, stConfig.szDbName, stConfig.iDbPort, stConfig.szLocalIp,
		stConfig.iDeleteMonitorRecords);
	stConfig.dwDbHostIp = inet_addr(stConfig.szDbHost);
	return 0;
}

static void RemoveAppInfo(int idx)
{
	int iAppId = stConfig.pAppInfo->stInfo[idx].iAppId;
	AppInfo * pAppInfo = stConfig.pAppInfo->stInfo+idx;

	AppInfo *pPrev = NULL, *pNext = NULL;
	if(pAppInfo->iPreIndex >= 0)
		pPrev = stConfig.pAppInfo->stInfo + pAppInfo->iPreIndex;
	if(pAppInfo->iNextIndex >= 0)
		pNext = stConfig.pAppInfo->stInfo + pAppInfo->iNextIndex;

	MtSystemConfig *psysConfig = stConfig.psysConfig;

	// 从全局配置 MtSystemConfig 中删除
	ILINK_DELETE_NODE(psysConfig, iAppInfoIndexStart, iAppInfoIndexEnd,
		pAppInfo, pPrev, pNext, iPreIndex, iNextIndex);
	psysConfig->wAppInfoCount--;
	INFO_LOG("delete app info from global config appid:%d - app count:%d",
		iAppId, psysConfig->wAppInfoCount);
	stConfig.pAppInfo->stInfo[idx].iAppId = 0;
	stConfig.pAppInfo->iAppCount--;
	INFO_LOG("delete appinfo appid:%d from shm", iAppId);
}

int32_t ReadTableAppInfo(Database &db, uint32_t up_id=0)
{
	char sSql[256];

	if(up_id != 0)
		snprintf(sSql, sizeof(sSql), "select * from mt_app_info where app_id=%u", up_id);
	else
		strcpy(sSql, "select * from mt_app_info");
	Query qu(db);
	if(!qu.get_result(sSql))
	{
		ERR_LOG("get appinfo failed (sql:%s)", sSql);
		return SLOG_ERROR_LINE;
	}

	int iMatch = 0, iFirstFree = -1, iStatus = 0, iAppId = 0;
	AppInfo *pAppInfo = NULL;
	SLogServer *pSrvInfo = NULL;
	const char *ptmp = NULL;

	uint32_t dwCurTime = time(NULL);
	uint32_t dwLastModTime = 0;
	Query qutmp(db);

	std::map<int,bool> mapAppValid;
	MtSystemConfig *psysConfig = stConfig.psysConfig;
	while(qu.fetch_row())
	{
		dwLastModTime = datetoui(qu.getstr("update_time"));
		iAppId = qu.getval("app_id");
		iStatus = qu.getval("xrk_status");
		iMatch = slog.GetAppInfo(iAppId, &iFirstFree);
		ptmp = qu.getstr("app_name");

		if(iMatch < 0 && iStatus == RECORD_STATUS_USE)
		{
			mapAppValid[iAppId] = true;

			// 插入共享内存
			if(iFirstFree < 0)
				ERR_LOG("need more space to save appinfo appid:%d", iAppId);
			else 
			{
				pAppInfo = stConfig.pAppInfo->stInfo+iFirstFree;
				memset(pAppInfo, 0, MYSIZEOF(*pAppInfo));
				pAppInfo->iAppId = iAppId;
				pAppInfo->dwLastModTime = dwLastModTime;
				pSrvInfo = slog.GetAppMasterSrv(iAppId);
				if(pSrvInfo == NULL) {
					ERR_LOG("get app master server failed, appid:%d", iAppId);
					pAppInfo->iLogSrvIndex = -1;
				}
				else
				{
					pAppInfo->iLogSrvIndex = slog.GetServerInfo(pSrvInfo->dwServiceId, NULL);
					pAppInfo->dwAppSrvMaster = pSrvInfo->dwIp;
					pAppInfo->wLogSrvPort = pSrvInfo->wPort;
					pAppInfo->dwAppSrvSeq = pSrvInfo->dwCfgSeq;
				}

				// iAppLogShmKey 由 key 探测算法设置, 这里初始化为0 - slog.GetAppLogShm
				pAppInfo->iAppLogShmKey = 0;
				if(ptmp != NULL && ptmp[0] != '\0')
					pAppInfo->iNameVmemIdx = MtReport_SaveToVmem(ptmp, strlen(ptmp)+1);
				else
					pAppInfo->iNameVmemIdx = -1;
				pAppInfo->wModuleCount = 0;
				pAppInfo->dwAppLogFlag = 0;
				pAppInfo->dwSeq = rand();
				stConfig.pAppInfo->iAppCount++;
				
				DEBUG_LOG("add appinfo appid:%d, log server index:%d, master srv:%s",
					iAppId, pAppInfo->iLogSrvIndex, ipv4_addr_str(pAppInfo->dwAppSrvMaster));

				// 加入全局配置 MtSystemConfig 中
				if(psysConfig->wAppInfoCount == 0) {
					ILINK_SET_FIRST(psysConfig, iAppInfoIndexStart, iAppInfoIndexEnd,
						pAppInfo, iPreIndex, iNextIndex, iFirstFree);
					psysConfig->wAppInfoCount = 1;
					INFO_LOG("add appinfo to global config - appid:%d, first app", pAppInfo->iAppId);
				}
				else {
					AppInfo *pAppInfoFirst = stConfig.pAppInfo->stInfo+psysConfig->iAppInfoIndexStart;
					ILINK_INSERT_FIRST(psysConfig, iAppInfoIndexStart, pAppInfo,
						iPreIndex, iNextIndex, pAppInfoFirst, iFirstFree);
					psysConfig->wAppInfoCount++;
					INFO_LOG("add app info to global config - appid:%d, appcount:%d",
						pAppInfo->iAppId, psysConfig->wAppInfoCount);
				}
			}
		}
		else if(iMatch >= 0 && iStatus == RECORD_STATUS_USE
			&& dwLastModTime != stConfig.pAppInfo->stInfo[iMatch].dwLastModTime) 
		{
			mapAppValid[iAppId] = true;

			// 更新数据
			pAppInfo = stConfig.pAppInfo->stInfo + iMatch;
			pAppInfo->dwLastModTime = dwLastModTime;
			pSrvInfo = slog.GetServerInfo(pAppInfo->iLogSrvIndex);
			if(pSrvInfo == NULL || pSrvInfo->dwServiceId == 0 || pSrvInfo->dwCfgSeq != pAppInfo->dwAppSrvSeq)
			{
				pSrvInfo = slog.GetAppMasterSrv(iAppId);
				if(pSrvInfo == NULL) {
					ERR_LOG("get app master server failed, appid:%d", iAppId);
					pAppInfo->iLogSrvIndex = -1;
				}
				else
				{
					pAppInfo->iLogSrvIndex = slog.GetServerInfo(pSrvInfo->dwServiceId, NULL);
					pAppInfo->dwAppSrvMaster = pSrvInfo->dwIp;
					pAppInfo->wLogSrvPort = pSrvInfo->wPort;
					pAppInfo->dwAppSrvSeq = pSrvInfo->dwCfgSeq;
					INFO_LOG("update app log server to:%s|%u, app:%d, servercfg seq:%u",
						pSrvInfo->szIpV4, pSrvInfo->dwServiceId, iAppId, pSrvInfo->dwCfgSeq);
				}
				pAppInfo->dwSeq++;
			}

			const char *pvname = NULL;
			if(pAppInfo->iNameVmemIdx > 0)
				pvname = MtReport_GetFromVmem_Local(pAppInfo->iNameVmemIdx);
			if(ptmp != NULL && (pvname == NULL || strcmp(pvname, ptmp)))
			{
				MtReport_FreeVmem(pAppInfo->iNameVmemIdx);
				pAppInfo->iNameVmemIdx = MtReport_SaveToVmem(ptmp, strlen(ptmp)+1);
			}

			DEBUG_LOG("app:%d app server seq:%u|%u, app server:%d|%s|%d", 
				iAppId, pAppInfo->dwAppSrvSeq, (pSrvInfo!=NULL ? pSrvInfo->dwCfgSeq : 0),
				pAppInfo->iLogSrvIndex, ipv4_addr_str(pAppInfo->dwAppSrvMaster), pAppInfo->wLogSrvPort);
		}
		else if(iMatch >= 0 && iStatus == RECORD_STATUS_USE)
		    mapAppValid[iAppId] = true;

		if(iMatch >= 0 && iStatus != RECORD_STATUS_USE)
		{  
			// 从共享内存中删除
			stConfig.pAppInfo->stInfo[iMatch].iAppId = 0;
			stConfig.pAppInfo->iAppCount--;
			pAppInfo = stConfig.pAppInfo->stInfo+iMatch;
			INFO_LOG("delete appinfo appid:%d from shm", iAppId);

			AppInfo *pPrev = NULL, *pNext = NULL;
			if(pAppInfo->iPreIndex >= 0)
				pPrev = stConfig.pAppInfo->stInfo + pAppInfo->iPreIndex;
			if(pAppInfo->iNextIndex >= 0)
				pNext = stConfig.pAppInfo->stInfo + pAppInfo->iNextIndex;

			// 从全局配置 MtSystemConfig 中删除
			ILINK_DELETE_NODE(psysConfig, iAppInfoIndexStart, iAppInfoIndexEnd,
				pAppInfo, pPrev, pNext, iPreIndex, iNextIndex);
			psysConfig->wAppInfoCount--;
			INFO_LOG("delete app info from global config appid:%d - app count:%d",
				iAppId, psysConfig->wAppInfoCount);
		}

		if(iStatus == RECORD_STATUS_DELETE)
		{
			// 其它相关数据也置为删除
			sprintf(sSql, "update mt_module_info set xrk_status=1 where xrk_status=0 and app_id=%d", iAppId);
			qutmp.execute(sSql);
			if(qutmp.affected_rows() > 0)
				INFO_LOG("set delete mt_module_info count:%d, app:%d", qutmp.affected_rows(), iAppId);

			sprintf(sSql, "update mt_log_config set xrk_status=1 where xrk_status=0 and app_id=%d", iAppId);
			if(qutmp.affected_rows() > 0)
				INFO_LOG("set delete mt_log_config count:%d, app:%d", qutmp.affected_rows(), iAppId);

			sprintf(sSql, "delete from mt_app_info where app_id=%d", iAppId);
			if(!stConfig.iDeleteDbRecord || dwCurTime < dwLastModTime+stConfig.iDelRecordTime)
			{
				// 跳过物理删除
				DEBUG_LOG("skip - %s, last update time:%u(%u)", sSql, dwLastModTime, dwCurTime);
			}
			else 
			{
				// 从数据库中删除
				if(!qutmp.execute(sSql)){
					ERR_LOG("execute sql:%s failed !", sSql);
					return SLOG_ERROR_LINE;
				}
				INFO_LOG("delete appinfo appid:%d from mysql", iAppId);
			}
		}
	}
	qu.free_result();

	// 从共享内存中删除已经被物理删除了的 app
	SLogAppInfo * pShmAppInfo =  stConfig.pAppInfo;
	for(int i=0, j=0; up_id == 0 && j < MAX_SLOG_APP_COUNT && i < pShmAppInfo->iAppCount; j++)
	{
		if(pShmAppInfo->stInfo[j].iAppId != 0) {
			i++;
			if(mapAppValid.find(pShmAppInfo->stInfo[j].iAppId) == mapAppValid.end())
			{
				INFO_LOG("find removed app, appid:%d", pShmAppInfo->stInfo[j].iAppId);
				RemoveAppInfo(j);
			}
		}
	}
	return 0;
}

// 读取表：module_info
int32_t ReadTableModuleInfo(Database &db, int *pCustModuleId=NULL, uint32_t uid=0)
{
	char sSql[256];

	if(pCustModuleId == NULL) {
		if(uid != 0)
			snprintf(sSql, sizeof(sSql), "select * from mt_module_info where module_id=%u", uid);
		else
			snprintf(sSql, sizeof(sSql), "select * from mt_module_info");
		stConfig.stModuleDelay.clear();
		stConfig.dwModuleDelaySetTime = time(NULL);
	}
	else {
		snprintf(sSql, sizeof(sSql), "select * from mt_module_info where module_id=%d", *pCustModuleId);
		DEBUG_LOG("reset delay module:%d", *pCustModuleId);
	}

	Query qu(db);
	if(!qu.get_result(sSql))
	{
		ERR_LOG("get slog module info failed (sql:%s)", sSql);
		return SLOG_ERROR_LINE;
	}

	int iMatch = 0, iMatchModule = 0, iFirstFreeModule = -1, iStatus = 0;
	int32_t iAppId = 0, iModuleId = 0;
	const char *ptmp = NULL;
	uint32_t dwCurTime = time(NULL);
	uint32_t dwLastModTime = 0;
	Query qutmp(db);
	
	while(qu.fetch_row())
	{
		dwLastModTime = datetoui(qu.getstr("mod_time"));
		iAppId = qu.getval("app_id");
		iStatus = qu.getval("xrk_status");
		ptmp = qu.getstr("module_name");
		iModuleId = qu.getval("module_id");

		iMatch = slog.GetAppInfo(iAppId, (int32_t*)NULL);
		iMatchModule = slog.GetAppModuleInfo(iMatch, iModuleId, &iFirstFreeModule);

		if(iStatus == RECORD_STATUS_USE && iMatch >= 0  && iMatchModule < 0) // 需要插入共享内存
		{
			if(iFirstFreeModule < 0){ // 空间不足, 存不下
				ERR_LOG("need more space to save module, appid:%d, module id:%d", iAppId, iModuleId);
			}else { // 存到共享内存
				stConfig.pAppInfo->stInfo[iMatch].arrModuleList[iFirstFreeModule].iModuleId = iModuleId;
				if(ptmp != NULL && ptmp[0] != '\0')
					stConfig.pAppInfo->stInfo[iMatch].arrModuleList[iFirstFreeModule].iNameVmemIdx = MtReport_SaveToVmem(ptmp, strlen(ptmp)+1);
				else
					stConfig.pAppInfo->stInfo[iMatch].arrModuleList[iFirstFreeModule].iNameVmemIdx = -1;

				stConfig.pAppInfo->stInfo[iMatch].wModuleCount++;
				stConfig.pAppInfo->stInfo[iMatch].dwSeq++;
				DEBUG_LOG("add module info appid:%d module id:%d save index:%d", iAppId, iModuleId, iFirstFreeModule);
			}
		}
		else if(iStatus == RECORD_STATUS_USE && iMatch >= 0  && iMatchModule >= 0) 
		{
			const char *pvname = NULL;
			if(stConfig.pAppInfo->stInfo[iMatch].arrModuleList[iMatchModule].iNameVmemIdx > 0)
				pvname = MtReport_GetFromVmem_Local(
					stConfig.pAppInfo->stInfo[iMatch].arrModuleList[iMatchModule].iNameVmemIdx);
			if(ptmp != NULL && (pvname == NULL || strcmp(pvname, ptmp)))
			{
				MtReport_FreeVmem(stConfig.pAppInfo->stInfo[iMatch].arrModuleList[iMatchModule].iNameVmemIdx);
				stConfig.pAppInfo->stInfo[iMatch].arrModuleList[iMatchModule].iNameVmemIdx
					= MtReport_SaveToVmem(ptmp, strlen(ptmp)+1);
				DEBUG_LOG("update module:%d name", iModuleId);
			}
		}

		// 读表顺序依赖问题 - orderfix
		if(iStatus == RECORD_STATUS_USE && iMatch < 0 && pCustModuleId == NULL) {
			stConfig.stModuleDelay.push_back(iModuleId);
			WARN_LOG("orderfix - not find app:%d, for module:%d", iAppId, iModuleId);
		}

		if(iStatus != RECORD_STATUS_USE && iMatch >= 0 && iMatchModule >= 0) // 需要从共享内存中删除
		{ 
			stConfig.pAppInfo->stInfo[iMatch].arrModuleList[iMatchModule].iModuleId = 0;
			MtReport_FreeVmem(stConfig.pAppInfo->stInfo[iMatch].arrModuleList[iMatchModule].iNameVmemIdx);
			stConfig.pAppInfo->stInfo[iMatch].arrModuleList[iMatchModule].iNameVmemIdx = -1;
			stConfig.pAppInfo->stInfo[iMatch].wModuleCount--;
			stConfig.pAppInfo->stInfo[iMatch].dwSeq--;
			INFO_LOG("delete module info appid:%d module id:%d from shm", iAppId, iModuleId); 
		}

		if(iStatus == RECORD_STATUS_DELETE)
		{
			sprintf(sSql, 
				"update mt_log_config set xrk_status=1 where xrk_status=0 and module_id=%d", iModuleId);
			qutmp.execute(sSql);
			if(qutmp.affected_rows() > 0)
				INFO_LOG("set delete mt_log_config count:%d, module:%d", qutmp.affected_rows(), iModuleId);

			sprintf(sSql, "delete from mt_module_info where app_id=%d and module_id=%d", iAppId, iModuleId);
			if(!stConfig.iDeleteDbRecord || dwCurTime < dwLastModTime+stConfig.iDelRecordTime)
			{
				DEBUG_LOG("skip - %s, last update time:%u(%u)", sSql, dwLastModTime, dwCurTime);
			}
			else
			{
				if(!qutmp.execute(sSql)){
					ERR_LOG("execute sql:%s failed !", sSql);
					return SLOG_ERROR_LINE;
				}

				// 删除配置
				sprintf(sSql, "update mt_log_config set xrk_status=%d where module_id=%d", RECORD_STATUS_DELETE, iModuleId);
				if(!qutmp.execute(sSql)){
					ERR_LOG("execute sql:%s failed !", sSql);
					return SLOG_ERROR_LINE;
				}
				INFO_LOG("delete module info appid:%d module id:%d from mysql", iAppId, iModuleId);
			}
		}
	}
	qu.free_result();

	if(iStatus == RECORD_STATUS_USE && iMatch < 0 && pCustModuleId == NULL) {
		return 1;
	}
	return 0;
}

int32_t ReadTableWarnConfig(Database &db, uint32_t rid=0)
{
	char sSql[256];

	if(rid != 0)
		sprintf(sSql, "select * from mt_warn_config where warn_config_id=%u", rid);
	else
		sprintf(sSql, "select * from mt_warn_config");
	Query qu(db);
	if(!qu.get_result(sSql))
	{
		ERR_LOG("get mt_warn_config info failed (sql:%s)", sSql);
		return SLOG_ERROR_LINE;
	}

	TWarnConfig *pConfig = NULL;
	int32_t iWarnId = 0, iAttrId = 0, id = 0;
	uint32_t dwIsFind = 0;
	int iStatus = 0;

	uint32_t dwLastModTime = 0;
	uint32_t dwCurTime = time(NULL);
	Query qutmp(db);
	SharedHashTable *pstHash = slog.GetWarnConfigInfoHash();

	while(qu.fetch_row())
	{
		dwLastModTime = datetoui(qu.getstr("update_time"));

		// warn_type_value -- 告警对象 id 视图或者机器id, 两个的范围不能重叠
		// 设置 视图id 起始值为：10000000
		iWarnId = qu.getval("warn_type_value");
		iAttrId = qu.getval("attr_id");
		iStatus = qu.getval("xrk_status");
		if(iStatus == RECORD_STATUS_USE)
			pConfig = slog.GetWarnConfigInfo(iWarnId, iAttrId, &dwIsFind);
		else
			pConfig = slog.GetWarnConfigInfo(iWarnId, iAttrId, NULL);
		id = qu.getval("warn_config_id");
		
		if(iStatus == RECORD_STATUS_USE && !dwIsFind)
		{
			if(pConfig == NULL)
				ERR_LOG("need more space to save warn config(%d,%d) info!", iWarnId, iAttrId);
			else {
				pConfig->iWarnId = iWarnId;
				pConfig->iAttrId = iAttrId;
				pConfig->iWarnMax = qu.getval("max_value");
				pConfig->iWarnMin = qu.getval("min_value");
				pConfig->iWarnWave = qu.getval("wave_value");
				pConfig->iWarnConfigFlag = qu.getval("warn_flag");
				pConfig->iWarnConfigId = id;
				pConfig->dwLastUpdateTime = dwLastModTime;
				INFO_LOG("add warn config info - %d|%d|%d", pConfig->iWarnConfigId, iWarnId, iAttrId);
			}
		}
		else if(iStatus == RECORD_STATUS_USE && dwIsFind && pConfig->dwLastUpdateTime != dwLastModTime)
		{
			INFO_LOG("update warn config info - %d|%d|%d|%d, new|%d|%d|%d|%d",
				pConfig->iWarnMax, pConfig->iWarnMin, pConfig->iWarnWave, pConfig->iWarnConfigFlag,
				qu.getval("max_value"), qu.getval("min_value"), qu.getval("wave_value"), qu.getval("warn_flag"));
			pConfig->iWarnMax = qu.getval("max_value");
			pConfig->iWarnMin = qu.getval("min_value");
			pConfig->iWarnWave = qu.getval("wave_value");
			pConfig->iWarnConfigFlag = qu.getval("warn_flag");
			pConfig->dwLastUpdateTime = dwLastModTime;
		}

		if(qu.getval("xrk_status") != RECORD_STATUS_USE)
		{
			if(pConfig) {
				memset(pConfig, 0, sizeof(*pConfig));
				RemoveHashNode(pstHash, pConfig);
			}
			sprintf(sSql, "delete from mt_warn_config where warn_config_id=%d", id);
			if(!stConfig.iDeleteDbRecord || dwCurTime < dwLastModTime+stConfig.iDelRecordTime)
			{
				DEBUG_LOG("skip - %s, last update time:%u(%u)", sSql, dwLastModTime, dwCurTime);
			}
			else
			{ 
				if(!qutmp.execute(sSql))
					ERR_LOG("execute sql:%s failed !", sSql);
				else
					INFO_LOG("delete warn config:%d from mysql", id);
			}
		}
	}
	qu.free_result();
	return 0;
}

int32_t ReadTableServer(Database &db, uint32_t uid=0)
{
	static char sSrvForBuf[12*PER_SERVER_FOR_XXX];
	static SLogServer stServerConfig;

	char sSql[256] = {0};

	if(uid == 0)
		snprintf(sSql, sizeof(sSql)-1, "select * from mt_server");
	else
		snprintf(sSql, sizeof(sSql)-1, "select * from mt_server where xrk_id=%u", uid);

	Query qu(db);
	if(!qu.get_result(sSql))
	{
		ERR_LOG("get server failed (sql:%s)", sSql);
		return SLOG_ERROR_LINE;
	}

	int iMatch = 0, iStatus = 0, iFirstFree = 0, iForCount = 0;
	const char *pIpV4 = NULL, *pSrvFor = NULL;
	char *pForStart = NULL, *pForSave = NULL, *pForVal = NULL;
	uint32_t dwCurTime = time(NULL);
	uint32_t dwLastModTime = 0;
	while(qu.fetch_row())
	{
		memset(&stServerConfig, 0, MYSIZEOF(stServerConfig));

#define INIT_SERVER_FOR_APP(srv_for) do { \
		strncpy(sSrvForBuf, pSrvFor, MYSIZEOF(sSrvForBuf)-1); \
		pForStart = sSrvForBuf; \
		pForSave = NULL; \
		for(iForCount=0; iForCount < PER_SERVER_FOR_XXX; pForStart=NULL) { \
			if((pForVal=strtok_r(pForStart, ",", &pForSave)) == NULL) \
				break; \
			srv_for.stForAppList.aiApp[iForCount] = atoi(pForVal); \
			iForCount++; \
		} \
		srv_for.stForAppList.wAppCount = iForCount; \
		qsort(srv_for.stForAppList.aiApp, iForCount, MYSIZEOF(int32_t), SrvForAppLogCmp); \
	}while(0)

		pIpV4 = qu.getstr("ip");
		stServerConfig.wType = qu.getval("xrk_type");
		strncpy(stServerConfig.szIpV4, pIpV4, MYSIZEOF(stServerConfig.szIpV4)-1);
		stServerConfig.dwIp = inet_addr(pIpV4);
		stServerConfig.bSandBox = qu.getval("sand_box");
		stServerConfig.bRegion = qu.getval("region");
		stServerConfig.bIdc = qu.getval("idc");
		stServerConfig.wPort = qu.getval("xrk_port");
		stServerConfig.iWeightConfig = qu.getuval("weight");
		stServerConfig.dwServiceId = qu.getval("xrk_id");
		stServerConfig.dwCfgSeq = qu.getuval("cfg_seq");

		dwLastModTime = datetoui(qu.getstr("update_time"));
		pSrvFor = qu.getstr("srv_for");
		iStatus = qu.getval("xrk_status");
		iMatch = slog.GetServerInfo(stServerConfig.dwServiceId, &iFirstFree);

		if(iStatus == RECORD_STATUS_USE)
		{
			// 插入/更新 共享内存
			if(iMatch < 0 && iFirstFree < 0)
				ERR_LOG("need more space to save server id:%u", stServerConfig.dwServiceId);

			else if(iMatch < 0 && iFirstFree >= 0) // 插入新配置
			{
				// 关联服务器需要处理的相关资源
				if(stServerConfig.wType == SRV_TYPE_APP_LOG)
					INIT_SERVER_FOR_APP(stServerConfig);

				// weight -- 初始化, iWeightCur 为0时，认为服务器不存活
				stServerConfig.iWeightCur = stServerConfig.iWeightConfig; 
				memcpy(stConfig.pShmConfig->stServerList+iFirstFree, &stServerConfig, MYSIZEOF(stServerConfig));
				INFO_LOG("add server id:%u, type:%d for count:%d, seq:%u, addr:%s:%d to index:%d",
					stServerConfig.dwServiceId, stServerConfig.wType, iForCount,
					stServerConfig.dwCfgSeq, pIpV4, stServerConfig.wPort, iFirstFree);
				stConfig.pShmConfig->wServerCount++;
			}
			else if(stConfig.bUpdateAll || (iMatch >= 0 // 尝试更新老配置 
				&& stConfig.pShmConfig->stServerList[iMatch].dwCfgSeq != stServerConfig.dwCfgSeq))
			{
				INFO_LOG("update server id:%d addr:%s:%d old seq:%u, new seq:%u",
						stServerConfig.dwServiceId, pIpV4, stServerConfig.wPort, 
						stConfig.pShmConfig->stServerList[iMatch].dwCfgSeq, stServerConfig.dwCfgSeq);

				SLogServer *pSrvInfo = stConfig.pShmConfig->stServerList+iMatch;
				if(stServerConfig.wType != pSrvInfo->wType)
				{
					ERR_LOG("bug server type changed - %d != %d, addr:%s:%d",
						stServerConfig.wType, pSrvInfo->wType, pIpV4, stServerConfig.wPort);
					break;
				}
				if(stServerConfig.wType == SRV_TYPE_APP_LOG)
					INIT_SERVER_FOR_APP(stServerConfig);

				// 使用当前的权重
				stServerConfig.iWeightCur = stConfig.pShmConfig->stServerList[iMatch].iWeightCur;
				memcpy(pSrvInfo, &stServerConfig, MYSIZEOF(stServerConfig));
			}
		}

		if(iMatch >= 0 && iStatus != RECORD_STATUS_USE)
		{   // 从共享内存中删除
			stConfig.pShmConfig->stServerList[iMatch].dwServiceId = 0;
			stConfig.pShmConfig->wServerCount--;
			INFO_LOG("delete server --- id:%d addr:%s:%d from shm",
				stServerConfig.dwServiceId, pIpV4, stServerConfig.wPort);
		}

		if(iStatus == RECORD_STATUS_DELETE)
		{  
			sprintf(sSql, "delete from mt_server where xrk_id=%u", stServerConfig.dwServiceId);

			// 执行物理删除必须要间隔一定时间 stConfig.iDelRecordTime
			if(!stConfig.iDeleteDbRecord || dwCurTime < dwLastModTime+stConfig.iDelRecordTime)
			{
				DEBUG_LOG("skip - %s, last update time:%u(%u)", sSql, dwLastModTime, dwCurTime);
			}
			else
			{
				Query qutmp(db);
				// 从数据库中删除
				if(!qutmp.execute(sSql)){
					ERR_LOG("execute sql:%s failed !", sSql);
					return SLOG_ERROR_LINE;
				}
				INFO_LOG("delete server id:%u from mysql", stServerConfig.dwServiceId);
			}
		}
	}
	qu.free_result();
	return 0;
}

int32_t ReadTableConfigInfo(Database &db, uint32_t *pCustCfgId=NULL, uint32_t rid=0)
{
	char sSql[256];

	if(pCustCfgId == NULL) {
		if(rid != 0)
			snprintf(sSql, sizeof(sSql)-1, "select * from mt_log_config where config_id=%u", rid);
		else
			strcpy(sSql, "select * from mt_log_config");
	}
	else {
		snprintf(sSql, sizeof(sSql), "select * from mt_log_config where config_id=%u", *pCustCfgId);
		DEBUG_LOG("orderfix - delay log config:%u", *pCustCfgId);
	}

	Query qu(db);
	if(!qu.get_result(sSql))
	{
		ERR_LOG("get config failed (sql:%s)", sSql);
		return SLOG_ERROR_LINE;
	}

	int iMatchApp = 0, iMatchModule = 0, iMatch = 0, iStatus = 0;
	int iFirstFreeConfig = 0;

	uint32_t dwCurTime = time(NULL);
	uint32_t dwLastModTime = 0;
	Query qutmp(db);

	SLogClientConfig stLogConfig;
	memset(&stLogConfig, 0, MYSIZEOF(stLogConfig));
	MtSystemConfig *psysConfig = &(stConfig.pShmConfig->stSysCfg);
	while(qu.fetch_row())
	{
		dwLastModTime = datetoui(qu.getstr("update_time"));
		stLogConfig.dwCfgId = qu.getuval("config_id");
		stLogConfig.iAppId = qu.getval("app_id");
		stLogConfig.iModuleId = qu.getval("module_id");
		stLogConfig.iLogType = qu.getval("log_types");
		stLogConfig.dwSpeedFreq = qu.getval("write_speed");
		stLogConfig.wTestKeyCount = 0;

		iStatus = qu.getval("xrk_status");
		iMatchApp = slog.GetAppInfo(stLogConfig.iAppId, (int32_t*)NULL);
		iMatchModule = slog.GetAppModuleInfo(iMatchApp, stLogConfig.iModuleId, NULL);
		iMatch = slog.GetSlogConfig(stLogConfig.dwCfgId, &iFirstFreeConfig);

		if(iStatus == RECORD_STATUS_USE && iMatchApp >= 0 && iMatchModule >= 0) 
		{ 
			// 插入/更新 共享内存
			if(iMatch < 0 && iFirstFreeConfig < 0)
				ERR_LOG("need more space to save config id:%u", stLogConfig.dwCfgId);
			else if(iMatch < 0 && iFirstFreeConfig >= 0)
			{   // 插入配置
				stLogConfig.dwConfigSeq = rand();
				memcpy(stConfig.pShmConfig->stConfig+iFirstFreeConfig, &stLogConfig, MYSIZEOF(stLogConfig));

				SLogClientConfig *pLogConfig = stConfig.pShmConfig->stConfig+iFirstFreeConfig;
				if(psysConfig->wLogConfigCount == 0) {
					ILINK_SET_FIRST(psysConfig, iLogConfigIndexStart, iLogConfigIndexEnd,
						pLogConfig, iPreIndex, iNextIndex, iFirstFreeConfig);
					psysConfig->wLogConfigCount = 1;
					INFO_LOG("add log config info to shm - config id:%u, first config", stLogConfig.dwCfgId);
				}
				else {
					SLogClientConfig *pInfoFirst = stConfig.pShmConfig->stConfig + psysConfig->iLogConfigIndexStart;
					ILINK_INSERT_FIRST(psysConfig, iLogConfigIndexStart, pLogConfig,
						iPreIndex, iNextIndex, pInfoFirst, iFirstFreeConfig);
					psysConfig->wLogConfigCount++;
					INFO_LOG("add log config info to shm - config id:%u, config count:%u",
						stLogConfig.dwCfgId, psysConfig->wLogConfigCount);
				}
				
				stConfig.pShmConfig->dwSLogConfigCount++;
			}
			else if(iMatch >= 0)
			{
				bool bChanged = false;
				if(stConfig.pShmConfig->stConfig[iMatch].iAppId != stLogConfig.iAppId
					|| stConfig.pShmConfig->stConfig[iMatch].iModuleId != stLogConfig.iModuleId) 
				{
					// app/module 不允许修改, 除非直接修改了数据库
					WARN_LOG("log config app/module changed - %d|%d, %d|%d",
						stConfig.pShmConfig->stConfig[iMatch].iAppId, stLogConfig.iAppId,
						stConfig.pShmConfig->stConfig[iMatch].iModuleId, stLogConfig.iModuleId);

					stConfig.pShmConfig->stConfig[iMatch].iAppId = stLogConfig.iAppId;
					stConfig.pShmConfig->stConfig[iMatch].iModuleId = stLogConfig.iModuleId;
					bChanged = true;
				}

				if(stConfig.pShmConfig->stConfig[iMatch].dwSpeedFreq != stLogConfig.dwSpeedFreq) {
					stConfig.pShmConfig->stConfig[iMatch].dwSpeedFreq = stLogConfig.dwSpeedFreq;
					bChanged = true;
				}

				if(stConfig.pShmConfig->stConfig[iMatch].iLogType != stLogConfig.iLogType) {
					stConfig.pShmConfig->stConfig[iMatch].iLogType = stLogConfig.iLogType;
					bChanged = true;
				}
				if(bChanged)
					stConfig.pShmConfig->stConfig[iMatch].dwConfigSeq++;
			}
		}
		else if(iStatus == RECORD_STATUS_USE && (iMatchApp < 0 || iMatchModule < 0))
		{
			WARN_LOG("config id:%d match app:%d(%d) or module:%d(%d) failed !",
				stLogConfig.dwCfgId, stLogConfig.iAppId, iMatchApp, stLogConfig.iModuleId, iMatchModule);
		}

		if(iMatch >= 0 && iStatus != RECORD_STATUS_USE)
		{   
			// 从共享内存中删除
			stConfig.pShmConfig->stConfig[iMatch].dwCfgId = 0;
			stConfig.pShmConfig->dwSLogConfigCount--;
			INFO_LOG("delete config id:%d from shm", stLogConfig.dwCfgId);

			SLogClientConfig *pLogConfig = stConfig.pShmConfig->stConfig+iMatch;
			SLogClientConfig *pPrev = NULL, *pNext = NULL;
			if(pLogConfig->iPreIndex >= 0)
				pPrev = stConfig.pShmConfig->stConfig+pLogConfig->iPreIndex;
			if(pLogConfig->iNextIndex >= 0)
				pNext = stConfig.pShmConfig->stConfig+pLogConfig->iNextIndex;

			// 从全局配置中删除
			ILINK_DELETE_NODE(psysConfig, iLogConfigIndexStart, iLogConfigIndexEnd,
				pLogConfig, pPrev, pNext, iPreIndex, iNextIndex);
			psysConfig->wLogConfigCount--;
			INFO_LOG("delete log config from shm, config id:%u, config count:%u",
				pLogConfig->dwCfgId, psysConfig->wLogConfigCount);
			
		}

		if(iStatus == RECORD_STATUS_DELETE)
		{
			sprintf(sSql, "delete from mt_log_config where config_id=%u", stLogConfig.dwCfgId);
			if(!stConfig.iDeleteDbRecord || dwCurTime < dwLastModTime+stConfig.iDelRecordTime)
			{
				DEBUG_LOG("skip - %s, last update time:%u(%u)", sSql, dwLastModTime, dwCurTime);
			}
			else
			{
				// 从数据库中删除
				if(!qutmp.execute(sSql)){
					ERR_LOG("execute sql:%s failed !", sSql);
					return SLOG_ERROR_LINE;
				}
				INFO_LOG("delete config id:%u from mysql match(app:%d, module:%d)", 
					stLogConfig.dwCfgId, iMatchApp, iMatchModule);
			}
		}
	}
	qu.free_result();
	return 0;
}

int SetUserSessionInfo(FloginInfo *psess, user::UserSessionInfo & user)
{
    int *piLen = (int*)psess->sReserved;
    char *pbuf = (char*)(psess->sReserved+4);
    std::string strval;
    if(!user.AppendToString(&strval))
    {    
        ERR_LOG("user session append failed!");
        MtReport_Attr_Add(226, 1);
        return SLOG_ERROR_LINE;
    }    

    if(strval.size()+sizeof(int) > sizeof(psess->sReserved))
    {    
        ERR_LOG("user session need more space %lu > %lu", strval.size()+sizeof(int), sizeof(psess->sReserved));
        MtReport_Attr_Add(226, 1);
        return SLOG_ERROR_LINE;
    }    

    if(strval.size()+sizeof(int)+50 > sizeof(psess->sReserved))
    {    
        INFO_LOG("user session bp buffer will full %lu > %lu", strval.size()+sizeof(int), sizeof(psess->sReserved));
        MtReport_Attr_Add(250, 1);
    }    

    *piLen = (int)strval.size();
    memcpy(pbuf, strval.c_str(), strval.size());
    DEBUG_LOG("user sess set ok, len:%d, info:%s", *piLen, user.ShortDebugString().c_str());
    return 0;
}

int32_t ReadTableFloginUser(Database &db, uint32_t uid=0)
{
	char sSql[256] = {0};

	if(uid == 0)
		snprintf(sSql, sizeof(sSql)-1, "select * from flogin_user");
	else
		snprintf(sSql, sizeof(sSql)-1, "select  * from flogin_user where user_id=%u", uid);

	Query qu(db);
	if(!qu.get_result(sSql))
		return SLOG_ERROR_LINE;

	uint32_t dwLoginTime = 0;
	int iLoginIdx = 0, iUserId = 0, iStatus = 0;
	const char *pmd5 = NULL;
	uint32_t dwCurTime = time(NULL);
	while(qu.fetch_row())
	{
		pmd5 = qu.getstr("login_md5");
		dwLoginTime = qu.getuval("last_login_time");
		iLoginIdx = qu.getval("login_index");
		iUserId = qu.getval("user_id");
		iStatus = qu.getval("xrk_status");
	
		if(iLoginIdx < 0 || iLoginIdx >= FLOGIN_SESSION_NODE_COUNT
			|| dwCurTime >= LOGIN_MAX_EXPIRE_TIME+dwLoginTime
			|| pmd5 == NULL || pmd5[0] == '\0')
		{
			continue;
		}

		// 单处登陆限制 -- 多处登陆时某些登陆点的日志系统跨域会有问题
		for(int i=0; i < FLOGIN_SESSION_NODE_COUNT; i++)
		{
			if(stConfig.pshmLoginList->stLoginList[i].iUserId==iUserId && iLoginIdx!=i) 
			{
				INFO_LOG("kick user:%d, login idx:%d, login ip:%s", 
					iUserId, i, ipv4_addr_str(stConfig.pshmLoginList->stLoginList[i].dwLoginIP));
				memset(stConfig.pshmLoginList->stLoginList+i, 0, sizeof(FloginInfo));
			}
			else if(stConfig.pshmLoginList->stLoginList[i].iUserId == iUserId) 
			{
				if(iStatus != RECORD_STATUS_USE) {
					// 用户已删除了，清掉 login cookie
					INFO_LOG("del user:%d from shm", iUserId);
					memset(stConfig.pshmLoginList->stLoginList+i, 0, sizeof(FloginInfo));
				}
				else 
				{
					// 看下资料是否有更新
					const char *ptmp = NULL;
					ptmp = qu.getstr("user_name");
					if(ptmp!=NULL && strcmp(stConfig.pshmLoginList->stLoginList[i].szUserName, ptmp))
					{
						DEBUG_LOG("update user name:%s - %s", ptmp, stConfig.pshmLoginList->stLoginList[i].szUserName);
						strncpy(stConfig.pshmLoginList->stLoginList[i].szUserName, 
							ptmp, sizeof(stConfig.pshmLoginList->stLoginList[i].szUserName)-1);
					}

					if(stConfig.pshmLoginList->stLoginList[i].dwUserFlag_1 != qu.getuval("user_flag_1"))
					{
						DEBUG_LOG("update user flag1:%u - %u", stConfig.pshmLoginList->stLoginList[i].dwUserFlag_1, qu.getuval("user_flag_1"));
						stConfig.pshmLoginList->stLoginList[i].dwUserFlag_1 = qu.getuval("user_flag_1");
					}

					user::UserSessionInfo sess;
					bool bUpSess = false;
					int *piLen = (int*)stConfig.pshmLoginList->stLoginList[i].sReserved;
					char *pbuf = (char*)(stConfig.pshmLoginList->stLoginList[i].sReserved+4);
					if(*piLen > 0 && !sess.ParseFromArray(pbuf, *piLen))
					{       
						WARN_LOG("ParseFromArray failed, len:%d", *piLen);
						continue;
					}
					ptmp = qu.getstr("email");
					if(ptmp != NULL && strcmp(ptmp, sess.email().c_str())) {
						DEBUG_LOG("user email changed from:%s to:%s", sess.email().c_str(), ptmp);
						sess.set_email(ptmp);
						bUpSess = true;
					}
					else if((ptmp == NULL || ptmp[0] == '\0') && sess.has_email())
					{
						DEBUG_LOG("user email changed to empty, uid:%d", iUserId);
						bUpSess = true;
						sess.clear_email();
					}

					if(bUpSess)
						SetUserSessionInfo(stConfig.pshmLoginList->stLoginList+i, sess);
				}
			}
		}
	}
	qu.free_result();
	return 0;
}

typedef struct {
	uint32_t cfg_id;
	char szKeyName[128];
}TtestkeyPrimInfo;

int32_t ReadTableTestKeyInfo(Database &db, uint32_t up_id=0)
{
	char sSql[256];

	if(up_id != 0)
		sprintf(sSql, "select * from test_key_list where xrk_id=%u", up_id);
	else
		sprintf(sSql, "select * from test_key_list");
	Query qu(db);
	if(!qu.get_result(sSql))
	{
		ERR_LOG("get test key info failed (sql:%s)", sSql);
		return SLOG_ERROR_LINE;
	}

	int iMatchConfig = 0, iMatchKey = 0, iFirstFree = -1, iStatus = 0;
	uint32_t dwCfgId = 0, dwTestKeyId = 0;
	uint8_t bKeyType = 0;
	const char *pszKeyValue = NULL;

	uint32_t dwCurTime = time(NULL);
	uint32_t dwLastModTime = 0;
	Query qutmp(db);

	while(qu.fetch_row())
	{
		dwLastModTime = datetoui(qu.getstr("update_time"));
		dwTestKeyId = qu.getuval("xrk_id");
		bKeyType = qu.getval("test_key_type");
		pszKeyValue = qu.getstr("test_key");
		dwCfgId = qu.getuval("config_id");
		iStatus = qu.getval("xrk_status");
		iMatchConfig = slog.GetSlogConfig(dwCfgId, NULL);
		iMatchKey = slog.GetTestKey(iMatchConfig, bKeyType, pszKeyValue, &iFirstFree);
		if(iStatus == RECORD_STATUS_USE && iMatchConfig >= 0 && iMatchKey < 0)
		{   // 插入共享内存
			if(iFirstFree < 0)
				ERR_LOG("need more space to save test key type:%d value:%s", bKeyType, pszKeyValue);
			else
			{
				stConfig.pShmConfig->stConfig[iMatchConfig].stTestKeys[iFirstFree].bKeyType = bKeyType;
				strncpy(stConfig.pShmConfig->stConfig[iMatchConfig].stTestKeys[iFirstFree].szKeyValue, 
					pszKeyValue, SLOG_TEST_KEY_LEN-1);
				stConfig.pShmConfig->stConfig[iMatchConfig].wTestKeyCount++;

				// fix bug
				stConfig.pShmConfig->stConfig[iMatchConfig].dwConfigSeq++;
				INFO_LOG("add test key type:%d value:%s for config:%u, count:%d", 
					bKeyType, pszKeyValue, dwCfgId, stConfig.pShmConfig->stConfig[iMatchConfig].wTestKeyCount);
			}
		}
		else if(iStatus == RECORD_STATUS_USE && iMatchConfig >= 0 && iMatchKey >= 0){
			// 更新配置，目前没有需要更新的字段
		}

		if(iMatchConfig >= 0 && iMatchKey >= 0 && iStatus != RECORD_STATUS_USE)
		{   
			// 从共享内存中删除, 有数据的节点凑一起，不留中间空洞
			stConfig.pShmConfig->stConfig[iMatchConfig].stTestKeys[iMatchKey].bKeyType = 0;
			stConfig.pShmConfig->stConfig[iMatchConfig].stTestKeys[iMatchKey].szKeyValue[0] = '\0';
			for(int i=iMatchKey; i < stConfig.pShmConfig->stConfig[iMatchConfig].wTestKeyCount-1; i++)
			{
				memcpy(stConfig.pShmConfig->stConfig[iMatchConfig].stTestKeys+i,
					stConfig.pShmConfig->stConfig[iMatchConfig].stTestKeys+i+1, sizeof(SLogTestKey));
			}
			stConfig.pShmConfig->stConfig[iMatchConfig].wTestKeyCount--;
			stConfig.pShmConfig->stConfig[iMatchConfig].dwConfigSeq++;
			INFO_LOG("delete test key type:%d value:%s for config id:%u, count:%d -- shm", 
				bKeyType, pszKeyValue, dwCfgId, stConfig.pShmConfig->stConfig[iMatchConfig].wTestKeyCount);
		}

		if(iStatus == RECORD_STATUS_DELETE)
		{
			// 从数据库中删除
			sprintf(sSql, "delete from test_key_list where xrk_id=%u", dwTestKeyId); 
			if(!stConfig.iDeleteDbRecord || dwCurTime < dwLastModTime+stConfig.iDelRecordTime)
			{
				DEBUG_LOG("skip - %s, last update time:%u(%u)", sSql, dwLastModTime, dwCurTime);
			}
			else
			{
				if(!qutmp.execute(sSql)){
					ERR_LOG("execute sql:%s failed !", sSql);
					return SLOG_ERROR_LINE;
				}
				INFO_LOG("delete test key for config:%u key type:%d value:%s -- mysql", 
						dwCfgId, bKeyType, pszKeyValue);
			}
		}
	}
	qu.free_result();
	return 0;
}

int32_t ReadTableWarnInfo(Database &db, uint32_t uid=0)
{
	char sSql[256] = {0};
	if(uid == 0)
		snprintf(sSql, sizeof(sSql)-1, "select * from mt_warn_info order by last_warn_time_utc desc");
	else
		snprintf(sSql, sizeof(sSql)-1, "select * from mt_warn_info where wid=%u", uid);

	Query qu(db);
	if(!qu.get_result(sSql))
	{
		ERR_LOG("get warn info failed (sql:%s)", sSql);
		return SLOG_ERROR_LINE;
	}

	int iStatus = 0, iWarnId = 0, iHashIndex = 0;
	uint32_t dwIsFind = 0;
	TWarnInfo *pInfo = NULL;
	SharedHashTableNoList *pWarnHash = slog.GetWarnInfoHash();
	MtSystemConfig *psysConfig = stConfig.psysConfig;

	uint32_t dwLastModTime = 0;
	uint32_t dwCurTime = time(NULL);
	Query qutmp(db);
	
	while(qu.fetch_row())
	{
		dwLastModTime = datetoui(qu.getstr("update_time"));
		iWarnId = qu.getval("wid");
		iStatus = qu.getval("xrk_status");
		pInfo = slog.GetWarnInfo(iWarnId, &dwIsFind);
		if(iStatus == RECORD_STATUS_USE && !dwIsFind)
		{   // 插入共享内存
			if(pInfo == NULL) {
				ERR_LOG("need more space to save warn info , warn id:%d", iWarnId);
			}
			else
			{
				// 看是否要存入 shm 中
				if((psysConfig->bLastWarnCount >= MAX_WARN_INFO_IN_SHM_PER_USER && uid != 0)
					|| psysConfig->bLastWarnCount < MAX_WARN_INFO_IN_SHM_PER_USER)
				{
					// shm 节点满，移除最老的, 即尾节点
					if(psysConfig->bLastWarnCount >= MAX_WARN_INFO_IN_SHM_PER_USER) 
					{
						TWarnInfo *pPrev = NULL, *pNext = NULL, *pInfoLast = NULL;
						pInfoLast = (TWarnInfo*)NOLIST_HASH_INDEX_TO_NODE(pWarnHash, psysConfig->iWarnIndexEnd);
						if(pInfoLast->iPreIndex >= 0)
							pPrev = (TWarnInfo*)NOLIST_HASH_INDEX_TO_NODE(pWarnHash, pInfoLast->iPreIndex);
						ILINK_DELETE_NODE(psysConfig, 
							iWarnIndexStart, iWarnIndexEnd, pInfoLast, pPrev, pNext, iPreIndex, iNextIndex);
						psysConfig->bLastWarnCount--;
						INFO_LOG("remove last warn info, warn id:%d", pInfoLast->id);

						// 从 shm 中释放掉
						pInfoLast->id = 0;
					}

					iHashIndex = NOLIST_HASH_NODE_TO_INDEX(pWarnHash, pInfo);
					if(psysConfig->bLastWarnCount == 0 || psysConfig->iWarnIndexStart < 0) {
						ILINK_SET_FIRST(psysConfig, iWarnIndexStart,
							iWarnIndexEnd, pInfo, iPreIndex, iNextIndex, iHashIndex);
						INFO_LOG("add warn info to shm, warn id:%u - first warn", iWarnId);
						psysConfig->bLastWarnCount = 1;
					}
					else if(uid == 0) {
						// 这里插入到最后
						TWarnInfo *pWarnInfoLast = (TWarnInfo*)NOLIST_HASH_INDEX_TO_NODE(pWarnHash, psysConfig->iWarnIndexEnd);
						ILINK_INSERT_LAST(psysConfig, iWarnIndexEnd, pInfo, iPreIndex, iNextIndex, pWarnInfoLast, iHashIndex);
						psysConfig->bLastWarnCount++;
						INFO_LOG("(back) add warn info to shm, warn id:%u - warn count:%d", iWarnId, psysConfig->bLastWarnCount);
					}
					else {
						// 这里插入到最前
						TWarnInfo *pWarnInfoFirst = (TWarnInfo*)NOLIST_HASH_INDEX_TO_NODE(pWarnHash, psysConfig->iWarnIndexStart);
						ILINK_INSERT_FIRST(psysConfig, iWarnIndexStart, pInfo, iPreIndex, iNextIndex, pWarnInfoFirst, iHashIndex);
						psysConfig->bLastWarnCount++;
						INFO_LOG("(front) add warn info to shm, warn id:%u - warn count:%d", iWarnId, psysConfig->bLastWarnCount);
					}

					// 存入 shm 中，并设置数据
					pInfo->id = iWarnId;
					pInfo->iWarnTypeId = qu.getval("warn_id");
					pInfo->iAttrId = qu.getval("attr_id");
					pInfo->dwWarnTime = qu.getuval("warn_time_utc");
					pInfo->dwWarnFlag = qu.getuval("warn_flag");
					pInfo->dwWarnVal = qu.getuval("warn_val");
					pInfo->dwWarnConfVal = qu.getuval("warn_config_val");
					pInfo->iWarnTimes = qu.getval("warn_times");
					pInfo->iDealStatus = qu.getval("deal_status"); 
					pInfo->dwLastWarnTime = qu.getuval("last_warn_time_utc");
					pInfo->dwLastUpdateTime = dwLastModTime;
				}
			}
		}
		else if(iStatus == RECORD_STATUS_USE && dwIsFind && pInfo->dwLastUpdateTime != dwLastModTime)
		{
			pInfo->dwLastUpdateTime = dwLastModTime;
			bool bUpdate = false;
			if(pInfo->iWarnTimes != qu.getval("warn_times")) {
				pInfo->iWarnTimes = qu.getval("warn_times");
				bUpdate = true;
			}

			// 时间变了，在 shm 链表中的位置也会发生变化，需要处理
			if(pInfo->dwLastWarnTime != qu.getuval("last_warn_time_utc")) 
			{
				pInfo->dwLastWarnTime = qu.getuval("last_warn_time_utc");
				bUpdate = true;

				// 按最近告警时间降序排列，所以需要换到最前面 
				iHashIndex = NOLIST_HASH_NODE_TO_INDEX(pWarnHash, pInfo);
				if(psysConfig->iWarnIndexStart != iHashIndex) 
				{
					// 先从链表中断开
					psysConfig->bLastWarnCount--;
					TWarnInfo *pPrev = NULL, *pNext = NULL;
					if(pInfo->iPreIndex >= 0)
						pPrev = (TWarnInfo*)NOLIST_HASH_INDEX_TO_NODE(pWarnHash, pInfo->iPreIndex);
					if(pInfo->iNextIndex >= 0)
						pNext = (TWarnInfo*)NOLIST_HASH_INDEX_TO_NODE(pWarnHash, pInfo->iNextIndex);
					ILINK_DELETE_NODE(psysConfig, iWarnIndexStart, iWarnIndexEnd, pInfo, pPrev, pNext, iPreIndex, iNextIndex);
					if(uid > 0) { 
						// 插入到最前
						TWarnInfo *pWarnInfoFirst = (TWarnInfo*)NOLIST_HASH_INDEX_TO_NODE(pWarnHash, psysConfig->iWarnIndexStart);
						ILINK_INSERT_FIRST(psysConfig, iWarnIndexStart, pInfo, iPreIndex, iNextIndex, pWarnInfoFirst, iHashIndex);
						psysConfig->bLastWarnCount++;
					}
					else {
						// 插入到最后
						TWarnInfo *pWarnInfoLast = (TWarnInfo*)NOLIST_HASH_INDEX_TO_NODE(pWarnHash, psysConfig->iWarnIndexEnd);
						ILINK_INSERT_LAST(psysConfig, iWarnIndexEnd, pInfo, iPreIndex, iNextIndex, pWarnInfoLast, iHashIndex);
						psysConfig->bLastWarnCount++;
					}
				}
			}
			if(pInfo->iDealStatus != qu.getval("deal_status")) {
				pInfo->iDealStatus = qu.getval("deal_status");
				bUpdate = true;
			}
			if(pInfo->dwWarnVal != qu.getuval("warn_val")) {
				pInfo->dwWarnVal = qu.getuval("warn_val");
				bUpdate = true;
			}
			if(bUpdate) {
				INFO_LOG("update warn info id:%d, iWarnTimes:%d, dwLastWarnTime:%s, iDealStatus:%d, val:%u",
					iWarnId, pInfo->iWarnTimes, uitodate(pInfo->dwLastWarnTime), pInfo->iDealStatus, pInfo->dwWarnVal);
			}
		}

		if(dwIsFind && iStatus != RECORD_STATUS_USE)
		{  
			// 从共享内存中删除
			INFO_LOG("delete warn info id:%u", pInfo->id);
			psysConfig->bLastWarnCount--;
			TWarnInfo *pPrev = NULL, *pNext = NULL;
			if(pInfo->iPreIndex >= 0)
				pPrev = (TWarnInfo*)NOLIST_HASH_INDEX_TO_NODE(pWarnHash, pInfo->iPreIndex);
			if(pInfo->iNextIndex >= 0)
				pNext = (TWarnInfo*)NOLIST_HASH_INDEX_TO_NODE(pWarnHash, pInfo->iNextIndex);
			ILINK_DELETE_NODE(psysConfig, iWarnIndexStart, iWarnIndexEnd, pInfo, pPrev, pNext, iPreIndex, iNextIndex);

			// 从 shm 中释放掉
			pInfo->id = 0;
		}

		if(iStatus == RECORD_STATUS_DELETE)
		{
			sprintf(sSql, "delete from mt_warn_info where wid=%u", iWarnId);  
			if(!stConfig.iDeleteDbRecord || dwCurTime < dwLastModTime+stConfig.iDelRecordTime)
			{
				DEBUG_LOG("skip - %s, last update time:%u(%u)", sSql, dwLastModTime, dwCurTime);
			}
			else
			{
				// 从数据库中删除
				if(!qutmp.execute(sSql)){
					ERR_LOG("execute sql:%s failed !", sSql);
					return SLOG_ERROR_LINE;
				}
				INFO_LOG("delete warn info id:%d -- mysql", iWarnId); 
			}
		}
	}
	INFO_LOG("get warn info count:%u", qu.num_rows());
	qu.free_result();
	return 0;
}

// 删除字符串型监控点上报的字符串数据，可能出现同 slog_monitor_server 操作冲突问题
// 可能导致部分 StrAttrInfo 节点未释放掉，影响不大
void ClearStrAttrNodeVal(int idx)
{
	if(stConfig.pStrAttrShm == NULL)
		return;
	while(idx >= 0 && idx < MAX_STR_ATTR_ARRY_NODE_VAL_COUNT) {
		stConfig.pStrAttrShm->stInfo[idx].iStrVal = -1;
		idx = stConfig.pStrAttrShm->stInfo[idx].iNextStrAttr;
		stConfig.pStrAttrShm->stInfo[idx].iNextStrAttr = -1;
	}
}

int32_t ReadTableAttr(Database &db, uint32_t uid=0)
{
	char sSql[256] = {0};
	if(uid == 0)
		snprintf(sSql, sizeof(sSql)-1, "select * from mt_attr");
	else
		snprintf(sSql, sizeof(sSql)-1, "select * from mt_attr where xrk_id=%u", uid);

	Query qutmp(db);
	Query qu(db);
	if(!qu.get_result(sSql))
	{
		ERR_LOG("get attr failed (sql:%s)", sSql);
		return SLOG_ERROR_LINE;
	}

	int iStatus = 0, iAttrId = 0, iHashIndex = 0;
	uint32_t dwIsFind = 0;

	uint32_t dwCurTime = time(NULL);
	uint32_t dwLastModTime = 0;

	AttrInfoBin *pInfo = NULL;
	SharedHashTableNoList *pAttrHash = slog.GetAttrHash();
	AttrTypeInfo *pAttrType = NULL;
	const char *ptmp = NULL;
	int iAttrType = 0, iStaticTime = 0;
	MtSystemConfig *psysConfig = stConfig.psysConfig;
	SharedHashTable *pstHash = slog.GetWarnConfigInfoHash();

	while(qu.fetch_row())
	{
		iAttrId = qu.getval("xrk_id");
		iStatus = qu.getval("xrk_status");
		ptmp = qu.getstr("attr_name");
		dwLastModTime = datetoui(qu.getstr("update_time"));
		iAttrType = qu.getval("attr_type");
		pAttrType = slog.GetAttrTypeInfo(iAttrType, NULL);
		iStaticTime = qu.getval("static_time");
		if(iStatus == RECORD_STATUS_USE && pAttrType == NULL)
		{
			WARN_LOG("attr:%d is delete, type:%d", iAttrId, iAttrType);
			iStatus = RECORD_STATUS_DELETE;
			snprintf(sSql, sizeof(sSql), "update mt_attr set xrk_status=%d where xrk_id=%d", 
				RECORD_STATUS_DELETE, iAttrId);
			qutmp.execute(sSql);
		}

		// 将异常属性保存到告警配置哈希中 ----------
		if(qu.getval("data_type") == EX_REPORT)
		{
			uint32_t t_dwIsFind = 0;
			TWarnConfig *pConfig = NULL;
			if(iStatus == RECORD_STATUS_USE)
				pConfig = slog.GetWarnConfigInfo(0, iAttrId, &t_dwIsFind);
			else
				pConfig = slog.GetWarnConfigInfo(0, iAttrId, NULL);

			if(iStatus == RECORD_STATUS_USE && !t_dwIsFind)
			{
				if(pConfig == NULL) {
					ERR_LOG("need more space to save attr exception warn config(0,%d) info!", iAttrId);
				}
				else {
					pConfig->iWarnId = 0;
					pConfig->iAttrId = iAttrId;
					pConfig->dwLastUpdateTime = dwLastModTime;
					if(qu.getval("excep_attr_mask"))
						pConfig->iWarnConfigFlag |= ATTR_WARN_FLAG_MASK_WARN;
					INFO_LOG("add attr exception warn config info - %d", iAttrId);
				}
			}
			else if(iStatus == RECORD_STATUS_USE && t_dwIsFind && pConfig->dwLastUpdateTime != dwLastModTime)
			{
				pConfig->dwLastUpdateTime = dwLastModTime;
				if(qu.getval("excep_attr_mask"))
					pConfig->iWarnConfigFlag |= ATTR_WARN_FLAG_MASK_WARN;
				else
					pConfig->iWarnConfigFlag = 0;
			}
			else if(iStatus != RECORD_STATUS_USE && t_dwIsFind)
			{
				memset(pConfig, 0, sizeof(*pConfig));
				RemoveHashNode(pstHash, pConfig);
				INFO_LOG("remove attr exception warn config info - %d", iAttrId);
			}
		}
		// 将异常属性保存的告警配置哈希中 -------------

		pInfo = slog.GetAttrInfo(iAttrId, &dwIsFind);
		if(iStatus == RECORD_STATUS_USE && !dwIsFind)
		{ 
			// 插入共享内存
			if(pInfo == NULL)
				ERR_LOG("need more space to save attr info , attr id:%d", iAttrId);
			else
			{
				iHashIndex = NOLIST_HASH_NODE_TO_INDEX(pAttrHash, pInfo);
				if(psysConfig->wAttrCount == 0) {
					ILINK_SET_FIRST(psysConfig, iAttrIndexStart, iAttrIndexEnd, pInfo, iPreIndex, iNextIndex, iHashIndex);
					INFO_LOG("add attrinfo to shm attr id:%u - first attr", iAttrId);
					psysConfig->wAttrCount = 1;
				}
				else {
					AttrInfoBin *pAttrInfoFirst = (AttrInfoBin*)NOLIST_HASH_INDEX_TO_NODE(pAttrHash, psysConfig->iAttrIndexStart);
					ILINK_INSERT_FIRST(psysConfig, iAttrIndexStart, pInfo, iPreIndex, iNextIndex, pAttrInfoFirst, iHashIndex);
					psysConfig->wAttrCount++;
					INFO_LOG("add attr info to shm attr id:%d - attr count:%d", iAttrId, psysConfig->wAttrCount);
				}

				pInfo->id = iAttrId;
				pInfo->iAttrType = iAttrType;
				pInfo->iDataType = qu.getval("data_type");
				pInfo->iStaticTime = iStaticTime;

				if(ptmp != NULL && ptmp[0] != '\0')
					pInfo->iNameVmemIdx = MtReport_SaveToVmem(ptmp, strlen(ptmp)+1);
				else
					pInfo->iNameVmemIdx = -1;
				pInfo->dwLastModTime = dwLastModTime;
				INFO_LOG("add attr info id:%d, nameidx:%d", iAttrId, pInfo->iNameVmemIdx);

				// 插入到监控点类型中 attrtype
				if(NULL != pAttrType) {
					iHashIndex = NOLIST_HASH_NODE_TO_INDEX(pAttrHash, pInfo);
					if(pAttrType->wAttrCount < 0) {
						ILINK_SET_FIRST(pAttrType, iAttrIndexStart,
							iAttrIndexEnd, pInfo, iAttrTypePreIndex, iAttrTypeNextIndex, iHashIndex);
						INFO_LOG("add attrinfo to attr type type id:%u attr id:%u - first attr", pInfo->iAttrType, iAttrId);
						pAttrType->wAttrCount = 1;
					}
					else {
						AttrInfoBin *pAttrInfoFirst 
							= (AttrInfoBin*)NOLIST_HASH_INDEX_TO_NODE(pAttrHash, pAttrType->iAttrIndexStart);
						ILINK_INSERT_FIRST(pAttrType, iAttrIndexStart, 
							pInfo, iAttrTypePreIndex, iAttrTypeNextIndex, pAttrInfoFirst, iHashIndex);
						pAttrType->wAttrCount++;
						INFO_LOG("add attrinfo to attr type:%u attr id:%d - attr count:%d",
							pInfo->iAttrType, iAttrId, pAttrType->wAttrCount);
					}
				}else {
					ERR_LOG("get attr type:%d failed", pInfo->iAttrType);
				}
			}
		}
		else if(iStatus == RECORD_STATUS_USE && dwIsFind && pInfo->dwLastModTime != dwLastModTime)
		{
			if(pInfo->iAttrType != iAttrType)
			{
				// 监控点类型改变处理 --- 从原监控点类型中移除
				AttrTypeInfo *pAttrTypeOld = slog.GetAttrTypeInfo(pInfo->iAttrType, NULL);
				if(pAttrTypeOld != NULL) 
				{
					AttrInfoBin *pPrev = NULL, *pNext = NULL;
					if(pInfo->iAttrTypePreIndex >= 0)
						pPrev = (AttrInfoBin*)NOLIST_HASH_INDEX_TO_NODE(pAttrHash, pInfo->iAttrTypePreIndex);
					if(pInfo->iAttrTypeNextIndex>= 0)
						pNext = (AttrInfoBin*)NOLIST_HASH_INDEX_TO_NODE(pAttrHash, pInfo->iAttrTypeNextIndex);
					ILINK_DELETE_NODE(pAttrTypeOld, iAttrIndexStart, iAttrIndexEnd, pInfo, pPrev, pNext, iAttrTypePreIndex, iAttrTypeNextIndex);
					pAttrTypeOld->wAttrCount--;
					INFO_LOG("delete attr info from old attr type:%u attr id:%d - remain attr count:%d",
						pInfo->iAttrType, iAttrId, pAttrTypeOld->wAttrCount);
				}

				// 监控点类型改变处理 --- 插入新的监控点类型中
				if(NULL != pAttrType) {
					iHashIndex = NOLIST_HASH_NODE_TO_INDEX(pAttrHash, pInfo);
					if(pAttrType->wAttrCount < 0) {
						ILINK_SET_FIRST(pAttrType, iAttrIndexStart,
							iAttrIndexEnd, pInfo, iAttrTypePreIndex, iAttrTypeNextIndex, iHashIndex);
						INFO_LOG("add attrinfo to attr type type id:%u attr id:%u - first attr", pInfo->iAttrType, iAttrId);
						pAttrType->wAttrCount = 1;
					}
					else {
						AttrInfoBin *pAttrInfoFirst 
							= (AttrInfoBin*)NOLIST_HASH_INDEX_TO_NODE(pAttrHash, pAttrType->iAttrIndexStart);
						ILINK_INSERT_FIRST(pAttrType, iAttrIndexStart, 
							pInfo, iAttrTypePreIndex, iAttrTypeNextIndex, pAttrInfoFirst, iHashIndex);
						pAttrType->wAttrCount++;
						INFO_LOG("add attrinfo to attr type:%u attr id:%d - attr count:%d",
							pInfo->iAttrType, iAttrId, pAttrType->wAttrCount);
					}
				}else {
					ERR_LOG("get attr type:%d failed", pInfo->iAttrType);
				}
				pInfo->iAttrType = iAttrType;
			}

			if(pInfo->iDataType != (uint8_t)qu.getval("data_type"))
			{
				pInfo->iDataType = qu.getval("data_type");
			}

			const char *pvname = NULL;
			if(pInfo->iNameVmemIdx > 0)
				pvname = MtReport_GetFromVmem_Local(pInfo->iNameVmemIdx);
			if(ptmp != NULL && (pvname == NULL || strcmp(pvname, ptmp)))
			{
				MtReport_FreeVmem(pInfo->iNameVmemIdx);
				pInfo->iNameVmemIdx = MtReport_SaveToVmem(ptmp, strlen(ptmp)+1);
			}
			pInfo->dwLastModTime = dwLastModTime;

			INFO_LOG("update global attr info id:%d, type:%d, data type:%d, name idx:%d",
				iAttrId, pInfo->iAttrType, pInfo->iDataType, pInfo->iNameVmemIdx);
		}

		if(dwIsFind && iStatus != RECORD_STATUS_USE)
		{
			// 从共享内存中删除
			INFO_LOG("delete attr info id:%d, free name idx:%d", pInfo->id, pInfo->iNameVmemIdx);
			pInfo->id = 0;
			MtReport_FreeVmem(pInfo->iNameVmemIdx);
			pInfo->iNameVmemIdx = -1;

			AttrInfoBin *pPrev = NULL, *pNext = NULL;
			if(pInfo->iPreIndex >= 0)
				pPrev = (AttrInfoBin*)NOLIST_HASH_INDEX_TO_NODE(pAttrHash, pInfo->iPreIndex);
			if(pInfo->iNextIndex >= 0)
				pNext = (AttrInfoBin*)NOLIST_HASH_INDEX_TO_NODE(pAttrHash, pInfo->iNextIndex);
			ILINK_DELETE_NODE(psysConfig, iAttrIndexStart, iAttrIndexEnd, pInfo, pPrev, pNext, iPreIndex, iNextIndex);
			psysConfig->wAttrCount--;
			INFO_LOG("delete attr info from shm attr id:%d - attr count:%d", iAttrId, psysConfig->wAttrCount);

			// 从 attrtype 中删除
			if(NULL != pAttrType) {
				AttrInfoBin *pPrev = NULL, *pNext = NULL;
				if(pInfo->iAttrTypePreIndex >= 0)
					pPrev = (AttrInfoBin*)NOLIST_HASH_INDEX_TO_NODE(pAttrHash, pInfo->iAttrTypePreIndex);
				if(pInfo->iAttrTypeNextIndex>= 0)
					pNext = (AttrInfoBin*)NOLIST_HASH_INDEX_TO_NODE(pAttrHash, pInfo->iAttrTypeNextIndex);
				ILINK_DELETE_NODE(pAttrType, iAttrIndexStart, iAttrIndexEnd, pInfo, pPrev, pNext, iAttrTypePreIndex, iAttrTypeNextIndex);
				pAttrType->wAttrCount--;
				INFO_LOG("delete attr info from attr type:%u attr id:%d - attr count:%d",
					pInfo->iAttrType, iAttrId, pAttrType->wAttrCount);
			}
		}

		if(iStatus == RECORD_STATUS_DELETE)
		{
			// 相关数据删除
			sprintf(sSql, 
				"update mt_view_battr set xrk_status=1 where xrk_status=0 and attr_id=%d", iAttrId);
			qutmp.execute(sSql);
			if(qutmp.affected_rows() > 0)
				INFO_LOG("set delete mt_view_battr count:%d, attr_id:%d", qutmp.affected_rows(), iAttrId);

			sprintf(sSql, 
				"update mt_warn_config set xrk_status=1 where xrk_status=0 and attr_id=%d", iAttrId);
			qutmp.execute(sSql);
			if(qutmp.affected_rows() > 0)
				INFO_LOG("set delete mt_warn_config count:%d, attr_id:%d", qutmp.affected_rows(), iAttrId);

			sprintf(sSql, "update mt_warn_info set xrk_status=1 where xrk_status=0 and attr_id=%d", iAttrId);
			qutmp.execute(sSql);
			if(qutmp.affected_rows() > 0)
				INFO_LOG("set delete mt_warn_info count:%d, attr_id:%d", qutmp.affected_rows(), iAttrId);

			snprintf(sSql, sizeof(sSql), "delete from mt_attr where xrk_id=%u", iAttrId);  
			if(!stConfig.iDeleteDbRecord || dwCurTime < dwLastModTime+stConfig.iDelRecordTime)
			{
				DEBUG_LOG("skip - %s, last update time:%u(%u)", sSql, dwLastModTime, dwCurTime);
			}
			else
			{
				// 从数据库中删除
				if(!qutmp.execute(sSql)){
					ERR_LOG("execute sql:%s failed !", sSql);
					return SLOG_ERROR_LINE;
				}
				INFO_LOG("delete attr info id:%d -- mysql", iAttrId); 
			}
		}
	}
	INFO_LOG("get attr info count:%u", qu.num_rows());
	qu.free_result();
	return 0;
}

static int DeleteAttrTypeTree(MmapUserAttrTypeTree & stTypeTree, int iType)
{
	if(iType == stTypeTree.attr_type_id())
		return 1;
	for(int i=0; i < stTypeTree.sub_type_list_size(); i++)
	{
		if(DeleteAttrTypeTree(*stTypeTree.mutable_sub_type_list(i), iType) > 0) {
			DEBUG_LOG("delete attr type :%d, sub type tree info:%s", 
				iType, stTypeTree.sub_type_list(i).ShortDebugString().c_str());
			if(i+1 < stTypeTree.sub_type_list_size()) 
				stTypeTree.mutable_sub_type_list()->SwapElements(i, stTypeTree.sub_type_list_size()-1);
			stTypeTree.mutable_sub_type_list()->RemoveLast();
			break;
		}
	}
	return 0;
}

static int TryAddAttrTypeTree(MmapUserAttrTypeTree & stTypeTree, int iTypeParent, int iType)
{
	int ret = -1;
	if(iTypeParent == stTypeTree.attr_type_id())
	{
		int i=0;
		for(i=0; i < stTypeTree.sub_type_list_size(); i++)
		{
			if(iType == stTypeTree.sub_type_list(i).attr_type_id())
				break;
		}
		if( i >= stTypeTree.sub_type_list_size())
		{
			DEBUG_LOG("add attr type to tree - parent:%d, type:%d", iTypeParent, iType);
			stTypeTree.add_sub_type_list()->set_attr_type_id(iType);
			ret = 1;
		}
		else
			ret = 0;
	}
	else {
		for(int i=0; i < stTypeTree.sub_type_list_size(); i++)
		{
			if((ret=TryAddAttrTypeTree(*stTypeTree.mutable_sub_type_list(i), iTypeParent, iType)) >= 0)
				break;
		}
	}
	return ret;
}

static void DealUserAttrTypeTree(int iTypeParent, int iType, bool bDeleteType)
{
	MtSystemConfig *psysConfig = stConfig.psysConfig;
	MmapUserAttrTypeTree stTypeTree;
	bool bFindTypeTree = false;
	bool bUpdateTypeTree = false;

	if(psysConfig->iAttrTypeTreeVmemIdx > 0)
	{
		char szShmValBuf[1024] = {0};
		int iShmValBufLen = sizeof(szShmValBuf);
		const char *pbuf = MtReport_GetFromVmemZeroCp(psysConfig->iAttrTypeTreeVmemIdx, szShmValBuf, &iShmValBufLen);
		if(pbuf != NULL && iShmValBufLen > 0) {
			if(!stTypeTree.ParseFromArray(pbuf, iShmValBufLen)) {
				WARN_LOG("stTypeTree ParseFromArray failed, len:%d", iShmValBufLen);
			}
			else {
				DEBUG_LOG("get user attr type tree ok, info:%s", stTypeTree.ShortDebugString().c_str());
				bFindTypeTree = true;
			}
		}
	}
	if(!bFindTypeTree) 
	{
		stTypeTree.set_attr_type_id(1);
		bUpdateTypeTree = true;
	}

	if(bDeleteType) {
		DeleteAttrTypeTree(stTypeTree, iType);
		DEBUG_LOG("after delete attr type:%d, type tree:%s", iType, stTypeTree.ShortDebugString().c_str());
		bUpdateTypeTree = true;
	}
	else {
		if(TryAddAttrTypeTree(stTypeTree, iTypeParent, iType) > 0) {
			DEBUG_LOG("after add attr type:%d, type tree:%s", iType, stTypeTree.ShortDebugString().c_str());
			bUpdateTypeTree = true;
		}
	}

	if(bUpdateTypeTree)
	{
		std::string strval;
		if(!stTypeTree.AppendToString(&strval))
		{
		    WARN_LOG("AppendToString failed !");
		}
		else 
		{
			int iNewIdx = MtReport_SaveToVmem(strval.c_str(), strval.size());
			if(iNewIdx >= 0) {
				DEBUG_LOG("put attr type tree to vmem ok, vmem idx:%d|%d, info:%s", 
					iNewIdx, psysConfig->iAttrTypeTreeVmemIdx, stTypeTree.ShortDebugString().c_str());
				// 删除老的存储
				MtReport_FreeVmem(psysConfig->iAttrTypeTreeVmemIdx);
				psysConfig->iAttrTypeTreeVmemIdx = iNewIdx;
			}
			else {
				WARN_LOG("save attr type tree failed, size:%d", (int)strval.size());
			}
		}
	}
}

int32_t ReadTableAttrType(Database &db, uint32_t uid=0)
{
	char sSql[256] = {0};

	if(uid == 0)
		snprintf(sSql, sizeof(sSql)-1, "select * from mt_attr_type order by xrk_type asc");
	else
		snprintf(sSql, sizeof(sSql)-1, "select * from mt_attr_type where xrk_type=%u", uid);

	Query qu(db);
	if(!qu.get_result(sSql))
	{
		ERR_LOG("get attr type failed (sql:%s)", sSql);
		return SLOG_ERROR_LINE;
	}

	int iStatus = 0, iType = 0, iTypeParent = 0;
	uint32_t dwIsFind = 0;

	uint32_t dwCurTime = time(NULL);
	uint32_t dwLastModTime = 0;
	Query qutmp(db);

	AttrTypeInfo *pInfo = NULL;
	const char *ptmp = NULL;
	bool bDeleteType = false;
	bool bAddType = false;
	while(qu.fetch_row())
	{
		dwLastModTime = datetoui(qu.getstr("update_time"));
		iType = qu.getval("xrk_type");
		iTypeParent = qu.getval("parent_type");
		iStatus = qu.getval("xrk_status");
		dwLastModTime = datetoui(qu.getstr("update_time"));
		ptmp = qu.getstr("xrk_name");
		bDeleteType = false;
		bAddType = false;

		pInfo = slog.GetAttrTypeInfo(iType, &dwIsFind);
		if(iStatus == RECORD_STATUS_USE && !dwIsFind)
		{   // 插入共享内存
			if(pInfo == NULL)
				ERR_LOG("need more space to save attr type info , attrtype id:%d", iType);
			else
			{
				pInfo->id = iType;
				if(ptmp != NULL && ptmp[0] != '\0')
					pInfo->iNameVmemIdx = MtReport_SaveToVmem(ptmp, strlen(ptmp)+1);
				else
					pInfo->iNameVmemIdx = -1;
				pInfo->dwLastModTime = dwLastModTime;
				pInfo->wAttrCount = 0;
				bAddType = true;
				INFO_LOG("add attr type info id:%d, name idx:%d", iType, pInfo->iNameVmemIdx);
			}
		}
		else if(iStatus == RECORD_STATUS_USE && dwIsFind && pInfo->dwLastModTime != dwLastModTime)
		{
			const char *pvname = NULL;
			if(pInfo->iNameVmemIdx > 0)
				pvname = MtReport_GetFromVmem_Local(pInfo->iNameVmemIdx);
			if(ptmp != NULL && (pvname == NULL || strcmp(pvname, ptmp)))
			{
				MtReport_FreeVmem(pInfo->iNameVmemIdx);
				pInfo->iNameVmemIdx = MtReport_SaveToVmem(ptmp, strlen(ptmp)+1);
			}
			pInfo->dwLastModTime = dwLastModTime;
			INFO_LOG("update attr type info id:%d, name idx:%d", iType, pInfo->iNameVmemIdx);
		}

		if(dwIsFind && iStatus != RECORD_STATUS_USE)
		{   // 从共享内存中删除
			INFO_LOG("delete attr type info id:%d", pInfo->id);
			pInfo->id = 0;
			if(pInfo->iNameVmemIdx > 0)
				MtReport_FreeVmem(pInfo->iNameVmemIdx);
			pInfo->iNameVmemIdx = -1;
		}

		if(iStatus != RECORD_STATUS_USE)
			bDeleteType = true;

		if(iType != 1 && (bAddType || bDeleteType)) {
			// deal user type tree
			DealUserAttrTypeTree(iTypeParent, iType, bDeleteType);
		}

		if(iStatus == RECORD_STATUS_DELETE)
		{
			// 其它相关数据也置为删除
			sprintf(sSql, "update mt_attr set xrk_status=1 where xrk_status=0 and attr_type=%d", iType);
			qutmp.execute(sSql);
			if(qutmp.affected_rows() > 0)
			{
				// 不允许删除有监控点的类型，所以这里不应该走到
				WARN_LOG("set delete mt_attr count:%d, attr_type:%d", qutmp.affected_rows(), iType);
			}

			// 子类型也设为删除状态
			sprintf(sSql, "update mt_attr_type set xrk_status=1 where xrk_status=0 and parent_type=%d", iType);
			qutmp.execute(sSql);

			sprintf(sSql, "delete from mt_attr_type where xrk_type=%u", iType);  
			if(!stConfig.iDeleteDbRecord || dwCurTime < dwLastModTime+stConfig.iDelRecordTime)
			{
				DEBUG_LOG("skip - %s, last update time:%u(%u)", sSql, dwLastModTime, dwCurTime);
			}
			else
			{
				// 从数据库中删除
				if(!qutmp.execute(sSql)){
					ERR_LOG("execute sql:%s failed !", sSql);
					return SLOG_ERROR_LINE;
				}
				INFO_LOG("delete attr type info type:%d -- mysql", iType); 
			}
		}
	}
	INFO_LOG("get attr type info count:%u", qu.num_rows());
	qu.free_result();
	return 0;
}

int32_t ReadTablePluginMachineInfo(Database &db, uint32_t up_id=0)
{
	char sSql[512];
	if(up_id != 0)
		snprintf(sSql, sizeof(sSql), "select * from mt_plugin_machine where xrk_id=%u", up_id);
	else {
        WARN_LOG("mt_plugin_machine try read all !");
        return 0;
    }
	Query qu(db);
	if(!qu.get_result(sSql))
	{
		ERR_LOG("get mt_plugin_machine failed (sql:%s)", sSql);
		return SLOG_ERROR_LINE;
	}

    int32_t iMachineId = 0, i = 0, iPluginId = 0, iStatus = 0;
    MtClientInfo *pclient = NULL;

	Query qutmp(db);
	while(qu.fetch_row() && qu.num_rows() > 0)
    {
        iMachineId = qu.getval("machine_id");
        iPluginId = qu.getval("open_plugin_id");
        iStatus = qu.getval("xrk_status");

        // pclient 用于将任务发放到 agent 登录的服务器上
        pclient = slog.GetMtClientInfo(iMachineId, (uint32_t *)NULL);
        if(!pclient) {
            DEBUG_LOG("not find machine client, machine:%d", iMachineId);
            continue;
        }

        for(i=0; i < 20; i++) {
            if(SYNC_FLAG_CAS_GET(&(stConfig.pShmConfig->stSysCfg.bEventModFlag)))
                break;
            usleep(slog.m_iRand%10000);
        }
        if(i >= 20) 
            ERR_LOG("get bEventModFlag failed !");
        for(i=0; i < MAX_EVENT_COUNT; i++) {
            if(stConfig.pShmConfig->stSysCfg.stEvent[i].iEventType == 0
				|| stConfig.pShmConfig->stSysCfg.stEvent[i].bEventStatus == EVENT_STATUS_FIN
                || stConfig.pShmConfig->stSysCfg.stEvent[i].dwExpireTime < slog.m_stNow.tv_sec) {
                break;
            }
        }
        if(i >= MAX_EVENT_COUNT) {
            ERR_LOG("have no space to save event !");
            SYNC_FLAG_CAS_FREE(stConfig.pShmConfig->stSysCfg.bEventModFlag);
            continue;
        }
        SYNC_FLAG_CAS_FREE(stConfig.pShmConfig->stSysCfg.bEventModFlag);

		// 处理特定事件
        if(iStatus == 0 && qu.getval("install_proc") == EV_PREINSTALL_START) {
            // 一键部署事件
        	stConfig.pShmConfig->stSysCfg.stEvent[i].iEventType = EVENT_PREINSTALL_PLUGIN;
        	stConfig.pShmConfig->stSysCfg.stEvent[i].dwExpireTime = EVENT_PREINSTALL_PLUGIN_EXPIRE_SEC+slog.m_stNow.tv_sec;
        	stConfig.pShmConfig->stSysCfg.stEvent[i].ev.stPreInstall.iPluginId = iPluginId;
        	stConfig.pShmConfig->stSysCfg.stEvent[i].ev.stPreInstall.iMachineId = iMachineId;
        	stConfig.pShmConfig->stSysCfg.stEvent[i].ev.stPreInstall.iDbId = up_id;
        	stConfig.pShmConfig->stSysCfg.stEvent[i].bEventStatus = EVENT_STATUS_INIT_SET;

			const char *ptmp = qu.getstr("local_cfg_url");
			if(!ptmp ||ptmp[0] == '\0'){
				WARN_LOG("invalid local config url, plugin:%d, machine:%d", qu.getval("open_plugin_id"), iMachineId);
				stConfig.pShmConfig->stSysCfg.stEvent[i].iEventType = 0;
				continue;
			}
        	snprintf(sSql, sizeof(sSql), 
        	    "update mt_plugin_machine set install_proc=%d where xrk_id=%u", EV_PREINSTALL_DB_RECV, up_id);
        	DEBUG_LOG("get preinstall plugin message, plugin:%d, event:%d, machine:%d", qu.getval("open_plugin_id"), i, iMachineId);
        	qutmp.execute(sSql);
		}
		else if(qu.getval("install_proc") == 0 && qu.getval("opr_proc") == 1) {
            // 配置修改/插件移除、启用、禁用等事件
            int iOprCmd = qu.getval("down_opr_cmd");
            uint32_t dwOprStartTime = qu.getuval("opr_start_time");
            if(dwOprStartTime+30 < slog.m_stNow.tv_sec) {
                WARN_LOG("machine:%d, plugin:%d, opr:%d timeout", iMachineId, iPluginId, qu.getval("down_opr_cmd")); 
                continue;
            }

            // 通用字段设置
            stConfig.pShmConfig->stSysCfg.stEvent[i].iEventType = 0;
            stConfig.pShmConfig->stSysCfg.stEvent[i].dwExpireTime = 
                EVENT_MULTI_MACH_PLUGIN_OPR_EXPIRE_SEC+slog.m_stNow.tv_sec;
            stConfig.pShmConfig->stSysCfg.stEvent[i].bEventStatus = EVENT_STATUS_INIT_SET; 
            stConfig.pShmConfig->stSysCfg.stEvent[i].ev.stMachPlugOpr.iPluginId = iPluginId;
            stConfig.pShmConfig->stSysCfg.stEvent[i].ev.stMachPlugOpr.iMachineId = iMachineId;
            stConfig.pShmConfig->stSysCfg.stEvent[i].ev.stMachPlugOpr.iDbId = up_id;

            // 在 dmt_dp_add_plugin.html 页面定义操作命令号
            switch(iOprCmd) {
                // 批量修改插件配置
                case 1:
                    stConfig.pShmConfig->stSysCfg.stEvent[i].iEventType = EVENT_MULTI_MACH_PLUGIN_CFG_MOD;
                    break;
                // 批量移除插件
                case 2:
                    stConfig.pShmConfig->stSysCfg.stEvent[i].iEventType = EVENT_MULTI_MACH_PLUGIN_REMOVE;
                   break;
                // 批量启用插件
                case 3:
                    stConfig.pShmConfig->stSysCfg.stEvent[i].iEventType = EVENT_MULTI_MACH_PLUGIN_ENABLE;
                    break;
                // 批量禁用插件
                case 4:
                    stConfig.pShmConfig->stSysCfg.stEvent[i].iEventType = EVENT_MULTI_MACH_PLUGIN_DISABLE;
                    break;
                default:
                    WARN_LOG("unknow opr:%d, machine:%d, plugin:%d", qu.getval("down_opr_cmd"), iMachineId, iPluginId);
                    break;
            }
            if(iOprCmd >= 1 && iOprCmd <= 4) {
                snprintf(sSql, sizeof(sSql), "update mt_plugin_machine set opr_proc=%d where xrk_id=%u", 
                    EV_MOP_OPR_DB_RECV, up_id);
                qutmp.execute(sSql);
            }
		}
    }
	qu.free_result();
    return 0;
}

int32_t ReadTableMachine(Database &db, uint32_t uid=0)
{
	char sSql[256];

	if(uid != 0)
		snprintf(sSql, sizeof(sSql)-1, "select * from mt_machine where xrk_id=%u", uid);
	else
		snprintf(sSql, sizeof(sSql)-1, "select * from mt_machine");
	Query qu(db);
	if(!qu.get_result(sSql))
	{
		ERR_LOG("get machine failed (sql:%s)", sSql);
		return SLOG_ERROR_LINE;
	}

	int iStatus = 0, iMachineId = 0, iHashIndex = 0;
	uint32_t dwIsFind = 0;
	MachineInfo *pInfo = NULL;
	SharedHashTableNoList *pMachHash = slog.GetMachineHash();
	const char *ptmp = NULL;
	MtSystemConfig *psysConfig = stConfig.psysConfig;

	uint32_t dwLastModTime = 0;
	uint32_t dwCurTime = time(NULL);
	Query qutmp(db);
	
	while(qu.fetch_row())
	{
		dwLastModTime = datetoui(qu.getstr("mod_time"));
		iMachineId = qu.getval("xrk_id");
		iStatus = qu.getval("xrk_status");
		pInfo = slog.GetMachineInfo(iMachineId, &dwIsFind);

		if(iStatus == RECORD_STATUS_USE && !dwIsFind)
		{ 
			// 插入共享内存
			if(pInfo == NULL)
				ERR_LOG("need more space to save machine info , machine id:%d", iMachineId);
			else
			{
				iHashIndex = NOLIST_HASH_NODE_TO_INDEX(pMachHash, pInfo);
				if(psysConfig->wMachineCount == 0 || psysConfig->iMachineListIndexStart < 0) {
					ILINK_SET_FIRST(psysConfig, iMachineListIndexStart,
						iMachineListIndexEnd, pInfo, iPreIndex, iNextIndex, iHashIndex);
					INFO_LOG("add machine info to shm, machine id:%u - first machine", iMachineId);
					psysConfig->wMachineCount = 1;
				}
				else {
					MachineInfo *pInfoFirst = (MachineInfo*)NOLIST_HASH_INDEX_TO_NODE(
							pMachHash, psysConfig->iMachineListIndexStart);
					ILINK_INSERT_FIRST(psysConfig, iMachineListIndexStart, pInfo, iPreIndex, iNextIndex, pInfoFirst, iHashIndex);
					psysConfig->wMachineCount++;
					INFO_LOG("add machine info to shm, machine id:%d - machine count:%d", iMachineId, psysConfig->wMachineCount);
				}

				pInfo->id = iMachineId;
				pInfo->ip1 = qu.getuval("ip1");
				pInfo->ip2 = qu.getuval("ip2");
				pInfo->ip3 = qu.getuval("ip3");
				pInfo->ip4 = qu.getuval("ip4");
				pInfo->bWarnFlag = qu.getval("warn_flag");
				pInfo->bModelId = qu.getval("model_id");
				ptmp = qu.getstr("xrk_name");
				if(ptmp != NULL && ptmp[0] != '\0')
					pInfo->iNameVmemIdx = MtReport_SaveToVmem(ptmp, strlen(ptmp)+1);
				else
					pInfo->iNameVmemIdx = -1;

				ptmp = qu.getstr("machine_desc");
				if(ptmp != NULL && ptmp[0] != '\0')
					pInfo->iDescVmemIdx = MtReport_SaveToVmem(ptmp, strlen(ptmp)+1);
				else
					pInfo->iDescVmemIdx = -1;

				ptmp = qu.getstr("rand_key");
				if(ptmp != NULL && ptmp[0] != '\0') {
					memcpy(pInfo->sRandKey, ptmp, 16);
					pInfo->sRandKey[16] = '\0';
				}

				pInfo->dwLastReportLogTime = qu.getuval("last_log_time");
				pInfo->dwLastReportAttrTime = qu.getuval("last_attr_time");
				pInfo->dwLastHelloTime = qu.getuval("last_hello_time");
				pInfo->dwAgentStartTime = qu.getuval("start_time");

				pInfo->dwLastModTime = dwLastModTime;
				DEBUG_LOG("add machine info id:%d ip1:%s bWarnFlag:%d bModelId:%d",
					iMachineId, ipv4_addr_str(qu.getuval("ip1")), qu.getval("warn_flag"), qu.getval("model_id"));
			}
		}
		else if(iStatus == RECORD_STATUS_USE && dwIsFind && dwLastModTime != pInfo->dwLastModTime)
		{
			// 更新发生变化的字段信息
			if(pInfo->ip1 != qu.getuval("ip1"))
				pInfo->ip1 = qu.getuval("ip1");
			if(pInfo->ip2 != qu.getuval("ip2"))
				pInfo->ip2 = qu.getuval("ip2");
			if(pInfo->ip3 != qu.getuval("ip3"))
				pInfo->ip3 = qu.getuval("ip3");
			if(pInfo->ip4 != qu.getuval("ip4"))
				pInfo->ip4 = qu.getuval("ip4");

			if(pInfo->bWarnFlag != qu.getval("warn_flag"))
				pInfo->bWarnFlag = qu.getval("warn_flag");
			if(pInfo->bModelId != qu.getval("model_id"))
				pInfo->bModelId = qu.getval("model_id");
			pInfo->dwLastModTime = dwLastModTime;

			const char *pvname = NULL;
			ptmp = qu.getstr("xrk_name");
			if(pInfo->iNameVmemIdx > 0)
				pvname = MtReport_GetFromVmem_Local(pInfo->iNameVmemIdx);
			if(ptmp != NULL && (pvname == NULL || strcmp(pvname, ptmp)))
			{
				MtReport_FreeVmem(pInfo->iNameVmemIdx);
				pInfo->iNameVmemIdx = MtReport_SaveToVmem(ptmp, strlen(ptmp)+1);
			}

			ptmp = qu.getstr("machine_desc");
			pvname = NULL;
			if(pInfo->iDescVmemIdx > 0)
				pvname = MtReport_GetFromVmem_Local(pInfo->iDescVmemIdx);
			if(ptmp != NULL && (pvname == NULL || strcmp(pvname, ptmp)))
			{
				MtReport_FreeVmem(pInfo->iDescVmemIdx);
				pInfo->iDescVmemIdx = MtReport_SaveToVmem(ptmp, strlen(ptmp)+1);
			}

			ptmp = qu.getstr("rand_key");
			if(ptmp != NULL && ptmp[0] != '\0' && strncmp(pInfo->sRandKey, ptmp, 16)) {
				memcpy(pInfo->sRandKey, ptmp, 16);
				pInfo->sRandKey[16] = '\0';
			}

			if(pInfo->dwLastHelloTime < qu.getuval("last_hello_time"))
				pInfo->dwLastHelloTime = qu.getuval("last_hello_time");

			if(pInfo->dwLastReportAttrTime < qu.getuval("last_attr_time"))
				pInfo->dwLastReportAttrTime = qu.getuval("last_attr_time");

			if(pInfo->dwLastReportLogTime < qu.getuval("last_log_time"))
				pInfo->dwLastReportLogTime = qu.getuval("last_log_time");

			if(pInfo->dwAgentStartTime < qu.getuval("start_time"))
				pInfo->dwAgentStartTime = qu.getuval("start_time");

			DEBUG_LOG("update machine info id:%d ip1:%s bWarnFlag:%d bModelId:%d mod time:%u, name:%d",
				iMachineId, ipv4_addr_str(qu.getuval("ip1")), qu.getval("warn_flag"), qu.getval("model_id"),
				dwLastModTime, pInfo->iNameVmemIdx); 
		}

		// 设置本机id 
		if(stConfig.pShmConfig->stSysCfg.iMachineId == 0 && iStatus == RECORD_STATUS_USE) 
		{
			uint32_t dwLocalIp = inet_addr(stConfig.szLocalIp);
			if(dwLocalIp == pInfo->ip1 || dwLocalIp == pInfo->ip2 
				|| dwLocalIp == pInfo->ip3 || dwLocalIp == pInfo->ip4) 
			{
				stConfig.pShmConfig->stSysCfg.iMachineId = pInfo->id;
				INFO_LOG("set local ip:%s, machine id:%d", stConfig.szLocalIp, pInfo->id);
			}
		}

		if(dwIsFind && iStatus != RECORD_STATUS_USE)
		{ 
			// 从共享内存中删除
			INFO_LOG("delete machine info id:%d ip1:%s", pInfo->id, ipv4_addr_str(pInfo->ip1));
			pInfo->id = 0;

			// vmem 释放
			if(pInfo->iNameVmemIdx > 0) {
				MtReport_FreeVmem(pInfo->iNameVmemIdx);
				pInfo->iNameVmemIdx = -1;
			}
			if(pInfo->iDescVmemIdx > 0) {
				MtReport_FreeVmem(pInfo->iDescVmemIdx);
				pInfo->iDescVmemIdx = -1;
			}

			MachineInfo *pPrev = NULL, *pNext = NULL;
			if(pInfo->iPreIndex >= 0)
				pPrev = (MachineInfo*)NOLIST_HASH_INDEX_TO_NODE(pMachHash, pInfo->iPreIndex);
			if(pInfo->iNextIndex >= 0)
				pNext = (MachineInfo*)NOLIST_HASH_INDEX_TO_NODE(pMachHash, pInfo->iNextIndex);
			ILINK_DELETE_NODE(psysConfig, 
				iMachineListIndexStart, iMachineListIndexEnd, pInfo, pPrev, pNext, iPreIndex, iNextIndex);
			if(psysConfig->iMachineListIndexStart < 0)
				psysConfig->wMachineCount = 0;
			else {
				psysConfig->wMachineCount--;
				if((int)psysConfig->wMachineCount < 0) {
					WARN_LOG("invalid machine count:%d", psysConfig->wMachineCount);
					psysConfig->wMachineCount = 0;
				}
			}
			INFO_LOG("delete machine info from shm, machine id:%d - machine count:%d", 
				iMachineId, psysConfig->wMachineCount);
		}

		if(iStatus == RECORD_STATUS_DELETE)
		{
			// 相关数据删除
			sprintf(sSql, 
				"update mt_view_bmach set xrk_status=1 where xrk_status=0 and machine_id=%d", iMachineId);
			qutmp.execute(sSql);
			if(qutmp.affected_rows() > 0)
				INFO_LOG("set delete mt_view_bmach count:%d, machine_id:%d", qutmp.affected_rows(), iMachineId);

			sprintf(sSql, "update mt_warn_config set xrk_status=1 where xrk_status=0 and warn_type_value=%d", iMachineId);
			qutmp.execute(sSql);
			if(qutmp.affected_rows() > 0)
				INFO_LOG("set delete mt_warn_config count:%d, warn_type_value:%d", qutmp.affected_rows(), iMachineId);

			sprintf(sSql, "update mt_warn_info set xrk_status=1 where xrk_status=0 and warn_id=%d", iMachineId);
			qutmp.execute(sSql);
			if(qutmp.affected_rows() > 0)
				INFO_LOG("set delete mt_warn_info count:%d, warn_id:%d", qutmp.affected_rows(), iMachineId);

			sprintf(sSql, "delete from mt_machine where xrk_id=%u", iMachineId);  
			if(!stConfig.iDeleteDbRecord || dwCurTime < dwLastModTime+stConfig.iDelRecordTime)
			{
				DEBUG_LOG("skip - %s, last update time:%u(%u)", sSql, dwLastModTime, dwCurTime);
			}
			else
			{
				// 从数据库中删除
				if(!qutmp.execute(sSql)){
					ERR_LOG("execute sql:%s failed !", sSql);
					return SLOG_ERROR_LINE;
				}
			}
		}
	}
	INFO_LOG("get machine info count:%u", qu.num_rows());
	qu.free_result();

	// 本机 ID 未找到，自动注册本机, 机器名设为IP 地址
	if(stConfig.pShmConfig->stSysCfg.iMachineId == 0)
	{
		snprintf(sSql, MYSIZEOF(sSql),
			"insert into mt_machine set xrk_name=\'%s\',ip1=%u,create_time=now(),"
			"mod_time=now(),machine_desc=\'系统自动添加\' ",
			stConfig.szLocalIp, (uint32_t)inet_addr(stConfig.szLocalIp));
		if(!qu.execute(sSql))
		{
			return SLOG_ERROR_LINE;
		}
		stConfig.pShmConfig->stSysCfg.iMachineId = qu.insert_id();
		INFO_LOG("insert new machine for local ip:%s, machine id:%d", 
			stConfig.szLocalIp, stConfig.pShmConfig->stSysCfg.iMachineId);
		ReadTableMachine(db, stConfig.pShmConfig->stSysCfg.iMachineId);
	}

	return 0;
}

void InitSysConfig()
{
	MtSystemConfig & sys = stConfig.pShmConfig->stSysCfg;

	// 设置机器 id
	MachineInfo *pLocalMachineInfo = slog.GetMachineInfoByIp(stConfig.szLocalIp);
	if(pLocalMachineInfo != NULL && sys.iMachineId != pLocalMachineInfo->id)
	{
		sys.iMachineId = pLocalMachineInfo->id;
		INFO_LOG("get local machine id:%d", sys.iMachineId);
	}

	// mysql 配置服务器信息设置
	strncpy(sys.szUserName, stConfig.szUserName, sizeof(sys.szUserName));
	strncpy(sys.szDbHost, stConfig.szDbHost, sizeof(sys.szDbHost));
	strncpy(sys.szPass, stConfig.szPass, sizeof(sys.szPass));
	strncpy(sys.szDbName, stConfig.szDbName, sizeof(sys.szDbName));
	sys.iDbPort = stConfig.iDbPort;
	strncpy(sys.sPreInstallCheckStr, stConfig.szPluginCheckStr, sizeof(sys.sPreInstallCheckStr));
	sys.dwConfigSeq = rand();
	if(sys.dwConfigSeq == 0)
		sys.dwConfigSeq = 1;
	sys.wHelloRetryTimes = 3;
	sys.wHelloPerTimeSec = 5;
	sys.wCheckLogPerTimeSec = 20;
	sys.wCheckAppPerTimeSec = 22;
	sys.wCheckServerPerTimeSec = 24;
	sys.wCheckSysPerTimeSec = 26;
	sys.bAttrSendPerTimeSec = 5;
	sys.bLogSendPerTimeSec = 3;
}

bool CheckDbConnect(uint32_t dwCurTime)
{
	static uint32_t s_dwLastCheckTime = 0;

	// 8 - 13 秒 check 一次
	if(dwCurTime <= s_dwLastCheckTime)
		return true;
	s_dwLastCheckTime = dwCurTime+8+rand()%5;

	Query qu(*stConfig.pdb);
	return qu.Connected() && stConfig.qu_info->Connected();
}

int CheckAllData()
{
	static uint32_t s_dwLastCheckTime = 0;
	uint32_t dwCurTime = time(NULL);
	if(s_dwLastCheckTime+stConfig.iCheckDataPerTime > dwCurTime)
		return 0;
	s_dwLastCheckTime = dwCurTime;

	// check appinfo -- server 
	SLogAppInfo *pAppInfoShm = slog.m_pShmAppInfo;
	if(pAppInfoShm != NULL) {
		for(int j=0; j < MAX_SLOG_APP_COUNT; j++)
		{
			if(pAppInfoShm->stInfo[j].iAppId != 0) {
				slog.CheckAppLogServer(pAppInfoShm->stInfo+j);
			}
		}
	}
	else {
		ERR_LOG("pAppInfoShm is NULL");
		return SLOG_ERROR_LINE;
	}

	slog.CheckAttrServer();

	// check module info
	if(stConfig.stModuleDelay.size() > 0) {
		INFO_LOG("orderfix - check delay module, count:%d", (int)stConfig.stModuleDelay.size());
		if(dwCurTime >= stConfig.dwModuleDelaySetTime+stConfig.iCheckDataPerTime+DELAY_RECORD_CHECK_TIMEOUT) {
			DEBUG_LOG("clear module config orderfix, count:%u", (uint32_t)stConfig.stModuleDelay.size());
			stConfig.stModuleDelay.clear();
		}
		else {
			int iRemainCount = 0;
			for(unsigned i=0; i < stConfig.stModuleDelay.size(); i++)
			{
				int iModuleId = stConfig.stModuleDelay[i];
				if(iModuleId != 0) {
					if(ReadTableModuleInfo(*stConfig.pdb, &iModuleId) == 0)
						stConfig.stModuleDelay[i] = 0;
					else
						iRemainCount++;
				}
			}
			if(iRemainCount <= 0)
				stConfig.stModuleDelay.clear();
			else
				DEBUG_LOG("orderfix module has remain count:%d", iRemainCount);
		}
	}

	return 0;
}

int32_t ReadTableViewInfo(Database &db, int iViewId_para =0)
{
	char sSql[256] = {0};

	if(iViewId_para == 0)
		snprintf(sSql, sizeof(sSql)-1, "select * from mt_view");
	else
		snprintf(sSql, sizeof(sSql)-1, "select * from mt_view where xrk_id=%d", iViewId_para);

	Query qu(db);
	if(!qu.get_result(sSql))
	{
		ERR_LOG("get mt_view failed (sql:%s)", sSql);
		return SLOG_ERROR_LINE;
	}

	int32_t iViewId = 0, iFreeIdx = 0;
	int iStatus = 0, iMatch = 0;
	TViewInfo *pinfo = NULL;

	uint32_t dwCurTime = time(NULL);
	Query qutmp(db);
	uint32_t dwLastModTime = 0;

	const char *ptmp = NULL;
	MtSystemConfig *psysConfig = stConfig.psysConfig;
	while(qu.fetch_row())
	{
		iViewId = qu.getval("xrk_id");
		iStatus = qu.getval("xrk_status");
		dwLastModTime = datetoui(qu.getstr("mod_time"));
		iMatch = slog.GetViewInfoIndex(iViewId, &iFreeIdx);
		pinfo = slog.GetViewInfo(iMatch);
		if(iMatch < 0 && iStatus == RECORD_STATUS_USE) 
		{
			if(iFreeIdx < 0)
				ERR_LOG("need more space to save view info view:%d", iViewId);
			else 
			{
				// add view info
				pinfo = slog.GetViewInfo(iFreeIdx);
				slog.AddViewInfoCount();
				pinfo->iViewId = iViewId;
				ptmp = qu.getstr("xrk_name");
				if(ptmp != NULL && ptmp[0] != '\0')
					pinfo->iViewNameVmemIdx = MtReport_SaveToVmem(ptmp, strlen(ptmp)+1);
				else
					pinfo->iViewNameVmemIdx = -1;
				pinfo->bViewFlag = qu.getval("view_flag");
				pinfo->bBindMachineCount = 0;
				pinfo->bBindAttrCount = 0;
				pinfo->dwLastModTime = dwLastModTime;

				// 加入全局配置 MtSystemConfig 中
				if(psysConfig->wViewCount == 0) {
					ILINK_SET_FIRST(psysConfig, iViewListIndexStart, iViewListIndexEnd,
						pinfo, iPreIndex, iNextIndex, iFreeIdx);
					psysConfig->wViewCount = 1;
				}
				else {
					TViewInfo *pViewInfoFirst = stConfig.pShmConfig->stViewInfo+psysConfig->iViewListIndexStart;
					ILINK_INSERT_FIRST(psysConfig, iViewListIndexStart, pinfo,
						iPreIndex, iNextIndex, pViewInfoFirst, iFreeIdx);
					psysConfig->wViewCount++;
				}
				DEBUG_LOG("add view info to shm , view:%d, name idx:%d", iViewId, pinfo->iViewNameVmemIdx);
			}
		}
		else if(iStatus == RECORD_STATUS_USE && iMatch >= 0 && dwLastModTime != pinfo->dwLastModTime)
		{
			// update view info
			const char *pvname = NULL;
			ptmp = qu.getstr("xrk_name");
			if(pinfo->iViewNameVmemIdx > 0)
				pvname = MtReport_GetFromVmem_Local(pinfo->iViewNameVmemIdx);
			if(ptmp != NULL && (pvname == NULL || strcmp(pvname, ptmp)))
			{
				MtReport_FreeVmem(pinfo->iViewNameVmemIdx);
				pinfo->iViewNameVmemIdx = MtReport_SaveToVmem(ptmp, strlen(ptmp)+1);
			}
			pinfo->bViewFlag = qu.getval("view_flag");
			pinfo->dwLastModTime = dwLastModTime;
			DEBUG_LOG("update view info, view:%d, modtime:%u", iViewId, dwLastModTime);
		}

		if(iStatus != RECORD_STATUS_USE && iMatch >= 0)
		{
			DEBUG_LOG("delete view info, view:%d, name idx:%d", iViewId, pinfo->iViewNameVmemIdx);

			// delete view info
			pinfo->iViewId = 0;
			if(pinfo->iViewNameVmemIdx > 0)
			{
				MtReport_FreeVmem(pinfo->iViewNameVmemIdx);
				pinfo->iViewNameVmemIdx = -1;
			}

			TViewInfo *pPrev = NULL, *pNext = NULL;
			if(pinfo->iPreIndex >= 0)
				pPrev = stConfig.pShmConfig->stViewInfo + pinfo->iPreIndex;
			if(pinfo->iNextIndex >= 0)
				pNext = stConfig.pShmConfig->stViewInfo + pinfo->iNextIndex;

			// 从全局配置 MtSystemConfig 中删除
			ILINK_DELETE_NODE(psysConfig, iViewListIndexStart, iViewListIndexEnd,
				pinfo, pPrev, pNext, iPreIndex, iNextIndex);
			slog.SubViewInfoCount();
		}

		if(iStatus == RECORD_STATUS_DELETE)
		{
			// 相关数据删除
			sprintf(sSql, "update mt_view_battr set xrk_status=1 where xrk_status=0 and view_id=%d", iViewId);
			qutmp.execute(sSql);
			if(qutmp.affected_rows() > 0)
				INFO_LOG("set delete mt_view_battr count:%d, view_id:%d", qutmp.affected_rows(), iViewId);

			sprintf(sSql, "update mt_view_bmach set xrk_status=1 where xrk_status=0 and view_id=%d", iViewId);
			qutmp.execute(sSql);
			if(qutmp.affected_rows() > 0)
				INFO_LOG("set delete mt_view_bmach count:%d, view_id:%d", qutmp.affected_rows(), iViewId);

			sprintf(sSql, "update mt_warn_config set xrk_status=1 where xrk_status=0 and warn_type_value=%d", iViewId);
			qutmp.execute(sSql);
			if(qutmp.affected_rows() > 0)
				INFO_LOG("set delete mt_warn_config count:%d, warn_type_value:%d", qutmp.affected_rows(), iViewId);

			sprintf(sSql, "update mt_warn_info set xrk_status=1 where xrk_status=0 and warn_id=%d", iViewId);
			qutmp.execute(sSql);
			if(qutmp.affected_rows() > 0)
				INFO_LOG("set delete mt_warn_info count:%d, warn_id:%d", qutmp.affected_rows(), iViewId);

			sprintf(sSql, "delete from mt_view where xrk_id=%u", iViewId);  
			if(!stConfig.iDeleteDbRecord || dwCurTime < dwLastModTime+stConfig.iDelRecordTime)
				DEBUG_LOG("skip - %s, last update time:%u(%u)", sSql, dwLastModTime, dwCurTime);
			else
			{
				Query qutmp(db);
				// 从数据库中删除
				if(!qutmp.execute(sSql)){
					ERR_LOG("execute sql:%s failed !", sSql);
					return SLOG_ERROR_LINE;
				}
			}
		}
	}
	qu.free_result();
	return 0;
}

typedef struct {
	uint32_t vid;
	uint32_t mid;
}TViewBindMachKey;
int32_t ReadTableMachineViewInfo(Database &db, TViewBindMachKey *pKeyInfo=NULL)
{
	char sSql[256] = {0};

	if(pKeyInfo == NULL)
		snprintf(sSql, sizeof(sSql)-1, "select * from mt_view_bmach");
	else
		snprintf(sSql, sizeof(sSql)-1, 
			"select * from mt_view_bmach where view_id=%u and machine_id=%u", pKeyInfo->vid, pKeyInfo->mid);

	Query qu(db);
	if(!qu.get_result(sSql))
	{
		ERR_LOG("get mt_view_bmach failed (sql:%s)", sSql);
		return SLOG_ERROR_LINE;
	}

	int32_t iMachineId = 0, iViewId = 0;
	uint32_t dwIsFind =  0;
	int iStatus = 0, i = 0;

	uint32_t dwCurTime = time(NULL);
	uint32_t dwLastModTime = 0;
	Query qutmp(db);

	TMachineViewConfigInfo *pInfo = NULL;
	while(qu.fetch_row())
	{
		dwLastModTime = datetoui(qu.getstr("update_time"));
		iMachineId = qu.getval("machine_id");
		iViewId = qu.getval("view_id");
		iStatus = qu.getval("xrk_status");
		pInfo = slog.GetMachineViewInfo(iMachineId, &dwIsFind);
		if(pInfo == NULL && iStatus == RECORD_STATUS_USE) 
		{
			ERR_LOG("need more space GetMachineViewInfo, machine id:%d, view id:%d", iMachineId, iViewId);
			continue;
		}

		if(!dwIsFind && iStatus == RECORD_STATUS_USE && pInfo != NULL) 
		{
			pInfo->iMachineId = iMachineId;
			pInfo->iBindViewCount = 1;
			pInfo->aryView[0] = iViewId;
			slog.AddViewBindMach(iViewId, iMachineId);
			INFO_LOG("get machine view info, machine id:%d, view id:%d", iMachineId, iViewId);
			continue;
		}
		else if(dwIsFind && iStatus == RECORD_STATUS_USE) 
		{
			slog.AddViewBindMach(iViewId, iMachineId);

			for(i=0; i < pInfo->iBindViewCount; i++)
			{
				if(iViewId == pInfo->aryView[i])
					break;
			}
			if(i < pInfo->iBindViewCount)
				continue;
			pInfo->aryView[i] = iViewId;
			pInfo->iBindViewCount++;
			INFO_LOG("get machine view info, machine id:%d, view id:%d, count:%d", iMachineId, iViewId, i);
		}

		if(dwIsFind && iStatus != RECORD_STATUS_USE)
		{
			// 移除
			for(i=0; i < pInfo->iBindViewCount; i++)
			{
				if(iViewId == pInfo->aryView[i])
				{
					for(; i+1 < pInfo->iBindViewCount; i++)
					{
						pInfo->aryView[i] = pInfo->aryView[i+1];
					}
					pInfo->iBindViewCount--;
					INFO_LOG("remove machine view info, machine id:%d, view id:%d, remain count:%d",
						iMachineId, iViewId, pInfo->iBindViewCount);
					break;
				}
			}
			slog.DelViewBindMach(iViewId, iMachineId);
			if(pInfo->iBindViewCount <= 0) 
			{
				pInfo->iMachineId = 0;
				INFO_LOG("remove machview info node, machine:%d", iMachineId);
			}
		}

		if(iStatus != RECORD_STATUS_USE)
		{
			sprintf(sSql, "delete from mt_view_bmach where machine_id=%d and view_id=%d", iMachineId, iViewId);
			if(!stConfig.iDeleteDbRecord || dwCurTime < dwLastModTime+stConfig.iDelRecordTime)
			{ 
				DEBUG_LOG("skip - %s, last update time:%u(%u)", sSql, dwLastModTime, dwCurTime);
			}
			else 
			{
				// 从数据库中删除
				if(!qutmp.execute(sSql)){
					ERR_LOG("execute sql:%s failed !", sSql);
					return SLOG_ERROR_LINE;
				}
				INFO_LOG("delete machine view info , machine id:%d , view id:%d from mysql", iMachineId, iViewId);
			}
		}
	}
	qu.free_result();
	return 0;
}

typedef struct {
	uint32_t vid;
	uint32_t aid;
}TViewBindAttrKey;
int32_t ReadTableAttrViewInfo(Database &db, TViewBindAttrKey *pKeyInfo=NULL)
{
	char sSql[256] = {0};

	if(pKeyInfo == NULL)
		snprintf(sSql, sizeof(sSql)-1, "select * from mt_view_battr");
	else
		snprintf(sSql, sizeof(sSql)-1, 
			"select * from mt_view_battr where view_id=%u and attr_id=%u", pKeyInfo->vid, pKeyInfo->aid);

	Query qu(db);
	if(!qu.get_result(sSql))
	{
		ERR_LOG("get mt_view_battr failed (sql:%s)", sSql);
		return SLOG_ERROR_LINE;
	}

	int32_t iAttrId = 0, iViewId = 0;
	uint32_t dwIsFind = 0;
	int iStatus = 0, i = 0;
	TAttrViewConfigInfo *pInfo = NULL;

	uint32_t dwCurTime = time(NULL);
	uint32_t dwLastModTime = 0;
	Query qutmp(db);
	while(qu.fetch_row())
	{
		dwLastModTime = datetoui(qu.getstr("update_time"));
		iAttrId = qu.getval("attr_id");
		iViewId = qu.getval("view_id");
		iStatus = qu.getval("xrk_status");
		pInfo = slog.GetAttrViewInfo(iAttrId, &dwIsFind);
		if(pInfo == NULL) {
			ERR_LOG("need more space GetAttrViewInfo, attr id:%d, view id:%d", iAttrId, iViewId);
			break;
		}

		if(!dwIsFind && iStatus == RECORD_STATUS_USE) 
		{
			pInfo->iAttrId = iAttrId;
			pInfo->iBindViewCount = 1;
			pInfo->aryView[0] = iViewId;
			slog.AddViewBindAttr(iViewId, iAttrId);
			INFO_LOG("get attr view info, attr id:%d, view id:%d", iAttrId, iViewId);
			continue;
		}
		else if(dwIsFind && iStatus == RECORD_STATUS_USE) 
		{
			slog.AddViewBindAttr(iViewId, iAttrId);

			for(i=0; i < pInfo->iBindViewCount; i++)
			{
				if(iViewId == pInfo->aryView[i])
					break;
			}
			if(i < pInfo->iBindViewCount)
				continue;

			pInfo->aryView[i] = iViewId;
			pInfo->iBindViewCount++;
			INFO_LOG("get attr view info, attr id:%d, view id:%d, count:%d", iAttrId, iViewId, i);
		}

		if(dwIsFind && iStatus != RECORD_STATUS_USE)
		{
			slog.DelViewBindAttr(iViewId, iAttrId);

			// 移除
			for(i=0; i < pInfo->iBindViewCount; i++)
			{
				if(iViewId == pInfo->aryView[i])
				{
					for(; i+1 < pInfo->iBindViewCount; i++)
					{
						pInfo->aryView[i] = pInfo->aryView[i+1];
					}
					pInfo->iBindViewCount--;
					INFO_LOG("remove attr view info, attr id:%d, view id:%d, remain count:%d",
						iAttrId, iViewId, pInfo->iBindViewCount);
					break;
				}
			}

			if(pInfo->iBindViewCount <= 0) 
			{
				// 回收节点
				pInfo->iAttrId = 0;
				INFO_LOG("remove attrview info node, attr:%d", iAttrId);
			}
		}

		if(iStatus != RECORD_STATUS_USE)
		{
			sprintf(sSql, "delete from mt_view_battr where attr_id=%d and view_id=%d", iAttrId, iViewId);
			if(!stConfig.iDeleteDbRecord || dwCurTime < dwLastModTime+stConfig.iDelRecordTime)
			{ 
				DEBUG_LOG("skip - %s, last update time:%u(%u)", sSql, dwLastModTime, dwCurTime);
			}
			else 
			{
				// 从数据库中删除
				if(!qutmp.execute(sSql)){
					ERR_LOG("execute sql:%s failed !", sSql);
					return SLOG_ERROR_LINE;
				}
				INFO_LOG("delete attr view info , attr id:%d , view id:%d from mysql", iAttrId, iViewId);
			}
		}
	}
	qu.free_result();
	return 0;
}

int ReadAllUpdateTableRecord(Database &db)
{
	char sSql[256];

	sprintf(sSql, "select * from mt_table_upate_monitor where r_change_id > %u order by r_change_id asc",
		stConfig.pShmConfig->stSysCfg.dwMonitorRecordsId);
	Query qu(db);
	if(!qu.get_result(sSql))
	{
		ERR_LOG("get mt_table_upate_monitor info failed (sql:%s)", sSql);
		return SLOG_ERROR_LINE;
	}

	const char *ptab = NULL;
	const char *ptime = NULL;
	uint32_t r_up_id = 0;
	uint32_t r_up_id_2 = 0;

	while(qu.fetch_row())
	{
		ptab = qu.getstr("u_table_name");
		ptime = qu.getstr("create_time");
		stConfig.pShmConfig->stSysCfg.dwMonitorRecordsId = qu.getuval("r_change_id");
		r_up_id = qu.getuval("r_primary_id");
		r_up_id_2 = qu.getuval("r_primary_id_2");

		INFO_LOG("get update record - table:%s, id:%u, id2:%u, change id:%u, time:%s", 
			ptab, r_up_id, r_up_id_2, stConfig.pShmConfig->stSysCfg.dwMonitorRecordsId, ptime);

		if(!strcmp(ptab, "mt_warn_config")) {
			if(ReadTableWarnConfig(db, r_up_id) < 0)
				return SLOG_ERROR_LINE;
		}
		else if(!strcmp(ptab, "mt_view")) {
			if(ReadTableViewInfo(db, r_up_id) < 0)
				return SLOG_ERROR_LINE;
		}
		else if(!strcmp(ptab, "mt_log_config")) {
			if(ReadTableConfigInfo(db, NULL, r_up_id) < 0)
				return SLOG_ERROR_LINE;
		}
		else if(!strcmp(ptab, "mt_app_info")) {
			if(ReadTableAppInfo(db, r_up_id) < 0)
				return SLOG_ERROR_LINE;
		}
		else if(!strcmp(ptab, "mt_module_info")) {
			if(ReadTableModuleInfo(db, NULL, r_up_id) < 0)
				return SLOG_ERROR_LINE;
		}
		else if(!strcmp(ptab, "mt_machine")) {
			if(ReadTableMachine(db, r_up_id) < 0)
				return SLOG_ERROR_LINE;
		}
		else if(!strcmp(ptab, "flogin_user")) {
			if(ReadTableFloginUser(db, r_up_id) < 0)
				return SLOG_ERROR_LINE;
		}
		else if(!strcmp(ptab, "mt_server")) {
			if(ReadTableServer(db, r_up_id) < 0)
				return SLOG_ERROR_LINE;
		}
		else if(!strcmp(ptab, "mt_view_battr")) {
			TViewBindAttrKey stKey;
			stKey.vid = r_up_id;
			stKey.aid = r_up_id_2;
			if(ReadTableAttrViewInfo(db, &stKey) < 0)
				return SLOG_ERROR_LINE;
		}
		else if(!strcmp(ptab, "mt_view_bmach")) {
			TViewBindMachKey stKey;
			stKey.vid = r_up_id;
			stKey.mid = r_up_id_2;
			if(ReadTableMachineViewInfo(db, &stKey) < 0)
				return SLOG_ERROR_LINE;
		}
		else if(!strcmp(ptab, "mt_attr")) {
			if(ReadTableAttr(db, r_up_id) < 0)
				return SLOG_ERROR_LINE;
		}
		else if(!strcmp(ptab, "mt_warn_info")) {
			if(ReadTableWarnInfo(db, r_up_id) < 0)
				return SLOG_ERROR_LINE;
		}
		else if(!strcmp(ptab, "mt_attr_type")) {
			if(ReadTableAttrType(db, r_up_id) < 0)
				return SLOG_ERROR_LINE;
		}
		else if(!strcmp(ptab, "test_key_list")) {
			if(ReadTableTestKeyInfo(db, r_up_id) < 0)
				return SLOG_ERROR_LINE;
		}
		else if(IsStrEqual(ptab, "mt_plugin_machine")) {
            if(ReadTablePluginMachineInfo(db, r_up_id) < 0)
                return SLOG_ERROR_LINE;
        }
	}
	qu.free_result();
	return 0;
}

void DelMonitorRecords(Database &db, uint32_t dwDelTime)
{
	const char *pDelTime = uitodate(dwDelTime);
	Query qu(db);
	char sSql[256] = {0};
	snprintf(sSql, sizeof(sSql)-1, "delete from mt_table_upate_monitor where create_time < \'%s\'", pDelTime);
	qu.execute(sSql);
}

int main(int argc, char *argv[])
{
	int iRet = 0;
	if((iRet=Init(NULL)) < 0)
	{
		ERR_LOG("Init Failed ret:%d !", iRet);
		return SLOG_ERROR_LINE;
	}

	slog.Daemon(1, 1, 1);
	INFO_LOG("slog_config start !");

	if(stConfig.iLoadIp) {
		if(slog.InitIpInfo(true) < 0) {
			ERR_LOG("init ip shm info failed !");
			return SLOG_ERROR_LINE;
		}
		if(ReadIpInfo("xrkmonitor_ip_info") < 0) {
			ERR_LOG("ReadIpInfo failed !");
			return SLOG_ERROR_LINE;
		}
		if(CheckIpInfo() < 0) {
			INFO_LOG("check ip info failed, reinit ip info");
			ReadIpInfo("xrkmonitor_ip_info", true);
		}
	}

	// for read table records
	Database db(stConfig.szDbHost, stConfig.szUserName, stConfig.szPass, stConfig.szDbName, &slog, stConfig.iDbPort);
	stConfig.pdb = &db;

	// for check table update
	Database db_info(stConfig.szDbHost, stConfig.szUserName, stConfig.szPass, "information_schema", &slog, stConfig.iDbPort);
	Query qu_info(db_info);
	stConfig.qu_info = &qu_info;

	bool bInitOk = true;

	// 重启依赖问题 -- 通过临时文件标志 db 是否读取完毕
	system("rm -f /tmp/_slog_config_read_ok");

	// 全部同步下数据库中的数据
	stConfig.bUpdateAll = true;
	if(ReadTableServer(db) < 0
		|| ReadTableFloginUser(db) < 0
		|| ReadTableViewInfo(db) < 0 || ReadTableWarnInfo(db) < 0
		|| ReadTableAttrType(db) < 0 || ReadTableAttr(db) < 0
		|| ReadTableAppInfo(db) < 0 || ReadTableModuleInfo(db) < 0
		|| ReadTableConfigInfo(db) < 0 || ReadTableTestKeyInfo(db) < 0
		|| ReadTableMachine(db) < 0 
		|| ReadTableWarnConfig(db) < 0 || ReadTableAttrViewInfo(db) < 0
		|| ReadTableMachineViewInfo(db) < 0) 
	{
		ERR_LOG("init read slog config failed !");
		MtReport_Attr_Add(78, 1);
		bInitOk = false;
	}
	stConfig.bUpdateAll = false;
	system("touch /tmp/_slog_config_read_ok");

	if(false == bInitOk || (iRet=slog.Init()) < 0)
	{ 
		ERR_LOG("slog init failed ret:%d\n", iRet);
		return SLOG_ERROR_LINE;
	}

	InitSysConfig();

	uint32_t dwNextDelUpdateRecordsTime = 0;
	uint32_t dwCurTime = 0;
	uint32_t dwTableMonitorTime = 0;
	while(slog.Run())
	{
		sleep(1);

		if(CheckAllData() < 0)
			break;

		if(IsTableUpdate(*stConfig.qu_info, stConfig.szDbName, 
			"mt_table_upate_monitor", dwTableMonitorTime)  == MYSQL_TABLE_UPDATED)
		{
			if((iRet=ReadAllUpdateTableRecord(db)) < 0) {
				ERR_LOG("read table update records failed, ret:%d", iRet);
				MtReport_Attr_Add(78, 1);
				break;
			}
		}

		dwCurTime = slog.m_stNow.tv_sec;
		if(!CheckDbConnect(dwCurTime))
			break;

		if(dwCurTime >= dwNextDelUpdateRecordsTime && stConfig.iDeleteMonitorRecords)
		{
			// 超过12小时的更新记录删除
			dwNextDelUpdateRecordsTime = dwCurTime+21600+rand()%60;
			DelMonitorRecords(db, dwCurTime-21600);
		}
	}
	INFO_LOG("slog_config exit");
	return 0;
}

