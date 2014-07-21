#include "stdafx.h"
#include "Exception.h"
#include "EduServer_IOCP.h"
#include "ClientSession.h"
#include "IocpManager.h"
#include "SessionManager.h"

bool ClientSession::OnConnect(SOCKADDR_IN* addr)
{
	//이 영역 lock으로 보호 할 것
	//OnConnect함수가 동시에 여러번 호출되면 내부 winapi함수가 동시에 적용이 되어버림
	FastSpinlockGuard spinLock( mLock );

	CRASH_ASSERT(LThreadType == THREAD_MAIN_ACCEPT);

	/// make socket non-blocking
	u_long arg = 1 ;
	ioctlsocket(mSocket, FIONBIO, &arg) ;

	/// turn off nagle
	int opt = 1 ;
	setsockopt(mSocket, IPPROTO_TCP, TCP_NODELAY, (const char*)&opt, sizeof(int)) ;

	opt = 0;
	if (SOCKET_ERROR == setsockopt(mSocket, SOL_SOCKET, SO_RCVBUF, (const char*)&opt, sizeof(int)) )
	{
		printf_s("[DEBUG] SO_RCVBUF change error: %d\n", GetLastError()) ;
		return false;
	} 
		
	//Completion key로 this(클라세션)을 등록
	HANDLE handle = CreateIoCompletionPort((HANDLE)mSocket, GIocpManager->GetComletionPort(), (DWORD)this, 0);
	if (handle != GIocpManager->GetComletionPort())
	{
		printf_s("[DEBUG] CreateIoCompletionPort error: %d\n", GetLastError());
		return false;
	}

	memcpy(&mClientAddr, addr, sizeof(SOCKADDR_IN));
	mConnected = true ;

	printf_s("[DEBUG] Client Connected: IP=%s, PORT=%d\n", inet_ntoa(mClientAddr.sin_addr), ntohs(mClientAddr.sin_port));

	GSessionManager->IncreaseConnectionCount();


	//최초 recv
	return PostRecv() ;
}

void ClientSession::Disconnect(DisconnectReason dr)
{
	//이 영역 lock으로 보호할 것
	FastSpinlockGuard spinLock( mLock );

	if ( !IsConnected() )
		return;

	LINGER lingerOption;
	lingerOption.l_onoff = 1;
	lingerOption.l_linger = 0;

	/// no TCP TIME_WAIT
	//서버에서 종료할 경우 time_wait을 남기지 않고 칼같이
	if ( SOCKET_ERROR == setsockopt( mSocket, SOL_SOCKET, SO_LINGER, (char*)&lingerOption, sizeof( LINGER ) ) )
	{
		printf_s( "[DEBUG] setsockopt linger option error: %d\n", GetLastError() );
	}

	printf_s( "[DEBUG] Client Disconnected: Reason=%d IP=%s, PORT=%d \n", dr, inet_ntoa( mClientAddr.sin_addr ), ntohs( mClientAddr.sin_port ) );

	GSessionManager->DecreaseConnectionCount();

	closesocket( mSocket );

	mConnected = false;
	
}

bool ClientSession::PostRecv() const
{
	if (!IsConnected())
		return false;

	OverlappedIOContext* recvContext = new OverlappedIOContext(this, IO_RECV);

	//WSARecv
	recvContext->mWsaBuf.buf = recvContext->mBuffer;
	recvContext->mWsaBuf.len = BUFSIZE;

	DWORD dwFlag = 0;
	if ( WSARecv( mSocket, &( recvContext->mWsaBuf ), 1, NULL, &dwFlag, &( recvContext->mOverlapped ), NULL ) )
	{
		if ( WSA_IO_PENDING != WSAGetLastError() )
		{
			//10054 강제종료
			//원격지에서 종료했을 때 알려줌
			printf_s( "WSA Recv Error : %d \n", WSAGetLastError() );
			delete recvContext;
			return false;
		}
	}

	return true;
}

bool ClientSession::PostSend(const char* buf, int len) const
{
	if (!IsConnected())
		return false;

	OverlappedIOContext* sendContext = new OverlappedIOContext(this, IO_SEND);

	/// copy for echoing back..
	memcpy_s(sendContext->mBuffer, BUFSIZE, buf, len);

	//WSASend
	sendContext->mWsaBuf.buf = sendContext->mBuffer;
	sendContext->mWsaBuf.len = len;

	if ( WSASend( mSocket, &( sendContext->mWsaBuf ), 1, NULL, NULL, &( sendContext->mOverlapped ), NULL ) )
	{
		if ( WSA_IO_PENDING != WSAGetLastError() )
		{
			//10053 강제종료
			printf_s( "WSA Send Error : %d \n", WSAGetLastError() );
			delete sendContext;
			return false;
		}
	}

	return true;
}