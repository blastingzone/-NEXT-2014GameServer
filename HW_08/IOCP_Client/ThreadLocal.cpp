#include "stdafx.h"
#include "ThreadLocal.h"
#include "Timer.h"
#include "LockOrderChecker.h"

//tls선언부
__declspec(thread) int LThreadType = -1;

//아래 두개 수정 예정
__declspec(thread) int LIoThreadId = -1;
__declspec(thread) int LWorkerThreadId = -1;

__declspec(thread) Timer* LTimer = nullptr;
__declspec(thread) int64_t LTickCount = 0;
__declspec(thread) LockOrderChecker* LLockOrderChecker = nullptr;

__declspec(thread) ThreadCallHistory* LThreadCallHistory = nullptr;
__declspec(thread) ThreadCallElapsedRecord* LThreadCallElapsedRecord = nullptr;
__declspec(thread) void* LRecentThisPointer = nullptr;

ThreadCallHistory* GThreadCallHistory[MAX_WORKER_THREAD] = { 0, };
ThreadCallElapsedRecord* GThreadCallElapsedRecord[MAX_WORKER_THREAD] = { 0, };