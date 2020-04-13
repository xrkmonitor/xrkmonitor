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
   当前版本：v1.0
   使用授权协议： apache license 2.0

   云版本主页：http://xrkmonitor.com

   云版本为开源版提供永久免费告警通道支持，告警通道支持短信、邮件、
   微信等多种方式，欢迎使用

   模块 slog_monitor_server 功能:
        用于接收客户端上报上来的监控点数据，生成监控点数据当天的 memcache 缓存
        并将数据写入 MySQL 数据库中		

****/

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS 1
#endif

#include <errno.h>
#include "top_proto.pb.h"
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <arpa/inet.h>

#include "top_include_comm.h"
#include "udp_sock.h"

#include <mt_report.h>
#include <sv_cfg.h>

CONFIG stConfig;
CSupperLog slog;

static int Init(const char *papp)
{
	int32_t iRet = 0;
	const char *pconf = "./slog_monitor_server.conf";
	if((iRet=slog.InitCommon(pconf)) < 0)
	{
		ERR_LOG("InitCommon failed ret:%d", iRet);
		return SLOG_ERROR_LINE;
	}

	int iPort = 0;
	if((iRet=LoadConfig(pconf,
		"LISTEN_PORT", CFG_INT, &iPort, 38080,
		"LOCAL_LISTEN_IP", CFG_STRING, stConfig.szListenIP, "0.0.0.0", sizeof(stConfig.szListenIP),
		"KEEP", CFG_INT, &(stConfig.iKeep), 30,
		"MEMCACHE_PORT", CFG_INT, &stConfig.iMemcachePort, 12121,
		"MEMCACHE_RTIMEOUT", CFG_INT, &stConfig.iRTimeout, 200,
		"MEMCACHE_WTIMEOUT", CFG_INT, &stConfig.iWTimeout, 200,
		"KEEP_DAY", CFG_INT, &(stConfig.iKeepDay), 365,
		"ENABLE_MEMCACHE", CFG_INT, &(stConfig.iEnableMemcache), 1,
		"LOCAL_MACHINE_ID", CFG_INT, &stConfig.iLocalMachineId, 0, 
		"SLOG_MEMCACHE_BUF_LEN", CFG_INT, &stConfig.iMemcacheBufSize, 1048576,
		"CHECK_DB_CONNECT_TIME_SEC", CFG_INT, &(stConfig.iCheckDbConnectTimeSec), 5,
		(void*)NULL)) < 0)
	{   
		ERR_LOG("LoadConfig:%s failed ! ret:%d", pconf, iRet);
		return SLOG_ERROR_LINE;
	} 
	stConfig.wServerPort = iPort;

	if(slog.InitMachineList() < 0)
	{
		FATAL_LOG("init machine list shm failed !");
		return SLOG_ERROR_LINE;
	}

	if(slog.InitAttrList() < 0)
	{
		FATAL_LOG("init mt_attr shm failed !");
		return SLOG_ERROR_LINE;
	}

	if(slog.InitWarnAttrListForWrite() < 0)
	{
		FATAL_LOG("init warn attr shm failed !");
		return SLOG_ERROR_LINE;
	}

	if(slog.InitStrAttrHashForWrite() < 0)
	{
		FATAL_LOG("init str attr shm failed !");
		return SLOG_ERROR_LINE;
	}

	// 由 slog_config 进程创建
	if(!(stConfig.pstrAttrShm=slog.GetStrAttrNodeValShm(false)))
	{
		FATAL_LOG("get StrAttrNodeValShmInfo failed, key:%d, size:%u",
			STR_ATTR_NODE_VAL_SHM_DEF_KEY, MYSIZEOF(StrAttrNodeValShmInfo));
		return SLOG_ERROR_LINE;
	}

	stConfig.pShmConfig = slog.GetSlogConfig();
	if(stConfig.pShmConfig != NULL && stConfig.iLocalMachineId==0)
	{
		stConfig.iLocalMachineId = stConfig.pShmConfig->stSysCfg.iMachineId;
		DEBUG_LOG("get local machine id:%d", stConfig.iLocalMachineId);
	}

	if(stConfig.iLocalMachineId != 0) 
	{
		stConfig.pLocalMachineInfo = slog.GetMachineInfo(stConfig.iLocalMachineId, NULL);
		if(stConfig.pLocalMachineInfo != NULL)
			DEBUG_LOG("get local machine:%d, %s", stConfig.pLocalMachineInfo->id, ipv4_addr_str(stConfig.pLocalMachineInfo->ip1));
	}
	else	
	{
		stConfig.pLocalMachineInfo = slog.GetMachineInfoByIp((char*)slog.GetLocalIP());
		if(stConfig.pLocalMachineInfo != NULL)
		{
			stConfig.iLocalMachineId = stConfig.pLocalMachineInfo->id;
			DEBUG_LOG("get local machine by ip:%s -- machine:%d, %s", 
				slog.GetLocalIP(), stConfig.pLocalMachineInfo->id, ipv4_addr_str(stConfig.pLocalMachineInfo->ip1));
		}
	}

	if(stConfig.iLocalMachineId==0 || stConfig.pLocalMachineInfo==NULL)
	{
		ERR_LOG("local machine not set or get failed, machine:%d", stConfig.iLocalMachineId);
		return SLOG_ERROR_LINE;
	}

	stConfig.psysConfig = slog.GetSystemCfg();
	if(stConfig.psysConfig == NULL) 
	{
		ERR_LOG("GetSystemCfg failed");
		return SLOG_ERROR_LINE;
	}

	INFO_LOG("monitor server ip:%s port:%d  keep day:%d keep:%d local machine id:%d",
		slog.GetLocalIP(), iPort, stConfig.iKeepDay, stConfig.iKeep, stConfig.iLocalMachineId);
	return 0;
}

