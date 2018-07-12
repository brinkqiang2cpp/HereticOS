#pragma once

#define IPC_TEST_LOCAL_CORE 1
#define IPC_TEST_REMOTE_TCP 2
#define IPC_TEST_MODE IPC_TEST_LOCAL_CORE
//#define IPC_TEST_MODE IPC_TEST_REMOTE_TCP

#define IPC_ALLOW_COUNT(_count) (_count>IPC_LOCAL_SINGLE_CORE_SLOT_COUNT?IPC_LOCAL_SINGLE_CORE_SLOT_COUNT:_count)
#define IPC_TEST_COUNT IPC_ALLOW_COUNT(1)

#define NET_DISK_TEST_MODE_RAND_READ 1	//随机读测试
#define NET_DISK_TEST_MODE_SEQ_READ 2	//顺序读测试
#define NET_DISK_TEST_MODE_WRITE_NOTIFY 3	//写入广播测试
//#define NET_DISK_TEST_MODE NET_DISK_TEST_MODE_RAND_READ
#define NET_DISK_TEST_MODE NET_DISK_TEST_MODE_SEQ_READ
//#define NET_DISK_TEST_MODE NET_DISK_TEST_MODE_WRITE_NOTIFY

#define DISK_BLOCK_SIZE_LOG2 6
#define DISK_SIZE_LOG2 30	//1G
#define DISK_BLOCK_COUNT	(1<<(DISK_SIZE_LOG2- DISK_BLOCK_SIZE_LOG2))
#define DISK_OFFSET_MASK (DISK_BLOCK_COUNT-1)

#define WRITE_NOTIFY_HASH_COUNT_LOG2 16
#define WRITE_NOTFIY_BLOCKOFFSET	1
#define WRITE_NOTFIY_BLOCKRANGE		(DISK_BLOCK_COUNT/16)
#define DISK_BLOCK_COUNT_LOG2 (DISK_SIZE_LOG2-DISK_BLOCK_SIZE_LOG2)
#define WRITE_NOTIFY_HASH_MASK	(((1<<WRITE_NOTIFY_HASH_COUNT_LOG2)-1)<<(DISK_BLOCK_COUNT_LOG2-WRITE_NOTIFY_HASH_COUNT_LOG2))

#if(NIC_TYPE==NIC_TYPE_DPDK)
#define NETDISK_CLIENT_COUNT 64		//客户端实例线程数量

#elif(NIC_TYPE==NIC_TYPE_CPU_SOCKET_RING)

#define NETDISK_CLIENT_COUNT 4		//客户端实例线程数量

#else
#define NETDISK_CLIENT_COUNT 1		//客户端实例线程数量
#endif

#define DISK_INSTANCE_NAME _T("MyTestDisk")
#define DISK_INSTANCE_IMAGE_NAME _T("MyTestDisk.bin")
#define DISK_BLOCK_MAXSIZE 1024


#if (IPC_TEST_MODE==IPC_TEST_LOCAL_CORE)

#define IpcType LocalCore

#else if(IPC_TEST_MODE==IPC_TEST_REMOTE_TCP)
#include "ipc_tcp.h"
#define IpcType RemoteTCP

#endif


template <int nBlockSizeLog2>
class NetRamDiskManager
{
public:
	ULONGLONG m_nPushThreadID;
	typedef NetRamDiskManager<nBlockSizeLog2> _Myt;
	static _Myt & GetInstance()
	{
		static _Myt _self;
		return _self;
	}

	struct DiskInstance
	{
		ULONGLONG nBlockSize;
		ULONGLONG nDiskSize;
		tstring  szImagePath;
		unsigned char * m_DiskInstance;
	};
	struct WriteNotfiyFilter
	{
		ULONGLONG nBlockOffset;
		ULONGLONG nBlockCount;

