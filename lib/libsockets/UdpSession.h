/*
   *
   *  可靠 udp session - by rockdeng
   */

#ifndef _SOCKETS_UdpSession_H
#define _SOCKETS_UdpSession_H 

#include <string.h>
#include "Ipv4Address.h"
#include "sockets-config.h"
#include "Socket.h"

#ifdef SOCKETS_NAMESPACE
namespace SOCKETS_NAMESPACE {
#endif

#define UDP_SESSION_BUF_LEN  64

class UdpSessionInfo;
typedef void (*TfunSessExpireCallback)(UdpSessionInfo *psess);
class UdpSessionInfo {
    public:
        UdpSessionInfo(uint64_t &qwSessId) {
            memset(m_sessBuf, 0, sizeof(m_sessBuf));
            m_qwSessionId = qwSessId;
            m_qwTimerId = 0;
            m_iMaxRetryTimes = 3;
            m_iRetryTimes = 0;
            m_iWaitTimeOutMs = 3000;
            m_pData = NULL;
            m_fun = NULL;
        }
        void SetTimerId(uint64_t &id) { m_qwTimerId=id; }
        void SetUdpPack(const char *pbuf, int ibuflen) { m_strUdpPack.assign(pbuf, ibuflen); }
        void SetRemote(uint32_t ip, int port) { m_stRmoteAddress.SetAddress(ip, port); }
        void SetMaxRetryTimes(int t) { m_iMaxRetryTimes=t; }
        void SetTimeoutMs(int ms) { m_iWaitTimeOutMs = ms; }
        void SetSessionData(void *pdata) { m_pData = pdata; }
        void SetSessExpireCallBack(TfunSessExpireCallback f) { m_fun = f; }

        Ipv4Address & GetRemoteAddr() { return m_stRmoteAddress;}
        std::string & GetPacket() { return m_strUdpPack; }
        uint64_t & GetSessId() { return m_qwSessionId; }
        uint64_t & GetTimerId() { return m_qwTimerId; }
        int GetTimeoutMs() { return m_iWaitTimeOutMs; }
        bool Retry() { return m_iRetryTimes < m_iMaxRetryTimes; }
        void AddRetry() { m_iRetryTimes++; }
        int GetRetry() { return m_iRetryTimes; }
        TfunSessExpireCallback GetSessExpireCallBack() { return m_fun; }
        char * GetSessBuf() { return m_sessBuf; }

    private:
        char m_sessBuf[UDP_SESSION_BUF_LEN];
        uint64_t m_qwSessionId; // session id
        uint64_t m_qwTimerId; // 定时器 id
        std::string m_strUdpPack; // udp 包
        Ipv4Address m_stRmoteAddress; // 接收方ip地址
        int m_iMaxRetryTimes; // 最多重试次数
        int m_iRetryTimes; // 当前重试次数
        int m_iWaitTimeOutMs; // 响应超时时间 ms
        void * m_pData;
        TfunSessExpireCallback m_fun;
};

#ifdef SOCKETS_NAMESPACE
}
#endif

#endif // _SOCKETS_UdpSession_H