bool TryUseDbHost(const char *pdb_ip, CUdpSock & stSock, bool bAttrDb)
{
	if(slog.IsIpMatchLocalMachine(inet_addr(pdb_ip)))
		pdb_ip = "127.0.0.1";

	if(bAttrDb) {
		if(stSock.m_qu) {
			if(stSock.m_qu->Connected())
				return true;
			delete stSock.m_qu;
			stSock.m_qu = NULL;
		}

		if(stSock.db) 
		{
			stSock.m_qu = new Query(*stSock.db);
			if(false == stSock.m_qu->Connected())
			{
				delete stSock.db;
				stSock.db = NULL;
				delete stSock.m_qu;
				stSock.m_qu = NULL;
			}
		}

		if(NULL == stSock.db)
		{
			// attr db
			stSock.db = new Database(pdb_ip, 
				stConfig.pShmConfig->stSysCfg.szUserName,
				stConfig.pShmConfig->stSysCfg.szPass, 
				"attr_db",
				&slog, 
				stConfig.pShmConfig->stSysCfg.iDbPort);
			stSock.m_qu = new Query(*stSock.db);
			if(false == stSock.m_qu->Connected())
			{
				ERR_LOG("connect to database failed ! dbhost:%s, dbname:attr_db", pdb_ip);
				delete stSock.db;
				stSock.db = NULL;
				delete stSock.m_qu;
				stSock.m_qu = NULL;
			}
			else {
				INFO_LOG("connect to db:%s ok, dbname:attr_db", pdb_ip);
			}
		}
		if(stSock.db && stSock.m_qu)
			return true;
	}
	else {
		if(stSock.m_qu_attr_info) {
			if(stSock.m_qu_attr_info->Connected())
				return true;
			delete stSock.m_qu_attr_info;
			stSock.m_qu_attr_info = NULL;
		}

		if(stSock.db_attr_info) 
		{
			stSock.m_qu_attr_info = new Query(*stSock.db_attr_info);
			if(false == stSock.m_qu_attr_info->Connected())
			{
				delete stSock.db_attr_info;
				stSock.db_attr_info = NULL;
				delete stSock.m_qu_attr_info;
				stSock.m_qu_attr_info = NULL;
			}
		}
		if(NULL == stSock.db_attr_info)
		{
			// attr info db
			stSock.db_attr_info = new Database(pdb_ip, 
				stConfig.pShmConfig->stSysCfg.szUserName,
				stConfig.pShmConfig->stSysCfg.szPass, 
				stConfig.pShmConfig->stSysCfg.szDbName,
				&slog, 
				stConfig.pShmConfig->stSysCfg.iDbPort);
			stSock.m_qu_attr_info = new Query(*stSock.db_attr_info);
			if(false == stSock.m_qu_attr_info->Connected()) 
			{
				ERR_LOG("connect to database failed ! dbhost:%s, dbname:%s", 
					pdb_ip, stConfig.pShmConfig->stSysCfg.szDbName);
				delete stSock.db_attr_info;
				stSock.db_attr_info = NULL;
				delete stSock.m_qu_attr_info;
				stSock.m_qu_attr_info = NULL;
			}
			else {
				INFO_LOG("connect to db:%s ok, dbName:%s", pdb_ip, stConfig.pShmConfig->stSysCfg.szDbName);
			}
		}
		if(stSock.db_attr_info && stSock.m_qu_attr_info)
			return true;
	}

	return false;
}

