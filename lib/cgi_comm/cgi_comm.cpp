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

   开发库 cgi_comm 说明:
        cgi/fcgi 的公共库，实现 cgi 的公共需求，比如登录态验证/cgi 初始化等

****/

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS 1
#endif

#include <errno.h>
#include <time.h>
#include <supper_log.h>
#include <assert.h>
#include "cgi_head.h"
#include "cgi_comm.h"
#include "sv_net.h"
#include "user.pb.h"
#include "mt_report.h"
#include "supper_log.h"

#define TOP_SEC_COOKIE "skdfj2!@#%^"
#define MYSIZEOF (unsigned)sizeof

// cgi 染色关键字类型
#define CGI_KEY_TYPE_USER 2 // 访问用户账号 id
#define CGI_KEY_TYPE_REMOTE 3 // 远程 ip 地址
#define CGI_KEY_TYPE_ACTION 4 // 请求 action 
#define CGI_KEY_TYPE_UNAME 5 // 访问用户账号

typedef struct {
	CGI *cgi;
	const char *remote;
	CgiReqUserInfo *preq_info;
}TCheckCgiLogTest;

// fast cgi 染色配置检查, 多个关键字设置时，关系为 与
// 注意：该函数在登录验证通过后调用
int CheckFastCgi_test(int iTestCfgCount, SLogTestKey *pstTestCfg, const void *pdata)
{
	// 没有任何关键字
	if(iTestCfgCount <= 0)
		return 0;

	const TCheckCgiLogTest *pcheck = (const TCheckCgiLogTest*)pdata;
	FloginInfo *pLogin = pcheck->preq_info->puser_info;
	for(int i=0; i < iTestCfgCount; i++) 
	{
		if(pstTestCfg[i].bKeyType == CGI_KEY_TYPE_USER) {
			if(pLogin->iUserId != atoi(pstTestCfg[i].szKeyValue))
				return 0;
			continue;
		}

		if(pstTestCfg[i].bKeyType == CGI_KEY_TYPE_REMOTE) {
			if(strcmp(pcheck->remote, pstTestCfg[i].szKeyValue))
				return 0;
			continue;
		}

		if(pstTestCfg[i].bKeyType == CGI_KEY_TYPE_ACTION)
		{
			const char *paction = hdf_get_value(pcheck->cgi->hdf, "Query.action", NULL);
			if(paction == NULL || strcmp(paction, pstTestCfg[i].szKeyValue))
				return 0;
			continue;
		}

		if(pstTestCfg[i].bKeyType == CGI_KEY_TYPE_UNAME) {
			if(strcmp(pLogin->szUserName, pstTestCfg[i].szKeyValue))
				return 0;
			continue;
		}
	}
	return 1;
}	

void WriteCgiErrorLog(NEOERR *err)
{
	STRING str;
	string_init(&str);
	nerr_error_traceback(err, &str);
	ERR_LOG("cgi error info:%s", str.buf);
	string_clear(&str);
}

int show_errpage(const char * cs_path, const char * err_msg, CGIConfig &stConfig)
{       
    NEOERR *err;
    CGI *cgi;
	if(stConfig.cgi == NULL)
	{
		err = cgi_init(&cgi, NULL);
		if (err != STATUS_OK) {
			WriteCgiErrorLog(err);
			return SLOG_ERROR_LINE; 
		}            
		stConfig.cgi = cgi;
	}
	else
		cgi = stConfig.cgi;

	if(stConfig.iEnableCgiDebug == DEBUG_MAGIC) {
		hdf_set_int_value(cgi->hdf, "config.debug", 1);
		if(stConfig.iDebugDumpHdf)
			hdf_set_int_value(cgi->hdf, "Config.DebugDumpHdf", 1);
	}

	if(stConfig.err != STATUS_OK)
		WriteCgiErrorLog(stConfig.err);

	// json 方式响应
	if(stConfig.iResponseType == RESPONSE_TYPE_JSON) {
		Json js;
		if(stConfig.pErrMsg != NULL)
			js["msg"] = stConfig.pErrMsg;
		else if(err_msg != NULL)
			js["msg"] = err_msg;
		else if(stConfig.pMsgId != NULL)
			js["msgid"] = stConfig.pMsgId;
		else 
			js["msg"] = CGI_ERR_SERVER;

		// 1 - 200 错误码由 js 公共函数处理
		// 301 - 400 错误码由各自业务 js 处理 
		if(stConfig.iErrorCode >= 1 && stConfig.iErrorCode < 400) 
		{
			js["statusCode"] = stConfig.iErrorCode;
			js["ec"] = stConfig.iErrorCode;
			js["ret"] = stConfig.iErrorCode;
		}
		else
		{
			js["statusCode"] = 300; // cgi 通用错误码
			js["ec"] = 300;
			js["ret"] = 300;
		}

		STRING str;
		string_init(&str);
		if((err=string_set(&str, js.ToString().c_str())) != STATUS_OK
			|| (err=cgi_output(cgi, &str)) != STATUS_OK) {
			string_clear(&str);
			WriteCgiErrorLog(err);
		}
		else
		{
			string_clear(&str);
			WARN_LOG("cgi error, response:%s", js.ToString().c_str());
		}
	}
	// html 方式响应
	else {
		if(stConfig.pErrMsg != NULL)
			err = hdf_set_value(cgi->hdf, "err.msg", stConfig.pErrMsg);
		else if(err_msg != NULL)
			err = hdf_set_value(cgi->hdf, "err.msg", err_msg);
		else if(stConfig.pMsgId != NULL)
			err = hdf_set_value(cgi->hdf, "err.msgid", stConfig.pMsgId);
		else {
			err = hdf_set_value(cgi->hdf, "err.msg", CGI_ERR_SERVER);
			INFO_LOG("set err.msg:%s", CGI_ERR_SERVER);
		}
		if (err != STATUS_OK)
			WriteCgiErrorLog(err);

		if(cs_path == NULL) {
			std::string strError(stConfig.szCsPath);
			strError += "error.html";
			err = cgi_display(cgi, strError.c_str());
			WARN_LOG("cgi comm error:%s, path:%s", strError.c_str(), stConfig.szCurPath);
		}
		else
		{
			err = cgi_display(cgi, cs_path);
			WARN_LOG("cgi cust error:%s, path:%s", cs_path, stConfig.szCurPath);
		}

		if (err != STATUS_OK) {
			cgi_neo_error(cgi, err);
			WriteCgiErrorLog(err);
		}
	}
    cgi_destroy(&stConfig.cgi);
    return 0;
}

static void SaveCgiRequest(CGIConfig &myCfg)
{
	char sBuf[256] = {0};
	char sCreateDirCmd[256] = {0};
	snprintf(sCreateDirCmd, MYSIZEOF(sCreateDirCmd), "mkdir -p %scgi_request", myCfg.szDebugPath); 
	system(sCreateDirCmd);

	snprintf(sBuf, MYSIZEOF(sBuf)-1, "%scgi_request/%s_req", myCfg.szDebugPath, myCfg.pszCgiName);
	int iRet = cgi_save_env_query_info(myCfg.cgi, sBuf);
	if(iRet < 0)
		ERR_LOG("save requre info to file:%s failed, ret:%d, msg:%s", sBuf, iRet, strerror(errno));
	else
		INFO_LOG("save requre info to file:%s success", sBuf);
}

void SaveCgiOutput(STRING & str)
{
	FILE *fp = fopen("/tmp/cgiout", "w+");
	if(fp != NULL)
	{
		fwrite(str.buf, 1, str.len, fp);
		fclose(fp);
	}
	else
	{
		ERR_LOG("SaveCgiOutput failed, msg:%s", strerror(errno));
	}
}

