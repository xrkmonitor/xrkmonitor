/**
 **	Database.cpp
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
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/
#include <stdio.h>
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
#include <stdarg.h>
#endif

#include <sv_time.h>
#include <mt_report.h>

#include "IError.h"
#include "Database.h"


#ifdef MYSQLW_NAMESPACE
namespace MYSQLW_NAMESPACE {
#endif

// 因为内部重试原因，实际超时时间要 * 3
int Database::s_iConnTimeoutSec = 3;
int Database::s_iReadTimeoutSec = 10; 
int Database::s_iWriteTimeoutSec = 15; 

static char s_ErrMsgBuf[4096];

Database::Database(const std::string& d,IError *e)
:database(d)
,m_errhandler(e)
,m_embedded(true)
,m_mutex(m_mutex)
,m_b_use_mutex(false)
{
}


Database::Database(Mutex& m,const std::string& d,IError *e)
:database(d)
,port(3306)
,m_errhandler(e)
,m_embedded(true)
,m_mutex(m)
,m_b_use_mutex(true)
{
}


Database::Database(const std::string& h,const std::string& u,const std::string& p,const std::string& d,IError *e, int pt)
:host(h)
,user(u)
,password(p)
,database(d)
,port(pt)
,m_errhandler(e)
,m_embedded(false)
,m_mutex(m_mutex)
,m_b_use_mutex(false)
{
}


Database::Database(Mutex& m,const std::string& h,const std::string& u,const std::string& p,const std::string& d,IError *e)
:host(h)
,user(u)
,password(p)
,database(d)
,port(3306)
,m_errhandler(e)
,m_embedded(false)
,m_mutex(m)
,m_b_use_mutex(true)
{
}


Database::~Database()
{
	for (opendb_v::iterator it = m_opendbs.begin(); it != m_opendbs.end(); it++)
	{
		OPENDB *p = *it;
		mysql_close(&p -> mysql);
	}
	while (m_opendbs.size())
	{
		opendb_v::iterator it = m_opendbs.begin();
		OPENDB *p = *it;
		if (p -> busy)
		{
			error("destroying Database object before Query object");
		}
		delete p;
		m_opendbs.erase(it);
	}
}


void Database::OnMyInit(OPENDB *odb)
{
	// 设置超时时间
	mysql_options(&odb -> mysql, MYSQL_OPT_CONNECT_TIMEOUT, (const char *)&s_iConnTimeoutSec);
	mysql_options(&odb -> mysql, MYSQL_OPT_READ_TIMEOUT, (const char*)&s_iReadTimeoutSec);
	mysql_options(&odb -> mysql, MYSQL_OPT_WRITE_TIMEOUT, (const char*)&s_iWriteTimeoutSec);

	info("set mysql timeout info - %d, %d, %d", s_iConnTimeoutSec, s_iReadTimeoutSec, s_iWriteTimeoutSec);

	// using embedded server (libmysqld)
	if (m_embedded)
	{
		mysql_options(&odb -> mysql, MYSQL_READ_DEFAULT_GROUP, "test_libmysqld_CLIENT");
	}
}


void Database::RegErrHandler(IError *p)
{
	m_errhandler = p;
}

bool Database::TryReconnect(OPENDB * odb)
{
	GetTimeDiffMs(1);
	int tmDiff = 0;
	if (m_embedded)
	{
		if (!mysql_real_connect(&odb -> mysql,NULL,NULL,NULL,database.c_str(),port,NULL,0) )
		{
			error("mysql_real_connect(NULL,NULL,NULL,%s,%d,NULL,0) failed - list size %d",database.c_str(),port,m_opendbs.size());
			MtReport_Attr_Add(116, 1);
			return false;
		}
		tmDiff = GetTimeDiffMs(0);
		info("connect to db:%s:%d, use time:%d ms", database.c_str(), port, tmDiff);
	}
	else
	{
		if (!mysql_real_connect(&odb -> mysql,host.c_str(),user.c_str(),password.c_str(),database.c_str(),port,NULL,0) )
		{
			error("mysql_real_connect(%s,%s,***,%s,%d,NULL,0) failed - list size %d",host.c_str(),user.c_str(),database.c_str(),port,m_opendbs.size());
			MtReport_Attr_Add(116, 1);
			return false;
		}
		tmDiff = GetTimeDiffMs(0);
		info("connect to db:%s:%d, use time:%d ms", host.c_str(), port, tmDiff);
	}

	// fix 中文乱码
	mysql_query(&odb -> mysql, "set names latin1;");

	if(tmDiff >= 0 && tmDiff < 10)
		MtReport_Attr_Add(117, 1);
	else if(tmDiff >= 10 && tmDiff < 20)
		MtReport_Attr_Add(118, 1);
	else if(tmDiff >= 20 && tmDiff < 50)
		MtReport_Attr_Add(119, 1);
	else if(tmDiff >= 50 && tmDiff < 100)
		MtReport_Attr_Add(120, 1);
	else if(tmDiff >= 100 && tmDiff < 200)
		MtReport_Attr_Add(130, 1);
	else if(tmDiff >= 200 && tmDiff < 500)
		MtReport_Attr_Add(131, 1);
	else if(tmDiff >= 500 && tmDiff < 1000)
		MtReport_Attr_Add(132, 1);
	else if(tmDiff >= 1000)
		MtReport_Attr_Add(121, 1);
	return true;
}

Database::OPENDB *Database::grabdb()
{
	Lock lck(m_mutex, m_b_use_mutex);
	OPENDB *odb = NULL;

	for (opendb_v::iterator it = m_opendbs.begin(); it != m_opendbs.end(); it++)
	{
		odb = *it;
		if (!odb -> busy)
		{
			break;
		}
		else
		{
			odb = NULL;
		}
	}
	if (!odb)
	{
		odb = new OPENDB;
		if (!odb)
		{
			error("grabdb: OPENDB struct couldn't be created");
			return NULL;
		}
		if (!mysql_init(&odb -> mysql))
		{
			error("mysql_init() failed - list size %d",m_opendbs.size());
			delete odb;
			return NULL;
		}
		// use callback to set mysql_options() before connect, etc
		this -> OnMyInit(odb);
		if(!TryReconnect(odb))
		{
			delete odb;
			return NULL;
		}
		odb -> busy = true;
		m_opendbs.push_back(odb);
	}
	else
	{
		if (mysql_ping(&odb -> mysql))
		{
			error("mysql_ping() failed when reusing an old connection from the connection pool");
			TryReconnect(odb);
		}
		odb -> busy = true;
	}
	return odb;
}


void Database::freedb(Database::OPENDB *odb)
{
	Lock lck(m_mutex, m_b_use_mutex);
	if (odb)
	{
		odb -> busy = false;
	}
}

void Database::debug(const char *format, ...)
{
	if (m_errhandler)
	{
		va_list ap;
		char str[1024] = {0};
		va_start(ap, format);
		vsnprintf(str, sizeof(str)-1, format, ap);
		va_end(ap);
		m_errhandler -> debug(*this, str);
	}
}

void Database::info(const char *format, ...)
{
	if (m_errhandler)
	{
		va_list ap;
		va_start(ap, format);
		vsnprintf(s_ErrMsgBuf, sizeof(s_ErrMsgBuf)-1, format, ap);
		va_end(ap);
		m_errhandler -> info(*this, s_ErrMsgBuf);
	}
}

void Database::error(const char *format, ...)
{
	if (m_errhandler)
	{
		va_list ap;
		va_start(ap, format);
		vsnprintf(s_ErrMsgBuf, sizeof(s_ErrMsgBuf)-1, format, ap);
		va_end(ap);
		m_errhandler -> error(*this, s_ErrMsgBuf);
	}
}

void Database::error(Query& q,const char *format, ...)
{
	if (m_errhandler)
	{
		va_list ap;
		va_start(ap, format);
		vsnprintf(s_ErrMsgBuf, sizeof(s_ErrMsgBuf)-1, format, ap);
		va_end(ap);
		m_errhandler -> error(*this, q, s_ErrMsgBuf);
	}
}


bool Database::Connected()
{
	OPENDB *odb = grabdb();
	if (!odb)
	{
		return false;
	}
	int ping_result = mysql_ping(&odb -> mysql);
	if (ping_result)
	{
		error("mysql_ping() failed, try reconnect");
		if(TryReconnect(odb))
		{
			ping_result = false;
			debug("reconnect db ok ..");
		}
	}
	freedb(odb);
	return ping_result ? false : true;
}


Database::Lock::Lock(Mutex& mutex,bool use) : m_mutex(mutex),m_b_use(use) 
{
	if (m_b_use) 
	{
		m_mutex.Lock();
	}
}


Database::Lock::~Lock() 
{
	if (m_b_use) 
	{
		m_mutex.Unlock();
	}
}


Database::Mutex::Mutex()
{
#ifdef _WIN32
	m_mutex = ::CreateMutex(NULL, FALSE, NULL);
#else
	pthread_mutex_init(&m_mutex, NULL);
#endif
}


Database::Mutex::~Mutex()
{
#ifdef _WIN32
	::CloseHandle(m_mutex);
#else
	pthread_mutex_destroy(&m_mutex);
#endif
}


void Database::Mutex::Lock()
{
#ifdef _WIN32
	DWORD d = WaitForSingleObject(m_mutex, INFINITE);
	// %! check 'd' for result
#else
	pthread_mutex_lock(&m_mutex);
#endif
}


void Database::Mutex::Unlock()
{
#ifdef _WIN32
	::ReleaseMutex(m_mutex);
#else
	pthread_mutex_unlock(&m_mutex);
#endif
}


std::string Database::safestr(const std::string& str)
{
	std::string str2;
	for (size_t i = 0; i < str.size(); i++)
	{
		switch (str[i])
		{
		case '\'':
		case '\\':
		case 34:
			str2 += '\\';
		default:
			str2 += str[i];
		}
	}
	return str2;
}


std::string Database::unsafestr(const std::string& str)
{
	std::string str2;
	for (size_t i = 0; i < str.size(); i++)
	{
		if (str[i] == '\\')
		{
			i++;
		}
		if (i < str.size())
		{
			str2 += str[i];
		}
	}
	return str2;
}


std::string Database::xmlsafestr(const std::string& str)
{
	std::string str2;
	for (size_t i = 0; i < str.size(); i++)
	{
		switch (str[i])
		{
		case '&':
			str2 += "&amp;";
			break;
		case '<':
			str2 += "&lt;";
			break;
		case '>':
			str2 += "&gt;";
			break;
		case '"':
			str2 += "&quot;";
			break;
		case '\'':
			str2 += "&apos;";
			break;
		default:
			str2 += str[i];
		}
	}
	return str2;
}


int64_t Database::a2bigint(const std::string& str)
{
	int64_t val = 0;
	bool sign = false;
	size_t i = 0;
	if (str[i] == '-')
	{
		sign = true;
		i++;
	}
	for (; i < str.size(); i++)
	{
		val = val * 10 + (str[i] - 48);
	}
	return sign ? -val : val;
}


uint64_t Database::a2ubigint(const std::string& str)
{
	uint64_t val = 0;
	for (size_t i = 0; i < str.size(); i++)
	{
		val = val * 10 + (str[i] - 48);
	}
	return val;
}


#ifdef MYSQLW_NAMESPACE
} // namespace MYSQLW_NAMESPACE {
#endif

