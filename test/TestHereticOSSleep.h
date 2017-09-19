#pragma once

//cache line 对齐能提升20%-30%的性能
//线程内分配内存，速度提升30%
#ifdef _DEBUG
#define HereticThreadConnt 10*10000
#define PrintCount 30
#define WaitTime 200
#define LoopCountMain 3
#define LoopCountChild 2
#define GroupCount 2
#else
#define HereticThreadConnt 5000*10000
#define PrintCount 50
#define WaitTime 1000
#define LoopCountMain 2
#define LoopCountChild 20
#define GroupCount 2
#endif // _DEBUG
//#define THREAD_CONCURRENT

struct TestWaitHereticThreadContext
{
	unsigned int nGroupCount;		//任务组数
	unsigned int nTaskCount;		//每组任务数量
};

extern unsigned int g_TotalCountAdd;
extern unsigned int g_TotalHereticThreadComplate;

class TestLoop :public HereticThread
{
public:
	//unsigned int m_nCallCount;
	HereticThreadTimer m_Timer;
	unsigned int m_LoopCount;
	TestLoop(){

	};
	~TestLoop(){};

	void Init()
	{
		m_Timer(this);
		m_LoopCount = 0;
	}

#pragma optimize("", off)
	void Loop()
	{

		USING_HERETICOS_THREAD;

		//Error(_T("LoopEnter init m_pContext=%s m_pParameter=%d nCallCount=%d\r\n"), m_pContext, m_pParameter, m_nCallCount);
		//设置协程断点标记地址
		for (; m_LoopCount<LoopCountChild; m_LoopCount++)
		{
			Sleep_ForHereticThread(Time1, m_Timer, WaitTime);
			g_TotalCountAdd++;
			//m_Timer.Sleep(100);
			//Error(_T("LoopEnter m_pContext=%s m_pParameter=%d m_LoopCount=%d\r\n"), m_pContext, m_pParameter, m_LoopCount);
		}
		//Error(_T("LoopEnter m_pContext=%s m_pParameter=%d m_LoopCount=%d Over\r\n"), m_pContext, m_pParameter, m_LoopCount);
		g_TotalHereticThreadComplate++;
		//if (m_pParameter == (void *)0x2)
		{
			//Error(_T("TestLoop Assert %x"), (ThreadImplT*)this);
		}
		ExitHereticThread();

	}
#pragma optimize("", on)

protected:

private:
};



#define SAFE_DATA_TEST_WORK_COUNT			6000000
#define SAFE_DATA_TEST_SAMPLE_PRECOUNT		1*1000
#define SAFE_DATA_TEST_SAMPLE_GRUOPCOUNT	20
#define SAFE_DATA_TEST_CONCURRENT			1			//并发测试
#define SAFE_DATA_CACHE_ALIGEND				1			//cache对齐

//#ifdef SAFE_DATA_CACHE_ALIGEND
struct  CACHE_ALIGN_HEAD AddCount
{
	unsigned int nCount;
	unsigned int a1;
}CACHE_ALIGN_END;

//#else
struct AddCount1
{
	unsigned int nCount;
	unsigned int a1;
};

//#endif




