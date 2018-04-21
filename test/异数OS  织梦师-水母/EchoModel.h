#pragma once
#define Error_Transfer 

#define TEST_MODE_LOOPBACK			1	//Client Server 都建立在本地核形成回环访问
#define TEST_MODE_BALANCED			2	//通过虚拟交换机将Client Server 均衡到不同CPU核
//#define TEST_MODE TEST_MODE_LOOPBACK
#define TEST_MODE TEST_MODE_BALANCED

#include "event.h"


//#define WAIT_IO_TIMEOUT
#ifdef WAIT_IO_TIMEOUT
#define SyncCall(_Name,_IoCode,_WaitTime) IoCall_WaitTimeOut(_Name,_IoCode,_WaitTime)
#else
#ifdef intel_cpp
#define SyncCall(_Name,_IoCode,_WaitTime) IoCall(_IoCode)
#else
#define SyncCall(_Name,_IoCode,_WaitTime) IoCall_Name(_Name,_IoCode)
#endif
#endif


template<typename WaitEventT>
class EchoProcessBase
{
public:
	
	WaitEventT * m_pWaitEvent;
	void * m_pRequest;
	void * m_pRespond;
	unsigned int m_nRequestLen;
	unsigned int m_nRespondLen;
	EchoProcessBase() {};
	~EchoProcessBase() {};

	//==============================Echo接口==============================
	//Echo线程收到请求时调用
	bool ProcessHandle(void * pMessage, unsigned int nLen)
	{
		return false;
	}
	//Echo线程发起请求时调用，返回false则表示Echo线程阻塞等待m_WaitEvent
	bool GetRequest()
	{
		return false;
	}
	//Echo线程回应时调用，返回false则表示Echo线程阻塞等待m_WaitEvent
	bool GetRespond()
	{
		return false;
	}
	void init()
	{
	}
	void close()
	{
	}
	//==============================APP线程接口==============================


private:

};


#define ErrorProcess(_LogName,_ID,_SucessError)\
nError = HereticThreadBaseT::GetLastError();\
if (nError == _SucessError) {\
	Error_Transfer(_T("%s id=%d ok this=%p"),_LogName, _ID,this);\
}\
else if (nError == DEVICE_ERROR_TIME_OUT)\
{\
	Error(_T("%s id=%d time out this=%p"), _LogName, _ID,this);\
	break;\
}\
else\
{\
	Error(_T("%s id=%d fail %d this=%p"), _LogName, _ID,nError,this);\
	HereticThreadBaseT::PrintDeviceError();\
	break;\
}\





template<bool ConditionT, typename CodeTypeT, typename CodeType1T>
struct _IF
{
	//typedef CodeTypeT CodeType;
};
template< typename CodeTypeT, typename CodeType1T>
struct _IF<true, CodeTypeT, CodeType1T>
{
	typedef  CodeTypeT CodeType;
};
template< typename CodeTypeT, typename CodeType1T>
struct _IF<false, CodeTypeT, CodeType1T>
{
	typedef  CodeType1T CodeType;
};


template<typename EchoProcessT,bool bServerMode=true, bool bPushMode = false, bool bWaitBegin = false>
class Echo : public HereticThread<Echo<EchoProcessT, bServerMode, bPushMode, bWaitBegin>, 2>
{
public:
	//USING_ALIGNEDMEM;
	unsigned int nError;
	typedef TCPSigleThreadClient<Echo<EchoProcessT, bServerMode, bPushMode, bWaitBegin>, DefTCPConfig> TcpClientT;
	typedef TCPSigleThreadServer<Echo<EchoProcessT, bServerMode, bPushMode, bWaitBegin>, DefTCPConfig> TcpServerT;
	typedef typename _IF<bServerMode, TcpServerT, TcpClientT>::CodeType _MyTcpType;
	typedef HereticThread<Echo<EchoProcessT, bServerMode, bPushMode, bWaitBegin>, 2> HereticThreadBaseT;

	EchoProcessT m_EchoProcess;
	volatile ULONGLONG nWaitTime;
	volatile ULONGLONG nWaitTimeTotal;
	unsigned int m_nID;
	unsigned int m_nReadActiveSession;
	unsigned int m_nWriteActiveSession;
	unsigned int m_nTotalActiveSession;
	Echo() {
	};
	~Echo() {};
	void Init() {
		m_nID = bServerMode? g_GInfo[XOS_LocalSystem->m_nCurrentCpu].g_nServerCount++:g_GInfo[XOS_LocalSystem->m_nCurrentCpu].g_nClientCount++;
		m_nReadActiveSession = m_nWriteActiveSession = m_nTotalActiveSession = 0;
		m_EchoProcess.init();
	};
	void Close() {
		m_EchoProcess.close();
	};


	void Loop(void * pContext = NULL);

private:

	//}CACHE_ALIGN_END;
};

