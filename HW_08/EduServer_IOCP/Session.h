﻿#pragma once
#include "CircularBuffer.h"
#include "OverlappedIOContext.h"
#include "google\protobuf\io\coded_stream.h"
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/text_format.h>
#include "MyPacket.pb.h"

class Session
{

	struct MessageHeader
	{
		google::protobuf::uint32 size;
		MyPacket::MessageType type;
	};

	const int MessageHeaderSize = sizeof( MessageHeader );


public:
	Session(size_t sendBufSize, size_t recvBufSize);
	virtual ~Session() {}

	bool IsConnected() const { return !!mConnected; }

	void DisconnectRequest(DisconnectReason dr);

	bool PreRecv(); ///< zero byte recv
	bool PostRecv();

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

	void PacketHandler();
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


	void EchoBack();

protected:

	SOCKET			mSocket;

	CircularBuffer	mRecvBuffer;
	CircularBuffer	mSendBuffer;
	FastSpinlock	mSendBufferLock;
	int				mSendPendingCount;

	volatile long	mRefCount;
	volatile long	mConnected;

};


extern __declspec(thread) std::deque<Session*>* LSendRequestSessionList;