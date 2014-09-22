#pragma once

enum CryptPacketType
{
	PKT_CP_NONE = 0,
	PKT_CP_FIRST,
	PKT_CP_SECOND,
	PKT_CP_OK
};

#pragma pack(push, 1)

struct CryptPacketHeader
{
	CryptPacketHeader() : mSize(0), mType(PKT_CP_NONE) {}

	short mSize;
	short mType;
};

