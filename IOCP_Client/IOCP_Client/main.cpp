
#include <winsock2.h>
#include <stdio.h>
#include <vector>
#include <process.h>
#include "MyPacket.pb.h"
#include "google\protobuf\io\coded_stream.h"
#include "google/protobuf/io/zero_copy_stream_impl_lite.h"
#include "google/protobuf/text_format.h"
#include <string>

#pragma comment(lib, "ws2_32.lib") 

#define MAX_CONNECTION 20
#define TOTAL_MESSAGE_BYTE 20000
#define MAX_BUFFER_SIZE 4096
#define TIMEOUT_MILLISECOND 40000

#define SERVER_PORT 9000

typedef struct
{
	float posX;
	float posY;
	float posZ;
} Player_Position;

typedef struct
{
	OVERLAPPED m_Overlapped;
	SOCKET	m_Socket;
	WSABUF m_WsaSendBuf;
	WSABUF m_WsaRecvBuf;
	//char m_RecvBuffer[MAX_BUFFER_SIZE];
	//char m_SendBuffer[MAX_BUFFER_SIZE];
	google::protobuf::uint8 m_SendBuffer[MAX_BUFFER_SIZE];
	google::protobuf::uint8 m_RecvBuffer[MAX_BUFFER_SIZE];
	DWORD m_Flag;
	int m_SessionId;

	Player_Position m_Position;

	int m_TotalSendSize;
	int m_TotalRecvSize;

	google::protobuf::io::ArrayOutputStream* m_pOutputArrayStream;
	google::protobuf::io::CodedOutputStream* m_pOutputCodedStream;

	google::protobuf::io::ArrayInputStream* m_pInputArrayStream;
	google::protobuf::io::CodedInputStream* m_pInputCodedStream;
} Client_Session;

std::vector<Client_Session*> g_sessionList;

void deleteClientSession( Client_Session* session )
{

	closesocket( session->m_Socket );

	if ( session->m_pInputCodedStream )
	{
		delete session->m_pInputCodedStream;
		session->m_pInputCodedStream = nullptr;
	}

	if ( session->m_pInputArrayStream )
	{
		delete session->m_pInputArrayStream;
		session->m_pInputArrayStream = nullptr;
	}

	if ( session->m_pOutputCodedStream )
	{
		delete session->m_pOutputCodedStream;
		session->m_pOutputCodedStream = nullptr;
	}

	if ( session->m_pOutputArrayStream )
	{
		delete session->m_pOutputArrayStream;
		session->m_pOutputArrayStream = nullptr;
	}

	delete session;
	session = nullptr;
}

struct MessageHeader
{
	google::protobuf::uint32 size;
	MyPacket::MessageType type;
};

const int MessageHeaderSize = sizeof( MessageHeader );

#define IOCP_WAIT_TIME 20

DWORD g_StartTimer;


void WriteMessageToStream(
	MyPacket::MessageType msgType,
	const google::protobuf::MessageLite& message,
	google::protobuf::io::CodedOutputStream& stream )
{
	MessageHeader messageHeader;
	messageHeader.size = message.ByteSize();
	messageHeader.type = msgType;
	stream.WriteRaw( &messageHeader, sizeof( MessageHeader ) );
	message.SerializeToCodedStream( &stream );
}

void InitSendBufferOnly( Client_Session* clientSession )
{
	delete clientSession->m_pOutputCodedStream;
	delete clientSession->m_pOutputArrayStream;

	clientSession->m_pOutputArrayStream = new google::protobuf::io::ArrayOutputStream( clientSession->m_SendBuffer, MAX_BUFFER_SIZE );
	clientSession->m_pOutputCodedStream = new google::protobuf::io::CodedOutputStream( clientSession->m_pOutputArrayStream );
}


