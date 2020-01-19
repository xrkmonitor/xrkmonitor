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

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>

#include <sys/types.h>
#include <ctype.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include "Memcache.h"

#define DEF_SK_RETRY_MAXCOUNT 30

#define END             "END"
#define VALUE           "VALUE"
#define LOW_VERSION     "LOW_VERSION"
#define SERVER_ERROR    "SERVER_ERROR"
#define CLIENT_ERROR    "CLIENT_ERROR"
#define EXISTS          "EXISTS"
#define NOT_FOUND       "NOT_FOUND"
#define STORED          "STORED"
#define DELETED         "DELETED"
#define OK              "OK"


int CMemCacheClient::Connect(const char *pszIP, uint16_t wPort)
{
	if(m_bDisable)
		return -1;

    Close();

    int m_iSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(m_iSocket < 0)
    {
        snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), 
                "[error] pid[%d] %s:%d create socket error ", 
                getpid(), __FILE__,__LINE__);

        return -15;
    }

	strncpy(m_srvip, pszIP, MYSIZEOF(m_srvip));
    m_dwSrvIp = inet_addr(pszIP);
    m_srvport = wPort;

    struct sockaddr_in srvaddr;
    srvaddr.sin_family = AF_INET;
    srvaddr.sin_addr.s_addr = inet_addr(pszIP);
    srvaddr.sin_port = htons(wPort);

    int rv = connect(m_iSocket, (struct sockaddr *)&srvaddr, sizeof(srvaddr));
    if(rv != 0) 
    {
        goto Err_Exit;
    }

    return 0;

Err_Exit:

    Close();

    snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), 
            "[error] pid[%d] %s:%d connect memserv error %s:%d", 
            getpid(), __FILE__,__LINE__, pszIP, wPort);

    return -15;
}


int CMemCacheClient::AsyncConnect(const char *pszIP, uint16_t wPort,
    uint32_t dwReadTimeout, uint32_t dwWriteTimeout)
{
	if(m_bDisable)
		return -1;

    m_dwReadTimeout = dwReadTimeout;
    m_dwWriteTimeout = dwWriteTimeout;
	strncpy(m_srvip, pszIP, MYSIZEOF(m_srvip));
    m_dwSrvIp = inet_addr(pszIP);
    m_srvport = wPort;

    Close();

    m_iSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(m_iSocket < 0) 
    {
        snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), 
                "[error] pid[%d] %s:%d create socket error ", 
                getpid(), __FILE__,__LINE__);

        return -15;
    }

    SetSocketOption();

    struct sockaddr_in srvaddr;
    srvaddr.sin_family = AF_INET;
    srvaddr.sin_addr.s_addr = inet_addr(pszIP);
    srvaddr.sin_port = htons(wPort);

    int rv = connect(m_iSocket, (struct sockaddr *)&srvaddr, sizeof(srvaddr));
    if(rv != 0) 
    {
        if(errno == EINPROGRESS)
        {
            int nRet = CheckConnection(50); 
            if(nRet != 0)
            {
                Close();
                goto Err_Exit;
            }
        }
        else
            goto Err_Exit;
    }

    return 0;

Err_Exit:

    Close();

    snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), 
            "[error] pid[%d] %s:%d connect memserv error %s:%d", 
            getpid(), __FILE__,__LINE__, pszIP, wPort);

    return -15;
}


int CMemCacheClient::AsyncReconnect()
{
	if(m_bDisable)
		return -110;

    Close();

    m_iSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(m_iSocket < 0) 
    {
        snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), 
                "[error] pid[%d] %s:%d create socket error ", 
                getpid(), __FILE__,__LINE__);

        return -15;
    }

    SetSocketOption();

    struct sockaddr_in srvaddr;
    srvaddr.sin_family = AF_INET;
    srvaddr.sin_addr.s_addr = m_dwSrvIp;
    srvaddr.sin_port = htons(m_srvport);

    int rv = connect(m_iSocket, (struct sockaddr *)&srvaddr, sizeof(srvaddr));
    if(rv != 0) 
    {
        if(errno == EINPROGRESS)
        {
            int nRet = CheckConnection(50);
            if(nRet != 0)
            {
                Close();
                goto Err_Exit;
            }
        }
        else
            goto Err_Exit;
    }

    return 0;

Err_Exit:

    Close();
    snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), 
            "[error] pid[%d] %s:%d connect mem serv %s:%d error ", 
            getpid(), __FILE__,__LINE__, m_srvip, m_srvport);

    return -1;
}

