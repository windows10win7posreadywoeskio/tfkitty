#pragma once
#include "mem.hpp"
#ifndef _SSDK_TIER0
#define _SSDK_TIER0
#define LittleShort( val )			( val )
#define LittleLong( val )			( val )
namespace tier0 {
	inline long ThreadInterlockedIncrement(long volatile* p) { return _InterlockedIncrement(p); }
	inline int ThreadInterlockedIncrement(int volatile* p) { return ThreadInterlockedIncrement((long volatile*)p); }

	using fnMsg = void(*)(const char* fmt, ...);
	fnMsg Msg = reinterpret_cast<fnMsg>(mem::modexport<"tier0.dll", "Msg">());
	using int32 = std::int32_t;
	using uint32 = std::uint32_t;
	using uint8 = std::uint8_t;
	using uint16 = std::uint16_t;
	using uint = unsigned;
}
#endif // !_SSDK_TIER0
