#pragma once

#include "PacketHeader.h"
#include "Session.h"

class DummyClientSession : public Session, public ObjectPool<DummyClientSession>
{
public:
	DummyClientSession();
	virtual ~DummyClientSession();

	bool PrepareSession();

	bool ConnectRequest();
	void ConnectCompletion();

	bool SendRequest(short packetType, const google::protobuf::MessageLite& payload);

	//virtual void OnReceive(size_t len);
	virtual void OnRelease();

	void PacketHandler();

	void ExportKey();
	void CryptOk();
	void Login();
	void Chat(int id);
	void Move(int id);

private:
	
	SOCKADDR_IN mConnectAddr;
	int			mPlayerID;
} ;



