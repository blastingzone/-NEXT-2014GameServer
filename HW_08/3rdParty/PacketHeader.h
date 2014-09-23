
#include <google/protobuf/io/zero_copy_stream_impl_lite.h>
#include <google/protobuf/text_format.h>
#include <google/protobuf/io/coded_stream.h>

struct PacketHeader
{
	short mSize;
	short mType;
};

const int PacketHeaderSize = sizeof(PacketHeader);