#include "stdafx.h"
#include "Exception.h"
#include "ThreadLocal.h"
#include "Log.h"
#include "EduServer_IOCP.h"
#include "OverlappedIOContext.h"
#include "Session.h"
#include "IocpManager.h"
#include "CryptPacket.h"

__declspec( thread ) std::deque<Session*>* LSendRequestSessionList = nullptr;
__declspec( thread ) std::deque<Session*>* LSendRequestFailedSessionList = nullptr;

Session::Session(size_t sendBufSize, size_t recvBufSize)
	: mSendBuffer(sendBufSize), mRecvBuffer(recvBufSize), mConnected(0), mRefCount(0), mSendPendingCount(0), mIsEnCrypt(false)
{
	mSocket = WSASocket( AF_INET, SOCK_STREAM, IPPROTO_TCP, NULL, 0, WSA_FLAG_OVERLAPPED );
}


void Session::DisconnectRequest( DisconnectReason dr )
{
	TRACE_THIS;

	/// 이미 끊겼거나 끊기는 중이거나
	if ( 0 == InterlockedExchange( &mConnected, 0 ) )
		return;

	OverlappedDisconnectContext* context = new OverlappedDisconnectContext( this, dr );

	if ( FALSE == DisconnectEx( mSocket, (LPWSAOVERLAPPED)context, TF_REUSE_SOCKET, 0 ) )
	{
		if ( WSAGetLastError() != WSA_IO_PENDING )
		{
			DeleteIoContext( context );
			printf_s( "Session::DisconnectRequest Error : %d\n", GetLastError() );
		}
	}

}


bool Session::PreRecv()
{
	TRACE_THIS;

	if ( !IsConnected() )
		return false;

	OverlappedPreRecvContext* recvContext = new OverlappedPreRecvContext( this );

	DWORD recvbytes = 0;
	DWORD flags = 0;
	recvContext->mWsaBuf.len = 0;
	recvContext->mWsaBuf.buf = nullptr;

	/// start async recv
	if ( SOCKET_ERROR == WSARecv( mSocket, &recvContext->mWsaBuf, 1, &recvbytes, &flags, (LPWSAOVERLAPPED)recvContext, NULL ) )
	{
		if ( WSAGetLastError() != WSA_IO_PENDING )
		{
			DeleteIoContext( recvContext );
			printf_s( "Session::PreRecv Error : %d\n", GetLastError() );
			return false;
		}
	}

	return true;
}

bool Session::PostRecv()
{
	TRACE_THIS;

	if ( !IsConnected() )
		return false;

	if ( 0 == mRecvBuffer.GetFreeSpaceSize() )
		return false;

	OverlappedRecvContext* recvContext = new OverlappedRecvContext( this );

	DWORD recvbytes = 0;
	DWORD flags = 0;
	recvContext->mWsaBuf.len = (ULONG)mRecvBuffer.GetFreeSpaceSize();
	recvContext->mWsaBuf.buf = mRecvBuffer.GetBuffer();


	/// start real recv
	if ( SOCKET_ERROR == WSARecv( mSocket, &recvContext->mWsaBuf, 1, &recvbytes, &flags, (LPWSAOVERLAPPED)recvContext, NULL ) )
	{
		if ( WSAGetLastError() != WSA_IO_PENDING )
		{
			DeleteIoContext( recvContext );
			printf_s( "Session::PostRecv Error : %d\n", GetLastError() );
			return false;
		}

	}

	return true;
}


bool Session::PostSend( const char* data, size_t len )
{
	TRACE_THIS;

	if ( !IsConnected() )
		return false;

	FastSpinlockGuard criticalSection( mSendBufferLock );

	if ( mSendBuffer.GetFreeSpaceSize() < len )
	{
		printf_s( "Buffer Too Small! buffer : %d, len : %d\n", mSendBuffer.GetFreeSpaceSize(), len );
		return false;
	}
	

	/// flush later...
	LSendRequestSessionList->push_back( this );

	char* destData = mSendBuffer.GetBuffer();

	memcpy( destData, data, len );

	mSendBuffer.Commit( len );

	return true;
}


