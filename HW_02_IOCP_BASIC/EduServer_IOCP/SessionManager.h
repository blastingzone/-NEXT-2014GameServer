#pragma once
#include <map>
#include <WinSock2.h>
#include "FastSpinlock.h"

class ClientSession;

class SessionManager
{
public:
	SessionManager() : mCurrentConnectionCount(0)	{}

	ClientSession* CreateClientSession(SOCKET sock);

	void DeleteClientSession(ClientSession* client);

	//인터락 증감 함수
	int IncreaseConnectionCount() { return InterlockedIncrement(&mCurrentConnectionCount); }
	int DecreaseConnectionCount() { return InterlockedDecrement(&mCurrentConnectionCount); }


private:
	typedef std::map<SOCKET, ClientSession*> ClientList;
	ClientList	mClientList;

	FastSpinlock mLock;

	//volatile
	//레지스터에 등록된 걸 사용하지 않고 메모리에 접근하여 읽고 쓰기 때문에
	//상대적으로 안전
	volatile long mCurrentConnectionCount;
};

extern SessionManager* GSessionManager;
