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

   cgi mt_user: 用户账号相关

****/

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS 1
#endif

#include <string>
#include <fcgi_stdio.h>
#include <fcgi_config.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>
#include <inttypes.h>
#include <cgi_head.h>
#include <cgi_comm.h>
#include <algorithm>

#include <sv_str.h>
#include <list>

using namespace std;

#define VERIFY_TYPE_MIN 0
#define VERIFY_TYPE_MAX 2

CSupperLog slog;
CGIConfig stConfig;

// ajax json 响应方式
static const char *s_JsonRequest [] = { 
	"change_pwd",
	"delete_user",
	"save_new_user",
	"send_op_vcode",
	"change_email",
	"reset_pass",
	"su_reset_pass",
	"change_uname",
	"save_system_config",
	NULL
};

typedef struct
{
	const char *pszUserName;
}UserSearchInfo;

static int GetUserTotalRecords(UserSearchInfo *pinfo=NULL)
{
	char sSqlBuf[512] = {0};
	char sTmpBuf[128] = {0};
	Query & qu = *stConfig.qu;
	
	sprintf(sSqlBuf, "select count(*) from flogin_user where xrk_status=%d", RECORD_STATUS_USE);
	if(pinfo != NULL && pinfo->pszUserName != NULL && pinfo->pszUserName[0] != '\0')
	{
		memset(sTmpBuf, 0, sizeof(sTmpBuf));
		snprintf(sTmpBuf, sizeof(sTmpBuf)-1, " and (flogin_user.user_name like \'%%%s%%\')", pinfo->pszUserName);
		if(strlen(sTmpBuf) + strlen(sSqlBuf) >= sizeof(sSqlBuf))
		{
			REQERR_LOG("search user name (%s) too long, tmp:%s sql:%s", pinfo->pszUserName, sTmpBuf, sSqlBuf);
			stConfig.pErrMsg = CGI_REQERR;
			return SLOG_ERROR_LINE;
		}
		strcat(sSqlBuf, sTmpBuf);
	}

	qu.get_result(sSqlBuf);
	if(qu.num_rows() <= 0 || qu.fetch_row() == NULL)
	{
		qu.free_result();
		ERR_LOG("get user count failed! sql:%s", sSqlBuf);
		return SLOG_ERROR_LINE;
	}
	DEBUG_LOG("search user count - exesql:%s", sSqlBuf);

	int iCount = qu.getval(0);
	qu.free_result();
	DEBUG_LOG("records count:%d", iCount);
	return iCount;
}

static int GetUserList(Json &js, UserSearchInfo *pinfo=NULL)
{
	char sSqlBuf[512] = {0};
	char sTmpBuf[128] = {0};
	Query & qu = *stConfig.qu;
	int iCurPage = hdf_get_int_value(stConfig.cgi->hdf, "config.currentPage", 0);
	int iNumPerPage = hdf_get_int_value(stConfig.cgi->hdf, "config.numPerPage", 0);
	if(iCurPage == 0 || iNumPerPage == 0)
	{
		ERR_LOG("invalid iCurPage(%d) or iNumPerPage(%d)", iCurPage, iNumPerPage);
		return SLOG_ERROR_LINE;
	}
	sprintf(sSqlBuf, "select * from flogin_user where xrk_status=%d", RECORD_STATUS_USE);
	if(pinfo != NULL && pinfo->pszUserName != NULL && pinfo->pszUserName[0] != '\0')
	{
		memset(sTmpBuf, 0, sizeof(sTmpBuf));
		snprintf(sTmpBuf, sizeof(sTmpBuf)-1, " and (flogin_user.user_name like \'%%%s%%\')", pinfo->pszUserName);
		if(strlen(sTmpBuf) + strlen(sSqlBuf) >= sizeof(sSqlBuf))
		{
			REQERR_LOG("search user name (%s) too long, tmp:%s sql:%s", pinfo->pszUserName, sTmpBuf, sSqlBuf);
			stConfig.pErrMsg = CGI_REQERR;
			return SLOG_ERROR_LINE;
		}
		strcat(sSqlBuf, sTmpBuf);
		hdf_set_value(stConfig.cgi->hdf, "config.dul_user_name", pinfo->pszUserName);
	}

	int iOrder = 0;
	iOrder = (iOrder==0 ? SetRecordsOrder(stConfig.cgi, sSqlBuf, "user_id") : 1);
	iOrder = (iOrder==0 ? SetRecordsOrder(stConfig.cgi, sSqlBuf, "user_name") : 1);
	iOrder = (iOrder==0 ? SetRecordsOrder(stConfig.cgi, sSqlBuf, "last_login_time") : 1);
	if(iOrder == 0) 
		strcat(sSqlBuf, " order by last_login_time desc");

	memset(sTmpBuf, 0, sizeof(sTmpBuf));
	sprintf(sTmpBuf, " limit %d,%d", iNumPerPage*(iCurPage-1), iNumPerPage);
	strcat(sSqlBuf, sTmpBuf);
	DEBUG_LOG("get user list - exesql:%s", sSqlBuf);

	qu.get_result(sSqlBuf);
	if(qu.num_rows() < 0)
	{
		qu.free_result();
		ERR_LOG("get user list failed!");
		return SLOG_ERROR_LINE;
	}
	
	Query qutmp(*stConfig.db);
	int i=0;
	for(i=0; i < qu.num_rows() && qu.fetch_row() != NULL; i++)
	{
		Json user;
		user["user_id"] = qu.getstr("user_id");
		user["user_name"] = qu.getstr("user_name");
		user["update_time"] = qu.getstr("update_time");
		user["email"] = qu.getstr("email");
		user["remark"] = qu.getstr("rmark");
		user["last_login_time"] = uitodate(qu.getuval("last_login_time"));
		user["last_login_address"] = qu.getstr("last_login_address");
		js["list"].Add(user);
	}
	js["count"] = i; 
	DEBUG_LOG("get user list - result count:%d(%d)", qu.num_rows(), i);
	qu.free_result();
	return 0;
}

static int DeleteUser()
{
	if(IsDaemonDenyOp(stConfig))
		return 0;

	int id = hdf_get_int_value(stConfig.cgi->hdf, "Query.id", 0);
	if(id == 0)
	{
		WARN_LOG("invalid parameter(id:%d) from:%s", id, stConfig.remote);
		stConfig.pErrMsg = CGI_REQERR;
		return SLOG_ERROR_LINE;
	}

	FloginInfo *pUserInfo = stConfig.stUser.puser_info;
	if(id == pUserInfo->iUserId) {
		ERR_LOG("try delete self !");
		stConfig.pErrMsg = CGI_DENY_DEL_USELF;
		return SLOG_ERROR_LINE;
	}

	static char sSqlBuf[256];
	sprintf(sSqlBuf, "select user_name,login_type from flogin_user where user_id=%d and xrk_status=%d",
		id, RECORD_STATUS_USE);
	Query & qu = *stConfig.qu;
	qu.get_result(sSqlBuf);
	if(qu.num_rows() <= 0 || qu.fetch_row() == NULL) {
		stConfig.pErrMsg = CGI_DEL_FAILED_UNOT_EXIST;
		ERR_LOG("find user id:%d failed !", id); 
		return SLOG_ERROR_LINE;
	}

	if(qu.getval("login_type") != 0) {
		// 管理员账号不允许删除
		stConfig.pErrMsg = CGI_ACCESS_DENY;
		ERR_LOG("try delete manager account (%s-%d)!", qu.getstr("user_name"), id);
		return SLOG_ERROR_LINE;
	}
	qu.free_result();

	sprintf(sSqlBuf, "update flogin_user set xrk_status=%d where user_id=%d", RECORD_STATUS_DELETE, id);
	if(!qu.execute(sSqlBuf))
	{
		qu.execute("ROLLBACK");
		return SLOG_ERROR_LINE;
	}
	qu.execute(sSqlBuf);

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

	DEBUG_LOG("delete user id:%d success, sql:%s, response string :%s to remote:%s",
		id, sSqlBuf, js.ToString().c_str(), stConfig.remote);
	return 0;
}