		size_t m_hash;
		void Hash()
		{
			//m_hash=Hash(x.m_Key[0]) ^ Hash(x.m_Key[1])
			//
			m_hash = std::hash<int>()(nBlockOffset&WRITE_NOTIFY_HASH_MASK);
		}
	};
#define RangeTestAatB(_a,_b) ((_a.nBlockOffset>=_b.nBlockOffset)&&(_a.nBlockOffset<=(_b.nBlockOffset+_b.nBlockCount)))
	struct WriteNotfiyFilter_Compare {
		inline bool operator()(const WriteNotfiyFilter & a, const WriteNotfiyFilter & b) const {

			if (RangeTestAatB(a,b)|| RangeTestAatB(b, a))
			{
				return true;
			}
			return false;

		}
	};

	struct WriteNotfiyFilter_Hash {
		size_t operator()(const WriteNotfiyFilter &x) const {
			//return Hash(x.m_Key[0]) ^ Hash(x.m_Key[1]) ^ Hash(x.m_Key[2]);
			return x.m_hash;
		}
	};
	typedef HereticEventQueue<WriteNotfiyFilter,true> WriteWriteNotfiyBroadcastT;
	typedef yss_allocator<pair<WriteNotfiyFilter, WriteWriteNotfiyBroadcastT*>, MemoryMgr_FreeList1T> WriteNotfiyFilter_Allocator;
	typedef unordered_map<WriteNotfiyFilter, WriteWriteNotfiyBroadcastT*, WriteNotfiyFilter_Hash, WriteNotfiyFilter_Compare, WriteNotfiyFilter_Allocator> WriteWriteNotfiyMapT;
	WriteWriteNotfiyMapT m_WriteWriteNotfiyMap;
	typedef std::map<tstring, DiskInstance, std::less<tstring>> DiskInstanceMapT;
	DiskInstanceMapT m_DiskInstanceMap;
	NetRamDiskManager() { m_nPushThreadID = 0; };
	~NetRamDiskManager() {};
	bool CreateDisk(TCHAR * szDiskName, TCHAR *szImagePath,ULONGLONG nBlockSize,ULONGLONG nDiskSize)
	{
		DiskInstance disk;
		disk.nBlockSize = nBlockSize;
		disk.nDiskSize = nDiskSize;
		disk.szImagePath = szImagePath;
		disk.m_DiskInstance = (unsigned char *)AllocLargePage(nDiskSize+ DISK_BLOCK_MAXSIZE);
		//
		for (ULONGLONG nBlockOffset=0; nBlockOffset<(nDiskSize/ nBlockSize); nBlockOffset++)
		{
			*(ULONGLONG*)&disk.m_DiskInstance[nBlockOffset*nBlockSize] = nBlockOffset;
		}
		m_DiskInstanceMap[szDiskName] = disk;
	}

	bool OpenDisk(TCHAR *  szDiskName, DiskInstance & Instance)
	{
		typename DiskInstanceMapT::iterator itDisk=m_DiskInstanceMap.find(szDiskName);
		if (itDisk != m_DiskInstanceMap.end())
		{
			Instance = itDisk->second;
			return true;
		}
		else
		{
			return false;
		}
	}
	WriteWriteNotfiyBroadcastT * RegistWriteNotfiyFilter(ULONGLONG nBlockOffset,ULONGLONG nBlockCount)
	{
		WriteNotfiyFilter Filter;
		Filter.nBlockOffset = nBlockOffset;
		Filter.nBlockCount = nBlockCount;
		Filter.Hash();
		typename WriteWriteNotfiyMapT::iterator itFind = m_WriteWriteNotfiyMap.find(Filter);
		WriteWriteNotfiyBroadcastT * pBroadcast = NULL;
		if (itFind == m_WriteWriteNotfiyMap.end())
		{
			pBroadcast = (WriteWriteNotfiyBroadcastT *)NewObjectFormMemPool<MemoryMgr_FreeList1T, WriteWriteNotfiyBroadcastT>();
			pBroadcast->init();
			m_WriteWriteNotfiyMap[Filter] = pBroadcast;
		}
		else
		{
			pBroadcast = itFind->second;
		}
		return pBroadcast;
	}
	void CheckWriteNotfiy(ULONGLONG nBlockOffset, ULONGLONG nBlockCount)
	{
		WriteNotfiyFilter Filter;
		Filter.nBlockOffset = nBlockOffset;
		Filter.nBlockCount = nBlockCount;
		Filter.Hash();
		typename WriteWriteNotfiyMapT::iterator itFind = m_WriteWriteNotfiyMap.find(Filter);
		WriteWriteNotfiyBroadcastT * pBroadcast = NULL;
		if (itFind != m_WriteWriteNotfiyMap.end())
		{
			pBroadcast = itFind->second;
			pBroadcast->m_Value = Filter;
			pBroadcast->SetEvent();
		}
	}
private:

};

