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
#include <sv_cfg.h>
#include <sstream>

#include "sv_errno.h"
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
				pitem_cfg->strConfigValue += "\r\n";
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

    std::ostringstream ssTmp;
    ssTmp << "cp " << pfile << " " << pfile << "_bk"; 
    system(ssTmp.str().c_str());
    DEBUG_LOG("backup file:%s", ssTmp.str().c_str());
	
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
        if(pitem_cfg->strComment.size() > 0) {
			std::string::reverse_iterator rit = pitem_cfg->strComment.rbegin();
            if(*rit == '\n')
            	fprintf(pstFile, "%s", pitem_cfg->strComment.c_str());
			else
            	fprintf(pstFile, "%s\r\n", pitem_cfg->strComment.c_str());
        }
        else {
        	fprintf(pstFile, "%s %s", pitem_cfg->strConfigName.c_str(), pitem_cfg->strConfigValue.c_str());
        }
	}
	fclose(pstFile);
	ReleaseConfigList(list_cfg);
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
		
		TConfigItem *pitem = new TConfigItem;
		if (GetParamVal(sLine, sParam, sVal) == 0)
		{
			pitem->strConfigName = sParam;
			pitem->strConfigValue = sVal;
			list.push_back(pitem);
		}
        else {
            pitem->strComment = sLine;
            list.push_back(pitem);
        }
	}
	fclose(pstFile);
	return 0;
}


