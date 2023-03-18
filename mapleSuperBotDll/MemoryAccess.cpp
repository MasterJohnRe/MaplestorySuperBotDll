
#ifndef MemoryAccess_HEADER
#define MemoryAccess_HEADER
#include "MemoryAccess.h"


DWORD MemoryAccess::getGamePID(LPCSTR processName) {
    HWND myWindow = FindWindowA(NULL, processName);
    DWORD PID;
    GetWindowThreadProcessId(myWindow, &PID);
    return PID;
}

uintptr_t MemoryAccess::GetModuleBaseAddress(DWORD procId, const wchar_t* modName)
{
    uintptr_t modBaseAddr = 0;
    HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE | TH32CS_SNAPMODULE32, procId);
    if (hSnap != INVALID_HANDLE_VALUE)
    {
        MODULEENTRY32 modEntry;
        modEntry.dwSize = sizeof(modEntry);
        if (Module32First(hSnap, &modEntry))
        {
            do
            {
                if (!_wcsicmp(modEntry.szModule, modName))
                {
                    modBaseAddr = (uintptr_t)modEntry.modBaseAddr;
                    break;
                }
            } while (Module32Next(hSnap, &modEntry));
        }
    }
    CloseHandle(hSnap);
    return modBaseAddr;
}



#endif