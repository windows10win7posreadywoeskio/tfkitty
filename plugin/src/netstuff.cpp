#include <Windows.h>
#include <cstring>
#include "net.hpp"
#include <unordered_map>
#include "mem.hpp"
#include "safetyhook.hpp"
#include "init.hpp"
#include "ssdk_tier0.hpp"

using namespace nettypes;
using namespace mem;
using namespace tier0;

static volatile int32 s_SplitPacketSequenceNumber[MAX_SOCKETS] = { 1 };

class INetChannel;
struct CNetChan;

#define Q_memcpy std::memcpy
//#define Msg std::printf
std::unordered_map<CNetChan*, int32> packet_sequence;

struct CNetChan {
	int GetDataRate() {
		return *((DWORD*)this + 44);
	}
};

//v16 = sub_100704B0(0, s, buf, Size + 10, to, tolen, -1);
//int NET_SendTo( bool verbose, SOCKET s, const char FAR * buf, int len, const struct sockaddr FAR * to, int tolen, int iGameDataLength )
using fnNET_SendTo = int(*)(int a1, SOCKET s, char* buf, int len, const struct sockaddr* to, int tolen, int iGameDataLength);
static fnNET_SendTo NET_SendTo = reinterpret_cast<fnNET_SendTo>(rva<"engine.dll">(0x704B0));

// v16 = sub_10076A30(v32, 0, s, buf, Size + 10, to, tolen);
// 1956:int NET_QueuePacketForSend( CNetChan *chan, bool verbose, SOCKET s, const char FAR *buf, int len, const struct sockaddr FAR * to, int tolen, uint32 msecDelay )
using fnNET_QueuePacketForSend = int(*)(CNetChan* chan, bool verbose, SOCKET s, const char FAR* buf, int len, const struct sockaddr FAR* to, int tolen, uint32 msecDelay);
static fnNET_QueuePacketForSend NET_QueuePacketForSend = reinterpret_cast<fnNET_QueuePacketForSend>(rva<"engine.dll">(0x076A30));

CNetChan* hackcast(INetChannel* nc) {
	/*  v32 = __RTDynamicCast(
		  a1,
		  0,
		  (struct _s_RTTICompleteObjectLocator *)&INetChannel `RTTI Type Descriptor',
		  &CNetChan `RTTI Type Descriptor',
		  0);
	*/
	/*
		1: [esp] 18BD0FF0 18BD0FF0 
		2: [esp+4] 00000000 00000000 
		3: [esp+8] 0FC0BEFC engine.0FC0BEFC 
		4: [esp+C] 0FC0BFB8 engine.0FC0BFB8 
		5: [esp+10] 00000000 00000000 
	*/
	static auto fnptr = signature_scanner::scan(module("engine.dll"), "6A ? 68 ? ? ? ? E8 ? ? ? ? 8B 75 ? 85 F6 75");
	using fnRTDynamicCast = CNetChan * (*)(void* a1,
		int a2,
		std::uintptr_t a3,
		std::uintptr_t a4,
		int a5);
	return reinterpret_cast<fnRTDynamicCast>(fnptr)(nc, 0, rva<"engine.dll">(0x34befc), rva<"engine.dll">(0x34bfb8), 0);
}

std::string sockaddress(sockaddr* addr) {
	std::string ret;
	auto in = reinterpret_cast<sockaddr_in*>(addr);

	union {
		in_addr in;
		struct {
			std::uint8_t o1;
			std::uint8_t o2;
			std::uint8_t o3;
			std::uint8_t o4;
		};
	} u;

	u.in = in->sin_addr;
	return std::format("{}.{}.{}.{}:{}", u.o1, u.o2, u.o3, u.o4, std::byteswap(in->sin_port));
}

struct fake_cvar {
	std::uint8_t pad[48];
	std::int32_t value;
};

class CSplitPacketEntry
{
public:
	CSplitPacketEntry()
	{
		memset(&from, 0, sizeof(from));

		int i;
		for (i = 0; i < MAX_SPLITPACKET_SPLITS; i++)
		{
			splitflags[i] = -1;
		}

		memset(&netsplit, 0, sizeof(netsplit));
		lastactivetime = 0.0f;
	}

public:
	netadr_t		from;
	int				splitflags[MAX_SPLITPACKET_SPLITS];
	LONGPACKET		netsplit;
	// host_time the last time any entry was received for this entry
	float			lastactivetime;
};

