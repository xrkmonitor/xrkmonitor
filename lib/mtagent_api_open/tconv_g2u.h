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

#ifndef __TCONV_GBK2UTF8_H_
#define __TCONV_GBK2UTF8_H_ 1

#include <stdint.h>

#if defined __x86_64__
#define C2_INT32 int
#define C2_TIME int
#define C2_SIZE unsigned int
#else
#define C2_INT32 long
#define C2_TIME time_t
#define C2_SIZE size_t
#endif

extern int tconv_utf82gbk(const char * in,
					C2_SIZE ilen,
					char * out,
					C2_SIZE * olen);

extern int tconv_gbk2utf8(const char * in,
					C2_SIZE ilen,
					char * out,
					C2_SIZE * olen);

extern int tconv_utf8to16(const unsigned char * u8,
					C2_SIZE u8len,
					uint16_t * u16,
					C2_SIZE u16maxlen);

extern int tconv_utf16to8(const uint16_t * u16,
					C2_SIZE u16len,
					unsigned char * u8,
					C2_SIZE u8maxlen);


#define TCONV_IS_GBK_CHAR(c1, c2) \
	((unsigned char)c1 >= 0x81 \
	&& (unsigned char)c1 <= 0xfe \
	&& (unsigned char)c2 >= 0x40)

#define TCONV_UTF8_LENGTH(c) \
	((unsigned char)c <= 0x7f ? 1 : \
	((unsigned char)c & 0xe0) == 0xc0 ? 2: \
	((unsigned char)c & 0xf0) == 0xe0 ? 3: \
	((unsigned char)c & 0xf8) == 0xf0 ? 4: 0)

#define TCONV_UTF8_LENGTH_ALL(c) \
	((unsigned char)c <= 0x7f ? 1 : \
	((unsigned char)c & 0xe0) == 0xc0 ? 2: \
	((unsigned char)c & 0xf0) == 0xe0 ? 3: \
	((unsigned char)c & 0xf8) == 0xf0 ? 4: \
	((unsigned char)c & 0xfc) == 0xf8 ? 5: \
	((unsigned char)c & 0xfe) == 0xfc ? 6: 0)

int IsUtf8Str(const char *sStr);
int IsGbkStr(const char *sStr);

#endif

