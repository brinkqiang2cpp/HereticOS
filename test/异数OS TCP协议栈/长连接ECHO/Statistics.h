#pragma once
//************************************************************************ 
// 公司: 2012-2015
// 文件名: Statistics.h
// 作者: 叶树深 
// 版本: 1.0 
// 日期: 2016/04/01
// 功能描述: 统计分析
// 其他备注:
// 维护历史:
//************************************************************************

//#define NOT_STATISTICS 1
#include <cmath>

template<typename intT, typename floatT>
class Statistics
{
public:
	typedef Statistics<intT, floatT> _Myt;
	volatile intT	m_nSampleCount;
	volatile intT	m_nTotalSampleDiff;	//总计样本差累计
	
	volatile intT	m_LastSampleDiff;	//最近样本差
	volatile intT	m_nMinSampleDiff;	//最小样本差
	volatile intT	m_nMaxSampleDiff;		//最大样本差

	//intT	m_nCurMean;			//实际延迟期望
	//intT	m_nCurAverage;		//延迟平均数
	intT	m_nDeviation;		//延迟方差
	//intT	m_nStandardDeviation;		//延迟标准差

	intT	m_nExpectedVal;		//预设的数学期望

	//统计活跃
	ULONGLONG m_nCurrentActiveCount;
	unsigned int m_nCurrentStatisticsSession;

	Statistics(){
		m_nSampleCount = 0;
		m_nTotalSampleDiff = 0;			
						
		m_LastSampleDiff = 0;			
		m_nMinSampleDiff = 0;			
		m_nMaxSampleDiff = 0;			

		//m_nCurMean = 0;				//期望
		//m_nCurAverage=0;				//平均数
		m_nDeviation = 0;				//方差
		//m_nStandardDeviation=0;		//标准差
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

		//m_nCurMean = 0;				//期望
		//m_nCurAverage=0;				//平均数
		m_nDeviation = 0;				//方差
		//m_nStandardDeviation=0;		//标准差
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
			//使用平均数作为期望
			m_nDeviation += m_LastSampleDiff*m_LastSampleDiff;
		}

#endif
	}
	//数学期望，均值平均数
	floatT GetAverage()
	{
		if (m_nSampleCount < 1) return 0.0;
		return (((floatT)m_nTotalSampleDiff) / ((floatT)(m_nSampleCount)));
	}
	//方差
	floatT GetDeviation()
	{
		if (m_nSampleCount < 1) return 0.0;
		if (m_nExpectedVal != 0)
		{
			return (((floatT)m_nDeviation) / ((floatT)(m_nSampleCount - 1)));
		}
		else
		{
			//(x1^2+x2^2+....xn^2)/n-((积分求和)/(n)^2)
			floatT x = ((floatT)m_nDeviation) / ((floatT)m_nSampleCount);
			floatT x1 = (floatT)(m_nTotalSampleDiff) / ((floatT)(m_nSampleCount));
			x1 = x1*x1;
			/*Error2(_T("x=%lf x1=%lf \r\nm_nDeviation=%I64d m_LastSampleDiff=%I64d m_nTotalSampleDiff=%I64d\r\nm_nSampleCount=%I64d\r\n"),
				x, x1, m_nDeviation, m_LastSampleDiff, m_nTotalSampleDiff, m_nSampleCount);*/
			return (floatT)(x - x1);
		}

	}
	//标准差
	floatT GetStandardDeviation()
	{
		if (m_nSampleCount < 1) return 0.0;
		return sqrt(GetDeviation());
	}
	intT GetCount()
	{
		return m_nSampleCount;
	}
	//聚合
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
	volatile intT	m_nLastSample;		//最近样本

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