static int SaveUser(bool bIsMod=false)
{
	if(IsDaemonDenyOp(stConfig))
		return 0;

	FloginInfo *pUserInfo = stConfig.stUser.puser_info;
	int32_t iUserId = hdf_get_int_value(stConfig.cgi->hdf, "Query.ddau_cur_user_id", 0);
	const char *pname = hdf_get_value(stConfig.cgi->hdf, "Query.ddau_user_name", NULL);
	const char *ppass = hdf_get_value(stConfig.cgi->hdf, "Query.ddau_newPassword", NULL);
	const char *premark = hdf_get_value(stConfig.cgi->hdf, "Query.remark", NULL);
	const char *pcallBkType = hdf_get_value(stConfig.cgi->hdf, "Query.call_back_type", "CloseCurrent");
	const char *pnavTabId = hdf_get_value(stConfig.cgi->hdf, "Query.reload_navTab_id", NULL);

	unsigned int iNameLen = MY_STRLEN(pname);
	unsigned int iPassLen = MY_STRLEN(ppass);
	if(pname == NULL || iNameLen < 4 || iNameLen > 30 
		|| (!bIsMod && ppass == NULL) || (bIsMod && iUserId==0)){
		stConfig.pErrMsg = CGI_REQERR;
		REQERR_LOG("pname:%s(%d) ppass:%s(%d) user id:%d", pname, iNameLen, ppass, iPassLen, iUserId);
		return SLOG_ERROR_LINE;
	}

	IM_SQL_PARA* ppara = NULL;
	InitParameter(&ppara);

	Query & qu = *stConfig.qu;
	AddParameter(&ppara, "user_mod_id", pUserInfo->iUserId, "DB_CAL");

	std::string strSql;
	if(!bIsMod) {
		AddParameter(&ppara, "user_name", pname, NULL);
		AddParameter(&ppara, "user_pass_md5", ppass, NULL); 
		AddParameter(&ppara, "user_add_id", pUserInfo->iUserId, "DB_CAL");
		AddParameter(&ppara, "register_time", stConfig.dwCurTime, "DB_CAL");
	}
	AddParameter(&ppara, "rmark", premark, NULL);

	if(!bIsMod)
		strSql = "insert into flogin_user";
	else
		strSql = "update flogin_user set";

	if(!bIsMod)
		JoinParameter_Insert(&strSql, qu.GetMysql(), ppara);
	else {
		JoinParameter_Set(&strSql, qu.GetMysql(), ppara);
		strSql += " where user_id=";
		strSql += itoa(iUserId);
	}

	ReleaseParameter(&ppara);
	if(!qu.execute(strSql))
	{
		ERR_LOG("execute sql:%s failed, msg:%s", strSql.c_str(), qu.GetError().c_str());

		strSql = "ROLLBACK";
		qu.execute(strSql);
		return SLOG_ERROR_LINE;
	}

	Json js;
	js["statusCode"] = 200;
	js["callbackType"] = pcallBkType;
	js["navTabId"] = pnavTabId;
	
	if(!bIsMod)
	{
		js["msgid"] = "addSuccess";
		iUserId = qu.insert_id();
	}
	else
		js["msgid"] = "modSuccess";
	std::string str_result(js.ToString());

	STRING str;
	string_init(&str);
	if((stConfig.err=string_set(&str, str_result.c_str())) != STATUS_OK
		|| (stConfig.err=cgi_output(stConfig.cgi, &str)) != STATUS_OK)
	{
		string_clear(&str);
		return SLOG_ERROR_LINE;
	}
	string_clear(&str);
	DEBUG_LOG("%s flogin_user name:%s(user id:%u) success, sql:%s, response string :%s to remote:%s ",
		(bIsMod ? "update" : "insert"), pname, iUserId, strSql.c_str(), js.ToString().c_str(), stConfig.remote);
	return 0;
}

static bool CheckUserPassMd5(const char *ppass)
{
	char sBuf[256] = {0};
	Query & qu = *stConfig.qu;

	// 验证老密码
	sprintf(sBuf, "select user_pass_md5 from flogin_user where user_id=%d and xrk_status=%d",
		stConfig.stUser.puser_info->iUserId, RECORD_STATUS_USE);
	if(qu.get_result(sBuf) && qu.fetch_row())
	{
		const char *ppass_md5 = qu.getstr("user_pass_md5");
		if(strcmp(ppass, ppass_md5)){
			INFO_LOG("check pass md5 %s != %s", ppass, ppass_md5);
			qu.free_result();
			return false;
		}
		qu.free_result();
	}
	else {
		WARN_LOG("find user:%d failed", stConfig.stUser.puser_info->iUserId);
		qu.free_result();
		return false;
	}
	return true;
}

static int DealChangePasswd()
{
	if(IsDaemonDenyOp(stConfig))
		return 0;

#define VERIFY_OLD_PASS_FAILED 311

	const char *pold_pass = hdf_get_value(stConfig.cgi->hdf, "Query.oldPassword", NULL);
	const char *pnew_pass = hdf_get_value(stConfig.cgi->hdf, "Query.newPassword", NULL);
	if(pold_pass == NULL || pnew_pass == NULL || CheckDbString(pnew_pass))
	{
		stConfig.pErrMsg = CGI_REQERR;
		REQERR_LOG("oldpass:%s, newpass:%s", pold_pass, pnew_pass);
		return SLOG_ERROR_LINE;
	}

	// 验证老密码
	if(CheckUserPassMd5(pold_pass) == false) 
	{
		stConfig.iErrorCode = VERIFY_OLD_PASS_FAILED;
		return SLOG_ERROR_LINE;
	}

	char sBuf[256] = {0};
	Query & qu = *stConfig.qu;
	char pmd5[33] = {0};
	OI_randstr(pmd5, 32);
	sprintf(sBuf, "update flogin_user set user_pass_md5=\'%s\',login_md5=\'%s\' where user_id=%d",
		pnew_pass, pmd5, stConfig.stUser.puser_info->iUserId); 
	if(!qu.execute(sBuf))
	{
		ERR_LOG("execute sql:%s failed, msg:%s", sBuf, qu.GetError().c_str());
		stConfig.pErrMsg = CGI_ERRDB_SERVER;
		return SLOG_ERROR_LINE;
	}

	// 更新 cookie 信息, cookie 用随机串
	FloginInfo *psess = stConfig.stUser.puser_info;
	strcpy(psess->szPassMd5, pmd5); 
		
	Json js;
	js["statusCode"] = 200;
	js["new_pass_md5"] = pmd5;
	DEBUG_LOG("update password for user:%s(%d) ok, new passmd5:%s",
		stConfig.stUser.puser, stConfig.stUser.puser_info->iUserId, pnew_pass);

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
	return 0;
}

