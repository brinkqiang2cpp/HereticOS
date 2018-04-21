#pragma once



#include "Statistics.h"

class CACHE_ALIGN_HEAD IoMonitor : public HereticThread<IoMonitor>
{
public:
	volatile ULONGLONG m_nLastAddIoCount;
	volatile ULONGLONG m_nAddCount;
	volatile ULONGLONG m_nAddIoCount;
	unsigned int m_nWatchCpuID;
	typedef LONGLONG _INT;
	typedef double _FLOAT;
	volatile ULONGLONG nCurrentTimeTick, nBeginTimeTick, nXosCurrentTimeTick;

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
		m_nWatchCpuID = 0;
		ClearLazyStatistics();
	};
	~IoMonitor() {};

	void AddWriteLazySample(unsigned int & nSession, _INT nLazy)
	{
		//CAutoStackLock wlk(&m_WriteIoStatisticsLock);
		m_WriteIoStatistics.AddSample(nSession, nLazy);

	}
	void AddReadLazySample(unsigned int & nSession, _INT nLazy)
	{
		//CAutoStackLock wlk(&m_WriteIoStatisticsLock);
		m_ReadIoStatistics.AddSample(nSession, nLazy);

	}
	void AddTotalLazySample(unsigned int & nSession, _INT nLazy)
	{
		//CAutoStackLock wlk(&m_WriteIoStatisticsLock);
		m_TotalIoStatistics.AddSample(nSession, nLazy);

	}
	void ClearLazyStatistics()
	{
		//CAutoStackLock rlk(&m_ReadIoStatisticsLock);
		//CAutoStackLock wlk(&m_WriteIoStatisticsLock);
		m_ReadIoStatistics(0, 1000000, -1);
		m_ReadIoStatistics.UpdataSession();
		m_WriteIoStatistics(0, 1000000, -1);
		m_WriteIoStatistics.UpdataSession();
		m_TotalIoStatistics(0, 1000000, -1);
		m_TotalIoStatistics.UpdataSession();
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
}CACHE_ALIGN_END;

#define MONITOR_LOOPS_LAZY 7

template<typename NicAdaptT, typename TcpMangerT>
class  NicAdaptWatchdog : public HereticThread<NicAdaptWatchdog<NicAdaptT, TcpMangerT>>
{
public:
	//USING_ALIGNEDMEM;
	typedef HereticThread<NicAdaptWatchdog<NicAdaptT, TcpMangerT>> HereticThreadBaseT;
	ULONGLONG m_nLastUpdataCount;
	ULONGLONG m_nUpdataCount;
	ULONGLONG nWaitTime;
	ULONGLONG nDiffTime;
	NicAdaptT * m_pAdapt;
	TcpMangerT * m_pTcpMgr;
	NicAdaptWatchdog() {
	};
	~NicAdaptWatchdog() {};
	void Init() {
		m_pTcpMgr = XOS_LocalSystem->m_LocalEnv.tcpmgr;

		m_pAdapt = &XOS_LocalSystem->m_LocalEnv.m_NicAdapt;

		m_nLastUpdataCount = m_nUpdataCount = 0;
		nDiffTime = 0;
		//XOS_LocalSystem->m_LocalEnv.m_NicAdapt.m_TxUpdataSpeed = 100000;
	};
	void Close() {};
	//static ThreadLocalStaticObject<ThreadLocalObj> as;
	//OPTIMIZE_CLOSE_BEGIN

	void Loop(void * pContext = NULL);

private:
	//}CACHE_ALIGN_END;
};



struct GInfo
{
	unsigned int g_nCurCreatePos;
	ULONGLONG	g_SwitchCount;
	ULONGLONG	g_TxSwitchCount = 0;
	unsigned int g_nClientCount = 0;
	unsigned int g_nServerCount = 0;

	ULONGLONG g_nClientCompleteCount = 0;
	ULONGLONG g_nServerCompleteCount = 0;
	ULONGLONG g_nCurrentServerLinkCount = 0;
	ULONGLONG g_nCurrentClientLinkCount = 0;