typedef NetRamDiskManager<DISK_BLOCK_SIZE_LOG2> NetRamDiskManagerT;



class NetRamDiskWriteNotify :public IpcInterface
{
public:
	ULONGLONG m_CallCount;
	GInfo * m_pOwnInfo;
	ULONGLONG m_nDiskSession;
	typedef NetRamDiskWriteNotify _Myt;

	NetRamDiskWriteNotify() { m_CallCount = 0; };
	~NetRamDiskWriteNotify() {};


	struct WriteNotifyInput :public IpcInterface::ParameterBase
	{
		ULONGLONG nSession;
		ULONGLONG nBlockOffset;
		ULONGLONG nBlockCount;
	};
	struct WriteNotifyOutput :public IpcInterface::ParameterBase
	{
	};
	static bool RamDiskWriteNotify(_Myt * pObject, WriteNotifyInput *pInput, WriteNotifyOutput * pOutput, unsigned int & nError)
	{
		pObject->m_pOwnInfo->g_nRamDiskRandWriteNotifyCount++;
		pOutput->SetSize(sizeof(WriteNotifyOutput));
		return true;
	}

	bool Link(unsigned int & nError)
	{
		//printf("Link Entry\r\n");

		m_pOwnInfo = (GInfo *)&g_GInfo[XOS_LocalSystem->m_nCurrentCpu];
		return true;
	}
	bool Close(unsigned int & nError)
	{
		printf("Close Entry\r\n");
		return true;
	}

	EXPORT_IPC(_Myt, 2, RamDiskWriteNotify, RamDiskWriteNotify);


private:

};

class WriteNotfiyPushThread : public HereticThread<WriteNotfiyPushThread, HereticThread_Type_TcpClient>
{
public:
	WriteNotfiyPushThread() {};
	~WriteNotfiyPushThread() {};
	
	NetRamDiskWriteNotify::WriteNotifyInput m_WriteNotifyInput;
	NetRamDiskWriteNotify::WriteNotifyOutput * m_pWriteNotifyOutput;
	NetRamDiskManagerT::WriteWriteNotfiyBroadcastT * m_pWaitEvent;
	GInfo * m_pOwnInfo;
	unsigned int nError;

	IpcDefine(1,IpcType, m_WriteNotfiyIpc, WriteNotfiyPushThread);

	void Init() {

		m_pOwnInfo = (GInfo *)&g_GInfo[XOS_LocalSystem->m_nCurrentCpu];
		m_WriteNotifyInput.SetSize<NetRamDiskWriteNotify::WriteNotifyInput>();
	};
	void Close() {
	};


	void Loop(void * pContext = NULL);
private:

};

OPTIMIZE_OFF
void WriteNotfiyPushThread::Loop(void * pContext)
{
	USING_HERETICOS_THREAD;
	//Error(_T("NetRamDiskTestThread Entry..."));

	IpcLink(IpcType, m_WriteNotfiyIpc);
	if (m_WriteNotfiyIpc.m_Error == IPC_ERROR_OK)
	{
		do
		{
			XOS_Wait_P(m_pWaitEvent);
			IpcCall(IpcType, m_WriteNotfiyIpc, NetRamDiskWriteNotify::IPC::RamDiskWriteNotify_1, m_WriteNotifyInput, m_pWriteNotifyOutput, nError);
		} while (true);

		IpcClose(IpcType, m_WriteNotfiyIpc);
		Error(_T("TCPWriteNotfiyPushThread Closed..."));
	}

	Error(_T("TCPWriteNotfiyPushThread Exit..."));

}
OPTIMIZE_OFF_END


template <int nBlockSizeLog2>
class NetRamDisk :public IpcInterface
{
public:
	ULONGLONG m_CallCount;
	GInfo * m_pOwnInfo;
	ULONGLONG m_nDiskSession;
	
