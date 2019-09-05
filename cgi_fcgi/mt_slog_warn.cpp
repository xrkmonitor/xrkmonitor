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

   cgi mt_warn_config: 告警相关配置的实现

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

CSupperLog slog;
CGIConfig stConfig;

// ajax json 响应方式
static const char *s_JsonRequest [] = { 
	"del_multi_warn",
	"multi_umask_warn",
	NULL
};

enum{
	VIEW_FLAG_SHOW_ALL=1,
	VIEW_FLAG_SHOW_REP=2,
};

#define INVALID_VIEW_FLAG(flag) (flag!=VIEW_FLAG_SHOW_ALL && flag!=VIEW_FLAG_SHOW_REP)

typedef struct
{
	int obj_type;
	int obj_id;
	int warn_type;
	int warn_status;
}SearchInfo;

static int AddSearchInfo(char *psql, int ibufLen, SearchInfo *pinfo)
{
	char sTmpBuf[256] = {0};
	if(pinfo->obj_type != 0)
	{
		sTmpBuf[0] = '\0';
		if(pinfo->obj_type == ATTR_WARN_TYPE_MACHINE)
			sprintf(sTmpBuf, " and (warn_flag&%d) ", ATTR_WARN_FLAG_TYPE_MACHINE);
		else if(pinfo->obj_type == ATTR_WARN_TYPE_VIEW)
			sprintf(sTmpBuf, " and (warn_flag&%d) ", ATTR_WARN_FLAG_TYPE_VIEW);
		if(sTmpBuf[0] != '\0')
		{	strcat(psql, sTmpBuf);
			hdf_set_int_value(stConfig.cgi->hdf, "config.dwc_warn_obj_type", pinfo->obj_type);
		}
	}
	if(pinfo->warn_type != 0)
	{
		sprintf(sTmpBuf, " and (warn_flag&%d) ", pinfo->warn_type);
		strcat(psql, sTmpBuf);
		hdf_set_int_value(stConfig.cgi->hdf, "config.dwc_warn_type", pinfo->warn_type);
	}
	if(pinfo->warn_status != 0)
	{
		if(pinfo->warn_status == 32)
			sprintf(sTmpBuf, " and (warn_flag&%d) ", ATTR_WARN_FLAG_MASK_WARN);
		else
			sprintf(sTmpBuf, " and !(warn_flag&%d) ", ATTR_WARN_FLAG_MASK_WARN);
		strcat(psql, sTmpBuf);
		hdf_set_int_value(stConfig.cgi->hdf, "config.dwc_warn_status", pinfo->warn_status);
	}
	if(pinfo->obj_id != 0)
	{
		sprintf(sTmpBuf, " and warn_type_value=%d ", pinfo->obj_id);
		strcat(psql, sTmpBuf);
		hdf_set_int_value(stConfig.cgi->hdf, "config.dwc_obj_id", pinfo->obj_id);
	}

	DEBUG_LOG("after add search info sql:%s", psql);
	return 0;
}

static int GetWarnConfigTotalRecords(SearchInfo *pinfo=NULL)
{
	char sSqlBuf[512] = {0};
	Query qu(*stConfig.db);
	
	sprintf(sSqlBuf, "select count(*) from mt_warn_config where status=%d", RECORD_STATUS_USE);
	if(pinfo != NULL && AddSearchInfo(sSqlBuf, sizeof(sSqlBuf), pinfo) < 0)
		return SLOG_ERROR_LINE;

	DEBUG_LOG("get warn_config count - exesql:%s", sSqlBuf);
	if(qu.get_result(sSqlBuf) == NULL || qu.num_rows() <= 0)
	{
		qu.free_result();
		return 0;
	}

	qu.fetch_row();
	int iCount = qu.getval(0);
	qu.free_result();
	DEBUG_LOG("warn config records count:%d", iCount);
	return iCount;
}

