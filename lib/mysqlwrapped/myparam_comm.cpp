#include "myparam_comm.h"
#include <sv_str.h>
#include <stdio.h>
#include <stdarg.h>

static IM_SQL_PARA sParaComm[PARAMER_COUNT_MAX];
static int siParaUse = 0;
static uint32_t s_InitParaTimeSec = 0;

using std::string;
int InitParameter(IM_SQL_PARA** ppara)
{
	if(siParaUse != 0) {
		if(s_InitParaTimeSec+5 >= time(NULL)) {
		}
		else 
		    return -1;
	}
	siParaUse = 0;
	*ppara = NULL;
	s_InitParaTimeSec = time(NULL);
	return 0;
}

int AddParameter(IM_SQL_PARA** ppara, const char* sName, const char* sValue, const char* sOperater)
{
	if(strlen(sValue) >= PARAMER_VALUE_BUF/2 || siParaUse >= PARAMER_COUNT_MAX)
		return -1;

	IM_SQL_PARA* ptmp = sParaComm+siParaUse;
	siParaUse++;
	ptmp->sName = sName;
	ptmp->sValue = sValue;
	ptmp->iValue = 0;
	if(sOperater && !strcmp(sOperater, "DB_CAL"))
		ptmp->bOperater = PARAMER_Operater_INT;
	else
		ptmp->bOperater = PARAMER_Operater_STR;
	ptmp->pnext = *ppara;
	*ppara = ptmp;
	return 1;
}

int AddParameterStr(IM_SQL_PARA** ppara, const char* sName, const char* sValue)
{
	return AddParameter(ppara, sName, sValue, NULL);
}

int AddParameter(IM_SQL_PARA** ppara, const char* sName, uint32_t iValue, const char* sOperater)
{

	IM_SQL_PARA* ptmp = sParaComm+siParaUse;
	siParaUse++;
	ptmp->sName = sName;
	ptmp->sValue = "";
	ptmp->iValue = iValue;
	ptmp->bOperater = PARAMER_Operater_INT;
	ptmp->pnext = *ppara;
	*ppara = ptmp;
	return 1;
}

int AddParameterInt(IM_SQL_PARA** ppara, const char* sName, uint32_t iValue)
{
	return AddParameter(ppara, sName, iValue, "DB_CAL");
}

const char *GetParameterValue(IM_SQL_PARA *ptmp, MYSQL *m_pstDBlink)
{
	static char sValueBuf[PARAMER_VALUE_BUF+2];
	if(ptmp->bOperater == PARAMER_Operater_STR)
		mysql_real_escape_string(m_pstDBlink, sValueBuf, ptmp->sValue.c_str(), ptmp->sValue.size());
	else
		sprintf(sValueBuf, "%u", ptmp->iValue);
	return sValueBuf;
}

int ReleaseParameter(IM_SQL_PARA** ppara)
{
	siParaUse = 0;
	*ppara = NULL;
	return 1;
}

int JoinParameter_Insert(string *pszDest, MYSQL *m_pstDBlink, IM_SQL_PARA* para)
{
	IM_SQL_PARA* ptmp = para;
	*pszDest += " (";
	while (ptmp)
	{
		*pszDest += "`";
		*pszDest += ptmp->sName;
		*pszDest += "`";

		if (ptmp->pnext)
			*pszDest += ", ";
		else
		{
			*pszDest += ") values(";
			break;
		}
		ptmp = ptmp->pnext;
	}

	ptmp = para;
	while (ptmp)
	{
		if (ptmp->bOperater == PARAMER_Operater_STR) {
			*pszDest += "\"";
			*pszDest += GetParameterValue(ptmp, m_pstDBlink);
			*pszDest += "\"";
		}
		else
			*pszDest += GetParameterValue(ptmp, m_pstDBlink);

		if(ptmp->pnext)
			*pszDest += ", ";
		else
		{
			*pszDest += ") \n";
			break;
		}
		ptmp = ptmp->pnext;
	}
	return 1;
}

int JoinParameter_Set(string *pszDest, MYSQL *m_pstDBlink, IM_SQL_PARA* para)
{
	IM_SQL_PARA* ptmp = para;
	*pszDest += " ";
	while (ptmp)
	{
		if (ptmp->pnext)
		{
			*pszDest += ptmp->sName;
			if (ptmp->bOperater == PARAMER_Operater_INT)
				*pszDest += "=";
			else
				*pszDest += "=\"";

			*pszDest += GetParameterValue(ptmp, m_pstDBlink);

			if (ptmp->bOperater == PARAMER_Operater_INT)
				*pszDest += ", ";
			else
				*pszDest += "\",";
		}
		else
		{
			*pszDest += ptmp->sName;
			if (ptmp->bOperater == PARAMER_Operater_INT)
				*pszDest += "=";
			else
				*pszDest += "=\"";

			*pszDest += GetParameterValue(ptmp, m_pstDBlink);

			if (ptmp->bOperater == PARAMER_Operater_INT)
				*pszDest += "";
			else
				*pszDest += "\"";
		}
		ptmp = ptmp->pnext;
	}
	return 1;
}

int JoinParameter_Limit(string *pszDest, MYSQL *m_pstDBlink, IM_SQL_PARA* para)
{
        IM_SQL_PARA* ptmp = para;
        while (ptmp)
        {
                if (ptmp->pnext)
                {
                        *pszDest += ptmp->sName;
						if (ptmp->bOperater == PARAMER_Operater_INT)
                                *pszDest += "=";
                        else
                                *pszDest += "=\"";

						*pszDest += GetParameterValue(ptmp, m_pstDBlink);

						if (ptmp->bOperater == PARAMER_Operater_INT)
                                *pszDest += " and ";
                        else
                                *pszDest += "\" and ";
                }
                else
                {
                        *pszDest += ptmp->sName;
						if (ptmp->bOperater == PARAMER_Operater_INT)
                                *pszDest += "=";
                        else
                                *pszDest += "=\"";

						*pszDest += GetParameterValue(ptmp, m_pstDBlink);

						if (ptmp->bOperater == PARAMER_Operater_INT)
                                *pszDest += "";
                        else
                                *pszDest += "\"";
                }
                ptmp = ptmp->pnext;
        }
        return 1;
}

int IsTableUpdate(Query &qu, const char *szDatabase, const char* szTableName, uint32_t & dwUpdateTime)
{
	char sBuf[256];
	snprintf(sBuf, sizeof(sBuf),
		"select update_time from tables where table_schema=\'%s\' and table_name=\'%s\'",
		szDatabase, szTableName);
	if(qu.get_result(sBuf) && qu.fetch_row())
	{
		const char *ptime = qu.getstr(0);
		uint32_t dwTableUpdateTime = datetoui(ptime); 
		if(dwTableUpdateTime != dwUpdateTime)
		{
			dwUpdateTime = dwTableUpdateTime;
			qu.free_result();
			return MYSQL_TABLE_UPDATED;
		}
		qu.free_result();
		return MYSQL_TABLE_NOT_UPDATE;
	}
	qu.free_result();
	return MYSQL_TABLE_SQL_ERROR;
}

