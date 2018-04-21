#pragma once

#include "EchoModel.h"
#include "event.h"
#include "HereticOSBus.h"
#define MSG_BODY_SIZE 10
#define MSG_MAX_COUNT 10

#define PRODUCER_TRYMISS_QOS			//开启生产者流控，开启后生产者入队失败后会阻塞生产者进入BUS等待队列

#define PERSISTENCE_VIRTUAL_COMPLATE	//持久化虚完成


enum WaitProgress
{
	PushQueueOver = 0x0,			//进入队列就返回
	PersistenceOver = 0x1,			//持久化结束返回
	ConsumerProcessOver = 0x2,		//等待消费者完成后返回
};

struct MsgAttribute
{
	unsigned int bNeedPersistence : 1;						//需要持久化
	unsigned int bNeedBroadcastToActiveConsumer : 1;		//广播给所有活跃的消费者
	unsigned int nWaitProgress:3;

};
template<unsigned int _nMsgSize, unsigned int _nMaxMsgCount>
struct Message
{

	enum MsgPar
	{
		nMsgSize=_nMsgSize,
		nMaxMsgCount= _nMaxMsgCount,
	};
	struct IDValue
	{
		ULONGLONG	IDType : 8;
		ULONGLONG	nID : 56;

	};
	IDValue	m_nProducerMsgID;				//当前包Producer起始ID
	IDValue	m_nGlobalMsgID;					//消息全局起始ID
	unsigned short 	m_nMsgCount;				//当前包消息数量
	MsgAttribute	m_MsgAttribute;				//消息属性
	unsigned char	m_MsgArray[MsgPar::nMaxMsgCount][MsgPar::nMsgSize];
};

typedef Message<MSG_BODY_SIZE, MSG_MAX_COUNT> DefaultMessageT;
typedef HereticEventQueue<DefaultMessageT*> DefaultEventT;
DefaultEventT g_ConsumerWaitEventQueue;

#ifdef PRODUCER_TRYMISS_QOS
DefaultEventT g_ProducerWaitEventQueue;
#endif

//构造HereticOSBus
#ifdef NOT_HAE_PERSISTENCE
enum MQState
{
	MSG_Can_Enqueue = 0,
	MSG_Can_Dequeue = 1,

};
static _tagTransformTable gMQStateTable[] = {
	{ MSG_Can_Enqueue ,MSG_Can_Dequeue },
{ MSG_Can_Dequeue ,MSG_Can_Enqueue },

};
typedef HereticOSBus<DefaultMessageT, 2, MessageQueueSize> HereticOSBusT;
HereticOSBusT g_HereticOSBus;


#else

enum MQState
{
	MSG_Can_Enqueue = 0,
	MSG_Can_Dequeue = 1,
	MSG_Can_Persistence = 2

};
static _tagTransformTable gMQStateTable[] = {
	{ MSG_Can_Enqueue ,MSG_Can_Dequeue },
{ MSG_Can_Dequeue ,MSG_Can_Persistence },
{ MSG_Can_Persistence,MSG_Can_Enqueue }

};
typedef HereticOSBus<DefaultMessageT, 3, MessageQueueSize> HereticOSBusT;
HereticOSBusT g_HereticOSBus;

#endif


enum RequestOperator
{
	PushMsg = 0x1,
	FindMsg,
	CancelMsg,
};

template<typename MsgT>
struct ProducerRequest
{
	RequestOperator m_Operator;
	ULONGLONG m_nTopic;
	MsgT m_Msg;
};

struct ProducerRespond
{
	enum ProducerError
	{
		RequestSucess=0x0,
		PushMsgFail,
		PersistenceFail,
	};
	ProducerError m_Error;
};

template<typename MsgT>
struct ConsumerPush
{
	RequestOperator m_Operator;
	ULONGLONG m_nTopic;
	MsgT m_Msg;
};

struct ConsumerPushRespond
{
	enum ConsumerError
	{
		ProcessSucess = 0x0,
		ProcessFail,
	};
	ConsumerError m_Error;
};


class ProducerWorkerProcess : public EchoProcessBase<DefaultEventT>
{
public:
	ProducerRequest<DefaultMessageT> m_ProducerRequest;
	ULONGLONG m_nProducerID;
	GInfo * m_pOwnInfo;

