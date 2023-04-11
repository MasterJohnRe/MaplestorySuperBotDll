#include "pch.h"
#include "mapleSuperBot.h"
#include "fileHandler.h"

int nmapleSuperBotDll = 0;
const int X = 0;
const int Y = 1;
//const unsigned int RESTORE_JUMP_HOOK = 0x0068A6B7;
const unsigned int MAPLESTORY_NUMBER_OF_MONSTERS_BASE_ADDRESS = 0x007EBFA4;
const std::vector<unsigned int> MAPLESTORY_NUMBER_OF_MONSTERS_OFFSETS = { 0x10 };
const LPCSTR MAPLESTORY_HANDLE_NAME = (LPCSTR)"Maplestory";
const wchar_t* MAPLESTORY_MOD_NAME = L"HeavenMS-localhost-WINDOW.exe";
FileHandler logger;
std::string LOG_FILE_PATH2 = "logs2/logs.txt";

uintptr_t GetModuleBaseAddress(DWORD procId, const wchar_t* modName)
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
				std::cout << modEntry.szExePath << std::endl;
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

uintptr_t FindDMAAddy(HANDLE hProc, uintptr_t ptr, std::vector<unsigned int> offsets)
{
	DWORD player2Addr = ptr;
	int readProcessResult;
	for (unsigned int i = 0; i < offsets.size() - 1; ++i)
	{
		player2Addr += offsets[i];
		readProcessResult = ReadProcessMemory(hProc, (BYTE*)player2Addr, &player2Addr, sizeof(DWORD), 0);

	}
	return player2Addr + offsets[offsets.size() - 1];
}

unsigned int getNumberOfMonsters(HANDLE process, unsigned int dynamicPtrBaseAddr)
{
	DWORD numberOfMonstersAddr;
	unsigned int numberOfMonsters;
	unsigned int dynamicPtrNumberOfMonstersBaseAddr = dynamicPtrBaseAddr + MAPLESTORY_NUMBER_OF_MONSTERS_BASE_ADDRESS;
	DWORD issuccedded = ReadProcessMemory(process, (BYTE*)(dynamicPtrNumberOfMonstersBaseAddr), &numberOfMonstersAddr, sizeof(DWORD), 0);
	numberOfMonstersAddr = FindDMAAddy(process, numberOfMonstersAddr, MAPLESTORY_NUMBER_OF_MONSTERS_OFFSETS);
	issuccedded = ReadProcessMemory(process, (BYTE*)(numberOfMonstersAddr), &numberOfMonsters, sizeof(DWORD), 0);
	return numberOfMonsters;
}





MapleSuperBot::MapleSuperBot() {
	this->memoryManipulation = MemoryAccess();
	//this->monstersPositionsRemovedOpcodes = { 0x0F, 0xBF , 0x47, 0x04, 0x3B,0x46,0x34 };
	this->monstersPositionsRemovedOpcodes = { 0x90, 0x90 , 0x90 , 0x90, 0x90,0x90,0x90 };
	//get Maplestory Window Process Id and store it in PID
	this->PID = this->memoryManipulation.getGamePID(MAPLESTORY_HANDLE_NAME);
	this->process = OpenProcess(
		PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE,
		FALSE,
		PID
	);

	this->dynamicPtrBaseAddr = GetModuleBaseAddress(PID, MAPLESTORY_MOD_NAME);
	this->numberOfMonsters = getNumberOfMonsters(this->process, this->dynamicPtrBaseAddr);
}

void MapleSuperBot::initializeSquares() {
	//clear monsterSquares and squaresMonsterCounter Array
	for (int i = 0; i < this->monstersPositionsAddressesVector.size(); i++) {
		//define Square
		DWORD monsterXAddress = this->monstersPositionsAddressesVector[i][X];
		signed int monsterX = -1;
		DWORD issucceddedX = ReadProcessMemory(process, (BYTE*)monsterXAddress, &monsterX, sizeof(signed int), 0);
		DWORD monsterYAddress = this->monstersPositionsAddressesVector[i][Y];
		signed int monsterY = -1;
		DWORD issucceddedY = ReadProcessMemory(process, (BYTE*)monsterYAddress, &monsterY, sizeof(signed int), 0);
		if (issucceddedX && issucceddedY) {
			int maxX = monsterX + 300;
			int minX = monsterX - 300;
			int maxY = monsterY + 100;
			int minY = monsterY - 100;

			std::vector<Point<DWORD, 2>> tmpVector;
			this->monstersSquares.push_back(tmpVector);
			this->squaresMonsterCounterVector.push_back(0);
			for (int j = 0; j < this->monstersPositionsAddressesVector.size(); j++) {
				//check if monster inside square
				//if so enter it to square array
				DWORD monsterXAddress = this->monstersPositionsAddressesVector[j][X];
				signed int monsterX = -1;
				DWORD issucceddedX = ReadProcessMemory(process, (BYTE*)monsterXAddress, &monsterX, sizeof(signed int), 0);
				DWORD monsterYAddress = this->monstersPositionsAddressesVector[j][Y];
				signed int monsterY = -1;
				DWORD issucceddedY = ReadProcessMemory(process, (BYTE*)monsterYAddress, &monsterY, sizeof(signed int), 0);
				if (issucceddedX && issucceddedY) {
					if (monsterX >= minX && monsterX <= maxX &&
						monsterY >= minY && monsterY <= maxY) {
						this->monstersSquares[i].push_back(this->monstersPositionsAddressesVector[j]);
						this->squaresMonsterCounterVector[i] += 1;
					}
				}
			}
		}
	}
}



