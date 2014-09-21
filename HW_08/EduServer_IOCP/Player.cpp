#include "stdafx.h"
#include "Timer.h"
#include "ThreadLocal.h"
#include "ClientSession.h"
#include "Player.h"
#include "PlayerManager.h"
#include "PlayerWideEvent.h"
#include "GrandCentralExecuter.h"
#include "PlayerDBContext.h"
#include "DBManager.h"
#include "Map.h"

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
	FastSpinlockGuard criticalSection(mPlayerLock);

	memset(mPlayerName, 0, sizeof(mPlayerName));
	memset(mComment, 0, sizeof(mComment));
	mPlayerId = -1;
	mIsValid = false;
	mPosX = mPosY = mPosZ = 0;

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
	//OnTick();

}

void Player::OnTick()
{
	if (!IsAlive())
		return;

	if (true)
	{
		int buffId = mPlayerId * 100;
		int duration = (rand() % 3 + 2) * 1000;
		//int duration = 0;

		//GCE 예. (lock-order 귀찮고, 전역적으로 순서 보장 필요할 때)
		auto playerEvent = std::make_shared<AllPlayerBuffEvent>(buffId, duration);
		GCEDispatch(playerEvent, &AllPlayerBuffEvent::DoBuffToAllPlayers, mPlayerId);
	}

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

void Player::RequestLoad(int pid)
{
	LoadPlayerDataContext* context = new LoadPlayerDataContext(mSession, pid);
	GDatabaseManager->PostDatabsaseRequest(context);
}

void Player::ResponseLoad(int pid, float x, float y, float z, bool valid, wchar_t* name, wchar_t* comment)
{
	FastSpinlockGuard criticalSection(mPlayerLock);

	mPlayerId = pid;
	mPosX = x;
	mPosY = y;
	mPosZ = z;
	mIsValid = valid;

	wcscpy_s(mPlayerName, name);
	wcscpy_s(mComment, comment);

	wprintf_s(L"PID[%d], X[%f] Y[%f] Z[%f] NAME[%s] COMMENT[%s]\n", mPlayerId, mPosX, mPosY, mPosZ, mPlayerName, mComment);
}

void Player::RequestUpdatePosition(float x, float y, float z)
{
	//todo: DB에 플레이어 위치를 x,y,z로 업데이트 요청하기
	UpdatePlayerPositionContext* context = new UpdatePlayerPositionContext(mSession, mPlayerId);
	context->SetNewPosition(x, y, z);

	GDatabaseManager->PostDatabsaseRequest(context);
}

void Player::ResponseUpdatePosition(float x, float y, float z)
{
	FastSpinlockGuard criticalSection(mPlayerLock);
	mPosX = x;
	mPosY = y;
	mPosZ = z;

	//DB처리를 성공하면 zone을 설정
	SetZone();
}

void Player::RequestUpdateComment(const wchar_t* comment)
{
	UpdatePlayerCommentContext* context = new UpdatePlayerCommentContext(mSession, mPlayerId);
	context->SetNewComment(comment);
	GDatabaseManager->PostDatabsaseRequest(context);
}

void Player::ResponseUpdateComment(const wchar_t* comment)
{
	FastSpinlockGuard criticalSection(mPlayerLock);
	wcscpy_s(mComment, comment);
}

void Player::RequestUpdateValidation(bool isValid)
{
	UpdatePlayerValidContext* context = new UpdatePlayerValidContext(mSession, mPlayerId);
	context->mIsValid = isValid;
	GDatabaseManager->PostDatabsaseRequest(context);
}

void Player::ResponseUpdateValidation(bool isValid)
{
	FastSpinlockGuard criticalSection(mPlayerLock);
	mIsValid = isValid;
}


void Player::TestCreatePlayerData(const wchar_t* newName)
{
	//todo: DB스레드풀에 newName에 해당하는 플레이어 생성 작업을 수행시켜보기
	CreatePlayerDataContext* context = new CreatePlayerDataContext(mSession, newName);

	GDatabaseManager->PostDatabsaseRequest(context);

}

void Player::ResponseCreatePlayerData(const wchar_t* generatedName)
{
	wprintf_s(L"NAME[%s] Player Created\n", generatedName);
}

void Player::TestDeletePlayerData(int playerId)
{
	//todo: DB스레드풀에 playerId에 해당하는 플레이어 생성 삭제 작업을 수행시켜보기
	DeletePlayerDataContext* context = new DeletePlayerDataContext(mSession, playerId);
	GDatabaseManager->PostDatabsaseRequest(context);
}

void Player::ResponseDeletePlayerData(int playerId)
{
	wprintf_s(L"ID[%d] Player Delete\n", playerId);
}

void Player::SetZone()
{
	//나중에 map밖으로 나갈경우를 고려해야함
	ZonePtr newZone = GMap->GetZone(mPosX, mPosZ);
	
	if (newZone != mZone) 
	{
		//이전 존에서 삭제
		//일단 최초의 경우는 무시하도록 함 (수정 예정)
		if (mZone != nullptr)
			mZone->PopPlayer(mPlayerId);
		//플레이어의 존 교체
		mZone = newZone;
		//새로운 존에 추가
		mZone->PushPlayer(GetSharedFromThis<Player>());
	}
}
