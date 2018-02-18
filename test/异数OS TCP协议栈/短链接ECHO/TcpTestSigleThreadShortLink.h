#pragma once


//#define Error_Transfer Error
#define Error_Transfer 

//工作模式 
#define TEST_MODE_LOOPBACK			1	//Client Server 都建立在本地核形成回环访问
#define TEST_MODE_BALANCED			2	//通过虚拟交换机将Client Server 均衡到不同CPU核
#define TEST_MODE TEST_MODE_LOOPBACK
//#define TEST_MODE TEST_MODE_BALANCED

//#define HALF_DUPLEX_MODE		//半双工
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

class  ContextServer : public HereticThread<ContextServer>
{
public:
	//USING_ALIGNEDMEM;

	ULONGLONG m_nRecvLastCount;
	ULONGLONG m_nSendLastCount;
	ULONGLONG i;
	ULONGLONG nWaitTime;
	unsigned int m_nServerID;
	ContextServer() {
	};
	~ContextServer() {};
	void Init() {
		i = m_nRecvLastCount = m_nSendLastCount = 0;
		m_nServerID = g_nServerCount++;
	};
	void Close() {};
	//static ThreadLocalStaticObject<ThreadLocalObj> as;
	//OPTIMIZE_CLOSE_BEGIN

	void Loop(void * pContext = NULL);

private:
	//}CACHE_ALIGN_END;
};


class  ContextClient : public HereticThread<ContextClient>
{
public:
	//USING_ALIGNEDMEM;
	ULONGLONG m_nRecvLastCount;
	ULONGLONG m_nSendLastCount;
	ULONGLONG i;
	ULONGLONG nWaitTime;
	unsigned int m_nClientID;
	ContextClient() {
	};
	~ContextClient() {};
	void Init() {
		i = m_nRecvLastCount = m_nSendLastCount = 0;
		m_nClientID = g_nClientCount++;
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
	for (;;)
	{
		SyncCall(ContextClientConnect, static_cast<TCPClientTest*>(this)->connect(), 6000);
		{

			unsigned int nError = GetLastError();

			if (nError == DEVICE_ERROR_SESSION_OK) {
				Error_Transfer(_T("Client(%u) connect ok"), m_nClientID);
			}
			else if (nError == DEVICE_ERROR_TIME_OUT)
			{
				Error(_T("Client(%u) connect time out"), m_nClientID);
			}
			else
			{
				Error(_T("Client(%u) connect fail %d"), m_nClientID, nError);
				PrintDeviceError();
			}
		}

		SyncCall(ClientSend, static_cast<TCPClientTest*>(this)->send((unsigned char *)szClientSend, TCP_PLAYLOAD_SIZE), 6000);
		{
			unsigned int nError = GetLastError();
			if (nError == DEVICE_ERROR_WRITE_OK) {
				
				Error_Transfer(_T("Client Send ok %s this=%p\r\n"), szClientSend, this);
			}
			else if (nError == DEVICE_ERROR_TIME_OUT)
			{
				Error(_T("Client Send time out %s this=%p\r\n"), szClientSend, this);
			}
			else
			{
				Error(_T("Client Send fail %d %s this=%p\r\n"), nError, szClientSend, this);
				PrintDeviceError();
			}
		}

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
			}
			else
			{
				Error(_T("Client recv fail %d this=%p\r\n"), this);
				PrintDeviceError();
			}
		}

		SyncCall(ClientCloseSocket, static_cast<TCPClientTest*>(this)->closesocket(), 6000);

