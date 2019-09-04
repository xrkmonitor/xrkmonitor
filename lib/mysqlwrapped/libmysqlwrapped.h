/**
 **	IError.h
 **
 **	Published / author: 2004-06-11 / grymse@alhem.net
 **/

/*
Copyright (C) 2004  Anders Hedstrom

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

#ifndef _IERROR_H
#define _IERROR_H

#include <string>

#ifdef MYSQLW_NAMESPACE
namespace MYSQLW_NAMESPACE {
#endif


class Database;
class Query;

/** Log class interface. */
class IError
{
public:
	virtual ~IError(){}; 
	virtual void error(Database&,const std::string&) = 0;
	virtual void debug(Database&,const std::string&) = 0;
	virtual void info(Database&,const std::string&) = 0;
	virtual void error(Database&,Query&,const std::string&) = 0;
};


#ifdef MYSQLW_NAMESPACE
} // namespace MYSQLW_NAMESPACE {
#endif

#endif // _IERROR_H
/*
enum_t.h

Copyright (C) 2004  Anders Hedstrom

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
#ifndef _ENUM_T_H
#define _ENUM_T_H

#include <string>
#include <map>
#ifdef WIN32
#include <config-win.h>
#include <mysql.h>
typedef unsigned __int64 uint64_t;
#define strcasecmp stricmp
#else
#include <stdint.h>
#endif

#ifdef MYSQLW_NAMESPACE
namespace MYSQLW_NAMESPACE {
#endif


/** Implements MySQL ENUM datatype. */
class enum_t
{
public:
	enum_t(std::map<std::string, uint64_t>& );

	const std::string& String();
	unsigned short Value();
	const char *c_str();

	void operator=(const std::string& );
	void operator=(unsigned short);
	bool operator==(const std::string& );
	bool operator==(unsigned short);
	bool operator!=(const std::string& );

private:
	std::map<std::string, uint64_t>& m_mmap;
	std::map<unsigned short, std::string> m_vmap;
	unsigned short m_value;

};


#ifdef MYSQLW_NAMESPACE
} // namespace MYSQLW_NAMESPACE {
#endif

#endif // _ENUM_T_H
/*
set_t.h

Copyright (C) 2004  Anders Hedstrom

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
#ifndef _SET_T_H
#define _SET_T_H

#include <string>
#include <map>
#ifdef WIN32
#include <config-win.h>
#include <mysql.h>
typedef unsigned __int64 uint64_t;
#else
#include <stdint.h>
#endif

#ifdef MYSQLW_NAMESPACE
namespace MYSQLW_NAMESPACE {
#endif


/** Implements MySQL SET datatype. */
class set_t
{
public:
	set_t(std::map<std::string, uint64_t>& );

	const std::string& String();
	uint64_t Value();
	const char *c_str();

	bool in_set(const std::string& );

	void operator=(const std::string& );
	void operator=(uint64_t);
	void operator|=(uint64_t);
	void operator&=(uint64_t);
	void operator+=(const std::string& );
	void operator-=(const std::string& );

private:
	std::map<std::string, uint64_t>& m_mmap;
	uint64_t m_value;
	std::string m_strvalue;

};


#ifdef MYSQLW_NAMESPACE
} // namespace MYSQLW_NAMESPACE {
#endif

#endif // _SET_T_H
/**
 **	Database.h
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

#ifndef _DATABASE_H
#define _DATABASE_H

#ifdef _WIN32
#include <windows.h>
#else
#include <pthread.h>
#endif
#include <string>
#include <list>
#ifdef WIN32
typedef unsigned __int64 uint64_t;
typedef __int64 int64_t;
#else
#include <stdint.h>
#include <mysql.h>
#endif

#ifdef MYSQLW_NAMESPACE
namespace MYSQLW_NAMESPACE {
#endif

class IError;
class Query;
class Mutex;


/** Connection information and pooling. */
class Database 
{
	/** Mutex container class, used by Lock. 
		\ingroup threading */
public:
	class Mutex {
	public:
		Mutex();
		~Mutex();
		void Lock();
		void Unlock();
	private:
#ifdef _WIN32
		HANDLE m_mutex;
#else
		pthread_mutex_t m_mutex;
#endif
	};
	/** Mutex helper class. */
private:
	class Lock {
	public:
		Lock(Mutex& mutex,bool use);
		~Lock();
	private:
		Mutex& m_mutex;
		bool m_b_use;
	};
public:
	/** Connection pool struct. */
	struct OPENDB {
		OPENDB() : busy(false) {}
		MYSQL mysql;
		bool busy;
	};
	typedef std::list<OPENDB *> opendb_v;

public:
	/** Use embedded libmysqld */
	Database(const std::string& database,
		IError * = NULL);

