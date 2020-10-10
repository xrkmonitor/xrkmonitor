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

   cgi/fcgi 相关模块说明:
          cgi 包含两类-普通的 cgi，fastcgi，cgi 主要使用了开源软件 clearsilver
          通过 clearsilver 的模板机制将控制逻辑是页面视图分开

   fastcgi mt_slog: 处理日志查询，日志应用模块配置等

****/

#ifndef __STDC_FORMAT_MACROS 
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>

#include <string>
#include <fcgi_stdio.h>
#include <fcgi_config.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdarg.h>
#include <inttypes.h>
#include <errno.h>
#include <math.h>
#include <sv_file.h>
#include <map>
#include <sstream>
#include <cgi_head.h>
#include <cgi_comm.h>
#include <cgi_attr.h>
#include <iostream>

// 机器插件操作每次能操作的最大机器数
const int PLUGIN_MAX_OPR_MULTI_MACHINES = 20;
const int PLUGIN_MAX_OPR_MULTI_PLUGINS = 10;

CSupperLog slog;
CGIConfig stConfig;
CSLogSearch logsearch;
int32_t g_iNeedDb = 0;
char g_sLogPath[256] = {0};
SLogServer* g_pHttpTestServer = NULL;
#define CHECK_RESULT_NOT_FIND_APP_LOG 777

#define LOG_TEST_CONFIG_CHAR "~" // 染色配置类型/值分隔符

const std::string g_strCustLogHaveNo("null");

// ajax json 响应方式
static const char *s_JsonRequest [] = { 
	"get_history",
	"get_app_file_list",
	"show_log_files",
	"get_realtime",
	"delete_app",
	"delete_config",
	"delete_module",
	"save_add_app",
	"save_mod_app",
	"save_add_module",
	"save_mod_module",
	"save_add_config",
	"save_mod_config",
	"refresh_log_info",
	"send_test_log",
	"refresh_main_info",
	"install_open_plugin",
	"update_open_plugin",
	"down_plugin_conf",
    "refresh_preinstall_plugin_status",
    "refresh_mach_opr_plugin_status",
    "ddap_save_mod_plugin_cfg",
	NULL
};

#define REALTIME_LOG_GET_MAX 800 // 实时日志每次请求默认返回的最大日志数目

typedef struct
{
	int app_id;
	int module_id;
}SearchInfo;

static int GetAppModuleList(CGI *cgi, Json & js);

static int AddSearchInfo(char *psql, int ibufLen, SearchInfo *pinfo)
{
	char sTmpBuf[128] = {0};
	if(pinfo->app_id != 0)
	{
		sTmpBuf[0] = '\0';
		sprintf(sTmpBuf, " and app_id=%d ", pinfo->app_id);
		strcat(psql, sTmpBuf);
	}
	hdf_set_int_value(stConfig.cgi->hdf, "config.dlc_app_id", pinfo->app_id);

	if(pinfo->module_id != 0)
	{
		sprintf(sTmpBuf, " and module_id=%d ", pinfo->module_id);
		strcat(psql, sTmpBuf);
	}
	hdf_set_int_value(stConfig.cgi->hdf, "config.dlc_module_id", pinfo->module_id);
	DEBUG_LOG("after add search info sql:%s", psql);
	return 0;
}

// fast cgi 必须要重载这些基础函数接口 --- start ---------------
static int cs_printf(void *ctx, const char *s, va_list args)
{
	return FCGI_vfprintf(FCGI_stdout, s, args);
}

static int cs_write(void *ctx, const char *s, int n)
{
	return FCGI_fwrite(const_cast<char *>(s), 1, n, FCGI_stdout);
}

static int cs_read(void *ctx, char *s, int n)
{
	return FCGI_fread(s, 1, n, FCGI_stdin);
}
// fast cgi 必须要重载这些基础函数接口 --- end ---------------

int SetSearchParaComm(CGI *cgi, CSLogSearch &logsearch)
{
	// 查询的 appid ---------------------
	char *pquery = hdf_get_value(cgi->hdf, "Query.appId", NULL);
	logsearch.SetLogPath(g_sLogPath);
	if(logsearch.SetAppId(atoi(pquery)) < 0)
	{
		// app 没有日志上报可能出错
		WARN_LOG("SetAppId failed:%s", pquery);
		stConfig.iErrorCode = CHECK_RESULT_NOT_FIND_APP_LOG;
		return SLOG_ERROR_LINE;
	}
	DEBUG_LOG("query para appid:%s", pquery);

	// 查询的模块 ID ------------------
	pquery = hdf_get_value(cgi->hdf, "Query.moduleIdList", NULL);
	if(pquery != NULL)
	{
		char *psave = NULL;
		char *pmodule = strdup(pquery);
		if(pmodule == NULL)
			return -101;
		char *pmem = pmodule;
		for(int i=1; ; i++)
		{
			pquery = strtok_r(pmodule, "_", &psave);
			if(NULL == pquery)
				break;
			DEBUG_LOG("query para:%d module id:%s", i, pquery);
			logsearch.SetModuleId(atoi(pquery));
			pmodule = NULL;
		}
		free(pmem);
	}

	// 查询日志类型 -----------
	int iLogType = hdf_get_int_value(cgi->hdf, "Query.logType", 0);
	if(iLogType != 0)
		logsearch.SetLogType(iLogType);

	// 查询的包含关键字 -------------
	pquery = hdf_get_value(cgi->hdf, "Query.incKeyList", NULL);
	if(pquery != NULL && pquery[0] != '\0')
	{
		char *pkeyinc = strdup(pquery);
		if(pkeyinc == NULL)
			return -103;
		char *pmem = pkeyinc;
		for(int i=1; ; i++)
		{
			pquery = strstr(pkeyinc, "_"); 
			if(NULL != pquery)
			{
				*pquery = '\0';
				pquery += 1;
			}

			DEBUG_LOG("query para key include:%d include key:%s", i, pkeyinc);
			logsearch.AddIncludeKey(pkeyinc);
			if(NULL == pquery)
				break;
			pkeyinc = pquery;
		}
		free(pmem);
	}

	// 查询的排除关键字 -------------
	pquery = hdf_get_value(cgi->hdf, "Query.exceKeyList", NULL);
	if(pquery != NULL && pquery[0] != '\0')
	{
		char *pkeyexcp = strdup(pquery);
		if(pkeyexcp == NULL)
			return -104;
		char *pmem = pkeyexcp;
		for(int i=1; ; i++)
		{
			pquery = strstr(pkeyexcp, "_"); 
			if(NULL != pquery)
			{
				*pquery = '\0';
				pquery += 1;
			}

			DEBUG_LOG("query para key except:%d except key:%s", i, pkeyexcp);
			logsearch.AddExceptKey(pkeyexcp);
			if(NULL == pquery)
				break;
			pkeyexcp = pquery;
		}
		free(pmem);
	}

	// 上报机器
	int iMachine = hdf_get_int_value(cgi->hdf, "Query.machine", 0);
	logsearch.SetMachine(iMachine);

	// 要显示的日志字段 --------------------
	int iLogField = hdf_get_int_value(cgi->hdf, "Query.logField", 0);
	logsearch.SetLogField(iLogField);
	DEBUG_LOG("query para iLogField:%#x, machine:%d", iLogField, iMachine);
	return 0;
}

int SetSearchPara(CGI *cgi, CSLogSearch &logsearch)
{
	if(SetSearchParaComm(cgi, logsearch) < 0)
		return -120;

	// 查询历史记录专有字段 时间或者文件 ------------------------------------------
	int32_t iLastTime = hdf_get_int_value(cgi->hdf, "Query.LastTime", 0);
	const char *plogFile = hdf_get_value(cgi->hdf, "Query.LogFile", NULL);
	if(plogFile != NULL)
		logsearch.SetLogFile(plogFile);	
	else if(iLastTime == 0) {
		char *pquery = hdf_get_value(cgi->hdf, "Query.StartTime", NULL);
		if(NULL == pquery)
		{
			REQERR_LOG("get start time failed !");
			return -110;
		}
		logsearch.SetStartTime(strtoul(pquery, NULL, 10));

		pquery = hdf_get_value(cgi->hdf, "Query.StopTime", NULL);
		if(NULL == pquery)
		{
			REQERR_LOG("get stop time failed !");
			return -110;
		}
		logsearch.SetEndTime(strtoul(pquery, NULL, 10));
	}
	else {
		if(iLastTime > 630720000) {
			REQERR_LOG("invalid lasttime req:%d", iLastTime);
			return SLOG_ERROR_LINE;
		}
		logsearch.SetStartTime(stConfig.dwCurTime-iLastTime);
		logsearch.SetEndTime(stConfig.dwCurTime);
	}
	DEBUG_LOG("query time:%u-%u", logsearch.GetStartTime(), logsearch.GetEndTime());

	int32_t iTmp = hdf_get_int_value(cgi->hdf, "Query.FileNo", 0);
	logsearch.SetFileNo(iTmp);
	iTmp = hdf_get_int_value(cgi->hdf, "Query.FilePos", 0);
	logsearch.SetFilePos(iTmp);
	iTmp = hdf_get_int_value(cgi->hdf, "Query.FileCount", 0);
	logsearch.SetFileCount(iTmp);
	iTmp = hdf_get_int_value(cgi->hdf, "Query.FileIndexStar", 0);
	logsearch.SetFileIndexStar(iTmp);
	return 0;
}

void AddModule(Json &js, uint32_t dwAppId)
{
	char sSqlBuf[128] = {0};
	Query qu(*stConfig.db);

	sprintf(sSqlBuf, 
		"select * from module_info where app_id=%u and xrk_status=%d", dwAppId, RECORD_STATUS_USE);
	qu.get_result(sSqlBuf);
	if(qu.num_rows() > 0) 
	{
		while(qu.fetch_row() != NULL)
		{
			Json module;
			module["desc"] = qu.getstr("module_desc");
			module["id"] = qu.getuval("module_id"); 
			js["modulelist"].Add(module);
		}
	}
	else
	{
		Json module;
		module["desc"] = "no_module_find";
		module["id"] = (short)0;
		js["modulelist"].Add(module);
	}
	qu.free_result();
}

static void GetLogConfigTestKey(int iConfigId, std::string &strKeys)
{
	SLogClientConfig *pLogConfig = slog.GetSlogConfig(iConfigId);
	strKeys.clear();
	if(pLogConfig != NULL) {
		for(int i=0; i < pLogConfig->wTestKeyCount; i++)
		{
			Json js;
			strKeys += itoa(pLogConfig->stTestKeys[i].bKeyType);
			strKeys += LOG_TEST_CONFIG_CHAR;
			strKeys += pLogConfig->stTestKeys[i].szKeyValue;
			if(i+1 >= pLogConfig->wTestKeyCount)
				break;
			strKeys += "|";
		}
	}
	else
	{
		Query qu(*stConfig.db);
		char sSqlBuf[128] = {0};
		snprintf(sSqlBuf, sizeof(sSqlBuf),
			"select * from test_key_list where xrk_status=0 and config_id=%d", iConfigId);
		qu.get_result(sSqlBuf);
		if(qu.num_rows() > 0) 
		{
			int iNum = qu.num_rows();
			for(int i=0; i < iNum && qu.fetch_row() != NULL; i++)
			{
				strKeys += qu.getstr("test_key_type");
				strKeys += LOG_TEST_CONFIG_CHAR;
				strKeys += qu.getstr("test_key");
				if(i+1 >= iNum)
					break;
				strKeys += "|";
			}
		}
		qu.free_result();
	}
	DEBUG_LOG("get testkeys:%s, log config id:%d", strKeys.c_str(), iConfigId);
}

static void SaveLogConfigTestKey(int iConfigId, const char *pLogKeys)
{
	static char sLocalBuf[MAX_SLOG_TEST_KEYS_PER_CONFIG*(SLOG_TEST_KEY_LEN+20)];
	static SLogTestKey stTestKeys[MAX_SLOG_TEST_KEYS_PER_CONFIG];
	static SLogTestKey stTestKeys_old[MAX_SLOG_TEST_KEYS_PER_CONFIG];

	memset(stTestKeys, 0, sizeof(stTestKeys));

	strncpy(sLocalBuf, pLogKeys, MYSIZEOF(sLocalBuf)-1);
	char *psave = NULL;
	char *pKeys = (char*)sLocalBuf;
	char *pTypeKey = NULL, *pTmp = NULL;
	int iCount = 0;
	for(int i=0; iCount < MAX_SLOG_TEST_KEYS_PER_CONFIG; i++)
	{
		pTypeKey = strtok_r(pKeys, "|", &psave);
		if(NULL == pTypeKey)
			break;
		pKeys = NULL;
		pTmp = strchr(pTypeKey, LOG_TEST_CONFIG_CHAR[0]);
		if(pTmp == NULL)
			continue;
		*pTmp = '\0';
		pTmp++;
		stTestKeys[iCount].bKeyType = atoi(pTypeKey);
		strncpy(stTestKeys[iCount].szKeyValue, pTmp, SLOG_TEST_KEY_LEN);
		DEBUG_LOG("get log config:%d, test key:%d - %s", iConfigId, stTestKeys[iCount].bKeyType, pTmp);
		iCount++;
	}

	SLogClientConfig *pLogConfig = slog.GetSlogConfig(iConfigId);

	// 临时保存下当前 test key 用于更新 db
	int iTestKeysOldCount = 0;
	
	// pLogConfig 为 NULL，是新增配置, 共享内存中可能暂时没有同步下来
	if(pLogConfig != NULL && pLogConfig->wTestKeyCount > 0)
		iTestKeysOldCount = pLogConfig->wTestKeyCount;

	if(iTestKeysOldCount > 0)
		memcpy(stTestKeys_old, pLogConfig->stTestKeys, sizeof(SLogTestKey)*iTestKeysOldCount);

	// 最新 test key 直接保存到共享内存中 (这里不检查是否变化、在db 操作之前就更新，避免竞争问题)
	if(pLogConfig != NULL) 
	{
		pLogConfig->wTestKeyCount = iCount;

		// fix bug 这里要整个拷贝 stTestKeys，否则 slog_config 会找到老的 key，减少 wTestKeyCount 值导致错误
		if(iCount > 0)
			memcpy(pLogConfig->stTestKeys, stTestKeys, sizeof(stTestKeys));
		else
			memset(pLogConfig->stTestKeys, 0, sizeof(pLogConfig->stTestKeys));
		DEBUG_LOG("config id:%d , test key count:%d", pLogConfig->dwCfgId, pLogConfig->wTestKeyCount);
	}

	char sSqlBuf[128] = {0};
	Query & qu = *(stConfig.qu);
	bool bChanged = false;

	// 检查哪些是被删除的 test key
	for(int i=0; i < iTestKeysOldCount; i++)
	{
		int j =0;
		for(; j < iCount; j++)
		{
			if(stTestKeys_old[i].bKeyType == stTestKeys[j].bKeyType
				&& !strcmp(stTestKeys_old[i].szKeyValue, stTestKeys[j].szKeyValue))
				break;
		}
		if(j >= iCount)
		{
			// 删除 key
			snprintf(sSqlBuf, sizeof(sSqlBuf),
				"update test_key_list set xrk_status=%d where config_id=%d and test_key_type=%d "
				" and test_key=\'%s\'", RECORD_STATUS_DELETE, iConfigId, 
				stTestKeys_old[i].bKeyType, stTestKeys_old[i].szKeyValue);
			qu.execute(sSqlBuf);
			bChanged = true;
			DEBUG_LOG("delete log config:%d test key, sql:%s", iConfigId, sSqlBuf);
		}
	}

	// 检查哪些是新增的 test key
	for(int i=0; i < iCount; i++)
	{
		int j =0;
		for(; j < iTestKeysOldCount; j++)
		{
			if(stTestKeys_old[j].bKeyType == stTestKeys[i].bKeyType
				&& !strcmp(stTestKeys_old[j].szKeyValue, stTestKeys[i].szKeyValue))
				break;
		}
		if(j >= iTestKeysOldCount)
		{
			// 新增 test key
			snprintf(sSqlBuf, sizeof(sSqlBuf),
				"replace into test_key_list set xrk_status=0,config_id=%d,test_key_type=%d, "
				"test_key=\'%s\'", iConfigId, stTestKeys[i].bKeyType, stTestKeys[i].szKeyValue);
			qu.execute(sSqlBuf);
			bChanged = true;
			DEBUG_LOG("add log config:%d test key, sql:%s", iConfigId, sSqlBuf);
		}
	}

	if(bChanged && pLogConfig != NULL)
		pLogConfig->dwConfigSeq++;
}

static int DealSaveConfig(CGI *cgi, bool bIsAdd=false)
{
	if(IsDaemonDenyOp(stConfig))
		return 0;

	FloginInfo *pUserInfo = stConfig.stUser.puser_info;
	const char *pdesc = hdf_get_value(cgi->hdf, "Query.ddac_config_desc", NULL);
	const char *pname = hdf_get_value(cgi->hdf, "Query.ddac_config_name", NULL);
	const char *pnav = hdf_get_value(cgi->hdf, "Query.navTabId", NULL);
	int32_t iAppId = hdf_get_int_value(cgi->hdf, "Query.ddac_op_app", -1);
	int32_t iModuleId = hdf_get_int_value(cgi->hdf, "Query.ddac_op_module", -1);
	int32_t iLogType  = hdf_get_int_value(cgi->hdf, "Query.ddac_ck_logtype_all", -1);
	int32_t iConfigId = hdf_get_int_value(cgi->hdf, "Query.ddac_config_id", -1);
	int32_t iLogFreq = hdf_get_int_value(cgi->hdf, "Query.ddac_log_freq", -1);
	const char *pLogKeys = hdf_get_value(cgi->hdf, "Query.ddac_log_test_keys", NULL);
	if(NULL == pname || iAppId < 0 || iModuleId < 0 || iLogType < 0 || (!bIsAdd && iConfigId < 0))
	{
		WARN_LOG("invalid parameter name:%s desc:%s appid:%d, moduleid:%d, logtype:%d, configid:%d",
			pname, pdesc, iAppId, iModuleId, iLogType, iConfigId);
		stConfig.pErrMsg = CGI_REQERR;
		return SLOG_ERROR_LINE;
	}

	AppInfo *pAppInfo =  slog.GetAppInfo(iAppId);
	if(pAppInfo == NULL)
	{
		REQERR_LOG("get app:%d failed", iAppId);
		stConfig.pErrMsg = CGI_REQERR;
		return SLOG_ERROR_LINE;
	}

	// module 归属验证
	int iModuleIdx = 0, k = 0;
	for(; k < SLOG_MODULE_COUNT_MAX_PER_APP && iModuleIdx < pAppInfo->wModuleCount; k++)
	{
		if(0 == pAppInfo->arrModuleList[k].iModuleId)
			continue;
		if(pAppInfo->arrModuleList[k].iModuleId == iModuleId)
			break;
		iModuleIdx++;
	}
	if(iModuleIdx >= pAppInfo->wModuleCount || k >= SLOG_MODULE_COUNT_MAX_PER_APP)
	{
		ERR_LOG("find app:%d, module:%d failed", iAppId, iModuleId);
		stConfig.pErrMsg = CGI_REQERR;
		return SLOG_ERROR_LINE;
	}

	Query & qu = *(stConfig.qu);
	IM_SQL_PARA* ppara = NULL;
	if(InitParameter(&ppara) < 0) {
		ERR_LOG("sql parameter init failed !");
		return SLOG_ERROR_LINE;
	}
	if(pdesc != NULL)
		AddParameter(&ppara, "config_desc", pdesc, NULL);
	if(pname != NULL)
		AddParameter(&ppara, "config_name", pname, NULL);
	if(iLogFreq > 0)
		AddParameter(&ppara, "write_speed", iLogFreq, "DB_CAL");

	AddParameter(&ppara, "user_mod_id", pUserInfo->iUserId, "DB_CAL");
	AddParameter(&ppara, "user_mod", pUserInfo->szUserName, NULL);
	std::string strSql;
	if(bIsAdd) {
		AddParameter(&ppara, "create_time", stConfig.dwCurTime, "DB_CAL");
		AddParameter(&ppara, "update_time", uitodate(stConfig.dwCurTime), NULL);
		AddParameter(&ppara, "user_add", pUserInfo->szUserName, NULL);
		AddParameter(&ppara, "user_add_id", pUserInfo->iUserId, "DB_CAL");
		AddParameter(&ppara, "app_id", iAppId, "DB_CAL");
		AddParameter(&ppara, "module_id", iModuleId, "DB_CAL");
		AddParameter(&ppara, "log_types", iLogType, "DB_CAL");
		strSql = "insert into mt_log_config";
		JoinParameter_Insert(&strSql, qu.GetMysql(), ppara);
	}
	else {
		AddParameter(&ppara, "log_types", iLogType, "DB_CAL");
		strSql = "update mt_log_config set";
		JoinParameter_Set(&strSql, qu.GetMysql(), ppara);
		strSql += " where config_id=";
		strSql += itoa(iConfigId);
	}

	ReleaseParameter(&ppara);
	if(!qu.execute(strSql))
	{
		ERR_LOG("execute sql:%s failed, msg:%s", strSql.c_str(), qu.GetError().c_str());
		return -1;
	}

	if(bIsAdd)
		iConfigId = qu.insert_id();
	if(pLogKeys != NULL)
		SaveLogConfigTestKey(iConfigId, pLogKeys);

	Json js;
	js["statusCode"] = 200;
	js["navTabId"] = pnav;
	js["callbackType"] = "closeCurrent";
	if(bIsAdd)
		js["msgid"] = "addSuccess";
	else
		js["msgid"] = "modSuccess";

	STRING str;
	string_init(&str);
	if((stConfig.err=string_set(&str, js.ToString().c_str())) != STATUS_OK
		|| (stConfig.err=cgi_output(stConfig.cgi, &str)) != STATUS_OK)
	{
		string_clear(&str);
		stConfig.pErrMsg = CGI_ERR_SERVER;
		return SLOG_ERROR_LINE;
	}
	string_clear(&str);

	DEBUG_LOG("%s mt_log_config success", bIsAdd ? "add" : "mod");
	return 0;
}

static int DealDelConfig(CGI *cgi)
{
	if(IsDaemonDenyOp(stConfig))
		return 0;

	int id = hdf_get_int_value(cgi->hdf, "Query.id", -1);
	if(id < 0)
	{
		WARN_LOG("invalid parameter have no config_id");
		stConfig.pErrMsg = CGI_REQERR;
		return -1;
	}

	static char sSqlBuf[256] = {0};
	sprintf(sSqlBuf, "update mt_log_config set xrk_status=%d where config_id=%d", RECORD_STATUS_DELETE, id);
	Query & qu = *(stConfig.qu);
	if(!qu.execute(sSqlBuf))
	{
		ERR_LOG("execute sql:%s failed, msg:%s", sSqlBuf, qu.GetError().c_str());
		return -2;
	}

	Json js;
	js["statusCode"] = 200;

	STRING str;
	string_init(&str);
	if((stConfig.err=string_set(&str, js.ToString().c_str())) != STATUS_OK
		|| (stConfig.err=cgi_output(stConfig.cgi, &str)) != STATUS_OK)
	{
		string_clear(&str);
		stConfig.pErrMsg = CGI_ERR_SERVER;
		return SLOG_ERROR_LINE;
	}
	string_clear(&str);

	DEBUG_LOG("change log config info status to:%d - id:%d success ", RECORD_STATUS_DELETE, id);
	return 0;
}

static int DealDetailLogConfig(CGI *cgi)
{
	int32_t iConfigId = hdf_get_int_value(cgi->hdf, "Query.config_id", -1);
	if(iConfigId < 0)
	{
		WARN_LOG("invalid parameter config id:%d", iConfigId);
		stConfig.pErrMsg = CGI_REQERR;
		return SLOG_ERROR_LINE;
	}
	
	SLogClientConfig *pLogConfig = slog.GetSlogConfig(iConfigId);
	if(pLogConfig == NULL) {
		REQERR_LOG("not find log config:%d", iConfigId);
		stConfig.pErrMsg = CGI_REQERR;
		return SLOG_ERROR_LINE;
	}

	std::string strKeys;
	GetLogConfigTestKey(iConfigId, strKeys);
	if(strKeys.size() > 0)
		hdf_set_value(cgi->hdf, "config.test_keys", strKeys.c_str());
	else
		hdf_set_value(cgi->hdf, "config.test_keys", "null");
	return 0;
}

