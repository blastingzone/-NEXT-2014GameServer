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

	//���Ͷ� ���� �Լ�
	int IncreaseConnectionCount() { return InterlockedIncrement(&mCurrentConnectionCount); }
	int DecreaseConnectionCount() { return InterlockedDecrement(&mCurrentConnectionCount); }


private:
	typedef std::map<SOCKET, ClientSession*> ClientList;
	ClientList	mClientList;

	FastSpinlock mLock;

	//volatile
	//�������Ϳ� ��ϵ� �� ������� �ʰ� �޸𸮿� �����Ͽ� �а� ���� ������
	//��������� ����
	volatile long mCurrentConnectionCount;
};

extern SessionManager* GSessionManager;
