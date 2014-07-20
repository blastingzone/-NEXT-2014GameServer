#include "stdafx.h"
#include "ClientSession.h"
#include "SessionManager.h"

SessionManager* GSessionManager = nullptr;

ClientSession* SessionManager::CreateClientSession(SOCKET sock)
{
	//메번 accpet될때마다 new를 해주어야 한다.
	//풀같은걸 사용할 수 는 없나
	ClientSession* client = new ClientSession(sock);

	mLock.EnterLock();
	{
		mClientList.insert(ClientList::value_type(sock, client));
	}
	mLock.LeaveLock();

	return client;
}


void SessionManager::DeleteClientSession(ClientSession* client)
{
	mLock.EnterLock();
	{
		mClientList.erase(client->mSocket);
	}
	mLock.LeaveLock();

	delete client;
}