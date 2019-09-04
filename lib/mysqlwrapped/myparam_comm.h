#ifndef __MYPARAM_COM_H__ 
#define __MYPARAM_COM_H__ 1 

#include <string>
#include <string.h>
#include "libmysqlwrapped.h"
#include "mysql.h"

typedef struct _IM_SQL_PARA
{
	const char* sName;
	const char* sValue;
	uint32_t iValue;
	const char* sOperater;
	struct _IM_SQL_PARA* pnext;
}IM_SQL_PARA;

#define PARAMER_VALUE_BUF 1024
#define PARAMER_COUNT_MAX 100

void InitParameter(IM_SQL_PARA** ppara);
int AddParameter(IM_SQL_PARA** ppara, const char* sName, const char* sValue, const char* sOperater);
int AddParameter(IM_SQL_PARA** ppara, const char* sName, uint32_t iValue, const char* sOperater);
int ReleaseParameter(IM_SQL_PARA** ppara); // should not use this, recommend use InitParameter
int JoinParameter_Insert(std::string *pszDest, MYSQL *m_pstDBlink, IM_SQL_PARA* para);
int JoinParameter_Set(std::string *pszDest, MYSQL *m_pstDBlink, IM_SQL_PARA* para);
int JoinParameter_Limit(std::string *pszDest, MYSQL *m_pstDBlink, IM_SQL_PARA* para);


// ---------------------------------------------
#define MYSQL_TABLE_UPDATED 1
#define MYSQL_TABLE_NOT_UPDATE 2
#define MYSQL_TABLE_SQL_ERROR 3

int IsTableUpdate(Query &qu, const char *szDatabase, const char* szTableName, uint32_t & dwUpdateTime);

#endif