int CMemCacheClient::Reconnect()
{
    Close();

    m_iSocket = socket(AF_INET, SOCK_STREAM, 0);
    if(m_iSocket < 0) 
    {
        snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), 
                "[error] pid[%d] %s:%d create socket error ", 
                getpid(), __FILE__,__LINE__);

        return -15;
    }

    struct sockaddr_in srvaddr;
    srvaddr.sin_family = AF_INET;
    srvaddr.sin_addr.s_addr = m_dwSrvIp;
    srvaddr.sin_port = htons(m_srvport);

    int rv = connect(m_iSocket, (struct sockaddr *)&srvaddr, sizeof(srvaddr));
    if(rv != 0) 
    {
        Close();
        snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), 
                "[error] pid[%d] %s:%d connect mem serv %s:%d error ", 
                getpid(), __FILE__,__LINE__, m_srvip, m_srvport);

        return -1;
    }

    return 0;
}

int CMemCacheClient::SendCmd(const char *pszbuf, int len)
{
    int nlen = len;
    int ret = 0;

    const char *pbuf = pszbuf;
    int retry_count = 0;

send_data:
    if(retry_count > DEF_SK_RETRY_MAXCOUNT)
    {
        snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), 
                "[error] pid[%d] %s:%d sendcmd %s:%d error, sendcmd retry > %d ", 
                getpid(), __FILE__,__LINE__, m_srvip, m_srvport, DEF_SK_RETRY_MAXCOUNT);		

        return -1;
    }

    retry_count++;

    ret = CheckWritesk();
    if(ret != 0)
    {
        snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), 
                "[error] pid[%d] %s:%d sendcmd %s:%d error, send timeout 50 ", 
                getpid(), __FILE__,__LINE__, m_srvip, m_srvport);	

        return -2;
    }

    ret = send(m_iSocket, pbuf, nlen, MSG_NOSIGNAL);
    if(ret < 0)
    {	
        snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), 
                "[error] pid[%d] %s:%d sendcmd %s:%d error, send rv= %d", 
                getpid(), __FILE__,__LINE__, m_srvip, m_srvport, ret);	
        return -3;
    }
    else if(ret < nlen)
    {
        nlen -= ret;
        if(nlen > 0)
        {
            pbuf += ret;
            usleep(0);
            goto send_data;
        }
    }

    return 0;
}

int CMemCacheClient::CheckReadsk(int timeout)
{
    fd_set stRdFds;
    int    iFdsNum;
    struct timeval stTimeVal;

    FD_ZERO(&stRdFds);
    FD_SET(m_iSocket, &stRdFds);
    stTimeVal.tv_sec = 0;
    stTimeVal.tv_usec = timeout * 1000;
    iFdsNum = m_iSocket + 1;

    if(select(iFdsNum, &stRdFds, NULL, NULL, &stTimeVal) > 0)
    {
        if(FD_ISSET(m_iSocket, &stRdFds)) 
            return 0;
    }

    Close();

    return -1;
}

int CMemCacheClient::CheckWritesk()
{
    int    iFdsNum;
    struct timeval stTimeVal;

    fd_set stWtFds;
    FD_ZERO(&stWtFds);
    FD_SET(m_iSocket, &stWtFds);

    stTimeVal.tv_sec = 0;
    stTimeVal.tv_usec = 50000;
    iFdsNum = m_iSocket + 1;
    if(select(iFdsNum, NULL, &stWtFds, NULL, &stTimeVal) > 0)
    {
        if(FD_ISSET(m_iSocket, &stWtFds)) 
            return 0;
    }

    Close();

    return -1;
}

int CMemCacheClient::CheckConnection(int timeout)
{
    int    iFdsNum;
    struct timeval stTimeVal;

    fd_set stWtFds;
    FD_ZERO(&stWtFds);
    FD_SET(m_iSocket, &stWtFds);

    stTimeVal.tv_sec = 0;
    stTimeVal.tv_usec = timeout * 1000;
    iFdsNum = m_iSocket + 1;
    if(select(iFdsNum, NULL, &stWtFds, NULL, &stTimeVal) > 0)
    {
        if(FD_ISSET(m_iSocket, &stWtFds)) 
        {
            int error = 0;
            socklen_t len = sizeof(error);
            if(getsockopt(m_iSocket, SOL_SOCKET, SO_ERROR, &error, &len) == 0 && error == 0)
                return 0;
        }
    }

    return -1;
}

int CMemCacheClient::RecvData(char *pszbuf, int maxlen)
{
    int timeout = m_dwWriteTimeout;
    if(m_bGet)
        timeout = m_dwReadTimeout;

    int ret = CheckReadsk(timeout);
    if(ret != 0)
    {
        snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), 
                "[error] pid[%d] %s:%d RecvData %s:%d error, recv timeout:%d ", 
                getpid(), __FILE__,__LINE__, m_srvip, m_srvport, timeout);	

        return -20;
    }

    ret = recv(m_iSocket, pszbuf, maxlen, 0);
    if(ret > 0)
    {
        pszbuf[ret] = 0;
        return ret;
    }
    if(ret == 0)
        return -15;

    if(AsyncReconnect() < 0)
        return -15;

	ret = recv(m_iSocket, pszbuf, maxlen, 0);
	if(ret < 0)
		return -20;
	pszbuf[ret] = 0;
	return ret;
}

