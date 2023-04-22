#include "pch.h"
#include "mapleSuperBot.h"
#include "fileHandler.h"

int nmapleSuperBotDll = 0;
const int X = 0;
const int Y = 1;
const float PLAYER_SPEED = 123.8;
const int ATTACK_WIDTH = 80;
const int ATTACK_HEIGHT = 30;
//const unsigned int RESTORE_JUMP_HOOK = 0x0068A6B7;
const unsigned int MAPLESTORY_NUMBER_OF_MONSTERS_BASE_ADDRESS = 0x007EBFA4;
const std::vector<unsigned int> MAPLESTORY_NUMBER_OF_MONSTERS_OFFSETS = { 0x10 };
const unsigned int PLAYER_OBJECT_BASE_ADDRESS = 0x00BEBF98;
const std::vector<unsigned int> MAPLESTORY_PLAYER_POSITION_X_OFFSETS = {0x00 , 0x116C };
const std::vector<unsigned int> MAPLESTORY_PLAYER_POSITION_Y_OFFSETS = {0x00, 0x1170 };
const LPCSTR MAPLESTORY_HANDLE_NAME = (LPCSTR)"MapleStory";
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

float getDistance(int x1, int y1, int x2, int y2) {
	return std::sqrt(std::pow(x2 - x1, 2) + std::pow(y2 - y1, 2) * 1.0);
}

bool isMonsterInRange(HANDLE process, Point<DWORD, 2> playerPositionAddresses, Point<DWORD, 2> monsterPositionAddresses, int attackWidth, int attackHeight) {
	logger.log(LOG_FILE_PATH2, "got here111");
	signed int playerX = 0;
	signed int playerY = 0;
	signed int monsterX = 0;
	signed int monsterY = 0;
	DWORD issuccedded = ReadProcessMemory(process, (BYTE*)(playerPositionAddresses[X]), &playerX, sizeof(DWORD), 0);
	issuccedded = ReadProcessMemory(process, (BYTE*)(playerPositionAddresses[Y]), &playerY, sizeof(DWORD), 0);
	issuccedded = ReadProcessMemory(process, (BYTE*)(monsterPositionAddresses[X]), &monsterX, sizeof(DWORD), 0);
	issuccedded = ReadProcessMemory(process, (BYTE*)(monsterPositionAddresses[Y]), &monsterY, sizeof(DWORD), 0);
	logger.log(LOG_FILE_PATH2, "monsterX: " + std::to_string(monsterX)  + ", playerX: " + std::to_string(playerX));
	logger.log(LOG_FILE_PATH2, "monsterY: " + std::to_string(monsterY) + ", playerY: " + std::to_string(playerY));
	if (abs(monsterX - playerX) > attackWidth)
		return false;
	if (abs(monsterY - playerY) > attackHeight)
		return false;
	return true;
}

void sendKeyWithSendMessage(HWND windowa, WORD key, char letter, int time = 0) {
	HWND window = FindWindowA(NULL, MAPLESTORY_HANDLE_NAME);
	SetForegroundWindow(window);
	// Define the input event for a arrow key press
	INPUT input = { 0 };
	input.type = INPUT_KEYBOARD;
	input.ki.wVk = key; // virtual key code for left arrow key
	input.ki.dwFlags = 0; // key press

	// Send the key press event
	SendInput(1, &input, sizeof(INPUT));

	// Wait for 3 seconds (3000 milliseconds)
	Sleep(time * 1000);

	// Define the input event for a left arrow key release
	input.ki.dwFlags = KEYEVENTF_KEYUP; // key release

	// Send the key release event
	SendInput(1, &input, sizeof(INPUT));
}

