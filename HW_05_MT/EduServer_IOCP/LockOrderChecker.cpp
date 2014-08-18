#include "stdafx.h"
#include "Exception.h"
#include "ThreadLocal.h"
#include "FastSpinlock.h"
#include "LockOrderChecker.h"

LockOrderChecker::LockOrderChecker(int tid) : mWorkerThreadId(tid), mStackTopPos(0)
{
	memset(mLockStack, 0, sizeof(mLockStack));
}

void LockOrderChecker::Push(FastSpinlock* lock)
{
	CRASH_ASSERT(mStackTopPos < MAX_LOCK_DEPTH);

	if (mStackTopPos > 0)
	{
		/// ���� ���� �ɷ� �ִ� ���¿� �����Ѱ��� �ݵ�� ���� ���� �켱������ ���ƾ� �Ѵ�.
		CRASH_ASSERT(mLockStack[mStackTopPos - 1]->mLockOrder < lock->mLockOrder);

		//TODO: �׷��� ���� ��� CRASH_ASSERT gogo
		///# �Ʒ�ó�� ���ַ��� >= ��ȣ�� ���� ��
		//if (mLockStack[mStackTopPos - 1]->mLockOrder > lock->mLockOrder)
		//	CRASH_ASSERT(false);
	}

	mLockStack[mStackTopPos++] = lock;
}

void LockOrderChecker::Pop(FastSpinlock* lock)
{

	/// �ּ��� ���� ���� �ִ� ���¿��� �� ���̰�
	CRASH_ASSERT(mStackTopPos > 0);
	
	//TODO: �翬�� �ֱٿ� push�ߴ� �༮�̶� ������ üũ.. Ʋ���� CRASH_ASSERT
	if (mLockStack[mStackTopPos - 1] != lock)
		CRASH_ASSERT(false);
		
	mLockStack[--mStackTopPos] = nullptr;

}

bool LockOrderChecker::IsTopPos(FastSpinlock* lock)
{
	return mLockStack[mStackTopPos - 1] == lock;
}
