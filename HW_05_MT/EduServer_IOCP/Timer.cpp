#include "stdafx.h"
#include "ThreadLocal.h"
#include "Exception.h"
#include "SyncExecutable.h"
#include "Timer.h"



Timer::Timer()
{
	LTickCount = GetTickCount64();
}


void Timer::PushTimerJob(SyncExecutablePtr owner, const TimerTask& task, uint32_t after)
{
	CRASH_ASSERT(LThreadType == THREAD_IO_WORKER);

	//FastSpinlockGuard exclusive(mLock);

	//TODO: mTimerJobQueue�� TimerJobElement�� push..
	//int64_t dueTimeTick = after + GetTickCount64();
	//�̰� ���� ����
	int64_t dueTimeTick = after + LTickCount;
	//mTimerJobQueue.push(TimerJobElement(owner, task, dueTimeTick));

	//���� ���� �ȳ��� ��
	static int testTemp = 10;
	mTimerJobQueue.push(TimerJobElement(owner, task, testTemp--));
}


void Timer::DoTimerJob()
{
	/// thread tick update
	LTickCount = GetTickCount64();

	while (!mTimerJobQueue.empty())
	{
		//std::priority_queue ���������� ������ �������
		//�̰��ΰ�?
		//_Container�� vector�� �������

		// FastSpinlockGuard exclusive(mLock);
		// ���� �ɼ� ���� ��(�����ο��� �ٽ� ȣ�����ݼ�)

		//���۷����� �Ѱܹ��� ����
		//sort�� �Ͼ�ٸ�?
		//
		const TimerJobElement& timerJobElem = mTimerJobQueue.top(); 
		
		/*�ϴ� �̷��Ը� �ص� ���� �ȳ�
		const TimerJobElement timerJobElem = mTimerJobQueue.top();
		mTimerJobQueue.pop();*/

		if (LTickCount < timerJobElem.mExecutionTick)
			break;

		timerJobElem.mOwner->EnterLock();
		//task���� PushTimerJob�� ȣ��� �� ����
		timerJobElem.mTask();

		timerJobElem.mOwner->LeaveLock();
		//sort�� �Ŀ� �ֻ����� �ִ� ģ���� ���� ������ �߻��� ���ѵ�
		mTimerJobQueue.pop();
	}


}