static int DealModUser()
{
	int id = hdf_get_int_value(stConfig.cgi->hdf, "Query.user_id", 0);
	const char *pname = hdf_get_value(stConfig.cgi->hdf, "Query.user_name", NULL);
	const char *premark = hdf_get_value(stConfig.cgi->hdf, "Query.remark", NULL);
	if(id==0 || pname==NULL)
	{
		WARN_LOG("invalid parameter(id:%d, name:%s from:%s", id, pname, stConfig.remote);
		stConfig.pErrMsg = CGI_REQERR;
		return SLOG_ERROR_LINE;
	}

	hdf_set_value(stConfig.cgi->hdf, "config.action", "save_mod_user");
	hdf_set_value(stConfig.cgi->hdf, "config.user_name", pname);
	hdf_set_value(stConfig.cgi->hdf, "config.remark", premark);
	hdf_set_int_value(stConfig.cgi->hdf, "config.user_id", id);
	DEBUG_LOG("try update flogin_user for user(%d:%s) from:%s", id, pname, stConfig.remote);
	return 0;
}

static int DealAddUser()
{
	hdf_set_value(stConfig.cgi->hdf, "config.action", "save_new_user");
	return 0;
}

static int DealListLoginInfo()
{
	UserSearchInfo stInfo;
	stInfo.pszUserName = hdf_get_value(stConfig.cgi->hdf, "Query.duli_user_name", NULL);
	if(stInfo.pszUserName != NULL)
		hdf_set_value(stConfig.cgi->hdf, "config.duli_user_name", stInfo.pszUserName);
	DEBUG_LOG("search login user - name:%s", stInfo.pszUserName);

	int i=0, j=0, iShow=0;
	std::list<int> listMatch;
	for(i=0,j=0; j < FLOGIN_SESSION_NODE_COUNT; j++)
	{
		if(stConfig.pshmLoginList->stLoginList[j].dwLastAccessTime+
			stConfig.pshmLoginList->stLoginList[j].iLoginExpireTime > stConfig.dwCurTime)
		{
			i++; // 总共在线
			if(stInfo.pszUserName != NULL 
				&& strstr(stConfig.pshmLoginList->stLoginList[j].szUserName, stInfo.pszUserName) == NULL)
			{
				continue;
			}
			iShow++; // 符合查询的在线
			listMatch.push_back(j);
		}
	}
	stConfig.pshmLoginList->iLoginCount = i;
	SetRecordsPageInfo(stConfig.cgi, iShow);

	int iCurPage = hdf_get_int_value(stConfig.cgi->hdf, "config.currentPage", 0);
	int iNumPerPage = hdf_get_int_value(stConfig.cgi->hdf, "config.numPerPage", 0);
	
	Json js;
	int iShowIndex = 0;
	std::list<int>::iterator it = listMatch.begin();
	for(j=0,iShow=0; it != listMatch.end(); it++)
	{
		j = *it;
		iShowIndex++;
		if(iShowIndex < iNumPerPage*(iCurPage-1))
			continue;

		Json info;
		info["type_id"] = stConfig.pshmLoginList->stLoginList[j].bLoginType;
		info["user_name"] = stConfig.pshmLoginList->stLoginList[j].szUserName;
		info["login_addr"] = ipv4_addr_str(stConfig.pshmLoginList->stLoginList[j].dwLoginIP);
		info["login_time"] = uitodate(stConfig.pshmLoginList->stLoginList[j].dwLoginTime);
		info["last_access_time"] = uitodate(stConfig.pshmLoginList->stLoginList[j].dwLastAccessTime);
		if(stConfig.pshmLoginList->stLoginList[j].dwLastAccessTime
			+ stConfig.pshmLoginList->stLoginList[j].iLoginExpireTime >
			stConfig.pshmLoginList->stLoginList[j].dwLoginTime+LOGIN_MAX_EXPIRE_TIME)
		{
			info["expire_time"] = uitodate(
				stConfig.pshmLoginList->stLoginList[j].dwLoginTime+LOGIN_MAX_EXPIRE_TIME);
		} else {
			info["expire_time"] = uitodate(stConfig.pshmLoginList->stLoginList[j].dwLastAccessTime
				+stConfig.pshmLoginList->stLoginList[j].iLoginExpireTime);
		}
		js["list"].Add(info);
		iShow++;
		if(iShow >= iNumPerPage)
			break;
	}
	js["count"] = iShow;

	std::string str_user(js.ToString());
	DEBUG_LOG("user login list json:%s", str_user.c_str());

	stConfig.err = hdf_set_value(stConfig.cgi->hdf, "config.user_login_info", str_user.c_str()); 
	if(stConfig.err != STATUS_OK)
	{
		ERR_LOG("set user login info failed, length:%u", (unsigned)str_user.size());
		return SLOG_ERROR_LINE;
	}
	return 0;
}

static int DealUserSearch()
{
	UserSearchInfo stInfo;
	stInfo.pszUserName = hdf_get_value(stConfig.cgi->hdf, "Query.dul_user_name", NULL);
	DEBUG_LOG("search user name:%s", stInfo.pszUserName);

	int iRecords = GetUserTotalRecords(&stInfo);
	if(iRecords < 0)
	{
		ERR_LOG("get attr record count failed !");
		return SLOG_ERROR_LINE;
	}
	SetRecordsPageInfo(stConfig.cgi, iRecords);

	Json js_user;
	if(GetUserList(js_user, &stInfo) < 0)
		return SLOG_ERROR_LINE;
	std::string str_user(js_user.ToString());
	DEBUG_LOG("user list json:%s", str_user.c_str());
	stConfig.err = hdf_set_value(stConfig.cgi->hdf, "config.userinfo", str_user.c_str()); 
	if(stConfig.err != STATUS_OK)
	{
		ERR_LOG("set attr type info failed, length:%u", (unsigned)str_user.size());
		return SLOG_ERROR_LINE;
	}
	return 0;
}

static int DealListUser()
{
	int iRecords = GetUserTotalRecords();
	if(iRecords < 0)
	{
		ERR_LOG("get user records count failed !");
		return SLOG_ERROR_LINE;
	}
	SetRecordsPageInfo(stConfig.cgi, iRecords);

	user::UserSessionInfo & sess = stConfig.stUser.pbSess;
	if(sess.email().size() > 0)
		hdf_set_int_value(stConfig.cgi->hdf, "config.has_email", 1);
	else
		hdf_set_int_value(stConfig.cgi->hdf, "config.has_email", 0);

	Json js_user;
	if(GetUserList(js_user) < 0)
		return SLOG_ERROR_LINE;
	std::string str_user(js_user.ToString());
	DEBUG_LOG("user list json:%s", str_user.c_str());

	stConfig.err = hdf_set_value(stConfig.cgi->hdf, "config.userinfo", str_user.c_str()); 
	if(stConfig.err != STATUS_OK)
	{
		ERR_LOG("set user info failed, length:%u", (unsigned)str_user.size());
		return SLOG_ERROR_LINE;
	}
	return 0;
}

