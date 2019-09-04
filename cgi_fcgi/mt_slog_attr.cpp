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

   fastcgi mt_slog_attr : 监控点/监控点类型相关

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
#include <iostream>
#include <sstream>
#include <string>
#include "Memcache.h"

// 判断属性类型合法性
#define INVALID_ATTR_DATA_TYPE(t) (t < SUM_REPORT_TYPE_MIN || t > SUM_REPORT_TYPE_MAX)

CSupperLog slog;
CGIConfig stConfig;

// ajax json 响应方式
static const char *s_JsonRequest [] = { 
	"action",
	"attr_delete",
	"delete",
	"add",
	"mod",
	"save_new_attr",
	"save_mod_attr",
	NULL
};

typedef struct
{
	int iAttrDataType;
	const char *pkey;
	int iAttrType;
	const char *pattrType;
	int iExcpAttrMasked;
}AttrSearchInfo;

static int GetAttrTotalRecords(AttrSearchInfo *pinfo=NULL)
{
	char sSqlBuf[1024] = {0};
	char sTmpBuf[256] = {0};
	Query & qu = *stConfig.qu;
	
	sprintf(sSqlBuf, "select count(*) from mt_attr inner join mt_attr_type on "
		" mt_attr.attr_type=mt_attr_type.type where mt_attr.status=0 and mt_attr_type.status=0");
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

	if(pinfo != NULL && pinfo->pkey != NULL && pinfo->pkey[0] != '\0' && isnumber(pinfo->pkey))
	{
		memset(sTmpBuf, 0, sizeof(sTmpBuf));
		snprintf(sTmpBuf, sizeof(sTmpBuf)-1, " and mt_attr.id=%d ", atoi(pinfo->pkey));
		if(strlen(sTmpBuf) + strlen(sSqlBuf) >= sizeof(sSqlBuf))
		{
			REQERR_LOG("search key(%s) too long, tmp:%s sql:%s", pinfo->pkey, sTmpBuf, sSqlBuf);
			hdf_set_value(stConfig.cgi->hdf, "err.msg", CGI_REQERR);
			return SLOG_ERROR_LINE;
		}
		strcat(sSqlBuf, sTmpBuf);
	}

	qu.get_result(sSqlBuf);
	if(qu.num_rows() <= 0 || qu.fetch_row() == NULL)
	{
		qu.free_result();
		ERR_LOG("get attr count failed! sql:%s", sSqlBuf);
		return SLOG_ERROR_LINE;
	}
	DEBUG_LOG("get attr count - exesql:%s", sSqlBuf);

	int iCount = qu.getval(0);
	qu.free_result();
	DEBUG_LOG("records count:%d", iCount);
	return iCount;
}