static int DealModConfig(CGI *cgi)
{
	const char *pdesc = hdf_get_value(cgi->hdf, "Query.config_desc", NULL);
	const char *pname = hdf_get_value(cgi->hdf, "Query.config_name", NULL);
	const char *pappName = hdf_get_value(cgi->hdf, "Query.app_name", NULL);
	int32_t iAppId = hdf_get_int_value(cgi->hdf, "Query.app_id", -1);
	int32_t iModuleId = hdf_get_int_value(cgi->hdf, "Query.module_id", -1);
	const char *pModuleName = hdf_get_value(cgi->hdf, "Query.module_name", NULL);
	int32_t iConfigId = hdf_get_int_value(cgi->hdf, "Query.config_id", -1);
	int32_t iLogType = hdf_get_int_value(cgi->hdf, "Query.log_type", -1);
	if(NULL == pname || iConfigId < 0 || iLogType < 0 || NULL == pappName || NULL == pModuleName
		|| iAppId < 0 || iModuleId < 0)
	{
		WARN_LOG("invalid parameter name:%s config id:%d logtype:%d", pname, iConfigId, iLogType);
		stConfig.pErrMsg = CGI_REQERR;
		return SLOG_ERROR_LINE;
	}
	
	SLogClientConfig *pLogConfig = slog.GetSlogConfig(iConfigId);
	if(pLogConfig == NULL) {
		REQERR_LOG("not find log config:%d", iConfigId);
		stConfig.pErrMsg = CGI_REQERR;
		return SLOG_ERROR_LINE;
	}

	hdf_set_value(cgi->hdf, "config.action", "save_mod_config");
	hdf_set_value(cgi->hdf, "config.app_name", pappName);
	hdf_set_value(cgi->hdf, "config.module_name", pModuleName);
	hdf_set_value(cgi->hdf, "config.config_name", pname);
	hdf_set_value(cgi->hdf, "config.config_desc", pdesc);
	hdf_set_int_value(cgi->hdf, "config.config_id", iConfigId);
	hdf_set_int_value(cgi->hdf, "config.log_type", iLogType);
	hdf_set_int_value(cgi->hdf, "config.app_id", iAppId);
	hdf_set_int_value(cgi->hdf, "config.module_id", iModuleId);

	int iFreq = 0, iFreqMax = 0;
	if(pLogConfig->dwSpeedFreq != 0)
		iFreq = (int)pLogConfig->dwSpeedFreq;

	if(iFreq != 0)
		hdf_set_int_value(cgi->hdf, "config.log_freq_limit", iFreq);
	hdf_set_int_value(cgi->hdf, "config.log_freq_limit_max", iFreqMax);

	std::string strKeys;
	GetLogConfigTestKey(iConfigId, strKeys);
	if(strKeys.size() > 0)
		hdf_set_value(cgi->hdf, "config.test_keys", strKeys.c_str());
	else
		hdf_set_value(cgi->hdf, "config.test_keys", "null");
	DEBUG_LOG("try modify config:%s - id:%d, freq:%d", pname, iConfigId, iFreq);
	return 0;
}

static int DealAddConfig(CGI *cgi)
{
	hdf_set_int_value(cgi->hdf, "config.log_freq_limit_max", 0);
	hdf_set_value(cgi->hdf, "config.action", "save_add_config");
	return 0;
}

static int GetLogConfigList(Json &js, SearchInfo *pinfo=NULL)
{
	char sSqlBuf[512] = {0};
	sprintf(sSqlBuf, "select * from mt_log_config where xrk_status=%d", RECORD_STATUS_USE);
	if(pinfo != NULL && AddSearchInfo(sSqlBuf, sizeof(sSqlBuf), pinfo) < 0)
		return SLOG_ERROR_LINE;
	strcat(sSqlBuf, " and module_id in (select distinct module_id from mt_module_info where xrk_status=0)");

	int iOrder = 0;
	iOrder = (iOrder==0 ? SetRecordsOrder(stConfig.cgi, sSqlBuf, "config_id") : 1);
	iOrder = (iOrder==0 ? SetRecordsOrder(stConfig.cgi, sSqlBuf, "config_name") : 1);
	iOrder = (iOrder==0 ? SetRecordsOrder(stConfig.cgi, sSqlBuf, "app_id") : 1);
	if(iOrder == 0) 
		strcat(sSqlBuf, " order by config_id desc");
	DEBUG_LOG("get log config list - exesql:%s", sSqlBuf);

	Query qu(*stConfig.db);
	qu.get_result(sSqlBuf);

	AppInfo *pAppInfo = NULL;
	TModuleInfo *pModuleInfo = NULL;
	int iConfigCount = 0;
	if(qu.num_rows() > 0) 
	{
		while(qu.fetch_row() != NULL)
		{
			Json cfg;
			cfg["config_id"] = qu.getuval("config_id");
			cfg["config_name"] = qu.getstr("config_name");
			cfg["config_desc"] = qu.getstr("config_desc");
			cfg["log_types"] = qu.getval("log_types");

			pAppInfo = slog.GetAppInfo(qu.getval("app_id"));
			if(pAppInfo != NULL && pAppInfo->iNameVmemIdx > 0)
				cfg["app_name"] = MtReport_GetFromVmem_Local(pAppInfo->iNameVmemIdx);
			else {
				ERR_LOG("get app name failed by app id :%d", qu.getval("app_id"));
				continue;
			}

			pModuleInfo = NULL;
			int iTmpModuleId = qu.getval("module_id");
			for(int i=0,j=0; i < SLOG_MODULE_COUNT_MAX_PER_APP && j < pAppInfo->wModuleCount; i++)
			{
				if(pAppInfo->arrModuleList[i].iModuleId == 0)
					continue;
				if(iTmpModuleId == pAppInfo->arrModuleList[i].iModuleId) {
					pModuleInfo = pAppInfo->arrModuleList+i;
					break;
				} 
				j++;
			}
			if(pModuleInfo != NULL && pModuleInfo->iNameVmemIdx > 0)
				cfg["module_name"] = MtReport_GetFromVmem_Local(pModuleInfo->iNameVmemIdx);
			else {
				ERR_LOG("get module name failed by module id :%u, app id:%d", iTmpModuleId, pAppInfo->iAppId);
				continue;
			}

			cfg["log_freq"] = qu.getval("write_speed");
			cfg["app_id"] = pAppInfo->iAppId;
			cfg["module_id"] = iTmpModuleId;
			cfg["user_add"] = qu.getstr("user_add");
			cfg["user_mod"] = qu.getstr("user_mod");
			cfg["update_time"] = qu.getstr("update_time");
			cfg["create_time"] = uitodate(qu.getuval("create_time"));
			js["configlist"].Add(cfg);
			iConfigCount++;
		}
	}
	js["config_count"] = iConfigCount;
	qu.free_result();
	DEBUG_LOG("get log config  count:%d", iConfigCount);
	return 0;
}

int DealListConfig(CGI *cgi)
{
	SearchInfo stInfo;
	memset(&stInfo, 0, sizeof(stInfo));
	stInfo.app_id = hdf_get_int_value(stConfig.cgi->hdf, "Query.dlc_app_id", 0);
	stInfo.module_id = hdf_get_int_value(stConfig.cgi->hdf, "Query.dlc_module_id", 0);

	Json js_app;
	if(GetAppModuleList(cgi, js_app) != 0)
		return SLOG_ERROR_LINE;
	std::string str_app(js_app.ToString());
	stConfig.err = hdf_set_value(stConfig.cgi->hdf, "config.app_module_list", str_app.c_str()); 
	if(stConfig.err != STATUS_OK)
	{
		ERR_LOG("set module list info failed, length:%u", (uint32_t)str_app.size());
		return SLOG_ERROR_LINE;
	}

	Json js;
	if(GetLogConfigList(js, &stInfo) < 0)
		return SLOG_ERROR_LINE;
	std::string str(js.ToString());
	stConfig.err = hdf_set_value(stConfig.cgi->hdf, "config.config_list", str.c_str()); 
	if(stConfig.err != STATUS_OK)
	{
		ERR_LOG("set config info failed, length:%u", (uint32_t)str.size());
		return SLOG_ERROR_LINE;
	}
	return 0;
}

static int GetAppListByIdx(int iAppCount, int iAppInfoIndexStart, Json &js)
{
	AppInfo *pApp = NULL;
	const char *pvname = NULL;
	int idx = iAppInfoIndexStart;
	int i = 0;
	for(; i < iAppCount; i++, idx=pApp->iNextIndex) 
	{
		pApp = slog.GetAppInfoByIndex(idx);
		if(pApp == NULL) {
			WARN_LOG("get appinfo failed, idx:%d", idx);
			break;
		}

		Json app;
		app["app_id"] = pApp->iAppId;
		pvname = NULL;
		if(pApp->iNameVmemIdx > 0)
			pvname = MtReport_GetFromVmem_Local(pApp->iNameVmemIdx);
		if(pvname != NULL)
			app["app_name"] = pvname;
		else
			app["app_name"] = "unknow";
		app["log_server_ip"] = ipv4_addr_str(pApp->dwAppSrvMaster);
		app["log_server_port"] = 80;
		js.Add(app);
	}
	DEBUG_LOG("get app info from shm, app count:%d", i);
	return i;
}

static int GetAppModuleListByIdx(int iAppCount, int iAppInfoIndexStart, Json &js)
{
	AppInfo *pApp = NULL;
	const char *pvname = NULL;
	int idx = iAppInfoIndexStart;
	int i = 0;
	for(; i < iAppCount; i++, idx=pApp->iNextIndex) 
	{
		pApp = slog.GetAppInfoByIndex(idx);
		if(pApp == NULL) {
			WARN_LOG("get appinfo failed, idx:%d", idx);
			break;
		}

		Json app;
		app["app_id"] = pApp->iAppId;
		pvname = NULL;
		if(pApp->iNameVmemIdx > 0)
			pvname = MtReport_GetFromVmem_Local(pApp->iNameVmemIdx);
		if(pvname != NULL)
			app["app_name"] = pvname;
		else
			app["app_name"] = "unknow";
		app["log_server_ip"] = ipv4_addr_str(pApp->dwAppSrvMaster);
		app["log_server_port"] = 80;
		int iModuleCount = 0;
		for(int j=0; iModuleCount < pApp->wModuleCount && j < SLOG_MODULE_COUNT_MAX_PER_APP; j++)
		{
			if(pApp->arrModuleList[j].iModuleId != 0) {
				iModuleCount++;
				Json module;
				module["id"] = pApp->arrModuleList[j].iModuleId;
				pvname = NULL;
				if(pApp->arrModuleList[j].iNameVmemIdx > 0)
					pvname = MtReport_GetFromVmem_Local(pApp->arrModuleList[j].iNameVmemIdx);
				if(pvname != NULL)
					module["name"] = pvname;
				else
					module["name"] = "unknow";
				app["modulelist"].Add(module);
			}
		}
		app["module_count"] = iModuleCount;
		js["applist"].Add(app);
	}
	DEBUG_LOG("get app/module info from shm, app count:%d", i);
	return i;
}

static int GetAppModuleList(CGI *cgi, Json & js)
{
	MtSystemConfig *pSysInfo = stConfig.stUser.pSysInfo;
	int iRet = 0;

	// try use vmem cache 
	if(!stConfig.iDisableVmemCache) {
		int iCount = 0;
		iCount = GetAppModuleListByIdx(pSysInfo->wAppInfoCount, pSysInfo->iAppInfoIndexStart, js);
		DEBUG_LOG("get user app/module info from shm, app count:%d", iCount);
		js["app_count"] = iCount;
		return 0;
	}

	int iAppCount=0, iModuleCount=0;
	char sSqlBuf[128] = {0};
	sprintf(sSqlBuf, "select app_id,app_name from mt_app_info where xrk_status=0 order by app_id");
	Query qu(*stConfig.db);
	qu.get_result(sSqlBuf);
	int iAppId = 0;
	AppInfo *pApp = NULL;
	Query qutmp(*stConfig.db);
	if(qu.num_rows() > 0) 
	{
		while(qu.fetch_row() != NULL)
		{
			Json app;
			iAppId = qu.getuval("app_id");
			pApp = slog.GetAppInfo(iAppId);
			if(NULL == pApp) {
				ERR_LOG("get appinfo failed, appid:%d", iAppId);
				continue;
			}
			 
			if((iRet=slog.CheckAppLogServer(pApp)) < 0) {
				ERR_LOG("CheckAppLogServer failed, ret:%d", iRet);
				continue;
			}

			app["app_name"] = qu.getstr("app_name");
			app["app_id"] = iAppId;
			app["log_server_ip"] = ipv4_addr_str(pApp->dwAppSrvMaster);
			app["log_server_port"] = 80;

			sprintf(sSqlBuf, 
				"select module_id,module_name from mt_module_info where app_id=%u and xrk_status=%d",
				qu.getuval("app_id"), RECORD_STATUS_USE);
			qutmp.get_result(sSqlBuf);
			if(qutmp.num_rows() > 0) {
				iModuleCount = 0;
				while(qutmp.fetch_row() != NULL)
				{
					Json module;
					module["id"] = qutmp.getuval("module_id"); 
					module["name"] = qutmp.getstr("module_name");
					app["modulelist"].Add(module);
					iModuleCount++;
				}
				app["module_count"] = iModuleCount;
			}
			else
				app["module_count"] = (uint32_t)0;
			qutmp.free_result();

			js["applist"].Add(app);
			iAppCount++;
		}
	}
	js["app_count"] = iAppCount;
	qu.free_result();
	return 0;
}

static int DealGetLogFiles(int iAppId, Json & js_files)
{
	logsearch.InitDefaultSearch();
	logsearch.SetLogPath(g_sLogPath);

	int iRet = logsearch.SetAppId(iAppId);
	SLogFile *plogFileShm = logsearch.GetLogFileShm();
	if(iRet < 0 || NULL == plogFileShm) {
		//应用没有产生日志的时候会走到这里
		INFO_LOG("GetLogFileShm failed, appid:%d, ret:%d", iAppId, iRet);
		return 0;
	}

	SLogFileInfo *plogFile= plogFileShm->stFiles;
	for(int i=plogFileShm->wLogFileCount-1; i >= 0; i--) 
	{
		if(plogFile[i].szAbsFileName[0] != '\0')
		{
			const char *plast = strrchr(plogFile[i].szAbsFileName, '/');
			if(plast != NULL)
				js_files.Add(plast+1);
			else
				js_files.Add(plogFile[i].szAbsFileName);
		}
	}
	return 0;
}

static int DealGetLogFilesForSearch(CGI *cgi)
{
	// 查询的 appid ---------------------
	int iAppId = hdf_get_int_value(cgi->hdf, "Query.appId", -1);
	if(iAppId < 0) {
		WARN_LOG("invalid appid - (%d)", iAppId);
		stConfig.pErrMsg = CGI_REQERR;
		return SLOG_ERROR_LINE;
	}

	Json js;
	if(DealGetLogFiles(iAppId, js) != 0)
		return SLOG_ERROR_LINE;

	STRING str;
	string_init(&str);
	if((stConfig.err=string_set(&str, js.ToString().c_str())) != STATUS_OK
		|| (stConfig.err=cgi_output(stConfig.cgi, &str)) != STATUS_OK)
	{
		string_clear(&str);
		stConfig.pErrMsg = CGI_ERR_SERVER;
		return SLOG_ERROR_LINE;
	}
	string_clear(&str);
	DEBUG_LOG("get app:%d file list ok", iAppId);
	return 0;
}

static int DealInitShowLog(CGI *cgi)
{
	Json js;
	if(GetAppModuleList(cgi, js) != 0)
		return SLOG_ERROR_LINE;

	std::string str(js.ToString());
	stConfig.err = hdf_set_value(stConfig.cgi->hdf, "config.app_module_list", str.c_str()); 
	if(stConfig.err != STATUS_OK)
	{
		ERR_LOG("set module list info failed, length:%u", (uint32_t)str.size());
		return SLOG_ERROR_LINE;
	}
	DEBUG_LOG("get appmodule info:%s", str.c_str());
	Json mach;

	// 开启 vmem 优先使用 vmem
	if(!stConfig.iDisableVmemCache) {
		if(GetUserMachineListFromVmem(mach) != 0)
			return SLOG_ERROR_LINE;
		DEBUG_LOG("get machine info from shm, count:%d", (int)(mach["mach_count"]));
	}
	else {
		Query qu(*stConfig.db);
		char sSql[256] = {0};
		snprintf(sSql, sizeof(sSql), 
			"select xrk_id,xrk_name from mt_machine where xrk_status=%d", RECORD_STATUS_USE);
		qu.get_result(sSql);
		if(qu.num_rows() > 0) {
			int iCount = 0;
			while(qu.fetch_row() != NULL)
			{
				Json mjs;
				mjs["id"] = qu.getval("xrk_id");
				mjs["name"] = qu.getstr("xrk_name");
				mach["mach_list"].Add(mjs);
				iCount++;
			}
			mach["mach_count"] = iCount;
		}
		else {
			mach["mach_count"] = 0;
		}
		qu.free_result();
	}

	str.assign(mach.ToString());
	stConfig.err = hdf_set_value(stConfig.cgi->hdf, "config.report_machines", str.c_str()); 
	if(stConfig.err != STATUS_OK)
	{
		ERR_LOG("set machine list info failed, length:%u", (uint32_t)str.size());
		return SLOG_ERROR_LINE;
	}

	int iAppid = hdf_get_int_value(stConfig.cgi->hdf, "Query.app_id", 0);
	if(iAppid != 0)
		hdf_set_int_value(stConfig.cgi->hdf, "config.qu_app_id", iAppid);

	const char *plogfile = hdf_get_value(stConfig.cgi->hdf, "Query.logfile", NULL);
	if(plogfile != NULL)
		hdf_set_value(stConfig.cgi->hdf, "config.qu_log_file", plogfile);
	return 0;
}

int DealListModule(CGI *cgi)
{
	int app_id = hdf_get_int_value(stConfig.cgi->hdf, "Query.dm_app_id", 0);
	int module_id = hdf_get_int_value(stConfig.cgi->hdf, "Query.dm_module_id", 0);
	hdf_set_int_value(stConfig.cgi->hdf, "config.dm_app_id", app_id);
	hdf_set_int_value(stConfig.cgi->hdf, "config.dm_module_id", module_id);

	char sSqlBuf[256] = {0};
	Query qu(*stConfig.db);

	sprintf(sSqlBuf, "select * from mt_app_info where xrk_status=0 order by app_id desc");
	qu.get_result(sSqlBuf);
	Json js;
	js["statusCode"] = 200;
	int iAppCount=0, iModuleCount=0, iModuleTotalCount = 0;
	int iSelfAppCount = 0;
	Query qutmp(*stConfig.db);
	if(qu.num_rows() > 0) 
	{
		while(qu.fetch_row() != NULL)
		{
			Json app;
			app["app_id"] = qu.getuval("app_id"); 
			app["app_name"] = qu.getstr("app_name");
			sprintf(sSqlBuf, 
				"select * from mt_module_info where app_id=%u and xrk_status=%d order by module_id desc",
				qu.getuval("app_id"), RECORD_STATUS_USE);
			qutmp.get_result(sSqlBuf);
			iModuleCount=0;
			if(qutmp.num_rows() > 0) {
				while(qutmp.fetch_row() != NULL)
				{
					Json module;
					module["id"] = qutmp.getuval("module_id"); 
					module["desc"] = qutmp.getstr("module_desc");
					module["name"] = qutmp.getstr("module_name");
					module["user_add"] = qutmp.getstr("user_add");
					module["user_mod"] = qutmp.getstr("user_mod");
					module["create_time"] = qutmp.getstr("create_time");
					module["mod_time"] = qutmp.getstr("mod_time");
					app["modulelist"].Add(module);
					iModuleCount++;
				}
				app["module_count"] = iModuleCount;
				iModuleTotalCount += iModuleCount;
			}
			else
				app["module_count"] = (uint32_t)0;
			qutmp.free_result();

			js["applist"].Add(app);
			iAppCount++;
		}
	}
	js["app_count"] = iAppCount;
	js["self_app_count"] = iSelfAppCount;
	qu.free_result();

	std::string str(js.ToString());
	DEBUG_LOG("module list app count:%d json:%s", iAppCount, str.c_str());

	stConfig.err = hdf_set_value(stConfig.cgi->hdf, "config.module_list", str.c_str()); 
	if(stConfig.err != STATUS_OK)
	{
		ERR_LOG("set module list info failed, length:%u", (uint32_t)str.size());
		return SLOG_ERROR_LINE;
	}
	return 0;
}

uint32_t GetLogSpaceUse(uint32_t &dwLogSpaceB, std::multimap<uint32_t, AppInfo*, greater<uint32_t> > & mpApp)
{
	MtSystemConfig *pSysInfo = stConfig.stUser.pSysInfo;
	AppInfo *pApp = NULL;
	uint32_t dwUseTotal = 0;

	int idx = pSysInfo->iAppInfoIndexStart;
	for(int i=0; i < pSysInfo->wAppInfoCount; i++, idx=pApp->iNextIndex) 
	{
		pApp = slog.GetAppInfoByIndex(idx);
		if(pApp == NULL) {
			WARN_LOG("get appinfo failed, idx:%d", idx);
			break;
		}

		// 说明: 跨机的app 信息会同步到本机，直接读取即可
		if(false == pApp->bReadLogStatInfo) {
			WARN_LOG("app:%d not read log stat", pApp->iAppId);
			continue;
		}
		dwLogSpaceB += (uint32_t)(pApp->stLogStatInfo.qwLogSizeInfo);
		uint32_t dwDiskKb = round((float)pApp->stLogStatInfo.qwLogSizeInfo/1024);
		if(dwDiskKb <= 0)
			continue;
		mpApp.insert(make_pair(dwDiskKb, pApp));
		dwUseTotal += dwDiskKb;
		DEBUG_LOG("get app:%d use space:%uKB - %luB", pApp->iAppId, dwDiskKb, pApp->stLogStatInfo.qwLogSizeInfo);
	}
	DEBUG_LOG("get app total disk use:%ukb, %u:B", dwUseTotal, dwLogSpaceB); 
	return dwUseTotal;
}

int GetModuleCountTotal()
{
	MtSystemConfig *pSysInfo = stConfig.stUser.pSysInfo;
	AppInfo *pApp = NULL;
	int iCount = 0;
	int idx = pSysInfo->iAppInfoIndexStart;
	for(int i=0; i < pSysInfo->wAppInfoCount; i++, idx=pApp->iNextIndex) 
	{
		pApp = slog.GetAppInfoByIndex(idx);
		if(pApp == NULL) {
			WARN_LOG("get appinfo failed, idx:%d", idx);
			continue;
		}
		iCount += pApp->wModuleCount;
		DEBUG_LOG("get app:%d module count:%d", pApp->iAppId, pApp->wModuleCount);
	}
	DEBUG_LOG("get app module count:%d", iCount);
	return iCount;
}

uint32_t GetLogTotalRecords()
{
	MtSystemConfig *pSysInfo = stConfig.stUser.pSysInfo;
	AppInfo *pApp = NULL;
	uint32_t dwCount = 0, dwTotal = 0;

	int idx = pSysInfo->iAppInfoIndexStart;
	for(int i=0; i < pSysInfo->wAppInfoCount; i++, idx=pApp->iNextIndex) 
	{
		pApp = slog.GetAppInfoByIndex(idx);
		if(pApp == NULL) {
			WARN_LOG("get appinfo failed, idx:%d", idx);
			continue;
		}
		if(false == pApp->bReadLogStatInfo) {
			WARN_LOG("app:%d not read log stat", pApp->iAppId);
			continue;
		}

		dwCount = pApp->stLogStatInfo.dwDebugLogsCount;
		dwCount += pApp->stLogStatInfo.dwInfoLogsCount;
		dwCount += pApp->stLogStatInfo.dwWarnLogsCount;
		dwCount += pApp->stLogStatInfo.dwReqerrLogsCount;
		dwCount += pApp->stLogStatInfo.dwErrorLogsCount;
		dwCount += pApp->stLogStatInfo.dwFatalLogsCount;
		dwCount += pApp->stLogStatInfo.dwOtherLogsCount;
		dwTotal += dwCount;
		DEBUG_LOG("get app:%d log records:%u", pApp->iAppId, dwCount);
	}
	DEBUG_LOG("get total log records:%u", dwTotal);
	return dwTotal;
}

void GetAttrInfoByTree(
	std::multimap<int, AttrTypeInfo*, greater<int> > & mpAttr, MmapUserAttrTypeTree & stTypeTree)
{
	AttrTypeInfo *pInfo = NULL;
	pInfo = slog.GetAttrTypeInfo(stTypeTree.attr_type_id(), NULL);
	if(pInfo != NULL && pInfo->wAttrCount > 0)
		mpAttr.insert(make_pair(pInfo->wAttrCount, pInfo));

	for(int i=0; i < stTypeTree.sub_type_list_size(); i++)
	{
		MmapUserAttrTypeTree *pType = stTypeTree.mutable_sub_type_list(i);
		GetAttrInfoByTree(mpAttr, *pType);
	}
}