void MapleSuperBot::executeAttack() {
	initializeSquares();
}

DWORD MapleSuperBot::enableHook(DWORD hookAt, DWORD newFunc, int size)
{
	if (size > 12) // shouldn't ever have to replace 12+ bytes
		return 0;
	DWORD newOffset = newFunc - hookAt - 5;
	auto oldProtection = memoryManipulation.protectMemory<DWORD[3]>(this->process, hookAt + 1, PAGE_EXECUTE_READWRITE);
	BYTE Nop[] = { 0x90,0x90,0x90,0x90,0x90,0x90,0x90 };
	int success = WriteProcessMemory(this->process, (LPVOID*)(DWORD)hookAt, &Nop, sizeof(Nop), NULL);
	memoryManipulation.writeMemory<BYTE>(this->process, hookAt, 0xE9);
	//memoryManipulation.writeMemory<DWORD>(this->process, hookAt + 1, newOffset);
	success = WriteProcessMemory(this->process, (LPVOID)(hookAt + 1 * sizeof(BYTE)), &newOffset, sizeof(DWORD), NULL);
	if (success)
		logger.log(LOG_FILE_PATH2, "enabled hook successfully");
	for (unsigned int i = 5; i < size; i++)
		memoryManipulation.writeMemory<BYTE>(this->process, hookAt + i, 0x90);
	memoryManipulation.protectMemory<DWORD[3]>(this->process, hookAt + 1, oldProtection);
	return hookAt + 5;
}

//optimized with breakpoint before - 0xCC:
//DWORD MapleSuperBot::enableHook(DWORD hookAt, DWORD newFunc, int size)
//{
//	if (size > 12) // shouldn't ever have to replace 12+ bytes
//		return 0;
//	DWORD newOffset = newFunc - hookAt - 5;
//	auto oldProtection = memoryManipulation.protectMemory<DWORD[3]>(this->process, hookAt + 1, PAGE_EXECUTE_READWRITE);
//	BYTE Nop[] = { 0x90,0x90,0x90,0x90,0x90,0x90,0x90 };
//	int success = WriteProcessMemory(this->process, (LPVOID*)(DWORD)hookAt, &Nop, sizeof(Nop), NULL);
//	//Writing software reakpoint
//	memoryManipulation.writeMemory<BYTE>(this->process, hookAt , 0xCC);
//
//	memoryManipulation.writeMemory<BYTE>(this->process, hookAt+1, 0xE9);
//	success = WriteProcessMemory(this->process, (LPVOID)(hookAt + 2 * sizeof(BYTE)), &newOffset, sizeof(DWORD), NULL);
//	if (success)
//		logger.log(LOG_FILE_PATH2, "enabled hook successfully");
//	for (unsigned int i = 6; i < size; i++)
//		memoryManipulation.writeMemory<BYTE>(this->process, hookAt + i, 0x90);
//	memoryManipulation.protectMemory<DWORD[3]>(this->process, hookAt + 1, oldProtection);
//	return hookAt + 7;
//}



void MapleSuperBot::disableHook(DWORD hookAt)
{
	auto oldProtection = memoryManipulation.protectMemory<DWORD[3]>(this->process, hookAt + 1, PAGE_EXECUTE_READWRITE);
	for (unsigned int i = 0; i < this->monstersPositionsRemovedOpcodes.size(); i++)
		memoryManipulation.writeMemory<BYTE>(this->process, hookAt + i, this->monstersPositionsRemovedOpcodes[i]);
	memoryManipulation.protectMemory<DWORD[3]>(this->process, hookAt + 1, oldProtection);
}