int GetRecordTotal(const char *szRecordDest, CGIConfig &stConfig)
{
	char sSql[1024];
	int iRet = 0;
	Query qu(*stConfig.db);

	if(!strcmp(szRecordDest, "config.customer_all"))
	{
		snprintf(sSql, MYSIZEOF(sSql), 
			"select count(*) from customer_user where sms_user_id=%u", stConfig.dwUserId);
		qu.get_result(sSql);
		if(qu.num_rows() > 0 && qu.fetch_row() != NULL)
		{
			stConfig.dwRecordTotal = qu.getval(0);
			DEBUG_LOG("get %s result:%u, for user:%u", szRecordDest, stConfig.dwRecordTotal, stConfig.dwUserId);
			iRet = 1;
		}
		else
		{
			stConfig.dwRecordTotal = 0;
			WARN_LOG("execute sql:%s failed !", sSql); 
			iRet = -1;
		}
		qu.free_result();
	}
	return iRet; 
}

int GetRecordCur(const char *szRecordDest, CGIConfig &stConfig)
{
	char sSql[1024];
	int iRet = 0;
	Query qu(*stConfig.db);

	if(!strcmp(szRecordDest, "config.customer_all"))
	{
		snprintf(sSql, MYSIZEOF(sSql), 
			"select user_id,top_user_nick,user_level,user_email,user_mobile,user_mobile_from_trade "
			" from customer_user,top_user where customer_user.sms_user_id=%u and "
			" customer_user.top_user_nick=top_user.user_nick", stConfig.dwUserId);
		qu.get_result(sSql);
		if(qu.num_rows() > 0)
		{
			DEBUG_LOG("execute:%s result:%d", sSql, qu.num_rows());
			int iRecord = 0;
			for(; qu.fetch_row(); iRecord++)
			{
				hdf_set_valuef(stConfig.cgi->hdf, "page.record.%d.user_id=%u", iRecord, qu.getuval("user_id"));
				hdf_set_valuef(stConfig.cgi->hdf, "page.record.%d.user_nick=%s", iRecord, qu.getstr("top_user_nick"));
				hdf_set_valuef(stConfig.cgi->hdf, "page.record.%d.user_level=%s", iRecord, qu.getstr("user_level"));
				hdf_set_valuef(stConfig.cgi->hdf, "page.record.%d.user_email=%s", iRecord, qu.getstr("user_email"));

				if(strcmp(qu.getstr("user_mobile"), "NULL") && strcmp(qu.getstr("user_mobile"), ""))
					hdf_set_valuef(stConfig.cgi->hdf, "page.record.%d.user_mobile=%s", 
						iRecord, qu.getstr("user_mobile"));
				else 
					hdf_set_valuef(stConfig.cgi->hdf, "page.record.%d.user_mobile=%s", 
						iRecord, qu.getstr("user_mobile_from_trade"));
				DEBUG_LOG("get record info:%u %s %s %s %s",
					qu.getuval("user_id"), qu.getstr("top_user_nick"), qu.getstr("user_level"), 
					qu.getstr("user_email"), hdf_get_valuef(stConfig.cgi->hdf, "page.record.%d.user_mobile", iRecord));
			}
		}
		else
		{
			WARN_LOG("execute sql:%s failed !", sSql); 
			iRet = -1;
		}
		qu.free_result();
	}
	return iRet; 
}

int SetPageInfo(CGIConfig &stConfig)
{
	if(stConfig.wPageSize == 0)
	{
		ERR_LOG("page size is 0, reset to 30");
		stConfig.wPageSize = 30;
	}
	hdf_set_valuef(stConfig.cgi->hdf, "Page.pageSize=%d", stConfig.wPageSize);

	if(stConfig.wPageCur == 0)
	{
		ERR_LOG("page cur is 0 reset to 1");
		stConfig.wPageCur = 1;
	}
	hdf_set_valuef(stConfig.cgi->hdf, "Page.page=%d", stConfig.wPageCur);
	hdf_set_valuef(stConfig.cgi->hdf, "Page.pageTotal=%d", 
		(stConfig.dwRecordTotal+stConfig.wPageSize) / stConfig.wPageSize);
	hdf_set_valuef(stConfig.cgi->hdf, "Page.recordTotal=%u", stConfig.dwRecordTotal);
	return 0;
}

int SetRecordsOrder(CGI *cgi, char *sApdSql, const char *pCheckF)
{
	// 单击排序字段时会带上
	const char *pReqF = hdf_get_value(cgi->hdf, "Query.orderField", NULL);
	const char *pReqOd = hdf_get_value(cgi->hdf, "Query.orderDirection", "asc");

	if(pReqF == NULL)
		return 0;

	if(strcmp(pReqF, pCheckF))
		return 0;

	if(strcmp(pReqOd, "asc") && strcmp(pReqOd, "desc"))
		return 0;

	strcat(sApdSql, " order by ");
	strcat(sApdSql, pReqF);

	// 注意cgi传送过来的参数是切换后的排序方式
	if(!strcmp(pReqOd, "desc"))
		strcat(sApdSql, " asc");
	else
		strcat(sApdSql, " desc");

	// 记忆历史，以便翻页
	hdf_set_value(cgi->hdf, "config.orderField", pReqF);
	hdf_set_value(cgi->hdf, "config.orderDirection", pReqOd);

	// 切换排序方式 升序 <-> 降序
	hdf_set_valuef(cgi->hdf, "config.order_%s=%s", pReqF, pReqOd);
	return 1;
}

int SetRecordsPageInfo(CGI *cgi, int iTotalRecords, int iDefNumPerPage, int iDefPageNumShown, int iDefCurrPage)
{
	int iCur = hdf_get_int_value(cgi->hdf, "Query.pageNum", iDefCurrPage);
	int iNumShown = hdf_get_int_value(cgi->hdf, "Query.pageNumShown", iDefPageNumShown);
	int iNumPerPage = hdf_get_int_value(cgi->hdf, "Query.numPerPage", iDefNumPerPage);

	hdf_set_int_value(cgi->hdf, "config.totalCount", iTotalRecords);

	if(iNumPerPage <= 0)
		iNumPerPage = 10;
	hdf_set_int_value(cgi->hdf, "config.numPerPage", iNumPerPage);
	int iTotalPages = iTotalRecords/iNumPerPage + (iTotalRecords%iNumPerPage ? 1: 0);

	if(iCur <= 0)
		iCur = 1;
	if(iCur > iTotalPages && iTotalPages > 0)
		iCur = iTotalPages;
	hdf_set_int_value(cgi->hdf, "config.currentPage", iCur);

	if(iNumShown > iTotalPages)
		iNumShown = iTotalPages;
	if(iNumShown <= 0)
		iNumShown = 1;
	hdf_set_int_value(cgi->hdf, "config.pageNumShown", iNumShown);

	DEBUG_LOG("records page info totalCount:%d, numPerPage:%d, currentPage:%d, pageNumShown:%d",
		iTotalRecords, iNumPerPage, iCur, iNumShown);
	return 0;
}

const char *GetCookieKey(uint32_t ttm, uint32_t dwUserId)
{
	char sCookieKeyMd5[MD5LEN];
	char sBuf[128]={0};
	snprintf(sBuf, MYSIZEOF(sBuf)-1, "useid=%u,time=%u,seccookie=%s",
		dwUserId, ttm, TOP_SEC_COOKIE);
	DEBUG_LOG("set or check cookie from info:%s", sBuf);
	Md5HashBuffer((unsigned char*)sCookieKeyMd5, (unsigned char*)sBuf, strlen(sBuf));
	return OI_DumpHex(sCookieKeyMd5, 0, MYSIZEOF(sCookieKeyMd5));
}

