
#include <winsock2.h>
#include <stdio.h>
#include <vector>
#pragma comment(lib, "ws2_32.lib") 

#define MAX_CONNECTION 20
#define TOTAL_MESSAGE_BYTE 2000
#define TIMEOUT_MILLISECOND 2000

typedef struct
{
	WSAOVERLAPPED m_Overlapped;
	SOCKET	m_Socket;
	WSABUF m_WsaBuf;
	char m_RecvBuffer[TOTAL_MESSAGE_BYTE / MAX_CONNECTION];
	char m_SendBuffer[TOTAL_MESSAGE_BYTE / MAX_CONNECTION];
	DWORD m_Flag;
	int m_SessionId;

} Client_Session;

#define IOCP_WAIT_TIME INFINITE

SRWLOCK g_pLock;

DWORD g_RecvedDataByte = 0;
DWORD g_SendedDataByte = 0;

DWORD g_StartTimer;
unsigned int g_TestCount = 0;

static DWORD WINAPI ClientWorkerThread( LPVOID lpParameter )
{
	HANDLE hCompletionPort = (HANDLE)lpParameter;
	DWORD dwByteRecv = 0;
	ULONG completionKey = 0;
	Client_Session* clientSession;
	
	while ( true )
	{
		int ret = GetQueuedCompletionStatus( hCompletionPort, &dwByteRecv, &completionKey, (LPOVERLAPPED*)&clientSession, IOCP_WAIT_TIME );

		if ( ret == 0 || dwByteRecv == 0 )
		{
			int err = GetLastError();

			printf_s( "Connection Failed : %d \n", GetLastError() );
		}
		else
		{
			clientSession->m_WsaBuf.buf = clientSession->m_RecvBuffer;
			clientSession->m_WsaBuf.len = TOTAL_MESSAGE_BYTE / MAX_CONNECTION;
			clientSession->m_Flag = 0;
			int ret = WSARecv( clientSession->m_Socket, &( clientSession->m_WsaBuf ), 1, &dwByteRecv, &clientSession->m_Flag, &( clientSession->m_Overlapped ), NULL );
			if ( ret == 0 )
			{
				AcquireSRWLockExclusive( &g_pLock );

				g_RecvedDataByte += dwByteRecv;
				if ( g_TestCount < 9 )
				{
					if ( clientSession->m_WsaBuf.buf[0] == clientSession->m_SessionId )
					{
						printf_s( "Echo Data OK \n" );
						printf_s( "%s \n", clientSession->m_WsaBuf.buf );
					}
					else
					{
						printf_s( "Echo Data.... SOMETHING WRONG!\n" );
					}
					++g_TestCount;
				}

				ReleaseSRWLockExclusive( &g_pLock );

				continue;
			}

			if ( WSAGetLastError() == WSA_IO_PENDING )
			{
				continue;
			}
		}
		// disconnect
		closesocket( clientSession->m_Socket );
		break;
	}

	delete clientSession;
	
	return 0;
}

int main( void )
{
	InitializeSRWLock( &g_pLock );

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

	for ( DWORD i = 0; i < si.dwNumberOfProcessors; ++i )
	{
		HANDLE hThread = CreateThread( NULL, 0, ClientWorkerThread, hCompletionPort, 0, NULL );
		CloseHandle( hThread );
	}

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
		SockAddr.sin_family = AF_INET;
		SockAddr.sin_addr.s_addr = inet_addr( "127.0.0.1" ); //local network
		SockAddr.sin_port = htons( 9001 );

		CreateIoCompletionPort( (HANDLE)(clientSession->m_Socket), hCompletionPort, 0, 0 );

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
		clientSession->m_WsaBuf.buf = clientSession->m_SendBuffer;
		clientSession->m_WsaBuf.len = TOTAL_MESSAGE_BYTE / MAX_CONNECTION;

		DWORD sendByte = 0;
		if ( SOCKET_ERROR == WSASend( clientSession->m_Socket, &clientSession->m_WsaBuf, 1, &sendByte, clientSession->m_Flag, (LPWSAOVERLAPPED)clientSession, NULL ) )
		{
			if ( WSAGetLastError() != WSA_IO_PENDING )
			{
				closesocket( m_sessionList[i]->m_Socket );
				delete clientSession;
				printf_s( "ClientSession::PostSend Error : %d\n", GetLastError() );

				return false;
			}
		}
		else
		{
			AcquireSRWLockExclusive( &g_pLock );

			g_SendedDataByte += sendByte;
			printf_s( "Data Send OK \n" );

			ReleaseSRWLockExclusive( &g_pLock );
		}
	}

	// 타이머 초기화
	g_StartTimer = GetTickCount();

	while ( g_RecvedDataByte < TOTAL_MESSAGE_BYTE && GetTickCount() - g_StartTimer < TIMEOUT_MILLISECOND)
	{
		Sleep( 100 );
	}

	// 귀신이 곡할 노릇일세.. 왜 보낸 거보다 돌아오는게 많지 ㅋㅋㅋ
	printf_s( "Time Passed : %f \n", g_StartTimer / 1000.f );
	printf_s( "Send Data : %d (maybe...) \n", TOTAL_MESSAGE_BYTE );
	printf_s( "Real Send Data : %d \n", g_SendedDataByte );
	printf_s( "Recv Data : %d \n", g_RecvedDataByte );

	for ( int i = 0; i < m_sessionList.size(); ++i )
	{
		if ( m_sessionList[i] )
		{
			shutdown( m_sessionList[i]->m_Socket, SD_BOTH );
			closesocket( m_sessionList[i]->m_Socket );
		}
	}
	m_sessionList.clear();
	WSACleanup();

	getchar();
	return 0;
}