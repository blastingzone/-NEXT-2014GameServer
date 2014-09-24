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
#include "PacketHeader.h"

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
	mCrpyt.Release();
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
	//ProtobufRelease();
}


void ClientSession::PacketHandler()
{
	PacketHeader messageHeader;

	char* start = mRecvBuffer.GetBufferStart();
	memcpy(&messageHeader, start, PacketHeaderSize);
	mRecvBuffer.Remove(PacketHeaderSize);

	const void* pPacket = mRecvBuffer.GetBufferStart();

	switch ( messageHeader.mType )
	{
	case MyPacket::MessageType::PKT_CS_CYPT:
	{
		mCrpyt.CreatePrivateKey();
		mCrpyt.ExportPublicKey();

		mCrpyt.ImportPublicKey((PBYTE)pPacket, messageHeader.mSize);
		mCrpyt.ConvertRC4();

		DWORD len = mCrpyt.GetDataLen();
		PBYTE data = mCrpyt.GetKeyBlob();

		FastSpinlockGuard criticalSection(mSendBufferLock);

		int totalSize = len + PacketHeaderSize;
		if (mSendBuffer.GetFreeSpaceSize() < totalSize)
			break;

		memcpy(mSendBuffer.GetBuffer(), data, len);

		PacketHeader header;
		header.mSize = len;
		header.mType = MyPacket::MessageType::PKT_SC_CYPT;

		//나중에 모아서 보냄
		LSendRequestSessionList->push_back(this);
		mSendBuffer.Commit(totalSize);

		//3hand shake로 해야함
		mIsEnCrypt = true;

		break;
	}
	case MyPacket::MessageType::PKT_CS_LOGIN:
	{
		google::protobuf::io::ArrayInputStream payloadArrayStream(pPacket, messageHeader.mSize);
		google::protobuf::io::CodedInputStream payloadInputStream(&payloadArrayStream);
		MyPacket::LoginRequest message;
		if ( false == message.ParseFromCodedStream( &payloadInputStream ) )
			break;

		payloadInputStream.ConsumedEntireMessage();

		printf( "Login Request! Player Id : %d \n", message.playerid() );

		// 이제 db 에서 저 플레이어의 이름을 받아와서 LoginResult에 채운 다음 보내줘야 함

		//mPlayer.RequestLoad( message.playerid() );
		
		MyPacket::LoginResult loginResult;
		loginResult.set_playerid( message.playerid() );

		mPlayer.mPlayerId = message.playerid();

		// test position
		mPlayer.SetPosition( 1.0 * message.playerid(), 1.0 * message.playerid() );
		mPlayer.SetZone();

		// test name
		char* name = "kim";
		loginResult.set_playername( name );

		// message 안에 message 쓰는 방법
		// mutable로 멤버변수를 불러온다 -> set 으로 값을 정해준다
		// 일반 함수인 .멤버변수() 얘는 읽기 전용이라 const로 리턴된다. 그럼 당연히 값을 못쓰겠죠잉
		loginResult.mutable_playerpos()->set_x( mPlayer.mPosX );
		loginResult.mutable_playerpos()->set_y( mPlayer.mPosY );
		loginResult.mutable_playerpos()->set_z( mPlayer.mPosZ );

		if(!SendRequest(MyPacket::PKT_SC_LOGIN, loginResult))
			printf_s("SendReauest failed!\n");

		break;
	}
	case MyPacket::MessageType::PKT_CS_CHAT:
	{
		google::protobuf::io::ArrayInputStream payloadArrayStream(pPacket, messageHeader.mSize);
		google::protobuf::io::CodedInputStream payloadInputStream(&payloadArrayStream);
		MyPacket::ChatRequest message;
		if ( false == message.ParseFromCodedStream( &payloadInputStream ) )
			break;
		
		std::string chat = message.playermessage();
		ZonePtr zone = GMap->GetZone(mPlayer.mPosX, mPlayer.mPosY);

		//printf_s("Chat!!! %s\n", chat.c_str());

		//아래 함수에서 매번 루프돌면서 복사가 일어나기 때문에 주의해야함
		MyPacket::ChatResult chatPacket;
		chat.append( "\n" );
		chat.append( "server to client : chat from id : " );
		chat.append( std::to_string( mPlayer.GetPlayerId() ) );
		chat.append( "\n" );
		chatPacket.set_playermessage( chat.c_str() );
		chatPacket.set_playername( "kim" ); //모조리 김씨

		PlayerPtrList playerList = zone->GetPlayerList();

		int i = 0;
		for (auto iter : playerList)
		{
			//printf_s("iteration count : %d\n", ++i);
			if(!(iter->mSession->SendRequest(MyPacket::PKT_SC_CHAT, chatPacket)))
				printf_s("SendReauest failed!\n");
		}

		break;
	}
	case MyPacket::MessageType::PKT_CS_MOVE:
	{
		google::protobuf::io::ArrayInputStream payloadArrayStream(pPacket, messageHeader.mSize);
		google::protobuf::io::CodedInputStream payloadInputStream(&payloadArrayStream);
		MyPacket::MoveRequest message;
		if ( false == message.ParseFromCodedStream( &payloadInputStream ) )
			break;

		//mPlayer.RequestUpdatePosition(message.playerpos().x(), message.playerpos().y(), message.playerpos().z());
		mPlayer.SetPosition( message.playerpos().x(), message.playerpos().z() );
		mPlayer.SetZone();
		//이동처리가 완료됨을 알리는 패킷
		
		printf_s("Player pos : %f, %f", mPlayer.mPosX, mPlayer.mPosZ);

		MyPacket::MoveResult movePacket;

		movePacket.set_playerid(mPlayer.GetPlayerId());
		movePacket.mutable_playerpos()->set_x( mPlayer.mPosX );
		movePacket.mutable_playerpos()->set_y( mPlayer.mPosY );
		movePacket.mutable_playerpos()->set_z( mPlayer.mPosZ );

		if (!SendRequest(MyPacket::PKT_SC_MOVE, movePacket))
			printf_s("SendReauest failed!\n");

		break;
	}
	default:
	{
		//와서는 안될 구역
		CRASH_ASSERT(false);
		break;
	}
	}

	mRecvBuffer.Remove( messageHeader.mSize );
}

bool ClientSession::SendRequest(short packetType, const google::protobuf::MessageLite& payload)
{
	TRACE_THIS;

	if (!IsConnected()) {
		printf_s("IsConnected Error in SendRequest\n");
		return false;
	}
		

	FastSpinlockGuard criticalSection(mSendBufferLock);

	int totalSize = payload.ByteSize() + PacketHeaderSize;
	if (mSendBuffer.GetFreeSpaceSize() < totalSize) {
		printf_s("BufferError in SendRequest\n");
		return false;
	}
		

	google::protobuf::io::ArrayOutputStream arrayOutputStream(mSendBuffer.GetBuffer(), totalSize);
	google::protobuf::io::CodedOutputStream codedOutputStream(&arrayOutputStream);

	PacketHeader header;
	header.mSize = payload.ByteSize();
	header.mType = packetType;

	codedOutputStream.WriteRaw(&header, PacketHeaderSize);
	payload.SerializeToCodedStream(&codedOutputStream);


	/// flush later...
	LSendRequestSessionList->push_back(this);

	mSendBuffer.Commit(totalSize);

	return true;
}
