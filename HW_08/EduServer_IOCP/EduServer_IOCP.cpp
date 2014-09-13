// EduServer_IOCP.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "Exception.h"
#include "MemoryPool.h"
#include "EduServer_IOCP.h"
#include "ThreadLocal.h"
#include "ClientSession.h"
#include "ClientSessionManager.h"
#include "IocpManager.h"
#include "LockOrderChecker.h"
#include "PlayerManager.h"
#include "GrandCentralExecuter.h"

int _tmain(int argc, _TCHAR* argv[])
{
	LThreadType = THREAD_MAIN;

	/// for dump on crash
	SetUnhandledExceptionFilter(ExceptionFilter);

	/// Global Managers
	GMemoryPool = new MemoryPool;
	GClientSessionManager = new ClientSessionManager;
	GIocpManager = new IocpManager;
	GGrandCentralExecuter = new GrandCentralExecuter;
	GPlayerManager = new PlayerManager;

	/// main thread도 lock order check...
	LLockOrderChecker = new LockOrderChecker(-1);


	if (false == GIocpManager->Initialize())
		return -1;

	if (false == GIocpManager->StartIoThreads())
		return -1;

	
	printf_s("Start Server\n");


	GIocpManager->StartAccept(); ///< block here...

	GIocpManager->Finalize();

	printf_s("End Server\n");

	delete GIocpManager;
	delete GClientSessionManager;
	delete GMemoryPool;

	return 0;
}