static int SetWarnConfigFlag()
{
	hdf_set_int_value(stConfig.cgi->hdf, "config.flag_max", ATTR_WARN_FLAG_MAX);
	hdf_set_int_value(stConfig.cgi->hdf, "config.flag_min", ATTR_WARN_FLAG_MIN);
	hdf_set_int_value(stConfig.cgi->hdf, "config.flag_wave", ATTR_WARN_FLAG_WAVE);
	hdf_set_int_value(stConfig.cgi->hdf, "config.flag_view", ATTR_WARN_FLAG_TYPE_VIEW);
	hdf_set_int_value(stConfig.cgi->hdf, "config.flag_machine", ATTR_WARN_FLAG_TYPE_MACHINE);
	return 0;
}

static int GetWarnConfigList(Json &js, SearchInfo *pinfo=NULL)
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

	sprintf(sSqlBuf, "select mt_attr.attr_name,mt_warn_config.* from mt_warn_config inner join "
		" mt_attr on mt_warn_config.attr_id=mt_attr.id where mt_attr.status=%d and "
		" mt_warn_config.status=%d", RECORD_STATUS_USE, RECORD_STATUS_USE);
	if(pinfo != NULL && AddSearchInfo(sSqlBuf, sizeof(sSqlBuf), pinfo) < 0)
		return SLOG_ERROR_LINE;

	int iOrder = 0;
	iOrder = (iOrder==0 ? SetRecordsOrder(stConfig.cgi, sSqlBuf, "attr_id") : 1);
	iOrder = (iOrder==0 ? SetRecordsOrder(stConfig.cgi, sSqlBuf, "warn_type_value") : 1);
	iOrder = (iOrder==0 ? SetRecordsOrder(stConfig.cgi, sSqlBuf, "create_time") : 1);
	if(iOrder == 0) // 默认按创建时间降序
		strcat(sSqlBuf, " order by create_time desc");

	char sTmpBuf[64]={0};
	sprintf(sTmpBuf, " limit %d,%d", iNumPerPage*(iCurPage-1), iNumPerPage);
	strcat(sSqlBuf, sTmpBuf);
	DEBUG_LOG("get warn config list - exesql:%s", sSqlBuf);

	qu.get_result(sSqlBuf);
	int i=0;
	for(i=0; i < qu.num_rows() && qu.fetch_row() != NULL; i++)
	{
		Json attr;
		attr["config_id"] = qu.getval("warn_config_id");
		attr["attr_id"] = qu.getuval("attr_id");
		attr["attr_name"] = qu.getstr("attr_name");
		attr["warn_flag"] = qu.getval("warn_flag");
		attr["warn_obj_value"] = qu.getval("warn_type_value");
		attr["warn_max"] = qu.getval("max_value");
		attr["warn_min"] = qu.getval("min_value");
		attr["warn_wave"] = qu.getval("wave_value");
		attr["add_time"] = qu.getstr("create_time");
		js["list"].Add(attr);
	}
	js["count"] = i; 
	DEBUG_LOG("get warn config list - result count:%d(%d)", qu.num_rows(), i);
	qu.free_result();
	SetWarnConfigFlag();
	return 0;
}

