#pragma once

#define MAX_IO_THREAD	4

enum THREAD_TYPE
{
	THREAD_MAIN,
	THREAD_IO_WORKER,
	THREAD_DB_WORKER
};

class Timer;
class LockOrderChecker;

extern __declspec(thread) int LThreadType;

//아래 두개 수정 예정
extern __declspec(thread) int LWorkerThreadId;
extern __declspec(thread) int LIoThreadId;

extern __declspec(thread) Timer* LTimer;
extern __declspec(thread) int64_t LTickCount;
extern __declspec(thread) LockOrderChecker* LLockOrderChecker;


class ThreadCallHistory;
class ThreadCallElapsedRecord;

extern __declspec(thread) ThreadCallHistory* LThreadCallHistory;
extern __declspec(thread) ThreadCallElapsedRecord* LThreadCallElapsedRecord;
extern __declspec(thread) void* LRecentThisPointer;