static int GetAttrList(Json &js, AttrSearchInfo *pinfo=NULL)
{
	char sSqlBuf[1024] = {0};
	char sTmpBuf[256] = {0};
	Query & qu = *stConfig.qu;
	int iCurPage = hdf_get_int_value(stConfig.cgi->hdf, "config.currentPage", 0);
	int iNumPerPage = hdf_get_int_value(stConfig.cgi->hdf, "config.numPerPage", 0);
	if(iCurPage == 0 || iNumPerPage == 0)
	{
		ERR_LOG("invalid iCurPage(%d) or iNumPerPage(%d)", iCurPage, iNumPerPage);
		return SLOG_ERROR_LINE;
	}

	sprintf(sSqlBuf, "select mt_attr.*,mt_attr_type.name from mt_attr inner join mt_attr_type on "
		" mt_attr.attr_type=mt_attr_type.type where mt_attr.status=0 and mt_attr_type.status=0");
	if(pinfo != NULL && pinfo->iAttrDataType != 0)
	{
		sprintf(sTmpBuf, " and mt_attr.data_type=%d", pinfo->iAttrDataType);
		strcat(sSqlBuf, sTmpBuf);

		// 参数重新设置回去，以便排序显示时可以按结果排序
		hdf_set_int_value(stConfig.cgi->hdf, "config.da_attr_data_type", pinfo->iAttrDataType);
		if(pinfo->iAttrDataType == EX_REPORT) {
			if(pinfo->iExcpAttrMasked) {
				strcat(sSqlBuf, " and mt_attr.excep_attr_mask=1");
				hdf_set_int_value(stConfig.cgi->hdf, "config.da_excp_mask", 1);
			}
		}
	}

	if(pinfo != NULL && pinfo->iAttrType != 0 && pinfo->pattrType != NULL)
	{
		sprintf(sTmpBuf, " and mt_attr.attr_type=%d", pinfo->iAttrType);
		strcat(sSqlBuf, sTmpBuf);
		hdf_set_int_value(stConfig.cgi->hdf, "config.dam_attr_type", pinfo->iAttrType);
		hdf_set_value(stConfig.cgi->hdf, "config.dam_attr_type_name", pinfo->pattrType);
	}

	if(pinfo != NULL && pinfo->pkey != NULL && pinfo->pkey[0] != '\0' && isnumber(pinfo->pkey))
	{
		memset(sTmpBuf, 0, sizeof(sTmpBuf));
		snprintf(sTmpBuf, sizeof(sTmpBuf)-1, " and mt_attr.id=%d ", atoi(pinfo->pkey));
		if(strlen(sTmpBuf) + strlen(sSqlBuf) >= sizeof(sSqlBuf)-128)
		{
			REQERR_LOG("search key(%s) too long, tmp:%s sql:%s", pinfo->pkey, sTmpBuf, sSqlBuf);
			hdf_set_value(stConfig.cgi->hdf, "err.msg", CGI_REQERR);
			return SLOG_ERROR_LINE;
		}
		strcat(sSqlBuf, sTmpBuf);
		hdf_set_value(stConfig.cgi->hdf, "config.da_keyword", pinfo->pkey);
	}

	int iOrder = 0;
	iOrder = (iOrder==0 ? SetRecordsOrder(stConfig.cgi, sSqlBuf, "id") : 1);
	iOrder = (iOrder==0 ? SetRecordsOrder(stConfig.cgi, sSqlBuf, "create_time") : 1);
	iOrder = (iOrder==0 ? SetRecordsOrder(stConfig.cgi, sSqlBuf, "attr_type") : 1);
	if(iOrder == 0) // 默认按创建时间降序
		strcat(sSqlBuf, " order by create_time desc");

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
		attr["id"] = qu.getuval("id");
		attr["name"] = qu.getstr("attr_name");
		attr["attr_type"] = qu.getstr("name");
		attr["attr_type_id"] = qu.getval("attr_type");
		attr["data_type"] = qu.getval("data_type");
		attr["attr_desc"] = qu.getstr("attr_desc");
		attr["user_add"] = qu.getstr("user_add");
		attr["add_time"] = qu.getstr("create_time");
		attr["excep_attr_mask"] = qu.getval("excep_attr_mask");
		attr["inner"] = qu.getval("inner_attr");
		js["list"].Add(attr);
	}
	js["count"] = i; 
	DEBUG_LOG("get attr list - result count:%d(%d)", qu.num_rows(), i);
	qu.free_result();
	return 0;
}