static int DealSetWarnChart()
{
	int iWarnCfgId = hdf_get_int_value(stConfig.cgi->hdf, "Query.warn_cfg_id", -1);
	const char *pwarn_type = hdf_get_value(stConfig.cgi->hdf, "Query.warn_type", NULL);
	const char *pshow_class = hdf_get_value(stConfig.cgi->hdf, "Query.show_class", NULL);
	const char *pattr_name = hdf_get_value(stConfig.cgi->hdf, "Query.attr_name", NULL);
	if(pwarn_type==NULL || pshow_class==NULL || pattr_name==NULL)
	{
		hdf_set_value(stConfig.cgi->hdf,"err.msg", CGI_REQERR);
		REQERR_LOG("invalid parameter %p|%p|%p", pwarn_type, pshow_class, pattr_name);
		return SLOG_ERROR_LINE;
	}

	hdf_set_value(stConfig.cgi->hdf, "config.attr_name", pattr_name);
	hdf_set_value(stConfig.cgi->hdf, "config.warn_type_show", pwarn_type);
	hdf_set_value(stConfig.cgi->hdf, "config.show_class", pshow_class);

	if(iWarnCfgId <= 0)
	{
		hdf_set_int_value(stConfig.cgi->hdf, "config.warn_config_id", 0);

		int iWarnTypeId = hdf_get_int_value(stConfig.cgi->hdf, "Query.warn_type_id", -1);
		int iWarnAttrId = hdf_get_int_value(stConfig.cgi->hdf, "Query.warn_attr_id", -1);
		const char *ptype = hdf_get_value(stConfig.cgi->hdf, "Query.warn_type", NULL);

		DEBUG_LOG("try init first set warn config, type id:%d, attr id:%d, type:%s", 
			iWarnTypeId, iWarnAttrId, (ptype!=NULL ? ptype : ""));
		if(iWarnTypeId <= 0 || iWarnAttrId <= 0 || ptype == NULL)
		{
			hdf_set_value(stConfig.cgi->hdf,"err.msg", CGI_REQERR);
			REQERR_LOG("invalid parameter %d|%d|%p", iWarnTypeId, iWarnAttrId, ptype);
			return SLOG_ERROR_LINE;
		}
		hdf_set_int_value(stConfig.cgi->hdf, "config.warn_type_id", iWarnTypeId);
		hdf_set_int_value(stConfig.cgi->hdf, "config.warn_attr_id", iWarnAttrId);
		if(!strcmp(ptype, "view"))
			hdf_set_int_value(stConfig.cgi->hdf, "config.warn_type", 2);
		else
			hdf_set_int_value(stConfig.cgi->hdf, "config.warn_type", 1);
		return 0;
	}

	char sSqlBuf[512] = {0};
	Query qu_data(*stConfig.db);
	snprintf(sSqlBuf, sizeof(sSqlBuf), "select * from mt_warn_config "
		" where warn_config_id=%d and status=0", iWarnCfgId);
	qu_data.get_result(sSqlBuf);
	if(qu_data.num_rows() <= 0 || false == qu_data.fetch_row())
	{
		hdf_set_value(stConfig.cgi->hdf,"err.msg", CGI_REQERR);
		REQERR_LOG("execute sql:%s failed, warn config id:%d", sSqlBuf, iWarnCfgId);
		qu_data.free_result();
		return SLOG_ERROR_LINE;
	}

	int iFlag = qu_data.getval("warn_flag");
	if(iFlag & ATTR_WARN_FLAG_MAX)
		hdf_set_int_value(stConfig.cgi->hdf, "config.warn_max", qu_data.getval("max_value"));
	if(iFlag & ATTR_WARN_FLAG_MIN)
		hdf_set_int_value(stConfig.cgi->hdf, "config.warn_min", qu_data.getval("min_value"));
	if(iFlag & ATTR_WARN_FLAG_WAVE)
		hdf_set_int_value(stConfig.cgi->hdf, "config.warn_wave", qu_data.getval("wave_value"));

	hdf_set_int_value(stConfig.cgi->hdf, "config.warn_config_id", iWarnCfgId);
	hdf_set_int_value(stConfig.cgi->hdf, "config.attr_id", qu_data.getval("attr_id"));
	hdf_set_int_value(stConfig.cgi->hdf, "config.warn_type_id", qu_data.getval("warn_type_value"));

	qu_data.free_result();

	DEBUG_LOG("init set warn config ok, warn config id:%d", iWarnCfgId);
	return 0;
}

