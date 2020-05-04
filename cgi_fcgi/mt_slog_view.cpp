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

   cgi mt_slog_view: 监控系统视图相关管理 

****/

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS 1
#endif

#include <string>
#include <errno.h>
#include <fcgi_stdio.h>
#include <fcgi_config.h>
#include <stdlib.h>
#include <stdarg.h>
#include <inttypes.h>
#include <cgi_head.h>
#include <cgi_comm.h>

CSupperLog slog;
CGIConfig stConfig;

// ajax json 响应方式
static const char *s_JsonRequest [] = { 
	"delete",
	"save_add_view",
	"save_mod_view",
	"view_unauto_bind_machine",
	NULL
};

typedef struct
{
	int id;
	int flag;
	const char *pkey;
}SearchInfo;

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

static int AddSearchInfo(char *psql, int ibufLen, SearchInfo *pinfo)
{
	char sTmpBuf[128] = {0};
	if(pinfo->id != 0)
	{
		sprintf(sTmpBuf, " and id=%d", pinfo->id);
		strcat(psql, sTmpBuf);
		hdf_set_int_value(stConfig.cgi->hdf, "config.dv_view_id", pinfo->id);
	}

	if(pinfo->pkey != NULL && pinfo->pkey[0] != '\0')
	{
		memset(sTmpBuf, 0, sizeof(sTmpBuf));
		snprintf(sTmpBuf, sizeof(sTmpBuf)-1, " and (name like \'%%%s%%\' or view_desc like "
			" \'%%%s%%\')", pinfo->pkey, pinfo->pkey);
		if((int)(strlen(sTmpBuf) + strlen(psql)) >= ibufLen)
		{
			REQERR_LOG("search key(%s) too long, tmp:%s sql:%s", pinfo->pkey, sTmpBuf, psql);
			hdf_set_value(stConfig.cgi->hdf, "err.msg", CGI_REQERR);
			return SLOG_ERROR_LINE;
		}
		strcat(psql, sTmpBuf);
		hdf_set_value(stConfig.cgi->hdf, "config.dv_keyword", pinfo->pkey);
	}
	DEBUG_LOG("after add search info sql:%s", psql);
	return 0;
}

static int GetViewTotalRecords(SearchInfo *pinfo=NULL)
{
	char sSqlBuf[512] = {0};
	Query & qu = *stConfig.qu;
	
	sprintf(sSqlBuf, "select count(*) from mt_view where xrk_status=%d", RECORD_STATUS_USE);
	if(pinfo != NULL && AddSearchInfo(sSqlBuf, sizeof(sSqlBuf), pinfo) < 0)
		return SLOG_ERROR_LINE;

	DEBUG_LOG("get view count - exesql:%s", sSqlBuf);
	if(qu.get_result(sSqlBuf) == NULL || qu.num_rows() <= 0)
	{
		qu.free_result();
		return 0;
	}

	qu.fetch_row();
	int iCount = qu.getval(0);
	qu.free_result();
	DEBUG_LOG("view records count:%d", iCount);
	return iCount;
}

static int DealViewLookUp()
{
	char sSqlBuf[512] = {0};
	Query & qu = *stConfig.qu;

	sprintf(sSqlBuf, "select xrk_id,xrk_name,view_desc from mt_view where xrk_status=%d", RECORD_STATUS_USE);
	qu.get_result(sSqlBuf);
	Json js;
	int i=0;
	for(i=0; i < qu.num_rows() && qu.fetch_row() != NULL; i++)
	{
		Json attr;
		attr["id"] = qu.getuval("xrk_id");
		attr["name"] = qu.getstr("xrk_name");
		attr["view_desc"] = qu.getstr("view_desc");
		js["list"].Add(attr);
	}
	js["count"] = i; 
	DEBUG_LOG("get view list - result count:%d(%d)", qu.num_rows(), i);

	std::string str(js.ToString());
	DEBUG_LOG("lookup view, json:%s", str.c_str());

	stConfig.err = hdf_set_value(stConfig.cgi->hdf, "config.view_info", str.c_str()); 
	if(stConfig.err != STATUS_OK)
	{
		ERR_LOG("set view info failed, length:%lu", str.size());
		return SLOG_ERROR_LINE;
	}
	return 0;
}