	NetRamDiskManagerT::DiskInstance  m_DiskInstance;
	typedef NetRamDisk<nBlockSizeLog2> _Myt;

	NetRamDisk() { m_CallCount = 0;  };
	~NetRamDisk() {};

	struct ReadDiskInput :public IpcInterface::ParameterBase
	{
		ULONGLONG nSession;
		ULONGLONG nOffset;
		ULONGLONG nRangeLen;
	};
	struct ReadDiskOutput :public IpcInterface::ParameterBase
	{
		unsigned char cData[1];
	};

	static bool ReadDisk(_Myt * pObject, ReadDiskInput * pInput, ReadDiskOutput * pOutput, unsigned int & nError)
	{
		unsigned int nReadLen = pInput->nRangeLen < DISK_BLOCK_MAXSIZE ? pInput->nRangeLen : DISK_BLOCK_MAXSIZE;
		pOutput->SetSize(sizeof(ReadDiskOutput)+nReadLen);
		memcpy(&pOutput->cData[0], &pObject->m_DiskInstance.m_DiskInstance[pInput->nOffset << nBlockSizeLog2], nReadLen);
		return true;
	};

	struct WriteDiskInput :public IpcInterface::ParameterBase
	{
		ULONGLONG nSession;
		ULONGLONG nOffset;
		ULONGLONG nRangeLen;
		unsigned char cData[1];
	};

	struct WriteDiskOutput :public IpcInterface::ParameterBase
	{
		unsigned int nResult;
	};
	static bool WriteDisk(_Myt * pObject, WriteDiskInput *pInput, WriteDiskOutput * pOutput, unsigned int & nError)
	{
		unsigned int nWriteLen = pInput->nRangeLen < DISK_BLOCK_MAXSIZE ? pInput->nRangeLen : DISK_BLOCK_MAXSIZE;

		pOutput->SetSize(sizeof(_Myt::WriteDiskOutput));
		memcpy(&pObject->m_DiskInstance.m_DiskInstance[pInput->nOffset << nBlockSizeLog2],
			&pInput->cData[0], nWriteLen);
		pOutput->nResult = nWriteLen;
		NetRamDiskManagerT::GetInstance().CheckWriteNotfiy(pInput->nOffset, nWriteLen>> nBlockSizeLog2);
		return true;
	}

	static bool OpenDisk(_Myt * pObject, void *pInput, void * pOutput, unsigned int & nError)
	{
		nError = IPC_ERROR_FAIL;
		pObject->m_DiskInstance.m_DiskInstance = NULL;
		if (NetRamDiskManagerT::GetInstance().OpenDisk(DISK_INSTANCE_NAME, pObject->m_DiskInstance))
		{
			nError = IPC_ERROR_OK;
		}
		return true;
	}

	static bool CloseDisk(_Myt * pObject, void *pInput, void * pOutput, unsigned int & nError)
	{
		
		return true;
	}
	//注册写入消息
	struct IPCAddress
	{
		enum AddressType
		{
			IPCAddress_LPC=1,
			IPCAddress_TCP = 2
		}m_AddressType;
		struct IPAddress
		{
			unsigned int nRemoteIP;
			unsigned short nRemotePort;
			unsigned short nRssAddress;
		};
		union AddressValue
		{
			IPAddress m_TCPAddress;
			unsigned int m_nLPCSlot;
		}m_AddressValue;
	};
	struct RegistWriteNotifyInput :public IpcInterface::ParameterBase
	{
		ULONGLONG nSession;
		IPCAddress WriteNotifyIPCAddress;
		ULONGLONG nBlockOffset;
		ULONGLONG nBlockCount;
	};
	struct RegistWriteNotifyOutput :public IpcInterface::ParameterBase
	{
	};

