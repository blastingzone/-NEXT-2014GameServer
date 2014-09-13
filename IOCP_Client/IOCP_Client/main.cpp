
#include <winsock2.h>
#include <stdio.h>
#include <vector>
#include <process.h>
#pragma comment(lib, "ws2_32.lib") 

#define MAX_CONNECTION 20
#define TOTAL_MESSAGE_BYTE 2000
#define TIMEOUT_MILLISECOND 20000
#define SERVER_PORT 9000

typedef struct
{
	OVERLAPPED m_Overlapped;
	SOCKET	m_Socket;
	WSABUF m_WsaSendBuf;
	WSABUF m_WsaRecvBuf;
	char m_RecvBuffer[TOTAL_MESSAGE_BYTE / MAX_CONNECTION];
	char m_SendBuffer[TOTAL_MESSAGE_BYTE / MAX_CONNECTION];
	DWORD m_Flag;
	int m_SessionId;

	int m_TotalSendSize;
	int m_TotalRecvSize;

} Client_Session;

#define IOCP_WAIT_TIME 20

DWORD g_StartTimer;

static unsigned int WINAPI ClientWorkerThread( LPVOID lpParameter )
{
	HANDLE hCompletionPort = (HANDLE)lpParameter;
	DWORD dwByteRecv = 0;
	ULONG completionKey = 0;
	Client_Session* clientSession = nullptr;


	while ( true )
	{
		dwByteRecv = 0;
		completionKey = 0;
		clientSession = nullptr;

		int ret = GetQueuedCompletionStatus( hCompletionPort, &dwByteRecv, (PULONG_PTR)&completionKey, (LPOVERLAPPED*)&clientSession, IOCP_WAIT_TIME );

		if ( ret == 0 || dwByteRecv == 0 )
		{
			if ( WSAGetLastError() == WAIT_TIMEOUT )
			{
				continue;
			}
			else
			{
				printf_s( "Connection Failed : %d \n", WSAGetLastError() );
			}
		}
		else
		{
			clientSession->m_Flag = 0;
			if ( !WSARecv( clientSession->m_Socket, &( clientSession->m_WsaRecvBuf ), 1, &dwByteRecv, &clientSession->m_Flag, (LPWSAOVERLAPPED)( clientSession ), NULL ) )
			{
				clientSession->m_TotalRecvSize += dwByteRecv;

				continue;
			}

			if ( WSAGetLastError() == WSA_IO_PENDING )
			{
				continue;
			}
		}
		// disconnect
		if ( clientSession )
		{
			closesocket( clientSession->m_Socket );
			break;
		}
	}
	if ( clientSession != nullptr )
	{
		delete clientSession;
	}
	
	return 0;
}

