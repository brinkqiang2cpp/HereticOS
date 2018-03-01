#pragma once
//************************************************************************ 
// ��˾: 2012-2015
// �ļ���: Statistics.h
// ����: Ҷ���� 
// �汾: 1.0 
// ����: 2016/04/01
// ��������: ͳ�Ʒ���
// ������ע:
// ά����ʷ:
//************************************************************************

//#define NOT_STATISTICS 1
#include <cmath>

template<typename intT, typename floatT>
class Statistics
{
public:
	typedef Statistics<intT, floatT> _Myt;
	volatile intT	m_nSampleCount;
	volatile intT	m_nTotalSampleDiff;	//�ܼ��������ۼ�
	
	volatile intT	m_LastSampleDiff;	//���������
	volatile intT	m_nMinSampleDiff;	//��С������
	volatile intT	m_nMaxSampleDiff;		//���������

	//intT	m_nCurMean;			//ʵ���ӳ�����
	//intT	m_nCurAverage;		//�ӳ�ƽ����
	intT	m_nDeviation;		//�ӳٷ���
	//intT	m_nStandardDeviation;		//�ӳٱ�׼��

	intT	m_nExpectedVal;		//Ԥ�����ѧ����

	//ͳ�ƻ�Ծ
	ULONGLONG m_nCurrentActiveCount;
	unsigned int m_nCurrentStatisticsSession;

	Statistics(){
		m_nSampleCount = 0;
		m_nTotalSampleDiff = 0;			
						
		m_LastSampleDiff = 0;			
		m_nMinSampleDiff = 0;			
		m_nMaxSampleDiff = 0;			

		//m_nCurMean = 0;				//����
		//m_nCurAverage=0;				//ƽ����
		m_nDeviation = 0;				//����
		//m_nStandardDeviation=0;		//��׼��
		m_nExpectedVal = 0;
		m_nCurrentActiveCount = 0;
		m_nCurrentStatisticsSession = 0xff;

	};
	~Statistics(){};

	void operator()(intT nExpectedVal = 0, intT nMinSampleDiff = 1000000, intT nMaxSampleDiff=-1000000)
	{
		m_nExpectedVal = nExpectedVal;
		m_nSampleCount = 0;
		m_nTotalSampleDiff = 0;

		m_LastSampleDiff = 0;
		m_nMinSampleDiff = nMinSampleDiff;
		m_nMaxSampleDiff = nMaxSampleDiff;

		//m_nCurMean = 0;				//����
		//m_nCurAverage=0;				//ƽ����
		m_nDeviation = 0;				//����
		//m_nStandardDeviation=0;		//��׼��
		//m_nExpectedVal = 0;
	}
	ULONGLONG GetActiveCount()
	{
		return m_nCurrentActiveCount;
	}
	void AddActiveSession(unsigned int & nSession)
	{
		if (nSession != m_nCurrentStatisticsSession)
		{
			nSession = m_nCurrentStatisticsSession;
			m_nCurrentActiveCount++;
		}
	}
	void UpdataSession()
	{
		m_nCurrentStatisticsSession++;
		m_nCurrentActiveCount = 0;
	}
	void AddSample(unsigned int & nSession,intT  nSample)
	{
		AddActiveSession(nSession);
		m_nSampleCount++;
#ifndef NOT_STATISTICS
		m_LastSampleDiff = nSample;/*(nCurTimeStamp - m_nLastSample)*/;
		m_nTotalSampleDiff += m_LastSampleDiff;
		
		//if (m_nMinSampleDiff == 0) m_nMinSampleDiff = m_LastSampleDiff;
		m_nMinSampleDiff = m_nMinSampleDiff < m_LastSampleDiff ? m_nMinSampleDiff : m_LastSampleDiff;
		m_nMaxSampleDiff = m_nMaxSampleDiff < m_LastSampleDiff ? m_LastSampleDiff : m_nMaxSampleDiff;
		if (m_nExpectedVal != 0)
		{

			m_nDeviation += (m_LastSampleDiff - m_nExpectedVal)*(m_LastSampleDiff - m_nExpectedVal);
		}
		else
		{
			//ʹ��ƽ������Ϊ����
			m_nDeviation += m_LastSampleDiff*m_LastSampleDiff;
		}

#endif
	}
	//��ѧ��������ֵƽ����
	floatT GetAverage()
	{
		if (m_nSampleCount < 1) return 0.0;
		return (((floatT)m_nTotalSampleDiff) / ((floatT)(m_nSampleCount)));
	}
	//����
	floatT GetDeviation()
	{
		if (m_nSampleCount < 1) return 0.0;
		if (m_nExpectedVal != 0)
		{
			return (((floatT)m_nDeviation) / ((floatT)(m_nSampleCount - 1)));
		}
		else
		{
			//(x1^2+x2^2+....xn^2)/n-((�������)/(n)^2)
			floatT x = ((floatT)m_nDeviation) / ((floatT)m_nSampleCount);
			floatT x1 = (floatT)(m_nTotalSampleDiff) / ((floatT)(m_nSampleCount));
			x1 = x1*x1;
			/*Error2(_T("x=%lf x1=%lf \r\nm_nDeviation=%I64d m_LastSampleDiff=%I64d m_nTotalSampleDiff=%I64d\r\nm_nSampleCount=%I64d\r\n"),
				x, x1, m_nDeviation, m_LastSampleDiff, m_nTotalSampleDiff, m_nSampleCount);*/
			return (floatT)(x - x1);
		}

	}
	//��׼��
	floatT GetStandardDeviation()
	{
		if (m_nSampleCount < 1) return 0.0;
		return sqrt(GetDeviation());
	}
	intT GetCount()
	{
		return m_nSampleCount;
	}
	//�ۺ�
	_Myt & operator <<(_Myt & src)
	{
		m_nSampleCount += src.m_nSampleCount;
		m_nMinSampleDiff = min(m_nMinSampleDiff, src.m_nMinSampleDiff);
		m_nMaxSampleDiff = max(m_nMaxSampleDiff, src.m_nMaxSampleDiff);
		m_nTotalSampleDiff += src.m_nTotalSampleDiff;
		m_nDeviation += src.m_nDeviation;
		return *this;
	}
protected:

private:
};



template<typename intT, typename floatT>
class IoStatistics : public Statistics<intT, floatT>
{
public:
	typedef Statistics<intT, floatT> BaseT;
	volatile intT	m_nLastSample;		//�������

	IoStatistics(){
		m_nLastSample = 0;

	};
	~IoStatistics(){};

	void operator()(intT nExpectedVal)
	{
		BaseT::m_nExpectedVal = nExpectedVal;
	}

	void AddIoBegin(intT  nCurTimeStamp)
	{
		BaseT::m_nLastSample = nCurTimeStamp;
	}

	void AddIoEnd(intT  nCurTimeStamp)
	{
		AddSample(nCurTimeStamp - m_nLastSample);
		BaseT::m_nLastSample = nCurTimeStamp;

	}
	
protected:

private:
};