void sendKey(int vk, BOOL bExtended, int timeToSleep = 0) {
	HWND window = FindWindowA(NULL, MAPLESTORY_HANDLE_NAME);
	SetForegroundWindow(window);
	KEYBDINPUT  kb = { 0 };
	INPUT       Input = { 0 };
	int scan = MapVirtualKey(vk, 0);

	/* Generate a "key down" */
	if (bExtended) { kb.dwFlags = KEYEVENTF_EXTENDEDKEY; }
	kb.wVk = vk;
	Input.type = INPUT_KEYBOARD;
	Input.ki = kb;
	Input.ki.wScan = scan;
	SendInput(1, &Input, sizeof(Input));

	Sleep(timeToSleep * 1000);
	/* Generate a "key up" */
	ZeroMemory(&kb, sizeof(KEYBDINPUT));
	ZeroMemory(&Input, sizeof(INPUT));
	kb.dwFlags = KEYEVENTF_KEYUP;
	if (bExtended) { kb.dwFlags |= KEYEVENTF_EXTENDEDKEY; }
	kb.wVk = vk;
	Input.type = INPUT_KEYBOARD;
	Input.ki = kb;
	Input.ki.wScan = scan;
	SendInput(1, &Input, sizeof(Input));

	return;
}

void changeDirectionToMonster(HWND window, HANDLE process, Point<DWORD, 2> playerPositionAddresses, Point<DWORD, 2> monsterPositionAddresses) {
	signed int playerX = 0;
	signed int playerY = 0;
	signed int monsterX = 0;
	signed int monsterY = 0;
	DWORD issuccedded = ReadProcessMemory(process, (BYTE*)(playerPositionAddresses[X]), &playerX, sizeof(DWORD), 0);
	issuccedded = ReadProcessMemory(process, (BYTE*)(playerPositionAddresses[Y]), &playerY, sizeof(DWORD), 0);
	issuccedded = ReadProcessMemory(process, (BYTE*)(monsterPositionAddresses[X]), &monsterY, sizeof(DWORD), 0);
	issuccedded = ReadProcessMemory(process, (BYTE*)(monsterPositionAddresses[Y]), &monsterY, sizeof(DWORD), 0);
	if ((playerX - monsterX) > 0) {
		logger.log(LOG_FILE_PATH2,"went left");
		sendKeyWithSendMessage(window, VK_LEFT, 0,0.05);
	}
	else {
		logger.log(LOG_FILE_PATH2, "went right");
		sendKeyWithSendMessage(window, VK_RIGHT, 0,0.05);
	}
}

bool isMonsterExists(HWND window, HANDLE process, Point<DWORD, 2> monsterPositionAddresses) {
	signed int monsterX = 0;
	signed int monsterY = 0;
	DWORD issuccedded = ReadProcessMemory(process, (BYTE*)(monsterPositionAddresses[X]), &monsterX, sizeof(DWORD), 0);
	issuccedded = ReadProcessMemory(process, (BYTE*)(monsterPositionAddresses[Y]), &monsterY, sizeof(DWORD), 0);
	logger.log(LOG_FILE_PATH2, "isMonsterExists Output:");
	logger.log(LOG_FILE_PATH2,"monsterX: " + std::to_string(monsterX) + ", monsterY: " + std::to_string(monsterY));
	if (monsterX <= -1000 || monsterX >= 2000)
	{
		return false;
	}
	return true;
}


void goToMonster(HWND window, HANDLE process , Point<DWORD, 2> playerPositionAddresses, Point<DWORD, 2> monsterPositionAddresses) {
	signed int playerX = 0;
	signed int playerY = 0;
	signed int monsterX = 0;
	signed int monsterY = 0;
	DWORD issuccedded = ReadProcessMemory(process, (BYTE*)(playerPositionAddresses[X]), &playerX, sizeof(DWORD), 0);
	issuccedded = ReadProcessMemory(process, (BYTE*)(playerPositionAddresses[Y]), &playerY, sizeof(DWORD), 0);
	issuccedded = ReadProcessMemory(process, (BYTE*)(monsterPositionAddresses[X]), &monsterX, sizeof(DWORD), 0);
	issuccedded = ReadProcessMemory(process, (BYTE*)(monsterPositionAddresses[Y]), &monsterY, sizeof(DWORD), 0);
	float distance = abs(monsterX - playerX);
	logger.log(LOG_FILE_PATH2, "distance: " + std::to_string(distance));
	float timeToWalk = distance / PLAYER_SPEED;
	logger.log(LOG_FILE_PATH2, "time to walk: " + std::to_string(timeToWalk));
	if (monsterX - playerX > 0) {
		logger.log(LOG_FILE_PATH2, "walking Right");
		sendKeyWithSendMessage(window, VK_RIGHT, 0,timeToWalk);
		
	}
	else {
		logger.log(LOG_FILE_PATH2, "walking Left");
		sendKeyWithSendMessage(window, VK_LEFT,0, timeToWalk);
	}
}