	static bool RegistWriteNotify(_Myt * pObject, RegistWriteNotifyInput *pInput, RegistWriteNotifyOutput * pOutput, unsigned int & nError)
	{

		switch (pInput->WriteNotifyIPCAddress.m_AddressType)
		{
		case IPCAddress::AddressType::IPCAddress_TCP:
			{
#if (IPC_TEST_MODE==IPC_TEST_REMOTE_TCP)
				WriteNotfiyPushThread::TcpRpcClientT* pClient = XOS_LocalSystem->m_LocalEnv.tcpmgr->CreateClient<WriteNotfiyPushThread::TcpRpcClientT>(
					IP_INT(192, 168, 1, 1),
					pInput->WriteNotifyIPCAddress.m_AddressValue.m_TCPAddress.nRemoteIP,
					pInput->WriteNotifyIPCAddress.m_AddressValue.m_TCPAddress.nRemotePort,
					X86Short((NetRamDiskManagerT::GetInstance().m_nPushThreadID % 60000) + 81));
				NetRamDiskManagerT::GetInstance().m_nPushThreadID++;
				pClient->m_pWaitEvent = NetRamDiskManagerT::GetInstance().RegistWriteNotfiyFilter(
					pInput->nBlockOffset, pInput->nBlockCount);
#endif
			}
			break;
		case IPCAddress::AddressType::IPCAddress_LPC:
			break;
		default:
			break;
		}
		return true;
	}
	
	bool Link(unsigned int & nError)
	{
		//printf("Link Entry\r\n");

		m_pOwnInfo = (GInfo *)&g_GInfo[XOS_LocalSystem->m_nCurrentCpu];
		return true;
	}
	bool Close(unsigned int & nError)
	{
		printf("Close Entry\r\n");
		return true;
	}
	
	EXPORT_IPC(_Myt, 8, ReadDisk, WriteDisk, OpenDisk,CloseDisk, RegistWriteNotify, RegistWriteNotify, RegistWriteNotify, RegistWriteNotify);

	
private:

};



typedef NetRamDisk<DISK_BLOCK_SIZE_LOG2> NetRamDiskIpcT;

#if (IPC_TEST_MODE==IPC_TEST_LOCAL_CORE)


typedef IpcLocalCoreStubThread<NetRamDiskIpcT, DISK_BLOCK_MAXSIZE> NetRamDiskStubThreadT;
typedef IpcLocalCoreStubThread<NetRamDiskWriteNotify> NetRamDiskWriteNotifyStubThreadT;


#else if(IPC_TEST_MODE==IPC_TEST_REMOTE_TCP)

#ifdef _LINUX_
typedef IpcRemoteTCPStubThread1<NetRamDiskIpcT, DISK_BLOCK_MAXSIZE> NetRamDiskStubThreadT;
typedef IpcRemoteTCPStubThread2<NetRamDiskWriteNotify> NetRamDiskWriteNotifyStubThreadT;
#else
typedef IpcRemoteTCPStubThread<NetRamDiskIpcT, DISK_BLOCK_MAXSIZE> NetRamDiskStubThreadT;
typedef IpcRemoteTCPStubThread<NetRamDiskWriteNotify> NetRamDiskWriteNotifyStubThreadT;
#endif

#endif


class NetRamDiskPerformantsWatch : public HereticThread<NetRamDiskPerformantsWatch>
{
public:
	NetRamDiskPerformantsWatch() {};
	~NetRamDiskPerformantsWatch() {};


	ULONGLONG nWaitTime;
	GInfo * m_pOwnInfo;

	void Init() {
#if (TEST_MODE==TEST_MODE_LOOPBACK)
		m_pOwnInfo = (GInfo *)&g_GInfo[XOS_LocalSystem->m_nCurrentCpu];
#else
#ifdef RSS_MODE
		m_pOwnInfo = (GInfo *)&g_GInfo[7];
#else
		m_pOwnInfo = (GInfo *)&g_GInfo[5];
#endif
#endif

	};
	void Close() {
	};


	void Loop(void * pContext = NULL);
private:

};