	ProducerWorkerProcess() {};
	~ProducerWorkerProcess() {};
	//Echo线程收到请求时调用
	bool ProcessHandle(void * pMessage, unsigned int nLen)
	{
		ProducerRespond * pRespond = (ProducerRespond*)pMessage;
		if (pRespond->m_Error == ProducerRespond::ProducerError::RequestSucess) {
			m_pOwnInfo->g_nProducerCompleteCount++;
			m_pOwnInfo->g_nProducerMsgCompleteCount+= m_ProducerRequest.m_Msg.m_nMsgCount;
		}
		else
		{
			m_pOwnInfo->g_nProducerErrorCount++;
		}
		return true;
	}
	//Echo线程发起请求时调用，返回false则表示Echo线程阻塞等待m_pWaitEvent
	bool GetRequest()
	{
		m_ProducerRequest.m_Msg.m_nProducerMsgID.nID++;
		//m_ProducerRequest.m_Msg.m_nMsgCount=
		return true;
	}
	//Echo线程回应时调用，返回false则表示Echo线程阻塞等待m_pWaitEvent
	bool GetRespond()
	{
		return false;
	}
	void init()
	{
		m_pOwnInfo = (GInfo *)&g_GInfo[XOS_LocalSystem->m_nCurrentCpu];
		m_nProducerID = ++m_pOwnInfo->g_nProducerWorkerID;
		m_ProducerRequest.m_Msg.m_nProducerMsgID.IDType = m_nProducerID;
		m_ProducerRequest.m_Msg.m_nProducerMsgID.nID = 0;
		m_ProducerRequest.m_Msg.m_nMsgCount = DefaultMessageT::MsgPar::nMaxMsgCount;
		m_ProducerRequest.m_Operator = RequestOperator::PushMsg;
		m_pRequest = &m_ProducerRequest;
		m_nRequestLen = sizeof(m_ProducerRequest);

		
	}
	void close()
	{
	}
private:

};

class ConsumerWorkerProcess : public EchoProcessBase<DefaultEventT>
{
public:
	ConsumerPushRespond m_ConsumerPushRespond;
	ULONGLONG m_nConsumerWorkerID;
	GInfo * m_pOwnInfo;
	ConsumerWorkerProcess() {};
	~ConsumerWorkerProcess() {};
	//Echo线程收到请求时调用
	bool ProcessHandle(void * pMessage, unsigned int nLen)
	{
		ConsumerPush<DefaultMessageT> * pPushMsg=(ConsumerPush<DefaultMessageT> *)pMessage;
		m_pOwnInfo->g_nConsumerCompleteCount++;
		m_pOwnInfo->g_nConsumerMsgCompleteCount += pPushMsg->m_Msg.m_nMsgCount;
		return true;
	}
	//Echo线程发起请求时调用，返回false则表示Echo线程阻塞等待m_pWaitEvent
	bool GetRequest()
	{
		return false;
	}
	//Echo线程回应时调用，返回false则表示Echo线程阻塞等待m_pWaitEvent
	bool GetRespond()
	{
		m_ConsumerPushRespond.m_Error = ConsumerPushRespond::ConsumerError::ProcessSucess;
		return true;
	}
	void init()
	{
		m_pOwnInfo = (GInfo *)&g_GInfo[XOS_LocalSystem->m_nCurrentCpu];
		m_nConsumerWorkerID=++m_pOwnInfo->g_nConsumerWorkerID;
		m_pRespond=&m_ConsumerPushRespond;
		m_nRespondLen = sizeof(m_ConsumerPushRespond);
		
	}
	void close()
	{
	}
private:

};


