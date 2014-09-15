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

	if (row < 0 || row >= MAP_ZONE_ROW_NUM || col < 0 || col >= MAP_ZONE_COL_NUM)
		return nullptr;

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

void Zone::PushPlayer(PlayerPtr player)
{
	mPlayerList[player->GetPlayerId()] = player;
}

PlayerPtr Zone::PopPlayer(int playerID)
{
	PlayerPtr player = mPlayerList.find( playerID )->second;
	mPlayerList.erase(playerID);
	return player;
}
