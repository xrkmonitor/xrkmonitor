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

#ifndef __SV_NET_H__
#define __SV_NET_H__

#include <net/if.h>
#include <netinet/in.h>
#include <endian.h>
#include <byteswap.h>
#include <stdint.h>

typedef struct{
	char name[IFNAMSIZ];
	struct sockaddr_in addr;
	int flags;
}IfDesc_ipv4;

int GetIf(IfDesc_ipv4 * pstDesc);
const char * GetLocalIP(const char *pszRemoteIp);
void GetCustLocalIP(char *pszLocalIp);

static inline uint64_t Hton64(uint64_t qwVal)
{
#if __BYTE_ORDER == __BIG_ENDIAN
	return qwVal;
#else
	return bswap_64(qwVal);
#endif
}

#define Ntoh64	Hton64

#ifndef SHOW_IPV4
#define SHOW_IPV4(paddr) \
	    ((unsigned char*)(&(paddr)))[0], \
        ((unsigned char*)(&(paddr)))[1], \
        ((unsigned char*)(&(paddr)))[2], \
        ((unsigned char*)(&(paddr)))[3] 
#define SHOW_IPV4_H(paddr) \
	    ((unsigned char*)(&(paddr)))[3], \
        ((unsigned char*)(&(paddr)))[2], \
        ((unsigned char*)(&(paddr)))[1], \
        ((unsigned char*)(&(paddr)))[0] 
#define SHOW_IPV4_FMT " [%d.%d.%d.%d] "
#endif

int SetNBlock(int iSock);
int SetSocketBuffer(int sockfd, int nRecvBuf, int nSendBuf);

#endif

