// DummyClients.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "DummyClients.h"
#include "Exception.h"
#include "DummyClientSession.h"
#include "SessionManager.h"
#include "IocpManager.h"
#include "MemoryPool.h"

/// config values
int MAX_CONNECTION = 10;
char CONNECT_ADDR[32] = "127.0.0.1";
unsigned short CONNECT_PORT = 9000;

int _tmain(int argc, _TCHAR* argv[])
{
	LThreadType = THREAD_MAIN;

	/// for dump on crash
	SetUnhandledExceptionFilter(ExceptionFilter);

	/// Global Managers
	GMemoryPool = new MemoryPool;
	GSessionManager = new SessionManager;
	GIocpManager = new IocpManager;

	if (false == GIocpManager->Initialize())
		return -1;

	if (false == GIocpManager->StartIoThreads())
		return -1;

	printf_s("Start Dummies\n");

	if (false == GSessionManager->ConnectSessions())
		return -1;

	/// block here...
	getchar();

	printf_s("Terminating Dummies...\n");

	/// Disconnect request here
	GSessionManager->DisconnectSessions();

	GIocpManager->StopIoThreads();

	GSessionManager->PrintTotalTransferred();

	GIocpManager->Finalize();

	delete GIocpManager;
	delete GSessionManager;
	delete GMemoryPool;

	return 0;
}

