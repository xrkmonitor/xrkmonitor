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
    
   fastcgi mt_slog_showview: 监控系统视图展示相关

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
#include <cgi_attr.h>
#include <sstream>
#include "Memcache.h"

#define DB_ATTR "attr_db"
#define SHOW_ATTR_PER_PAGE 24 

CSupperLog slog;
SLogServer* g_pHttpTestServer = NULL;
bool g_isShowViewAttr = false;
CGIConfig stConfig;

// ajax json 响应方式
static const char *s_JsonRequest [] = { 
	"show_machine_attr_cust",
	"show_single_attr_cust",
	"show_view_attr_cust",
	"save_not_bind_machine",
	"save_bind_machine",
	"deal_multi_warns",
	"get_attr_type_attrs",
	"send_test_attr",
	NULL
};

// 属性展示图表类型 --
#define ATTR_SHOW_TYPE_MACHINE 1
#define ATTR_SHOW_TYPE_VIEW 2

std::map<const char *, Database*> g_mapDb;

class CDbConnect
{
	public:
		static void PingAllAttrDb() {
			std::map<const char *, Database*>::iterator it = g_mapDb.begin();
			for(; it != g_mapDb.end();)
			{
				Query qu(*(it->second));
				if(!qu.Connected()) {
					delete it->second;
					g_mapDb.erase(it++);
				}
				else 
					it++;
			}
		}

		CDbConnect(const char* ip) : pqu(NULL)
		{
			std::map<const char *, Database*>::iterator it = g_mapDb.find(ip);
			if(it != g_mapDb.end())
			{
				pqu = new Query(*(it->second));
				if(false == pqu->Connected())
				{
					delete pqu;
					pqu = NULL;
					delete it->second;
					g_mapDb.erase(it);
				}
				else
				{
					DEBUG_LOG("get db:%s connect from pool map", ip);
					return;
				}
			}

			if(NULL == pqu)
			{
				Database *pdb = new Database(
					ip, stConfig.pShmConfig->stSysCfg.szUserName, 
					stConfig.pShmConfig->stSysCfg.szPass, DB_ATTR, &slog, stConfig.pShmConfig->stSysCfg.iDbPort);
				pqu = new Query(*pdb);
				if(pdb && pqu && pqu->Connected())
				{
					INFO_LOG("connect to attr db %s ok", ip);
					g_mapDb[ip] = pdb;
				}
				else
				{
					if(pdb)
						delete pdb;
					if(pqu) {
						delete pqu;
						pqu = NULL;
					}
					WARN_LOG("connect to attr db %s failed", ip);
				}
			}
		}

		Query *GetQuery() {
			return pqu;
		}
	
		~CDbConnect() {
			if(pqu)
				delete pqu;
		}

	private:
		Query *pqu;
};

typedef struct
{
	int iMachineId;
	int iViewId;
	int iDealStatus;
	int iWarnObj;
	int iWarnType;
	int iAttrId;
}WarnListSearchInfo;

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

static bool IsPercentAttr(int id)
{
	if(id >= 163 && id <= 179)
		return true;
	if(id == 184 || id == 183 || id == 189)
		return true;
	return false;
}

static int GetWarnList(Json & js, WarnListSearchInfo *pinfo=NULL, int iTotal=0);
static int AddWarnInfoSearch(char *psql, int ibufLen, WarnListSearchInfo *pinfo)
{
	char sTmpBuf[128] = {0};
	int iFilter = 0;
	if(pinfo->iMachineId != 0)
	{
		sTmpBuf[0] = '\0';
		sprintf(sTmpBuf, " and warn_id=%d ", pinfo->iMachineId);
		strcat(psql, sTmpBuf);
		hdf_set_int_value(stConfig.cgi->hdf, "config.dwl_machine_id", pinfo->iMachineId);
		iFilter++;
	}
	if(pinfo->iViewId != 0)
	{
		sTmpBuf[0] = '\0';
		sprintf(sTmpBuf, " and warn_id=%d ", pinfo->iViewId);
		strcat(psql, sTmpBuf);
		hdf_set_int_value(stConfig.cgi->hdf, "config.dwl_view_id", pinfo->iViewId);
		iFilter++;
	}

	if(pinfo->iWarnObj != 0)
	{
		sTmpBuf[0] = '\0';
		sprintf(sTmpBuf, " and (warn_flag&%d) ", pinfo->iWarnObj);
		strcat(psql, sTmpBuf);
		hdf_set_int_value(stConfig.cgi->hdf, "config.dwl_warn_obj", pinfo->iWarnObj);
		hdf_set_int_value(stConfig.cgi->hdf, "config.dwl_warn_obj_sel", pinfo->iWarnObj);
		iFilter++;
	}

	if(pinfo->iWarnType != 0)
	{
		sTmpBuf[0] = '\0';
		sprintf(sTmpBuf, " and (warn_flag&%d) ", pinfo->iWarnType);
		strcat(psql, sTmpBuf);
		hdf_set_int_value(stConfig.cgi->hdf, "config.dwl_warn_type", pinfo->iWarnType);
		hdf_set_int_value(stConfig.cgi->hdf, "config.dwl_warn_type_sel", pinfo->iWarnType);
		iFilter++;
	}

	if(pinfo->iDealStatus != -1)
	{
		sTmpBuf[0] = '\0';
		sprintf(sTmpBuf, " and deal_status=%d ", pinfo->iDealStatus);
		strcat(psql, sTmpBuf);
		hdf_set_int_value(stConfig.cgi->hdf, "config.dwl_deal_status", pinfo->iDealStatus);
		hdf_set_int_value(stConfig.cgi->hdf, "config.dwl_deal_status_sel", pinfo->iDealStatus);
		iFilter++;
	}

	if(pinfo->iAttrId != 0)
	{
		sTmpBuf[0] = '\0';
		sprintf(sTmpBuf, " and attr_id=%d ", pinfo->iAttrId);
		strcat(psql, sTmpBuf);
		hdf_set_int_value(stConfig.cgi->hdf, "config.dwl_attr_id", pinfo->iAttrId);
		iFilter++;
	}

	DEBUG_LOG("after add search info sql:%s, filter:%d", psql, iFilter);
	return iFilter;
}

static int SaveNotBindMachine(int view_id)
{
	if(IsDaemonDenyOp(stConfig))
		return 0;

	int32_t machine_id = hdf_get_int_value(stConfig.cgi->hdf, "Query.machine_id", 0);
	if(machine_id == 0)
	{
		hdf_set_value(stConfig.cgi->hdf,"err.msg", CGI_REQERR);
		REQERR_LOG("invalid param view id:%d machine id:%d", view_id, machine_id);
		return SLOG_ERROR_LINE;
	}

	Query & qu = *stConfig.qu;
	char sSqlBuf[256] = {0};
	sprintf(sSqlBuf, "update mt_view_bmach set status=%d where view_id=%d and machine_id=%d",
		RECORD_STATUS_DELETE, view_id, machine_id);
	if(!qu.execute(sSqlBuf))
	{
		ERR_LOG("execute sql:%s failed, msg:%s", sSqlBuf, qu.GetError().c_str());
		return SLOG_ERROR_LINE;
	}
	DEBUG_LOG("delete bind machine - view id:%d machine id:%d success", view_id, machine_id);
	slog.DelViewBindMach(view_id, machine_id);

	Json js;
	js["statusCode"] = 200;
	js["callbackType"] = "closeCurrent";
	js["msgid"] = "notBindSuccess";

	STRING str;
	string_init(&str);
	if((stConfig.err=string_set(&str, js.ToString().c_str())) != STATUS_OK
		|| (stConfig.err=cgi_output(stConfig.cgi, &str)) != STATUS_OK)
	{
		string_clear(&str);
		return SLOG_ERROR_LINE;
	}
	string_clear(&str);
	DEBUG_LOG("delete bind machine response string:%s", js.ToString().c_str());
	return 0;
}

static int SaveBindMachine(int view_id)
{
	if(IsDaemonDenyOp(stConfig))
		return 0;

	int32_t machine_id = hdf_get_int_value(stConfig.cgi->hdf, "Query.machine_id", 0);
	if(machine_id == 0)
	{
		hdf_set_value(stConfig.cgi->hdf,"err.msg", CGI_REQERR);
		REQERR_LOG("invalid param view id:%d machine id:%d", view_id, machine_id);
		return SLOG_ERROR_LINE;
	}

	// check exist
	if(NULL==slog.GetMachineInfo(machine_id, NULL))
	{
		REQERR_LOG("get machine id:%d failed !", machine_id);
		return SLOG_ERROR_LINE;
	}

	Query & qu = *stConfig.qu;

	IM_SQL_PARA* ppara = NULL;
	InitParameter(&ppara);
	AddParameter(&ppara, "view_id", view_id, "DB_CAL");
	AddParameter(&ppara, "machine_id", machine_id, "DB_CAL");
	AddParameter(&ppara, "status", (uint32_t)0, "DB_CAL");
	AddParameter(&ppara, "update_time", uitodate(stConfig.dwCurTime), NULL);
	std::string strSql;
	strSql = "replace into mt_view_bmach";
	JoinParameter_Insert(&strSql, qu.GetMysql(), ppara);
	ReleaseParameter(&ppara);

	qu.free_result();
	if(!qu.execute(strSql))
	{
		ERR_LOG("execute sql:%s failed, msg:%s", strSql.c_str(), qu.GetError().c_str());
		return SLOG_ERROR_LINE;
	}
	DEBUG_LOG("bind machine id:%d for view:%d success", machine_id, view_id);
	slog.AddViewBindMach(view_id, machine_id);

	Json js;
	js["statusCode"] = 200;
	js["callbackType"] = "closeCurrent";
	js["msgid"] = "bindSuccess";

	STRING str;
	string_init(&str);
	if((stConfig.err=string_set(&str, js.ToString().c_str())) != STATUS_OK
		|| (stConfig.err=cgi_output(stConfig.cgi, &str)) != STATUS_OK)
	{
		string_clear(&str);
		return SLOG_ERROR_LINE;
	}
	string_clear(&str);
	DEBUG_LOG("bind machine response string:%s", js.ToString().c_str());
	return 0;
}

typedef struct
{
	int iAttrId;
	int iAttrDataType;
	int iAttrType;
	const char *pattrType;
	bool bByPage; // true 分页获取, false 拉取全部
}AttrSearchInfo;

static int GetAttrInfoFromShm(int iAttrId, Json & attr);
static int GetBindAttrTotalRecords(int view_id, AttrSearchInfo *pinfo)
{
	int idx = slog.GetViewInfoIndex(view_id, NULL);
	if(idx < 0) {
		WARN_LOG("get view info failed, view:%d", view_id);
		return 0;
	}
	TViewInfo *pViewInfo = slog.GetViewInfo(idx);
	int iCount = 0;
	for(int i=0; i < pViewInfo->bBindAttrCount; i++)
	{
		Json attr;
		if(GetAttrInfoFromShm(pViewInfo->aryBindAttrs[i], attr) < 0)
			continue;
		if(pinfo != NULL && pinfo->iAttrDataType != 0 && pinfo->iAttrDataType != (int)attr["data_type"])
			continue;
		if(pinfo != NULL && pinfo->iAttrType != 0 && pinfo->iAttrType != (int)attr["attr_type"])
			continue;
		if(pinfo != NULL && pinfo->iAttrId != 0 && pinfo->iAttrId != (int)attr["id"])
		    continue;
		iCount++;
	}
	DEBUG_LOG("get view:%d bind attr count:%d", view_id, iCount);
	return iCount;
}

static int GetBindAttrList(int view_id, Json &js, AttrSearchInfo *pinfo)
{
	int iCurPage = hdf_get_int_value(stConfig.cgi->hdf, "config.currentPage", 1);
	int iNumPerPage = hdf_get_int_value(stConfig.cgi->hdf, "config.numPerPage", 10);

	int idx = slog.GetViewInfoIndex(view_id, NULL);
	if(idx < 0) {
		WARN_LOG("get view info failed, view:%d", view_id);
		return 0;
	}
	TViewInfo *pViewInfo = slog.GetViewInfo(idx);
	int iCount = 0, iPageCount = 0;
	for(int i=0; i < pViewInfo->bBindAttrCount; i++)
	{
		Json attr;
		if(GetAttrInfoFromShm(pViewInfo->aryBindAttrs[i], attr) < 0)
			continue;
		if(pinfo != NULL && pinfo->iAttrDataType != 0 && pinfo->iAttrDataType != (int)attr["data_type"])
			continue;
		if(pinfo != NULL && pinfo->iAttrType != 0 && pinfo->iAttrType != (int)attr["attr_type"])
			continue;
		if(pinfo != NULL && pinfo->iAttrId != 0 && pinfo->iAttrId != (int)attr["id"])
		    continue;
		if(pinfo == NULL 
			|| (pinfo != NULL && !pinfo->bByPage)
			|| (pinfo != NULL && pinfo->bByPage && iCount >= iNumPerPage*(iCurPage-1)))
		{
			iPageCount++;
			js["list"].Add(attr);
		}
		if(pinfo != NULL && pinfo->bByPage && iPageCount >= iNumPerPage)
			break;
		iCount++;
	}

	if(pinfo != NULL && pinfo->iAttrDataType != 0)
	{
		// 参数重新设置回去，以便排序显示时可以按结果排序
		hdf_set_int_value(stConfig.cgi->hdf, "config.dsba_attr_data_type", pinfo->iAttrDataType);
	}

	if(pinfo != NULL && pinfo->iAttrType != 0 && pinfo->pattrType != NULL)
	{
		hdf_set_int_value(stConfig.cgi->hdf, "config.dam_attr_type", pinfo->iAttrType);
		hdf_set_value(stConfig.cgi->hdf, "config.dam_attr_type_name", pinfo->pattrType);
	}

	if(pinfo != NULL && pinfo->iAttrId != 0)
	{
	    hdf_set_int_value(stConfig.cgi->hdf, "config.dam_attr_id", pinfo->iAttrId);
	}

	js["count"] = iPageCount; 
	js["view_id"] = view_id;
	DEBUG_LOG("get bind attr list - total:%d|%d, result count:%d", pViewInfo->bBindAttrCount, iCount, iPageCount);
	return 0;
}

static int GetAttrType(Json &js, MmapUserAttrTypeTree & stTypeTree)
{
	int iType = stTypeTree.attr_type_id();
	const char *pTypeName = NULL;
	char sType[16] = {0};
	snprintf(sType, sizeof(sType), "%d", iType);

	// 优先尝试 vmem
	if(!stConfig.iDisableVmemCache)
		pTypeName = GetAttrTypeNameFromVmem(iType, stConfig);
	if(pTypeName != NULL)
	{
		DEBUG_LOG("get attr type from vmem - type:%d, name:%s", iType, pTypeName);
		js[(const char*)sType] = pTypeName;
	}
	else 
	{
		char sSqlBuf[256] = {0};
		Query qu(*stConfig.db);
		sprintf(sSqlBuf, "select name from mt_attr_type where type=%d and status=0", iType);
		qu.get_result(sSqlBuf);
		if(qu.num_rows() <= 0 || qu.fetch_row() == NULL)
		{
			ERR_LOG("get attr type :%d failed!", iType);
			qu.free_result();
			return SLOG_ERROR_LINE;
		}

		if(qu.num_rows() > 1)
			WARN_LOG("get attr type :%d, have count:%d, use first !", iType, qu.num_rows());
		const char *pname = qu.getstr("name");

		if(!stConfig.iDisableVmemCache)
			SetAttrTypeNameToVmem(iType, pname, stConfig);

		js[(const char*)sType] = pname;
		DEBUG_LOG("get attr type :%d name:%s from db", iType, pname);
		qu.free_result();
	}

	for(int i=0; i < stTypeTree.sub_type_list_size(); i++)
	{
		MmapUserAttrTypeTree *pType = stTypeTree.mutable_sub_type_list(i);
		if(GetAttrType(js, *pType) < 0)
			return SLOG_ERROR_LINE;
	}
	return 0;
}