OPTIMIZE_OFF
void NetRamDiskPerformantsWatch::Loop(void * pContext)
{
	USING_HERETICOS_THREAD;
	Error(_T("NetRamDiskPerformantsWatch Entry..."));
	do
	{
		Error(_T("RandRead(%llu %lluKiops) RandWrite(%llu %lluKipos) WriteNotify(%llu %lluKtps)"), 
			m_pOwnInfo->g_nRamDiskRandReadCount,
			(m_pOwnInfo->g_nRamDiskRandReadCount- m_pOwnInfo->g_nLastRamDiskRandReadCount)/ (CurrentTimeTick - nWaitTime),
			m_pOwnInfo->g_nRamDiskRandWriteCount,
			(m_pOwnInfo->g_nRamDiskRandWriteCount - m_pOwnInfo->g_nLastRamDiskRandWriteCount) / (CurrentTimeTick - nWaitTime),
			m_pOwnInfo->g_nRamDiskRandWriteNotifyCount,
			(m_pOwnInfo->g_nRamDiskRandWriteNotifyCount - m_pOwnInfo->g_nLastRamDiskRandWriteNotifyCount) / (CurrentTimeTick - nWaitTime)
			);
		m_pOwnInfo->g_nLastRamDiskRandWriteNotifyCount = m_pOwnInfo->g_nRamDiskRandWriteNotifyCount;
		m_pOwnInfo->g_nLastRamDiskRandWriteCount = m_pOwnInfo->g_nRamDiskRandWriteCount;
		m_pOwnInfo->g_nLastRamDiskRandReadCount = m_pOwnInfo->g_nRamDiskRandReadCount;

		nWaitTime = CurrentTimeTick;

		XOS_Sleep(3000+ (rand() % 2000));

	} while (true);

}
OPTIMIZE_OFF_END

class NetRamDiskTestThread : public HereticThread<NetRamDiskTestThread, HereticThread_Type_TcpClient>
{
public:
	NetRamDiskTestThread() {};
	~NetRamDiskTestThread() {};
	
	unsigned int nTestName;
	NetRamDiskIpcT::ReadDiskInput m_ReadCommand;
	NetRamDiskIpcT::ReadDiskOutput * m_pReadResult;
	NetRamDiskIpcT::WriteDiskInput m_WriteCommand;
	NetRamDiskIpcT::WriteDiskOutput * m_pWriteResult;

	NetRamDiskIpcT::RegistWriteNotifyInput m_RegistWriteNotifyInput;
	NetRamDiskIpcT::RegistWriteNotifyOutput * m_pRegistWriteNotifyOutput;
	
	IpcInterface::ParameterBase m_tmpPar1, m_tmpPar2;
	IpcInterface::ParameterBase* m_ptmpPar1, m_ptmpPar2;
	GInfo * m_pOwnInfo;
	unsigned int nError;
	
	IpcDefine(2,IpcType, m_DiskReadWriteIpc, NetRamDiskTestThread);

	void Init() {
		
		m_pOwnInfo = (GInfo *)&g_GInfo[XOS_LocalSystem->m_nCurrentCpu];
		m_RegistWriteNotifyInput.nBlockOffset = WRITE_NOTFIY_BLOCKOFFSET;
		m_RegistWriteNotifyInput.nBlockCount = WRITE_NOTFIY_BLOCKRANGE;
		m_RegistWriteNotifyInput.WriteNotifyIPCAddress.m_AddressType = NetRamDiskIpcT::IPCAddress::IPCAddress_TCP;
		m_RegistWriteNotifyInput.WriteNotifyIPCAddress.m_AddressValue.m_TCPAddress.nRemoteIP = IP_INT(192,168,2,1);
		m_RegistWriteNotifyInput.WriteNotifyIPCAddress.m_AddressValue.m_TCPAddress.nRemotePort = X86Short(39);
		m_RegistWriteNotifyInput.WriteNotifyIPCAddress.m_AddressValue.m_TCPAddress.nRssAddress = 0;
		m_RegistWriteNotifyInput.SetSize<NetRamDiskIpcT::RegistWriteNotifyInput>();
	};
	void Close() {
	};


	void Loop(void * pContext = NULL);
private:

};