int CheckDbConnect(CUdpSock & stSock)
{
	static uint32_t tmLastCheck;

	if(tmLastCheck+stConfig.iCheckDbConnectTimeSec > slog.m_stNow.tv_sec)
		return 0;
	tmLastCheck = time(NULL);

	SLogServer *psrv = slog.GetValidServerByType(SRV_TYPE_ATTR_DB, NULL);
	if(NULL == psrv) {
		ERR_LOG("GetValidServerByType failed");
		return SLOG_ERROR_LINE;
	}
	TryUseDbHost(psrv->szIpV4, stSock, true);

	TryUseDbHost(stConfig.pShmConfig->stSysCfg.szDbHost, stSock, false);
	if(stSock.db!=NULL && stSock.m_qu!=NULL && stSock.db_attr_info!=NULL && stSock.m_qu_attr_info!=NULL) 
		return 0;
	return SLOG_ERROR_LINE;
}

// 一写多读模式 shared hash, 写进程定时扫描哈希表, 回收过期节点
void CheckWarnAttrHashNode()
{
	static uint32_t s_dwLastCheckTime = 0;
	uint32_t now = time(NULL);

	// 5min check 一次
	if(now < s_dwLastCheckTime+60*5)
		return;
	s_dwLastCheckTime = now;

	bool bReverse = false;
	SharedHashTable & hash = slog.GetWarnAttrHash();
	_HashTableHead *pTableHead = (_HashTableHead*)hash.pHash;
	TWarnAttrReportInfo *pNode = (TWarnAttrReportInfo*)GetFirstNode(&hash);
	if(pNode == NULL && pTableHead->dwNodeUseCount > 0) {
		bReverse = true;
		pNode = (TWarnAttrReportInfo*)GetFirstNodeRevers(&hash);
		ERR_LOG("man by bug - has node:%u, bug get first null", pTableHead->dwNodeUseCount);
	}
	for(; pNode != NULL; )
	{
		// fix bug @ 2020-04-13 历史累计监控点不能回收
		if(pNode->bAttrDataType != SUM_REPORT_TOTAL
			&& pNode->iAttrId != 0 && pNode->iMachineId != 0 && pNode->dwLastReportTime+60*10 <= now)
		{
			INFO_LOG("recycle warn attr hash node - machine id:%d , attr id:%d , last report time:%s",
				pNode->iMachineId, pNode->iAttrId, uitodate(pNode->dwLastReportTime));
			pNode->iMachineId = 0;
			pNode->iAttrId = 0;
			RemoveHashNode(&hash, pNode);
			if(bReverse)
				pNode = (TWarnAttrReportInfo*)GetCurNodeRevers(&hash);
			else
				pNode = (TWarnAttrReportInfo*)GetCurNode(&hash);
		}
		else
		{
			if(bReverse)
				pNode = (TWarnAttrReportInfo*)GetNextNodeRevers(&hash);
			else
				pNode = (TWarnAttrReportInfo*)GetNextNode(&hash);
		}
	}

	if(bReverse && pTableHead->dwNodeUseCount <=1 )  {
		ResetHashTable(&hash);
		hash.bAccessCheck = 1;
	}
	else if(!bReverse && pTableHead->dwNodeUseCount <= 0) {
		hash.bAccessCheck = 0;
	}
}

