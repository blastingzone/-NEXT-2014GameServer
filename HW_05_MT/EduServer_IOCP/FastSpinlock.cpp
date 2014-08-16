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
	while (true)
	{
		/// �ٸ����� writelock Ǯ���ٶ����� ��ٸ���.
		while (mLockFlag & LF_WRITE_MASK)
			YieldProcessor();

		if ((InterlockedAdd(&mLockFlag, LF_WRITE_FLAG) & LF_WRITE_MASK) == LF_WRITE_FLAG)
		{
			/// �ٸ����� readlock Ǯ���ٶ����� ��ٸ���.
			while (mLockFlag & LF_READ_MASK)
				YieldProcessor();

			/// �� ���� �Ű� �Ƚᵵ �Ǵ� ���� �׳� �н�
			if (mLockOrder != LO_DONT_CARE)
				LLockOrderChecker->Push(this);

			return;
		}

		InterlockedAdd(&mLockFlag, -LF_WRITE_FLAG);
	}

}

void FastSpinlock::LeaveWriteLock()
{
	InterlockedAdd(&mLockFlag, -LF_WRITE_FLAG);

	/// �� ���� �Ű� �Ƚᵵ �Ǵ� ���� �׳� �н�
	if (mLockOrder != LO_DONT_CARE)
		LLockOrderChecker->Pop(this);
}

void FastSpinlock::EnterReadLock()
{
	while (true)
	{
		/// �ٸ����� writelock Ǯ���ٶ����� ��ٸ���.
		while (mLockFlag & LF_WRITE_MASK)
			YieldProcessor();

		//TODO: Readlock ���� ���� (mLockFlag�� ��� ó���ϸ� �Ǵ���?)
		if ((InterlockedIncrement(&mLockFlag) & LF_WRITE_MASK) != LF_WRITE_FLAG) {
			
			if (mLockOrder != LO_DONT_CARE)
				LLockOrderChecker->Push(this);

			return;
		}
		
		InterlockedDecrement(&mLockFlag);

		// if ( readlock�� ������ )
			//return;
		// else
			// mLockFlag ����
	}
}

void FastSpinlock::LeaveReadLock()
{
	//TODO: mLockFlag ó�� 
	InterlockedDecrement(&mLockFlag);

	if (mLockOrder != LO_DONT_CARE)
		LLockOrderChecker->Pop(this);
}