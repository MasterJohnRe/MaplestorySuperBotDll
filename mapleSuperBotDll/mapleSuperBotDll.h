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


// This class is exported from the dll
class MAPLESUPERBOTDLL_API CmapleSuperBotDll {
public:
	CmapleSuperBotDll MapleSuperBot();
	DWORD enableHook(DWORD hookAt, DWORD newFunc, int size);
	void disableHook(DWORD hookAt);
	bool isMonstersPositionsAddressesVectorFull();
	bool isMonsterInAddressesVector(Point<DWORD, 2> newMonsterPositionAddress);
	bool getIsHookOn();
	std::vector<Point<DWORD, 2>> getMonstersPositionsAddressesVector();
	int getPositionCounter();
	void increasePositionCounter();
	void setIsHookOn(bool isHookOn);
	//void setRestoreJumpHook(DWORD restoreJumpHook);

private:
	HANDLE process;
	DWORD PID;
	MemoryAccess memoryManipulation;
	DWORD dynamicPtrBaseAddr;
	unsigned int numberOfMonsters = 0;
	std::vector<BYTE> monstersPositionsRemovedOpcodes;
	std::vector<Point<DWORD, 2>> monstersPositionsAddressesVector;
	std::vector<int> squaresMonsterCounterVector;
	std::vector<std::vector<Point<int, 2>>> monstersSquares;
	bool isHookOn = false;
	int positionCounter = 0;
};

extern MAPLESUPERBOTDLL_API int nmapleSuperBotDll;

MAPLESUPERBOTDLL_API int fnmapleSuperBotDll(void);