void PacketHandler( google::protobuf::io::CodedInputStream &codedInputStream, Client_Session* clientSession )
{
	MessageHeader messageHeader;

	while ( codedInputStream.ReadRaw( &messageHeader, MessageHeaderSize ) )
	{
		if ( clientSession == NULL )
		{
			printf_s( "NULL 이네요\n" );
		}

		const void* pPacket = NULL;
		int remainSize = 0;
		bool IsOK = false;
		IsOK = codedInputStream.GetDirectBufferPointer( &pPacket, &remainSize );

		if ( IsOK == false )
		{
			break;
		}

		if ( remainSize < (signed)messageHeader.size )
		{
			printf_s( "buffer remain size is lesser than Message!" );
			break;
		}

		google::protobuf::io::ArrayInputStream payloadArrayStream( pPacket, messageHeader.size );
		google::protobuf::io::CodedInputStream payloadInputStream( &payloadArrayStream );

		codedInputStream.Skip( messageHeader.size );

		switch ( messageHeader.type )
		{
		case MyPacket::MessageType::PKT_SC_LOGIN:
		{
			MyPacket::LoginResult message;
			if ( false == message.ParseFromCodedStream( &payloadInputStream ) )
				break;
			
			codedInputStream.ConsumedEntireMessage();

			printf( "Login Success ! Player Id : %d Session Id : %d \n", message.playerid(), clientSession->m_SessionId );
			printf( "Player Name : %s \n", message.playername() );
			printf( "Player Position : %f, %f, %f \n", message.playerpos().x(), message.playerpos().y(), message.playerpos().z() );

			clientSession->m_Position.posX = message.playerpos().x();
			clientSession->m_Position.posY = message.playerpos().y();
			clientSession->m_Position.posZ = message.playerpos().z();

			// 로그인 성공하면 채팅을 보내기 시작

			MyPacket::ChatRequest chatPacket;
			std::string chat( "chatting!" );
			chat.append( "session id : " );
			chat.append( std::to_string(clientSession->m_SessionId) );

			chatPacket.set_playerid( clientSession->m_SessionId );
			chatPacket.set_playermessage(chat.c_str());

			// 주의 : 본래 여기서 send buffer를 비우면 안 된다.
			// recv 로직 중에 send 버퍼를 비워버리면 실제 send가 비동기로 잘 돌아가는 도중에 먹는거 뺏아가는 수가 있음;;
			// but 이 클라이언트는 하는 일이 간단해서 통제가 가능하다. 여기로 들어왔다는건 send가 끝났다는 게 보장된다.
			InitSendBufferOnly( clientSession );

			WriteMessageToStream( MyPacket::MessageType::PKT_CS_CHAT, chatPacket, *(clientSession->m_pOutputCodedStream) );

			clientSession->m_WsaSendBuf.buf = (CHAR*)( clientSession->m_SendBuffer );
			clientSession->m_WsaSendBuf.len = clientSession->m_pOutputCodedStream->ByteCount();

			clientSession->m_WsaRecvBuf.buf = (CHAR*)( clientSession->m_RecvBuffer );
			clientSession->m_WsaRecvBuf.len = MAX_BUFFER_SIZE;
			DWORD sendByte = 0;

			if ( SOCKET_ERROR == WSASend( clientSession->m_Socket, &clientSession->m_WsaSendBuf, 1, &sendByte, clientSession->m_Flag, (LPWSAOVERLAPPED)clientSession, NULL ) )
			{
				if ( WSAGetLastError() != WSA_IO_PENDING )
				{
					deleteClientSession( clientSession );
					printf_s( "MyPacket::MessageType::PKT_SC_LOGIN Handling Error : %d\n", GetLastError() );
				}
			}
			else
			{
				clientSession->m_TotalSendSize += sendByte;
				printf_s( " Chat Data Send OK \n" );
			}

			
			break;
		}
		//서버에 only 에코 기능만 있을 때 테스트용
		case MyPacket::MessageType::PKT_CS_LOGIN:
		{
			MyPacket::LoginRequest message;
			if ( false == message.ParseFromCodedStream( &payloadInputStream ) )
				break;

			codedInputStream.ConsumedEntireMessage();

			printf("Player Id : %d \n", message.playerid());
			break;
		}
		case MyPacket::MessageType::PKT_SC_CHAT:
		{
			MyPacket::ChatResult message;
			if ( false == message.ParseFromCodedStream( &payloadInputStream ) )
				break;
			
			codedInputStream.ConsumedEntireMessage();

			printf_s( "[CHAT] [%s] : %s \n", message.playername(), message.playermessage().c_str() );

			// 채팅 성공하면 이동을 보내기 시작

			clientSession->m_Position.posX = ( clientSession->m_Position.posX + 1.0f );
			clientSession->m_Position.posY = ( 0.0f );
			clientSession->m_Position.posZ = ( clientSession->m_Position.posZ + 1.0f );

			MyPacket::MoveRequest movePacket;
			movePacket.set_playerid( clientSession->m_SessionId );

			movePacket.mutable_playerpos()->set_x( clientSession->m_Position.posX );
			movePacket.mutable_playerpos()->set_y( clientSession->m_Position.posY );
			movePacket.mutable_playerpos()->set_z( clientSession->m_Position.posZ );

			InitSendBufferOnly( clientSession );

			WriteMessageToStream( MyPacket::MessageType::PKT_CS_CHAT, movePacket, *( clientSession->m_pOutputCodedStream ) );

			clientSession->m_WsaSendBuf.buf = (CHAR*)( clientSession->m_SendBuffer );
			clientSession->m_WsaSendBuf.len = clientSession->m_pOutputCodedStream->ByteCount();

			clientSession->m_WsaRecvBuf.buf = (CHAR*)( clientSession->m_RecvBuffer );
			clientSession->m_WsaRecvBuf.len = MAX_BUFFER_SIZE;
			DWORD sendByte = 0;

			if ( SOCKET_ERROR == WSASend( clientSession->m_Socket, &clientSession->m_WsaSendBuf, 1, &sendByte, clientSession->m_Flag, (LPWSAOVERLAPPED)clientSession, NULL ) )
			{
				if ( WSAGetLastError() != WSA_IO_PENDING )
				{
					deleteClientSession( clientSession );
					printf_s( "MyPacket::MessageType::PKT_SC_LOGIN Handling Error : %d\n", GetLastError() );
				}
			}
			else
			{
				clientSession->m_TotalSendSize += sendByte;
				printf_s( " Move Data Send OK \n" );
			}
			break;
		}
		case MyPacket::MessageType::PKT_SC_MOVE:
		{
			MyPacket::MoveResult message;
			if ( false == message.ParseFromCodedStream( &payloadInputStream ) )
				break;

			codedInputStream.ConsumedEntireMessage();


			// 이동 성공하면 채팅을 보내기 시작

			MyPacket::ChatRequest chatPacket;
			std::string chat( "chatting!" );
			chat.append( "session id : " );
			chat.append( std::to_string( clientSession->m_SessionId ) );

			chatPacket.set_playerid( clientSession->m_SessionId );
			chatPacket.set_playermessage( chat.c_str() );

			// 주의 : 본래 여기서 send buffer를 비우면 안 된다.
			// recv 로직 중에 send 버퍼를 비워버리면 실제 send가 비동기로 잘 돌아가는 도중에 먹는거 뺏아가는 수가 있음;;
			// but 이 클라이언트는 하는 일이 간단해서 통제가 가능하다. 여기로 들어왔다는건 send가 끝났다는 게 보장된다.
			InitSendBufferOnly( clientSession );

			WriteMessageToStream( MyPacket::MessageType::PKT_CS_CHAT, chatPacket, *( clientSession->m_pOutputCodedStream ) );

			clientSession->m_WsaSendBuf.buf = (CHAR*)( clientSession->m_SendBuffer );
			clientSession->m_WsaSendBuf.len = clientSession->m_pOutputCodedStream->ByteCount();

			clientSession->m_WsaRecvBuf.buf = (CHAR*)( clientSession->m_RecvBuffer );
			clientSession->m_WsaRecvBuf.len = MAX_BUFFER_SIZE;
			DWORD sendByte = 0;

			if ( SOCKET_ERROR == WSASend( clientSession->m_Socket, &clientSession->m_WsaSendBuf, 1, &sendByte, clientSession->m_Flag, (LPWSAOVERLAPPED)clientSession, NULL ) )
			{
				if ( WSAGetLastError() != WSA_IO_PENDING )
				{
					deleteClientSession( clientSession );
					printf_s( "MyPacket::MessageType::PKT_SC_LOGIN Handling Error : %d\n", GetLastError() );
				}
			}
			else
			{
				clientSession->m_TotalSendSize += sendByte;
				printf_s( " Chat Data Send OK \n" );
			}


			break;
		}
		}

	}
}


