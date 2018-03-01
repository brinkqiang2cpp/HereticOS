#pragma once


//#define Error_Transfer Error
#define Error_Transfer 

//工作模式 
#define TEST_MODE_LOOPBACK			1	//Client Server 都建立在本地核形成回环访问
#define TEST_MODE_BALANCED			2	//通过虚拟交换机将Client Server 均衡到不同CPU核
#define TEST_MODE TEST_MODE_LOOPBACK
//#define TEST_MODE TEST_MODE_BALANCED
//#define QOS_SEND_CTL			//控制每链接发包规模
//#define HALF_DUPLEX_MODE		//半双工
#define CONCURRENT_LINK_COUNT	600*10000
#define PERFORMANCE_MODE		//带一个sprintf序列化负载
#define TCP_PLAYLOAD_SIZE 150
TCHAR szServerSend[TCP_PLAYLOAD_SIZE];
TCHAR szClientSend[TCP_PLAYLOAD_SIZE];

//#define WAIT_IO_TIMEOUT
#ifdef WAIT_IO_TIMEOUT
#define SyncCall(_Name,_IoCode,_WaitTime) IoCall_WaitTimeOut(_Name,_IoCode,_WaitTime)
#else
#define SyncCall(_Name,_IoCode,_WaitTime) IoCall_Name(_Name,_IoCode)
#endif




unsigned int g_nClientCount = 0;
unsigned int g_nServerCount = 0;

ULONGLONG g_nClientCompleteCount = 0;
ULONGLONG g_nServerCompleteCount = 0;
ULONGLONG g_nCurrentLinkCount = 0;

#include "Statistics.h"

class IoMonitor : public HereticThread<IoMonitor>
{
public:
	volatile ULONGLONG m_nLastAddIoCount;
	volatile ULONGLONG m_nAddCount;
	volatile ULONGLONG m_nAddIoCount;
	typedef LONGLONG _INT;
	typedef double _FLOAT;
	volatile ULONGLONG nCurrentTimeTick, nBeginTimeTick;
	
	Statistics<_INT, _FLOAT>	m_ReadIoStatistics;
	Statistics<_INT, _FLOAT>	m_WriteIoStatistics;
	Statistics<_INT, _FLOAT>	m_TotalIoStatistics;

	Statistics<_INT, _FLOAT>	m_ReadIoStatisticsSnapshot;
	Statistics<_INT, _FLOAT>	m_WriteIoStatisticsSnapshot;
	Statistics<_INT, _FLOAT>	m_TotalIoStatisticsSnapshot;

	//CCriticalSectionStack m_ReadIoStatisticsLock;
	//CCriticalSectionStack m_WriteIoStatisticsLock;

	IoMonitor() {
		m_nAddCount = 0;
		m_nAddIoCount = 0;
		m_nLastAddIoCount = 0;
		
		ClearLazyStatistics();
	};
	~IoMonitor() {};

	void AddWriteLazySample(unsigned int & nSession,_INT nLazy)
	{
		//CAutoStackLock wlk(&m_WriteIoStatisticsLock);
		m_WriteIoStatistics.AddSample(nSession,nLazy);
		
	}
	void AddReadLazySample(unsigned int & nSession,_INT nLazy)
	{
		//CAutoStackLock wlk(&m_WriteIoStatisticsLock);
		m_ReadIoStatistics.AddSample(nSession,nLazy);

	}
	void AddTotalLazySample(unsigned int & nSession,_INT nLazy)
	{
		//CAutoStackLock wlk(&m_WriteIoStatisticsLock);
		m_TotalIoStatistics.AddSample(nSession,nLazy);

	}
	void ClearLazyStatistics()
	{
		//CAutoStackLock rlk(&m_ReadIoStatisticsLock);
		//CAutoStackLock wlk(&m_WriteIoStatisticsLock);
		m_ReadIoStatistics(0, 1000000, -1);
		m_WriteIoStatistics(0, 1000000, -1);
		m_TotalIoStatistics(0, 1000000, -1);
	}
	void GetLazyStatisticsSnapshot()
	{
		//CAutoStackLock rlk(&m_ReadIoStatisticsLock);
		//CAutoStackLock wlk(&m_WriteIoStatisticsLock);
		m_ReadIoStatisticsSnapshot = m_ReadIoStatistics;
		m_WriteIoStatisticsSnapshot = m_WriteIoStatistics;
		m_TotalIoStatisticsSnapshot = m_TotalIoStatistics;

	}
	//m_ReadIoStatistics.AddSample();
	
