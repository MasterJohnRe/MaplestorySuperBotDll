// dllmain.cpp : Defines the entry point for the DLL application.
#include "pch.h"
#include "mapleSuperBotDll.h"
//#include <thread>


BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
    case DLL_PROCESS_ATTACH:
    {
        //std::thread t1(runBot,);
        
        auto thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)runBot, NULL, 0, NULL);
        //MessageBoxA(NULL, "HELLO", "A", NULL);

        
        //CloseHandle(thread);
    }
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
    case DLL_PROCESS_DETACH:
        break;
    }
    return TRUE;
}