OPTIMIZE_OFF
void NetRamDiskTestThread::Loop(void * pContext)
{
	USING_HERETICOS_THREAD;
	//Error(_T("NetRamDiskTestThread Entry..."));
	
	IpcLink(IpcType, m_DiskReadWriteIpc);
	if (m_DiskReadWriteIpc.m_Error == IPC_ERROR_OK)
	{

		//Error(_T("NetRamDiskTestThread  m_DiskReadWriteIpc Linked..."));
		IpcCall(IpcType, m_DiskReadWriteIpc, NetRamDiskIpcT::IPC::OpenDisk_3, m_tmpPar1, m_ptmpPar1, nError);
		if (m_DiskReadWriteIpc.m_Error == IPC_ERROR_OK)
		{
			//Error(_T("NetRamDiskTestThread  m_DiskReadWriteIpc Opened..."));
			m_ReadCommand.nRangeLen = m_WriteCommand.nRangeLen = TTL_CEXP::power(2, DISK_BLOCK_SIZE_LOG2);
			m_WriteCommand.SetSize<NetRamDiskIpcT::WriteDiskInput>(TTL_CEXP::power(2, DISK_BLOCK_SIZE_LOG2));
			m_ReadCommand.SetSize<NetRamDiskIpcT::ReadDiskInput>(TTL_CEXP::power(2, DISK_BLOCK_SIZE_LOG2));
			
#if(NET_DISK_TEST_MODE ==NET_DISK_TEST_MODE_WRITE_NOTIFY)
			IpcCall(IpcType, m_DiskReadWriteIpc, NetRamDiskIpcT::IPC::RegistWriteNotify_5, m_RegistWriteNotifyInput,
				m_pRegistWriteNotifyOutput, nError);
			if (m_DiskReadWriteIpc.m_Error != IPC_ERROR_OK)
			{
				Error(_T("NetRamDiskTestThread  RegistWriteNotify Fail..."));
			}
#endif
			do
			{
#if(NET_DISK_TEST_MODE ==NET_DISK_TEST_MODE_RAND_READ)
				m_ReadCommand.nOffset = FNVHash(&m_pOwnInfo->g_nRamDiskRandReadCount)&DISK_OFFSET_MASK;
				IpcCall(IpcType, m_DiskReadWriteIpc, NetRamDiskIpcT::IPC::ReadDisk_1, m_ReadCommand, m_pReadResult, nError);
				if(*(ULONGLONG*)&m_pReadResult->cData[0]== m_ReadCommand.nOffset)m_pOwnInfo->g_nRamDiskRandReadCount++;

#elif(NET_DISK_TEST_MODE ==NET_DISK_TEST_MODE_SEQ_READ)
				
				m_ReadCommand.nOffset = m_pOwnInfo->g_nRamDiskRandReadCount&DISK_OFFSET_MASK;
				IpcCall(IpcType, m_DiskReadWriteIpc, NetRamDiskIpcT::IPC::ReadDisk_1, m_ReadCommand, m_pReadResult, nError);
				if (*(ULONGLONG*)&m_pReadResult->cData[0] == m_ReadCommand.nOffset)m_pOwnInfo->g_nRamDiskRandReadCount++;

#elif(NET_DISK_TEST_MODE ==NET_DISK_TEST_MODE_WRITE_NOTIFY)
				m_WriteCommand.nOffset = FNVHash(&m_pOwnInfo->g_nRamDiskRandWriteCount)&DISK_OFFSET_MASK;
				IpcCall(IpcType, m_DiskReadWriteIpc, NetRamDiskIpcT::IPC::WriteDisk_2, m_WriteCommand, m_pWriteResult, nError);
				m_pOwnInfo->g_nRamDiskRandWriteCount++;

#endif

			} while (true);
		}
		IpcCall(IpcType, m_DiskReadWriteIpc, NetRamDiskIpcT::IPC::CloseDisk_4, m_tmpPar1, m_ptmpPar1, nError);

		IpcClose(IpcType, m_DiskReadWriteIpc);
		Error(_T("IpcTestThread Closed..."));
	}


	Error(_T("IpcTestThread Exit..."));
	
}
OPTIMIZE_OFF_END


LocalCoreObject NetRamDiskTestThread * g_NetRamDiskTestThreadArray = NULL;

LocalCoreObject  NetRamDiskStubThreadT *g_NetRamDiskStubThreadTArray = NULL;


