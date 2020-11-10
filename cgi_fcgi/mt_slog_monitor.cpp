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

   fastcgi mt_slog_monitor: web 控制台网站框架生成, 使用了开源的 dwz 后台管理框架

****/

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS 1
#endif

#include <string>
#include <fcgi_stdio.h>
#include <fcgi_config.h>
#include <stdlib.h>
#include <stdarg.h>
#include <inttypes.h>
#include <cgi_head.h>
#include <cgi_comm.h>
#include <supper_log.h>
#include <sv_file.h>

CSupperLog slog;
CGIConfig stConfig;

// ajax json 响应方式
static const char *s_JsonRequest [] = { 
	NULL
};

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

static int SetMachineList()
{
	int iStartIdx = stConfig.stUser.pSysInfo->iMachineListIndexStart;
	MachineInfo *pInfo = NULL;
	char hdf_pex[32], hdf_name[64];
	SharedHashTableNoList *pMachHash = slog.GetMachineHash();
	for(int i=0; i < stConfig.stUser.pSysInfo->wMachineCount; i++)
	{
		pInfo = (MachineInfo*)NOLIST_HASH_INDEX_TO_NODE(pMachHash, iStartIdx);
		iStartIdx = pInfo->iNextIndex;
		const char *pvname = NULL;
		if(pInfo->iNameVmemIdx > 0)
		    pvname = MtReport_GetFromVmem_Local(pInfo->iNameVmemIdx);
		else
			pvname = "unknow";
		sprintf(hdf_pex, "Output.machlists.%d", i);
		sprintf(hdf_name, "%s.id", hdf_pex);
		hdf_set_int_value(stConfig.cgi->hdf, hdf_name, pInfo->id);
		sprintf(hdf_name, "%s.name", hdf_pex);
		hdf_set_value(stConfig.cgi->hdf, hdf_name, pvname);
	}
	DEBUG_LOG("get machine - result count:%d", stConfig.stUser.pSysInfo->wMachineCount);
	return 0;
}

static int DealLeftInnerMt()
{
	char sSqlBuf[256] = {0};
	Query & qu = *stConfig.qu;
	sprintf(sSqlBuf, 
		"select site_id,site_name from mt_user_site_net_delay where xrk_status=%d", RECORD_STATUS_USE);
	strcat(sSqlBuf, " order by create_time asc");
	qu.get_result(sSqlBuf);

	int i=0;
	char hdf_pex[32], hdf_name[64];
	for(i=0; i < qu.num_rows() && qu.fetch_row() != NULL; i++)
	{
		sprintf(hdf_pex, "Output.amlists.%d", i);
		sprintf(hdf_name, "%s.id", hdf_pex);
		hdf_set_int_value(stConfig.cgi->hdf, hdf_name, qu.getval("site_id"));
		sprintf(hdf_name, "%s.name", hdf_pex);
		hdf_set_value(stConfig.cgi->hdf, hdf_name, qu.getstr("site_name"));
	}
	hdf_set_int_value(stConfig.cgi->hdf, "Output.inner_site_count", i);
	DEBUG_LOG("get monitor site list sql:%s - result count:%d(%d)", sSqlBuf, qu.num_rows(), i);
	qu.free_result();
	return 0;
}

static int SetViewList()
{
	char sSqlBuf[512] = {0};
	Query & qu = *stConfig.qu;
	sprintf(sSqlBuf, "select * from mt_view where xrk_status=%d", RECORD_STATUS_USE);
	strcat(sSqlBuf, " order by create_time asc");
	qu.get_result(sSqlBuf);

	int i=0;
	char hdf_pex[32], hdf_name[64];
	for(i=0; i < qu.num_rows() && qu.fetch_row() != NULL; i++)
	{
		sprintf(hdf_pex, "Output.amlists.%d", i);
		sprintf(hdf_name, "%s.id", hdf_pex);
		hdf_set_int_value(stConfig.cgi->hdf, hdf_name, qu.getval("xrk_id"));
		sprintf(hdf_name, "%s.flag", hdf_pex);
		hdf_set_int_value(stConfig.cgi->hdf, hdf_name, qu.getval("view_flag"));
		sprintf(hdf_name, "%s.name", hdf_pex);
		hdf_set_value(stConfig.cgi->hdf, hdf_name, qu.getstr("xrk_name"));
	}
	hdf_set_int_value(stConfig.cgi->hdf, "Output.view_count", i);
	DEBUG_LOG("get view list sql:%s - result count:%d(%d)", sSqlBuf, qu.num_rows(), i);
	qu.free_result();
	return 0;
}