	void Add()
	{
#ifdef CORE_CONCURRENT
		InterlockedExchangeAdd(&m_nAddCount, 1);
		InterlockedExchangeAdd(&m_nMaxAddCount, 1);
#else
		m_nAddCount++;
#endif // THREAD_CONCURRENT


	}
	void AddIoCount()
	{
#ifdef CORE_CONCURRENT
		InterlockedExchangeAdd(&m_nAddIoCount, 1);
#else
		m_nAddIoCount++;
#endif // THREAD_CONCURRENT


	}
	void clear()
	{
#ifdef CORE_CONCURRENT
		//InterlockedDecrement(&m_nAddCount,m_nAddCount);
		InterlockedExchangeAdd(&m_nAddIoCount, -m_nAddIoCount);
#else
		m_nAddIoCount = 0;
#endif
	}

	void Dec()
	{
#ifdef CORE_CONCURRENT
		InterlockedDecrement(&m_nAddCount);
#else
		m_nAddCount--;
#endif
	}
	int Get()
	{
#ifdef CORE_CONCURRENT
		return InterlockedExchangeAdd(&m_nAddCount, 0);
#else
		return m_nAddCount;
#endif
	}
	int GetIo()
	{
#ifdef CORE_CONCURRENT
		return InterlockedExchangeAdd(&m_nAddIoCount, 0);
#else
		return m_nAddIoCount;
#endif
	}
	void Init()
	{
	}
	void Close() {};

	void Loop();
	
protected:

private:
};

