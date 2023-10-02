#include "pch.h"
#include "mapleSuperBot.h"
#include "fileHandler.h"

int nmapleSuperBotDll = 0;
const int X = 0;
const int Y = 1;
const float PLAYER_SPEED = 175.8;//123.8;
const int ATTACK_WIDTH = 30;
const int ATTACK_HEIGHT = 80;
const int RANGE_FOR_DASH = 355;
const int TILE_UP_DISTANCE = 150;//TODO: edit this
const int SIDE_DASH_TIME_FOR_FIRST_ALT = 0.25;
const int DASH_UP_TIME_FOR_FIRST_ALT = 0.25;
const int DASH_UP_TIME_FOR_SECOND_ALT = 0.1;
const int FALL_DOWN_TIME_FOR_ALT = 0.1;
const int DASH_UP_ANIMATION_TIME = 0.9;
const int SIDE_DASH_ANIMATION_TIME = 0.93;
const int FALL_DOWN_ANIMATION_TIME = 0.9;

const int PLAYER_Y_OFFSET = -100;

const int MAP_MAX_X = -100;
const int MAP_MIN_X = -2100;
const int MAP_MAX_Y = 0;
const int MAP_MIN_Y = -400;
//const unsigned int RESTORE_JUMP_HOOK = 0x0068A6B7;
const unsigned int MAPLESTORY_NUMBER_OF_MONSTERS_BASE_ADDRESS = 0x007EBFA4;
const std::vector<unsigned int> MAPLESTORY_NUMBER_OF_MONSTERS_OFFSETS = { 0x10 };
const LPCSTR PLAYER_POSITION_BASE_MODULE = "SHAPE2D.DLL";
const std::vector<unsigned int> MAPLESTORY_PLAYER_POSITION_X_OFFSETS = { 0x000F1AB8,0x28, 0x28, 0x28, 0xC0, 0x70 }; //TODO: edit this
const std::vector<unsigned int> MAPLESTORY_PLAYER_POSITION_Y_OFFSETS = {0x00, 0x1170 }; //TODO: edit this
const LPCSTR MAPLESTORY_HANDLE_NAME = (LPCSTR)"MapleStory";
const wchar_t* MAPLESTORY_MOD_NAME = L"HeavenMS-localhost-WINDOW.exe";
FileHandler logger;
std::string LOG_FILE_PATH2 = "C:/logs/mapleSuperBotDll/success_positions_logs.txt";
std::string FIND_MY_ADDY_LOG_FILE_PATH = "C:/logs/mapleSuperBotDll/find_my_addy.txt";


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
	uintptr_t player2Addr = ptr;
	int readProcessResult;
	logger.log(FIND_MY_ADDY_LOG_FILE_PATH, "player position base address: " + std::to_string(ptr));
	for (unsigned int i = 0; i < offsets.size() - 1; ++i)
	{
		player2Addr += offsets[i];
		readProcessResult = ReadProcessMemory(hProc, reinterpret_cast<LPCVOID>(player2Addr), &player2Addr, sizeof(unsigned int), 0);
		logger.log(FIND_MY_ADDY_LOG_FILE_PATH, "address" + std::to_string(i) + ": " + std::to_string(readProcessResult));

	}
	logger.log(FIND_MY_ADDY_LOG_FILE_PATH, "address should be: " + std::to_string(player2Addr + offsets[offsets.size() - 1]));
	return player2Addr + offsets[offsets.size() - 1];
}

unsigned int getNumberOfMonsters(HANDLE process, unsigned int dynamicPtrBaseAddr)
{
	DWORD numberOfMonstersAddr;
	unsigned int numberOfMonsters;
	unsigned int dynamicPtrNumberOfMonstersBaseAddr = dynamicPtrBaseAddr + MAPLESTORY_NUMBER_OF_MONSTERS_BASE_ADDRESS;
	DWORD issuccedded = ReadProcessMemory(process, reinterpret_cast<LPCVOID>(dynamicPtrNumberOfMonstersBaseAddr), &numberOfMonstersAddr, sizeof(unsigned int), 0);
	numberOfMonstersAddr = FindDMAAddy(process, numberOfMonstersAddr, MAPLESTORY_NUMBER_OF_MONSTERS_OFFSETS);
	issuccedded = ReadProcessMemory(process, reinterpret_cast<LPCVOID>(numberOfMonstersAddr), &numberOfMonsters, sizeof(unsigned int), 0);
	//return numberOfMonsters; //TODO: change back to number of monsters
	return 10;
}

