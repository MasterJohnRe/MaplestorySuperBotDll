#pragma once

#include <Windows.h>
#include <iostream>
#include "MemoryAccess.h"
#include <vector>
#include "Point.h"


// This class is exported from the dll
class MapleSuperBot {
public:
	MapleSuperBot();
	DWORD enableHook(DWORD hookAt, DWORD newFunc, int size);
	void disableHook(DWORD hookAt);
	bool isMonstersPositionsAddressesVectorFull();
	bool isMonsterInAddressesVector(Point<DWORD, 2> newMonsterPositionAddress);
	bool getIsHookOn();
	std::vector<Point<DWORD, 2>> getMonstersPositionsAddressesVector();
	int getPositionCounter();
	void increasePositionCounter();
	void setIsHookOn(bool isHookOn);
	void start();
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
