#include "dt.hpp"
#include <cstring>
#include <Windows.h>
#include "mem.hpp"
#include <fstream>
#include <format> // DOING THIS MAKES IT WORK FOR SOME REASON??????????

namespace dt {
	void destroy_member(SendProp* sendTablePtr, int* sendTableLengthPtr, const char* propname) {

		for (int i = 0; i < *sendTableLengthPtr; ++i) {
			if (std::strcmp(sendTablePtr[i].m_pVarName, propname) == 0) {
				size_t bytesToMove = (*sendTableLengthPtr - i) * sizeof(SendProp);
				if (bytesToMove > 0) {
					std::memmove(&sendTablePtr[i], &sendTablePtr[i + 1], bytesToMove);
				}
				--(*sendTableLengthPtr);
				return;
			}
		}
	}

	ServerClass* GetAllServerClasses() {
		using fnCreateInterface = void* (*)(const char*, int*);
		auto gamedll = reinterpret_cast<fnCreateInterface>(GetProcAddress(reinterpret_cast<HMODULE>(GetModuleHandleW(L"server.dll")), "CreateInterface"))("ServerGameDLL005", nullptr);
		return mem::vcall<10, ServerClass*>(gamedll);
	}

	void print_table(std::fstream& f, SendTable* table, int indent) {
		std::string indent_string(indent, '\t');
		for (int i = 0; i < table->m_nProps; i++) {
			auto prop = table->m_pProps[i];
			auto dt = prop.m_pDataTable;
			if (dt) {
				f << std::vformat("{}table: {} type: {}\n", std::make_format_args(indent_string, prop.m_pVarName, dt->m_pNetTableName));
				print_table(f, dt, indent + 1);
			}
			else {
				f << std::vformat("{}member: {}\n", std::make_format_args(indent_string, prop.m_pVarName));
			}
		}
	}

	void dump_sendtables(std::string_view filename) {
		std::fstream f(filename.data(), std::ios_base::out);
		auto current = GetAllServerClasses();
		while (current) {
			auto str = std::vformat("class {} (type {})\n", std::make_format_args(current->m_pNetworkName, current->m_pTable->m_pNetTableName));
			f << str;
			print_table(f, current->m_pTable, 0);
			current = current->m_pNext;
		}
	}
}
