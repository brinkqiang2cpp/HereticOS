#pragma once


//#define Error_Transfer Error
#define Error_Transfer 

//工作模式 
#define TEST_MODE_LOOPBACK			1	//Client Server 都建立在本地核形成回环访问
#define TEST_MODE_BALANCED			2	//通过虚拟交换机将Client Server 均衡到不同CPU核
//#define TEST_MODE TEST_MODE_LOOPBACK
#define TEST_MODE TEST_MODE_BALANCED

#define HALF_DUPLEX_MODE		//半双工
#define PERFORMANCE_MODE		//带一个sprintf序列化负载
#define TCP_PLAYLOAD_SIZE 150
TCHAR szServerSend[TCP_PLAYLOAD_SIZE];
TCHAR szClientSend[TCP_PLAYLOAD_SIZE];


#ifdef WAIT_IO_TIMEOUT
#define SyncCall(_Name,_IoCode,_WaitTime) IoCall_WaitTimeOut(_Name,_IoCode,_WaitTime)
#else
#define SyncCall(_Name,_IoCode,_WaitTime) IoCall_Name(_Name,_IoCode)
#endif

template<int nParentHereticThreadType>
class  RecvTest : public HereticThread<RecvTest<nParentHereticThreadType>>
{
public:
	typedef void(*StateTransitionFunctionT)(void * pContext, TcpGroupState state, unsigned char * pPacket, unsigned int nLen);
	ULONGLONG i;
	RecvTest() { i = 0; };
	~RecvTest() {};
	void Init() {
		srand(1234);
		
	};
	void Close() {};
	void Loop(void * pContext = NULL);
	//static ThreadLocalStaticObject<ThreadLocalObj> as;
	//OPTIMIZE_CLOSE_BEGIN
	

private:
	//}CACHE_ALIGN_END;
};

//RecvTest::StateMachineTableT m_StateTransitionTab;
//RecvTest::TcpConfigT m_config;
template<int nParentHereticThreadType>
class  SendTest : public HereticThread<SendTest<nParentHereticThreadType>>
{
public:
	//USING_ALIGNEDMEM;
	ULONGLONG i;
	
	SendTest() { i = 0; 
	};
	~SendTest() {};
	void Init() {};
	void Close() {};
	void Loop(void * pContext = NULL);
	//static ThreadLocalStaticObject<ThreadLocalObj> as;
	//OPTIMIZE_CLOSE_BEGIN
	

private:
	//}CACHE_ALIGN_END;
};


unsigned int g_nClientCount = 0;
unsigned int g_nServerCount = 0;

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
		m_nClientID=g_nClientCount++;
	};
	void Close() {};
	//static ThreadLocalStaticObject<ThreadLocalObj> as;
	//OPTIMIZE_CLOSE_BEGIN
	
	void Loop(void * pContext = NULL);

private:
	//}CACHE_ALIGN_END;
};
/*

*/
typedef TCPServer<RecvTest<HereticThread_Type_TcpServer>, SendTest<HereticThread_Type_TcpServer>, ContextServer, DefTCPConfig> TCPServerTest;
typedef TCPClient<RecvTest<HereticThread_Type_TcpClient>, SendTest<HereticThread_Type_TcpClient>, ContextClient, DefTCPConfig> TCPClientTest;

TCPServerTest * g_TcpServer;

//template<typename _T>
struct GetParentT
{
	typedef TCPServerTest Rst;
};