int CMemCacheClient::Send2RecvData(int nsendlen)
{
    int ret = -1;
    ret = SendCmd(m_szbuffer, nsendlen);
    if(ret != 0)
    {
        if(AsyncReconnect() < 0)
            return -15;

		ret = SendCmd(m_szbuffer, nsendlen);
		if(ret != 0)
			return -20;
    }

    memset(m_szbuffer, 0, m_iBufferLen);

    ret = RecvData(m_szbuffer, m_iBufferLen);

    return  ret;
}

int CMemCacheClient::SetKey(const char *pszfmt, ...)
{
	if(m_bDisable)
		return -2;

	va_list ap;
	va_start(ap, pszfmt);
	if(vsnprintf(m_szKey, MYSIZEOF(m_szKey), pszfmt, ap) >= (int)sizeof(m_szKey))
	{
		snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), "SetKey failed, key buffer len:%d",
			(int)sizeof(m_szKey));
		return -1;
	}
	va_end(ap);
	return 0;
}

int CMemCacheClient::GetMonitorMemcache(MemcAttrInfo &memcache, const char* pszKey)
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
		DEBUG_LOG("memcache get ok - %s", pszKey);
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

int CMemCacheClient::GetMonitorMemcache(MemcViewInfo &memcache, const char* pszKey)
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
		DEBUG_LOG("memcache get ok - %s", pszKey);
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

int CMemCacheClient::GetMonitorMemcache(MemcMachineInfo &memcache, const char* pszKey)
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
		DEBUG_LOG("memcache get ok - %s", pszKey);
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

int CMemCacheClient::GetMonitorMemcache(MonitorMemcache &memcache, const char* pszKey)
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
		DEBUG_LOG("memcache get ok - %s", pszKey);
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

char * CMemCacheClient::GetValue(const char* pszKey)
{
	if(m_bDisable)
		return NULL;

	char *pval = _GetValue(pszKey); 

	MtReport_Attr_Add(204, 1);
	if(NULL == pszKey)
		pszKey = (const char*)m_szKey;
	if(pval != NULL)
	{ 
		MtReport_Attr_Add(203, 1);
#ifdef ENABLE_MEM_LOG 
		DEBUG_LOG("memcache get ok - %s - %s", pszKey, pval);
#endif
	}
	else
	{
		MtReport_Attr_Add(205, 1);
		WARN_LOG("memcache get fail - %s - %s", pszKey, m_szLastErrMsg);
	}
	return pval;
}



char * CMemCacheClient::_GetValue(const char* pszKey)
{
	int32_t dwFlag = 0;

	if(NULL == pszKey)
		pszKey = (const char*)m_szKey;
	int nKeyLen = (int)strlen(pszKey);

    m_bGet = true;
	m_iDataLen = -1;

    if(m_iSocket < 0)
    {
        if(AsyncReconnect() < 0)
            return NULL;
    }

    int nLen = snprintf(m_szbuffer, m_iBufferLen, "get %s\r\n", pszKey);

    int recvlen = Send2RecvData(nLen);
    if(recvlen <= 0)
        return NULL;

    char *pstr = (char*)strchr(m_szbuffer, (int)'\n');
    if(pstr != NULL && memcmp(m_szbuffer, END, strlen(END)) == 0)
    {	
        snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), "Key[%s] does not exist in [%s:%u]!", 
			pszKey, m_srvip, m_srvport);
        return NULL;		
    }
    else if(pstr != NULL && memcmp(m_szbuffer, SERVER_ERROR, strlen(SERVER_ERROR)) == 0)
    {
        snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), 
                "[error] pid[%d] %s:%d getdata %s:%d error:SERVER_ERROR, key[%s] ", 
                getpid(), __FILE__,__LINE__, m_srvip, m_srvport, pszKey);	

        return NULL;
    }
    else if(pstr != NULL && memcmp(m_szbuffer, VALUE, strlen(VALUE)) == 0)
    {	
        char *p = m_szbuffer;

        int iFlagLen = GetFlag(nKeyLen, dwFlag);
        p = m_szbuffer + 6 + nKeyLen + 1 + iFlagLen + 1;
        recvlen -= (pstr+1-m_szbuffer);
        *pstr = 0; 
        *(pstr-1) = 0; 
        pstr = pstr+1; 

        int datalen = atoi(p);

        if(datalen + 7 == recvlen)
        {
			*(pstr+datalen) = '\0';
			m_iDataLen = datalen;
            return pstr;
        }
        else if(datalen + 7 > recvlen)
        {
            int retry_count = 0;
ReRecvData:		
            if(retry_count > DEF_SK_RETRY_MAXCOUNT)
            {
                Close();
                snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), 
                        "[error] pid[%d] %s:%d getdata %s:%d  error getvalue retry > %d, key[%s] ", 
                        getpid(), __FILE__,__LINE__, m_srvip, m_srvport, DEF_SK_RETRY_MAXCOUNT, pszKey);	
                return NULL;
            }

            retry_count++;

            int nRet = RecvData(pstr+recvlen, datalen+7-recvlen);
            if(nRet < 0)
                return NULL;

            recvlen += nRet;
            if(datalen + 7 > recvlen)
            {
                usleep(0);
                goto ReRecvData;
            }
            else
            {
				*(pstr+datalen) = '\0';
				m_iDataLen = datalen;
                return pstr;
            }			
        }
    }
    else
    {
        snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), 
                "[error] pid[%d] %s:%d RecvData %s:%d key[%s]  other error data ", 
                getpid(), __FILE__,__LINE__, m_srvip, m_srvport, pszKey);	
        Close();
    }

    return NULL;
}

