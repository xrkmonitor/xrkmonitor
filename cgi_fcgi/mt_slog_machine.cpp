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

   fastcgi mt_slog_machine: 上报机器相关

****/

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS 1
#endif

#include <string>
#include <assert.h>
#include <fcgi_stdio.h>
#include <fcgi_config.h>
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <inttypes.h>
#include <cgi_head.h>
#include <cgi_comm.h>

CSupperLog slog;
CGIConfig stConfig;

// ajax json 响应方式
static const char *s_JsonRequest [] = { 
	"search",
	"delete",
	"delete_syssrv",
	"save_add_machine",
	"save_mod_machine",
	"save_add_syssrv",
	"save_mod_syssrv",
	NULL
};

typedef struct
{
	int id;
	const char *pkey;
	const char *ip;
	int model;
	int warn_flag;
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

static void SetMachineInfo()
{
	hdf_set_int_value(stConfig.cgi->hdf, "config.dmm_mach_allow_all", MACH_WARN_ALLOW_ALL);
	hdf_set_int_value(stConfig.cgi->hdf, "config.dmm_mach_deny_all", MACH_WARN_DENY_ALL);
	hdf_set_int_value(stConfig.cgi->hdf, "config.dmm_mach_deny_basic", MACH_WARN_DENY_BASIC);
	hdf_set_int_value(stConfig.cgi->hdf, "config.dmm_mach_deny_except", MACH_WARN_DENY_EXCEPT);
}

static int AddSearchInfo(char *psql, int ibufLen, SearchInfo *pinfo)
{
	char sTmpBuf[128] = {0};
	int iFilter = 0;
	if(pinfo->id != 0)
	{
		sprintf(sTmpBuf, " and xrk_id=%d", pinfo->id);
		strcat(psql, sTmpBuf);
		hdf_set_int_value(stConfig.cgi->hdf, "config.dm_machine_id", pinfo->id);
		iFilter++;
	}

	if(pinfo->ip != NULL && pinfo->ip[0] != '\0')
	{
		uint32_t ip = inet_addr(pinfo->ip);
		sprintf(sTmpBuf, " and (ip1=%u or ip2=%u or ip3=%u or ip4=%u)", ip, ip, ip, ip);
		strcat(psql, sTmpBuf);
		hdf_set_value(stConfig.cgi->hdf, "config.dm_machine_ip", pinfo->ip);
		iFilter++;
	}

	if(pinfo->pkey != NULL && pinfo->pkey[0] != '\0')
	{
		memset(sTmpBuf, 0, sizeof(sTmpBuf));
		snprintf(sTmpBuf, sizeof(sTmpBuf)-1, " and (xrk_name like \'%%%s%%\' or machine_desc like "
			" \'%%%s%%\')", pinfo->pkey, pinfo->pkey);
		if((int)(strlen(sTmpBuf) + strlen(psql)) >= ibufLen)
		{
			REQERR_LOG("search key(%s) too long, tmp:%s sql:%s", pinfo->pkey, sTmpBuf, psql);
			hdf_set_value(stConfig.cgi->hdf, "err.msg", CGI_REQERR);
			return SLOG_ERROR_LINE;
		}
		strcat(psql, sTmpBuf);
		hdf_set_value(stConfig.cgi->hdf, "config.dm_keyword", pinfo->pkey);
		iFilter++;
	}

	if(pinfo->model != 0)
	{
		sprintf(sTmpBuf, " and model_id=%d", pinfo->model);
		strcat(psql, sTmpBuf);
		hdf_set_int_value(stConfig.cgi->hdf, "config.dm_machine_model", pinfo->model);
		iFilter++;
	}

	if(pinfo->warn_flag != 0)
	{
		sprintf(sTmpBuf, " and warn_flag=%d", pinfo->warn_flag);
		strcat(psql, sTmpBuf);
		hdf_set_int_value(stConfig.cgi->hdf, "config.dm_machine_warn_flag", pinfo->warn_flag);
		iFilter++;
	}
	DEBUG_LOG("after add search info sql:%s, filter:%d", psql, iFilter);
	return iFilter;
}

static int GetMachineTotalRecords(SearchInfo *pinfo=NULL)
{
	char sSqlBuf[512] = {0};
	Query & qu = *stConfig.qu;
	
	int iFilter = 0;
	sprintf(sSqlBuf, "select count(*) from mt_machine where xrk_status=%d", RECORD_STATUS_USE);
	if(pinfo != NULL && (iFilter=AddSearchInfo(sSqlBuf, sizeof(sSqlBuf), pinfo)) < 0)
		return SLOG_ERROR_LINE;

	if(iFilter <= 0 && !stConfig.iDisableVmemCache) {
		DEBUG_LOG("get machine count:%u from shm", stConfig.stUser.pSysInfo->wMachineCount);
		return (int)stConfig.stUser.pSysInfo->wMachineCount;
	}
	
	int iCount = qu.get_count(sSqlBuf);
	DEBUG_LOG("machine records count:%d", iCount);
	return iCount;
}

static void DeleteMachineFromShm(int id)
{
	MachineInfo *pInfo = slog.GetMachineInfo(id, NULL);
	if(NULL == pInfo) {
		REQERR_LOG("not find machine:%d", id);
		return;
	}

	SharedHashTableNoList *pMachHash = slog.GetMachineHash();
	MachineInfo *pPrev = NULL, *pNext = NULL;
	if(pInfo->iPreIndex >= 0)
	    pPrev = (MachineInfo*)NOLIST_HASH_INDEX_TO_NODE(pMachHash, pInfo->iPreIndex);
	if(pInfo->iNextIndex >= 0)
	    pNext = (MachineInfo*)NOLIST_HASH_INDEX_TO_NODE(pMachHash, pInfo->iNextIndex);
	ILINK_DELETE_NODE(stConfig.stUser.pSysInfo, 
		iMachineListIndexStart, iMachineListIndexEnd, pInfo, pPrev, pNext, iPreIndex, iNextIndex);
	stConfig.stUser.pSysInfo->wMachineCount--;
	if((int)stConfig.stUser.pSysInfo->wMachineCount < 0)
	{
		WARN_LOG("invalid machine count:%d", stConfig.stUser.pSysInfo->wMachineCount);
		stConfig.stUser.pSysInfo->wMachineCount = 0;
	}

	// 从共享内存中删除 -- 需要同  slog_config 中的删除逻辑一致
	pInfo->id = 0;

	// vmem 释放
	if(pInfo->iNameVmemIdx > 0) {
	    MtReport_FreeVmem(pInfo->iNameVmemIdx);
	    pInfo->iNameVmemIdx = -1;
	}
	if(pInfo->iDescVmemIdx > 0) {
	    MtReport_FreeVmem(pInfo->iDescVmemIdx);
	    pInfo->iDescVmemIdx = -1;
	}
	DEBUG_LOG("delete machine:%d from shm, remain count:%d", id, stConfig.stUser.pSysInfo->wMachineCount);
}

static int DealMachineAddPlugin()
{
	int id = hdf_get_int_value(stConfig.cgi->hdf, "Query.id", 0);
	if(id==0)
	{
		REQERR_LOG("invalid parameter from:%s", stConfig.remote);
		hdf_set_value(stConfig.cgi->hdf, "err.msg", CGI_REQERR);
		return SLOG_ERROR_LINE;
	}

    MachineInfo *pMachinfo = slog.GetMachineInfo(id, NULL);
    if(!pMachinfo) {
        REQERR_LOG("not find machine : %d", id);
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
    hdf_set_int_value(stConfig.cgi->hdf, "config.machine_id", id);
    hdf_set_value(stConfig.cgi->hdf, "config.machine_ip", ips.c_str());
    const char *pname = MtReport_GetFromVmem_Local(pMachinfo->iNameVmemIdx);
    hdf_set_value(stConfig.cgi->hdf, "config.machine_name", pname ? pname : "unknow");

    int iCurAgentRunTime = 0;
    if(pMachinfo->dwLastHelloTime+300 >= stConfig.dwCurTime && stConfig.dwCurTime > pMachinfo->dwLastHelloTime)  {
        hdf_set_int_value(stConfig.cgi->hdf, "config.run_time", stConfig.dwCurTime-pMachinfo->dwAgentStartTime);
        iCurAgentRunTime = stConfig.dwCurTime-pMachinfo->dwAgentStartTime;
    }
    else
        hdf_set_int_value(stConfig.cgi->hdf, "config.run_time", 0);

    std::ostringstream ss;
    ss << "select agent_os,os_arc,libc_ver,libcpp_ver from mt_machine where xrk_id=" << id;
	Query & qu = *stConfig.qu;
    if(qu.get_result(ss.str().c_str()) && qu.num_rows() > 0) {
        qu.fetch_row();
        hdf_set_value(stConfig.cgi->hdf, "config.run_os", qu.getstr("agent_os"));
        hdf_set_value(stConfig.cgi->hdf, "config.os_arc", qu.getstr("os_arc"));
        hdf_set_value(stConfig.cgi->hdf, "config.libc_ver", qu.getstr("libc_ver"));
        hdf_set_value(stConfig.cgi->hdf, "config.libcpp_ver", qu.getstr("libcpp_ver"));
    }
    qu.free_result();

    std::map<int, int> stAlreadyInstall;
    ss.str("");
    ss << "select xrk_id,open_plugin_id from mt_plugin_machine where install_proc=0 and machine_id=" 
        << pMachinfo->id << " and xrk_status=0";
    if(qu.get_result(ss.str().c_str()) && qu.num_rows() > 0) {
	    for(int i=0; i < qu.num_rows() && qu.fetch_row() != NULL; i++) {
            stAlreadyInstall.insert(std::pair<int, int>(qu.getval("open_plugin_id"), qu.getval("xrk_id")));
        }
    }
    qu.free_result();

    Json js;
    const char *ptmp = NULL;
    const char *pm_os = hdf_get_value(stConfig.cgi->hdf, "config.run_os", NULL);
    int iPluginCount = 0;

	ss.str("");
	ss << " select * from mt_plugin where xrk_status=0";
	if(!qu.get_result(ss.str().c_str()) || qu.num_rows() <= 0) {
		WARN_LOG("get no plugin !");
	}
	else {
		const char *pinfo = NULL;
		size_t iParseIdx = 0;
		size_t iReadLen = 0;
	
		while(qu.fetch_row()) {
			Json plugin;
			pinfo = qu.getstr("pb_info");
			iParseIdx = 0;
			iReadLen = strlen(pinfo);
			plugin.Parse(pinfo, iParseIdx);
			if(iParseIdx != iReadLen) {
				WARN_LOG("parse json content, size:%u!=%u", (uint32_t)iParseIdx, (uint32_t)iReadLen);
				continue;
			}

			Json jp;
            if(stAlreadyInstall.find((int)(plugin["plugin_id"])) != stAlreadyInstall.end())
                jp["installed"] = 1;
            else {
                // agent 未部署或未运行，不能一键部署插件
                if(iCurAgentRunTime <= 0)
                    continue;

                // 插件是否支持一键部署
                ptmp = plugin["install_tp_file"];
                if(ptmp[0] == '\0' || !strcmp(ptmp, "no"))
                    continue;

                // 运行平台是否适合部署
                std::string strBigOsType;
                GetOsType(pm_os, strBigOsType);
                ptmp = plugin["run_os"];
                if(!IsStrEqual(pm_os, ptmp) && !IsStrEqual(ptmp, strBigOsType.c_str())) {
                    DEBUG_LOG("os not macth, mach:%s(%s), plugin:%s", pm_os, strBigOsType.c_str(), ptmp);
                    continue;
                }
                jp["installed"] = 0;
            }

            jp["id"] = (int)(plugin["plugin_id"]);
            jp["name"] = (const char*)(plugin["plus_name"]);
            jp["show_name"] = (const char*)(plugin["show_name"]);
            jp["language"] = (const char*)(plugin["dev_language"]);
            jp["auth"] = (const char*)(plugin["plugin_auth"]);
            jp["last_version"] = (const char*)(plugin["plus_version"]);
			jp["run_os"] = (const char*)(plugin["run_os"]);

            ss.str("");
            ss << "http://" << stConfig.szXrkmonitorSiteAddr << "/plugin/" << (const char*)(jp["name"]) << ".html";
            jp["desc_url"] = ss.str();
            js["plugins"].Add(jp);
            iPluginCount++;
        }
    }
    js["count"] = iPluginCount;

    std::string str(js.ToString());
    hdf_set_value(stConfig.cgi->hdf, "config.machine_plugins", str.c_str());
	DEBUG_LOG("machine:%d, add plugin", id);
    return 0;
}

static int DealShowMachineStatus()
{
	int id = hdf_get_int_value(stConfig.cgi->hdf, "Query.id", 0);
	if(id==0)
	{
		REQERR_LOG("invalid parameter from:%s", stConfig.remote);
		hdf_set_value(stConfig.cgi->hdf, "err.msg", CGI_REQERR);
		return SLOG_ERROR_LINE;
	}

    MachineInfo *pMachinfo = slog.GetMachineInfo(id, NULL);
    if(!pMachinfo) {
        REQERR_LOG("not find machine : %d", id);
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
    hdf_set_int_value(stConfig.cgi->hdf, "config.machine_id", id);
    hdf_set_value(stConfig.cgi->hdf, "config.machine_ip", ips.c_str());
    const char *pname = MtReport_GetFromVmem_Local(pMachinfo->iNameVmemIdx);
    hdf_set_value(stConfig.cgi->hdf, "config.machine_name", pname ? pname : "unknow");
    hdf_set_value(stConfig.cgi->hdf, "config.last_attr", uitodate(pMachinfo->dwLastReportAttrTime));

    hdf_set_value(stConfig.cgi->hdf, "config.last_log", uitodate(pMachinfo->dwLastReportLogTime));
	DEBUG_LOG("machine time, attr:%u, log:%u, hello:%u", pMachinfo->dwLastReportAttrTime,
		pMachinfo->dwLastReportLogTime, pMachinfo->dwLastHelloTime);

    hdf_set_value(stConfig.cgi->hdf, "config.last_hello", uitodate(pMachinfo->dwLastHelloTime));
    hdf_set_int_value(stConfig.cgi->hdf, "config.rep_status", GetMachineRepStatus(pMachinfo, stConfig));
    if(pMachinfo->dwLastHelloTime+300 >= stConfig.dwCurTime && stConfig.dwCurTime > pMachinfo->dwLastHelloTime) 
        hdf_set_int_value(stConfig.cgi->hdf, "config.run_time", stConfig.dwCurTime-pMachinfo->dwAgentStartTime);
    else
        hdf_set_int_value(stConfig.cgi->hdf, "config.run_time", 0);

	Query & qu = *stConfig.qu;
    std::ostringstream ss;
    Json js;
    ss << "select * from mt_plugin_machine where install_proc=0 and machine_id=" << pMachinfo->id << " and xrk_status=0";
    if(qu.get_result(ss.str().c_str()) && qu.num_rows() > 0) {
		Query myqu(*stConfig.db);
	    for(int i=0; i < qu.num_rows() && qu.fetch_row() != NULL; i++)
        {
            Json info;
            info["id"] = qu.getval("open_plugin_id");
			ss.str("");
			ss << "select plugin_name,plugin_show_name from mt_plugin where open_plugin_id=" << (int)(info["id"]);
			if(!myqu.get_result(ss.str().c_str()) || myqu.num_rows() <= 0) {
				WARN_LOG("not find plugin:%d, machine id:%d", (int)(info["id"]), pMachinfo->id);
				continue;
			}
			myqu.fetch_row();
			info["name"] = myqu.getstr("plugin_name");
			info["show_name"] = myqu.getstr("plugin_show_name");
			myqu.free_result();

            info["last_attr_time"] = uitodate(qu.getuval("last_attr_time"));
            info["last_log_time"] = uitodate(qu.getuval("last_log_time"));
            info["start_time"] = uitodate(qu.getuval("start_time"));
            if(qu.getuval("last_hello_time")+300 >= stConfig.dwCurTime && stConfig.dwCurTime > qu.getuval("last_hello_time")) 
                info["run_time"] = stConfig.dwCurTime-qu.getuval("start_time");
            else
                info["run_time"] = 0;
            info["last_hello_time"] = uitodate(qu.getuval("last_hello_time"));
            info["lib_ver_num"] = uitodate(qu.getuval("lib_ver_num"));
            info["cfg_version"] = qu.getstr("cfg_version");
            info["build_version"] = qu.getstr("build_version");
            info["rep_status"] = ReportStatus(qu.getuval("last_attr_time"),
                qu.getuval("last_log_time"), qu.getuval("last_hello_time"), stConfig);
            js["list"].Add(info);
        }
        js["count"] = qu.num_rows();
    }
    else {
        js["count"] = 0;
    }
    qu.free_result();

    std::string str(js.ToString());
    hdf_set_value(stConfig.cgi->hdf, "config.machine_plugins", str.c_str());
	DEBUG_LOG("show status machine:%d, plugins:%s", id, str.c_str());
    return 0;
}

static int GetMachineListFromShm(Json &js, int iCurPage, int iNumPerPage)
{
	int iStart = iNumPerPage*(iCurPage-1);
	int iStartIdx = stConfig.stUser.pSysInfo->iMachineListIndexStart;
	SharedHashTableNoList *pMachHash = slog.GetMachineHash();
	MachineInfo *pInfo = NULL;
	for(int i=0; i < iStart && i < stConfig.stUser.pSysInfo->wMachineCount; i++)
	{
		pInfo = (MachineInfo*)NOLIST_HASH_INDEX_TO_NODE(pMachHash, iStartIdx);
		iStartIdx = pInfo->iNextIndex;
	}

	int iCount = 0;
	for(int i=iStart; i < iStart+iNumPerPage && i < stConfig.stUser.pSysInfo->wMachineCount; i++)
	{
		iCount++;
		pInfo = (MachineInfo*)NOLIST_HASH_INDEX_TO_NODE(pMachHash, iStartIdx);
		iStartIdx = pInfo->iNextIndex;
		Json info;
		info["id"] = pInfo->id;

		const char *pvname = NULL;
		if(pInfo->iNameVmemIdx > 0)
			pvname = MtReport_GetFromVmem_Local(pInfo->iNameVmemIdx);
		if(pvname == NULL)
			info["name"] = " ";
		else
			info["name"] = pvname;

		pvname = NULL;
		if(pInfo->iDescVmemIdx > 0)
			pvname = MtReport_GetFromVmem_Local(pInfo->iDescVmemIdx);
		if(pvname == NULL)
			info["desc"] = " ";
		else
			info["desc"] = pvname;

		info["ip"] = ipv4_addr_str(pInfo->ip1);
		if(pInfo->ip2 != 0)
			info["ip2"] = ipv4_addr_str(pInfo->ip2);

		info["model"] = pInfo->bModelId;
		info["warn_flag"] = pInfo->bWarnFlag;
        info["rep_status"] = GetMachineRepStatus(pInfo, stConfig);
		js["list"].Add(info);
	}
	js["count"] = iCount;
	DEBUG_LOG("get machine info from shm, count:%d", iCount);
	return 0;
}

static int GetMachineList(Json &js, SearchInfo *pinfo=NULL)
{
	char sSqlBuf[512] = {0};
	int iCurPage = hdf_get_int_value(stConfig.cgi->hdf, "config.currentPage", 0);
	int iNumPerPage = hdf_get_int_value(stConfig.cgi->hdf, "config.numPerPage", 0);
	if(iCurPage == 0 || iNumPerPage == 0)
	{
		ERR_LOG("invalid iCurPage(%d) or iNumPerPage(%d)", iCurPage, iNumPerPage);
		return SLOG_ERROR_LINE;
	}

	int iFilter = 0;
	sprintf(sSqlBuf, "select xrk_id from mt_machine where xrk_status=%d", RECORD_STATUS_USE);
	if(pinfo != NULL && (iFilter=AddSearchInfo(sSqlBuf, sizeof(sSqlBuf), pinfo)) < 0)
		return SLOG_ERROR_LINE;

	SetMachineInfo();

	// 未设置查询条件，从共享内存获取
	if(iFilter <= 0 && !stConfig.iDisableVmemCache) {
		int iRet = GetMachineListFromShm(js, iCurPage, iNumPerPage);
		if(iRet <= 0)
			return iRet;
	}

	Query & qu = *stConfig.qu;
	char sTmpBuf[64]={0};
	sprintf(sTmpBuf, " limit %d,%d", iNumPerPage*(iCurPage-1), iNumPerPage);
	strcat(sSqlBuf, sTmpBuf);

	qu.get_result(sSqlBuf);
	int i=0;
	MachineInfo *pInfo = NULL;
	const char *pvname = NULL;
	for(i=0; i < qu.num_rows() && qu.fetch_row() != NULL; i++)
	{
		Json info;
		info["id"] = qu.getuval("xrk_id");

		pInfo = slog.GetMachineInfo((int)(info["id"]), NULL);
        if(!pInfo)
            continue;

		pvname = NULL;
		if(pInfo->iNameVmemIdx > 0)
			pvname = MtReport_GetFromVmem_Local(pInfo->iNameVmemIdx);
		if(pvname == NULL)
			info["name"] = " ";
		else
			info["name"] = pvname;

		pvname = NULL;
		if(pInfo->iDescVmemIdx > 0)
			pvname = MtReport_GetFromVmem_Local(pInfo->iDescVmemIdx);
		if(pvname == NULL)
			info["desc"] = " ";
		else
			info["desc"] = pvname;

		info["ip"] = ipv4_addr_str(pInfo->ip1);
		if(pInfo->ip2 != 0)
			info["ip2"] = ipv4_addr_str(pInfo->ip2);

		info["model"] = pInfo->bModelId;
		info["warn_flag"] = pInfo->bWarnFlag;
        info["rep_status"] = GetMachineRepStatus(pInfo, stConfig);
		js["list"].Add(info);
	}
	js["count"] = i; 
	DEBUG_LOG("get machine list - result count:%d(%d)", qu.num_rows(), i);
	qu.free_result();

	return 0;
}

static int DealMachineLookUp()
{
	char sSqlBuf[512] = {0};
	Query & qu = *stConfig.qu;

	sprintf(sSqlBuf, "select * from mt_machine where xrk_status=%d", RECORD_STATUS_USE);
	qu.get_result(sSqlBuf);
	Json js;
	int i=0;
	for(i=0; i < qu.num_rows() && qu.fetch_row() != NULL; i++)
	{
		Json attr;
		attr["id"] = qu.getuval("xrk_id");
		attr["name"] = qu.getstr("xrk_name");
		attr["ip"] = ipv4_addr_str(qu.getuval("ip1"));
		attr["model"] = qu.getval("model_id");
		attr["warn_flag"] = qu.getval("warn_flag");
		attr["machine_desc"] = qu.getstr("machine_desc");
		js["list"].Add(attr);
	}
	qu.free_result();
	js["count"] = i; 
	
	std::string str(js.ToString());
	DEBUG_LOG("lookup machine, json:%s", str.c_str());

	stConfig.err = hdf_set_value(stConfig.cgi->hdf, "config.machine_info", str.c_str()); 
	if(stConfig.err != STATUS_OK)
	{
		ERR_LOG("set machine info failed, length:%lu", str.size());
		return SLOG_ERROR_LINE;
	}
	SetMachineInfo();
	return 0;
}

static int DealSysSrvList()
{
	FloginInfo *pUserInfo = stConfig.stUser.puser_info;
	if(pUserInfo->bLoginType != 1)
	{
		ERR_LOG("access deny for user:%s to list system server!", pUserInfo->szUserName);
		stConfig.pErrMsg = CGI_ACCESS_DENY;
		return SLOG_ERROR_LINE;
	}
	
	int iSetId = hdf_get_int_value(stConfig.cgi->hdf, "Query.set", 0);
	char sSqlBuf[256] = {0};
	Query & qu = *stConfig.qu;
	if(iSetId == 0)
		sprintf(sSqlBuf, "select * from mt_server where xrk_status=%d ", RECORD_STATUS_USE);
	else
		sprintf(sSqlBuf, "select * from mt_server where xrk_status=%d and `set_id`=%d ",
			RECORD_STATUS_USE, iSetId);

	int iOrder = 0;
	iOrder = (iOrder==0 ? SetRecordsOrder(stConfig.cgi, sSqlBuf, "xrk_id") : 1);
	iOrder = (iOrder==0 ? SetRecordsOrder(stConfig.cgi, sSqlBuf, "ip") : 1);
	iOrder = (iOrder==0 ? SetRecordsOrder(stConfig.cgi, sSqlBuf, "xrk_type") : 1);
	iOrder = (iOrder==0 ? SetRecordsOrder(stConfig.cgi, sSqlBuf, "weight") : 1);
	iOrder = (iOrder==0 ? SetRecordsOrder(stConfig.cgi, sSqlBuf, "update_time") : 1);
	if(iOrder == 0) 
		strcat(sSqlBuf, " order by xrk_type desc");

	qu.get_result(sSqlBuf);
	Json js;
	js["statusCode"] = 200;
	int i = 0;
	if(qu.num_rows() > 0) 
	{
		while(qu.fetch_row() != NULL)
		{
			Json srv;
			srv["id"] = qu.getuval("xrk_id"); 
			srv["ip"] = qu.getstr("ip");
			srv["port"] = qu.getuval("xrk_port"); 
			srv["type"] = qu.getuval("xrk_type"); 
			srv["sand_box"] = qu.getval("sand_box");
			srv["region"] = qu.getval("region");
			srv["idc"] = qu.getval("idc");
			srv["srv_for"] = qu.getstr("srv_for");
			srv["cfg_seq"] = qu.getval("cfg_seq");
			srv["user_add"] = qu.getstr("user_add");
			srv["user_mod"] = qu.getstr("user_mod");
			srv["create_time"] = uitodate(qu.getuval("create_time"));
			srv["update_time"] = qu.getstr("update_time");
			srv["weight"] = qu.getuval("weight");
			srv["desc"] = qu.getstr("m_desc");
			js["syssrv_list"].Add(srv);
			i++;
		}
	}
	js["syssrv_count"] = i;
	qu.free_result();

	std::string str(js.ToString());
	DEBUG_LOG("sys server list count:%d json:%s", i, str.c_str());

	stConfig.err = hdf_set_value(stConfig.cgi->hdf, "config.syssrv_list", str.c_str()); 
	if(stConfig.err != STATUS_OK)
	{
		ERR_LOG("set system server list info failed, length:%lu", str.size());
		return SLOG_ERROR_LINE;
	}
	return 0;
}

static int DealAddSysSrv()
{
	hdf_set_value(stConfig.cgi->hdf, "config.action", "save_add_syssrv");
	return 0;
}

static int DealSaveSysSrv(bool bIsAdd=false)
{
	if(IsDaemonDenyOp(stConfig))
		return 0;

	const char *ip = hdf_get_value(stConfig.cgi->hdf, "Query.ddassc_ip", NULL);
	int32_t iType = hdf_get_int_value(stConfig.cgi->hdf, "Query.ddassc_type", 0);
	int32_t iPort = hdf_get_int_value(stConfig.cgi->hdf, "Query.ddassc_port", 0);
	int32_t iWeight = hdf_get_int_value(stConfig.cgi->hdf, "Query.ddassc_weight", 0);
	const char *pfor = hdf_get_value(stConfig.cgi->hdf, "Query.ddassc_srv_for", NULL);
	const char *pnav = hdf_get_value(stConfig.cgi->hdf, "Query.navTabId", NULL);
	int32_t id = hdf_get_int_value(stConfig.cgi->hdf, "Query.ddassc_id", 0);
	int32_t iSandBox = hdf_get_int_value(stConfig.cgi->hdf, "Query.ddassc_sand_box", 0);
	const char *pdesc = hdf_get_value(stConfig.cgi->hdf, "Query.ddassc_desc", NULL);

	FloginInfo *pUserInfo = stConfig.stUser.puser_info;
	IM_SQL_PARA* ppara = NULL;
	if(InitParameter(&ppara) < 0) {
		ERR_LOG("sql parameter init failed !");
		return SLOG_ERROR_LINE;
	}
	
	if(bIsAdd) {
		AddParameter(&ppara, "xrk_type", iType, "DB_CAL");
	}
	AddParameter(&ppara, "ip", ip, NULL);
	AddParameter(&ppara, "xrk_port", iPort, "DB_CAL");
	AddParameter(&ppara, "weight", iWeight, "DB_CAL");
	AddParameter(&ppara, "sand_box", iSandBox, "DB_CAL");
	AddParameter(&ppara, "cfg_seq", stConfig.dwCurTime, "DB_CAL");
	if(pfor != NULL && pfor[0] != '\0')
		AddParameter(&ppara, "srv_for", pfor, NULL);
	else
		AddParameter(&ppara, "srv_for", "", NULL);
	if(pdesc != NULL && pdesc[0] != '\0')
		AddParameter(&ppara, "m_desc", pdesc, NULL);
	else
		AddParameter(&ppara, "m_desc", "", NULL);

	std::string strSql;
	Query & qu = *stConfig.qu;
	if(bIsAdd) {
		AddParameter(&ppara, "update_time", uitodate(stConfig.dwCurTime), NULL);
		AddParameter(&ppara, "create_time", stConfig.dwCurTime, "DB_CAL");
		AddParameter(&ppara, "user_add", pUserInfo->szUserName, NULL);
		AddParameter(&ppara, "user_mod", pUserInfo->szUserName, NULL);
		strSql = "replace into mt_server";
		MYSQL *m_pstDBlink = qu.GetMysql();
		JoinParameter_Insert(&strSql, m_pstDBlink, ppara);
	}
	else {
		AddParameter(&ppara, "user_mod", pUserInfo->szUserName, NULL);
		strSql = "update mt_server set";
		JoinParameter_Set(&strSql, qu.GetMysql(), ppara);
		strSql += " where xrk_id=";
		strSql += itoa(id);
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
	DEBUG_LOG("%s mt_server success - ip:%s type:%d for:%s",
		bIsAdd ? "add" : "mod", ip, iType, pfor);
	return 0;
}

static int DealModSysSrv()
{
	int32_t srvid = hdf_get_int_value(stConfig.cgi->hdf, "Query.srv_id", 0);
	if(srvid <= 0) {
		WARN_LOG("invalid parameter srvid:%d", srvid);
		stConfig.pErrMsg = CGI_REQERR;
		return SLOG_ERROR_LINE;
	}

	char sSqlBuf[128] = {0};
	Query & qu = *stConfig.qu;
	sprintf(sSqlBuf, "select * from mt_server where xrk_status=%d and xrk_id=%d", RECORD_STATUS_USE, srvid);
	qu.get_result(sSqlBuf);
	if(qu.num_rows() > 0) 
	{
		qu.fetch_row();
		hdf_set_int_value(stConfig.cgi->hdf, "config.config_type", qu.getval("xrk_type")); 
		hdf_set_value(stConfig.cgi->hdf, "config.config_ip", qu.getstr("ip")); 
		hdf_set_value(stConfig.cgi->hdf, "config.config_desc", qu.getstr("m_desc")); 
		hdf_set_int_value(stConfig.cgi->hdf, "config.config_port", qu.getval("xrk_port")); 
		hdf_set_int_value(stConfig.cgi->hdf, "config.config_set_id", qu.getval("set_id")); 
		hdf_set_int_value(stConfig.cgi->hdf, "config.config_weight", qu.getval("weight")); 
		hdf_set_value(stConfig.cgi->hdf, "config.config_srv_for", qu.getstr("srv_for")); 
		hdf_set_int_value(stConfig.cgi->hdf, "config.sand_box", qu.getval("sand_box")); 
		DEBUG_LOG("init modify server:%s type:%d set:%d id:%d for:%s", 
			qu.getstr("ip"), qu.getval("type"), qu.getval("set_id"), srvid, qu.getstr("srv_for"));
	}
	else {
		WARN_LOG("not find server by id:%d", srvid);
		stConfig.pErrMsg = CGI_REQERR;
		qu.free_result();
		return SLOG_ERROR_LINE;
	}
	qu.free_result();
	hdf_set_int_value(stConfig.cgi->hdf, "config.config_id", srvid);
	hdf_set_value(stConfig.cgi->hdf, "config.action", "save_mod_syssrv");
	return 0;
}

static int DealDelSysSrv()
{
	if(IsDaemonDenyOp(stConfig))
		return 0;

	int32_t srvid = hdf_get_int_value(stConfig.cgi->hdf, "Query.id", 0);
	if(srvid <= 0) {
		WARN_LOG("invalid parameter srvid:%d", srvid);
		stConfig.pErrMsg = CGI_REQERR;
		return SLOG_ERROR_LINE;
	}

	char sSqlBuf[128] = {0};
	Query & qu = *stConfig.qu;
	sprintf(sSqlBuf, "update mt_server set xrk_status=%d where xrk_id=%d", RECORD_STATUS_DELETE, srvid);
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

	DEBUG_LOG("change mt_server info status to:%d - id:%d success ", RECORD_STATUS_DELETE, srvid);
	return 0;
}

static int DeleteMachine()
{
	if(IsDaemonDenyOp(stConfig))
		return 0;

	int id = hdf_get_int_value(stConfig.cgi->hdf, "Query.id", 0);
	if(id == 0)
	{
		WARN_LOG("invalid parameter(id:%d) from:%s", id, stConfig.remote);
		hdf_set_value(stConfig.cgi->hdf,"err.msg", CGI_REQERR);
		return SLOG_ERROR_LINE;
	}

	static char sSqlBuf[128];

	DeleteMachineFromShm(id);

	sprintf(sSqlBuf, "update mt_machine set xrk_status=%d, user_mod_id=%d where xrk_id=%d",
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

	DEBUG_LOG("delete machine id:%d success, sql:%s, response string :%s to remote:%s",
		id, sSqlBuf, js.ToString().c_str(), stConfig.remote);
	return 0;
}

static int SaveMachine(bool bIsMod=false)
{
	if(IsDaemonDenyOp(stConfig))
		return 0;

	int32_t id = hdf_get_int_value(stConfig.cgi->hdf, "Query.dmm_machine_id", 0);
	int32_t iWarnFlag = hdf_get_int_value(stConfig.cgi->hdf, "Query.dmm_machine_warn_flag", 0);
	const char *ip1 = hdf_get_value(stConfig.cgi->hdf, "Query.dmm_machine_ip", NULL);
	const char *ip2 = hdf_get_value(stConfig.cgi->hdf, "Query.dmm_machine_ip2", NULL);
	const char *ip3 = hdf_get_value(stConfig.cgi->hdf, "Query.dmm_machine_ip3", NULL);
	const char *ip4 = hdf_get_value(stConfig.cgi->hdf, "Query.dmm_machine_ip4", NULL);
	const char *pname = hdf_get_value(stConfig.cgi->hdf, "Query.dmm_machine_name", NULL);
	const char *pdesc = hdf_get_value(stConfig.cgi->hdf, "Query.dmm_machine_desc", NULL);
	const char *pnavTabId = hdf_get_value(stConfig.cgi->hdf, "Query.navTabId", "mt_machine");
	int32_t iModel = MACHINE_MODEL_USER_CREATE;

	if((bIsMod && id == 0) || INVALID_MACHINE_WARN_FLAG(iWarnFlag) 
		|| ip1==NULL || inet_addr(ip1)==INADDR_NONE || pname==NULL || pname[0] == '\0'){ 
		hdf_set_value(stConfig.cgi->hdf,"err.msg", CGI_REQERR);
		REQERR_LOG("bMod:%d invalid param iWarnFlag:%d ip1:%s ip2:%s ip3:%s ip4:%s pname:%s pdesc:%s",
			bIsMod, iWarnFlag, ip1, ip2, ip3, ip4, pname, pdesc);
		return SLOG_ERROR_LINE;
	}

	// ip 重复配置检查
	static char szErrMsg[256] = {0};
	stConfig.pErrMsg = (const char*)szErrMsg;
	std::set<uint32_t> ipcheck;
	uint32_t ip_addr = 0;
	MachineInfo *pMachCheck = NULL;
	MachineInfo stMachInfoToShm;

#define IP_CONFIG_CHECK_RE(ip) \
	if((ip_addr=inet_addr(ip)) != INADDR_NONE) { \
		if(ipcheck.find(ip_addr) != ipcheck.end()) { \
			REQERR_LOG("add/mod machine failed, ip:%u, %s repeated", ip_addr, ip); \
			snprintf(szErrMsg, sizeof(szErrMsg), CGI_ERROR_ALREADY_HAS_MACHINE, ip); \
			return SLOG_ERROR_LINE; \
		} \
		ipcheck.insert(ip_addr); \
		if((pMachCheck=slog.GetMachineInfoByIp((char*)ip)) != NULL && id != pMachCheck->id) { \
			REQERR_LOG("add/mod machine failed, ip:%s is in, machine id is:%d", ip, pMachCheck->id); \
			snprintf(szErrMsg, sizeof(szErrMsg), CGI_ERROR_ALREADY_HAS_MACHINE, ip); \
			return SLOG_ERROR_LINE; \
		}\
	}
	IP_CONFIG_CHECK_RE(ip1);
	IP_CONFIG_CHECK_RE(ip2);
	IP_CONFIG_CHECK_RE(ip3);
	IP_CONFIG_CHECK_RE(ip4);

	memset(&stMachInfoToShm, 0, sizeof(stMachInfoToShm));

	uint32_t ip_addr_1 = inet_addr(ip1);
	IM_SQL_PARA* ppara = NULL;
	if(InitParameter(&ppara) < 0) {
		ERR_LOG("sql parameter init failed !");
		return SLOG_ERROR_LINE;
	}

	AddParameter(&ppara, "xrk_name", pname, NULL);
	if((ip_addr=inet_addr(ip2)) != INADDR_NONE) {
		AddParameter(&ppara, "ip2", ip_addr, "DB_CAL");
		stMachInfoToShm.ip2 = ip_addr;
	}
	else
		AddParameter(&ppara, "ip2", (uint32_t)0, "DB_CAL");
	if((ip_addr=inet_addr(ip3)) != INADDR_NONE) {
		AddParameter(&ppara, "ip3", ip_addr, "DB_CAL");
		stMachInfoToShm.ip3 = ip_addr;
	}
	else
		AddParameter(&ppara, "ip3", (uint32_t)0, "DB_CAL");
	if((ip_addr=inet_addr(ip4)) != INADDR_NONE) {
		AddParameter(&ppara, "ip4", ip_addr, "DB_CAL");
		stMachInfoToShm.ip4 = ip_addr;
	}
	else
		AddParameter(&ppara, "ip4", (uint32_t)0, "DB_CAL");

	Query & qu = *stConfig.qu;
	qu.execute("START TRANSACTION");

	if(!bIsMod)
	{
		AddParameter(&ppara, "user_add", stConfig.stUser.puser, NULL);
		AddParameter(&ppara, "create_time", uitodate(stConfig.dwCurTime), NULL);
		AddParameter(&ppara, "user_mod_id", stConfig.stUser.puser_info->iUserId, "DB_CAL");
		AddParameter(&ppara, "user_add_id", stConfig.stUser.puser_info->iUserId, "DB_CAL");
		AddParameter(&ppara, "ip1", ip_addr_1, "DB_CAL");
		stMachInfoToShm.ip1 = ip_addr_1;
	}
	else
	{
		AddParameter(&ppara, "user_mod_id", stConfig.stUser.puser_info->iUserId, "DB_CAL");
		AddParameter(&ppara, "user_mod", stConfig.stUser.puser, NULL);
	}
	AddParameter(&ppara, "machine_desc", pdesc, NULL);
	AddParameter(&ppara, "warn_flag", iWarnFlag, "DB_CAL");
	stMachInfoToShm.bWarnFlag = iWarnFlag;

	AddParameter(&ppara, "model_id", iModel, "DB_CAL");
	stMachInfoToShm.bModelId = iModel;

	std::string strSql;

	if(!bIsMod)
		strSql = "replace into mt_machine";
	else
		strSql = "update mt_machine set";

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
		if(!bIsMod)
			qu.execute("ROLLBACK");
		return SLOG_ERROR_LINE;
	}

	uint32_t dwIsFind = 0;
	if(!bIsMod) {
		id = qu.insert_id();

		// 系统ID 限制
		if(id >= 1000000) 
			ERR_LOG("machine auto id over limit:1000000, id:%d", id);

		MachineInfo *pInfo = slog.GetMachineInfo(id, &dwIsFind);
		if(pInfo != NULL && !dwIsFind) {
			stMachInfoToShm.id = id;
			stMachInfoToShm.iNameVmemIdx = MtReport_SaveToVmem(pname, strlen(pname)+1);
			if(pdesc != NULL && pdesc[0] != '\0')
				stMachInfoToShm.iDescVmemIdx = MtReport_SaveToVmem(pdesc, strlen(pdesc)+1);
			else
				stMachInfoToShm.iDescVmemIdx = -1;
			memcpy(pInfo, &stMachInfoToShm, sizeof(stMachInfoToShm));

			// 插入 mtsystemconfig 
			SharedHashTableNoList *pMachHash = slog.GetMachineHash();
			int iHashIndex = NOLIST_HASH_NODE_TO_INDEX(pMachHash, pInfo);
			if(stConfig.stUser.pSysInfo->wMachineCount == 0) {
				ILINK_SET_FIRST(stConfig.stUser.pSysInfo, iMachineListIndexStart,
					iMachineListIndexEnd, pInfo, iPreIndex, iNextIndex, iHashIndex);
				INFO_LOG("add machine info to shm, machine id:%u - first machine", id);
				stConfig.stUser.pSysInfo->wMachineCount = 1;
			}
			else {
				MachineInfo *pInfoFirst = (MachineInfo*)NOLIST_HASH_INDEX_TO_NODE(
					pMachHash, stConfig.stUser.pSysInfo->iMachineListIndexStart);
				ILINK_INSERT_FIRST(stConfig.stUser.pSysInfo, 
					iMachineListIndexStart, pInfo, iPreIndex, iNextIndex, pInfoFirst, iHashIndex);
				stConfig.stUser.pSysInfo->wMachineCount++;
				INFO_LOG("add machine info to shm, machine id:%d - machine count:%d",
					id, stConfig.stUser.pSysInfo->wMachineCount);
			}
		}
		else {
			WARN_LOG("add machine to shm - %p|%u",  pInfo, dwIsFind);
		}
	}
	else {
		MachineInfo *pInfo = slog.GetMachineInfo(id, &dwIsFind);
		if(pInfo != NULL) {
			if(pInfo->ip2 != stMachInfoToShm.ip2)
				pInfo->ip2 = stMachInfoToShm.ip2;
			if(pInfo->ip3 != stMachInfoToShm.ip3)
				pInfo->ip3 = stMachInfoToShm.ip3;
			if(pInfo->ip4 != stMachInfoToShm.ip4)
				pInfo->ip4 = stMachInfoToShm.ip4;
			if(pInfo->bWarnFlag != stMachInfoToShm.bWarnFlag)
				pInfo->bWarnFlag = stMachInfoToShm.bWarnFlag;

			const char *pvtmp = NULL;
			if(pInfo->iNameVmemIdx > 0)
				pvtmp = MtReport_GetFromVmem_Local(pInfo->iNameVmemIdx);
			if(pvtmp == NULL || strcmp(pvtmp, pname))
			{
				MtReport_FreeVmem(pInfo->iNameVmemIdx);
				pInfo->iNameVmemIdx = MtReport_SaveToVmem(pname, strlen(pname)+1);
			}

			pvtmp = NULL;
			if(pInfo->iDescVmemIdx > 0)
				pvtmp = MtReport_GetFromVmem_Local(pInfo->iDescVmemIdx);
			if(pdesc != NULL && (pvtmp==NULL || strcmp(pdesc, pvtmp)))
			{
				MtReport_FreeVmem(pInfo->iDescVmemIdx);
				pInfo->iDescVmemIdx = MtReport_SaveToVmem(pdesc, strlen(pdesc)+1);
			}
		}
		else {
			WARN_LOG("not find machine, when update machine:%d", id);
		}
	}

	qu.execute("COMMIT");
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
	DEBUG_LOG("%s mt_machine name:%s(id:%d) success, sql:%s, response string :%s to remote:%s ",
		(bIsMod ? "update" : "insert"), pname, id, strSql.c_str(), js.ToString().c_str(), stConfig.remote);
	return 0;
}

static int DealModMachine()
{
	int id = hdf_get_int_value(stConfig.cgi->hdf, "Query.id", 0);
	if(id==0)
	{
		WARN_LOG("invalid parameter from:%s", stConfig.remote);
		hdf_set_value(stConfig.cgi->hdf, "err.msg", CGI_REQERR);
		return SLOG_ERROR_LINE;
	}

	char sSqlBuf[128] = {0};
	sprintf(sSqlBuf, "select * from mt_machine where xrk_status=%d and xrk_id=%d", RECORD_STATUS_USE, id);
	Query & qu = *stConfig.qu;
	qu.get_result(sSqlBuf);
	
	if(qu.num_rows() <= 0){
		WARN_LOG("have no machine id:%d", id);
		hdf_set_value(stConfig.cgi->hdf, "err.msg", CGI_REQERR);
		return SLOG_ERROR_LINE;
	}

	int i=0;
	for(i=0; i < qu.num_rows() && qu.fetch_row() != NULL; i++)
	{
		hdf_set_value(stConfig.cgi->hdf, "machine.dmm_machine_id", qu.getstr("xrk_id"));
		hdf_set_value(stConfig.cgi->hdf, "machine.dmm_machine_name", qu.getstr("xrk_name"));
		hdf_set_value(stConfig.cgi->hdf, "machine.dmm_machine_ip", ipv4_addr_str(qu.getuval("ip1")));
		if(qu.getuval("ip2") != 0)
			hdf_set_value(stConfig.cgi->hdf, "machine.dmm_machine_ip2", ipv4_addr_str(qu.getuval("ip2")));
		if(qu.getuval("ip3") != 0)
			hdf_set_value(stConfig.cgi->hdf, "machine.dmm_machine_ip3", ipv4_addr_str(qu.getuval("ip3")));
		if(qu.getuval("ip4") != 0)
			hdf_set_value(stConfig.cgi->hdf, "machine.dmm_machine_ip4", ipv4_addr_str(qu.getuval("ip4")));
		hdf_set_value(stConfig.cgi->hdf, "machine.dmm_machine_model", qu.getstr("model_id"));
		hdf_set_value(stConfig.cgi->hdf, "machine.dmm_machine_warn_flag", qu.getstr("warn_flag"));
		hdf_set_value(stConfig.cgi->hdf, "machine.dmm_machine_desc", qu.getstr("machine_desc"));
	}
	SetMachineInfo();
	qu.free_result();
	hdf_set_value(stConfig.cgi->hdf, "config.action", "save_mod_machine");
	DEBUG_LOG("try update machine:%d ip:%s from:%s", id, qu.getstr("ip1"), stConfig.remote);
	return 0;
}

static int DealAddMachine()
{
	hdf_set_value(stConfig.cgi->hdf, "config.action", "save_add_machine");
	SetMachineInfo();
	return 0;
}

static int DealMachineSearch()
{
	SearchInfo stInfo;
	memset(&stInfo, 0, sizeof(stInfo));
	stInfo.pkey = hdf_get_value(stConfig.cgi->hdf, "Query.dm_keyword", NULL);
	stInfo.id = hdf_get_int_value(stConfig.cgi->hdf, "Query.dm_machine_id", 0);
	stInfo.ip = hdf_get_value(stConfig.cgi->hdf, "Query.dm_machine_ip", NULL);
	stInfo.model = hdf_get_int_value(stConfig.cgi->hdf, "Query.dm_machine_model_list", 0);
	stInfo.warn_flag = hdf_get_int_value(stConfig.cgi->hdf, "Query.dm_machine_warn_flag_list", 0);

	DEBUG_LOG("search info key:%s id:%d ip:%s model:%d warn_flag:%d",
		stInfo.pkey, stInfo.id, stInfo.ip, stInfo.model, stInfo.warn_flag);

	int iRecords = GetMachineTotalRecords(&stInfo);
	if(iRecords < 0)
	{
		ERR_LOG("get attr record count failed !");
		return SLOG_ERROR_LINE;
	}
	SetRecordsPageInfo(stConfig.cgi, iRecords);

	Json js;
	if(GetMachineList(js, &stInfo) < 0)
		return SLOG_ERROR_LINE;
	std::string str_attr(js.ToString());
	DEBUG_LOG("machine list json:%s", str_attr.c_str());
	stConfig.err = hdf_set_value(stConfig.cgi->hdf, "config.machine_info", str_attr.c_str()); 
	if(stConfig.err != STATUS_OK)
	{
		ERR_LOG("set attr type info failed, length:%lu", str_attr.size());
		return SLOG_ERROR_LINE;
	}

	return 0;
}

static int DealListMachine()
{
	int iRecords = GetMachineTotalRecords();
	if(iRecords < 0)
	{
		ERR_LOG("get attr record count failed !");
		return SLOG_ERROR_LINE;
	}
	SetRecordsPageInfo(stConfig.cgi, iRecords);

	Json js;
	if(GetMachineList(js) < 0)
		return SLOG_ERROR_LINE;
	std::string str(js.ToString());
	DEBUG_LOG("machine list json:%s", str.c_str());

	stConfig.err = hdf_set_value(stConfig.cgi->hdf, "config.machine_info", str.c_str()); 
	if(stConfig.err != STATUS_OK)
	{
		ERR_LOG("set machine info failed, length:%lu", str.size());
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

	int32_t iRet = 0;
	if((iRet=slog.InitConfigByFile(myConf.szConfigFile)) < 0 || (iRet=slog.Init(myConf.szLocalIp)) < 0)
		return SLOG_ERROR_LINE;

	if(slog.InitMachineList() < 0)
	{
		ERR_LOG("init machine list shm failed !");
		return SLOG_ERROR_LINE;
	}

	myConf.pAppInfo = slog.GetAppInfo();
	if(myConf.pAppInfo == NULL)
	{
		FATAL_LOG("get pAppInfo:%p failed !", myConf.pAppInfo);
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
		printf("InitCgi failed ! ret:%d", iRet);
		return -1;
	}

	// 
	if(AfterCgiInit(stConfig) <= 0)
		return SLOG_ERROR_LINE;

	INFO_LOG("fcgi:%s argc:%d start pid:%u", stConfig.pszCgiName, argc, stConfig.pid);
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

		const char *pAction = stConfig.pAction;
		DEBUG_LOG("get action :%s from :%s", pAction, stConfig.remote);

		if(DealDbConnect(stConfig) < 0) {
			show_errpage(NULL, CGI_ERR_SERVER, stConfig);
			continue;
		}

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

        if(!strcmp(pAction, "show_status"))
            iRet = DealShowMachineStatus();
		else if(!strcmp(pAction, "add"))
			iRet = DealAddMachine();
		else if(!strcmp(pAction, "delete"))
			iRet = DeleteMachine();
		else if(!strcmp(pAction, "mod"))
			iRet = DealModMachine();
		else if(!strcmp(pAction, "save_add_machine"))
			iRet = SaveMachine();
		else if(!strcmp(pAction, "save_mod_machine"))
			iRet = SaveMachine(true);
		else if(!strcmp(pAction, "search"))
			iRet = DealMachineSearch();
		else if(!strcmp(pAction, "lookUpMachine"))
			iRet = DealMachineLookUp();
		else if(!strcmp(pAction, "list_sys_srv"))
			iRet = DealSysSrvList();
		else if(!strcmp(pAction, "add_syssrv"))
			iRet = DealAddSysSrv();
		else if(!strcmp(pAction, "save_add_syssrv"))
			iRet = DealSaveSysSrv(true);
		else if(!strcmp(pAction, "mod_syssrv"))
			iRet = DealModSysSrv();
		else if(!strcmp(pAction, "save_mod_syssrv"))
			iRet = DealSaveSysSrv(false);
		else if(!strcmp(pAction, "delete_syssrv"))
			iRet = DealDelSysSrv();
		else if(!strcmp(pAction, "machine_add_plugin"))
			iRet = DealMachineAddPlugin();
		else  // default -- list
		{
			pAction = "list";
			iRet = DealListMachine();
		}

		const char *pcsTemplate = NULL;
        if(!strcmp(pAction, "show_status"))
			pcsTemplate = "dmt_dlg_machine_status.html";
		else if(!strcmp(pAction, "add") || !strcmp(pAction, "mod"))
			pcsTemplate = "dmt_machine_manage.html";
		else if(!strcmp(pAction, "list") || !strcmp(pAction, "search"))
			pcsTemplate = "dmt_machine.html";
		else if(!strcmp(pAction, "lookUpMachine"))
			pcsTemplate = "dmt_look_machine.html";
		else if(!strcmp(pAction, "list_sys_srv"))
			pcsTemplate = "dmt_sys_srv.html";
		else if(!strcmp(pAction, "add_syssrv") || !strcmp(pAction, "mod_syssrv"))
			pcsTemplate = "dmt_dlg_add_sys_srv_config.html";
		else if(!strcmp(pAction, "machine_add_plugin"))
			pcsTemplate = "dmt_machine_add_plugin.html";

		if(iRet < 0)
		{
			if(pcsTemplate)
				stConfig.iResponseType = RESPONSE_TYPE_HTML;
			else
				stConfig.iResponseType = RESPONSE_TYPE_JSON;
			show_errpage(NULL, CGI_REQERR, stConfig);
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

	if(stConfig.cgi != NULL)
		DealCgiFailedExit(stConfig, stConfig.err);

	stConfig.dwEnd = time(NULL);
	INFO_LOG("fcgi - %s stop at:%u run:%u pid:%u errmsg:%s",
		stConfig.pszCgiName, stConfig.dwEnd, stConfig.dwEnd - stConfig.dwStart, stConfig.pid, strerror(errno));
	return 0;
}