static int CreateAttrTypeTree(MmapUserAttrTypeTree & stTypeTree) 
{
	char sSqlBuf[256] = {0};
	Query qu(*stConfig.db);

	// 尝试获取子类型
	sprintf(sSqlBuf, 
		"select type from mt_attr_type where status=0 and parent_type=%d",
		stTypeTree.attr_type_id());
	qu.get_result(sSqlBuf);
	for(int i=0; i < qu.num_rows() && qu.fetch_row() != NULL; i++)
	{
		MmapUserAttrTypeTree *pType = stTypeTree.add_sub_type_list();
		pType->set_attr_type_id(qu.getval("type"));
		if(CreateAttrTypeTree(*pType) < 0)
		{
			qu.free_result();
			return SLOG_ERROR_LINE;
		}
	}
	qu.free_result();
	return 0;
}

static int GetAttrTypeTree(MmapUserAttrTypeTree & stTypeTree)
{
	int iVmemIdx = stConfig.stUser.pSysInfo->iAttrTypeTreeVmemIdx;
	if(iVmemIdx > 0)
	{
		char szShmValBuf[1024] = {0};
		int iShmValBufLen = sizeof(szShmValBuf);
		const char *pbuf = MtReport_GetFromVmemZeroCp(iVmemIdx, szShmValBuf, &iShmValBufLen);
		if(pbuf != NULL && iShmValBufLen > 0) {
			if(!stTypeTree.ParseFromArray(pbuf, iShmValBufLen)) {
				WARN_LOG("stTypeTree ParseFromArray failed, len:%d", iShmValBufLen);
			}
			else {
				DEBUG_LOG("get type tree info:%s", stTypeTree.ShortDebugString().c_str());
				return 0;
			}
		}
	}

	stTypeTree.set_attr_type_id(1);
	if(CreateAttrTypeTree(stTypeTree) < 0)
		return SLOG_ERROR_LINE;
	DEBUG_LOG("create attr type tree info:%s", stTypeTree.ShortDebugString().c_str());

	std::string strval;
	if(!stTypeTree.AppendToString(&strval))
	{
	    WARN_LOG("AppendToString failed !");
	}
	else 
	{
		int iNewIdx = MtReport_SaveToVmem(strval.c_str(), strval.size());
		if(iNewIdx >= 0) {
			DEBUG_LOG("put attr type tree to vmem ok, vmem idx:%d|%d, info:%s", 
				iNewIdx, iVmemIdx, stTypeTree.ShortDebugString().c_str());

			// 删除老的存储
			MtReport_FreeVmem(iVmemIdx);
			stConfig.stUser.pSysInfo->iAttrTypeTreeVmemIdx = iNewIdx;
		}
		else {
			WARN_LOG("save attr type tree failed, size:%d", (int)strval.size());
		}
	}
	return 0;
}

static int SetAttrTypeInfo()
{
	// vmem user attr type
	MmapUserAttrTypeTree stTypeTree;
	if(GetAttrTypeTree(stTypeTree) != 0)
	{
		ERR_LOG("GetAttrTypeTree failed");
		return SLOG_ERROR_LINE;
	}

	// for vmem 
	Json js;
	if(GetAttrType(js, stTypeTree) < 0)
		return SLOG_ERROR_LINE;
	std::string str(js.ToString());
	DEBUG_LOG("attr type list json:%s", str.c_str());
	stConfig.err = hdf_set_value(stConfig.cgi->hdf, "config.attr_type_list", str.c_str()); 
	if(stConfig.err != STATUS_OK)
	{
		ERR_LOG("set attr type info failed");
		return SLOG_ERROR_LINE;
	}
	return 0;
}

static int DealBindAttrList(int view_id, AttrSearchInfo *pshInfo=NULL)
{
	AttrSearchInfo stInfo;
	memset(&stInfo, 0, sizeof(stInfo));
	stInfo.bByPage = true;
	if(pshInfo == NULL)
		pshInfo = &stInfo;

	int iRecords = GetBindAttrTotalRecords(view_id, pshInfo);
	if(iRecords < 0)
	{
		ERR_LOG("get bind attr record count failed !");
		return SLOG_ERROR_LINE;
	}
	SetRecordsPageInfo(stConfig.cgi, iRecords, 10);

	hdf_set_int_value(stConfig.cgi->hdf, "config.sum_report_m", SUM_REPORT_M);
	hdf_set_int_value(stConfig.cgi->hdf, "config.sum_report_min", SUM_REPORT_MIN);
	hdf_set_int_value(stConfig.cgi->hdf, "config.str_report_d", STR_REPORT_D);
	hdf_set_int_value(stConfig.cgi->hdf, "config.sum_report_his", SUM_REPORT_TOTAL);
	hdf_set_int_value(stConfig.cgi->hdf, "config.sum_report_max", SUM_REPORT_MAX);
	hdf_set_int_value(stConfig.cgi->hdf, "config.ex_report", EX_REPORT);

	Json js_bind_attr;
	if(GetBindAttrList(view_id, js_bind_attr, pshInfo) < 0)
		return SLOG_ERROR_LINE;
	std::string str_attr(js_bind_attr.ToString());
	hdf_set_value(stConfig.cgi->hdf, "config.attr_list", str_attr.c_str()); 

	DEBUG_LOG("bind attr list json:%s", str_attr.c_str());
	return 0;
}

static TViewInfo * GetMachineBindList(Json &js, int32_t view_id)
{
	int idx = slog.GetViewInfoIndex(view_id, NULL);
	if(idx < 0)
	{
		js["count"] = 0;
		WARN_LOG("get view info failed, view:%d", view_id);
		return NULL;
	}
	TViewInfo *pinfo = slog.GetViewInfo(idx);
	MachineInfo *pmach = NULL;
	const char *pname = NULL;

	if(pinfo != NULL && (pinfo->bViewFlag & VIEW_FLAG_AUTO_BIND_MACHINE))
	{
		hdf_set_int_value(stConfig.cgi->hdf, "config.auto_bind_machine", 1);
		hdf_set_int_value(stConfig.cgi->hdf, "config.bind_machine_dlg_layout", 50);
		hdf_set_value(stConfig.cgi->hdf, "config.btn_disabled", "disabled");
	}
	else
	{
		hdf_set_int_value(stConfig.cgi->hdf, "config.auto_bind_machine", 0);
		hdf_set_int_value(stConfig.cgi->hdf, "config.bind_machine_dlg_layout", 35);
	}

	int i=0;
	for(i=0; i < pinfo->bBindMachineCount; i++)
	{
		pmach = slog.GetMachineInfo(pinfo->aryBindMachines[i], NULL);
		if(pmach == NULL)
		{
			WARN_LOG("get machine failed, id:%d", pinfo->aryBindMachines[i]);
			continue;
		}

		Json attr;
		attr["id"] = pinfo->aryBindMachines[i];

		pname = MtReport_GetFromVmem_Local(pmach->iNameVmemIdx);
		if(pname == NULL)
			attr["name"] = "unknow";
		else 
			attr["name"] = pname;
		attr["ip1"] = ipv4_addr_str(pmach->ip1);
		js["list"].Add(attr);
	}
	js["count"] = i; 
	DEBUG_LOG("get bind machine list for view:%d - result count:%d", view_id, pinfo->bBindMachineCount);
	return pinfo;
}

static int GetMachineList(Json &js_bind, Json &js_not_bind)
{
	const char *pname = NULL;
	int idx = stConfig.stUser.pSysInfo->iMachineListIndexStart;
	MachineInfo *pmach = NULL;
	Json::json_list_t::const_iterator it;
	const Json::json_list_t & jslist = js_bind["list"].GetArray(); 
	int iCount = 0;
	for(int i=0; i < stConfig.stUser.pSysInfo->wMachineCount; i++)
	{
		Json mach;
		pmach = slog.GetMachineInfo(idx);
		idx = pmach->iNextIndex;
		if(pmach == NULL) {
			WARN_LOG("get machine info failed, index:%d", idx);
			break;
		}

		// 是否已绑
		for(it = jslist.begin(); it != jslist.end(); it++)
		{
			const Json &mach = *it;
			if((int)(mach["id"]) == pmach->id)
				break;
		}
		if(it != jslist.end())
			continue;

		mach["id"] = pmach->id;
		pname = MtReport_GetFromVmem_Local(pmach->iNameVmemIdx);
		if(pname != NULL)
			mach["name"] = pname;
		else {
			mach["name"] = "unknow";
			WARN_LOG("get machine name failed, machine:%d, name idx:%d", pmach->id, pmach->iNameVmemIdx);
		}
		js_not_bind["list"].Add(mach);
		iCount++;
	}
	js_not_bind["count"] = iCount; 
	DEBUG_LOG("get not bind machine list - result count:%d(%d)", iCount, stConfig.stUser.pSysInfo->wMachineCount);
	return 0;
}

static int DealBindMachine(int view_id)
{
	Json js_bind;
	GetMachineBindList(js_bind, view_id);

	Json js_not_bind;
	if(GetMachineList(js_bind, js_not_bind) < 0)
		return SLOG_ERROR_LINE;

	std::string str_not_bind(js_not_bind.ToString());
	std::string str_bind(js_bind.ToString());
	DEBUG_LOG("bind machine list json:%s", str_bind.c_str());
	DEBUG_LOG("not bind machine list json:%s", str_not_bind.c_str());
	hdf_set_value(stConfig.cgi->hdf, "config.bind_machine_list", str_bind.c_str()); 
	hdf_set_value(stConfig.cgi->hdf, "config.not_bind_machine_list", str_not_bind.c_str()); 
	return 0;
}

static int GetAttrTotalRecords(int view_id, AttrSearchInfo *pinfo=NULL)
{
	char sSqlBuf[512] = {0};
	char sTmpBuf[128] = {0};
	Query & qu = *stConfig.qu;

	sprintf(sSqlBuf, "select count(*) from mt_attr where id not in (select attr_id from mt_view_battr "
		"where view_id=%d and status=%d) and status=%d", 
		view_id, RECORD_STATUS_USE, RECORD_STATUS_USE);
	if(pinfo != NULL && pinfo->iAttrDataType != 0)
	{
		sprintf(sTmpBuf, " and mt_attr.data_type=%d", pinfo->iAttrDataType);
		strcat(sSqlBuf, sTmpBuf);
	}

	if(pinfo != NULL && pinfo->iAttrType != 0 && pinfo->pattrType != NULL)
	{
		sprintf(sTmpBuf, " and mt_attr.attr_type=%d", pinfo->iAttrType);
		strcat(sSqlBuf, sTmpBuf);
	}

	if(pinfo != NULL && pinfo->iAttrId != 0)
	{
	    sprintf(sTmpBuf, " and mt_attr.id=%d", pinfo->iAttrId);
	    strcat(sSqlBuf, sTmpBuf);
	}

	qu.get_result(sSqlBuf);
	if(qu.num_rows() <= 0 || qu.fetch_row() == NULL)
	{
		qu.free_result();
		WARN_LOG("get attr count failed or have no records");
		return 0;
	}
	DEBUG_LOG("get attr count - exesql:%s", sSqlBuf);

	int iCount = qu.getval(0);
	qu.free_result();
	DEBUG_LOG("records count:%d", iCount);
	return iCount;
}

static int GetAttrList(int view_id, Json &js, AttrSearchInfo *pinfo=NULL)
{
	char sSqlBuf[512] = {0};
	char sTmpBuf[128] = {0};
	Query & qu = *stConfig.qu;
	int iCurPage = hdf_get_int_value(stConfig.cgi->hdf, "config.currentPage", 1);
	int iNumPerPage = hdf_get_int_value(stConfig.cgi->hdf, "config.numPerPage", 10);

	sprintf(sSqlBuf, "select * from mt_attr where id not in (select attr_id from mt_view_battr "
		"where view_id=%d and status=%d) and status=%d", view_id, RECORD_STATUS_USE, RECORD_STATUS_USE);
	if(pinfo != NULL && pinfo->iAttrDataType != 0)
	{
		sprintf(sTmpBuf, " and mt_attr.data_type=%d", pinfo->iAttrDataType);
		strcat(sSqlBuf, sTmpBuf);

		// 参数重新设置回去，以便排序显示时可以按结果排序
		hdf_set_int_value(stConfig.cgi->hdf, "config.dsnba_attr_data_type", pinfo->iAttrDataType);
	}

	if(pinfo != NULL && pinfo->iAttrType != 0 && pinfo->pattrType != NULL)
	{
		sprintf(sTmpBuf, " and mt_attr.attr_type=%d", pinfo->iAttrType);
		strcat(sSqlBuf, sTmpBuf);
		hdf_set_int_value(stConfig.cgi->hdf, "config.dam_attr_type", pinfo->iAttrType);
		hdf_set_value(stConfig.cgi->hdf, "config.dam_attr_type_name", pinfo->pattrType);
	}

	if(pinfo != NULL && pinfo->iAttrId != 0)
	{
		sprintf(sTmpBuf, " and mt_attr.id=%d", pinfo->iAttrId);
		strcat(sSqlBuf, sTmpBuf);
		hdf_set_int_value(stConfig.cgi->hdf, "config.dam_attr_id", pinfo->iAttrId);
	}
	SetRecordsOrder(stConfig.cgi, sSqlBuf, "id");

	memset(sTmpBuf, 0, sizeof(sTmpBuf));
	sprintf(sTmpBuf, " limit %d,%d", iNumPerPage*(iCurPage-1), iNumPerPage);
	strcat(sSqlBuf, sTmpBuf);
	DEBUG_LOG("get attr list - exesql:%s", sSqlBuf);

	qu.get_result(sSqlBuf);
	if(qu.num_rows() < 0)
	{
		qu.free_result();
		ERR_LOG("get attr list failed!");
		return SLOG_ERROR_LINE;
	}

	int i=0;
	for(i=0; i < qu.num_rows() && qu.fetch_row() != NULL; i++)
	{
		Json attr;
		attr["id"] = qu.getval("id");
		attr["name"] = qu.getstr("attr_name");
		attr["attr_type"] = qu.getstr("attr_type");
		attr["data_type"] = qu.getval("data_type");
		js["list"].Add(attr);
	}
	js["count"] = i; 
	DEBUG_LOG("get attr list - result count:%d(%d)", qu.num_rows(), i);
	qu.free_result();
	return 0;
}

/*
   * 注意： 这个函数不需要输出属性类型列表了，默认打开 Bind attr 时已输出到页面
   */
static int DealNotBindAttrList(int view_id, AttrSearchInfo *pshInfo=NULL)
{
	Json js;

	int iRecords = GetAttrTotalRecords(view_id, pshInfo);
	if(iRecords < 0)
	{
		ERR_LOG("get attr record count failed !");
		return SLOG_ERROR_LINE;
	}
	SetRecordsPageInfo(stConfig.cgi, iRecords, 10);

	hdf_set_int_value(stConfig.cgi->hdf, "config.sum_report_m", SUM_REPORT_M);
	hdf_set_int_value(stConfig.cgi->hdf, "config.sum_report_min", SUM_REPORT_MIN);
	hdf_set_int_value(stConfig.cgi->hdf, "config.str_report_d", STR_REPORT_D);
	hdf_set_int_value(stConfig.cgi->hdf, "config.sum_report_his", SUM_REPORT_TOTAL);
	hdf_set_int_value(stConfig.cgi->hdf, "config.sum_report_max", SUM_REPORT_MAX);
	hdf_set_int_value(stConfig.cgi->hdf, "config.ex_report", EX_REPORT);

	Json js_attr;
	if(GetAttrList(view_id, js_attr, pshInfo) < 0)
		return SLOG_ERROR_LINE;

	std::string str_attr(js_attr.ToString());
	hdf_set_value(stConfig.cgi->hdf, "config.attr_list", str_attr.c_str()); 
	DEBUG_LOG("attr list json:%s", str_attr.c_str());
	return 0;
}

