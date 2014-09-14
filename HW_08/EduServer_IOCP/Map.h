#pragma once

#define MAP_MAX_SIZE_TOP		1000
#define MAP_MAX_SIZE_BOT		-1000
#define MAP_MAX_SIZE_RIGHT	1000
#define MAP_MAX_SIZE_LEFT	-1000

#define ZONE_SIZE			10

#define MAP_ZONE_ROW_NUM		(MAP_MAX_SIZE_RIGHT - MAP_MAX_SIZE_LEFT) / ZONE_SIZE
#define MAP_ZONE_COL_NUM		(MAP_MAX_SIZE_TOP - MAP_MAX_SIZE_BOT) / ZONE_SIZE

class Zone;
typedef std::shared_ptr<Zone> ZonePtr;

class Map
{
public:
	Map();
	~Map();


	

private:

	ZonePtr mZoneList[MAP_ZONE_ROW_NUM][MAP_ZONE_COL_NUM];


	void MakeZoneList();

};


//������ ���� ����ǰ� ����� �;���. ������ ����ϰ� �����Ͽ���.
class Zone
{
	enum NEIGHBOR_TYPE { NEIGHBOR_T = 0, NEIGHBOR_L, NEIGHBOR_B, NEIGHBOR_R, 
						 NEIGHBOR_TL, NEIGHBOR_TR, NEIGHBOR_BL, NEIGHBOR_BR, NEIGHBOR_MAX };

	//    TopLeft(TL)      TopRight(TR)
	//              0------1
	//              |      |
	//              |      |
	//              2------3
	// BottomLeft(BL)      BottomRight(BR)
	enum CONNER_TYPE { CORNER_TL, CORNER_TR, CORNER_BL, CORNER_BR };
	enum { posX = 0, posY };

public:
	Zone(float xTL, float yTL);

	//������
	//���� �����غ��� ����ϰ� ���������� ���� �� ����.
	Zone(int zoneSize, int maxSize);
	~Zone();

private:

	ZonePtr mNeighbor[NEIGHBOR_MAX];
	float	mConner[4][2];
	


};

