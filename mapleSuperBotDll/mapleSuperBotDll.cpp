// mapleSuperBotDll.cpp : Defines the exported functions for the DLL.
//

#include "pch.h"
#include "framework.h"
#include "mapleSuperBot.h"
#include "mapleSuperBotDll.h"
#include "fileHandler.h"


std::string ERRORS_LOG_FILE_PATH = "C:/logs/mapleSuperBotDll/error_logs.txt";
std::string SUCCESS_LOG_FILE_PATH = "C:/logs/mapleSuperBotDll/success_logs.txt";

ULONG FIRST_HANDLER = 1;

std::string ENABLE_HWBP_HOOK = "enable";
std::string DISABLE_HWBP_HOOK = "disable";

unsigned int const X_OFFSET = 0x00;
unsigned int const Y_OFFSET = 0x04;
const int X = 0;
const int Y = 1;
MapleSuperBot superBot;
const LPCSTR MAPLESTORY_MONSTER_POSITION_FUNCTION_MODULE_NAME = "GR2D_DX9.DLL";;
const uintptr_t MAPLESTORY_MONSTER_POSITION_FUNCTION_OFFSET = 0x153A4A;



//remove this and replace this with AddVectoredExceptionHandler
LONG WINAPI UnhandledExceptionFilterNew(EXCEPTION_POINTERS* pExceptionInfo)
{
	FileHandler logger;
	DWORD dwEndsceneJMP;
	//Check exception type
	if (pExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT || pExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_SINGLE_STEP)
	{
		// get value of register that holds the monster x and y Position Addreses
		Point<uintptr_t, 2> newMonsterPositionAddress;
		newMonsterPositionAddress[X] = pExceptionInfo->ContextRecord->Rdi + X_OFFSET;//Rdi + X_OFFSET;
		newMonsterPositionAddress[Y] = pExceptionInfo->ContextRecord->Rdi + Y_OFFSET;//Rdi + Y_OFFSET;
		if (!superBot.isMonsterInAddressesVector(newMonsterPositionAddress) && !superBot.isMonstersPositionsAddressesVectorFull())
		{
			superBot.addToMonstersPositionsAddressesVector(newMonsterPositionAddress);
			superBot.increasePositionCounter();
			logger.log(SUCCESS_LOG_FILE_PATH, "monster number: " + std::to_string(superBot.getMonstersPositionsAddressesVector().size()));
			logger.log(SUCCESS_LOG_FILE_PATH, "monster number: " + std::to_string(superBot.getnumberOfMonsters())); //TODO: fix getNumberOfMonsters function to dynamically get monsters
			logger.log(SUCCESS_LOG_FILE_PATH, "monster X position Address: " + std::to_string(newMonsterPositionAddress[X]));
			logger.log(SUCCESS_LOG_FILE_PATH, "monster Y position Address: " + std::to_string(newMonsterPositionAddress[Y]));

		}
		pExceptionInfo->ContextRecord->Dr0 = 0;
		pExceptionInfo->ContextRecord->Dr1 = 0;
		pExceptionInfo->ContextRecord->Dr2 = 0;
		pExceptionInfo->ContextRecord->Dr3 = 0;
		pExceptionInfo->ContextRecord->Dr7 = 0;

		superBot.setIsHookOn(false);
		//pExceptionInfo->ContextRecord->EFlags |= 00010000;

		return EXCEPTION_CONTINUE_EXECUTION; //Copies all registers from pExceptionInfo to the real registers
	}
	else {
		logger.log(ERRORS_LOG_FILE_PATH, "Wrong exception code: " + std::to_string(pExceptionInfo->ExceptionRecord->ExceptionCode));
	}

	return EXCEPTION_CONTINUE_SEARCH; //Keep searching for any exceptions
}


