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

   开发库  mtreport_api 说明:
         用户使用监控系统的c/c++ 开发库，本库使用 标准 c 开发无任何第
		 三方库依赖，用户可以在 c或者 c++ 项目中使用

****/

#ifndef _MT_SHARED_HASH_H_11130304_
#define _MT_SHARED_HASH_H_11130304_ 1

/*
   *
   * 共享内存哈希表，支持快速遍历所有节点
   *
   */

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdlib.h>

#define STATIC_HASH_ROW_MAX 10 // 哈希阶数, 最大 100
#define STATIC_HASH_ROW_WARN 7 // 哈希使用预警阶数

// 简易哈希实现接口，特性如下
// 1) 以共享内存保存哈希节点，多个进程可以同时读，写接口只能一个进程访问
// 2) 提供快速遍历节点接口，无须扫描所有的哈希节点

// 节点比较函数, 查找和插入节点时调用
// 参数: pKey 哈希键值
// 参数: pNode 待比较的节点
// 返回值: pKey 和 pNode 匹配则返回 0，否则返回非 0
//   如果内置的链表需要按顺序插入，pKey 逻辑上比 pNode 大时请返回 1，逻辑小时请返回 -1
typedef int (*FunCompare)(const void *pKey, const void *pNode); 

// 哈希容量使用告警回调函数, 插入节点发生容量告警时调用
// 参数: dwCurUse 当前使用节点数(可遍历表) | 当前使用的哈希阶数(不可遍历表)
// 参数: dwTotal 总共节点数	| 总共的哈希阶数
// 返回值: 1 容量使用完，0 容量使用发生预警 
typedef int (*FunWarning)(uint32_t dwCurUse, uint32_t dwTotal); 

typedef struct 
{
	uint32_t dwNodeSize; // 调用方传入的，节点大小
	uint32_t dwNodeCount; // 调用方传入的, 节点数目
	uint32_t dwRealNodeCount; // 实际节点数目
	uint32_t dwShareMemBytes; // pHash 指向的共享内存大小
	uint32_t dwNodeNums[STATIC_HASH_ROW_MAX]; // 各阶节点数，同时也是各阶的模值, 取素数
	uint32_t dwLastUpdateTime; // 更新进程，最后一次更新时间
	uint8_t  bInitSuccess;
	uint8_t  bAccessCheck; // 启用访问检查，检查操作内部双向链表的函数是否为写进程等
	uint32_t dwCurIndex;
	uint32_t dwCurIndexRevers;
	FunCompare fun_cmp;
	FunWarning fun_war;

	// pHash: 大小为 sizeof(_HashTableHead)+(dwNodeSize+sizeof(_HashNodeHead))*dwRealNodeCount
	char *pHash; // 指向共享内存
}SharedHashTable; // 哈希表结构，进程相关

#pragma pack(1)
typedef struct
{
	uint32_t dwNodeStartIndex;
	uint32_t dwNodeEndIndex;
	uint32_t dwNodeUseCount;
	uint32_t dwNodeSize; // 用于校验
	uint32_t dwNodeCount; // 用于校验
	uint32_t dwWriteProcessId; // 写进程 id
	uint32_t dwLastUseTimeSec;
	volatile uint8_t bHashUseFlag; // 用于支持多进程多线程 [MtReport__]
} _HashTableHead; // 哈希表头，存在于共享内存中 _HashTable.pHash 前面部分

typedef struct
{
	uint8_t bIsUsed;
	uint32_t dwNodePreIndex;
	uint32_t dwNodeNextIndex;
} _HashNodeHead; // 节点头部结构，用于存储链接信息等
#pragma pack()

#define MTREPORT_SHARED_HASH_CHECK_STR "23k523k5jk@#%#%^#^@"


void ResetHashTable(SharedHashTable *phash);
_HashTableHead * MtReport_GetHashTableHead(SharedHashTable *phash);

// 初始化只读，使用前必须调用该接口
// 参数： dwNodeSize 节点数据大小
// 参数： dwNodeCount 节点数目最大值，实际分配的哈希节点
// 返回值: 成功返回 0， 否则非 0
int32_t MtReport_InitHashTable(SharedHashTable *phash, 
	uint32_t dwNodeSize, uint32_t dwNodeCount, int32_t *piSharedKey, FunCompare cmp, FunWarning warn);

// 共享内存不存在则创建
int32_t MtReport_CreateHashTable(SharedHashTable *phash, void *pShm, char cShmInit,
	uint32_t dwNodeSize, uint32_t dwNodeCount, FunCompare cmp, FunWarning warn);