static int GetViewList(Json &js, SearchInfo *pinfo=NULL)
{
	char sSqlBuf[512] = {0};
	Query qu(*stConfig.db);
	int iCurPage = hdf_get_int_value(stConfig.cgi->hdf, "config.currentPage", 0);
	int iNumPerPage = hdf_get_int_value(stConfig.cgi->hdf, "config.numPerPage", 0);
	if(iCurPage == 0 || iNumPerPage == 0)
	{
		ERR_LOG("invalid iCurPage(%d) or iNumPerPage(%d)", iCurPage, iNumPerPage);
		return SLOG_ERROR_LINE;
	}

	sprintf(sSqlBuf, "select * from mt_view where xrk_status=%d", RECORD_STATUS_USE);
	if(pinfo != NULL && AddSearchInfo(sSqlBuf, sizeof(sSqlBuf), pinfo) < 0)
		return SLOG_ERROR_LINE;

	int iOrder = 0;
	iOrder = (iOrder==0 ? SetRecordsOrder(stConfig.cgi, sSqlBuf, "xrk_id") : 1);
	iOrder = (iOrder==0 ? SetRecordsOrder(stConfig.cgi, sSqlBuf, "create_time") : 1);
	iOrder = (iOrder==0 ? SetRecordsOrder(stConfig.cgi, sSqlBuf, "xrk_name") : 1);
	iOrder = (iOrder==0 ? SetRecordsOrder(stConfig.cgi, sSqlBuf, "user_add") : 1);
	iOrder = (iOrder==0 ? SetRecordsOrder(stConfig.cgi, sSqlBuf, "view_flag") : 1);
	if(iOrder == 0) 
		strcat(sSqlBuf, " order by xrk_id desc");

	char sTmpBuf[64]={0};
	sprintf(sTmpBuf, " limit %d,%d", iNumPerPage*(iCurPage-1), iNumPerPage);
	strcat(sSqlBuf, sTmpBuf);
	DEBUG_LOG("get view list - exesql:%s", sSqlBuf);

	qu.get_result(sSqlBuf);
	int i=0;
	for(i=0; i < qu.num_rows() && qu.fetch_row() != NULL; i++)
	{
		Json attr;
		attr["id"] = qu.getuval("xrk_id");
		attr["name"] = qu.getstr("xrk_name");
		attr["view_desc"] = qu.getstr("view_desc");
		attr["user_add"] = qu.getstr("user_add");
		attr["add_time"] = qu.getstr("create_time");
		attr["user_mod"] = qu.getstr("user_mod");
		attr["mod_time"] = qu.getstr("mod_time");
		attr["view_flag"] = qu.getval("view_flag");
		js["list"].Add(attr);
	}
	js["count"] = i; 
	DEBUG_LOG("get view list - result count:%d(%d)", qu.num_rows(), i);
	qu.free_result();
	return 0;
}

static int DeleteView()
{
	if(IsDaemonDenyOp(stConfig))
		return 0;

	stConfig.iResponseType = RESPONSE_TYPE_JSON;
	int id = hdf_get_int_value(stConfig.cgi->hdf, "Query.id", 0);
	if(id == 0)
	{
		WARN_LOG("invalid parameter(id:%d) from:%s", id, stConfig.remote);
		hdf_set_value(stConfig.cgi->hdf,"err.msg", CGI_REQERR);
		return SLOG_ERROR_LINE;
	}

	static char sSqlBuf[128];

	sprintf(sSqlBuf, "update mt_view set xrk_status=%d,user_mod_id=%d where xrk_id=%d",
		stConfig.iDeleteStatus, stConfig.stUser.puser_info->iUserId, id);
	Query & qu = *stConfig.qu;
	if(!qu.execute(sSqlBuf))
	{
		ERR_LOG("execute sql:%s failed, msg:%s", sSqlBuf, qu.GetError().c_str());
		return SLOG_ERROR_LINE;
	}

	Json js;
	js["statusCode"] = 200;
	js["msgid"] = "delSuccess";

	STRING str;
	string_init(&str);
	if((stConfig.err=string_set(&str, js.ToString().c_str())) != STATUS_OK
		|| (stConfig.err=cgi_output(stConfig.cgi, &str)) != STATUS_OK)
	{
		string_clear(&str);
		return SLOG_ERROR_LINE;
	}
	string_clear(&str);

	DEBUG_LOG("delete view id:%d success, sql:%s, response string :%s to remote:%s",
		id, sSqlBuf, js.ToString().c_str(), stConfig.remote);
	return 0;
}