void attackMonster(HWND window, HANDLE process) {
	logger.log(LOG_FILE_PATH2, "attacking monster");
	sendKey('A', 0, 1);
}


bool isMonsterDead(HANDLE process,Point<DWORD,2> monsterPosition) {
	signed int firstMonsterX = 0;
	signed int firstMonsterY = 0;
	DWORD issuccedded = ReadProcessMemory(process, (BYTE*)(monsterPosition[X]), &firstMonsterX, sizeof(DWORD), 0);
	issuccedded = ReadProcessMemory(process, (BYTE*)(monsterPosition[Y]), &firstMonsterY, sizeof(DWORD), 0);
	Sleep(3500); //wait second so the monster has to move.
	signed int secondMonsterX = 0;
	signed int secondMonsterY = 0;
	issuccedded = ReadProcessMemory(process, (BYTE*)(monsterPosition[X]), &secondMonsterX, sizeof(DWORD), 0);
	issuccedded = ReadProcessMemory(process, (BYTE*)(monsterPosition[Y]), &secondMonsterY, sizeof(DWORD), 0);
	logger.log(LOG_FILE_PATH2, "--------output from isMonsterDead--------");
	logger.log(LOG_FILE_PATH2, "first monster X: " + std::to_string(firstMonsterX));
	logger.log(LOG_FILE_PATH2, "second monster X: " + std::to_string(secondMonsterX));
	logger.log(LOG_FILE_PATH2, "first monster Y: " + std::to_string(firstMonsterY));
	logger.log(LOG_FILE_PATH2, "second monster Y: " + std::to_string(secondMonsterY));
	if (firstMonsterX == secondMonsterX && firstMonsterY == secondMonsterY)
		return true;
	else
		return false;
}


Point<DWORD,2> getPlayerPosition(HANDLE process) {
	Point<DWORD, 2> playerPosition;
	playerPosition[X] = FindDMAAddy(process, PLAYER_OBJECT_BASE_ADDRESS, MAPLESTORY_PLAYER_POSITION_X_OFFSETS);
	playerPosition[Y] = FindDMAAddy(process, PLAYER_OBJECT_BASE_ADDRESS, MAPLESTORY_PLAYER_POSITION_Y_OFFSETS);
	signed int playerX = 0;
	signed int playerY = 0;
	DWORD issuccedded = ReadProcessMemory(process, (BYTE*)(playerPosition[X]), &playerX, sizeof(DWORD), 0);
	issuccedded = ReadProcessMemory(process, (BYTE*)(playerPosition[Y]), &playerY, sizeof(DWORD), 0);
	logger.log(LOG_FILE_PATH2,"player position X: " + std::to_string(playerX));
	logger.log(LOG_FILE_PATH2,"player position Y: " + std::to_string(playerY));
	return playerPosition;
}

int getBiggestSquareIndex(std::vector<int> squaresMonsterCounterVector) {
	int biggestSquaresize = -1;
	int biggestSquareIndex = -1;
	for (int i = 0; i < squaresMonsterCounterVector.size(); i++) {
		if (squaresMonsterCounterVector[i] > biggestSquaresize) {
			biggestSquaresize = squaresMonsterCounterVector[i];
			biggestSquareIndex = i;
		}
	}
	return biggestSquareIndex;
}