const char *GetCookieTime(uint32_t dwExpire)
{
	static char sCookieTime[64];

	time_t exp_date = 0;
	if(dwExpire > 0)
		exp_date = dwExpire;
	else
		exp_date = time(NULL) + COOKIE_TIMEOUT_SECOND;
	strftime(sCookieTime, 48, "%A, %d-%b-%Y 23:59:59 GMT", gmtime(&exp_date));
	return sCookieTime;
}

const char *GetCookieTime(CGIConfig &stConfig, uint32_t dwExpire)
{
	static char sCookieTime[64];
	if(stConfig.dwCurTime <= 0)
		stConfig.dwCurTime = time(NULL);
	time_t exp_date = 0;
	if(dwExpire > 0)
		exp_date = stConfig.dwCurTime + dwExpire;
	else
		exp_date = stConfig.dwCurTime + COOKIE_TIMEOUT_SECOND;
	strftime(sCookieTime, 48, "%A, %d-%b-%Y 23:59:59 GMT", gmtime(&exp_date));
	return sCookieTime;
}

void SetCookie(const char *name, const char *value, uint32_t dwExpireTime, CGIConfig &stConfig)
{
	const char *pCookieTime = GetCookieTime(stConfig, dwExpireTime);
	//char sBuf[32];
	cgi_cookie_set(stConfig.cgi, name, value, NULL, NULL, pCookieTime, 0, 0);
}

FloginInfo* SearchOnlineUserById(FloginList *pshmLoginList, int uid, int & index, int & count)
{
	int i=count, j=index;
	uint32_t dwCurTime = time(NULL);
	for(; i < pshmLoginList->iLoginCount && j < FLOGIN_SESSION_NODE_COUNT; j++)
	{
		if(pshmLoginList->stLoginList[j].bLoginType != 0
			&& pshmLoginList->stLoginList[j].dwLastAccessTime+
			pshmLoginList->stLoginList[j].iLoginExpireTime > dwCurTime){
			i++; 
			if(pshmLoginList->stLoginList[j].iUserId == uid) {
				count = i;
				index = j+1;
				return pshmLoginList->stLoginList + j;
			}
		}
	}
	return NULL;
}

void RedirectToFastLogin(CGIConfig &stConfig)
{
	// 日志系统可能需要跨站
	hdf_set_value(stConfig.cgi->hdf, "cgiout.other.cros", "Access-Control-Allow-Origin:*");
	if(stConfig.iResponseType == RESPONSE_TYPE_HTML)
	{
		cgi_redirect_uri(stConfig.cgi, stConfig.szRedirectUri);
		return;
	}


	Json js;
	js["ec"] = (unsigned int)111;
	js["redirect_url"] = stConfig.szRedirectUri;
	STRING str;
	string_init(&str);
	NEOERR *err = NULL;
	if((err=string_set(&str, js.ToString().c_str())) != STATUS_OK
	    || (err=cgi_output(stConfig.cgi, &str)) != STATUS_OK)
	{
		string_clear(&str);
		string_init(&str);
		nerr_error_traceback(err, &str);
		ERR_LOG("cgi_output failed ! msg:%s", str.buf);
	}
	string_clear(&str);
}

void SetFloginCookie(CGI *cgi, const char *puser, const char *pmd5, int32_t index, int32_t id, int32_t itype)
{
	// 以下cookie 永久有效(实际是1年)，使用服务器端的 session 验证有效性-- soo safe
	cgi_cookie_set(cgi, "flogin_user", puser, NULL, NULL, NULL, 1, 0);
	cgi_cookie_set(cgi, "flogin_md5", pmd5, NULL, NULL, NULL, 1, 0);
	cgi_cookie_set(cgi, "flogin_type", itoa(itype), NULL, NULL, NULL, 1, 0);
	cgi_cookie_set(cgi, "flogin_index", itoa(index), NULL, NULL, NULL, 1, 0);
	cgi_cookie_set(cgi, "flogin_uid", itoa(id), NULL, NULL, NULL, 1, 0);
	INFO_LOG("set flogin cookie: user:%s, md5:%s, index:%d, uid:%d", puser, pmd5, index, id);
}

// 不支持多进程 single process
int32_t GetFloginFree(FloginList *pshmLoginList, uint32_t dwCurTime)
{
    int i, j;
    for(i=0; i < FLOGIN_SESSION_NODE_COUNT; i++)
    {   
        j = pshmLoginList->iWriteIndex;
        if(j+1 >= FLOGIN_SESSION_NODE_COUNT)
            pshmLoginList->iWriteIndex = 0;
        else 
            pshmLoginList->iWriteIndex = j+1;

        if(pshmLoginList->stLoginList[j].bLoginType == 0
            || dwCurTime > pshmLoginList->stLoginList[j].dwLastAccessTime
                + pshmLoginList->stLoginList[j].iLoginExpireTime
            || dwCurTime > pshmLoginList->stLoginList[j].dwLoginTime+LOGIN_MAX_EXPIRE_TIME)
        {   
            if(pshmLoginList->stLoginList[j].bLoginType != 0)
            {   
                INFO_LOG("user:%s login session timeout, login addr:%s, login at:%s, last access:%s", 
                    pshmLoginList->stLoginList[j].szUserName,
                    ipv4_addr_str(pshmLoginList->stLoginList[j].dwLoginIP),
                    uitodate(pshmLoginList->stLoginList[j].dwLoginTime),
                    uitodate(pshmLoginList->stLoginList[j].dwLastAccessTime));
                pshmLoginList->iLoginCount--;
            }   
            memset(pshmLoginList->stLoginList+j, 0, sizeof(FloginInfo));
            return j;
        }   
    }   
    return -1; 
}

int CheckLoginEx_Local(FloginInfo & s_sess, CGIConfig & stConfig,
	CGI *cgi, FloginList *pshmLoginList, const char *remote, uint32_t dwCurTime, CgiReqUserInfo *pinfo) 
{
	int32_t iUserId = hdf_get_int_value(cgi->hdf, "Query.local_flogin_uid", -1);
	if(iUserId < 0) {
		REQERR_LOG("invalid request parameter, uid:%d", iUserId);
		return SLOG_ERROR_LINE;
	}

	if(dwCurTime == 0)
		dwCurTime = time(NULL);

	static char s_UserName[64] = {0};
	char sBuf[256];
	snprintf(sBuf, sizeof(sBuf), "select * from flogin_user where user_id=%d", iUserId);
	if(stConfig.qu->get_result(sBuf) && stConfig.qu->fetch_row())
	{
		strncpy(s_UserName, stConfig.qu->getstr("user_name"), sizeof(s_UserName)-1);
		pinfo->puser = s_UserName;
		pinfo->iLoginType = stConfig.qu->getval("login_type");

		INFO_LOG("user login from local server, uid:%d, ip:%s", iUserId, remote);
		FloginInfo *psess = &s_sess;
		psess->bLoginType = pinfo->iLoginType;
		strncpy(psess->szUserName, pinfo->puser, sizeof(psess->szUserName)-1);
		strncpy(psess->szPassMd5, "fromlocal_req", sizeof(psess->szPassMd5)-1);
		psess->dwLoginIP = inet_addr(stConfig.remote);
		psess->dwLoginTime = dwCurTime;
		psess->iLoginExpireTime = 7*24*60*60;
		psess->dwUserFlag_1 = stConfig.qu->getuval("user_flag_1");
		psess->iUserId = iUserId;
		pinfo->iLoginIndex = 0;
		pinfo->puser_info = psess;
		psess->dwLastAccessIP = inet_addr(remote);
		psess->dwLastAccessTime = dwCurTime;
		stConfig.qu->free_result();
	}
	else {
		ERR_LOG("execute sql: %s failed !", sBuf);
		stConfig.qu->free_result();
		return -1;
	}
	return 1;
}

