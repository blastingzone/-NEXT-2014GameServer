#pragma once

enum CryptPacketType
{
	PKT_CP_FIRST = 0,
	PKT_CP_SECOND,
	PKT_CP_OK
};

struct CryptPacketHeader
{
	short size;
	short type;
};