class ProducerBrokerProcess : public EchoProcessBase<DefaultEventT>
{
public:
	HereticOSBusT::Queue<MSG_Can_Enqueue> m_EnqueueBus;
	ProducerRespond m_ProducerRespond;
	ULONGLONG m_nProducerBrokerID;
	GInfo * m_pOwnInfo;
	struct StateCtl
	{
		unsigned int bNeedNextTryMiss:1;
		unsigned int bWaitTryMiss : 1;
	};
	StateCtl m_StateCtl;
	ProducerRequest<DefaultMessageT> m_TryMissRequest;
	ProducerBrokerProcess() {};
	~ProducerBrokerProcess() {};
	//Echo线程收到请求时调用
	bool ProcessHandle(void * pMessage, unsigned int nLen)
	{
		//入队
		ProducerRequest<DefaultMessageT> * pProducerRequest=(ProducerRequest<DefaultMessageT>*)pMessage;
		m_StateCtl.bWaitTryMiss = FALSE;
		if (pProducerRequest->m_Operator == RequestOperator::PushMsg)
		{
			HereticOSBusT::MsgT * pCurrentMsgSlot= m_EnqueueBus.TryGet();
			if (pCurrentMsgSlot)
			{
				*pCurrentMsgSlot = pProducerRequest->m_Msg;
				pCurrentMsgSlot->m_nGlobalMsgID.IDType = m_nProducerBrokerID;
				pCurrentMsgSlot->m_nGlobalMsgID.nID = m_pOwnInfo->g_nBrokerMsgCompleteCount;
				m_pOwnInfo->g_nBrokerCompleteCount++;
				m_pOwnInfo->g_nBrokerMsgCompleteCount += pProducerRequest->m_Msg.m_nMsgCount;
				m_ProducerRespond.m_Error = ProducerRespond::ProducerError::RequestSucess;
				m_EnqueueBus.Complate();
			}
			else
			{
#ifdef PRODUCER_TRYMISS_QOS
				m_StateCtl.bWaitTryMiss =TRUE;
				m_StateCtl.bNeedNextTryMiss = TRUE;
				m_TryMissRequest = *pProducerRequest;
				m_pOwnInfo->g_nProducerMissCount++;
				return false;
#else
				m_ProducerRespond.m_Error = ProducerRespond::ProducerError::PushMsgFail;
#endif
			}
		}
		return true;
	}
	//Echo线程发起请求时调用，返回false则表示Echo线程阻塞等待m_WaitEvent
	bool GetRequest()
	{
		return false;
	}
	//Echo线程回应时调用，返回false则表示Echo线程阻塞等待m_WaitEvent
	bool GetRespond()
	{
		//默认不等待结果立即返回。
		if (m_StateCtl.bWaitTryMiss == TRUE)
		{
			if (m_StateCtl.bNeedNextTryMiss)
			{
				m_StateCtl.bNeedNextTryMiss = FALSE;
				return false;
			}
			m_StateCtl.bWaitTryMiss = FALSE;
			return ProcessHandle(&m_TryMissRequest,sizeof(m_TryMissRequest));
		}
		return true;
	}
	void init()
	{
		
		m_pOwnInfo = (GInfo *)&g_GInfo[XOS_LocalSystem->m_nCurrentCpu];
		m_nProducerBrokerID = ++m_pOwnInfo->g_nProducerBrokerID;

		g_HereticOSBus.GetQueue(m_EnqueueBus);
		m_pRespond = &m_ProducerRespond;
		m_nRespondLen = sizeof(m_ProducerRespond);
#ifdef PRODUCER_TRYMISS_QOS
		m_pWaitEvent=&g_ProducerWaitEventQueue;
#endif
	}
	void close()
	{
	}
private:

};

class ConsumerBrokerProcess : public EchoProcessBase<DefaultEventT>
{
public:
	HereticOSBusT::Queue<MSG_Can_Dequeue> m_DequeueBus;
	ULONGLONG m_nConsumerBrokerID;
	GInfo * m_pOwnInfo;
	ConsumerPush<DefaultMessageT> m_PushRequest;

	ConsumerBrokerProcess() {};
	~ConsumerBrokerProcess() {};
	//Echo线程收到请求时调用
	bool ProcessHandle(void * pMessage, unsigned int nLen)
	{
		ConsumerPushRespond * pConsumerPushRespond = (ConsumerPushRespond*)pMessage;
		if (pConsumerPushRespond->m_Error == ConsumerPushRespond::ConsumerError::ProcessSucess)
		{
			m_pOwnInfo->g_nConsumerCompleteCount++;
			m_pOwnInfo->g_nConsumerMsgCompleteCount += m_PushRequest.m_Msg.m_nMsgCount;
		}
		else
		{
			m_pOwnInfo->g_nConsumerErrorCount++;
			m_pOwnInfo->g_nConsumerErrorMsgCount += m_PushRequest.m_Msg.m_nMsgCount;
		}
		return true;
	}
	//Echo线程发起请求时调用，返回false则表示Echo线程阻塞等待m_pWaitEvent
	bool GetRequest()
	{
		HereticOSBusT::MsgT * pCurrentMsgSlot = m_DequeueBus.TryGet();
		if (pCurrentMsgSlot)
		{
			m_PushRequest.m_Msg = *pCurrentMsgSlot;
			m_PushRequest.m_Operator = RequestOperator::PushMsg;
			m_DequeueBus.Complate();
			return true;
		}
		//阻塞等待BrokerWatchThread
		return false;
	}
	//Echo线程回应时调用，返回false则表示Echo线程阻塞等待m_pWaitEvent
	bool GetRespond()
	{
		return false;
	}
	void init()
	{
		m_pOwnInfo = (GInfo *)&g_GInfo[XOS_LocalSystem->m_nCurrentCpu];
		m_nConsumerBrokerID = ++m_pOwnInfo->g_nConsumerBrokerID;
		//设置等待事件
		m_pWaitEvent = &g_ConsumerWaitEventQueue;
		g_HereticOSBus.GetQueue(m_DequeueBus);
		m_pRequest = &m_PushRequest;
		m_nRequestLen = sizeof(m_PushRequest);
	}
	void close()
	{
	}
private:

};