int CMemCacheClient::GetValue(const char* pszKey, int nKeyLen, int32_t& dwFlag, char *pData, int iDataLen)
{
	if(m_bDisable)
		return -1;

    m_bGet = true;
	m_iDataLen = -1;

    if(m_iSocket < 0)
    {
        if(AsyncReconnect() < 0)
            return -15;
    }

    int nLen = snprintf(m_szbuffer, m_iBufferLen, "get %s\r\n", pszKey);

    int recvlen = Send2RecvData(nLen);
    if(recvlen <= 0)
        return recvlen;

    char *pstr = (char*)strchr(m_szbuffer, (int)'\n');
    if(pstr != NULL && memcmp(m_szbuffer, END, strlen(END)) == 0)
    {	
        snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), "Key[%s] does not exist in [%s:%u]!", 
			pszKey, m_srvip, m_srvport);
        return -11;		
    }
    else if(pstr != NULL && memcmp(m_szbuffer, SERVER_ERROR, strlen(SERVER_ERROR)) == 0)
    {
        snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), 
                "[error] pid[%d] %s:%d getdata %s:%d error:SERVER_ERROR, key[%s] ", 
                getpid(), __FILE__,__LINE__, m_srvip, m_srvport, pszKey);	

        return -13;
    }
    else if(pstr != NULL && memcmp(m_szbuffer, VALUE, strlen(VALUE)) == 0)
    {	
        char *p = m_szbuffer;

        int iFlagLen = GetFlag(nKeyLen, dwFlag);
        p = m_szbuffer + 6 + nKeyLen + 1 + iFlagLen + 1;
        recvlen -= (pstr+1-m_szbuffer);
        *pstr = 0; // \n -> 0
        *(pstr-1) = 0; // \r -> 0
        pstr = pstr+1; //point to data

        int datalen = atoi(p);

        if(iDataLen < datalen)
        {
            snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), "dwDataLen[%u] < %u", iDataLen, datalen);
            return -1;
        }

        //7 = \r\nEND\r\n
        if(datalen + 7 == recvlen)
        {
            memcpy(pData, pstr, datalen);
			m_iDataLen = datalen;
            return datalen;
        }
        else if(datalen + 7 > recvlen)
        {
            int retry_count = 0;
ReRecvData:		
            if(retry_count > DEF_SK_RETRY_MAXCOUNT)
            {
                Close();
                snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), 
                        "[error] pid[%d] %s:%d getdata %s:%d  error getvalue retry > %d, key[%s] ", 
                        getpid(), __FILE__,__LINE__, m_srvip, m_srvport, DEF_SK_RETRY_MAXCOUNT, pszKey);	
                return -20;
            }

            retry_count++;

            int nRet = RecvData(pstr+recvlen, datalen+7-recvlen);
            if(nRet < 0)
                return nRet;

            recvlen += nRet;
            if(datalen + 7 > recvlen)
            {
                usleep(0);
                goto ReRecvData;
            }
            else
            {
                memcpy(pData, pstr, datalen);
				m_bDisable = datalen;

                return datalen;
            }			
        }
    }
    else
    {//get data error
        snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), 
                "[error] pid[%d] %s:%d RecvData %s:%d key[%s]  other error data ", 
                getpid(), __FILE__,__LINE__, m_srvip, m_srvport, pszKey);	
        Close();
    }

    return -21;
}

int CMemCacheClient::SetMonitorMemcache(MemcAttrInfo &memcache, const char *pszKey, uint32_t dwExpire)
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
		Reconnect();
	}
	return iret;
}

int CMemCacheClient::SetMonitorMemcache(MemcViewInfo &memcache, const char *pszKey, uint32_t dwExpire)
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
		Reconnect();
	}
	return iret;
}

int CMemCacheClient::SetMonitorMemcache(MemcMachineInfo &memcache, const char *pszKey, uint32_t dwExpire)
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
		Reconnect();
	}
	return iret;
}

