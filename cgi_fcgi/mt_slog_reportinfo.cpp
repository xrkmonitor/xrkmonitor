#ifndef __STDC_FORMAT_MACROS 
#define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>

#include <string>
#include <fcgi_stdio.h>
#include <fcgi_config.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <inttypes.h>
#include <cgi_head.h>
#include <cgi_comm.h>
#include <sstream>
#include <cgi_attr.h>
#include <basic_packet.h>

#ifdef MAX_ATTR_READ_PER_EACH
#undef MAX_ATTR_READ_PER_EACH
#endif
#define MAX_ATTR_READ_PER_EACH 120

CSupperLog slog;
CGIConfig stConfig;
MtSystemConfig *g_psysConfig = NULL;

int32_t g_iNeedDb = 0;
static const char *s_JsonRequest [] = { 
	"http_report_data",
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

static int InitFastCgi_first(CGIConfig &myConf)
{
	if(InitFastCgiStart(myConf) < 0) {
		ERR_LOG("InitFastCgiStart failed !");
		return SLOG_ERROR_LINE;
	}

	if(LoadConfig(myConf.szConfigFile,
	   "NEED_DB", CFG_INT, &g_iNeedDb, 0,
		NULL) < 0){
		ERR_LOG("loadconfig failed, from file:%s", myConf.szConfigFile);
		return SLOG_ERROR_LINE;
	}

	int32_t iRet = 0;
	if((iRet=slog.InitConfigByFile(myConf.szConfigFile)) < 0 || (iRet=slog.Init(myConf.szLocalIp)) < 0)
		return SLOG_ERROR_LINE;

	if(slog.InitAttrList() < 0)
	{           
		FATAL_LOG("init mt_attr shm failed !");
		return SLOG_ERROR_LINE;
	}           

	g_psysConfig = slog.GetSystemCfg();
	if(g_psysConfig == NULL) {
	    ERR_LOG("GetSystemCfg failed");
	    return SLOG_ERROR_LINE;
	}
	return 0;
}

void SendLogToServer(SLogClientConfig *plogconfig, char *pContent, int iContentLen)
{
	static uint32_t s_dwLoopIp = inet_addr("127.0.0.1");

	SLogServer *psrv = slog.GetAppMasterSrv(plogconfig->iAppId);
	if(!psrv) {
		ERR_LOG("get app:%d, log server failed", plogconfig->iAppId);
		return;
	}
	CBasicPacket pkg;

	// ReqPkgHead 
	ReqPkgHead stHead;
	pkg.InitReqPkgHead(&stHead, CMD_CGI_SEND_LOG, slog.m_iRand);
	*(int32_t*)(stHead.sReserved) = htonl(slog.GetLocalMachineId());

	// TSignature - empty
	// [cmd content]
	pkg.InitCmdContent((void*)pContent, (uint16_t)iContentLen);
	// TPkgBody - empty

	static char pkgBuf[MAX_ATTR_PKG_LENGTH];
	int iPkgLen = MAX_ATTR_PKG_LENGTH;
	if(pkg.MakeReqPkg(pkgBuf, &iPkgLen) > 0) {
		Ipv4Address addr;
		if(slog.IsIpMatchLocalMachine(psrv->dwIp))
			addr.SetAddress(s_dwLoopIp, psrv->wPort);
		else
			addr.SetAddress(psrv->dwIp, psrv->wPort);
		int iRet = SendUdpPacket(&addr, pkgBuf, iPkgLen, 800, &slog);
		if(iRet != 0) {
			ERR_LOG("SendUdpPacket failed, ret:%d, pkglen:%d, server:%s:%d",
				iRet, iPkgLen, psrv->szIpV4, psrv->wPort);
		}
		else {
			DEBUG_LOG("SendLogToServer ok, server:%s:%d", psrv->szIpV4, psrv->wPort);
		}
	}
}

void SendAttrToServer(char *pContent, int iContentLen, bool bIsStrAttr)
{
	static uint32_t s_dwLoopIp = inet_addr("127.0.0.1");
	CBasicPacket pkg;

	// ReqPkgHead 
	ReqPkgHead stHead;
	if(bIsStrAttr)
	    pkg.InitReqPkgHead(&stHead, CMD_CGI_SEND_STR_ATTR, slog.m_iRand);
	else
	    pkg.InitReqPkgHead(&stHead, CMD_CGI_SEND_ATTR, slog.m_iRand);
	*(int32_t*)(stHead.sReserved) = htonl(slog.GetLocalMachineId());

	// TSignature - empty
	// [cmd content]
	pkg.InitCmdContent((void*)pContent, (uint16_t)iContentLen);
	// TPkgBody - empty

	static char pkgBuf[MAX_ATTR_PKG_LENGTH];
	int iPkgLen = MAX_ATTR_PKG_LENGTH;
	if(pkg.MakeReqPkg(pkgBuf, &iPkgLen) > 0) {
		Ipv4Address addr;
		if(slog.IsIpMatchLocalMachine(g_psysConfig->dwAttrSrvMasterIp))
			addr.SetAddress(s_dwLoopIp, g_psysConfig->wAttrSrvPort);
		else
			addr.SetAddress(g_psysConfig->dwAttrSrvMasterIp, g_psysConfig->wAttrSrvPort);
		int iRet = SendUdpPacket(&addr, pkgBuf, iPkgLen, 800, &slog);
		if(iRet != 0) {
			ERR_LOG("SendUdpPacket failed, ret:%d, pkglen:%d, server:%s",
				iRet, iPkgLen, addr.Convert(true).c_str());
		}
		else {
			DEBUG_LOG("SendAttrToServer ok, server:%s", addr.Convert(true).c_str());
		}
	}
}

int DealReport(CGI *cgi)
{
	const char *pdata = hdf_get_value(cgi->hdf, "Query.data", NULL);
	uint32_t dwSeq = hdf_get_uint_value(cgi->hdf, "Query.req_seq", 0);
	stConfig.pErrMsg = CGI_REQERR;
	if(pdata == NULL) {
		WARN_LOG("invalid parameter");
		return SLOG_ERROR_LINE;
	}
	DEBUG_LOG("get report data, seq:%u, data:%s", dwSeq, pdata);

	size_t len = 0;
	Json jsdata;
	try {
		jsdata.Parse(pdata, len); 
	} catch (Exception e) {
		WARN_LOG("parse data failed, msg:%s", e.ToString().c_str());
		return SLOG_ERROR_LINE;
	}

	AttrInfoBin *pAttrInfo = NULL;
	Json::json_list_t::iterator it;
	if(jsdata.HasValue("attrs")) {
		static AttrNodeClient stAttrRead[MAX_ATTR_READ_PER_EACH];
		Json::json_list_t & jslist = jsdata["attrs"].GetArray();
		int i=0;
		for(it = jslist.begin(); it != jslist.end(); it++) {
			Json &attr = *it;
			// 只允许上报用户自己私有的监控点数据
			pAttrInfo = slog.GetAttrInfo((int)(attr["id"]), NULL);
			if(!pAttrInfo) {
				REQERR_LOG("not find attr:%d", (int)(attr["id"]));
				continue;
			}

			if(i >= MAX_ATTR_READ_PER_EACH) {
				REQERR_LOG("report attr count over limit:%d", MAX_ATTR_READ_PER_EACH);
				break;
			}

			if((int)(attr["number"]) <= 0) {
				REQERR_LOG("invalid cgi attr report info attr:%d", (int)(attr["id"]));
				continue;
			}
	
			stAttrRead[i].iAttrID = htonl((int)(attr["id"]));
			stAttrRead[i].iCurValue = htonl((int)(attr["number"]));
			DEBUG_LOG("js report attr:%d, value:%d",
				ntohl(stAttrRead[i].iAttrID), ntohl(stAttrRead[i].iCurValue));
			i++;
		}

		if(i > 0) {
			SendAttrToServer((char*)stAttrRead, i*sizeof(AttrNodeClient), false);
		}
	}

	if(jsdata.HasValue("strattrs")) {
		static char strAttrSendBuf[MAX_ATTR_PKG_LENGTH];
		StrAttrNodeClient *pNodeBuf = NULL;
		const char  *pstr = NULL;
		Json::json_list_t & jslist = jsdata["strattrs"].GetArray();
		int iUseBufLen = 0, iTmpLen = 0;
		for(it = jslist.begin(); it != jslist.end(); it++) {
			Json &strattr = *it;
			// 只允许上报用户自己私有的监控点数据
			pAttrInfo = slog.GetAttrInfo((int)(strattr["id"]), NULL);
			if(!pAttrInfo || (pAttrInfo->iDataType != STR_REPORT_D && pAttrInfo->iDataType != STR_REPORT_D_IP))
			{
				REQERR_LOG("not find str attr:%d", (int)(strattr["id"]));
				continue;
			}
			if(!strattr.HasValue("strings"))
				continue;

			Json::json_list_t & jslist_str = strattr["strings"].GetArray();
			Json::json_list_t::iterator it_str = jslist_str.begin();
			for(; it_str != jslist_str.end(); it_str++) {
				Json &attr = *it_str;
				pstr = (const char*)(attr["string"]);
				DEBUG_LOG("cgi report str attr:%d, str:%s, value:%d", (int)(strattr["id"]), pstr, (int)(attr["number"]));

				iTmpLen = strlen(pstr)+1;
				if(iTmpLen <= 1 || (int)(attr["number"]) <= 0) {
					REQERR_LOG("invalid str attr report info: str attr:%d", (int)(strattr["id"]));
					continue;
				}
				if(sizeof(StrAttrNodeClient)+iTmpLen+iUseBufLen >= sizeof(strAttrSendBuf)) {
					REQERR_LOG("need more space to save cgi str attr report, str attr:%d, uselen:%d, tmplen:%d",
						(int)(strattr["id"]), iUseBufLen, iTmpLen);
					break;
				}

				pNodeBuf = (StrAttrNodeClient*)(strAttrSendBuf+iUseBufLen);
				pNodeBuf->iStrAttrId = htonl((int)(strattr["id"]));
				pNodeBuf->iStrVal = htonl((int)(attr["number"]));
				pNodeBuf->iStrLen = htonl(iTmpLen);
				memcpy(pNodeBuf->szStrInfo, pstr, iTmpLen);
				iUseBufLen += sizeof(StrAttrNodeClient) + iTmpLen;
			}

			if(it_str != jslist_str.end())
				break;
		}

		if(iUseBufLen > 0) {
			SendAttrToServer((char*)strAttrSendBuf, iUseBufLen, true);
		}
	}

	if(jsdata.HasValue("logs")) {
		static char sAppLogBuf[MAX_APP_LOG_PKG_LENGTH];
		LogInfo *pLogBuf = NULL; 
		Json::json_list_t & jslist = jsdata["logs"].GetArray();
		int iUseBufLen = 0, iTmpLen = 0, iLogType = 0;
		const char  *pstr = NULL, *plogtype = NULL;
		SLogClientConfig *plogconfig = slog.GetSlogConfig((uint32_t)(jsdata["log_config_id"]));
		if(!plogconfig) {
			REQERR_LOG("not find log config:%u", (uint32_t)(jsdata["log_config_id"]));
		}
		else {
			// 不允许上报全局应用的日志
			AppInfo *pAppInfo = slog.GetAppInfo(plogconfig->iAppId);
			if(!pAppInfo) {
				REQERR_LOG("not find app info, log config:%u, appid:%d",
					plogconfig->dwCfgId, plogconfig->iAppId); 
				plogconfig = NULL;
			}
		}

		for(it = jslist.begin(); plogconfig && it != jslist.end(); it++) 
		{
			Json &log = *it;
			plogtype = (const char*)(log["type"]);
			pstr = (const char*)(log["msg"]);
			iTmpLen = strlen(pstr)+1;
			iLogType = GetLogTypeByStr(plogtype);
			if(iTmpLen <= 1 || iLogType == 0)
			{
				REQERR_LOG("invalid log report info - configid:%u, logtype:%d", plogconfig->dwCfgId, iLogType);
				continue;
			}

			if(!(iLogType & plogconfig->iLogType)) {
				DEBUG_LOG("log type:%d(%s), not match:%d, config id:%u",
					iLogType, plogtype, plogconfig->iLogType, plogconfig->dwCfgId);
				continue;
			}

			if(iUseBufLen+sizeof(LogInfo)+iTmpLen >= sizeof(sAppLogBuf)) {
				REQERR_LOG("need more space to save cgi log report, config id:%u, uselen:%d, tmplen:%d",
					plogconfig->dwCfgId, iUseBufLen, iTmpLen);
				break;
			}
			DEBUG_LOG("cgi report log info - config id:%u, log:%s", plogconfig->dwCfgId, pstr);
					
			pLogBuf = (LogInfo*)(sAppLogBuf+iUseBufLen);
			pLogBuf->qwLogTime = htonll((uint64_t)(log["time"]));
			pLogBuf->dwLogSeq = (pLogBuf->qwLogTime%10000000);
			pLogBuf->iAppId = htonl(plogconfig->iAppId);
			pLogBuf->iModuleId = htonl(plogconfig->iModuleId);
			pLogBuf->dwLogConfigId = htonl(plogconfig->dwCfgId);
			pLogBuf->wLogType = htons(iLogType);
			pLogBuf->wCustDataLen = 0;
			pLogBuf->wLogDataLen = htons(iTmpLen);
			strcpy(pLogBuf->sLog, pstr);
			iUseBufLen += sizeof(LogInfo) + iTmpLen;
		}

		if(iUseBufLen > 0) {
			SendLogToServer(plogconfig, sAppLogBuf, iUseBufLen);
		}
	}

	stConfig.cgi->time_end = ne_timef();
	int iUseMs = (int)(((stConfig.cgi->time_end-stConfig.cgi->time_start)*1000000+500)/1000);

	Json js;
	js["ret"] = 0;
	js["seq"] = dwSeq;
	js["cgi_run_time"] = iUseMs;
	js["client_ip"] = stConfig.remote;

	std::string strResp = js.ToString();
	STRING str;
	string_init(&str);
	if((stConfig.err=string_set(&str, strResp.c_str())) != STATUS_OK
		|| (stConfig.err=cgi_output(stConfig.cgi, &str)) != STATUS_OK)
	{
		string_clear(&str);
		stConfig.pErrMsg = CGI_ERR_SERVER;
		return SLOG_ERROR_LINE;
	}
	string_clear(&str);

	DEBUG_LOG("report data response:%s", strResp.c_str());
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

		DEBUG_LOG("get action :%s from :%s", pAction, stConfig.remote);
		if(!strcmp(pAction, "http_report_data"))
			iRet = DealReport(stConfig.cgi);
		else {
			REQERR_LOG("unknow action:%s", pAction);
			iRet = -1;
		}
		if(iRet < 0)
		{
			show_errpage(NULL, CGI_REQERR, stConfig);
			continue;
		}

		const char *pcsTemplate = NULL;
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