	//水母消息队列统计数据
	ULONGLONG g_nProducerCompleteCount = 0;
	ULONGLONG g_nConsumerCompleteCount = 0;
	ULONGLONG g_nBrokerCompleteCount = 0;	
	ULONGLONG g_nPersistenceCompleteCount = 0;
	ULONGLONG g_nTotalCompleteCount = 0;

	ULONGLONG g_nProducerMsgCompleteCount = 0;
	ULONGLONG g_nConsumerMsgCompleteCount = 0;
	ULONGLONG g_nBrokerMsgCompleteCount = 0;
	ULONGLONG g_nPersistenceMsgCompleteCount = 0;
	ULONGLONG g_nTotalMsgCompleteCount = 0;

	ULONGLONG g_nProducerErrorCount = 0;
	ULONGLONG g_nConsumerErrorCount = 0;
	ULONGLONG g_nBrokerErrorCount = 0;
	ULONGLONG g_nPersistenceErrorCount = 0;
	ULONGLONG g_nTotalErrorCount = 0;


	ULONGLONG g_nLastProducerCompleteCount = 0;
	ULONGLONG g_nLastConsumerCompleteCount = 0;
	ULONGLONG g_nLastBrokerCompleteCount = 0;
	ULONGLONG g_nLastPersistenceCompleteCount = 0;
	ULONGLONG g_nLastTotalCompleteCount = 0;

	ULONGLONG g_nLastProducerMsgCompleteCount = 0;
	ULONGLONG g_nLastConsumerMsgCompleteCount = 0;
	ULONGLONG g_nLastBrokerMsgCompleteCount = 0;
	ULONGLONG g_nLastPersistenceMsgCompleteCount = 0;
	ULONGLONG g_nLastTotalMsgCompleteCount = 0;
	
	ULONGLONG g_nLastProducerErrorCount = 0;
	ULONGLONG g_nLastConsumerErrorCount = 0;
	ULONGLONG g_nLastBrokerErrorCount = 0;
	ULONGLONG g_nLastPersistenceErrorCount = 0;
	ULONGLONG g_nLastTotalErrorCount = 0;

	ULONGLONG g_nProducerErrorMsgCount = 0;
	ULONGLONG g_nConsumerErrorMsgCount = 0;
	ULONGLONG g_nBrokerMsgErrorCount = 0;
	ULONGLONG g_nPersistenceMsgErrorCount = 0;
	ULONGLONG g_nTotalMsgErrorCount = 0;

	ULONGLONG g_nProducerMissCount = 0;
	ULONGLONG g_nLastProducerMissCount = 0;
	ULONGLONG g_nConsumerMissCount = 0;
	ULONGLONG g_nLastConsumerMissCount = 0;

	ULONGLONG g_nProducerWorkerID = 0;
	ULONGLONG g_nConsumerWorkerID = 0;
	ULONGLONG g_nProducerBrokerID = 0;
	ULONGLONG g_nConsumerBrokerID = 0;
	
	ULONGLONG g_nMsgQueuePeakUsed = 0;//峰值占用
	ULONGLONG g_nMsgQueueMaxPeakUsed = 0;//历史最大峰值占用
	BOOL g_bBeginEcho = FALSE;
	IoMonitor * g_IoMonitor;

};

volatile GInfo g_GInfo[16];// = { 0 };

