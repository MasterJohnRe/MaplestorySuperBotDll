#pragma once
#include "pch.h"
#include <Windows.h>
#include <iostream>
#include <tlhelp32.h>

class MemoryAccess
{
public:
	template<typename T>
	T readMemory(HANDLE proc, DWORD adr) {
		T val;
		ReadProcessMemory(proc, (LPVOID)adr, &val, sizeof(T), NULL);
		return val;
	}

	template<typename T>
	void writeMemory(HANDLE proc, DWORD adr, T val) {
		WriteProcessMemory(proc, (LPVOID)adr, &val, sizeof(T), NULL);
	}

	template<typename T>
	DWORD protectMemory(HANDLE proc, DWORD adr, DWORD prot) {
		DWORD oldProt;
		VirtualProtectEx(proc, (LPVOID)adr, sizeof(T), prot, &oldProt);
		return oldProt;
	}

	DWORD getGamePID(LPCSTR processName);

	uintptr_t GetModuleBaseAddress(DWORD procId, const wchar_t* modName);
};