int CheckLoginEx(CGIConfig & stConfig,
	CGI *cgi, FloginList *pshmLoginList, const char *remote, uint32_t dwCurTime, CgiReqUserInfo *pinfo) 
{
	// 供 cgi 业务逻辑取相关数据用
	static FloginInfo s_sess;

	// 看是否是监控系统服务器发过来的请求
	if(slog.GetMachineInfoByIp((char*)remote) != NULL) {
		MtReport_Attr_Add(500, 1);
		DEBUG_LOG("get cgi request from server:%s, action:%s", remote, stConfig.pAction);
		return CheckLoginEx_Local(s_sess, stConfig, cgi, pshmLoginList, remote, dwCurTime, pinfo);
	}

	const char *puser = hdf_get_value(cgi->hdf, "Query.ex_flogin_user", NULL);
	const char *pmd5 = hdf_get_value(cgi->hdf, "Query.ex_flogin_md5", NULL);
	int32_t iLoginType = hdf_get_int_value(cgi->hdf, "Query.ex_flogin_type", 0);
	int32_t iLoginIndex = hdf_get_int_value(cgi->hdf, "Query.ex_flogin_index", -1);
	int32_t iUserId = hdf_get_int_value(cgi->hdf, "Query.ex_flogin_uid", -1);

	if(NULL == puser || NULL == pmd5 || NULL == pinfo 
		|| iLoginIndex < 0 || iUserId < 0 || puser[0] == '\0' || CheckDbString(puser))
	{
		DEBUG_LOG("use ex -- failed: user:%p md5:%p login type:%d login index:%d uid:%d",
			puser, pmd5, iLoginType, iLoginIndex, iUserId);
		return 0;
	}

	pinfo->puser = puser;
	pinfo->iLoginType = iLoginType;

	if(iLoginIndex >= FLOGIN_SESSION_NODE_COUNT)
	{
		REQERR_LOG("invalid login index :%d > %d", iLoginIndex, FLOGIN_SESSION_NODE_COUNT);
		return -1;
	}

	if(dwCurTime == 0)
		dwCurTime = time(NULL);

	// 用户名合法性检查
	if(!IsUserNameValid(puser))
	{
		WARN_LOG("invalid user name:%s", puser);
		return SLOG_ERROR_LINE;
	}

	char sBuf[256];
	snprintf(sBuf, sizeof(sBuf),
	    "select * from flogin_user where user_name=\'%s\'", puser);
	if(stConfig.qu->get_result(sBuf) && stConfig.qu->fetch_row())
	{
		// 正常 login 
	    const char *ppass_md5 = stConfig.qu->getstr("login_md5");
		int32_t iLoginType_db = stConfig.qu->getval("login_type");
		int32_t iUserId_db = stConfig.qu->getval("user_id");
		int32_t iLoginIndex_db = stConfig.qu->getval("login_index");
		const char *plogin_ip = stConfig.qu->getstr("last_login_address");
		uint32_t dwLoginTime = stConfig.qu->getuval("last_login_time");

		// su 切换 login
		uint32_t dwLoginTime_su = stConfig.qu->getuval("dwReserved1");
		int32_t iLoginIndex_db_su = stConfig.qu->getval("dwReserved2");
	    const char *ppass_md5_su = stConfig.qu->getstr("strReserved2");

		// 验证登录 
	    if(iLoginType_db == iLoginType && iUserId_db == iUserId &&
			((dwLoginTime+LOGIN_MAX_EXPIRE_TIME > dwCurTime 
			  	&& iLoginIndex_db == iLoginIndex && !strcmp(pmd5, ppass_md5)) ||
			 (dwLoginTime_su+LOGIN_MAX_EXPIRE_TIME > dwCurTime
			  	&& iLoginIndex_db_su == iLoginIndex && !strcmp(pmd5, ppass_md5_su))))
		{
			INFO_LOG("check user login_ex by input ok, su:%u index:%d, md5:%s user name:%s login type:%d, uid:%d,"
				"ip:%s|%s", dwLoginTime_su, iLoginIndex, pmd5, puser, iLoginType_db, iUserId_db, plogin_ip, remote);
			FloginInfo *psess = &s_sess;
			psess->bLoginType = iLoginType;
			strncpy(psess->szUserName, puser, sizeof(psess->szUserName)-1);
			strncpy(psess->szPassMd5, pmd5, sizeof(psess->szPassMd5)-1);
			psess->dwLoginIP = inet_addr(stConfig.remote);
			psess->dwLoginTime = dwCurTime;
			psess->dwLastAccessTime = dwCurTime;
			psess->dwLastAccessIP = inet_addr(stConfig.remote);
			psess->iLoginExpireTime = 7*24*60*60;
			psess->dwUserFlag_1 = stConfig.qu->getuval("user_flag_1");
			psess->iUserId = iUserId_db;
			pinfo->iLoginIndex = iLoginIndex;
			pinfo->puser_info = psess;
			stConfig.qu->free_result();

		}
		else 
		{
			REQERR_LOG("check login by ex failed - su:%u|%u|%u, index:%d-%d-%d, "
				"md5:%s-%s-%s, logintype:%d-%d, uid:%d-%d",
				dwCurTime, dwLoginTime, dwLoginTime_su, iLoginIndex, iLoginIndex_db, iLoginIndex_db_su,
				pmd5, ppass_md5, ppass_md5_su, iLoginType, iLoginType_db, iUserId, iUserId_db);
			stConfig.qu->free_result();
			return -2;
		}
	}
	else {
		ERR_LOG("execute sql: %s failed !", sBuf);
		stConfig.qu->free_result();
		return -1;
	}
	return 1;
}

void SetCgiRequestInfoToCustField(CgiReqUserInfo *pinfo)
{
	slog.SetCust_2(pinfo->puser_info->bLoginType);
	slog.SetCust_3(pinfo->puser_info->iUserId);
	slog.SetCust_5(ipv4_addr_str(pinfo->puser_info->dwLoginIP));
	slog.SetCust_6(pinfo->puser_info->szUserName);
}

int GetUserSessionInfo(FloginInfo *psess, user::UserSessionInfo & user)
{
    int *piLen = (int*)psess->sReserved;
    char *pbuf = (char*)(psess->sReserved+4);

    user.Clear();
    if(*piLen > 0 && !user.ParseFromArray(pbuf, *piLen))
    {    
        ERR_LOG("get user session failed, len:%d", *piLen);
        MtReport_Attr_Add(227, 1);
        return SLOG_ERROR_LINE;
    }    
    DEBUG_LOG("get user sess ok, sess:%p, len:%d, info:%s", psess, *piLen, user.ShortDebugString().c_str());
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

    if(*piLen > (int)sizeof(psess->sReserved)-256) {
        WARN_LOG("user session info need more buf - %d > %d", *piLen, (int)sizeof(psess->sReserved)-256);
    }
    return 0;
}

