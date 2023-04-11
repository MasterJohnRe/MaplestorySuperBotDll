// The following ifdef block is the standard way of creating macros which make exporting
// from a DLL simpler. All files within this DLL are compiled with the MAPLESUPERBOTDLL_EXPORTS
// symbol defined on the command line. This symbol should not be defined on any project
// that uses this DLL. This way any other project whose source files include this file see
// MAPLESUPERBOTDLL_API functions as being imported from a DLL, whereas this DLL sees symbols
// defined with this macro as being exported.
#pragma once
#ifdef MAPLESUPERBOTDLL_EXPORTS
#define MAPLESUPERBOTDLL_API __declspec(dllexport)
#else
#define MAPLESUPERBOTDLL_API __declspec(dllimport)
#endif

#include <Windows.h>
#include <iostream>
#include "MemoryAccess.h"
#include <vector>
#include "Point.h"


//extern MAPLESUPERBOTDLL_API int nmapleSuperBotDll;




MAPLESUPERBOTDLL_API DWORD runBot();
//MAPLESUPERBOTDLL_API DWORD runBot();