static int DealWarnMask()
{
	if(IsDaemonDenyOp(stConfig))
		return 0;

	int iWarnCfgId = hdf_get_int_value(stConfig.cgi->hdf, "Query.warn_cfg_id", -1);
	int iMask = hdf_get_int_value(stConfig.cgi->hdf, "Query.mask", -1);
	if(iWarnCfgId <= 0 || iMask < 0)
	{
		hdf_set_value(stConfig.cgi->hdf,"err.msg", CGI_REQERR);
		REQERR_LOG("invalid parameter, iWarnCfgId:%d, iMask:%d", iWarnCfgId, iMask);
		return SLOG_ERROR_LINE;
	}

	FloginInfo *pUserInfo = stConfig.stUser.puser_info;

	char sSqlBuf[512] = {0};
	Query qu_data(*stConfig.db);
	snprintf(sSqlBuf, sizeof(sSqlBuf), 
		"select * from mt_warn_config where warn_config_id=%d and status=0", iWarnCfgId);
	qu_data.get_result(sSqlBuf);
	if(qu_data.num_rows() <= 0 || false == qu_data.fetch_row())
	{
		hdf_set_value(stConfig.cgi->hdf,"err.msg", CGI_REQERR);
		REQERR_LOG("execute sql:%s failed, warn config id:%d, mask:%d", sSqlBuf, iWarnCfgId, iMask);
		qu_data.free_result();
		return SLOG_ERROR_LINE;
	}

	int iFlag = qu_data.getval("warn_flag");

	Json js;
	if(iFlag & ATTR_WARN_FLAG_MAX)
		js["warn_max"] = qu_data.getval("max_value");
	if(iFlag & ATTR_WARN_FLAG_MIN)
		js["warn_min"] = qu_data.getval("min_value");
	if(iFlag & ATTR_WARN_FLAG_WAVE)
		js["warn_wave"] = qu_data.getval("wave_value");

	if(iMask)
		iFlag |= ATTR_WARN_FLAG_MASK_WARN;
	else
		iFlag &= ~ATTR_WARN_FLAG_MASK_WARN;
	js["warn_flag"] = iFlag;
	js["warn_config_id"] = iWarnCfgId;
	js["attr_id"] = qu_data.getval("attr_id");
	js["warn_type_id"] = qu_data.getval("warn_type_value");

	qu_data.free_result();

	snprintf(sSqlBuf, sizeof(sSqlBuf), "update mt_warn_config set warn_flag=%d,user_mod_id=%d "
		" where warn_config_id=%d", iFlag, pUserInfo->iUserId, iWarnCfgId);
	if(!qu_data.execute(sSqlBuf))
	{
	    ERR_LOG("execute sql:%s failed, msg:%s", sSqlBuf, qu_data.GetError().c_str());
		js["statusCode"] = -1;
		js["msgid"] = "oprFailed";
	}
	else
	{
		js["statusCode"] = 200;
		js["msgid"] = "oprSuccess"; 
	}

	STRING str;
	string_init(&str);
	if((stConfig.err=string_set(&str, js.ToString().c_str())) != STATUS_OK
		|| (stConfig.err=cgi_output(stConfig.cgi, &str)) != STATUS_OK)
	{
		string_clear(&str);
		return SLOG_ERROR_LINE;
	}

	DEBUG_LOG("set warn mask config ok, warn config id:%d, mask:%d", iWarnCfgId, iMask);
	return 0;
}

static int DealMultiDel()
{
	if(IsDaemonDenyOp(stConfig))
		return 0;

	const char *pwlist = hdf_get_value(stConfig.cgi->hdf, "Query.warn_list", NULL);
	if(pwlist == NULL)
	{
		WARN_LOG("invalid parameter from:%s", stConfig.remote);
		hdf_set_value(stConfig.cgi->hdf, "err.msg", CGI_REQERR);
		return SLOG_ERROR_LINE;
	}
	char *wlist = strdup(pwlist);
	char *pwarn_config = strtok(wlist, ",");
	char sSqlBuf[512] = {0};
	Query qutmp(*stConfig.db);
	int iDelete = 0;
	for(; pwarn_config != NULL; pwarn_config=strtok(NULL, ","))
	{
		sprintf(sSqlBuf, "update mt_warn_config set status=%d,user_mod_id=%d"
			" where warn_config_id=%s", 
			RECORD_STATUS_DELETE, stConfig.stUser.puser_info->iUserId, pwarn_config);
		if(!qutmp.execute(sSqlBuf))
		{
			ERR_LOG("execute sql:%s failed, msg:%s", sSqlBuf, qutmp.GetError().c_str());
			continue;
		}
		iDelete++;
	}
	DEBUG_LOG("try delete warn config :%s success count:%d from:%s", pwlist, iDelete, stConfig.remote);

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
	return 0;
}

