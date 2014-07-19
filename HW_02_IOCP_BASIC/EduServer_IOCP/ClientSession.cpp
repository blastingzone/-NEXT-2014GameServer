#include "stdafx.h"
#include "Exception.h"
#include "EduServer_IOCP.h"
#include "ClientSession.h"
#include "IocpManager.h"
#include "SessionManager.h"

bool ClientSession::OnConnect(SOCKADDR_IN* addr)
{
	//TODO: 이 영역 lock으로 보호 할 것
	FastSpinlockGuard spinLock( mLock );
	{
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
	
		HANDLE handle = 0; //TODO: 여기에서 CreateIoCompletionPort((HANDLE)mSocket, ...);사용하여 연결할 것
		
		// connect I/O Completion Port to Socket
		handle = CreateIoCompletionPort( (HANDLE)mSocket, GIocpManager->GetComletionPort(), (DWORD)this, 0 );

		if (handle != GIocpManager->GetComletionPort())
		{
			printf_s("[DEBUG] CreateIoCompletionPort error: %d\n", GetLastError());
			return false;
		}

		memcpy(&mClientAddr, addr, sizeof(SOCKADDR_IN));
		mConnected = true ;

		printf_s("[DEBUG] Client Connected: IP=%s, PORT=%d\n", inet_ntoa(mClientAddr.sin_addr), ntohs(mClientAddr.sin_port));

		GSessionManager->IncreaseConnectionCount();
	}

	return PostRecv() ;
}

void ClientSession::Disconnect(DisconnectReason dr)
{
	//TODO: 이 영역 lock으로 보호할 것
	FastSpinlockGuard spinLock( mLock );
	{
		if ( !IsConnected() )
			return;

		LINGER lingerOption;
		lingerOption.l_onoff = 1;
		lingerOption.l_linger = 0;

		/// no TCP TIME_WAIT
		if ( SOCKET_ERROR == setsockopt( mSocket, SOL_SOCKET, SO_LINGER, (char*)&lingerOption, sizeof( LINGER ) ) )
		{
			printf_s( "[DEBUG] setsockopt linger option error: %d\n", GetLastError() );
		}

		printf_s( "[DEBUG] Client Disconnected: Reason=%d IP=%s, PORT=%d \n", dr, inet_ntoa( mClientAddr.sin_addr ), ntohs( mClientAddr.sin_port ) );

		GSessionManager->DecreaseConnectionCount();

		closesocket( mSocket );

		mConnected = false;
	}
}

bool ClientSession::PostRecv() const
{
	if (!IsConnected())
		return false;

	OverlappedIOContext* recvContext = new OverlappedIOContext(this, IO_RECV);

	//TODO: WSARecv 사용하여 구현할 것
	DWORD dwTransferred = 0;
	DWORD dwFlag = 0;
	int ret = WSARecv( mSocket,
		&( recvContext->mWsaBuf ),
		1,
		&dwTransferred,
		&dwFlag,
		&(recvContext->mOverlapped),
		NULL);

	if ( NULL == ret )
	{
		if ( WSA_IO_PENDING != WSAGetLastError() )
		{
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

	sendContext->mWsaBuf.buf = sendContext->mBuffer;
	sendContext->mWsaBuf.len = len;

	//TODO: WSASend 사용하여 구현할 것
	int ret = WSASend( mSocket,
		&( sendContext->mWsaBuf ),
		1,
		NULL,
		NULL,
		&( sendContext->mOverlapped ),
		NULL );

	if ( NULL == ret )
	{
		if ( WSA_IO_PENDING != WSAGetLastError() )
		{
			return false;
		}
	}

	return true;
}