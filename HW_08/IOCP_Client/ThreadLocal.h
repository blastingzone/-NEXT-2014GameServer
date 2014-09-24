﻿#pragma once

#define MAX_IO_THREAD	4
#define MAX_DB_THREAD	4
#define MAX_WORKER_THREAD	(MAX_IO_THREAD+MAX_DB_THREAD)

enum THREAD_TYPE
{
	THREAD_MAIN,
	THREAD_IO_WORKER,
	THREAD_DB_WORKER
};

enum COMPLETION_KEY_TYPE
{
	CK_NONE,
	CK_DB_REQUEST,
	CK_DB_RESULT
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

extern ThreadCallHistory* GThreadCallHistory[MAX_WORKER_THREAD];
extern ThreadCallElapsedRecord* GThreadCallElapsedRecord[MAX_WORKER_THREAD];