int DealRefreshMainInfo(CGI *cgi)
{
	// 涉及日志服务器可能需要跨站
	hdf_set_value(cgi->hdf, "cgiout.other.cros", "Access-Control-Allow-Origin:*");

	Json js;
	js["statusCode"] = 0;

	char szTmp[256] = {0};
	Query & qu = *(stConfig.qu);

	// 处理中告警
	snprintf(szTmp, sizeof(szTmp),
	    "select count(*) from mt_warn_info where xrk_status=%d and deal_status=1", RECORD_STATUS_USE);
	if(!qu.get_result(szTmp) || qu.num_rows() <= 0) {
	    WARN_LOG("get warn dealing count failed !");
	    js["warn_dealings"] = 0;
	}
	else {
	    qu.fetch_row();
	    js["warn_dealings"] = (int)(qu.getval(0));
	}
	qu.free_result();
	
	// 未处理告警
	snprintf(szTmp, sizeof(szTmp),
	    "select count(*) from mt_warn_info where xrk_status=%d and deal_status=0", RECORD_STATUS_USE);
	if(!qu.get_result(szTmp) || qu.num_rows() <= 0) {
	    WARN_LOG("get warn undeal count failed !");
	    js["warn_undeals"] = 0;
	}
	else {
	    qu.fetch_row();
	    js["warn_undeals"] = (int)(qu.getval(0));
	}
	qu.free_result();

	MtSystemConfig *pSysInfo = stConfig.stUser.pSysInfo;
	js["attr_count"] = pSysInfo->wAttrCount;
	js["app_count"] = pSysInfo->wAppInfoCount;
	js["machine_count"] = pSysInfo->wMachineCount;
	js["log_records"] = GetLogTotalRecords();

	// 监控点分布
	MmapUserAttrTypeTree stTypeTree;
	if(GetAttrTypeTreeFromVmem(stConfig, stTypeTree) >= 0) {
		DEBUG_LOG("get user attr type tree from vmem ok");
	}
	else {
		// 可能未创建监控点类型
		WARN_LOG("get user attr type tree from vmem failed, may have no attr type");
	}
	std::multimap<int, AttrTypeInfo*, greater<int> > mpAttr;
	GetAttrInfoByTree(mpAttr, stTypeTree);
	std::multimap<int, AttrTypeInfo*, greater<int> >::iterator it = mpAttr.begin();
	std::string strAttrInfo;
	int iAttrCount = 0;
	static std::string s_strType("监控点类型-");
	for(int i=0; i < 7 && it != mpAttr.end(); it++, i++)
	{
		Json jsattr;
		AttrTypeInfo *pInfo = it->second;
		const char *pTypeName = NULL;
		if(!stConfig.iDisableVmemCache)
			pTypeName = GetAttrTypeNameFromVmem(pInfo->id, stConfig);
		if(pTypeName == NULL)
			jsattr["name"] = s_strType+itoa(pInfo->id);
		else
			jsattr["name"] = std::string(pTypeName) + "(" + itoa(pInfo->id)+")";
		jsattr["value"] = pInfo->wAttrCount;
		iAttrCount += pInfo->wAttrCount;
		js["attr_info"].Add(jsattr);
		DEBUG_LOG("get attr type:%d, attr count:%d", pInfo->id, pInfo->wAttrCount);
	}
	if(pSysInfo->wAttrCount > iAttrCount) {
		Json jsattr;
		jsattr["name"] = "其它类型";
		jsattr["value"] = pSysInfo->wAttrCount - iAttrCount;
		js["attr_info"].Add(jsattr);
	}

	// 磁盘空间使用分布
	std::multimap<uint32_t, AppInfo*, greater<uint32_t> > mpApp;
	uint32_t dwLogSpaceB = 0;
	uint32_t dwLogSpaceUseKB = GetLogSpaceUse(dwLogSpaceB, mpApp);
	js["log_space_kb"] = dwLogSpaceUseKB;
	js["log_space_b"] = dwLogSpaceB;
	std::multimap<uint32_t, AppInfo*, greater<uint32_t> >::iterator itApp = mpApp.begin();
	std::string strAppDiskInfo;
	uint32_t dwAppDiskUse = 0;
	for(int i=0; i < 7 && itApp != mpApp.end(); itApp++, i++)
	{
		Json jsapp;
		AppInfo* pInfo = itApp->second;
		const char *pName = NULL;
		if(!stConfig.iDisableVmemCache)
			pName = MtReport_GetFromVmem_Local(pInfo->iNameVmemIdx);
		if(pName == NULL)
			jsapp["name"] = std::string("应用-")+itoa(pInfo->iAppId);
		else
			jsapp["name"] = pName;
		jsapp["value"] = itApp->first;
		dwAppDiskUse += itApp->first;
		js["app_disk_info"].Add(jsapp);
		DEBUG_LOG("get app:%d, disk use:%u KB", pInfo->iAppId, itApp->first);
	}
	if(dwLogSpaceUseKB > dwAppDiskUse) {
		Json jsapp;
		jsapp["name"] = "其它应用";
		jsapp["value"] = dwLogSpaceUseKB-dwAppDiskUse;
		js["app_disk_info"].Add(jsapp);
	}

	int iModules = GetModuleCountTotal();
	js["module_count"] = iModules;

	snprintf(szTmp, sizeof(szTmp), "%d,%d,%d,%d,%u", pSysInfo->wAppInfoCount, 
		iModules, iAttrCount, pSysInfo->wMachineCount, (uint32_t)round((float)dwLogSpaceUseKB/1024));
	js["resource_use"] = szTmp;

	STRING str;
	string_init(&str);
	if((stConfig.err=string_set(&str, js.ToString().c_str())) != STATUS_OK
		|| (stConfig.err=cgi_output(cgi, &str)) != STATUS_OK)
	{
		string_clear(&str);
		return -1;
	}
	string_clear(&str);
	DEBUG_LOG("refresh log info response:%s", js.ToString().c_str());
	return 0;
}

void StringReplaceAll(std::string &str, const char *pstrSrc, const char *pstrDst)
{
	size_t pos;
	size_t s_len = strlen(pstrSrc);
	while((pos=str.find(pstrSrc)) != std::string::npos)
		str.replace(pos, s_len, pstrDst); 
}

int DealListApp(CGI *cgi)
{
	char sSqlBuf[128] = {0};
	Query qu(*stConfig.db);

	sprintf(sSqlBuf, "select * from mt_app_info where xrk_status=0 order by app_id desc");
	qu.get_result(sSqlBuf);
	Json js;
	js["statusCode"] = 200;
	int i=0;

	if(qu.num_rows() > 0) 
	{
		while(qu.fetch_row() != NULL)
		{
			Json app;
			app["id"] = qu.getuval("app_id"); 
			app["name"] = qu.getstr("app_name");
			app["desc"] = qu.getstr("app_desc");
			app["create_time"] = uitodate(qu.getuval("create_time"));
			app["update_time"] = qu.getstr("update_time");
			app["user_add"] = qu.getstr("user_add");
			app["user_mod"] = qu.getstr("user_mod");
			js["applist"].Add(app);
			i++;
		}
	}
	js["app_count"] = i;
	qu.free_result();

	std::string str(js.ToString());
	DEBUG_LOG("app list count:%d json:%s", i, str.c_str());

	stConfig.err = hdf_set_value(stConfig.cgi->hdf, "config.app_list", str.c_str()); 
	if(stConfig.err != STATUS_OK)
	{
		ERR_LOG("set app list info failed, length:%u", (uint32_t)str.size());
		return SLOG_ERROR_LINE;
	}
	return 0;
}

static int DealGetLogFilesStart(CGI *cgi)
{
	MtSystemConfig *pSysInfo = stConfig.stUser.pSysInfo;
	Json js;
	if(!stConfig.iDisableVmemCache) 
	{
		int iCount = 0;
		iCount = GetAppListByIdx(pSysInfo->wAppInfoCount, pSysInfo->iAppInfoIndexStart, js);
		DEBUG_LOG("get user app info from shm, app count:%d", iCount);
	}
	else 
	{
		char sSqlBuf[256] = {0};
		Query qu(*stConfig.db);
		sprintf(sSqlBuf, 
			"select app_id,app_name from mt_app_info where xrk_status=%d", RECORD_STATUS_USE);
		qu.get_result(sSqlBuf);

		AppInfo *pApp = NULL;
		int iAppId = 0, iRet = 0;
		if(qu.num_rows() > 0) 
		{
			while(qu.fetch_row() != NULL) 
			{
				Json app;
				iAppId = qu.getuval("app_id");
				pApp = slog.GetAppInfo(iAppId);
				if(NULL == pApp) {
					ERR_LOG("get appinfo failed, appid:%d", iAppId);
					continue;
				}
				 
				if((iRet=slog.CheckAppLogServer(pApp)) < 0) {
					ERR_LOG("CheckAppLogServer failed, ret:%d", iRet);
					continue;
				}

				app["app_id"] = qu.getuval("app_id"); 
				app["app_name"] = qu.getstr("app_name");
				app["log_server_ip"] = ipv4_addr_str(pApp->dwAppSrvMaster);
				app["log_server_port"] = 80;
				js.Add(app);
			}
		}
		qu.free_result();
	}

	std::string str(js.ToString());
	stConfig.err = hdf_set_value(stConfig.cgi->hdf, "config.dfl_app_list", str.c_str()); 
	if(stConfig.err != STATUS_OK)
	{
		ERR_LOG("set app list info failed, length:%u", (uint32_t)str.size());
		return SLOG_ERROR_LINE;
	}
	return 0;
}

static char * GetTimeString(uint64_t qwTime)
{
	static char s_szTmpBuf[64];

	time_t tmLog = qwTime/1000000;
	struct tm *tmp = localtime(&tmLog);
	int iFailed = 0;
	if(tmp == NULL) {
		ERR_LOG("localtime failed, msg:%s", strerror(errno));
		iFailed = -1;
	}
	else {
		if(strftime(s_szTmpBuf, sizeof(s_szTmpBuf), "%Y-%m-%d %H:%M:%S", tmp) == 0){
			ERR_LOG("strftime failed, msg:%s", strerror(errno));
			iFailed = -1;
		}
	}
	if(iFailed < 0)
		snprintf(s_szTmpBuf, sizeof(s_szTmpBuf), "%" PRIu64, qwTime);
	return s_szTmpBuf;
}

static int DealGetLogFiles(CGI *cgi)
{
	int32_t iAppId = hdf_get_int_value(cgi->hdf, "Query.app_id", -1);
	if(iAppId < 0) {
		WARN_LOG("invalid appid - (%d)", iAppId);
		stConfig.pErrMsg = CGI_REQERR;
		return SLOG_ERROR_LINE;
	}

	logsearch.InitDefaultSearch();
	logsearch.SetLogPath(g_sLogPath);
	int iRet = logsearch.SetAppId(iAppId);
	SLogFile *plogFileShm = logsearch.GetLogFileShm();
	if(iRet < 0 || NULL == plogFileShm) {
		WARN_LOG("GetLogFileShm failed, appid:%d, ret:%d !", iAppId, iRet);

		//应用没有产生日志的时候可能会走到这里
		Json js;
		js["app_id"] = iAppId;
		js["totalCount"] = 0;
		js["numPerPage"] = 20;
		js["currentPage"] = 1;
		js["pageNumShown"] = 10;
		js["logtotal"] = 0;
		js["logsize"] = 0;
		STRING str_cgi;
		string_init(&str_cgi);
		if((stConfig.err=string_set(&str_cgi, js.ToString().c_str())) != STATUS_OK
			|| (stConfig.err=cgi_output(cgi, &str_cgi)) != STATUS_OK)
		{
			string_clear(&str_cgi);
			return SLOG_ERROR_LINE;
		}
		string_clear(&str_cgi);
		return 0;
	}

	SetRecordsPageInfo(cgi, plogFileShm->wLogFileCount, 20);

	int iCurPage = hdf_get_int_value(stConfig.cgi->hdf, "config.currentPage", 0);
	int iNumPerPage = hdf_get_int_value(stConfig.cgi->hdf, "config.numPerPage", 0);
	int iNumShown = hdf_get_int_value(cgi->hdf, "config.pageNumShown", 0);
	if((iCurPage-1)*iNumPerPage > plogFileShm->wLogFileCount || iNumShown==0) {
		REQERR_LOG("invalid page info - curpage:%d numper:%d, max records:%d iNumShown:%d",
			iCurPage, iNumPerPage, plogFileShm->wLogFileCount, iNumShown);
		stConfig.pErrMsg = CGI_REQERR;
		return SLOG_ERROR_LINE;
	}

	Json js;
	js["app_id"] = iAppId;
	js["totalCount"] = plogFileShm->wLogFileCount;
	js["numPerPage"] = iNumPerPage;
	js["currentPage"] = iCurPage;
	js["pageNumShown"] = iNumShown;

	char szTmpBuf[32];
	SLogFileInfo *plogFile= plogFileShm->stFiles; 

	uint64_t qwLogSize = 0;
	AppInfo * pAppInfo = slog.GetAppInfo(iAppId);
	qwLogSize = pAppInfo->stLogStatInfo.qwLogSizeInfo;
	int iLogTotal = 0;
	for(int i=plogFileShm->wLogFileCount-1; i >= 0; i--)
		iLogTotal += plogFile[i].stFileHead.iLogRecordsWrite;
	js["logtotal"] = iLogTotal;

	static const int i_G = 1024*1024*1024;
	static const int i_M = 1024*1024;
	if(qwLogSize >= i_G)
		snprintf(szTmpBuf, sizeof(szTmpBuf), "%.2f G", (float)qwLogSize/i_G);
	else if(qwLogSize >= i_M)
		snprintf(szTmpBuf, sizeof(szTmpBuf), "%.2f M", (float)qwLogSize/i_M);
	else 
		snprintf(szTmpBuf, sizeof(szTmpBuf), "%.2f K", (float)qwLogSize/1024);
	js["logsize"] = szTmpBuf;

	for(int j=(iCurPage-1)*iNumPerPage; j < iCurPage*iNumPerPage && j < plogFileShm->wLogFileCount; j++)
	{
		// 按时间排序, 当前时间靠前
		int i = plogFileShm->wLogFileCount-j-1;
		if(i < 0)
			break;

		Json file;
		file["name"] = plogFile[i].szAbsFileName;
		if(plogFile[i].stFileHead.stLogStatInfo.qwLogSizeInfo >= i_M)
			snprintf(szTmpBuf, sizeof(szTmpBuf), "%.2f M", 
				(float)(plogFile[i].stFileHead.stLogStatInfo.qwLogSizeInfo)/i_M);
		else
			snprintf(szTmpBuf, sizeof(szTmpBuf), "%.2f K", 
				(float)(plogFile[i].stFileHead.stLogStatInfo.qwLogSizeInfo)/1024);
		file["size"] = szTmpBuf; 

		file["records"] = plogFile[i].stFileHead.iLogRecordsWrite;
		file["time_start"] = GetTimeString(plogFile[i].stFileHead.qwLogTimeStart);
		file["time_end"] = GetTimeString(plogFile[i].stFileHead.qwLogTimeEnd);
		file["version"] = plogFile[i].stFileHead.bLogFileVersion;
		const char *plast = strrchr(plogFile[i].szAbsFileName, '/');
		if(plast != NULL)
			file["fname"] = plast+1;
		else
			file["fname"] = plogFile[i].szAbsFileName;
		js["list"].Add(file);
	}

	std::string str(js.ToString());
	stConfig.err = hdf_set_value(stConfig.cgi->hdf, "config.dfl_app_list", str.c_str()); 
	if(stConfig.err != STATUS_OK)
	{
		ERR_LOG("set app list info failed, length:%u", (uint32_t)str.size());
		return SLOG_ERROR_LINE;
	}

	STRING str_cgi;
	string_init(&str_cgi);
	if((stConfig.err=string_set(&str_cgi, str.c_str())) != STATUS_OK
		|| (stConfig.err=cgi_output(cgi, &str_cgi)) != STATUS_OK)
	{
		string_clear(&str_cgi);
		return SLOG_ERROR_LINE;
	}
	string_clear(&str_cgi);
	return 0;
}

static void OnAppHaveNoLog(CGI *cgi, bool bRealtime=true)
{
	uint32_t dwReqNo = hdf_get_int_value(cgi->hdf, "Query.RequestNo", 1);
	Json js;
	js["RequestNo"] = (unsigned int)dwReqNo;

	if(bRealtime) {
		js["ec"] = (unsigned int)0;
		js["Send"] = 0;
		js["Rece"] = 0;
	}
	else {
		js["FileCount"] = 0;
		js["FileNo"] = 0;
		js["FilePos"] = 0;
		js["Percentage"] = 100;
		js["FileIndexStar"] = 0;
		js["ec"] = (short)CHECK_RESULT_SEARCH_HISLOG_COMPLETE;
	}

	STRING str;
	string_init(&str);
	if((stConfig.err=string_set(&str, js.ToString().c_str())) != STATUS_OK
		|| (stConfig.err=cgi_output(cgi, &str)) != STATUS_OK)
	{
		string_clear(&str);
	}
	DEBUG_LOG("app may have no log");
}

static int DealGetHistoryLog(CGI *cgi)
{
	logsearch.InitDefaultSearch();
	if(SetSearchPara(cgi, logsearch) < 0)
	{
		if(CHECK_RESULT_NOT_FIND_APP_LOG == stConfig.iErrorCode)
		{
			OnAppHaveNoLog(cgi, false);
			return 0;
		}
		return -101;
	}

	uint32_t dwReqNo = hdf_get_uint_value(cgi->hdf, "Query.RequestNo", 1);

	DEBUG_LOG("get history log, file count:%d, file start index:%d, file no:%d, file pos:%d, reqno:%u",
		logsearch.GetFileCount(), logsearch.GetFileIndexStar(), 
		logsearch.GetFileNo(), logsearch.GetFilePos(), dwReqNo);

	Json js;
	js["RequestNo"] = (unsigned int)dwReqNo;
	TSLogOut *pstLog = NULL;

	int32_t i = 0;
	char *pszLogEscp = NULL;
	int32_t iGetRecords = hdf_get_int_value(cgi->hdf, "Query.ShowPageRecords", 10);
	int32_t iShowLogField = logsearch.GetLogField();

	for(i=0; i < iGetRecords && (pstLog=logsearch.HistoryLog()) != NULL;)
	{
		// fix bug @ 2020-05-07 -by rockdeng
		//stConfig.err = cgi_js_escape(pstLog->pszLog, &pszLogEscp);
		//if(stConfig.err != STATUS_OK)
		//	return -1;
		pszLogEscp = pstLog->pszLog;

		i++;
		char *ptmp = strchr(pszLogEscp, '[');
		const char *pszLogContent = NULL; 
		const char *pszLogPos = NULL;
		char *pszLogTime = NULL;
		if(ptmp != NULL){
			pszLogPos = ptmp+1;
			*ptmp = '\0';
			pszLogTime = pszLogEscp; // 时间字符串
			pszLogEscp = (char*)pszLogPos;
			ptmp = strchr(pszLogEscp, ']');
			if(ptmp != NULL) {
				pszLogContent = ptmp+1; // 日志内容
				*ptmp = '\0';
				pszLogPos = pszLogEscp; // Log 位置信息
			}
			else {
				pszLogContent = pszLogPos; 
				pszLogPos = "get failed";  
			}
		}
		else {
			pszLogContent = pszLogEscp;
			pszLogPos = "get failed";
			pszLogTime = qwtoa(pstLog->qwLogTime);
		}

		Json log;
		if(iShowLogField & SLOG_FIELD_TIME)
			log["time"] = pszLogTime;
		if(iShowLogField & SLOG_FIELD_ADDR)
		{
			MachineInfo *pInfo = slog.GetMachineInfo(pstLog->dwLogHost, NULL);
			if(pInfo != NULL && pInfo->iNameVmemIdx > 0)
				log["addr"] = MtReport_GetFromVmem_Local(pInfo->iNameVmemIdx);
			else
				log["addr"] = "unknow";
		}
		if(iShowLogField & SLOG_FIELD_CUST_1) {
			if(pstLog->bCustFlag & MTLOG_CUST_FLAG_C1_SET)
				log["cust_1"] = pstLog->dwCust_1;
			else
				log["cust_1"] = g_strCustLogHaveNo;
		}

		if(iShowLogField & SLOG_FIELD_CUST_2) {
			if(pstLog->bCustFlag & MTLOG_CUST_FLAG_C2_SET)
				log["cust_2"] = pstLog->dwCust_2;
			else
				log["cust_2"] = g_strCustLogHaveNo;
		}

		if(iShowLogField & SLOG_FIELD_CUST_3) {
			if(pstLog->bCustFlag & MTLOG_CUST_FLAG_C3_SET)
				log["cust_3"] = pstLog->iCust_3;
			else
				log["cust_3"] = g_strCustLogHaveNo;
		}

		if(iShowLogField & SLOG_FIELD_CUST_4) {
			if(pstLog->bCustFlag & MTLOG_CUST_FLAG_C4_SET)
				log["cust_4"] = pstLog->iCust_4;
			else
				log["cust_4"] = g_strCustLogHaveNo;
		}

		if(iShowLogField & SLOG_FIELD_CUST_5) {
			if((pstLog->bCustFlag & MTLOG_CUST_FLAG_C5_SET) && pstLog->szCust_5[0] != '\0')
				log["cust_5"] = pstLog->szCust_5;
			else
				log["cust_5"] = g_strCustLogHaveNo;
		}

		if(iShowLogField & SLOG_FIELD_CUST_6) {
			if((pstLog->bCustFlag & MTLOG_CUST_FLAG_C6_SET) && pstLog->szCust_6[0] != '\0')
				log["cust_6"] = pstLog->szCust_6;
			else
				log["cust_6"] = g_strCustLogHaveNo;
		}

		if(iShowLogField & SLOG_FIELD_APP)
			log["app"] = pstLog->iAppId;
		if(iShowLogField & SLOG_FIELD_MODULE)
			log["module"] = pstLog->iModuleId;
		if(iShowLogField & SLOG_FIELD_CONTENT)
			log["Content"] = pszLogContent;
		if(iShowLogField & SLOG_FIELD_CONFIG)
			log["config"] = pstLog->dwLogConfigId;
		if(iShowLogField & SLOG_FIELD_POS)
			log["pos"] = pszLogPos;

		log["seq"] = pstLog->dwLogSeq;
		log["type"] = pstLog->wLogType;
		log["s_time"] = pstLog->qwLogTime;
		js["list"].Add(log);
	}

	js["FileCount"] = (unsigned int)(logsearch.GetFileCount());
	js["FileNo"] = (unsigned int)(logsearch.GetFileNo());
	js["FilePos"] = (unsigned int)(logsearch.GetFilePos());
	js["Percentage"] = (unsigned int)(logsearch.GetSearchCurFilePercent());
	js["FileIndexStar"] = (unsigned int)(logsearch.GetFileIndexStar());
	js["LogField"] = iShowLogField;

	if(logsearch.IsSearchHistoryComplete())
		js["ec"] = (short)CHECK_RESULT_SEARCH_HISLOG_COMPLETE;
	else
		js["ec"] = (short)CHECK_RESULT_SUCCESS;

	STRING str;
	string_init(&str);
	if((stConfig.err=string_set(&str, js.ToString().c_str())) != STATUS_OK
		|| (stConfig.err=cgi_output(cgi, &str)) != STATUS_OK)
	{
		string_clear(&str);
		return -2;
	}
	string_clear(&str);
	return 0;
}

static int DealGetRealTimeLog(CGI *cgi)
{
	logsearch.InitDefaultSearch();
	if(SetSearchParaComm(cgi, logsearch) < 0)
	{
		if(CHECK_RESULT_NOT_FIND_APP_LOG == stConfig.iErrorCode)
		{
			OnAppHaveNoLog(cgi);
			return 0;
		}
		return SLOG_ERROR_LINE;
	}

	uint32_t dwReqNo = hdf_get_uint_value(cgi->hdf, "Query.RequestNo", 1);
	uint32_t dwLastSeqNo = hdf_get_uint_value(cgi->hdf, "Query.RequestSeq", 0);
	int32_t iMaxCount = hdf_get_int_value(cgi->hdf, "Query.MaxResultCount", 200);
	TSLogShm *pShmLog = logsearch.GetLogShm();
	if(iMaxCount > REALTIME_LOG_GET_MAX)
		iMaxCount = REALTIME_LOG_GET_MAX;

	DEBUG_LOG("get realtime log, req no:%u, req seq:%u", dwReqNo, dwLastSeqNo);

	Json js;
	js["ec"] = (unsigned int)0;
	js["RequestNo"] = (unsigned int)dwReqNo;
	TSLogOut *pstLog = NULL;

	int32_t i = 0;
	int32_t iLastIndex = -1;
	int32_t iShowLogField = logsearch.GetLogField();
	struct timeval stNow;
	gettimeofday(&stNow, 0);
	uint64_t qwTimeNow = TIME_SEC_TO_USEC(stNow.tv_sec)+stNow.tv_usec;
	char *pszLogEscp = NULL;

	uint32_t dwInnerSeq = 0;
	for(i=0; pShmLog != NULL
		&& (pstLog=logsearch.RealTimeLog(iLastIndex, dwLastSeqNo, qwTimeNow)) != NULL;)
	{
		if(++i < iMaxCount && (dwInnerSeq > pstLog->dwLogSeq || dwInnerSeq == 0))
		{
			//stConfig.err = cgi_js_escape(pstLog->pszLog, (char**)&pszLogEscp);
			//if(stConfig.err != STATUS_OK)
			//	return -1;
			pszLogEscp = pstLog->pszLog;

			char *ptmp = (char*)strchr(pszLogEscp, '[');
			char *pszLogContent = NULL; 
			const char *pszLogPos = NULL;
			char *pszLogTime = NULL;
			if(ptmp != NULL){
				pszLogPos = ptmp+1;
				*ptmp = '\0';
				pszLogTime = pszLogEscp; // 时间字符串
				pszLogEscp = (char*)pszLogPos;
				ptmp = (char*)strchr(pszLogEscp, ']');
				if(ptmp != NULL) {
					pszLogContent = ptmp+1; // 日志内容
					*ptmp = '\0';
					pszLogPos = pszLogEscp; // Log 位置信息
				}
				else {
					pszLogContent = (char*)pszLogPos; 
					pszLogPos = "get failed";  
				}
			}
			else {
				pszLogContent = (char*)pszLogEscp;
				pszLogPos = "get failed";
				pszLogTime = qwtoa(pstLog->qwLogTime);
			}

			Json log;
			if(iShowLogField & SLOG_FIELD_TIME)
				log["time"] = pszLogTime;
			if(iShowLogField & SLOG_FIELD_ADDR)
			{
				MachineInfo *pInfo = slog.GetMachineInfo(pstLog->dwLogHost, NULL);
				if(pInfo != NULL && pInfo->iNameVmemIdx > 0)
					log["addr"] = MtReport_GetFromVmem_Local(pInfo->iNameVmemIdx);
				else
					log["addr"] = "unknow";
				log["addr_id"] = pstLog->dwLogHost;
			}
			if(iShowLogField & SLOG_FIELD_CUST_1) {
				if(pstLog->bCustFlag & MTLOG_CUST_FLAG_C1_SET)
					log["cust_1"] = pstLog->dwCust_1;
				else
					log["cust_1"] = g_strCustLogHaveNo;
			}

			if(iShowLogField & SLOG_FIELD_CUST_2) {
				if(pstLog->bCustFlag & MTLOG_CUST_FLAG_C2_SET)
					log["cust_2"] = pstLog->dwCust_2;
				else
					log["cust_2"] = g_strCustLogHaveNo;
			}

			if(iShowLogField & SLOG_FIELD_CUST_3) {
				if(pstLog->bCustFlag & MTLOG_CUST_FLAG_C3_SET)
					log["cust_3"] = pstLog->iCust_3;
				else
					log["cust_3"] = g_strCustLogHaveNo;
			}

			if(iShowLogField & SLOG_FIELD_CUST_4) {
				if(pstLog->bCustFlag & MTLOG_CUST_FLAG_C4_SET)
					log["cust_4"] = pstLog->iCust_4;
				else
					log["cust_4"] = g_strCustLogHaveNo;
			}

			if(iShowLogField & SLOG_FIELD_CUST_5) {
				if((pstLog->bCustFlag & MTLOG_CUST_FLAG_C5_SET) && pstLog->szCust_5[0] != '\0')
					log["cust_5"] = pstLog->szCust_5;
				else
					log["cust_5"] = g_strCustLogHaveNo;
			}

			if(iShowLogField & SLOG_FIELD_CUST_6) {
				if((pstLog->bCustFlag & MTLOG_CUST_FLAG_C6_SET) && pstLog->szCust_6[0] != '\0')
					log["cust_6"] = pstLog->szCust_6;
				else
					log["cust_6"] = g_strCustLogHaveNo;
			}

			if(iShowLogField & SLOG_FIELD_APP)
				log["app"] = pstLog->iAppId;
			if(iShowLogField & SLOG_FIELD_MODULE)
				log["module"] = pstLog->iModuleId;
			if(iShowLogField & SLOG_FIELD_CONTENT)
				log["Content"] = pszLogContent;
			if(iShowLogField & SLOG_FIELD_CONFIG)
				log["config"] = pstLog->dwLogConfigId;
			if(iShowLogField & SLOG_FIELD_POS)
				log["pos"] = pszLogPos;
			
			log["type"] = pstLog->wLogType;
			log["s_time"] = pstLog->qwLogTime;
			js["list"].Add(log);

			dwInnerSeq = pstLog->dwLogSeq;
		}
		else  {
			if(time(NULL) >= stNow.tv_sec+1) {
				WARN_LOG("realtime log run over limit time(1s) -- get log:%d, max count:%d", i, iMaxCount);
			}
			break;
		}

		if(iLastIndex < 0)
			break;
	}

	uint32_t dwNewSeqNo = logsearch.GetLastLogSeq();

	if(i >= iMaxCount)
		js["Send"] = (unsigned int)iMaxCount;
	else
		js["Send"] = i; 
	js["Rece"] = i; // Rece - Send 为丢失没有显示的实时日志
	js["RequestSeq"] = dwNewSeqNo;
	js["LogField"] = iShowLogField;

	STRING str;
	string_init(&str);
	if((stConfig.err=string_set(&str, js.ToString().c_str())) != STATUS_OK
		|| (stConfig.err=cgi_output(cgi, &str)) != STATUS_OK)
	{
		string_clear(&str);
		return -2;
	}
	DEBUG_LOG("get realtime log send:%d total:%u cur seq:%u next seq:%u", 
		(uint32_t)js["Send"], (uint32_t)js["Rece"], dwLastSeqNo, dwNewSeqNo);
	string_clear(&str);
	return 0;
}

