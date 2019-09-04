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

   模块 slog_server 功能:
         接收日志客户端上报的日志，并将日志写入本机共享内存中

****/

#ifndef _UDP_SOCK_H_
#define _UDP_SOCK_H_ 1

#include <Sockets/UdpSocket.h>
#include <Sockets/SocketHandler.h>
#include <basic_packet.h>
#include <map> 
#include "top_proto.pb.h"

#define MAX_APP_LOG_PKG_LENGTH 1400 

typedef struct {
	AppInfo *papp_info;
	TSLogShm *plog_shm;
}TMapAppShmInfo;

typedef std::map<uint32_t, TMapAppShmInfo> TMapLogShm;
typedef std::map<uint32_t, TMapAppShmInfo>::iterator TMapLogShmItetor;

// log cust info
//  - cust1: req cmd, cust2: req seq, cust3: app id, cust4: user id, cust5:, cust6:remote address
//               
struct TGetAppLogSizeKey;
class CUdpSock: public UdpSocket, public CBasicPacket 
{
	public:
		CUdpSock(ISocketHandler& h);
		~CUdpSock();
		void OnRawData(const char *buf, size_t len, struct sockaddr *sa, socklen_t sa_len);
		void OnRawDataLocalPkg(const char *buf, size_t len, struct sockaddr *sa, socklen_t sa_len);
		int32_t OnRawDataClientLog(const char *buf, size_t len);
		int32_t CheckBodyTlvMd5(const char *pmd5) { return 0; }
		virtual int32_t SendResponsePacket(const char *pkg, int len);
		int CheckSignature();

		int CheckUserAppMatch(int iAppId);
		uint64_t GetAppLogSize(int iAppId);

		void SendGetAppLogSizeReq(const TGetAppLogSizeKey &info, top::SlogGetAppLogSizeReq *pApps);
		void GetAppLogSize(AppInfo *pAppInfo, top::SlogGetAppLogSizeRsp & rsp);
		void DealGetAppLogSizeReq(top::SlogGetAppLogSizeReq &reqinfo);

		void SendGetAppLogSizeRsp(top::SlogGetAppLogSizeRsp &rsp);
		void DealGetAppLogSizeRsp(top::SlogGetAppLogSizeRsp &rsp);

		int SendPacketPb(top::SlogRemoveAppLogFile &req, AppInfo * pAppInfo);
		void DealDeleteAppLogFile(top::SlogRemoveAppLogFile &req);
		bool IsLogFreqOver(TSLogShm * pLogShm);

	private:
		char m_sDecryptBuf[MAX_SIGNATURE_LEN+16];
		char m_pkgBuf[MAX_UDP_REQ_PACKET_SIZE*2];
		int m_iPkgLen;
		MachineInfo * m_pcltMachine;
		TMapLogShm m_mapLogShm;
		uint32_t m_dwReqSeqLocal;
};

#endif

