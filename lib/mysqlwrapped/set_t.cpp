/*
set_t.cpp

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

#include "set_t.h"


#ifdef MYSQLW_NAMESPACE
namespace MYSQLW_NAMESPACE {
#endif


set_t::set_t(std::map<std::string, uint64_t>& ref) : m_mmap(ref), m_value(0)
{
}


void set_t::operator=(const std::string& str)
{
	size_t x = 0;
	size_t i;
	m_value = 0;
	for (i = 0; i < str.size(); i++)
	{
		if (str[i] == ',')
		{
			m_value |= m_mmap[str.substr(x,i - x)];
			x = i + 1;
		}
	}
	m_value |= m_mmap[str.substr(x,i - x)];
}


void set_t::operator=(uint64_t s)
{
	m_value = s;
}


void set_t::operator|=(uint64_t s)
{
	m_value |= s;
}


void set_t::operator&=(uint64_t s)
{
	m_value &= s;
}


const std::string& set_t::String()
{
	std::string str;
	for (std::map<std::string, uint64_t>::iterator it = m_mmap.begin(); it != m_mmap.end(); it++)
	{
		std::string tmp = (*it).first;
		uint64_t bit = (*it).second;
		if (m_value & bit)
		{
			if (str.size())
				str += ",";
			str += tmp;
		}
	}
	m_strvalue = str;
	return m_strvalue;
}


uint64_t set_t::Value()
{
	return m_value;
}


bool set_t::in_set(const std::string& str)
{
	if (m_value & m_mmap[str])
		return true;
	return false;
}


const char *set_t::c_str()
{
	return String().c_str();
}


void set_t::operator+=(const std::string& str)
{
	size_t x = 0;
	size_t i;
	for (i = 0; i < str.size(); i++)
	{
		if (str[i] == ',')
		{
			m_value |= m_mmap[str.substr(x,i - x)];
			x = i + 1;
		}
	}
	m_value |= m_mmap[str.substr(x,i - x)];
}


void set_t::operator-=(const std::string& str)
{
	size_t x = 0;
	size_t i;
	for (i = 0; i < str.size(); i++)
	{
		if (str[i] == ',')
		{
			m_value &= ~m_mmap[str.substr(x,i - x)];
			x = i + 1;
		}
	}
	m_value &= ~m_mmap[str.substr(x,i - x)];
}


#ifdef MYSQLW_NAMESPACE
} // namespace MYSQLW_NAMESPACE {
#endif