static int DealModModule(CGI *cgi)
{
	const char *pdesc = hdf_get_value(cgi->hdf, "Query.module_desc", NULL);
	const char *pname = hdf_get_value(cgi->hdf, "Query.module_name", NULL);
	const char *pappName = hdf_get_value(cgi->hdf, "Query.app_name", NULL);
	int32_t iAppId = hdf_get_int_value(cgi->hdf, "Query.app_id", -1);
	int32_t iModuleId = hdf_get_int_value(cgi->hdf, "Query.id", -1);
	if(NULL == pname || iModuleId < 0 || iAppId < 0)
	{
		WARN_LOG("invalid parameter name:%s module id:%d appid:%d", pname, iModuleId, iAppId);
		stConfig.pErrMsg = CGI_REQERR;
		return SLOG_ERROR_LINE;
	}
	hdf_set_value(cgi->hdf, "config.action", "save_mod_module");
	hdf_set_int_value(cgi->hdf, "config.app_id", iAppId);
	hdf_set_int_value(cgi->hdf, "config.module_id", iModuleId);
	hdf_set_value(cgi->hdf, "config.app_name", pappName);
	hdf_set_value(cgi->hdf, "config.module_name", pname);
	hdf_set_value(cgi->hdf, "config.module_desc", pdesc);
	return 0;
}

static int DealModApp(CGI *cgi)
{
	const char *pdesc = hdf_get_value(cgi->hdf, "Query.app_desc", NULL);
	const char *pname = hdf_get_value(cgi->hdf, "Query.app_name", NULL);
	int32_t iAppId = hdf_get_int_value(cgi->hdf, "Query.app_id", -1);
	if(NULL == pname || iAppId < 0)
	{
		WARN_LOG("invalid parameter name:%s desc:%s appid:%d", pname, pdesc, iAppId);
		stConfig.pErrMsg = CGI_REQERR;
		return SLOG_ERROR_LINE;
	}
	hdf_set_value(cgi->hdf, "config.action", "save_mod_app");
	hdf_set_int_value(cgi->hdf, "config.app_id", iAppId);
	hdf_set_value(cgi->hdf, "config.app_name", pname);
	hdf_set_value(cgi->hdf, "config.app_desc", pdesc);
	return 0;
}

static int DealAddModule(CGI *cgi)
{
	hdf_set_value(cgi->hdf, "config.action", "save_add_module");
	return 0;
}

static int DealAddApp(CGI *cgi)
{
	hdf_set_value(cgi->hdf, "config.action", "save_add_app");
	return 0;
}

static int DealSaveModule(CGI *cgi, bool bIsAdd=false)
{
	if(IsDaemonDenyOp(stConfig))
		return 0;

	FloginInfo *pUserInfo = stConfig.stUser.puser_info;
	const char *pdesc = hdf_get_value(cgi->hdf, "Query.ddam_module_desc", NULL);
	const char *pname = hdf_get_value(cgi->hdf, "Query.ddam_module_name", NULL);
	const char *pnav = hdf_get_value(cgi->hdf, "Query.navTabId", NULL);
	int32_t iAppId = hdf_get_int_value(cgi->hdf, "Query.ddam_op_app", -1);
	int32_t iModuleId = hdf_get_int_value(cgi->hdf, "Query.ddam_module_id", -1);
	AppInfo *pApp = slog.GetAppInfo(iAppId);
	if(NULL == pApp || NULL == pname || (bIsAdd && iAppId < 0) || (iModuleId < 0 && !bIsAdd))
	{
		WARN_LOG("invalid parameter name:%s desc:%s appid:%d, moduleid:%d",
			pname, pdesc, iAppId, iModuleId);
		stConfig.pErrMsg = CGI_REQERR;
		return SLOG_ERROR_LINE;
	}

	IM_SQL_PARA* ppara = NULL;
	if(InitParameter(&ppara) < 0) {
		ERR_LOG("sql parameter init failed !");
		return SLOG_ERROR_LINE;
	}
	if(pdesc != NULL)
		AddParameter(&ppara, "module_desc", pdesc, NULL);
	if(pname != NULL)
		AddParameter(&ppara, "module_name", pname, NULL);

	std::string strSql;
	Query & qu = *(stConfig.qu);

	AddParameter(&ppara, "user_mod_id", pUserInfo->iUserId, "DB_CAL");
	AddParameter(&ppara, "user_mod", pUserInfo->szUserName, NULL);
	if(bIsAdd) {
		AddParameter(&ppara, "create_time", uitodate(stConfig.dwCurTime), NULL);
		AddParameter(&ppara, "mod_time", uitodate(stConfig.dwCurTime), NULL);
		AddParameter(&ppara, "user_add", pUserInfo->szUserName, NULL);
		AddParameter(&ppara, "user_add_id", pUserInfo->iUserId, "DB_CAL");
		AddParameter(&ppara, "app_id", iAppId, "DB_CAL");
		strSql = "insert into mt_module_info ";
		JoinParameter_Insert(&strSql, qu.GetMysql(), ppara);
	}
	else {
		strSql = "update mt_module_info set";
		JoinParameter_Set(&strSql, qu.GetMysql(), ppara);
		strSql += " where module_id=";
		strSql += itoa(iModuleId);
	}

	ReleaseParameter(&ppara);
	if(!qu.execute(strSql))
	{
		ERR_LOG("execute sql:%s failed, msg:%s", strSql.c_str(), qu.GetError().c_str());
		return -1;
	}

	Json js;
	js["statusCode"] = 200;
	js["navTabId"] = pnav;
	js["callbackType"] = "closeCurrent";
	if(bIsAdd)
		js["msgid"] = "addSuccess";
	else
		js["msgid"] = "modSuccess";

	STRING str;
	string_init(&str);
	if((stConfig.err=string_set(&str, js.ToString().c_str())) != STATUS_OK
		|| (stConfig.err=cgi_output(stConfig.cgi, &str)) != STATUS_OK)
	{
		string_clear(&str);
		stConfig.pErrMsg = CGI_ERR_SERVER;
		return SLOG_ERROR_LINE;
	}
	string_clear(&str);

	DEBUG_LOG("%s mt_module_info success", bIsAdd ? "add" : "mod");
	return 0;
}

// dispatch one app log server to app, 分派一个 app log server
static int DispatchLogServer(int iNewAppId, Query &qu)
{
	int iStartIdx = 0;
	SLogServer *psvr = NULL;
	std::map<uint32_t,bool> mapTry;
	std::map<uint32_t, SLogServer *> mapUse;
	while(iStartIdx < MAX_SERVICE_COUNT)
	{
		psvr = slog.GetValidServerByType(SRV_TYPE_APP_LOG, &iStartIdx);
		if(psvr == NULL || mapTry.find(psvr->dwServiceId) != mapTry.end())
			break;
		if(psvr->bSandBox == SAND_BOX_ENABLE_NEW) {
			mapUse[psvr->dwServiceId] = psvr;
		}
		mapTry[psvr->dwServiceId] = true;
	}

	if(mapUse.size() <= 0)
		return SLOG_ERROR_LINE;

	int idx = rand()%mapUse.size();
	std::map<uint32_t, SLogServer *>::iterator it = mapUse.begin();
	for(int i=0; it != mapUse.end() && i<idx; i++)
		it++;
	psvr = it->second;


    std::ostringstream sql; 
    sql << "select srv_for from mt_server where xrk_id=" << it->first;
    if(!qu.get_result(sql.str().c_str()) ||  !qu.fetch_row()) {
        ERR_LOG("get server info failed, id:%d", it->first);
        return SLOG_ERROR_LINE;
    }    
    std::string strSvrFor = qu.getstr("srv_for");
    qu.free_result();
    if(strSvrFor.size() > 0) 
        strSvrFor += ","; 
    strSvrFor += itoa(iNewAppId);

    sql.str("");
    sql << "update mt_server set srv_for=\'" << strSvrFor << "\', cfg_seq=" << time(NULL) 
        << " where xrk_id=" << it->first;
    if(!qu.execute(sql.str()))
    {    
        ERR_LOG("execute sql:%s failed", sql.str().c_str());
        return SLOG_ERROR_LINE;
    }    
    INFO_LOG("dispatch log server:%s, for app:%d ok", psvr->szIpV4, iNewAppId);
	return 0;
}

static int DealSaveApp(CGI *cgi, bool bIsAdd=false)
{
	if(IsDaemonDenyOp(stConfig))
		return 0;

	FloginInfo *pUserInfo = stConfig.stUser.puser_info;
	const char *pdesc = hdf_get_value(cgi->hdf, "Query.ddaa_app_desc", NULL);
	const char *pname = hdf_get_value(cgi->hdf, "Query.ddaa_app_name", NULL);
	const char *pnav = hdf_get_value(cgi->hdf, "Query.navTabId", NULL);
	int32_t iAppId = hdf_get_int_value(cgi->hdf, "Query.ddaa_app_id", -1);
	if((NULL == pdesc && NULL == pname) || (!bIsAdd && iAppId < 0))
	{
		WARN_LOG("invalid parameter name:%s desc:%s appid:%d", pname, pdesc, iAppId);
		stConfig.pErrMsg = CGI_REQERR;
		return SLOG_ERROR_LINE;
	}

	IM_SQL_PARA* ppara = NULL;

	if(InitParameter(&ppara) < 0) {
		ERR_LOG("sql parameter init failed !");
		return SLOG_ERROR_LINE;
	}
	
	if(bIsAdd) {
	}
	else {
	}

	if(pdesc != NULL)
		AddParameter(&ppara, "app_desc", pdesc, NULL);
	if(pname != NULL)
		AddParameter(&ppara, "app_name", pname, NULL);

	std::string strSql;
	Query & qu = *(stConfig.qu);
	AddParameter(&ppara, "user_mod_id", pUserInfo->iUserId, "DB_CAL");
	AddParameter(&ppara, "user_mod", pUserInfo->szUserName, NULL);
	if(bIsAdd) {
		strSql = "START TRANSACTION";
		if(!qu.execute(strSql)) {
			ReleaseParameter(&ppara);
			return SLOG_ERROR_LINE;
		}

		AddParameter(&ppara, "create_time", stConfig.dwCurTime, "DB_CAL");
		AddParameter(&ppara, "update_time", uitodate(stConfig.dwCurTime), NULL);
		AddParameter(&ppara, "user_add_id", pUserInfo->iUserId, "DB_CAL");
		AddParameter(&ppara, "user_add", pUserInfo->szUserName, NULL);

		// 先设为删除状态
		AddParameter(&ppara, "xrk_status", RECORD_STATUS_DELETE, "DB_CAL");
		strSql = "insert into mt_app_info ";
		JoinParameter_Insert(&strSql, qu.GetMysql(), ppara);
	}
	else {
		strSql = "update mt_app_info set";
		JoinParameter_Set(&strSql, qu.GetMysql(), ppara);
		strSql += " where app_id=";
		strSql += itoa(iAppId);
	}

	ReleaseParameter(&ppara);
	if(!qu.execute(strSql))
	{
		ERR_LOG("execute sql:%s failed, msg:%s", strSql.c_str(), qu.GetError().c_str());
		return -1;
	}

	int iNewAppId = 0;
	if(bIsAdd)
	{
		iNewAppId = qu.insert_id();
		if(DispatchLogServer(iNewAppId, qu) >= 0)
		{
			// 分派成功，改为可用状态
			strSql = "update mt_app_info set xrk_status=";
			strSql += itoa(RECORD_STATUS_USE);
			strSql += " where app_id=";
			strSql += itoa(iNewAppId);
			qu.execute(strSql);

			strSql = "COMMIT";
			qu.execute(strSql);
		}
		else
		{
			strSql = "ROLLBACK";
			qu.execute(strSql);

			ERR_LOG("DispatchLogServer for app:%d failed", iNewAppId);
			return SLOG_ERROR_LINE;
		}
	}

	Json js;
	js["statusCode"] = 200;
	js["navTabId"] = pnav;
	js["callbackType"] = "closeCurrent";
	if(bIsAdd)
		js["msgid"] = "addSuccess";
	else
		js["msgid"] = "modSuccess";

	STRING str;
	string_init(&str);
	if((stConfig.err=string_set(&str, js.ToString().c_str())) != STATUS_OK
		|| (stConfig.err=cgi_output(stConfig.cgi, &str)) != STATUS_OK)
	{
		string_clear(&str);
		stConfig.pErrMsg = CGI_ERR_SERVER;
		return SLOG_ERROR_LINE;
	}
	string_clear(&str);

	DEBUG_LOG("%s mt_app_info success, new appid:%d", bIsAdd ? "add" : "mod", iNewAppId);
	return 0;
}

static int DealDelModule(CGI *cgi)
{
	if(IsDaemonDenyOp(stConfig))
		return 0;

	int id= hdf_get_int_value(cgi->hdf, "Query.id", -1);
	int iapp_id = hdf_get_int_value(cgi->hdf, "Query.app_id", -1);
	AppInfo *pApp = slog.GetAppInfo(iapp_id);
	if(id < 0 || iapp_id < 0 || pApp == NULL)
	{
		WARN_LOG("invalid parameter have no module_id");
		stConfig.pErrMsg = CGI_REQERR;
		return -1;
	}

	static char sSqlBuf[128];

	// 超级管理员或授权普通管理
	sprintf(sSqlBuf, "update mt_module_info set xrk_status=%d where module_id=%d",
		RECORD_STATUS_DELETE, id);

	Query & qu = *(stConfig.qu);
	if(!qu.execute(sSqlBuf))
	{
		ERR_LOG("execute sql:%s failed, msg:%s", sSqlBuf, qu.GetError().c_str());
		return -2;
	}

	Json js;
	js["statusCode"] = 200;

	STRING str;
	string_init(&str);
	if((stConfig.err=string_set(&str, js.ToString().c_str())) != STATUS_OK
		|| (stConfig.err=cgi_output(stConfig.cgi, &str)) != STATUS_OK)
	{
		string_clear(&str);
		stConfig.pErrMsg = CGI_ERR_SERVER;
		return SLOG_ERROR_LINE;
	}
	string_clear(&str);

	DEBUG_LOG("change module info status to:%d - id:%d success ", RECORD_STATUS_DELETE, id);
	return 0;
}

static int DealDelApp(CGI *cgi)
{
	if(IsDaemonDenyOp(stConfig))
		return 0;

	static char sSqlBuf[128];
	int id= hdf_get_int_value(cgi->hdf, "Query.id", -1);
	AppInfo *pApp = slog.GetAppInfo(id);
	if(id < 0 || pApp == NULL)
	{
		WARN_LOG("invalid parameter have no app_id:%d(%p)", id, pApp);
		stConfig.pErrMsg = CGI_REQERR;
		return -1;
	}

	sprintf(sSqlBuf, "update mt_app_info set xrk_status=%d where app_id=%d", RECORD_STATUS_DELETE, id);

	Query & qu = *(stConfig.qu);
	if(!qu.execute(sSqlBuf))
	{
		ERR_LOG("execute sql:%s failed, msg:%s", sSqlBuf, qu.GetError().c_str());
		return -2;
	}
	DEBUG_LOG("delete app :%d sql:%s", id, sSqlBuf);

	sprintf(sSqlBuf, "update mt_module_info set xrk_status=%d where app_id=%u", 
		RECORD_STATUS_DELETE, id);
	if(!qu.execute(sSqlBuf))
	{
		ERR_LOG("execute sql:%s failed, msg:%s", sSqlBuf, qu.GetError().c_str());
		return -3;
	}
	DEBUG_LOG("delete module sql:%s", sSqlBuf);

	Json js;
	js["statusCode"] = 200;

	STRING str;
	string_init(&str);
	if((stConfig.err=string_set(&str, js.ToString().c_str())) != STATUS_OK
		|| (stConfig.err=cgi_output(stConfig.cgi, &str)) != STATUS_OK)
	{
		string_clear(&str);
		stConfig.pErrMsg = CGI_ERR_SERVER;
		return SLOG_ERROR_LINE;
	}
	string_clear(&str);

	DEBUG_LOG("change app info status to:%d - id:%d success ", RECORD_STATUS_DELETE, id);
	return 0;
}