//typedef EchoServer<ProducerBrokerProcess>::TcpServerT ProducerBrokerT;

#ifdef _LINUX_
//LINUX GCC下需要手动展开模板...
typedef Echo1 < ProducerBrokerProcess, true, false > ::TcpServerT ProducerBrokerT;
typedef	Echo2<ConsumerBrokerProcess, true, true>::TcpServerT	ConsumerBrokerT;

typedef Echo3<ProducerWorkerProcess, false, false,true>::TcpClientT ProducerWorkerT;
typedef Echo4<ConsumerWorkerProcess, false, true>::TcpClientT ConsumerWorkerT;
#else

typedef Echo< ProducerBrokerProcess, true, false > ::TcpServerT ProducerBrokerT;
typedef	Echo<ConsumerBrokerProcess, true, true>::TcpServerT	ConsumerBrokerT;

typedef Echo<ProducerWorkerProcess, false, false, true>::TcpClientT ProducerWorkerT;
typedef Echo<ConsumerWorkerProcess, false, true>::TcpClientT ConsumerWorkerT;

#endif


//构造生产者，消费者
template<typename WorkerT, unsigned int nCreateCount, unsigned int nLocalIp2, unsigned short nPort>
class CACHE_ALIGN_HEAD SystemCreaterThread : public HereticThread<SystemCreaterThread<WorkerT, nCreateCount, nLocalIp2, nPort>>
{
public:
	typedef HereticThread<SystemCreaterThread<WorkerT, nCreateCount, nLocalIp2, nPort>> HereticThreadBaseT;
	//USING_ALIGNEDMEM;
	ULONGLONG nWaitTime;
	ULONGLONG nWaitTime1;
	ULONGLONG nLastClientLinkCount;
	ULONGLONG nLinkSpeed;
	ULONGLONG m_nCurRssCreatePos;
	ULONGLONG m_nCurCreatePos;
	SystemCreaterThread() {
	};
	~SystemCreaterThread() {};
	void Init() {
		nLinkSpeed = 100000;
		m_nCurRssCreatePos = 0;
		m_nCurCreatePos = 0;
	};
	void Close() {};
	//static ThreadLocalStaticObject<ThreadLocalObj> as;
	//OPTIMIZE_CLOSE_BEGIN

	void Loop(void * pContext = NULL);

private:
	//}CACHE_ALIGN_END;
}CACHE_ALIGN_END;