#ifdef SAFE_DATA_CACHE_ALIGEND
class  CACHE_ALIGN_HEAD SafeDataTest :public HereticThread
#else
class  SafeDataTest :public HereticThread
#endif
{
public:
	//unsigned int m_nCallCount;
	unsigned int m_nIndex;
	unsigned int m_nCount;
	unsigned int m_nPreCount;
	AddCount * m_pnData;
	AddCount m_nData;
	unsigned char  m_pSumData[64];
	unsigned char  m_pSumDataDst[64];
	unsigned int nSum1;
	unsigned int nSum2;
	//AddCount  m_TestSampleData;
	unsigned int i,j;
	EnvT::IpSessionT * m_pIpSession;
	HereticThreadTimer m_Timer;
	SafeDataTest(){
		m_pnData = NULL;
	};
	~SafeDataTest(){};

	void Init()
	{
		m_Timer(this);
		m_nPreCount = SAFE_DATA_TEST_SAMPLE_PRECOUNT;
		m_nCount = SAFE_DATA_TEST_SAMPLE_PRECOUNT * SAFE_DATA_TEST_SAMPLE_GRUOPCOUNT;
		//m_pSumData = new unsigned char[64];
		//m_pSumDataDst = new unsigned char[64];
		//m_pnData = new AddCount;
		
		m_nIndex = *(unsigned int*)m_pParameter;
		m_pIpSession = &(GetEnv()->m_IpSession);

		
		if (m_pIpSession->size() != TEST_RAND_SEEK)
		{
			DWORD nCurTick = GetTickCount();
			GetEnv()->m_IpKeyArray.reserve(TEST_RAND_SEEK);
			m_pIpSession->reserve(TEST_RAND_SEEK);
			for (unsigned int i = 0; i < TEST_RAND_SEEK; i++)
			{
				GetEnv()->m_IpKeyArray.push_back(IpKey::RandKey());
				(*m_pIpSession)[GetEnv()->m_IpKeyArray[i]] = i;
			}

			DWORD nCurTick1 = GetTickCount();
			nCurTick = nCurTick1 - nCurTick;

			GetEnv()->m_IpKeyArray.clear();
			m_pIpSession->clear();
			for (unsigned int i = 0; i < TEST_RAND_SEEK; i++)
			{
				GetEnv()->m_IpKeyArray.push_back(IpKey::RandKey());
				(*m_pIpSession)[GetEnv()->m_IpKeyArray[i]] = i;
			}

			m_nIndex = 0;
			Error2(_T("SafeDataTest CreateHashmap Over UsedTime=%u %u tid=%u, nSize=%u heap=%p %p total(%u %u %u %u  new=%u del=%u)"), nCurTick, GetTickCount() - nCurTick1,
				GetCurrentThreadId(), m_pIpSession->size(), GetEnv()->m_pTlsHeap, ::TlsGetValue(g_hTlsForHeap),
				GetEnv()->m_pTlsHeap->nConCount[0], GetEnv()->m_pTlsHeap->nConCount[1],
				GetEnv()->m_pTlsHeap->nConCount[2], GetEnv()->m_pTlsHeap->nConCount[3],
				GetEnv()->m_pTlsHeap->nConCount[4], GetEnv()->m_pTlsHeap->nConCount[5]);
		}
		
		

		m_pnData = &m_nData;
		m_pnData->nCount = i = j = 0;
		
	}

	unsigned int checksum(unsigned char * pData, unsigned int nBytes)
	{
		unsigned int nSum = 0;
		for (unsigned int i = 0; i < nBytes; i++)
		{
			nSum += pData[i];
		}
		return nSum;
	}
#define USED_TIME(_nWaitTime,_Code) _nWaitTime=GetTickCount();_Code;_nWaitTime=GetTickCount()-_nWaitTime;
	int TestInsertSeek()
	{
		
		if (m_pIpSession->find(GetEnv()->m_IpKeyArray[GetEnv()->m_nIndex%TEST_RAND_SEEK]) != m_pIpSession->end())
		{
			GetEnv()->m_nIndex++;
			return 1;
		}
		GetEnv()->m_nIndex++;
		return 2;

		//printf("TestInsertSeek RandSeekUseTime mem %d nFind=%u nWaitTime=%d pps=%u kpps \r\n", GetWorkSetSize(),
		//	nFind, nWaitTime, TEST_RAND_SEEK / (nWaitTime ));
	}
	void updata()
	{
		for (j = 0; j < SAFE_DATA_TEST_SAMPLE_PRECOUNT;)
		{
			m_pnData->nCount++;
			m_pnData->nCount++;
			m_pnData->nCount++;
			m_pnData->nCount++;
			m_pnData->nCount++;

			m_pnData->nCount++;
			m_pnData->nCount++;
			m_pnData->nCount++;
			m_pnData->nCount++;
			m_pnData->nCount++;

			m_pnData->nCount++;
			m_pnData->nCount++;
			m_pnData->nCount++;
			m_pnData->nCount++;
			m_pnData->nCount++;

			m_pnData->nCount++;
			m_pnData->nCount++;
			m_pnData->nCount++;
			m_pnData->nCount++;
			m_pnData->nCount++;

			nSum1 += checksum(m_pSumData, 64);
			memcpy(m_pSumDataDst, m_pSumData, 64);
			nSum2 += checksum(m_pSumDataDst, 64);
			nSum2 += TestInsertSeek();
			j += 20;
		}

		if (m_nIndex % 300 == 0)
		{
			Error(_T("SafeDataTest %u %u"), m_nIndex, i);
		}
	}

#pragma optimize("", off)
	void Loop()
	{

		USING_HERETICOS_THREAD;
		for (; i< SAFE_DATA_TEST_SAMPLE_GRUOPCOUNT;i++)
		//for (; i<m_nCount;)
		{
			Sleep_ForHereticThread(Time1, m_Timer, 10);
			//for (j=0;j<m_nPreCount;j++)
			updata();
		}
		//Error2(_T("SafeDataTest "));
		ExitHereticThread();

	}
#pragma optimize("", on)

protected:

private:
}CACHE_ALIGN_END;



