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
