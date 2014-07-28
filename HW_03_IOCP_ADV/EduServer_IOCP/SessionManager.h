#pragma once
#include <list>
#include <WinSock2.h>
#include "FastSpinlock.h"

class ClientSession;

class SessionManager
{
public:
	SessionManager() : mCurrentIssueCount(0), mCurrentReturnCount(0)
	{}
	
	~SessionManager();

	void PrepareSessions();
	bool AcceptSessions();

	void ReturnClientSession(ClientSession* client);

	

private:
	typedef std::list<ClientSession*> ClientList;
	ClientList	mFreeSessionList;

	FastSpinlock mLock;

	//10의 19승까지 가지만 overflow의 위험이 있음
	//overflow가 일어나면 성공했다는 뜻이니...
	uint64_t mCurrentIssueCount;
	uint64_t mCurrentReturnCount;
};

extern SessionManager* GSessionManager;