static int GetAttrType(Json &js, int32_t iType)
{
	char sSqlBuf[256] = {0};
	Query qu(*stConfig.db);
	sprintf(sSqlBuf, "select * from mt_attr_type where status=%d and type=%d ",
		RECORD_STATUS_USE, iType);
	qu.get_result(sSqlBuf);
	if(qu.num_rows() == 0) {
		qu.free_result();
		js["subcount"] = 0;
		DEBUG_LOG("get attr type count is 0, sql:%s", sSqlBuf);
		return 0;
	}
		
	if(qu.fetch_row() == NULL)
	{
		qu.free_result();
		ERR_LOG("get attr type :%d failed sql:%s!", iType, sSqlBuf);
		return SLOG_ERROR_LINE;
	}

	if(qu.num_rows() > 1)
		WARN_LOG("get attr type :%d, have count:%d, use first !", iType, qu.num_rows());

	js["type"] = iType;
	js["name"] = qu.getstr("name");
	js["type_pos"] = qu.getstr("type_pos");
	js["desc"] = qu.getstr("attr_desc");
	js["create_user"] = qu.getstr("create_user");
	js["mod_user"] = qu.getstr("mod_user");
	js["update_time"] = qu.getstr("update_time");
	js["create_time"] = qu.getstr("create_time");

	AttrTypeInfo * pAttrTypeInfo = NULL;
	pAttrTypeInfo = slog.GetAttrTypeInfo(iType, NULL);
	if(pAttrTypeInfo == NULL)
	{
		WARN_LOG("get attr type:%d info from shm failed", iType);
		js["attr_count"] = 0;
	}
	else
		js["attr_count"] = pAttrTypeInfo->wAttrCount;

	DEBUG_LOG("get attr type :%d attr count:%d, name:%s type_pos:%s parent_type:%d",
		iType, (int)js["attr_count"], qu.getstr("name"), qu.getstr("type_pos"), qu.getval("parent_type"));
	qu.free_result();

	// 尝试获取子类型
	sprintf(sSqlBuf, "select type from mt_attr_type where status=%d and parent_type=%d",
		RECORD_STATUS_USE, iType);
	qu.get_result(sSqlBuf);
	int i=0, iRet=0, iCount = 0;
	for(i=0; i < qu.num_rows() && qu.fetch_row() != NULL; i++)
	{
		Json subtype;
		if((iRet=GetAttrType(subtype, qu.getval("type"))) < 0)
		{
			qu.free_result();
			return SLOG_ERROR_LINE;
		}
		if(iRet > 0) {
			js["list"].Add(subtype);
			iCount++;
		}
	}
	js["subcount"] = iCount; 
	qu.free_result();
	return iCount+1; // 1 -- iType 的数据已获取 
}