int NET_SendLong(INetChannel* chan, int sock, SOCKET s, const char FAR* buf, int len, const struct sockaddr FAR* to, int tolen, int nMaxRoutableSize)
{
	CNetChan* netchan = hackcast(chan);

	int nSplitSizeMinusHeader = nMaxRoutableSize - sizeof(SPLITPACKET);

	int nSequenceNumber = -1;
	if (netchan)
	{
		//nSequenceNumber = netchan->IncrementSplitPacketSequence();
		nSequenceNumber = ++packet_sequence[netchan];
	}
	else
	{
		nSequenceNumber = ThreadInterlockedIncrement(&s_SplitPacketSequenceNumber[sock]);
	}

	const char* sendbuf = buf;
	int sendlen = len;

	char			packet[MAX_ROUTABLE_PAYLOAD];
	SPLITPACKET* pPacket = (SPLITPACKET*)packet;

	// Make pPacket data network endian correct
	pPacket->netID = LittleLong(NET_HEADER_FLAG_SPLITPACKET);
	pPacket->sequenceNumber = LittleLong(nSequenceNumber);
	pPacket->nSplitSize = LittleShort(nSplitSizeMinusHeader);

	int nPacketCount = (sendlen + nSplitSizeMinusHeader - 1) / nSplitSizeMinusHeader;

	int nBytesLeft = sendlen;
	int nPacketNumber = 0;
	int nTotalBytesSent = 0;
	bool bFirstSend = true;

	while (nBytesLeft > 0)
	{
		int size = min(nSplitSizeMinusHeader, nBytesLeft);

		pPacket->packetID = LittleShort((short)((nPacketNumber << 8) + nPacketCount));

		Q_memcpy(packet + sizeof(SPLITPACKET), sendbuf + (nPacketNumber * nSplitSizeMinusHeader), size);

		int ret = 0;

		// Setting net_queued_packet_thread to NET_QUEUED_PACKET_THREAD_DEBUG_VALUE goes into a mode where all packets are queued.. can be used to stress-test it.
		// Linux threads aren't prioritized well enough for this to work well (i.e. the queued packet thread doesn't get enough
		// attention to flush itself well). Plus, Linux doesn't have the problem that Windows has where, if you send 6 UDP packets
		// without giving up your timeslice, it'll just discard the 7th and later packets until you Sleep(). Linux doesn't
		// have that problem so it has no need for a queue.
#ifndef _LINUX 
		auto net_queued_packet_thread = *reinterpret_cast<fake_cvar**>(rva<"engine.dll">(0x38D114));

		if (netchan && (!bFirstSend || net_queued_packet_thread->value == 581304))
		{
			uint32 delay = (int)(1000.0f * ((float)(nPacketNumber * (nMaxRoutableSize + UDP_HEADER_SIZE)) / (float)netchan->GetDataRate()) + 0.5f);
			ret = NET_QueuePacketForSend(netchan, false, s, packet, size + sizeof(SPLITPACKET), to, tolen, delay);
		}
		else
#endif
		{
			// Also, we send the first packet no matter what
			// w/o a netchan, if there are too many splits, its possible the packet can't be delivered.  However, this would only apply to out of band stuff like
			//  server query packets, which should never require splitting anyway.
			ret = NET_SendTo(false, s, packet, size + sizeof(SPLITPACKET), to, tolen, -1);
		}

		// First split send
		bFirstSend = false;

		if (ret < 0)
		{
			return ret;
		}

		if (ret >= size)
		{
			nTotalBytesSent += size;
		}

		nBytesLeft -= size;
		++nPacketNumber;

		// Always bitch about split packets in debug
		auto cvar = *reinterpret_cast<fake_cvar**>(rva<"engine.dll">(0x38CB1C));
		if (cvar->value && cvar->value != 2)
		{
			Msg("--> [%s] Split packet %4i/%4i seq %5i size %4i mtu %4i to %s [ total %4i ]\n",
				DescribeSocket(sock),
				nPacketNumber,
				nPacketCount,
				pPacket->sequenceNumber,
				size,
				nMaxRoutableSize,
				sockaddress(const_cast<sockaddr*>(to)).c_str(),
				sendlen);
		}
	}

	return nTotalBytesSent;
}

static double* net_time = reinterpret_cast<double*>(rva<"engine.dll">(0x38C8D8));

using fnNET_FindOrCreateSplitPacketEntry = CSplitPacketEntry * (*)(const int, netadr_s* from);
fnNET_FindOrCreateSplitPacketEntry NET_FindOrCreateSplitPacketEntry = reinterpret_cast<fnNET_FindOrCreateSplitPacketEntry>(rva<"engine.dll">(0x751C0));