int CheckLogin(CGI *cgi, FloginList *pshmLoginList, 
	const char *remote, uint32_t dwCurTime, CgiReqUserInfo *pinfo) 
{
	const char *puser = hdf_get_value(cgi->hdf, "Cookie.flogin_user", NULL); 
	const char *pmd5 = hdf_get_value(cgi->hdf, "Cookie.flogin_md5", NULL);
	int32_t iLoginType = hdf_get_int_value(cgi->hdf, "Cookie.flogin_type", 0);
	int32_t iLoginIndex = hdf_get_int_value(cgi->hdf, "Cookie.flogin_index", -1);
	int32_t iUserId = hdf_get_int_value(cgi->hdf, "Cookie.flogin_uid", -1);

	if(NULL == puser || NULL == pmd5 || NULL == pinfo || iLoginIndex < 0 || iUserId < 0)
	{
		DEBUG_LOG("cookie info: user:%p md5:%p login type:%d login index:%d uid:%d",
			puser, pmd5, iLoginType, iLoginIndex, iUserId);
		return 0;
	}

	pinfo->puser = puser;
	pinfo->iLoginType = iLoginType;

	if(iLoginIndex >= FLOGIN_SESSION_NODE_COUNT)
	{
		REQERR_LOG("invalid login index :%d > %d", iLoginIndex, FLOGIN_SESSION_NODE_COUNT);
		return -1;
	}

	if(dwCurTime == 0)
		dwCurTime = time(NULL);

	FloginInfo *psess = pshmLoginList->stLoginList + iLoginIndex;
	psess->dwLastAccessIP = inet_addr(remote);
	if(iLoginType != psess->bLoginType 
		|| iUserId != psess->iUserId
		|| dwCurTime > psess->dwLastAccessTime+psess->iLoginExpireTime
		|| dwCurTime > psess->dwLoginTime+LOGIN_MAX_EXPIRE_TIME
		|| strcmp(puser, psess->szUserName) || strcmp(pmd5, psess->szPassMd5))
	{
		REQERR_LOG("cookie check failed ! -- type:%d(%d), uid:%d time:%u(%u), user:%s(%s), md5:%s(%s), "
			", ip:%u(%u) max expire:%d, login index:%d",
			iLoginType, psess->bLoginType, iUserId, psess->dwLastAccessTime+psess->iLoginExpireTime,
			dwCurTime, puser, psess->szUserName, pmd5, psess->szPassMd5, 
			psess->dwLoginIP, psess->dwLastAccessIP, LOGIN_MAX_EXPIRE_TIME, iLoginIndex);
		return -1;
	}

	pinfo->iLoginIndex = iLoginIndex;
	pinfo->puser_info = psess;
	psess->dwLastAccessTime = dwCurTime;
	GetUserSessionInfo(psess, pinfo->pbSess);
	SetCgiRequestInfoToCustField(pinfo);

	if(0 == hdf_get_int_value(cgi->hdf, "config.test_log", 0))
	{
		TCheckCgiLogTest stCheckTest;
		stCheckTest.cgi = cgi;
		stCheckTest.remote = remote;
		stCheckTest.preq_info = pinfo;
		slog.CheckTest(CheckFastCgi_test, &stCheckTest);
	}

	hdf_set_value(cgi->hdf, "comm.user_name", puser);
	hdf_set_int_value(cgi->hdf, "comm.user_type", iLoginType);
	hdf_set_int_value(cgi->hdf, "comm.user_id", iUserId);
	hdf_set_value(cgi->hdf, "comm.user_ip", remote);

	DEBUG_LOG("cookie check login ok -- user:%s, update cookie time, expire in:%u", 
		puser, psess->dwLastAccessTime+psess->iLoginExpireTime);
	return 1;
}

int DealDbConnect(CGIConfig &stCfg)
{
	if(stCfg.db != NULL && stCfg.qu != NULL) {
		if(stCfg.qu->Connected())
			return 0;
		delete stCfg.db;
		stCfg.db = NULL;
		delete stCfg.qu;
		stCfg.qu = NULL;
	}

	stCfg.db = new Database(
		stCfg.pShmConfig->stSysCfg.szDbHost, stCfg.pShmConfig->stSysCfg.szUserName, 
		stCfg.pShmConfig->stSysCfg.szPass, stCfg.pShmConfig->stSysCfg.szDbName, &slog,
		stCfg.pShmConfig->stSysCfg.iDbPort);
	stCfg.qu = new Query(*stCfg.db);
	if(stCfg.db && stCfg.qu && stCfg.qu->Connected()) {
		INFO_LOG("connect to db:%s ok dbname:%s:%d", 
			stCfg.pShmConfig->stSysCfg.szDbHost, stCfg.pShmConfig->stSysCfg.szDbName,
			stCfg.pShmConfig->stSysCfg.iDbPort);
		return 0;
	}
	ERR_LOG("connect to database failed ! dbhost:%s, dbname:%s:%d", 
		stCfg.pShmConfig->stSysCfg.szDbHost, stCfg.pShmConfig->stSysCfg.szDbName,
		stCfg.pShmConfig->stSysCfg.iDbPort);

	delete stCfg.db;
	stCfg.db = NULL;
	delete stCfg.qu;
	stCfg.qu = NULL;

	return SLOG_ERROR_LINE;
}

int DealCgiHeart(CGI *cgi, int iStatusCode)
{
	Json js;
	js["statusCode"] = 0;
	js["processId"] = getpid();
	STRING str;
	string_init(&str);
	NEOERR *err = NULL;
	if((err=string_set(&str, js.ToString().c_str())) != STATUS_OK
		|| (err=cgi_output(cgi, &str)) != STATUS_OK)
	{
		iStatusCode = 10001;
	}
	string_clear(&str);
	DEBUG_LOG("cgi heart status:%d", iStatusCode);
	return iStatusCode;
}

void SetCgiResponseErrInfo(CGIConfig &stConfig, const char *strErrMsg, int iErrorCode)
{
	stConfig.bSetError = true;
	stConfig.pErrMsg = strErrMsg;
	stConfig.iErrorCode = iErrorCode;
}

char *strmov(char *dst,const char *src)
{
	strcpy(dst, src);
	return dst+strlen(src);
}

static NEOERR *render_cb (void *ctx, char *buf)
{
  	STRING *str = (STRING *)ctx;
  	NEOERR *err;

  	err = nerr_pass(string_append(str, buf));
  	return err;
}

NEOERR *my_cgi_output_local(CGI *cgi, const char *cs_file)
{
  	NEOERR *err = STATUS_OK;
  	CSPARSE *cs = NULL;
  	STRING str;

  	string_init(&str);
  	do
  	{
    		err = cs_init (&cs, cgi->hdf);
    		if (err != STATUS_OK) break;
    		err = cgi_register_strfuncs(cs);
    		if (err != STATUS_OK) break;
    		err = cs_parse_file (cs, cs_file);
			if (err != STATUS_OK) break;
      		cgiwrap_writef("Content-Type: text/plain\n\n");
      		hdf_dump_str(cgi->hdf, "", 0, &str);
      		cs_dump(cs, &str, render_cb);
      		cgiwrap_writef("%s", str.buf);
  	} while (0);

  	cs_destroy(&cs);
  	string_clear (&str);
  	return nerr_pass(err);
}


NEOERR *my_cgi_display (CGI *cgi, const char *cs_file, bool bSaveOut)
{
  	NEOERR *err = STATUS_OK;
  	CSPARSE *cs = NULL;
  	STRING str;

  	string_init(&str);
  	do
  	{
    		err = cs_init (&cs, cgi->hdf);
    		if (err != STATUS_OK) break;
    		err = cgi_register_strfuncs(cs);
    		if (err != STATUS_OK) break;
    		err = cs_parse_file (cs, cs_file);
			if (err != STATUS_OK) break;
			err = cs_render (cs, &str, render_cb);
			if (err != STATUS_OK) break;
			err = cgi_output(cgi, &str);
    		if (err != STATUS_OK) break;
  	} while (0);

	if(bSaveOut)
		SaveCgiOutput(str);
  	cs_destroy(&cs);
  	string_clear (&str);
  	return nerr_pass(err);
}

