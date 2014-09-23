#pragma once

#include "Session.h"
#include "Player.h"
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/text_format.h>

class ClientSessionManager;

class ClientSession : public Session, public ObjectPool < ClientSession >
{
public:
	ClientSession();
	virtual ~ClientSession();

	//Session부분으로 나눠야 겠다
	void SessionReset();

	bool PostAccept();
	void AcceptCompletion();

	virtual void OnDisconnect(DisconnectReason dr);
	virtual void OnRelease();

	void PacketHandler();

	bool SendRequest(short packetType, const google::protobuf::MessageLite& payload);

public:
	enum { CLIENT_BUFSIZE = 65536 };


	Player			mPlayer;

private:
	SOCKADDR_IN		mClientAddr;

	friend class ClientSessionManager;
};