float getDistance(int x1, int y1, int x2, int y2) {
	return std::sqrt(std::pow(x2 - x1, 2) + std::pow(y2 - y1, 2) * 1.0);
}

bool isSquareInRange(HANDLE process, Point<uintptr_t, 2> playerPositionAddresses, float biggestSquareX, float biggestSquareY , int attackWidth, int attackHeight) {
	logger.log(LOG_FILE_PATH2, "got here111");
	signed int playerX = 0;
	signed int playerY = 0;
	DWORD issuccedded = ReadProcessMemory(process, reinterpret_cast<LPCVOID>(playerPositionAddresses[X]), &playerX, sizeof(unsigned int), 0);
	issuccedded = ReadProcessMemory(process, reinterpret_cast<LPCVOID>(playerPositionAddresses[Y]), &playerY, sizeof(unsigned int), 0);
	playerY = playerY + PLAYER_Y_OFFSET;
	logger.log(LOG_FILE_PATH2, "playerX: " + std::to_string(playerX)  + ", playerY: " + std::to_string(playerY));
	logger.log(LOG_FILE_PATH2, "biggest square X: " + std::to_string(biggestSquareX) + ", biggest SquareY: " + std::to_string(biggestSquareY));
	if (abs(biggestSquareX - playerX) > attackWidth)
		return false;
	if (abs(biggestSquareY - playerY) > attackHeight)
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
void turnLeft(HWND window, HANDLE process, int timeToSleep = 0) {
	FileHandler logger;
	logger.log(LOG_FILE_PATH2, "turning left");
	SetForegroundWindow(window);
	KEYBDINPUT leftArrowKb = { 0 };
	// Define the input event for a arrow key press
	INPUT KEY_B;
	int leftArrowScan = MapVirtualKey(VK_LEFT, 0);

	leftArrowKb.wVk = VK_LEFT;
	KEY_B.type = INPUT_KEYBOARD;
	KEY_B.ki = leftArrowKb;
	KEY_B.ki.wScan = leftArrowScan;  // left arrow key scan code

	// Press and release the left arrow key

	SendInput(1, &KEY_B, sizeof(INPUT));
	Sleep(timeToSleep * 1000 + 50);
	leftArrowKb.dwFlags = KEYEVENTF_KEYUP;
	KEY_B.ki = leftArrowKb;
	SendInput(1, &KEY_B, sizeof(INPUT));
}
void turnRight(HWND window, HANDLE process, int timeToSleep = 0) {
	FileHandler logger;
	logger.log(LOG_FILE_PATH2, "turning right");
	SetForegroundWindow(window);
	KEYBDINPUT rightArrowKb = { 0 };
	// Define the input event for a arrow key press
	INPUT KEY_B;
	int rightArrowScan = MapVirtualKey(VK_RIGHT, 0);

	rightArrowKb.wVk = VK_RIGHT;
	KEY_B.type = INPUT_KEYBOARD;
	KEY_B.ki = rightArrowKb;
	KEY_B.ki.wScan = rightArrowScan;  // left arrow key scan code

	// Press and release the left arrow key

	SendInput(1, &KEY_B, sizeof(INPUT));
	Sleep(timeToSleep * 1000 + 50);
	rightArrowKb.dwFlags = KEYEVENTF_KEYUP;
	KEY_B.ki = rightArrowKb;
	SendInput(1, &KEY_B, sizeof(INPUT));
}

void changeDirectionToSquare(HWND window, HANDLE process, Point<uintptr_t, 2> playerPositionAddresses, signed int squareX, signed int SquareY) {
	signed int playerX = 0;
	DWORD issuccedded = ReadProcessMemory(process, reinterpret_cast<LPCVOID>(playerPositionAddresses[X]), &playerX, sizeof(unsigned int), 0);
	if (playerX > squareX) {
		logger.log(LOG_FILE_PATH2,"went left");
		//sendKey(VK_LEFT, 0,0.15);
		turnLeft(window, process, 0.15);
	}
	else {
		logger.log(LOG_FILE_PATH2, "went right");
		//sendKey(VK_RIGHT, 0, 0.15);
		turnRight(window, process, 0.15);
	}
}

bool isSquareExists(HWND window, HANDLE process, signed int squareX, signed int squareY) {
	logger.log(LOG_FILE_PATH2, "isMonsterExists Output:");
	logger.log(LOG_FILE_PATH2,"squareX: " + std::to_string(squareX) + ", squareY: " + std::to_string(squareY));
	if (squareX <= MAP_MIN_X || squareX >= MAP_MAX_X || squareY <= MAP_MIN_Y || squareY >= MAP_MAX_Y)
	{
		return false;
	}
	return true;
}


void dash(HWND window, HANDLE process) {
	FileHandler logger;
	logger.log(LOG_FILE_PATH2, "dashing");
	SetForegroundWindow(window);
	// Define the input event for a arrow key press
	INPUT KEY_B;

	KEY_B.type = INPUT_KEYBOARD;
	KEY_B.ki.time = 0;
	KEY_B.ki.wVk = 0;
	KEY_B.ki.dwExtraInfo = 0;
	KEY_B.ki.dwFlags = KEYEVENTF_SCANCODE;
	KEY_B.ki.wScan = 0x39;  // Space key scan code

	// Press and release the Space key

	SendInput(1, &KEY_B, sizeof(INPUT));
	Sleep(50);
	KEY_B.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
	SendInput(1, &KEY_B, sizeof(INPUT));

	// Wait until next jump
	Sleep(0.20 * 1000);

	// Press and release the Space key again
	KEY_B.ki.dwFlags = KEYEVENTF_SCANCODE;
	SendInput(1, &KEY_B, sizeof(INPUT));
	Sleep(50);
	KEY_B.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
	SendInput(1, &KEY_B, sizeof(INPUT));

	Sleep(0.9 * 1000);
}

void dashUp(HWND window, HANDLE process) {
	FileHandler logger;
	logger.log(LOG_FILE_PATH2, "dashing up");
	SetForegroundWindow(window);
	INPUT KEY_B;

	KEY_B.type = INPUT_KEYBOARD;
	KEY_B.ki.time = 0;
	KEY_B.ki.wVk = 0;
	KEY_B.ki.dwExtraInfo = 0;
	KEY_B.ki.dwFlags = KEYEVENTF_SCANCODE;
	KEY_B.ki.wScan = 0x39;  // Space key scan code

	// Press and release the Space key

	SendInput(1, &KEY_B, sizeof(INPUT));
	Sleep(50);
	KEY_B.ki.dwFlags = KEYEVENTF_SCANCODE | KEYEVENTF_KEYUP;
	SendInput(1, &KEY_B, sizeof(INPUT));

	// Wait until next jump
	Sleep(0.20 * 1000);

	KEYBDINPUT upArrowKb = { 0 };
	KEYBDINPUT spaceKb = { 0 };
	INPUT input[2] = { 0 };
	int upArrowScan = MapVirtualKey(VK_UP, 0);
	int spaceScan = MapVirtualKey(VK_SPACE, 0);

	// Generate "key down" events for both keys
	upArrowKb.wVk = VK_UP;
	input[0].type = INPUT_KEYBOARD;
	input[0].ki = upArrowKb;
	input[0].ki.wScan = upArrowScan;

	spaceKb.wVk = VK_SPACE;
	input[1].type = INPUT_KEYBOARD;
	input[1].ki = spaceKb;
	input[1].ki.wScan = spaceScan;

	SendInput(2, input, sizeof(INPUT));

	// Simulate the keys being held down for some time (you can adjust the sleep duration)
	Sleep(1000);  // Sleep for 1 second

	// Generate "key up" events for both keys
	upArrowKb.dwFlags = KEYEVENTF_KEYUP;
	spaceKb.dwFlags = KEYEVENTF_KEYUP;

	input[0].ki = upArrowKb;
	input[1].ki = spaceKb;

	SendInput(2, input, sizeof(INPUT));
}

void fallDown(HWND window, HANDLE process) {
	FileHandler logger;
	logger.log(LOG_FILE_PATH2, "falling down a tile");
	SetForegroundWindow(window);

	KEYBDINPUT downArrowKb = { 0 };
	KEYBDINPUT spaceKb = { 0 };
	INPUT input[2] = { 0 };
	int upArrowScan = MapVirtualKey(VK_DOWN, 0);
	int spaceScan = MapVirtualKey(VK_SPACE, 0);

	// Generate "key down" events for both keys
	downArrowKb.wVk = VK_DOWN;
	input[0].type = INPUT_KEYBOARD;
	input[0].ki = downArrowKb;
	input[0].ki.wScan = upArrowScan;

	spaceKb.wVk = VK_SPACE;
	input[1].type = INPUT_KEYBOARD;
	input[1].ki = spaceKb;
	input[1].ki.wScan = spaceScan;

	SendInput(2, input, sizeof(INPUT));

	// Simulate the keys being held down for some time (you can adjust the sleep duration)
	Sleep(1000);  // Sleep for 1 second

	// Generate "key up" events for both keys
	downArrowKb.dwFlags = KEYEVENTF_KEYUP;
	spaceKb.dwFlags = KEYEVENTF_KEYUP;

	input[0].ki = downArrowKb;
	input[1].ki = spaceKb;

	SendInput(2, input, sizeof(INPUT));
}



void goToMonster(HWND window, HANDLE process , Point<uintptr_t, 2> playerPositionAddresses, signed int squareX, signed int squareY) {
	signed int playerX = 0;
	signed int playerY = 0;
	DWORD issuccedded;

	//handle X positioning
	issuccedded = ReadProcessMemory(process, reinterpret_cast<LPCVOID>(playerPositionAddresses[X]), &playerX, sizeof(unsigned int), 0);
	issuccedded = ReadProcessMemory(process, reinterpret_cast<LPCVOID>(playerPositionAddresses[Y]), &playerY, sizeof(unsigned int), 0);
	playerY = playerY + PLAYER_Y_OFFSET;
	float distanceX = abs(squareX - playerX);
	logger.log(LOG_FILE_PATH2, "distanceX: " + std::to_string(distanceX));
	while (distanceX > ATTACK_WIDTH) {
		if (distanceX > RANGE_FOR_DASH) {
			changeDirectionToSquare(window, process, playerPositionAddresses, squareX, squareY);
			dash(window, process);
		}
		else {
			float timeToWalk = distanceX / PLAYER_SPEED;
			logger.log(LOG_FILE_PATH2, "time to walk: " + std::to_string(timeToWalk));
			if (squareX > playerX) {
				logger.log(LOG_FILE_PATH2, "walking Right");
				//sendKey(VK_RIGHT, 0, timeToWalk);
				turnRight(window, process, timeToWalk);

			}
			else {
				logger.log(LOG_FILE_PATH2, "walking Left");
				//sendKey(VK_LEFT, 0, timeToWalk);
				turnLeft(window, process, timeToWalk);
			}
		}
		//get new x distance and positioning of player and monster
		issuccedded = ReadProcessMemory(process, reinterpret_cast<LPCVOID>(playerPositionAddresses[X]), &playerX, sizeof(unsigned int), 0);
		issuccedded = ReadProcessMemory(process, reinterpret_cast<LPCVOID>(playerPositionAddresses[Y]), &playerY, sizeof(unsigned int), 0);
		playerY = playerY + PLAYER_Y_OFFSET;
		logger.log(LOG_FILE_PATH2, "playerX: " + std::to_string(playerX) + "playerY: " + std::to_string(playerY));
		logger.log(LOG_FILE_PATH2, "squareX: " + std::to_string(squareX) + "squareY: " + std::to_string(squareY));
		distanceX = abs(squareX - playerX);
		logger.log(LOG_FILE_PATH2, "distance: " + std::to_string(distanceX));
	}

	//handle Y positioning
	issuccedded = ReadProcessMemory(process, reinterpret_cast<LPCVOID>(playerPositionAddresses[X]), &playerX, sizeof(unsigned int), 0);
	issuccedded = ReadProcessMemory(process, reinterpret_cast<LPCVOID>(playerPositionAddresses[Y]), &playerY, sizeof(unsigned int), 0);
	playerY = playerY + PLAYER_Y_OFFSET;
	float distanceY = abs(playerY - squareY);
	logger.log(LOG_FILE_PATH2, "distanceY: " + std::to_string(distanceY));
	while (distanceY > ATTACK_HEIGHT) {
		//if (distanceY > TILE_UP_DISTANCE) {//must be tile up  or down from player
			if (playerY > squareY) {
				dashUp(window, process);
			}
			else {
				fallDown(window, process);
			}
		//}
		issuccedded = ReadProcessMemory(process, reinterpret_cast<LPCVOID>(playerPositionAddresses[X]), &playerX, sizeof(unsigned int), 0);
		issuccedded = ReadProcessMemory(process, reinterpret_cast<LPCVOID>(playerPositionAddresses[Y]), &playerY, sizeof(unsigned int), 0);
		playerY = playerY + PLAYER_Y_OFFSET;
		distanceY = abs(playerY - squareY);
		logger.log(LOG_FILE_PATH2, "distanceY: " + std::to_string(distanceY));
	}
}

void attackMonster(HWND window, HANDLE process) {
	logger.log(LOG_FILE_PATH2, "attacking monster");
	sendKey('A', 0, 0.5);
}



//Point<uintptr_t, 2> getPlayerPosition(HANDLE process, uintptr_t playerPositionBaseAddress) {
//	Point<uintptr_t, 2> playerPosition;
//	playerPosition[X] = FindDMAAddy(process, playerPositionBaseAddress, MAPLESTORY_PLAYER_POSITION_X_OFFSETS);
//	playerPosition[Y] = FindDMAAddy(process, playerPositionBaseAddress, MAPLESTORY_PLAYER_POSITION_Y_OFFSETS);
//	signed int playerX = 0;
//	signed int playerY = 0;
//	DWORD issuccedded = ReadProcessMemory(process, reinterpret_cast<LPCVOID>(playerPosition[X]), &playerX, sizeof(unsigned int), 0);
//	issuccedded = ReadProcessMemory(process, reinterpret_cast<LPCVOID>(playerPosition[Y]), &playerY, sizeof(unsigned int), 0);
//	logger.log(LOG_FILE_PATH2,"player position X: " + std::to_string(playerX));
//	logger.log(LOG_FILE_PATH2,"player position Y: " + std::to_string(playerY));
//	return playerPosition;
//}




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



MapleSuperBot::MapleSuperBot() {
	this->memoryManipulation = MemoryAccess();
	this->monstersPositionsRemovedOpcodes = { 0x58, 0x24 , 0x44, 0x8B, 0x48, 0x00,0x00,0x00,0x90,0x86,0x89,0x48};
	//this->monstersPositionsRemovedOpcodes = { 0x90, 0x90 , 0x90 , 0x90, 0x90,0x90,0x90 };
	//get Maplestory Window Process Id and store it in PID
	this->PID = this->memoryManipulation.getGamePID(MAPLESTORY_HANDLE_NAME);
	this->process = OpenProcess(
		PROCESS_VM_OPERATION | PROCESS_VM_READ | PROCESS_VM_WRITE,
		FALSE,
		PID
	);
	this->isHookOn = false;
	this->dynamicPtrBaseAddr = GetModuleBaseAddress(PID, MAPLESTORY_MOD_NAME);
	this->numberOfMonsters = 22;//getNumberOfMonsters(this->process, this->dynamicPtrBaseAddr); //TODO: edit this
}


int MapleSuperBot::initializePlayerPosition() {
	HMODULE hModule = GetModuleHandleA(PLAYER_POSITION_BASE_MODULE);
	if (hModule == NULL)
		return 0;
	uintptr_t playerBaseAddress = reinterpret_cast<uintptr_t>(hModule);
	this->playerPosition[X] = FindDMAAddy(process, playerBaseAddress, MAPLESTORY_PLAYER_POSITION_X_OFFSETS);
	this->playerPosition[Y] = this->playerPosition[X] + 0x04;
	return 1;
}

void MapleSuperBot::removeMonstersFromAddressesVector(int biggestSquareIndex) {
	std::vector<Point<uintptr_t, 2>>::iterator it = this->monstersPositionsAddressesVector.begin();
	while (it != this->monstersPositionsAddressesVector.end()) {
		if (std::count(this->monstersSquares[biggestSquareIndex].begin(), this->monstersSquares[biggestSquareIndex].end(), *it)){
			try {
				it = this->monstersPositionsAddressesVector.erase(it);
			}
			catch (...) {
				logger.log(LOG_FILE_PATH2, "Exception: couldn't erase monster - does not exist in monsterPositionsAddressesVector");
			}
		}
		else {
			++it;  // Move to the next element if not erasing
		}
	}
	this->monstersSquares[biggestSquareIndex].clear();
	this->squaresMonsterCounterVector[biggestSquareIndex] = 0;
}


int MapleSuperBot::executeAttack() {
	//printMonstersSquares();
	bool isAttackExecuted = false;
	int biggestSquareIndex = getBiggestSquareIndex(this->squaresMonsterCounterVector);
	logger.log(LOG_FILE_PATH2, "biggest Square Index: " + std::to_string(biggestSquareIndex));
	if (biggestSquareIndex == -1) {
		logger.log(LOG_FILE_PATH2, "failed getting biggest square index");
		return 0;
	}
	HWND window = FindWindowA(NULL, MAPLESTORY_HANDLE_NAME);
	if (IsWindow(window)){
		signed int biggestSquareX = (this->monstersSquaresXRanges[biggestSquareIndex][X] + this->monstersSquaresXRanges[biggestSquareIndex][Y]) / 2;
		signed int biggestSquareY = (this->monstersSquaresYRanges[biggestSquareIndex][X] + this->monstersSquaresYRanges[biggestSquareIndex][Y]) / 2;
		if (!isSquareExists(window, process, biggestSquareX, biggestSquareY)) {
			this->removeMonstersFromAddressesVector(biggestSquareIndex);
			isAttackExecuted = true;
		}
		logger.log(LOG_FILE_PATH2, "currentSquare going to Attack: " + biggestSquareIndex);
		printMonstersSquare(biggestSquareIndex);
		while (!isAttackExecuted){
			if (isSquareInRange(this->process, this->playerPosition, biggestSquareX, biggestSquareY, ATTACK_WIDTH, ATTACK_HEIGHT))
			{
				//sendKey(VK_LEFT, 0, 0.15);
				turnLeft(window, this->process, 0.15);
				attackMonster(window, this->process);
				//sendKey(VK_RIGHT, 0, 0.15);
				turnRight(window, this->process, 0.15);
				attackMonster(window, this->process);
				this->removeMonstersFromAddressesVector(biggestSquareIndex);
				isAttackExecuted = true;
			}
			else {
				goToMonster(window, this->process, this->playerPosition, biggestSquareX, biggestSquareY);
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
	this->monstersSquaresXRanges.clear();
	this->monstersSquaresYRanges.clear();
	logger.log(LOG_FILE_PATH2, "initializing squares");

	for (int i = 0; i < this->monstersPositionsAddressesVector.size(); i++) {
		//define Square
		uintptr_t monsterXAddress = this->monstersPositionsAddressesVector[i][X];
		signed int monsterX = -1;
		DWORD issucceddedX = ReadProcessMemory(process, reinterpret_cast<LPCVOID>(monsterXAddress), &monsterX, sizeof(signed int), 0);
		uintptr_t monsterYAddress = this->monstersPositionsAddressesVector[i][Y];
		signed int monsterY = -1;
		DWORD issucceddedY = ReadProcessMemory(process, reinterpret_cast<LPCVOID>(monsterYAddress), &monsterY, sizeof(signed int), 0);
		if (issucceddedX && issucceddedY) {
			int maxX = monsterX + 150;
			int minX = monsterX - 150;
			int maxY = monsterY + 100;
			int minY = monsterY - 100;
			logger.log(LOG_FILE_PATH2, "Square " + std::to_string(i) + " max X: " + std::to_string(maxX));
			logger.log(LOG_FILE_PATH2, "Square " + std::to_string(i) + " min X: " + std::to_string(minX));
			Point<uintptr_t, 2> currentXRange;
			Point<uintptr_t, 2> currentYRange;
			currentXRange[X] = minX;
			currentXRange[Y] = maxX;
			currentYRange[X] = minY;
			currentYRange[Y] = maxY;
			this->monstersSquaresXRanges.push_back(currentXRange);
			this->monstersSquaresYRanges.push_back(currentYRange);

			std::vector<Point<uintptr_t, 2>> tmpVector;
			this->monstersSquares.push_back(tmpVector);
			this->squaresMonsterCounterVector.push_back(0);
			for (int j = 0; j < this->monstersPositionsAddressesVector.size(); j++) {
				//check if monster inside square
				//if so enter it to square array
				uintptr_t monsterXAddress = this->monstersPositionsAddressesVector[j][X];
				signed int monsterX = -1;
				DWORD issucceddedX = ReadProcessMemory(process, reinterpret_cast<LPCVOID>(monsterXAddress), &monsterX, sizeof(signed int), 0);
				uintptr_t monsterYAddress = this->monstersPositionsAddressesVector[j][Y];
				signed int monsterY = -1;
				DWORD issucceddedY = ReadProcessMemory(process, reinterpret_cast<LPCVOID>(monsterYAddress), &monsterY, sizeof(signed int), 0);
				if (issucceddedX && issucceddedY) {
					if (monsterX >= minX && monsterX <= maxX &&   
						monsterY >= minY && monsterY <= maxY) {
						this->monstersSquares[i].push_back(this->monstersPositionsAddressesVector[j]);
						this->squaresMonsterCounterVector[i] += 1 ;
					}
				}
			}
		}
	}
}


uintptr_t MapleSuperBot::enableHook(uintptr_t hookAt, uintptr_t newFunc, int size)
{
	if (size > 16)
		return 0;
	logger.log(LOG_FILE_PATH2, "Trampoline address: " + std::to_string(newFunc));
	auto oldProtection = memoryManipulation.protectMemory<DWORD[3]>(this->process, hookAt + 1, PAGE_EXECUTE_READWRITE);
	BYTE Nop[] = { 0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90 };
	int success = WriteProcessMemory(this->process, (LPVOID*)(uintptr_t)hookAt, &Nop, sizeof(Nop), NULL);

	BYTE movToRDI[] = { 0x48,0xBF};
	BYTE JmpRDI[] = { 0xFF, 0xE7 };
	success = WriteProcessMemory(this->process, (LPVOID*)((uintptr_t)hookAt), &movToRDI, sizeof(movToRDI), NULL);
	success = WriteProcessMemory(this->process, (LPVOID*)((uintptr_t)hookAt + 2), &newFunc, sizeof(uintptr_t), NULL);
	success = WriteProcessMemory(this->process, (LPVOID*)((uintptr_t)hookAt + 10), &JmpRDI, sizeof(JmpRDI), NULL);
	if (success)
		logger.log(LOG_FILE_PATH2, "enabled hook successfully");
	for (unsigned int i = 12; i < size; i++)
		memoryManipulation.writeMemory<BYTE>(this->process, hookAt + i, 0x90);
	memoryManipulation.protectMemory<DWORD[3]>(this->process, hookAt + 1, oldProtection);
	return hookAt + 12; 
}



bool MapleSuperBot::isMonstersPositionsAddressesVectorFull() {
	if (this->monstersPositionsAddressesVector.size() < this->numberOfMonsters)
	{ 
		//logger.log(LOG_FILE_PATH2, "wtf" );
		return false;
	}
	//logger.log(LOG_FILE_PATH2, "returned true");
	return true;
}

bool MapleSuperBot::isMonsterInAddressesVector(Point<uintptr_t, 2> newMonsterPositionAddress) {
	for (unsigned int i = 0; i < this->monstersPositionsAddressesVector.size(); i++) {

		if (this->monstersPositionsAddressesVector[i] == newMonsterPositionAddress)
			return true;
	}
	return false;

}

std::vector<Point<uintptr_t, 2>> MapleSuperBot::getMonstersPositionsAddressesVector() {
	return this->monstersPositionsAddressesVector;
}

void MapleSuperBot::printMonstersPositions() {
	logger.log(LOG_FILE_PATH2, "");
	logger.log(LOG_FILE_PATH2, "Monsters Positions:");
	for (unsigned int i = 0; i < this->monstersPositionsAddressesVector.size(); i++) {
		uintptr_t monsterXAddress = this->monstersPositionsAddressesVector[i][X];
		signed int monsterX = -1;
		DWORD issuccedded = ReadProcessMemory(process, reinterpret_cast<LPCVOID>(monsterXAddress), &monsterX, sizeof(signed int), 0);
		uintptr_t monsterYAddress = this->monstersPositionsAddressesVector[i][Y];
		signed int monsterY = -1;
		issuccedded = ReadProcessMemory(process, reinterpret_cast<LPCVOID>(monsterYAddress), &monsterY, sizeof(signed int), 0);
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
			uintptr_t monsterXAddress = this->monstersSquares[i][j][X];
			signed int monsterX = -1;
			DWORD issuccedded = ReadProcessMemory(process, reinterpret_cast<LPCVOID>(monsterXAddress), &monsterX, sizeof(signed int), 0);
			uintptr_t monsterYAddress = this->monstersSquares[i][j][Y];
			signed int monsterY = -1;
			issuccedded = ReadProcessMemory(process, reinterpret_cast<LPCVOID>(monsterYAddress), &monsterY, sizeof(signed int), 0);
			logger.log(LOG_FILE_PATH2, "monster " + std::to_string(j) + ":");
			logger.log(LOG_FILE_PATH2, "X coordinate value: " + std::to_string(monsterX));
			logger.log(LOG_FILE_PATH2, "Y coordinate value: " + std::to_string(monsterY));
		}
	}
}

void MapleSuperBot::printMonstersSquare(int squareIndex) {

	logger.log(LOG_FILE_PATH2, "-------------PrintMonstersSquare OUTPUT for Square: ------------" + std::to_string(squareIndex));
	for (unsigned int j = 0; j < this->monstersSquares[squareIndex].size(); j++) {
		uintptr_t monsterXAddress = this->monstersSquares[squareIndex][j][X];
		signed int monsterX = -1;
		DWORD issuccedded = ReadProcessMemory(process, reinterpret_cast<LPCVOID>(monsterXAddress), &monsterX, sizeof(signed int), 0);
		uintptr_t monsterYAddress = this->monstersSquares[squareIndex][j][Y];
		signed int monsterY = -1;
		issuccedded = ReadProcessMemory(process, reinterpret_cast<LPCVOID>(monsterYAddress), &monsterY, sizeof(signed int), 0);
		logger.log(LOG_FILE_PATH2, "monster " + std::to_string(j) + ":");
		logger.log(LOG_FILE_PATH2, "X coordinate value: " + std::to_string(monsterX));
		logger.log(LOG_FILE_PATH2, "Y coordinate value: " + std::to_string(monsterY));
	}
}

Point<uintptr_t, 2> MapleSuperBot::logPlayerPosition() {
	Point<uintptr_t, 2> playerPosition;
	signed int playerX = 0;
	signed int playerY = 0;
	DWORD issuccedded = ReadProcessMemory(this->process, reinterpret_cast<LPCVOID>(this->playerPosition[X]), &playerX, sizeof(unsigned int), 0);
	issuccedded = ReadProcessMemory(this->process, reinterpret_cast<LPCVOID>(this->playerPosition[Y]), &playerY, sizeof(unsigned int), 0);
	playerY = playerY + PLAYER_Y_OFFSET;
	return playerPosition;
}


void MapleSuperBot::addToMonstersPositionsAddressesVector(Point<uintptr_t, 2> newMonsterPosition) {
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
