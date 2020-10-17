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

   模块 slog_monitor_server 功能:
        用于接收客户端上报上来的监控点数据，生成监控点数据当天的 memcache 缓存
        并将数据写入 MySQL 数据库中		

****/

#ifndef _UDP_SOCK_H_
#define _UDP_SOCK_H_ 1
#include <Sockets/UdpSocket.h>
#include <Sockets/SocketHandler.h>
#include <basic_packet.h>
#include <list>
#include <map>
#include "top_proto.pb.h"
#include "top_include_comm.h"
#include "memcache.pb.h"

const int32_t ATTR_SAVE_TYPE_PER_MIN = 1; // 每分钟的上报作为一条记录的存储方式
const int32_t ATTR_SAVE_TYPE_PER_DAY = 2; // 每天的上报作为一条记录的存储方式

typedef struct
{
	int32_t iKeepDay;
	int32_t iKeep;
	uint16_t wServerPort;
	uint16_t wInnerServerPort;
	char szListenIP[32];
	int iCheckDbConnectTimeSec;
	int iLocalMachineId;
	MachineInfo *pLocalMachineInfo;
	SLogConfig *pShmConfig;
	int iEnableMemcache;

	// memcache
	int iMemcacheBufSize;
	std::map<std::string, CMemCacheClient*> stMapMemcaches;
	int iRTimeout;
	int iWTimeout;
	int iMemcachePort;
	
	MtSystemConfig *psysConfig;
	char szBinSql[1024*512];
	StrAttrNodeValShmInfo *pstrAttrShm;
}CONFIG;

class CUdpSock: public UdpSocket, public CBasicPacket
{
	public:
		CUdpSock(ISocketHandler& h);
		~CUdpSock();

		// 写 memcache
		void WriteAttrDataToMemcache();

		// 写 db
		int SaveStrAttrInfoToDb(TStrAttrReportInfo* pStrAttrShm, Query &qu);
		int ReadStrAttrInfoFromDbToShm();

		void WriteStrAttrToDb();
		void WriteAttrDataToDb();
		int TryChangeAttrSaveType();
		int ChangeAttrSaveType(const char *ptable, Query &qu);
		void GetMonitorAttrMemcache(
			Query &qu, int attr_id, int machine_id, const char *ptable, comm::MonitorMemcache & memInfo, int iStaticTime); 
		void GetMonitorAttrMemcache(
			Query &qu, int attr_id, int machine_id, const char *ptable, uint32_t & uiMin, uint32_t & uiMax,
			uint32_t & uiTotal, int32_t & iReportCount, uint32_t *puiValArry, uint32_t *puiLastIp, int iStaticTime);
		int CheckTableName();
		const char *GetTodayTableName();
		const char *GetBeforeDayTableName();
		int CreateAttrTable(const char *pszTbName=NULL);
		int CreateAttrDayTable(const char *pszTbName=NULL);
		int GetMonitorAttrVal(Query &qu, int attr_id, int machine_id, comm::MonitorMemcache &memInfo);

		void CheckAllAttrTotal();
		void DealUserTotalAttr(int iAttrId, int iStaticTime, const char *pOldDay, const char *pNewDay);
		void InitTotalAttrReportShm(int32_t iAttrId, int iStaticTime, int iMachineId, comm::MonitorMemcache &memInfo, int iMinIdx);

        void DealQuickProcessMsg();
        void ReportQuickToSlowMsg();

		// 处理上报
		void DealLocalReportAttr();
		void DealSiteReportAttr();
		void DealReportAttr(::comm::ReportAttr &stReport);
		void OnRawData(const char *buf, size_t len, struct sockaddr *sa, socklen_t sa_len);
		int32_t SendResponsePacket(const char *pkg, int len);
		MachineInfo * GetReportMachine(uint32_t ip);
		int32_t OnRawDataClientAttr(const char *buf, size_t len);
		int CheckSignature();
		int CheckClearStrAttrNodeShm(TStrAttrReportInfo* pStrAttrShm);
		void SetLocalTimeInfo() { localtime_r(&slog.m_stNow.tv_sec, &m_currDateTime); }
		void DealMachineAttrReport(TStrAttrReportInfo *pAttrShm, int iStaticTime);

		Database *db;
		Query *m_qu;
		Database *db_attr_info;
		Query *m_qu_attr_info;
		int32_t m_iKeepDay;
		int32_t m_iKeep;
		int m_iDealAttrId;

		char m_sDecryptBuf[MAX_SIGNATURE_LEN+16];
		char m_szLastTableName[32];
		MachineInfo * m_pcltMachine;
		struct tm m_currDateTime;
		std::string m_strCommReportMachIp;
        std::string m_strSlowProcessIp;

	private:
		int DealCgiReportAttr(const char *buf, size_t len);
		void ResetRequest();
		void DealViewAutoBindMachine(TWarnAttrReportInfo *pRepAttrInfo, Query &qu);
		TWarnAttrReportInfo * AddReportToWarnAttrShm(
			uint32_t dwAttrId, int32_t iMachineId, uint32_t dwVal, int32_t iMinIdx, int iDataType, int32_t iStaticTime);
		TStrAttrReportInfo * AddStrAttrReportToShm(
			const ::comm::AttrInfo & reportInfo, int32_t iMachineId, uint8_t bStrAttrStrType);
};

#endif