bool NET_GetLong(const int sock, netpacket_t* packet)
{
	int				packetNumber, packetCount, sequenceNumber, offset;
	short			packetID;
	SPLITPACKET* pHeader;

	if (packet->size < sizeof(SPLITPACKET))
	{
		Msg("Invalid split packet length %i\n", packet->size);
		return false;
	}

	CSplitPacketEntry* entry = NET_FindOrCreateSplitPacketEntry(sock, &packet->from);
	if (!entry)
		return false;

	entry->lastactivetime = static_cast<float>(*net_time);

	pHeader = (SPLITPACKET*)packet->data;
	// pHeader is network endian correct
	sequenceNumber = LittleLong(pHeader->sequenceNumber);
	packetID = LittleShort(pHeader->packetID);
	// High byte is packet number
	packetNumber = (packetID >> 8);
	// Low byte is number of total packets
	packetCount = (packetID & 0xff);

	int nSplitSizeMinusHeader = (int)LittleShort(pHeader->nSplitSize);
	if (nSplitSizeMinusHeader < MIN_SPLIT_SIZE ||
		nSplitSizeMinusHeader > MAX_SPLIT_SIZE)
	{
		Msg("NET_GetLong:  Split packet from %s with invalid split size (number %i/ count %i) where size %i is out of valid range [%d - %d ]\n",
			packet->from.ToString().c_str(),
			packetNumber,
			packetCount,
			nSplitSizeMinusHeader,
			MIN_SPLIT_SIZE,
			MAX_SPLIT_SIZE);
		return false;
	}

	if (packetNumber >= MAX_SPLITPACKET_SPLITS ||
		packetCount > MAX_SPLITPACKET_SPLITS)
	{
		Msg("NET_GetLong:  Split packet from %s with too many split parts (number %i/ count %i) where %i is max count allowed\n",
			packet->from.ToString().c_str(),
			packetNumber,
			packetCount,
			MAX_SPLITPACKET_SPLITS);
		return false;
	}

	// First packet in split series?
	if (entry->netsplit.currentSequence == -1 ||
		sequenceNumber != entry->netsplit.currentSequence)
	{
		entry->netsplit.currentSequence = sequenceNumber;
		entry->netsplit.splitCount = packetCount;
		entry->netsplit.nExpectedSplitSize = nSplitSizeMinusHeader;
	}

	if (entry->netsplit.nExpectedSplitSize != nSplitSizeMinusHeader)
	{
		Msg("NET_GetLong:  Split packet from %s with inconsistent split size (number %i/ count %i) where size %i not equal to initial size of %i\n",
			packet->from.ToString().c_str(),
			packetNumber,
			packetCount,
			nSplitSizeMinusHeader,
			entry->netsplit.nExpectedSplitSize
		);
		return false;
	}

	int size = packet->size - sizeof(SPLITPACKET);

	if (entry->splitflags[packetNumber] != sequenceNumber)
	{
		// Last packet in sequence? set size
		if (packetNumber == (packetCount - 1))
		{
			entry->netsplit.totalSize = (packetCount - 1) * nSplitSizeMinusHeader + size;
		}

		entry->netsplit.splitCount--;		// Count packet
		entry->splitflags[packetNumber] = sequenceNumber;

		auto cvar = *reinterpret_cast<fake_cvar**>(rva<"engine.dll">(0x38CB1C));
		if (cvar->value && cvar->value != 3)
		{
			Msg("<-- [%s] Split packet %4i/%4i seq %5i size %4i mtu %4i from %s\n",
				DescribeSocket(sock),
				packetNumber + 1,
				packetCount,
				sequenceNumber,
				size,
				nSplitSizeMinusHeader + sizeof(SPLITPACKET),
				packet->from.ToString().c_str());
		}
	}
	else
	{
		Msg("NET_GetLong:  Ignoring duplicated split packet %i of %i ( %i bytes ) from %s\n", packetNumber + 1, packetCount, size, packet->from.ToString().c_str());
	}


	// Copy the incoming data to the appropriate place in the buffer
	offset = (packetNumber * nSplitSizeMinusHeader);
	memcpy(entry->netsplit.buffer + offset, packet->data + sizeof(SPLITPACKET), size);

	// Have we received all of the pieces to the packet?
	if (entry->netsplit.splitCount <= 0)
	{
		entry->netsplit.currentSequence = -1;	// Clear packet
		if (entry->netsplit.totalSize > sizeof(entry->netsplit.buffer))
		{
			Msg("Split packet too large! %d bytes from %s\n", entry->netsplit.totalSize, packet->from.ToString().c_str());
			return false;
		}

		Q_memcpy(packet->data, entry->netsplit.buffer, entry->netsplit.totalSize);
		packet->size = entry->netsplit.totalSize;
		packet->wiresize = entry->netsplit.totalSize;
		return true;
	}

	return false;
}

safetyhook::InlineHook g_NET_SendLong{};
safetyhook::InlineHook g_NET_GetLong{};

int __cdecl Hook_NET_SendLong(INetChannel* a1, int a2, SOCKET s, char* Src, size_t a5, struct sockaddr* to, int tolen) {
	return NET_SendLong(a1, a2, s, Src, a5, to, tolen, 1260);
}

bool __cdecl Hook_NET_GetLong(const int a1, netpacket_t* a2) {
	return NET_GetLong(a1, a2);
}

void init_net() {
	g_NET_SendLong = safetyhook::create_inline(rva<"engine.dll">(0x76AF0), &Hook_NET_SendLong);
	g_NET_GetLong = safetyhook::create_inline(rva<"engine.dll">(0x752A0), &Hook_NET_GetLong);
}