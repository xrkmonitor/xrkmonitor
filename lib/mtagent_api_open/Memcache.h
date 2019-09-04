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

#ifndef MEM_CACHE_CLIENT_H_
#define MEM_CACHE_CLIENT_H_

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS 1
#endif

#include <time.h>
#include <stdint.h>
#include <string.h>
#include <set>
#include "comm.pb.h"
#include "memcache.pb.h"
#include "mt_report.h"
#include "supper_log.h"

using namespace comm;

//#define ENABLE_MEM_LOG 1

class CMemCacheClient
{
    public:
        CMemCacheClient() :
            m_iSocket(-1),
            m_bGet(false),
            m_dwSrvIp(0),
            m_srvport(0),
            m_dwReadTimeout(50),
            m_dwWriteTimeout(200),
            m_szbuffer(NULL),
            m_iBufferLen(0),
			m_bDisable(true)
            {
                memset(m_srvip, 0, sizeof(m_srvip));
				memset(m_szKey, 0, sizeof(m_szKey));
				memset(m_szLastErrMsg, 0, sizeof(m_szLastErrMsg));
            }

		void InitDisable()
		{
			m_iSocket = -1;
			m_bDisable = true;
		}

		bool Disable() {return m_bDisable;}

        int Init(char *pBuff, int iBuffLen)
        {
            if(iBuffLen < 0 || pBuff == NULL)
            {
                return -1;
            }

            m_szbuffer = pBuff;
            m_iBufferLen = iBuffLen;
			m_bDisable = false;
            return 0;
        }

        ~CMemCacheClient(){ Close(); }

        int Connect(const char *pszIP, uint16_t wPort);	
        int AsyncConnect(const char *pszIP, uint16_t wPort, uint32_t dwReadTimeout = 50, uint32_t dwWriteTimeout = 50);
        void Close();

		// add by rockdeng
		const char * GetKey() { return m_szKey; }
		int SetKey(const char *pszfmt, ...);

		template<class T>
		int GetMonitorMemcache(T & memcache, const char* pszKey)
		{
			if(m_bDisable)
				return -1;

			char *pval = _GetValue(pszKey); 

			MtReport_Attr_Add(204, 1);
			if(NULL == pszKey)
				pszKey = (const char*)m_szKey;
			if(pval != NULL)
			{ 
				int len = *(int*)pval;
				MtReport_Attr_Add(203, 1);
				if(!memcache.ParseFromArray(pval+4, len))
				{
					WARN_LOG("memcahe data check failed - %d", len);
					return SLOG_ERROR_LINE;
				}

#ifdef ENABLE_MEM_LOG 
				DEBUG_LOG("memcahe get ok - %s", pszKey);
#endif
			}
			else
			{
				MtReport_Attr_Add(205, 1);
				WARN_LOG("memcache get fail - %s - %s", pszKey, m_szLastErrMsg);
				return SLOG_ERROR_LINE;
			}
			return 0;

		}

		template<class T>
		int SetMonitorMemcache(T &memcache, const char *pszKey, uint32_t dwExpire)
		{
			if(m_bDisable)
				return -1;

			static char s_Buf[1024*1024];
			int *plen = (int*)s_Buf;
			char *pData = s_Buf+4;
			MtReport_Attr_Add(208, 1);
			std::string strval;
			if(!memcache.AppendToString(&strval))
			{
				WARN_LOG("AppendToString failed !");
				return SLOG_ERROR_LINE;
			}

			if(strval.size()+4 > sizeof(s_Buf))
			{
				WARN_LOG("SetMonitorMemcache failed ,need more buffer - %u", (uint32_t)strval.size());
				return SLOG_ERROR_LINE;
			}

			*plen = (int)strval.size();
			memcpy(pData, strval.c_str(), strval.size());

			if(NULL == pszKey)
				pszKey = (const char*)m_szKey;

			int iret = SetValue(pszKey, strlen(pszKey), s_Buf, 4+strval.size(), dwExpire, 0);
			if(0 == iret)
			{ 
				MtReport_Attr_Add(207, 1);
#ifdef ENABLE_MEM_LOG 
				DEBUG_LOG("memcache set ok - key:%s - len:%d ", pszKey, (int)(4+strval.size()));
#endif
			}
			else
			{
				MtReport_Attr_Add(206, 1);
				WARN_LOG("memcache set fail - %s - %d ", pszKey, (int)(4+strval.size()));
			}
			return iret;
		}

		int GetMonitorMemcache(MonitorMemcache &memcache, const char* pszKey=NULL);
		int GetMonitorMemcache(MemcAttrInfo &memcache, const char* pszKey=NULL);
		int GetMonitorMemcache(MemcViewInfo &memcache, const char* pszKey=NULL);
		int GetMonitorMemcache(MemcMachineInfo &memcache, const char* pszKey=NULL);
		int SetMonitorMemcache(MonitorMemcache &memcache, const char *pszKey=NULL, uint32_t dwExpire=600);
		int SetMonitorMemcache(MemcAttrInfo &memcache, const char *pszKey=NULL, uint32_t dwExpire=600);
		int SetMonitorMemcache(MemcViewInfo &memcache, const char *pszKey=NULL, uint32_t dwExpire=600);
		int SetMonitorMemcache(MemcMachineInfo &memcache, const char *pszKey=NULL, uint32_t dwExpire=600);

		char * GetValue(const char* pszKey=NULL);
		int SetValue(const char *pszData, const char *pszKey=NULL, uint32_t dwExpire=600);
        int SetValue(const char *pszKey, int iKeyLen, const char *pData, int iDataLen, uint32_t dwExpire, int iFlag);
        int GetValue(const char* pszKey, int nKeyLen, int32_t& dwFlag, char *pData, int iDataLen);
        int DelValue(const char *pszKey, int iKeyLen);
        int SetAndCheckValue(const char *pszKey, int iKeyLen,
                const char *pData, int iDataLen, uint32_t dwExpire, uint64_t ddwVersion);
        int GetAndCheckValue(const char *pszKey, int iKeyLen, uint64_t& ddwVersion, char *pData, int iDataLen);
        int FlushValue(uint32_t dwTime=0);
		int GetDataLen() { return m_iDataLen; }
        char *GetLastErrMsg(){ return m_szLastErrMsg;};

    private:
		int _SetValue(const char *pszData, const char *pszKey, uint32_t dwExpire);
		char * _GetValue(const char* pszKey);

        int Reconnect();
        int AsyncReconnect();
        int SetSocketOption();
        int CheckReadsk(int iTimeout);
        int CheckWritesk();
        int CheckConnection(int iTimeout);
        int SendCmd(const char *pCmd, int iCmdLen);
        int RecvData(char *pData, int iDataLen);
        int Send2RecvData(int iSendLen); 
        int GetFlag(int iKeyLen, int& iFlag);
        uint64_t GetVersion(char *pDatalen);

    private:
        int m_iSocket;

        bool m_bGet;

        char m_srvip[20];
        uint32_t m_dwSrvIp;
        unsigned short m_srvport;

        uint32_t m_dwReadTimeout;
        uint32_t m_dwWriteTimeout;

        char *m_szbuffer; //[1024*1024];
        int m_iBufferLen;
		int m_iDataLen;
		char m_szKey[256];
        char m_szLastErrMsg[1024];
		bool m_bDisable;
};

#endif  /* MEM_CACHE_CLIENT_H_ */

