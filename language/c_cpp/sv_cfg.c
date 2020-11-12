/*** xrkmonitor license ***

   Copyright (c) 2019 by rockdeng

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.


   字符云监控(xrkmonitor) 开源版 (c) 2019 by rockdeng
   当前版本：v1.0
   使用授权协议： apache license 2.0

   云版本主页：http://xrkmonitor.com

   云版本为开源版提供永久免费告警通道支持，告警通道支持短信、邮件、
   微信等多种方式，欢迎使用

   开发库 mtagent_api_open 说明:
        字符云监控系统内部公共库，提供各种公共的基础函数调用

****/

#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>

#include "sv_cfg.h"
#include "mt_report.h"

#define MAX_CONFIG_LINE_LEN (1024 - 1)


inline int get_char_count(const char *pstr, char c)
{
	int i = 0;
	while(*pstr != '\0') {
		if(*pstr == c)  
			i++; 
		pstr++;
	}   
	return i;
}   


// 版本号: v2.2.2 or v1.1
int IsVersionOk(const char *pbig_eq, const char *psmall)
{
    uint32_t dwBigEq = 0, dwSmall = 0;
    int b1 = 0, b2 = 0, b3 = 0, s1 = 0, s2 = 0, s3 = 0;

    int i = get_char_count(pbig_eq, '.');
    if(i == 2)
        sscanf(pbig_eq, "%*[vV]%d.%d.%d", &b1, &b2, &b3);
    else if(i == 1)
        sscanf(pbig_eq, "%*[vV]%d.%d", &b1, &b2);
    else
        return 0;

    dwBigEq |= b1; dwBigEq <<= 8;
    dwBigEq |= b2; dwBigEq <<= 8;
    dwBigEq |= b3;

    i = get_char_count(psmall, '.');
    if(i == 2)
        sscanf(psmall, "%*[vV]%d.%d.%d", &s1, &s2, &s3);
    else if(i == 1)
        sscanf(psmall, "%*[vV]%d.%d", &s1, &s2);
    else
        return 0;
    dwSmall |= s1; dwSmall <<= 8;
    dwSmall |= s2; dwSmall <<= 8;
    dwSmall |= s3;
    return (dwBigEq >= dwSmall);
}

int GetConfigFile(const char *pAppStr, char **pconfOut)
{
	if(pconfOut == NULL)
		return -1; 

	char *papp = strdup(pAppStr);
	if(papp == NULL)
		return -2; 

	char *pconf = strrchr(papp, '/');
	if(pconf != NULL)
	{   
		*pconf = '\0';
		pconf++;
	}   

	static char szConf[256];
	snprintf(szConf, sizeof(szConf), "%s.conf", pconf);
	free(papp);
	*pconfOut = szConf;
	return 0;
}

void get_config_val(char* desc, char* src)
{
	get_val(desc, src);
}

char * get_val(char* desc, char* src)
{
	char *descp=desc, *srcp=src;
	int mtime=0, space=0;

	while ( mtime!=2 && *srcp != '\0' ) {
		switch(*srcp) {
			case ' ':
			case '\t':
			case '\0':
			case '\n':
			case '\r':
			case '\v':
				space=1;
				srcp++;
				break;
			default:
				if (space||srcp==src) mtime++;
				space=0;
				if ( mtime==2 ) break;
				*descp=*srcp;
				descp++;
				srcp++;
		}
	}
	*descp='\0';

	// fix bug by rockdeng
	//strcpy(src, srcp);
	while(*srcp != '\0') {
		*src = *srcp;
		src++;
		srcp++; 
	}           
	*src = '\0';

	return desc;
}

static void InitDefault(va_list ap)
{
	char *sParam, *sVal, *sDefault;
	double *pdVal, dDefault;
	long *plVal, lDefault;
	int iType, *piVal, iDefault;
	uint32_t lSize;
	uint32_t *pdwVal, dwDefault;
	uint64_t *pqwVal, qwDefault;

	sParam = va_arg(ap, char *);
	while (sParam != NULL)
	{
		iType = va_arg(ap, int);
		switch(iType)
		{
			case CFG_LINE:
				sVal = va_arg(ap, char *);
				sDefault = va_arg(ap, char *);
				lSize = va_arg(ap, uint32_t);
				strncpy(sVal, sDefault, (int)lSize-1);
				sVal[lSize-1] = 0;
				break;
			case CFG_STRING:
				sVal = va_arg(ap, char *);
				sDefault = va_arg(ap, char *);
				lSize = va_arg(ap, uint32_t);
				strncpy(sVal, sDefault, (int)lSize-1);
				sVal[lSize-1] = 0;
				break;
			case CFG_LONG:
				plVal = va_arg(ap, long *);
				lDefault = va_arg(ap, long);
				*plVal = lDefault;
				break;
			case CFG_INT:
				piVal = va_arg(ap, int *);
				iDefault = va_arg(ap, int);
				*piVal = iDefault;
				break;
			case CFG_DOUBLE:
				pdVal = va_arg(ap, double *);
				dDefault = va_arg(ap, double);
				*pdVal = dDefault;
				break;
			case CFG_UINT32:
				pdwVal = va_arg(ap, uint32_t *);
				dwDefault = va_arg(ap, uint32_t);
				*pdwVal = dwDefault;
				break;
			case CFG_UINT64:
				pqwVal = va_arg(ap, uint64_t *);
				qwDefault = va_arg(ap, uint64_t);
				*pqwVal = qwDefault;
				break;
		}
		sParam = va_arg(ap, char *);
	}
}