int DetailUser()
{
	int iUserId =  hdf_get_int_value(stConfig.cgi->hdf, "Query.user_id", 0);
	if(iUserId == 0) {
		ERR_LOG("have no user id in request !");
		stConfig.pErrMsg = CGI_REQERR;
		return SLOG_ERROR_LINE;
	}

	char sSqlBuf[256] = {0};
	Query & qu = *stConfig.qu;
	sprintf(sSqlBuf, "select * from flogin_user where user_id=%d and xrk_status=%d", 
		iUserId, RECORD_STATUS_USE);
	qu.get_result(sSqlBuf);
	if(qu.num_rows() < 1 || qu.fetch_row() == NULL) {
		WARN_LOG("get user info have count:%d !", qu.num_rows());
		return SLOG_ERROR_LINE;
	}

	hdf_set_int_value(stConfig.cgi->hdf, "config.user_id", iUserId);
	hdf_set_value(stConfig.cgi->hdf, "config.user_name", qu.getstr("user_name"));
	hdf_set_int_value(stConfig.cgi->hdf, "config.user_type", qu.getval("login_type"));
	hdf_set_value(stConfig.cgi->hdf, "config.register_time", uitodate(qu.getuval("register_time")));
	hdf_set_value(stConfig.cgi->hdf, "config.update_time", qu.getstr("update_time"));
	hdf_set_value(stConfig.cgi->hdf, "config.remark", qu.getstr("rmark"));
	hdf_set_value(stConfig.cgi->hdf, "config.last_login_address", qu.getstr("last_login_address"));
	hdf_set_value(stConfig.cgi->hdf, "config.last_login_time", uitodate(qu.getuval("last_login_time"))); 
	hdf_set_int_value(stConfig.cgi->hdf, "config.user_add_id", qu.getval("user_add_id")); 
	hdf_set_int_value(stConfig.cgi->hdf, "config.user_mod_id", qu.getval("user_mod_id")); 

	const char *ptmp = qu.getstr("email");
	if(ptmp != NULL && ptmp[0] != '\0')
		hdf_set_value(stConfig.cgi->hdf, "config.email", DumpStrByMask(ptmp, 0)); 
	hdf_set_value(stConfig.cgi->hdf, "config.dwReserved1", qu.getstr("dwReserved1"));
	hdf_set_value(stConfig.cgi->hdf, "config.dwReserved2", qu.getstr("dwReserved2"));
	hdf_set_value(stConfig.cgi->hdf, "config.strReserved1", qu.getstr("strReserved1"));
	hdf_set_value(stConfig.cgi->hdf, "config.strReserved2", qu.getstr("strReserved2"));
	hdf_set_value(stConfig.cgi->hdf, "config.user_flag_1", qu.getstr("user_flag_1"));
	qu.free_result();
	return 0;
}

static int DealUserCenter()
{
	FloginInfo *pUserInfo = stConfig.stUser.puser_info;
	hdf_set_int_value(stConfig.cgi->hdf, "config.user_id", pUserInfo->iUserId);
	hdf_set_value(stConfig.cgi->hdf, "config.user_name", pUserInfo->szUserName);

	user::UserSessionInfo & user = stConfig.stUser.pbSess;
	if(user.email().size() > 0)
		hdf_set_value(stConfig.cgi->hdf, "config.user_email", DumpStrByMask2(user.email().c_str()));
	char sSqlBuf[512] = {0};
	Query & qu = *stConfig.qu;

	if(user.has_bind_xrkmonitor_uid())
		hdf_set_int_value(stConfig.cgi->hdf, "config.xrkmonitor_account", user.bind_xrkmonitor_uid());
	
	snprintf(sSqlBuf, sizeof(sSqlBuf), 
		"select * from flogin_history where user_id=%d order by login_time desc", pUserInfo->iUserId);
	qu.get_result(sSqlBuf);
	int iCount = 0;
	Json js;
	while(qu.fetch_row())
	{
		Json login;
		login["time"] = uitodate(qu.getuval("login_time"));
		login["remote"] = qu.getstr("login_remote_address");

		login["method"] = qu.getval("method");
		login["valid_time"] = uitodate(qu.getuval("valid_time"));

		std::string s = qu.getstr("user_agent");
		transform(s.begin(), s.end(),s.begin(), ::tolower);
		if(s.find("firefox") != std::string::npos)
			login["agent"] = 1;
		else if(s.find("chrome") != std::string::npos)
			login["agent"] = 2;
		else
			login["agent"] = 0;
		js["list"].Add(login);
		iCount++;
	}
	js["count"] = iCount;
	hdf_set_value(stConfig.cgi->hdf, "config.history_list", js.ToString().c_str());
	qu.free_result();
	DEBUG_LOG("login history count:%d", iCount);
	return 0;
}

static int DealInitChangeInfo()
{
	const char *pf = hdf_get_value(stConfig.cgi->hdf, "Query.field", NULL);
	int iHasEmail = hdf_get_int_value(stConfig.cgi->hdf, "Query.has_email", -1);
	int iModUserId = hdf_get_int_value(stConfig.cgi->hdf, "Query.mod_user_id", -1);
	const char *pModUserName = hdf_get_value(stConfig.cgi->hdf, "Query.mod_user_name", NULL);

	/*
	   * 注意该函数添加校验后，所有调用的页面需要确认是否受影响
	   * grep init_change_info *.html --
	   *
	   */
	if(pf == NULL || iHasEmail < 0 || (strcmp(pf, "xrkmonitor") && strcmp(pf, "name") 
			&& strcmp(pf, "email") && strcmp(pf, "pass") && strcmp(pf, "su_pass"))
		|| (!strcmp(pf, "su_pass") && (iModUserId <= 0 || pModUserName==NULL)))
	{
		REQERR_LOG("invalid para (%p|%s|%d|%d|%p)", pf, (pf!=NULL ? pf : ""), 
			iHasEmail, iModUserId, pModUserName);
		stConfig.pErrMsg = CGI_REQERR;
		return SLOG_ERROR_LINE;
	}

	// 管理员重置密码
	if(!strcmp(pf, "su_pass"))
	{
		user::UserSessionInfo & sess = stConfig.stUser.pbSess;
		if(sess.email().size() <= 0 || stConfig.stUser.puser_info->bLoginType != 1)
		{
			REQERR_LOG("invalid req %d|%d", (int)sess.email().size(), stConfig.stUser.puser_info->iUserId);
			stConfig.pErrMsg = CGI_REQERR;
			return SLOG_ERROR_LINE;
		}
		if(sess.email().size() > 0)
			iHasEmail = 1;
		hdf_set_int_value(stConfig.cgi->hdf, "config.mod_user_id", iModUserId);
		hdf_set_value(stConfig.cgi->hdf, "config.mod_user_name", pModUserName);
	}

	hdf_set_int_value(stConfig.cgi->hdf, "config.has_email", iHasEmail);
	hdf_set_value(stConfig.cgi->hdf, "config.field", pf);

	if(!strcmp(pf, "xrkmonitor")) 
		hdf_set_value(stConfig.cgi->hdf, "config.xrkmonitor_url", stConfig.szXrkmonitorSiteAddr);
	DEBUG_LOG("receive change info:%s, has email:%d", pf, iHasEmail);
	return 0;
}