int main( void )
{
	WSADATA wsadata;
	if ( WSAStartup( MAKEWORD( 2, 2 ), &wsadata ) != 0 )
	{
		return 0;
	}

	HANDLE hCompletionPort = CreateIoCompletionPort( INVALID_HANDLE_VALUE, NULL, 0, 0 );
	if ( !hCompletionPort )
	{
		return 0;
	}

	SYSTEM_INFO si;
	GetSystemInfo( &si );

	std::vector<Client_Session*> m_sessionList;

	for ( int i = 0; i < MAX_CONNECTION; ++i )
	{
		Client_Session* clientSession = new Client_Session();
		ZeroMemory( clientSession, sizeof( Client_Session ) );

		clientSession->m_Socket = WSASocket( AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED );
		if ( clientSession->m_Socket == INVALID_SOCKET )
		{
			printf_s( "Socket Create Error! %d \n", WSAGetLastError() );
			return 0;
		}

		SOCKADDR_IN SockAddr;
		ZeroMemory( &SockAddr, sizeof( SockAddr ) );
		SockAddr.sin_family = AF_INET;
		SockAddr.sin_addr.s_addr = inet_addr( "127.0.0.1" ); //local network
		SockAddr.sin_port = htons( SERVER_PORT );

		if ( hCompletionPort != CreateIoCompletionPort( (HANDLE)( clientSession->m_Socket ), hCompletionPort, (ULONG_PTR)clientSession, 0 ) )
		{
			printf_s( "Create Io Completion Port Error : %d \n", GetLastError() );
		}


		if ( WSAConnect( clientSession->m_Socket, (SOCKADDR*)( &SockAddr ), sizeof( SockAddr ), NULL, NULL, NULL, NULL ) == SOCKET_ERROR )
		{
			printf_s( "Connection Error! %d \n",WSAGetLastError() );
			return 0;
		}

		clientSession->m_Overlapped.hEvent = WSACreateEvent();
		//clientSession->m_WsaBuf.buf = clientSession->m_Buffer;
		//clientSession->m_WsaBuf.len = sizeof( clientSession->m_WsaBuf );
		
		// id 할당
		clientSession->m_SessionId = i % 10 + 1;

		// send 버퍼에 글자 채우기
		for ( int j = 0; j < TOTAL_MESSAGE_BYTE / MAX_CONNECTION; ++j )
		{
			clientSession->m_SendBuffer[j] = clientSession->m_SessionId;
		}
		clientSession->m_SendBuffer[TOTAL_MESSAGE_BYTE / MAX_CONNECTION - 1] = '\0';
		clientSession->m_WsaSendBuf.buf = clientSession->m_SendBuffer;
		clientSession->m_WsaSendBuf.len = TOTAL_MESSAGE_BYTE / MAX_CONNECTION;

		clientSession->m_WsaRecvBuf.buf = clientSession->m_RecvBuffer;
		clientSession->m_WsaRecvBuf.len = TOTAL_MESSAGE_BYTE / MAX_CONNECTION;
		DWORD sendByte = 0;

		if ( SOCKET_ERROR == WSASend( clientSession->m_Socket, &clientSession->m_WsaSendBuf, 1, &sendByte, clientSession->m_Flag, (LPWSAOVERLAPPED)clientSession, NULL ) )
		{
			if ( WSAGetLastError() != WSA_IO_PENDING )
			{
				closesocket( m_sessionList[i]->m_Socket );
				delete clientSession;
				printf_s( "ClientSession::PostSend Error : %d\n", GetLastError() );

				return -1;
			}
		}
		else
		{
			clientSession->m_TotalSendSize += sendByte;
			printf_s( "Data Send OK \n" );
			m_sessionList.push_back( clientSession );
		}
		
	}

	for ( DWORD i = 0; i < si.dwNumberOfProcessors; ++i )
	{
		DWORD dwThreadId;
		HANDLE hThread = (HANDLE)_beginthreadex( NULL, 0, ClientWorkerThread, hCompletionPort, 0, (unsigned int*)&dwThreadId );
		CloseHandle( hThread );
	}

	// 타이머 초기화
	g_StartTimer = GetTickCount();

	while ( GetTickCount() - g_StartTimer < TIMEOUT_MILLISECOND)
	{
		Sleep( 100 );
	}

	for ( int i = 0; i < m_sessionList.size(); ++i )
	{
		if ( m_sessionList[i] )
		{
			shutdown( m_sessionList[i]->m_Socket, SD_BOTH );
			closesocket( m_sessionList[i]->m_Socket );
		}
	}


	printf_s( "Time Passed : %f \n", g_StartTimer / 1000.f );
	printf_s( "Send Data : %d (maybe...) \n", TOTAL_MESSAGE_BYTE );

	// 보내고 받은 데이터 전부 더해서 총 데이터 전송량 계산
	UINT64 sendDataByte = 0;
	UINT64 recvDataByte = 0;
	for ( int i = 0; i < MAX_CONNECTION; ++i )
	{
		recvDataByte += m_sessionList[i]->m_TotalRecvSize;
		sendDataByte += m_sessionList[i]->m_TotalSendSize;
	}

	printf_s( "Real Send Data : %d \n", sendDataByte );
	printf_s( "Recv Data : %d \n", recvDataByte );


	m_sessionList.clear();
	WSACleanup();

	getchar();
	return 0;
}