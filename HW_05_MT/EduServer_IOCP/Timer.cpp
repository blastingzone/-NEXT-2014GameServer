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
		//그리고 통채로 heap에다 쑤셔넣음

		// FastSpinlockGuard exclusive(mLock);
		// 락을 걸순 없을 듯(락내부에서 다시 호출하잖소)

		//레퍼런스를 넘겨받은 이후
		//sort가 일어나면서 문제가 발생하는 것 같다.
		//vector의 경우 레퍼런스를 받게 되면 
		//해당하는 영역이 sort할때나 메모리 확장이 필요할 때 위험하다.
		//일단 레퍼런스가 아닌 복사를 일으키면 뻑이 사라진다.
		//레퍼런스를 받고싶다면 노드를 이용한 자료구조를 사용하면 될 듯하다.
		const TimerJobElement& timerJobElem = mTimerJobQueue.top(); 
		
		/*vector로 했을 경우 일단 아래처럼 해놓으니 뻑은 안난다. 
		const TimerJobElement timerJobElem = mTimerJobQueue.top();
		mTimerJobQueue.pop();*/
		//자꾸 이부분이 tls라는 걸 까먹는다.

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

