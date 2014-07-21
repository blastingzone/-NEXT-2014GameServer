#include "stdafx.h"
#include "IocpManager.h"
#include "EduServer_IOCP.h"
#include "ClientSession.h"
#include "SessionManager.h"
#include <sysinfoapi.h>

#define GQCS_TIMEOUT	20

__declspec(thread) int LIoThreadId = 0; ///< 이건 어디 쓰려고?

IocpManager* GIocpManager = nullptr;

IocpManager::IocpManager() : mCompletionPort(NULL), mIoThreadCount(2), mListenSocket(NULL)
{
}


IocpManager::~IocpManager()
{
}

bool IocpManager::Initialize()
{
	//GetSystemInfo사용해서 set num of I/O threads
	SYSTEM_INFO sysInfo;
	GetSystemInfo(&sysInfo);
	//echo서버이기 때문에 그냥 코어 갯수를 그대로 쓰레드 카운트로 사용
	//쓰레드루프에 sleep이 있을 경우 2n, 2n+1등을 사용
	mIoThreadCount = sysInfo.dwNumberOfProcessors;

	/// winsock initializing
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return false;

	/// Create I/O Completion Port
	//마지막 인자를 0으로 두면 시스템코어의 갯수만큼 자동으로 setting
	mCompletionPort = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	if ( NULL == mCompletionPort )
	{
		printf( "CreateIoCompletionPort failed : %d\n", GetLastError() );
		return false;
	}

	/// create TCP socket	
	mListenSocket = WSASocket( AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED );

	int opt = 1;
	setsockopt(mListenSocket, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(int));

	///bind
	SOCKADDR_IN serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
	serveraddr.sin_port = htons(LISTEN_PORT);

	if (SOCKET_ERROR == bind(mListenSocket, (SOCKADDR*)&serveraddr, sizeof(serveraddr)))
		return false;

	return true;
}


bool IocpManager::StartIoThreads()
{
	/// I/O Thread
	for (int i = 0; i < mIoThreadCount; ++i)
	{
		DWORD dwThreadId;

		HANDLE hThread = (HANDLE)_beginthreadex( NULL, 0, IoWorkerThread, mCompletionPort, 0, (unsigned*)&dwThreadId );
		if (NULL == hThread)
		{
			printf(" Thread Create Error : %d \n", GetLastError());

			return false;
		}
		
		CloseHandle(hThread);
	}

	return true;
}


bool IocpManager::StartAcceptLoop()
{
	/// listen
	if (SOCKET_ERROR == listen(mListenSocket, SOMAXCONN))
		return false;


	/// accept loop
	while (true)
	{
		SOCKET acceptedSock = accept(mListenSocket, NULL, NULL);
		if (acceptedSock == INVALID_SOCKET)
		{
			printf_s("accept: invalid socket\n");
			continue;
		}

		SOCKADDR_IN clientaddr;
		int addrlen = sizeof(clientaddr);
		getpeername(acceptedSock, (SOCKADDR*)&clientaddr, &addrlen);

		/// 소켓 정보 구조체 할당과 초기화
		ClientSession* client = GSessionManager->CreateClientSession(acceptedSock);

		/// 클라 접속 처리
		if (false == client->OnConnect(&clientaddr))
		{
			client->Disconnect(DR_ONCONNECT_ERROR);
			GSessionManager->DeleteClientSession(client);
		}
	}

	return true;
}

void IocpManager::Finalize()
{
	CloseHandle(mCompletionPort);

	/// winsock finalizing
	WSACleanup();

}


unsigned int WINAPI IocpManager::IoWorkerThread(LPVOID lpParam)
{
	LThreadType = THREAD_IO_WORKER;

	//넘겨준 dwThreadId의 주솟값을 id로 사용
	LIoThreadId = reinterpret_cast<int>(lpParam);
	HANDLE hComletionPort = GIocpManager->GetComletionPort();

	while ( true )
	{
		DWORD dwTransferred = 0;
		OverlappedIOContext* context = nullptr;
		ClientSession* asCompletionKey = nullptr;

		//int ret = 0;
		int ret = GetQueuedCompletionStatus(
			hComletionPort,
			&dwTransferred,
			(PULONG_PTR)&asCompletionKey,
			(LPOVERLAPPED*)&context, //항상 1번 주소는 overlap구조체이어야 함
			GQCS_TIMEOUT);

		/// check time out first
		//
		if ( ret == 0 && GetLastError() == WAIT_TIMEOUT )
			continue;

		if ( ret == 0 || dwTransferred == 0 )
		{
			/// connection closing
			asCompletionKey->Disconnect( DR_RECV_ZERO );
			GSessionManager->DeleteClientSession( asCompletionKey );
			continue;
		}

		//여기 들어오는 경우는 iocp객체가 사라졌을 경우
		//쓰레드 종료
		if ( nullptr == context )
			return 0;


		bool completionOk = true;
		switch (context->mIoType)
		{
		case IO_SEND:
			completionOk = SendCompletion(asCompletionKey, context, dwTransferred);
			break;

		case IO_RECV:
			completionOk = ReceiveCompletion(asCompletionKey, context, dwTransferred);
			break;

		default:
			printf_s("Unknown I/O Type: %d\n", context->mIoType);
			break;
		}

		if ( !completionOk )
		{
			/// connection closing
			asCompletionKey->Disconnect(DR_COMPLETION_ERROR);
			GSessionManager->DeleteClientSession(asCompletionKey);
		}

	}

	return 0;
}

bool IocpManager::ReceiveCompletion(const ClientSession* client, OverlappedIOContext* context, DWORD dwTransferred)
{

	/// echo back 처리 client->PostSend()사용.
	if ( !client->PostSend( context->mWsaBuf.buf, dwTransferred ) ) ///< 엄밀히 말하면 mWsaBuf가 아니라 mBuffer의 데이터를 보내는 것이다.
	{
		delete context;
		return false;
	}

	delete context;
	return client->PostRecv();
}

bool IocpManager::SendCompletion(const ClientSession* client, OverlappedIOContext* context, DWORD dwTransferred)
{
	/// 전송 다 되었는지 확인하는 것 처리..
	if ( context->mWsaBuf.len != dwTransferred )
	{
		//보낸 만큼의 뒤에서 계속해서 전송
		if (!client->PostSend(context->mBuffer + dwTransferred, context->mWsaBuf.len - dwTransferred)) { ///< 이렇게 처리하면 동시에 2개의 스레드에서 하나의 소켓에 대해 send를 할 수 있다.
			delete context;
			return false;
		}
	}

	delete context;
	return true;
}
