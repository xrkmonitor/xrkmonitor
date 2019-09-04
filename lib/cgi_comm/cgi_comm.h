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

#ifndef __CGICOMM_H__ 
#define __CGICOMM_H__

#include "cgi_head.h"
#include "des.h"
#include "user.pb.h"

#define CHECK_RESULT_SUCCESS 0 
#define CHECK_RESULT_UNAME_UPASS 2 
#define CHECK_RESULT_SERVER 3 
#define CHECK_RESULT_TOO_MUCH_PEOPLE_LOGIN 4 
#define CHECK_RESULT_INVALID_LOGIN_TYPE 5 
#define CHECK_RESULT_LOGIN_EXPIRE 111 
#define CHECK_RESULT_OK 200 
#define CHECK_RESULT_COMM_ERROR 300 
#define CHECK_RESULT_POP_LV2_CHECK_DLG 666 
#define CHECK_RESULT_SEARCH_HISLOG_COMPLETE 888 
#define CHECK_RESULT_NOT_FIND_APP_LOG 777 

#define COOKIE_TIMEOUT_SECOND (2*24*60*60)

int show_errpage(const char * cs_path, const char * err_msg, CGIConfig &stConfig);
NEOERR *my_cgi_display (CGI *cgi, const char *cs_file, bool bSaveOut=false);
NEOERR *my_cgi_output_local(CGI *cgi, const char *cs_file);
int my_cgi_file_download (const char *file, const char *fileName);
int my_cgi_excel_output (const char *fileName, const char *buff);
char *get_value(CGI *cgi, char *para);
char *strmov(char *dst,const char *src);

const char *GetCookieKey(uint32_t ttm, uint32_t dwUserId);
const char *GetCookieTime(CGIConfig &stConfig, uint32_t dwExpire=0);
const char *GetCookieTime(uint32_t dwExpire);
int GetUserInfo(CGIConfig &stConfig, uint32_t dwUserId=0);
int GetTradeInfo(uint32_t dwUserId=0);
const char *GetTopTimeStamp(uint32_t ttm);
void RedirectToMain(CGI *cgi);
void RedirectToMainTop(CGI *cgi);
int GetRecordTotal(const char *szRecordDest, CGIConfig &stConfig);
void SetCookie(const char *name, const char *value, uint32_t dwExpireTime, CGIConfig &stConfig);
int GetRecordCur(const char *szRecordDest, CGIConfig &stConfig);
int SetPageInfo(CGIConfig &stConfig); 
void MyRedirect(CGI *cgi, const char *pszCgiName);
int InitFastCgiStart(CGIConfig &myCfg);
int InitFastCgi(CGIConfig &myCfg, const char *pszLogPath=CGI_COREDUMP_DEBUG_OUTPUT_PATH);
int CheckLogin(CGI *cgi,
	FloginList *pshmLoginList, const char *remote, uint32_t dwCurTime=0, CgiReqUserInfo *pinfo=NULL);
int CheckLoginEx(CGIConfig &stConfig, CGI *cgi,
	FloginList *pshmLoginList, const char *remote, uint32_t dwCurTime=0, CgiReqUserInfo *pinfo=NULL);
void InitCgiDebug(CGIConfig &myCfg);
void DealCgiFailedExit(CGI *cgi, NEOERR *err);
int DealDbConnect(CGIConfig &stCfg);
void DealCgiCoredump(int sig, const char *pszFile, void *pdata);

void RedirectToFastLogin(CGIConfig &stConfig);
int FastLogin(CGI *cgi, CGIConfig &stConfig);
int FcgiCheckTest(uint8_t bKeyType, const char *pkey, const void *pdata);
int SetRecordsPageInfo(CGI *cgi, int iTotalRecords, int iDefNumPerPage=20, int iDefPageNumShown=8, int iDefCurrPage=1);
int SetRecordsOrder(CGI *cgi, char *sApdSql, const char *pCheckF);
void DealReportTimeInfo(CGI *cgi);

FloginInfo* SearchOnlineUserById(FloginList *pshmLoginList, int uid, int & index, int & count);
void SetFloginCookie(
	CGI *cgi, const char *puser, const char *pmd5, int32_t index, int32_t id, int32_t itype);
int32_t GetFloginFree(FloginList *pshmLoginList, uint32_t dwCurTime);

void SetCgiResponseType(CGIConfig &stConfig, const char *s_JsonRequest[]); 
void SetCgiResponseErrInfo(CGIConfig &stConfig, const char *strErrMsg, int iErrorCode=CGI_ERROR_CODE_COMM);

#define VERIFY_CODE_LEN 4
#define VERIFY_COUNT_MAX 20000
#define VERIFY_EXPIRE_TIME 90
#define DEFAULT_VERIFY_SHM_KEY 20141010

typedef struct {
	uint32_t dwExpireTime;
	uint32_t dwRemoteIP;
	char sVerifyCode[VERIFY_CODE_LEN+1];
}TVerifyInfo;

typedef struct {
	uint32_t iFreeVerifyIndex;
	TVerifyInfo stVerifyList[VERIFY_COUNT_MAX];
}TVerifyList;

#define HT_DB_CHECK_FAILED 1

int DealCgiHeart(CGIConfig &stConfig);
int DealCgiHeart(CGI *cgi, int ht_ret);

int SetUserSessionInfo(FloginInfo *psess, user::UserSessionInfo & user);
int GetUserSessionInfo(FloginInfo *psess, user::UserSessionInfo & user);
int FindAttrTypeByTree(MmapUserAttrTypeTree & stTypeTree, int iAttrType);

int GetUserMachineListFromVmem(Json &js);

bool IsUserNameValid(const char *pname);
bool IsUserEmailValid(const char *pemail);
bool IsUserMobileValid(const char *pmobile);

size_t CurlWrite(char *ptr, size_t size, size_t nmemb, std::string* userdata);
bool IsDaemonDenyOp(CGIConfig &stConfig, bool bRespon=true);

int AfterCgiInit(CGIConfig &stConfig); 
int BeforeCgiRequestInit(CGIConfig &stConfig); 
int AfterCgiRequestInit(CGIConfig &stConfig); 
int AfterCgiLogin(CGIConfig &stConfig); 
int AfterCgiResponse(CGIConfig &stConfig); 

#endif