static int InitFastCgi_first(CGIConfig &myConf)
{
	if(InitFastCgiStart(myConf) < 0) {
		ERR_LOG("InitFastCgiStart failed !");
		return SLOG_ERROR_LINE;
	}

	if(LoadConfig(myConf.szConfigFile,
	   "NEED_DB", CFG_INT, &g_iNeedDb, 1,
	   "SLOG_SERVER_FILE_PATH", CFG_STRING, g_sLogPath, DEF_SLOG_LOG_FILE_PATH, sizeof(g_sLogPath),
		NULL) < 0){
		ERR_LOG("loadconfig failed, from file:%s", myConf.szConfigFile);
		return SLOG_ERROR_LINE;
	}

	int32_t iRet = 0;
	if((iRet=slog.InitConfigByFile(myConf.szConfigFile)) < 0 || (iRet=slog.Init(myConf.szLocalIp)) < 0)
		return SLOG_ERROR_LINE;

	logsearch.Init();
	if(!(logsearch.IsInit()))
	{
		ERR_LOG("CSLogSearch init failed msg:%s!\n", strerror(errno));
		return -101;
	}

	if(MtReport_InitVmem() < 0)
	{
		FATAL_LOG("MtReport_InitVmem failed");
		return SLOG_ERROR_LINE;
	}

	myConf.pAppInfo = slog.GetAppInfo();
	if(myConf.pAppInfo == NULL)
	{
		FATAL_LOG("get pAppInfo:%p failed !", myConf.pAppInfo);
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
	return 0;
}

int DealInitLogReportTest(CGI *cgi)
{
	char sSqlBuf[512] = {0};
	Json js;

	// 获取上报机器
	sprintf(sSqlBuf, "select xrk_id,xrk_name from mt_machine where xrk_status=%d", RECORD_STATUS_USE);
	Query qu(*stConfig.db);
	qu.get_result(sSqlBuf);
	for(int i=0; i < qu.num_rows() && qu.fetch_row() != NULL; i++)
	{
		Json mach;
		mach["id"] = qu.getuval("xrk_id");
		mach["name"] = qu.getstr("xrk_name");
		js["mach_list"].Add(mach);
	}
	qu.free_result();

	// log 配置
	sprintf(sSqlBuf, "select config_id,config_name,app_id,module_id,log_types from mt_log_config "
		" where xrk_status=%d", RECORD_STATUS_USE);
	qu.get_result(sSqlBuf);
	for(int i=0; i < qu.num_rows() && qu.fetch_row() != NULL; i++)
	{
		Json log;
		log["id"] = qu.getuval("config_id");
		log["name"] = qu.getstr("config_name");
		log["app_id"] = qu.getval("app_id");
		log["module_id"] = qu.getval("module_id");
		log["log_type"] = qu.getval("log_types");
		js["config_list"].Add(log);
	}
	qu.free_result();

	std::string str(js.ToString());
	stConfig.err = hdf_set_value(stConfig.cgi->hdf, "config.test_para", str.c_str()); 
	if(stConfig.err != STATUS_OK)
	{
		ERR_LOG("set log test para failed, length:%lu", str.size());
		return SLOG_ERROR_LINE;
	}
	DEBUG_LOG("set log test para:%s", str.c_str());
	return 0;
}

static int DealListPlugin(CGI *cgi, const char *ptype="open")
{
	hdf_set_value(stConfig.cgi->hdf, "config.plugin_pre", "dpopen");
	hdf_set_valuef(stConfig.cgi->hdf, "config.navTabId=dmt_plugin_%s", ptype);

	char sSqlBuf[128] = {0};
	Query qu(*stConfig.db);
	int iCount = 0;

	// 本地已安装的公共插件信息
	if(!strcmp(ptype, "open")) {
		Json js;
		std::ostringstream ss;
		sprintf(sSqlBuf, "select pb_info from mt_plugin where xrk_status=%d", RECORD_STATUS_USE);
		if(qu.get_result(sSqlBuf) && qu.num_rows() > 0) 
		{
			const char *pinfo = NULL;
			size_t iParseIdx = 0;
			size_t iReadLen = 0;
			while(qu.fetch_row() != NULL)
			{
				Json plugin;
				pinfo = qu.getstr("pb_info");
				iParseIdx = 0;
				iReadLen = strlen(pinfo);
				plugin.Parse(pinfo, iParseIdx);
				if(iParseIdx != iReadLen) {
					WARN_LOG("parse json content, size:%u!=%u", (uint32_t)iParseIdx, (uint32_t)iReadLen);
					continue;
				}
				js["list"].Add(plugin);
				iCount++;
				ss << "plugin_" << (int)(plugin["plugin_id"]) << "_" << (const char*)(plugin["plus_version"]) << ",";
			}
		}
		js["count"] = iCount;
		qu.free_result();
		hdf_set_value(cgi->hdf, "config.local_open_list", js.ToString().c_str());
		hdf_set_value(cgi->hdf, "config.local_open_list_ids", ss.str().c_str());
	}
	else {
		hdf_set_value(cgi->hdf, "config.local_open_list", "{count:0}");
		hdf_set_value(cgi->hdf, "config.local_open_list_ids", "");
	}
	return 0;
}

class CTransSavePlugin
{
    public:
        CTransSavePlugin(Query &qu, const int &iRet):m_qu(qu), m_iRet(iRet) { }
        ~CTransSavePlugin() {
            if(m_iRet == 0)
                m_qu.execute("COMMIT");
            else 
                m_qu.execute("ROLLBACK");
        }    
    private:
        Query &m_qu;
        const int &m_iRet;
};

static int AddPluginLogModule(Query &qu, Json & js_plugin)
{
	FloginInfo *pUserInfo = stConfig.stUser.puser_info;
	IM_SQL_PARA* ppara = NULL;

	const char *pname = js_plugin["plus_name"];
	if(InitParameter(&ppara) < 0) {
		ERR_LOG("sql parameter init failed !");
		return SLOG_ERROR_LINE;
	}

	std::ostringstream strdesc;
	strdesc << "插件: " << pname << " 日志模块";
	AddParameter(&ppara, "module_desc", strdesc.str().c_str(), NULL);
	if(pname != NULL)
		AddParameter(&ppara, "module_name", pname, NULL);
	std::string strSql;
	AddParameter(&ppara, "user_mod_id", pUserInfo->iUserId, "DB_CAL");
	AddParameter(&ppara, "user_mod", pUserInfo->szUserName, NULL);
	AddParameter(&ppara, "create_time", uitodate(stConfig.dwCurTime), NULL);
	AddParameter(&ppara, "mod_time", uitodate(stConfig.dwCurTime), NULL);
	AddParameter(&ppara, "user_add", pUserInfo->szUserName, NULL);
	AddParameter(&ppara, "user_add_id", pUserInfo->iUserId, "DB_CAL");
	AddParameter(&ppara, "app_id", PLUGIN_PARENT_APP_ID, "DB_CAL");

	strSql = "insert into mt_module_info ";
	JoinParameter_Insert(&strSql, qu.GetMysql(), ppara);

	ReleaseParameter(&ppara);
	if(!qu.execute(strSql))
	{
		ERR_LOG("execute sql:%s failed, msg:%s", strSql.c_str(), qu.GetError().c_str());
		return SLOG_ERROR_LINE;
	}

	js_plugin["module_id"] = (int)(qu.insert_id());
	DEBUG_LOG("add plugin:%s, log module:%d", pname, (int)qu.insert_id());
	return 0;
}

static int AddPluginLogConfig(Query &qu, Json & js_plugin)
{
	FloginInfo *pUserInfo = stConfig.stUser.puser_info;
	const int32_t iLogType = SLOG_LEVEL_INFO
		|SLOG_LEVEL_WARNING|SLOG_LEVEL_REQERROR|SLOG_LEVEL_ERROR|SLOG_LEVEL_FATAL;
	const char *pname = js_plugin["plus_name"];

	IM_SQL_PARA* ppara = NULL;
	if(InitParameter(&ppara) < 0) {
		ERR_LOG("sql parameter init failed !");
		return SLOG_ERROR_LINE;
	}
	AddParameter(&ppara, "config_name", pname, NULL);
	AddParameter(&ppara, "user_mod_id", pUserInfo->iUserId, "DB_CAL");
	AddParameter(&ppara, "user_mod", pUserInfo->szUserName, NULL);

	std::string strSql;
	AddParameter(&ppara, "create_time", stConfig.dwCurTime, "DB_CAL");
	AddParameter(&ppara, "update_time", uitodate(stConfig.dwCurTime), NULL);
	AddParameter(&ppara, "user_add", pUserInfo->szUserName, NULL);
	AddParameter(&ppara, "user_add_id", pUserInfo->iUserId, "DB_CAL");

	std::string strDesc("插件：");
	strDesc += pname;
	strDesc += " 日志配置";
	AddParameter(&ppara, "config_desc", strDesc.c_str(), NULL);

	AddParameter(&ppara, "app_id", PLUGIN_PARENT_APP_ID, "DB_CAL");
	AddParameter(&ppara, "module_id", (int)(js_plugin["module_id"]), "DB_CAL");
	AddParameter(&ppara, "log_types", iLogType, "DB_CAL");

	strSql = "insert into mt_log_config";
	JoinParameter_Insert(&strSql, qu.GetMysql(), ppara);
	ReleaseParameter(&ppara);
	if(!qu.execute(strSql))
	{
		ERR_LOG("execute sql:%s failed, msg:%s", strSql.c_str(), qu.GetError().c_str());
		return -1;
	}

	js_plugin["log_config_id"] = (int)(qu.insert_id());
	DEBUG_LOG("add plugin:%s, log config:%d", pname, (int)qu.insert_id());
	return 0;
}

static int AddPlugin(Query &qu, Json &js_plugin)
{
    std::ostringstream sql; 
	sql << "select plugin_id from mt_plugin where open_plugin_id=" 
		<< (int)(js_plugin["plugin_id"]) << ";";
	qu.get_result(sql.str().c_str());
	if(qu.num_rows() > 0 && qu.fetch_row()) 
	{
		WARN_LOG("already install plugin:%s(%d), local plugin id:%d",
			(const char*)(js_plugin["name"]), (int)(js_plugin["plugin_id"]), qu.getval("plugin_id"));
		qu.free_result();
		stConfig.iErrorCode = 301;
		return SLOG_ERROR_LINE;
	}
	qu.free_result();

	std::string strSql;
	IM_SQL_PARA* ppara = NULL;
	if(InitParameter(&ppara) < 0) {
		ERR_LOG("sql parameter init failed !");
		return SLOG_ERROR_LINE;
	}
	AddParameter(&ppara, "create_time", stConfig.dwCurTime, "DB_CAL");
	AddParameter(&ppara, "update_time", uitodate(stConfig.dwCurTime), NULL);
	AddParameter(&ppara, "open_plugin_id", (int)(js_plugin["plugin_id"]), "DB_CAL");
	AddParameter(&ppara, "plugin_name", (const char*)(js_plugin["plus_name"]), NULL);
	AddParameter(&ppara, "plugin_show_name", (const char*)(js_plugin["show_name"]), NULL);
	strSql = "insert into mt_plugin";
	JoinParameter_Insert(&strSql, qu.GetMysql(), ppara);

	ReleaseParameter(&ppara);
	if(!qu.execute(strSql))
	{
		ERR_LOG("execute sql:%s failed, msg:%s", strSql.c_str(), qu.GetError().c_str());
		stConfig.pErrMsg = CGI_ERR_SERVER;
		return SLOG_ERROR_LINE;
	}
	js_plugin["local_plugin_id"] = (int)(qu.insert_id());

	DEBUG_LOG("add plugin:%s to db, local id:%d, open id:%d",
		(const char*)(js_plugin["name"]), (int)(js_plugin["local_plugin_id"]), (int)(js_plugin["plugin_id"]));
	return 0;
}

static int AddPluginParentAttrTypes(Query &qu, Json &js_plugin)
{
	FloginInfo *pUserInfo = stConfig.stUser.puser_info;
	IM_SQL_PARA* ppara = NULL;

	std::string strSql;
	const char *pcurTime = uitodate(stConfig.dwCurTime);
	if(InitParameter(&ppara) < 0) {
		ERR_LOG("sql parameter init failed !");
		return SLOG_ERROR_LINE;
	}
	AddParameter(&ppara, "parent_type", PLUGIN_PARENT_ATTR_TYPE, "DB_CAL");
	AddParameter(&ppara, "xrk_name", (const char *)(js_plugin["show_name"]), NULL);
	AddParameter(&ppara, "type_pos", "1.1.1", NULL);
	AddParameter(&ppara, "attr_desc", (const char *)(js_plugin["plus_desc"]), NULL);
	AddParameter(&ppara, "create_user", stConfig.stUser.puser, NULL);
	AddParameter(&ppara, "mod_user", stConfig.stUser.puser, NULL);
	AddParameter(&ppara, "create_time", pcurTime, NULL);
	AddParameter(&ppara, "update_time", pcurTime, NULL);
	AddParameter(&ppara, "user_add_id", pUserInfo->iUserId, "DB_CAL");
	AddParameter(&ppara, "user_mod_id", pUserInfo->iUserId, "DB_CAL");
	strSql = "insert into mt_attr_type";
	JoinParameter_Insert(&strSql, qu.GetMysql(), ppara);
	ReleaseParameter(&ppara);
	if(!qu.execute(strSql))
	{
		ERR_LOG("execute sql:%s failed, msg:%s", strSql.c_str(), qu.GetError().c_str());
		return SLOG_ERROR_LINE;
	}
	DEBUG_LOG("add plugin:%s root attr type:%d", (const char *)(js_plugin["show_name"]), (int)qu.insert_id());
	js_plugin["plugin_root_attr_type_id"] = (int)(qu.insert_id());
	return 0;
}

static int TryUpdatePluginAttr(Query &qu, Json &js_local, Json &js_up, const Json &jspb_local)
{
	char szSql[128] = {0};
	FloginInfo *pUserInfo = stConfig.stUser.puser_info;

	if(strcmp((const char*)(js_local["attr_name"]), (const char*)(js_up["attr_name"]))) 
		js_local["attr_name"] = (const char*)(js_up["attr_name"]);
	if(js_up.HasValue("attr_desc")) 
		js_local["attr_desc"] = (const char*)(js_up["attr_desc"]);
	else 
		js_local.RemoveValue("attr_desc");
	if((int)(js_up["attr_data_type"]) != (int)(js_local["attr_data_type"]))
		js_local["attr_data_type"] = (int)(js_up["attr_data_type"]);
	if((int)(js_up["plug_attr_type_id"]) != (int)(js_local["plug_attr_type_id"])) 
		js_local["plug_attr_type_id"] = (int)(js_up["plug_attr_type_id"]);

	const Json::json_list_t & jslist_local_attrtype = jspb_local["attr_types"].GetArray();
	Json::json_list_t::const_iterator it_local_attrtype = jslist_local_attrtype.begin();
	int iDbAttrType = 0;
	for(; it_local_attrtype != jslist_local_attrtype.end(); it_local_attrtype++) {
		if((int)((*it_local_attrtype)["plug_attr_type_id"]) == (int)(js_up["plug_attr_type_id"])) {
			iDbAttrType = (int)((*it_local_attrtype)["attr_type_id"]);
			break;
		}
	}
	if(iDbAttrType == 0) {
		ERR_LOG("not find plugin attr type:%d in local db", (int)(js_up["plug_attr_type_id"]));
		return SLOG_ERROR_LINE;
	}

	snprintf(szSql, sizeof(szSql), "select data_type,attr_name,attr_desc,attr_type from mt_attr "
	    " where xrk_id=%d and xrk_status=0", (int)(js_local["attr_id"]));
	if(!qu.get_result(szSql) || qu.num_rows() <= 0 || !qu.fetch_row()) {
	    WARN_LOG("not find plugin:%s attr :%d", (const char*)(js_local["plus_name"]),
			(int)(js_local["attr_id"]));
		return SLOG_ERROR_LINE;
	}

	if(!strcmp(qu.getstr("attr_name"), (const char*)(js_up["attr_name"]))
	    && ((js_up.HasValue("attr_desc") && !strcmp(qu.getstr("attr_desc"), (const char*)(js_up["attr_desc"])))
			|| (!js_up.HasValue("attr_desc") && !strcmp(qu.getstr("attr_desc"), "")))
		&& (int)(js_up["attr_data_type"]) == qu.getval("data_type")
		&& iDbAttrType == qu.getval("attr_type"))
	{
	    qu.free_result();
		return 0;
	}

	IM_SQL_PARA* ppara = NULL;
	std::string strSql;
	if(InitParameter(&ppara) < 0) {
		ERR_LOG("sql parameter init failed !");
		return SLOG_ERROR_LINE;
	}
	if(strcmp(qu.getstr("attr_name"), (const char*)(js_up["attr_name"]))) 
		AddParameter(&ppara, "attr_name", (const char*)(js_up["attr_name"]), NULL);
	if((js_up.HasValue("attr_desc") && strcmp(qu.getstr("attr_desc"), (const char*)(js_up["attr_desc"])))
		|| (!js_up.HasValue("attr_desc") && strcmp(qu.getstr("attr_desc"), "")))
	{
		if(js_up.HasValue("attr_desc")) {
			 AddParameter(&ppara, "attr_desc", (const char *)(js_up["attr_desc"]), NULL);
		}
		else {
			AddParameter(&ppara,  "attr_desc", "", NULL);
		}
	}
	if(iDbAttrType == qu.getval("attr_type"))
		AddParameter(&ppara, "attr_type", iDbAttrType, "DB_CAL");
	if((int)(js_up["attr_data_type"]) != qu.getval("data_type")) 
		AddParameter(&ppara, "data_type", (int)(js_up["attr_data_type"]), "DB_CAL");

	AddParameter(&ppara, "user_mod_id", pUserInfo->iUserId, "DB_CAL");
	strSql = "update mt_attr set ";
	JoinParameter_Set(&strSql, qu.GetMysql(), ppara);
	strSql += " where xrk_id=";
	strSql += itoa((int)(js_local["attr_id"]));

	qu.free_result();
	ReleaseParameter(&ppara);
	if(!qu.execute(strSql))
	{
		ERR_LOG("execute sql:%s failed, msg:%s", strSql.c_str(), qu.GetError().c_str());
		return SLOG_ERROR_LINE;
	}
	DEBUG_LOG("update plugin attr:%d", (int)(js_local["attr_id"]));
	return 0;
}

static int AddPluginAttr(Query &qu, Json &js_attr, Json &js_plugin)
{
	FloginInfo *pUserInfo = stConfig.stUser.puser_info;
	Json::json_list_t & jslist_attrtype = js_plugin["attr_types"].GetArray();
	Json::json_list_t::iterator it_attrtype = jslist_attrtype.begin();
	int iAttrType = 0;
	while(it_attrtype != jslist_attrtype.end()) {
		const Json &js = *it_attrtype;
		if((int)(js["plug_attr_type_id"]) == (int)(js_attr["plug_attr_type_id"])) {
			iAttrType = (int)(js["attr_type_id"]);
			break;
		}
		it_attrtype++;
	}

	if(iAttrType == 0 || it_attrtype == jslist_attrtype.end()) {
		WARN_LOG("invalid plugin:%s,  not find plug_attr_type_id:%d", 
			(const char*)(js_plugin["plus_name"]), (int)(js_attr["plug_attr_type_id"]));
		return SLOG_ERROR_LINE;
	}

	IM_SQL_PARA* ppara = NULL;
	if(InitParameter(&ppara) < 0) {
		ERR_LOG("sql parameter init failed !");
		return SLOG_ERROR_LINE;
	}

	std::string strSql;
	char *pcurTime = uitodate(stConfig.dwCurTime);
	AddParameter(&ppara, "attr_name", (const char*)(js_attr["attr_name"]), NULL);
	if(js_attr.HasValue("attr_desc"))
		AddParameter(&ppara, "attr_desc", (const char*)(js_attr["attr_desc"]), NULL);
	AddParameter(&ppara, "user_add", stConfig.stUser.puser, NULL);
	AddParameter(&ppara, "data_type", (int)(js_attr["attr_data_type"]), "DB_CAL");
	AddParameter(&ppara, "attr_type", iAttrType, "DB_CAL");
	AddParameter(&ppara, "user_add_id", pUserInfo->iUserId, "DB_CAL");
	AddParameter(&ppara, "user_mod_id", pUserInfo->iUserId, "DB_CAL");
	AddParameter(&ppara, "create_time", pcurTime, NULL);
	AddParameter(&ppara, "update_time", pcurTime, NULL);
	strSql = "insert into mt_attr";
	JoinParameter_Insert(&strSql, qu.GetMysql(), ppara);

	ReleaseParameter(&ppara);
	if(!qu.execute(strSql))
	{
		ERR_LOG("execute sql:%s failed, msg:%s", strSql.c_str(), qu.GetError().c_str());
		return SLOG_ERROR_LINE;
	}

	DEBUG_LOG("add plugin:%s attr :%d(%s)", (const char*)(js_plugin["plus_name"]),
		(int)qu.insert_id(), (const char*)(js_attr["attr_name"]));
	js_attr["attr_id"] = (int)(qu.insert_id());
	return 0;
}

static int TryUpdatePluginAttrType(Query &qu, Json &js_local, Json &js_up)
{
	char szSql[128] = {0};
	FloginInfo *pUserInfo = stConfig.stUser.puser_info;
	if(strcmp((const char*)(js_up["attr_type_name"]), (const char*)(js_local["attr_type_name"])))
		js_local["attr_type_name"] = (const char*)(js_up["attr_type_name"]);
	if(js_up.HasValue("attr_type_desc")) 
		js_local["attr_type_desc"] = (const char*)(js_up["attr_type_desc"]);
	else 
		js_local.RemoveValue("attr_type_desc");

	snprintf(szSql, sizeof(szSql), "select xrk_name,attr_desc from mt_attr_type "
	    " where xrk_type=%d and xrk_status=0", (int)(js_local["attr_type_id"]));
	if(!qu.get_result(szSql) || qu.num_rows() <= 0 || !qu.fetch_row()) {
	    WARN_LOG("not find plugin:%s attr type:%d", (const char*)(js_local["plus_name"]),
			(int)(js_local["attr_type_id"]));
		return SLOG_ERROR_LINE;
	}

	if(!strcmp(qu.getstr("xrk_name"), (const char*)(js_up["attr_type_name"]))
	    && ((js_up.HasValue("attr_type_desc") && !strcmp(qu.getstr("attr_desc"), (const char*)(js_up["attr_type_desc"])))
		|| (!js_up.HasValue("attr_type_desc") && !strcmp(qu.getstr("attr_desc"), ""))))
	{
	    qu.free_result();
		return 0;
	}

	IM_SQL_PARA* ppara = NULL;
	std::string strSql;
	const char *pcurTime = uitodate(stConfig.dwCurTime);

	if(InitParameter(&ppara) < 0) {
		ERR_LOG("sql parameter init failed !");
	    qu.free_result();
		return SLOG_ERROR_LINE;
	}

	if(strcmp(qu.getstr("xrk_name"), (const char*)(js_up["attr_type_name"]))) {
		AddParameter(&ppara, "xrk_name", (const char*)(js_up["attr_type_name"]), NULL);
	}
	if((js_up.HasValue("attr_type_desc") && strcmp(qu.getstr("attr_desc"), (const char*)(js_up["attr_type_desc"])))
		|| (!js_up.HasValue("attr_type_desc") && strcmp(qu.getstr("attr_desc"), "")))
	{
		if(js_up.HasValue("attr_type_desc")) {
			 AddParameter(&ppara, "attr_desc", (const char *)(js_up["attr_type_desc"]), NULL);
		}
		else {
			AddParameter(&ppara,  "attr_desc", "", NULL);
		}
	}

	AddParameter(&ppara, "mod_user", stConfig.stUser.puser, NULL);
	AddParameter(&ppara, "update_time", pcurTime, NULL);
	AddParameter(&ppara, "user_mod_id", pUserInfo->iUserId, "DB_CAL");
	strSql = "update mt_attr_type set ";
	JoinParameter_Set(&strSql, qu.GetMysql(), ppara);
	strSql += " where xrk_type=";
	strSql += itoa((int)(js_local["attr_type_id"]));

	qu.free_result();
	ReleaseParameter(&ppara);
	if(!qu.execute(strSql))
	{
		ERR_LOG("execute sql:%s failed, msg:%s", strSql.c_str(), qu.GetError().c_str());
		return SLOG_ERROR_LINE;
	}
	DEBUG_LOG("update plugin attr type:%d", (int)(js_local["attr_type_id"]));
	return 0;
}

static int AddPluginAttrTypes(Query &qu, Json &js_attr_type, Json &js_plugin)
{
	FloginInfo *pUserInfo = stConfig.stUser.puser_info;

	IM_SQL_PARA* ppara = NULL;
	std::string strSql;
	const char *pcurTime = uitodate(stConfig.dwCurTime);

	if(InitParameter(&ppara) < 0) {
		ERR_LOG("sql parameter init failed !");
		return SLOG_ERROR_LINE;
	}

	AddParameter(&ppara, "xrk_name", (const char *)(js_attr_type["attr_type_name"]), NULL);
	if(js_attr_type.HasValue("attr_type_desc"))
		AddParameter(&ppara, 
			"attr_desc", (const char *)(js_attr_type["attr_type_desc"]), NULL);
	AddParameter(&ppara, "parent_type", (int)(js_plugin["plugin_root_attr_type_id"]), "DB_CAL");
	AddParameter(&ppara, "type_pos", "1.1.1.1", NULL);
	AddParameter(&ppara, "create_user", stConfig.stUser.puser, NULL);
	AddParameter(&ppara, "mod_user", stConfig.stUser.puser, NULL);
	AddParameter(&ppara, "create_time", pcurTime, NULL);
	AddParameter(&ppara, "update_time", pcurTime, NULL);
	AddParameter(&ppara, "user_add_id", pUserInfo->iUserId, "DB_CAL");
	AddParameter(&ppara, "user_mod_id", pUserInfo->iUserId, "DB_CAL");
	strSql = "insert into mt_attr_type";
	JoinParameter_Insert(&strSql, qu.GetMysql(), ppara);

	ReleaseParameter(&ppara);
	if(!qu.execute(strSql))
	{
		ERR_LOG("execute sql:%s failed, msg:%s", strSql.c_str(), qu.GetError().c_str());
		return SLOG_ERROR_LINE;
	}
	DEBUG_LOG("add plugin:%s, attr type:%d(%s)", (const char*)(js_plugin["plus_name"]),
		(int)qu.insert_id(), (const char *)(js_attr_type["attr_type_name"]));
	js_attr_type["attr_type_id"] = (int)(qu.insert_id());
	return 0;
}

static int GetLocalPlugin(Json &js_plugin, int iPluginId)
{
	char sSqlBuf[128] = {0};
	Query qu(*stConfig.db);

	snprintf(sSqlBuf, sizeof(sSqlBuf),
		"select pb_info from mt_plugin where open_plugin_id=%d", iPluginId);
	qu.get_result(sSqlBuf);
	if(qu.num_rows() > 0 && qu.fetch_row() != NULL) 
	{
		hdf_set_value(stConfig.cgi->hdf, "config.plugin_info", qu.getstr("pb_info"));
		DEBUG_LOG("get plugin:%d info", iPluginId);
	}
	else {
		WARN_LOG("not find plugin:%d", iPluginId);
		qu.free_result();
		return SLOG_ERROR_LINE;
	}

	const char *pinfo = qu.getstr("pb_info");
	size_t iParseIdx = 0;
	size_t iReadLen = strlen(pinfo);
	js_plugin.Parse(pinfo, iParseIdx);
	if(iParseIdx != iReadLen) {
		WARN_LOG("parse json content, size:%u!=%u", (uint32_t)iParseIdx, (uint32_t)iReadLen);
	}
	qu.free_result();
	return 0;
}

static int MakePluginConfFile_def(Json &plug_info, ostringstream &oAbsFile)
{
    FCGI_FILE * fp = FCGI_fopen(oAbsFile.str().c_str(), "w+");
    if(fp == NULL) {
        ERR_LOG("create plugin config file:%s failed, msg:%s", oAbsFile.str().c_str(), strerror(errno));
        stConfig.pErrMsg = "文件创建失败";
        return SLOG_ERROR_LINE;
    }    
    FCGI_fprintf(fp, "###########################################################################\r\n");
    FCGI_fprintf(fp, "#该文件为字符云监控系统插件部署配置文件, 在部署插件时需要该文件\r\n");
    FCGI_fprintf(fp, "#如您修改了该文件, 插件升级重新部署时注意合并修改到新配置文件\r\n");
    FCGI_fprintf(fp, "#\r\n");
	FCGI_fprintf(fp, "#适用版本: 开源版\r\n");
	FCGI_fprintf(fp, "#插件显示名: %s\r\n", (const char*)(plug_info["show_name"]));
    FCGI_fprintf(fp, "#插件名: %s\r\n", (const char*)(plug_info["plus_name"]));
    FCGI_fprintf(fp, "#插件ID: %u\r\n", (int)(plug_info["plugin_id"]));
    FCGI_fprintf(fp, "#文件版本: %s\r\n", (const char*)(plug_info["plus_version"]));
    FCGI_fprintf(fp, "#\r\n");
    FCGI_fprintf(fp, "###########################################################################\r\n");
    FCGI_fprintf(fp, "\r\n");
    FCGI_fprintf(fp, "\r\n");
	FCGI_fprintf(fp, "#用于版本check, 当前适用开源版\r\n");
	FCGI_fprintf(fp, "XRK_PLUGIN_CONFIG_PLAT open\r\n");
    FCGI_fprintf(fp, "\r\n");
    FCGI_fprintf(fp, "#配置文件版本 \r\n");
    FCGI_fprintf(fp, "XRK_PLUGIN_CONFIG_FILE_VER %s\r\n", (const char*)(plug_info["plus_version"]));
    FCGI_fprintf(fp, "\r\n");
    FCGI_fprintf(fp, "#插件日志配置ID, 为0表示不上报插件日志到监控系统日志中心 \r\n");
    FCGI_fprintf(fp, "XRK_PLUGIN_CONFIG_ID %u\r\n", (int)(plug_info["log_config_id"]));
    FCGI_fprintf(fp, "\r\n");
    FCGI_fprintf(fp, "#插件本地日志类型, 可选值: info debug warn reqerr error fatal all \"\"\r\n");
    FCGI_fprintf(fp, "XRK_LOCAL_LOG_TYPE error|fatal\r\n");
    FCGI_fprintf(fp, "\r\n");
    FCGI_fprintf(fp, "#插件本地日志文件, 注意插件需要有写权限\r\n");
    FCGI_fprintf(fp, "XRK_LOCAL_LOG_FILE ./%s.log\r\n", (const char*)(plug_info["plus_name"]));
    FCGI_fprintf(fp, "\r\n");
	FCGI_fprintf(fp, "#插件校验错误时，如果设为1, 校验失败5分钟后可重新校验\r\n");
	FCGI_fprintf(fp, "XRK_PLUGIN_RE_CHECK 0\r\n");
    FCGI_fprintf(fp, "\r\n");

	Json::json_list_t & jslist_cfg = plug_info["cfgs"].GetArray();
	Json::json_list_t::iterator it_cfg = jslist_cfg.begin();
    std::ostringstream ss_cfg;
	for(; it_cfg != jslist_cfg.end(); it_cfg++) {
		Json &cfg = *it_cfg;
        if(!(bool)(cfg["enable_modify"]))
            continue;
        FCGI_fprintf(fp, "#%s\r\n", (const char*)(cfg["item_desc"]));
        FCGI_fprintf(fp, "%s %s\r\n",
            (const char*)(cfg["item_name"]), (const char*)(cfg["item_value"]));
        FCGI_fprintf(fp, "\r\n");
        if(ss_cfg.str().size() > 0)
            ss_cfg << "," << (const char*)(cfg["item_name"]);
        else
            ss_cfg << (const char*)(cfg["item_name"]);
    }

    if(ss_cfg.str().size() > 0) {
        FCGI_fprintf(fp, "#控制台可配置的配置项\r\n");
        FCGI_fprintf(fp, "XRK_ENABLE_MOD_CFGS %s,xrk_end\r\n", ss_cfg.str().c_str());
        FCGI_fprintf(fp, "\r\n");
    }

    FCGI_fprintf(fp, "\r\n");
    FCGI_fprintf(fp, "#以下监控点配置项不能修改\r\n");
	Json::json_list_t & jslist_attr = plug_info["attrs"].GetArray();
	Json::json_list_t::iterator it_attr = jslist_attr.begin();
	for(; it_attr != jslist_attr.end(); it_attr++) {
		Json & attr = *it_attr;
        FCGI_fprintf(fp, "#监控点: %s\r\n", (const char*)(attr["attr_name"]));
        FCGI_fprintf(fp, "%s %d\r\n", (const char*)(attr["attr_id_macro"]), (int)(attr["attr_id"]));
        FCGI_fprintf(fp, "\r\n");
    }

	FCGI_fprintf(fp, "#插件ID\r\n");
	FCGI_fprintf(fp, "XRK_PLUGIN_ID %d\r\n", (int)(plug_info["plugin_id"]));
    FCGI_fprintf(fp, "\r\n");
	FCGI_fprintf(fp, "#插件部署名\r\n");
	FCGI_fprintf(fp, "XRK_PLUGIN_NAME %s\r\n", (const char*)(plug_info["plus_name"]));
    FCGI_fprintf(fp, "\r\n");
	FCGI_fprintf(fp, "#插件可执行版本\r\n");
	FCGI_fprintf(fp, "XRK_PLUGIN_HEADER_FILE_VER %s\r\n", (const char*)(plug_info["plus_version"]));
	FCGI_fprintf(fp, "\r\n");
	FCGI_fclose(fp);
    DEBUG_LOG("create config file:%s ok", oAbsFile.str().c_str());
    return 0;
}

static const char* GetLogTypeStr(int iLogConfigId)
{
	static std::string s_type;

	SLogClientConfig *plogconfig = slog.GetSlogConfig(iLogConfigId);
	if(!plogconfig) {
		ERR_LOG("not find logconfig:%d", iLogConfigId);
		return "no";
	}

	s_type = "";
	if(plogconfig->iLogType & SLOG_LEVEL_DEBUG)
		s_type += "debug ";
	if(plogconfig->iLogType & SLOG_LEVEL_INFO)
		s_type += "info ";
	if(plogconfig->iLogType & SLOG_LEVEL_OTHER)
		s_type += "other ";
	if(plogconfig->iLogType & SLOG_LEVEL_WARNING)
		s_type += "warn ";
	if(plogconfig->iLogType & SLOG_LEVEL_REQERROR)
		s_type += "reqerr ";
	if(plogconfig->iLogType & SLOG_LEVEL_ERROR)
		s_type += "error ";
	if(plogconfig->iLogType & SLOG_LEVEL_FATAL)
		s_type += "fatal ";
	return s_type.c_str();
}

static int MakePluginConfFile_js(Json & plug_info, ostringstream &oAbsFile)
{
	FCGI_FILE * fp = FCGI_fopen(oAbsFile.str().c_str(), "w+");
	if(fp == NULL) {
		ERR_LOG("create plugin config file:%s failed, msg:%s", oAbsFile.str().c_str(), strerror(errno));
		stConfig.pErrMsg = "文件创建失败";
		return SLOG_ERROR_LINE;
	}

	FCGI_fprintf(fp, "///////////////////////////////////////////////////////////////////////////\r\n");
	FCGI_fprintf(fp, "//该文件为字符云监控系统 js 插件部署配置文件, 在部署插件时需要该文件\r\n");
	FCGI_fprintf(fp, "//如您修改了该文件, 插件升级重新部署时注意合并修改到新配置文件\r\n");
	FCGI_fprintf(fp, "//\r\n");
	FCGI_fprintf(fp, "//适用版本: 开源版\r\n");
	FCGI_fprintf(fp, "//插件名: %s\r\n", (const char*)(plug_info["plus_name"]));
	FCGI_fprintf(fp, "//插件ID: %u\r\n", (int)(plug_info["plugin_id"]));
	FCGI_fprintf(fp, "//文件版本: %s\r\n", (const char*)(plug_info["plus_version"]));
	FCGI_fprintf(fp, "//\r\n");
	FCGI_fprintf(fp, "///////////////////////////////////////////////////////////////////////////\r\n");
	FCGI_fprintf(fp, "\r\n");
	FCGI_fprintf(fp, "\r\n");
	FCGI_fprintf(fp, "var xrk_config_%s = { \r\n", (const char*)(plug_info["plus_name"]));
	FCGI_fprintf(fp, "    plugin_id:%d, // 插件id\r\n", (int)(plug_info["plugin_id"]));
	FCGI_fprintf(fp, "    report_url:\'http://%s%s/mt_slog_reportinfo\', // 上报地址\r\n", 
		(const char*)(plug_info["self_domain"]), stConfig.szCgiPath);
	FCGI_fprintf(fp, "    plugin_name:\'%s\', // 插件名\r\n", (const char*)(plug_info["plus_name"]));
	FCGI_fprintf(fp, "    plugin_ver:\'%s\', // 配置文件版本\r\n", (const char*)(plug_info["plus_version"]));
	FCGI_fprintf(fp, "    logconfig_id:%u, // 插件日志配置ID, 为0表示不上报插件日志\r\n", (int)(plug_info["log_config_id"]));
	FCGI_fprintf(fp, "    logconfig_type:\'%s\', // 插件日志记录类型\r\n", GetLogTypeStr((int)(plug_info["log_config_id"])));

	// 配置
    std::ostringstream ss_cfg;
	Json::json_list_t & jslist_cfg = plug_info["cfgs"].GetArray();
	Json::json_list_t::iterator it_cfg = jslist_cfg.begin();
	for(; it_cfg != jslist_cfg.end(); it_cfg++) {
		Json &cfg = *it_cfg;
        if(!(bool)(cfg["enable_modify"]))
			FCGI_fprintf(fp, "    %s: \'%s\', // [不可修改] %s\r\n", 
				(const char*)(cfg["item_name"]), (const char*)(cfg["item_value"]),
				(const char*)(cfg["item_desc"]));
		else {
			FCGI_fprintf(fp, "    %s: \'%s\', // %s\r\n", 
				(const char*)(cfg["item_name"]), (const char*)(cfg["item_value"]),
				(const char*)(cfg["item_desc"]));
            if(ss_cfg.str().size() > 0)
                ss_cfg << "," << (const char*)(cfg["item_name"]);
            else
                ss_cfg << (const char*)(cfg["item_name"]);
		}
	}

    if(ss_cfg.str().size() > 0) {
        FCGI_fprintf(fp, "    xrk_enable_mod_cfgs: \'%s\', // [控制台可修改的配置项] \r\n", ss_cfg.str().c_str());
    }

	// 监控点
	Json::json_list_t & jslist_attr = plug_info["attrs"].GetArray();
	Json::json_list_t::iterator it_attr = jslist_attr.begin();
	for(; it_attr != jslist_attr.end(); it_attr++) {
		Json & attr = *it_attr;
		FCGI_fprintf(fp, "    %s: %d, // 监控点: %s\r\n", 
			(const char*)(attr["attr_id_macro"]), (int)(attr["attr_id"]),
			(const char*)(attr["attr_name"]));
	}

	FCGI_fprintf(fp, "    xrk_end:0 \r\n");
	FCGI_fprintf(fp, "}; \r\n");
	FCGI_fprintf(fp, "\r\n");
	FCGI_fclose(fp);
	DEBUG_LOG("create config file:%s ok", oAbsFile.str().c_str());
	return 0;
}

static int MakePluginConfFile(Json &plug_info, ostringstream &oAbsFile)
{
	int iRet=0;
	if(!strcmp((const char*)(plug_info["dev_language"]), "javascript")) 
		iRet=MakePluginConfFile_js(plug_info, oAbsFile);
	else
		iRet=MakePluginConfFile_def(plug_info, oAbsFile);
	return iRet;
}

static int DealRefreshPreInstallStatus()
{
	int mid = hdf_get_int_value(stConfig.cgi->hdf, "Query.machine_id", 0);
    int pid = hdf_get_int_value(stConfig.cgi->hdf, "Query.plugin_id", 0);
	if(mid==0 || pid==0)
	{
		REQERR_LOG("invalid parameter mid:%d, pid:%d", mid, pid);
		hdf_set_value(stConfig.cgi->hdf, "err.msg", CGI_REQERR);
		return SLOG_ERROR_LINE;
	}

    Json js;
	Query & qu = *stConfig.qu;
    std::ostringstream ss;
    ss << "select install_proc from mt_plugin_machine where machine_id=" << mid;
    ss << " and open_plugin_id=" << pid << " and xrk_status=0";
    if(!qu.get_result(ss.str().c_str()) ||  qu.num_rows() <= 0) {
        REQERR_LOG("not find preinstall machine:%d, plugin:%d !", mid, pid);
        js["ret"] = 1;
        js["msg"] = "未找到一键部署任务";
    }
    else {
        qu.fetch_row();
        js["ret"] = 0;
        js["proc_status"] = qu.getval("install_proc");
    }

    qu.free_result();
	std::string str(js.ToString());
    DEBUG_LOG("preinstall machine:%d, plugin:%d, new process status:%s", mid, pid, str.c_str());
    return my_cgi_output(str.c_str(), stConfig);
}

static int MakeLocalPluginConf(
	const char *pself_domain, int iPluginId, ostringstream &oDownUrl, ostringstream &oFile)
{
	Json js_local;
	if(GetLocalPlugin(js_local, iPluginId) < 0)
		return SLOG_ERROR_LINE;

	if(pself_domain == NULL)
		pself_domain = hdf_get_value(stConfig.cgi->hdf, "CGI.ServerAddress", "");
	js_local["self_domain"] = pself_domain;

	ostringstream ostrFile;
	if(!strcmp((const char*)(js_local["dev_language"]), "javascript"))  {
		ostrFile << stConfig.szCsPath << "download/" << (const char*)(js_local["plus_name"]);
		ostrFile << "_conf.js";
		oFile << (const char*)(js_local["plus_name"]) << "_conf.js";
		oDownUrl << stConfig.szDocPath << "download/" << oFile.str();
	}
	else {
		ostrFile << stConfig.szCsPath << "download/xrk_" << (const char*)(js_local["plus_name"]);
		ostrFile << ".conf";
		oFile << "xrk_" << (const char*)(js_local["plus_name"]) << ".conf";
		oDownUrl << stConfig.szDocPath << "download/" << oFile.str();
	}

	if(MakePluginConfFile(js_local, ostrFile) < 0)
		return SLOG_ERROR_LINE;
	return 0;
}


// 整个安装过程需要执行的操作步骤数
#define MAX_INSTALL_PLUGIN_PROC 8
static int DealPreInstallPlugin(std::string &strCsTemplateFile)
{
	int mid = hdf_get_int_value(stConfig.cgi->hdf, "Query.mach", 0);
    int pid = hdf_get_int_value(stConfig.cgi->hdf, "Query.plugin", 0);
	const char *pself_domain = hdf_get_value(stConfig.cgi->hdf, "Query.self_domain", NULL);
	if(mid==0 || pid==0 || !pself_domain)
	{
		REQERR_LOG("invalid parameter mid:%d, pid:%d, local:%p", mid, pid, pself_domain);
		hdf_set_value(stConfig.cgi->hdf, "err.msg", CGI_REQERR);
		return SLOG_ERROR_LINE;
	}

	Json js_local;
	if(GetLocalPlugin(js_local, pid) < 0)
		return SLOG_ERROR_LINE;
	hdf_set_value(stConfig.cgi->hdf, "config.plugin_name", (const char*)(js_local["plus_name"]));
	hdf_set_value(stConfig.cgi->hdf, "config.plugin_show_name", (const char*)(js_local["show_name"]));
	hdf_set_value(stConfig.cgi->hdf, "config.plugin_ver", (const char*)(js_local["plus_version"]));
	if(js_local.HasValue("install_tp_file") && !IsStrEqual((const char*)(js_local["install_tp_file"]), "no"))
	{
		std::string strCsFile(stConfig.szCsPath);
		strCsFile += (const char*)(js_local["install_tp_file"]);
		if(IsFileExist(strCsFile.c_str()))  {
			strCsTemplateFile = (const char*)(js_local["install_tp_file"]);
			DEBUG_LOG("use template file:%s", (const char*)(js_local["install_tp_file"]));
		}
		else {
			WARN_LOG("not find file:%s(%s)", strCsFile.c_str(), (const char*)(js_local["install_tp_file"]));
		}
	}

    MachineInfo *pMachinfo = slog.GetMachineInfo(mid, NULL);
    if(!pMachinfo) {
        REQERR_LOG("not find machine:%d", mid);
		hdf_set_value(stConfig.cgi->hdf, "err.msg", CGI_REQERR);
        return SLOG_ERROR_LINE;
    }

    std::string ips;
    if(pMachinfo->ip1 != 0) {
        ips += ipv4_addr_str(pMachinfo->ip1);
        ips += " | ";
    }
    if(pMachinfo->ip2 != 0) {
        ips += ipv4_addr_str(pMachinfo->ip2);
        ips += " | ";
    }
    if(pMachinfo->ip3 != 0) {
        ips += ipv4_addr_str(pMachinfo->ip3);
        ips += " | ";
    }
    if(pMachinfo->ip4 != 0) {
        ips += ipv4_addr_str(pMachinfo->ip4);
        ips += " | ";
    }
    if(ips.length() > 3)
        ips.erase(ips.length()-3, 3);
    hdf_set_value(stConfig.cgi->hdf, "config.machine_ip", ips.c_str());
    const char *ptmp = MtReport_GetFromVmem_Local(pMachinfo->iNameVmemIdx);
    hdf_set_value(stConfig.cgi->hdf, "config.machine_name", ptmp ? ptmp : "unknow");

	if(IsStrEqual((const char*)(js_local["dev_language"]), "javascript")) {
		hdf_set_value(stConfig.cgi->hdf, "config.valid_status", "2,3,4,5,6,9");
		hdf_set_int_value(stConfig.cgi->hdf, "config.max_install_proc", EV_PREINSTALL_CLIENT_INSTALL_PLUGIN_OK);;
	}
	else {
		hdf_set_value(stConfig.cgi->hdf, "config.valid_status", "2,3,4,5,6,7,8");
		hdf_set_int_value(stConfig.cgi->hdf, "config.max_install_proc", EV_PREINSTALL_SERVER_RECV_PLUGIN_MSG);;
	}

	if(pself_domain == NULL)
		pself_domain = hdf_get_value(stConfig.cgi->hdf, "CGI.ServerAddress", "");

	ostringstream oDownUrl;
	ostringstream oFile;
	if(MakeLocalPluginConf(pself_domain, pid, oDownUrl, oFile) < 0) {
		WARN_LOG("MakeLocalPluginConf failed, plugin:%d", pid);
		return SLOG_ERROR_LINE;
	}

	Query & qu = *stConfig.qu;
    std::ostringstream ss;
    ss << "select start_time,install_proc from mt_plugin_machine where machine_id=" << mid;
    ss << " and open_plugin_id=" << pid << " and xrk_status=0";

    // 最近5分钟是否有执行部署操作，如有则不允许再发起
    int iHasLastWork = 0;
    if(qu.get_result(ss.str().c_str()) &&  qu.num_rows() > 0) {
        qu.fetch_row();
        uint32_t dwStart = qu.getuval("start_time");
        int iProc = qu.getval("install_proc");
        if(iProc != 0 && dwStart+300 >= stConfig.dwCurTime) {
            iHasLastWork = 1;
            hdf_set_value(stConfig.cgi->hdf, "config.last_install_time", uitodate(dwStart));
            DEBUG_LOG("has last install, start time:%s(%u), cur:%u", uitodate(dwStart), dwStart, stConfig.dwCurTime);
        }
        else if(iProc == 0) {
            // 已安装了插件，前台应该不能发起部署
            qu.free_result();
            hdf_set_value(stConfig.cgi->hdf, "err.msg", CGI_REQERR);
            REQERR_LOG("plugin already install, machine:%d, plugin:%d !", mid, pid);
            return SLOG_ERROR_LINE;
        }
    }
    qu.free_result();
    hdf_set_int_value(stConfig.cgi->hdf, "config.has_last_work", iHasLastWork);
    if(!iHasLastWork) {
        ss.str("");
        ss << "replace into mt_plugin_machine set machine_id=" << mid;
        ss << ", open_plugin_id=" << pid << ", install_proc=" << EV_PREINSTALL_START;
        ss << ", start_time=" << stConfig.dwCurTime;
        ss << ", cfg_version=\'" << (const char*)(js_local["plus_version"]) << "\'";
        ss << ", build_version=\'" << (const char*)(js_local["plus_version"]) << "\'";
		ss << ", local_cfg_url=\'http://" << pself_domain << oDownUrl.str() << "\'";
        if(!qu.execute(ss.str().c_str())) 
            hdf_set_int_value(stConfig.cgi->hdf, "config.install_proc", MAX_INSTALL_PLUGIN_PROC+1);
        else 
            hdf_set_int_value(stConfig.cgi->hdf, "config.install_proc", EV_PREINSTALL_START);
    }
    else {
        hdf_set_int_value(stConfig.cgi->hdf, "config.install_proc", MAX_INSTALL_PLUGIN_PROC+1);
    }

    hdf_set_int_value(stConfig.cgi->hdf, "config.machine_id", mid);
    hdf_set_int_value(stConfig.cgi->hdf, "config.plugin_id", pid);
    DEBUG_LOG("machine:%d try install plugin:%d", mid, pid);
    return 0;
}

class TMachinePluginStatus {
    public:
    bool bDisabled;
    std::string strBuildVer;
};

static int DealDpAddPlugin()
{
	int id = hdf_get_int_value(stConfig.cgi->hdf, "Query.id", 0);
	if(id==0)
	{
		REQERR_LOG("invalid parameter from:%s", stConfig.remote);
		hdf_set_value(stConfig.cgi->hdf, "err.msg", CGI_REQERR);
		return SLOG_ERROR_LINE;
	}

	Json js_local;
	if(GetLocalPlugin(js_local, id) < 0)
		return SLOG_ERROR_LINE;

    hdf_set_int_value(stConfig.cgi->hdf, "config.plugin_id", id);
	hdf_set_value(stConfig.cgi->hdf, "config.plugin_name", (const char*)(js_local["plus_name"]));
    hdf_set_value(stConfig.cgi->hdf, "config.plugin_show_name", (const char*)(js_local["show_name"])); 
    hdf_set_value(stConfig.cgi->hdf, "config.plugin_last_version", (const char*)(js_local["plus_version"])); 
    hdf_set_value(stConfig.cgi->hdf, "config.plugin_run_os", (const char*)(js_local["run_os"]));
    hdf_set_value(stConfig.cgi->hdf, "config.plugin_auth", (const char*)(js_local["plugin_auth"]));
    hdf_set_value(stConfig.cgi->hdf, "config.plugin_language", (const char*)(js_local["dev_language"]));

	if((bool)(js_local["b_self_detail"])) {
		std::ostringstream ss;
		ss << stConfig.szXrkmonitorSiteAddr << "/plugin/" << (const char*)(js_local["plus_name"]) << ".html";
    	hdf_set_value(stConfig.cgi->hdf, "config.plugin_desc_url", ss.str().c_str());
	}
	else if(js_local.HasValue("plus_url"))
	    hdf_set_value(stConfig.cgi->hdf, "config.plugin_desc_url", (const char*)(js_local["plus_url"]));
	else
	    hdf_set_value(stConfig.cgi->hdf, "config.plugin_desc_url", stConfig.szXrkmonitorSiteAddr);

    std::map<int, TMachinePluginStatus> mpMachPluginStatus;
    std::map<int, TMachinePluginStatus>::iterator itMachStatus;

	Query qu(*stConfig.db);
	std::ostringstream ss;
    ss << "select last_hello_time,machine_id,build_version from mt_plugin_machine where "
		<< " xrk_status=0 and install_proc=0 and open_plugin_id=" << id;
    if(qu.get_result(ss.str().c_str()) && qu.num_rows() > 0) {
        for(int i=0; i < qu.num_rows() && qu.fetch_row(); i++) {
            TMachinePluginStatus stInfo;
            stInfo.bDisabled = (qu.getuval("last_hello_time") == 0 ? true : false);
            stInfo.strBuildVer = qu.getstr("build_version");
            mpMachPluginStatus.insert(std::make_pair<int, TMachinePluginStatus>(qu.getval("machine_id"), stInfo));
        }
    }
    qu.free_result();

    Json js;
	int iCount = 0;
    ss.str("");
	ss << "select * from mt_machine where xrk_status=0";
	if(qu.get_result(ss.str().c_str()) && qu.num_rows() > 0) {
		while(qu.fetch_row()) {
        	Json mach;
        	mach["id"] = qu.getval("xrk_id");
        	itMachStatus = mpMachPluginStatus.find((int)(mach["id"]));
            if(itMachStatus != mpMachPluginStatus.end()) {
                mach["plugin_ver"] = itMachStatus->second.strBuildVer;
                mach["plugin_disable"] = (int)(itMachStatus->second.bDisabled);
            }
            else 
                mach["plugin_ver"] = "no";

        	std::string ips;
        	if(qu.getuval("ip1") != 0) {
        	    ips += ipv4_addr_str(qu.getuval("ip1"));
        	    ips += " | ";
        	}
        	if(qu.getuval("ip2") != 0) {
        	    ips += ipv4_addr_str(qu.getuval("ip2"));
        	    ips += " | ";
        	}
        	if(qu.getuval("ip3") != 0) {
        	    ips += ipv4_addr_str(qu.getuval("ip3"));
        	    ips += " | ";
        	}
        	if(qu.getuval("ip4") != 0) {
        	    ips += ipv4_addr_str(qu.getuval("ip4"));
        	    ips += " | ";
        	}
        	if(ips.length() > 3)
        	    ips.erase(ips.length()-3, 3);

        	mach["ip"] = ips;
        	mach["name"] = qu.getstr("xrk_name");

			uint32_t dwLastHelloTime =  qu.getuval("last_hello_time");
			if(dwLastHelloTime > 0)
				mach["last_hello_time"] = uitodate(dwLastHelloTime);
			else
				mach["last_hello_time"] = "无";

			uint32_t dwAgentStartTime = qu.getuval("start_time");
        	if(dwAgentStartTime > 0 && dwLastHelloTime+300 >= stConfig.dwCurTime && stConfig.dwCurTime > dwAgentStartTime) 
        	    mach["run_time"] = stConfig.dwCurTime-dwAgentStartTime;
        	else
        	    mach["run_time"] = 0;

        	mach["run_os"] = qu.getstr("agent_os");
        	mach["os_arc"] = qu.getstr("os_arc");
        	mach["libc_ver"] = qu.getstr("libc_ver");
        	mach["libcpp_ver"] = qu.getstr("libcpp_ver");
        	std::string osType;
        	GetOsType(mach["run_os"], osType);
        	mach["run_os_type"] = osType;
        	js["list"].Add(mach);
        	iCount++;
		}
    }
	qu.free_result();

    js["count"] = iCount;
    std::string str(js.ToString());
    hdf_set_value(stConfig.cgi->hdf, "config.machine_list", str.c_str());
    hdf_set_int_value(stConfig.cgi->hdf, "config.max_opr_multi_machines", PLUGIN_MAX_OPR_MULTI_MACHINES);
    DEBUG_LOG("try setup plugin :%d, machine count:%d", id, iCount);
    return 0;
}

static int DealDownloadPluginConf(CGI *cgi)
{
	const char *pself_domain = hdf_get_value(cgi->hdf, "Query.self_domain", NULL);
	int iPluginId = hdf_get_int_value(cgi->hdf, "Query.plugin_id", 0);
	if(iPluginId <= 0) {
		REQERR_LOG("invalid plugin id:%d", iPluginId);
		return SLOG_ERROR_LINE;
	}

	ostringstream oDownUrl;
	ostringstream oFile;
	if(MakeLocalPluginConf(pself_domain, iPluginId, oDownUrl, oFile) < 0) {
		ERR_LOG("MakeLocalPluginConf failed, plugin:%d", iPluginId);
		return SLOG_ERROR_LINE;
	}

	Json js;
	js["ret"] = 0;
	js["downurl"] = oDownUrl.str();
	js["filename"] = oFile.str();
	STRING str;
	string_init(&str);
	if((stConfig.err=string_set(&str, js.ToString().c_str())) != STATUS_OK
		|| (stConfig.err=cgi_output(stConfig.cgi, &str)) != STATUS_OK)
	{
		string_clear(&str);
		stConfig.pErrMsg = CGI_ERR_SERVER;
		return SLOG_ERROR_LINE;
	}
	string_clear(&str);
	DEBUG_LOG("make plugin:%d config file:%s success", iPluginId, oFile.str().c_str());
	return 0;
}

static int DealUpdatePlugin(CGI *cgi)
{
	if(IsDaemonDenyOp(stConfig))
		return 0;

	const char *pinfo = hdf_get_value(cgi->hdf, "Query.plugin", NULL);
	if(pinfo == NULL) {
		REQERR_LOG("invalid parameter");
		return SLOG_ERROR_LINE;
	}

	size_t iParseIdx = 0;
	size_t iReadLen = strlen(pinfo);
	Json js_plugin;
	js_plugin.Parse(pinfo, iParseIdx);
	if(iParseIdx != iReadLen) {
		WARN_LOG("parse json content, size:%u!=%u", (uint32_t)iParseIdx, (uint32_t)iReadLen);
		stConfig.pErrMsg = "插件数据解析错误！";
		return SLOG_ERROR_LINE;
	}

	Json js_local;
	if(GetLocalPlugin(js_local, (int)(js_plugin["plugin_id"])) < 0)
		return SLOG_ERROR_LINE;

	if(!strcmp((const char*)(js_plugin["plus_version"]), (const char*)(js_local["plus_version"]))) {
		WARN_LOG("plus version is same, not need to update plugin:%s", (const char*)(js_plugin["plus_name"]));
		return SLOG_ERROR_LINE;
	}

	Query & qu = *(stConfig.qu);
	int iIsSqlFailed = -1;
	if(!qu.execute("START TRANSACTION"))
	    return SLOG_ERROR_LINE;
	CTransSavePlugin sMysqlTrans(qu, iIsSqlFailed);

	if((int)(js_plugin["b_add_log_module"]) && !(int)(js_local["b_add_log_module"])) {
		if(AddPluginLogModule(qu, js_local) < 0) {
			return SLOG_ERROR_LINE;
		}
		if(AddPluginLogConfig(qu, js_local) < 0) {
			return SLOG_ERROR_LINE;
		}
	}

	Json::json_list_t & jslist_attrtype = js_plugin["attr_types"].GetArray();
	Json::json_list_t & jslist_local_attrtype = js_local["attr_types"].GetArray();
	Json::json_list_t::iterator it_attrtype = jslist_attrtype.begin();
	while(it_attrtype != jslist_attrtype.end()) {
		Json::json_list_t::iterator it_local_attrtype = jslist_local_attrtype.begin();
		for(; it_local_attrtype != jslist_local_attrtype.end(); it_local_attrtype++) {
			if((int)((*it_local_attrtype)["plug_attr_type_id"]) 
				== (int)((*it_attrtype)["plug_attr_type_id"])) {
				// 尝试更新监控点类型
				if(TryUpdatePluginAttrType(qu, *it_local_attrtype, *it_attrtype) < 0) {
					return SLOG_ERROR_LINE;
				}
				break;
			}
		}

		// 新增监控点类型
		if(it_local_attrtype == jslist_local_attrtype.end()) {
			if(AddPluginAttrTypes(qu, *it_attrtype, js_local) < 0) {
				return SLOG_ERROR_LINE;
			}
			js_local["attr_types"].Add(*it_attrtype);
		}
		it_attrtype++;
	}

	Json::json_list_t & jslist_attr = js_plugin["attrs"].GetArray();
	Json::json_list_t & jslist_local_attr = js_local["attrs"].GetArray();
	Json::json_list_t::iterator it_attr = jslist_attr.begin();
	while(it_attr != jslist_attr.end()) {
		Json::json_list_t::iterator it_local_attr = jslist_local_attr.begin();
		for(; it_local_attr != jslist_local_attr.end(); it_local_attr++) {
			if((int)((*it_local_attr)["plug_attr_id"]) 
				== (int)((*it_attr)["plug_attr_id"])) {
				if(TryUpdatePluginAttr(qu, *it_local_attr, *it_attr, js_local) < 0) {
					return SLOG_ERROR_LINE;
				}
				break;
			}
		}
		if(it_local_attr == jslist_local_attr.end()) {
			if(AddPluginAttr(qu, *it_attr, js_local) < 0) {
				return SLOG_ERROR_LINE;
			}
			js_local["attrs"].Add(*it_attr);
		}
		it_attr++;
	}

	Json::json_list_t & jslist_cfg = js_plugin["cfgs"].GetArray();
	Json::json_list_t & jslist_local_cfg = js_local["cfgs"].GetArray();
	Json::json_list_t::iterator it_cfg = jslist_cfg.begin();
	while(it_cfg != jslist_cfg.end()) {
		Json &remote = *it_cfg;
		Json::json_list_t::iterator it_local_cfg = jslist_local_cfg.begin();
		for(; it_local_cfg != jslist_local_cfg.end(); it_local_cfg++) {
			Json &local = *it_local_cfg;
			if(!strcmp((const char*)(local["item_name"]), 
				(const char*)(remote["item_name"])))
			{
				bool bUpcfg = false;
				if(strcmp((const char*)(local["item_value"]), (const char*)(remote["item_value"]))) {
					local["item_value"] = (const char*)(remote["item_value"]);
					bUpcfg = true;
				}
				if(strcmp((const char*)(local["item_desc"]), (const char*)(remote["item_desc"]))) {
					local["item_desc"] = (const char*)(remote["item_desc"]);
					bUpcfg = true;
				}
				if((bool)(local["enable_modify"]) != (bool)(remote["enable_modify"])) {
					local["enable_modify"] = (bool)(remote["enable_modify"]);
					bUpcfg = true;
				}
				if(bUpcfg) {
					DEBUG_LOG("try update cfg:%s", (const char*)(local["item_name"]));
				}
				break;
			}
		}
		if(it_local_cfg == jslist_local_cfg.end()) {
			js_local["cfgs"].Add(remote);
			DEBUG_LOG("add config:%s", (const char*)(remote["item_name"]));
		}
		it_cfg++;
	}

	// 检查配置删除
	Json::json_list_t::iterator it_local_cfg = jslist_local_cfg.begin();
	while(it_local_cfg != jslist_local_cfg.end()) {
		Json &local = *it_local_cfg;
		for(it_cfg = jslist_cfg.begin(); it_cfg!=jslist_cfg.end(); it_cfg++) {
			Json &remote = *it_cfg;
			if(!strcmp((const char*)(local["item_name"]),
				(const char*)(remote["item_name"])))
			{
				break;
			}
		}
		if(it_cfg == jslist_cfg.end()) {
			DEBUG_LOG("remove plugin:%s, cfg:%s",
				(const char*)(js_local["plus_name"]), (const char*)(local["item_name"]));
			jslist_local_cfg.erase(it_local_cfg++);
		}
		else
			it_local_cfg++;
	}

	std::string strSql;
	IM_SQL_PARA* ppara = NULL;
	if(InitParameter(&ppara) < 0) {
		ERR_LOG("sql parameter init failed !");
		return SLOG_ERROR_LINE;
	}
	js_local["plus_version"] = (const char*)(js_plugin["plus_version"]);
	if((int)(js_plugin["b_add_log_module"]) && !(int)(js_local["b_add_log_module"])) {
		js_local["b_add_log_module"] = (int)(js_plugin["b_add_log_module"]);
	}
	if(strcmp((const char*)(js_plugin["plus_desc"]), (const char*)(js_local["plus_desc"]))) {
		js_local["plus_desc"] = (const char*)(js_plugin["plus_desc"]);
	}
	if(strcmp((const char*)(js_plugin["plugin_auth"]), (const char*)(js_local["plugin_auth"]))) {
		js_local["plugin_auth"] = (const char*)(js_plugin["plugin_auth"]);
	}
	if((bool)(js_plugin["b_open_source"]) != (bool)(js_local["b_open_source"])) {
		js_local["b_open_source"] = (bool)(js_plugin["b_open_source"]);
	}
	if(strcmp((const char*)(js_plugin["run_os"]), (const char*)(js_local["run_os"]))) {
		js_local["run_os"] = (const char*)(js_plugin["run_os"]);
	}
	if((int)(js_plugin["set_method"]) != (int)(js_local["set_method"])) {
		js_local["set_method"] = (int)(js_plugin["set_method"]);
	}
	if(strcmp((const char*)(js_plugin["dev_language"]), (const char*)(js_local["dev_language"]))) {
		js_local["dev_language"] = (const char*)(js_plugin["dev_language"]);
	}
	if(!IsStrEqual((const char*)(js_plugin["pic_filename"]), (const char*)(js_local["pic_filename"]))) {
		js_local["pic_filename"] = (const char*)(js_plugin["pic_filename"]);
	}
	if(strcmp((const char*)(js_plugin["plus_url"]), (const char*)(js_local["plus_url"]))) {
		js_local["plus_url"] = (const char*)(js_plugin["plus_url"]);
	}
	if(!IsStrEqual((const char*)(js_plugin["plus_name"]), (const char*)(js_local["plus_name"]))) {
		js_local["plus_name"] = (const char*)(js_plugin["plus_name"]);
		AddParameter(&ppara, "plugin_name", (const char*)(js_plugin["plus_name"]), NULL);
	}
	if(!IsStrEqual((const char*)(js_plugin["show_name"]), (const char*)(js_local["show_name"]))) {
		js_local["show_name"] = (const char*)(js_plugin["show_name"]);
		AddParameter(&ppara, "plugin_show_name", (const char*)(js_plugin["show_name"]), NULL);
	}
	if(!IsStrEqual((const char*)(js_plugin["install_tp_file"]), (const char*)(js_local["install_tp_file"]))) {
		js_local["install_tp_file"] = (const char*)(js_plugin["install_tp_file"]);
	}
	if((bool)(js_plugin["b_self_detail"]) != (bool)(js_local["b_self_detail"])) {
		js_local["b_self_detail"] = (bool)(js_plugin["b_self_detail"]);
	}

	std::string strJs = js_local.ToString();
	AddParameter(&ppara, "pb_info", strJs.c_str(), NULL);

	strSql = "update mt_plugin set ";
	JoinParameter_Set(&strSql, qu.GetMysql(), ppara);
	strSql += " where open_plugin_id=";
	strSql += itoa((int)(js_plugin["plugin_id"]));
	ReleaseParameter(&ppara);
	if(!qu.execute(strSql.c_str()))
	{
		stConfig.pErrMsg = CGI_ERR_SERVER;
		return SLOG_ERROR_LINE;
	}

	iIsSqlFailed = 0;

	Json js;
	js["ret"] = 0;
	STRING str;
	string_init(&str);
	if((stConfig.err=string_set(&str, js.ToString().c_str())) != STATUS_OK
		|| (stConfig.err=cgi_output(stConfig.cgi, &str)) != STATUS_OK)
	{
		string_clear(&str);
		stConfig.pErrMsg = CGI_ERR_SERVER;
		return SLOG_ERROR_LINE;
	}
	string_clear(&str);
	DEBUG_LOG("update open plugin:%s success", (const char*)(js_plugin["plus_name"]));
	return 0;
}

static int DealInstallPlugin(CGI *cgi)
{
	if(IsDaemonDenyOp(stConfig))
		return 0;

	const char *pinfo = hdf_get_value(cgi->hdf, "Query.plugin", NULL);
	if(pinfo == NULL) {
		REQERR_LOG("invalid parameter");
		return SLOG_ERROR_LINE;
	}

	size_t iParseIdx = 0;
	size_t iReadLen = strlen(pinfo);
	Json js_plugin;
	js_plugin.Parse(pinfo, iParseIdx);
	if(iParseIdx != iReadLen) {
		WARN_LOG("parse json content, size:%u!=%u", (uint32_t)iParseIdx, (uint32_t)iReadLen);
		stConfig.pErrMsg = "插件数据解析错误！";
		return SLOG_ERROR_LINE;
	}

	Query & qu = *(stConfig.qu);
	int iIsSqlFailed = -1;
	if(!qu.execute("START TRANSACTION"))
	    return SLOG_ERROR_LINE;
	CTransSavePlugin sMysqlTrans(qu, iIsSqlFailed);

	if(AddPlugin(qu, js_plugin) < 0)
		return SLOG_ERROR_LINE;

	if((int)(js_plugin["b_add_log_module"])) {
		if(AddPluginLogModule(qu, js_plugin) < 0)
			return SLOG_ERROR_LINE;
		if(AddPluginLogConfig(qu, js_plugin) < 0)
			return SLOG_ERROR_LINE;
	}

	if(AddPluginParentAttrTypes(qu, js_plugin) < 0) 
		return SLOG_ERROR_LINE;

	Json::json_list_t & jslist_attrtype = js_plugin["attr_types"].GetArray();
	Json::json_list_t::iterator it_attrtype = jslist_attrtype.begin();
	while(it_attrtype != jslist_attrtype.end()) {
		if(AddPluginAttrTypes(qu, *it_attrtype, js_plugin) < 0)
			return SLOG_ERROR_LINE;
		it_attrtype++;
	}

	Json::json_list_t & jslist_attr = js_plugin["attrs"].GetArray();
	Json::json_list_t::iterator it_attr = jslist_attr.begin();
	while(it_attr != jslist_attr.end()) {
		if(AddPluginAttr(qu, *it_attr, js_plugin) < 0)
			return SLOG_ERROR_LINE;
		it_attr++;
	}

    std::ostringstream sql; 
	sql << "update mt_plugin set pb_info=\'" <<  js_plugin.ToString() << "\' where open_plugin_id="
		<< (int)(js_plugin["plugin_id"]) << ";";
	if(!qu.execute(sql.str().c_str()))
	{
		stConfig.pErrMsg = CGI_ERR_SERVER;
		return SLOG_ERROR_LINE;
	}
	iIsSqlFailed = 0;

	Json js;
	js["ret"] = 0;
	js["plugin_id"] = (int)(js_plugin["plugin_id"]);
	STRING str;
	string_init(&str);
	if((stConfig.err=string_set(&str, js.ToString().c_str())) != STATUS_OK
		|| (stConfig.err=cgi_output(stConfig.cgi, &str)) != STATUS_OK)
	{
		string_clear(&str);
		stConfig.pErrMsg = CGI_ERR_SERVER;
		return SLOG_ERROR_LINE;
	}
	string_clear(&str);
	DEBUG_LOG("install open plugin:%s success", (const char*)(js_plugin["plus_name"]));
	return 0;
}

// 机器插件变更状态
enum {
    // 机器未安装插件
    MOP_RET_NOT_INSTALL_PLUGIN = 1,  
    // 最近5分钟内有未完成的修改操作
    MOP_RET_HAS_WAIT_OPR = 2,
    // 修改操作等待完成
    MOP_RET_WAIT_COMPLETE = 3,
    // 修改失败，服务器错误 
    MOP_RET_SERVER_FAILED = 4,
    // 未找到操作任务
    MOP_RET_NOT_FIND_OPR = 5,
    // 操作超时(30s 没有完成)
    MOP_RET_OPR_TIMEOUT = 6,
    // agent 未启动
    MOP_RET_AGENT_NOT_START = 7,
    // 在机器上未找到插件
    MOP_RET_NOT_FIND_PLUGIN = 8,
    // 已接收
    MOP_RET_OPR_DB_RECV = 12,
    // 已下发
    MOP_RET_OPR_DOWNLOAD = 13,
    // 操作失败
    MOP_RET_OPR_FAILED = 14,
    // 操作成功
    MOP_RET_OPR_SUCCESS = 15,
    // agent 响应超时
    MOP_RET_OPR_AGENT_TIMEOUT = 16,
};

static int DealOprMachPlugins(std::string &strCsTemplateFile)
{
    stConfig.iResponseType = RESPONSE_TYPE_HTML;
	const char *pp = hdf_get_value(stConfig.cgi->hdf, "Query.plugins", NULL);
    int iMachine = hdf_get_int_value(stConfig.cgi->hdf, "Query.machine", 0);
    int iOprType = hdf_get_int_value(stConfig.cgi->hdf, "Query.opr_type", 0);
	if(!pp || !iMachine || iOprType <= 0 || iOprType > 4)
	{
		REQERR_LOG("invalid parameter pp:%p, iMachine:%d, iOprType:%d", pp, iMachine, iOprType);
		hdf_set_value(stConfig.cgi->hdf, "err.msg", CGI_REQERR);
		return SLOG_ERROR_LINE;
	}
    
    // 机器校验
    MachineInfo *pmach = slog.GetMachineInfo(iMachine, NULL);
    if(!pmach) {
		REQERR_LOG("not find machine:%d", iMachine);
        hdf_set_value(stConfig.cgi->hdf, "err.msg", CGI_REQERR);
        return SLOG_ERROR_LINE;
    }
    hdf_set_int_value(stConfig.cgi->hdf, "config.opr_type", iOprType);
    hdf_set_int_value(stConfig.cgi->hdf, "config.machine_id", iMachine);

    Json js;
    std::string strPlugin;
    std::istringstream iss(pp);
    int iAryPlugins[PLUGIN_MAX_OPR_MULTI_PLUGINS+1], i = 0;
    for(i=0; i < PLUGIN_MAX_OPR_MULTI_PLUGINS+1 && getline(iss, strPlugin, ','); ) {
        if(strPlugin.size() > 0) {
            iAryPlugins[i] = atoi(strPlugin.c_str());
            i++;
        }
    }
    int iPlugCount = i;
    if(iPlugCount <= 0 || iPlugCount > PLUGIN_MAX_OPR_MULTI_PLUGINS) {
        REQERR_LOG("plugin count invalid:%d", iPlugCount);
        hdf_set_value(stConfig.cgi->hdf, "err.msg", CGI_REQERR);
        return SLOG_ERROR_LINE;
    }

	MyQuery myqu(stConfig.qu, stConfig.db);
	Query & qu = myqu.GetQuery();
    std::ostringstream ss;

    DEBUG_LOG("multi plugin machine - pp:%s, machine:%d, oprtype:%d", pp, iMachine, iOprType);
    int iWaitCount = 0;
    for(i=0; i < iPlugCount; ++i) {
        Json jr;

        if(pmach->dwAgentStartTime <= 0 || pmach->dwLastHelloTime+300 < stConfig.dwCurTime)
        {
            jr["ret"] = MOP_RET_AGENT_NOT_START;
            jr["plugin"] = iAryPlugins[i];
            js["list"].Add(jr);
            continue;
        }

        ss.str("");
        ss << "select last_hello_time,opr_start_time,down_opr_cmd,opr_proc from mt_plugin_machine where machine_id=" 
            << iMachine <<  " and open_plugin_id=" << iAryPlugins[i] << " and install_proc=0 and xrk_status=0";

        // 最近5分钟是否有执行修改操作，如有则不允许再发起
        if(qu.get_result(ss.str().c_str()) &&  qu.num_rows() > 0) {
            qu.fetch_row();
            uint32_t dwStart = qu.getuval("opr_start_time");
            int iProc = qu.getval("opr_proc");
            if(iProc != 0 
                && iProc != EV_MOP_OPR_FAILED
                && iProc != EV_MOP_OPR_SUCCESS
                && iProc != EV_MOP_OPR_RESPONSE_TIMEOUT
                && dwStart+300 >= stConfig.dwCurTime) 
            {
                hdf_set_value(stConfig.cgi->hdf, "config.last_opr_time", uitodate(dwStart));
                DEBUG_LOG("has last opr, start time:%s(%u), cur:%u", uitodate(dwStart), dwStart, stConfig.dwCurTime);
                jr["ret"] = MOP_RET_HAS_WAIT_OPR;
                jr["plugin"] = iAryPlugins[i];
                js["list"].Add(jr);
                qu.free_result();
                continue;
            }

            if(iOprType == 4 && qu.getval("last_hello_time") == 0) {
                // 已是禁用状态
                qu.free_result();
                jr["ret"] = MOP_RET_OPR_SUCCESS;
            }
            else {
                qu.free_result();
                ss.str("");
                ss << "update mt_plugin_machine set opr_start_time=" << stConfig.dwCurTime << ", opr_proc=1, down_opr_cmd="
                    << iOprType << " where machine_id=" << iMachine <<  " and open_plugin_id=" << iAryPlugins[i];
                if(qu.execute(ss.str())) {
                    jr["ret"] = MOP_RET_WAIT_COMPLETE;
                    iWaitCount++;
                }
                else
                    jr["ret"] = MOP_RET_SERVER_FAILED;
            }

            jr["plugin"] = iAryPlugins[i];
            js["list"].Add(jr);
        }
        else {
            jr["ret"] = MOP_RET_NOT_INSTALL_PLUGIN;
            jr["plugin"] = iAryPlugins[i];
            js["list"].Add(jr);
            REQERR_LOG("machine:%d not install plugin:%d", iMachine, iAryPlugins[i]);
            qu.free_result();
        }
    }
    js["wait_count"] = iWaitCount;
    js["count"] = iPlugCount;

    std::string str(js.ToString());
    hdf_set_int_value(stConfig.cgi->hdf, "config.wait_status", MOP_RET_WAIT_COMPLETE);
    hdf_set_value(stConfig.cgi->hdf, "config.pp_status", str.c_str());
    DEBUG_LOG("machine plugin opr deal result:%s", str.c_str());
    strCsTemplateFile = "dmt_dlg_mach_opr_plugins.html";
    return 0;
}

static int DealRefreshOprMachinePlugin(std::string &strCsTemplateFile)
{
	const char *pm = hdf_get_value(stConfig.cgi->hdf, "Query.machines", NULL);
    const char *pp = hdf_get_value(stConfig.cgi->hdf, "Query.plugins", NULL);
    int iMultiPlugin = hdf_get_int_value(stConfig.cgi->hdf, "Query.multi_plugins", 0);
    int iOprType = hdf_get_int_value(stConfig.cgi->hdf, "Query.opr_type", 0);
	if(!pm || !pp || iOprType <= 0 || iOprType > 4)
	{
		REQERR_LOG("invalid parameter pm:%p, pp:%p, iOprType:%d", pm, pp, iOprType);
		hdf_set_value(stConfig.cgi->hdf, "err.msg", CGI_REQERR);
		return SLOG_ERROR_LINE;
	}

    // 插件提取
    std::string strPlug;
    std::istringstream iss_plug(pp);
    int iAryPlugins[PLUGIN_MAX_OPR_MULTI_PLUGINS+1], i = 0;
    for(i=0; i < PLUGIN_MAX_OPR_MULTI_PLUGINS+1 && getline(iss_plug, strPlug, ','); ) {
        if(strPlug.size() > 0) {
            iAryPlugins[i] = atoi(strPlug.c_str());
            i++;
        }
    }
    int iPluginCount = i;

    // 机器提取
    std::string strMach;
    std::istringstream iss(pm);
    int iAryMachines[PLUGIN_MAX_OPR_MULTI_MACHINES+1];
    for(i=0; i < PLUGIN_MAX_OPR_MULTI_MACHINES+1 && getline(iss, strMach, ','); ) {
        if(strMach.size() > 0) {
            iAryMachines[i] = atoi(strMach.c_str());
            i++;
        }
    }
    int iMachCount = i;

    // 查询状态
    Json js;
	MyQuery myqu(stConfig.qu, stConfig.db);
	Query & qu = myqu.GetQuery();
    std::ostringstream ss;
    int iWaitCount = 0;
    for(i=0; i < iMachCount; ++i) {
        for(int j=0; j < iPluginCount; ++j) {
            Json jr;
            ss.str("");
            ss << "select opr_start_time,opr_proc,xrk_status from mt_plugin_machine where machine_id=" 
                << iAryMachines[i] <<  " and open_plugin_id=" << iAryPlugins[j] << " and install_proc=0 "
                << " and down_opr_cmd=" << iOprType;
            if(qu.get_result(ss.str().c_str()) &&  qu.num_rows() > 0) {
                qu.fetch_row();
                uint32_t dwStart = qu.getuval("opr_start_time");
                // iProc 进度
                int iProc = qu.getval("opr_proc");
                if(iOprType != 2 && qu.getval("xrk_status") == 1)
                    jr["ret_proc"] = MOP_RET_NOT_FIND_PLUGIN;
                else if(dwStart+30 < stConfig.dwCurTime)
                    jr["ret_proc"] = MOP_RET_OPR_TIMEOUT;
                else {
                    if(iProc == EV_MOP_OPR_START)
                        jr["ret_proc"] = MOP_RET_WAIT_COMPLETE;
                    else
                        jr["ret_proc"] = 10+iProc;
                    iWaitCount++;
                }
                if(iMultiPlugin)
                    jr["obj_id"] = iAryPlugins[j];
                else
                    jr["obj_id"] = iAryMachines[i];
            }
            else {
                jr["ret_proc"] = MOP_RET_NOT_FIND_OPR;
                if(iMultiPlugin)
                    jr["obj_id"] = iAryPlugins[j];
                else
                    jr["obj_id"] = iAryMachines[i];
                REQERR_LOG("machine:%d not find opr, plugin:%d", iAryMachines[i], iAryPlugins[j]);
            }
            js["list"].Add(jr);
            qu.free_result();
        }
    }
    js["ret"] = 0;
    if(iMultiPlugin)
        js["count"] = iPluginCount;
    else
        js["count"] = iMachCount;
    js["wait_count"] = iWaitCount;

	std::string str(js.ToString());
    DEBUG_LOG("refresh machine opr plugin process status:%s", str.c_str());
    return my_cgi_output(str.c_str(), stConfig);
}

static int DealOprMachinePlugin(std::string &strCsTemplateFile)
{
    stConfig.iResponseType = RESPONSE_TYPE_HTML;
	const char *pm = hdf_get_value(stConfig.cgi->hdf, "Query.machines", NULL);
    int iPluginId = hdf_get_int_value(stConfig.cgi->hdf, "Query.plugin", 0);
    int iOprType = hdf_get_int_value(stConfig.cgi->hdf, "Query.opr_type", 0);
	if(!pm || !iPluginId || iOprType <= 0 || iOprType > 4)
	{
		REQERR_LOG("invalid parameter pm:%p, iPluginId:%d, iOprType:%d", pm, iPluginId, iOprType);
		hdf_set_value(stConfig.cgi->hdf, "err.msg", CGI_REQERR);
		return SLOG_ERROR_LINE;
	}
    
    hdf_set_int_value(stConfig.cgi->hdf, "config.opr_type", iOprType);
    hdf_set_int_value(stConfig.cgi->hdf, "config.plugin_id", iPluginId);

    // 机器校验和提取
    Json js;
    std::string strMach;
    std::istringstream iss(pm);
    int iAryMachines[PLUGIN_MAX_OPR_MULTI_MACHINES+1], i = 0;
    MachineInfo *pMachsInfo[PLUGIN_MAX_OPR_MULTI_MACHINES+1];

    std::ostringstream ssMachines;
    for(i=0; i < PLUGIN_MAX_OPR_MULTI_MACHINES+1 && getline(iss, strMach, ','); ) {
        if(strMach.size() > 0) {
            iAryMachines[i] = atoi(strMach.c_str());
            pMachsInfo[i] = slog.GetMachineInfo(iAryMachines[i], NULL);
            if(!pMachsInfo[i]) {
                REQERR_LOG("not find machine:%d", iAryMachines[i]);
                hdf_set_value(stConfig.cgi->hdf, "err.msg", CGI_REQERR);
                return SLOG_ERROR_LINE;
            }
            if(ssMachines.str().size() > 0)
                ssMachines << " " << iAryMachines[i];
            else
                ssMachines << iAryMachines[i];
            i++;
        }
    }
    int iMachCount = i;
    if(iMachCount <= 0 || iMachCount > PLUGIN_MAX_OPR_MULTI_MACHINES) {
        REQERR_LOG("machine count invalid:%d", iMachCount);
        hdf_set_value(stConfig.cgi->hdf, "err.msg", CGI_REQERR);
        return SLOG_ERROR_LINE;
    }

	MyQuery myqu(stConfig.qu, stConfig.db);
	Query & qu = myqu.GetQuery();
    std::ostringstream ss;

    DEBUG_LOG("multi plugin machine - pm:%s, plugin:%d, oprtype:%d", pm, iPluginId, iOprType);

    // 修改插件配置
    if(iOprType == 1) {
		Json plug_info;
		if(GetLocalPlugin(plug_info, iPluginId) < 0) {
            REQERR_LOG("get plugin:%d info failed", iPluginId);
            return SLOG_ERROR_LINE;
        }

        hdf_set_value(stConfig.cgi->hdf, "config.machines_id", ssMachines.str().c_str());
        hdf_set_int_value(stConfig.cgi->hdf, "config.machine_count", iMachCount);

        // 修改1台机器的配置，可以加载当前配置，基于当前配置修改
        bool bLoadCurCfg = false;
        char hdf_pex[32] = {0};
        if(iMachCount == 1) {
            ss.str("");
            ss << "select cfg_file_time,xrk_cfgs_list from mt_plugin_machine where machine_id="
                << iAryMachines[0] << " and open_plugin_id=" << iPluginId << " and xrk_status=0";
            if(qu.get_result(ss.str().c_str()) &&  qu.num_rows() > 0) {
                qu.fetch_row();
                const char *pstrCfgsCur = qu.getstr("xrk_cfgs_list");
                const char *pst = NULL, *ped = NULL;
                if(strlen(pstrCfgsCur) > 0) {
                    hdf_set_value(stConfig.cgi->hdf, "config.cur_cfg_time", uitodate(qu.getuval("cfg_file_time")));
                    hdf_set_value(stConfig.cgi->hdf, "config.machine_name", 
                        MtReport_GetFromVmem_Local(pMachsInfo[0]->iNameVmemIdx));
                    DEBUG_LOG("get plugin:%d, last mod time:%u, config str:%s", 
                        iPluginId, qu.getuval("cfg_file_time"), pstrCfgsCur);

					Json::json_list_t & jslist_cfg = plug_info["cfgs"].GetArray();
					Json::json_list_t::iterator it_cfg = jslist_cfg.begin();
					for(int j=0; it_cfg != jslist_cfg.end(); it_cfg++) {
						Json &cfg = *it_cfg;
						if((bool)(cfg["enable_modify"])) {
                            sprintf(hdf_pex, "plug_cfg.list.%d", ++j);
							const char *pcfgname = (const char*)(cfg["item_name"]);
                            hdf_set_valuef(stConfig.cgi->hdf, "%s.name=%s", hdf_pex, pcfgname);
                            hdf_set_valuef(stConfig.cgi->hdf, "%s.desc=%s", hdf_pex, (const char*)(cfg["item_desc"]));
                            pst = strstr(pstrCfgsCur, pcfgname);
                            if(pst != NULL) {
                                pst += strlen(pcfgname)+1; // 1 - 空格
                                ped = strchr(pst, ';');
                                if(!ped)
                                    ped = pst + strlen(pst);
                            }

                            if(pst) {
                                std::string strval(pst, ped-pst);
                                hdf_set_valuef(stConfig.cgi->hdf, "%s.value=%s", hdf_pex, strval.c_str());
                                DEBUG_LOG("get plugin:%d config item:%s val:%s", iPluginId, pcfgname, strval.c_str());
                            }
                            else
                                hdf_set_valuef(stConfig.cgi->hdf, "%s.value=%s", hdf_pex, (const char*)(cfg["item_value"]));
                        }
                    }
                    bLoadCurCfg = true;
                }
                qu.free_result();
            }
        }

        if(!bLoadCurCfg || iMachCount > 1) {
			Json::json_list_t & jslist_cfg = plug_info["cfgs"].GetArray();
			Json::json_list_t::iterator it_cfg = jslist_cfg.begin();
			for(int j=0; it_cfg != jslist_cfg.end(); it_cfg++) {
				Json &cfg = *it_cfg;
				if((bool)(cfg["enable_modify"])) {
					sprintf(hdf_pex, "plug_cfg.list.%d", ++j);
                    hdf_set_valuef(stConfig.cgi->hdf, "%s.name=%s", hdf_pex, (const char*)(cfg["item_name"]));
                    hdf_set_valuef(stConfig.cgi->hdf, "%s.value=%s", hdf_pex, (const char*)(cfg["item_value"]));
                    hdf_set_valuef(stConfig.cgi->hdf, "%s.desc=%s", hdf_pex, (const char*)(cfg["item_desc"]));
                }
            }
        }
        strCsTemplateFile = "dmt_dlg_machine_mod_plugin_cfg.html";
        return 0;
    }

    int iWaitCount = 0;
    for(i=0; i < iMachCount; ++i) {
        Json jr;

        if(pMachsInfo[i]->dwAgentStartTime <= 0 || pMachsInfo[i]->dwLastHelloTime+300 < stConfig.dwCurTime)
        {
            jr["ret"] = MOP_RET_AGENT_NOT_START;
            jr["machine"] = iAryMachines[i];
            js["list"].Add(jr);
            continue;
        }

        ss.str("");
        ss << "select last_hello_time,opr_start_time,down_opr_cmd,opr_proc from mt_plugin_machine where machine_id=" 
            << iAryMachines[i] <<  " and open_plugin_id=" << iPluginId << " and install_proc=0 and xrk_status=0";

        // 最近5分钟是否有执行修改操作，如有则不允许再发起
        if(qu.get_result(ss.str().c_str()) &&  qu.num_rows() > 0) {
            qu.fetch_row();
            uint32_t dwStart = qu.getuval("opr_start_time");
            int iProc = qu.getval("opr_proc");
            if(iProc != 0 
                && iProc != EV_MOP_OPR_FAILED
                && iProc != EV_MOP_OPR_SUCCESS
                && iProc != EV_MOP_OPR_RESPONSE_TIMEOUT
                && dwStart+300 >= stConfig.dwCurTime) 
            {
                hdf_set_value(stConfig.cgi->hdf, "config.last_opr_time", uitodate(dwStart));
                DEBUG_LOG("has last opr, start time:%s(%u), cur:%u", uitodate(dwStart), dwStart, stConfig.dwCurTime);
                jr["ret"] = MOP_RET_HAS_WAIT_OPR;
                jr["machine"] = iAryMachines[i];
                js["list"].Add(jr);
                qu.free_result();
                continue;
            }

            if(iOprType == 4 && qu.getval("last_hello_time") == 0) {
                // 已是禁用状态
                qu.free_result();
                jr["ret"] = MOP_RET_OPR_SUCCESS;
            }
            else {
                qu.free_result();
                ss.str("");
                ss << "update mt_plugin_machine set opr_start_time=" << stConfig.dwCurTime << ", opr_proc=1, down_opr_cmd="
                    << iOprType << " where machine_id=" << iAryMachines[i] <<  " and open_plugin_id=" << iPluginId;
                if(qu.execute(ss.str())) {
                    jr["ret"] = MOP_RET_WAIT_COMPLETE;
                    iWaitCount++;
                }
                else
                    jr["ret"] = MOP_RET_SERVER_FAILED;
            }

            jr["machine"] = iAryMachines[i];
            js["list"].Add(jr);
        }
        else {
            jr["ret"] = MOP_RET_NOT_INSTALL_PLUGIN;
            jr["machine"] = iAryMachines[i];
            js["list"].Add(jr);
            REQERR_LOG("machine:%d not install plugin:%d", iAryMachines[i], iPluginId);
            qu.free_result();
        }
    }
    js["count"] = iMachCount;
    js["wait_count"] = iWaitCount;

    std::string str(js.ToString());
    hdf_set_int_value(stConfig.cgi->hdf, "config.wait_status", MOP_RET_WAIT_COMPLETE);
    hdf_set_value(stConfig.cgi->hdf, "config.pm_status", str.c_str());
    DEBUG_LOG("machine plugin opr deal result:%s", str.c_str());
    strCsTemplateFile = "dmt_dlg_machine_opr_plugin.html";
    return 0;
}

int DealSaveOprMachinePlugin()
{
    int iPluginId = hdf_get_int_value(stConfig.cgi->hdf, "Query.plugin", 0);
    const char *pmachs = hdf_get_value(stConfig.cgi->hdf, "Query.machines", 0);

    std::list<int> ltMachines;
    std::istringstream ssMachines(pmachs);
    int iMachineId = 0, iMachCount = 0;
    while(ssMachines >> iMachineId) {
        MachineInfo *pMachineInfo = slog.GetMachineInfo(iMachineId, NULL);
        if(!pMachineInfo) {
            REQERR_LOG("not find machine:%d", iMachineId);
            hdf_set_value(stConfig.cgi->hdf, "err.msg", CGI_REQERR);
            return SLOG_ERROR_LINE;
        }
        ltMachines.push_back(iMachineId);
        iMachCount++;
    }
    if(iMachCount <= 0) {
        REQERR_LOG("have no machines for modify plugin:%d", iPluginId);
        hdf_set_value(stConfig.cgi->hdf, "err.msg", CGI_REQERR);
        return SLOG_ERROR_LINE;
    }

	Json plug_info;
	if(GetLocalPlugin(plug_info, iPluginId) < 0) {
		REQERR_LOG("get plugin:%d info failed", iPluginId);
		return SLOG_ERROR_LINE;
	}

    const char *pval = NULL, *pchk = NULL;
    int iModCfgItemCount = 0;
    std::ostringstream ss;
	Json::json_list_t & jslist_cfg = plug_info["cfgs"].GetArray();
	Json::json_list_t::iterator it_cfg = jslist_cfg.begin();
	for(int i=0; it_cfg != jslist_cfg.end(); it_cfg++,i++) {
		Json &cfg = *it_cfg;
		if((bool)(cfg["enable_modify"])) {
			const char *pcfgname = (const char*)(cfg["item_name"]);
            pchk = hdf_get_valuef(stConfig.cgi->hdf, "Query.chk_%s", pcfgname);
            pval = hdf_get_valuef(stConfig.cgi->hdf, "Query.%s", pcfgname);
            if(pval && strlen(pval) > 0 && IsStrEqual(pchk, "on")) {
                if(ss.str().size() > 0)
                    ss << ";" << pcfgname << " " << pval;
                else
                    ss << pcfgname << " " << pval;
                iModCfgItemCount++;
            }
            else {
                DEBUG_LOG("skip config item:%s, pval:%p, pchk:%p", pcfgname, pval, pchk);
            }
        }
    }
    if(iModCfgItemCount > 0) {
        MyQuery myqu(stConfig.qu, stConfig.db);
        Query & qu = myqu.GetQuery();
        std::ostringstream sql;

        if(!qu.execute("START TRANSACTION")) {
            WARN_LOG("START TRANSACTION failed");
            hdf_set_value(stConfig.cgi->hdf, "err.msg", CGI_ERR_SERVER);
            return SLOG_ERROR_LINE;
        }

        int iRestartPlugin = 0;
        pchk = hdf_get_value(stConfig.cgi->hdf, "Query.chk_ddmmpc_restart_plugin", NULL);
        if(IsStrEqual(pchk, "on"))
            iRestartPlugin = 1;

        std::list<int>::iterator it_list = ltMachines.begin();
        for(; it_list != ltMachines.end(); it_list++) {
            sql.str("");
            sql << "update mt_plugin_machine set opr_start_time=" << slog.m_stNow.tv_sec 
                << ", down_opr_cmd=1" << ", opr_proc=1, down_cfgs_restart=" << iRestartPlugin
                << ", down_cfgs_list=\'" << ss.str() << "\'" << " where machine_id=" << (int)(*it_list) 
                << " and open_plugin_id=" << iPluginId << " and xrk_status=0";
            qu.execute(sql.str());
            if(qu.affected_rows() != 1) {
                WARN_LOG("execute sql:%s failed", sql.str().c_str());
                hdf_set_value(stConfig.cgi->hdf, "err.msg", CGI_ERR_SERVER);
                qu.execute("ROLLBACK");
                return SLOG_ERROR_LINE;
            }
        }
        qu.execute("COMMIT");
    }
    else {
        REQERR_LOG("have no config item to update, plugin:%d, machine:%d", iPluginId, iMachineId);
        hdf_set_value(stConfig.cgi->hdf, "err.msg", "插件配置项提取失败");
        return SLOG_ERROR_LINE;
    }

	Json js;
	js["statusCode"] = 200;
	js["callbackType"] = "closeCurrent";
	js["msgid"] = "modSuccess";

    std::string str = js.ToString();
    my_cgi_output(str.c_str(), stConfig);
    DEBUG_LOG("result:%s", str.c_str());
    return 0;
}

int main(int argc, char **argv, char **envp)
{
	int32_t iRet = 0;
	stConfig.argc = argc;
	stConfig.argv = argv;
	if((iRet=InitFastCgi_first(stConfig)) < 0)
	{
		printf("InitCgi failed ! ret:%d", iRet);
		return -1;
	}

	if(!slog.GetAttrTypeInfo(PLUGIN_PARENT_ATTR_TYPE, NULL)) {
		ERR_LOG("not find plugin parent attr type:%d", PLUGIN_PARENT_ATTR_TYPE);
		return -2;
	}

	if(!slog.GetAppInfo(PLUGIN_PARENT_APP_ID)) {
		ERR_LOG("not find plugin parent app:%d", PLUGIN_PARENT_APP_ID);
		return -3;
	}

	INFO_LOG("fcgi:%s argc:%d start pid:%u", stConfig.pszCgiName, argc, stConfig.pid);
	if(AfterCgiInit(stConfig) <= 0)
		return SLOG_ERROR_LINE;

	while(FCGI_Accept() >= 0)
	{
		stConfig.argc = argc;
		stConfig.argv = argv;
		stConfig.envp = envp;

		iRet=BeforeCgiRequestInit(stConfig);
		if(iRet == 0)
			continue;
		else if(iRet < 0)
			break;

		if(argc <= 1) {
			cgiwrap_init_std(argc, argv, environ);
			cgiwrap_init_emu(NULL, cs_read, cs_printf, cs_write, NULL, NULL, NULL);
		}

		if(InitFastCgi(stConfig, stConfig.szDebugPath) < 0)
			break;

		iRet=AfterCgiRequestInit(stConfig);
		if(iRet == 0)
			continue;
		else if(iRet < 0)
			break;
		SetCgiResponseType(stConfig, s_JsonRequest);

		const char *pAction = stConfig.pAction;
		if(g_iNeedDb && DealDbConnect(stConfig) < 0) {
			show_errpage(NULL, CGI_ERR_SERVER, stConfig);
			continue;
		}

		if(CheckLogin(
			stConfig.cgi, stConfig.pshmLoginList, stConfig.remote, stConfig.dwCurTime, &stConfig.stUser) <= 0)
		{
			INFO_LOG("remote:%s access:%s with no logined cookie !", stConfig.remote, stConfig.pszCgiName);
			if(CheckLoginEx(stConfig, stConfig.cgi, 
				stConfig.pshmLoginList, stConfig.remote, stConfig.dwCurTime, &stConfig.stUser) <= 0)
			{
				RedirectToFastLogin(stConfig);
				cgi_destroy(&stConfig.cgi);
				continue;
			}
			else {
				INFO_LOG("check login by login_ex ok -- remote:%s user:%s", 
					stConfig.remote, stConfig.stUser.puser);

				// 允许跨域访问
				hdf_set_value(stConfig.cgi->hdf, "cgiout.other.cros", "Access-Control-Allow-Origin:*");
			}
		}

		iRet=AfterCgiLogin(stConfig);
		if(iRet == 0)
			continue;
		else if(iRet < 0)
			break;

		if(NULL == pAction)
		{
			REQERR_LOG("have no action from :%s", stConfig.remote);
			show_errpage(NULL, CGI_REQERR, stConfig);
			continue;
		}
		INFO_LOG("get action :%s from :%s", pAction, stConfig.remote);
		std::string strCsTemplateFile;

		// log ---
		if(!strcmp(pAction, "show_realtime_log")
			|| !strcmp(pAction, "show_history_log"))
			iRet = DealInitShowLog(stConfig.cgi);
		else if(!strcmp(pAction, "get_realtime"))
			iRet = DealGetRealTimeLog(stConfig.cgi);
		else if(!strcmp(pAction, "get_history"))
			iRet = DealGetHistoryLog(stConfig.cgi);
		else if(!strcmp(pAction, "log_files"))
			iRet = DealGetLogFilesStart(stConfig.cgi);
		else if(!strcmp(pAction, "show_log_files"))
			iRet = DealGetLogFiles(stConfig.cgi);
		else if(!strcmp(pAction, "get_app_file_list"))
			iRet = DealGetLogFilesForSearch(stConfig.cgi);
		else if(!strcmp(pAction, "init_log_report_test"))
			iRet = DealInitLogReportTest(stConfig.cgi);
		else if(!strcmp(pAction, "refresh_main_info"))
		    iRet = DealRefreshMainInfo(stConfig.cgi);
		
		// app ---
		else if(!strcmp(pAction, "add_app"))
			iRet = DealAddApp(stConfig.cgi);
		else if(!strcmp(pAction, "mod_app"))
			iRet = DealModApp(stConfig.cgi);
		else if(!strcmp(pAction, "save_add_app")) 
			iRet = DealSaveApp(stConfig.cgi, true);
		else if(!strcmp(pAction, "save_mod_app"))
			iRet = DealSaveApp(stConfig.cgi, false);
		else if(!strcmp(pAction, "list_app"))
			iRet = DealListApp(stConfig.cgi);
		else if(!strcmp(pAction, "delete_app"))
			iRet = DealDelApp(stConfig.cgi);

		// module ---
		else if(!strcmp(pAction, "list_module"))
			iRet = DealListModule(stConfig.cgi);
		else if(!strcmp(pAction, "add_module"))
			iRet = DealAddModule(stConfig.cgi);
		else if(!strcmp(pAction, "mod_module"))
			iRet = DealModModule(stConfig.cgi);
		else if(!strcmp(pAction, "save_add_module")) 
			iRet = DealSaveModule(stConfig.cgi, true);
		else if(!strcmp(pAction, "save_mod_module"))
			iRet = DealSaveModule(stConfig.cgi, false);
		else if(!strcmp(pAction, "delete_module"))
			iRet = DealDelModule(stConfig.cgi);

		// log config
		else if(!strcmp(pAction, "list_log_config"))
			iRet = DealListConfig(stConfig.cgi);
		else if(!strcmp(pAction, "add_config"))
			iRet = DealAddConfig(stConfig.cgi);
		else if(!strcmp(pAction, "detail_log_config"))
			iRet = DealDetailLogConfig(stConfig.cgi);
		else if(!strcmp(pAction, "mod_config"))
			iRet = DealModConfig(stConfig.cgi);
		else if(!strcmp(pAction, "save_add_config")) 
			iRet = DealSaveConfig(stConfig.cgi, true);
		else if(!strcmp(pAction, "save_mod_config"))
			iRet = DealSaveConfig(stConfig.cgi, false);
		else if(!strcmp(pAction, "delete_config"))
			iRet = DealDelConfig(stConfig.cgi);

		// monitor plugin
		else if(!strcmp(pAction, "open_plugin"))
			iRet = DealListPlugin(stConfig.cgi);
		else if(!strcmp(pAction, "install_open_plugin"))
			iRet = DealInstallPlugin(stConfig.cgi);
		else if(!strcmp(pAction, "update_open_plugin"))
			iRet = DealUpdatePlugin(stConfig.cgi);
		else if(!strcmp(pAction, "down_plugin_conf"))
			iRet = DealDownloadPluginConf(stConfig.cgi);
		else if(!strcmp(pAction, "dp_add_plugin")) 
            iRet = DealDpAddPlugin();
		else if(!strcmp(pAction, "ddap_install_plugin")) 
            iRet = DealPreInstallPlugin(strCsTemplateFile);
		else if(!strcmp(pAction, "refresh_preinstall_plugin_status")) 
            iRet = DealRefreshPreInstallStatus();
		else if(!strcmp(pAction, "dmap_multi_opr_plugin")) 
            iRet = DealOprMachPlugins(strCsTemplateFile);
		else if(!strcmp(pAction, "ddap_multi_opr_plugin")) 
            iRet = DealOprMachinePlugin(strCsTemplateFile);
		else if(!strcmp(pAction, "refresh_mach_opr_plugin_status")) 
            iRet = DealRefreshOprMachinePlugin(strCsTemplateFile);
		else if(!strcmp(pAction, "ddap_save_mod_plugin_cfg")) 
            iRet = DealSaveOprMachinePlugin();
	
		else {
			ERR_LOG("unknow action:%s", pAction);
			iRet = -1;
		}
		if(iRet < 0)
		{
			show_errpage(NULL, CGI_REQERR, stConfig);
			continue;
		}

		const char *pcsTemplate = NULL;
		if(!strcmp(pAction, "list_app"))
			pcsTemplate = "dmt_app.html";
		else if(!strcmp(pAction, "add_app") || !strcmp(pAction, "mod_app"))
			pcsTemplate = "dmt_dlg_add_app.html";
		else if(!strcmp(pAction, "list_module"))
			pcsTemplate = "dmt_module.html";
		else if(!strcmp(pAction, "add_module") || !strcmp(pAction, "mod_module"))
			pcsTemplate = "dmt_dlg_add_module.html";
		else if(!strcmp(pAction, "list_log_config"))
			pcsTemplate = "dmt_log_config.html";
		else if(!strcmp(pAction, "add_config") || !strcmp(pAction, "mod_config"))
			pcsTemplate = "dmt_dlg_add_config.html";
		else if(!strcmp(pAction, "detail_log_config"))
			pcsTemplate = "dmt_dlg_detail_config.html";
		else if(!strcmp(pAction, "list_tv_config"))
			pcsTemplate = "dmt_tv_config.html";
		else if(!strcmp(pAction, "add_tv_config") || !strcmp(pAction, "mod_tv_config"))
			pcsTemplate = "dmt_dlg_add_tv_config.html";
		else if(!strcmp(pAction, "show_realtime_log"))
			pcsTemplate = "dmt_realtime_log.html";
		else if(!strcmp(pAction, "show_history_log"))
			pcsTemplate = "dmt_history_log.html";
		else if(!strcmp(pAction, "log_files"))
			pcsTemplate = "dmt_log_files.html";
		else if(!strcmp(pAction, "init_log_report_test"))
			pcsTemplate = "dmt_dlg_log_report_test.html";
		else if(!strcmp(pAction, "open_plugin"))
			pcsTemplate = "dmt_plugin.html";
		else if(!strcmp(pAction, "dp_add_plugin")) 
			pcsTemplate = "dmt_dp_add_plugin.html";

		if(!pcsTemplate && !strCsTemplateFile.empty())
			pcsTemplate = strCsTemplateFile.c_str();
		if(pcsTemplate != NULL)
		{
			std::string strCsFile(stConfig.szCsPath);
			strCsFile += pcsTemplate;
			stConfig.err = cgi_display(stConfig.cgi, strCsFile.c_str()); 
			if(stConfig.err != STATUS_OK)
			{
				show_errpage(NULL, NULL, stConfig);
				continue;
			}
		}

		iRet=AfterCgiResponse(stConfig);
		if(iRet == 0)
			continue;
		else if(iRet < 0)
			break;
	}

	if(stConfig.cgi != NULL)
		DealCgiFailedExit(stConfig, stConfig.err);

	stConfig.dwEnd = time(NULL);
	INFO_LOG("fcgi - %s stop at:%u run:%u pid:%u errmsg:%s",
		stConfig.pszCgiName, stConfig.dwEnd, stConfig.dwEnd - stConfig.dwStart, stConfig.pid, strerror(errno));
	return 0;
}

