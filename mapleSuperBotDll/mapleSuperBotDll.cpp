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

unsigned int const X_OFFSET = 0x88;
unsigned int const Y_OFFSET = 0x92;
const int X = 0;
const int Y = 1;
const LPCSTR MAPLESTORY_HEALTH_MODULE_NAME = "maplestory.exe";
const uintptr_t MAPLESTORY_HEATLH_FUNCTION_OFFSET = 0x38A8818;
MapleSuperBot superBot;

//remove this and replace this with AddVectoredExceptionHandler
LONG WINAPI UnhandledExceptionFilterNew(EXCEPTION_POINTERS* pExceptionInfo)
{
	FileHandler logger;
	DWORD dwEndsceneJMP;
	//Check exception type
	if (pExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_BREAKPOINT || pExceptionInfo->ExceptionRecord->ExceptionCode == EXCEPTION_SINGLE_STEP)
	{
		// get value of register that holds the player health
		DWORD playerHealth = pExceptionInfo->ContextRecord->R13;
		logger.log(SUCCESS_LOG_FILE_PATH, "player Health: " + std::to_string(pExceptionInfo->ContextRecord->R13));
		logger.log(SUCCESS_LOG_FILE_PATH, "player Health address: " + std::to_string((pExceptionInfo->ContextRecord->R14 + 40)));
		pExceptionInfo->ContextRecord->Dr0 = 0;
		pExceptionInfo->ContextRecord->Dr1 = 0;
		pExceptionInfo->ContextRecord->Dr2 = 0;
		pExceptionInfo->ContextRecord->Dr3 = 0;
		pExceptionInfo->ContextRecord->Dr7 = 0;
		superBot.setIsHookOn(false);
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

MAPLESUPERBOTDLL_API DWORD runBot()
{
	FileHandler logger;
	uintptr_t hookAtAddress;
	logger.log(SUCCESS_LOG_FILE_PATH, "started bot");
	registerExceptionHandler();
	HMODULE hModule = GetModuleHandleA(MAPLESTORY_HEALTH_MODULE_NAME);
	if (hModule == NULL)
		return 0;
	hookAtAddress = reinterpret_cast<uintptr_t>(hModule) + MAPLESTORY_HEATLH_FUNCTION_OFFSET;
	while (true) {
		if (superBot.isMonstersPositionsAddressesVectorFull())
		{
			logger.log(SUCCESS_LOG_FILE_PATH, "isMonstersPositionsAddressesVector is full");
			//logger.log(SUCCESS_LOG_FILE_PATH, "isMonstersPositionsAddressesVector is Full");
			if (superBot.getIsHookOn()) {
				changeHardwareBpHookState(DISABLE_HWBP_HOOK, hookAtAddress);
				superBot.setIsHookOn(false);
				logger.log(SUCCESS_LOG_FILE_PATH, "set isHookOn to False");
			}
			//superBot.printMonstersPositions();
			//execute attack
			//superBot.initializeSquares();
			superBot.printMonstersSquares();
			//superBot.executeAttack();
			//maybe set timeout to like 0.5 so that the positions adress vector gets full again.
		}
		else {
			//logger.log(SUCCESS_LOG_FILE_PATH, "isMonstersPositionsAddressesVector is not full");
			if (!superBot.getIsHookOn())
			{
				logger.log(SUCCESS_LOG_FILE_PATH, "got here");
				changeHardwareBpHookState(ENABLE_HWBP_HOOK, hookAtAddress);
				superBot.setIsHookOn(true);
				logger.log(SUCCESS_LOG_FILE_PATH, "set isHookOn to true");
				//sleep for 1 second so that the hook will full it's monsters
				Sleep(1000);
			}

		}
	}
	return 0;
	
}