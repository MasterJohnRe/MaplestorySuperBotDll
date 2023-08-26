// mapleSuperBotDll.cpp : Defines the exported functions for the DLL.
//

#include "pch.h"
#include "framework.h"
#include "mapleSuperBot.h"
#include "mapleSuperBotDll.h"
#include "fileHandler.h"
#include <intrin.h>
// This is an example of an exported variable

//MAPLESUPERBOTDLL_API CmapleSuperBotDll superBot;


// This is an example of an exported function.

//MAPLESUPERBOTDLL_API int fnmapleSuperBotDll(void)
//{
//    return 0;
//}

std::string LOG_FILE_PATH = "logs2/logs.txt";
unsigned int const X_OFFSET = 0x88;
unsigned int const Y_OFFSET = 0x92;
const int X = 0;
const int Y = 1;
MapleSuperBot superBot;
DWORD restoreJumpHook = 0;
LPCSTR MONSTER_POSITION_ACCESS_FUNCTION_MODULE_NAME = "SHAPE2D.DLL";
DWORD MONSTER_POSITION_ACCESS_FUNCTION_OFFSET_FROM_MODULE = 0x1851B;//0x18514

extern "C" void RunAssemblyCode(DWORD restoreJumpHook);

void jumpHookCallback(CONTEXT context) {
	FileHandler logger;
	// get value of register that holds the monster x and y Position Addreses
	Point<DWORD, 2> newMonsterPositionAddress;
	newMonsterPositionAddress[X] = context.Rsi + X_OFFSET;
	newMonsterPositionAddress[Y] = context.Rsi + Y_OFFSET;
	if (!superBot.isMonsterInAddressesVector(newMonsterPositionAddress) && !superBot.isMonstersPositionsAddressesVectorFull())
	{
		superBot.addToMonstersPositionsAddressesVector(newMonsterPositionAddress);
		superBot.increasePositionCounter();
		logger.log(LOG_FILE_PATH, "monster number: " + std::to_string(superBot.getMonstersPositionsAddressesVector().size()));
		logger.log(LOG_FILE_PATH, "monster number: " + std::to_string(superBot.getnumberOfMonsters()));
		logger.log(LOG_FILE_PATH,"monster X position: " + std::to_string(newMonsterPositionAddress[X]) );
		logger.log(LOG_FILE_PATH, "monster Y position: " + std::to_string(newMonsterPositionAddress[Y]) );
		
	}
}


void logFunction() {
	FileHandler logger;
	logger.log(LOG_FILE_PATH, "myTrampoline executed");
}

void myTrampoline()
{
	// Call logging function to indicate we've entered the trampoline
	logFunction();
	// Save registers
	HANDLE hThread = GetCurrentThread();
	CONTEXT context;
	context.ContextFlags = CONTEXT_FULL;  // Specify the desired context flags
	GetThreadContext(hThread, &context);

	// Call the hook callback function
	jumpHookCallback(context);

	// Restore registers
	SetThreadContext(hThread, &context);

	RunAssemblyCode(restoreJumpHook);
}

MAPLESUPERBOTDLL_API DWORD runBot()
{
	DWORD hookAtFunctionModuleAddress = (DWORD)GetModuleHandleA(MONSTER_POSITION_ACCESS_FUNCTION_MODULE_NAME);
	if (!hookAtFunctionModuleAddress) {
		//GetLastError();
		return 0;
	}
	DWORD hookAtFunctionAddress = hookAtFunctionModuleAddress + MONSTER_POSITION_ACCESS_FUNCTION_OFFSET_FROM_MODULE;
	FileHandler logger;
	logger.log(LOG_FILE_PATH, "started bot");
	while (true) {
		if (superBot.isMonstersPositionsAddressesVectorFull())
		{
			logger.log(LOG_FILE_PATH, "isMonstersPositionsAddressesVector is full");
			//logger.log(LOG_FILE_PATH, "isMonstersPositionsAddressesVector is Full");
			if (superBot.getIsHookOn()) {
				superBot.disableHook(hookAtFunctionAddress);
				superBot.setIsHookOn(false);
				logger.log(LOG_FILE_PATH, "set isHookOn to False");
			}
			//superBot.printMonstersPositions();
			//execute attack
			superBot.initializeSquares();
			superBot.printMonstersSquares();
			superBot.executeAttack();
			//maybe set timeout to like 0.5 so that the positions adress vector gets full again.
		}
		else {
			logger.log(LOG_FILE_PATH, "isMonstersPositionsAddressesVector is not full");
			if (!superBot.getIsHookOn())
			{

				//MessageBoxA(NULL, "HELLO", "A", NULL);
				restoreJumpHook = superBot.enableHook(hookAtFunctionAddress, (DWORD)&myTrampoline, 5);
				superBot.setIsHookOn(true);
				logger.log(LOG_FILE_PATH, "set isHookOn to true");
				//sleep for 1 second so that the hook will full it's monsters
				Sleep(1000);
			}

		}
	}
	return 0;
	
}