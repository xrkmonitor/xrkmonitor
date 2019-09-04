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