static int DealUnAutoBindMachine()
{
	if(IsDaemonDenyOp(stConfig))
		return 0;
	 
	int32_t id = hdf_get_int_value(stConfig.cgi->hdf, "Query.view_id", 0);
	int idx = slog.GetViewInfoIndex(id);
	TViewInfo *pView = slog.GetViewInfo(idx);
	if(id == 0 || pView == NULL)
	{
		REQERR_LOG("invalid view id or not find view:%d", id);
		return SLOG_ERROR_LINE;
	}

	if(pView->bViewFlag & VIEW_FLAG_AUTO_BIND_MACHINE)
	{
		CLEAR_BIT(pView->bViewFlag, VIEW_FLAG_AUTO_BIND_MACHINE);
		char sql[128] = {0};
		snprintf(sql, sizeof(sql)-1, "update mt_view set view_flag=%d where xrk_id=%d", pView->bViewFlag, id);
		Query & qu = *stConfig.qu;
		if(!qu.execute(sql))
			return SLOG_ERROR_LINE;
		INFO_LOG("set view:%d unauto bind machine", id);
	}
	
	Json js;
	js["statusCode"] = 200;

	STRING str;
	string_init(&str);
	if((stConfig.err=string_set(&str, js.ToString().c_str())) != STATUS_OK
		|| (stConfig.err=cgi_output(stConfig.cgi, &str)) != STATUS_OK)
	{
		string_clear(&str);
		return SLOG_ERROR_LINE;
	}
	string_clear(&str);
	return 0;
}

static int SaveView(bool bIsMod=false)
{
	if(IsDaemonDenyOp(stConfig))
		return 0;

	int32_t id = hdf_get_int_value(stConfig.cgi->hdf, "Query.dvm_view_id", 0);
	const char *pname = hdf_get_value(stConfig.cgi->hdf, "Query.dvm_view_name", NULL);
	const char *pdesc = hdf_get_value(stConfig.cgi->hdf, "Query.dvm_view_desc", NULL);
	const char *pnavTabId = hdf_get_value(stConfig.cgi->hdf, "Query.navTabId", "mt_view");
	const char *pauto_bindm = hdf_get_value(stConfig.cgi->hdf, "Query.dvm_auto_bind_machine", NULL);

	int32_t flag = 0;
	if((bIsMod && id == 0) || pname == NULL || pdesc == NULL)
	{
		hdf_set_value(stConfig.cgi->hdf,"err.msg", CGI_REQERR);
		REQERR_LOG("bMod:%d invalid param id:%d pname:%s pdesc:%s", bIsMod, id, pname, pdesc); 
		return SLOG_ERROR_LINE;
	}

	if(pauto_bindm != NULL && !strcmp(pauto_bindm, "on"))
		SET_BIT(flag, VIEW_FLAG_AUTO_BIND_MACHINE);

	IM_SQL_PARA* ppara = NULL;
	if(InitParameter(&ppara) < 0) {
		ERR_LOG("sql parameter init failed !");
		return SLOG_ERROR_LINE;
	}

	AddParameter(&ppara, "xrk_name", pname, NULL);
	AddParameter(&ppara, "view_flag", flag, "DB_CAL");

	if(!bIsMod)
	{
		AddParameter(&ppara, "user_add", stConfig.stUser.puser, NULL);
		AddParameter(&ppara, "create_time", uitodate(stConfig.dwCurTime), NULL);
		AddParameter(&ppara, "mod_time", uitodate(stConfig.dwCurTime), NULL);
		AddParameter(&ppara, "user_add_id", stConfig.stUser.puser_info->iUserId, "DB_CAL");
	}
	else
	{
		AddParameter(&ppara, "user_mod", stConfig.stUser.puser, NULL);
	}
	AddParameter(&ppara, "view_desc", pdesc, NULL);
	AddParameter(&ppara, "user_mod_id", stConfig.stUser.puser_info->iUserId, "DB_CAL");

	std::string strSql;
	Query & qu = *stConfig.qu;

	if(!bIsMod)
		strSql = "insert into mt_view";
	else
		strSql = "update mt_view set";

	if(!bIsMod)
		JoinParameter_Insert(&strSql, qu.GetMysql(), ppara);
	else {
		JoinParameter_Set(&strSql, qu.GetMysql(), ppara);
		strSql += " where xrk_id=";
		strSql += itoa(id);
	}

	ReleaseParameter(&ppara);
	if(!qu.execute(strSql))
	{
		ERR_LOG("execute sql:%s failed, msg:%s", strSql.c_str(), qu.GetError().c_str());
		return SLOG_ERROR_LINE;
	}

	if(!bIsMod) {
		id = qu.insert_id();
		if(id < 10000000)
			ERR_LOG("view auto id invalid:%d, over 10000000", id);
	}

	Json js;
	js["statusCode"] = 200;
	js["navTabId"] = pnavTabId;
	js["callbackType"] = "closeCurrent";
	if(!bIsMod)
		js["msgid"] = "addSuccess";
	else
		js["msgid"] = "modSuccess";

	STRING str;
	string_init(&str);
	if((stConfig.err=string_set(&str, js.ToString().c_str())) != STATUS_OK
		|| (stConfig.err=cgi_output(stConfig.cgi, &str)) != STATUS_OK)
	{
		string_clear(&str);
		return SLOG_ERROR_LINE;
	}
	string_clear(&str);
	DEBUG_LOG("%s mt_view name:%s(id:%d) success, sql:%s, response string :%s to remote:%s ",
		(bIsMod ? "update" : "insert"), pname, id, strSql.c_str(), js.ToString().c_str(), stConfig.remote);
	return 0;
}

