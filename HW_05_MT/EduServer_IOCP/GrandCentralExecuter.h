#pragma once
#include "Exception.h"
#include "TypeTraits.h"
#include "XTL.h"
#include "ThreadLocal.h"

class GrandCentralExecuter
{
public:
	typedef std::function<void()> GCETask;

	GrandCentralExecuter(): mRemainTaskCount(0)
	{}

	void DoDispatch(const GCETask& task)
	{
		CRASH_ASSERT(LThreadType == THREAD_IO_WORKER); ///< 일단 IO thread 전용

		
		if (InterlockedIncrement64(&mRemainTaskCount) > 1)
		{
			//TODO: 이미 누군가 작업중이면 어떻게?
			mCentralTaskQueue.push(task);
		}
		else
		{
			/// 처음 진입한 놈이 책임지고 다해주자 -.-;

			mCentralTaskQueue.push(task);
			
			while (true)
			{
				GCETask task;
				if (mCentralTaskQueue.try_pop(task))
				{
					//TODO: task를 수행하고 mRemainTaskCount를 하나 감소 
					// mRemainTaskCount가 0이면 break;

					task(); ///# 이거 해줘야지

					if (InterlockedDecrement64(&mRemainTaskCount) == 0)
						break;
				}
			}
		}

	}


private:
	typedef concurrency::concurrent_queue<GCETask, STLAllocator<GCETask>> CentralTaskQueue;
	CentralTaskQueue mCentralTaskQueue;
	int64_t mRemainTaskCount;
};

extern GrandCentralExecuter* GGrandCentralExecuter;



template <class T, class F, class... Args>
void GCEDispatch(T instance, F memfunc, Args&&... args)
{
	/// shared_ptr이 아닌 녀석은 받으면 안된다. 작업큐에 들어있는중에 없어질 수 있으니..
	static_assert(true == is_shared_ptr<T>::value, "T should be shared_ptr");

	//TODO: intance의 memfunc를 std::bind로 묶어서 전달
	//T&& -> 오로지 우측에만 올 수 있는 타입 rvalue
	//vector 등에서 기존의 크기보다 더 큰 크기가 필요할 때 
	//기존의 메모리를 삭제하고 임시메모리를 할당하는 등의 불필요한 연산이 일어남
	//이를 방지하기 위해 생겨남
	//다양한 c++ value들 http://en.cppreference.com/w/cpp/language/value_category
	//템플릿사용시 인자추론으로 인해 템플릿 사용시 rvalue와 lvalue의 경계가 모호해져 버린다. 
	//forward는 rvalue는 rvalue로 lvalue는 lvalue로 캐스팅하여 템플릿이 해깔리지 않게 해준다.

	///# very good!

	auto task = std::bind(memfunc, instance, std::forward<Args>(args)...);
	GGrandCentralExecuter->DoDispatch(task);
}