		{
			unsigned int nError = GetLastError();
			if (nError == DEVICE_ERROR_SESSION_FAIL) {
				i++;
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


	}
	

}
OPTIMIZE_OFF_END

OPTIMIZE_OFF
void ContextServer::Loop(void * pContext)
{
	USING_HERETICOS_THREAD

		for (;;)
		{
			static_cast<TCPServerTest*>(this)->accept();
			SyncCall(ServerRecv, static_cast<TCPServerTest*>(this)->recv(), 6000);
			{
				unsigned int nError = GetLastError();
				if (nError == DEVICE_ERROR_READ_OK) {
					Error_Transfer(_T("Server recv ok %s this=%p\r\n"), static_cast<TCPServerTest*>(this)->m_pRecvBuffer, this);
				}
				else if (nError == DEVICE_ERROR_TIME_OUT)
				{
					Error(_T("Server recv time out this=%p\r\n"), this);
				}
				else
				{
					Error(_T("Server recv fail %d this=%p\r\n"), this);
					PrintDeviceError();
				}
			}
			memcpy(szServerSend, static_cast<TCPServerTest*>(this)->m_pRecvBuffer, win_min(static_cast<TCPServerTest*>(this)->m_nRecvLen,sizeof(szServerSend)));
			SyncCall(ServerSend, static_cast<TCPServerTest*>(this)->send((unsigned char *)szServerSend, TCP_PLAYLOAD_SIZE), 6000);
			{
				unsigned int nError = GetLastError();
				if (nError == DEVICE_ERROR_WRITE_OK) {

					Error_Transfer(_T("Server Send ok %s this=%p\r\n"), szClientSend, this);
				}
				else if (nError == DEVICE_ERROR_TIME_OUT)
				{
					Error(_T("Server Send time out %s this=%p\r\n"), szClientSend, this);
				}
				else
				{
					Error(_T("Server Send fail %d %s this=%p\r\n"), nError, szClientSend, this);
					PrintDeviceError();
				}
			}

			

			SyncCall(ServerCloseSocket, static_cast<TCPServerTest*>(this)->closesocket(), 6000);
			{
				unsigned int nError = GetLastError();
				if (nError == DEVICE_ERROR_SESSION_FAIL) {
					g_nServerCompleteCount++;
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
			XOS_Sleep((3000 + (rand() % 3000)));
			i++;
			
			Error(_T("Watchdog-%d i=%llu time(%llu) Complete(C=%llu S=%llu) EchoIOPS(C=%llu S=%llu)Kiops"),
				XOS_LocalSystem->m_nCurrentCpu, i, (GetTickCount64() + 1 - nWaitTime),
				g_nClientCompleteCount,g_nServerCompleteCount,
				(g_nClientCompleteCount - m_nLastClientCompleteCount) / (GetTickCount64() + 1 - nWaitTime),
				(g_nServerCompleteCount - m_nLastServerCompleteCount) / (GetTickCount64() + 1 - nWaitTime)
			);
			m_nLastClientCompleteCount = g_nClientCompleteCount;
			m_nLastServerCompleteCount = g_nServerCompleteCount;


		}


}
OPTIMIZE_OFF_END



void TestTCP()
{
	//VirtualSwitchNICRouteConfig RouteConfig = { 0,0 };
	//XOS_T::CXOSLocalSystemT::LocalEnvContextT::VirtualSwitchT::GetInstance().BindNic(RouteConfig, XOS_LocalSystem->m_LocalEnv.m_NicAdapt);
	CreateLocalHereticThread<WatchdogThread>(TRUE);
#if (TEST_MODE==TEST_MODE_LOOPBACK)

	XOS_T::CXOSLocalSystemT::LocalEnvContextT::VirtualSwitchT::GetInstance().BindNicToLoopBack(
		XOS_LocalSystem->m_LocalEnv.m_NicAdapt, ((XOS_LocalSystem->m_nCurrentCpu-1)/ HT_JMP));
	XOS_LocalSystem->m_LocalEnv.tcpmgr->BindServer<TCPServerTest>(_T("192.168.1.1"), X86Short(80));
	XOS_LocalSystem->m_LocalEnv.tcpmgr->CreateClient<TCPClientTest>(IP_INT(192, 168, 1, 2), IP_INT(192, 168, 1, 1), X86Short(80), X86Short(81));
	Error(_T("Create Client Server on core(%d)"), XOS_LocalSystem->m_nCurrentCpu);
	
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
		XOS_LocalSystem->m_LocalEnv.tcpmgr->CreateClient<TCPClientTest>(IP_INT(192, 168, 1, 2), IP_INT(192, 168, 1, 1), X86Short(80), X86Short(81));
		Error(_T("Create Client port 81 on core(%d)"), XOS_LocalSystem->m_nCurrentCpu);
		
		XOS_LocalSystem->m_LocalEnv.tcpmgr->CreateClient<TCPClientTest>(IP_INT(192, 168, 1, 2), IP_INT(192, 168, 1, 1), X86Short(80), X86Short(82));
		Error(_T("Create Client port 82 on core(%d)"), XOS_LocalSystem->m_nCurrentCpu);
		
		XOS_LocalSystem->m_LocalEnv.tcpmgr->CreateClient<TCPClientTest>(IP_INT(192, 168, 1, 2), IP_INT(192, 168, 1, 1), X86Short(80), X86Short(83));
		Error(_T("Create Client port 83 on core(%d)"), XOS_LocalSystem->m_nCurrentCpu);
		
		XOS_LocalSystem->m_LocalEnv.tcpmgr->CreateClient<TCPClientTest>(IP_INT(192, 168, 1, 2), IP_INT(192, 168, 1, 1), X86Short(80), X86Short(84));
		Error(_T("Create Client port 84 on core(%d)"), XOS_LocalSystem->m_nCurrentCpu);
		/**/
	}

#endif

	
	//XOS_LocalSystem->m_LocalEnv.tcpmgr->CreateClient<TCPClientTest>(IP_INT(192, 168, 1, 2), IP_INT(192, 168, 1, 1), X86Short(80), X86Short(82));
}