static unsigned int WINAPI ClientWorkerThread( LPVOID lpParameter )
{
	while ( true )
	{
		HANDLE hCompletionPort = (HANDLE)lpParameter;
		DWORD dwByteRecv = 0;
		ULONG completionKey = 0;
		LPOVERLAPPED overlapped = nullptr;

		int ret = GetQueuedCompletionStatus( hCompletionPort, &dwByteRecv, (PULONG_PTR)&completionKey, &overlapped, IOCP_WAIT_TIME );

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
			Client_Session* session = reinterpret_cast<Client_Session*>( overlapped );
			session->m_Flag = 0;

			if ( !WSARecv( session->m_Socket, &( session->m_WsaRecvBuf ), 1, &dwByteRecv, &session->m_Flag, (LPWSAOVERLAPPED)( session ), NULL ) )
			{
				PacketHandler( *( session->m_pInputCodedStream ), session );
				session->m_TotalRecvSize += dwByteRecv;

				continue;
			}

			if ( WSAGetLastError() == WSA_IO_PENDING )
			{
				continue;
			}
		}
	}

	return 0;
}

void DoSpreadChat()
{
	
}

void DoSpreadMove()
{

}

void DoIntervalJob()
{
	DoSpreadChat();
	DoSpreadMove();
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
			printf_s( "Connection Error! %d \n", WSAGetLastError() );
			return 0;
		}

		clientSession->m_Overlapped.hEvent = WSACreateEvent();
		//clientSession->m_WsaBuf.buf = clientSession->m_Buffer;
		//clientSession->m_WsaBuf.len = sizeof( clientSession->m_WsaBuf );

		// id 할당
		clientSession->m_SessionId = i;

		// 출력 / 입력 스트림 생성
		clientSession->m_pOutputArrayStream = new google::protobuf::io::ArrayOutputStream( clientSession->m_SendBuffer, MAX_BUFFER_SIZE );
		clientSession->m_pOutputCodedStream = new google::protobuf::io::CodedOutputStream( clientSession->m_pOutputArrayStream );
		
		clientSession->m_pInputArrayStream = new google::protobuf::io::ArrayInputStream( clientSession->m_RecvBuffer, MAX_BUFFER_SIZE );
		clientSession->m_pInputCodedStream = new google::protobuf::io::CodedInputStream( clientSession->m_pInputArrayStream );

		// 로그인 패킷 보내기
		MyPacket::LoginRequest loginPacket;
		loginPacket.set_playerid( clientSession->m_SessionId );

		WriteMessageToStream( MyPacket::MessageType::PKT_CS_LOGIN, loginPacket, *(clientSession->m_pOutputCodedStream) );

		// send 버퍼에 글자 채우기(old)
