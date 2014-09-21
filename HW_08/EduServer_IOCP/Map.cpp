#include "stdafx.h"
#include "Map.h"
#include "Player.h"

Map* GMap = nullptr;

Map::Map()
{
	MakeZoneList();
}

Map::~Map()
{

}

ZonePtr Map::GetZone(float x, float y)
{
	int row = static_cast<int>((x - MAP_MAX_SIZE_LEFT) / ZONE_SIZE);
	int col = static_cast<int>((y - MAP_MAX_SIZE_TOP) / ZONE_SIZE);

	//맵 밖으로 나가면 zone[0][0]에서 처리하도록
	if (row < 0 || 
		row >= MAP_ZONE_ROW_NUM || 
		col < 0 || 
		col >= MAP_ZONE_COL_NUM)
		return mZoneList[0][0];

	return mZoneList[row][col];
}

void Map::MakeZoneList()
{
	for (int i = 0; i < MAP_ZONE_ROW_NUM; ++i)
	{
		for (int j = 0; j < MAP_ZONE_COL_NUM; ++j)
		{
			//복사일어남
			mZoneList[i][j] = ZonePtr(new Zone(MAP_MAX_SIZE_LEFT + j*ZONE_SIZE,
				MAP_MAX_SIZE_TOP - i*ZONE_SIZE));
		}
	}
}



Zone::Zone(int zoneSize, int maxSize)
{

}

Zone::Zone(float xTL, float yTL)
{
	mConner[CORNER_TL][posX] = xTL;
	mConner[CORNER_TL][posY] = yTL;

	mConner[CORNER_TR][posX] = xTL + ZONE_SIZE;
	mConner[CORNER_TR][posY] = yTL;

	mConner[CORNER_BL][posX] = xTL;
	mConner[CORNER_BL][posY] = yTL + ZONE_SIZE;

	mConner[CORNER_BR][posX] = xTL + ZONE_SIZE;
	mConner[CORNER_BR][posY] = yTL + ZONE_SIZE;
}


Zone::~Zone()
{
}

void Zone::PushPlayer(Player* player)
{
	FastSpinlockGuard criticalSection(mZoneLock);
	
	//있을 경우 덮어쓰고 없을 경우 생성
// 	if ( player == mPlayerMap.find( player->GetPlayerId() )->second )
// 	{
// 
// 	}
// 	else
// 	{
// 		mPlayerMap.insert( make_pair(player->GetPlayerId(), player) );
// 	}

	// 이게 더 좋단 말이지? ㅋㅋ
	mPlayerMap[player->GetPlayerId()] = player;

}

Player* Zone::PopPlayer( int playerID )
{
	FastSpinlockGuard criticalSection(mZoneLock);

	Player* player = mPlayerMap.find(playerID)->second;
	mPlayerMap.erase(playerID);
	return player;
}

PlayerPtrList Zone::GetPlayerList()
{
	//read mode
	FastSpinlockGuard criticalSection(mZoneLock, false);

	//todo :vector 미리 확장해두어야함
	PlayerPtrList playerVec;

// 	int i = 0;
// 	for (auto iter : mPlayerMap)
// 	{
// 		playerVec[i] = iter.second;
// 		++i;
// 	}

	// 고전적인 루프 ㅋㅋ
	for ( auto iter = mPlayerMap.begin(); iter != mPlayerMap.end(); ++iter )
	{
		playerVec.push_back( iter->second );
	}

	return playerVec;
}
