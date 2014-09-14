#include "stdafx.h"
#include "Map.h"


Map::Map()
{

}

Map::~Map()
{

}

void Map::MakeZoneList()
{
	for (int i = 0; i < MAP_ZONE_ROW_NUM; ++i)
	{
		for (int j = 0; j < MAP_ZONE_COL_NUM; ++j)
		{
			//복사일어남
			mZoneList[i][j] = ZonePtr(new Zone(	MAP_MAX_SIZE_TOP - i*ZONE_SIZE, 
												MAP_MAX_SIZE_LEFT + j*ZONE_SIZE));

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
