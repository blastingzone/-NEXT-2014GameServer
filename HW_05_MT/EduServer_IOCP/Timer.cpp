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

	//TODO: mTimerJobQueue에 TimerJobElement를 push..
	//int64_t dueTimeTick = after + GetTickCount64();
	//이거 순서 섞기
	int64_t dueTimeTick = after + LTickCount;
	//mTimerJobQueue.push(TimerJobElement(owner, task, dueTimeTick));

	//일단 뻑나라고 넣은 코드
	//뻑이 가끔씩 안날때가 있다.
	static int testTemp = 10;
	mTimerJobQueue.push(TimerJobElement(owner, task, testTemp--));
}


void Timer::DoTimerJob()
{
	/// thread tick update
	LTickCount = GetTickCount64();

	while (!mTimerJobQueue.empty())
	{
		//std::priority_queue 내부적으로 힙으로 만들어짐
		//이거인가?
		//_Container가 vector로 만들어짐

		// FastSpinlockGuard exclusive(mLock);
		// 락을 걸순 없을 듯(락내부에서 다시 호출하잖소)

		//레퍼런스를 넘겨받은 이후
		//sort가 일어나면서 문제가 발생하는 것 같다.
		//일단 레퍼런스가 아닌 복사를 일으키면 뻑이 사라진다.
		const TimerJobElement& timerJobElem = mTimerJobQueue.top(); 
		
		/*일단 아래처럼 해놓으니 뻑은 안난다.
		const TimerJobElement timerJobElem = mTimerJobQueue.top();
		mTimerJobQueue.pop();*/
		//하지만 완벽하지 않은 것 같다.
		//top과 pop이 동시에 이루어지는 자료구조가 필요할 듯 하다.

		if (LTickCount < timerJobElem.mExecutionTick)
			break;

		timerJobElem.mOwner->EnterLock();
		//task에서 PushTimerJob이 호출될 수 있음
		timerJobElem.mTask();

		timerJobElem.mOwner->LeaveLock();
		//sort된 후에 최상위에 있는 친구를 빼면 문제가 발생할 듯한데
		mTimerJobQueue.pop();
	}


}