bool MapleSuperBot::isMonstersPositionsAddressesVectorFull() {
	if (this->monstersPositionsAddressesVector.size() < this->numberOfMonsters)
	{ 
		logger.log(LOG_FILE_PATH2, "wtf" );
		return false;
	}
	/*for (unsigned int i = 0; i < this->monstersPositionsAddressesVector.size(); i++)
	{
		DWORD monsterXAddress = this->monstersPositionsAddressesVector[i][X];
		signed int monsterX = -1;
		DWORD issuccedded = ReadProcessMemory(process, (BYTE*)monsterXAddress, &monsterX, sizeof(signed int), 0);
		logger.log(LOG_FILE_PATH2, "monster X" + std::to_string(monsterX));
		if (monsterX <= -1000 || monsterX >= 2000)
			logger.log(LOG_FILE_PATH2,"monster X out of range: " + std::to_string(monsterX));
			return false;
	}*/
	logger.log(LOG_FILE_PATH2, "returned true");
	return true;
}

bool MapleSuperBot::isMonsterInAddressesVector(Point<DWORD, 2> newMonsterPositionAddress) {
	for (unsigned int i = 0; i < this->monstersPositionsAddressesVector.size(); i++) {

		if (this->monstersPositionsAddressesVector[i] == newMonsterPositionAddress)
			return true;
	}
	return false;

}

std::vector<Point<DWORD, 2>> MapleSuperBot::getMonstersPositionsAddressesVector() {
	return this->monstersPositionsAddressesVector;
}

void MapleSuperBot::printMonstersPositions() {
	logger.log(LOG_FILE_PATH2, "");
	logger.log(LOG_FILE_PATH2, "Monsters Positions:");
	for (unsigned int i = 0; i < this->monstersPositionsAddressesVector.size(); i++) {
		DWORD monsterXAddress = this->monstersPositionsAddressesVector[i][X];
		signed int monsterX = -1;
		DWORD issuccedded = ReadProcessMemory(process, (BYTE*)monsterXAddress, &monsterX, sizeof(signed int), 0);
		DWORD monsterYAddress = this->monstersPositionsAddressesVector[i][Y];
		signed int monsterY = -1;
		issuccedded = ReadProcessMemory(process, (BYTE*)monsterYAddress, &monsterY, sizeof(signed int), 0);
		logger.log(LOG_FILE_PATH2, "monster " + std::to_string(i) + ":");
		logger.log(LOG_FILE_PATH2, "X coordinate value: " + std::to_string(monsterX));
		logger.log(LOG_FILE_PATH2, "Y coordinate value: " + std::to_string(monsterY));
	}
}

void MapleSuperBot::printMonstersSquares() {
	logger.log(LOG_FILE_PATH2, "");
	logger.log(LOG_FILE_PATH2, "Monsters Squares:");
	for (unsigned int i = 0; i < this->monstersSquares.size(); i++) {
		logger.log(LOG_FILE_PATH2, "Square: " + std::to_string(i));
		for (unsigned int j = 0; j < this->monstersSquares[i].size(); j++) {
			DWORD monsterXAddress = this->monstersSquares[i][j][X];
			signed int monsterX = -1;
			DWORD issuccedded = ReadProcessMemory(process, (BYTE*)monsterXAddress, &monsterX, sizeof(signed int), 0);
			DWORD monsterYAddress = this->monstersSquares[i][j][Y];
			signed int monsterY = -1;
			issuccedded = ReadProcessMemory(process, (BYTE*)monsterYAddress, &monsterY, sizeof(signed int), 0);
			logger.log(LOG_FILE_PATH2, "monster " + std::to_string(j) + ":");
			logger.log(LOG_FILE_PATH2, "X coordinate value: " + std::to_string(monsterX));
			logger.log(LOG_FILE_PATH2, "Y coordinate value: " + std::to_string(monsterY));
		}
	}
}

void MapleSuperBot::addToMonstersPositionsAddressesVector(Point<DWORD, 2> newMonsterPosition) {
	this->monstersPositionsAddressesVector.push_back(newMonsterPosition);
}

int MapleSuperBot::getnumberOfMonsters() {
	return this->numberOfMonsters;
}

int MapleSuperBot::getPositionCounter() {
	return this->positionCounter;
}


bool MapleSuperBot::getIsHookOn() {
	return this->isHookOn;
}

void MapleSuperBot::setIsHookOn(bool isHookOn) {
	this->isHookOn = isHookOn;
}

void MapleSuperBot::increasePositionCounter() {
	this->positionCounter++;
}