#define MONITOR_LOOPS_LAZY 7
OPTIMIZE_OFF
void IoMonitor::Loop()
{

	USING_HERETICOS_THREAD;

	nBeginTimeTick = CurrentTimeTick;
	for (;;)
	{
		GetLazyStatisticsSnapshot();
		if (m_nAddIoCount)
		{
			Error(_T("IoMonitor UseTime %llu %llu CurCount(%llu) Ios=%llu Iops=%llukiops")
				_T("\r\nname\t\t\tActiveCount\t\tIoCount\t\t\t\tmin\t\t\tmax\t\t\tavg\t\t\tdvt\t\t\tsdvt")
				_T("\r\nSend\t\t\t%llu\t\t\t%llu\t\t\t%llu\t\t\t%llu\t\t\t%.2f\t\t\t%.2f\t\t\t%.2f")
				_T("\r\nRecv\t\t\t%llu\t\t\t%llu\t\t\t%llu\t\t\t%llu\t\t\t%.2f\t\t\t%.2f\t\t\t%.2f")
				_T("\r\nTotal\t\t\t%llu\t\t\t%llu\t\t\t%llu\t\t\t%llu\t\t\t%.2f\t\t\t%.2f\t\t\t%.2f"),
				CurrentTimeTick- nBeginTimeTick,CurrentTimeTick- nCurrentTimeTick,
				m_nAddCount, m_nAddIoCount, (m_nAddIoCount - m_nLastAddIoCount) / (CurrentTimeTick - nCurrentTimeTick),
				
				m_WriteIoStatisticsSnapshot.GetActiveCount(),m_WriteIoStatisticsSnapshot.GetCount(),
				m_WriteIoStatisticsSnapshot.m_nMinSampleDiff, m_WriteIoStatisticsSnapshot.m_nMaxSampleDiff,
				m_WriteIoStatisticsSnapshot.GetAverage(), m_WriteIoStatisticsSnapshot.GetDeviation(), m_WriteIoStatisticsSnapshot.GetStandardDeviation(),

				m_ReadIoStatisticsSnapshot.GetActiveCount(),m_ReadIoStatisticsSnapshot.GetCount(),
				m_ReadIoStatisticsSnapshot.m_nMinSampleDiff, m_ReadIoStatisticsSnapshot.m_nMaxSampleDiff,
				m_ReadIoStatisticsSnapshot.GetAverage(), m_ReadIoStatisticsSnapshot.GetDeviation(), m_ReadIoStatisticsSnapshot.GetStandardDeviation(),

				m_TotalIoStatisticsSnapshot.GetActiveCount(),m_TotalIoStatisticsSnapshot.GetCount(),
				m_TotalIoStatisticsSnapshot.m_nMinSampleDiff, m_TotalIoStatisticsSnapshot.m_nMaxSampleDiff,
				m_TotalIoStatisticsSnapshot.GetAverage(), m_TotalIoStatisticsSnapshot.GetDeviation(), m_TotalIoStatisticsSnapshot.GetStandardDeviation()
			);
			m_WriteIoStatisticsSnapshot.UpdataSession();
			m_ReadIoStatisticsSnapshot.UpdataSession();
			m_TotalIoStatisticsSnapshot.UpdataSession();
			nCurrentTimeTick = CurrentTimeTick;
			m_nLastAddIoCount = m_nAddIoCount;
			ClearLazyStatistics();
		}

		XOS_Sleep_Name(CreateClient, MONITOR_LOOPS_LAZY*1000);
	}

}
OPTIMIZE_OFF_END

IoMonitor * g_IoMonitor;


class  ContextServer : public HereticThread<ContextServer,1,4096*1024>
{
public:
	//USING_ALIGNEDMEM;

	//ULONGLONG m_nRecvLastCount;
	//ULONGLONG m_nSendLastCount;
	//ULONGLONG nWaitTime;
	unsigned int m_nServerID;
	ContextServer() {
	};
	~ContextServer() {};
	void Init() {
		m_nServerID = g_nServerCount++;
	};
	void Close() {};
	//static ThreadLocalStaticObject<ThreadLocalObj> as;
	//OPTIMIZE_CLOSE_BEGIN

	void Loop(void * pContext = NULL);

private:
	//}CACHE_ALIGN_END;
};

//class CACHE_ALIGN_HEAD ContextClient : public HereticThread<ContextClient>
class ContextClient : public HereticThread<ContextClient,2, 4096 * 1024>
{
public:
	//USING_ALIGNEDMEM;
	volatile ULONGLONG nWaitTime;
	volatile ULONGLONG nWaitTimeTotal;
	unsigned int m_nClientID;
	unsigned int m_nReadActiveSession;
	unsigned int m_nWriteActiveSession;
	unsigned int m_nTotalActiveSession;
	ContextClient() {
	};
	~ContextClient() {};
	void Init() {
		m_nClientID = g_nClientCount++;
		m_nReadActiveSession = m_nWriteActiveSession= m_nTotalActiveSession=0;
	};
	void Close() {};
	//static ThreadLocalStaticObject<ThreadLocalObj> as;
	//OPTIMIZE_CLOSE_BEGIN

	void Loop(void * pContext = NULL);

private:

	//}CACHE_ALIGN_END;
};

typedef TCPSigleThreadServer<ContextServer, DefTCPConfig> TCPServerTest;
typedef TCPSigleThreadClient<ContextClient, DefTCPConfig> TCPClientTest;

TCPServerTest * g_TcpServer;
ULONGLONG g_nRecvTimeOut, g_nSendTimeOut;