static int DealChangeEmail()
{
	if(IsDaemonDenyOp(stConfig))
		return 0;

	const char *pnew_email = hdf_get_value(stConfig.cgi->hdf, "Query.new_email", NULL);
	const char *ppass = hdf_get_value(stConfig.cgi->hdf, "Query.pass", NULL);
	const char *pvcode = hdf_get_value(stConfig.cgi->hdf, "Query.vcode", NULL);
	int iVerifyType = hdf_get_int_value(stConfig.cgi->hdf, "Query.verify_type", -1);
	if(pnew_email == NULL || iVerifyType != 1 || !IsUserEmailValid(pnew_email) || ppass == NULL || pvcode == NULL)
	{
		REQERR_LOG("invalid para(%p[%s]|%p|%p|%d)", 
			pnew_email, (pnew_email != NULL ? pnew_email : ""), ppass, pvcode, iVerifyType);
		stConfig.pErrMsg = CGI_REQERR;
		return SLOG_ERROR_LINE;
	}

	int ret = 0;

	// 校验操作
	if(!CheckUserPassMd5(ppass)) { // 密码校验
		REQERR_LOG("check user passwd failed|%s", ppass);
		ret = 301;
	}

	user::UserSessionInfo & sess = stConfig.stUser.pbSess;
	if(ret == 0) 
	{ 
		if(sess.op_code_type() != user::OP_VCODE_TYPE_EMAIL) // 邮箱验证码校验
			WARN_LOG("check op_code_type failed %d != %d", sess.op_code_type(), user::OP_VCODE_TYPE_EMAIL);

		if(sess.op_code_set_time()+OP_CODE_VALID_TIME < stConfig.dwCurTime)
		{
			ret = 302;
			REQERR_LOG("email vcode expire %u < %u", sess.op_code_set_time()+OP_CODE_VALID_TIME, stConfig.dwCurTime);
		}
		else if(strcmp(sess.op_code_val().c_str(), pvcode))
		{
			ret = 303;
			REQERR_LOG("check email vcode failed %s != %s", sess.op_code_val().c_str(), pvcode);
		}
		else if(strcmp(sess.op_new_email().c_str(), pnew_email))
		{
			ret = 305;
			REQERR_LOG("email not match %s != %s", sess.op_new_email().c_str(), pnew_email);
		}
	}

	if(ret == 0) {
		char sBuf[256] = {0};
		Query & qu = *stConfig.qu;
		sprintf(sBuf, "update flogin_user set email=\'%s\' where user_id=%d",
			pnew_email, stConfig.stUser.puser_info->iUserId); 
		if(!qu.execute(sBuf))
		{
			stConfig.pErrMsg = CGI_ERRDB_SERVER;
			return SLOG_ERROR_LINE;
		}

		// 验证通过清除老的验证码信息
		if(ret == 0) {
			sess.clear_op_code_val();
			sess.clear_op_code_set_time();
			sess.clear_op_new_email();
			sess.set_email(pnew_email);
			SetUserSessionInfo(stConfig.stUser.puser_info, sess);
		}
	}

	Json js;
	js["ec"] = ret;

	STRING str;
	string_init(&str);
	if((stConfig.err=string_set(&str, js.ToString().c_str())) != STATUS_OK
		|| (stConfig.err=cgi_output(stConfig.cgi, &str)) != STATUS_OK)
	{
		string_clear(&str);
		return SLOG_ERROR_LINE;
	}
	string_clear(&str);
	DEBUG_LOG("send response:%s", js.ToString().c_str());
	return 0;
}

static bool IsUserEmailExist(const char *pemail)
{
	char sBuf[256];
	snprintf(sBuf, sizeof(sBuf), "select user_id from flogin_user where email=\'%s\' and xrk_status=%d",
		pemail, STATUS_USE);
	if(stConfig.qu->get_result(sBuf) && stConfig.qu->fetch_row()) 
	{
		DEBUG_LOG("user email:%s exist uid:%d", pemail, stConfig.qu->getval("user_id"));
		stConfig.qu->free_result();
		return true;
	}
	stConfig.qu->free_result();
	return false;
}

static bool IsUserNameExist(const char *pname)
{
	char sBuf[256];
	snprintf(sBuf, sizeof(sBuf), "select user_id from flogin_user where user_name=\'%s\' and xrk_status=%d",
		pname, STATUS_USE);
	if(stConfig.qu->get_result(sBuf) && stConfig.qu->fetch_row()) 
	{
		DEBUG_LOG("user name:%s exist uid:%d", pname, stConfig.qu->getval("user_id"));
		stConfig.qu->free_result();
		return true;
	}
	stConfig.qu->free_result();
	return false;
}

static int DealResetPass(bool bIsSuReset)
{
	if(IsDaemonDenyOp(stConfig))
		return 0;

	const char *ppass = hdf_get_value(stConfig.cgi->hdf, "Query.pass", NULL);
	int iModUserId = hdf_get_int_value(stConfig.cgi->hdf, "Query.mod_user_id", -1);
	const char *pvcode = hdf_get_value(stConfig.cgi->hdf, "Query.vcode", NULL);
	int iVerifyType = hdf_get_int_value(stConfig.cgi->hdf, "Query.verify_type", -1);
	if(ppass==NULL || pvcode==NULL || iVerifyType!=2 || (bIsSuReset && iModUserId < 0))
	{
		REQERR_LOG("invalid para(%p|%p|%d|%d|%d)", ppass, pvcode, iVerifyType, bIsSuReset, iModUserId);
		stConfig.pErrMsg = CGI_REQERR;
		return SLOG_ERROR_LINE;
	}

	int ret = 0;
	user::UserSessionInfo & sess = stConfig.stUser.pbSess;
	if((bIsSuReset && iModUserId != (int)sess.su_op_user_id()) || (!bIsSuReset && sess.has_su_op_user_id()))
	{
		ret = 304;
		REQERR_LOG("su reset pass, check user failed |%d|%d|%d", bIsSuReset, iModUserId, sess.su_op_user_id());
	}
	else if(sess.op_code_set_time()+OP_CODE_VALID_TIME < stConfig.dwCurTime)
	{
		ret = 302;
		REQERR_LOG("email vcode expire %u < %u", sess.op_code_set_time()+OP_CODE_VALID_TIME, stConfig.dwCurTime);
	}
	else if(strcmp(sess.op_code_val().c_str(), pvcode))
	{
		ret = 303;
		REQERR_LOG("check email vcode failed %s != %s", sess.op_code_val().c_str(), pvcode);
	}

	// 验证通过清除老的验证码信息
	if(ret == 0) {
		sess.clear_op_code_val();
		sess.clear_op_code_set_time();
		sess.clear_su_op_user_id();
		sess.clear_op_new_email();
		sess.clear_op_check_expire_time();
		SetUserSessionInfo(stConfig.stUser.puser_info, sess);
	}

	Json js;
	if(ret == 0) 
	{
		char sBuf[256] = {0};
		char pmd5[33] = {0};
		Query & qu = *stConfig.qu;

		if(iModUserId > 0 && iModUserId != stConfig.stUser.puser_info->iUserId) {
			// 管理员重置其它用户的密码
			snprintf(sBuf, sizeof(sBuf), "update flogin_user set user_pass_md5=\'%s\' where user_id=%d",
				ppass, iModUserId);
		}
		else {
			OI_randstr(pmd5, 32);
			snprintf(sBuf, sizeof(sBuf), "update flogin_user set user_pass_md5=\'%s\',login_md5=\'%s\' where user_id=%d",
				ppass, pmd5, stConfig.stUser.puser_info->iUserId); 
		}
		if(!qu.execute(sBuf))
		{
			stConfig.pErrMsg = CGI_ERRDB_SERVER;
			return SLOG_ERROR_LINE;
		}

		if(!bIsSuReset || iModUserId == stConfig.stUser.puser_info->iUserId)
		{
			// 更新 cookie 信息, cookie 用随机串
			FloginInfo *psess = stConfig.stUser.puser_info;
			strcpy(psess->szPassMd5, pmd5); 
			js["new_login_md5"] = pmd5;
			DEBUG_LOG("update password for user:%s(%d) ok, new login md5:%s",
				stConfig.stUser.puser, stConfig.stUser.puser_info->iUserId, pmd5);
		}
	}

	js["ec"] = ret;

	STRING str;
	string_init(&str);
	if((stConfig.err=string_set(&str, js.ToString().c_str())) != STATUS_OK
		|| (stConfig.err=cgi_output(stConfig.cgi, &str)) != STATUS_OK)
	{
		string_clear(&str);
		return SLOG_ERROR_LINE;
	}
	string_clear(&str);
	DEBUG_LOG("reset pass ok - reset:%d, pass:%s, iModUserId:%d, iVerifyType:%d", bIsSuReset, ppass, iModUserId, iVerifyType);
	return 0;
}

