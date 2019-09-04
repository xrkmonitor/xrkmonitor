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

#ifndef FLOGIN_SESSION_H__
#define FLOGIN_SESSION_H__ (1)

#include "sv_str.h"
#include "sv_struct.h"

#define FLOGIN_MD5_STRING "23@#%@@20130504^"

// 机器 model 字段
#define MACHINE_MODEL_SYS_CREATE 1 // 注册时系统创建
#define MACHINE_MODEL_USER_CREATE 2  // 用户手动注册

#define FLOGIN_SESSION_HASH_SHM_KEY 20131224
#define FLOGIN_SESSION_NODE_COUNT 4000 // 单机最大同时在线
#define LOGIN_MAX_EXPIRE_TIME 14*24*60*60 // 最大的登录重验证密码时间
#define OP_CODE_VALID_TIME 180 // 操作验证码有效期

typedef struct 
{
	uint8_t bLoginType; // bLoginType 值为 1 表示管理员
	char szUserName[33]; // 用户名
	char szPassMd5[33]; // 验证成功后, 生成的登录session key
	uint32_t dwLoginIP; // 登录IP，输入用户名密码验证时的IP
	uint32_t dwLoginTime; // 登录时间，输入用户名密码验证时的IP
	uint32_t dwLastAccessTime;
	uint32_t dwLastAccessIP; // 最近访问的 IP
	int32_t iLoginExpireTime; // 登录有效期单位 s
	uint32_t dwUserFlag_1; // 用户标记位, 落地存储,对应DB 中的user_flag_1
	uint32_t dwUserFlag_2; // 用户标记位, 内存中使用
	int32_t iUserId; // 用户 id
	char sReserved[2048]; // pb 结构信息 - user::UserSessionInfo

	void Show() {
		SHOW_FIELD_VALUE_UINT(bLoginType);
		SHOW_FIELD_VALUE_CSTR(szUserName);
		SHOW_FIELD_VALUE_CSTR(szPassMd5);
		SHOW_FIELD_VALUE_UINT_IP(dwLoginIP);
		SHOW_FIELD_VALUE_UINT_TIME(dwLoginTime);
		SHOW_FIELD_VALUE_UINT_TIME(dwLastAccessTime);
		SHOW_FIELD_VALUE_UINT_IP(dwLastAccessIP);
		SHOW_FIELD_VALUE_INT(iLoginExpireTime);
		SHOW_FIELD_VALUE_UINT(dwUserFlag_1);
		SHOW_FIELD_VALUE_UINT(dwUserFlag_2);
		SHOW_FIELD_VALUE_INT(iUserId);
	}
}FloginInfo;

typedef struct
{
	int32_t iLoginCount; // 指示作用，并不能真实代表在线用户数
	int32_t iWriteIndex; // 写数组索引，环形: 0 至 FLOGIN_SESSION_NODE_COUNT-1
	FloginInfo stLoginList[FLOGIN_SESSION_NODE_COUNT];
}FloginList;

#endif