OPTIMIZE_OFF
void IoMonitor::Loop()
{

	USING_HERETICOS_THREAD;

	nBeginTimeTick = CurrentTimeTick;
#ifdef NOT_USING_XOS_TICK
	nXosCurrentTimeTick = GetTickCount64_XOS();
#endif

	nCurrentTimeTick = CurrentTimeTick;
	for (;;)
	{
		GetLazyStatisticsSnapshot();
		//if (m_nAddIoCount)
		{
			if (g_XOSSystem.m_LocalSystemArray[0][m_nWatchCpuID - 1].m_bReady)
			{
				typename XOS_T::CXOSLocalSystemT* WatchLocalSystem = (XOS_T::CXOSLocalSystemT*)g_XOSSystem.m_LocalSystemArray[0][m_nWatchCpuID - 1].pLocalSystem;
				if (WatchLocalSystem->m_LocalEnv.m_NicAdapt.IsReady())
				{
					if (g_GInfo[XOS_LocalSystem->m_nCurrentCpu].g_nCurrentServerLinkCount < CONCURRENT_LINK_COUNT)
					{
						Error(_T("TCPLinkCreaterThread CurrentLinkCount=%llu %llu g_nCurCreatePos=%u g_SwitchCount=%llu UpdataSpeed=%llu TxSwitchCount=%llu \n"),
							g_GInfo[m_nWatchCpuID].g_nCurrentClientLinkCount, CONCURRENT_LINK_COUNT - g_GInfo[m_nWatchCpuID].g_nCurrentClientLinkCount,
							g_GInfo[m_nWatchCpuID].g_nCurCreatePos, g_GInfo[m_nWatchCpuID].g_SwitchCount,
							WatchLocalSystem->m_LocalEnv.m_NicAdapt.m_TxUpdataSpeed,
							g_GInfo[m_nWatchCpuID].g_TxSwitchCount
						);
						g_GInfo[m_nWatchCpuID].g_SwitchCount = 0;
						g_GInfo[m_nWatchCpuID].g_TxSwitchCount = 0;

					}
					//%14
					//%- 16llu\t
#ifdef IO_ANALYSYS
					Error(_T("IoMonitor %d %s UseTime %llu %llu CurCount(%llu) Ios=%llu Iops=%llukiops XosIops=%llukiops")
						_T("\r\nname\t\tActiveCount\t\tIoCount\t\t\tmin\t\t\tmax\t\t\t\tavg\t\t\tdvt\t\t\tsdvt")
						_T("\r\nSend\t\t%- 24llu%- 24llu%- 24llu%- 24llu%- 24f%- 24f%- 24f")
						//_T("\r\nSend\t\t%- 16llu\t%- 16llu\t%- 16llu\t%- 16llu\t%- 16f\t%- 16f\t%- 16f\t")
						_T("\r\nRecv\t\t%- 24llu%- 24llu%- 24llu%- 24llu%- 24f%- 24f%- 24f")
						_T("\r\nTotal\t\t%- 24llu%- 24llu%- 24llu%- 24llu%- 24f%- 24f%- 24f"),
						m_nWatchCpuID, WatchLocalSystem->m_LocalEnv.m_NicAdapt.m_pNicName, CurrentTimeTick - nBeginTimeTick, CurrentTimeTick - nCurrentTimeTick,
						m_nAddCount, m_nAddIoCount, (m_nAddIoCount - m_nLastAddIoCount) / (CurrentTimeTick - nCurrentTimeTick),
#ifdef NOT_USING_XOS_TICK
						(m_nAddIoCount - m_nLastAddIoCount) / (GetTickCount64_XOS() - nXosCurrentTimeTick),
#else	
						0,
#endif
						m_WriteIoStatisticsSnapshot.GetActiveCount(), m_WriteIoStatisticsSnapshot.GetCount(),
						m_WriteIoStatisticsSnapshot.m_nMinSampleDiff, m_WriteIoStatisticsSnapshot.m_nMaxSampleDiff,
						m_WriteIoStatisticsSnapshot.GetAverage(), m_WriteIoStatisticsSnapshot.GetDeviation(), m_WriteIoStatisticsSnapshot.GetStandardDeviation(),

						m_ReadIoStatisticsSnapshot.GetActiveCount(), m_ReadIoStatisticsSnapshot.GetCount(),
						m_ReadIoStatisticsSnapshot.m_nMinSampleDiff, m_ReadIoStatisticsSnapshot.m_nMaxSampleDiff,
						m_ReadIoStatisticsSnapshot.GetAverage(), m_ReadIoStatisticsSnapshot.GetDeviation(), m_ReadIoStatisticsSnapshot.GetStandardDeviation(),

						m_TotalIoStatisticsSnapshot.GetActiveCount(), m_TotalIoStatisticsSnapshot.GetCount(),
						m_TotalIoStatisticsSnapshot.m_nMinSampleDiff, m_TotalIoStatisticsSnapshot.m_nMaxSampleDiff,
						m_TotalIoStatisticsSnapshot.GetAverage(), m_TotalIoStatisticsSnapshot.GetDeviation(), m_TotalIoStatisticsSnapshot.GetStandardDeviation()
					);
#endif
					//m_WriteIoStatisticsSnapshot.UpdataSession();
					//m_ReadIoStatisticsSnapshot.UpdataSession();
					//m_TotalIoStatisticsSnapshot.UpdataSession();
#ifdef NOT_USING_XOS_TICK
					nXosCurrentTimeTick = GetTickCount64_XOS();
#endif
					nCurrentTimeTick = CurrentTimeTick;
					m_nLastAddIoCount = m_nAddIoCount;
					ClearLazyStatistics();
				}
			}
		}



		XOS_Sleep_Name(IoMonitorSleep, 7000);
	}

}
OPTIMIZE_OFF_END





