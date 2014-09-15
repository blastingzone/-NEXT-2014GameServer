#pragma once

#include "Session.h"
#include "Player.h"

class ClientSessionManager;

//헤더추가
struct MessageHeader
{
	google::protobuf::uint32 size;
	MyPacket::MessageType type;
};

const int MessageHeaderSize = sizeof(MessageHeader);

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

public:
	Player			mPlayer;

private:

	SOCKADDR_IN		mClientAddr;


	friend class ClientSessionManager;
};