int CMemCacheClient::SetMonitorMemcache(MonitorMemcache &memcache, const char *pszKey, uint32_t dwExpire)
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
		WARN_LOG("memcache set fail - %s - %d - msg:%s", pszKey, (int)(4+strval.size()), m_szLastErrMsg);
		Reconnect();
	}
	return iret;
}

int CMemCacheClient::SetValue(const char *pData, const char *pszKey, uint32_t dwExpire)
{
	if(m_bDisable)
		return -1;

	int iret = _SetValue(pData, pszKey, dwExpire);
	MtReport_Attr_Add(208, 1);
	if(NULL == pszKey)
		pszKey = (const char*)m_szKey;
	if(0 == iret)
	{ 
		MtReport_Attr_Add(207, 1);
#ifdef ENABLE_MEM_LOG 
		DEBUG_LOG("memcache set ok - %s - %s", pszKey, pData);
#endif
	}
	else
	{
		MtReport_Attr_Add(206, 1);
		WARN_LOG("memcache set fail - %s - %s", pszKey, pData);
		Reconnect();
	}
	return iret;
}

int CMemCacheClient::_SetValue(const char *pData, const char *pszKey, uint32_t dwExpire)
{
	int iFlag = 0;
	int iDataLen = (int)strlen(pData);

	if(NULL == pszKey)
		pszKey = (const char*)m_szKey;

    m_bGet = false;
	m_iDataLen = -1;

    if(m_iSocket < 0)
    {
        if(AsyncReconnect() < 0)
            return -15;
    }

    int nLen = snprintf(m_szbuffer, m_iBufferLen, "set %s %u %u %u\r\n", 
            pszKey, iFlag, dwExpire, iDataLen);

    if(m_iBufferLen - nLen - 4 < iDataLen)
    {
        snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), 
                "[error] pid[%d] %s:%d setdata %s:%d data full buffer error, key[%s] buffer_len: %d, data_len: %d, cmd_len: %d", 
                getpid(), __FILE__,__LINE__, m_srvip, m_srvport, pszKey, m_iBufferLen,iDataLen,nLen);	

        memset(m_szbuffer, 0, m_iBufferLen);

        return -1;
    }

    memcpy(m_szbuffer+nLen, pData, iDataLen);
    nLen += iDataLen;

    memcpy(m_szbuffer+nLen, "\r\n", 2);
    nLen += 2;

    m_szbuffer[nLen] = 0;

    int recvlen = Send2RecvData(nLen);
    if(recvlen < 0)
        return recvlen;

    char *pstr = NULL;
    pstr = (char*)strchr(m_szbuffer, (int)'\n');
    if(pstr != NULL && memcmp(m_szbuffer, STORED, strlen(STORED)) == 0)
    {//set data ok
        return 0;
    }
    else if(pstr != NULL && memcmp(m_szbuffer, LOW_VERSION, strlen(LOW_VERSION)) == 0)
    {//set data error
        snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), 
                "[error] pid[%d] %s:%d setdata %s:%d  LOW_VERSION error, key[%s] ", 
                getpid(), __FILE__,__LINE__, m_srvip, m_srvport, pszKey);	

        return -16;
    }
    else if(pstr != NULL && memcmp(m_szbuffer, SERVER_ERROR, strlen(SERVER_ERROR)) == 0)
    {//set data error
        snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), 
                "[error] pid[%d] %s:%d setdata %s:%d  SERVER_ERROR error, key[%s] ", 
                getpid(), __FILE__,__LINE__, m_srvip, m_srvport, pszKey);	

        return -13;
    }
    else if(pstr != NULL && memcmp(m_szbuffer, CLIENT_ERROR, strlen(CLIENT_ERROR)) == 0)
    {//set data error
        snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), 
                "[error] pid[%d] %s:%d setdata %s:%d  CLIENT_ERROR error[%s], key[%s] ", 
                getpid(), __FILE__,__LINE__, m_srvip, m_srvport, m_szbuffer, pszKey);	
        return -14;
    }
    else
    {//set data error
        snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), 
                "[error] pid[%d] %s:%d setdata %s:%d key[%s]  other error data", 
                getpid(), __FILE__,__LINE__, m_srvip, m_srvport, pszKey);	
        Close();
    }

    return -21;
}