static int UpdateAttrType()
{
	if(IsDaemonDenyOp(stConfig))
		return 0;

	int itype = hdf_get_int_value(stConfig.cgi->hdf, "Query.mod_attr_type", 0);
	const char *pname = hdf_get_value(stConfig.cgi->hdf, "Query.attr_type_name", NULL);
	const char *pdesc = hdf_get_value(stConfig.cgi->hdf, "Query.attr_type_desc", NULL);
	if(0 == itype || pname==NULL || pdesc==NULL)
	{
		WARN_LOG(CGI_BASIC_LOG_FMT"invalid parameter type:%d pname:%s pdesc:%s from:%s", 
			CGI_BASIC_LOG(stConfig.stUser.puser_info), itype, pname, pdesc, stConfig.remote);
		hdf_set_value(stConfig.cgi->hdf, "err.msg", CGI_REQERR);
		return SLOG_ERROR_LINE;
	}
	FloginInfo *pUserInfo = stConfig.stUser.puser_info;

	Json js;
	std::string strSql;
	Query & qu = *stConfig.qu;
	js["statusCode"] = 200;
	IM_SQL_PARA* ppara = NULL;
	InitParameter(&ppara);
	AddParameter(&ppara, "name", pname, NULL);
	AddParameter(&ppara, "attr_desc", pdesc, NULL);
	AddParameter(&ppara, "mod_user", pUserInfo->szUserName, NULL);
	AddParameter(&ppara, "user_mod_id", pUserInfo->iUserId, NULL);

	strSql = "update mt_attr_type set ";
	JoinParameter_Set(&strSql, qu.GetMysql(), ppara);
	strSql += " where type=";
	strSql += itoa(itype);
	ReleaseParameter(&ppara);
	if(!qu.execute(strSql))
	{
		ERR_LOG("execute sql:%s failed, msg:%s", strSql.c_str(), qu.GetError().c_str());
		return SLOG_ERROR_LINE;
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

	if((int)(js["statusCode"]) == 200)
		DEBUG_LOG("update attr_type:%s(%d) success, sql:%s, response string :%s to remote:%s",
			pname, itype, strSql.c_str(), js.ToString().c_str(), stConfig.remote);
	return 0;
}

static int DeleteAttr()
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

	sprintf(sSqlBuf, "update mt_attr set status=%d where id=%d and user_mod_id=%d",
		stConfig.iDeleteStatus, id, stConfig.stUser.puser_info->iUserId);
	Query & qu = *stConfig.qu;
	if(!qu.execute(sSqlBuf))
	{
		ERR_LOG("execute sql:%s failed, msg:%s", sSqlBuf, qu.GetError().c_str());
		return SLOG_ERROR_LINE;
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

	DEBUG_LOG("delete attr id:%d success, sql:%s, response string :%s to remote:%s",
		id, sSqlBuf, js.ToString().c_str(), stConfig.remote);
	return 0;
}

static int DelAttrType()
{
	if(IsDaemonDenyOp(stConfig))
		return 0;

	const char *ptype = hdf_get_value(stConfig.cgi->hdf, "Query.type", NULL);
	if(NULL == ptype)
	{
		WARN_LOG(CGI_BASIC_LOG_FMT "invalid parameter from:%s", 
			CGI_BASIC_LOG(stConfig.stUser.puser_info), stConfig.remote);
		hdf_set_value(stConfig.cgi->hdf,"err.msg", CGI_REQERR);
		return SLOG_ERROR_LINE;
	}

	Json js;

	int iType = atoi(ptype);
	AttrTypeInfo * pAttrTypeInfo = slog.GetAttrTypeInfo(iType, NULL);
	if(pAttrTypeInfo == NULL)
	{
		REQERR_LOG("not find attr type:%d", iType);
		stConfig.pErrMsg = CGI_ERR_SERVER;
		return SLOG_ERROR_LINE;
	}

	if(pAttrTypeInfo->wAttrCount > 0)
	{
		REQERR_LOG("attr type:%d, has attr count:%d, deny delete", iType, pAttrTypeInfo->wAttrCount);
		stConfig.pErrMsg = "该监控点类型下有监控点，请先删除相关监控点后再试。";
		return SLOG_ERROR_LINE;
	}

	Query & qu = *stConfig.qu;
	char sSqlBuf[256] = {0};

	// 尝试获取子类型
	sprintf(sSqlBuf, "select type from mt_attr_type where status=%d and parent_type=%d",
		RECORD_STATUS_USE, iType);
	qu.get_result(sSqlBuf);
	if(qu.num_rows() > 0)
	{
		REQERR_LOG("attr type:%d, has parent count:%d, deny delete", iType, (int)qu.num_rows());
		stConfig.pErrMsg = CGI_ACCESS_DENY;
		qu.free_result();
		return SLOG_ERROR_LINE;
	}
	qu.free_result();

	if(iType == 1) {
		ERR_LOG(CGI_BASIC_LOG_FMT"not allow to delete root attr type", CGI_BASIC_LOG(stConfig.stUser.puser_info));
		js["statusCode"] = 300;
		js["msgid"] = "denyOpr";
	}
	else {
		Query & qu = *stConfig.qu;
		FloginInfo *pUserInfo = stConfig.stUser.puser_info;
		js["statusCode"] = 200;
		sprintf(sSqlBuf, "update mt_attr_type set status=%d where type=%u", stConfig.iDeleteStatus, atoi(ptype));
		if(!qu.execute(sSqlBuf))
		{
			ERR_LOG(CGI_BASIC_LOG_FMT"execute sql:%s failed, msg:%s", 
				CGI_BASIC_LOG(pUserInfo), sSqlBuf, qu.GetError().c_str());
			return SLOG_ERROR_LINE;
		}
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

	if((int)(js["statusCode"]) == 200)
		DEBUG_LOG("delete attr_type:%s success, sql:%s, response string :%s to remote:%s",
			ptype, sSqlBuf, js.ToString().c_str(), stConfig.remote);
	return 0;
}

static int SaveAttr(bool bIsMod=false)
{
	if(IsDaemonDenyOp(stConfig))
		return 0;

	int32_t iAttrType = hdf_get_int_value(stConfig.cgi->hdf, "Query.dam_attr_type", 0);
	int32_t iDataType = hdf_get_int_value(stConfig.cgi->hdf, "Query.dam_new_attr_data_type", 0);
	int32_t iAttrId = hdf_get_int_value(stConfig.cgi->hdf, "Query.dam_attr_id", 0);

	int iExcepAttrMask = 0;
	const char *pmask = hdf_get_value(stConfig.cgi->hdf, "Query.dam_excep_attr_mask", NULL);
	if(iDataType==EX_REPORT && pmask != NULL && !strcasecmp(pmask, "on"))
		iExcepAttrMask = 1;

	const char *pname = hdf_get_value(stConfig.cgi->hdf, "Query.dam_new_attr_name", NULL);
	const char *pdesc = hdf_get_value(stConfig.cgi->hdf, "Query.dam_new_attr_desc", NULL);
	const char *pcallBkType = hdf_get_value(stConfig.cgi->hdf, "Query.dam_call_back_type", "CloseCurrent");
	const char *pnavTabId = hdf_get_value(stConfig.cgi->hdf, "Query.dam_reload_navTab_id", NULL);

	if(iAttrType == 0 || (bIsMod && iAttrId == 0)
		|| INVALID_ATTR_DATA_TYPE(iDataType) || pname==NULL || pdesc==NULL){ 
		hdf_set_value(stConfig.cgi->hdf,"err.msg", CGI_REQERR);
		REQERR_LOG("bMod:%d invalid param iAttrType:%d iAttrDataType:%d iAttrId:%d pname:%s pdesc:%s",
			bIsMod, iAttrType, iDataType, iAttrId, pname, pdesc);
		return SLOG_ERROR_LINE;
	}

	IM_SQL_PARA* ppara = NULL;
	InitParameter(&ppara);

	AddParameter(&ppara, "attr_type", iAttrType, "DB_CAL");
	AddParameter(&ppara, "data_type", iDataType, "DB_CAL");
	AddParameter(&ppara, "attr_name", pname, NULL);
	AddParameter(&ppara, "attr_desc", pdesc, NULL);
	AddParameter(&ppara, "user_mod_id", stConfig.stUser.puser_info->iUserId, "DB_CAL");
	AddParameter(&ppara, "excep_attr_mask", iExcepAttrMask, "DB_CAL");

	std::string strSql;
	Query & qu = *stConfig.qu;

	if(!bIsMod)
		strSql = "insert into mt_attr";
	else
		strSql = "update mt_attr set";

	if(!bIsMod) {
		AddParameter(&ppara, "user_add", stConfig.stUser.puser, NULL);
		AddParameter(&ppara, "user_add_id", stConfig.stUser.puser_info->iUserId, "DB_CAL");
		JoinParameter_Insert(&strSql, qu.GetMysql(), ppara);
	}
	else {
		JoinParameter_Set(&strSql, qu.GetMysql(), ppara);
		strSql += " where id=";
		strSql += itoa(iAttrId);
	}

	ReleaseParameter(&ppara);
	if(!qu.execute(strSql))
	{
		ERR_LOG("execute sql:%s failed, msg:%s", strSql.c_str(), qu.GetError().c_str());
		return SLOG_ERROR_LINE;
	}

	if(!bIsMod)
		iAttrId = qu.insert_id();

	Json js;
	js["statusCode"] = 200;
	js["callbackType"] = pcallBkType;
	js["navTabId"] = pnavTabId;
	if(!bIsMod)
		js["msgid"] = "addSuccess";
	else
		js["msgid"] = "modSuccess";
	js["attrId"] = iAttrId;

	STRING str;
	string_init(&str);
	if((stConfig.err=string_set(&str, js.ToString().c_str())) != STATUS_OK
		|| (stConfig.err=cgi_output(stConfig.cgi, &str)) != STATUS_OK)
	{
		string_clear(&str);
		return SLOG_ERROR_LINE;
	}
	string_clear(&str);
	DEBUG_LOG("%s mt_attr name:%s(attr id:%u, attr type:%d) success, sql:%s, response string :%s to remote:%s ",
		(bIsMod ? "update" : "insert"), pname, iAttrId, iAttrType, strSql.c_str(), js.ToString().c_str(), stConfig.remote);
	return 0;
}

static int AddAttrType()
{
	if(IsDaemonDenyOp(stConfig))
		return 0;

	int iParentType = hdf_get_int_value(stConfig.cgi->hdf, "Query.parent_attr_type", 0);
	const char *pname = hdf_get_value(stConfig.cgi->hdf, "Query.attr_type_name", NULL);
	const char *pdesc = hdf_get_value(stConfig.cgi->hdf, "Query.attr_type_desc", NULL);
	const char *ppos = hdf_get_value(stConfig.cgi->hdf, "Query.attr_type_pos", NULL);
	if(iParentType == 0 || pname==NULL || pdesc==NULL || ppos==NULL){
		hdf_set_value(stConfig.cgi->hdf,"err.msg", CGI_REQERR);
		REQERR_LOG(CGI_BASIC_LOG_FMT"invalid param iParentType:%d pname:%s pdesc:%s ppos:%s",
			CGI_BASIC_LOG(stConfig.stUser.puser_info), iParentType, pname, pdesc, ppos);
		return SLOG_ERROR_LINE;
	}
	FloginInfo *pUserInfo = stConfig.stUser.puser_info;
	IM_SQL_PARA* ppara = NULL;
	InitParameter(&ppara);
	AddParameter(&ppara, "parent_type", iParentType, "DB_CAL");
	AddParameter(&ppara, "name", pname, NULL);
	AddParameter(&ppara, "type_pos", ppos, NULL);
	AddParameter(&ppara, "attr_desc", pdesc, NULL);
	AddParameter(&ppara, "create_user", stConfig.stUser.puser, NULL);
	AddParameter(&ppara, "mod_user", stConfig.stUser.puser, NULL);
	const char *pcurTime = uitodate(stConfig.dwCurTime);
	AddParameter(&ppara, "create_time", pcurTime, NULL);
	AddParameter(&ppara, "update_time", pcurTime, NULL);
	AddParameter(&ppara, "user_add_id", pUserInfo->iUserId, NULL);
	AddParameter(&ppara, "user_mod_id", pUserInfo->iUserId, NULL);

	std::string strSql;
	Query & qu = *stConfig.qu;

	strSql = "insert into mt_attr_type";
	JoinParameter_Insert(&strSql, qu.GetMysql(), ppara);
	ReleaseParameter(&ppara);
	if(!qu.execute(strSql))
	{
		ERR_LOG("execute sql:%s failed, msg:%s", strSql.c_str(), qu.GetError().c_str());
		return SLOG_ERROR_LINE;
	}

	uint32_t id = qu.insert_id();

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
	DEBUG_LOG("insert mt_attr_type name:%s(type:%u) success, sql:%s, response string :%s to remote:%s ",
		pname, id, strSql.c_str(), js.ToString().c_str(), stConfig.remote);
	return 0;
}

static int DealListAttrType()
{
	Json js;

	// 从根类型开始获取, 根类型的类型编号为 1
	if(GetAttrType(js, 1) < 0)
		return SLOG_ERROR_LINE;
	std::string str(js.ToString());
	DEBUG_LOG("attr type list json:%s", str.c_str());

	stConfig.err = hdf_set_value(stConfig.cgi->hdf, "config.treeinfo", str.c_str()); 
	if(stConfig.err != STATUS_OK)
	{
		ERR_LOG("set attr type info failed, length:%lu", str.size());
		return SLOG_ERROR_LINE;
	}
	return 0;
}

static int DealModAttr()
{
	int id = hdf_get_int_value(stConfig.cgi->hdf, "Query.attr_id", 0);
	const char *pname = hdf_get_value(stConfig.cgi->hdf, "Query.attr_name", NULL);
	const char *pdesc = hdf_get_value(stConfig.cgi->hdf, "Query.attr_desc", NULL);
	int iDataType = hdf_get_int_value(stConfig.cgi->hdf, "Query.attr_data_type", 0);
	const char *ptype_name = hdf_get_value(stConfig.cgi->hdf, "Query.attr_type_name", NULL);
	int iAttrTypeId = hdf_get_int_value(stConfig.cgi->hdf, "Query.attr_type_id", 0);
	int iExcepAttrMask = hdf_get_int_value(stConfig.cgi->hdf, "Query.excep_attr_mask", 0);
	int iAttrInner = hdf_get_int_value(stConfig.cgi->hdf, "Query.attr_inner", 0);
	if(id==0 || iAttrTypeId==0 || iDataType==0 || pname==NULL || ptype_name==NULL)
	{
		WARN_LOG("invalid parameter(id:%d, atype:%d dtype:%d name:%s, type:%s) from:%s",
			id, iAttrTypeId, iDataType, pname, ptype_name, stConfig.remote);
		hdf_set_value(stConfig.cgi->hdf,"err.msg", CGI_REQERR);
		return SLOG_ERROR_LINE;
	}

	hdf_set_value(stConfig.cgi->hdf, "config.action", "save_mod_attr");
	hdf_set_value(stConfig.cgi->hdf, "config.attr_name", pname);
	hdf_set_value(stConfig.cgi->hdf, "config.attr_desc", pdesc);
	hdf_set_value(stConfig.cgi->hdf, "config.attr_type_name", ptype_name);
	hdf_set_int_value(stConfig.cgi->hdf, "config.attr_type_id", iAttrTypeId);
	hdf_set_int_value(stConfig.cgi->hdf, "config.attr_id", id);
	hdf_set_int_value(stConfig.cgi->hdf, "config.attr_data_type", iDataType);
	hdf_set_int_value(stConfig.cgi->hdf, "config.sum_report_m", SUM_REPORT_M);
	hdf_set_int_value(stConfig.cgi->hdf, "config.sum_report_min", SUM_REPORT_MIN);
	hdf_set_int_value(stConfig.cgi->hdf, "config.sum_report_max", SUM_REPORT_MAX);
	hdf_set_int_value(stConfig.cgi->hdf, "config.sum_report_his", SUM_REPORT_TOTAL);
	hdf_set_int_value(stConfig.cgi->hdf, "config.str_report_d", STR_REPORT_D);
	hdf_set_int_value(stConfig.cgi->hdf, "config.ex_report", EX_REPORT);
	hdf_set_int_value(stConfig.cgi->hdf, "config.excep_attr_mask", iExcepAttrMask);
	hdf_set_int_value(stConfig.cgi->hdf, "config.inner", iAttrInner);
	DEBUG_LOG("try update attr for attr(%d:%s) from:%s", id, pname, stConfig.remote);
	return 0;
}

static int DealAddAttr()
{
	hdf_set_value(stConfig.cgi->hdf, "config.action", "save_new_attr");
	hdf_set_int_value(stConfig.cgi->hdf, "config.sum_report_m", SUM_REPORT_M);
	hdf_set_int_value(stConfig.cgi->hdf, "config.sum_report_min", SUM_REPORT_MIN);
	hdf_set_int_value(stConfig.cgi->hdf, "config.str_report_d", STR_REPORT_D);
	hdf_set_int_value(stConfig.cgi->hdf, "config.sum_report_his", SUM_REPORT_TOTAL);
	hdf_set_int_value(stConfig.cgi->hdf, "config.sum_report_max", SUM_REPORT_MAX);
	hdf_set_int_value(stConfig.cgi->hdf, "config.ex_report", EX_REPORT);
	return 0;
}

static int DealAttrSearch()
{
	AttrSearchInfo stInfo;
	stInfo.pkey = hdf_get_value(stConfig.cgi->hdf, "Query.da_keyword", NULL);
	stInfo.iAttrDataType = hdf_get_int_value(stConfig.cgi->hdf, "Query.da_attr_data_type", 0);
	stInfo.iAttrType = hdf_get_int_value(stConfig.cgi->hdf, "Query.dam_attr_type", 0);
	stInfo.pattrType = hdf_get_value(stConfig.cgi->hdf, "Query.dam_attr_type_name", NULL);
	const char *pmask = hdf_get_value(stConfig.cgi->hdf, "Query.da_excp_mask", NULL);
	stInfo.iExcpAttrMasked = 0;
	if(pmask != NULL && !strcasecmp(pmask, "on"))
		stInfo.iExcpAttrMasked = 1;

	DEBUG_LOG("search data type:%d attr type:%d(%s) key:%s",
		stInfo.iAttrDataType, stInfo.iAttrType, stInfo.pattrType, stInfo.pkey);

	int iRecords = GetAttrTotalRecords(&stInfo);
	if(iRecords < 0)
	{
		ERR_LOG("get attr record count failed !");
		return SLOG_ERROR_LINE;
	}
	SetRecordsPageInfo(stConfig.cgi, iRecords);

	Json js;

	// 从根类型开始获取, 根类型的类型编号为 1
	if(GetAttrType(js, 1) < 0)
		return SLOG_ERROR_LINE;

	std::string str(js.ToString());
	DEBUG_LOG("attr type list json:%s", str.c_str());
	stConfig.err = hdf_set_value(stConfig.cgi->hdf, "config.treeinfo", str.c_str()); 
	if(stConfig.err != STATUS_OK)
	{
		ERR_LOG("set attr type info failed, length:%lu", str.size());
		return SLOG_ERROR_LINE;
	}

	hdf_set_int_value(stConfig.cgi->hdf, "config.sum_report_m", SUM_REPORT_M);
	hdf_set_int_value(stConfig.cgi->hdf, "config.sum_report_min", SUM_REPORT_MIN);
	hdf_set_int_value(stConfig.cgi->hdf, "config.str_report_d", STR_REPORT_D);
	hdf_set_int_value(stConfig.cgi->hdf, "config.sum_report_his", SUM_REPORT_TOTAL);
	hdf_set_int_value(stConfig.cgi->hdf, "config.sum_report_max", SUM_REPORT_MAX);
	hdf_set_int_value(stConfig.cgi->hdf, "config.ex_report", EX_REPORT);

	Json js_attr;
	if(GetAttrList(js_attr, &stInfo) < 0)
		return SLOG_ERROR_LINE;
	std::string str_attr(js_attr.ToString());
	DEBUG_LOG("attr list json:%s", str_attr.c_str());
	stConfig.err = hdf_set_value(stConfig.cgi->hdf, "config.attrinfo", str_attr.c_str()); 
	if(stConfig.err != STATUS_OK)
	{
		ERR_LOG("set attr type info failed, length:%lu", str_attr.size());
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

		// 属性类型操作
		if(!strcmp(pAction, "list") || !strcmp(pAction, "lookUpAttrType") || !strcmp(pAction, "lookUpAttrType4Add"))
			iRet = DealListAttrType();
		else if(!strcmp(pAction, "add"))
			iRet = AddAttrType();
		else if(!strcmp(pAction, "delete"))
			iRet = DelAttrType();
		else if(!strcmp(pAction, "mod"))
			iRet = UpdateAttrType();

		// 属性操作
		else if(!strcmp(pAction, "attr_list"))
			iRet = DealAttrSearch();
		else if(!strcmp(pAction, "add_attr"))
			iRet = DealAddAttr();
		else if(!strcmp(pAction, "mod_attr"))
			iRet = DealModAttr();
		else if(!strcmp(pAction, "save_new_attr"))
			iRet = SaveAttr();
		else if(!strcmp(pAction, "save_mod_attr"))
			iRet = SaveAttr(true);
		else if(!strcmp(pAction, "attr_delete"))
			iRet = DeleteAttr();
		else if(!strcmp(pAction, "search"))
			iRet = DealAttrSearch();

		else {
			iRet = -1;
			REQERR_LOG("unknow action:%s from:%s", pAction, stConfig.remote);
			hdf_set_value(stConfig.cgi->hdf,"err.msg", CGI_REQERR);
		}

		// -------------------------------------------

		if(iRet < 0)
		{
			show_errpage(NULL, NULL, stConfig);
			continue;
		}

		const char *pcsTemplate = NULL;

		if(!strcmp(pAction, "attr_list") || !strcmp(pAction, "search"))
			pcsTemplate = "dmt_attr.html";
		else if(!strcmp(pAction, "add_attr") || !strcmp(pAction, "mod_attr"))
			pcsTemplate = "dmt_attr_manage.html";
		else if(!strcmp(pAction, "list"))
			pcsTemplate = "dmt_attr_type.html";
		else if(!strcmp(pAction, "lookUpAttrType") || !strcmp(pAction, "lookUpAttrType4Add"))
			pcsTemplate = "dmt_look_attr_type.html";

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
		DealCgiFailedExit(stConfig.cgi, stConfig.err);

	stConfig.dwEnd = time(NULL);
	INFO_LOG("fcgi - %s stop at:%u run:%u pid:%u errmsg:%s",
		stConfig.pszCgiName, stConfig.dwEnd, stConfig.dwEnd - stConfig.dwStart, stConfig.pid, strerror(errno));
	return 0;
}