class SafeDataTestMgr :public HereticThread
{
public:
	//unsigned int m_nCallCount;
	unsigned int m_nIndex;
	unsigned int m_nCount;
	unsigned int m_nPreCount;
	unsigned int * m_pnData;
	unsigned int m_nComplateCount;
	IOCPDeviceT * m_pDevice;
	EnvT::IpSessionT * m_pIpSession;
//#ifdef SAFE_DATA_CACHE_ALIGEND
	SafeDataTest * m_TestSample[SAFE_DATA_TEST_WORK_COUNT];
//#else
	//SafeDataTest  m_TestSample1[SAFE_DATA_TEST_WORK_COUNT];
//#endif 

	unsigned int m_nCurTick;
	unsigned int m_nMaxCount;
	unsigned int m_nMinCount;

	HereticThread::ThreadImplT ** m_TaskArray;
	TimerThreadPrv m_Timer;
	SafeDataTestMgr(){

	};
	~SafeDataTestMgr(){};

	void Init()
	{
		m_Timer(this);
		m_pDevice = (IOCPDeviceT *)m_pContext;
		m_nIndex = 0;
		m_nComplateCount = 0;
		
		m_nMaxCount = m_nMinCount = 0;
		m_pIpSession = &(GetEnv()->m_IpSession);
	}

#pragma optimize("", off)
	void Loop()
	{

		USING_HERETICOS_THREAD;

		Error2(_T("SafeDataTestMgr Entry ComputerCount workcount(%u) addcount(%u) size(%u %u %u)"),
			SAFE_DATA_TEST_WORK_COUNT, SAFE_DATA_TEST_SAMPLE_PRECOUNT * SAFE_DATA_TEST_SAMPLE_GRUOPCOUNT,
			sizeof(m_TestSample)/*, sizeof(m_TestSampleData), sizeof(m_TestSampleData1)*/);
		memset(&m_TestSample[0], 0, sizeof(m_TestSample));
		m_nCurTick = GetTickCount();
		for (m_nIndex; m_nIndex<SAFE_DATA_TEST_WORK_COUNT; m_nIndex++)
		{
			
#ifdef SAFE_DATA_TEST_CONCURRENT
			m_pDevice->CreateHereticThread<SafeDataTest>(IOCPDeviceT::WorkType_Compute, m_nIndex, &m_TestSample[m_nIndex], m_pDevice, &m_nIndex,false);
			//m_TestSample[m_nIndex] = new SafeDataTest;
			//m_TestSample[m_nIndex]->SetNotUsedGc();
			//m_pDevice->CreateHereticThread(IOCPDeviceT::WorkType_Compute, m_nIndex, m_TestSample[m_nIndex], m_pDevice, &m_nIndex);
#else
			m_pDevice->CreateHereticThread(IOCPDeviceT::WorkType_Compute, 0, &m_TestSample[m_nIndex], m_pDevice, m_nIndex);
#endif
			//m_TestSample[m_nIndex].SetNotUsedGc();
		}
		

		Error2(_T("SafeDataTestMgr CreateHereticThread Call Over UsedTime=%u "),
			(unsigned int)(GetTickCount() - m_nCurTick));

		m_nCurTick = GetTickCount();

		for (;;)
		{
			m_nComplateCount = 0;
			m_nMaxCount = 0;
			for (m_nIndex = 0; m_nIndex < SAFE_DATA_TEST_WORK_COUNT; m_nIndex++)
			{
				if (m_TestSample[m_nIndex] && m_TestSample[m_nIndex]->m_pnData)
				{
					m_nComplateCount++;
					m_nMaxCount += m_TestSample[m_nIndex]->m_pIpSession->size();
				}

			}

			Error2(_T("SafeDataTestMgr CreateComplate %u  min(%u) max(%u) UsedTime=%u...."), m_nComplateCount,
				m_nMinCount, m_nMaxCount, (unsigned int)(GetTickCount() - m_nCurTick));
			if (m_nComplateCount == SAFE_DATA_TEST_WORK_COUNT)
			{
				break;
			}

			Sleep_ForHereticThread(Time1, m_Timer, 2000);

		}
		Error2(_T("SafeDataTestMgr CreateHereticThread Over UsedTime=%u "),
			(unsigned int)(GetTickCount() - m_nCurTick));

		m_nCurTick = GetTickCount();
		for (;;)
		{
			m_nComplateCount = 0;
			m_nMaxCount = m_nMinCount = m_TestSample[0]->m_pnData[0].nCount;
			for (m_nIndex = 0; m_nIndex<SAFE_DATA_TEST_WORK_COUNT; m_nIndex++)
			{
				//if (m_TestSample[m_nIndex]->m_nCount == m_TestSampleData[m_nIndex]->nCount) m_nComplateCount++;
				
				if (m_TestSample[m_nIndex]->m_pnData)
				{
					if (m_TestSample[m_nIndex]->m_nCount == m_TestSample[m_nIndex]->m_pnData->nCount) m_nComplateCount++;
					//m_nMaxCount = max(m_nMaxCount, m_TestSample[m_nIndex].m_pnData[m_nIndex].nCount);
					//m_nMinCount = min(m_nMinCount, m_TestSample[m_nIndex].m_pnData[m_nIndex].nCount);

					m_nMaxCount = max(m_nMaxCount, m_TestSample[m_nIndex]->m_pnData->nCount);
					m_nMinCount = min(m_nMinCount, m_TestSample[m_nIndex]->m_pnData->nCount);
				}
				
			}
			

			Error2(_T("SafeDataTestMgr Complate %u  min(%d) max(%d) UsedTime=%u...."), m_nComplateCount,
				m_nMinCount, m_nMaxCount,(unsigned int)(GetTickCount() - m_nCurTick));
			if (m_nComplateCount == SAFE_DATA_TEST_WORK_COUNT)
			{
				break;
			}

			Sleep_ForHereticThread(Time2, m_Timer, 2000);
			
		}
		Error2(_T("SafeDataTestMgr Exit UsedTime=%u...."), (unsigned int)(GetTickCount() - m_nCurTick));

		ExitHereticThread();

	}
#pragma optimize("", on)

protected:

private:
};