OPTIMIZE_OFF
template<typename EchoProcessT, bool bServerMode, bool bPushMode, bool bWaitBegin>
void Echo<EchoProcessT, bServerMode, bPushMode, bWaitBegin>::Loop(void * pContext)
{
	USING_HERETICOS_THREAD;
	do
	{

		if (bServerMode)
		{
			static_cast<_MyTcpType*>(this)->accept();
			g_GInfo[XOS_LocalSystem->m_nCurrentCpu].g_nCurrentServerLinkCount++;
		}
		else
		{
			SyncCall(ContextClientConnect, static_cast<_MyTcpType*>(this)->connect(), 6000);
			ErrorProcess(typeid(EchoProcessT).name(), m_nID, DEVICE_ERROR_SESSION_OK);
			g_GInfo[XOS_LocalSystem->m_nCurrentCpu].g_nCurrentClientLinkCount++;
			g_GInfo[XOS_LocalSystem->m_nCurrentCpu].g_IoMonitor->Add();
		}
		
		if (bWaitBegin)
		{
			for (; g_GInfo[XOS_LocalSystem->m_nCurrentCpu].g_bBeginEcho == FALSE;)
			{
				XOS_Sleep_Name(CreateClient, 5000);
			}
		}
		for (;;)
		{
			nWaitTimeTotal = nWaitTime = CurrentTimeTick;
			if (bServerMode^bPushMode)
			{
				nWaitTime = CurrentTimeTick;
				//XOS_Sleep_Name(ClientRecvSleep, 37);
				SyncCall(PushModeRecv, static_cast<_MyTcpType*>(this)->recv(), 6000);
				ErrorProcess(bServerMode?_T("Server Recv"): _T("Client Recv"), m_nID, DEVICE_ERROR_READ_OK);
				//g_GInfo[XOS_LocalSystem->m_nCurrentCpu].g_nClientCompleteCount++;

				m_EchoProcess.ProcessHandle(static_cast<_MyTcpType*>(this)->m_pRecvBuffer,
					win_min(static_cast<_MyTcpType*>(this)->m_nRecvLen, MTU));
				if (!bServerMode)
				{
					g_GInfo[XOS_LocalSystem->m_nCurrentCpu].g_IoMonitor->AddIoCount();
#ifdef ANALYSYS_READ
					g_GInfo[XOS_LocalSystem->m_nCurrentCpu].g_IoMonitor->AddReadLazySample(m_nReadActiveSession, CurrentTimeTick - nWaitTime);
#endif
				}
				
				do
				{
					if (m_EchoProcess.GetRespond() == false)
					{
						XOS_Wait_P(m_EchoProcess.m_pWaitEvent);
					}
					else
					{
						break;
					}
				} while (true);

				SyncCall(PushModeSend, static_cast<_MyTcpType*>(this)->send((unsigned char *)m_EchoProcess.m_pRespond,
					m_EchoProcess.m_nRespondLen), 6000);
				ErrorProcess(bServerMode ? _T("Server Send") : _T("Client Send"), m_nID, DEVICE_ERROR_WRITE_OK);
				if (!bServerMode)
				{
#ifdef ANALYSYS_WRITE
					g_GInfo[XOS_LocalSystem->m_nCurrentCpu].g_IoMonitor->AddWriteLazySample(m_nWriteActiveSession, CurrentTimeTick - nWaitTime);
#endif

#ifdef ANALYSYS_READ
#ifdef ANALYSYS_WRITE
					g_GInfo[XOS_LocalSystem->m_nCurrentCpu].g_IoMonitor->AddTotalLazySample(m_nTotalActiveSession, CurrentTimeTick - nWaitTimeTotal);
#endif
#endif
				}
			}
			else
			{
				do
				{
					if (m_EchoProcess.GetRequest() == false)
					{
						XOS_Wait_P(m_EchoProcess.m_pWaitEvent);
					}
					else
					{
						break;
					}
				} while (true);

				SyncCall(Send, static_cast<_MyTcpType*>(this)->send((unsigned char *)m_EchoProcess.m_pRequest,
					m_EchoProcess.m_nRequestLen), 6000);
				ErrorProcess(bServerMode ? _T("Server Send") : _T("Client Send"), m_nID, DEVICE_ERROR_WRITE_OK);

#ifdef ANALYSYS_WRITE
				if (!bServerMode)
				{
					g_GInfo[XOS_LocalSystem->m_nCurrentCpu].g_IoMonitor->AddWriteLazySample(m_nWriteActiveSession, CurrentTimeTick - nWaitTime);
				}
#endif

				nWaitTime = CurrentTimeTick;
				//XOS_Sleep_Name(ClientRecvSleep, 37);
				SyncCall(Recv, static_cast<_MyTcpType*>(this)->recv(), 6000);
				ErrorProcess(bServerMode ? _T("Server Recv") : _T("Client Recv"), m_nID, DEVICE_ERROR_READ_OK);
				//g_GInfo[XOS_LocalSystem->m_nCurrentCpu].g_nClientCompleteCount++;

				m_EchoProcess.ProcessHandle(static_cast<_MyTcpType*>(this)->m_pRecvBuffer,
					win_min(static_cast<_MyTcpType*>(this)->m_nRecvLen, sizeof(MTU)));

				if (!bServerMode)
				{
					g_GInfo[XOS_LocalSystem->m_nCurrentCpu].g_IoMonitor->AddIoCount();
#ifdef ANALYSYS_READ
					g_GInfo[XOS_LocalSystem->m_nCurrentCpu].g_IoMonitor->AddReadLazySample(m_nReadActiveSession, CurrentTimeTick - nWaitTime);

#endif

#ifdef ANALYSYS_READ
#ifdef ANALYSYS_WRITE
					g_GInfo[XOS_LocalSystem->m_nCurrentCpu].g_IoMonitor->AddTotalLazySample(m_nTotalActiveSession, CurrentTimeTick - nWaitTimeTotal);
#endif
#endif
				}
			}

		}


		SyncCall(CloseSocket, static_cast<_MyTcpType*>(this)->closesocket(), 6000);
		ErrorProcess(bServerMode ? _T("Server closesocket") : _T("Client closesocket"), m_nID, DEVICE_ERROR_SESSION_FAIL);


	} while (false);

	g_GInfo[XOS_LocalSystem->m_nCurrentCpu].g_nCurrentClientLinkCount--;

}
OPTIMIZE_OFF_END


//#ifdef _LINUX_
#include "EchoModelTemplate.h"
//#endif