// 初始化只读，使用前必须调用该接口
// 参数： dwNodeSize 节点数据大小
// 参数： dwNodeCount 节点数目最大值，实际分配的哈希节点
// 返回值: 成功返回 0， 否则非 0
int32_t InitHashTable(SharedHashTable *phash, 
	uint32_t dwNodeSize, uint32_t dwNodeCount, uint32_t dwSharedKey, FunCompare cmp, FunWarning warn);

// 同 InitHashTable ，注册为可以写共享内存，全局只能一个进程调用
int32_t InitHashTableForWrite(SharedHashTable *phash, 
	uint32_t dwNodeSize, uint32_t dwNodeCount, uint32_t dwSharedKey, FunCompare cmp, FunWarning warn);

// 查找节点
// 参数: pKey 键值，用于匹配节点
// 参数: dwShortKey 短键值，用于定位查找位置
// 返回值: 成功则返回对应节点的指针，失败返回 NULL
void * HashTableSearch(SharedHashTable *phash, const void *pKey, uint32_t dwShortKey);

// 查找节点，并可用于插入时返回空闲节点
// 参数: pKey 键值，用于匹配节点
// 参数: dwShortKey 短键值，用于定位查找位置
// 参数: pdwIsFind 用于输出 *pdwIsFind 为 1 表示查找成功，为 0 表示查找失败
// 返回值: 查找成功则返回对应节点指针，否则返回可插入的节点或者空节点 NULL 
void * HashTableSearchEx(
	SharedHashTable *phash, const void *pKey,  uint32_t dwShortKey, uint32_t *pdwIsFind);

// 使用节点，并设置链接信息, 只能一个进程访问
// 参数: pNode 指向的节点
// 功能: 使用节点，并插入到链接表中
// 返回值: 成功返回 0， 否则非 0
int32_t InsertHashNode(SharedHashTable *phash, void *pNode); // 全局只能有单一进程访问

// 使用节点，并设置链接信息, 只能一个进程访问
// 参数: pNode 指向的节点
// 功能: 使用节点，并按逻辑升序插入到内置链表中
// 返回值: 成功返回 0， 否则非 0
int32_t InsertHashNodeSort(SharedHashTable *phash, void *pNode); // 全局只能有单一进程访问

// 在指定节点后插入节点
// 返回值: 成功返回 0， 否则非 0
int32_t InsertHashNodeAfter(SharedHashTable *phash, void *pNode, uint32_t dwIndex); // 全局只能有单一进程访问

// 参数: pNode 指向的节点, 只能一个进程访问
// 功能: 回收节点，并从链接表中断开链接 
// 返回值: 成功返回 0， 否则非 0
int32_t RemoveHashNode(SharedHashTable *phash, void *pNode); // 全局只能有单一进程访问

// 删除指定位置的节点
// 返回值: 成功返回 0， 否则非 0
int32_t RemoveHashNodeByIndex(SharedHashTable *phash, uint32_t dwIndex); // 全局只能有单一进程访问

void ShowInnerSharedInfoByNode(SharedHashTable *phash, void *pNode);
void ShowInnerSharedInfoByIndex(SharedHashTable *phash, uint32_t dwIndex);

uint8_t IsValidIndex(SharedHashTable *phash, uint32_t dwIndex);

// 哈希节点遍历接口, 遍历完毕时返回 NULL
void * GetFirstNode(SharedHashTable *phash);
void * GetNextNode(SharedHashTable *phash);
void * GetNextNodeByNode(SharedHashTable *phash, void *pnode);
void * GetNextNodeByIndex(SharedHashTable *phash, uint32_t dwIndex);
void * GetCurNode(SharedHashTable *phash);

uint32_t GetFirstIndex(SharedHashTable *phash);
uint32_t GetLastIndex(SharedHashTable *phash);
uint32_t GetCurIndex(SharedHashTable *phash);
uint32_t GetCurIndexRevers(SharedHashTable *phash);

// 反序遍历接口
void * GetFirstNodeRevers(SharedHashTable *phash);
void * GetNextNodeRevers(SharedHashTable *phash);
void * GetNextNodeByNodeRevers(SharedHashTable *phash, void *pnode);
void * GetNextNodeByIndexRevers(SharedHashTable *phash, uint32_t dwIndex);
void * GetCurNodeRevers(SharedHashTable *phash);


// 返回当前使用的节点数
uint32_t GetNodeUsed(SharedHashTable *phash);

// 返回总共节点数
uint32_t GetNodeTotal(SharedHashTable *phash);

// 返回共享内存大小, 单位: Bytes
uint32_t GetSharedMemBytes(SharedHashTable *phash);


// ---------------------------------------------------------------------------------------------------------
// 以下哈希不支持快速遍历，其他同上面的

