#pragma once

#include "Session.h"
#include "Player.h"
#include "google\protobuf\io\coded_stream.h"
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/text_format.h>

class ClientSessionManager;

//헤더추가

struct MessageHeader
{
	google::protobuf::uint32 size;
	MyPacket::MessageType type;
};

const int MessageHeaderSize = sizeof( MessageHeader );



class ClientSession : public Session, public ObjectPool < ClientSession >
{
public:
	ClientSession();
	virtual ~ClientSession();

	void SessionReset();

	bool PostAccept();
	void AcceptCompletion();

	virtual void OnDisconnect(DisconnectReason dr);
	virtual void OnRelease();

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


public:
	Player			mPlayer;

private:

	SOCKADDR_IN		mClientAddr;


	friend class ClientSessionManager;
};



