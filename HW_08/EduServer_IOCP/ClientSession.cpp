#include "stdafx.h"
#include "Exception.h"
#include "Log.h"
#include "EduServer_IOCP.h"
#include "OverlappedIOContext.h"
#include "ClientSession.h"
#include "IocpManager.h"
#include "ClientSessionManager.h"
#include "Player.h"
#include "MyPacket.pb.h"
#include "Map.h"

#define CLIENT_BUFSIZE	65536

ClientSession::ClientSession() : Session(CLIENT_BUFSIZE, CLIENT_BUFSIZE), mPlayer(this)
{
	memset(&mClientAddr, 0, sizeof(SOCKADDR_IN));
}

ClientSession::~ClientSession()
{
}

void ClientSession::SessionReset()
{
	TRACE_THIS;

	mConnected = 0;
	mRefCount = 0;
	memset(&mClientAddr, 0, sizeof(SOCKADDR_IN));

	mRecvBuffer.BufferReset();

	mSendBufferLock.EnterWriteLock();
	mSendBuffer.BufferReset();
	mSendBufferLock.LeaveWriteLock();

	ProtobufReleae();
	ProtobufInit();

	LINGER lingerOption;
	lingerOption.l_onoff = 1;
	lingerOption.l_linger = 0;

	/// no TCP TIME_WAIT
	if (SOCKET_ERROR == setsockopt(mSocket, SOL_SOCKET, SO_LINGER, (char*)&lingerOption, sizeof(LINGER)))
	{
		printf_s("[DEBUG] setsockopt linger option error: %d\n", GetLastError());
	}
	closesocket(mSocket);

	mSocket = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED);

	mPlayer.PlayerReset();
}

bool ClientSession::PostAccept()
{
	CRASH_ASSERT(LThreadType == THREAD_MAIN);

	OverlappedAcceptContext* acceptContext = new OverlappedAcceptContext(this);
	DWORD bytes = 0;
	DWORD flags = 0;
	acceptContext->mWsaBuf.len = 0;
	acceptContext->mWsaBuf.buf = nullptr;

	if (FALSE == AcceptEx(*GIocpManager->GetListenSocket(), mSocket, GIocpManager->mAcceptBuf, 0,
		sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &bytes, (LPOVERLAPPED)acceptContext))
	{
		if (WSAGetLastError() != WSA_IO_PENDING)
		{
			DeleteIoContext(acceptContext);
			printf_s("AcceptEx Error : %d\n", GetLastError());

			return false;
		}
	}

	return true;
}

void ClientSession::AcceptCompletion()
{
	TRACE_THIS;

	CRASH_ASSERT(LThreadType == THREAD_IO_WORKER);

	if (1 == InterlockedExchange(&mConnected, 1))
	{
		/// already exists?
		CRASH_ASSERT(false);
		return;
	}

	bool resultOk = true;
	do
	{
		if (SOCKET_ERROR == setsockopt(mSocket, SOL_SOCKET, SO_UPDATE_ACCEPT_CONTEXT, (char*)GIocpManager->GetListenSocket(), sizeof(SOCKET)))
		{
			printf_s("[DEBUG] SO_UPDATE_ACCEPT_CONTEXT error: %d\n", GetLastError());
			resultOk = false;
			break;
		}

		int opt = 1;
		if (SOCKET_ERROR == setsockopt(mSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, sizeof(int)))
		{
			printf_s("[DEBUG] TCP_NODELAY error: %d\n", GetLastError());
			resultOk = false;
			break;
		}

		opt = 0;
		if (SOCKET_ERROR == setsockopt(mSocket, SOL_SOCKET, SO_RCVBUF, (const char*)&opt, sizeof(int)))
		{
			printf_s("[DEBUG] SO_RCVBUF change error: %d\n", GetLastError());
			resultOk = false;
			break;
		}

		int addrlen = sizeof(SOCKADDR_IN);
		if (SOCKET_ERROR == getpeername(mSocket, (SOCKADDR*)&mClientAddr, &addrlen))
		{
			printf_s("[DEBUG] getpeername error: %d\n", GetLastError());
			resultOk = false;
			break;
		}

		HANDLE handle = CreateIoCompletionPort((HANDLE)mSocket, GIocpManager->GetComletionPort(), (ULONG_PTR)this, 0);
		if (handle != GIocpManager->GetComletionPort())
		{
			printf_s("[DEBUG] CreateIoCompletionPort error: %d\n", GetLastError());
			resultOk = false;
			break;
		}

	} while (false);


	if (!resultOk)
	{
		DisconnectRequest(DR_ONCONNECT_ERROR);
		return;
	}

	printf_s("[DEBUG] Client Connected: IP=%s, PORT=%d\n", inet_ntoa(mClientAddr.sin_addr), ntohs(mClientAddr.sin_port));

	if (false == PreRecv())
	{
		printf_s("[DEBUG] PreRecv error: %d\n", GetLastError());
	}


	//TEST: 요놈의 위치는 원래 C_LOGIN 핸들링 할 때 해야하는거지만 지금은 접속 완료 시점에서 테스트 ㄱㄱ

	//todo: 플레이어 id는 여러분의 플레이어 테이블 상황에 맞게 적절히 고쳐서 로딩하도록 
	//static int id = 101;
	//mPlayer.RequestLoad(id++);
}

