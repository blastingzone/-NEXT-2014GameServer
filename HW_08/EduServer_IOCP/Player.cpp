#include "stdafx.h"
#include "Timer.h"
#include "ThreadLocal.h"
#include "ClientSession.h"
#include "Player.h"
#include "PlayerManager.h"
#include "PlayerWideEvent.h"
#include "GrandCentralExecuter.h"

Player::Player(ClientSession* session) : mSession(session)
{
	PlayerReset();
}

Player::~Player()
{
	CRASH_ASSERT(false);
}

void Player::PlayerReset()
{
	/// 플레이어 맵에서 제거
	GPlayerManager->UnregisterPlayer(mPlayerId);

	mPlayerId = -1;
	mHeartBeat = -1;
	mIsAlive = false;
}

void Player::Start(int heartbeat)
{
	mIsAlive = true;
	mHeartBeat = heartbeat;
	
	/// ID 발급 및 플레잉어 맵에 등록
	mPlayerId = GPlayerManager->RegisterPlayer(GetSharedFromThis<Player>());

	/// 생명 불어넣기 ㄱㄱ
	OnTick();

}

void Player::OnTick()
{
	if (!IsAlive())
		return;

	
	/// 랜덤으로 이벤트를 발생시켜보기 (예: 다른 모든 플레이어에게 버프 주기)
	//이걸 늘리면 버그를 찾을 수 있을 듯하군
	//if (rand() % 100 == 0) ///< 1% 확률
	//가끔 LockOrder에서 뻑이남 처음에 몇번나고 안나서 짜증남
	//tick에 있는 lock에서 뻑이남
	//덤프파일 남기기 성공 LockOrder의 pop순서에서 뻑이남
	//timer에서 owner의 lock을 사용 task실행후 어 leavelock하니 이전에 걸었던 락하고 다르네?
	//1.task실행과정 중에 뭔가가 일어났다.
	//2.다른 쓰레드에서 해당 owner의 lock을 건드렸다.
	//
	if (true)
	{
		int buffId = mPlayerId * 100;
		int duration = (rand() % 3 + 2) * 1000;
		//int duration = 0;

		//GCE 예. (lock-order 귀찮고, 전역적으로 순서 보장 필요할 때)
		auto playerEvent = std::make_shared<AllPlayerBuffEvent>(buffId, duration);
		GCEDispatch(playerEvent, &AllPlayerBuffEvent::DoBuffToAllPlayers, mPlayerId);
	}


	//TODO: AllPlayerBuffDecay::CheckBuffTimeout를 GrandCentralExecuter를 통해 실행
	//make_shared이거 메모리 할당 아닌가? 그럼 풀에서 받아와야하는 것 아닌가?
	//뜯어보니 stl에서 만들어진 객체(_Ref_count_obj)에 할당하고 있는 것 같다.
	//allocate_shared를 이용하면 가능하다고 한다.(make_shared와 같은 기능을 하는 것 같다)
	//make_shared 하나의 메모리 블럭안에 포인터와 객체를 담는다.

	///# 그래 정석대로라면 풀에서 받는게 맞지..

	auto playerDecay = std::make_shared<AllPlayerBuffDecay>();
	GCEDispatch(playerDecay, &AllPlayerBuffDecay::CheckBuffTimeout);
	
	if (mHeartBeat > 0)
		DoSyncAfter(mHeartBeat, GetSharedFromThis<Player>(), &Player::OnTick);
}

void Player::AddBuff(int fromPlayerId, int buffId, int duration)
{
	printf_s("I am Buffed [%d]! from Player [%d]\n", buffId, fromPlayerId);

	/// 플레이어의 버프 리스트에 추가
	mBuffList.insert(std::make_pair(buffId, duration));
}

void Player::DecayTickBuff()
{
	/// 버프의 남은 시간을 주기적으로 수정하고, 시간이 종료되면 제거하기
	for (auto it = mBuffList.begin(); it != mBuffList.end();)
	{
		it->second -= mHeartBeat;

		if (it->second <= 0)
		{
			printf_s("Player [%d] BUFF [%d] expired\n", mPlayerId, it->first);
			mBuffList.erase(it++);
		}
		else
		{
			++it;
		}
	}
}