static int DealTestXrkmonitorWarn()
{
	if(IsDaemonDenyOp(stConfig))
		return 0;

	Json js;

	// 261 为预置的异常监控点，用于测试告警通道
	AttrInfoBin *pInfo = slog.GetAttrInfo(261, NULL);
	if(pInfo == NULL) {
		js["ec"] = 1;
		WARN_LOG("not find attr:261");
	}
	else {
		if(MtReport_Attr_Add(261, 1) < 0)
			js["ec"] = 2;
		else
			js["ec"] = 0;
	}

	STRING str;
	string_init(&str);
	if((stConfig.err=string_set(&str, js.ToString().c_str())) != STATUS_OK
		|| (stConfig.err=cgi_output(stConfig.cgi, &str)) != STATUS_OK)
	{
		string_clear(&str);
		return SLOG_ERROR_LINE;
	}
	string_clear(&str);
	DEBUG_LOG("send response:%s", js.ToString().c_str());
	return 0;
}

static int DealUnBindXrkmonitor()
{
	if(IsDaemonDenyOp(stConfig))
		return 0;

	int ixrkmonitor_id = hdf_get_int_value(stConfig.cgi->hdf, "Query.xrkmonitor_uid", -1);
	if(ixrkmonitor_id < 0) {
		REQERR_LOG("invalid para:%d", ixrkmonitor_id);
		stConfig.pErrMsg = CGI_REQERR;
		return SLOG_ERROR_LINE;
	}

	user::UserSessionInfo & user = stConfig.stUser.pbSess;
	if(user.bind_xrkmonitor_uid() != 0) {
		user.set_bind_xrkmonitor_uid(0);
		FloginInfo *pUserInfo = stConfig.stUser.puser_info;
		SetUserSessionInfo(pUserInfo, user);

		char sBuf[256] = {0};
		Query & qu = *stConfig.qu;
		sprintf(sBuf, 
			"update flogin_user set bind_xrkmonitor_uid=0 where bind_xrkmonitor_uid=%d and user_id=%d",
			ixrkmonitor_id, stConfig.stUser.puser_info->iUserId); 
		qu.execute(sBuf);
	}

	Json js;
	js["ec"] = 0;
	STRING str;
	string_init(&str);
	if((stConfig.err=string_set(&str, js.ToString().c_str())) != STATUS_OK
		|| (stConfig.err=cgi_output(stConfig.cgi, &str)) != STATUS_OK)
	{
		string_clear(&str);
		return SLOG_ERROR_LINE;
	}
	string_clear(&str);
	DEBUG_LOG("send response:%s", js.ToString().c_str());
	return 0;
}

static int DealBindXrkmonitor()
{
	const char *pxrkmonitor_name = hdf_get_value(stConfig.cgi->hdf, "Query.xrkmonitor_uname", NULL);
	int ixrkmonitor_id = hdf_get_int_value(stConfig.cgi->hdf, "Query.xrkmonitor_uid", -1);
	const char *ppass = hdf_get_value(stConfig.cgi->hdf, "Query.pass", NULL);
	int iVerifyType = hdf_get_int_value(stConfig.cgi->hdf, "Query.verify_type", -1);
	if(pxrkmonitor_name == NULL || iVerifyType != 1 || ppass==NULL || ixrkmonitor_id < 0)
	{
		REQERR_LOG("invalid para(%p[%s]|%p|%d|%d)", 
			pxrkmonitor_name, (pxrkmonitor_name != NULL ? pxrkmonitor_name : ""),
			ppass, iVerifyType, ixrkmonitor_id);
		stConfig.pErrMsg = CGI_REQERR;
		return SLOG_ERROR_LINE;
	}

	int ret = 0;
	if(!CheckUserPassMd5(ppass)) { // 密码校验
		REQERR_LOG("check user passwd failed|%s", ppass);
		ret = 301;
	}

	if(ret == 0) {
		char sBuf[256] = {0};
		Query & qu = *stConfig.qu;
		snprintf(sBuf, sizeof(sBuf),
			"update flogin_user set bind_xrkmonitor_uid=%d where user_id=1 and login_type=1", 
			ixrkmonitor_id); 
		if(!qu.execute(sBuf))
		{
			stConfig.pErrMsg = CGI_ERRDB_SERVER;
			return SLOG_ERROR_LINE;
		}
		INFO_LOG("bind xrkmonitor info uid:%d, uname:%s", ixrkmonitor_id, pxrkmonitor_name);

		user::UserSessionInfo & user = stConfig.stUser.pbSess;
		user.set_bind_xrkmonitor_uid(ixrkmonitor_id);
		FloginInfo *pUserInfo = stConfig.stUser.puser_info;
		SetUserSessionInfo(pUserInfo, user);
	}

	Json js;
	js["ec"] = ret;

	STRING str;
	string_init(&str);
	if((stConfig.err=string_set(&str, js.ToString().c_str())) != STATUS_OK
		|| (stConfig.err=cgi_output(stConfig.cgi, &str)) != STATUS_OK)
	{
		string_clear(&str);
		return SLOG_ERROR_LINE;
	}
	string_clear(&str);
	DEBUG_LOG("send response:%s", js.ToString().c_str());
	return 0;

}

