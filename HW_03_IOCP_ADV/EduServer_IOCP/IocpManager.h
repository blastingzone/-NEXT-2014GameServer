#pragma once

class ClientSession;

struct OverlappedSendContext;
struct OverlappedPreRecvContext;
struct OverlappedRecvContext;
struct OverlappedDisconnectContext;
struct OverlappedAcceptContext;

class IocpManager
{
public:
	IocpManager();
	~IocpManager();

	bool Initialize();
	void Finalize();

	bool StartIoThreads();
	void StartAccept();


	HANDLE GetComletionPort()	{ return mCompletionPort; }
	int	GetIoThreadCount()		{ return mIoThreadCount;  }

	SOCKET* GetListenSocket()  { return &mListenSocket;  }

	BOOL DisconnectEx( SOCKET hSocket, LPOVERLAPPED lpOverlapped, DWORD dwFlags, DWORD reserved );
	BOOL AcceptEx( SOCKET sListenSocket, SOCKET sAcceptSocket, PVOID lpOutputBuffer, DWORD dwReceiveDataLength, DWORD dwLocalAddressLength, DWORD dwRemoteAddressLength, LPDWORD lpdwBytesReceived, LPOVERLAPPED lpOverlapped );

private:

	static unsigned int WINAPI IoWorkerThread(LPVOID lpParam);

	static bool PreReceiveCompletion(ClientSession* client, OverlappedPreRecvContext* context, DWORD dwTransferred);
	static bool ReceiveCompletion(ClientSession* client, OverlappedRecvContext* context, DWORD dwTransferred);
	static bool SendCompletion(ClientSession* client, OverlappedSendContext* context, DWORD dwTransferred);
	
private:

	HANDLE	mCompletionPort;
	int		mIoThreadCount;

	SOCKET	mListenSocket;

	// 함수 포인터 추가
	LPFN_ACCEPTEX mLpfnAcceptEx = NULL;
	LPFN_DISCONNECTEX mLpfnDisconnectEx = NULL;
};

extern __declspec(thread) int LIoThreadId;
extern IocpManager* GIocpManager;
