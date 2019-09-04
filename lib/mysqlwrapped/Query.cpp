/**
 **	Query.cpp
 **
 **	Published / author: 2001-02-15 / grymse@alhem.net
 **/

/*
Copyright (C) 2001  Anders Hedstrom

This program is made available under the terms of the GNU GPL.

If you would like to use this program in a closed-source application,
a separate license agreement is available. For information about 
the closed-source license agreement for this program, please
visit http://www.alhem.net/sqlwrapped/license.html and/or
email license@alhem.net.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
aint with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#ifdef _WIN32
#pragma warning(disable:4786)
#endif

#include <string>
#include <map>
#ifdef WIN32
#include <config-win.h>
#include <mysql.h>
#else
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mysql/mysql.h>
#endif

#include "mt_report.h"
#include "sv_time.h"
#include "Database.h"
#include "Query.h"

#ifdef MYSQLW_NAMESPACE
namespace MYSQLW_NAMESPACE {
#endif


Query::Query(Database *dbin)
:m_db(*dbin)
,odb(dbin ? dbin -> grabdb() : NULL)
,res(NULL)
,row(NULL)
,m_num_cols(0)
{
	// add by rock
	m_p_lengths = NULL;
}


Query::Query(Database& dbin) : m_db(dbin),odb(dbin.grabdb()),res(NULL),row(NULL)
,m_num_cols(0)
{
	// add by rock
	m_p_lengths = NULL;
}


Query::Query(Database *dbin,const std::string& sql) : m_db(*dbin)
,odb(dbin ? dbin -> grabdb() : NULL),res(NULL),row(NULL)
,m_num_cols(0)
{
	execute(sql);

	// add by rock
	m_p_lengths = NULL;
}


Query::Query(Database& dbin,const std::string& sql) : m_db(dbin),odb(dbin.grabdb()),res(NULL),row(NULL)
,m_num_cols(0)
{
	execute(sql); // returns 0 if fail
	// add by rock
	m_p_lengths = NULL;
}


Query::~Query()
{
	if (res)
	{
		GetDatabase().error(*this, "mysql_free_result in destructor");
		mysql_free_result(res);
	}
	if (odb)
	{
		m_db.freedb(odb);
	}
}


Database& Query::GetDatabase() const
{
	return m_db;
}

MYSQL* Query::GetMysql()
{
	if(odb)
		return &odb -> mysql;
	return NULL;
}

bool Query::execute(const std::string& sql)
{		// query, no result
	m_last_query = sql;
	if (odb && res)
	{
		GetDatabase().error(*this, "execute: query busy, sql:%s", sql.c_str());
		free_result();
	}
	if (odb && !res)
	{
		GetTimeDiffMs(1);
		if (mysql_query(&odb -> mysql,sql.c_str()))
		{
			GetDatabase().error(*this, "sql:%s, execut failed:%s", sql.c_str(), mysql_error(&odb -> mysql));
		}
		else
		{
			int tmDiff = GetTimeDiffMs(0);
			if(tmDiff >= 0 && tmDiff < 20)
				MtReport_Attr_Add(122, 1);
			else if(tmDiff >= 20 && tmDiff < 50)
				MtReport_Attr_Add(123, 1);
			else if(tmDiff >= 50 && tmDiff < 100)
				MtReport_Attr_Add(124, 1);
			else if(tmDiff >= 100 && tmDiff < 200)
				MtReport_Attr_Add(125, 1);
			else if(tmDiff >= 200 && tmDiff < 500)
				MtReport_Attr_Add(126, 1);
			else if(tmDiff >= 500 && tmDiff < 1000)
				MtReport_Attr_Add(127, 1);
			else if(tmDiff >= 1000 && tmDiff < 2000)
				MtReport_Attr_Add(128, 1);
			else if(tmDiff >= 2000)
				MtReport_Attr_Add(129, 1);
			GetDatabase().debug("execute sql:%s , use time:%d ms", sql.c_str(), tmDiff);
			return true;
		}
	}
	return false;
}



// methods using db specific api calls

int32_t Query::SetBinaryData(char *pbuf, const char *pdata, int32_t iDataLen)
{
	if (odb && res)
	{
		GetDatabase().error(*this, "execute: query busy");
		free_result();
	}

	char *psave = pbuf;
	*pbuf = '\'';
	pbuf++;
	int32_t iWrite = mysql_real_escape_string(&odb -> mysql, pbuf, pdata, iDataLen);
	if(iWrite < iDataLen)
	{
		GetDatabase().error(*this, "SetBinaryData failed-%d|%d, msg:%s", 
			iDataLen, iWrite,  mysql_error(&odb -> mysql));
		return -1;
	}
	pbuf += iWrite;
	*pbuf = '\'';
	pbuf++;
	return (int32_t)(pbuf-psave);
}

int32_t Query::ExecuteBinary(const char *sql, int32_t iSqlLen)
{
	if (odb && res)
	{
		GetDatabase().error(*this, "execute: query busy");
		free_result();
	}
	if (odb && !res)
	{
		GetTimeDiffMs(1);
		if (mysql_real_query(&odb -> mysql, sql, iSqlLen))
		{
			GetDatabase().error(*this, "ExecuteBinary query failed:%s", mysql_error(&odb -> mysql));
			return -2;
		}
		else
		{
			int tmDiff = GetTimeDiffMs(0);
			if(tmDiff >= 0 && tmDiff < 20)
				MtReport_Attr_Add(122, 1);
			else if(tmDiff >= 20 && tmDiff < 50)
				MtReport_Attr_Add(123, 1);
			else if(tmDiff >= 50 && tmDiff < 100)
				MtReport_Attr_Add(124, 1);
			else if(tmDiff >= 100 && tmDiff < 200)
				MtReport_Attr_Add(125, 1);
			else if(tmDiff >= 200 && tmDiff < 500)
				MtReport_Attr_Add(126, 1);
			else if(tmDiff >= 500 && tmDiff < 1000)
				MtReport_Attr_Add(127, 1);
			else if(tmDiff >= 1000 && tmDiff < 2000)
				MtReport_Attr_Add(128, 1);
			else if(tmDiff >= 2000)
				MtReport_Attr_Add(129, 1);
			GetDatabase().debug("ExecuteBinary query, use time:%d ms", tmDiff);
		}
		return 0;
	}
	return -1;
}

MYSQL_RES *Query::GetBinaryResult(const std::string& sql)
{	// query, result
	if (odb && res)
	{
		GetDatabase().error(*this, "get_result: query busy");
		free_result();
	}
	if (odb && !res)
	{
		if (ExecuteBinary(sql.c_str(), sql.size()) >= 0)
		{
			GetTimeDiffMs(1);
			res = mysql_store_result(&odb -> mysql);
			if (res)
			{
				MYSQL_FIELD *f = mysql_fetch_field(res);
				int i = 1;
				while (f)
				{
					if (f -> name)
						m_nmap[f -> name] = i;
					f = mysql_fetch_field(res);
					i++;
				}
				m_num_cols = i - 1;
			}
		}
	}
	return res;
}

MYSQL_RES *Query::get_result(const std::string& sql)
{	// query, result
	if (odb && res)
	{
		GetDatabase().error(*this, "get_result: query busy");

		// add by rockdeng --- 2019-02-12
		free_result();
	}
	if (odb && !res)
	{
		if (execute(sql))
		{
			GetTimeDiffMs(1);
			res = mysql_store_result(&odb -> mysql);
			if (res)
			{
				MYSQL_FIELD *f = mysql_fetch_field(res);
				int i = 1;
				while (f)
				{
					if (f -> name)
						m_nmap[f -> name] = i;
					f = mysql_fetch_field(res);
					i++;
				}
				m_num_cols = i - 1;
			}
		}
		else {
			GetDatabase().error(*this, "execute: %s failed", sql.c_str());
		}
	}
	return res;
}


void Query::free_result()
{
	if (odb && res)
	{
		mysql_free_result(res);
		res = NULL;
		row = NULL;

		int tmDiff = GetTimeDiffMs(0);
		if(tmDiff >= 0 && tmDiff < 20)
			MtReport_Attr_Add(133, 1);
		else if(tmDiff >= 20 && tmDiff < 50)
			MtReport_Attr_Add(134, 1);
		else if(tmDiff >= 50 && tmDiff < 100)
			MtReport_Attr_Add(135, 1);
		else if(tmDiff >= 100 && tmDiff < 200)
			MtReport_Attr_Add(136, 1);
		else if(tmDiff >= 200 && tmDiff < 500)
			MtReport_Attr_Add(137, 1);
		else if(tmDiff >= 500 && tmDiff < 1000)
			MtReport_Attr_Add(138, 1);
		else if(tmDiff >= 1000)
			MtReport_Attr_Add(139, 1);
	}
	while (m_nmap.size())
	{
		std::map<std::string,int>::iterator it = m_nmap.begin();
		m_nmap.erase(it);
	}
	m_num_cols = 0;

	// add by rock
	m_p_lengths = NULL;
}


MYSQL_ROW Query::fetch_row()
{
	rowcount = 0;
	return odb && res ? row = mysql_fetch_row(res) : NULL;
}

unsigned long * Query::fetch_lengths()
{
	odb && res ?  m_p_lengths = mysql_fetch_lengths(res) : NULL;
	return m_p_lengths;
}

int Query::affected_rows()
{
	if(odb)
		return mysql_affected_rows(&odb -> mysql);
	return 0;
}

my_ulonglong Query::insert_id()
{
	if (odb)
	{
		return mysql_insert_id(&odb -> mysql);
	}
	else
	{
		return 0;
	}
}


int Query::num_rows()
{
	return odb && res ? mysql_num_rows(res) : 0;
}


int Query::num_cols()
{
	return m_num_cols;
}


bool Query::is_null(int x)
{
	if (odb && res && row)
	{
		return row[x] ? false : true;
	}
	return false; // ...
}


bool Query::is_null(const std::string& x)
{
	int index = m_nmap[x] - 1;
	if (index >= 0)
		return is_null(index);
	error("Column name lookup failure: " + x);
	return false;
}


bool Query::is_null()
{
	return is_null(rowcount++);
}


unsigned long Query::getlength(const std::string& x)
{
	int index = m_nmap[x] - 1;
	if (index >= 0)
		return getlength(index);
	error("Column name lookup failure: " + x);
	return 0;
}

unsigned long Query::getlength(int x)
{
	if (odb && res && m_p_lengths)
	{
		return m_p_lengths[x];
	}
	return 0;
}

const char *Query::getstr(const std::string& x)
{
	int index = m_nmap[x] - 1;
	if (index >= 0)
		return getstr(index);
	error("Column name lookup failure: " + x);
	return NULL;
}


const char *Query::getstr(int x)
{
	if (odb && res && row)
	{
		return row[x] ? row[x] : "";
	}
	return NULL;
}


const char *Query::getstr()
{
	return getstr(rowcount++);
}


double Query::getnum(const std::string& x)
{
	int index = m_nmap[x] - 1;
	if (index >= 0)
		return getnum(index);
	error("Column name lookup failure: " + x);
	return 0;
}


double Query::getnum(int x)
{
	return odb && res && row && row[x] ? atof(row[x]) : 0;
}


int Query::getval(const std::string& x)
{
	int index = m_nmap[x] - 1;
	if (index >= 0)
		return getval(index);
	error("Column name lookup failure: " + x);
	return 0;
}


int Query::getval(int x)
{
	return odb && res && row && row[x] ? atol(row[x]) : 0;
}


double Query::getnum()
{
	return getnum(rowcount++);
}


int Query::getval()
{
	return getval(rowcount++);
}


unsigned int Query::getuval(const std::string& x)
{
	int index = m_nmap[x] - 1;
	if (index >= 0)
		return getuval(index);
	error("Column name lookup failure: " + x);
	return 0;
}


unsigned int Query::getuval(int x)
{
	unsigned int l = 0;
	if (odb && res && row && row[x])
	{
		l = m_db.a2ubigint(row[x]);
	}
	return l;
}


unsigned int Query::getuval()
{
	return getuval(rowcount++);
}


int64_t Query::getbigint(const std::string& x)
{
	int index = m_nmap[x] - 1;
	if (index >= 0)
		return getbigint(index);
	error("Column name lookup failure: " + x);
	return 0;
}


int64_t Query::getbigint(int x)
{
	return odb && res && row && row[x] ? m_db.a2bigint(row[x]) : 0;
}


int64_t Query::getbigint()
{
	return getbigint(rowcount++);
}


uint64_t Query::getubigint(const std::string& x)
{
	int index = m_nmap[x] - 1;
	if (index >= 0)
		return getubigint(index);
	error("Column name lookup failure: " + x);
	return 0;
}


uint64_t Query::getubigint(int x)
{
	return odb && res && row && row[x] ? m_db.a2ubigint(row[x]) : 0;
}


uint64_t Query::getubigint()
{
	return getubigint(rowcount++);
}


double Query::get_num(const std::string& sql)
{
	double l = 0;
	if (get_result(sql))
	{
		if (fetch_row())
		{
			l = getnum();
		}
		free_result();
	}
	return l;
}


int Query::get_count(const std::string& sql)
{
	int l = 0;
	if (get_result(sql))
	{
		if (fetch_row())
			l = getval();
		free_result();
	}
	return l;
}


const char *Query::get_string(const std::string& sql)
{
	bool found = false;
	m_tmpstr = "";
	if (get_result(sql))
	{
		if (fetch_row())
		{
			m_tmpstr = getstr();
			found = true;
		}
		free_result();
	}
	return m_tmpstr.c_str(); // %! changed from 1.0 which didn't return NULL on failed query
}


const std::string& Query::GetLastQuery()
{
	return m_last_query;
}


std::string Query::GetError()
{
	return odb ? mysql_error(&odb -> mysql) : "";
}


int Query::GetErrno()
{
	return odb ? mysql_errno(&odb -> mysql) : 0;
}


bool Query::Connected()
{
	if (odb)
	{
		if (mysql_ping(&odb -> mysql))
		{
			GetDatabase().error(*this, "mysql_ping() failed, msg:%s", GetError().c_str());
			return false;
		}
	}
	return odb ? true : false;
}


void Query::error(const std::string& x)
{
	m_db.error(*this, x.c_str());
}


std::string Query::safestr(const std::string& x)
{
	return m_db.safestr(x);
}


#ifdef MYSQLW_NAMESPACE
} // namespace MYSQLW_NAMESPACE {
#endif