	/** Use embedded libmysqld + thread safe */
	Database(Mutex& ,const std::string& database,
		IError * = NULL);

	/** Connect to a MySQL server */
	Database(const std::string& host,
		const std::string& user,
		const std::string& password = "",
		const std::string& database = "",
		IError * = NULL, 
		int port=3306);

	/** Connect to a MySQL server + thread safe */
	Database(Mutex& ,const std::string& host,
		const std::string& user,
		const std::string& password = "",
		const std::string& database = "",
		IError * = NULL);

	virtual ~Database();

	/** Callback after mysql_init */
	virtual void OnMyInit(OPENDB *);

	/** try to establish connection with given host */
	bool Connected();
	bool TryReconnect(OPENDB *odb); // 尝试重新连接数据库，成功返回 true, 否则 false

	void RegErrHandler(IError *);
	void error(Query&,const char *format, ...);

	/** Request a database connection.
The "grabdb" method is used by the Query class, so that each object instance of Query gets a unique
database connection. I will reimplement your connection check logic in the Query class, as that's where
the database connection is really used.
It should be used something like this.
{
		Query q(db);
		if (!q.Connected())
			 return false;
		q.execute("delete * from user"); // well maybe not
}

When the Query object is deleted, then "freedb" is called - the database connection stays open in the
m_opendbs vector. New Query objects can then reuse old connections.
	*/
	OPENDB *grabdb();
	void freedb(OPENDB *odb);

	// utility
	std::string safestr(const std::string& );
	std::string unsafestr(const std::string& );
	std::string xmlsafestr(const std::string& );

	int64_t a2bigint(const std::string& );
	uint64_t a2ubigint(const std::string& );

	// add by rockdeng
	void debug(const char *format, ...);
	void info(const char *format, ...);

private:
	Database(const Database& ) : m_mutex(m_mutex) {}
	Database& operator=(const Database& ) { return *this; }
	void error(const char *format, ...);
	//
	std::string host;
	std::string user;
	std::string password;
	std::string database;
	int port; // add by rock
	opendb_v m_opendbs;
	IError *m_errhandler;
	bool m_embedded;
	Mutex& m_mutex;
	bool m_b_use_mutex;

public:
	static void SetConnTimeout(int sec) { s_iConnTimeoutSec = sec; }
	static void SetReadTimeout(int sec) { s_iReadTimeoutSec = sec; }
	static void SetWriteTimeout(int sec) { s_iWriteTimeoutSec = sec; }

private:
	static int s_iConnTimeoutSec;
	static int s_iReadTimeoutSec;
	static int s_iWriteTimeoutSec;
};

#ifdef MYSQLW_NAMESPACE
} //namespace MYSQLW_NAMESPACE {
#endif


#endif // _DATABASE_H
#ifdef _WIN32
#pragma warning(disable:4786)
#endif
/*
 **	Query.h
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

#ifndef _QUERY_H
#define _QUERY_H

#include <string>
#include <map>
#ifdef WIN32
typedef unsigned __int64 uint64_t;
typedef __int64 int64_t;
#else
#include <stdint.h>
#endif


#ifdef MYSQLW_NAMESPACE
namespace MYSQLW_NAMESPACE {
#endif

// 表记录状态值定义
#define TABLE_RECORD_STATUS_USE 0  // 记录正常使用
#define TABLE_RECORD_STATUS_DELETE 1 // 记录被删除，不可使用

/** SQL Statement execute / result set helper class. */
class Query 
{
public:
	/** Constructor accepting reference to database object. */
	Query(Database& dbin);
	/** Constructor accepting reference to database object
		and query to execute. */
	Query(Database& dbin,const std::string& sql);
	~Query();

