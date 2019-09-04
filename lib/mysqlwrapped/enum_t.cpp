/*
enum_t.cpp

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
#ifdef _WIN32
#pragma warning(disable:4786)
#endif

#include <string.h>
#include "enum_t.h"


#ifdef MYSQLW_NAMESPACE
namespace MYSQLW_NAMESPACE {
#endif


enum_t::enum_t(std::map<std::string, uint64_t>& ref) : m_mmap(ref), m_value(0)
{
	for (std::map<std::string, uint64_t>::iterator it = ref.begin(); it != ref.end(); it++)
	{
		std::string str = (*it).first;
		uint64_t value = (*it).second;
		m_vmap[value] = str;
	}
}


void enum_t::operator=(const std::string& str)
{
	m_value = m_mmap[str];
}


void enum_t::operator=(unsigned short s)
{
	m_value = s;
}


const std::string& enum_t::String()
{
	if (m_vmap.size() != m_mmap.size())
	{
		for (std::map<std::string, uint64_t>::iterator it = m_mmap.begin(); it != m_mmap.end(); it++)
		{
			std::string str = (*it).first;
			uint64_t value = (*it).second;
			m_vmap[value] = str;
		}
	}
	return m_vmap[m_value];
}


unsigned short enum_t::Value()
{
	return m_value;
}


bool enum_t::operator==(const std::string& str)
{
	if (!strcasecmp(c_str(), str.c_str()))
		return true;
	return false;
}


bool enum_t::operator==(unsigned short s)
{
	return m_value == s;
}


const char *enum_t::c_str()
{
	return String().c_str();
}


bool enum_t::operator!=(const std::string& str)
{
	if (!strcasecmp(c_str(), str.c_str()))
		return !true;
	return !false;
}


#ifdef MYSQLW_NAMESPACE
} // namespace MYSQLW_NAMESPACE {
#endif