static int DealModView()
{
	int id = hdf_get_int_value(stConfig.cgi->hdf, "Query.id", 0);
	if(id==0)
	{
		WARN_LOG("invalid parameter from:%s", stConfig.remote);
		hdf_set_value(stConfig.cgi->hdf, "err.msg", CGI_REQERR);
		return SLOG_ERROR_LINE;
	}

	char sSqlBuf[128] = {0};
	sprintf(sSqlBuf, "select * from mt_view where xrk_status=%d and xrk_id=%d", RECORD_STATUS_USE, id);
	Query & qu = *stConfig.qu;
	qu.get_result(sSqlBuf);
	
	if(qu.num_rows() <= 0){
		WARN_LOG("have no view id:%d", id);
		hdf_set_value(stConfig.cgi->hdf, "err.msg", CGI_REQERR);
		qu.free_result();
		return SLOG_ERROR_LINE;
	}

	if(qu.fetch_row() != NULL)
	{
		hdf_set_value(stConfig.cgi->hdf, "view.dvm_view_id", qu.getstr("xrk_id"));
		hdf_set_int_value(stConfig.cgi->hdf, "view.dvm_view_flag", qu.getval("view_flag"));
		hdf_set_value(stConfig.cgi->hdf, "view.dvm_view_name", qu.getstr("xrk_name"));
		hdf_set_value(stConfig.cgi->hdf, "view.dvm_view_desc", qu.getstr("view_desc"));
	}
	hdf_set_value(stConfig.cgi->hdf, "config.action", "save_mod_view");
	DEBUG_LOG("try update view:%d-%s from:%s", id, qu.getstr("xrk_name"), stConfig.remote);
	qu.free_result();
	return 0;
}

static int DealAddView()
{
	hdf_set_value(stConfig.cgi->hdf, "config.action", "save_add_view");
	return 0;
}

static int DealViewSearch()
{
	SearchInfo stInfo;
	memset(&stInfo, 0, sizeof(stInfo));
	stInfo.pkey = hdf_get_value(stConfig.cgi->hdf, "Query.dv_keyword", NULL);
	stInfo.id = hdf_get_int_value(stConfig.cgi->hdf, "Query.dv_view_id", 0);

	DEBUG_LOG("search info key:%s id:%d", stInfo.pkey, stInfo.id);

	int iRecords = GetViewTotalRecords(&stInfo);
	if(iRecords < 0)
	{
		ERR_LOG("get view records count failed !");
		return SLOG_ERROR_LINE;
	}
	SetRecordsPageInfo(stConfig.cgi, iRecords);

	Json js;
	if(GetViewList(js, &stInfo) < 0)
		return SLOG_ERROR_LINE;
	std::string str_attr(js.ToString());
	DEBUG_LOG("view list json:%s", str_attr.c_str());
	stConfig.err = hdf_set_value(stConfig.cgi->hdf, "config.view_info", str_attr.c_str()); 
	if(stConfig.err != STATUS_OK)
	{
		ERR_LOG("set attr type info failed, length:%lu", str_attr.size());
		return SLOG_ERROR_LINE;
	}
	return 0;
}

static int DealListView()
{
	int iRecords = GetViewTotalRecords();
	if(iRecords < 0)
	{
		ERR_LOG("get attr record count failed !");
		return SLOG_ERROR_LINE;
	}
	SetRecordsPageInfo(stConfig.cgi, iRecords);

	Json js;
	if(GetViewList(js) < 0)
		return SLOG_ERROR_LINE;
	std::string str(js.ToString());
	DEBUG_LOG("view list json:%s", str.c_str());

	stConfig.err = hdf_set_value(stConfig.cgi->hdf, "config.view_info", str.c_str()); 
	if(stConfig.err != STATUS_OK)
	{
		ERR_LOG("set view info failed, length:%lu", str.size());
		return SLOG_ERROR_LINE;
	}
	return 0;
}