OPTIMIZE_OFF
void ContextClient::Loop(void * pContext)
{
	USING_HERETICOS_THREAD

	
	//for (;;)
	do
	{

#ifdef _TEST27
		Error(_T("Client(%u) connect entry %llu"), m_nClientID, g_nCurrentLinkCount);
		if (m_nClientID == 45)
		{
			
			static_cast<TCPClientTest*>(this)->m_TaskState.bTest27 = 1;
		}
#endif
		
		SyncCall(ContextClientConnect, static_cast<TCPClientTest*>(this)->connect(), 6000);
		{

			unsigned int nError = GetLastError();

			if (nError == DEVICE_ERROR_SESSION_OK) {
				Error_Transfer(_T("Client(%u) connect ok"), m_nClientID);
			}
			else if (nError == DEVICE_ERROR_TIME_OUT)
			{
				Error(_T("Client(%u) connect time out"), m_nClientID);
				break;
			}
			else
			{
				Error(_T("Client(%u) connect fail %d"), m_nClientID, nError);
				PrintDeviceError();
				break;
			}
		}

#ifdef _DEBUG
		Error(_T("Client(%u) connect entry1 %llu"), m_nClientID, g_nCurrentLinkCount);
#endif
		g_nCurrentLinkCount++;
		g_IoMonitor->Add();
		for (;;)
		{
			nWaitTimeTotal = nWaitTime = CurrentTimeTick;

#ifdef QOS_SEND_CTL
			XOS_Sleep_Name(ClientSendSleep, 76);
#endif
			SyncCall(ClientSend, static_cast<TCPClientTest*>(this)->send((unsigned char *)szClientSend, TCP_PLAYLOAD_SIZE), 6000);
			{
				unsigned int nError = GetLastError();
				if (nError == DEVICE_ERROR_WRITE_OK) {

					Error_Transfer(_T("Client Send ok %s this=%p\r\n"), szClientSend, this);
				}
				else if (nError == DEVICE_ERROR_TIME_OUT)
				{
					Error(_T("Client Send time out %s this=%p\r\n"), szClientSend, this);
					break;
				}
				else
				{
					Error(_T("Client Send fail %d %s this=%p\r\n"), nError, szClientSend, this);
					PrintDeviceError();
					break;
				}
			}
			g_IoMonitor->AddWriteLazySample(m_nWriteActiveSession,CurrentTimeTick - nWaitTime);
			nWaitTime = CurrentTimeTick;
			//XOS_Sleep_Name(ClientRecvSleep, 37);
			SyncCall(ClientRecv, static_cast<TCPClientTest*>(this)->recv(), 6000);
			{
				unsigned int nError = GetLastError();
				if (nError == DEVICE_ERROR_READ_OK) {
					g_nClientCompleteCount++;
					Error_Transfer(_T("Client recv ok %s this=%p\r\n"), static_cast<TCPClientTest*>(this)->m_pRecvBuffer, this);
				}
				else if (nError == DEVICE_ERROR_TIME_OUT)
				{
					Error(_T("Client recv time out this=%p\r\n"), this);
					break;
				}
				else
				{
					Error(_T("Client recv fail %d this=%p\r\n"), this);
					PrintDeviceError();
					break;
				}
			}
			g_IoMonitor->AddIoCount();
			g_IoMonitor->AddReadLazySample(m_nReadActiveSession,CurrentTimeTick - nWaitTime);

			g_IoMonitor->AddTotalLazySample(m_nTotalActiveSession,CurrentTimeTick - nWaitTimeTotal);
		}
		

		SyncCall(ClientCloseSocket, static_cast<TCPClientTest*>(this)->closesocket(), 6000);

		{
			unsigned int nError = GetLastError();
			if (nError == DEVICE_ERROR_SESSION_FAIL) {
				Error_Transfer(_T("Client closesocket ok this=%p\r\n"), this);
			}
			else if (nError == DEVICE_ERROR_TIME_OUT)
			{
				Error(_T("Client closesocket time out this=%p\r\n"), this);
			}
			else
			{
				Error(_T("Client closesocket fail %d this=%p\r\n"), this);
				PrintDeviceError();
			}
		}


	} while (false);

	g_nCurrentLinkCount--;
	

}
OPTIMIZE_OFF_END




