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

#ifndef _SV_ERRNO_H
#define _SV_ERRNO_H

#ifdef __cplusplus
extern "C" {
#endif

// network retcode
enum
{
	NO_ERROR = 0,
	ERR_SERVER = 1,
	ERR_INVALID_PACKET = 2,
	ERR_INVALID_PACKET_LEN = 3,
	ERR_UNKNOW_CMD= 4,
	ERR_CHECK_TLV = 5,
	ERR_CHECK_SIGNATUR = 6,
	ERR_NOT_ACK = 7,
	ERR_INVALID_TLV_VALUE = 8,
	ERR_FIND_TLV = 9,
	ERR_RESP_SIZE_OVER_LIMIT = 10,
	ERR_HTTP_KEEP_NOT_FIND = 11,
	ERR_DECRYPT_FAILED = 12,
	ERR_FIND_SESSION_INFO = 13,
	ERR_INVALID_CMD_CONTENT = 14,
	ERR_CHECK_DISTRIBUTE = 15,
	ERR_CHECK_BODY_MD5 = 16,
	ERR_UNKNOW_SIGTYPE = 17,
	ERR_NO_USER_MASTER = 18,
	ERR_DECRYPT_SIGNATURE = 19,
	ERR_INVALID_SIGNATURE_INFO = 20,
	ERR_CHECK_PACKET_CRC = 21,
	ERR_CHECK_DATA_FAILED = 80,
	ERR_MAX=100,
};

// generic
#define E_ERR				(-1)
#define E_BAD_PARAM			(-2)
#define E_CK_SOF			(-3)
#define E_OVERFLOW			(-4)
#define E_UNDERFLOW			(-5)
#define E_NOT_INIT			(-6)
#define E_IO_FAIL			(-7)
#define E_EMPTY				(-8)
#define E_FULL				(-9)
#define E_NOT_FOUND			(-10)
#define	E_INCONSISTENT		(-11)
#define E_TRUNC             (-12)
#define	E_DUPLICATE			(-13)
#define E_CONN_FAIL			(-14)
#define E_SEL_GRADE			(-15)
#define E_TIMEOUT			(-16)
#define E_TIMEIN			(-17)
#define E_OUT_OF_RANGE		(-18)
#define E_IP_ADDR_ERR		(-19)
#define E_BAD_VERSION		(-20)
#define E_BAD_APP_ID		(-21)
#define E_LOAD_FULL			(-22)

// crypt
#define E_BAD_CRYPT_ALGO	(-101)

// state
#define E_STATE_INVALID		(-201)
#define E_STATE_REDEFINE	(-202)
#define E_STATE_UNDEFINE	(-203)
#define E_STATE_MISMATCH	(-204)
#define E_NO_HANDLER		(-205)

// lock
#define E_LOCK_BUSY			(-301)

// config&sched API
#define E_SCHED_API_STATIC_USED (-401)
#define E_SCHED_

#ifdef __cplusplus
}
#endif

#endif // _SV_ERRNO_H