void ClientSession::OnDisconnect(DisconnectReason dr)
{
	TRACE_THIS;

	printf_s("[DEBUG] Client Disconnected: Reason=%d IP=%s, PORT=%d \n", dr, inet_ntoa(mClientAddr.sin_addr), ntohs(mClientAddr.sin_port));
}

void ClientSession::OnRelease()
{
	TRACE_THIS;

	GClientSessionManager->ReturnClientSession(this);
	ProtobufReleae();
}


void ClientSession::PacketHandler()
{
	MessageHeader messageHeader;

	char* start = mRecvBuffer.GetBufferStart();
	memcpy( &messageHeader, start, MessageHeaderSize );
	mRecvBuffer.Remove( MessageHeaderSize );

	const void* pPacket = mRecvBuffer.GetBufferStart();
	int remainSize = 0;

	google::protobuf::io::ArrayInputStream payloadArrayStream( pPacket, messageHeader.size );
	google::protobuf::io::CodedInputStream payloadInputStream( &payloadArrayStream );

	switch ( messageHeader.type )
	{
	case MyPacket::MessageType::PKT_CS_LOGIN:
	{
		MyPacket::LoginRequest message;
		if ( false == message.ParseFromCodedStream( &payloadInputStream ) )
			break;
		printf( "Player Id : %d \n", message.playerid() );

		// 이제 db 에서 저 플레이어의 이름을 받아와서 LoginResult에 채운 다음 보내줘야 함
		MyPacket::LoginResult loginResult;

		loginResult.set_playerid( message.playerid() );

		// test name
		char* name = "kim";
		loginResult.set_playername( name );

		MyPacket::Position position;
		position.set_x( 1.0f );
		position.set_y( 2.0f );
		position.set_z( 3.0f );

		// MyToDo: message 안에 message 쓰는 방법을 찾아봐야한다...!
		loginResult.mutable_playerpos()->set_x(1.0f);
		loginResult.mutable_playerpos()->set_y( 2.0f );
		loginResult.mutable_playerpos()->set_z( 3.0f );

		WriteMessageToStream( MyPacket::MessageType::PKT_SC_LOGIN, loginResult, *m_pCodedOutputStream );

		// CircularBuffer랑 protobuf를 같이 쓰는 방법을 찾아보자
		// 하하 그런건 없었습니다!
		if ( false == PostSend( (const char*)m_SessionBuffer, loginResult.ByteSize() + MessageHeaderSize ) )
			break;

		break;
	}
	case MyPacket::MessageType::PKT_CS_CHAT:
	{
		MyPacket::ChatRequest message;
		if ( false == message.ParseFromCodedStream( &payloadInputStream ) )
			break;

		std::string chat = message.playermessage();
		ZonePtr zone = GMap->GetZone(mPlayer.mPosX, mPlayer.mPosY);

		for (auto iter : zone->mPlayerList)
		{
			PlayerPtr player();

			MyPacket::ChatRequest chatPacket;
			chat.append("아이디는 %d", mPlayer.GetPlayerId());

			chatPacket.set_playerid(mPlayer.GetPlayerId());
			chatPacket.set_playermessage(chat.c_str());

			//WriteMessageToStream(MyPacket::MessageType::PKT_CS_CHAT, chatPacket, );
		}
		
		break;
	}
	case MyPacket::MessageType::PKT_CS_MOVE:
	{
		MyPacket::MoveRequest message;
		if ( false == message.ParseFromCodedStream( &payloadInputStream ) )
			break;

		mPlayer.RequestUpdatePosition(message.playerpos().x(), message.playerpos().y(), message.playerpos().z());
		break;
	}
	}

	mRecvBuffer.Remove( messageHeader.size );
}

void ClientSession::ProtobufInit()
{
	m_pArrayOutputStream = new google::protobuf::io::ArrayOutputStream(m_SessionBuffer, MAX_BUFFER_SIZE);
	m_pCodedOutputStream = new google::protobuf::io::CodedOutputStream( m_pArrayOutputStream );
}

void ClientSession::ProtobufReleae()
{
	if ( m_pArrayOutputStream )
	{
		m_pArrayOutputStream;
	}
		
	if ( m_pCodedOutputStream )
	{
		delete m_pCodedOutputStream;
	}
}

