#pragma once

#include "Session.h"
#include "Player.h"
#include "google\protobuf\io\coded_stream.h"
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/text_format.h>

#define MAX_BUFFER_SIZE 20480

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

	//Session부분으로 나눠야 겠다
	void SessionReset();

	void ProtobufInit();
	void ProtobufSendbufferRecreate();
	void ProtobufRelease();

	bool PostAccept();
	void AcceptCompletion();

	virtual void OnDisconnect(DisconnectReason dr);
	virtual void OnRelease();

	void PacketHandler();

	//이 함수는 여기 있으면 안될 것 같은데
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
	google::protobuf::io::ArrayOutputStream* mArrayOutputStream = nullptr;
	google::protobuf::io::CodedOutputStream* mCodedOutputStream = nullptr;

	google::protobuf::uint8 mSessionBuffer[MAX_BUFFER_SIZE];

	friend class ClientSessionManager;
};