DWORD registerExceptionHandler() {
	FileHandler logger;
	PVOID handle = AddVectoredExceptionHandler(FIRST_HANDLER, UnhandledExceptionFilterNew);
	if (handle == nullptr)
	{
		// Failed to register the exception handler
		logger.log(ERRORS_LOG_FILE_PATH, "Failed to register exception handler\n");
		return 1;
	}
	else {
		logger.log(SUCCESS_LOG_FILE_PATH, "registered exception handler\n");
	}
	return 0;
}


DWORD changeHardwareBpHookState(std::string action, uintptr_t hookAtAddress) {
	FileHandler logger;
	logger.log(SUCCESS_LOG_FILE_PATH, "changing hardwareBpHookState");
	HANDLE hThreadSnap = INVALID_HANDLE_VALUE;
	THREADENTRY32 te32;

	//LPTOP_LEVEL_EXCEPTION_FILTER Top = SetUnhandledExceptionFilter(UnhandledExceptionFilterNew);
	hThreadSnap = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);

	if (hThreadSnap)
	{
		te32.dwSize = sizeof(THREADENTRY32);

		if (!Thread32First(hThreadSnap, &te32))
		{
			logger.log(ERRORS_LOG_FILE_PATH, "couldn't get thread");
			CloseHandle(hThreadSnap);
			return 0;
		}

		do
		{
			if (te32.th32OwnerProcessID == GetCurrentProcessId() && te32.th32ThreadID != GetCurrentThreadId()) //Ignore threads from other processes AND the own thread of course
			{
				HANDLE hThread = OpenThread(THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME, 0, te32.th32ThreadID);
				if (hThread)
				{
					CONTEXT context;
					context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
					SuspendThread(hThread); //Suspend the thread so we can safely set a breakpoint

					if (GetThreadContext(hThread, &context))
					{
						if (action == ENABLE_HWBP_HOOK)
						{
							context.Dr0 = hookAtAddress; //Dr0 - Dr3 contain the address you want to break at
							//Those flags activate the breakpoints set in Dr0 - Dr3
							//You can either set break on: EXECUTE, WRITE or ACCESS
							//The Flags I'm using represent the break on write
							context.Dr6 = 0x00000000;
							context.Dr7 = 0x00000001;
							logger.log(SUCCESS_LOG_FILE_PATH, "set DR values successfully");
						}
						else if (action == DISABLE_HWBP_HOOK) 
						{
							context.Dr0 = 0; //Dr0 - Dr3 contain the address you want to break at
							context.Dr7 = 0;
							logger.log(SUCCESS_LOG_FILE_PATH, "disabled hook");
						}
						

						if(!SetThreadContext(hThread, &context)){
							logger.log(ERRORS_LOG_FILE_PATH, "couldn't get set Thread Context for the error code: " + std::to_string(GetLastError()));
						}
					}
					else {
						logger.log(ERRORS_LOG_FILE_PATH, "GetThreadContext failed");
					}

					ResumeThread(hThread);
					CloseHandle(hThread);
				}
				else {
					logger.log(ERRORS_LOG_FILE_PATH, "OpenThread failed");
				}
			}
		} while (Thread32Next(hThreadSnap, &te32));
		CloseHandle(hThreadSnap);
	}
}

void setDebugRegisters(DWORD mainThreadID, uintptr_t hookAtAddress) {
	FileHandler logger;
	logger.log(SUCCESS_LOG_FILE_PATH, "checking if hardwareBpExists");
	HANDLE hThread = OpenThread(THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_SUSPEND_RESUME, 0, mainThreadID);
	if (hThread)
	{
		CONTEXT context;
		context.ContextFlags = CONTEXT_DEBUG_REGISTERS;
		SuspendThread(hThread); //Suspend the thread so we can safely set a breakpoint

		if (GetThreadContext(hThread, &context))
		{
			if (context.Dr0 != hookAtAddress || context.Dr7 != 0x00000001) {
				logger.log(SUCCESS_LOG_FILE_PATH, "Debug Registers are not set");
				context.Dr0 = hookAtAddress;
				context.Dr6 = 0x00000000;
				context.Dr7 = 0x00000001;
			}
			if (!SetThreadContext(hThread, &context)) {
				logger.log(ERRORS_LOG_FILE_PATH, "couldn't set Thread Context for the error code: " + std::to_string(GetLastError()));
			}
		}
		else {
			logger.log(ERRORS_LOG_FILE_PATH, "GetThreadContext failed");
		}
		ResumeThread(hThread);
		CloseHandle(hThread);
	}
	else {
		logger.log(ERRORS_LOG_FILE_PATH, "OpenThread failed");
	}
}

