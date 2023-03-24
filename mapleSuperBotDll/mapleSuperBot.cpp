#include "pch.h"
#include "mapleSuperBot.h"


int nmapleSuperBotDll = 0;
const int X = 0;
const int Y = 1;
const unsigned int RESTORE_JUMP_HOOK = 0x0068A6B7;
const unsigned int MAPLESTORY_NUMBER_OF_MONSTERS_BASE_ADDRESS = 0x007EBFA4;
const std::vector<unsigned int> MAPLESTORY_NUMBER_OF_MONSTERS_OFFSETS = { 0x10 };
const LPCSTR MAPLESTORY_HANDLE_NAME = (LPCSTR)"Maplestory";
const wchar_t* MAPLESTORY_MOD_NAME = L"HeavenMS-localhost-WINDOW.exe";



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
	this->monstersPositionsRemovedOpcodes = { 0x0F, 0xBF , 0x47, 0x04, 0x3B,0x46,0x34 };
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
	for (unsigned int i = 5; i < size; i++)
		memoryManipulation.writeMemory<BYTE>(this->process, hookAt + i, 0x90);
	memoryManipulation.protectMemory<DWORD[3]>(this->process, hookAt + 1, oldProtection);
	return hookAt + 5;
}

void MapleSuperBot::disableHook(DWORD hookAt)
{
	auto oldProtection = memoryManipulation.protectMemory<DWORD[3]>(this->process, hookAt + 1, PAGE_EXECUTE_READWRITE);
	for (unsigned int i = 0; i < this->monstersPositionsRemovedOpcodes.size(); i++)
		memoryManipulation.writeMemory<BYTE>(this->process, hookAt + i, this->monstersPositionsRemovedOpcodes[i]);
	memoryManipulation.protectMemory<DWORD[3]>(this->process, hookAt + 1, oldProtection);
}

bool MapleSuperBot::isMonstersPositionsAddressesVectorFull() {
	if (this->monstersPositionsAddressesVector.size() != this->numberOfMonsters)
		return false;
	for (unsigned int i = 0; i < this->monstersPositionsAddressesVector.size(); i++)
	{
		DWORD monsterXAddress = this->monstersPositionsAddressesVector[i][X];
		DWORD monsterX = -1;
		DWORD issuccedded = ReadProcessMemory(process, (BYTE*)monsterXAddress, &monsterX, sizeof(DWORD), 0);
		if (monsterX <= -1000 || monsterX >= 2000)
			return false;
	}
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