void SetWebInfo()
{
	// web site info
	Json jsSite;
	jsSite["cgi_path"] = stConfig.szCgiPath;
	jsSite["cs_path"] = stConfig.szCsPath;
	jsSite["doc_path"] = stConfig.szDocPath;
	jsSite["user_name"] = stConfig.stUser.puser;
	jsSite["user_id"] = stConfig.stUser.puser_info->iUserId;
	hdf_set_value(stConfig.cgi->hdf, "config.site_info", jsSite.ToString().c_str());
	hdf_set_int_value(stConfig.cgi->hdf, "config.user_flag_1", stConfig.stUser.puser_info->dwUserFlag_1);
	AppInfo *pApp = slog.GetAppInfoByIndex(stConfig.stUser.pSysInfo->iAppInfoIndexStart);
	if(pApp != NULL && !slog.IsIpMatchLocalMachine(pApp->dwAppSrvMaster))
		hdf_set_value(stConfig.cgi->hdf, "config.app_info_addr", 
			ipv4_addr_str(pApp->dwAppSrvMaster));
	else 
		hdf_set_value(stConfig.cgi->hdf, "config.app_info_addr", "local");

	FloginInfo *psess= stConfig.stUser.puser_info;
	uint32_t dwExpireTime = psess->dwLastAccessTime+psess->iLoginExpireTime;
	if(dwExpireTime > psess->dwLoginTime+LOGIN_MAX_EXPIRE_TIME)
		dwExpireTime = psess->dwLoginTime+LOGIN_MAX_EXPIRE_TIME;
	hdf_set_value(stConfig.cgi->hdf, "config.login_expire_time", uitodate(dwExpireTime));

	int iNotifyDaemon= 0;
	if(IsDaemonDenyOp(stConfig, false))
		iNotifyDaemon= 1;

	hdf_set_int_value(stConfig.cgi->hdf, "config.notify_daemon", iNotifyDaemon);

	if(stConfig.stUser.puser_info->bLoginType == 1)
		hdf_set_value(stConfig.cgi->hdf, "config.type_name", "管理员");
	else
		hdf_set_value(stConfig.cgi->hdf, "config.type_name", "普通账号");
}

static int InitFastCgi_first(CGIConfig &myConf)
{
	if(InitFastCgiStart(myConf) < 0) {
		ERR_LOG("InitFastCgiStart failed !");
		return SLOG_ERROR_LINE;
	}

	int32_t iRet = 0;
	if((iRet=slog.InitConfigByFile(myConf.szConfigFile)) < 0 || (iRet=slog.Init()) < 0)
		return SLOG_ERROR_LINE;

	myConf.pAppInfo = slog.GetAppInfo();
	if(myConf.pAppInfo == NULL)
	{
		FATAL_LOG("get pAppInfo:%p failed !", myConf.pAppInfo);
		return SLOG_ERROR_LINE;
	}
	return 0;
}

