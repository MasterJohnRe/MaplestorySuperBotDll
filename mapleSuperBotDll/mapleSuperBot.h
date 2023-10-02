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
	uintptr_t enableHook(uintptr_t hookAt, uintptr_t newFunc, int size);
	void disableHook(uintptr_t hookAt);
	bool isMonstersPositionsAddressesVectorFull();
	bool isMonsterInAddressesVector(Point<uintptr_t, 2> newMonsterPositionAddress);
	bool getIsHookOn();
	std::vector<Point<uintptr_t, 2>> getMonstersPositionsAddressesVector();
	void printMonstersPositions();
	void printMonstersSquares();
	void printMonstersSquare(int squareIndex);
	int getPositionCounter();
	void increasePositionCounter();
	void setIsHookOn(bool isHookOn);
	void start();
	void addToMonstersPositionsAddressesVector(Point<uintptr_t, 2> newMonsterPosition);
	void initializeSquares();
	int executeAttack();
	int getnumberOfMonsters();
	void removeMonstersFromAddressesVector(int biggestSquareIndex);
	int initializePlayerPosition();
	Point<uintptr_t, 2> logPlayerPosition();
	//void setRestoreJumpHook(DWORD restoreJumpHook);
	
private:
	HANDLE process;
	DWORD PID;
	MemoryAccess memoryManipulation;
	DWORD dynamicPtrBaseAddr;
	Point<uintptr_t, 2> playerPosition;
	std::vector<BYTE> monstersPositionsRemovedOpcodes;
	std::vector<Point<uintptr_t, 2>> monstersPositionsAddressesVector;
	std::vector<int> squaresMonsterCounterVector;
	std::vector<Point<uintptr_t, 2>> monstersSquaresXRanges;
	std::vector<Point<uintptr_t, 2>> monstersSquaresYRanges;
	std::vector<std::vector<Point<uintptr_t, 2>>> monstersSquares;
	unsigned int numberOfMonsters = 0;
	bool isHookOn;
	int positionCounter = 0;
};