int my_cgi_file_download (const char *file, const char *fileName)
{
	char tempBuf[1024];
	FILE *outFile = fopen(file, "r");	
	if(outFile == NULL)
	{
		return -1;
	}
	char *type = (char*)strstr(fileName, ".");
	int read_len;
	type++;
	
	cgiwrap_writef("Content-Type:application/%s\n", type);
	cgiwrap_writef("Content-Disposition:attachment;filename=\"%s\"\n\n", fileName);
	while((read_len = fread(tempBuf, 1, 1024, outFile)) > 0)
	{
		cgiwrap_write(tempBuf, read_len); 
	}
	
	fclose(outFile);
  	return 0;
}

int my_cgi_excel_output (const char *fileName, const char *buff)
{
	const char *type = strstr(fileName, ".");
	type++;
	
	cgiwrap_writef("Content-Type:application/%s\n", type);
	cgiwrap_writef("Content-Disposition:attachment;filename=\"%s\"\n\n", fileName);
	cgiwrap_write(buff, strlen(buff)); 
	
  	return 0;
}

char *get_value(CGI *cgi, char *para)
{
	char *value = hdf_get_value(cgi->hdf,para,NULL);
	if(value != NULL && strlen(value) == 0)
		value = NULL;
	return value;
}

void InitCgiDebug(CGIConfig &myCfg)
{
	static uint32_t s_FileSeq = 1;

	char sBuf[256] = {0};
	char szDateTime[64] = {0};
	char sCreateDirCmd[256] = {0};
	snprintf(sCreateDirCmd, MYSIZEOF(sCreateDirCmd), "mkdir -p %s%s", myCfg.szDebugPath, myCfg.pszCgiName);
	system(sCreateDirCmd);

	struct tm stTm;
	time_t tmnew = time(NULL);
	localtime_r(&tmnew, &stTm);
	strftime(szDateTime, MYSIZEOF(szDateTime)-1, "_%Y_%m_%d_%H_%M_%S", &stTm);

	snprintf(sBuf, MYSIZEOF(sBuf)-1, "%s%s/req%s_%d_%d", myCfg.szDebugPath, myCfg.pszCgiName, szDateTime, getpid(), s_FileSeq);
	INFO_LOG("debug inner file:%s", sBuf);
	int iRet = cgi_save_env_query_info(myCfg.cgi, sBuf);
	if(iRet < 0)
		ERR_LOG("save requre info to file:%s failed, cmd:%s, ret:%d, msg:%s", 
			sBuf, sCreateDirCmd, iRet, strerror(errno));
	else
		INFO_LOG("save requre info to file:%s success", sBuf);

	snprintf(sBuf, MYSIZEOF(sBuf)-1, "%s%s/output%s_%d_%d", myCfg.szDebugPath, myCfg.pszCgiName, szDateTime, getpid(), s_FileSeq);
	hdf_set_value(myCfg.cgi->hdf, "Config.DebugOutputFile", sBuf);
	hdf_set_int_value(myCfg.cgi->hdf, "Config.DebugEnabled", 1);
	s_FileSeq++;
	INFO_LOG("debug output file:%s", sBuf);
}

int InitFastCgiStart(CGIConfig &myConf)
{
	int32_t iShmKey = 0;
	char *ptmp = NULL;
	myConf.pszCgiName = strdup(myConf.argv[0]);
	if((ptmp=strrchr(myConf.argv[0], '/')) != NULL)
		strcpy(myConf.pszCgiName, ptmp+1);
	snprintf(myConf.szConfigFile, MYSIZEOF(myConf.szConfigFile)-1, "%s.conf", myConf.pszCgiName);

	if(myConf.argc <= 1)
		slog.SetLogToStd(false);

	if(LoadConfig(myConf.szConfigFile,
	   "SLOG_SET_TEST", CFG_INT, &myConf.iCfgTestLog, 0,
	   "LOCAL_IF_NAME", CFG_STRING, myConf.szLocalIp, "eth0", MYSIZEOF(myConf.szLocalIp),
	   "DEBUG_LOG_FILE", CFG_STRING, myConf.szDebugPath, CGI_COREDUMP_DEBUG_OUTPUT_PATH, MYSIZEOF(myConf.szDebugPath),
	   "FAST_CGI_MAX_HITS", CFG_INT, &myConf.dwMaxHits, FAST_CGI_DEF_HITS_MAX,
	   "FAST_CGI_RUN_MAX_HOURS", CFG_INT, &myConf.dwExitTime, FAST_CGI_DEF_RUN_MAX_TIME_HOURS,
	   "FLOGIN_SHM_KEY", CFG_INT, &iShmKey, FLOGIN_SESSION_HASH_SHM_KEY,
	   "REDIRECT_URI", CFG_STRING, myConf.szRedirectUri, 
	   		"/cgi-bin/slog_flogin?action=redirect_main", MYSIZEOF(myConf.szRedirectUri),
	   "CGI_PATH", CFG_STRING, myConf.szCgiPath, CGI_PATH, MYSIZEOF(myConf.szCgiPath),
	   "CS_PATH", CFG_STRING, myConf.szCsPath, CS_PATH, MYSIZEOF(myConf.szCsPath),
	   "DOC_PATH", CFG_STRING, myConf.szDocPath, DOC_PATH, MYSIZEOF(myConf.szDocPath),
	   "ENABLE_CGI_DEBUG", CFG_INT, &myConf.iEnableCgiDebug, 0,
	   "DEBUG_DUMP_HDF", CFG_INT, &myConf.iDebugDumpHdf, 0,
	   "ENABLE_CGI_PAUSE", CFG_INT, &myConf.iEnableCgiPause, 0,
	   "DISABLE_VMEM_CACHE", CFG_INT, &myConf.iDisableVmemCache, 0,
	   "DELETE_RECORD_STATUS", CFG_INT, &myConf.iDeleteStatus, 1,
	   "XRKMONITOR_URL", CFG_STRING, myConf.szXrkmonitorSiteAddr, "http://xrkmonitor.com", MYSIZEOF(myConf.szXrkmonitorSiteAddr),
	   "SLOW_CGI_TIME_MS", CFG_INT, &myConf.iCgiSlowRunMs, 100,
		NULL) < 0){
		ERR_LOG("loadconfig failed, from file:%s", myConf.szConfigFile);
		return SLOG_ERROR_LINE;
	}

	myConf.dwStart = time(NULL);
	myConf.dwCurTime = myConf.dwStart;
	myConf.pid = getpid();
	myConf.dwExitTime = myConf.dwCurTime+myConf.dwExitTime*60*60 + rand()%60;

	if(INADDR_NONE == inet_addr(myConf.szLocalIp))
	{
		if(GetIpFromIf(myConf.szLocalIp, myConf.szLocalIp) != 0) {
			ERR_LOG("get local ip failed !");
			return SLOG_ERROR_LINE;
		}
	}

	int iRet = 0;
	if((iRet=GetShm2((void**)(&myConf.pshmLoginList), iShmKey, MYSIZEOF(FloginList), 0666|IPC_CREAT)) < 0)
	{
		ERR_LOG("attach shm FloginList failed, size:%u, key:%d", MYSIZEOF(FloginList), iShmKey);
		return SLOG_ERROR_LINE;
	}

	INFO_LOG("attach shm FloginList ok size:%u, key:%d, ret:%d, testlog:%d",
		MYSIZEOF(FloginList), iShmKey, iRet, myConf.iCfgTestLog);
	INFO_LOG("fcgi - %s start at:%u pid:%u will exist at:%u(curis:%u) coredump file:%s debug:%d local:%s",
		myConf.pszCgiName, myConf.dwStart, myConf.pid, myConf.dwExitTime, myConf.dwCurTime, 
		myConf.szCoredumpFile, myConf.iEnableCgiDebug, myConf.szLocalIp);

	char szCurDir[256];
	if(NULL == getcwd(szCurDir, 256)) 
		WARN_LOG("getcwd failed, msg:%s", strerror(errno));
	else
		strncpy(myConf.szCurPath, szCurDir, sizeof(myConf.szCurPath));

	return 0;
}