OPTIMIZE_OFF
template<typename WorkerT, unsigned int nCreateCount,unsigned int nLocalIp2,unsigned short nPort>
void SystemCreaterThread<WorkerT, nCreateCount, nLocalIp2, nPort>::Loop(void * pContext)
{
	USING_HERETICOS_THREAD;

	Error(_T("SystemCreaterThread this->%p WorkerName=%s WorkerSize=%d LinkCount=%u\n"),
			this, typeid(WorkerT).name(), sizeof(WorkerT), nCreateCount);
	nWaitTime  = CurrentTimeTick;
	nWaitTime1 = GetTickCount64ns();
	//g_GInfo[XOS_LocalSystem->m_nCurrentCpu].g_NicAdaptWatchdog->m_UpdataSpeed = 200000;
	XOS_LocalSystem->m_LocalEnv.m_NicAdapt.m_TxUpdataSpeed = 100000;
	for (; m_nCurCreatePos < nCreateCount;)
	{
		//if ((g_GInfo[XOS_LocalSystem->m_nCurrentCpu].g_nCurCreatePos - g_GInfo[XOS_LocalSystem->m_nCurrentCpu].g_nCurrentServerLinkCount) < 1000000)
		{
			unsigned int nIp = X86int(IP_INT(192, nLocalIp2, 1, 2));
#ifdef RSS_MODE
			nIp += (m_nCurRssCreatePos / 60000);
			for (;;)
			{

				if (XOS_LocalSystem->m_LocalEnv.m_NicAdapt.RssCheck(
					IP_INT(192, 168, 1, 1), X86int(nIp),
					X86Short(nPort), X86Short((m_nCurRssCreatePos % 60000) + 81)))
				{
#ifdef RSS_DEBUG
					Error(_T("Ip %08x %d hash=%08x"), X86int(nIp), (m_nCurRssCreatePos % 60000) + 81,
						XOS_LocalSystem->m_LocalEnv.m_NicAdapt.GetRssHash(
							IP_INT(192, 168, 1, 1), X86int(nIp),
							X86Short(nPort), X86Short((m_nCurRssCreatePos % 60000) + 81)));
#endif


					XOS_LocalSystem->m_LocalEnv.tcpmgr->CreateClient<TCPClientTest>(
						X86int(nIp),
						IP_INT(192, 168, 1, 1), X86Short(nPort),
						X86Short((m_nCurRssCreatePos % 60000) + 81));
					m_nCurRssCreatePos++;
					break;
				}
				m_nCurRssCreatePos++;
			}

#else

			nIp += (g_GInfo[XOS_LocalSystem->m_nCurrentCpu].g_nCurCreatePos / 60000);
			XOS_LocalSystem->m_LocalEnv.tcpmgr->CreateClient<WorkerT>(
				X86int(nIp),
				IP_INT(192, 168, 1, 1), X86Short(nPort),
				X86Short((g_GInfo[XOS_LocalSystem->m_nCurrentCpu].g_nCurCreatePos % 60000) + 81));

#endif
			m_nCurCreatePos++;
			g_GInfo[XOS_LocalSystem->m_nCurrentCpu].g_nCurCreatePos++;

			if ((CurrentTimeTick - nWaitTime1) > 3000)
			{
				nWaitTime1 = CurrentTimeTick;
			}

		}
	
	}
	
	

#ifndef _LINUX_
	for (;;)
	{
		XOS_Sleep_Name(TCPLinkCreaterThreadWait, 3000);
	}
#endif

	HereticThreadBaseT::ExitThread();

}
OPTIMIZE_OFF_END


//监视创建任务
template<unsigned int nCreateCount>
class CACHE_ALIGN_HEAD WatchCreateThread : public HereticThread<WatchCreateThread<nCreateCount>, 1, 1>
{
public:
	//USING_ALIGNEDMEM;
	typedef WatchCreateThread<nCreateCount> HereticThreadBaseT;
	GInfo * m_pOwnInfo;
	ULONGLONG nWaitTime;
	WatchCreateThread() {
	};
	~WatchCreateThread() {};
	void Init() {
		m_pOwnInfo = (GInfo *)&g_GInfo[XOS_LocalSystem->m_nCurrentCpu];
	};
	void Close() {};

	void Loop(void * pContext = NULL);

private:
	//}CACHE_ALIGN_END;
}CACHE_ALIGN_END;


OPTIMIZE_OFF
template<unsigned int nCreateCount>
void WatchCreateThread<nCreateCount>::Loop(void * pContext)
{
	USING_HERETICOS_THREAD;

	Error(_T("WatchCreateThread entry\n"));
	nWaitTime = CurrentTimeTick;

	for (; m_pOwnInfo->g_nCurrentClientLinkCount<nCreateCount;)
	{
		XOS_Sleep_Name(CreateClientOK, 2000);
		if ((CurrentTimeTick - nWaitTime) > 30000)
		{
			break;
		}
	}
		
	g_GInfo[XOS_LocalSystem->m_nCurrentCpu].g_bBeginEcho = TRUE;
	XOS_LocalSystem->m_LocalEnv.m_NicAdapt.m_TxUpdataSpeed = 280000;

	for (;;)
	{
		XOS_Sleep_Name(WatchCreateThread, 3000);
	}
	

}

OPTIMIZE_OFF_END


