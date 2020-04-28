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

   模块 slog_monitor_server 功能:
        用于接收客户端上报上来的监控点数据，生成监控点数据当天的 memcache 缓存
        并将数据写入 MySQL 数据库中		

****/

#include <supper_log.h>
#include <errno.h>
#include <sv_struct.h>
#include <assert.h>

#include <Base64.h>
#include "udp_sock.h"
#include "aes.h"
#include "Memcache.h"

#ifndef RECORD_STATUS_USE
#define RECORD_STATUS_USE 0 // 记录使用中 
#define RECORD_STATUS_DELETE 1 // 记录更新后，程序自动删除 
#endif

int SrvForAttrCmp(const void *a, const void *b);

extern CONFIG stConfig;

// 收包进程的染色配置
#define TEST_KEY_TYPE_MACHINE 102
#define TEST_KEY_TYPE_ATTR 103
#define TEST_KEY_TYPE_CMD 104
int CheckRequestLog_test(int iTestCfgCount, SLogTestKey *pstTestCfg, const void *pdata)
{
	if(iTestCfgCount <= 0)
		return 0;

	CUdpSock *preq = (CUdpSock*)pdata;
	bool bHasCfg = false;
	for(int i=0; i < iTestCfgCount; i++)
	{
		if(pstTestCfg[i].bKeyType == TEST_KEY_TYPE_MACHINE
			|| pstTestCfg[i].bKeyType == TEST_KEY_TYPE_ATTR
			|| pstTestCfg[i].bKeyType == TEST_KEY_TYPE_CMD)
			bHasCfg = true;

		if(pstTestCfg[i].bKeyType == TEST_KEY_TYPE_MACHINE 
			&& (preq->m_pcltMachine==NULL
				|| atoi(pstTestCfg[i].szKeyValue) != preq->m_pcltMachine->id))
			return 0;

		if(pstTestCfg[i].bKeyType == TEST_KEY_TYPE_ATTR
			&& atoi(pstTestCfg[i].szKeyValue) != preq->m_iDealAttrId)
			return 0;

		if(pstTestCfg[i].bKeyType == TEST_KEY_TYPE_CMD
			&& preq->m_dwReqCmd != strtoul(pstTestCfg[i].szKeyValue, NULL, 10))
			return 0;
	}
	return bHasCfg;
}

int CUdpSock::ChangeAttrSaveType(const char *ptable, Query &qu)
{
	std::string strSql;
	strSql = "select distinct attr_id,machine_id from ";
	strSql += ptable;
	strSql += " where machine_id!=0";

	if(!qu.get_result(strSql))
	{
		ERR_LOG("exec sql:%s failed!", strSql.c_str());
		return SLOG_ERROR_LINE;
	}
	MyQuery myqu(m_qu, db);
	Query & qu_day = myqu.GetQuery();

	uint32_t arrydwValue[COUNT_MIN_PER_DAY];
	uint32_t dwMax=0, dwMin=0, dwTotal=0, dwLastIp=0;
	int32_t iReportCount = 0, iMaxDataLen = 0;
	for(int i=0; i < qu.num_rows() && qu.fetch_row() != NULL; i++)
	{
		int32_t iAttrId = qu.getval("attr_id");
		int32_t iMachineId = qu.getval("machine_id");

		GetMonitorAttrMemcache(qu_day, iAttrId, iMachineId, ptable, 
			dwMin, dwMax, dwTotal, iReportCount, (uint32_t*)arrydwValue, &dwLastIp);
		if(iReportCount <= 0)
		{
			WARN_LOG("attr no report - attr:%d machine:%d , table:%s", iAttrId, iMachineId, ptable);
			continue;
		}

		if(dwMax <= 0)
			continue;

		DEBUG_LOG("change attr:%d machine:%d save type , count report:%d, min:%u, max:%u, total:%u",
			iAttrId, iMachineId, iReportCount, dwMin, dwMax, dwTotal);

		/*  binary 方式存储 */
		snprintf(stConfig.szBinSql, sizeof(stConfig.szBinSql),
			"insert into %s_day set attr_id=%d,machine_id=%d,min=%u,max=%u,total=%u,last_ip=%u,value=",
			ptable, iAttrId, iMachineId, dwMin, dwMax, dwTotal, dwLastIp);
	
		char *pbuf = (char*)stConfig.szBinSql+strlen(stConfig.szBinSql);
		int32_t iBinaryDataLen = qu_day.SetBinaryData(pbuf, (const char*)arrydwValue, sizeof(arrydwValue));
		if(iBinaryDataLen < 0)
		{
			qu.free_result();
			return SLOG_ERROR_LINE;
		}
		pbuf += iBinaryDataLen;
		if(iBinaryDataLen > iMaxDataLen)
			iMaxDataLen = iBinaryDataLen;
		if(qu_day.ExecuteBinary(stConfig.szBinSql, (int32_t)(pbuf-stConfig.szBinSql)) < 0)
		{
			qu.free_result();
			return SLOG_ERROR_LINE;
		}

		INFO_LOG("change attr save type to day ok - attr_id:%d,machine_id:%d,report count:%d",
			iAttrId, iMachineId, iReportCount);
	}
	INFO_LOG("get attr day type from:%s record count:%d, max length:%d", ptable, qu.num_rows(), iMaxDataLen);
	qu.free_result();

	// 字符型监控点数据转存到 day table
	snprintf(stConfig.szBinSql, sizeof(stConfig.szBinSql),
		" insert into %s_day (`attr_id`, `machine_id`, `total`, `value`) "
		" select attr_id,machine_id,value,str_val from %s where value=0",
		ptable, ptable);
	if(!qu.execute(stConfig.szBinSql)) 
	{
		ERR_LOG("execute sql:%s failed !", stConfig.szBinSql);
		return SLOG_ERROR_LINE;
	}
	return 0;
}

int CUdpSock::TryChangeAttrSaveType()
{
	if(db == NULL)
	{
		ERR_LOG("check failed, db is NULL");
		return SLOG_ERROR_LINE;
	}

	std::string strSql;
	strSql = "select id,table_name from table_info order by id desc";

	MyQuery myqu(m_qu, db);
	Query & qu = myqu.GetQuery();
	if(!qu.get_result(strSql))
	{
		ERR_LOG("exec sql:%s failed!", strSql.c_str());
		return SLOG_ERROR_LINE;
	}

	if(qu.num_rows() > 0)
	{
		// skip last
		qu.fetch_row();
		DEBUG_LOG("get table info count:%d", qu.num_rows());
	}

	MyQuery myqu_2(m_qu, db);
	Query & qu_day = myqu_2.GetQuery();
	for(int i=1; i < qu.num_rows() && qu.fetch_row() != NULL; i++)
	{
		const char *ptb = qu.getstr("table_name");
		if(ptb != NULL)
		{
			INFO_LOG("try change save - get attr table:%s id:%d", ptb, qu.getval("id"));
			strSql = "select id from table_info_day where table_name=\'";
			strSql += ptb;
			strSql += "_day\';";
			if(!qu_day.get_result(strSql))
			{
				ERR_LOG("exec sql:%s failed!", strSql.c_str());
				break;
			}
			if(qu_day.num_rows() > 0)
			{
				DEBUG_LOG("attr table :%s already change table day:%d", ptb, qu_day.getval(0));
				break;
			}
			qu_day.free_result();

			if(CreateAttrDayTable(ptb) < 0)
				break;
			if(ChangeAttrSaveType(ptb, qu_day) < 0)
				break;
		}
		else
		{
			ERR_LOG("get table_name failed (is null), sql:%s", strSql.c_str());
			break;
		}
	}

	qu.free_result();
	qu_day.free_result();
	return 0;
}

int CUdpSock::CreateAttrDayTable(const char *pszTbName)
{
	MyQuery myqu(m_qu, db);
	Query & qu = myqu.GetQuery();
	snprintf(stConfig.szBinSql, sizeof(stConfig.szBinSql), "drop table IF EXISTS %s_day", pszTbName);
	qu.execute(stConfig.szBinSql);

	snprintf(stConfig.szBinSql, sizeof(stConfig.szBinSql),
		"create table `%s_day` ( `attr_id` int(11) DEFAULT '0', `machine_id` int(11) DEFAULT '0',"
		"`value` BLOB, `max` int(12) unsigned , `min` int(12) unsigned , `total` int(12) unsigned,"
		"`last_ip` int(12) unsigned, PRIMARY KEY (`attr_id`, `machine_id`) ) ENGINE=InnoDB", pszTbName);
	if(!qu.execute(stConfig.szBinSql))
	{
		ERR_LOG("execute sql:%s failed !", stConfig.szBinSql);
		return SLOG_ERROR_LINE;
	}
	INFO_LOG("create attr day table :%s_day success ", pszTbName);

	snprintf(stConfig.szBinSql, sizeof(stConfig.szBinSql), 
		"insert into table_info_day set table_name=\'%s_day\'", pszTbName);
	if(!qu.execute(stConfig.szBinSql))
	{
		ERR_LOG("exe sql:%s failed !", stConfig.szBinSql);
		return SLOG_ERROR_LINE;
	}

	strcpy(stConfig.szBinSql, "select count(*) from table_info_day");
	if(qu.get_result(stConfig.szBinSql) == NULL || qu.num_rows() <= 0)
	{
		qu.free_result();
		return 0;
	}

	qu.fetch_row();
	int iCount = qu.getval(0);
	if(iCount > m_iKeepDay)
	{
		qu.free_result();
		strcpy(stConfig.szBinSql, "select id,table_name from table_info_day order by table_name asc limit 1");
		if(qu.get_result(stConfig.szBinSql) == NULL || qu.num_rows() <= 0)
		{
			WARN_LOG("try delete attr table failed, table count:%d", iCount);
			qu.free_result();
			return SLOG_ERROR_LINE;
		}
		else
		{
			qu.fetch_row();
			int id = qu.getval("id");
			std::string strTb(qu.getstr("table_name"));
			qu.free_result();

			// 删除表
			snprintf(stConfig.szBinSql, sizeof(stConfig.szBinSql), "drop table %s", strTb.c_str());
			if(!qu.execute(stConfig.szBinSql))
			{
				ERR_LOG("try drop attr day table :%s failed!", strTb.c_str());
				return SLOG_ERROR_LINE;
			}

			// 删除记录
			snprintf(stConfig.szBinSql, sizeof(stConfig.szBinSql), "delete from table_info_day where id=%d", id);
			qu.execute(stConfig.szBinSql);
			INFO_LOG("delete table:%s success", strTb.c_str());
		}
	}
	else
	{
		qu.free_result();
		INFO_LOG("attr day table count:%d", iCount);
	}
	return 0;
}