bool Session::FlushSend()
{
	TRACE_THIS;

	if ( !IsConnected() )
	{
		DisconnectRequest( DR_SENDFLUSH_ERROR );
		return true;
	}


	FastSpinlockGuard criticalSection( mSendBufferLock );

	/// 보낼 데이터가 없는 경우
	if ( 0 == mSendBuffer.GetContiguiousBytes() )
	{
		/// 보낼 데이터도 없는 경우
		if ( 0 == mSendPendingCount )
			return true;

		//return false;
		//임시방편
		return true;
	}

	/// 이전의 send가 완료 안된 경우
	if ( mSendPendingCount > 0 )
	{
		//return false;
		//임시방편
		return true;
	}
		

	char* start = mSendBuffer.GetBufferStart();
	ULONG len = mSendBuffer.GetContiguiousBytes();

	if (mIsEnCrypt)
		mCrypt.RC4Encyrpt((PBYTE)start, len);

	OverlappedSendContext* sendContext = new OverlappedSendContext( this );

	DWORD sendbytes = 0;
	DWORD flags = 0;
	sendContext->mWsaBuf.len = len;
	sendContext->mWsaBuf.buf = start;

	/// start async send
	if ( SOCKET_ERROR == WSASend( mSocket, &sendContext->mWsaBuf, 1, &sendbytes, flags, (LPWSAOVERLAPPED)sendContext, NULL ) )
	{
		if ( WSAGetLastError() != WSA_IO_PENDING )
		{
			DeleteIoContext( sendContext );
			printf_s( "Session::FlushSend Error : %d\n", GetLastError() );

			DisconnectRequest( DR_SENDFLUSH_ERROR );
			return true;
		}

	}

	mSendPendingCount++;

	return mSendPendingCount == 1;
}

void Session::DisconnectCompletion( DisconnectReason dr )
{
	TRACE_THIS;

	OnDisconnect( dr );

	/// release refcount when added at issuing a session
	ReleaseRef();
}


void Session::SendCompletion( DWORD transferred )
{
	TRACE_THIS;

	FastSpinlockGuard criticalSection( mSendBufferLock );

	mSendBuffer.Remove( transferred );

	mSendPendingCount--;
}

void Session::RecvCompletion( DWORD transferred )
{
	TRACE_THIS;

	if (mIsEnCrypt)
		mCrypt.RC4Decrypt((PBYTE)mRecvBuffer.GetBuffer(), transferred);

	mRecvBuffer.Commit( transferred );
}



void Session::AddRef()
{
	CRASH_ASSERT( InterlockedIncrement( &mRefCount ) > 0 );
}

void Session::ReleaseRef()
{
	long ret = InterlockedDecrement( &mRefCount );
	CRASH_ASSERT( ret >= 0 );

	if ( ret == 0 )
	{
		OnRelease();
	}
}

void Session::EchoBack()
{
	TRACE_THIS;

	size_t len = mRecvBuffer.GetContiguiousBytes();

	if ( len == 0 )
		return;

	if ( false == PostSend( mRecvBuffer.GetBufferStart(), len ) )
		return;

	mRecvBuffer.Remove( len );

}

/*
void Session::CryptPacketHandler()
{
	CryptPacketHeader cryptHeader;
	//나중에 따로 빼자
	int crypteHeaderSize = sizeof(CryptPacketHeader);

	//헤더제거작업
	char* start = mRecvBuffer.GetBufferStart();
	memcpy(&cryptHeader, start, crypteHeaderSize);
	mRecvBuffer.Remove(crypteHeaderSize);

	//포인트간의 형변환
	PBYTE data = (PBYTE)mRecvBuffer.GetBufferStart();
	
	switch (cryptHeader.mType)
	{
	case PKT_CP_FIRST:
		
		mCrpyt.CreatePrivateKey();
		mCrpyt.ExportPublicKey();

		mCrpyt.ImportPublicKey(data, cryptHeader.mSize);
		mCrpyt.ConvertRC4();

		

		//PostSend((const char*), mCrpyt.GetDataLen() + crypteHeaderSize);

		break;
	case PKT_CP_SECOND:
		//이게 올리가 없다.
		CRASH_ASSERT(false);
		break;
	case PKT_CP_OK:

		//암호해독
		mCrpyt.RC4Decrypt(data, cryptHeader.mSize);

		//이후 decrypted버퍼에 저장
		char* destData = mDecryptedPacketBuffer.GetBuffer();
		memcpy(destData, data, cryptHeader.mSize);
		mDecryptedPacketBuffer.Commit(cryptHeader.mSize);

		break;
	}

	mRecvBuffer.Remove(cryptHeader.mSize);
}

bool Session::EncryptSend(char* data, size_t len)
{
	//암호화
	mCrpyt.RC4Encyrpt((PBYTE)data, (DWORD)len);

	//페킷 헤더 추가
	

	PostSend(data, len);
}
*/