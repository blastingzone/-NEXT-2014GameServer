#pragma once
#include <map>

#define MAP_MAX_SIZE_TOP		1000
#define MAP_MAX_SIZE_BOT		-1000
#define MAP_MAX_SIZE_RIGHT	1000
#define MAP_MAX_SIZE_LEFT	-1000

#define ZONE_SIZE			10

#define MAP_ZONE_ROW_NUM		(MAP_MAX_SIZE_RIGHT - MAP_MAX_SIZE_LEFT) / ZONE_SIZE
#define MAP_ZONE_COL_NUM		(MAP_MAX_SIZE_TOP - MAP_MAX_SIZE_BOT) / ZONE_SIZE

class Zone;
typedef std::shared_ptr<Zone> ZonePtr;

class Player;
typedef std::shared_ptr<Player> PlayerPtr;
typedef std::map<int, PlayerPtr> PlayerList;

class Map
{
public:
	Map();
	~Map();

	ZonePtr GetZone(float x, float y);


private:

	ZonePtr mZoneList[MAP_ZONE_ROW_NUM][MAP_ZONE_COL_NUM];

	void MakeZoneList();
};


class Zone
{
	//������ ���� ����ǰ� ����� �;���. ������ ����ϰ� �����Ͽ���.
	enum NEIGHBOR_TYPE {
		NEIGHBOR_T = 0, NEIGHBOR_L, NEIGHBOR_B, NEIGHBOR_R,
		NEIGHBOR_TL, NEIGHBOR_TR, NEIGHBOR_BL, NEIGHBOR_BR, NEIGHBOR_MAX
	};

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

	void		PushPlayer(PlayerPtr player);
	PlayerPtr	PopPlayer();

private:

	ZonePtr mNeighbor[NEIGHBOR_MAX];
	float	mConner[4][2];

	PlayerList mPlayerList;
};

