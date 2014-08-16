#include "stdafx.h"
#include "Exception.h"
#include "FastSpinlock.h"
#include "LockOrderChecker.h"
#include "ThreadLocal.h"

FastSpinlock::FastSpinlock(const int lockOrder) : mLockFlag(0), mLockOrder(lockOrder)
{
}


FastSpinlock::~FastSpinlock()
{
}


void FastSpinlock::EnterWriteLock()
{
	/// 락 순서 신경 안써도 되는 경우는 그냥 패스
	//먼저들어오는 순서대로 push
	if (mLockOrder != LO_DONT_CARE)
		LLockOrderChecker->Push(this);

	while (true)
	{
		/// 다른놈이 writelock 풀어줄때까지 기다린다.
		while (mLockFlag & LF_WRITE_MASK)
			YieldProcessor();

		//스텍의 위부터 크리티컬 세션에 들어갈 수 있도록 보장
		if ((mLockOrder != LO_DONT_CARE) && (LLockOrderChecker->IsTopPos(this) == false))
			YieldProcessor();

		if ((InterlockedAdd(&mLockFlag, LF_WRITE_FLAG) & LF_WRITE_MASK) == LF_WRITE_FLAG)
		{
			/// 다른놈이 readlock 풀어줄때까지 기다린다.
			while (mLockFlag & LF_READ_MASK)
				YieldProcessor();

			return;
		}

		InterlockedAdd(&mLockFlag, -LF_WRITE_FLAG);
	}

}

void FastSpinlock::LeaveWriteLock()
{
	InterlockedAdd(&mLockFlag, -LF_WRITE_FLAG);

	/// 락 순서 신경 안써도 되는 경우는 그냥 패스
	//먼저 실행되는 순서대로 pop
	if (mLockOrder != LO_DONT_CARE)
		LLockOrderChecker->Pop(this);
}

void FastSpinlock::EnterReadLock()
{
	if (mLockOrder != LO_DONT_CARE)
		LLockOrderChecker->Push(this);

	while (true)
	{
		/// 다른놈이 writelock 풀어줄때까지 기다린다.
		while (mLockFlag & LF_WRITE_MASK)
			YieldProcessor();

		//스텍의 위부터 크리티컬 세션에 들어갈 수 있도록 보장
		if ((mLockOrder != LO_DONT_CARE) && (LLockOrderChecker->IsTopPos(this) == false))
			YieldProcessor();

		//TODO: Readlock 진입 구현 (mLockFlag를 어떻게 처리하면 되는지?)
		if ((InterlockedIncrement(&mLockFlag) & LF_WRITE_MASK) != LF_WRITE_FLAG)
			return;
		else
			InterlockedDecrement(&mLockFlag);

		// if ( readlock을 얻으면 )
			//return;
		// else
			// mLockFlag 원복
	}
}

void FastSpinlock::LeaveReadLock()
{
	//TODO: mLockFlag 처리 
	InterlockedDecrement(&mLockFlag);

	if (mLockOrder != LO_DONT_CARE)
		LLockOrderChecker->Pop(this);
}