int CMemCacheClient::SetValue(const char *pszKey, int iKeyLen, const char *pData, int iDataLen, uint32_t dwExpire, int iFlag)
{
	if(m_bDisable)
		return -1;

	if(NULL == pszKey)
	{
		pszKey = (const char*)m_szKey;
		iKeyLen = (int)strlen(pszKey);
	}

    m_bGet = false;

    if(m_iSocket < 0)
    {
        if(AsyncReconnect() < 0)
            return -15;
    }

    int nLen = snprintf(m_szbuffer, m_iBufferLen, "set %s %u %u %u\r\n", 
            pszKey, iFlag, dwExpire, iDataLen);

    if(m_iBufferLen - nLen - 4 < iDataLen)
    {
        snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), 
                "[error] pid[%d] %s:%d setdata %s:%d data full buffer error, key[%s] buffer_len: %d, data_len: %d, cmd_len: %d", 
                getpid(), __FILE__,__LINE__, m_srvip, m_srvport, pszKey, m_iBufferLen,iDataLen,nLen);	

        memset(m_szbuffer, 0, m_iBufferLen);

        return -1;
    }

    memcpy(m_szbuffer+nLen, pData, iDataLen);
    nLen += iDataLen;

    memcpy(m_szbuffer+nLen, "\r\n", 2);
    nLen += 2;

    m_szbuffer[nLen] = 0;

    int recvlen = Send2RecvData(nLen);
    if(recvlen <= 0)
        return recvlen;

    char *pstr = NULL;
    pstr = (char*)strchr(m_szbuffer, (int)'\n');
    if(pstr != NULL && memcmp(m_szbuffer, STORED, strlen(STORED)) == 0)
    {//set data ok
        return 0;
    }
    else if(pstr != NULL && memcmp(m_szbuffer, LOW_VERSION, strlen(LOW_VERSION)) == 0)
    {//set data error
        snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), 
                "[error] pid[%d] %s:%d setdata %s:%d  LOW_VERSION error, key[%s] ", 
                getpid(), __FILE__,__LINE__, m_srvip, m_srvport, pszKey);	

        return -16;
    }
    else if(pstr != NULL && memcmp(m_szbuffer, SERVER_ERROR, strlen(SERVER_ERROR)) == 0)
    {//set data error
        snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), 
                "[error] pid[%d] %s:%d setdata %s:%d  SERVER_ERROR error, key[%s] ", 
                getpid(), __FILE__,__LINE__, m_srvip, m_srvport, pszKey);	

        return -13;
    }
    else if(pstr != NULL && memcmp(m_szbuffer, CLIENT_ERROR, strlen(CLIENT_ERROR)) == 0)
    {//set data error
        snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), 
                "[error] pid[%d] %s:%d setdata %s:%d  CLIENT_ERROR error[%s], key[%s] ", 
                getpid(), __FILE__,__LINE__, m_srvip, m_srvport, m_szbuffer, pszKey);	
        return -14;
    }
    else
    {//set data error
        snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), 
                "[error] pid[%d] %s:%d setdata %s:%d key[%s]  other error data", 
                getpid(), __FILE__,__LINE__, m_srvip, m_srvport, pszKey);	
        Close();
    }

    return -21;
}

int CMemCacheClient::DelValue(const char *pszKey, int iKeyLen)
{
	if(m_bDisable)
		return -1;

    m_bGet = false;

    if(m_iSocket < 0)
    {
        if(AsyncReconnect() < 0)
            return -15;
    }

    int nLen = snprintf(m_szbuffer, m_iBufferLen, "delete %s\r\n", pszKey);
    m_szbuffer[nLen] = 0;

    int recvlen = Send2RecvData(nLen);
    if(recvlen <= 0)
        return recvlen;

    char *pstr = NULL;
    pstr = (char*)strchr(m_szbuffer, (int)'\n');
    if(pstr != NULL && memcmp(m_szbuffer, DELETED, strlen(DELETED)) == 0)
    {//delete  data ok
        return 0;
    }
    else if(pstr != NULL && memcmp(m_szbuffer, NOT_FOUND, strlen(NOT_FOUND)) == 0)
    {//the data not exist
        snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), "Key[%s] does not exist in [%s:%u]!", 
			pszKey, m_srvip, m_srvport);
        return -11;
    }
    else if(pstr != NULL && memcmp(m_szbuffer, SERVER_ERROR, strlen(SERVER_ERROR)) == 0)
    {//delete data error
        snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), 
                "[error] pid[%d] %s:%d deldata %s:%d  SERVER_ERROR error, key[%s] ", 
                getpid(), __FILE__,__LINE__, m_srvip, m_srvport, pszKey);	

        return -13;
    }
    else if(pstr != NULL && memcmp(m_szbuffer, CLIENT_ERROR, strlen(CLIENT_ERROR)) == 0)
    {//delete data error
        snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), 
                "[error] pid[%d] %s:%d deldata %s:%d  CLIENT_ERROR error, key[%s] ", 
                getpid(), __FILE__,__LINE__, m_srvip, m_srvport, pszKey);	
        return -14;
    }
    else
    {//delete data error
        snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), 
                "[error] pid[%d] %s:%d deldata %s:%d key[%s]  other error data", 
                getpid(), __FILE__,__LINE__, m_srvip, m_srvport, pszKey);	
        Close();
    }

    return -21;
}

