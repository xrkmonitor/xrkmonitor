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

#ifndef __CGIHEAD_H__ 
#define __CGIHEAD_H__

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS 1
#endif

#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <Base64.h>
#include <supper_log.h>
#include <Utility.h>
#include <myparam_comm.h>
#include <FloginSession.h>
#include <set>
#include <Json.h>
#include <user.pb.h>

#include "Memcache.h"

#include <ClearSilver.h>
#include <sv_log.h>
#include <sv_struct.h>
#include <md5.h>
#include <mt_report.h>
#include <sv_cfg.h>
#include <sv_struct.h>
#include <sv_coredump.h>
#include <sv_net.h>

// 表记录 status 值
#define STATUS_USE 0
#define STATUS_DELETE 1

// 启用调试支持，部署到正式环境时，请关闭, 以免出现安全问题
#define CGI_DEBUG_ENABLE 1

// fast cgi 进程重启参数
#define FAST_CGI_DEF_HITS_MAX 20000
#define FAST_CGI_DEF_RUN_MAX_TIME_HOURS 128 

// CGI 调试信息或者coredump 信息输出文件路径
#define CGI_COREDUMP_DEBUG_OUTPUT_PATH "../htdocs/cgi_debug/" 

#define CGI_PATH "/cgi-bin/"
#define DWZ_PATH "/monitor/"
#define DOC_PATH "/monitor/"
#define CS_PATH "../htdocs/monitor/"

// 数据库状态码
#define RECORD_STATUS_USE 0 // 记录使用中
#define RECORD_STATUS_DELETE 1 // 记录更新后，程序自动删除

// cgi 日志基本信息
#define CGI_BASIC_LOG(puser) puser->szUserName, puser->iUserId
#define CGI_BASIC_LOG_FMT " request info [%s-%d] "

// web note msg id list
#define SET_MSG_ID(stConfig, id) do { if(0==stConfig.dwMsgId) { stConfig.dwMsgId = id; \
		if(stConfig.cgi) hdf_set_int_value(stConfig.cgi->hdf, "config.msg_id", stConfig.dwMsgId); \
}}while(0)

#define MSG_ID_NONE 0
#define MSG_ID_SAVE_CONFIG_SUCCESS 1
#define MSG_ID_SAVE_CONFIG_FAILED 2
#define MSG_ID_INVALID_REQUEST_PARA 3
#define MSG_ID_GET_TEMPLATE_FAILED 4
#define MSG_ID_DENY_MOD_SYS_TEMPLATE 5
#define MSG_ID_DENY_DEL_SYS_TEMPLATE 6
#define MSG_ID_DELETE_FAILED 7
#define MSG_ID_ERR_SERVER 8
#define MSG_ID_USE_TEMPLATE_NOT_CHECK 9
#define MSG_ID_USE_DEF_CONFIG_OK 10
#define MSG_ID_USE_DEF_CONFIG_FAILED 11
#define MSG_ID_MOD_FAILED 12

// 成功提示字符串ID
#define MSG_ID_OP_SUCCESS_START 1000
#define MSG_ID_DELETE_SUCCESS 1001
#define MSG_ID_ADD_SUCCESS 1002
#define MSG_ID_MODIFY_SUCCESS 1003

#define CGI_CONFFILE "./cgi.conf"
#define FCGI_CONFFILE "./fcgi.conf"
#define CSPATH "../htdocs/"

#define JQUERY_FILE "/js/jquery-1.8.3.min.js"
#define PAGE_ERROR "error.html"
#define PAGE_MAIN CSPATH"monitor/index.html"
#define PAGE_FCGI_LOGIN CSPATH"monitor/dmt_login.html"
#define PAGE_FCGI_LOGIN_DWZ CSPATH"monitor/dmt_login_dwz.html"
#define PAGE_FCGI_DLG_LOGIN_DWZ CSPATH"monitor/dmt_dlg_login_dwz.html"