/*


void Sort(int numbers[], int left, int right) {
	if (left >= right) {
		return;
	}
	int l = left;
	int r = right;
	int middle = numbers[left];
	while (l < r) {
		while (numbers[r] >= middle && l < r) {
			r--;
		}
		numbers[l] = numbers[r];
		while (numbers[l] <= middle && l < r) {
			l++;
		}
		numbers[r] = numbers[l];
	}
	numbers[l] = middle;
	Sort(numbers, left, l - 1);
	Sort(numbers, l + 1, right);
}

*/


class TestWaitThread :public HereticThread
{
public:
	unsigned int m_nCurLoopCount;
	unsigned int m_nCurGroup;
	unsigned int m_nCurCount;
	HereticThreadTimer m_Timer;
	HereticThread::ThreadImplT ** m_TaskArray;
	unsigned int m_nUsedTime;
	TestWaitThread(){

	};
	~TestWaitThread(){};

	void Init()
	{

		m_Timer(this);
		m_nCurLoopCount = 0;
		m_nCurCount = 0;
		m_TaskArray = new HereticThread::ThreadImplT *[((TestWaitHereticThreadContext *)m_pContext)->nTaskCount];

	}
	void Close()
	{
		delete[] m_TaskArray;
	}
#pragma optimize("", off)
	void Loop()
	{


		USING_HERETICOS_THREAD;

		m_nUsedTime = GetTickCount();



		TestWaitHereticThreadContext * pContext = (TestWaitHereticThreadContext *)m_pContext;
		Error2(_T("TestWaitHereticThread LoopEnter init nGroupCount=%d nTaskCount=%d %d size=%I64d\r\n"), 
			pContext->nGroupCount, pContext->nTaskCount,
			sizeof(TestLoop), (__int64)(sizeof(TestLoop))*(__int64)pContext->nTaskCount);
		//设置协程断点标记地址


		for (; m_nCurLoopCount < LoopCountMain; m_nCurLoopCount++)
		{
			Error(_T("TestWaitHereticThread  m_nCurLoopCount=%u LoopEnter\r\n"), m_nCurLoopCount);

			Sleep_ForHereticThread(Time1, m_Timer, 1000);


			for (m_nCurGroup = 0; m_nCurGroup < ((TestWaitHereticThreadContext *)m_pContext)->nGroupCount; m_nCurGroup++)
			{
				Error2(_T("TestWaitHereticThread LoopEnter m_nCurLoopCount=%u begin create HereticThread\r\n"), m_nCurLoopCount);
				__assume(1);
				for (m_nCurCount = 0; m_nCurCount<((TestWaitHereticThreadContext *)m_pContext)->nTaskCount; m_nCurCount++)
				{
					HereticThreadPoolForThreadPrv::HANDLE hHereticThread = CreateHereticThread<TestLoop>((void *)m_nCurGroup, (void *)m_nCurCount);
					m_TaskArray[m_nCurCount] = (TestLoop *)hHereticThread.GetObject();
					//StartHereticThread(hHereticThread);
				}
				Error2(_T("TestWaitHereticThread LoopEnter m_nCurLoopCount=%u  m_nCurGroup=%u WaitEnter\r\n"), m_nCurLoopCount, m_nCurGroup);

				WaitAll(Wait1, m_TaskArray, ((TestWaitHereticThreadContext *)m_pContext)->nTaskCount);


			

				Error2(_T("TestWaitHereticThread LoopEnter m_nCurLoopCount=%u m_nCurGroup=%u WaitOk\r\n"), m_nCurLoopCount, m_nCurGroup);
				DoGc();
				Error2(_T("TestWaitHereticThread LoopEnter m_nCurLoopCount=%u m_nCurGroup=%u GcOk\r\n"), m_nCurLoopCount, m_nCurGroup);
			}

			//m_Timer.Sleep(100);
			//Error(_T("LoopEnter m_pContext=%s m_pParameter=%d m_LoopCount=%d\r\n"), m_pContext, m_pParameter, m_LoopCount);
		}

		m_nUsedTime = GetTickCount() - m_nUsedTime;


		//Error(_T("LoopEnter m_pContext=%s m_pParameter=%d m_LoopCount=%d Over\r\n"), m_pContext, m_pParameter, m_LoopCount);
		Error2(_T("TestWaitHereticThread LoopEnter ExitHereticThread UesdTime=%u\r\n"), m_nUsedTime);

		g_TotalHereticThreadComplate++;
		ExitHereticThread();

	}
#pragma optimize("", on)

protected:

private:
};