int DealPluginManager()
{
	char sSqlBuf[128] = {0};
	Query qu(*stConfig.db);

	Json js;
	std::ostringstream ss;
	sprintf(sSqlBuf, "select pb_info from mt_plugin where xrk_status=%d", RECORD_STATUS_USE);

	char hdf_pex[32];
    int iCount = 0;
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

			ss.str("");
			ss << stConfig.szCsPath << "plugin_show/" << (const char*)(plugin["plus_name"])<< "_show/index_tp.html";
			if(IsFileExist(ss.str().c_str())) {
				sprintf(hdf_pex, "ShowPlugin.lists.%d", iCount);
				hdf_set_valuef(stConfig.cgi->hdf, "%s.plugin_id=%u", hdf_pex, (int)(plugin["plugin_id"]));
				hdf_set_valuef(stConfig.cgi->hdf, "%s.name=%s", hdf_pex, (const char*)(plugin["plus_name"]));
				iCount++;
			}
		}
	}
    hdf_set_int_value(stConfig.cgi->hdf, "ShowPlugin.count", iCount);
	qu.free_result();
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

	if(AfterCgiInit(stConfig) <= 0)
		return SLOG_ERROR_LINE;

	INFO_LOG("fcgi:%s argc:%d start pid:%u", stConfig.pszCgiName, argc, stConfig.pid);
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
		DEBUG_LOG("get action :%s from :%s", pAction, stConfig.remote);

		if(DealDbConnect(stConfig) < 0) {
			show_errpage(NULL, CGI_ERR_SERVER, stConfig);
			continue;
		}

		// check
		if((iRet=CheckLogin(
			stConfig.cgi, stConfig.pshmLoginList, stConfig.remote, stConfig.dwCurTime, &stConfig.stUser)) <= 0)
		{
			INFO_LOG("remote:%s access:%s with no logined cookie !", stConfig.remote, stConfig.pszCgiName);

			// 日志系统可能需要跨站
			hdf_set_value(stConfig.cgi->hdf, "cgiout.other.cros", "Access-Control-Allow-Origin:*");
			RedirectToFastLogin(stConfig);
			cgi_destroy(&stConfig.cgi);
			continue;
		}

		// 
		iRet=AfterCgiLogin(stConfig);
		if(iRet == 0)
			continue;
		else if(iRet < 0)
			break;

		const char *pcsTemplate = NULL;
		if(pAction == NULL || !strcmp(pAction, "left_attr"))
		{
			if((iRet=SetViewList()) < 0)
			{
				ERR_LOG("SetViewList failed ret:%d", iRet);
				iRet = -5;
			}

			if(iRet >= 0 && (iRet=SetMachineList()) < 0)
			{
				ERR_LOG("SetMachineList failed, ret:%d", iRet);
				iRet = -6;
			}

			if(iRet >= 0 && pAction == NULL)
			{
				SetWebInfo();
				pcsTemplate = "dwz_index.html";
			}
			else if(iRet >= 0 && !strcmp(pAction, "left_attr"))
				pcsTemplate = "dwz_left_attr.html";
		}
		else if(!strcmp(pAction, "left_m"))
			pcsTemplate = "dwz_left_m.html";
		else if(!strcmp(pAction, "left_log"))
			pcsTemplate = "dwz_left_log.html";
		else if(!strcmp(pAction, "left_plus")) {
			iRet = DealPluginManager();
			pcsTemplate = "dwz_left_plus.html";
		}
		else if(!strcmp(pAction, "left_user"))
			pcsTemplate = "dwz_left_user.html";
		else if(!strcmp(pAction, "left_syssrv"))
			pcsTemplate = "dwz_left_syssrv.html";
		else if(!strcmp(pAction, "left_main"))
			pcsTemplate = "dwz_left_main.html";
		else if(!strcmp(pAction, "left_inner_mt"))
		{
			iRet = DealLeftInnerMt();
			pcsTemplate = "dwz_left_innter_mt.html";
		}
		else
		{
			iRet = -1;
			REQERR_LOG("unknow action:%s from:%s", pAction, stConfig.remote);
			hdf_set_value(stConfig.cgi->hdf,"err.msg", CGI_REQERR);
		}

		if(iRet < 0)
		{
			show_errpage(NULL, NULL, stConfig);
			continue;
		}

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

		// 
		iRet=AfterCgiResponse(stConfig);
		if(iRet == 0)
			continue;
		else if(iRet < 0)
			break;
	}

	INFO_LOG(" fcgi - %s exist", stConfig.pszCgiName);
	return 0;
}