void TestTCP()
{
	Error(_T("TestNetRamDisk Entry..."));

	
#if (IPC_TEST_MODE==IPC_TEST_LOCAL_CORE)
	NetRamDiskManagerT::GetInstance().CreateDisk(
		DISK_INSTANCE_NAME, DISK_INSTANCE_IMAGE_NAME,
		TTL_CEXP::power(2, DISK_BLOCK_SIZE_LOG2),
		TTL_CEXP::power(2, DISK_SIZE_LOG2)
	);
	g_NetRamDiskTestThreadArray = NewObjectFormMemPoolArray<MemoryMgr_FreeList1T, NetRamDiskTestThread>(NETDISK_CLIENT_COUNT);
	g_NetRamDiskStubThreadTArray = NewObjectFormMemPoolArray<MemoryMgr_FreeList1T, NetRamDiskStubThreadT>(NETDISK_CLIENT_COUNT);
	for (unsigned int n = 0; n < NETDISK_CLIENT_COUNT; n++)
	{
		g_NetRamDiskStubThreadTArray[n].BindAddress(n);
		CreateLocalHereticThread(&g_NetRamDiskStubThreadTArray[n], FALSE);

		g_NetRamDiskTestThreadArray[n].m_DiskReadWriteIpc.BindAddress(n);
		g_NetRamDiskTestThreadArray[n].nTestName = n;
		CreateLocalHereticThread(&g_NetRamDiskTestThreadArray[n], FALSE);
	}


#else if(IPC_TEST_MODE==IPC_TEST_REMOTE_TCP)

	

#if (TEST_MODE==TEST_MODE_LOOPBACK)
	NetRamDiskManagerT::GetInstance().CreateDisk(
		DISK_INSTANCE_NAME, DISK_INSTANCE_IMAGE_NAME,
		TTL_CEXP::power(2, DISK_BLOCK_SIZE_LOG2),
		TTL_CEXP::power(2, DISK_SIZE_LOG2)
	);
	XOS_LocalSystem->m_LocalEnv.tcpmgr->BindServer<NetRamDiskStubThreadT::TcpRpcServerT>(_T("192.168.1.1"), X86Short(38));
	XOS_LocalSystem->m_LocalEnv.tcpmgr->BindServer<NetRamDiskWriteNotifyStubThreadT::TcpRpcServerT>(_T("192.168.2.1"), X86Short(39));
	CreateLocalHereticThread<SystemCreaterThread<NetRamDiskTestThread::TcpRpcClientT, NETDISK_CLIENT_COUNT, 169, 38>>(TRUE);

#elif (TEST_MODE==TEST_MODE_BALANCED)
#ifdef RSS_MODE
	if (XOS_LocalSystem->m_nCurrentCpu == 3 || XOS_LocalSystem->m_nCurrentCpu == 5)
	{
#else
	if (XOS_LocalSystem->m_nCurrentCpu == 3)
	{

#endif
		NetRamDiskManagerT::GetInstance().CreateDisk(
			DISK_INSTANCE_NAME, DISK_INSTANCE_IMAGE_NAME,
			TTL_CEXP::power(2, DISK_BLOCK_SIZE_LOG2),
			TTL_CEXP::power(2, DISK_SIZE_LOG2)
		);
		XOS_LocalSystem->m_LocalEnv.tcpmgr->BindServer<NetRamDiskStubThreadT::TcpRpcServerT>(_T("192.168.1.1"), X86Short(38));
	}

#ifdef RSS_MODE
	if (XOS_LocalSystem->m_nCurrentCpu == 7 || XOS_LocalSystem->m_nCurrentCpu == 9)
	{

#else
	if (XOS_LocalSystem->m_nCurrentCpu == 5)
	{
#endif
		XOS_LocalSystem->m_LocalEnv.tcpmgr->BindServer<NetRamDiskWriteNotifyStubThreadT::TcpRpcServerT>(_T("192.168.2.1"), X86Short(39));
		CreateLocalHereticThread<SystemCreaterThread<NetRamDiskTestThread::TcpRpcClientT, NETDISK_CLIENT_COUNT, 169, 38>>(TRUE);
		/**/
	}


#endif
#endif
	if ((1 << (XOS_LocalSystem->m_nCurrentCpu - 1)) == USED_CPU_CORE_WATCH)
	{
		CreateLocalHereticThread<NetRamDiskPerformantsWatch>(TRUE);
	}
	Error(_T("TestNetRamDisk Exit..."));
	return;
	

}