static int DealMultiMod()
{
	const char *pwlist = hdf_get_value(stConfig.cgi->hdf, "Query.warn_list", NULL);
	if(pwlist == NULL)
	{
		WARN_LOG("invalid parameter from:%s", stConfig.remote);
		hdf_set_value(stConfig.cgi->hdf, "err.msg", CGI_REQERR);
		return SLOG_ERROR_LINE;
	}

	hdf_set_value(stConfig.cgi->hdf, "config.action", "save_multi_mod");
	hdf_set_value(stConfig.cgi->hdf, "config.warn_list", pwlist);
	DEBUG_LOG("try update warn config :%s from:%s", pwlist, stConfig.remote);
	return 0;
}

static int DealSaveWarnConfig(bool bIsMod=false)
{
	if(IsDaemonDenyOp(stConfig))
		return 0;

	int32_t iAttrId = hdf_get_int_value(stConfig.cgi->hdf, "Query.ddwc_attr_id", 0);
	const char *pWarnList  = hdf_get_value(stConfig.cgi->hdf, "Query.ddwc_mod_warn_list", NULL);
	const char *pmax = hdf_get_value(stConfig.cgi->hdf, "Query.c_warn_max", NULL);
	const char *pmin = hdf_get_value(stConfig.cgi->hdf, "Query.c_warn_min", NULL);
	const char *pwave= hdf_get_value(stConfig.cgi->hdf, "Query.c_warn_wave", NULL);
	int32_t iMax = hdf_get_int_value(stConfig.cgi->hdf, "Query.ddwc_warn_max", 0);
	int32_t iMin = hdf_get_int_value(stConfig.cgi->hdf, "Query.ddwc_warn_min", 0);
	int32_t iWave = hdf_get_int_value(stConfig.cgi->hdf, "Query.ddwc_warn_wave", 0); 
	int32_t iWarnType = hdf_get_int_value(stConfig.cgi->hdf, "Query.ddwc_warn_type", 0);
	const char *pcallBkType = hdf_get_value(stConfig.cgi->hdf, "Query.ddwc_call_back_type", "CloseCurrent");
	const char *pnavTabId = hdf_get_value(
		stConfig.cgi->hdf, "Query.ddwc_reload_navTab_id", "dmt_warn_config");
	int32_t iWarnTypeValue = hdf_get_int_value(stConfig.cgi->hdf, "Query.ddwc_warn_type_val", 0);

	DEBUG_LOG("iAttrId:%d, pWarnList:%s, pmax:%s, pmin:%s, pwave:%s, iMax:%d, iMin:%d, iWave:%d, iWarnType:%d, "
		"iWarnTypeValue:%d", iAttrId, (pWarnList==NULL ? "" :pWarnList), (pmax==NULL ? "" : pmax),
		(pmin==NULL ? "" : pmin), (pwave==NULL ? "" : pwave), iMax, iMin, iWave, iWarnType, iWarnTypeValue);

	if((!bIsMod && (iAttrId == 0 || iWarnType == 0 || iWarnTypeValue == 0
		|| (pmax!=NULL && iMax==0) 
		|| (pmin!=NULL && iMin==0) 
		|| (pwave!=NULL && iWave==0)
		|| (pmax==NULL && pmin==NULL && pwave==NULL)))
		|| (bIsMod && pWarnList == NULL))
	{
		REQERR_LOG("bMod:%d invalid param iAttrId:%d WarnList:%s (max:%s:%d,min:%s:%d,wave:%s:%d)"
			"iWarnType:%d iWarnTypeValue:%d", bIsMod, iAttrId, pWarnList, 
			pmax, iMax, pmin, iMin, pwave, iWave, iWarnType, iWarnTypeValue);

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

	IM_SQL_PARA* ppara = NULL;
	InitParameter(&ppara);

	int32_t iWarnFlag = 0, iNewWarnFlag = 0;
	if(pmax != NULL)
	{
		iWarnFlag |= ATTR_WARN_FLAG_MAX;
		AddParameter(&ppara, "max_value", iMax, "DB_CAL");
	}
	if(pmin != NULL)
	{
		iWarnFlag |= ATTR_WARN_FLAG_MIN;
		AddParameter(&ppara, "min_value", iMin, "DB_CAL");
	}
	if(pwave != NULL)
	{
		iWarnFlag |= ATTR_WARN_FLAG_WAVE;
		AddParameter(&ppara, "wave_value", iWave, "DB_CAL");
	}

	if(iWarnType == ATTR_WARN_TYPE_MACHINE)
		iWarnFlag |= ATTR_WARN_FLAG_TYPE_MACHINE;
	else if(iWarnType == ATTR_WARN_TYPE_VIEW)
		iWarnFlag |= ATTR_WARN_FLAG_TYPE_VIEW;

	std::string strSql;
	Query qu(*stConfig.db);

	int iwarn_config_id = 0;
	AddParameter(&ppara, "user_mod_id", stConfig.stUser.puser_info->iUserId, "DB_CAL");
	if(!bIsMod)
	{
		AddParameter(&ppara, "user_add_id", stConfig.stUser.puser_info->iUserId, "DB_CAL");
		AddParameter(&ppara, "warn_flag", iWarnFlag, "DB_CAL");
		AddParameter(&ppara, "attr_id", iAttrId, "DB_CAL");
		AddParameter(&ppara, "warn_type_value", iWarnTypeValue, "DB_CAL");
		AddParameter(&ppara, "user_add",
			stConfig.pshmLoginList->stLoginList[stConfig.stUser.iLoginIndex].szUserName, NULL);
		strSql = "insert into mt_warn_config";
		JoinParameter_Insert(&strSql, qu.GetMysql(), ppara);
		if(!qu.execute(strSql))
		{
			ERR_LOG("execute sql:%s failed, msg:%s", strSql.c_str(), qu.GetError().c_str());
			return SLOG_ERROR_LINE;
		}

		iwarn_config_id = qu.insert_id();
		iNewWarnFlag = iWarnFlag;
		DEBUG_LOG("add new warn config, exec sql:%s success ", strSql.c_str());
	}
	else
	{
		Query qutmp(*stConfig.db);
		char *wlist = strdup(pWarnList);
		char *pwarn_config = strtok(wlist, ",");
		char sSqlBuf[512] = {0};
		const char *pclear_other = hdf_get_value(stConfig.cgi->hdf, "Query.c_warn_clear_not_sel", NULL);
		const char *pmask_sel = hdf_get_value(stConfig.cgi->hdf, "Query.c_warn_mask_sel", NULL);
		const char *pnot_mask_sel = hdf_get_value(stConfig.cgi->hdf, "Query.c_warn_not_mask_sel", NULL);
		JoinParameter_Set(&strSql, qu.GetMysql(), ppara);
		for(; pwarn_config != NULL; pwarn_config=strtok(NULL, ","))
		{
			iwarn_config_id = atoi(pwarn_config);

			sprintf(sSqlBuf, "select * from mt_warn_config where status=%d and warn_config_id=%s",
				RECORD_STATUS_USE, pwarn_config);
			if(qutmp.get_result(sSqlBuf) == NULL || qutmp.num_rows() <= 0)
			{
				REQERR_LOG("execu sql:%s failed !", sSqlBuf);
				qutmp.free_result();
				continue;
			}
			qutmp.fetch_row();
			DEBUG_LOG("execu:%s ok ,flag:%d", sSqlBuf, qutmp.getval("warn_flag"));

			if(pclear_other == NULL)
				iNewWarnFlag = (qutmp.getval("warn_flag") | iWarnFlag);
			else
			{
				iNewWarnFlag = (qutmp.getval("warn_flag") & 
					~(ATTR_WARN_FLAG_MAX+ATTR_WARN_FLAG_MIN+ATTR_WARN_FLAG_WAVE));
				iNewWarnFlag |= iWarnFlag;
			}
			if(pmask_sel != NULL)
				iNewWarnFlag |= ATTR_WARN_FLAG_MASK_WARN;
			if(pnot_mask_sel != NULL)
				iNewWarnFlag &= ~ATTR_WARN_FLAG_MASK_WARN;

			if(!(iNewWarnFlag & (ATTR_WARN_FLAG_MAX+ATTR_WARN_FLAG_MIN+ATTR_WARN_FLAG_WAVE)))
			{
				sprintf(sSqlBuf, "update mt_warn_config set status=%d,user_mod_id=%d "
					" where warn_config_id=%s",
					RECORD_STATUS_DELETE, stConfig.stUser.puser_info->iUserId, pwarn_config);
			}
			else
			{
				sprintf(sSqlBuf, "update mt_warn_config set warn_flag=%d,%s "
					" where warn_config_id=%s", iNewWarnFlag, strSql.c_str(), pwarn_config);
			}

			iAttrId =  qutmp.getval("attr_id");
			iWarnTypeValue = qutmp.getval("warn_type_value");
			if(pmax == NULL && (iNewWarnFlag&ATTR_WARN_FLAG_MAX))
				iMax = qutmp.getval("max_value");
			if(pmin == NULL && (iNewWarnFlag&ATTR_WARN_FLAG_MIN))
				iMin = qutmp.getval("min_value");
			if(pwave == NULL && (iNewWarnFlag&ATTR_WARN_FLAG_WAVE))
				iWave = qutmp.getval("wave_value");

			qutmp.free_result();
			if(!qu.execute(sSqlBuf))
			{
				ERR_LOG("execute sql:%s failed, msg:%s", sSqlBuf, qu.GetError().c_str());
				continue;
			}
			DEBUG_LOG("execu:%s success ", sSqlBuf);
		}
		free(wlist);
	}
	ReleaseParameter(&ppara);

	Json js;
	js["statusCode"] = 200;
	js["navTabId"] = pnavTabId;
	js["callbackType"] = pcallBkType;
	
	// add for chart set 
	js["attr_id"] = iAttrId;
	js["warn_type_id"] = iWarnTypeValue;
	js["warn_flag"] = iNewWarnFlag;
	js["warn_max"] = iMax;
	js["warn_min"] = iMin;
	js["warn_wave"] = iWave;
	js["warn_config_id"] = iwarn_config_id;

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
	return 0;
}

static int DealSaveSetWarnChart()
{
	if(IsDaemonDenyOp(stConfig))
		return 0;

	const char *pCfgId = hdf_get_value(stConfig.cgi->hdf, "Query.ddwc_warn_config_id", NULL);
	bool bIsMod = false;
	if(pCfgId != NULL && strcmp(pCfgId, "0"))
	{
		hdf_set_value(stConfig.cgi->hdf, "Query.ddwc_mod_warn_list", pCfgId);
		bIsMod = true;
	}

	DEBUG_LOG("set chart warn config - %s", (pCfgId != NULL ? pCfgId : ""));
	return DealSaveWarnConfig(bIsMod);
}

static int DealAddWarnConfig()
{
	int iType = hdf_get_int_value(stConfig.cgi->hdf, "Query.type", 0);
	if(iType != ATTR_WARN_TYPE_MACHINE && iType != ATTR_WARN_TYPE_VIEW) // 单机, 视图
	{
		ERR_LOG("invalid warn type:%d for add !", iType); 
		return SLOG_ERROR_LINE;
	}
	hdf_set_int_value(stConfig.cgi->hdf, "config.type", iType);
	hdf_set_value(stConfig.cgi->hdf, "config.action", "save_new_warn_config");
	DEBUG_LOG("try add warn type :%d (%s)", iType, (iType==1 ? "machine" : "view"));
	return 0;
}

static int DealWarnConfigSearch()
{
	SearchInfo stInfo;
	memset(&stInfo, 0, sizeof(stInfo));
	stInfo.obj_type = hdf_get_int_value(stConfig.cgi->hdf, "Query.dwc_warn_obj_type", 0);
	stInfo.obj_id = hdf_get_int_value(stConfig.cgi->hdf, "Query.dwc_obj_id", 0);
	stInfo.warn_type = hdf_get_int_value(stConfig.cgi->hdf, "Query.dwc_warn_type", 0);
	stInfo.warn_status = hdf_get_int_value(stConfig.cgi->hdf, "Query.dwc_warn_status", 0);

	DEBUG_LOG("search info obj_type:%d obj_id:%d, warn_type:%d, status:%d",
		stInfo.obj_type, stInfo.obj_id, stInfo.warn_type, stInfo.warn_status);

	int iRecords = GetWarnConfigTotalRecords(&stInfo);
	if(iRecords < 0)
	{
		ERR_LOG("get view records count failed !");
		return SLOG_ERROR_LINE;
	}
	SetRecordsPageInfo(stConfig.cgi, iRecords);

	Json js;
	if(GetWarnConfigList(js, &stInfo) < 0)
		return SLOG_ERROR_LINE;

	std::string str(js.ToString());
	DEBUG_LOG("warn config list json:%s", str.c_str());

	stConfig.err = hdf_set_value(stConfig.cgi->hdf, "config.warn_config_info", str.c_str()); 
	if(stConfig.err != STATUS_OK)
	{
		ERR_LOG("set warn config info failed, length:%u", (uint32_t)str.size());
		return SLOG_ERROR_LINE;
	}
	return 0;
}

static int DealListWarnConfig()
{
	int iRecords = GetWarnConfigTotalRecords();
	if(iRecords < 0)
	{
		ERR_LOG("get attr record count failed !");
		return SLOG_ERROR_LINE;
	}
	SetRecordsPageInfo(stConfig.cgi, iRecords);

	Json js;
	if(GetWarnConfigList(js) < 0)
		return SLOG_ERROR_LINE;

	std::string str(js.ToString());
	DEBUG_LOG("warn config list json:%s", str.c_str());

	stConfig.err = hdf_set_value(stConfig.cgi->hdf, "config.warn_config_info", str.c_str()); 
	if(stConfig.err != STATUS_OK)
	{
		ERR_LOG("set warn config info failed, length:%u", (uint32_t)str.size());
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

		if(!strcmp(pAction, "add_warn"))
			iRet = DealAddWarnConfig();
		else if(!strcmp(pAction, "save_new_warn_config"))
			iRet = DealSaveWarnConfig();
		else if(!strcmp(pAction, "mod_multi_warn"))
			iRet = DealMultiMod();
		else if(!strcmp(pAction, "save_multi_mod"))
			iRet = DealSaveWarnConfig(true);
		else if(!strcmp(pAction, "del_multi_warn"))
			iRet = DealMultiDel();
		else if(!strcmp(pAction, "search"))
			iRet = DealWarnConfigSearch();
		else if(!strcmp(pAction, "mask_warn_config"))
			iRet = DealWarnMask();
		else if(!strcmp(pAction, "chart_set_attr_warn"))
			iRet = DealSetWarnChart();
		else if(!strcmp(pAction, "save_set_warn_chart"))
			iRet = DealSaveSetWarnChart();
		else if(!strcmp(pAction, "multi_mask_warn") || !strcmp(pAction, "multi_umask_warn"))
			iRet = DealSaveWarnConfig(true);

		else  // default -- list
		{
			pAction = "list";
			iRet = DealListWarnConfig();
		}
		if(iRet < 0)
		{
			show_errpage(NULL, NULL, stConfig);
			continue;
		}

		const char *pcsTemplate = NULL;
		if(!strcmp(pAction, "add_warn") || !strcmp(pAction, "mod_warn")
			|| !strcmp(pAction, "mod_multi_warn"))
			pcsTemplate = "dmt_dlg_warn_config.html";
		else if(!strcmp(pAction, "list") || !strcmp(pAction, "search"))
			pcsTemplate = "dmt_warn_config.html";
		else if(!strcmp(pAction, "chart_set_attr_warn"))
			pcsTemplate = "dmt_dlg_warn_config_chart.html";

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