int InitFastCgi(CGIConfig &myCfg, const char *pszLogPath)
{
	myCfg.cgi = NULL;
	myCfg.err = NULL;
	myCfg.dwHits++;

	if(myCfg.argc <= 1)
		slog.SetLogToStd(false);

	if(myCfg.argc > 1)
	{
		myCfg.dwHits = myCfg.dwMaxHits;
		cgi_debug_init(myCfg.argc, myCfg.argv);
	}

	HDF *hdf = NULL;
	myCfg.err = hdf_init (&hdf);
	if(myCfg.err != STATUS_OK)  {
		FATAL_LOG("cgi hdf_init failed !");
		WriteCgiErrorLog(myCfg.err);
		return SLOG_ERROR_LINE;
	}
	if(myCfg.iEnableCgiDebug == DEBUG_MAGIC) {
		hdf_set_int_value(hdf, "Config.DebugEnabled", 1);
		if(myCfg.iDebugDumpHdf)
			hdf_set_int_value(hdf, "Config.DebugDumpHdf", 1);
	}

	if(myCfg.cgi != NULL) {
		cgi_destroy(&myCfg.cgi);
		WARN_LOG("cgi is not null !");
	}

	if((myCfg.err=cgi_init(&myCfg.cgi, hdf)) != STATUS_OK) {
		FATAL_LOG("cgi_init failed !");
		WriteCgiErrorLog(myCfg.err);
		return -1;
	}
	if((myCfg.err=cgi_parse(myCfg.cgi)) != STATUS_OK) {
		FATAL_LOG("cgi_parse failed !");
		WriteCgiErrorLog(myCfg.err);
		return -2;
	}

	myCfg.remote = hdf_get_value(myCfg.cgi->hdf, "CGI.RemoteAddress", NULL); 

	// 关闭运行时间的统计输出 (json 格式响应需要)
	hdf_set_int_value(myCfg.cgi->hdf, "Config.TimeFooter", 0);
	hdf_set_value(myCfg.cgi->hdf, "config.dwzpath", DWZ_PATH); 
	hdf_set_value(myCfg.cgi->hdf, "config.cgipath", myCfg.szCgiPath); 
	hdf_set_value(myCfg.cgi->hdf, "config.cspath", myCfg.szCsPath); 
	hdf_set_value(myCfg.cgi->hdf, "config.docpath", myCfg.szDocPath); 
	if(myCfg.iEnableCgiDebug == DEBUG_MAGIC)
	{
		InitCgiDebug(myCfg);
		SaveCgiRequest(myCfg);
		if(myCfg.iDebugDumpHdf)
			hdf_set_int_value(myCfg.cgi->hdf, "Config.DebugDumpHdf", 1);
	}

	if(myCfg.iEnableCgiPause == DEBUG_PAUSE_MAGIC)
		sleep(15);
	return 0;
}

void DealCgiFailedExit(CGI *cgi, NEOERR *err)
{
	if(NULL == cgi->hdf)
		return;

	const char *pfile = hdf_get_value(cgi->hdf, "Config.DebugInnerFile", NULL);
	if(NULL == pfile)
	{
		hdf_set_value(cgi->hdf,"err.msg", CGI_REQERR);
		cgi_display(cgi, PAGE_ERROR);
		return;
	}

	cgi_save_env_query_info(cgi, pfile);
	if(err != NULL && err != STATUS_OK)
	{
		FILE *fp = fopen(pfile, "w+");
		if(fp != NULL)
		{
			STRING str;
			string_init(&str);
			nerr_error_traceback(err, &str);
			fprintf(fp, "\n\n%s\n", str.buf);
			string_clear(&str);
			fclose(fp);
		}
	}
	hdf_set_value(cgi->hdf,"err.msg", CGI_REQERR);
	hdf_set_value(cgi->hdf, "err.file", pfile);
	cgi_display(cgi, PAGE_ERROR);
}

void DealCgiCoredump(int sig, const char *pszFile, void *pdata) 
{
	CGIConfig *pcfg = (CGIConfig*)pdata;
	if(pszFile != NULL && pcfg != NULL && pcfg->cgi != NULL)
		cgi_save_env_query_info(pcfg->cgi, pszFile);
}

int FcgiCheckTest(uint8_t bKeyType, const char *pkey, const void *pdata)
{
	// key type 255 , key 1 表示所有的都染色
	if(bKeyType == 255 && !strcmp(pkey, "1"))
		return 1;
	return 0;
}

void SetCgiResponseType(CGIConfig &stConfig, const char *s_JsonRequest[])
{
	const char *pRespType = hdf_get_value(stConfig.cgi->hdf, "Query.resp_type", NULL);
	if(NULL == pRespType || strcmp(pRespType, "json"))
		stConfig.iResponseType = RESPONSE_TYPE_HTML;
	else if(!strcmp(pRespType, "json"))
		stConfig.iResponseType = RESPONSE_TYPE_JSON;
	if(stConfig.pAction == NULL)
	{
		return;
	}

	for(int i=0; s_JsonRequest[i] != NULL; i++)
	{
		if(!strcmp(stConfig.pAction, s_JsonRequest[i]))
		{
			stConfig.iResponseType = RESPONSE_TYPE_JSON;
			return;
		}
	}
}

// 递归查找 attr type
int FindAttrTypeByTree(MmapUserAttrTypeTree & stTypeTree, int iAttrType)
{
	if(iAttrType == stTypeTree.attr_type_id())
		return 1;

	for(int i=0; i < stTypeTree.sub_type_list_size(); i++)
	{
		MmapUserAttrTypeTree *pType = stTypeTree.mutable_sub_type_list(i);
		if(FindAttrTypeByTree(*pType, iAttrType) > 0)
			return 1;
	}
	return 0;
}


// 获取用户的上报机器列表
int GetUserMachineListFromVmem(Json &js)
{
	MtSystemConfig *pum = slog.GetSystemCfg();
	if(pum == NULL) {
		ERR_LOG("GetSystemCfg failed!");
		return -11;
	}

	if(slog.InitMachineList() < 0)
	{
		WARN_LOG("InitMachineList failed");
		return SLOG_ERROR_LINE;
	}

	if(pum->wMachineCount <= 0)
	{
		js["mach_count"] = 0;
		return 0;
	}

	const char *pname = NULL;
	int idx = pum->iMachineListIndexStart;
	MachineInfo *pmach = NULL;
	int i = 0;
	for(i=0; i < pum->wMachineCount; i++)
	{
		Json mach;
		pmach = slog.GetMachineInfo(idx);
		if(pmach == NULL) {
			WARN_LOG("get machine info failed, index:%d", idx);
			break;
		}
		mach["id"] = pmach->id;
		pname = MtReport_GetFromVmem_Local(pmach->iNameVmemIdx);
		if(pname != NULL)
			mach["name"] = pname;
		else {
			mach["name"] = "unknow";
			WARN_LOG("get machine name failed, machine:%d, name idx:%d", pmach->id, pmach->iNameVmemIdx);
		}
		js["mach_list"].Add(mach);
		idx = pmach->iNextIndex;
	}
	js["mach_count"] = i;
	return 0;
}