OPTIMIZE_OFF
void ContextServer::Loop(void * pContext)
{
	USING_HERETICOS_THREAD

		//for (;;)
		{
			static_cast<TCPServerTest*>(this)->accept();
			
#ifdef _TEST27
	Error(_T("Server(%u) connect entry %llu"), m_nServerID, g_nCurrentLinkCount);
			if (static_cast<TCPServerTest*>(this)->m_PreSendHead.tcp_head.des_port == 0x7e00)
			{

				static_cast<TCPServerTest*>(this)->m_TaskState.bTest27 = 1;
			}
#endif
			for (;;)
			{

				SyncCall(ServerRecv, static_cast<TCPServerTest*>(this)->recv(), 6000);
				{
					unsigned int nError = GetLastError();
					if (nError == DEVICE_ERROR_READ_OK) {
						Error_Transfer(_T("Server recv ok %s this=%p\r\n"), static_cast<TCPServerTest*>(this)->m_pRecvBuffer, this);
					}
					else if (nError == DEVICE_ERROR_TIME_OUT)
					{
						Error(_T("Server recv time out this=%p\r\n"), this);
						break;
					}
					else
					{
						Error(_T("Server recv fail %d this=%p\r\n"), this);
						PrintDeviceError();
						break;
					}
				}
#ifdef _TEST27
				
				if (static_cast<TCPServerTest*>(this)->m_PreSendHead.tcp_head.des_port == 0x7e00)
				{
					Error(_T("Server(%u) connect entry1 %llu"), m_nServerID, g_nCurrentLinkCount);
					static_cast<TCPServerTest*>(this)->m_TaskState.bTest27 = 1;
				}
#endif
				memcpy(szServerSend, static_cast<TCPServerTest*>(this)->m_pRecvBuffer, win_min(static_cast<TCPServerTest*>(this)->m_nRecvLen, sizeof(szServerSend)));
				SyncCall(ServerSend, static_cast<TCPServerTest*>(this)->send((unsigned char *)szServerSend, TCP_PLAYLOAD_SIZE), 6000);
				{
					unsigned int nError = GetLastError();
					if (nError == DEVICE_ERROR_WRITE_OK) {
						g_nServerCompleteCount++;
						Error_Transfer(_T("Server Send ok %s this=%p\r\n"), szClientSend, this);
					}
					else if (nError == DEVICE_ERROR_TIME_OUT)
					{
						Error(_T("Server Send time out %s this=%p\r\n"), szClientSend, this);
						break;
					}
					else
					{
						Error(_T("Server Send fail %d %s this=%p\r\n"), nError, szClientSend, this);
						PrintDeviceError();
						break;
					}
				}
			}
			

			SyncCall(ServerCloseSocket, static_cast<TCPServerTest*>(this)->closesocket(), 6000);
			{
				unsigned int nError = GetLastError();
				if (nError == DEVICE_ERROR_SESSION_FAIL) {
					
					Error_Transfer(_T("Server closesocket ok this=%p\r\n"), this);
				}
				else if (nError == DEVICE_ERROR_TIME_OUT)
				{
					Error(_T("Server closesocket time out this=%p\r\n"), this);
				}
				else
				{
					Error(_T("Server closesocket fail %d this=%p\r\n"), this);
					PrintDeviceError();
				}
			}
		}


}
OPTIMIZE_OFF_END




class  WatchdogThread : public HereticThread<WatchdogThread>
{
public:
	//USING_ALIGNEDMEM;