typedef struct 
{
	uint32_t dwNodeSize; // 调用方传入的，节点大小
	uint32_t dwNodeCount; // 调用方传入的, 节点数目
	uint32_t dwRealNodeCount; // 实际节点数目
	uint32_t dwShareMemBytes; // pHash 指向的共享内存大小
	uint32_t dwNodeNums[STATIC_HASH_ROW_MAX]; // 各阶节点数，同时也是各阶的模值, 取素数
	uint8_t bInitSuccess;
	FunCompare fun_cmp;
	FunWarning fun_war;

	char *pHash; // 指向共享内存
}SharedHashTableNoList; // 哈希表结构，进程相关

// 初始化只读，使用前必须调用该接口
// 参数： iNodeSize 节点数据大小
// 参数： iNodeCount 节点数目最大值，实际分配的哈希节点
// 参数： iNodeSize 节点数据大小
// 参数： iNodeSize 节点数据大小
// 返回值: 成功返回 0， 否则非 0
int32_t InitHashTable_NoList(SharedHashTableNoList *phash, 
	uint32_t dwNodeSize, uint32_t dwNodeCount, uint32_t dwSharedKey, FunCompare cmp, FunWarning warn);

// 动态内存接口
int32_t InitHashTable_NoList_Heap(SharedHashTableNoList *phash, 
	uint32_t dwNodeSize, uint32_t dwNodeCount, FunCompare cmp, FunWarning warn);


// 查找节点
// 参数: pKey 键值，用于匹配节点
// 参数: dwShortKey 短键值，用于定位查找位置
// 返回值: 成功则返回对应节点的指针，失败返回 NULL
void * HashTableSearch_NoList(SharedHashTableNoList *phash, const void *pKey, uint32_t dwShortKey);

// 查找节点，并可用于插入时返回空闲节点
// 参数: pKey 键值，用于匹配节点
// 参数: dwShortKey 短键值，用于定位查找位置
// 参数: pdwIsFind 用于输出 *pdwIsFind 为 1 表示查找成功，为 0 表示查找失败
// 返回值: 查找成功则返回对应节点指针，否则返回可插入的节点或者空节点 NULL 
void * HashTableSearchEx_NoList(SharedHashTableNoList *phash, 
	const void *pKey, const void *pEmptyKey, uint32_t dwShortKey, uint32_t *pdwIsFind);

#define container_of(ptr, type, member) ({ \
		const typeof( ((type *)0)->member ) *__mptr = (ptr);    \
		(type *)( (char *)__mptr - offsetof(type,member) );})

#define NODE_TO_NODE_HEAD(pNode) (_HashNodeHead*)((char*)pNode-sizeof(_HashNodeHead))
#define NODE_TO_INDEX(pNode) ((char*)pNode-phash->pHash-sizeof(_HashNodeHead)\
	-sizeof(_HashTableHead))/(sizeof(_HashNodeHead)+phash->dwNodeSize)
#define INDEX_TO_NODE_HEAD(index) (_HashNodeHead*)(phash->pHash+sizeof(_HashTableHead)+\
	index*(sizeof(_HashNodeHead)+phash->dwNodeSize))
#define INDEX_TO_NODE(index) (phash->pHash+sizeof(_HashTableHead)+\
	index*(sizeof(_HashNodeHead)+phash->dwNodeSize)+sizeof(_HashNodeHead))

// 外部可以调用的宏
#define HASH_NODE_TO_INDEX(pstHash, pNode) ((char*)pNode-(pstHash)->pHash-sizeof(_HashNodeHead)\
	-sizeof(_HashTableHead))/(sizeof(_HashNodeHead)+(pstHash)->dwNodeSize)
#define HASH_INDEX_TO_NODE(pstHash, index) ((pstHash)->pHash+sizeof(_HashTableHead)+\
	index*(sizeof(_HashNodeHead)+(pstHash)->dwNodeSize)+sizeof(_HashNodeHead))
#define NOLIST_HASH_NODE_TO_INDEX(pstHash, pNode) ((char*)pNode-(pstHash)->pHash)/(pstHash)->dwNodeSize
#define NOLIST_HASH_INDEX_TO_NODE(pstHash, index) ((pstHash)->pHash+index*(pstHash)->dwNodeSize)
#define HASH_NODE_IS_NODE_USED(pNode) (((_HashNodeHead*)((char*)pNode-sizeof(_HashNodeHead)))->bIsUsed)

uint32_t GetNextIndex(SharedHashTable *phash, uint32_t dwIndex);


void InitGetAllHashNode();
void * GetAllHashNode(SharedHashTable *phash);
void * GetAllHashNode_NoList(SharedHashTableNoList *phash);

#ifdef __cplusplus
}
#endif

#endif