static void SetVal(va_list ap, char *sP, char *sV)
{
	char *sParam, *sVal = NULL;
	double *pdVal = NULL;
	long *plVal = NULL;
	int iType, *piVal = NULL;
	uint32_t lSize = 0;
	char sLine[MAX_CONFIG_LINE_LEN+1], sLine1[MAX_CONFIG_LINE_LEN+1];
	uint32_t *pdwVal = NULL;
	uint64_t *pqwVal = NULL;

	strcpy(sLine, sV);
	strcpy(sLine1, sV);
	get_val(sV, sLine1);
	sParam = va_arg(ap, char *);
	while (sParam != NULL)
	{
		iType = va_arg(ap, int);
		switch(iType)
		{
			case CFG_LINE:
				sVal = va_arg(ap, char *);
				va_arg(ap, char *);
				lSize = va_arg(ap, long);
				if (strcmp(sP, sParam) == 0) {
					strncpy(sVal, sLine, (int)lSize-1);
					sVal[lSize-1] = 0;
				}
				break;
			case CFG_STRING:
				sVal = va_arg(ap, char *);
				va_arg(ap, char *);
				lSize = va_arg(ap, uint32_t);
				break;
			case CFG_LONG:
				plVal = va_arg(ap, long *);
				va_arg(ap, long);
				break;
			case CFG_INT:
				piVal = va_arg(ap, int *);
				va_arg(ap, int);
				break;
			case CFG_DOUBLE:
				pdVal = va_arg(ap, double *);
				va_arg(ap, double);
				break;
			case CFG_UINT32:
				pdwVal = va_arg(ap, uint32_t *);
				va_arg(ap, uint32_t);
				break;
			case CFG_UINT64:
				pqwVal = va_arg(ap, uint64_t *);
				va_arg(ap, uint64_t);
				break;
		}
		if (strcmp(sP, sParam) == 0)
		{
			switch(iType)
			{
				case CFG_STRING:
					strncpy(sVal, sV, (int)lSize-1);
					sVal[lSize-1] = 0;
					break;
				case CFG_LONG:
					*plVal = atol(sV);
					break;
				case CFG_INT:
					*piVal = atoi(sV);
					break;
				case CFG_DOUBLE:
					*pdVal = atof(sV);
					break;
				case CFG_UINT32:
					*pdwVal = (uint32_t)strtoul(sV, NULL, 0);
					break;
				case CFG_UINT64:
					*pqwVal = (uint64_t)strtoull(sV, NULL, 0);
					break;
			}
			return;
		}

		sParam = va_arg(ap, char *);
	}
}

static int GetParamVal(char *sLine, char *sParam, char *sVal)
{
	if (sLine[0] == '#')
		return 1;
	
	get_val(sParam, sLine);
	strcpy(sVal, sLine);
	
	if (sParam[0] == '#')
		return 1;
		
	return 0;
}

// 校验插件配置，插件的配置值不能含有 ; 符号 (控制台修改配置逻辑用到了该符号)
int CheckPluginConfig(const char *pszConfigFilePath)
{
    FILE *pstFile;
    char sLine[MAX_CONFIG_LINE_LEN+1], sParam[MAX_CONFIG_LINE_LEN+1], sVal[MAX_CONFIG_LINE_LEN+1];
    if ((pstFile = fopen(pszConfigFilePath, "r")) == NULL)
        return -1;

    int iRet = 0;
    while (1)
    {
        fgets(sLine, sizeof(sLine), pstFile);
        if (feof(pstFile))
            break; 

        if (GetParamVal(sLine, sParam, sVal) == 0) {
            if(strchr(sVal, ';')) {
                iRet = -1;
                break;
            }
        }
    }	
    fclose(pstFile);
	return iRet;
}


int LoadConfig(const char *pszConfigFilePath, ...)
{
	FILE *pstFile;
	char sLine[MAX_CONFIG_LINE_LEN+1], sParam[MAX_CONFIG_LINE_LEN+1], sVal[MAX_CONFIG_LINE_LEN+1];
	va_list ap;
	
	va_start(ap, pszConfigFilePath);
	InitDefault(ap);
	va_end(ap);

	if ((pstFile = fopen(pszConfigFilePath, "r")) == NULL)
	{
		return -1;
	}

	while (1)
	{
		fgets(sLine, sizeof(sLine), pstFile);
		if (feof(pstFile))
		{
			break; 
		}
		
		if (GetParamVal(sLine, sParam, sVal) == 0)
		{
			va_start(ap, pszConfigFilePath);
			SetVal(ap, sParam, sVal);
			va_end(ap);
		}
	}	
	fclose(pstFile);

	return 0;
}

int GetLogTypeByStr(const char *pstrType)
{
	int iType = 0;
	if(!pstrType || pstrType[0] == '\0')
		return 0;

	if(strstr(pstrType, "all"))
	    return 255;
	if(strstr(pstrType, "debug"))
		iType += MTLOG_TYPE_DEBUG;
	if(strstr(pstrType, "info"))
		iType += MTLOG_TYPE_INFO;
	if(strstr(pstrType, "warn"))
		iType += MTLOG_TYPE_WARN;
	if(strstr(pstrType, "reqerr"))
		iType += MTLOG_TYPE_REQERR;
	if(strstr(pstrType, "error"))
		iType += MTLOG_TYPE_ERROR;
	if(strstr(pstrType, "fatal"))
		iType += MTLOG_TYPE_FATAL;
	return iType;
}