static int DealChangeName()
{
	if(IsDaemonDenyOp(stConfig))
		return 0;

	const char *pnew_name = hdf_get_value(stConfig.cgi->hdf, "Query.new_name", NULL);
	const char *ppass = hdf_get_value(stConfig.cgi->hdf, "Query.pass", NULL);
	const char *pvcode = hdf_get_value(stConfig.cgi->hdf, "Query.vcode", NULL);
	int iVerifyType = hdf_get_int_value(stConfig.cgi->hdf, "Query.verify_type", -1);
	if(pnew_name == NULL || (iVerifyType != 1 && iVerifyType != 2) || !IsUserNameValid(pnew_name)
		|| (iVerifyType==1 && ppass==NULL) || (iVerifyType==2 && pvcode == NULL))
	{
		REQERR_LOG("invalid para(%p[%s]|%p|%p|%d)", 
			pnew_name, (pnew_name != NULL ? pnew_name : ""), ppass, pvcode, iVerifyType);
		stConfig.pErrMsg = CGI_REQERR;
		return SLOG_ERROR_LINE;
	}

	int ret = 0;
	if(IsUserNameExist(pnew_name))
	{
		REQERR_LOG("check user name:%s failed, already exist", pnew_name);
		ret = 304;
	}
	else if(iVerifyType == 1) {
		if(!CheckUserPassMd5(ppass)) { // 密码校验
			REQERR_LOG("check user passwd failed|%s", ppass);
			ret = 301;
		}
	}
	else if(iVerifyType == 2)
	{ 
		user::UserSessionInfo & sess = stConfig.stUser.pbSess;
		if(sess.op_code_set_time()+OP_CODE_VALID_TIME < stConfig.dwCurTime)
		{
			ret = 302;
			REQERR_LOG("email vcode expire %u < %u", sess.op_code_set_time()+OP_CODE_VALID_TIME, stConfig.dwCurTime);
		}
		else if(strcmp(sess.op_code_val().c_str(), pvcode))
		{
			ret = 303;
			REQERR_LOG("check email vcode failed %s != %s", sess.op_code_val().c_str(), pvcode);
		}

		// 验证通过清除老的验证码信息
		if(ret == 0) {
			sess.clear_op_code_val();
			sess.clear_op_code_set_time();
			SetUserSessionInfo(stConfig.stUser.puser_info, sess);
		}
	}
	else  {
		REQERR_LOG("unknow verify type:%d", iVerifyType);
		ret = 333;
	}

	if(ret == 0) {
		char sBuf[256] = {0};
		Query & qu = *stConfig.qu;
		sprintf(sBuf, "update flogin_user set user_name=\'%s\' where user_id=%d",
			pnew_name, stConfig.stUser.puser_info->iUserId); 
		if(!qu.execute(sBuf))
		{
			stConfig.pErrMsg = CGI_ERRDB_SERVER;
			return SLOG_ERROR_LINE;
		}
		strncpy(stConfig.stUser.puser_info->szUserName, pnew_name, sizeof(stConfig.stUser.puser_info->szUserName)-1);
	}

	Json js;
	js["ec"] = ret;

	STRING str;
	string_init(&str);
	if((stConfig.err=string_set(&str, js.ToString().c_str())) != STATUS_OK
		|| (stConfig.err=cgi_output(stConfig.cgi, &str)) != STATUS_OK)
	{
		string_clear(&str);
		return SLOG_ERROR_LINE;
	}
	string_clear(&str);
	DEBUG_LOG("send response:%s", js.ToString().c_str());
	return 0;
}

static int DealSendOpVcode()
{
	if(IsDaemonDenyOp(stConfig))
		return 0;

	const char *pf = hdf_get_value(stConfig.cgi->hdf, "Query.field", NULL);
	int iHasEmail = hdf_get_int_value(stConfig.cgi->hdf, "Query.has_email", -1);
	const char *pnew_email = hdf_get_value(stConfig.cgi->hdf, "Query.new_email", NULL);
	
	int iVerifyType = hdf_get_int_value(stConfig.cgi->hdf, "Query.verify_type", -1);
	int iModUserId = hdf_get_int_value(stConfig.cgi->hdf, "Query.mod_user_id", -1);
	const char *pModUserName = hdf_get_value(stConfig.cgi->hdf, "Query.mod_user_name", NULL);
	if(pf == NULL || iHasEmail < 0
		|| (strcmp(pf, "su_pass") && strcmp(pf, "pass") && strcmp(pf, "name") 
			&& strcmp(pf, "email")) 
		|| (!strcmp(pf, "email") && (pnew_email == NULL || !IsUserEmailValid(pnew_email)))
		|| (!strcmp(pf, "pass") && !iHasEmail)
		|| (iVerifyType==2 && !iHasEmail)
		|| (!strcmp(pf, "su_pass") && (iModUserId <= 0 || pModUserName==NULL)))
	{
		REQERR_LOG("invalid para (%p|%s|%d|%p|%d|%d|%p)", 
			pf, (pf!=NULL ? pf : ""), iHasEmail, pnew_email, iVerifyType, iModUserId, pModUserName);
		stConfig.pErrMsg = CGI_REQERR;
		return SLOG_ERROR_LINE;
	}

	// 管理员重置用户密码 -- 检查下是否有权限
	if(!strcmp(pf, "su_pass"))
	{
		// 只允许管理员操作
		FloginInfo *pUserInfo = stConfig.stUser.puser_info;
		if(pUserInfo->bLoginType != 1)
		{
			REQERR_LOG("invalid user type:%d for reset pass", pUserInfo->bLoginType);
			stConfig.pErrMsg = CGI_REQERR;
			return SLOG_ERROR_LINE;
		}
		INFO_LOG("user:%d try reset user:%d password", pUserInfo->iUserId, iModUserId);
	}
	
	Json js;
	user::UserSessionInfo & sess = stConfig.stUser.pbSess;

	// 60 - 验证码发送时间间隔
	if(sess.op_code_set_time() + 60 > stConfig.dwCurTime)
	{
		js["ec"] = 301;
		js["timeout_remain"] = (int)(sess.op_code_set_time() + 60 - stConfig.dwCurTime);
	}
	// 修改/绑定 email - 先检查下新 email 是否已存在
	else if(!strcmp(pf, "email") && IsUserEmailExist(pnew_email)) 
	{
		js["ec"] = 302;
	}
	else 
	{
		char sVerifyCode[7] = {0};
		OI_randstr_number(sVerifyCode, 6);
		sess.set_op_code_val(sVerifyCode);
		sess.set_op_code_set_time(stConfig.dwCurTime);

		if(!strcmp(pf, "su_pass"))
			sess.set_su_op_user_id(iModUserId);
		else
			sess.clear_su_op_user_id();

		static TCommSendMailInfo stMail;
		if(!strcmp(pf, "email"))
		{
			// 修改或者绑定 email
			memset(&stMail, 0, sizeof(stMail));
			strncpy(stMail.szToEmailAddr, pnew_email, sizeof(stMail.szToEmailAddr));
			sess.set_op_code_type(user::OP_VCODE_TYPE_EMAIL);
			sess.set_op_new_email(pnew_email);
		}
		else if((!strcmp(pf, "name") || !strcmp(pf, "pass") || !strcmp(pf, "su_pass")) && iVerifyType == 2
			&& sess.email().size() > 0 && sess.email().size() < sizeof(stMail.szToEmailAddr))
		{
			// 修改用户名或者重置密码 -- 使用 email 验证码
			memset(&stMail, 0, sizeof(stMail));
			strcpy(stMail.szToEmailAddr, sess.email().c_str());
			sess.set_op_code_type(user::OP_VCODE_TYPE_EMAIL);
		}
		else {
			REQERR_LOG("unknow operator, para|%s|%d", pf, iVerifyType);
			return SLOG_ERROR_LINE;
		}

		if(sess.op_code_type() == user::OP_VCODE_TYPE_EMAIL)
		{ 	
			// 邮箱验证码
			stMail.dwMailSeq = rand();
			stMail.dwToUserId =  stConfig.stUser.puser_info->iUserId;
			stMail.dwValidTimeUtc = stConfig.dwCurTime+OP_CODE_VALID_TIME;
			strncpy(stMail.szEmailSubject, "操作验证码", sizeof(stMail.szEmailSubject));
			if(!strcmp(pf, "name")) {
				snprintf(stMail.szEmailContent, sizeof(stMail.szEmailContent), 
					"您修改账号名的验证码是：%s, 有效期 %d 分钟。", sVerifyCode, (int)(OP_CODE_VALID_TIME/60));
			}
			else if(!strcmp(pf, "email") && iHasEmail) {
				snprintf(stMail.szEmailContent, sizeof(stMail.szEmailContent), 
					"您正在修改电子邮箱，操作验证码是：%s, 有效期 %d 分钟。", sVerifyCode, (int)(OP_CODE_VALID_TIME/60));
			}
			else if(!strcmp(pf, "email") && !iHasEmail) {
				snprintf(stMail.szEmailContent, sizeof(stMail.szEmailContent), 
					"您正在绑定电子邮箱，操作验证码是：%s, 有效期 %d 分钟。", sVerifyCode, (int)(OP_CODE_VALID_TIME/60));
			}
			else if(!strcmp(pf, "pass") && iVerifyType == 2) {
				snprintf(stMail.szEmailContent, sizeof(stMail.szEmailContent),
					"您正在重置登陆密码，操作验证码是：%s, 有效期 %d 分钟。", sVerifyCode, (int)(OP_CODE_VALID_TIME/60));
			}
			else if(!strcmp(pf, "su_pass") && iVerifyType == 2) {
				snprintf(stMail.szEmailContent, sizeof(stMail.szEmailContent),
					"您正在重置用户：%s(%d) 的登陆密码，操作验证码是：%s, 有效期 %d 分钟。", 
					pModUserName, iModUserId, sVerifyCode, (int)(OP_CODE_VALID_TIME/60));
			}
			else {
				REQERR_LOG("unsupport modify:%s", pf);
				return SLOG_ERROR_LINE;
			}
			if(slog.AddMailToShm(stMail) < 0)
			{
				show_errpage(NULL, NULL, stConfig);
				return SLOG_ERROR_LINE;
			}
		}
		else {
			REQERR_LOG("unknow operator, para|%s|%d", pf, iVerifyType);
			return SLOG_ERROR_LINE;
		}

		// save session
		if(SetUserSessionInfo(stConfig.stUser.puser_info, sess) < 0)
		{
			stConfig.pErrMsg = CGI_ERR_SERVER;
			return SLOG_ERROR_LINE;
		}

		js["ec"] = 0;
		INFO_LOG("set op verify code :%s for:%s, seq:%u, pf:%s, to email:%s", 
			sVerifyCode, stConfig.remote, stMail.dwMailSeq, pf, stMail.szToEmailAddr);
	}

	STRING str;
	string_init(&str);
	if((stConfig.err=string_set(&str, js.ToString().c_str())) != STATUS_OK
		|| (stConfig.err=cgi_output(stConfig.cgi, &str)) != STATUS_OK)
	{
		string_clear(&str);
		return SLOG_ERROR_LINE;
	}
	string_clear(&str);
	DEBUG_LOG("send op vcode response:%s", js.ToString().c_str());
	return 0;
}