int CUdpSock::CreateAttrTable(const char *pszTbName)
{
	if(pszTbName == NULL)
		pszTbName = GetTodayTableName();
	strncpy(m_szLastTableName, pszTbName, sizeof(m_szLastTableName)-1);
	MyQuery myqu(m_qu, db);
	Query & qu = myqu.GetQuery();
	snprintf(stConfig.szBinSql, sizeof(stConfig.szBinSql), "drop table IF EXISTS %s", pszTbName);
	qu.execute(stConfig.szBinSql);

	snprintf(stConfig.szBinSql, sizeof(stConfig.szBinSql), 
		"create table `%s` (`id` int(11) auto_increment, `attr_id` int(11) default 0,"
		"`machine_id` int(11) default 0, `value` int(11) default 0, `client_ip` char(20) NOT NULL,"
		"`str_val` BLOB, `report_time` timestamp default CURRENT_TIMESTAMP, PRIMARY KEY  (`id`), "
		" KEY (`attr_id`, `machine_id`) )", pszTbName);

	if(!qu.execute(stConfig.szBinSql))
	{
		ERR_LOG("execute sql:%s failed !", stConfig.szBinSql);
		return SLOG_ERROR_LINE;
	}
	INFO_LOG("create table :%s success ", pszTbName);

	snprintf(stConfig.szBinSql, sizeof(stConfig.szBinSql), "insert into table_info set table_name=\'%s\'", pszTbName);
	if(!qu.execute(stConfig.szBinSql))
	{
		ERR_LOG("exe sql:%s failed !", stConfig.szBinSql);
		return SLOG_ERROR_LINE;
	}

	// 前一天的数据转为 day table 
	TryChangeAttrSaveType();

	strcpy(stConfig.szBinSql, "select count(*) from table_info");
	if(qu.get_result(stConfig.szBinSql) == NULL || qu.num_rows() <= 0)
	{
		qu.free_result();
		return 0;
	}

	qu.fetch_row();
	int iCount = qu.getval(0);
	if(iCount > m_iKeep)
	{
		qu.free_result();
		strcpy(stConfig.szBinSql, "select id,table_name from table_info order by id asc limit 1");
		if(qu.get_result(stConfig.szBinSql) == NULL || qu.num_rows() <= 0)
		{
			WARN_LOG("try delete attr table failed, table count:%d", iCount);
			qu.free_result();
			return SLOG_ERROR_LINE;
		}
		else
		{
			qu.fetch_row();
			int id = qu.getval("id");
			std::string strTb(qu.getstr("table_name"));
			qu.free_result();

			// 删除表
			snprintf(stConfig.szBinSql, sizeof(stConfig.szBinSql), "drop table %s", strTb.c_str());
			if(!qu.execute(stConfig.szBinSql))
			{
				ERR_LOG("try drop table :%s failed!", strTb.c_str());
				return SLOG_ERROR_LINE;
			}

			// 删除记录
			snprintf(stConfig.szBinSql, sizeof(stConfig.szBinSql), "delete from table_info where id=%d", id);
			qu.execute(stConfig.szBinSql);
			INFO_LOG("delete table:%s success", strTb.c_str());
		}
	}
	else
	{
		qu.free_result();
		INFO_LOG("attr table count:%d", iCount);
	}
	return 0;
}

// 获取前一天的表名
const char * CUdpSock::GetBeforeDayTableName()
{
	static char szTableName[32];
	time_t tmBefore = slog.m_stNow.tv_sec  - 24*60*60;
	struct tm curr = *localtime(&tmBefore);
	if(curr.tm_year > 50)
		snprintf(szTableName, sizeof(szTableName), "attr_%04d%02d%02d", 
			curr.tm_year+1900, curr.tm_mon+1, curr.tm_mday);
	else
		snprintf(szTableName, sizeof(szTableName), "attr_%04d%02d%02d", 
			curr.tm_year+2000, curr.tm_mon+1, curr.tm_mday);
	return szTableName;
}

// 获取当前时间的表名
const char * CUdpSock::GetTodayTableName()
{
	static char szTableName[32];
	struct tm curr = *localtime(&slog.m_stNow.tv_sec);
	if(curr.tm_year > 50)
		snprintf(szTableName, sizeof(szTableName), "attr_%04d%02d%02d", 
			curr.tm_year+1900, curr.tm_mon+1, curr.tm_mday);
	else
		snprintf(szTableName, sizeof(szTableName), "attr_%04d%02d%02d", 
			curr.tm_year+2000, curr.tm_mon+1, curr.tm_mday);
	return szTableName;
}

int CUdpSock::CheckTableName()
{
	std::string strSql;

	if(db == NULL)
	{
		ERR_LOG("check failed, db is NULL");
		return SLOG_ERROR_LINE;
	}

	const char *pTodayTable = GetTodayTableName();

	// 首次启动，获取最近一个表的表名
	if(m_szLastTableName[0] == '\0')
	{
		strSql = "select id,table_name from table_info order by id desc limit 1";
		MyQuery myqu(m_qu, db);
		Query & qu = myqu.GetQuery();
		if(!qu.get_result(strSql))
		{
			ERR_LOG("exec sql:%s failed!", strSql.c_str());
			return SLOG_ERROR_LINE;
		}

		if(qu.num_rows() > 0)
		{
			qu.fetch_row();
			const char *ptb = qu.getstr("table_name");
			if(ptb != NULL)
			{
				strncpy(m_szLastTableName, ptb, sizeof(m_szLastTableName)-1);
				INFO_LOG("get last attr table:%s id:%d", m_szLastTableName, qu.getval("id"));
				if(strcmp(m_szLastTableName, pTodayTable))
				{
					INFO_LOG("get last table name:%s not eq :%s", ptb, pTodayTable);
					if(CreateAttrTable(pTodayTable) < 0)
					{
						qu.free_result();
						return SLOG_ERROR_LINE;
					}
				}
				qu.free_result();
				return 0;
			}
			else
				ERR_LOG("get table_name failed (is null), sql:%s", strSql.c_str());
		}
		else if(CreateAttrTable(pTodayTable) < 0)
		{
			qu.free_result();
			return SLOG_ERROR_LINE;
		}
		qu.free_result();
		return 0;
	}
	else if(!strcmp(m_szLastTableName, pTodayTable))
		return 0;

	if(CreateAttrTable(pTodayTable) < 0)
		return SLOG_ERROR_LINE;
	return 0;
}

CUdpSock::CUdpSock(ISocketHandler&h): UdpSocket(h), CBasicPacket()
{
	Attach(CreateSocket(PF_INET, SOCK_DGRAM, "udp"));
	memset(m_szLastTableName, 0, sizeof(m_szLastTableName));
	db = NULL;
	m_qu = NULL;
	db_attr_info = NULL;
	m_qu_attr_info = NULL;
	m_iKeepDay = 30;
	m_iKeep = 1;
	m_pcltMachine = NULL;
}

CUdpSock::~CUdpSock()
{
}

bool IsMachineMatch(uint32_t dwCheckMachineIp, MachineInfo* pMachine)
{   
    if(pMachine->ip1 == dwCheckMachineIp || pMachine->ip2 == dwCheckMachineIp  
        || pMachine->ip3 == dwCheckMachineIp || pMachine->ip4 == dwCheckMachineIp)
    {   
        return true;
    }   
    return false;
} 

MachineInfo * CUdpSock::GetReportMachine(uint32_t ip)
{
	int idx = stConfig.psysConfig->iMachineListIndexStart;
	for(int i=0; i < stConfig.psysConfig->wMachineCount; i++)
	{
		MachineInfo *pMachine = slog.GetMachineInfo(idx);
		if(IsMachineMatch(ip, pMachine))
		{
			return pMachine;
		}
		idx = pMachine->iNextIndex;
	}
	return NULL;
}

