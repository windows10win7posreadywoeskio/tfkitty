#include "server_plugin.hpp"
#include "dt.hpp"
#include "mem.hpp"
#include "safetyhook.hpp"
#include "init.hpp"
#include <string>
using namespace ssdk;
using mem::byte_patch;
using mem::signature_scanner;

// 8B 44 24 ? 83 EC ? 53 8B D9 8D 48 -> 31 C0 40 C2 20 00
// 6A 20 89 4C 24 ? E8 ? ? ? ? 8B 54 24 -> 6A 10
// 6A 20 89 47 ? E8 -> 6A 10
class kittyplugin : public server_plugin {

	virtual const char* GetPluginDescription(void) override {
		return "kittyhook :3";
	}

	virtual bool Load(pfnCreateInterface interfaceFactory, pfnCreateInterface gameServerFactory) override{
		auto engine_module = mem::module("engine.dll");

		bool client = GetModuleHandle(L"client.dll") ? true : false;

		try {
			static byte_patch no_challenge(signature_scanner::scan(engine_module, "8B 44 24 ? 83 EC ? 53 8B D9 8D 48"), { 0x31, 0xC0, 0x40, 0xC2, 0x20,0x00 });
			static byte_patch stringtables_r(signature_scanner::scan(engine_module, "6A 20 89 4C 24 ? E8 ? ? ? ? 8B 54 24"), { 0x6A,0x10 });
			static byte_patch stringtables_w(signature_scanner::scan(engine_module, "6A 20 89 47 ? E8"), { 0x6A,0x10 });
			static byte_patch ignore_tables(signature_scanner::scan(engine_module, "74 ? E8 ? ? ? ? 39 86"), { 0xEB });
		}
		catch (std::exception& ex) {
			MessageBoxA(nullptr, std::format("sigscan failed: {}", ex.what()).c_str() , "kittyhook", 0);
		}

		auto current = dt::GetAllServerClasses();
		while (current) {
			if (std::string("DT_Beam") == current->m_pTable->m_pNetTableName) {
				dt::destroy_member(current->m_pTable->m_pProps, &current->m_pTable->m_nProps, "m_nMinDXLevel");
			}
			current = current->m_pNext;
		}

		dt::dump_sendtables("sendtables.txt");
		init_net();
		wchar_t _title[512]{};
		if (!client) {
			GetConsoleTitle(_title, ARRAYSIZE(_title));
			std::wstring title(_title);
			title += L" (kittyhook)";
			SetConsoleTitle(title.c_str());
		}
		else {
			HWND main_window{};
			EnumWindows([](HWND hWnd, LPARAM param) {
				wchar_t classname[1024];
				static std::wstring valve001(L"Valve001");
				static DWORD mypid = GetCurrentProcessId();
				DWORD pid;
				GetWindowThreadProcessId(hWnd, &pid);
				RealGetWindowClass(hWnd, classname, ARRAYSIZE(classname));
				if (pid == mypid && valve001 == classname) {
					auto mainwnd = reinterpret_cast<HWND*>(param);
					*mainwnd = hWnd;

					return FALSE;
				}
				return TRUE;
				}, reinterpret_cast<LPARAM>(&main_window));
			GetWindowText(main_window, _title, ARRAYSIZE(_title));
			std::wstring title(_title);
			title += L" (kittyhook)";
			SetWindowText(main_window, title.c_str());
		}
		return true;
	};
};
PLUGIN_ENTRY(kittyplugin)