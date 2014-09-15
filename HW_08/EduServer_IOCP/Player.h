#pragma once

#include "SyncExecutable.h"
#include "ContentsConfig.h"
class ClientSession;

class Zone;
typedef std::shared_ptr<Zone> ZonePtr;

class Player : public SyncExecutable
{
public:
	Player(ClientSession* session);
	virtual ~Player();

	int GetPlayerId() { return mPlayerId; }
	bool IsAlive() { return mIsAlive;  }
	void Start(int heartbeat);

	//자신의 위치를 이용해서 존을 설정
	void SetZone();

	void OnTick(); ///< 로그인후 1초마다 불리는 기능

	void PlayerReset();

	/// 플레이어에게 버프를 걸어주는 함수라고 치자.
	void AddBuff(int fromPlayerId, int buffId, int duration);

	/// 주기적으로 버프 시간 업데이트하는 함수
	void DecayTickBuff();


	bool IsLoaded() { return mPlayerId > 0; }

	void RequestLoad(int pid);
	void ResponseLoad(int pid, float x, float y, float z, bool valid, wchar_t* name, wchar_t* comment);

	void RequestUpdatePosition(float x, float y, float z);
	void ResponseUpdatePosition(float x, float y, float z);

	void RequestUpdateComment(const wchar_t* comment);
	void ResponseUpdateComment(const wchar_t* comment);

	void RequestUpdateValidation(bool isValid);
	void ResponseUpdateValidation(bool isValid);

	//추가 테스트 함수
	void ResponseCreatePlayerData(const wchar_t* generatedName);
	void ResponseDeletePlayerData(int playerId);

private:

	//TODO: 아래 2개 테스트 함수가 성공적으로 동작하는지 확인하기
	void TestCreatePlayerData(const wchar_t* newName);
	void TestDeletePlayerData(int playerId);

	FastSpinlock mPlayerLock;
	
	float	mPosX;
	float	mPosY;
	float	mPosZ;
	
	int		mPlayerId;
	bool	mIsValid;
	
	wchar_t	mPlayerName[MAX_NAME_LEN];
	wchar_t	mComment[MAX_COMMENT_LEN];

	int		mHeartBeat;
	bool	mIsAlive;

	/// 버프 리스트는 lock없이 GCE로 해보기
	std::map<int, int> mBuffList; ///< (id, time)

	ZonePtr				 mZone;
	ClientSession* const mSession;
	friend class ClientSession;
};