// 		for ( int j = 0; j < TOTAL_MESSAGE_BYTE / MAX_CONNECTION; ++j )
// 		{
// 			clientSession->m_SendBuffer[j] = clientSession->m_SessionId;
// 		}
// 		clientSession->m_SendBuffer[TOTAL_MESSAGE_BYTE / MAX_CONNECTION - 1] = '\0';

		clientSession->m_WsaSendBuf.buf = (CHAR*)(clientSession->m_SendBuffer);
		clientSession->m_WsaSendBuf.len = clientSession->m_pOutputCodedStream->ByteCount();

		clientSession->m_WsaRecvBuf.buf = (CHAR*)( clientSession->m_RecvBuffer );
		clientSession->m_WsaRecvBuf.len = MAX_BUFFER_SIZE;
		DWORD sendByte = 0;

		if ( SOCKET_ERROR == WSASend( clientSession->m_Socket, &clientSession->m_WsaSendBuf, 1, &sendByte, clientSession->m_Flag, (LPOVERLAPPED)clientSession, NULL ) )
		{
			if ( WSAGetLastError() != WSA_IO_PENDING )
			{
				deleteClientSession( clientSession );
				printf_s( "ClientSession::PostSend Error : %d\n", GetLastError() );

				return -1;
			}
		}
		else
		{
			clientSession->m_TotalSendSize += sendByte;
			printf_s( "Data Send OK ID : %d \n", clientSession->m_SessionId );
			g_sessionList.push_back( clientSession );
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

	while ( GetTickCount() - g_StartTimer < TIMEOUT_MILLISECOND )
	{
		Sleep( 1000 );
		DoIntervalJob();
	}

	for ( int i = 0; i < g_sessionList.size(); ++i )
	{
		if ( g_sessionList[i] )
		{
			shutdown( g_sessionList[i]->m_Socket, SD_BOTH );
			closesocket( g_sessionList[i]->m_Socket );
		}
	}

	printf_s( "Time Passed : %f \n", g_StartTimer / 1000.f );
	printf_s( "Send Data : %d (maybe...) \n", TOTAL_MESSAGE_BYTE );

	// 보내고 받은 데이터 전부 더해서 총 데이터 전송량 계산
	UINT64 sendDataByte = 0;
	UINT64 recvDataByte = 0;
	for ( int i = 0; i < MAX_CONNECTION; ++i )
	{
		recvDataByte += g_sessionList[i]->m_TotalRecvSize;
		sendDataByte += g_sessionList[i]->m_TotalSendSize;
	}

	printf_s( "Real Send Data : %d \n", sendDataByte );
	printf_s( "Recv Data : %d \n", recvDataByte );

	for ( int i = 0; i < MAX_CONNECTION; ++i )
	{
		deleteClientSession( g_sessionList[i] );
	}

	g_sessionList.clear();
	WSACleanup();

	getchar();
	return 0;
}