	ULONGLONG m_nLastClientCompleteCount;
	ULONGLONG m_nLastServerCompleteCount;
	ULONGLONG i;
	ULONGLONG nWaitTime;
	WatchdogThread() {
	};
	~WatchdogThread() {};
	void Init() {
		i = m_nLastClientCompleteCount = m_nLastServerCompleteCount = 0;
	};
	void Close() {};
	//static ThreadLocalStaticObject<ThreadLocalObj> as;
	//OPTIMIZE_CLOSE_BEGIN

	void Loop(void * pContext = NULL);

private:
	//}CACHE_ALIGN_END;
};

OPTIMIZE_OFF
void WatchdogThread::Loop(void * pContext)
{
	USING_HERETICOS_THREAD

		for (;;)
		{
			nWaitTime = GetTickCount64();
			XOS_Sleep((13000 + (rand() % 3000)));
			i++;
			
			Error(_T("Watchdog-%d i=%llu time(%llu) LinkCount(%llu) Complete(C=%llu S=%llu) EchoIOPS(C=%llu S=%llu)Kiops"),
				XOS_LocalSystem->m_nCurrentCpu, i, (GetTickCount64() + 1 - nWaitTime), g_nCurrentLinkCount,
				g_nClientCompleteCount,g_nServerCompleteCount,
				(g_nClientCompleteCount - m_nLastClientCompleteCount) / (GetTickCount64() + 1 - nWaitTime),
				(g_nServerCompleteCount - m_nLastServerCompleteCount) / (GetTickCount64() + 1 - nWaitTime)
			);
			m_nLastClientCompleteCount = g_nClientCompleteCount;
			m_nLastServerCompleteCount = g_nServerCompleteCount;


		}


}
OPTIMIZE_OFF_END


class CACHE_ALIGN_HEAD TCPLinkCreaterThread : public HereticThread<TCPLinkCreaterThread>
{
public:
	//USING_ALIGNEDMEM;
	ULONGLONG m_nCurPos;
	ULONGLONG nWaitTime;
	TCPLinkCreaterThread() {
	};
	~TCPLinkCreaterThread() {};
	void Init() {
	};
	void Close() {};
	//static ThreadLocalStaticObject<ThreadLocalObj> as;
	//OPTIMIZE_CLOSE_BEGIN

	void Loop(void * pContext = NULL);

private:
	//}CACHE_ALIGN_END;
}CACHE_ALIGN_END;

OPTIMIZE_OFF
void TCPLinkCreaterThread::Loop(void * pContext)
{
	USING_HERETICOS_THREAD

	Error(_T("TCPLinkCreaterThread this->%p ClientSize=%d ServerSize=%d LinkCount=%u\n"),
		this, sizeof(TCPClientTest), sizeof(TCPServerTest), CONCURRENT_LINK_COUNT);
	nWaitTime = GetTickCount64();
	for (; m_nCurPos<CONCURRENT_LINK_COUNT;)
	{
		if ((m_nCurPos - g_nCurrentLinkCount) < 1000000)
		{
			unsigned int nIp = X86int(IP_INT(192, 168, 1, 2));
			nIp += (m_nCurPos / 60000);
			XOS_LocalSystem->m_LocalEnv.tcpmgr->CreateClient<TCPClientTest>(
				X86int(nIp),
				IP_INT(192, 168, 1, 1), X86Short(80),
				X86Short((m_nCurPos%60000)+81));
			m_nCurPos++;
		}
		else
		{
			//Error(_T("TCPLinkCreaterThread CurrentLinkCount=%u\n"), g_nCurrentLinkCount);
			//XOS_Sleep_Name(CreateClient, 300);
			
			XOS_Switch();
		}
	}
	XOS_Sleep_Name(CreateClientOK, 2000);
	//XOS_LocalSystem->m_LocalEnv.tcpmgr->ReOptimize();

	{
		unsigned int nIp = X86int(IP_INT(192, 168, 1, 2));
		nIp += (m_nCurPos / 60000);
		nIp = X86int(nIp);
		Error(_T("TCPLinkCreaterThread  CreateClientOK Used Time %llu ms LastIp=%u.%u.%u.%u\n"),
			GetTickCount64() - nWaitTime,
			((unsigned char*)&nIp)[0],
			((unsigned char*)&nIp)[1], 
			((unsigned char*)&nIp)[2],
			((unsigned char*)&nIp)[3]);
	}

	for (;;)
	{
		XOS_Sleep_Name(TCPLinkCreaterThreadWait, 3000);
	}

	ExitThread();

}
OPTIMIZE_OFF_END


