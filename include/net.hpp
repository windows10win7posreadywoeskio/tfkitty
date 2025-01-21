#pragma once
#include <cstdint>
#include <format>
#include "Windows.h"
// Use this to pick apart the network stream, must be packed

#define UDP_HEADER_SIZE				(20+8)	// IP = 20, UDP = 8
#define MAX_ROUTABLE_PAYLOAD		1260	// Matches x360 size
#define NET_HEADER_FLAG_SPLITPACKET				-2
#define MIN_USER_MAXROUTABLE_SIZE   576  // ( X.25 Networks )
#define MIN_SPLIT_SIZE (MIN_USER_MAXROUTABLE_SIZE - sizeof(SPLITPACKET))
#define NET_MAX_MESSAGE                      4096
#define MAX_SPLITPACKET_SPLITS ( NET_MAX_MESSAGE / MIN_SPLIT_SIZE )
#define MAX_USER_MAXROUTABLE_SIZE   MAX_ROUTABLE_PAYLOAD
#define MAX_SPLIT_SIZE (MAX_USER_MAXROUTABLE_SIZE - sizeof(SPLITPACKET))

namespace nettypes {

#pragma pack(1)
	typedef struct
	{
		int		netID;
		int		sequenceNumber;
		int		packetID : 16;
		int		nSplitSize : 16;
	} SPLITPACKET;
#pragma pack()

	enum
	{
		NS_CLIENT = 0,	// client socket
		NS_SERVER,	// server socket
		NS_HLTV,
		NS_MATCHMAKING,
		NS_SYSTEMLINK,
		MAX_SOCKETS
	};

	constexpr inline char const* DescribeSocket(int sock)
	{
		switch (sock)
		{
		default:
			break;
		case NS_CLIENT:
			return "cl ";
		case NS_SERVER:
			return "sv ";
		case NS_HLTV:
			return "htv";
		case NS_MATCHMAKING:
			return "mat";
		case NS_SYSTEMLINK:
			return "lnk";
		}

		return "??";
	}

	typedef enum
	{
		NA_NULL = 0,
		NA_LOOPBACK,
		NA_BROADCAST,
		NA_IP,
	} netadrtype_t;
//#undef SetPort

	typedef struct netadr_s
	{
	public:	// members are public to avoid to much changes

		netadrtype_t	type;
		unsigned char	ip[4];
		unsigned short	port;
		inline std::string ToString() {
			return std::format("{}.{}.{}.{}:{}", ip[0], ip[1], ip[2], ip[3], port);
		}
	} netadr_t;

	// Split long packets.  Anything over 1460 is failing on some routers
	typedef struct
	{
		int		currentSequence;
		int		splitCount;
		int		totalSize;
		int		nExpectedSplitSize;
		char	buffer[NET_MAX_MESSAGE];	// This has to be big enough to hold the largest message
	} LONGPACKET;

	/*
		00000000 struct __cppobj bf_read : CBitRead // sizeof=0x24
		00000000 {                                       // XREF: netpacket_s/r
		00000000                                         // SVC_SendTable/r ...
		00000024 };
	*/
	struct bf_read {
		char pad[0x24];
	};
	typedef struct netpacket_s
	{
		netadr_t		from;		// sender IP
		int				source;		// received source 
		double			received;	// received time
		unsigned char* data;		// pointer to raw packet data
		bf_read			message;	// easy bitbuf data access
		int				size;		// size in bytes
		int				wiresize;   // size in bytes before decompression
		bool			stream;		// was send as stream
		struct netpacket_s* pNext;	// for internal use, should be NULL in public
	} netpacket_t;
}