static int InitFastCgi_first(CGIConfig &myConf)
{
	if(InitFastCgiStart(myConf) < 0) {
		ERR_LOG("InitFastCgiStart failed !");
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

int main(int argc, char **argv, char **envp)
{
	int32_t iRet = 0;
	stConfig.argc = argc;
	stConfig.argv = argv;
	if((iRet=InitFastCgi_first(stConfig) < 0))
	{
		printf("InitCgi failed ! ret:%d", iRet);
		return -1;
	}

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

		// 
		iRet=AfterCgiRequestInit(stConfig);
		if(iRet == 0)
			continue;
		else if(iRet < 0)
			break;
		SetCgiResponseType(stConfig, s_JsonRequest);

		const char *pAction = stConfig.pAction;
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

		if(NULL == pAction)
		{
			REQERR_LOG("have no action from :%s", stConfig.remote);
			show_errpage(NULL, CGI_REQERR, stConfig);
			continue;
		}
		DEBUG_LOG("get action :%s from :%s", pAction, stConfig.remote);

		if(DealDbConnect(stConfig) < 0) {
			show_errpage(NULL, CGI_ERR_SERVER, stConfig);
			continue;
		}

		// 用户列表
		if(!strcmp(pAction, "list_user"))
			iRet = DealListUser();
		else if(!strcmp(pAction, "export_user"))
			;
		else if(!strcmp(pAction, "add_user"))
			iRet = DealAddUser();
		else if(!strcmp(pAction, "mod_user"))
			iRet = DealModUser();
		else if(!strcmp(pAction, "save_new_user"))
			iRet = SaveUser();
		else if(!strcmp(pAction, "save_mod_user"))
			iRet = SaveUser(true);
		else if(!strcmp(pAction, "delete_user"))
			iRet = DeleteUser();
		else if(!strcmp(pAction, "search_user"))
			iRet = DealUserSearch();
		else if(!strcmp(pAction, "detail_user"))
			iRet = DetailUser();

		// 用户登录列表
		else if(!strcmp(pAction, "login_info"))
			iRet = DealListLoginInfo();

		// 用户配置相关
		else if(!strcmp(pAction, "user_center"))
			iRet = DealUserCenter();	
		else if(!strcmp(pAction, "init_change_pwd"))
			;
		else if(!strcmp(pAction, "init_change_info"))
			iRet = DealInitChangeInfo();
		else if(!strcmp(pAction, "send_op_vcode"))
			iRet = DealSendOpVcode();
		else if(!strcmp(pAction, "su_reset_pass"))
			iRet = DealResetPass(true);
		else if(!strcmp(pAction, "reset_pass"))
			iRet = DealResetPass(false);
		else if(!strcmp(pAction, "change_uname"))
			iRet = DealChangeName();
		else if(!strcmp(pAction, "change_email"))
			iRet = DealChangeEmail();
		else if(!strcmp(pAction, "change_pwd"))
			iRet = DealChangePasswd();
		else if(!strcmp(pAction, "bind_xrkmonitor"))
			iRet = DealBindXrkmonitor();
		else if(!strcmp(pAction, "unbind_xrkmonitor"))
			iRet = DealUnBindXrkmonitor();
		else if(!strcmp(pAction, "test_xrkmonitor_sendwarn"))
			iRet = DealTestXrkmonitorWarn();
		else {
			iRet = -1;
			REQERR_LOG("unknow action:%s from:%s", pAction, stConfig.remote);
			stConfig.pErrMsg = CGI_REQERR;
		}

		// -------------------------------------------
		if(iRet < 0)
		{
			show_errpage(NULL, NULL, stConfig);
			continue;
		}

		const char *pcsTemplate = NULL;
		if(!strcmp(pAction, "list_user") || !strcmp(pAction, "search_user"))
			pcsTemplate = "dmt_user.html";
		else if(!strcmp(pAction, "add_user") || !strcmp(pAction, "mod_user"))
			pcsTemplate = "dmt_dlg_add_user.html";
		else if(!strcmp(pAction, "login_info"))
			pcsTemplate = "dmt_user_login_info.html";
		else if(!strcmp(pAction, "detail_user"))
			pcsTemplate = "dmt_dlg_user_detail.html";
		else if(!strcmp(pAction, "init_change_pwd"))
			pcsTemplate = "dmt_changepwd.html";
		else if(!strcmp(pAction, "init_change_info"))
			pcsTemplate = "dmt_change_info.html";
		else if(!strcmp(pAction, "user_center"))
			pcsTemplate = "dmt_user_center.html";
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