static int InitFastCgi_first(CGIConfig &myConf)
{
	if(InitFastCgiStart(myConf) < 0) {
		ERR_LOG("InitFastCgiStart failed !");
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

	int32_t iRet = 0;
	if((iRet=slog.InitConfigByFile(myConf.szConfigFile)) < 0 || (iRet=slog.Init(myConf.szLocalIp)) < 0)
		return SLOG_ERROR_LINE;

	myConf.pAppInfo = slog.GetAppInfo();
	if(myConf.pAppInfo == NULL)
	{
		FATAL_LOG("get pAppInfo:%p failed !", myConf.pAppInfo);
		return SLOG_ERROR_LINE;
	}

	if(slog.InitWarnInfo() < 0)
	{
		FATAL_LOG("InitWarnInfo failed");
		return SLOG_ERROR_LINE;
	}

	return 0;
}

int main(int argc, char **argv, char **envp)
{
	int32_t iRet = 0;
	stConfig.argc = argc;
	stConfig.argv = argv;
	if((iRet=InitFastCgi_first(stConfig)) < 0)
	{
		printf("InitCgi failed ! ret:%d\n", iRet);
		return -1;
	}

	// 
	if(AfterCgiInit(stConfig) <= 0)
		return SLOG_ERROR_LINE;

	while(FCGI_Accept() >= 0)
	{
		stConfig.argc = argc;
		stConfig.argv = argv;
		stConfig.envp = envp;

		// 
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

		// 
		iRet=AfterCgiRequestInit(stConfig);
		if(iRet == 0)
			continue;
		else if(iRet < 0)
			break;
		SetCgiResponseType(stConfig, s_JsonRequest);

		if((iRet=CheckLogin(
			stConfig.cgi, stConfig.pshmLoginList, stConfig.remote, stConfig.dwCurTime, &stConfig.stUser)) <= 0)
		{
			INFO_LOG("remote:%s access:%s with no logined cookie !", stConfig.remote, stConfig.pszCgiName);
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

		const char *pAction = stConfig.pAction;
		if(NULL == pAction)
		{
			REQERR_LOG("have no action from :%s", stConfig.remote);
			show_errpage(NULL, CGI_REQERR, stConfig);
			continue;
		}
		INFO_LOG("get action :%s from :%s", pAction, stConfig.remote);

		if(DealDbConnect(stConfig) < 0) {
			show_errpage(NULL, CGI_ERR_SERVER, stConfig);
			continue;
		}

		if(!strcmp(pAction, "add"))
			iRet = DealAddView();
		else if(!strcmp(pAction, "delete"))
			iRet = DeleteView();
		else if(!strcmp(pAction, "mod"))
			iRet = DealModView();
		else if(!strcmp(pAction, "save_add_view"))
			iRet = SaveView();
		else if(!strcmp(pAction, "save_mod_view"))
			iRet = SaveView(true);
		else if(!strcmp(pAction, "search"))
			iRet = DealViewSearch();
		else if(!strcmp(pAction, "view_unauto_bind_machine"))
			iRet = DealUnAutoBindMachine();
		else if(!strcmp(pAction, "lookUpView"))
			iRet = DealViewLookUp();
		else  // default -- list
		{
			pAction = "list";
			iRet = DealListView();
		}

		if(iRet < 0)
		{
			show_errpage(NULL, NULL, stConfig);
			continue;
		}

		const char *pcsTemplate = NULL;

		if(!strcmp(pAction, "add") || !strcmp(pAction, "mod"))
			pcsTemplate = "dmt_view_manage.html";
		else if(!strcmp(pAction, "list") || !strcmp(pAction, "search"))
			pcsTemplate = "dmt_view.html";
		else if(!strcmp(pAction, "lookUpView"))
			pcsTemplate = "dmt_look_view.html";

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

	if(stConfig.cgi != NULL)
		DealCgiFailedExit(stConfig, stConfig.err);

	stConfig.dwEnd = time(NULL);
	INFO_LOG("fcgi - %s stop at:%u run:%u pid:%u errmsg:%s",
		stConfig.pszCgiName, stConfig.dwEnd, stConfig.dwEnd - stConfig.dwStart, stConfig.pid, strerror(errno));
	return 0;
}