//BrokerWatchThread 用于监视Bus是否存在入队的消息，并触发消费者队列事件
class CACHE_ALIGN_HEAD BrokerWatchThread : public HereticThread<BrokerWatchThread,1,1>
{
public:
	//USING_ALIGNEDMEM;
#ifdef PRODUCER_TRYMISS_QOS
	HereticOSBusT::Queue<MSG_Can_Enqueue> m_EnqueueBus;
#endif
	HereticOSBusT::Queue<MSG_Can_Dequeue> m_DequeueBus;
	HereticOSBusT::MsgT * m_pCurrentMsg;
	BOOL m_nNeedSwitch;
	GInfo * m_pOwnInfo;
	BrokerWatchThread() {
	};
	~BrokerWatchThread() {};
	void Init() {
		
		g_HereticOSBus.init(&gMQStateTable[0], MSG_Can_Enqueue);
		g_HereticOSBus.GetQueue(m_DequeueBus);
#ifdef PRODUCER_TRYMISS_QOS
		g_HereticOSBus.GetQueue(m_EnqueueBus);
#endif
		m_nNeedSwitch = 0;
		m_pOwnInfo = (GInfo *)&g_GInfo[XOS_LocalSystem->m_nCurrentCpu];
	};
	void Close() {};
	void Loop(void * pContext = NULL);

private:
	//}CACHE_ALIGN_END;
}CACHE_ALIGN_END;


OPTIMIZE_OFF
void BrokerWatchThread::Loop(void * pContext)
{
	USING_HERETICOS_THREAD;

	Error(_T("BrokerWatchThread entry\n"));
	for (;;)
	{
		m_nNeedSwitch = 0;
		if (m_DequeueBus.TryGet())
		{
			if (g_ConsumerWaitEventQueue.SetEvent())
			{
				//转换消息状态到下一个状态
				
			}
			else
			{
				m_pOwnInfo->g_nConsumerMissCount++;
				m_nNeedSwitch |= 0x1;
			}
			
		}
		else {
			m_nNeedSwitch |= 0x1;
		}
#ifdef PRODUCER_TRYMISS_QOS
		if (m_EnqueueBus.TryGet())
		{
			if (g_ProducerWaitEventQueue.SetEvent() == false)
			{
				m_nNeedSwitch |= 0x2;
			}
		}
		else
		{
			m_nNeedSwitch |= 0x2;
		}
		if (m_nNeedSwitch==0x3) {
			//生产者 消费者都没有需要处理消息的线程时挂起线程
			//m_pOwnInfo->g_nMsgQueuePeakUsed = m_pOwnInfo->g_nBrokerCompleteCount - m_pOwnInfo->g_nPersistenceCompleteCount;
			XOS_Switch1();
		}
#else
		if (m_nNeedSwitch) {
			//m_pOwnInfo->g_nMsgQueuePeakUsed = m_pOwnInfo->g_nBrokerCompleteCount - m_pOwnInfo->g_nPersistenceCompleteCount;
			XOS_Switch1();
		}
#endif

	}

}

OPTIMIZE_OFF_END

//持久化工作线程
class CACHE_ALIGN_HEAD PersistenceWatchThread : public HereticThread<PersistenceWatchThread,1,1>
{
public:
	//USING_ALIGNEDMEM;
	HereticOSBusT::Queue<MSG_Can_Persistence> m_PersistenceBus;
	HereticOSBusT::MsgT * m_pCurrentMsg;
	GInfo * m_pOwnInfo;
	PersistenceWatchThread() {
	};
	~PersistenceWatchThread() {};
	void Init() {
		m_pOwnInfo = (GInfo *)&g_GInfo[XOS_LocalSystem->m_nCurrentCpu];
		g_HereticOSBus.GetQueue(m_PersistenceBus);
	};
	void Close() {};

	void Loop(void * pContext = NULL);

private:
	//}CACHE_ALIGN_END;
}CACHE_ALIGN_END;


OPTIMIZE_OFF
void PersistenceWatchThread::Loop(void * pContext)
{
	USING_HERETICOS_THREAD;

	Error(_T("PersistenceWatchThread entry\n"));
	for (;;)
	{
		m_pCurrentMsg = m_PersistenceBus.TryGet();
		if (m_pCurrentMsg)
		{
			//虚完成
			m_pOwnInfo->g_nPersistenceCompleteCount++;
			m_pOwnInfo->g_nPersistenceMsgCompleteCount += m_pCurrentMsg->m_nMsgCount;
			m_PersistenceBus.Complate();
		}
		else
		{
			XOS_Switch1();
		}
		
		
	}

}

