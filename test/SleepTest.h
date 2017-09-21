#pragma once

long long g_SleepComplateCount = 0;
long long g_SleepLastComplateCount = 0;
long long g_SleepThreadComplateCount = 0;
#define LOOP_COUNT			1000000
#define SLEEP_THREAD_COUNT	10000*5000  //1��
#define CREATE_STEP_CONT	512345		//�ֲ�������ɢ��ʱ�����ж�
#define CREATE_STEP_LAZY	123			//�ֲ������ӳ�ʱ��
#define SLEEP_LAZY			10000			//����IO �ӳ�ʱ��

class  SleepWorkThread : public HereticThread<SleepWorkThread>
{
public:
	//USING_ALIGNEDMEM;
	__int64 i;
	SleepWorkThread() { i = 0; };
	~SleepWorkThread() {};
	void Init() {};
	void Close() {};
	//static ThreadLocalStaticObject<ThreadLocalObj> as;
	//OPTIMIZE_CLOSE_BEGIN
	OPTIMIZE_OFF
	void Loop()
	{
		USING_HERETICOS_THREAD
		
		for (;i<LOOP_COUNT;)
		{
			XOS_Sleep(SLEEP_LAZY);
			i++;
			g_SleepComplateCount++;
		}
		g_SleepThreadComplateCount;
		ExitThread();
	}
	OPTIMIZE_OFF_END

private:
};

class  SleepManagerThread : public HereticThread<SleepManagerThread>
{
public:
	//USING_ALIGNEDMEM;
	SleepWorkThread m_SleepWorkThread[SLEEP_THREAD_COUNT];
	int m_nCurTime;
	int m_nLastTime;
	int m_nStep;
	SleepManagerThread() { };
	~SleepManagerThread() {};
	void Init() {
		m_nCurTime = ::GetTickCount();
		m_nLastTime = m_nCurTime;
		m_nStep = 0;
	};
	void Close() {};
	//static ThreadLocalStaticObject<ThreadLocalObj> as;
	//OPTIMIZE_CLOSE_BEGIN
	OPTIMIZE_OFF
	void Loop()
	{
		USING_HERETICOS_THREAD
		//��������
		m_nCurTime = ::GetTickCount();
		printf("\n��ʼ�������� �ܼ�����(%d) Sleep�ӳ�(%d)ms Loop����(%d)...\n",
			SLEEP_THREAD_COUNT, SLEEP_LAZY, LOOP_COUNT
			);
		for (m_nStep=0; m_nStep<SLEEP_THREAD_COUNT;m_nStep++)
		{
			CreateHereticThread(&m_SleepWorkThread[m_nStep], TRUE);
			if ((m_nStep%CREATE_STEP_CONT) == 0)
			{
				XOS_Sleep(CREATE_STEP_LAZY);
				//printf("\n��ʼ�������� nStep(%d)...\n");
			}
		}
		m_nCurTime = ::GetTickCount();
		printf("���������������ʱ%u ms ʵ����ʱ%u ms...\n",
			m_nCurTime- m_nLastTime,
			m_nCurTime - m_nLastTime-(CREATE_STEP_LAZY*(m_nStep/ CREATE_STEP_CONT)));
		m_nLastTime = m_nCurTime;
		//��������������
		for (;;)
		{
			XOS_Sleep(3456);
			printf("�����߳� ��ʱ %u ms  SleepComplateCount=%llu ThreadCompalte=%llu IOPS(%llu)...\n",
				::GetTickCount()- m_nLastTime,
				g_SleepComplateCount, g_SleepThreadComplateCount,
				((1000*(g_SleepComplateCount - g_SleepLastComplateCount))/ 3456));
			g_SleepLastComplateCount = g_SleepComplateCount;
		}
		ExitThread();
	}
	OPTIMIZE_OFF_END

private:
};


SleepManagerThread * g_SleepManagerThread;

void TestSleep()
{
	g_SleepManagerThread = new SleepManagerThread;
	CreateHereticThread(g_SleepManagerThread, TRUE);
}