void TestTCP()
{
	//VirtualSwitchNICRouteConfig RouteConfig = { 0,0 };
	//XOS_T::CXOSLocalSystemT::LocalEnvContextT::VirtualSwitchT::GetInstance().BindNic(RouteConfig, XOS_LocalSystem->m_LocalEnv.m_NicAdapt);
	CreateLocalHereticThread<WatchdogThread>(TRUE);
	g_IoMonitor = new IoMonitor;
	CreateLocalHereticThread(g_IoMonitor,TRUE);
#if (TEST_MODE==TEST_MODE_LOOPBACK)

	XOS_T::CXOSLocalSystemT::LocalEnvContextT::VirtualSwitchT::GetInstance().BindNicToLoopBack(
		XOS_LocalSystem->m_LocalEnv.m_NicAdapt, ((XOS_LocalSystem->m_nCurrentCpu-1)/ HT_JMP));
	XOS_LocalSystem->m_LocalEnv.tcpmgr->BindServer<TCPServerTest>(_T("192.168.1.1"), X86Short(80));
	CreateLocalHereticThread<TCPLinkCreaterThread>(TRUE);
	Error(_T("Create Server on core(%d)"), XOS_LocalSystem->m_nCurrentCpu);
	
	//XOS_LocalSystem->m_LocalEnv.tcpmgr->CreateClient<TCPClientTest>(IP_INT(192, 168, 1, 2), IP_INT(192, 168, 1, 1), X86Short(80), X86Short(82));
	//Error(_T("Create Client port 82 on core(%d)"), XOS_LocalSystem->m_nCurrentCpu);
#elif (TEST_MODE==TEST_MODE_BALANCED)
	unsigned int nPos = ((XOS_LocalSystem->m_nCurrentCpu - 1) / (HT_JMP*2))*2;
	if (((XOS_LocalSystem->m_nCurrentCpu-1)/ HT_JMP)%2 == 0)
	{
		VirtualSwitchNICRouteConfig RouteConfig = { nPos+0,nPos + 1 };
		XOS_T::CXOSLocalSystemT::LocalEnvContextT::VirtualSwitchT::GetInstance().BindNic(RouteConfig, XOS_LocalSystem->m_LocalEnv.m_NicAdapt);
		XOS_LocalSystem->m_LocalEnv.tcpmgr->BindServer<TCPServerTest>(_T("192.168.1.1"), X86Short(80));
		Error(_T("Create Server port 80 on core(%d)"), XOS_LocalSystem->m_nCurrentCpu);
	}
	else if (((XOS_LocalSystem->m_nCurrentCpu - 1) / HT_JMP) % 2 == 1)
	{
		VirtualSwitchNICRouteConfig RouteConfig = { nPos + 1,nPos + 0 };
		XOS_T::CXOSLocalSystemT::LocalEnvContextT::VirtualSwitchT::GetInstance().BindNic(RouteConfig, XOS_LocalSystem->m_LocalEnv.m_NicAdapt);
		CreateLocalHereticThread<TCPLinkCreaterThread>(TRUE);
		/**/
	}

#endif

	
	//XOS_LocalSystem->m_LocalEnv.tcpmgr->CreateClient<TCPClientTest>(IP_INT(192, 168, 1, 2), IP_INT(192, 168, 1, 1), X86Short(80), X86Short(82));
}