volatile unsigned int g_nWatchdogThreadCount = 0;


class  WatchdogThread : public HereticThread<WatchdogThread>
{
public:
	//USING_ALIGNEDMEM;

	ULONGLONG m_nLastClientCompleteCount;
	ULONGLONG m_nLastServerCompleteCount;
	ULONGLONG i;
	ULONGLONG nWaitTime;
	BOOL m_bServerMode;
	BOOL m_bReOptimize;

	struct LastInfo
	{
		ULONGLONG m_nLastRxPollCount;
		ULONGLONG m_nLastTxPollCount;
		ULONGLONG m_nLastPMDCount;
		ULONGLONG m_nLastRecvCount;
		ULONGLONG m_nLastSendCount;
		ULONGLONG m_nLastRecvMiss;
		ULONGLONG m_nLastSendMiss;
		ULONGLONG m_nLastIdleCount;

		ULONGLONG m_nLastIoComplateCount;
		ULONGLONG m_nLastMsgComplateCount;
	};
	LastInfo m_LastInfo[32];
	WatchdogThread() {
	};
	~WatchdogThread() {};
	void Init() {
		i = m_nLastClientCompleteCount = m_nLastServerCompleteCount = 0;
		m_bServerMode = g_nWatchdogThreadCount ? FALSE : TRUE;
		g_nWatchdogThreadCount++;
		m_bReOptimize = FALSE;

		memset(&m_LastInfo[0], 0, sizeof(m_LastInfo));
	};
	