int CMemCacheClient::SetAndCheckValue(const char *pszKey, int iKeyLen,
        const char *pData, int iDataLen, uint32_t dwExpire, uint64_t ddwVersion)
{
	if(m_bDisable)
		return -1;

    m_bGet = false;

    if(m_iSocket < 0)
    {
        if(AsyncReconnect() < 0)
            return -15;
    }

#ifdef __x86_64
    int nLen = snprintf(m_szbuffer, m_iBufferLen, "cas %s %u %u %u %lu\r\n", 
            pszKey, (uint32_t)time(NULL), dwExpire, iDataLen, ddwVersion);
#else
    int nLen = snprintf(m_szbuffer, m_iBufferLen, "cas %s %u %u %u %llu\r\n",
            pszKey, (uint32_t)time(NULL), dwExpire, iDataLen, ddwVersion);
#endif

    if(m_iBufferLen - nLen - 4 < iDataLen)
    {
        snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), 
                "[error] pid[%d] %s:%d setandcheckvalue %s:%d data full buffer error, key[%s] ", 
                getpid(), __FILE__,__LINE__, m_srvip, m_srvport, pszKey);	

        m_szbuffer[0] = 0;

        return -1;
    }

    memcpy(m_szbuffer+nLen, pData, iDataLen);
    nLen += iDataLen;

    memcpy(m_szbuffer+nLen, "\r\n", 2);
    nLen += 2;

    m_szbuffer[nLen] = 0;

    int recvlen = Send2RecvData(nLen);
    if(recvlen <= 0)
        return recvlen;

    char *pstr = NULL;
    pstr = (char*)strchr(m_szbuffer, (int)'\n');
    if(pstr != NULL && memcmp(m_szbuffer, STORED, strlen(STORED)) == 0)
    {//cas data ok
        return 0;
    }
    else if(pstr != NULL && memcmp(m_szbuffer, EXISTS, strlen(EXISTS)) == 0)
    {//cas data error
        snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), 
                "[error] pid[%d] %s:%d setandcheckvalue %s:%d  EXISTS error, key[%s] ", 
                getpid(), __FILE__,__LINE__, m_srvip, m_srvport, pszKey);	

        return -17;
    }
    else if(pstr != NULL && memcmp(m_szbuffer, NOT_FOUND, strlen(NOT_FOUND)) == 0)
    {//cas data error
        snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), 
                "[error] pid[%d] %s:%d setandcheckvalue %s:%d  NOT_FOUND error, key[%s] ", 
                getpid(), __FILE__,__LINE__, m_srvip, m_srvport, pszKey);	

        return -18;
    }
    else if(pstr != NULL && memcmp(m_szbuffer, SERVER_ERROR, strlen(SERVER_ERROR)) == 0)
    {//cas data error
        snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), 
                "[error] pid[%d] %s:%d setandcheckvalue %s:%d  SERVER_ERROR error, key[%s] ", 
                getpid(), __FILE__,__LINE__, m_srvip, m_srvport, pszKey);	

        return -13;
    }
    else if(pstr != NULL && memcmp(m_szbuffer, CLIENT_ERROR, strlen(CLIENT_ERROR)) == 0)
    {//cas data error
        snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), 
                "[error] pid[%d] %s:%d setandcheckvalue %s:%d  CLIENT_ERROR error, key[%s] ", 
                getpid(), __FILE__,__LINE__, m_srvip, m_srvport, pszKey);	
        return -14;
    }
    else
    {//cas data error
        snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), 
                "[error] pid[%d] %s:%d setandcheckvalue %s:%d key[%s]  other error data", 
                getpid(), __FILE__,__LINE__, m_srvip, m_srvport, pszKey);	
        Close();
    }

    return -21;
}


