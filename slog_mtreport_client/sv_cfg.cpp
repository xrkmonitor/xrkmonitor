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

   模块 slog_mtreport_client 功能:
        用于上报除监控系统本身产生的监控点数据、日志，为减少部署上的依赖
		未引入任何第三方组件

****/

#include <sys/stat.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <ctype.h>
#include <stdarg.h>
#include <stdint.h>

#include "sv_errno.h"
#include "sv_cfg.h"
#include "mtreport_client.h"

#define MAX_CONFIG_LINE_LEN (5 * 1024 - 1)

int GetConfigItemList(const char *pfile, TConfigItemList & list);
void UpdateConfigFile(const char *pfile, TConfigItemList & list)
{
	TConfigItemList list_cfg;
	if(GetConfigItemList(pfile, list_cfg) < 0)
		return;

	TConfigItemList::iterator it = list.begin();
	TConfigItem *pitem = NULL, *pitem_cfg = NULL;
	for(; it != list.end(); it++)
	{
		pitem = *it;
		TConfigItemList::iterator it_cfg = list_cfg.begin();
		for(; it_cfg != list_cfg.end(); it_cfg++)
		{
			pitem_cfg = *it_cfg;
			if(pitem->strConfigName == pitem_cfg->strConfigName){
				pitem_cfg->strConfigValue = pitem->strConfigValue;
				break;
			}
		}
		if(it_cfg == list_cfg.end())
		{
			TConfigItem *pitem_tmp = new TConfigItem;
			pitem_tmp->strConfigName = pitem->strConfigName;
			pitem_tmp->strConfigValue = pitem->strConfigValue;;
			list_cfg.push_back(pitem_tmp);
		}
	}
	
	FILE *pstFile = NULL;
	if ((pstFile = fopen(pfile, "w+")) == NULL)
	{
		ERROR_LOG("open file : %s failed, msg:%s", pfile, strerror(errno));
		return ;
	}

	TConfigItemList::iterator it_cfg = list_cfg.begin();
	for(; it_cfg != list_cfg.end(); it_cfg++)
	{
		pitem_cfg = *it_cfg;
		std::string::reverse_iterator rit = pitem_cfg->strConfigValue.rbegin();
		if(*rit == '\n')
			fprintf(pstFile, "%s %s", pitem_cfg->strConfigName.c_str(), pitem_cfg->strConfigValue.c_str());
		else
			fprintf(pstFile, "%s %s\n", pitem_cfg->strConfigName.c_str(), pitem_cfg->strConfigValue.c_str());
	}
	fclose(pstFile);
	ReleaseConfigList(list_cfg);
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

static char * get_val(char* desc, char* src)
{
	char *descp=desc, *srcp=src;
	int mtime=0, space=0;

	while ( mtime!=2 && *srcp != '\0' ) {
		switch(*srcp) {
			case ' ':
			case '\t':
			case '\0':
			case '\n':
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
	strcpy(src, srcp);
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

	get_val(sParam, sLine);
	strcpy(sVal, sLine);
	
	if (sParam[0] == '#')
		return 1;
		
	return 0;
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
		return E_ERR;
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

int GetConfigItemList(const char *pfile, TConfigItemList & list)
{
	FILE *pstFile = NULL;
	char sLine[MAX_CONFIG_LINE_LEN+1], sParam[MAX_CONFIG_LINE_LEN+1], sVal[MAX_CONFIG_LINE_LEN+1];

	if ((pstFile = fopen(pfile, "r")) == NULL)
	{
		ERROR_LOG("open file : %s failed, msg:%s", pfile, strerror(errno));
		return -1;
	}

	while (1)
	{
		fgets(sLine, sizeof(sLine), pstFile);
		if (feof(pstFile))
			break; 
		
		if (GetParamVal(sLine, sParam, sVal) == 0)
		{
			TConfigItem *pitem = new TConfigItem;
			pitem->strConfigName = sParam;
			pitem->strConfigValue = sVal;
			list.push_back(pitem);
		}
	}
	fclose(pstFile);
	return 0;
}