	/** binary data **/
	int32_t SetBinaryData(char *pbuf, const char *pdata, int32_t iDataLen);
	MYSQL_RES *GetBinaryResult(const std::string& sql);
	int32_t ExecuteBinary(const char *sql, int32_t iSqlLen);

	/** Check to see if database object is connectable. */
	bool Connected();
	/** Return reference to database object. */
	Database& GetDatabase() const;
	MYSQL* GetMysql();
	/** Return string of last query executed. */
	const std::string& GetLastQuery();

	/** execute() returns true if query is successful,
		does not store result */
	bool execute(const std::string& sql);
	/** execute query and store result. */
	MYSQL_RES *get_result(const std::string& sql);

	/** free stored result, must be called after get_result() */
	void free_result();
	/** Fetch next result row.
		\return false if there was no row to fetch (end of rows) */
	MYSQL_ROW fetch_row();
	/** Get id of last insert. */
	my_ulonglong insert_id();
	/** Returns number of rows returned by last select call. */
	int num_rows();
	/** Number of columns in current result. */
	int num_cols();
	/** Last error string. */
	std::string GetError();
	/** Last error code. */
	int GetErrno();
	/** add by rockdeng -- get last affected rows */
	int affected_rows();

	/** Check if column x in current row is null. */
	bool is_null(const std::string& x);
	bool is_null(int x);
	bool is_null();

	/** rock add **/
	bool IsBusy() { return res != NULL; } 

	/** Execute query and return first result as a string. */
	const char *get_string(const std::string& sql);
	/** Execute query and return first result as a int integer. */
	int get_count(const std::string& sql);
	/** Execute query and return first result as a double. */
	double get_num(const std::string& sql);

	// add by rock for binary data
	unsigned long * fetch_lengths();
	unsigned long getlength(const std::string& x);
	unsigned long getlength(int x);

	const char *getstr(const std::string& x);
	const char *getstr(int x);
	const char *getstr();
	int getval(const std::string& x);
	int getval(int x);
	int getval();
	unsigned int getuval(const std::string& x);
	unsigned int getuval(int x);
	unsigned int getuval();
	int64_t getbigint(const std::string& x);
	int64_t getbigint(int x);
	int64_t getbigint();
	uint64_t getubigint(const std::string& x);
	uint64_t getubigint(int x);
	uint64_t getubigint();
	double getnum(const std::string& x);
	double getnum(int x);
	double getnum();

	std::string safestr(const std::string& x);

protected:
	Query(Database *dbin);
	Query(Database *dbin,const std::string& sql);
private:
	Query(const Query& q) : m_db(q.GetDatabase()) {}
	Query& operator=(const Query& ) { return *this; }
	void error(const std::string& );
	Database& m_db;
	Database::OPENDB *odb;
	MYSQL_RES *res;
	MYSQL_ROW row;
	short rowcount;
	std::string m_tmpstr;
	std::string m_last_query;
	std::map<std::string,int> m_nmap;
	int m_num_cols;

	// add by rock
	unsigned long *m_p_lengths;
};

// rockdeng -- add -- 复用 Query
class MyQuery
{
public:
	MyQuery(Query *qu, Database *db) {
		if(qu->IsBusy()) {
			m_qu = new Query(*db);
			m_bNew = true;
		}
		else {
			m_qu = qu;
			m_bNew = false;
		}
	}
	~MyQuery() {
		if(m_qu->IsBusy())
			m_qu->free_result();
		if(m_bNew) 
			delete m_qu;
	}
	Query & GetQuery() { return *m_qu; }

private:
	Query *m_qu;
	bool m_bNew;
};


#ifdef MYSQLW_NAMESPACE
} // namespace MYSQLW_NAMESPACE {
#endif

#endif // _QUERY_H