static int DealAttrSearch(int view_id, bool bIsBind=false)
{
	AttrSearchInfo stInfo;
	stInfo.bByPage = true;
	if(bIsBind)
		stInfo.iAttrDataType = hdf_get_int_value(stConfig.cgi->hdf, "Query.dsba_attr_data_type", 0);
	else
		stInfo.iAttrDataType = hdf_get_int_value(stConfig.cgi->hdf, "Query.dsnba_attr_data_type", 0);
	stInfo.iAttrId = hdf_get_int_value(stConfig.cgi->hdf, "Query.dam_attr_id", 0);
	stInfo.iAttrType = hdf_get_int_value(stConfig.cgi->hdf, "Query.dam_attr_type", 0);
	stInfo.pattrType = hdf_get_value(stConfig.cgi->hdf, "Query.dam_attr_type_name", NULL);

	DEBUG_LOG("search attr data type:%d attr type:%d(%s) attrid:%d, view id:%d bind:%s",
		stInfo.iAttrDataType, stInfo.iAttrType, stInfo.pattrType, stInfo.iAttrId, view_id, (bIsBind?"true":"false"));
	if(bIsBind)
		return DealBindAttrList(view_id, &stInfo);
	return DealNotBindAttrList(view_id, &stInfo);
}

static int SaveNotBindAttr(int view_id)
{
	if(IsDaemonDenyOp(stConfig))
		return 0;

	char sTmpStr[32] = {0};
	sprintf(sTmpStr, "Query.bind_attr_%d", view_id);
	const char *pattr = hdf_get_value(stConfig.cgi->hdf, sTmpStr, NULL);
	if(pattr == NULL)
	{
		hdf_set_value(stConfig.cgi->hdf,"err.msg", CGI_REQERR);
		REQERR_LOG("invalid param, attr list is NULL");
		return SLOG_ERROR_LINE;
	}

	char *pattr_list = strdup(pattr);
	char *pattr_id = NULL;
	char *psave = NULL;
	Query & qu = *stConfig.qu;

	char sSqlBuf[256] = {0};
	for(int32_t i=0; 1; i++, pattr_list=NULL)
	{
		pattr_id = strtok_r(pattr_list, ",", &psave);
		if(pattr_id == NULL)
			break;
		sprintf(sSqlBuf, "update mt_view_battr set status=%d where view_id=%d and attr_id=%s", 
			RECORD_STATUS_DELETE, view_id, pattr_id);
		if(!qu.execute(sSqlBuf))
		{
			ERR_LOG("execute sql:%s failed, msg:%s", sSqlBuf, qu.GetError().c_str());
			hdf_set_value(stConfig.cgi->hdf,"err.msg", CGI_ERR_SERVER);
			return SLOG_ERROR_LINE;
		}
		DEBUG_LOG("not bind(%d) attr id:%s for view:%d success", i, pattr_id, view_id);
		slog.DelViewBindAttr(view_id, atoi(pattr_id));
	}
	INFO_LOG("not bind attr:%s for view:%d", pattr, view_id);

	Json js;
	js["statusCode"] = 200;
	js["callbackType"] = "closeCurrent";
	js["msgid"] = "notBindSuccess";

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

static int SaveBindAttr(int view_id)
{
	if(IsDaemonDenyOp(stConfig))
		return 0;

	char sTmpStr[32] = {0};
	sprintf(sTmpStr, "Query.not_bind_attr_%d", view_id);
	const char *pattr = hdf_get_value(stConfig.cgi->hdf, sTmpStr, NULL);
	if(pattr == NULL)
	{
		hdf_set_value(stConfig.cgi->hdf,"err.msg", CGI_REQERR);
		REQERR_LOG("invalid param, attr list is NULL");
		return SLOG_ERROR_LINE;
	}

	char *pattr_list = strdup(pattr);
	char *pattr_id = NULL;
	char *psave = NULL;
	Query & qu = *stConfig.qu;

	char sSqlBuf[256] = {0};
	for(int32_t i=0; 1; i++, pattr_list=NULL)
	{
		pattr_id = strtok_r(pattr_list, ",", &psave);
		if(pattr_id == NULL)
			break;
		sprintf(sSqlBuf, "replace into mt_view_battr set view_id=%d,attr_id=%s,status=0,update_time=''%s\'",
			view_id, pattr_id, uitodate(stConfig.dwCurTime));
		if(!qu.execute(sSqlBuf))
		{
			ERR_LOG("execute sql:%s failed, msg:%s", sSqlBuf, qu.GetError().c_str());
			hdf_set_value(stConfig.cgi->hdf,"err.msg", CGI_ERR_SERVER);
			return SLOG_ERROR_LINE;
		}
		DEBUG_LOG("bind(%d) attr id:%s for view:%d success", i, pattr_id, view_id);
		slog.AddViewBindAttr(view_id, atoi(pattr_id));
	}
	INFO_LOG("bind attr:%s for view:%d", pattr, view_id);

	Json js;
	js["statusCode"] = 200;
	js["callbackType"] = "closeCurrent";
	js["msgid"] = "bindSuccess";

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

static void GetAttrTableNameForMonth(time_t tm, 
	char s_aryTableMonthDay[31][32],  uint32_t *pdwFirstMonthDayUtc, std::string * pstrMonthDays=NULL)
{
	char szTmpDay[31][20] = {0};
	struct tm curr = *localtime(&tm);

	int iEnd = (curr.tm_mday == 31 ? 30 : curr.tm_mday);
	int iMonth = curr.tm_mon;
	time_t tmTmp = tm;
	for(int i=iEnd; i >= 1; i--)
	{
		curr = *localtime(&tmTmp);
		if(curr.tm_year > 50)
		{
			snprintf(s_aryTableMonthDay[i-1], sizeof(s_aryTableMonthDay[0]), "attr_%04d%02d%02d",
				curr.tm_year+1900, curr.tm_mon+1, curr.tm_mday);
			snprintf(szTmpDay[i-1], sizeof(szTmpDay[0]), "%04d-%02d-%02d",
				curr.tm_year+1900, curr.tm_mon+1, curr.tm_mday);
		}
		else
		{
			snprintf(s_aryTableMonthDay[i-1], sizeof(s_aryTableMonthDay[0]), "attr_%04d%02d%02d",
				curr.tm_year+2000, curr.tm_mon+1, curr.tm_mday);
			snprintf(szTmpDay[i-1], sizeof(szTmpDay[0]), "%04d-%02d-%02d",
				curr.tm_year+2000, curr.tm_mon+1, curr.tm_mday);
		}
	
		if(i == 1 && pdwFirstMonthDayUtc != NULL)
			*pdwFirstMonthDayUtc = tmTmp;
		tmTmp -= 24*60*60;
	}

	tmTmp = tm;
	for(int i=iEnd+1; i <= 31; i++)
	{
		tmTmp += 24*60*60;
		curr = *localtime(&tmTmp);
		if(tmTmp >= stConfig.dwCurTime || curr.tm_mon != iMonth)
			break;
		if(curr.tm_year > 50)
		{
			snprintf(s_aryTableMonthDay[i-1], sizeof(s_aryTableMonthDay[0]), "attr_%04d%02d%02d",
				curr.tm_year+1900, curr.tm_mon+1, curr.tm_mday);
			snprintf(szTmpDay[i-1], sizeof(szTmpDay[0]), "%04d-%02d-%02d",
				curr.tm_year+1900, curr.tm_mon+1, curr.tm_mday);
		}
		else
		{
			snprintf(s_aryTableMonthDay[i-1], sizeof(s_aryTableMonthDay[0]), "attr_%04d%02d%02d",
				curr.tm_year+2000, curr.tm_mon+1, curr.tm_mday);
			snprintf(szTmpDay[i-1], sizeof(szTmpDay[0]), "%04d-%02d-%02d",
				curr.tm_year+2000, curr.tm_mon+1, curr.tm_mday);
		}
	}

	if(pstrMonthDays != NULL)
	{
		*pstrMonthDays = "";
		for(int i=0; i < 31; i++)
		{
			if(szTmpDay[i][0] != '\0')
				*pstrMonthDays += szTmpDay[i];
			else
				*pstrMonthDays += "0";
			if(i < 30)
				*pstrMonthDays += ",";
		}
	}
}

// 以 iWeekDayCur 为中心获取一周的 table name
static void GetAttrTableNameForWeek(
	time_t tm, char s_aryTableWeekDay[7][32], int & iWeekDayCur, uint32_t *pdwTimeMondayUtc=NULL,
	std::string * pstrWeekDays=NULL)
{
	char szTmpDay[7][20] = {0};
	struct tm curr = *localtime(&tm);

	int iEnd = (curr.tm_wday == 0 ? 7 : curr.tm_wday);
	iWeekDayCur = iEnd-1;
	time_t tmTmp = tm;
	for(int i=iEnd; i >= 1; i--)
	{
		curr = *localtime(&tmTmp);
		if(curr.tm_year > 50)
		{
			snprintf(s_aryTableWeekDay[i-1], sizeof(s_aryTableWeekDay[0]), "attr_%04d%02d%02d",
				curr.tm_year+1900, curr.tm_mon+1, curr.tm_mday);
			snprintf(szTmpDay[i-1], sizeof(szTmpDay[0]), "%04d-%02d-%02d",
				curr.tm_year+1900, curr.tm_mon+1, curr.tm_mday);
		}
		else
		{
			snprintf(s_aryTableWeekDay[i-1], sizeof(s_aryTableWeekDay[0]), "attr_%04d%02d%02d",
				curr.tm_year+2000, curr.tm_mon+1, curr.tm_mday);
			snprintf(szTmpDay[i-1], sizeof(szTmpDay[0]), "%04d-%02d-%02d",
				curr.tm_year+2000, curr.tm_mon+1, curr.tm_mday);
		}
	
		if(i == 1 && pdwTimeMondayUtc != NULL)
			*pdwTimeMondayUtc = tmTmp;
		tmTmp -= 24*60*60;
	}

	tmTmp = tm;
	for(int i=iEnd+1; i <= 7; i++)
	{
		tmTmp += 24*60*60;
		curr = *localtime(&tmTmp);
		if(tmTmp >= stConfig.dwCurTime)
			break;
		if(curr.tm_year > 50)
		{
			snprintf(s_aryTableWeekDay[i-1], sizeof(s_aryTableWeekDay[0]), "attr_%04d%02d%02d",
				curr.tm_year+1900, curr.tm_mon+1, curr.tm_mday);
			snprintf(szTmpDay[i-1], sizeof(szTmpDay[0]), "%04d-%02d-%02d",
				curr.tm_year+1900, curr.tm_mon+1, curr.tm_mday);
		}
		else
		{
			snprintf(s_aryTableWeekDay[i-1], sizeof(s_aryTableWeekDay[0]), "attr_%04d%02d%02d",
				curr.tm_year+2000, curr.tm_mon+1, curr.tm_mday);
			snprintf(szTmpDay[i-1], sizeof(szTmpDay[0]), "%04d-%02d-%02d",
				curr.tm_year+2000, curr.tm_mon+1, curr.tm_mday);
		}
	}

	if(pstrWeekDays != NULL)
	{
		*pstrWeekDays = "";
		for(int i=0; i < 7; i++)
		{
			if(szTmpDay[i][0] != '\0')
				*pstrWeekDays += szTmpDay[i];
			else
				*pstrWeekDays += "0";
			if(i < 6)
				*pstrWeekDays += ",";
		}
	}
}

static const char * GetAttrTableName(time_t tm)
{
	static char szTableName[32];
	struct tm curr = *localtime(&tm);
	if(curr.tm_year > 50)
	    snprintf(szTableName, sizeof(szTableName), "attr_%04d%02d%02d",
	        curr.tm_year+1900, curr.tm_mon+1, curr.tm_mday);
	else
	    snprintf(szTableName, sizeof(szTableName), "attr_%04d%02d%02d",
	        curr.tm_year+2000, curr.tm_mon+1, curr.tm_mday);
	return szTableName;
}

// 将日期格式：2014-01-30 20:43:21 转为 0-1439 分钟数
static int GetDayOfMin(const char *ptime=NULL)
{
	static char s_sLocalBuf[64];

	char *ptime_str = (char*)s_sLocalBuf;
	if(NULL == ptime) {
		ptime_str = uitodate(stConfig.dwCurTime);
	}
	else {
		ptime_str = (char*)s_sLocalBuf;
		strncpy(ptime_str, ptime, sizeof(s_sLocalBuf));
	}

	char *phour = strchr(ptime_str, ':');
	char *pmin = phour+1;
	if(phour == NULL)
		return 0;
	*phour = '\0';
	phour -= 2;
	pmin[2] = '\0';

	int i = atoi(phour)*60+atoi(pmin);
	if(i >= 1440)
		return 1439;
	if(i <= 0)
		return 0;
	return i;
}

static void InitGetEachMachAttrInfo(Json &js, Json & JsEachMach)
{
	Json::json_list_t & jslist = js["list"].GetArray();
	Json::json_list_t::iterator it = jslist.begin();
	for(; it != jslist.end(); it++)
	{
		(*it)["max"] = (uint32_t)0;
		(*it)["value_list_str"] = "";
		JsEachMach["list"].Add(*it);
	}
}

static void EndGetEachMachAttrInfo(Json & js)
{
	Json::json_list_t & jslist = js["list"].GetArray();
	Json::json_list_t::iterator it = jslist.begin();
	for(; it != jslist.end(); it++)
	{
		if((uint32_t)(*it)["max"] == 0)
			(*it)["value_list_str"] = "0";
	}
}

static int GetAttrDayVal(Json &js, Json &attr, const char *pattrTab, const Json &js_attr_info, int iCurMin, Query & qu)
{
	int attr_id = js_attr_info["id"];
	attr["max"] = 0U;
	attr["id"] = attr_id;

	char sSqlBuf[256] = {0};
	uint32_t iReport[1440] = {0};
	const Json::json_list_t & jslist = js["list"].GetArray();
	Json::json_list_t::const_iterator it = jslist.begin();
	uint32_t arydwValDay[1440] = {0};
	const char *pAttrTableToday = GetAttrTableName(stConfig.dwCurTime);

	bool bIsToday = false;
	if(!strcmp(pattrTab, pAttrTableToday))
		bIsToday = true;

	for(; it != jslist.end(); it++)
	{
		comm::MonitorMemcache memInfo;
		memInfo.Clear();
		slog.memcache.SetKey("machine-attr-val-%s-%d-%d-monitor", pattrTab, attr_id, (int)((*it)["id"]));

		// memcache 缓存只有当天的
		if(bIsToday && slog.memcache.GetMonitorMemcache(memInfo) >= 0)
		{
			for(int i=0; i < memInfo.machine_attr_day_val().attr_val_size(); i++)
			{
				if(memInfo.machine_attr_day_val().attr_val(i).idx() >= 1440)
				{
					ERR_LOG("invalid attr val idx:%d, key:%s", 
						memInfo.machine_attr_day_val().attr_val(i).idx(), slog.memcache.GetKey());
					continue;
				}
				iReport[memInfo.machine_attr_day_val().attr_val(i).idx()] +=
					memInfo.machine_attr_day_val().attr_val(i).val();
			}
			DEBUG_LOG("get attr:%d machine:%d value count:%d from memcache",
				attr_id, (int)((*it)["id"]), memInfo.machine_attr_day_val().attr_val_size());
		}
		// 查询的是当天的数据
		else if(bIsToday)
		{
			// 如果是视图的查询且启用了 memcache 缓存机制，则不查数据库
			if(g_isShowViewAttr && slog.IsEnableMemcache()) {
				DEBUG_LOG("skip get attr info, day:%s|%s, attrid:%d, machine:%d, memkey:%s", 
					pattrTab, pAttrTableToday, attr_id, (int)((*it)["id"]), slog.memcache.GetKey());
				continue;
			}

			sprintf(sSqlBuf, "select value,report_time from %s where attr_id=%d and machine_id=%d",
				pattrTab, attr_id, (int)((*it)["id"]));

			qu.get_result(sSqlBuf);
			int i=0;
			for(i=0; qu.fetch_row() != NULL; i++)
			{
				const char *prepTime = qu.getstr("report_time");
				int iDayOfMinTmp = GetDayOfMin(prepTime);
				iReport[iDayOfMinTmp] += qu.getval("value");
			}
			DEBUG_LOG("get attr:%d machine:%d value count:%d", attr_id, (int)((*it)["id"]), i);
			qu.free_result();
		}
		// 查询历史数据 -- 从 attr day table 中读取
		else
		{
			sprintf(sSqlBuf, "select * from %s_day where attr_id=%d and machine_id=%d",
				pattrTab, attr_id, (int)((*it)["id"]));
			MYSQL_RES *res = qu.GetBinaryResult(std::string(sSqlBuf));
			if(res && qu.num_rows() > 0)
			{
				qu.fetch_row();
				const char *pval = qu.getstr("value");
				memcpy((char*)arydwValDay, pval, sizeof(arydwValDay));
				uint32_t  uiMin=UINT_MAX, uiMax =0, uiTotal = 0;
				for(int i=0; i < 1440; i++)
				{
					uiTotal += arydwValDay[i];
					if(arydwValDay[i] > uiMax)
						uiMax = arydwValDay[i];
					if(arydwValDay[i] < uiMin && arydwValDay[i] != 0)
						uiMin = arydwValDay[i];
					iReport[i] += arydwValDay[i];
				}

				if(uiMin != qu.getuval("min") || uiMax != qu.getuval("max") || uiTotal != qu.getuval("total"))
				{
					WARN_LOG("check failed- min:%u|%u, max:%u|%u, total:%u|%u info|%d|%d|%s_day",
						uiMin, qu.getuval("min"), uiMax, qu.getuval("max"), uiTotal, qu.getuval("total"),
						attr_id, (int)((*it)["id"]), pattrTab);
				}

				DEBUG_LOG("get attr from day record -info attr:%d machine:%d min:%u, max:%u, total:%u",
					attr_id, (int)((*it)["id"]), uiMin, uiMax, uiTotal);
			}
			else
			{
				WARN_LOG("GetBinaryResult failed, sql:%s|%p|%d", sSqlBuf, res, qu.num_rows());
			}
			qu.free_result();
		}
	}

	uint32_t iValueMax = 0;
	uint32_t iValueMin = UINT_MAX;

	if(iCurMin < 0 || iCurMin >= 1440)
		iCurMin = 1440;
	
	stringstream strVal;

	// 历史累积监控点类型，中间无上报需要用历史数据填补
	uint32_t dwHisVal = 0;
	bool bIsHisTotalAttr = false;
	if((int)js_attr_info["data_type"] == SUM_REPORT_TOTAL)
		bIsHisTotalAttr = true;

	int iAvgVal = 0, iRepCount = 0;
	for(int i=0; i < iCurMin; i++)
	{
		// 历史累积数据，最新的永远是大于等于老的上报值
		if(bIsHisTotalAttr)
		{
			if(iReport[i] > dwHisVal)
				dwHisVal = iReport[i];
			else
				iReport[i] = dwHisVal;
		}

		if(iReport[i] > iValueMax)
			iValueMax = iReport[i];

		if(iReport[i] < iValueMin)
			iValueMin = iReport[i];

		strVal << iReport[i];
		if(i+1 < iCurMin)
			strVal << ",";
	}
	attr["value_list_str"] = strVal.str().c_str();

	if(iRepCount > 0)
		attr["avg"] = (int)(iAvgVal/iRepCount);
	else
		attr["avg"] = 0;

	attr["max"] = iValueMax;
	attr["min"] = iValueMin;

	// 当前时间分钟，显示上一分钟的数据上报，因为当前这一分钟可能有数据没收集完(图表按分钟级别显示)
	if(iCurMin >= 1)
		attr["cur"] = iReport[iCurMin-1];
	else
		attr["cur"] = iReport[iCurMin];

	attr["max_x"] = iCurMin;
	DEBUG_LOG("attr:%d max value:%d, cur:%d, curmin:%d", attr_id, iValueMax, (int)attr["cur"], iCurMin);
	return 0;
}

static int GetAttrDayVal(Json &js, Json &attr, 
	const char *pattrTab, const Json &js_attr_info, int iCurMin, Query & qu, Json & JsEachMach)
{
	const Json::json_list_t & jslist = js["list"].GetArray();
	Json::json_list_t::const_iterator it = jslist.begin();

	Json::json_list_t & jslistEach = JsEachMach["list"].GetArray();
	Json::json_list_t::iterator itEach = jslistEach.begin();

	int iMax = (iCurMin < 0 ? 1439 : iCurMin);

	uint32_t arydwValDay[1440] = {0};
	for(; it != jslist.end(); it++, itEach++)
	{
		Json jsTmp, attr_tmp;
		jsTmp["count"] = 1;
		jsTmp["list"].Add(*it);

		if(GetAttrDayVal(jsTmp, attr_tmp, pattrTab, js_attr_info, iCurMin, qu) < 0)
			return SLOG_ERROR_LINE;

		// 单机的 max, value_list_str
		if((uint32_t)(*itEach)["max"] < (uint32_t)attr_tmp["max"])
			(*itEach)["max"] = (uint32_t)attr_tmp["max"];
		std::string strVal((const char*)(*itEach)["value_list_str"]);
		strVal += (const char*)attr_tmp["value_list_str"];
		strVal += ",";
		(*itEach)["value_list_str"] = strVal.c_str();

		// 视图的 max, value_list_str --- 计算预处理
		char *pattr_val_list = strdup((const char*)attr_tmp["value_list_str"]);
		char *pfree_attr_val = pattr_val_list;
		char *pattr_val = NULL;
		char *psave = NULL;
		for(int i=0; i < iMax; i++, pattr_val_list=NULL)
		{
			pattr_val =  strtok_r(pattr_val_list, ",", &psave);
			if(pattr_val == NULL || pattr_val[0] == '\0')
				break;
			arydwValDay[i] += strtoul(pattr_val, NULL, 10);
		}
		free(pfree_attr_val);
	}

	stringstream strVal;
	uint32_t iValueMax = 0;
	uint32_t iValueMin = UINT_MAX;
	int iAvgVal = 0, iRepCount = 0;

	// 历史累积监控点类型，中间无上报需要用历史数据填补
	uint32_t dwHisVal = 0;
	bool bIsHisTotalAttr = false;
	if((int)js_attr_info["data_type"] == SUM_REPORT_TOTAL)
		bIsHisTotalAttr = true;

	for(int i=0; i < iMax; i++)
	{
		// 历史累积数据，最新的永远是大于等于老的上报值
		if(bIsHisTotalAttr)
		{
			if(arydwValDay[i] > dwHisVal)
				dwHisVal = arydwValDay[i];
			else
				arydwValDay[i] = dwHisVal;
		}

		if(arydwValDay[i] > iValueMax)
			iValueMax = arydwValDay[i];

		if(arydwValDay[i] < iValueMin)
			iValueMin = arydwValDay[i];
		strVal << arydwValDay[i];
		if(i+1 < iMax)
			strVal << ",";
	}
	attr["value_list_str"] = strVal.str().c_str();
	attr["max"] = iValueMax;
	attr["min"] = iValueMin;
	if(iRepCount > 0)
		attr["avg"] = (int)(iAvgVal/iRepCount);
	else
		attr["avg"] = 0;

	// 当前时间分钟，显示上一分钟的数据上报，因为当前这一分钟可能有数据没收集完(图表按分钟级别显示)
	if(iMax >= 1)
		attr["cur"] = arydwValDay[iMax-1];
	else
		attr["cur"] = 0;

	attr["max_x"] = iMax;
	return 0;	
}

static int GetAttrInfoFromShm(int iAttrId, Json & attr)
{
	AttrInfoBin *pInfo = slog.GetAttrInfo(iAttrId, NULL);
	int iNameIdx = 0;
	if(NULL != pInfo) {
		attr["id"] = iAttrId;
		attr["attr_type"] = pInfo->iAttrType;
		attr["data_type"] = pInfo->iDataType;
		iNameIdx = pInfo->iNameVmemIdx;
	}
	else {
		ERR_LOG("get attr info from shm failed, attr:%d, info failed", iAttrId);
		return SLOG_ERROR_LINE;
	}

	const char *pvname = NULL;
	if(iNameIdx > 0)
		pvname = MtReport_GetFromVmem_Local(iNameIdx);
	if(pvname != NULL)
		attr["name"] = pvname;
	else {
		attr["name"] = "unknow";
		WARN_LOG("get attr:%d name failed, nameidx:%d", iAttrId, iNameIdx);
	}

	if(IsPercentAttr(iAttrId))
		attr["show_class"] = "percent";
	else
		attr["show_class"] = "comm";
	DEBUG_LOG("get attr:%d, info from memcache/vmem", iAttrId);
	return 0;
}

// 获取单机有上报的属性
static int GetMachineAttrList(int machine_id, Json &js, const char *pszTableName)
{
	SLogServer *psrv = slog.GetValidServerByType(SRV_TYPE_ATTR_DB, NULL);
	if(NULL == psrv) {
		ERR_LOG("Get config server failed");
		return SLOG_ERROR_LINE;
	}

	// db 连接池使用
	const char *pserver_ip = psrv->szIpV4;
	if(slog.IsIpMatchLocalMachine(psrv->dwIp))
		pserver_ip = "127.0.0.1";
	CDbConnect db(pserver_ip);
	Query *pqu = db.GetQuery();
	if(NULL == pqu)
	{
		return SLOG_ERROR_LINE;
	}
	Query &qu = *pqu;

	// 获取机器有上报的属性列表 ----- first ---
	char sSqlBuf[1024] = {0};
	sprintf(sSqlBuf, "select distinct attr_id from %s where machine_id=%d", pszTableName, machine_id);
	strcat(sSqlBuf, " order by attr_id asc"); 

	qu.get_result(sSqlBuf);
	if(qu.num_rows() < 0)
	{
		qu.free_result();
		ERR_LOG("get machine attr list failed!");
		return SLOG_ERROR_LINE;
	}
	else if(qu.num_rows() == 0)
	{
		qu.free_result();
		js["count"] = 0; 
		js["machine_id"] = machine_id;
		return 0;
	}

	// 获取机器有上报属性的详细信息 ----- second ---
	int i=0, iTotalAttrCount=0, iFailedCount=0;
	string strAttrs("(");
	for(i=0; i < qu.num_rows() && qu.fetch_row() != NULL; i++)
	{
		Json attr;
		if(GetAttrInfoFromShm(qu.getval("attr_id"), attr) < 0)
		{
			strAttrs += qu.getstr("attr_id");
			strAttrs += ",";
			iFailedCount++;
		}
		else 
		{
			js["list"].Add(attr);
			iTotalAttrCount++;
		}
	}
	qu.free_result();
	js["machine_id"] = machine_id;

	if(iFailedCount <= 0)
	{
		js["count"] = iTotalAttrCount; 
		DEBUG_LOG("get machine attr list - result count:%d", iTotalAttrCount);
		return 0;
	}
	strAttrs.replace(strAttrs.size()-1, 1, ")");

	string strSql("select id,attr_name,attr_type,data_type from mt_attr where id in ");
	strSql += strAttrs;
	strSql += " and status=0 order by attr_type asc,id asc";

	Query & qu_taobao = *stConfig.qu;
	qu_taobao.get_result(strSql.c_str());
	if(qu_taobao.num_rows() < 0)
	{
		qu_taobao.free_result();
		ERR_LOG("get machine attr list failed, sql:%s", strSql.c_str());
		js["count"] = iTotalAttrCount; 
		return SLOG_ERROR_LINE;
	}

	for(i=0; i < qu_taobao.num_rows() && qu_taobao.fetch_row() != NULL; i++)
	{
		Json attr;
		attr["id"] = qu_taobao.getval("id");
		attr["name"] = qu_taobao.getstr("attr_name");
		attr["attr_type"] = qu_taobao.getstr("attr_type");
		attr["data_type"] = qu_taobao.getval("data_type");
		if(IsPercentAttr(qu_taobao.getval("id")))
			attr["show_class"] = "percent";
		else
			attr["show_class"] = "comm";
		js["list"].Add(attr);
	}
	js["count"] = i+iTotalAttrCount; 
	DEBUG_LOG("get machine attr list - result count:%d", i+iTotalAttrCount);
	qu_taobao.free_result();
	return 0;
}

static int GetAttrInfoByIdList(const char *pattr_id_list, Json & js)
{
	char *pattr_list = strdup(pattr_id_list);
	char *pfree_attr = pattr_list;
	char *pattr_id = NULL;
	char *psave = NULL;

	// for mmap
	AttrInfoBin *pAttrInfo = NULL;
	int iattr_id = 0;
	int iGetCount = 0;
	Query & qu_taobao = *stConfig.qu;
	for(; 1; pattr_list=NULL)
	{
		pattr_id = strtok_r(pattr_list, ",", &psave);
		if(pattr_id == NULL)
			break;
		iattr_id = atoi(pattr_id);

		Json attr;
		if(GetAttrInfoFromShm(iattr_id, attr) >= 0)
		{
			js["list"].Add(attr);
			iGetCount++;
			continue;
		}

		string strSql("select id,attr_name,attr_type,data_type from mt_attr where ");
		strSql += " and id=";
		strSql += pattr_id;
		DEBUG_LOG("get attr info - sql:%s", strSql.c_str());

		qu_taobao.get_result(strSql.c_str());
		if(qu_taobao.num_rows() < 0 || NULL == qu_taobao.fetch_row())
		{
			qu_taobao.free_result();
			REQERR_LOG("get attr info failed, sql:%s", strSql.c_str());
			continue;
		}

		attr["id"] = iattr_id;
		const char *pattr_name = qu_taobao.getstr("attr_name");
		attr["name"] = pattr_name;
		attr["attr_type"] = qu_taobao.getstr("attr_type");
		attr["data_type"] = qu_taobao.getval("data_type");

		if(IsPercentAttr(iattr_id))
			attr["show_class"] = "percent";
		else
			attr["show_class"] = "comm";
	
		js["list"].Add(attr);
		qu_taobao.free_result();
		iGetCount++;

		// save to vmem 
		if(pAttrInfo != NULL)
			pAttrInfo->iNameVmemIdx = MtReport_SaveToVmem(pattr_name, strlen(pattr_name)+1);
	}
	js["count"] = iGetCount; 
	DEBUG_LOG("get attr info - result count:%d", iGetCount);
	free(pfree_attr);
	return 0;
}

static int SetMachineAttr()
{
	// check 下 machine id 是否是该用户的
	int32_t type = hdf_get_int_value(stConfig.cgi->hdf, "Query.type", 2);
	int32_t machine_id = hdf_get_int_value(stConfig.cgi->hdf, "Query.machine_id", 0);
	MachineInfo* pMach = slog.GetMachineInfo(machine_id, NULL);
	if(0 == machine_id || pMach == NULL)
	{
		REQERR_LOG("invalid request , have no machine");
		return SLOG_ERROR_LINE;
	}

	hdf_set_int_value(stConfig.cgi->hdf, "config.machine_id", machine_id);
	hdf_set_int_value(stConfig.cgi->hdf, "config.per_count", SHOW_ATTR_PER_PAGE);
	hdf_set_int_value(stConfig.cgi->hdf, "config.show_type", type);

	struct timeval tv;
	gettimeofday(&tv, 0);
	uint32_t dwReqSeq = tv.tv_sec+tv.tv_usec;
	hdf_set_int_value(stConfig.cgi->hdf, "config.last_seq", dwReqSeq);

	const char *pdate = hdf_get_value(stConfig.cgi->hdf, "Query.date", NULL);
	if(pdate == NULL)
	{
		REQERR_LOG("invalid request , pdate:%p", pdate);
		return SLOG_ERROR_LINE;
	}
	char szTableName[32];
	if(!strcmp(pdate, "today"))
	{
		const char *pAttrTable = GetAttrTableName(stConfig.dwCurTime);
		strcpy(szTableName, pAttrTable);
		DEBUG_LOG("reqinfo today :%s", szTableName);
		pdate = uitodate2(stConfig.dwCurTime);
	}
	else
	{
		int y,m,d;
		sscanf(pdate, "%d-%d-%d", &y, &m, &d);
		sprintf(szTableName, "attr_%04d%02d%02d", y, m, d);
		DEBUG_LOG("reqinfo date:%s y:%d m:%d d:%d", szTableName, y, m, d);
	}
	hdf_set_value(stConfig.cgi->hdf, "config.cust_date", pdate);

	slog.memcache.SetKey("machine-attr-%s-%d-monitor", szTableName, machine_id);
	int *pattr_list = (int*)slog.memcache.GetValue();
	Json js_machine_attr;
	if(NULL == pattr_list)
	{
		WARN_LOG("get machine attr info from memcahce failed, key:%s, will user db", slog.memcache.GetKey());
		if(GetMachineAttrList(machine_id, js_machine_attr, szTableName) < 0)
			return SLOG_ERROR_LINE;
	}
	else
	{
		int iLen = slog.memcache.GetDataLen();
		int iCount = 0, iAttrId = 0;
		DEBUG_LOG("get machine attr info from memcahce ok, key:%s, datalen:%d", slog.memcache.GetKey(), iLen);
		for(int i=0; iLen >= (int)sizeof(int); iLen -= (int)sizeof(int), i++)
		{
			Json attr;
			iAttrId = ntohl(pattr_list[i]);

			if(iAttrId == 0) {
				WARN_LOG("get attr zero, index:%d, iLen:%d, key:%s",
					i, iLen, slog.memcache.GetKey());
				continue;
			}

			if(GetAttrInfoFromShm(iAttrId, attr) >= 0)
			{
				js_machine_attr["list"].Add(attr);
				iCount++;
			}
		}
		js_machine_attr["count"] = iCount;
		js_machine_attr["machine_id"] = machine_id;
	}

	std::string str_attr(js_machine_attr.ToString());
	hdf_set_value(stConfig.cgi->hdf, "config.attr_list", str_attr.c_str()); 
	DEBUG_LOG("attr list:%s, last seq:%u", str_attr.c_str(), dwReqSeq);
	return 0;
}

static int SetViewAttr(int view_id)
{
	hdf_set_int_value(stConfig.cgi->hdf, "config.per_count", SHOW_ATTR_PER_PAGE);
	const char *pdate = hdf_get_value(stConfig.cgi->hdf, "Query.date", NULL);
	if(NULL == pdate || !strcmp(pdate, "today"))
	{
		pdate = uitodate2(stConfig.dwCurTime);
	}
	hdf_set_value(stConfig.cgi->hdf, "config.cust_date", pdate);

	int32_t type = hdf_get_int_value(stConfig.cgi->hdf, "Query.type", 2);
	hdf_set_int_value(stConfig.cgi->hdf, "config.show_type", type);

	struct timeval tv;
	gettimeofday(&tv, 0);
	uint32_t dwReqSeq = tv.tv_sec+tv.tv_usec;
	hdf_set_int_value(stConfig.cgi->hdf, "config.last_seq", dwReqSeq);

	Json js_bind_attr;
	if(GetBindAttrList(view_id, js_bind_attr, NULL) < 0)
		return SLOG_ERROR_LINE;
	std::string str_attr(js_bind_attr.ToString());
	hdf_set_value(stConfig.cgi->hdf, "config.view_attr_list", str_attr.c_str()); 
	DEBUG_LOG("bind attr list json:%s", str_attr.c_str());
	return 0;
}

void ReadStrAttrInfoFromShm(
	TStrAttrReportInfo *pstrAttrShm, std::map<std::string, int> & stMapStrRepInfo)
{
	static StrAttrNodeValShmInfo *pstrAttrArryShm = slog.GetStrAttrNodeValShm(false);
	if(pstrAttrArryShm == NULL)
	{
		ERR_LOG("get GetStrAttrNodeValShm failed");
		return;
	}

	StrAttrNodeVal *pNodeShm = NULL;
	int idx = pstrAttrShm->iReportIdx, i = 0;
	std::map<std::string, int>::iterator it;
	for(i=0; i < pstrAttrShm->bStrCount; i++)
	{
	    if(idx < 0 || idx >= MAX_STR_ATTR_ARRY_NODE_VAL_COUNT)
	    {
	        ERR_LOG("invalid str attr report idx, idx:%d(0-%d)", idx, MAX_STR_ATTR_ARRY_NODE_VAL_COUNT);
	        return;
	    }
	
	    pNodeShm = pstrAttrArryShm->stInfo+idx;
		it = stMapStrRepInfo.find(pNodeShm->szStrInfo);
		if(it == stMapStrRepInfo.end()) 
			stMapStrRepInfo[pNodeShm->szStrInfo] = pNodeShm->iStrVal;
		else
			it->second += pNodeShm->iStrVal;
		idx = pNodeShm->iNextStrAttr;
	}
}

static int GetStrAttrDayVal(Json &js_mach, std::map<std::string, int> & stMapStrRepInfo,
	const Json &js_attr_info, Query &qu, const char *pszDayTableName, bool bIsLocalAttrSrv)		
{
	const char *pAttrTableToday = GetAttrTableName(stConfig.dwCurTime);
	const Json::json_list_t & jslist = js_mach["list"].GetArray();
	Json::json_list_t::const_iterator it = jslist.begin();

	if(!strcmp(pszDayTableName, pAttrTableToday) && bIsLocalAttrSrv) 
	{
		// 查询当天的，直接查 shm 共享内存
		TStrAttrReportInfo* pStrAttrShm = NULL;
		for(; it != jslist.end(); it++)
		{
			pStrAttrShm = slog.GetStrAttrShmInfo((int)js_attr_info["id"], (int)((*it)["id"]), NULL); 
			if(pStrAttrShm != NULL) {
				ReadStrAttrInfoFromShm(pStrAttrShm, stMapStrRepInfo);
			}
			else {
				DEBUG_LOG("not find str attr:%d, machine:%d, in shm", (int)js_attr_info["id"], (int)((*it)["id"]));
			}
		}
	}
	else {
		// 查询历史或者非本机的数据，从 db 读取
		char szSql[256] = {0};
		for(; it != jslist.end(); it++) 
		{
			if(!strcmp(pszDayTableName, pAttrTableToday)) {
				snprintf(szSql, sizeof(szSql),
					"select str_val from %s where attr_id=%d and machine_id=%d and value=0",
					pszDayTableName, (int)js_attr_info["id"], (int)((*it)["id"]));
			}
			else {
				snprintf(szSql, sizeof(szSql),
					"select value from %s_day where attr_id=%d and machine_id=%d and total=0",
					pszDayTableName, (int)js_attr_info["id"], (int)((*it)["id"]));
			}

			if(!qu.get_result(szSql) || qu.num_rows() <= 0) 
				continue;

			qu.fetch_row();
			qu.fetch_lengths();
			unsigned long ulValLen = qu.getlength(0);
			const char *pval = qu.getstr(0);
			comm::ReportAttr stAttrInfoPb;
			if(ulValLen > 0 && !stAttrInfoPb.ParseFromArray(pval, ulValLen))
			{
				ERR_LOG("ParseFromArray failed-%p-%lu", pval, ulValLen);
				return SLOG_ERROR_LINE;
			}
			DEBUG_LOG("read str attr:%d, machine:%d, from db:%s", 
				(int)js_attr_info["id"], (int)((*it)["id"]), stAttrInfoPb.ShortDebugString().c_str());

			std::map<std::string, int>::iterator it_str;
			for(int j=0; j < stAttrInfoPb.msg_attr_info_size(); j++)
			{
				it_str = stMapStrRepInfo.find(stAttrInfoPb.msg_attr_info(j).str());
				if(it_str == stMapStrRepInfo.end())
					stMapStrRepInfo[stAttrInfoPb.msg_attr_info(j).str()] 
						= stAttrInfoPb.msg_attr_info(j).uint32_attr_value();
				else
					it_str->second += stAttrInfoPb.msg_attr_info(j).uint32_attr_value();
			}
		}
	}
	return 0;
}

static int ShowAttrMulti(int iShowAttrType)
{
	const char *pattr_list = hdf_get_value(stConfig.cgi->hdf, "Query.attr_list", NULL);
	uint32_t dwLastReqSeq = hdf_get_int_value(stConfig.cgi->hdf, "Query.last_seq", 0);
	int32_t type = hdf_get_int_value(stConfig.cgi->hdf, "Query.type", 0);
	const char *pdate = hdf_get_value(stConfig.cgi->hdf, "Query.date", NULL);
	int type_id = 0;
	if(iShowAttrType == ATTR_SHOW_TYPE_MACHINE)
		type_id = hdf_get_int_value(stConfig.cgi->hdf, "Query.machine_id", 0);
	else if(iShowAttrType == ATTR_SHOW_TYPE_VIEW)
		type_id = hdf_get_int_value(stConfig.cgi->hdf, "Query.view_id", 0);
	if(NULL == pdate || NULL == pattr_list || 0 == type || 0 == type_id)
	{
		REQERR_LOG("invalid request , date:%p, attr_list:%p, type:%d, type_id:%d", 
			pdate, pattr_list, type, type_id);
		return SLOG_ERROR_LINE;
	}

	// check & get attr info
	Json js_attr_list;
	if(GetAttrInfoByIdList(pattr_list, js_attr_list) < 0)
	{
		REQERR_LOG("GetAttrInfoByIdList failed, attr info:%s", pattr_list);
		return SLOG_ERROR_LINE;
	}

	char szTableName[32];

	// 指定日期的前一日
	char szTableNameYstday[32] = {0}; 

	// 指定期上周同一日
	char szTableNameLastWkday[32] = {0}; 
	int iCurMin = -1;
	uint32_t dwUtcTime = 0;
	std::string strToday(uitodate2(stConfig.dwCurTime));
	const char *puiToday = strToday.c_str();
	if(!strcmp(pdate, "today") || !strcmp(pdate, puiToday))
	{
		const char *pAttrTable = GetAttrTableName(stConfig.dwCurTime);
		strcpy(szTableName, pAttrTable);
		iCurMin = GetDayOfMin();
		DEBUG_LOG("reqinfo today :%s", szTableName);
		dwUtcTime = stConfig.dwCurTime;
	}
	else
	{
		int y,m,d;
		sscanf(pdate, "%d-%d-%d", &y, &m, &d);
		sprintf(szTableName, "attr_%04d%02d%02d", y, m, d);
		DEBUG_LOG("reqinfo date:%s y:%d m:%d d:%d", szTableName, y, m, d);
		std::string strTmp(pdate);
		strTmp += " 0:0:0";
		dwUtcTime = datetoui(strTmp.c_str());
	}

	// 周图获取
	char s_aryTableWeekDay[7][32] = {0};
	int iWeekDayCur = 0;

	Json js_attr_val;
	std::string strTmp(uitodate2(dwUtcTime));
	js_attr_val["date_time_cur"] = strTmp.c_str();
	strTmp += " 0:0:0";
	js_attr_val["date_time_start_utc"] = datetoui(strTmp.c_str());
	js_attr_val["type"] = type;
	js_attr_val["cust_date"] = pdate;

	// 拉取同比图
	if(1 == type)
	{
		js_attr_val["date_time_cur"] = uitodate2(dwUtcTime);

		dwUtcTime -= 24*60*60;
		js_attr_val["date_time_yst"] = uitodate2(dwUtcTime);
		const char *pAttrTable = GetAttrTableName(dwUtcTime);
		strcpy(szTableNameYstday, pAttrTable);

		dwUtcTime -= 6*24*60*60;
		js_attr_val["date_time_wkd"] = uitodate2(dwUtcTime);
		pAttrTable = GetAttrTableName(dwUtcTime);
		strcpy(szTableNameLastWkday, pAttrTable);
		DEBUG_LOG("reqinfo Ystday:%s, LastWkday:%s", szTableNameYstday, szTableNameLastWkday);
	}
	// 周图
	else if(3 == type)
	{
		uint32_t dwMondayUtc = 0;
		std::string strWeekDays;
		GetAttrTableNameForWeek(dwUtcTime, s_aryTableWeekDay, iWeekDayCur, &dwMondayUtc, &strWeekDays);
		strTmp = uitodate2(dwMondayUtc);
		js_attr_val["date_time_monday"] = strTmp.c_str();
		strTmp += " 0:0:0";
		js_attr_val["date_time_start_utc"] = datetoui(strTmp.c_str());
		js_attr_val["date_week_days"] = strWeekDays.c_str();
		DEBUG_LOG("week monday:%s, week days:%s", strTmp.c_str(), strWeekDays.c_str());
	}

	Json js_mach;
	if(iShowAttrType == ATTR_SHOW_TYPE_VIEW)
	{
		// 视图
		TViewInfo * pinfo = GetMachineBindList(js_mach, type_id);
		if(pinfo != NULL && pinfo->iViewNameVmemIdx > 0) {
			const char * pname = MtReport_GetFromVmem_Local(pinfo->iViewNameVmemIdx);
			if(pname == NULL)
				js_attr_val["view_name"] = "unknow";
			else 
				js_attr_val["view_name"] = pname;
		}
		else {
			js_attr_val["view_name"] = "unknow";
		}
		js_attr_val["view_id"] = type_id;
	}
	else if(iShowAttrType == ATTR_SHOW_TYPE_MACHINE)
	{
		js_attr_val["machine_id"] = type_id;
		Json js;
		js["id"] = type_id;
		js_mach["list"].Add(js);
		js_mach["count"] = 1;
	}

	SLogServer *psrv = slog.GetValidServerByType(SRV_TYPE_ATTR_DB, NULL);
	if(NULL == psrv) {
		ERR_LOG("Get config server failed");
		return SLOG_ERROR_LINE;
	}

	// db 连接池使用
	bool bIsAttrServerLocal = false;
	const char *pserver_ip = psrv->szIpV4;
	if(slog.IsIpMatchLocalMachine(psrv->dwIp))
	{
		pserver_ip = "127.0.0.1";
		bIsAttrServerLocal = true;
	}
	CDbConnect db(pserver_ip);
	Query *pqu = db.GetQuery();
	if(NULL == pqu)
		return SLOG_ERROR_LINE;
	Query & qu = *pqu;

	const Json::json_list_t & jslist = js_attr_list["list"].GetArray();
	Json::json_list_t::const_iterator it = jslist.begin();
	int iShowRep = hdf_get_int_value(stConfig.cgi->hdf, "Query.show_rep", -1);
	int i=0;

	TWarnConfig *pConfig = NULL;
	for(i=0; it != jslist.end() && i < SHOW_ATTR_PER_PAGE; i++, it++)
	{
		Json attr;

		// 字符型监控点 --- start ---
		if((int)(*it)["data_type"] == STR_REPORT_D)
		{
			std::map<std::string, int> stMapStrRepInfo;
			if(type == 3) {
				// 周图
				for(int k=0; k < 7; k++) {
					if(s_aryTableWeekDay[k][0] == '\0')
						break;
					if(GetStrAttrDayVal(js_mach,
						stMapStrRepInfo, *it, qu, s_aryTableWeekDay[k], bIsAttrServerLocal) < 0)
						break;
				}
			}
			else {
				// 日图
				if(GetStrAttrDayVal(js_mach, stMapStrRepInfo, *it, qu, szTableName, bIsAttrServerLocal) < 0)
					break;
			}

			int iGetCount = MAX_STR_ATTR_STR_COUNT;

			// 按上报值降序排列显示 
			std::multimap<int, std::string> stMapStrReqOrder;
			std::map<std::string, int>::iterator it_str_attr = stMapStrRepInfo.begin();
			for(; it_str_attr != stMapStrRepInfo.end(); it_str_attr++)
			{
				stMapStrReqOrder.insert(
					std::make_pair<int, std::string>(it_str_attr->second, it_str_attr->first));
			}
			int ik = 0;
			std::multimap<int, std::string>::reverse_iterator it_str_order = stMapStrReqOrder.rbegin();
			for(ik=0; ik < iGetCount && it_str_order != stMapStrReqOrder.rend(); ik++, it_str_order++)
			{
				Json js_str_attr;
				js_str_attr["name"] = it_str_order->second;
				js_str_attr["value"] = it_str_order->first;
				attr["str_list"].Add(js_str_attr);
			}
			DEBUG_LOG("get str attr:%d, str count:%d, info:%s", (int)((*it)["id"]), ik, attr.ToString().c_str());
			if(iShowRep == 1 && ik <= 0)
				continue;

			attr["id"] = (int)((*it)["id"]);
			attr["str_count"] = ik;
			js_attr_val["list"].Add(attr);
			continue;
		}
		// 字符型监控点 --- end ---


		// 告警配置 -- start
		pConfig = slog.GetWarnConfigInfo(type_id, (int)((*it)["id"]), NULL);
		attr["warn_flag"] = 0;
		if(pConfig != NULL)
		{
			int flag = pConfig->iWarnConfigFlag;
			if((iShowAttrType==ATTR_SHOW_TYPE_VIEW && (flag&ATTR_WARN_FLAG_TYPE_VIEW))
				|| (iShowAttrType==ATTR_SHOW_TYPE_MACHINE && (flag&ATTR_WARN_FLAG_TYPE_MACHINE)))
			{
				if(flag & ATTR_WARN_FLAG_MASK_WARN)
					attr["warn_mask"] = 1;
				if(flag & ATTR_WARN_FLAG_MAX)
					attr["warn_max"] = pConfig->iWarnMax;
				if(flag & ATTR_WARN_FLAG_MIN)
					attr["warn_min"] = pConfig->iWarnMin;
				if(flag & ATTR_WARN_FLAG_WAVE)
					attr["warn_wave"] = pConfig->iWarnWave;
				attr["warn_flag"] = flag;
				attr["warn_config_id"] = pConfig->iWarnConfigId;
			}
			attr["warn_flag"] = pConfig->iWarnConfigFlag;
			DEBUG_LOG("get warn info, attr_id:%d, warn_type_value:%d, flag:%d, max:%d, min:%d, wave:%d",
				(int)((*it)["id"]), type_id, (int)attr["warn_flag"], 
				(int)attr["warn_max"], (int)attr["warn_min"], (int)attr["warn_wave"]);
		}
		// 告警配置 -- end 

		// 周图
		if(3 == type)
		{
			std::string strVal;
			attr["id"] = (*it)["id"];
			attr["max"] = 0;

			for(int k=0; k < 7; k++)
			{
				if(s_aryTableWeekDay[k][0] == '\0')
					break;
				Json attr_tmp;
				int iGetMin = -1;
				if(!strcmp(szTableName, s_aryTableWeekDay[k]))
					iGetMin = iCurMin;
	
				if(GetAttrDayVal(js_mach, attr_tmp, s_aryTableWeekDay[k], *it, iGetMin, qu) < 0)
					break;
				DEBUG_LOG("req week monitor - attr table name:-%d-%s", k, s_aryTableWeekDay[k]);
				strVal += (const char*)attr_tmp["value_list_str"];
				strVal += ",";

				if((unsigned int)attr["max"] < (unsigned int)attr_tmp["max"])
					attr["max"] = (unsigned int)attr_tmp["max"];

				if(k == iWeekDayCur)
					attr["cur"] = (unsigned int)attr_tmp["cur"];
			}
			if((unsigned int)attr["max"] <= 0)
				attr["value_list_str"] = "0";
			else
				attr["value_list_str"] = strVal.c_str();
		}
		else
		{
			if(GetAttrDayVal(js_mach, attr, szTableName, *it, iCurMin, qu) < 0)
				break;

			// 同比图
			unsigned int iMaxYst = 0, iMaxLastWk = 0;
			if(1 == type)
			{
				Json attr_yst;
				if(GetAttrDayVal(js_mach, attr_yst, szTableNameYstday, *it, iCurMin, qu) < 0)
					break;
				attr["value_list_yst_str"] = attr_yst["value_list_str"];
				iMaxYst = (unsigned int)attr_yst["max"];

				Json attr_lastwk;
				if(GetAttrDayVal(js_mach, attr_lastwk, szTableNameLastWkday, *it, iCurMin, qu) < 0)
					break;
				attr["value_list_lwk_str"] = attr_lastwk["value_list_str"];
				iMaxLastWk = (unsigned int)attr_lastwk["max"];
			}

			if((unsigned int)attr["max"] <= 0 && iMaxYst <= 0 && iMaxLastWk <= 0)
			{
				attr["value_list_str"] = "0";
				attr["value_list_yst_str"] = "0";
				attr["value_list_lwk_str"] = "0";
			}
		}

		if(iShowRep == 1 && (int)(attr["max"]) == 0)
			continue;
		js_attr_val["list"].Add(attr);
	}

	js_attr_val["count"] = i;
	if(!strcmp(pdate, "today") || !strcmp(pdate, puiToday))
		js_attr_val["date_time"] = uitodate(stConfig.dwCurTime);
	else
		js_attr_val["date_time"] = pdate;

	js_attr_val["cgi_path"] = stConfig.szCgiPath;

	Json js_show;
	js_show["statusCode"] = 0;
	js_show["attr_list"] = js_attr_list;
	js_show["attr_val_list"] = js_attr_val;
	js_show["last_req_seq"] = dwLastReqSeq;

	if(hdf_get_int_value(stConfig.cgi->hdf, "Query.req_timestamp", 0) != 0)
	{
		double end = ne_timef();
		js_show["cgi_run_ms"] = (int)(end-stConfig.cgi->time_start);
		INFO_LOG("send to client cgi run:%d ms", (int)(end-stConfig.cgi->time_start));
	}

	STRING str;
	string_init(&str);
	if((stConfig.err=string_set(&str, js_show.ToString().c_str())) != STATUS_OK
		|| (stConfig.err=cgi_output(stConfig.cgi, &str)) != STATUS_OK)
	{
		string_clear(&str);
		return SLOG_ERROR_LINE;
	}
	string_clear(&str);
	DEBUG_LOG("get attr json type:%d, type_id:%d, result:%s", iShowAttrType, type_id, js_show.ToString().c_str());
	return 0;
}

static int InitFastCgi_first(CGIConfig &myConf)
{
	if(InitFastCgiStart(myConf) < 0) {
		ERR_LOG("InitFastCgiStart failed !");
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

	if(slog.InitStrAttrHashForWrite() < 0)
	{
	    ERR_LOG("init str attr shm failed !");
	    return SLOG_ERROR_LINE;
	}   

	return 0;
}

static int ShowAttrSingle()
{
	// check & get attr info
	const char *pattr_list = hdf_get_value(stConfig.cgi->hdf, "Query.attr_list", NULL);
	int32_t type = hdf_get_int_value(stConfig.cgi->hdf, "Query.show_single_type", 0);
	const char *pdate = hdf_get_value(stConfig.cgi->hdf, "Query.date", NULL);
	int32_t type_id = hdf_get_int_value(stConfig.cgi->hdf, "Query.type_id", 0);
	const char *show_type = hdf_get_value(stConfig.cgi->hdf, "Query.show_type", NULL);
	if(NULL == pdate || NULL == show_type || NULL == pattr_list || 0 == type || 0 == type_id)
	{
		REQERR_LOG("invalid request , info|%p|%p|%p|%d|%d", pdate, show_type, pattr_list, type, type_id);
		return SLOG_ERROR_LINE;
	}

	Json js_attr_list;
	if(GetAttrInfoByIdList(pattr_list, js_attr_list) < 0)
	{
		REQERR_LOG("GetAttrInfoByIdList failed, attr info:%s", pattr_list);
		return SLOG_ERROR_LINE;
	}

	Json &js_attr_info = js_attr_list["list"].GetArray().front();
	int attr_id = atoi(pattr_list);

	char szTableName[32];

	// 指定日期的前一日
	char szTableNameYstday[32] = {0}; 

	// 指定日期上周同一日
	char szTableNameLastWkday[32] = {0}; 
	int iCurMin = -1;
	uint32_t dwUtcTime = 0;

	std::string strToday(uitodate2(stConfig.dwCurTime));
	const char *puiToday = strToday.c_str();
	if(!strcmp(pdate, "today") || !strcmp(pdate, puiToday))
	{
		const char *pAttrTable = GetAttrTableName(stConfig.dwCurTime);
		strcpy(szTableName, pAttrTable);
		iCurMin = GetDayOfMin();
		DEBUG_LOG("reqinfo today :%s", szTableName);
		dwUtcTime = stConfig.dwCurTime;
	}
	else
	{
		int y,m,d;
		sscanf(pdate, "%d-%d-%d", &y, &m, &d);
		sprintf(szTableName, "attr_%04d%02d%02d", y, m, d);
		DEBUG_LOG("reqinfo date:%s y:%d m:%d d:%d", szTableName, y, m, d);
		std::string strTmp(pdate);
		strTmp += " 0:0:0";
		dwUtcTime = datetoui(strTmp.c_str());
	}

	// 周图获取
	char s_aryTableWeekDay[7][32] = {0};
	// 月图
	char s_aryTableMonthDay[31][32] = {0};
	int iWeekDayCur = 0;

	Json js_attr_val;

	std::string strTmp(uitodate2(dwUtcTime));
	js_attr_val["date_time_cur"] = strTmp.c_str();
	strTmp += " 0:0:0";
	js_attr_val["date_time_start_utc"] = datetoui(strTmp.c_str());
	js_attr_val["cust_date"] = pdate;

	// 拉取同比图
	if(1 == type)
	{
		dwUtcTime -= 24*60*60;
		js_attr_val["date_time_yst"] = uitodate2(dwUtcTime);
		const char *pAttrTable = GetAttrTableName(dwUtcTime);
		strcpy(szTableNameYstday, pAttrTable);

		dwUtcTime -= 6*24*60*60;
		js_attr_val["date_time_wkd"] = uitodate2(dwUtcTime);
		pAttrTable = GetAttrTableName(dwUtcTime);
		strcpy(szTableNameLastWkday, pAttrTable);
		DEBUG_LOG("reqinfo Ystday:%s, LastWkday:%s", szTableNameYstday, szTableNameLastWkday);
	}
	// 周图
	else if(3 == type)
	{
		uint32_t dwMondayUtc = 0;
		std::string strWeekDays;
		GetAttrTableNameForWeek(dwUtcTime, s_aryTableWeekDay, iWeekDayCur, &dwMondayUtc, &strWeekDays);
		strTmp = uitodate2(dwMondayUtc);
		js_attr_val["date_time_monday"] = strTmp.c_str();
		strTmp += " 0:0:0";
		js_attr_val["date_time_start_utc"] = datetoui(strTmp.c_str());
		js_attr_val["date_week_days"] = strWeekDays.c_str();
		DEBUG_LOG("week monday:%s, week days:%s", strTmp.c_str(), strWeekDays.c_str());
	}
	else if(4 == type)
	{
		//puiToday;
		uint32_t dwFirstMonthDayUtc = 0;
		std::string strMonthDays;
		GetAttrTableNameForMonth(dwUtcTime, s_aryTableMonthDay, &dwFirstMonthDayUtc, &strMonthDays);
		strTmp = uitodate2(dwFirstMonthDayUtc);
		js_attr_val["date_time_month_day"] = strTmp.c_str();
		strTmp += " 0:0:0";
		js_attr_val["date_time_start_utc"] = datetoui(strTmp.c_str());
		js_attr_val["date_month_days"] = strMonthDays.c_str();
		DEBUG_LOG("month first day:%s, month days:%s", strTmp.c_str(), strMonthDays.c_str());
	}

	Json js_mach;
	if(!strcmp(show_type, "view"))
	{
		GetMachineBindList(js_mach, type_id);
	}
	else if(!strcmp(show_type, "machine"))
	{
		Json js;
		js["id"] = type_id;
		js_mach["list"].Add(js);
		js_mach["count"] = 1;
	}

	SLogServer *psrv = slog.GetValidServerByType(SRV_TYPE_ATTR_DB, NULL);
	if(NULL == psrv) {
		ERR_LOG("Get config server failed");
		return SLOG_ERROR_LINE;
	}

	// db 连接池使用
	const char *pserver_ip = psrv->szIpV4;
	if(slog.IsIpMatchLocalMachine(psrv->dwIp))
		pserver_ip = "127.0.0.1";
	CDbConnect db(pserver_ip);
	Query *pqu = db.GetQuery();
	if(NULL == pqu)
	{
		return SLOG_ERROR_LINE;
	}
	Query & qu = *pqu;

	js_attr_val["id"] = attr_id;

	Json JsEachMach;
	InitGetEachMachAttrInfo(js_mach, JsEachMach);

	// 周图
	if(3 == type)
	{
		std::string strVal;

		js_attr_val["max"] = 0;
		for(int k=0; k < 7; k++)
		{
			if(s_aryTableWeekDay[k][0] == '\0')
				break;
			Json attr_tmp;
			int iGetMin = -1;
			if(!strcmp(szTableName, s_aryTableWeekDay[k]))
				iGetMin = iCurMin;
			if(GetAttrDayVal(js_mach, attr_tmp, s_aryTableWeekDay[k], js_attr_info, iGetMin, qu, JsEachMach) < 0)
				break;
			strVal += (const char*)attr_tmp["value_list_str"];
			strVal += ",";
			DEBUG_LOG("req week monitor - attr table name:-%d-%s, size:%u", 
					k, s_aryTableWeekDay[k], (uint32_t)strVal.size());
			if((unsigned int)js_attr_val["max"] < (unsigned int)attr_tmp["max"])
				js_attr_val["max"] = (unsigned int)attr_tmp["max"];
		}
		if((unsigned int)js_attr_val["max"] <= 0)
			js_attr_val["value_list_str"] = "0";
		else
			js_attr_val["value_list_str"] = strVal.c_str();
	}
	// 月图
	else if(4 == type)
	{
		std::string strVal;

		js_attr_val["max"] = 0;
		for(int k=0; k < 31; k++)
		{
			if(s_aryTableMonthDay[k][0] == '\0')
				break;
			Json attr_tmp;
			int iGetMin = -1;
			if(!strcmp(szTableName, s_aryTableMonthDay[k]))
				iGetMin = iCurMin;
			if(GetAttrDayVal(js_mach, attr_tmp, s_aryTableMonthDay[k], js_attr_info, iGetMin, qu, JsEachMach) < 0)
				break;
			strVal += (const char*)attr_tmp["value_list_str"];
			strVal += ",";
			DEBUG_LOG("req month monitor - attr table name:-%d-%s, size:%u", 
					k, s_aryTableMonthDay[k], (uint32_t)strVal.size());

			if((unsigned int)js_attr_val["max"] < (unsigned int)attr_tmp["max"])
				js_attr_val["max"] = (unsigned int)attr_tmp["max"];
		}
		if((unsigned int)js_attr_val["max"] <= 0)
			js_attr_val["value_list_str"] = "0";
		else
			js_attr_val["value_list_str"] = strVal.c_str();
	}
	else
	{
		if(GetAttrDayVal(js_mach, js_attr_val, szTableName, js_attr_info, iCurMin, qu, JsEachMach) >= 0)
		{
			// 同比图, 还要取昨日/上周同日数据
			unsigned int iMaxYst = 0, iMaxLastWk = 0;
			if(1 == type)
			{
				Json attr_yst;
				Json jsEachMachYst;
				InitGetEachMachAttrInfo(js_mach, jsEachMachYst);

				if(GetAttrDayVal(js_mach, attr_yst, szTableNameYstday, js_attr_info, iCurMin, qu, jsEachMachYst) >= 0)
				{
					js_attr_val["value_list_yst_str"] = attr_yst["value_list_str"];
					iMaxYst = (unsigned int)attr_yst["max"];
					EndGetEachMachAttrInfo(jsEachMachYst);
					js_attr_val["each_mach_yst"] = jsEachMachYst;
				}

				Json attr_lastwk;
				Json jsEachMachLwk;
				InitGetEachMachAttrInfo(js_mach, jsEachMachLwk);
				if(GetAttrDayVal(
					js_mach, attr_lastwk, szTableNameLastWkday, js_attr_info, iCurMin, qu, jsEachMachLwk) >= 0)
				{
					js_attr_val["value_list_lwk_str"] = attr_lastwk["value_list_str"];
					iMaxLastWk = (unsigned int)attr_lastwk["max"];
					EndGetEachMachAttrInfo(jsEachMachYst);
					js_attr_val["each_mach_lwk"] = jsEachMachYst;
				}
			}

			if((unsigned int)js_attr_val["max"] <= 0 && iMaxYst <= 0 && iMaxLastWk <= 0)
			{
				js_attr_val["value_list_str"] = "0";
				js_attr_val["value_list_yst_str"] = "0";
				js_attr_val["value_list_lwk_str"] = "0";
			}
		}
	}

	EndGetEachMachAttrInfo(JsEachMach);
	js_attr_val["each_mach"] = JsEachMach;

	if(!strcmp(pdate, "today") || !strcmp(pdate, puiToday))
		js_attr_val["date_time"] = uitodate(stConfig.dwCurTime);
	else
		js_attr_val["date_time"] = pdate;
	js_attr_val["cgi_path"] = stConfig.szCgiPath;

	js_attr_val["statusCode"] = 0;
	js_attr_val["attr_list"] = js_attr_list;
	js_attr_val["type"] = type;
	js_attr_val["show_type"] = show_type;

	STRING str;
	string_init(&str);
	if((stConfig.err=string_set(&str, js_attr_val.ToString().c_str())) != STATUS_OK
		|| (stConfig.err=cgi_output(stConfig.cgi, &str)) != STATUS_OK)
	{
		string_clear(&str);
		return SLOG_ERROR_LINE;
	}
	string_clear(&str);
	DEBUG_LOG("json result:%s", js_attr_val.ToString().c_str());
	return 0;
}

static int SetAttrSingle()
{
	int show_single_type = hdf_get_int_value(stConfig.cgi->hdf, "Query.show_single_type", 0);
	const char *show_type = hdf_get_value(stConfig.cgi->hdf, "Query.show_type", NULL);
	const char *pdate = hdf_get_value(stConfig.cgi->hdf, "Query.cust_date", NULL);
	int type_id = hdf_get_int_value(stConfig.cgi->hdf, "Query.type_id", 0);
	int attr_id = hdf_get_int_value(stConfig.cgi->hdf, "Query.attr_id", 0);
	if(pdate == NULL || show_type == NULL || type_id==0 || attr_id==0 || show_single_type==0)
	{
		REQERR_LOG("invalid request , %p|%p|%d|%d|%d ", pdate, show_type, type_id, attr_id, show_single_type);
		return SLOG_ERROR_LINE;
	}

	hdf_set_int_value(stConfig.cgi->hdf, "config.type_id", type_id);
	hdf_set_int_value(stConfig.cgi->hdf, "config.attr_id", attr_id);
	hdf_set_int_value(stConfig.cgi->hdf, "config.show_single_type", show_single_type);
	hdf_set_value(stConfig.cgi->hdf, "config.show_type", show_type);
	hdf_set_value(stConfig.cgi->hdf, "config.cust_date", pdate);
	DEBUG_LOG("receive request, info-%s|%s|%d|%d|%d ", pdate, show_type, type_id, attr_id, show_single_type);
	return 0;
}

static int GetWarnInfoTotalRecords(WarnListSearchInfo *pinfo=NULL)
{
	char sSqlBuf[512] = {0};
	sprintf(sSqlBuf, "select count(*) from mt_warn_info where status=%d", RECORD_STATUS_USE);
	int iFilter = 0;
	if(pinfo != NULL && (iFilter=AddWarnInfoSearch(sSqlBuf, sizeof(sSqlBuf), pinfo)) < 0)
		return SLOG_ERROR_LINE;

	// modify by rock -- 2019-04-20 告警总数从db 获取
	Query qu(*stConfig.db);
	DEBUG_LOG("get warn_info count - exesql:%s", sSqlBuf);
	if(qu.get_result(sSqlBuf) == NULL || qu.num_rows() <= 0)
	{
		qu.free_result();
		return 0;
	}

	qu.fetch_row();
	int iCount = qu.getval(0);
	qu.free_result();
	DEBUG_LOG("warn info records count:%d", iCount);
	return iCount;
}

static int DealGetWarnList(WarnListSearchInfo *pinfo=NULL)
{
	int iRecords = GetWarnInfoTotalRecords(pinfo);
	if(iRecords < 0)
	{
		ERR_LOG("get attr record count failed !");
		return SLOG_ERROR_LINE;
	}
	SetRecordsPageInfo(stConfig.cgi, iRecords);

	Json js;
	if(GetWarnList(js, pinfo, iRecords) < 0)
		return SLOG_ERROR_LINE;

	std::string str(js.ToString());
	DEBUG_LOG("warn info list json:%s", str.c_str());

	stConfig.err = hdf_set_value(stConfig.cgi->hdf, "config.warn_info_list", str.c_str()); 
	if(stConfig.err != STATUS_OK)
	{
		ERR_LOG("set warn info list failed, length:%u", (uint32_t)str.size());
		return SLOG_ERROR_LINE;
	}
	return 0;
}

static int DealWarnListSearchPage()
{
	WarnListSearchInfo stInfo;
	memset(&stInfo, 0, sizeof(stInfo));
	stInfo.iMachineId = hdf_get_int_value(stConfig.cgi->hdf, "Query.dwl_machine_id_page", 0);
	stInfo.iViewId = hdf_get_int_value(stConfig.cgi->hdf, "Query.dwl_view_id_page", 0);
	stInfo.iWarnObj = hdf_get_int_value(stConfig.cgi->hdf, "Query.dwl_warn_obj", 0);
	stInfo.iWarnType = hdf_get_int_value(stConfig.cgi->hdf, "Query.dwl_warn_type", 0);
	stInfo.iDealStatus = hdf_get_int_value(stConfig.cgi->hdf, "Query.dwl_deal_status", -1);
	stInfo.iAttrId = hdf_get_int_value(stConfig.cgi->hdf, "Query.dwl_attr_id", 0);

	return DealGetWarnList(&stInfo);
}

static int DealWarnListSearch()
{
	WarnListSearchInfo stInfo;
	memset(&stInfo, 0, sizeof(stInfo));
	stInfo.iMachineId = hdf_get_int_value(stConfig.cgi->hdf, "Query.dwl_machine_id", 0);
	stInfo.iViewId = hdf_get_int_value(stConfig.cgi->hdf, "Query.dwl_view_id", 0);
	stInfo.iWarnObj = hdf_get_int_value(stConfig.cgi->hdf, "Query.dwl_warn_obj_sel", 0);
	stInfo.iAttrId = hdf_get_int_value(stConfig.cgi->hdf, "Query.dwl_attr_id", 0);

	if((stInfo.iMachineId != 0 && stInfo.iViewId != 0)
		|| (stInfo.iWarnObj == ATTR_WARN_FLAG_TYPE_MACHINE && stInfo.iViewId != 0)
		|| (stInfo.iWarnObj == ATTR_WARN_FLAG_TYPE_VIEW && stInfo.iMachineId != 0))
	{
		WARN_LOG("invalid reqparam : %d, %d, %d", stInfo.iMachineId, stInfo.iViewId, stInfo.iWarnObj);
		stConfig.pErrMsg = CGI_REQERR;
		return SLOG_ERROR_LINE;
	}

	if(stInfo.iMachineId != 0)
		hdf_set_int_value(stConfig.cgi->hdf, "config.dwl_machine_id", stInfo.iMachineId);
	if(stInfo.iViewId != 0)
		hdf_set_int_value(stConfig.cgi->hdf, "config.dwl_view_id", stInfo.iViewId);
	if(stInfo.iAttrId != 0)
		hdf_set_int_value(stConfig.cgi->hdf, "config.dwl_attr_id", stInfo.iAttrId);

	stInfo.iDealStatus = hdf_get_int_value(stConfig.cgi->hdf, "Query.dwl_deal_status_sel", -1);
	hdf_set_int_value(stConfig.cgi->hdf, "config.dwl_deal_status_sel", stInfo.iDealStatus);

	hdf_set_int_value(stConfig.cgi->hdf, "config.dwl_warn_obj_sel", stInfo.iWarnObj);

	stInfo.iWarnType = hdf_get_int_value(stConfig.cgi->hdf, "Query.dwl_warn_type_sel", 0);
	hdf_set_int_value(stConfig.cgi->hdf, "config.dwl_warn_type_sel", stInfo.iWarnType);

	return DealGetWarnList(&stInfo);
}

static const char * GetAttrInfo(uint32_t dwAttrId)
{
	static std::string s_attrInfo;

	if(!stConfig.iDisableVmemCache) {
		// 未禁用 cache 
		AttrInfoBin *pInfo = slog.GetAttrInfo(dwAttrId, NULL);
		int iNameIdx = (pInfo != NULL ? pInfo->iNameVmemIdx : -1);
		const char *pvname = NULL;
		if(iNameIdx > 0)
			pvname = MtReport_GetFromVmem_Local(iNameIdx);
		if(pvname != NULL)
		{
			DEBUG_LOG("get attr:%u name from shm", dwAttrId);
			s_attrInfo = pvname;
			return s_attrInfo.c_str();
		}
	}

	char sSqlBuf[512] = {0};
	snprintf(sSqlBuf, sizeof(sSqlBuf), "select attr_name from mt_attr where id=%u and status=%d",
		dwAttrId, RECORD_STATUS_USE);

	Query qu(*stConfig.db);
	qu.get_result(sSqlBuf);
	if(qu.num_rows() <= 0 || NULL == qu.fetch_row())
	{
		qu.free_result();
		ERR_LOG("get attr info failed, sql:%s", sSqlBuf);
		return NULL;
	}

	s_attrInfo = qu.getstr("attr_name");
	qu.free_result();
	return s_attrInfo.c_str();
}

static int DealWarnsInShm(const char *pWarns, int iStatus)
{
	int iStartIdx = stConfig.stUser.pSysInfo->iWarnIndexStart;
	SharedHashTableNoList *pWarnHash = slog.GetWarnInfoHash();
	TWarnInfo *pInfo = NULL;
	int iCount = 0;
	const char *pWid = NULL;
	for(int i=0; i < stConfig.stUser.pSysInfo->bLastWarnCount; i++)
	{
		iCount++;
		pInfo = (TWarnInfo*)NOLIST_HASH_INDEX_TO_NODE(pWarnHash, iStartIdx);
		iStartIdx = pInfo->iNextIndex;
		pWid = itoa(pInfo->id);
		if(pInfo->iDealStatus == iStatus-1 && strstr(pWarns, pWid)) {
			pInfo->iDealStatus = iStatus;
			iCount++;
		}
	}
	DEBUG_LOG("set shm warn status to:%d, warns:%s", iStatus, pWarns);
	return 0;
}

static int GetWarnListFromShm(Json &js, int iCurPage, int iNumPerPage, int iTotal)
{
	// 只有最近的  bLastWarnCount 个告警存在内存中 
	if(stConfig.stUser.pSysInfo->bLastWarnCount < iNumPerPage*iCurPage 
		&& iTotal > stConfig.stUser.pSysInfo->bLastWarnCount)
	{
		return 1;
	}

	uint32_t dwTime = 0;
	const char *pattr = NULL;
	int iStart = iNumPerPage*(iCurPage-1);
	int iStartIdx = stConfig.stUser.pSysInfo->iWarnIndexStart;
	SharedHashTableNoList *pWarnHash = slog.GetWarnInfoHash();
	TWarnInfo *pInfo = NULL;
	for(int i=0; i < iStart && i < stConfig.stUser.pSysInfo->bLastWarnCount; i++)
	{
		pInfo = (TWarnInfo*)NOLIST_HASH_INDEX_TO_NODE(pWarnHash, iStartIdx);
		if(pInfo == NULL) {
			WARN_LOG("TWarnInfo is NULL, idx:(%d,%d), count:%d",
				iStart, iStartIdx, stConfig.stUser.pSysInfo->bLastWarnCount);
			break;
		}
		iStartIdx = pInfo->iNextIndex;
	}

	int iCount = 0;
	MachineInfo *pMachinfo = NULL;
	TViewInfo *pViewinfo = NULL;
	const char *pname = NULL;
	for(int i=iStart; i < iStart+iNumPerPage && i < stConfig.stUser.pSysInfo->bLastWarnCount; i++)
	{
		pInfo = (TWarnInfo*)NOLIST_HASH_INDEX_TO_NODE(pWarnHash, iStartIdx);
		if(pInfo == NULL) {
			WARN_LOG("TWarnInfo is invalid, idx:(%d,%d,%d), count:%d",
				iStart, iStartIdx, i, stConfig.stUser.pSysInfo->bLastWarnCount);
			break;
		}
		iStartIdx = pInfo->iNextIndex;
		Json info;
		info["wid"] = pInfo->id;

		if(pInfo->dwWarnFlag & 16)
		{
			pname = NULL;
			int t_idx = slog.GetViewInfoIndex(pInfo->iWarnTypeId, NULL);
			pViewinfo = slog.GetViewInfo(t_idx);
			if(pViewinfo != NULL && pViewinfo->iViewNameVmemIdx > 0)
				pname = MtReport_GetFromVmem_Local(pViewinfo->iViewNameVmemIdx);
			if(pname != NULL)
				info["warn_name"] = pname; 
			else
			{
				info["warn_name"] = "unknow"; 
				DEBUG_LOG("get view name failed, id:%d", pInfo->iWarnTypeId); 
			}
		}
		else 
		{
			pname = NULL;
			pMachinfo = slog.GetMachineInfo(pInfo->iWarnTypeId, NULL);
			if(pMachinfo != NULL && pMachinfo->iNameVmemIdx > 0)
				pname = MtReport_GetFromVmem_Local(pMachinfo->iNameVmemIdx);
			if(pname != NULL)
				info["warn_name"] = pname; 
			else
			{
				info["warn_name"] = "unknow"; 
				DEBUG_LOG("get machine name failed, id:%d", pInfo->iWarnTypeId); 
			}
		}
		info["warn_id"] = pInfo->iWarnTypeId;

		info["attr_id"] = pInfo->iAttrId;
		pattr = GetAttrInfo((uint32_t)(info["attr_id"]));
		if(pattr == NULL)
			info["attr_name"] = "unknow";
		else
			info["attr_name"] = pattr;
		dwTime = pInfo->dwWarnTime;
		info["warn_time"] = uitodate(dwTime);
		info["warn_flag"] = pInfo->dwWarnFlag;
		info["warn_val"] = pInfo->dwWarnVal;
		info["warn_conf_val"] = pInfo->dwWarnConfVal;
		info["warn_times"] = pInfo->iWarnTimes;
		info["deal_status"] = pInfo->iDealStatus;
		dwTime = pInfo->dwLastWarnTime;
		info["last_warn_time"] = uitodate(dwTime);
		js["list"].Add(info);
		iCount++;
	}
	js["count"] = iCount;
	DEBUG_LOG("get warn info from shm, count:%d", iCount);
	return 0;
}

static int GetWarnList(Json & js, WarnListSearchInfo *pinfo, int iTotal)
{
	int iCurPage = hdf_get_int_value(stConfig.cgi->hdf, "config.currentPage", 0);
	int iNumPerPage = hdf_get_int_value(stConfig.cgi->hdf, "config.numPerPage", 0);
	if(iCurPage == 0 || iNumPerPage == 0)
	{
		ERR_LOG("invalid iCurPage(%d) or iNumPerPage(%d)", iCurPage, iNumPerPage);
		return SLOG_ERROR_LINE;
	}

	char sSqlBuf[512] = {0};
	int iFilter = 0;
	snprintf(sSqlBuf, sizeof(sSqlBuf), "select * from mt_warn_info where status=0 ");
	if(pinfo != NULL && (iFilter=AddWarnInfoSearch(sSqlBuf, sizeof(sSqlBuf), pinfo)) < 0)
		return SLOG_ERROR_LINE;

	// 未设置查询条件，从共享内存获取
	if(iFilter <= 0 && !stConfig.iDisableVmemCache)
	{
		int iRet = GetWarnListFromShm(js, iCurPage, iNumPerPage, iTotal);
		if(iRet <= 0)
			return iRet;
	}

	Query & qu = *stConfig.qu;
	char sTmpBuf[64]={0};
	sprintf(sTmpBuf, " order by last_warn_time_utc desc limit %d,%d", iNumPerPage*(iCurPage-1), iNumPerPage);
	strcat(sSqlBuf, sTmpBuf);
	DEBUG_LOG("get warn info list - exesql:%s", sSqlBuf);

	qu.get_result(sSqlBuf);
	if(qu.num_rows() < 0)
	{
		qu.free_result();
		ERR_LOG("get warn info list failed, sql:%s", sSqlBuf);
		return SLOG_ERROR_LINE;
	}

	int i=0;
	uint32_t dwTime = 0;
	const char *pattr = NULL, *pname = NULL;
	uint32_t dwWarnFlag = 0;
	int32_t iWarnTypeId = 0;
	MachineInfo *pMachinfo = NULL;
	TViewInfo *pViewinfo = NULL;

	for(; i < qu.num_rows() && qu.fetch_row() != NULL; i++)
	{
		Json info;
		info["wid"] = qu.getuval("wid");

		dwWarnFlag = qu.getuval("warn_flag");
		iWarnTypeId = qu.getuval("warn_id");
		if(dwWarnFlag & 16)
		{
			pname = NULL;
			int t_idx = slog.GetViewInfoIndex(iWarnTypeId, NULL);
			pViewinfo = slog.GetViewInfo(t_idx);
			if(pViewinfo != NULL && pViewinfo->iViewNameVmemIdx > 0)
				pname = MtReport_GetFromVmem_Local(pViewinfo->iViewNameVmemIdx);
			if(pname != NULL)
				info["warn_name"] = pname; 
			else
			{
				info["warn_name"] = "unknow"; 
				DEBUG_LOG("get view name failed, id:%d", iWarnTypeId); 
			}
		}
		else 
		{
			pname = NULL;
			pMachinfo = slog.GetMachineInfo(iWarnTypeId, NULL);
			if(pMachinfo != NULL && pMachinfo->iNameVmemIdx > 0)
				pname = MtReport_GetFromVmem_Local(pMachinfo->iNameVmemIdx);
			if(pname != NULL)
				info["warn_name"] = pname; 
			else
			{
				info["warn_name"] = "unknow"; 
				DEBUG_LOG("get machine name failed, id:%d", iWarnTypeId); 
			}
		}
		info["warn_id"] = iWarnTypeId;


		info["attr_id"] = qu.getuval("attr_id");
		pattr = GetAttrInfo((uint32_t)(info["attr_id"]));
		if(pattr == NULL)
			info["attr_name"] = "get failed";
		else
			info["attr_name"] = pattr;
		dwTime = qu.getuval("warn_time_utc");
		info["warn_time"] = uitodate(dwTime);
		info["warn_flag"] = qu.getuval("warn_flag");

		info["warn_val"] = qu.getuval("warn_val");
		info["warn_conf_val"] = qu.getuval("warn_config_val");
		info["warn_times"] = qu.getval("warn_times");
		info["deal_status"] = qu.getval("deal_status");
		dwTime = qu.getuval("last_warn_time_utc");
		info["last_warn_time"] = uitodate(dwTime);
		js["list"].Add(info);
	}
	js["count"] = i;
	qu.free_result();
	return 0;
}

static int DealMultiWarns()
{
	if(IsDaemonDenyOp(stConfig))
		return 0;

	const char *pWarns = hdf_get_value(stConfig.cgi->hdf, "Query.warn_list", NULL);
	int iStatus = hdf_get_int_value(stConfig.cgi->hdf, "Query.to_status", 0);
	if(pWarns == NULL || (iStatus != WARN_DEAL_STATUS_DO && iStatus != WARN_DEAL_STATUS_OK))
	{
		hdf_set_value(stConfig.cgi->hdf,"err.msg", CGI_REQERR);
		REQERR_LOG("invalid param warns:%p status:%d", pWarns, iStatus);
		return SLOG_ERROR_LINE;
	}

	Query & qu = *stConfig.qu;
	char sSqlBuf[512] = {0};
	if(WARN_DEAL_STATUS_DO == iStatus)
		snprintf(sSqlBuf, sizeof(sSqlBuf), "update mt_warn_info set deal_status=%d,start_deal_time_utc=%u "
			" where deal_status=%d and wid in(%s)", iStatus, (uint32_t)time(NULL), iStatus-1, pWarns);
	else
		snprintf(sSqlBuf, sizeof(sSqlBuf), "update mt_warn_info set deal_status=%d,end_deal_time_utc=%u "
			" where deal_status=%d and wid in(%s)", 
			iStatus, (uint32_t)time(NULL), iStatus-1, pWarns);
	
	if(!qu.execute(sSqlBuf))
	{
		WARN_LOG("execute sql:%s failed, msg:%s", sSqlBuf, qu.GetError().c_str());
		return SLOG_ERROR_LINE;
	}
	DEBUG_LOG("execute sql:%s ok", sSqlBuf);

	DealWarnsInShm(pWarns, iStatus);

	Json js;
	js["statusCode"] = 200;
	js["navTabId"] = "dmt_warn_list";
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
	return 0;
}

void GetAttrTypeInfoForTest(Json & js, int & iCount, MmapUserAttrTypeTree & stTypeTree)
{
	if(stTypeTree.sub_type_list_size() <= 0 && stTypeTree.attr_type_id() != 1)
	{
		Json jsInfo;
		if(GetAttrType(jsInfo, stTypeTree) >= 0)
		{
			Json jsType;
			char sType[16] = {0};
			snprintf(sType, sizeof(sType), "%d", stTypeTree.attr_type_id());
			iCount++;
			jsType["id"] = stTypeTree.attr_type_id();
			jsType["name"] = jsInfo[(const char*)sType];
		}
	}

	if(stTypeTree.sub_type_list_size() > 0)
	{
		for(int i=0; i < stTypeTree.sub_type_list_size(); i++)
		{
			MmapUserAttrTypeTree *pType = stTypeTree.mutable_sub_type_list(i);
			GetAttrTypeInfoForTest(js, iCount, *pType);
		}
	}
}

void GetAttrTypeAttrs(int iType, Json &js)
{
	AttrTypeInfo *pAttrType = slog.GetAttrTypeInfo(iType, NULL);
	if(NULL == pAttrType) {
		REQERR_LOG("get attr type:%d failed", iType);
		return ;
	}

	SharedHashTableNoList *pAttrHash = slog.GetAttrHash();
	int idx = pAttrType->iAttrIndexStart;
	AttrInfoBin *pAttr = NULL; 
	const char *pvname = NULL;
	int iCount = 0;
	for(int i=0; i < pAttrType->wAttrCount; i++)
	{
		pAttr = (AttrInfoBin*)NOLIST_HASH_INDEX_TO_NODE(pAttrHash, idx);
		Json js_attr;

		// check 一下
		if(pAttr->iAttrType != iType) {
			WARN_LOG("attr type check failed, %d != %d, attr:%d", pAttr->iAttrType, iType, pAttr->id);
			continue;
		}

		js_attr["id"] = pAttr->id;
		if(pAttr->iNameVmemIdx > 0) {
		    pvname = MtReport_GetFromVmem_Local(pAttr->iNameVmemIdx);
			js_attr["name"] = pvname;
		}
		else {
			js_attr["name"] = "unknow";
		}
		idx = pAttr->iAttrTypeNextIndex;
		iCount++;
		js["attrs"].Add(js_attr);
	}
	js["count"] = iCount;
}

int DealGetAttrTypeAttrs(CGI *cgi)
{
	int iType = hdf_get_int_value(cgi->hdf, "Query.type", -1);
	Json js;
	if(iType > 0)
		GetAttrTypeAttrs(iType, js);
	else {
		REQERR_LOG("invalid para, iType:%d", iType);
	}
	
	js["retcode"] = 0;

	STRING str;
	string_init(&str);
	if((stConfig.err=string_set(&str, js.ToString().c_str())) != STATUS_OK
		|| (stConfig.err=cgi_output(stConfig.cgi, &str)) != STATUS_OK)
	{
		string_clear(&str);
		return SLOG_ERROR_LINE;
	}
	string_clear(&str);
	DEBUG_LOG("get attr type attrs:%s", js.ToString().c_str());
	return 0;
}

int DealInitAttrReportTest(CGI *cgi)
{
	if(stConfig.iDisableVmemCache) {
		ERR_LOG("vmem not enable !");
		return SLOG_ERROR_LINE;
	}
	Json js;

	// 获取上报机器
	if(GetUserMachineListFromVmem(js) >= 0)
		DEBUG_LOG("get machine from vmem ok");
	else {
		ERR_LOG("get machine from vmem failed");
		return SLOG_ERROR_LINE;
	}

	int iCount = 0;
	MmapUserAttrTypeTree stTypeTree;
	if(GetAttrTypeTreeFromVmem(stConfig, stTypeTree) >= 0) {
		DEBUG_LOG("get attr type tree from vmem ok");
	}
	else {
		ERR_LOG("get attr type tree from vmem failed");
		return SLOG_ERROR_LINE;
	}
	GetAttrTypeInfoForTest(js, iCount, stTypeTree);
	js["cust_attr_type_count"] = iCount;
	DEBUG_LOG("get user attr type count:%d", iCount);

	std::string str(js.ToString());
	stConfig.err = hdf_set_value(stConfig.cgi->hdf, "config.test_para", str.c_str()); 
	if(stConfig.err != STATUS_OK)
	{
		ERR_LOG("set attr report test para failed, length:%lu", str.size());
		return SLOG_ERROR_LINE;
	}
	DEBUG_LOG("set attr report test para:%s", str.c_str());
	return 0;
}

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

	INFO_LOG("fcgi:%s argc:%d start pid:%u", stConfig.pszCgiName, argc, stConfig.pid);
	if(slog.InitMemcache() < 0)
	{
		return SLOG_ERROR_LINE;
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

		const char *pAction = stConfig.pAction;
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

		DEBUG_LOG("get action :%s from :%s", pAction, stConfig.remote);
		int32_t view_id = hdf_get_int_value(stConfig.cgi->hdf, "Query.view_id", 0);
		if(view_id != 0)
			hdf_set_int_value(stConfig.cgi->hdf, "config.view_id", view_id);

		if(!strcmp(pAction, "save_not_bind_machine"))
			iRet = SaveNotBindMachine(view_id);
		else if(!strcmp(pAction, "save_bind_machine"))
			iRet = SaveBindMachine(view_id);
		else if(!strcmp(pAction, "bind_machine"))
			iRet = DealBindMachine(view_id);

		else if(!strcmp(pAction, "bind_attr"))
			iRet = DealBindAttrList(view_id);
		else if(!strcmp(pAction, "bind_attr_search"))
			iRet = DealAttrSearch(view_id, true);
		else if(!strcmp(pAction, "not_bind_attr"))
			iRet = DealNotBindAttrList(view_id);
		else if(!strcmp(pAction, "not_bind_attr_search"))
			iRet = DealAttrSearch(view_id);
		else if(!strcmp(pAction, "save_bind_attr"))
			iRet = SaveBindAttr(view_id);
		else if(!strcmp(pAction, "save_not_bind_attr"))
			iRet = SaveNotBindAttr(view_id);
		else if(!strcmp(pAction, "show_single"))
			iRet = SetAttrSingle();
		else if(!strcmp(pAction, "show_single_attr_cust"))
		{
			hdf_set_int_value (stConfig.cgi->hdf, "Config.CompressionEnabled", 1);
			hdf_get_value (stConfig.cgi->hdf, "cgiout.ContentType", "text/html");
			iRet = ShowAttrSingle();
		}

		// 单机监控图表相关接口
		else if(!strcmp(pAction, "show_machine_attr"))
		{
			iRet = SetAttrTypeInfo();
			if(iRet >= 0)
				iRet = SetMachineAttr();
		}
		else if(!strcmp(pAction, "show_machine_attr_cust"))
		{
			hdf_set_int_value (stConfig.cgi->hdf, "Config.CompressionEnabled", 1);
			hdf_get_value (stConfig.cgi->hdf, "cgiout.ContentType", "text/html");
			g_isShowViewAttr = false;
			iRet = ShowAttrMulti(ATTR_SHOW_TYPE_MACHINE); 
		}

		// 视图监控图表相关接口
		else if(!strcmp(pAction, "show_view_attr_cust"))
		{
			hdf_set_int_value (stConfig.cgi->hdf, "Config.CompressionEnabled", 1);
			hdf_get_value (stConfig.cgi->hdf, "cgiout.ContentType", "text/html");
			g_isShowViewAttr = true;
			iRet = ShowAttrMulti(ATTR_SHOW_TYPE_VIEW); 
		}
		else if(!strcmp(pAction, "list"))
		{
			const char *pvName = hdf_get_value(stConfig.cgi->hdf, "Query.view_name", NULL);
			if(pvName != NULL)
				hdf_set_value(stConfig.cgi->hdf, "config.view_name", pvName);

			// 获取属性类型列表 -- 输出类型列表，以便显示类型名称
			iRet = SetAttrTypeInfo();
			if(iRet >= 0)
				iRet = SetViewAttr(view_id);
		}
		else if(!strcmp(pAction, "search_warn_list"))
		{
			iRet = DealWarnListSearch();
		}
		else if(!strcmp(pAction, "search_page_warn_list"))
		{
			iRet = DealWarnListSearchPage();
		}
		else if(!strcmp(pAction, "show_warn_list"))
		{
			iRet = DealGetWarnList();
		}
		else if(!strcmp(pAction, "deal_multi_warns"))
		{
			iRet = DealMultiWarns();
		}
		else if(!strcmp(pAction, "init_attr_report_test"))
			iRet = DealInitAttrReportTest(stConfig.cgi);
		else if(!strcmp(pAction, "get_attr_type_attrs"))
			iRet = DealGetAttrTypeAttrs(stConfig.cgi);
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

		const char *pcsTemplate = NULL;

		if(!strcmp(pAction, "add") || !strcmp(pAction, "mod"))
			pcsTemplate = "dmt_view_manage.html";
		else if(!strcmp(pAction, "list"))
			pcsTemplate = "dmt_showview.html";
		else if(!strcmp(pAction, "bind_machine"))
			pcsTemplate = "dmt_showview_bmachine.html";
		else if(!strcmp(pAction, "not_bind_attr") || !strcmp(pAction, "not_bind_attr_search"))
			pcsTemplate = "dmt_showview_not_bind_attr.html";
		else if(!strcmp(pAction, "bind_attr"))
			pcsTemplate = "dmt_showview_battr.html";
		else if(!strcmp(pAction, "bind_attr_search"))
			pcsTemplate = "dmt_showview_bind_attr.html";

		else if(!strcmp(pAction, "show_machine_attr"))
			pcsTemplate = "dmt_show_machine.html";
		else if(!strcmp(pAction, "show_single"))
			pcsTemplate = "dmt_show_single.html";
		else if(!strcmp(pAction, "show_warn_list") || !strcmp(pAction, "search_page_warn_list")
				|| !strcmp(pAction, "search_warn_list"))
			pcsTemplate = "dmt_warn_list.html";
		else if(!strcmp(pAction, "init_attr_report_test"))
			pcsTemplate = "dmt_dlg_attr_report_test.html";

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

	INFO_LOG(" fcgi - %s exist", stConfig.pszCgiName);
	return 0;
}

