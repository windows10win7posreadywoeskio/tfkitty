#pragma once
#include "ssdk_dt.hpp"
#include <string_view>
using namespace ssdk;
namespace dt {
	void destroy_member(SendProp* sendTablePtr, int* sendTableLengthPtr, const char* propname);
	ServerClass* GetAllServerClasses();
	void dump_sendtables(std::string_view filename);
}