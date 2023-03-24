// mapleSuperBotDll.cpp : Defines the exported functions for the DLL.
//

#include "pch.h"
#include "framework.h"
#include "mapleSuperBot.h"
#include "mapleSuperBotDll.h"

// This is an example of an exported variable

//MAPLESUPERBOTDLL_API CmapleSuperBotDll superBot;


// This is an example of an exported function.

//MAPLESUPERBOTDLL_API int fnmapleSuperBotDll(void)
//{
//    return 0;
//}


const unsigned int HOOK_AT_FUNCTION_ADDRESS = 0x0068A6B2;
unsigned int const X_OFFSET = 0x30;
unsigned int const Y_OFFSET = 0x34;
const int X = 0;
const int Y = 1;
MapleSuperBot superBot;
DWORD restoreJumpHook = 0;

void jumpHookCallback(DWORD EDI, DWORD ESI, DWORD EBP, DWORD ESP,
	DWORD EBX, DWORD EDX, DWORD ECX, DWORD EAX) {
	// get value of register that holds the monster x and y Position Addreses
	Point<DWORD, 2> newMonsterPositionAddress;
	newMonsterPositionAddress[X] = ESI + X_OFFSET;
	newMonsterPositionAddress[Y] = ESI + Y_OFFSET;
	if (!superBot.isMonsterInAddressesVector(newMonsterPositionAddress) && !superBot.isMonstersPositionsAddressesVectorFull())
	{
		superBot.getMonstersPositionsAddressesVector()[superBot.getPositionCounter()][X] = newMonsterPositionAddress[X];
		superBot.getMonstersPositionsAddressesVector()[superBot.getPositionCounter()][Y] = newMonsterPositionAddress[Y];
		superBot.increasePositionCounter();
	}
}


void  __declspec(naked) myTrampoline()
{
	__asm {
		PUSHFD
		PUSHAD
		CALL jumpHookCallback
		POPAD
		POPFD
		MOVSX EAX, WORD PTR[EDI + 0x04]
		CMP EAX, [ESI + 0x34]
		JMP[restoreJumpHook]
	}
}



MAPLESUPERBOTDLL_API DWORD runBot()
{
	while (true) {
		if (superBot.isMonstersPositionsAddressesVectorFull())
		{
			if (superBot.getIsHookOn()) {
				superBot.disableHook(HOOK_AT_FUNCTION_ADDRESS);
				superBot.setIsHookOn(false);
			}
			//EXECUTE THE BOT
		}
		else {
			if (!superBot.getIsHookOn())
			{

				restoreJumpHook = superBot.enableHook(HOOK_AT_FUNCTION_ADDRESS, (DWORD)&myTrampoline, 7);
				//superBot.setRestoreJumpHook(restoreJumpHook);
				superBot.setIsHookOn(true);
			}

		}
	}
}