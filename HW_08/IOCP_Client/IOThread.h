#pragma once


class IOThread
{
public:
	IOThread(HANDLE hThread, HANDLE hCompletionPort);
	~IOThread();

	DWORD Run();

	bool DoIocpJob();
	void DoSendJob();

	HANDLE GetHandle() { return mThreadHandle; }

private:

	HANDLE mThreadHandle;
	HANDLE mCompletionPort;
};

