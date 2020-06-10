#ifndef __MYPARAM_COM_H__ 
#define __MYPARAM_COM_H__ 1 

#include <string>
#include <string.h>
#include "libmysqlwrapped.h"
#include "mysql.h"

typedef struct _IM_SQL_PARA
{
	std::string sName;
	std::string sValue;
	uint32_t iValue;
	char bOperater;
	struct _IM_SQL_PARA* pnext;
}IM_SQL_PARA;

#define PARAMER_Operater_STR 1
#define PARAMER_Operater_INT 2

#define PARAMER_VALUE_BUF 1024*1024
#define PARAMER_COUNT_MAX 100

const char *GetParameterValue(IM_SQL_PARA *ptmp, MYSQL *m_pstDBlink);
int InitParameter(IM_SQL_PARA** ppara);
int AddParameter(IM_SQL_PARA** ppara, const char* sName, const char* sValue, const char* sOperater);
int AddParameter(IM_SQL_PARA** ppara, const char* sName, uint32_t iValue, const char* sOperater);
int AddParameterStr(IM_SQL_PARA** ppara, const char* sName, const char* sValue);
int AddParameterInt(IM_SQL_PARA** ppara, const char* sName, uint32_t iValue);
int ReleaseParameter(IM_SQL_PARA** ppara);
int JoinParameter_Insert(std::string *pszDest, MYSQL *m_pstDBlink, IM_SQL_PARA* para);
int JoinParameter_Set(std::string *pszDest, MYSQL *m_pstDBlink, IM_SQL_PARA* para);
int JoinParameter_Limit(std::string *pszDest, MYSQL *m_pstDBlink, IM_SQL_PARA* para);


// ---------------------------------------------
#define MYSQL_TABLE_UPDATED 1
#define MYSQL_TABLE_NOT_UPDATE 2
#define MYSQL_TABLE_SQL_ERROR 3

int IsTableUpdate(Query &qu, const char *szDatabase, const char* szTableName, uint32_t & dwUpdateTime);

#endif