// string list 
#define SET_ERRMSG(msg) do { if(!stConfig.bSetError) { \
	hdf_set_value(stConfig.cgi->hdf, "err.msg", msg); \
	stConfig.bSetError = true; }}while(0)

#define CGI_INIT_FAILED "cgi 初始化失败！"
#define CGI_PARSE_ERROR "cgi 解析失败！" 
#define CGI_ERR_SERVER "服务器错误！" // system bug
#define CGI_REQERR "cgi 请求错误，参数不合法！" // user make
#define CGI_ERRDB_SERVER "数据库操作失败！"
#define CGI_ACCESS_DENY "无权操作！"
#define CGI_FIND_FAILED "查找失败"
#define CGI_DENY_DEL_USELF "不允许删除自己的账号"
#define CGI_DEL_FAILED_UNOT_EXIST "删除失败，账号不存在！"
#define CGI_DENY_DEL_USELF "不允许删除自己的账号"
#define CGI_NO_APP_MODULE_TO_CONFIG "应用模块数目为0，请先添加应用和模块！"
#define CGI_ERROR_ALREADY_HAS_MACHINE "机器IP重复配置（%s）"
#define CGI_ERROR_INFO_HAVE_NO_APP_LOG "app 没有日志上报"

// error code
#define CGI_ERROR_CODE_COMM 300

#define ERROR_LINE -(__LINE__)

#define RESPONSE_TYPE_HTML 0 // default
#define RESPONSE_TYPE_JSON 1
#define DEBUG_MAGIC 1317791
#define DEBUG_PAUSE_MAGIC 2317893

typedef struct
{
	const char *puser;
	FloginInfo *puser_info;
	int iLoginType;
	int iLoginIndex;
	bool bNeedCheckLv2;
	MtSystemConfig *pSysInfo;
	user::UserSessionInfo pbSess; // 从登录 session 中取出的信息
}CgiReqUserInfo;

// 用于 fast cgi
typedef struct _CGIConfig
{
	char szXrkmonitorSiteAddr[256];
	int iDisableVmemCache; // 是否禁用 vmem 缓存

	int argc;
	char **argv;
	char **envp;
	uint32_t pid;
	uint32_t dwCurTime;
	uint32_t dwHits;
	uint32_t dwMaxHits;
	uint32_t dwExitTime;
	uint32_t dwStart;
	uint32_t dwEnd;
	const char *pAction;
	char szLocalIp[20]; 
	char szDebugPath[256];
	char szCoredumpFile[256];
	char szConfigFile[64];
	Database *db;
	Query *qu;
	CGI *cgi;
	HDF *hdf;
	NEOERR *err;
	char szCgiPath[256]; // cgi 路径 -- html web 用
	char szCsPath[256]; // cs 模板路径 -- cgi 用
	char szDocPath[256]; // 网站根目录下 -- html web 用
	char szRedirectUri[256];
	char szCurPath[256]; // 当前工作目录

	const char *remote;
	char szUserNick[SIZE_OF_SESS_USER_NICK];
	char *pszCgiName;

	uint32_t dwUserId;
	uint16_t wUserType;
	bool bSetError;
	uint32_t dwMsgId;
	FloginList *pshmLoginList;
	int32_t iDeleteStatus; // 删除记录使用的状态码，默认值 1，程序更新后自动删除
	CgiReqUserInfo stUser;

	int iResponseType; // 响应类型(html, json ...)
	int32_t iErrorCode; // 错误码  -- json 
	const char *pErrMsg; // 错误提示 -- json
	const char *pMsgId; // 错误提示 -- json
	int iEnableCgiPause; // 开启 cgi sleep 等待gdb
	int iEnableCgiDebug; 
	int iDebugDumpHdf;
	int iCgiSlowRunMs; // 慢cgi 耗时配置

	int iCfgTestLog; // 配置中的染色标记用于调试程序

	// page 
	int32_t dwRecordTotal;
	int16_t wPageSize;
	int16_t wPageCur;
	SLogConfig *pShmConfig;
	SLogAppInfo *pAppInfo;
}CGIConfig;

#endif