int CMemCacheClient::GetAndCheckValue(const char *pszKey, int iKeyLen, uint64_t& ddwVersion, char *pData, int iDataLen)
{
	if(m_bDisable)
		return -1;

    m_bGet = true;
	m_iDataLen = -1;

    if(m_iSocket < 0)
    {
        if(AsyncReconnect() < 0)
            return -15;
    }

    int nLen = snprintf(m_szbuffer, m_iBufferLen, "gets %s\r\n", pszKey);

    int recvlen = Send2RecvData(nLen);
    if(recvlen <= 0)
        return recvlen;

    char *pstr = (char*)strchr(m_szbuffer, (int)'\n');
    if(pstr != NULL && memcmp(m_szbuffer, END, strlen(END)) == 0)
    {	
        snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), "Key[%s] does not exist in [%s:%u]!", 
			pszKey, m_srvip, m_srvport);
        return -11;		
    }
    else if(pstr != NULL && memcmp(m_szbuffer, SERVER_ERROR, strlen(SERVER_ERROR)) == 0)
    {
        snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), 
                "[error] pid[%d] %s:%d getdata %s:%d error:SERVER_ERROR, key[%s] ", 
                getpid(), __FILE__,__LINE__, m_srvip, m_srvport, pszKey);	

        return -13;
    }
    else if(pstr != NULL && memcmp(m_szbuffer, VALUE, strlen(VALUE)) == 0)
    {	
        char *p = m_szbuffer;

        int iFlag = 0; // no used
        int iflagLen = GetFlag(iKeyLen, iFlag);
        p = m_szbuffer + 6 + iKeyLen + 1 + iflagLen + 1; // p now points to datalen
        recvlen -= (pstr+1-m_szbuffer);
        *pstr = 0; // \n -> 0
        *(pstr-1) = 0; // \r -> 0
        pstr = pstr+1; //point to data
        ddwVersion = GetVersion(p);
        int datalen = atoi(p);

        if(iDataLen < datalen)
        {
            snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), "dwDataLen[%u] < %u", iDataLen, datalen);
            return -1;
        }

        //7 = \r\nEND\r\n
        if(datalen + 7 == recvlen)
        {
            memcpy(pData, pstr, datalen);

			m_iDataLen = datalen;
            return datalen;
        }
        else if(datalen + 7 > recvlen)
        {
            int retry_count = 0;
ReRecvData:		
            if(retry_count > DEF_SK_RETRY_MAXCOUNT)
            {
                Close();
                snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), 
                        "[error] pid[%d] %s:%d getdata %s:%d  error getvalue retry > %d, key[%s] ", 
                        getpid(), __FILE__,__LINE__, m_srvip, m_srvport, DEF_SK_RETRY_MAXCOUNT, pszKey);	
                return -20;
            }

            retry_count++;

            int nRet = RecvData(pstr+recvlen, datalen+7-recvlen);
            if(nRet < 0)
                return nRet;

            recvlen += nRet;
            if(datalen + 7 > recvlen)
            {
                usleep(0);
                goto ReRecvData;
            }
            else
            {
                memcpy(pData, pstr, datalen);
				m_iDataLen = datalen;

                return datalen;
            }			
        }
    }
    else
    {//get data error
        snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), 
                "[error] pid[%d] %s:%d RecvData %s:%d key[%s]  other error data ", 
                getpid(), __FILE__,__LINE__, m_srvip, m_srvport, pszKey);	
        Close();
    }

    return -21;
}

int CMemCacheClient::FlushValue(uint32_t dwTime/*not used currently*/)
{
	if(m_bDisable)
		return -1;
    m_bGet = false;

    if(m_iSocket < 0)
    {
        if(AsyncReconnect() < 0)
            return -15;
    }

    int nLen = snprintf(m_szbuffer, m_iBufferLen, "flush_all\r\n");
    m_szbuffer[nLen] = 0;

    int recvlen = Send2RecvData(nLen);
    if(recvlen <= 0)
        return recvlen;

    char *pstr = NULL;
    pstr = (char*)strchr(m_szbuffer, (int)'\n');
    if(pstr != NULL && memcmp(m_szbuffer, OK, strlen(OK)) == 0)
    {//flush data ok
        return 0;
    }
    else
    {//flush data error
        snprintf(m_szLastErrMsg, sizeof(m_szLastErrMsg), 
                "[error] pid[%d] %s:%d flushdata %s:%d other error data", 
                getpid(), __FILE__,__LINE__, m_srvip, m_srvport);	
        Close();
    }

    return -21;
}

int CMemCacheClient::GetFlag(int nKeyLen, int32_t& dwFlag)
{
    char* p = m_szbuffer + 6 + nKeyLen + 1;
    dwFlag = strtoul(p, NULL, 10);
    char* pspace = strchr(p, ' ');

    if(pspace == NULL)
        return 1;

    return (pspace - p);
}

uint64_t CMemCacheClient::GetVersion(char* pDataLen)
{
    char *p = pDataLen;
    while(*p != ' ')
        p++;

    *p = 0;
    p++;

    return strtoull(p, NULL, 10);
}

void CMemCacheClient::Close()
{
    if(m_iSocket >= 0)
        close(m_iSocket);

    m_iSocket = -1;
}

int CMemCacheClient::SetSocketOption()
{
    int flags = 1;
    flags = fcntl(m_iSocket, F_GETFL, 0);
    fcntl(m_iSocket, F_SETFL, flags | O_NONBLOCK);

    struct linger li;
    memset(&li, 0, sizeof(li));
    li.l_onoff = 1;
    li.l_linger = 0;
    setsockopt(m_iSocket, SOL_SOCKET, SO_LINGER, &li, sizeof(li));

    int iKeep = 0;
    setsockopt(m_iSocket, SOL_SOCKET, SO_KEEPALIVE, (char*)&iKeep,  sizeof(iKeep));

    return 0;
}

