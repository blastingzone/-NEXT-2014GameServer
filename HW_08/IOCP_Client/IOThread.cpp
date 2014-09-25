#include "stdafx.h"
#include "Exception.h"
#include "ThreadLocal.h"
#include "IOThread.h"
#include "IocpManager.h"
#include "SessionManager.h"
#include "Session.h"
#include "Timer.h"
#include "DummyClients.h"
#include "DummyClientSession.h"

IOThread::IOThread(HANDLE hThread, HANDLE hCompletionPort) : mThreadHandle(hThread), mCompletionPort(hCompletionPort)
{
}


IOThread::~IOThread()
{
	CloseHandle(mThreadHandle);
}

DWORD IOThread::Run()
{

	while (true)
	{
		LTimer->DoTimerJob();

		if (false == DoIocpJob())
			break;
		
		DoSendJob(); ///< aggregated sends

		//... ...
	}

	return 1;
}

bool IOThread::DoIocpJob()
{
	DWORD dwTransferred = 0;
	OverlappedIOContext* context = nullptr;

	ULONG_PTR completionKey = 0;

	int ret = GetQueuedCompletionStatus(mCompletionPort, &dwTransferred, (PULONG_PTR)&completionKey, (LPOVERLAPPED*)&context, GQCS_TIMEOUT);

	/// 아래로는 일반적인 I/O 처리

	Session* remote = context ? context->mSessionObject : nullptr;

	if (ret == 0 || dwTransferred == 0)
	{
		//클라이언트 정지신호
		if (completionKey == CK_STOP_NOW)
			return false;

		/// check time out first 
		if (context == nullptr && GetLastError() == WAIT_TIMEOUT)
			return true;

		if (context->mIoType == IO_RECV || context->mIoType == IO_SEND)
		{
			CRASH_ASSERT(nullptr != remote);

			/// In most cases in here: ERROR_NETNAME_DELETED(64)

			remote->DisconnectRequest(DR_COMPLETION_ERROR);

			DeleteIoContext(context);

			return true;
		}
	}

	CRASH_ASSERT(nullptr != remote);

	bool completionOk = false;
	switch (context->mIoType)
	{
	case IO_CONNECT:
		dynamic_cast<DummyClientSession*>(remote)->ConnectCompletion();
		completionOk = true;
		break;

	case IO_DISCONNECT:
		remote->DisconnectCompletion(static_cast<OverlappedDisconnectContext*>(context)->mDisconnectReason);
		completionOk = true;
		break;

	case IO_RECV_ZERO:
		completionOk = remote->PostRecv();
		break;

	case IO_SEND:
		remote->SendCompletion(dwTransferred);

		if (context->mWsaBuf.len != dwTransferred)
			printf_s("Partial SendCompletion requested [%d], sent [%d]\n", context->mWsaBuf.len, dwTransferred);
		else
			completionOk = true;

		break;

	case IO_RECV:
		remote->RecvCompletion(dwTransferred);

		dynamic_cast<DummyClientSession*>(remote)->PacketHandler();

		completionOk = remote->PreRecv();

		break;

	default:
		printf_s("Unknown I/O Type: %d\n", context->mIoType);
		CRASH_ASSERT(false);
		break;
	}

	if (!completionOk)
	{
		/// connection closing
		remote->DisconnectRequest(DR_IO_REQUEST_ERROR);
	}

	DeleteIoContext(context);

	return true;
}


void IOThread::DoSendJob()
{
	while (!LSendRequestSessionList->empty())
	{
		auto& session = LSendRequestSessionList->front();
		LSendRequestSessionList->pop_front();

		if (!session->FlushSend())
		{
			//실패했을 경우 FailedSessionList에 수집
			LSendRequestFailedSessionList->push_back(session);
		}
	}

	//큐를 스왑
	//이부분은 덕철형의 아이디어를 이용
	std::deque<Session*>* tempDeq = LSendRequestSessionList;
	LSendRequestSessionList = LSendRequestFailedSessionList;
	LSendRequestFailedSessionList = tempDeq;
}