int CUdpSock::GetDayOfMin(const char *ptime)
{
	static char s_sLocalBuf[64];

	char *ptime_str = NULL; 
	if(NULL == ptime) {
		ptime_str = uitodate(slog.m_stNow.tv_sec);
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

int CUdpSock::GetMonitorAttrVal(Query &qu, int attr_id, int machine_id, comm::MonitorMemcache &memInfo)
{
	memInfo.Clear();
	sprintf(stConfig.szBinSql, "select value from %s where attr_id=%d and machine_id=%d",
			m_szLastTableName, attr_id, machine_id);

	MYSQL_RES *res = qu.get_result(stConfig.szBinSql);
	if(qu.num_rows() > 0) 
	{ 
		qu.fetch_row();
		unsigned long* lengths = mysql_fetch_lengths(res);
		unsigned long ulValLen = lengths[0];
		const char *pval = qu.getstr("value");
		if(!memInfo.ParseFromArray(pval, ulValLen))
		{
			ERR_LOG("ParseFromArray failed-%p-%lu", pval, ulValLen);
			qu.free_result();
			return SLOG_ERROR_LINE;
		}
		DEBUG_LOG("get attrinfo from db: val count:%d, max:%d",
			memInfo.machine_attr_day_val().attr_val_size(), memInfo.machine_attr_day_val().max_idx());
	}
	qu.free_result();
	return 0;
}

void CUdpSock::GetMonitorAttrMemcache(
	Query &qu, int attr_id, int machine_id, const char *ptable, 
	uint32_t & uiMin, uint32_t & uiMax, uint32_t & uiTotal, int32_t & iReportCount, 
	uint32_t *puiValArry, uint32_t *puiLastIp)
{
	sprintf(stConfig.szBinSql, 
		"select value,report_time,client_ip from %s where attr_id=%d and machine_id=%d",
		ptable, attr_id, machine_id);
	qu.get_result(stConfig.szBinSql);
	const char *prepTime = NULL;
	int iDayOfMinTmp = 0;
	uiMin = uiMax = uiTotal = 0;
	iReportCount = 0;
	for(int i=0; qu.fetch_row() != NULL; i++)
	{
		if(0 == i)
		{
			uiMax = qu.getuval("value");;
			uiMin = uiMax;
			uiTotal = 0;
			memset(puiValArry, 0, sizeof(uint32_t)*COUNT_MIN_PER_DAY);
		}
		iReportCount++;

		prepTime = qu.getstr("report_time");
		iDayOfMinTmp = GetDayOfMin(prepTime);
		puiValArry[iDayOfMinTmp] += qu.getuval("value");

		if(uiMax < puiValArry[iDayOfMinTmp])
			uiMax = puiValArry[iDayOfMinTmp];
		if(uiMin > puiValArry[iDayOfMinTmp])
			uiMin = puiValArry[iDayOfMinTmp];
		uiTotal += puiValArry[iDayOfMinTmp];

		if(i+1 >= qu.num_rows() && puiLastIp != NULL) {
			*puiLastIp = inet_addr(qu.getstr("client_ip"));
			break;
		}
	}
	qu.free_result();
} 

void CUdpSock::GetMonitorAttrMemcache(
	Query &qu, int attr_id, int machine_id, const char *ptable, comm::MonitorMemcache & memInfo)
{
	sprintf(stConfig.szBinSql, "select value,report_time from %s where attr_id=%d and machine_id=%d",
		ptable, attr_id, machine_id);
	qu.get_result(stConfig.szBinSql);
	const char *prepTime = NULL;
	int iDayOfMinTmp = 0;
	comm::AttrVal *pAttrVal = NULL;
	uint32_t iMaxVal = 0;
	for(int i=0; i < qu.num_rows() && qu.fetch_row() != NULL; i++)
	{
		if(qu.getval("value") <= 0)
			continue;

		prepTime = qu.getstr("report_time");
		iDayOfMinTmp = GetDayOfMin(prepTime);
		pAttrVal = memInfo.mutable_machine_attr_day_val()->add_attr_val();
		if(iDayOfMinTmp > (int)memInfo.mutable_machine_attr_day_val()->max_idx())
			memInfo.mutable_machine_attr_day_val()->set_max_idx(iDayOfMinTmp);
		pAttrVal->set_idx(iDayOfMinTmp);
		pAttrVal->set_val(qu.getval("value"));
		if(pAttrVal->val() > iMaxVal)
			iMaxVal = pAttrVal->val();
	}

	int iTmpIdx = memInfo.machine_attr_day_val().attr_val_size() - 1;
	if(iTmpIdx >= 0 && memInfo.machine_attr_day_val().attr_val(iTmpIdx).val() < iMaxVal)
		memInfo.mutable_machine_attr_day_val()->mutable_attr_val(iTmpIdx)->set_val(iMaxVal);
	qu.free_result();
}

TWarnAttrReportInfo * CUdpSock::AddReportToWarnAttrShm(
	uint32_t dwAttrId, int32_t iMachineId, uint32_t dwVal, int32_t iMinIdx, int iDataType)
{
	uint32_t dwIsFind = 0;
	TWarnAttrReportInfo *pAttrShm = NULL;
	pAttrShm = slog.GetWarnAttrInfo((int32_t)dwAttrId, iMachineId, &dwIsFind);
	if(pAttrShm == NULL)
	{
		ERR_LOG("add attr report val failed, attrid:%d, machineid:%d, val:%u",
			(int32_t)dwAttrId, iMachineId, dwVal);
		return NULL;
	}

	if(!dwIsFind)
	{
		pAttrShm->iAttrId = (int32_t)dwAttrId;
		pAttrShm->iMachineId = iMachineId;
		pAttrShm->dwCurVal = dwVal;
		pAttrShm->bAttrDataType = (uint8_t)iDataType;
		INFO_LOG("init add warn attr report info, attrid:%d, machineid:%d, val:%u, minIdx:%d, data type:%d",
			(int32_t)dwAttrId, iMachineId, dwVal, iMinIdx, iDataType);
	}
	else
	{
		DEBUG_LOG("before add warn attr val, attrid:%d, machineid:%d, PreLastVal:%u, LastVal:%u"
			", val:%u|%u, minIdx:%d|%d",
			pAttrShm->iAttrId, pAttrShm->iMachineId, pAttrShm->dwPreLastVal, pAttrShm->dwLastVal,
			pAttrShm->dwCurVal, dwVal, pAttrShm->iMinIdx, iMinIdx);
		if(pAttrShm->bAttrDataType != iDataType)
		{
			INFO_LOG("attr info - attr:%u data type changed from:%d, to:%d", 
				dwAttrId, pAttrShm->bAttrDataType, iDataType);
			pAttrShm->bAttrDataType = (uint8_t)iDataType;
		}

		// 跨天无上报
		if(slog.m_stNow.tv_sec >= pAttrShm->dwLastReportTime+1440*60)
		{
			if(iDataType == SUM_REPORT_TOTAL) {
				// 历史累计数据类型
				pAttrShm->dwPreLastVal = pAttrShm->dwCurVal;
				pAttrShm->dwLastVal = pAttrShm->dwCurVal;
				pAttrShm->dwCurVal += dwVal;
			}
			else {
				pAttrShm->dwPreLastVal = 0;
				pAttrShm->dwLastVal = 0;
				pAttrShm->dwCurVal = dwVal;
			}
			WARN_LOG("attrid:%d, machineid:%d, data type:%d, report value may lost", 
				pAttrShm->iAttrId, pAttrShm->iMachineId, iDataType);
		}
		else if(pAttrShm->iMinIdx == iMinIdx)
		{
			if(SUM_REPORT_MIN == iDataType) {
				// 取最小值
				if(dwVal < pAttrShm->dwCurVal)
					pAttrShm->dwCurVal = dwVal;
			}
			else if(SUM_REPORT_MAX == iDataType) {
				// 取最大
				if(dwVal > pAttrShm->dwCurVal)
					pAttrShm->dwCurVal = dwVal;
			}
			else
				pAttrShm->dwCurVal += dwVal;
		}
		else if(((pAttrShm->iMinIdx+1)%1440) == iMinIdx)
		{
			pAttrShm->dwPreLastVal = pAttrShm->dwLastVal;
			pAttrShm->dwLastVal = pAttrShm->dwCurVal;
			if(iDataType == SUM_REPORT_TOTAL)
				pAttrShm->dwCurVal += dwVal;
			else
				pAttrShm->dwCurVal = dwVal;
		}
		else if(((pAttrShm->iMinIdx+2)%1440) == iMinIdx)
		{
			pAttrShm->dwPreLastVal = pAttrShm->dwCurVal;
			if(iDataType == SUM_REPORT_TOTAL) {
				pAttrShm->dwLastVal = pAttrShm->dwCurVal;
				pAttrShm->dwCurVal += dwVal;
			}
			else {
				pAttrShm->dwLastVal = 0;
				pAttrShm->dwCurVal = dwVal;
			}
		}
		else
		{
			// 中间至少有2分钟无上报数据或者上报丢失
			if(iDataType == SUM_REPORT_TOTAL) {
				// 历史累计数据类型
				pAttrShm->dwPreLastVal = pAttrShm->dwCurVal;
				pAttrShm->dwLastVal = pAttrShm->dwCurVal;
				pAttrShm->dwCurVal += dwVal;
			}
			else {
				pAttrShm->dwPreLastVal = 0;
				pAttrShm->dwLastVal = 0;
				pAttrShm->dwCurVal = dwVal;
			}
			INFO_LOG("attrid:%d, machineid:%d, report value may lost", pAttrShm->iAttrId, pAttrShm->iMachineId);
		}

		DEBUG_LOG("after add warn attr val, attrid:%d, machineid:%d, PreLastVal:%u, LastVal:%u, val:%u, minIdx:%d",
			pAttrShm->iAttrId, pAttrShm->iMachineId, pAttrShm->dwPreLastVal, pAttrShm->dwLastVal,
			pAttrShm->dwCurVal, iMinIdx);
	}

	pAttrShm->iMinIdx = iMinIdx;
	pAttrShm->dwLastReportTime = m_pcltMachine->dwLastReportAttrTime;
	return pAttrShm;
}

//  机器有上报属性 memcache 缓存有效期
#define MEMCACHE_MACHINE_ATTR_EXPIRE_TIME (24*60*60+300)

//  机器有上报属性/上报值 memcache 缓存有效期
#define MEMCACHE_MACHINE_ATTR_VAL_EXPIRE_TIME (24*60*60+300)
void CUdpSock::WriteAttrDataToMemcache()
{
	static int s_ReportAttrs[MAX_REPORT_ATTR_COUNT_PER_MACHINE*10];
	static int s_LastSaveAttrToMemcacheIdx = -1;

	if(!stConfig.iEnableMemcache)
		return;

	// 实现累计一分钟数据后写入 memcache
	int idx = GetDayOfMin();
	if(idx == s_LastSaveAttrToMemcacheIdx)
		return;

	s_LastSaveAttrToMemcacheIdx = idx;
	uint32_t dwLastVal  = 0;
	uint32_t dwTimeCur = time(NULL);

	if(m_szLastTableName[0] == '\0' && CheckTableName() < 0)
	{
		ERR_LOG("CheckTableName failed!");
		return;
	}

	SharedHashTable & hash = slog.GetWarnAttrHash();
	MachineInfo *pMachInfo = NULL;
	int *pattr = NULL;
	int iTmpLen = 0, iCount = 0, iRet = 0;
	SLogServer* psrv = NULL;
	TWarnAttrReportInfo *pReportShm = (TWarnAttrReportInfo*)GetFirstNode(&hash);
	bool bReverse = false;
	_HashTableHead *pTableHead = (_HashTableHead*)hash.pHash;
	if(pReportShm == NULL && pTableHead->dwNodeUseCount > 0) {
		bReverse = true;
		pReportShm = (TWarnAttrReportInfo*)GetFirstNodeRevers(&hash);
		ERR_LOG("man by bug - has node:%u, bug get first null", pTableHead->dwNodeUseCount);
	}

	MyQuery myqu(m_qu, db);
	Query & qu = myqu.GetQuery();
	for(; pReportShm != NULL; 
		bReverse ? (pReportShm = (TWarnAttrReportInfo*)GetNextNodeRevers(&hash)) :
			(pReportShm = (TWarnAttrReportInfo*)GetNextNode(&hash)))
	{
		// 找到归属于当前时间的上报数值
		dwLastVal = pReportShm->dwLastVal;
		if(dwTimeCur >= pReportShm->dwLastReportTime+1440*60)
			dwLastVal = 0;
		else if(idx != pReportShm->iMinIdx)
		{
			if(((pReportShm->iMinIdx+1)%1440) == idx)
				dwLastVal = pReportShm->dwCurVal;
			else
				dwLastVal = 0;
		}
		if(dwLastVal <= 0
			&& pReportShm->bAttrDataType != STR_REPORT_D && pReportShm->bAttrDataType != STR_REPORT_D_IP)
			continue;

		pMachInfo = slog.GetMachineInfo(pReportShm->iMachineId, NULL);
		if(NULL == pMachInfo) {
			WARN_LOG("GetMachineInfo failed, machine id:%d", pReportShm->iMachineId);
			continue;
		}

		// 看下是否跨天了, 如已跨天则清掉 vmem
		if(m_szLastTableName[0] != '\0' && strcmp(m_szLastTableName, pMachInfo->szAttrReportDay))
		{
			INFO_LOG("machine:%d, change day, info vmemidx:%d, day:%s-%s", 
				pReportShm->iMachineId, pMachInfo->iReportAttrVmemIdx, pMachInfo->szAttrReportDay, m_szLastTableName);
			if(pMachInfo->iReportAttrVmemIdx > 0)
				MtReport_FreeVmem(pMachInfo->iReportAttrVmemIdx);
			strncpy(pMachInfo->szAttrReportDay, m_szLastTableName, sizeof(pMachInfo->szAttrReportDay));
			pMachInfo->iReportAttrVmemIdx = -1;
		}

		// 查找当前的上报属性列表
		pattr = NULL;
		if(pMachInfo->iReportAttrVmemIdx > 0) {
			iTmpLen = (int)sizeof(s_ReportAttrs);
			pattr = (int*)MtReport_GetFromVmemZeroCp(pMachInfo->iReportAttrVmemIdx, (char*)s_ReportAttrs, &iTmpLen);
			if(pattr == NULL) {
				MtReport_FreeVmem(pMachInfo->iReportAttrVmemIdx);
				WARN_LOG("get data from vmem failed, vmem index:%d", pMachInfo->iReportAttrVmemIdx);
				pMachInfo->iReportAttrVmemIdx = -1;
			}
		}

		bool bAdd = false;
		if(pattr == NULL) {
			s_ReportAttrs[0] = pReportShm->iAttrId;
			iTmpLen = sizeof(int);
			iCount = 1;
			pMachInfo->iReportAttrVmemIdx = MtReport_SaveToVmem((char*)s_ReportAttrs, iTmpLen);
			INFO_LOG("receive new attr report, attr:%d, machine:%d, vmemidx:%d", 
				pReportShm->iAttrId, pReportShm->iMachineId, pMachInfo->iReportAttrVmemIdx);
			bAdd = true;
		}
		else {
			if(NULL == bsearch(&pReportShm->iAttrId, pattr, iTmpLen/sizeof(int), sizeof(int), SrvForAttrCmp))
			{
				DEBUG_LOG("find attr info from vmem- len:%d - %d, machine:%d|vmem:%d, cur:%d",
					iTmpLen, *pattr, pReportShm->iMachineId, pMachInfo->iReportAttrVmemIdx, pReportShm->iAttrId);
	
				iCount = iTmpLen/sizeof(int);
				if(iCount+1 >= MAX_REPORT_ATTR_COUNT_PER_MACHINE) {
					ERR_LOG("machine:%d, report attr count over limit:%d", 
						pReportShm->iMachineId, MAX_REPORT_ATTR_COUNT_PER_MACHINE);
					continue;
				}
				else {
					memcpy(s_ReportAttrs, pattr, iTmpLen);
					s_ReportAttrs[iCount] = pReportShm->iAttrId;
					iCount++;
					iTmpLen += sizeof(int);
					qsort(s_ReportAttrs, iCount, sizeof(int), SrvForAttrCmp);

					// 释放原来的 vmem
					if(pMachInfo->iReportAttrVmemIdx > 0)
					{
						MtReport_FreeVmem(pMachInfo->iReportAttrVmemIdx);
						DEBUG_LOG("free vmem machine:%d, vmemidx:%d", pReportShm->iMachineId, pMachInfo->iReportAttrVmemIdx);
					}
					pMachInfo->iReportAttrVmemIdx = MtReport_SaveToVmem((char*)s_ReportAttrs, iTmpLen);
					INFO_LOG("receive new attr report, attr:%d, machine:%d, count:%d, vmemidx:%d, len:%d", 
						pReportShm->iAttrId, pReportShm->iMachineId, iCount, pMachInfo->iReportAttrVmemIdx, iTmpLen);
					bAdd = true;
				}
			}
		}

		// 建立/同步 memcache 缓存
		psrv = slog.GetWebMasterSrv();
		if(psrv == NULL) {
			ERR_LOG("Get web server failed");
			continue;
		}

		// memcache 缓存服务跟 web 服务器同机部署, attr 服务器也可能部署在web 服务器上,需要视情况转换ip
		const char *pserver_ip = psrv->szIpV4;
		if(slog.IsIpMatchLocalMachine(psrv->dwIp))
			pserver_ip = "127.0.0.1";
		std::map<std::string, CMemCacheClient*>::iterator it = stConfig.stMapMemcaches.find(pserver_ip);
		CMemCacheClient *pmemcache = NULL;
		if(it != stConfig.stMapMemcaches.end()) {
			pmemcache = it->second;
		}
		else {
			// 连接 memcache 服务
			char *pbuf = (char*)malloc(stConfig.iMemcacheBufSize);
			pmemcache = new CMemCacheClient;
			if(NULL == pbuf || NULL == pmemcache)
			{
			    ERR_LOG("malloc failed , len:%d", stConfig.iMemcacheBufSize);
				exit(-1);
			}       
			pmemcache->Init(pbuf, stConfig.iMemcacheBufSize);
			if(pmemcache->AsyncConnect(pserver_ip, stConfig.iMemcachePort, stConfig.iRTimeout, stConfig.iWTimeout) < 0)
			{
			    ERR_LOG("AsyncConnect failed, ip:%s, port:%d, rt:%d, wt:%d, msg:%s", 
					pserver_ip, stConfig.iMemcachePort, stConfig.iRTimeout, stConfig.iWTimeout, pmemcache->GetLastErrMsg());
				delete pmemcache;
				free(pbuf);
				continue;
			}
			else
			{
			    INFO_LOG("connect memcache ok, info- ip:%s, port:%d, rt:%d, wt:%d", 
					pserver_ip, stConfig.iMemcachePort, stConfig.iRTimeout, stConfig.iWTimeout);
				stConfig.stMapMemcaches[pserver_ip] = pmemcache;
			}
		}

		// 机器有上报的属性列表 -- memcache 缓存同步 -- 看下 attr 是否在缓存中存在
		if(bAdd || pMachInfo->dwLastVmemSyncMemcacheTime+ATTR_LOCAL_VMEM_SYNC_MEMCACHE_TIME <= dwTimeCur) 
		{
			pMachInfo->dwLastVmemSyncMemcacheTime = dwTimeCur;

			if(!bAdd && pattr != NULL) {
				// 拷贝vmem 用于对比memcache
				memcpy(s_ReportAttrs, pattr, iTmpLen);
				iCount = iTmpLen/sizeof(int);
			}

			// 转为网路序
			for(int i=0; i < iCount; i++)
				s_ReportAttrs[i] = htonl(s_ReportAttrs[i]);

			// 机器有上报的属性 memcache key
			pmemcache->SetKey("machine-attr-%s-%d-monitor", pMachInfo->szAttrReportDay, pReportShm->iMachineId);
			if(!bAdd) {
				int *pattr_list = (int*)pmemcache->GetValue();
				int iMemcacheLen = pmemcache->GetDataLen();
				if(pattr_list == NULL || iMemcacheLen != iTmpLen || memcmp(pattr_list, s_ReportAttrs, iTmpLen))
				{
					// 经对比 vmem 和 memcache 缓存不一样
					bAdd = true;
					ERR_LOG("local vmem not eq memcache|%p|%d|%d, sync it - machine:%d, key:%s", pattr_list, 
						iMemcacheLen, iTmpLen, pReportShm->iMachineId, pmemcache->GetKey());
				}
			}

			if(bAdd) {
				// 有效期设为一天
				if(0 != pmemcache->SetValue(NULL, 0, (char*)s_ReportAttrs, iTmpLen, MEMCACHE_MACHINE_ATTR_EXPIRE_TIME, 0))
				{
					ERR_LOG("memcache set failed, key:%s, len:%d, memcache count:%d, server:%s", pmemcache->GetKey(), 
						iTmpLen, (int)(stConfig.stMapMemcaches.size()), pserver_ip);
				}
				else {
					INFO_LOG("memcache set ok, key:%s, len:%d, info|%u|%u|%d, memcache count:%d, server:%s",
						pmemcache->GetKey(), iTmpLen, ntohl(s_ReportAttrs[0]), ntohl(s_ReportAttrs[iCount-1]), 
						iCount, (int)(stConfig.stMapMemcaches.size()), pserver_ip);
				}
			}
		}

		if(pReportShm->bAttrDataType == STR_REPORT_D || pReportShm->bAttrDataType == STR_REPORT_D_IP)
		    continue;

		comm::MonitorMemcache memInfo;
		comm::AttrVal *pAttrVal = NULL;
		pmemcache->SetKey("machine-attr-val-%s-%d-%d-monitor", 
			pMachInfo->szAttrReportDay, pReportShm->iAttrId, pReportShm->iMachineId);
		if(pmemcache->GetMonitorMemcache(memInfo) >= 0 && memInfo.machine_attr_day_val().attr_val_size() > 0)
		{
			int i = memInfo.machine_attr_day_val().attr_val_size() - 1;
			if(memInfo.machine_attr_day_val().attr_val(i).idx() == (uint32_t)idx
				&& memInfo.machine_attr_day_val().attr_val(i).val() != dwLastVal)
			{
				memInfo.mutable_machine_attr_day_val()->mutable_attr_val(i)->set_val(dwLastVal);
				if((iRet=pmemcache->SetMonitorMemcache(memInfo, NULL, MEMCACHE_MACHINE_ATTR_VAL_EXPIRE_TIME)) != 0)
				{
					WARN_LOG("SetMonitorMemcache failed, count:%d, max:%d, val:%u", 
						memInfo.machine_attr_day_val().attr_val_size(), memInfo.machine_attr_day_val().max_idx(), dwLastVal);
				}
				else {
					DEBUG_LOG("SetMonitorMemcache ok, machine:%d, attr:%d, count:%d, max:%d, val:%u", 
						pReportShm->iMachineId, pReportShm->iAttrId,
						memInfo.machine_attr_day_val().attr_val_size(), memInfo.machine_attr_day_val().max_idx(), dwLastVal);
				}
			}
			else if(memInfo.machine_attr_day_val().attr_val(i).idx() != (uint32_t)idx)
			{
				pAttrVal = memInfo.mutable_machine_attr_day_val()->add_attr_val();
				pAttrVal->set_idx(idx);
				pAttrVal->set_val(dwLastVal);
				memInfo.mutable_machine_attr_day_val()->set_max_idx(idx);
				if((iRet=pmemcache->SetMonitorMemcache(memInfo, NULL, MEMCACHE_MACHINE_ATTR_VAL_EXPIRE_TIME)) != 0)
				{
					ERR_LOG("SetMonitorMemcache failed, idx:%d, count:%d, attr:%d, machine:%d, server:%s",
						idx, memInfo.machine_attr_day_val().attr_val_size(), pReportShm->iAttrId, 
						pReportShm->iMachineId, pserver_ip);
				}
				else
				{
					DEBUG_LOG("SetMonitorMemcache ok, idx:%d, count:%d, attr:%d, machine:%d, server:%s",
						idx, memInfo.machine_attr_day_val().attr_val_size(), pReportShm->iAttrId, 
						pReportShm->iMachineId, pserver_ip);
				}
			}
		}
		else
		{
			GetMonitorAttrMemcache(qu, pReportShm->iAttrId, pReportShm->iMachineId, m_szLastTableName, memInfo);
			pAttrVal = memInfo.mutable_machine_attr_day_val()->add_attr_val();
			pAttrVal->set_idx(idx);
			pAttrVal->set_val(dwLastVal);
			memInfo.mutable_machine_attr_day_val()->set_max_idx(idx);
			if((iRet=pmemcache->SetMonitorMemcache(memInfo, NULL, MEMCACHE_MACHINE_ATTR_VAL_EXPIRE_TIME)) != 0)
			{
				ERR_LOG("SetMonitorMemcache failed, idx:%d, count:%d, attr:%d, machine:%d, server:%s",
					idx, memInfo.machine_attr_day_val().attr_val_size(), pReportShm->iAttrId,
					pReportShm->iMachineId, pserver_ip);
			}
			else
			{
				DEBUG_LOG("SetMonitorMemcache ok, idx:%d, count:%d, attr:%d, machine:%d, server:%s",
					idx, memInfo.machine_attr_day_val().attr_val_size(), pReportShm->iAttrId, 
					pReportShm->iMachineId, pserver_ip);
			}
		}
	}
	if(bReverse && pTableHead->dwNodeUseCount <=1 )  {
		ResetHashTable(&hash);
		hash.bAccessCheck = 1;
	}
	else if(!bReverse && pTableHead->dwNodeUseCount <= 0) {
		hash.bAccessCheck = 0;
	}
}

void CUdpSock::DealViewAutoBindMachine(TWarnAttrReportInfo *pReportShm, Query &qu)
{
	TAttrViewConfigInfo *pViewInfo = slog.GetAttrViewInfo(pReportShm->iAttrId, NULL);
	if(pViewInfo == NULL || pViewInfo->iBindViewCount <= 0)
		return;

	char sql[128] = {0};
	TViewInfo *pView = NULL;
	for(int i = 0, j = 0, idx = 0; i < pViewInfo->iBindViewCount; i++) {
		idx = slog.GetViewInfoIndex(pViewInfo->aryView[i]);
		pView = slog.GetViewInfo(idx);
		if(pView == NULL)
		{
			continue;
		}

		if(!(pView->bViewFlag & VIEW_FLAG_AUTO_BIND_MACHINE))
			continue;
		for(j=0; j < pView->bBindMachineCount; j++) {
			if(pView->aryBindMachines[j] == pReportShm->iMachineId)
				break;
		}

		//注意：AddViewBindMach 直接写入共享内存，可能小概率跟 slog_config 操作冲突, 影响不大
		if(j < pView->bBindMachineCount || slog.AddViewBindMach(pView, pReportShm->iMachineId) == 0)
			continue;

		snprintf(sql, sizeof(sql)-1, "replace into mt_view_bmach set machine_id=%d, view_id=%d",
			pReportShm->iMachineId, pViewInfo->aryView[i]);
		if(!qu.execute(sql))
			WARN_LOG("execute sql:%s failed", sql);
		else 
			INFO_LOG("view:%d auto bind machine:%d, attr:%d", 
				pViewInfo->aryView[i], pReportShm->iMachineId, pReportShm->iAttrId);
	}
}

// 初始化 历史累计数据类型 的共享内存上报信息
void CUdpSock::InitTotalAttrReportShm(int32_t iAttrId, int iMachineId, comm::MonitorMemcache &memInfo, int iMinIdx)
{
	uint32_t dwIsFind = 0;
	TWarnAttrReportInfo *pAttrShm = NULL;
	pAttrShm = slog.GetWarnAttrInfo(iAttrId, iMachineId, &dwIsFind);
	if(pAttrShm == NULL)
	{
		ERR_LOG("add history total attr report shm failed, attrid:%d, machineid:%d", iAttrId, iMachineId);
		return ;
	}

	int i = memInfo.machine_attr_day_val().attr_val_size() - 1;
	uint32_t dwMemcacheVal = memInfo.machine_attr_day_val().attr_val(i).val();
	if(!dwIsFind)
	{
		pAttrShm->iAttrId = iAttrId;
		pAttrShm->iMachineId = iMachineId;
		pAttrShm->bAttrDataType = SUM_REPORT_TOTAL;
		pAttrShm->dwPreLastVal = dwMemcacheVal;
		pAttrShm->dwLastVal = dwMemcacheVal;
		pAttrShm->dwCurVal = dwMemcacheVal;
	}

	if(dwMemcacheVal > pAttrShm->dwCurVal) {
		pAttrShm->dwLastVal = dwMemcacheVal;
		pAttrShm->dwCurVal = dwMemcacheVal;
	}
	pAttrShm->iMinIdx = iMinIdx;
	pAttrShm->dwLastReportTime = slog.m_stNow.tv_sec;
	INFO_LOG("set total history attr report shm info, attrid:%d, machineid:%d, val:%u(%u), minidx:%d",
		iAttrId, iMachineId, pAttrShm->dwCurVal, dwMemcacheVal, iMinIdx);
}


// 处理监控点类型为：历史累计的共享内存逻辑
// 在日期变更时，对于每台上报机器需要为当前时间建立 memcache 缓存信息(历史累计值/或者空值)
// 建立完 memcache 缓存后，WriteAttrDataToDb 这个函数会至少写入一次上报/历史信息到 db 中
void CUdpSock::DealUserTotalAttr(int iAttrId, const char *pOldDay, const char *pNewDay)
{
	SLogServer* psrv = NULL;
	CMemCacheClient *pmemcache = NULL;

	// 建立/同步 memcache 缓存
	psrv = slog.GetWebMasterSrv();
	if(psrv == NULL) {
		ERR_LOG("Get attr memcache/web server failed");
		return;
	}

	// memcache 缓存服务跟 web 服务器同机部署, attr 服务器也可能部署在web 服务器上,需要视情况转换ip
	const char *pserver_ip = psrv->szIpV4;
	if(slog.IsIpMatchLocalMachine(psrv->dwIp))
		pserver_ip = "127.0.0.1";
	std::map<std::string, CMemCacheClient*>::iterator it = stConfig.stMapMemcaches.find(pserver_ip);
	if(it != stConfig.stMapMemcaches.end()) {
		pmemcache = it->second;
	}
	else 
	{
		// 连接 memcache 服务
		char *pbuf = (char*)malloc(stConfig.iMemcacheBufSize);
		pmemcache = new CMemCacheClient;
		if(NULL == pbuf || NULL == pmemcache)
		{
		    ERR_LOG("malloc failed , len:%d", stConfig.iMemcacheBufSize);
			exit(-1);
		}       
		pmemcache->Init(pbuf, stConfig.iMemcacheBufSize);
		if(pmemcache->AsyncConnect(
			pserver_ip, stConfig.iMemcachePort, stConfig.iRTimeout, stConfig.iWTimeout) < 0)
		{
		    ERR_LOG("AsyncConnect failed, ip:%s, port:%d, rt:%d, wt:%d, msg:%s", 
				pserver_ip, stConfig.iMemcachePort, 
				stConfig.iRTimeout, stConfig.iWTimeout, pmemcache->GetLastErrMsg());
			delete pmemcache;
			free(pbuf);
			return;
		}
		else
		{
		    INFO_LOG("connect memcache ok, info - ip:%s, port:%d, rt:%d, wt:%d", 
				pserver_ip, stConfig.iMemcachePort, stConfig.iRTimeout, stConfig.iWTimeout);
			stConfig.stMapMemcaches[pserver_ip] = pmemcache;
		}
	}

	MyQuery myqu(m_qu, db);
	Query & qu = myqu.GetQuery();
	
	// 处理所有上报机器
	comm::MonitorMemcache memInfo;
	comm::AttrVal *pAttrVal = NULL;
	MachineInfo *pInfo = slog.GetMachineInfo(stConfig.psysConfig->iMachineListIndexStart);
	int iCurDayOfMin = GetDayOfMin();
	for(int i=0; i < stConfig.psysConfig->wMachineCount && pInfo != NULL; i++) 
	{
		memInfo.Clear();

		// 尝试查询下 当天的 db, 如果在 memcache 中找到则无需处理
		pmemcache->SetKey("machine-attr-val-%s-%d-%d-monitor", pNewDay, iAttrId, pInfo->id);
		if(pmemcache->GetMonitorMemcache(memInfo) >= 0 && memInfo.machine_attr_day_val().attr_val_size() > 0)
		{
			InitTotalAttrReportShm(iAttrId, pInfo->id, memInfo, iCurDayOfMin);
			pInfo = slog.GetMachineInfo(pInfo->iNextIndex);
			continue;
		}

		// 从 db 数据建立 memcache
		GetMonitorAttrMemcache(qu, iAttrId, pInfo->id, pNewDay, memInfo);
		if(memInfo.machine_attr_day_val().attr_val_size() > 0)
		{
			InitTotalAttrReportShm(iAttrId, pInfo->id, memInfo, iCurDayOfMin);
			if(0 != pmemcache->SetMonitorMemcache(memInfo, NULL, MEMCACHE_MACHINE_ATTR_VAL_EXPIRE_TIME))
				ERR_LOG("SetMonitorMemcache failed, count:%d, attr id:%d, machine id:%d",
					memInfo.machine_attr_day_val().attr_val_size(), iAttrId, pInfo->id);
			pInfo = slog.GetMachineInfo(pInfo->iNextIndex);
			continue;
		}

		// 尝试查询下 前一天的 db, 有上报数据则建立历史累计值 memcache 信息
		GetMonitorAttrMemcache(qu, iAttrId, pInfo->id, pOldDay, memInfo); 
		if(memInfo.machine_attr_day_val().attr_val_size() > 0) 
		{
			// 取前一天最后一分钟的上报数据 建立  memcache 缓存
			comm::MonitorMemcache memTmp;
			int iTmpIdx = memInfo.machine_attr_day_val().attr_val_size() - 1;
			pAttrVal = memTmp.mutable_machine_attr_day_val()->add_attr_val();
			pAttrVal->set_idx(iCurDayOfMin);
			pAttrVal->set_val(memInfo.machine_attr_day_val().attr_val(iTmpIdx).val());
			memTmp.mutable_machine_attr_day_val()->set_max_idx(iCurDayOfMin);
			InitTotalAttrReportShm(iAttrId, pInfo->id, memInfo, iCurDayOfMin);
			if(0 != pmemcache->SetMonitorMemcache(memTmp, NULL, MEMCACHE_MACHINE_ATTR_VAL_EXPIRE_TIME))
				ERR_LOG("SetMonitorMemcache failed, count:%d, attr id:%d, machine id:%d, day:%s|%s",
					memInfo.machine_attr_day_val().attr_val_size(), 
					iAttrId, pInfo->id, pOldDay, pNewDay);
			else
				INFO_LOG("get sum history attr id:%d, machine id:%d, repinfo:%s, day:%s|%s",
					iAttrId, pInfo->id, pAttrVal->ShortDebugString().c_str(), pOldDay, pNewDay);
		}
		else {
			// 无上报值，建立空的 memcache 缓存
			pAttrVal = memInfo.mutable_machine_attr_day_val()->add_attr_val();
			pAttrVal->set_idx(iCurDayOfMin);
			memInfo.mutable_machine_attr_day_val()->set_max_idx(iCurDayOfMin);
			InitTotalAttrReportShm(iAttrId, pInfo->id, memInfo, iCurDayOfMin);
			if(0 != pmemcache->SetMonitorMemcache(memInfo, NULL, MEMCACHE_MACHINE_ATTR_VAL_EXPIRE_TIME))
				ERR_LOG("SetMonitorMemcache failed, (empty) attr id:%d, machine id:%d, day:%s|%s",
					iAttrId, pInfo->id, pOldDay, pNewDay);
			else
				INFO_LOG("set empty history attr memcache attr id:%d, machine id:%d, day:%s|%s",
					iAttrId, pInfo->id, pOldDay, pNewDay);
		}
		pInfo = slog.GetMachineInfo(pInfo->iNextIndex);
	}
}

// 处理类型为：历史累计 的监控点
void CUdpSock::CheckAllAttrTotal()
{
	static uint32_t s_LastCheckTime = 0;
	static char s_LastDayInfo[32] = {0};

	if(slog.m_stNow.tv_sec < s_LastCheckTime+5+slog.m_iRand%8)
		return ;
	s_LastCheckTime = slog.m_stNow.tv_sec;

	const char *pday = GetTodayTableName();
	char szOldDayInfo[32] = {0};
	if(s_LastDayInfo[0] == '\0')
	{
		strncpy(s_LastDayInfo, pday, sizeof(s_LastDayInfo));
		const char *pday_bf = GetBeforeDayTableName();
		strncpy(szOldDayInfo, pday_bf, sizeof(szOldDayInfo));
		INFO_LOG("day info init, day:%s, before:%s", s_LastDayInfo, pday_bf);
	}
	else if(strcmp(pday, s_LastDayInfo))
	{
		strncpy(szOldDayInfo, s_LastDayInfo, sizeof(szOldDayInfo));
		strncpy(s_LastDayInfo, pday, sizeof(s_LastDayInfo));
		INFO_LOG("day info changed from:%s to %s", szOldDayInfo, s_LastDayInfo);
	}
	else
		return ;

	AttrInfoBin *pAttrInfo = NULL;
	pAttrInfo = slog.GetAttrInfo(stConfig.psysConfig->iAttrIndexStart);
	for(int i = 0; i < stConfig.psysConfig->wAttrCount && pAttrInfo != NULL; i++) {
		if(pAttrInfo->iDataType == SUM_REPORT_TOTAL) 
			DealUserTotalAttr(pAttrInfo->id, szOldDayInfo, s_LastDayInfo);
		pAttrInfo = slog.GetAttrInfo(pAttrInfo->iNextIndex);
	}
}

int CUdpSock::ReadStrAttrInfoFromDbToShm()
{
	if(CheckTableName() < 0)
		return SLOG_ERROR_LINE;

	MyQuery myqu(m_qu, db);
	Query & qu = myqu.GetQuery();

	char szSql[256] = {0};
	snprintf(szSql, sizeof(szSql),
		"select str_val,attr_id,report_time from %s where value=0", m_szLastTableName);
	if(!qu.get_result(szSql)) {
		qu.free_result();
		INFO_LOG("have no str attr report info in db");
		return 0;
	}

	const char *pval = NULL;
	TStrAttrReportInfo* pStrAttrShm = NULL;
	comm::ReportAttr stAttrInfoPb;
	unsigned long* lengths = NULL;
	int iAttrId = 0;
	AttrInfoBin *pAttrInfo = NULL;

	for(int i=0; i < qu.num_rows() && qu.fetch_row(); i++)
	{
		lengths = qu.fetch_lengths();
		pval = qu.getstr("str_val");
		stAttrInfoPb.Clear();
		if(lengths[0] > 0 && !stAttrInfoPb.ParseFromArray(pval, lengths[0]))
		{
			WARN_LOG("ParseFromArray failed-%p-%lu", pval, lengths[0]);
			continue;
		}
		iAttrId = qu.getval("attr_id"); 
		pAttrInfo = slog.GetAttrInfo(iAttrId, NULL);
		if(pAttrInfo == NULL) {
			ERR_LOG("not find attr:%d", iAttrId);
			continue;
		}

		DEBUG_LOG("read str attr id:%d, info:%s, from db", iAttrId, stAttrInfoPb.ShortDebugString().c_str());
		for(int j=0; j < stAttrInfoPb.msg_attr_info().size(); j++)
		{
			stAttrInfoPb.mutable_msg_attr_info(j)->set_uint32_attr_id(iAttrId);

			// 字符串类型已经转换过了, 这里不用处理
			pStrAttrShm = AddStrAttrReportToShm(stAttrInfoPb.msg_attr_info(j), stAttrInfoPb.report_host_id(), 0);
			if(pStrAttrShm != NULL) {
				pStrAttrShm->dwLastReportTime = qu.getuval("report_time");
				pStrAttrShm->bAttrDataType = pAttrInfo->iDataType;
				DealMachineAttrReport(pStrAttrShm);
			}
		}
	}
	qu.free_result();
	return 0;
}


int CUdpSock::SaveStrAttrInfoToDb(TStrAttrReportInfo* pStrAttrShm, Query &qu)
{
	ReportAttr stAttrInfoPb;
	StrAttrNodeVal *pNodeShm = NULL;
	int idx = pStrAttrShm->iReportIdx, i = 0;
	comm::AttrInfo *pInfo = NULL;

	for(i=0; i < pStrAttrShm->bStrCount; i++) 
	{
		pNodeShm = stConfig.pstrAttrShm->stInfo+idx;
		pInfo = stAttrInfoPb.add_msg_attr_info();
		pInfo->set_uint32_attr_value(pNodeShm->iStrVal);
		pInfo->set_str(pNodeShm->szStrInfo);
		idx = pNodeShm->iNextStrAttr;
	}
	stAttrInfoPb.set_report_host_id(pStrAttrShm->iMachineId);
	DEBUG_LOG("get str attr info from shm to pb:%s", stAttrInfoPb.ShortDebugString().c_str());

	std::string strval;
	if(!stAttrInfoPb.AppendToString(&strval))
	{
		ERR_LOG("AppendToString failed, pb:%s", stAttrInfoPb.ShortDebugString().c_str());
		return SLOG_ERROR_LINE;
	}

	if(CheckTableName() < 0)
	{
		ERR_LOG("CheckTableName failed!");
		return SLOG_ERROR_LINE;
	}

	snprintf(stConfig.szBinSql, sizeof(stConfig.szBinSql),
		"select id from %s where attr_id=%d and machine_id=%d", 
		m_szLastTableName, pStrAttrShm->iAttrId, stAttrInfoPb.report_host_id());
	if(!qu.get_result(stConfig.szBinSql))
	{
		ERR_LOG("exec sql:%s failed!", stConfig.szBinSql);
		return SLOG_ERROR_LINE;
	}

	int iDbId = 0, iSqlLen = 0, iTmpLen = 0;
	if(qu.num_rows() > 0 && qu.fetch_row() != NULL)
		iDbId = qu.getval("id");
	qu.free_result();

	if(iDbId > 0) {
		iTmpLen = snprintf(stConfig.szBinSql, sizeof(stConfig.szBinSql),
			"update %s set report_time=\'%s\',client_ip=\'%s\', str_val=", 
			m_szLastTableName, uitodate(pStrAttrShm->dwLastReportTime), ipv4_addr_str(pStrAttrShm->dwLastReportIp));
		char *pbuf = (char*)stConfig.szBinSql+iTmpLen;
		int32_t iBinaryDataLen = qu.SetBinaryData(pbuf, strval.c_str(), strval.size());
		if(iBinaryDataLen < 0)
			return SLOG_ERROR_LINE;
		pbuf += iBinaryDataLen;
		iSqlLen = (int32_t)(pbuf-stConfig.szBinSql);
		iTmpLen = snprintf(pbuf, sizeof(stConfig.szBinSql)-iSqlLen, " where id=%d", iDbId);
		if(iTmpLen < (int)(sizeof(stConfig.szBinSql)-iSqlLen))
			iSqlLen += iTmpLen;
		else
		{
			ERR_LOG("need more space %d < %d", iTmpLen, (int)(sizeof(stConfig.szBinSql)-iSqlLen));
			return SLOG_ERROR_LINE;
		}
		if(qu.ExecuteBinary(stConfig.szBinSql, iSqlLen) < 0)
			return SLOG_ERROR_LINE;
	}
	else {
		// value 字段设为 0，表示该记录的数据为字符串型监控点
		iTmpLen = snprintf(stConfig.szBinSql, sizeof(stConfig.szBinSql),
			"insert into %s set attr_id=%d,machine_id=%d,client_ip=\'%s\',report_time=\'%s\',value=0,str_val=", 
			m_szLastTableName, pStrAttrShm->iAttrId, stAttrInfoPb.report_host_id(),
			ipv4_addr_str(pStrAttrShm->dwLastReportIp), uitodate(pStrAttrShm->dwLastReportTime));
		char *pbuf = (char*)stConfig.szBinSql+iTmpLen;
		int32_t iBinaryDataLen = qu.SetBinaryData(pbuf, strval.c_str(), strval.size());
		if(iBinaryDataLen < 0)
			return SLOG_ERROR_LINE;
		pbuf += iBinaryDataLen;
		iSqlLen = (int32_t)(pbuf-stConfig.szBinSql);
		if(qu.ExecuteBinary(stConfig.szBinSql, iSqlLen) < 0)
			return SLOG_ERROR_LINE;
		iDbId = qu.insert_id();
	}

	DEBUG_LOG("save str attr:%d to db ok, record id:%d, sql len:%d, info:%s",
		pStrAttrShm->iAttrId, iDbId, iSqlLen, stAttrInfoPb.ShortDebugString().c_str());
	return 0;
}

void CUdpSock::WriteStrAttrToDb()
{
	// 间隔一定时间，全盘写入 db
	static uint32_t s_dwNextSaveTime = 0;
	static SharedHashTable s_stStrAttrHash = slog.GetStrAttrShmHash();
	if(s_dwNextSaveTime >= slog.m_stNow.tv_sec)
		return;
	s_dwNextSaveTime = slog.m_stNow.tv_sec+rand()%60+30;

	MyQuery myqu(m_qu, db);
	Query & qu = myqu.GetQuery();
	
	bool bReverse = false;
	_HashTableHead *pTableHead = (_HashTableHead*)s_stStrAttrHash.pHash;
	TStrAttrReportInfo *pStrAttrShm = (TStrAttrReportInfo*)GetFirstNode(&s_stStrAttrHash);
	if(pStrAttrShm == NULL && pTableHead->dwNodeUseCount > 0) {
		bReverse = true;
		pStrAttrShm = (TStrAttrReportInfo*)GetFirstNodeRevers(&s_stStrAttrHash);
	}

	int iStrAttrCount = 0;
	while(pStrAttrShm != NULL) 
	{
		if(pStrAttrShm->dwLastReportTime > pStrAttrShm->dwLastSaveDbTime) {
			SaveStrAttrInfoToDb(pStrAttrShm, qu);
			pStrAttrShm->dwLastSaveDbTime = slog.m_stNow.tv_sec;
		}
		iStrAttrCount++;
		if(CheckClearStrAttrNodeShm(pStrAttrShm))
			RemoveHashNode(&s_stStrAttrHash, pStrAttrShm);
		if(bReverse)
			pStrAttrShm = (TStrAttrReportInfo*)GetNextNodeRevers(&s_stStrAttrHash);
		else
			pStrAttrShm = (TStrAttrReportInfo*)GetNextNode(&s_stStrAttrHash);
	}
	INFO_LOG("save str attr to db, count:%d", iStrAttrCount);
}

void CUdpSock::WriteAttrDataToDb()
{
	int idx = GetDayOfMin();
	if(idx == stConfig.pShmConfig->stSysCfg.iLastSaveAttrToDbIdx)
		return;

	stConfig.pShmConfig->stSysCfg.iLastSaveAttrToDbIdx = idx;
	if(CheckTableName() < 0)
	{
		ERR_LOG("CheckTableName failed!");
		return;
	}

	MyQuery myqu(m_qu, db);
	Query & qu = myqu.GetQuery();
	MyQuery myqu_if(m_qu_attr_info, db_attr_info);
	Query & qu_info = myqu_if.GetQuery();

	uint32_t dwLastVal  = 0; 
	uint32_t now = slog.m_stNow.tv_sec;
	SharedHashTable & hash = slog.GetWarnAttrHash();
	bool bReverse = false;
	_HashTableHead *pTableHead = (_HashTableHead*)hash.pHash;
	TWarnAttrReportInfo *pReportShm = (TWarnAttrReportInfo*)GetFirstNode(&hash);
	if(pReportShm == NULL && pTableHead->dwNodeUseCount > 0) {
		bReverse = true;
		pReportShm = (TWarnAttrReportInfo*)GetFirstNodeRevers(&hash);
		ERR_LOG("man by bug - has node:%u, bug get first null", pTableHead->dwNodeUseCount);
	}

	int i = 0;
	for(i=0; pReportShm != NULL; 
		bReverse ? (pReportShm = (TWarnAttrReportInfo*)GetNextNodeRevers(&hash)) :
			(pReportShm = (TWarnAttrReportInfo*)GetNextNode(&hash)), i++)
	
	{
		dwLastVal = pReportShm->dwLastVal;
		if(now >= pReportShm->dwLastReportTime+1440*60)
			dwLastVal = 0;
		else if(idx != pReportShm->iMinIdx)
		{
			if(((pReportShm->iMinIdx+1)%1440) == idx)
				dwLastVal = pReportShm->dwCurVal;
			else
				dwLastVal = 0;
		}

		if(dwLastVal <= 0)
			continue;

		snprintf(stConfig.szBinSql, sizeof(stConfig.szBinSql), 
			"insert into %s set attr_id=%u,machine_id=%d,value=%u,client_ip=\'%s\'",
			m_szLastTableName, pReportShm->iAttrId, pReportShm->iMachineId, 
			dwLastVal, ipv4_addr_str(pReportShm->dwLastReportIp));
		if(!qu.execute(stConfig.szBinSql))
		{
			ERR_LOG("execute sql:%s failed ! (attr:%u|%u, val:%u|%u)", stConfig.szBinSql,
				pReportShm->iAttrId, pReportShm->iMachineId, dwLastVal, pReportShm->dwLastVal);
			break;
		}

		// 视图自动绑定上报机器实现
		DealViewAutoBindMachine(pReportShm, qu_info);
	}
	DEBUG_LOG("write attr report to db count:%d", i);

	// 非配置可以重建哈希中的链表
	if(bReverse && pTableHead->dwNodeUseCount <=1 )  {
		ResetHashTable(&hash);
		hash.bAccessCheck = 1;
	}
	else if(!bReverse && pTableHead->dwNodeUseCount <= 0) {
		hash.bAccessCheck = 0;
	}
}

int CUdpSock::CheckClearStrAttrNodeShm(TStrAttrReportInfo* pStrAttrShm)
{
	if(pStrAttrShm->dwLastReportTime+24*3600 < slog.m_stNow.tv_sec
		|| pStrAttrShm->bLastReportDayOfMonth != m_currDateTime.tm_mday)
	{
		if(pStrAttrShm->bStrCount <= 0)
			return 1;

		// 跨天了，清掉数据重新统计
		DEBUG_LOG("clear all str attr report, attr:%d, type:%d, count:%d, idx:%d, time:%u(%u), day:%d(%d)",
			pStrAttrShm->iAttrId, pStrAttrShm->bAttrDataType, pStrAttrShm->bStrCount, pStrAttrShm->iReportIdx,
			pStrAttrShm->dwLastReportTime, (uint32_t)slog.m_stNow.tv_sec, pStrAttrShm->bLastReportDayOfMonth,
			m_currDateTime.tm_mday);

		int idx = pStrAttrShm->iReportIdx;
		stConfig.pstrAttrShm->iNodeUse -= pStrAttrShm->bStrCount;
		pStrAttrShm->bStrCount = 0;
		pStrAttrShm->iReportIdx = -1;
		while(idx >= 0 && idx < MAX_STR_ATTR_ARRY_NODE_VAL_COUNT)
		{
			stConfig.pstrAttrShm->stInfo[idx].iStrVal = -1;
			idx = stConfig.pstrAttrShm->stInfo[idx].iNextStrAttr;
		}
		return 1;
	}
	return 0;
}

TStrAttrReportInfo * CUdpSock::AddStrAttrReportToShm(
	const ::comm::AttrInfo & reportInfo, int32_t iMachineId, uint8_t bAttrDataType)
{
	uint32_t dwIsFind = 0;
	uint32_t dwAttrId = reportInfo.uint32_attr_id();
	TStrAttrReportInfo *pAttrShm = NULL;

	pAttrShm = slog.GetStrAttrShmInfo((int32_t)dwAttrId, iMachineId, &dwIsFind);
	if(pAttrShm == NULL)
	{
		ERR_LOG("add str attr report failed, attrid:%d, machineid:%d, str:%s, val:%u",
			(int32_t)dwAttrId, iMachineId, reportInfo.str().c_str(), 
			reportInfo.uint32_attr_value());
		return NULL;
	}
	CheckClearStrAttrNodeShm(pAttrShm);

	const char *pstr = reportInfo.str().c_str();
	if(bAttrDataType == STR_REPORT_D_IP) {
		pstr = GetRemoteRegionInfoNew(reportInfo.str().c_str(), IPINFO_FLAG_PROV_VMEM).c_str();
		DEBUG_LOG("change str attr:%d, ip str from:%s to:%s", 
			reportInfo.uint32_attr_id(), reportInfo.str().c_str(), pstr);
	}

	if(!dwIsFind)
	{
		pAttrShm->iAttrId = (int32_t)dwAttrId;
		pAttrShm->iMachineId = iMachineId;
		pAttrShm->bStrCount = 0;
		pAttrShm->iReportIdx = -1;
		pAttrShm->bAttrDataType = bAttrDataType;
		INFO_LOG("init add str attr report info, attrid:%d, machineid:%d", (int32_t)dwAttrId, iMachineId);
	}
	if(pAttrShm->bAttrDataType != bAttrDataType)
		pAttrShm->bAttrDataType = bAttrDataType;

	StrAttrNodeVal *pNodeShm = NULL;
	int idx = pAttrShm->iReportIdx, i = 0, iLastIdx = -1;
	for(i=0; i < pAttrShm->bStrCount; i++) 
	{
		if(idx < 0 || idx >= MAX_STR_ATTR_ARRY_NODE_VAL_COUNT)
		{
			ERR_LOG("invalid str attr report idx, attr:%u, idx:%d(0-%d)", 
				dwAttrId, idx, MAX_STR_ATTR_ARRY_NODE_VAL_COUNT);
			return pAttrShm;
		}

		pNodeShm = stConfig.pstrAttrShm->stInfo+idx;
		if(!strcmp(pNodeShm->szStrInfo, pstr)) {
			pNodeShm->iStrVal += reportInfo.uint32_attr_value();
			pAttrShm->bLastReportDayOfMonth = m_currDateTime.tm_mday;
			DEBUG_LOG("add str attr report, attr:%d, str:%s, rep val:%d, cur rep:%d, day:%d",
				reportInfo.uint32_attr_id(), pstr, reportInfo.uint32_attr_value(), 
				pNodeShm->iStrVal, pAttrShm->bLastReportDayOfMonth);
			return pAttrShm;
		}
		if(i+1 >= pAttrShm->bStrCount) {
			iLastIdx = idx;
			break;
		}
		idx = stConfig.pstrAttrShm->stInfo[idx].iNextStrAttr;
	}

	int iMaxNode = STR_ATTR_COUNT_FOR_SELECT_STR + MAX_STR_ATTR_STR_COUNT;
	if(pAttrShm->bStrCount >= iMaxNode) 
	{
		// 超过最大保留节点数了，去掉小上报值的保留节点数个节点，以便重新筛选 
		std::multimap<int, int> stMapRep;
		idx = pAttrShm->iReportIdx;
		for(i=0; i < pAttrShm->bStrCount && idx > 0 && idx < MAX_STR_ATTR_ARRY_NODE_VAL_COUNT; i++)
		{
			stMapRep.insert(std::make_pair(stConfig.pstrAttrShm->stInfo[idx].iStrVal, idx));
			idx = stConfig.pstrAttrShm->stInfo[idx].iNextStrAttr;
		}

		// 移除前 STR_ATTR_COUNT_FOR_SELECT_STR 个上报
		std::multimap<int, int>::iterator it = stMapRep.begin();
		for(i=0; i < STR_ATTR_COUNT_FOR_SELECT_STR && it != stMapRep.end(); it++, i++)
		{
			DEBUG_LOG("remove str attr:%d report, str:%s, val:%d, max node:%d",
				reportInfo.uint32_attr_id(), stConfig.pstrAttrShm->stInfo[it->second].szStrInfo, 
				stConfig.pstrAttrShm->stInfo[it->second].iStrVal, iMaxNode);

			stConfig.pstrAttrShm->stInfo[it->second].iStrVal = -1;
			stConfig.pstrAttrShm->stInfo[it->second].iNextStrAttr = -1;
		}
		stConfig.pstrAttrShm->iNodeUse -= STR_ATTR_COUNT_FOR_SELECT_STR;

		pAttrShm->iReportIdx = it->second;
		pAttrShm->bStrCount -= STR_ATTR_COUNT_FOR_SELECT_STR;
		while(true) {
			idx = it->second;
			it++;
			if(it != stMapRep.end())
				stConfig.pstrAttrShm->stInfo[idx].iNextStrAttr = it->second;
			else {
				stConfig.pstrAttrShm->stInfo[idx].iNextStrAttr = -1;
				iLastIdx = idx;	
				break;
			}
		}
	} 

	// 获取一个空闲节点存储新的上报值
	for(i=0; i < MAX_STR_ATTR_ARRY_NODE_VAL_COUNT; i++) 
	{
		idx = stConfig.pstrAttrShm->iWriteIdx;
		stConfig.pstrAttrShm->iWriteIdx++;
		if(stConfig.pstrAttrShm->iWriteIdx >= MAX_STR_ATTR_ARRY_NODE_VAL_COUNT)
			stConfig.pstrAttrShm->iWriteIdx = 0;

		if(stConfig.pstrAttrShm->stInfo[idx].iStrVal <= 0) 
		{
			stConfig.pstrAttrShm->iNodeUse++;
			stConfig.pstrAttrShm->stInfo[idx].szStrInfo[
				sizeof(stConfig.pstrAttrShm->stInfo[idx].szStrInfo)-1] = '\0';
			strncpy(stConfig.pstrAttrShm->stInfo[idx].szStrInfo, 
				pstr, sizeof(stConfig.pstrAttrShm->stInfo[idx].szStrInfo)-1);
			stConfig.pstrAttrShm->stInfo[idx].iStrVal = reportInfo.uint32_attr_value();
			stConfig.pstrAttrShm->stInfo[idx].iNextStrAttr = -1;
			if(iLastIdx >= 0 && iLastIdx < MAX_STR_ATTR_ARRY_NODE_VAL_COUNT) {
				stConfig.pstrAttrShm->stInfo[iLastIdx].iNextStrAttr = idx;
				pAttrShm->bStrCount++;
			}
			else 
			{
				pAttrShm->iReportIdx = idx;
				pAttrShm->bStrCount = 1;
			}
			pAttrShm->bLastReportDayOfMonth = m_currDateTime.tm_mday;
			DEBUG_LOG("save str attr report, attr id:%d, str:%s, val:%d, cur count:%d, max count:%d, day:%d",
				reportInfo.uint32_attr_id(), pstr, reportInfo.uint32_attr_value(), pAttrShm->bStrCount,
				iMaxNode, pAttrShm->bLastReportDayOfMonth);
			break;
		}
	}
	return pAttrShm;
}

// 字符串型监控点上报,监控点写入 TWarnAttrReportInfo 表中,以便建立memcache 缓存
void CUdpSock::DealMachineAttrReport(TStrAttrReportInfo *pAttrShm)
{
	uint32_t dwIsFind = 0;
	TWarnAttrReportInfo *pAttrShmRep = NULL;
	pAttrShmRep = slog.GetWarnAttrInfo(pAttrShm->iAttrId, pAttrShm->iMachineId, &dwIsFind);
	if(pAttrShmRep == NULL)
	{
		ERR_LOG("add str attr report val failed, attrid:%d, machineid:%d",
			pAttrShm->iAttrId, pAttrShm->iMachineId);
		return ;
	}

	pAttrShmRep->dwLastReportTime = slog.m_stNow.tv_sec;
	if(!dwIsFind)
	{
		pAttrShmRep->iAttrId = pAttrShm->iAttrId;
		pAttrShmRep->iMachineId = pAttrShm->iMachineId;
		pAttrShmRep->bAttrDataType = pAttrShm->bAttrDataType;
		DEBUG_LOG("add str attr info to TWarnAttrReportInfo, attrid:%d, machineid:%d",
			pAttrShm->iAttrId, pAttrShm->iMachineId);
	}
	else if(pAttrShmRep->bAttrDataType != pAttrShm->bAttrDataType)
	{
		pAttrShmRep->bAttrDataType = pAttrShm->bAttrDataType;
		DEBUG_LOG("change str attr info in TWarnAttrReportInfo, attrid:%d, machineid:%d, bAttrDataType:%d",
			pAttrShmRep->iAttrId, pAttrShmRep->iMachineId, pAttrShmRep->bAttrDataType);
	}
}

void CUdpSock::DealReportAttr(::comm::ReportAttr &stReport)
{
	DEBUG_LOG("report attr time client:%u server:%u, attr total:%u, from:%s",
		stReport.uint32_client_rep_time(), (uint32_t)time(NULL), stReport.msg_attr_info().size(),
		stReport.bytes_report_ip().c_str());

	AttrInfoBin *pAttrInfo = NULL;
	TWarnAttrReportInfo *pInfoShm = NULL;
	TStrAttrReportInfo *pStrInfoShm = NULL;
	int idx = GetDayOfMin();
	int iDataType = 0;

	for(int i=0; i < stReport.msg_attr_info().size(); i++)
	{
		if(stReport.msg_attr_info(i).uint32_attr_value() <= 0)
			continue;
		m_iDealAttrId = stReport.msg_attr_info(i).uint32_attr_id();
		slog.CheckTest(CheckRequestLog_test, this);

		pAttrInfo = slog.GetAttrInfo(stReport.msg_attr_info(i).uint32_attr_id(), NULL); 
		if(NULL==pAttrInfo)
		{
			REQERR_LOG("not find attr - id:%u, value:%u, client:%s",
				stReport.msg_attr_info(i).uint32_attr_id(), 
				stReport.msg_attr_info(i).uint32_attr_value(), stReport.bytes_report_ip().c_str());
			continue;
		}
		iDataType = pAttrInfo->iDataType;
		m_pcltMachine->dwLastReportAttrTime = time(NULL);

		// 字符串型监控点
		if(iDataType == STR_REPORT_D || iDataType == STR_REPORT_D_IP) 
		{
			pStrInfoShm = AddStrAttrReportToShm(stReport.msg_attr_info(i), m_pcltMachine->id, iDataType);
			if(pStrInfoShm != NULL) 
			{
				// 更新需要在上报时检查的相关字段
				uint32_t dwRemoteIp = inet_addr(m_addrRemote.Convert().c_str());
				if(pStrInfoShm->dwLastReportIp != dwRemoteIp)
				{
					DEBUG_LOG("remove connect ip changed from:%s to:%s", 
						ipv4_addr_str(pStrInfoShm->dwLastReportIp), m_addrRemote.Convert().c_str());
					pStrInfoShm->dwLastReportIp = dwRemoteIp;
				}
				pStrInfoShm->dwLastReportTime = slog.m_stNow.tv_sec;
				DealMachineAttrReport(pStrInfoShm);
			}
			DEBUG_LOG("report:%d, str attr:%u, str:%s, val:%u client:%s",
				i, stReport.msg_attr_info(i).uint32_attr_id(), stReport.msg_attr_info(i).str().c_str(),
				stReport.msg_attr_info(i).uint32_attr_value(), stReport.bytes_report_ip().c_str());
		}
		else 
		{
			// 非字符串型监控点
			pInfoShm = AddReportToWarnAttrShm(stReport.msg_attr_info(i).uint32_attr_id(), 
				m_pcltMachine->id, stReport.msg_attr_info(i).uint32_attr_value(), idx, iDataType);
			if(pInfoShm != NULL) {
				uint32_t dwRemoteIp = inet_addr(m_addrRemote.Convert().c_str());
				if(pInfoShm->dwLastReportIp != dwRemoteIp)
				{
					DEBUG_LOG("ip changed from:%s to:%s", 
						ipv4_addr_str(pInfoShm->dwLastReportIp), m_addrRemote.Convert().c_str());
					pInfoShm->dwLastReportIp = dwRemoteIp;
				}
			}
			DEBUG_LOG("report:%d attr:%u val:%u client:%s", i, stReport.msg_attr_info(i).uint32_attr_id(), 
				stReport.msg_attr_info(i).uint32_attr_value(), stReport.bytes_report_ip().c_str());
		}
	}
}

int32_t CUdpSock::SendResponsePacket(const char *pkg, int len)
{
	SendToBuf(m_addrRemote, pkg, len, 0);
	DEBUG_LOG("send response packet to client:%s, pkglen:%d", m_addrRemote.Convert(true).c_str(), len);
	return 0;
}

void CUdpSock::ResetRequest()
{
	ResetPacketInfo();
	m_pcltMachine = NULL;
	m_iDealAttrId = 0;
	localtime_r(&slog.m_stNow.tv_sec, &m_currDateTime);
	slog.SetTestLog(false);
	slog.ClearAllCust();
}

void CUdpSock::OnRawData(const char *buf, size_t len, struct sockaddr *sa, socklen_t sa_len)
{
	ResetRequest();
	SetConnected();
	m_addrRemote.SetAddress(sa);
	slog.SetCust_6(m_addrRemote.Convert(true).c_str());

	// check packet
	int iRet = 0;
	if((iRet=CheckBasicPacket(buf, len)) != NO_ERROR)
	{
		if(iRet != ERR_NOT_ACK)
			CBasicPacket::AckToReq(iRet);
		MtReport_Attr_Add(74, 1);
		return ;
	}

	m_pcltMachine = NULL;
	slog.CheckTest(NULL);
	OnRawDataClientAttr(buf, len);
}

int32_t CUdpSock::CheckSignature()
{
	MonitorCommSig *pcommSig = NULL;
	size_t iDecSigLen = 0;

	switch(m_pstSig->bSigType) {
		case MT_SIGNATURE_TYPE_COMMON:
			aes_decipher_data((const uint8_t*)m_pstSig->sSigValue, ntohs(m_pstSig->wSigLen),
			    (uint8_t*)m_sDecryptBuf, &iDecSigLen, 
				(const uint8_t*)stConfig.psysConfig->szAgentAccessKey, AES_128);
			if(iDecSigLen != sizeof(MonitorCommSig)) {
				REQERR_LOG("decrypt failed, %lu != %lu, siglen:%d, key:%s",
					iDecSigLen, sizeof(MonitorCommSig), ntohs(m_pstSig->wSigLen), 
					m_pcltMachine->sRandKey);
				return ERR_DECRYPT_FAILED;
			}
			pcommSig = (MonitorCommSig*)m_sDecryptBuf;
			if(ntohl(pcommSig->dwSeq) != m_dwReqSeq) {
				REQERR_LOG("invalid signature info - seq(%u,%u)", ntohl(pcommSig->dwSeq), m_dwReqSeq);
				return ERR_INVALID_SIGNATURE_INFO;
			}
			break; 

		default:
			break;
	}
	return 0;
}

int CUdpSock::DealCgiReportAttr(const char *buf, size_t len)
{
	static char s_CommReportMachIp[] = "127.0.0.1";

	m_pcltMachine = slog.GetMachineInfoByIp(s_CommReportMachIp);
	if(!m_pcltMachine) {
		ERR_LOG("have no common report machine(127.0.0.1)");
		return ERR_SERVER;
	}

	// 请求来自服务器
	int iReqMachineId = ntohl(*(int32_t*)(m_pstReqHead->sReserved));
	if(iReqMachineId == 0 || NULL == slog.GetMachineInfo(iReqMachineId, NULL)) {
		REQERR_LOG("not find server machine, id:%d", iReqMachineId);
		return ERR_INVALID_PACKET;
	}

	int iReadAttrCount = 0;
	::comm::ReportAttr stReport;
	::comm::AttrInfo *pstAttr = NULL;
	stReport.set_uint32_client_rep_time(time(NULL));
	stReport.set_bytes_report_ip(s_CommReportMachIp);
	stReport.set_report_host_id(m_pcltMachine->id);

	if(m_dwReqCmd == CMD_CGI_SEND_ATTR) {
		AttrNodeClient *pInfo= NULL;
		for(int iRead=0; m_wCmdContentLen >= iRead+sizeof(AttrNodeClient); )  
		{
			pInfo = (AttrNodeClient*)((char*)m_pstCmdContent+iRead);
			AttrInfoNtoH(pInfo);
			iRead += sizeof(AttrNodeClient);
			DEBUG_LOG("read cgi report attr from:%s id:%d, value:%d, iRead:%d, len:%d",
				m_addrRemote.Convert().c_str(), pInfo->iAttrID, pInfo->iCurValue, iRead, m_wCmdContentLen);
			pstAttr = stReport.add_msg_attr_info();
			pstAttr->set_uint32_attr_id(pInfo->iAttrID);
			pstAttr->set_uint32_attr_value(pInfo->iCurValue);
			iReadAttrCount++;
		}
	}
	else {
		// 字符串型监控点
		StrAttrNodeClient *pInfo = NULL;
		for(int iRead=0; m_wCmdContentLen >= iRead+sizeof(StrAttrNodeClient); )  
		{
			pInfo = (StrAttrNodeClient*)((char*)m_pstCmdContent+iRead);
			pInfo->iStrAttrId = ntohl(pInfo->iStrAttrId);
			pInfo->iStrVal = ntohl(pInfo->iStrVal);
			pInfo->iStrLen = ntohl(pInfo->iStrLen);
			if(iRead+sizeof(StrAttrNodeClient)+pInfo->iStrLen > m_wCmdContentLen) {
				REQERR_LOG("invalid packet from:%s, %d, %d, %d, %d", m_addrRemote.Convert(true).c_str(),
					iRead, (int)sizeof(StrAttrNodeClient), pInfo->iStrLen, m_wCmdContentLen);
				return ERR_INVALID_PACKET;
			}

			iRead += sizeof(StrAttrNodeClient)+pInfo->iStrLen;
			DEBUG_LOG("read cgi report strattr from:%s, id:%d, str:%s, value:%d, iRead:%d, len:%d",
				m_addrRemote.Convert().c_str(), pInfo->iStrAttrId, 
				pInfo->szStrInfo, pInfo->iStrVal, iRead, m_wCmdContentLen);
			pstAttr = stReport.add_msg_attr_info();
			pstAttr->set_uint32_attr_id(pInfo->iStrAttrId);
			pstAttr->set_uint32_attr_value(pInfo->iStrVal);
			pstAttr->set_str(pInfo->szStrInfo);
			iReadAttrCount++;
		}
	}

	if(iReadAttrCount > 0)
		DealReportAttr(stReport);
	INFO_LOG("write attr count: %d(%d), from cgi report:%s, from server:%d", 
		iReadAttrCount, (int)stReport.msg_attr_info_size(), m_addrRemote.Convert().c_str(), iReqMachineId);
	return NO_ERROR;
}

int32_t CUdpSock::OnRawDataClientAttr(const char *buf, size_t len)
{
	int iRet = 0;
	if(m_dwReqCmd == CMD_CGI_SEND_STR_ATTR || m_dwReqCmd == CMD_CGI_SEND_ATTR)
	{
		iRet = DealCgiReportAttr(buf, len);
		return AckToReq(iRet);
	}
	else if(NULL == m_pstBody ||
		(m_dwReqCmd != CMD_MONI_SEND_ATTR &&  m_dwReqCmd != CMD_MONI_SEND_STR_ATTR))
	{  
		REQERR_LOG("invalid packet cmd:%u, or pbody:%p", m_dwReqCmd, m_pstBody);
		return AckToReq(ERR_INVALID_PACKET);
	} 

	TWTlv *pTlv = GetWTlvByType2(TLV_MONI_COMM_INFO, m_pstBody);
	if(pTlv == NULL || ntohs(pTlv->wLen) != MYSIZEOF(TlvMoniCommInfo)) {
		REQERR_LOG("get tlv(%d) failed, or invalid len(%d-%u)",
			TLV_MONI_COMM_INFO, (pTlv!=NULL ? ntohs(pTlv->wLen) : 0), MYSIZEOF(TlvMoniCommInfo));
		return AckToReq(ERR_CHECK_TLV);
	}
	TlvMoniCommInfo *pctinfo = (TlvMoniCommInfo*)pTlv->sValue;
	int iMachineId = ntohl(pctinfo->iMachineId);

	DEBUG_LOG("get attr request from :%s - machine id:%d ", m_addrRemote.Convert(true).c_str(), iMachineId);

	// 获取会话 key
	m_pcltMachine = slog.GetMachineInfo(iMachineId, NULL);
	if(m_pcltMachine == NULL) {
		REQERR_LOG("find client machine:%d failed !", iMachineId);
		return AckToReq(ERR_INVALID_PACKET);
	}

	if((iRet=CheckSignature()) != NO_ERROR) {
		return AckToReq(iRet);
	}

	MonitorCommSig *pcommSig = (MonitorCommSig*)m_sDecryptBuf;

	// 是否启用了数据加密
	if(pcommSig->bEnableEncryptData)
	{
		static char sTmpBuf[MAX_ATTR_PKG_LENGTH+256];
		size_t iDecSigLen = 0;
		aes_decipher_data((const uint8_t*)m_pstCmdContent, m_wCmdContentLen,
			(uint8_t*)sTmpBuf, &iDecSigLen, (const uint8_t*)m_pcltMachine->sRandKey, AES_128);
		m_wCmdContentLen = (int)iDecSigLen;
		m_pstCmdContent = (void*)sTmpBuf;
	}

	DEBUG_LOG("get cmd content length:%d", m_wCmdContentLen);
	int iReadAttrCount = 0;
	::comm::ReportAttr stReport;
	::comm::AttrInfo *pstAttr = NULL;
	stReport.set_uint32_client_rep_time(time(NULL));
	stReport.set_bytes_report_ip(ipv4_addr_str(m_pcltMachine->ip1));
	stReport.set_report_host_id(m_pcltMachine->id);

	if(m_dwReqCmd == CMD_MONI_SEND_ATTR) {
		AttrNodeClient *pInfo= NULL;
		for(int iRead=0; m_wCmdContentLen >= iRead+sizeof(AttrNodeClient); )  
		{
			pInfo = (AttrNodeClient*)((char*)m_pstCmdContent+iRead);
			AttrInfoNtoH(pInfo);
			iRead += sizeof(AttrNodeClient);
			DEBUG_LOG("read client:%s report attr, id:%d, value:%d, iRead:%d, len:%d",
				m_addrRemote.Convert(true).c_str(), pInfo->iAttrID, pInfo->iCurValue,
				iRead, m_wCmdContentLen);
			pstAttr = stReport.add_msg_attr_info();
			pstAttr->set_uint32_attr_id(pInfo->iAttrID);
			pstAttr->set_uint32_attr_value(pInfo->iCurValue);
			iReadAttrCount++;
		}
	}
	else {
		// 字符串型监控点
		StrAttrNodeClient *pInfo = NULL;
		for(int iRead=0; m_wCmdContentLen >= iRead+sizeof(StrAttrNodeClient); )  
		{
			pInfo = (StrAttrNodeClient*)((char*)m_pstCmdContent+iRead);
			pInfo->iStrAttrId = ntohl(pInfo->iStrAttrId);
			pInfo->iStrVal = ntohl(pInfo->iStrVal);
			pInfo->iStrLen = ntohl(pInfo->iStrLen);
			if(iRead+sizeof(StrAttrNodeClient)+pInfo->iStrLen > m_wCmdContentLen) {
				REQERR_LOG("invalid packet from client %d, %d, %d, %d", 
					iRead, (int)sizeof(StrAttrNodeClient), pInfo->iStrLen, m_wCmdContentLen);
				return AckToReq(ERR_INVALID_PACKET);
			}

			iRead += sizeof(StrAttrNodeClient)+pInfo->iStrLen;
			DEBUG_LOG("read client:%s report str attr, id:%d, str:%s, value:%d, iRead:%d, len:%d",
				m_addrRemote.Convert(true).c_str(), pInfo->iStrAttrId, pInfo->szStrInfo, pInfo->iStrVal,
				iRead, m_wCmdContentLen);
			pstAttr = stReport.add_msg_attr_info();
			pstAttr->set_uint32_attr_id(pInfo->iStrAttrId);
			pstAttr->set_uint32_attr_value(pInfo->iStrVal);
			pstAttr->set_str(pInfo->szStrInfo);
			iReadAttrCount++;
		}
	}

	if(iReadAttrCount > 0)
		DealReportAttr(stReport);
	INFO_LOG("write attr count: %d(%d), from:%s", 
		iReadAttrCount, (int)stReport.msg_attr_info_size(), m_addrRemote.Convert(true).c_str());
	return AckToReq(NO_ERROR);
}