OPTIMIZE_OFF_END


void TestTCP()
{
	//VirtualSwitchNICRouteConfig RouteConfig = { 0,0 };
	//XOS_T::CXOSLocalSystemT::LocalEnvContextT::VirtualSwitchT::GetInstance().BindNic(RouteConfig, XOS_LocalSystem->m_LocalEnv.m_NicAdapt);
	//CreateLocalHereticThread<WatchdogThread>(TRUE);


#if (TEST_MODE==TEST_MODE_LOOPBACK)
	
	
#elif (TEST_MODE==TEST_MODE_BALANCED)
#ifdef RSS_MODE
	if (XOS_LocalSystem->m_nCurrentCpu==3|| XOS_LocalSystem->m_nCurrentCpu == 5)
	{
		VirtualSwitchNICRouteConfig RouteConfig = { ((XOS_LocalSystem->m_nCurrentCpu - 1) / 2) - 1,1 };
#else
	if (XOS_LocalSystem->m_nCurrentCpu == 3)
	{
		VirtualSwitchNICRouteConfig RouteConfig = { 0,1 };
#endif
#if(NIC_TYPE==NIC_TYPE_NETMAP)
		VirtualSwitchNICRouteConfig RouteConfig = { 0,0 };
#else
		
#endif

		XOS_T::CXOSLocalSystemT::LocalEnvContextT::VirtualSwitchT::GetInstance().BindNic(RouteConfig, XOS_LocalSystem->m_LocalEnv.m_NicAdapt);
		g_ConsumerWaitEventQueue.init();
#ifdef PRODUCER_TRYMISS_QOS
		g_ProducerWaitEventQueue.init();
#endif
		

		CreateLocalHereticThread<BrokerWatchThread>(TRUE);
		CreateLocalHereticThread<PersistenceWatchThread>(TRUE);
		
		XOS_LocalSystem->m_LocalEnv.tcpmgr->BindServer<ProducerBrokerT>(_T("192.168.1.1"), X86Short(30));
		XOS_LocalSystem->m_pLocalHereticThreadManger->GrowPool<ProducerBrokerT>();
		XOS_LocalSystem->m_LocalEnv.tcpmgr->BindServer<ConsumerBrokerT>(_T("192.168.1.1"), X86Short(31));
		XOS_LocalSystem->m_pLocalHereticThreadManger->GrowPool<ConsumerBrokerT>();
		
		Error(_T("Create Server port 20 21 on core(%d)"), XOS_LocalSystem->m_nCurrentCpu);
	}

#ifdef RSS_MODE
	if (XOS_LocalSystem->m_nCurrentCpu == 7 || XOS_LocalSystem->m_nCurrentCpu == 9)
	{
		VirtualSwitchNICRouteConfig RouteConfig = { ((XOS_LocalSystem->m_nCurrentCpu - 1) / 2) - 1,0 };
#else
	if (XOS_LocalSystem->m_nCurrentCpu == 5)
	{ 
		VirtualSwitchNICRouteConfig RouteConfig = { 1,0 };
#endif
#if(NIC_TYPE==NIC_TYPE_NETMAP)
		VirtualSwitchNICRouteConfig RouteConfig = { 1,1 };
#else
		
#endif
		XOS_T::CXOSLocalSystemT::LocalEnvContextT::VirtualSwitchT::GetInstance().BindNic(RouteConfig, XOS_LocalSystem->m_LocalEnv.m_NicAdapt);
		XOS_LocalSystem->m_LocalEnv.tcpmgr->m_bServerMode = FALSE;
		
		CreateLocalHereticThread<SystemCreaterThread<ConsumerWorkerT, Client_Count_Consumer, 169, 31>>(TRUE);
		CreateLocalHereticThread<SystemCreaterThread<ProducerWorkerT, Client_Count_Producer,168, 30>>(TRUE);
		CreateLocalHereticThread<WatchCreateThread<Client_Count_Consumer+ Client_Count_Producer>>(TRUE);
		/**/
	}

#endif

	
	//XOS_LocalSystem->m_LocalEnv.tcpmgr->CreateClient<TCPClientTest>(IP_INT(192, 168, 1, 2), IP_INT(192, 168, 1, 1), X86Short(80), X86Short(82));
}