/*
template<typename T>
void printfclass(T * pObj)
{

	TCPServerTest* p = static_cast< TCPServerTest*>(pObj);
	Error(_T("%s i(%u %u) %p %p %p %p size(%u)\n"),
		typeid(T).name(),
		p->i, pObj->i,
		pObj, p,
		static_cast< TCPServerTest::RecvHereticThreadT*>(p),
		static_cast< TCPServerTest::SendHereticThreadT*>(p),
		sizeof(TCPServerTest));
}
*/
OPTIMIZE_OFF
void ContextClient::Loop(void * pContext)
{
	USING_HERETICOS_THREAD
	SyncCall(ContextClientConnect,static_cast<TCPClientTest*>(this)->connect(), 6000);
	unsigned int nError = GetLastError();
	
	if (nError == DEVICE_ERROR_SESSION_OK) {
		Error(_T("Client(%u) connect ok"),m_nClientID);
	}
	else if (nError == DEVICE_ERROR_TIME_OUT)
	{
		Error(_T("Client(%u) connect time out"), m_nClientID);
	}
	else
	{
		Error(_T("Client(%u) connect fail %d"), m_nClientID,nError);
		PrintDeviceError();
	}
	static_cast<TCPClientTest*>(this)->m_pTCPManager->ReOptimize();

	for (;;)
	{
		nWaitTime = GetTickCount64();
		XOS_Sleep((4000 + (rand() % 3000)));
		i++;
		{
			TCPClientTest * m_pParent = (TCPClientTest *)static_cast<TCPClientTest*>(this);

			Error(_T("Client-%d-%u i=%llu time(%llu) Recv(%llu %llu) Send(%llu %llu) IOPS(R=%llu S=%llu)Kiops"),
				XOS_LocalSystem->m_nCurrentCpu,
				m_nClientID, i, (GetTickCount64() + 1 - nWaitTime),
				static_cast<TCPClientTest::RecvHereticThreadT*>(m_pParent)->i,
				static_cast<TCPClientTest::RecvHereticThreadT*>(m_pParent)->i - m_nRecvLastCount,
				static_cast<TCPClientTest::SendHereticThreadT*>(m_pParent)->i,
				static_cast<TCPClientTest::SendHereticThreadT*>(m_pParent)->i - m_nSendLastCount,
				(static_cast<TCPClientTest::RecvHereticThreadT*>(m_pParent)->i - m_nRecvLastCount) / (GetTickCount64() + 1 - nWaitTime),
				(static_cast<TCPClientTest::SendHereticThreadT*>(m_pParent)->i - m_nSendLastCount) / (GetTickCount64() + 1 - nWaitTime)
			);
			m_nRecvLastCount = static_cast<TCPClientTest::RecvHereticThreadT*>(m_pParent)->i;
			m_nSendLastCount = static_cast<TCPClientTest::SendHereticThreadT*>(m_pParent)->i;

		}
	}
	Error(_T("Client(%u) Loop over"), m_nClientID);
	SyncCall(ContextClientCloseSocket, static_cast<TCPClientTest*>(this)->closesocket(),6000);
	Error(_T("Client(%u) closesocket ok"), m_nClientID);

}
OPTIMIZE_OFF_END

OPTIMIZE_OFF
void ContextServer::Loop(void * pContext)
{
	USING_HERETICOS_THREAD

		for (;;)
		{
			nWaitTime = GetTickCount64();
			XOS_Sleep((3000+(rand()%3000)));
			i++;
			{
				TCPServerTest * m_pParent = (TCPServerTest *)static_cast<TCPServerTest*>(this);

				Error(_T("Server-%d-%u i=%llu time(%llu) Recv(%llu %llu) Send(%llu %llu) IOPS(R=%llu S=%llu)Kiops"), 
					XOS_LocalSystem->m_nCurrentCpu,
					m_nServerID,i, (GetTickCount64() + 1 - nWaitTime),
					static_cast<TCPServerTest::RecvHereticThreadT*>(m_pParent)->i,
					static_cast<TCPServerTest::RecvHereticThreadT*>(m_pParent)->i - m_nRecvLastCount,
					static_cast<TCPServerTest::SendHereticThreadT*>(m_pParent)->i,
					static_cast<TCPServerTest::SendHereticThreadT*>(m_pParent)->i - m_nSendLastCount,
					(static_cast<TCPServerTest::RecvHereticThreadT*>(m_pParent)->i - m_nRecvLastCount)/ (GetTickCount64() + 1 - nWaitTime),
					(static_cast<TCPServerTest::SendHereticThreadT*>(m_pParent)->i - m_nSendLastCount)/ (GetTickCount64() + 1 - nWaitTime)
					);
				m_nRecvLastCount = static_cast<TCPServerTest::RecvHereticThreadT*>(m_pParent)->i;
				m_nSendLastCount = static_cast<TCPServerTest::SendHereticThreadT*>(m_pParent)->i;
				

			}
		}


}
OPTIMIZE_OFF_END