	void PrintfMsgQueueInfo(unsigned int nBrokerLocalSystem, unsigned int nConsumerLocalSystem, unsigned int nProducerLocalSystem , unsigned int nPersistenceLocalSystem, unsigned int nTotalLocalSystem)
	{
#if (TCP_TEST_MODE==TCP_TEST_MODE_MSG_QUEUE)

		if (g_XOSSystem.m_LocalSystemArray[0][nBrokerLocalSystem - 1].m_bReady)
		{
			typename XOS_T::CXOSLocalSystemT* LocalSystem = (XOS_T::CXOSLocalSystemT*)g_XOSSystem.m_LocalSystemArray[0][nBrokerLocalSystem - 1].pLocalSystem;
			if (LocalSystem->m_LocalEnv.m_NicAdapt.IsReady())
			{
#define Val(_User,_Name) g_GInfo[n##_User##LocalSystem].g_n##_User##_Name##Count
#define LastVal(_User,_Name) g_GInfo[n##_User##LocalSystem].g_nLast##_User##_Name##Count

				static const char * border = "------------------------------";
				LONGLONG nMsgQueueUesd = 0;
#ifdef NOT_HAE_PERSISTENCE
				nMsgQueueUesd = Val(Producer, Complete) - Val(Consumer, Complete);
				
#else
				nMsgQueueUesd = Val(Producer, Complete) - Val(Persistence, Complete);
				
#endif
				//多线程下nMsgQueueUesd可能是负值.
				g_GInfo[nBrokerLocalSystem].g_nMsgQueuePeakUsed = (nMsgQueueUesd > 0) ? nMsgQueueUesd : 0;
				g_GInfo[nBrokerLocalSystem].g_nMsgQueueMaxPeakUsed = (g_GInfo[nBrokerLocalSystem].g_nMsgQueueMaxPeakUsed > g_GInfo[nBrokerLocalSystem].g_nMsgQueuePeakUsed) ?
					g_GInfo[nBrokerLocalSystem].g_nMsgQueueMaxPeakUsed : g_GInfo[nBrokerLocalSystem].g_nMsgQueuePeakUsed;

				Error(_T("%sMsgQueue-%u-%s UsedTime=%llu QueueSize=%u P-C-Count(%llu %llu)%s\n"), border, LocalSystem->m_nCurrentCpu, 
					LocalSystem->m_LocalEnv.m_NicAdapt.m_pNicName, (ULONGLONG)(GetTickCount64() + 1 - nWaitTime), MessageQueueSize,
					g_GInfo[nBrokerLocalSystem].g_nProducerBrokerID, g_GInfo[nBrokerLocalSystem].g_nConsumerBrokerID, border);
				Val(Total, Complete) = Val(Producer, Complete) + Val(Consumer, Complete) + Val(Broker, Complete) + Val(Persistence, Complete);

				Val(Total, MsgComplete) = Val(Producer, MsgComplete) + Val(Consumer, MsgComplete) + Val(Broker, MsgComplete) + Val(Persistence, MsgComplete);

				Val(Total, Error) = Val(Producer, Error) + Val(Consumer, Error) + Val(Broker, Error) + Val(Persistence, Error);

				Error(_T("QueueUsed(%llu %.2f%%) MaxPeak(%llu %.2f%%) Io(%llu %llukiops) Msg(%llu %llukiops) ProducerMiss(%llu %lluKmiss) ConsumerMiss(%llu %lluKmiss)")
					_T("\r\nMsgQueue\t\tEchoCount\t\tMsgCount\t\tErrorCount\t\tEchoPPS(kiops)\t\tMsgPPS(kiops)")
					_T("\r\nProducer\t\t%- 24llu%- 24llu%- 24llu%- 24llu%- 24llu")
					_T("\r\nBroker\t\t\t%- 24llu%- 24llu%- 24llu%- 24llu%- 24llu")
					_T("\r\nConsumer\t\t%- 24llu%- 24llu%- 24llu%- 24llu%- 24llu")
					_T("\r\nPersistence\t\t%- 24llu%- 24llu%- 24llu%- 24llu%- 24llu")
					_T("\r\nTotal\t\t\t%- 24llu%- 24llu%- 24llu%- 24llu%- 24llu\r\n"),

					g_GInfo[nBrokerLocalSystem].g_nMsgQueuePeakUsed, (((float)(g_GInfo[nBrokerLocalSystem].g_nMsgQueuePeakUsed*100))/ (float)MessageQueueSize),
					g_GInfo[nBrokerLocalSystem].g_nMsgQueueMaxPeakUsed, (((float)(g_GInfo[nBrokerLocalSystem].g_nMsgQueueMaxPeakUsed * 100)) / (float)MessageQueueSize),
					
					Val(Producer, Complete) + Val(Consumer, Complete),
					(Val(Producer, Complete) + Val(Consumer, Complete) - m_LastInfo[nBrokerLocalSystem - 1].m_nLastIoComplateCount) / (GetTickCount64() + 1 - nWaitTime),
					Val(Producer, MsgComplete) + Val(Consumer, MsgComplete),
					(Val(Producer, MsgComplete) + Val(Consumer, MsgComplete) - m_LastInfo[nBrokerLocalSystem - 1].m_nLastMsgComplateCount) / (GetTickCount64() + 1 - nWaitTime),
					g_GInfo[nBrokerLocalSystem].g_nProducerMissCount,
					(g_GInfo[nBrokerLocalSystem].g_nProducerMissCount - g_GInfo[nBrokerLocalSystem].g_nLastProducerMissCount) / (GetTickCount64() + 1 - nWaitTime),
					g_GInfo[nBrokerLocalSystem].g_nConsumerMissCount,
					(g_GInfo[nBrokerLocalSystem].g_nConsumerMissCount- g_GInfo[nBrokerLocalSystem].g_nLastConsumerMissCount) / (GetTickCount64() + 1 - nWaitTime),

					Val(Producer, Complete), Val(Producer, MsgComplete), Val(Producer, Error),
					(Val(Producer, Complete) - LastVal(Producer, Complete)) / (GetTickCount64() + 1 - nWaitTime),
					(Val(Producer, MsgComplete) - LastVal(Producer, MsgComplete)) / (GetTickCount64() + 1 - nWaitTime),

					Val(Broker, Complete), Val(Broker, MsgComplete), Val(Broker, Error),
					(Val(Broker, Complete) - LastVal(Broker, Complete)) / (GetTickCount64() + 1 - nWaitTime),
					(Val(Broker, MsgComplete) - LastVal(Broker, MsgComplete)) / (GetTickCount64() + 1 - nWaitTime),

					Val(Consumer, Complete), Val(Consumer, MsgComplete), Val(Consumer, Error),
					(Val(Consumer, Complete) - LastVal(Consumer, Complete)) / (GetTickCount64() + 1 - nWaitTime),
					(Val(Consumer, MsgComplete) - LastVal(Consumer, MsgComplete)) / (GetTickCount64() + 1 - nWaitTime),

					Val(Persistence, Complete), Val(Persistence, MsgComplete), Val(Persistence, Error),
					(Val(Persistence, Complete) - LastVal(Persistence, Complete)) / (GetTickCount64() + 1 - nWaitTime),
					(Val(Persistence, MsgComplete) - LastVal(Persistence, MsgComplete)) / (GetTickCount64() + 1 - nWaitTime),

					Val(Total, Complete), Val(Total, MsgComplete), Val(Producer, Error),
					(Val(Total, Complete) - LastVal(Total, Complete)) / (GetTickCount64() + 1 - nWaitTime),
					(Val(Total, MsgComplete) - LastVal(Total, MsgComplete)) / (GetTickCount64() + 1 - nWaitTime)

					);
				m_LastInfo[nBrokerLocalSystem - 1].m_nLastIoComplateCount = Val(Producer, Complete) + Val(Consumer, Complete);
				m_LastInfo[nBrokerLocalSystem - 1].m_nLastMsgComplateCount = Val(Producer, MsgComplete) + Val(Consumer, MsgComplete);
				g_GInfo[nBrokerLocalSystem].g_nLastConsumerMissCount = g_GInfo[nBrokerLocalSystem].g_nConsumerMissCount;
				g_GInfo[nBrokerLocalSystem].g_nLastProducerMissCount - g_GInfo[nBrokerLocalSystem].g_nProducerMissCount;

#define SetLastVal(_name) \
g_GInfo[n##_name##LocalSystem].g_nLast##_name##CompleteCount=g_GInfo[n##_name##LocalSystem].g_n##_name##CompleteCount;\
g_GInfo[n##_name##LocalSystem].g_nLast##_name##MsgCompleteCount=g_GInfo[n##_name##LocalSystem].g_n##_name##MsgCompleteCount;\
g_GInfo[n##_name##LocalSystem].g_nLast##_name##ErrorCount=g_GInfo[n##_name##LocalSystem].g_n##_name##ErrorCount;\

				SetLastVal(Producer); SetLastVal(Consumer); SetLastVal(Broker); SetLastVal(Persistence); SetLastVal(Total);
				
				Error(_T("%s-----------------------------------------%s\n"), border, border);
			}
			
		}
#endif
	}
	void PintfInfo(unsigned int nLocalSystem, BOOL bPrintfPort = FALSE)
	{
		if (g_XOSSystem.m_LocalSystemArray[0][nLocalSystem - 1].m_bReady)
		{
			typename XOS_T::CXOSLocalSystemT* LocalSystem = (XOS_T::CXOSLocalSystemT*)g_XOSSystem.m_LocalSystemArray[0][nLocalSystem - 1].pLocalSystem;
			if (LocalSystem->m_LocalEnv.m_NicAdapt.IsReady())
			{

				if (bPrintfPort) LocalSystem->m_LocalEnv.m_NicAdapt.PrintfStats();

				
				Error(_T("WT-%u %s i=%llu time(%llu) PMD(%llu %lluK) RxTxPoll(%llu %llu %lluK %lluK) RxTx(%llu %llu) PPS(R=%llu S=%llu)K,NIC_MISS(R=%llu S=%llu)K Idle(%llu %lluK) TxUpDataSpeed=%llu"),
					LocalSystem->m_nCurrentCpu,
					LocalSystem->m_LocalEnv.m_NicAdapt.m_pNicName,
					i, (ULONGLONG)(GetTickCount64() + 1 - nWaitTime),
					LocalSystem->m_nPMDCount,
					(ULONGLONG)((LocalSystem->m_nPMDCount - m_LastInfo[nLocalSystem - 1].m_nLastPMDCount) / (GetTickCount64() + 1 - nWaitTime)),
					(ULONGLONG)LocalSystem->m_LocalEnv.m_NicAdapt.m_nRxPollCount, (ULONGLONG)LocalSystem->m_LocalEnv.m_NicAdapt.m_nTxPollCount,
					(ULONGLONG)(LocalSystem->m_LocalEnv.m_NicAdapt.m_nRxPollCount - m_LastInfo[nLocalSystem - 1].m_nLastRxPollCount) / (GetTickCount64() + 1 - nWaitTime),
					(ULONGLONG)(LocalSystem->m_LocalEnv.m_NicAdapt.m_nTxPollCount - m_LastInfo[nLocalSystem - 1].m_nLastTxPollCount) / (GetTickCount64() + 1 - nWaitTime),
					LocalSystem->m_LocalEnv.m_NicAdapt.m_nRecvComplateCount,
					LocalSystem->m_LocalEnv.m_NicAdapt.m_nSendComplateCount,
					(ULONGLONG)((LocalSystem->m_LocalEnv.m_NicAdapt.m_nRecvComplateCount - m_LastInfo[nLocalSystem - 1].m_nLastRecvCount) / (GetTickCount64() + 1 - nWaitTime)),
					(ULONGLONG)((LocalSystem->m_LocalEnv.m_NicAdapt.m_nSendComplateCount - m_LastInfo[nLocalSystem - 1].m_nLastSendCount) / (GetTickCount64() + 1 - nWaitTime)),
					(ULONGLONG)((LocalSystem->m_LocalEnv.m_NicAdapt.m_nRecvMissCount - m_LastInfo[nLocalSystem - 1].m_nLastRecvMiss) / (GetTickCount64() + 1 - nWaitTime)),
					(ULONGLONG)((LocalSystem->m_LocalEnv.m_NicAdapt.m_nSendMissCount - m_LastInfo[nLocalSystem - 1].m_nLastSendMiss) / (GetTickCount64() + 1 - nWaitTime)),
					LocalSystem->m_nIdleCount,
					(ULONGLONG)((LocalSystem->m_nIdleCount - m_LastInfo[nLocalSystem - 1].m_nLastIdleCount) / (GetTickCount64() + 1 - nWaitTime)),
					LocalSystem->m_LocalEnv.m_NicAdapt.m_TxUpdataSpeed
				);
				m_LastInfo[nLocalSystem - 1].m_nLastIdleCount = LocalSystem->m_nIdleCount;
				m_LastInfo[nLocalSystem - 1].m_nLastRxPollCount = LocalSystem->m_LocalEnv.m_NicAdapt.m_nRxPollCount;
				m_LastInfo[nLocalSystem - 1].m_nLastTxPollCount = LocalSystem->m_LocalEnv.m_NicAdapt.m_nTxPollCount;

				m_LastInfo[nLocalSystem - 1].m_nLastPMDCount = LocalSystem->m_nPMDCount;
				m_LastInfo[nLocalSystem - 1].m_nLastRecvCount = LocalSystem->m_LocalEnv.m_NicAdapt.m_nRecvComplateCount;
				m_LastInfo[nLocalSystem - 1].m_nLastRecvMiss = LocalSystem->m_LocalEnv.m_NicAdapt.m_nRecvMissCount;
				m_LastInfo[nLocalSystem - 1].m_nLastSendCount = LocalSystem->m_LocalEnv.m_NicAdapt.m_nSendComplateCount;
				m_LastInfo[nLocalSystem - 1].m_nLastSendMiss = LocalSystem->m_LocalEnv.m_NicAdapt.m_nSendMissCount;


				return;
			}
			//Error(_T("m_NicAdapt %d not ready"), nLocalSystem);
			return;
		}
		Error(_T("LocalSystem %d not ready"), nLocalSystem);
	}
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
#ifdef RSS_MODE
			PintfInfo(3, TRUE);
			PrintfMsgQueueInfo(3);
			PintfInfo(5);
			PrintfMsgQueueInfo(3, 5, 5, 3, 3);
			PintfInfo(7, TRUE);
			PintfInfo(9);
#else
			PintfInfo(3, TRUE);
			PrintfMsgQueueInfo(3,5,5,3,3);
			PintfInfo(5, TRUE);
#endif
			
			/*
			Error(_T("Watchdog-%d-%s i=%llu time(%llu) LinkCount(%llu) Complete(C=%llu S=%llu) EchoIOPS(C=%llu S=%llu)Kiops"),
			XOS_LocalSystem->m_nCurrentCpu, m_bServerMode?_T("Server"):_T("Client"),
			i, (GetTickCount64() + 1 - nWaitTime), g_nCurrentServerLinkCount,
			g_nClientCompleteCount,g_nServerCompleteCount,
			(g_nClientCompleteCount - m_nLastClientCompleteCount) / (GetTickCount64() + 1 - nWaitTime),
			(g_nServerCompleteCount - m_nLastServerCompleteCount) / (GetTickCount64() + 1 - nWaitTime)
			);
			m_nLastClientCompleteCount = g_nClientCompleteCount;
			m_nLastServerCompleteCount = g_nServerCompleteCount;

			if ((!m_bReOptimize)&&(g_nServerCount>= CONCURRENT_LINK_COUNT))
			{
			m_bReOptimize = TRUE;
			Error(_T("Watchdog-%d-%s ReOptimize"),
			XOS_LocalSystem->m_nCurrentCpu, m_bServerMode ? _T("Server") : _T("Client"));
			XOS_LocalSystem->m_LocalEnv.tcpmgr->ReOptimize();
			}
			*/
		}


}
OPTIMIZE_OFF_END


void InitSysWatch()
{

#if(NET_WORK_MODE==NET_WORK_MODE_MASS_LINK)
	g_NicAdaptWatchdog = NewObjectFormMemPool<MemoryMgr_FreeList1T, NicAdaptWatchdog<XOS_T::CXOSLocalSystemT::LocalEnvContextT::NicAdaptT, TCPManager>>();
	CreateLocalHereticThread(g_NicAdaptWatchdog, FALSE);

#endif
	if (XOS_LocalSystem->m_nCurrentCpu == 1)
	{
		memset((void *)&g_GInfo, 0, sizeof(g_GInfo));
#ifdef RSS_MODE
		g_GInfo[7].g_IoMonitor = NewObjectFormMemPool<MemoryMgr_FreeList1T, IoMonitor>();
		g_GInfo[7].g_IoMonitor->m_nWatchCpuID = 7;
		CreateLocalHereticThread(g_GInfo[7].g_IoMonitor, TRUE);
		g_GInfo[9].g_IoMonitor = NewObjectFormMemPool<MemoryMgr_FreeList1T, IoMonitor>();
		g_GInfo[9].g_IoMonitor->m_nWatchCpuID = 9;
		CreateLocalHereticThread(g_GInfo[9].g_IoMonitor, TRUE);
#else
		g_GInfo[5].g_IoMonitor = NewObjectFormMemPool<MemoryMgr_FreeList1T, IoMonitor>();
		g_GInfo[5].g_IoMonitor->m_nWatchCpuID = 5;
		CreateLocalHereticThread(g_GInfo[5].g_IoMonitor, TRUE);
#endif

		CreateLocalHereticThread<WatchdogThread>(TRUE);
	}
	

}
