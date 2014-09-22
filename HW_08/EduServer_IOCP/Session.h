#pragma once
#include "CircularBuffer.h"
#include "OverlappedIOContext.h"
#include "MyPacket.pb.h"
#include "Crypt.h"

class Session
{

public:
	Session(size_t sendBufSize, size_t recvBufSize);
	virtual ~Session() {}

	bool IsConnected() const { return !!mConnected; }

	void DisconnectRequest(DisconnectReason dr);

	void CryptPacketHandler();

	bool PreRecv(); ///< zero byte recv
	bool PostRecv();

	bool EncryptSend(char* data, size_t len);
	bool PostSend(const char* data, size_t len);
	bool FlushSend();

	void DisconnectCompletion(DisconnectReason dr);
	void SendCompletion(DWORD transferred);
	void RecvCompletion(DWORD transferred);

	void AddRef();
	void ReleaseRef();

	virtual void OnDisconnect(DisconnectReason dr) {}
	virtual void OnRelease() {}

	void	SetSocket(SOCKET sock) { mSocket = sock; }
	SOCKET	GetSocket() const { return mSocket; }

	void EchoBack();

protected:

	SOCKET			mSocket;
	Crypt			mCrpyt;
	CircularBuffer	mDecryptedPacketBuffer;

	CircularBuffer	mRecvBuffer;
	CircularBuffer	mSendBuffer;
	FastSpinlock	mSendBufferLock;
	int				mSendPendingCount;

	volatile long	mRefCount;
	volatile long	mConnected;

};


extern __declspec(thread) std::deque<Session*>* LSendRequestSessionList;