// 合法名字：以字母开头，由字母数字下划线组成，长度在:3-16
bool IsUserNameValid(const char *pname)
{
	if(!isalpha(pname[0]))
		return false;
	if(strlen(pname) < 3 || strlen(pname) > 16)
		return false;
	for(int i=0; pname[i] != '\0'; i++) {
		if(pname[i] != '_' && !isalpha(pname[i]) && !isdigit(pname[i]))
			return false;
	}
	return true;
}

// 合法 email：长度为 4-40 的电子邮箱地址
bool IsUserEmailValid(const char *pemail)
{
	if(strlen(pemail) < 4 || strlen(pemail) > 40 || pemail[0] == '@')
		return false;

	for(int i=0; pemail[i] != '\0'; i++) {
		if(!isalpha(pemail[i]) && !isdigit(pemail[i]) && pemail[i] != '.' && pemail[i] != '_' && pemail[i] != '@')
			return false;
	}
	return true;
}

bool IsUserMobileValid(const char *pmobile)
{
	for(int i=0; pmobile[i] != '\0'; i++)
	{
		if(!isdigit(pmobile[i]))
			return false;
	}
	return true;
}

size_t CurlWrite(char *ptr, size_t size, size_t nmemb, std::string* userdata)
{
	size_t len = size * nmemb;
	DEBUG_LOG("receive size:%u, nmemb:%u", (unsigned)size, (unsigned)nmemb);
	if(userdata)
		userdata->append(ptr, len);
	return len;
}

bool IsDaemonDenyOp(CGIConfig &stConfig, bool bRespon)
{
	if(!(stConfig.stUser.pSysInfo->dwSystemFlag & SYSTEM_FLAG_DAEMON))
	{
		return false;
	}

	// 超级管理员账号用于管理演示版
	if(stConfig.stUser.puser_info != NULL 
		&& stConfig.stUser.puser_info->iUserId == 1 && stConfig.stUser.puser_info->bLoginType == 1)
	{
		return false;
	}

	if(!bRespon)
		return true;

	Json js;
	js["ec"] = 666; // daemon 演示版返回码

    NEOERR *err;
	STRING str;
	string_init(&str);
	if((err=string_set(&str, js.ToString().c_str())) != STATUS_OK
		|| (err=cgi_output(stConfig.cgi, &str)) != STATUS_OK) {
		string_clear(&str);
		WriteCgiErrorLog(err);
	}
	else
		string_clear(&str);
	return true;
}

int AfterCgiInit(CGIConfig &stConfig)
{
	stConfig.dwCurTime = time(NULL);

	stConfig.pShmConfig = slog.GetSlogConfig();
	if(stConfig.pShmConfig == NULL) 
	{
		ERR_LOG("GetSlogConfig failed");
		return SLOG_ERROR_LINE;
	}

	stConfig.stUser.pSysInfo = slog.GetSystemCfg();
	if(stConfig.stUser.pSysInfo == NULL) {
		ERR_LOG("GetSystemCfg failed!");
		return SLOG_ERROR_LINE;
	}

	return 1;
}

int BeforeCgiRequestInit(CGIConfig &stConfig)
{
	// 染色标记
	if(stConfig.iCfgTestLog)
		slog.SetTestLog(true);
	else
		slog.SetTestLog(false);
	slog.Run();
	stConfig.dwCurTime = time(NULL);
	return 1;
}

int AfterCgiRequestInit(CGIConfig &stConfig)
{
	stConfig.pAction = hdf_get_value(stConfig.cgi->hdf, "Query.action", NULL);
	stConfig.remote = hdf_get_value(stConfig.cgi->hdf, "CGI.RemoteAddress", NULL); 
	stConfig.stUser.puser_info = NULL;
	stConfig.stUser.puser = hdf_get_value(stConfig.cgi->hdf, "Cookie.flogin_user", NULL);;

	// 染色标记用于调试
	if(stConfig.iCfgTestLog)
		hdf_set_int_value(stConfig.cgi->hdf, "config.test_log", 1);
	return 1;
}

int AfterCgiResponse(CGIConfig &stConfig)
{
	int iRunMs = 0;
	if(stConfig.cgi != NULL)
	{
		iRunMs = (int)((stConfig.cgi->time_end-stConfig.cgi->time_start)*1000);
		INFO_LOG("cgiruninfo - %s hits:%u run:%u pid:%u cgi run:%d ms",
			stConfig.pszCgiName, stConfig.dwHits, stConfig.dwCurTime-stConfig.dwStart,
			stConfig.pid, iRunMs);

		if(iRunMs < 100)
			MtReport_Attr_Add(144, 1);
		else if(iRunMs >= 100 && iRunMs < 300)
			MtReport_Attr_Add(145, 1);
		else if(iRunMs >= 300 && iRunMs < 500)
			MtReport_Attr_Add(146, 1);
		else
			MtReport_Attr_Add(147, 1);
	}

	// 慢 cgi
	if(iRunMs >= stConfig.iCgiSlowRunMs)
	{
		WARN_LOG("slow cgiruninfo - cgi:%s, action:%s, run:%d ms, slow:%d", 
			stConfig.pszCgiName, stConfig.pAction, iRunMs, stConfig.iCgiSlowRunMs);
		MtReport_Attr_Add(286, 1);
	}

	cgi_destroy(&stConfig.cgi);
	stConfig.cgi = NULL;

	if(!(stConfig.dwHits < stConfig.dwMaxHits && stConfig.dwExitTime > stConfig.dwCurTime))
	{
		INFO_LOG("cgi:%s hits:%u(%u), run total:%u, will restart", 
			stConfig.pszCgiName, stConfig.dwHits, stConfig.dwMaxHits, stConfig.dwExitTime-stConfig.dwCurTime);
		return SLOG_ERROR_LINE;
	}
	return 1;
}

int AfterCgiLogin(CGIConfig &stConfig)
{
	if(stConfig.stUser.bNeedCheckLv2 
		&& !(!strcmp(stConfig.pszCgiName, "mt_slog_monitor") && stConfig.pAction==NULL)) 
	{
		// 跳转到 lv2 check code 弹窗(这里设置跨站是出于日志系统访问)
		hdf_set_value(stConfig.cgi->hdf, "cgiout.other.cros", "Access-Control-Allow-Origin:*");
		if(stConfig.iResponseType == RESPONSE_TYPE_HTML)
		{
			cgi_redirect_uri(stConfig.cgi, 
				"/cgi-bin/mt_user?action=init_pop_lv2_check\r\nAccess-Control-Allow-Origin:*");
			DEBUG_LOG("cgi pop lv2 check html");
		}
		else
		{
			Json js;
			js["ec"] = (unsigned int)666;
			STRING str;
			string_init(&str);
			NEOERR *err = NULL;
			if((err=string_set(&str, js.ToString().c_str())) != STATUS_OK
					|| (err=cgi_output(stConfig.cgi, &str)) != STATUS_OK)
			{
				string_clear(&str);
				string_init(&str);
				nerr_error_traceback(err, &str);
				ERR_LOG("cgi_output failed ! msg:%s", str.buf);
			}
			string_clear(&str);
			DEBUG_LOG("cgi pop lv2 check json");
		}
		cgi_destroy(&stConfig.cgi);
		stConfig.cgi = NULL;
		return 0;
	}

	return 1;
}	