int main(int argc, char* argv[])
{
	int iRet = 0;
	if((iRet=Init(argv[0])) < 0)
	{
		ERR_LOG("Init Failed ret:%d !", iRet);
		return SLOG_ERROR_LINE;
	}

	if(slog.m_iProcessCount != 2) {
		FATAL_LOG("process count invalid %d != 2", slog.m_iProcessCount);
		return SLOG_ERROR_LINE;
	}

	slog.Daemon(1, 1, 0);
	slog.TryRun();
	INFO_LOG("monitor_server start:%d", slog.m_iProcessId);

	SocketHandler h(&slog);
	CUdpSock stSock(h);
	stSock.SetLocalTimeInfo();

	stConfig.pShmConfig = slog.GetSlogConfig();

	// 该进程负责将上报数据写入 db 
	if(1 == slog.m_iProcessId) 
	{
		stSock.db = NULL;
		stSock.m_iKeep = stConfig.iKeep;
		stSock.m_iKeepDay = stConfig.iKeepDay;
		if(CheckDbConnect(stSock) < 0)
			return SLOG_ERROR_LINE;
		stSock.TryChangeAttrSaveType();
		while(slog.Run())
		{
			if(slog.IsExitSet()) {
				stSock.SetCloseAndDelete();
				h.Select(1, 2000);
				continue;
			}
			if(CheckDbConnect(stSock) < 0)
				break;

			stSock.CheckAllAttrTotal();
			stSock.WriteAttrDataToMemcache();
			stSock.WriteAttrDataToDb();
			stSock.WriteStrAttrToDb();
			CheckWarnAttrHashNode();
			h.Select(1, 2000);
		}
		INFO_LOG("slog_monitor_server (write attr to db) exit !");
		return 0;
	}

	//该进程接收上报请求并将数据写到共享内存中
	const char *pszListenIp = stConfig.szListenIP;
	if('\0' == pszListenIp[0])
		pszListenIp = slog.GetLocalIP();
	Ipv4Address addr(pszListenIp, stConfig.wServerPort);
	if(stSock.Bind(addr, 0) < 0)
	{
		FATAL_LOG("bind port:%d failed", stConfig.wServerPort);
		return SLOG_ERROR_LINE;
	}
	h.Add(&stSock);

	// 字符型监控点共享内存启动处理 --- start(为避免多进程操作shm，只能在该进程中处理)
	SharedHashTable s_stStrAttrHash = slog.GetStrAttrShmHash();
	bool bReverse = false;
	_HashTableHead *pTableHead = (_HashTableHead*)s_stStrAttrHash.pHash;
	TStrAttrReportInfo *pStrAttrShm = (TStrAttrReportInfo*)GetFirstNode(&s_stStrAttrHash);
	if(pStrAttrShm == NULL && pTableHead->dwNodeUseCount > 0) {
		bReverse = true;
		pStrAttrShm = (TStrAttrReportInfo*)GetFirstNodeRevers(&s_stStrAttrHash);
	}

	// 如果清理过共享内存，则需要重新从 DB 读取当天的数据写入到 shm
	if(pStrAttrShm == NULL) {
		if(!TryUseDbHost(stConfig.pShmConfig->stSysCfg.szDbHost, stSock, true)) {
			ERR_LOG("check attr db failed !");
			return SLOG_ERROR_LINE;
		}
		if(stSock.ReadStrAttrInfoFromDbToShm() < 0)
			return SLOG_ERROR_LINE;

		delete stSock.m_qu;
		delete stSock.db;
		stSock.db = NULL;
		stSock.m_qu = NULL;
	}
	while(pStrAttrShm != NULL) 
	{
		if(stSock.CheckClearStrAttrNodeShm(pStrAttrShm)) {
			pStrAttrShm->iMachineId = 0;
			pStrAttrShm->iAttrId = 0;
			RemoveHashNode(&s_stStrAttrHash, pStrAttrShm);
		}
		if(bReverse)
			pStrAttrShm = (TStrAttrReportInfo*)GetNextNodeRevers(&s_stStrAttrHash);
		else 
			pStrAttrShm = (TStrAttrReportInfo*)GetNextNode(&s_stStrAttrHash);
	}
	// 字符型监控点共享内存启动处理 --- end 

	while(h.GetCount() && slog.TryRun())
	{
		if(slog.IsExitSet())
		{
			stSock.SetCloseAndDelete();
			h.Select(0, 1000);
			continue;
		}
		h.Select(0, 2000);
	}
	INFO_LOG("slog_monitor_server (write attr to shm) exit !");
	return 0;
}