DWORD GetMainThreadId() {
	const std::shared_ptr<void> hThreadSnapshot(CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0), CloseHandle);
	if (hThreadSnapshot.get() == INVALID_HANDLE_VALUE) {
		throw std::runtime_error("GetMainThreadId failed");
	}

	THREADENTRY32 tEntry;
	tEntry.dwSize = sizeof(THREADENTRY32);
	DWORD result = 0;
	DWORD currentPID = GetCurrentProcessId();

	for (BOOL success = Thread32First(hThreadSnapshot.get(), &tEntry);
		!result && success && GetLastError() != ERROR_NO_MORE_FILES;
		success = Thread32Next(hThreadSnapshot.get(), &tEntry))
	{
		if (tEntry.th32OwnerProcessID == currentPID) {
			result = tEntry.th32ThreadID;
		}
	}

	return result;
}



MAPLESUPERBOTDLL_API DWORD runBot()
{
	FileHandler logger;
	uintptr_t hookAtAddress;
	DWORD mainThreadID = GetMainThreadId();
	HMODULE hModule = GetModuleHandleA(MAPLESTORY_MONSTER_POSITION_FUNCTION_MODULE_NAME);
	if (hModule == NULL)
		return 0;
	hookAtAddress = reinterpret_cast<uintptr_t>(hModule) + MAPLESTORY_MONSTER_POSITION_FUNCTION_OFFSET;
	logger.log(SUCCESS_LOG_FILE_PATH, "started bot");
	logger.log(SUCCESS_LOG_FILE_PATH, "main thread ID: " + std::to_string(mainThreadID));
	registerExceptionHandler();
	superBot.initializePlayerPosition();
	//superBot.logPlayerPosition();

	while (true) {
		if (superBot.isMonstersPositionsAddressesVectorFull())
		{
			//logger.log(SUCCESS_LOG_FILE_PATH, "isMonstersPositionsAddressesVector is full");
			if (superBot.getIsHookOn()) {
				changeHardwareBpHookState(DISABLE_HWBP_HOOK, hookAtAddress);
				superBot.setIsHookOn(false);
				logger.log(SUCCESS_LOG_FILE_PATH, "set isHookOn to False");
			}
			//superBot.printMonstersPositions();
			//execute attack
			/*superBot.initializeSquares();
			//superBot.printMonstersSquares();
			superBot.executeAttack();
			Sleep(200);//0.2 seconds TODO: check how much does this need to be and lower it
			*/
		}
		else {
			//logger.log(SUCCESS_LOG_FILE_PATH, "isMonstersPositionsAddressesVector is not full");
			setDebugRegisters(mainThreadID, hookAtAddress);
			Sleep(75);
			/*if (!superBot.getIsHookOn())
			{
				logger.log(SUCCESS_LOG_FILE_PATH, "got here");
				changeHardwareBpHookState(ENABLE_HWBP_HOOK, hookAtAddress);
				superBot.setIsHookOn(true);
				logger.log(SUCCESS_LOG_FILE_PATH, "set isHookOn to true");
				//sleep for 0.2 second so that the hook will full it's monsters
				Sleep(100);//TODO: lower this
			}*/

		}
	}
	
	return 0;
	
}