OPTIMIZE_OFF
template<>
void RecvTest<HereticThread_Type_TcpServer>::Loop(void * pContext)
{
	USING_HERETICOS_THREAD

	for (;;)
	{
		
		if (static_cast<TCPServerTest*>(this)->CanIo())
		{
			SyncCall(ServerRecv, static_cast<TCPServerTest*>(this)->recv(), 6000);
			{
				unsigned int nError = GetLastError();
				if (nError == DEVICE_ERROR_READ_OK) {
					i++;
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
			XOS_Switch();

		}
		else
		{
			Error(_T("Server recv wait ESTABLISED this=%p\r\n"), this);
			XOS_Sleep_Name(ServerRecvTestSleep, (3000));
		}
		

	}
	Error(_T("Test Loop End...\r\n"));

}
template<>
void RecvTest<HereticThread_Type_TcpClient>::Loop(void * pContext)
{
	USING_HERETICOS_THREAD

		for (;;)
		{
			
			
			{
				if (static_cast<TCPClientTest*>(this)->CanIo())
				{
					SyncCall(ClientRecv, static_cast<TCPClientTest*>(this)->recv(), 6000);
					{
						unsigned int nError = GetLastError();
						if (nError == DEVICE_ERROR_READ_OK) {
							i++;
							Error_Transfer(_T("Client recv ok  %s\r\n"), static_cast<TCPClientTest*>(this)->m_pRecvBuffer);
						}
						else if (nError == DEVICE_ERROR_TIME_OUT)
						{
							Error(_T("Client recv time out\r\n"));
						}
						else
						{
							Error(_T("Client recv fail %d\r\n"), nError);
							PrintDeviceError();
						}
					}
					XOS_Switch();
					
				}
				else
				{
					Error(_T("Client recv wait ESTABLISED \r\n"));
					XOS_Sleep_Name(ClientRecvTestSleep, (3000));
				}
			}
			
			
		}
	Error(_T("Test Loop End...\r\n"));

}

OPTIMIZE_OFF_END



OPTIMIZE_OFF
template<>
void SendTest<HereticThread_Type_TcpServer>::Loop(void * pContext)
{
	USING_HERETICOS_THREAD

		for (;;)
		{
			
			
			if (static_cast<TCPServerTest*>(this)->CanIo())
			{
#ifdef HALF_DUPLEX_MODE
				XOS_Sleep_Name(DUPLEX_MODE, (3000));
#else
#ifndef PERFORMANCE_MODE
				sprintf_t(szServerSend, _T("S-%d"), i);
#endif
				SyncCall(ServerSend, static_cast<TCPServerTest*>(this)->send((unsigned char *)szServerSend, TCP_PLAYLOAD_SIZE), 6000);
				{
					unsigned int nError = GetLastError();
					if (nError == DEVICE_ERROR_WRITE_OK) {
						i++;
						Error_Transfer(_T("Server Send ok %s this=%p\r\n"), szServerSend, this);
					}
					else if (nError == DEVICE_ERROR_TIME_OUT)
					{
						Error(_T("Server Send time out %s this=%p\r\n"), szServerSend, this);
					}
					else
					{
						Error(_T("Server Send fail %d %s this=%p\r\n"), nError, szServerSend,this);
						PrintDeviceError();
					}
				}
#endif
				
			}
			else
			{
				Error(_T("Server Send wait ESTABLISED i=%d this=%p\r\n"),i,this);
				XOS_Sleep_Name(ServerSendTestSleep, (3000));
			}
			XOS_Switch();
			
		}

}

template<>
void SendTest<HereticThread_Type_TcpClient>::Loop(void * pContext)
{
	USING_HERETICOS_THREAD

		for (;;)
		{
			
			
			if (static_cast<TCPClientTest*>(this)->CanIo())
			{
#ifndef PERFORMANCE_MODE
				sprintf_t(szClientSend, _T("C-%d"), i);
#endif
				SyncCall(ClientSend,static_cast<TCPClientTest*>(this)->send((unsigned char *)szClientSend, TCP_PLAYLOAD_SIZE), 6000);
				{
					unsigned int nError = GetLastError();
					if (nError == DEVICE_ERROR_WRITE_OK) {
						i++;
						Error_Transfer(_T("Client Send ok %s this=%p\r\n"), szClientSend, this);
					}
					else if (nError == DEVICE_ERROR_TIME_OUT)
					{
						Error(_T("Client Send time out %s this=%p\r\n"), szClientSend, this);
					}
					else
					{
						Error(_T("Client Send fail %d %s this=%p\r\n"), szClientSend, this);
						PrintDeviceError();
					}
				}

			}
			else
			{
				Error(_T("Client send wait ESTABLISED \r\n"));
				XOS_Sleep_Name(ClientSendTestSleep, (3000));
			}
			XOS_Switch();
			
			
		}

}
OPTIMIZE_OFF_END

void TestTCP()
{
	//VirtualSwitchNICRouteConfig RouteConfig = { 0,0 };
	//XOS_T::CXOSLocalSystemT::LocalEnvContextT::VirtualSwitchT::GetInstance().BindNic(RouteConfig, XOS_LocalSystem->m_LocalEnv.m_NicAdapt);
#if (TEST_MODE==TEST_MODE_LOOPBACK)

	XOS_T::CXOSLocalSystemT::LocalEnvContextT::VirtualSwitchT::GetInstance().BindNicToLoopBack(
		XOS_LocalSystem->m_LocalEnv.m_NicAdapt, ((XOS_LocalSystem->m_nCurrentCpu-1)/ HT_JMP));
	XOS_LocalSystem->m_LocalEnv.tcpmgr->BindServer<TCPServerTest>(_T("192.168.1.1"), X86Short(80));
	XOS_LocalSystem->m_LocalEnv.tcpmgr->CreateClient<TCPClientTest>(IP_INT(192, 168, 1, 2), IP_INT(192, 168, 1, 1), X86Short(80), X86Short(81));
	Error(_T("Create Client Server on core(%d)"), XOS_LocalSystem->m_nCurrentCpu);
	
	
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
		/*
		XOS_LocalSystem->m_LocalEnv.tcpmgr->CreateClient<TCPClientTest>(IP_INT(192, 168, 1, 2), IP_INT(192, 168, 1, 1), X86Short(80), X86Short(83));
		Error(_T("Create Client port 83 on core(%d)"), XOS_LocalSystem->m_nCurrentCpu);

		XOS_LocalSystem->m_LocalEnv.tcpmgr->CreateClient<TCPClientTest>(IP_INT(192, 168, 1, 2), IP_INT(192, 168, 1, 1), X86Short(80), X86Short(84));
		Error(_T("Create Client port 84 on core(%d)"), XOS_LocalSystem->m_nCurrentCpu);
		*/
	}

#endif

	
	//XOS_LocalSystem->m_LocalEnv.tcpmgr->CreateClient<TCPClientTest>(IP_INT(192, 168, 1, 2), IP_INT(192, 168, 1, 1), X86Short(80), X86Short(82));
}