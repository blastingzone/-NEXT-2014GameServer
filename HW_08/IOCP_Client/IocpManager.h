#pragma once
#include "ThreadLocal.h"
#include "DummyClients.h"

class Session;
class IOThread;

struct OverlappedSendContext;
struct OverlappedPreRecvContext;
struct OverlappedRecvContext;
struct OverlappedDisconnectContext;
struct OverlappedAcceptContext;

enum COMPLETION_KEY
{
	CK_NONE,
	CK_STOP_NOW = 0xDEAD
};

class IocpManager
{
public:
	IocpManager();
	~IocpManager();

	bool Initialize();
	void Finalize();

	bool StartIoThreads();
	void StopIoThreads();

	HANDLE	GetComletionPort()	{ return mCompletionPort; }
	int	GetIoThreadCount() { return mIoThreadCount; }

	static char mAcceptBuf[64];
	static LPFN_DISCONNECTEX mFnDisconnectEx;
	static LPFN_CONNECTEX mFnConnectEx;

private:

	static unsigned int WINAPI IoWorkerThread(LPVOID lpParam);

private:

	HANDLE	mCompletionPort;
	int		mIoThreadCount;

	IOThread* mIoWorkerThread[MAX_IO_THREAD];
	//std::vector<
};

extern IocpManager* GIocpManager;


BOOL DisconnectEx(SOCKET hSocket, LPOVERLAPPED lpOverlapped, DWORD dwFlags, DWORD reserved);

BOOL ConnectEx(SOCKET hSocket, const struct sockaddr* name, int namelen, PVOID lpSendBuffer, DWORD dwSendDataLength,
	LPDWORD lpdwBytesSent, LPOVERLAPPED lpOverlapped);