int getClosestMonsterIndex(HANDLE process, Point<DWORD,2> playerPosition , std::vector<Point<DWORD, 2>> monsterSquare) {
	//get player x,y
	//go through monsters and get the lowest distance monster from player.
	int lowestDistance = 10000;
	int lowestDistanceMonsterIndex = -1;
	signed int playerX = 0;
	signed int playerY = 0;
	DWORD issuccedded = ReadProcessMemory(process, (BYTE*)(playerPosition[X]), &playerX, sizeof(DWORD), 0);
	issuccedded = ReadProcessMemory(process, (BYTE*)(playerPosition[Y]), &playerY, sizeof(DWORD), 0);

	for (int i = 0; i < monsterSquare.size(); i++) {
		signed int monsterX = 0;
		signed int monsterY = 0;
		issuccedded = ReadProcessMemory(process, (BYTE*)(monsterSquare[i][X]), &monsterX, sizeof(DWORD), 0);
		issuccedded = ReadProcessMemory(process, (BYTE*)(monsterSquare[i][Y]), &monsterY, sizeof(DWORD), 0);
		signed int distance = getDistance(monsterX, monsterY, playerX, playerY);
		if (distance < lowestDistance) {
			lowestDistance = distance;
			lowestDistanceMonsterIndex = i;
		}
	}
	//return it's index.
	return lowestDistanceMonsterIndex;
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
	this->playerPosition = getPlayerPosition(this->process);
	this->numberOfMonsters = getNumberOfMonsters(this->process, this->dynamicPtrBaseAddr);
}


void MapleSuperBot::removeMonsterFromAddressesVector(DWORD xAddress) {
	std::vector<Point<DWORD, 2>>::iterator it = this->monstersPositionsAddressesVector.begin();
	std::vector<Point<DWORD, 2>>::iterator toRemove;
	logger.log(LOG_FILE_PATH2,"got here 1");
	for (it; it < this->monstersPositionsAddressesVector.end(); it++) {
		logger.log(LOG_FILE_PATH2, std::to_string((*it)[X]));
		if (xAddress == (*it)[X])
		{
			toRemove = it;
		}
	}
	try{
		this->monstersPositionsAddressesVector.erase(toRemove);
	}
	catch (...) {
		logger.log(LOG_FILE_PATH2, "Exception: couldn't erase monster - does not exist in monsterPositionsAddressesVector");
	}
}


int MapleSuperBot::executeAttack() {
	int biggestSquareIndex = getBiggestSquareIndex(this->squaresMonsterCounterVector);
	logger.log(LOG_FILE_PATH2, "biggest Square Index: " + std::to_string(biggestSquareIndex));
	if (biggestSquareIndex == -1) {
		logger.log(LOG_FILE_PATH2, "failed getting biggest square index");
		return 0;
	}
	int closestMonsterIndex = getClosestMonsterIndex(this->process, this->playerPosition, this->monstersSquares[biggestSquareIndex]);
	bool isClosestMonsterDead = false;
	logger.log(LOG_FILE_PATH2, "closestMonsterIndex: " + std::to_string(closestMonsterIndex));
	HWND window = FindWindowA(NULL, MAPLESTORY_HANDLE_NAME);
	if (IsWindow(window)){
		while (!isClosestMonsterDead) {
			logger.log(LOG_FILE_PATH2, "monster exists");
			if (isMonsterInRange(this->process, this->playerPosition, this->monstersSquares[biggestSquareIndex][closestMonsterIndex], ATTACK_WIDTH, ATTACK_HEIGHT))
			{
				changeDirectionToMonster(window, this->process, this->playerPosition, this->monstersSquares[biggestSquareIndex][closestMonsterIndex]);
				attackMonster(window, this->process);
				if (isMonsterDead(process, this->monstersSquares[biggestSquareIndex][closestMonsterIndex])) {
					isClosestMonsterDead = true;
					logger.log(LOG_FILE_PATH2, "monster is dead");
					removeMonsterFromAddressesVector(this->monstersSquares[biggestSquareIndex][closestMonsterIndex][X]);
					logger.log(LOG_FILE_PATH2, "removed monster from Addreses vector");
				}
			}
			else {
				logger.log(LOG_FILE_PATH2, "going to monster");
				goToMonster(window, this->process, this->playerPosition, this->monstersSquares[biggestSquareIndex][closestMonsterIndex]);
			}
		}
	}
	else {
		DWORD error = GetLastError();
		logger.log(LOG_FILE_PATH2,"couldn't get window in executeAttack for the error code: " + std::to_string(error));
	}
}


void MapleSuperBot::initializeSquares() {
	//clear monsterSquares and squaresMonsterCounter Array
	this->monstersSquares.clear();
	this->squaresMonsterCounterVector.clear();
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
		{
			logger.log(LOG_FILE_PATH2,"monster X out of range: " + std::to_string(monsterX));
			return false;
		}
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
