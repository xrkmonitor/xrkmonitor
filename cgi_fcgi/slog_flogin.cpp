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

   fastcgi slog_flogin: 
          实现用户登录登出监控系统的 web 控制台

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
#include <errno.h>
#include <sv_net.h>
#include <mt_report.h>
#include <FloginSession.h>
#include <map>
#include <cgi_head.h>
#include <cgi_comm.h>

#define MAX_LOG_HISTORY_PER_USER 12

// table flogin_type status 状态码
#define STATUS_USE 0
#define STATUS_DELETE 1

CSupperLog slog;
CGIConfig stConfig;
int32_t g_iLoginType;
char g_strRedirectUri[256];

typedef struct {
	int iLoginType;
	char szUserName[33];
}TFreeAccountInfo;

static int InitFastCgi_first(CGIConfig &stConfig)
{
	if(InitFastCgiStart(stConfig) < 0) {
		ERR_LOG("InitFastCgiStart failed !");
		return SLOG_ERROR_LINE;
	}

	snprintf(g_strRedirectUri, sizeof(g_strRedirectUri), "%s/mt_slog_monitor", stConfig.szCgiPath);
	if(slog.InitConfigByFile(stConfig.szConfigFile) < 0 || slog.Init(stConfig.szLocalIp) < 0)
		return SLOG_ERROR_LINE;
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

static int SetFreeAccountInfo(CGI *cgi, HDF *hdf)
{
	uint32_t dwCurTime = time(NULL);
	char sBuf[256] = {0};
	snprintf(sBuf, sizeof(sBuf), 
		"select user_name,user_id,login_type,login_index from flogin_user where last_login_time<%u or login_index<0",
		dwCurTime-60); 
	if(stConfig.qu->get_result(sBuf)) 
	{
		int32_t iUserId = 0, iLoginType = 0, iLoginIdx = 0;
		std::multimap<uint32_t, TFreeAccountInfo> mapFreeUser;
		TFreeAccountInfo info;
		memset(&info, 0, sizeof(info));
		uint32_t dwLastAccessTime = 0;
		bool bFree = false;

		while(stConfig.qu->fetch_row()) {
			iLoginIdx = stConfig.qu->getval("login_index");
			iUserId = stConfig.qu->getval("user_id");
			iLoginType = stConfig.qu->getval("login_type");
			bFree = false;
			dwLastAccessTime = 0;

			if(iLoginIdx >= 0 && iLoginIdx < FLOGIN_SESSION_NODE_COUNT
				&& stConfig.pshmLoginList->stLoginList[iLoginIdx].bLoginType == iLoginType
				&& stConfig.pshmLoginList->stLoginList[iLoginIdx].iUserId == iUserId
				&& stConfig.pshmLoginList->stLoginList[iLoginIdx].dwLastAccessTime+60 < dwCurTime)
			{
				dwLastAccessTime = stConfig.pshmLoginList->stLoginList[iLoginIdx].dwLastAccessTime;
				bFree = true;
			}
			else if(iLoginIdx < 0 || iLoginIdx >= FLOGIN_SESSION_NODE_COUNT
				|| stConfig.pshmLoginList->stLoginList[iLoginIdx].iUserId != iUserId
				|| stConfig.pshmLoginList->stLoginList[iLoginIdx].bLoginType != iLoginType)
			{
				bFree = true;
			}

			if(bFree && !(iLoginType==1 && iUserId==1))
			{
				info.iLoginType = iLoginType;
				strncpy(info.szUserName, stConfig.qu->getstr("user_name"), sizeof(info.szUserName)-1);
				mapFreeUser.insert(make_pair<uint32_t, TFreeAccountInfo>(dwLastAccessTime, info));
			}
		}

		int iFreeCount = 0;
		int32_t iCommCount = 0, iManagerCount = 0;
		char hdf_pex[32], hdf_name[64];
		std::multimap<uint32_t, TFreeAccountInfo>::iterator it = mapFreeUser.begin();
		for(; it != mapFreeUser.end() && (iCommCount < 1 || iManagerCount < 1); it++) 
		{
			if((iCommCount < 1 && it->second.iLoginType != 1)
				|| (iManagerCount < 1 && it->second.iLoginType == 1))
			{
				sprintf(hdf_pex, "Output.freelists.%d", iFreeCount);
				sprintf(hdf_name, "%s.uname", hdf_pex);
				hdf_set_value(hdf, hdf_name, it->second.szUserName);
				sprintf(hdf_name, "%s.utype", hdf_pex);
				hdf_set_int_value(hdf, hdf_name, it->second.iLoginType);
				iFreeCount++;
				if(it->second.iLoginType != 1)
					iCommCount++;
				else
					iManagerCount++;
				DEBUG_LOG("set free user info - name:%s, type:%d, last actime:%u",
					it->second.szUserName, it->second.iLoginType, it->first);
			}
		}
	}
	else {
		ERR_LOG("have no free account for daemon use");
	}
	stConfig.qu->free_result();
	return 0;
}

static int PopLoginWindow(CGI *cgi, HDF *hdf)
{
	static std::string s_page_dlg_login_dwz;
	static std::string s_page_login_dwz;
	static std::string s_page_login;

	NEOERR *err = NULL;
	if(s_page_login.empty()) {
		s_page_login = stConfig.szCsPath;
		s_page_login += "dmt_login.html";
	}
	if(s_page_dlg_login_dwz.empty()) {
		s_page_dlg_login_dwz = stConfig.szCsPath;
		s_page_dlg_login_dwz += "dmt_dlg_login_dwz.html";
	}
	if(s_page_login_dwz.empty()) {
		s_page_login_dwz = stConfig.szCsPath;
		s_page_login_dwz += "dmt_login_dwz.html";
	}

	if(g_iLoginType != 0)
		hdf_set_int_value(hdf, "config.login_type", g_iLoginType);
	hdf_set_value(hdf, "config.redirect_url", g_strRedirectUri);
	hdf_set_value(hdf, "config.dwzpath", stConfig.szDocPath);
	hdf_set_value(hdf, "config.cgipath", stConfig.szCgiPath);

	if(IsDaemonDenyOp(stConfig, false)) {
		hdf_set_int_value(hdf, "config.isdaemon", 1);
		SetFreeAccountInfo(cgi, hdf);
	}
	else  {
		hdf_set_int_value(hdf, "config.isdaemon", 0);
	}

	if(stConfig.pAction != NULL && !strcmp(stConfig.pAction, "login_dwz"))
	{
		// 日志系统重登可能需要跨站
		hdf_set_value(cgi->hdf, "cgiout.other.cros", "Access-Control-Allow-Origin:*");
		err = cgi_display(cgi, s_page_login_dwz.c_str()); 
	}
	else if(stConfig.pAction != NULL && !strcmp(stConfig.pAction, "pop_dlg_dwz_login"))
	{
		// 日志系统重登可能需要跨站
		hdf_set_value(cgi->hdf, "cgiout.other.cros", "Access-Control-Allow-Origin:*");
		err = cgi_display(cgi, s_page_dlg_login_dwz.c_str()); 
	}
	else
		err = cgi_display(cgi, s_page_login.c_str()); 

	if(err != NULL && err != STATUS_OK)
	{
		STRING str;
		string_init(&str);
		nerr_error_traceback(err, &str);
		ERR_LOG("cgi_display error:%s", str.buf);
		string_clear(&str);
	}

	return 0;
}

static int ResponseCheckResult(CGI *cgi, HDF *hdf, int32_t iResultCode)
{
	static std::string s_page_login;
	if(s_page_login.empty()) {
		s_page_login = stConfig.szCsPath;
		s_page_login += "dmt_login.html";
	}
	
	NEOERR *err = NULL;
	STRING str;
	Json js;

	const char *pResponseMethod = hdf_get_value(stConfig.cgi->hdf, "Query.response_method", "html");
	if(pResponseMethod != NULL && !strcmp(pResponseMethod, "json"))
	{
		js["code"] = (unsigned int)iResultCode;
		js["redirect_url"] = g_strRedirectUri;
		string_init(&str);
		if((err=string_set(&str, js.ToString().c_str())) != STATUS_OK 
			|| (err=cgi_output(cgi, &str)) != STATUS_OK)
		{
			string_clear(&str); // string_set 可能成功，所以要释放
			string_init(&str);
			nerr_error_traceback(err, &str);
			ERR_LOG("hdf_init or cgi_init failed, msg:%s", str.buf);
			string_clear(&str);
			return -1;
		}
		string_clear(&str);
		DEBUG_LOG("flogin result:%d json response string:%s", iResultCode, js.ToString().c_str());
	}
	else
	{
		if(CHECK_RESULT_SUCCESS != iResultCode)
		{
			if(PopLoginWindow(cgi, hdf) >= 0)
			{
				DEBUG_LOG("flogin result:%d jump to login page:%s", iResultCode, s_page_login.c_str()); 
			}
		}
		else
		{
			cgi_redirect_uri(cgi, "%s", g_strRedirectUri);
		}
	}
	return 0;
}

// 剔除同类型的登录, 如不剔除在跨站访问日志系统时会出错
// 注意同一个用户只允许在一个站点上登录,否则在跨站访问日志系统时会出错
static int KickOldLoginInfo(FloginInfo *psess, int iNewIndex)
{
	DEBUG_LOG("try kick user:%u", psess->iUserId);

	FloginList *pshmLoginList = stConfig.pshmLoginList;
	for(int i=0; i < FLOGIN_SESSION_NODE_COUNT; i++)
	{
		if(iNewIndex != i 
			&& psess->iUserId == pshmLoginList->stLoginList[i].iUserId)
		{
			INFO_LOG("kick old login -- user:%u, login index:%d, old login:%d", psess->iUserId, iNewIndex, i);
			memset(pshmLoginList->stLoginList+i, 0, sizeof(FloginInfo));
			break;
		}
	}
	return 0;
}

static int GetBindxrkmonitorUid()
{
	// 从内置管理员账号中获取云账号绑定情况
	Query qu(*stConfig.db);
	char sBuf[128] = {0};
	snprintf(sBuf, sizeof(sBuf), 
		"select bind_xrkmonitor_uid from flogin_user where user_id=1 and login_type=1"); 
	int iBindXrkmonitorUid = 0;
	if(qu.get_result(sBuf) && qu.fetch_row()) 
		iBindXrkmonitorUid = qu.getval("bind_xrkmonitor_uid");
	qu.free_result();
	return iBindXrkmonitorUid;
}

static void AddLoginHistory(FloginInfo *psess)
{
	IM_SQL_PARA* ppara = NULL;
	if(InitParameter(&ppara) < 0) {
		ERR_LOG("sql parameter init failed !");
		return ;
	}
	AddParameter(&ppara, "user_id", psess->iUserId, "DB_CAL");
	AddParameter(&ppara, "login_time", stConfig.dwCurTime, "DB_CAL");
	AddParameter(&ppara, "valid_time", stConfig.dwCurTime+psess->iLoginExpireTime, "DB_CAL");
	AddParameter(&ppara, "login_remote_address", stConfig.remote, NULL);
	AddParameter(&ppara, "receive_server", stConfig.szLocalIp, NULL);
	const char *ptmp = hdf_get_value(stConfig.cgi->hdf, "HTTP.Referer", NULL);
	if(ptmp != NULL)
		AddParameter(&ppara, "referer", ptmp, NULL);
	ptmp = hdf_get_value(stConfig.cgi->hdf, "HTTP.UserAgent", NULL);
	if(ptmp != NULL)
		AddParameter(&ppara, "user_agent", ptmp, NULL);

	std::string strSql;
	strSql = "insert into flogin_history";
	JoinParameter_Insert(&strSql, stConfig.qu->GetMysql(), ppara);
	ReleaseParameter(&ppara);
	stConfig.qu->execute(strSql);
	DEBUG_LOG("add login history, sql:%s", strSql.c_str());

	// 只保留 MAX_LOG_HISTORY_PER_USER 条记录
	char sSql[256] = {0};
	snprintf(sSql, sizeof(sSql), 
		"select xrk_id from flogin_history where user_id=%d order by xrk_id asc", psess->iUserId);
	stConfig.qu->get_result(sSql);
	int iCount = stConfig.qu->num_rows();
	if(iCount > MAX_LOG_HISTORY_PER_USER) 
	{
		DEBUG_LOG("login history records:%d over limit:%d, try delete", iCount, MAX_LOG_HISTORY_PER_USER);
		Query qutmp(*stConfig.db);
		while(iCount > MAX_LOG_HISTORY_PER_USER && stConfig.qu->fetch_row())
		{
			snprintf(sSql, sizeof(sSql), "delete from flogin_history where xrk_id=%d", 
				stConfig.qu->getval("xrk_id"));
			qutmp.execute(sSql);
			iCount--;
		}
	}
	stConfig.qu->free_result();
}


static int CheckLogin(HDF *hdf)
{
	const char *puser = hdf_get_value(hdf, "Query.username", NULL);
	const char *ppass = hdf_get_value(hdf, "Query.password", NULL);

	if(NULL == puser || NULL == ppass || CheckDbString(puser))
	{
		REQERR_LOG("invalid input - user:%p or pass:%p not input !", puser, ppass);
		return SLOG_ERROR_LINE; 
	}

	// 用户名合法性检查
	for(int i=0; *(puser+i) != '\0'; i++)
	{
		if(!isalnum(*(puser+i))) {
			WARN_LOG("invalid user name:%s", puser);
			return CHECK_RESULT_UNAME_UPASS;
		}
	}

	char sBuf[256] = {0};
	snprintf(sBuf, sizeof(sBuf), 
		"select * from flogin_user where user_name=\'%s\'", puser); 
	if(stConfig.qu->get_result(sBuf) && stConfig.qu->fetch_row())
	{
		const char *ppass_db = stConfig.qu->getstr("user_pass_md5");
		if(!strcasecmp(ppass, ppass_db)) // 验证登录成功
		{
			int iBindXrkmonitorUid = stConfig.qu->getval("bind_xrkmonitor_uid");
			g_iLoginType = stConfig.qu->getval("login_type");
			if(g_iLoginType == 1) {
				if(1 != stConfig.qu->getval("user_id"))
					iBindXrkmonitorUid = GetBindxrkmonitorUid();
			}

			int32_t iFreeIndex = GetFloginFree(stConfig.pshmLoginList, stConfig.dwCurTime);
			INFO_LOG("check user login by input ok, user name:%s login type:%d, uid:%d,"
				"logintime:%u, index:%d",
				puser, g_iLoginType, stConfig.qu->getval("user_id"), stConfig.dwCurTime, iFreeIndex);

			// 设置登录 session 信息 
			if(iFreeIndex < 0)
			{
				ERR_LOG("GetFloginFree failed, no free node for user:%s from:%s", puser, stConfig.remote);
				stConfig.qu->free_result();
				return CHECK_RESULT_TOO_MUCH_PEOPLE_LOGIN;
			}
			FloginInfo *psess = stConfig.pshmLoginList->stLoginList+iFreeIndex;
			stConfig.pshmLoginList->iLoginCount++;
			psess->bLoginType = g_iLoginType;
			strncpy(psess->szUserName, puser, sizeof(psess->szUserName)-1);
			strncpy(psess->szPassMd5, ppass, sizeof(psess->szPassMd5)-1);
			psess->dwLoginIP = inet_addr(stConfig.remote);
			psess->dwLoginTime = stConfig.dwCurTime;
			psess->dwLastAccessTime = stConfig.dwCurTime;
			psess->dwLastAccessIP = inet_addr(stConfig.remote);
			psess->iLoginExpireTime = 7*24*60*60;
			psess->dwUserFlag_1 = stConfig.qu->getuval("user_flag_1");
			psess->iUserId = stConfig.qu->getval("user_id");

			user::UserSessionInfo localsess;
			const char *ptmp = stConfig.qu->getstr("email");
			if(ptmp != NULL && ptmp[0] != '\0')
			    localsess.set_email(ptmp);
			localsess.set_register_time(stConfig.qu->getuval("register_time"));
			if(iBindXrkmonitorUid != 0)
				localsess.set_bind_xrkmonitor_uid(iBindXrkmonitorUid);
			SetUserSessionInfo(psess, localsess);

			SetFloginCookie(
				stConfig.cgi, puser, ppass, iFreeIndex, psess->iUserId, g_iLoginType);
			snprintf(sBuf, sizeof(sBuf), 
				"update flogin_user set last_login_time=%u,last_login_address=\'%s\',login_index=%d"
				",login_md5=\'%s\',last_login_server=\'%s\' where user_id=%u",
				psess->dwLoginTime, stConfig.remote, iFreeIndex, ppass, 
				stConfig.szLocalIp, psess->iUserId); 
			stConfig.qu->free_result();
			stConfig.qu->execute(sBuf);
			KickOldLoginInfo(psess, iFreeIndex);
			AddLoginHistory(psess);
			return CHECK_RESULT_SUCCESS;
		}
		REQERR_LOG("check user login failed, sql:%s, dbpass:%s, ppass:%s, user name:%s login type:%d",
			sBuf, ppass_db, ppass, puser, g_iLoginType);
	}
	else
	{
		REQERR_LOG("check user login failed, sql:%s, pass md5:%s user name:%s login type:%d",
			sBuf, ppass, puser, g_iLoginType);
	}
	stConfig.qu->free_result();
	return CHECK_RESULT_UNAME_UPASS;
}


static int DealUserLogout(FloginInfo *psess)
{
	char sBuf[1024] = {0};
	snprintf(sBuf, sizeof(sBuf), 
		"update flogin_user set login_index=-1,login_md5=\'\' where user_id=%u", psess->iUserId); 
	stConfig.qu->execute(sBuf);
	return 0;
}

static int CheckLoginByCookie(HDF *hdf)
{
	const char *puser = hdf_get_value(hdf, "Cookie.flogin_user", NULL); 
	const char *pmd5 = hdf_get_value(hdf, "Cookie.flogin_md5", NULL);
	g_iLoginType = hdf_get_int_value(hdf, "Cookie.flogin_type", 0);
	int32_t iLoginIndex = hdf_get_int_value(hdf, "Cookie.flogin_index", -1);
	int32_t iUserId = hdf_get_int_value(hdf, "Cookie.flogin_uid", -1);

	if(NULL == puser || NULL == pmd5 || iLoginIndex < 0 || iUserId < 0)
	{
		DEBUG_LOG("cookie info: user:%p md5:%p login type:%d login index:%d uid:%d",
			puser, pmd5, g_iLoginType, iLoginIndex, iUserId);
		MtReport_Attr_Add(110, 1);
		return -1; 
	}

	if(iLoginIndex >= FLOGIN_SESSION_NODE_COUNT)
	{
		REQERR_LOG("invalid login index :%d > %d from:%s", 
			iLoginIndex, FLOGIN_SESSION_NODE_COUNT, stConfig.remote);
		MtReport_Attr_Add(110, 1);
		return -1;
	}

	FloginInfo *psess = stConfig.pshmLoginList->stLoginList + iLoginIndex;
	psess->dwLastAccessIP = inet_addr(stConfig.remote);
	if(g_iLoginType != psess->bLoginType 
		|| iUserId != psess->iUserId
		|| stConfig.dwCurTime > psess->dwLastAccessTime+psess->iLoginExpireTime
		|| stConfig.dwCurTime > psess->dwLoginTime+LOGIN_MAX_EXPIRE_TIME
		|| strcmp(puser, psess->szUserName) || strcmp(pmd5, psess->szPassMd5))
	{
		REQERR_LOG("cookie check failed ! -- "
			"type:%d(%d), uid:%d time:%u(%u), user:%s(%s), md5:%s(%s), ip:%u(%u), from:%s",
			g_iLoginType, psess->bLoginType, iUserId, psess->dwLastAccessTime+psess->iLoginExpireTime,
			stConfig.dwCurTime, puser, psess->szUserName, pmd5, psess->szPassMd5, 
			psess->dwLoginIP, psess->dwLastAccessIP, stConfig.remote);
		MtReport_Attr_Add(110, 1);
		return -1;
	}

	psess->dwLastAccessTime = stConfig.dwCurTime;
	stConfig.stUser.puser_info = psess;
	INFO_LOG("cookie check ok -- user:%s(%d), update cookie time, expire in:%u",
		puser, iUserId, psess->dwLastAccessTime+psess->iLoginExpireTime);
	MtReport_Attr_Add(111, 1);
	return 1;
}

int main(int argc, char **argv, char **envp)
{
	int32_t iRet = 0, iResultCode = 0;
	stConfig.argc = argc;
	stConfig.argv = argv;
	if((iRet=InitFastCgi_first(stConfig)) < 0)
	{
		printf("InitCgi failed ! ret:%d, argc:%d\n", iRet, argc);
		MtReport_Attr_Add(108, 1);
		return -1;
	}
	INFO_LOG("fcgi:%s argc:%d start pid:%u", stConfig.pszCgiName, argc, stConfig.pid);

	if(AfterCgiInit(stConfig) <= 0)
		return SLOG_ERROR_LINE;

	while(FCGI_Accept() >= 0 || argc > 1)
	{
		stConfig.argc = argc;
		stConfig.argv = argv;
		stConfig.envp = envp;

		iRet=BeforeCgiRequestInit(stConfig);
		if(iRet == 0)
			continue;
		else if(iRet < 0)
			break;

		if(argc <= 1)
		{
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

		if(DealDbConnect(stConfig) < 0)
			return SLOG_ERROR_LINE;

		const char *paction = stConfig.pAction;
		const char *puser = stConfig.stUser.puser;
		if(puser != NULL && paction != NULL && !strcmp(paction, "logout")) {
			if(CheckLoginByCookie(stConfig.cgi->hdf) > 0) {
				FloginInfo *psess = stConfig.stUser.puser_info;
				DealUserLogout(psess);
				int32_t iLoginIndex = hdf_get_int_value(stConfig.cgi->hdf, "Cookie.flogin_index", -1);
				stConfig.pshmLoginList->stLoginList[iLoginIndex].bLoginType = 0;
				stConfig.pshmLoginList->iLoginCount--;
			}

			g_iLoginType = hdf_get_int_value(stConfig.cgi->hdf, "Cookie.flogin_type", 0);
			cgi_cookie_clear(stConfig.cgi, "flogin_md5", NULL, NULL);
			PopLoginWindow(stConfig.cgi, stConfig.cgi->hdf);
			MtReport_Attr_Add(112, 1);
			INFO_LOG("user :%s type:%d logout !", puser, g_iLoginType);
		}
		else if((iRet=CheckLoginByCookie(stConfig.cgi->hdf)) > 0) // 通过 cookie 验证登录成功
		{
			if(ResponseCheckResult(stConfig.cgi, stConfig.cgi->hdf, 0) < 0)
			{
				break;
			}
		} 
		else 
		{
			iResultCode = CheckLogin(stConfig.cgi->hdf); 
			if(iResultCode >= 0) // 通过输入密码验证登录
			{
				if(ResponseCheckResult(stConfig.cgi, stConfig.cgi->hdf, iResultCode) < 0)
					break;
				MtReport_Attr_Add(113, 1);
			}
			else  // 弹出登陆框
			{
				PopLoginWindow(stConfig.cgi, stConfig.cgi->hdf);
				MtReport_Attr_Add(112, 1);
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
	INFO_LOG("fcgi - %s exist at:%u run:%u pid:%u msg:%s",
		stConfig.pszCgiName, stConfig.dwEnd, stConfig.dwEnd-stConfig.dwStart, stConfig.pid, strerror(errno));
	return 0;
}

