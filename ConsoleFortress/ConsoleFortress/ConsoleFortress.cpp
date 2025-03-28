#include <windows.h>
#include <conio.h>
#include <math.h>
#include <random>
#include "KeyCodes.h"
#include "Color.h"
#include "Tank.h"
#include "Numbers.h"
#include "UI.h"
#include "MainMenu.h"
#include <mmsystem.h>
#pragma comment(lib,"winmm.lib")




#define X_PIXELS 400
#define Y_PIXELS 200

// ===========================================================
// Added: Asynchronous Input Handling
// ===========================================================
enum KeyState {
	KEY_NONE,       // not pressed last frame nor this frame
	KEY_PRESSED,    // pressed this frame (wasn't pressed last frame)
	KEY_HELD,       // held down (pressed last frame & this frame)
	KEY_RELEASED    // released this frame (was pressed last frame, not pressed now)
};

static KeyState g_keyStates[256];
static bool g_prevKeyDown[256];
static bool g_currKeyDown[256];

// Updates the key states each frame
void UpdateInput()
{
	// 1) Move current down-states to 'previous'
	for (int i = 0; i < 256; i++) {
		g_prevKeyDown[i] = g_currKeyDown[i];
	}

	// 2) Read new states
	for (int i = 0; i < 256; i++) {
		SHORT state = GetAsyncKeyState(i);
		g_currKeyDown[i] = (state & 0x8000) != 0; // highest bit indicates down
	}

	// 3) Determine KeyState based on old vs. new
	for (int i = 0; i < 256; i++) {
		if (!g_prevKeyDown[i] && !g_currKeyDown[i]) {
			g_keyStates[i] = KEY_NONE;
		}
		else if (!g_prevKeyDown[i] && g_currKeyDown[i]) {
			g_keyStates[i] = KEY_PRESSED;
		}
		else if (g_prevKeyDown[i] && g_currKeyDown[i]) {
			g_keyStates[i] = KEY_HELD;
		}
		else if (g_prevKeyDown[i] && !g_currKeyDown[i]) {
			g_keyStates[i] = KEY_RELEASED;
		}
	}
}

// Check if a key is held down (pressed or held)
bool IsKeyDown(int vkCode)
{
	return (g_keyStates[vkCode] == KEY_PRESSED || g_keyStates[vkCode] == KEY_HELD);
}

// Check if a key was pressed this frame (like Unity's GetKeyDown)
bool IsKeyPressed(int vkCode)
{
	return (g_keyStates[vkCode] == KEY_PRESSED);
}

// Check if a key was released this frame
bool IsKeyReleased(int vkCode)
{
	return (g_keyStates[vkCode] == KEY_RELEASED);
}
// ===========================================================


// Panel Rendering


struct Picture {
	wchar_t unicode[Y_PIXELS][X_PIXELS];
	WORD color[Y_PIXELS][X_PIXELS];
};

Picture MainScreen;


HANDLE g_hScreen[2];
int screen_cur = 0;

bool IS_FLOOR[4][Y_PIXELS];



// Function Prototypes

int Getkey(void);								
void PrintPos(int x, int y, const char* s);	
void CursorView(HANDLE hConsole, char show);
void ScreenInit();
void ScreenStart();								
void ScreenEnd();								
void ScreenClear(HANDLE hConsole);				
void DrawUnicodeFast(HANDLE hConsole, wchar_t picture[Y_PIXELS][X_PIXELS], WORD colors[Y_PIXELS][X_PIXELS]);
void DrawScreen(void);
void DrawToMainScreen(int x, int y, const wchar_t* s);
void DrawToMainScreen(int x, int y, const wchar_t* s, WORD color);
void ClearPictureCell(int x, int y);
void initScreen(Picture& screen);
void DrawMultilineToMainScreen(int x, int y, const wchar_t* s);
void DrawMultilineToMainScreen(int x, int y, const wchar_t* s, WORD color);
void RenderStatusPanel(int x, int y, int player);
void PlayerInit();
void DrawTankCamera(int player);
void HandleMainGamePlayerInput(int player);
int ballistics(int player);
void PrintFloor();
bool isEnemyHit(double bulletHor, double bulletVer);
void AdjustCameraLocation(int player);
void MainMenu();
void GameOverScreen();
void SoundCannon();
void SoundExplosion();
void SoundTankIdle();
void SoundCharging();
void SoundNULL();

// GameManager Variables

const double DEFAULTENERGY = 100;
const int DEFAULTMOVE = 300;
const double MAXARTILLARYANGLE = 75;
const double MAXARTILLARYPOWER = 2.5;
const double MAXARTILLARYWIND = 0.02;

struct Camera
{
	int x = 0;
	int y = 0;
};

Camera CAMERA;
Camera CAMERA2;

struct Player {
	double xAxis;
	double yAxis;
	int energy = DEFAULTENERGY;
	int move = DEFAULTMOVE;
	double artillaryPower = 0;
	double artillaryAngle = 30;
	double tankRotation = 0;
	int ammoType = 0;
};
Player PLAYER[2];

const int PLAYER1 = 0;
const int PLAYER2 = 1;

// 이거 바꾸면 위아래 기본값 조절가능

const int P1_OFFSET_X = 50;
const int P1_OFFSET_Y = 30;
const int P2_OFFSET_X = 180;
const int P2_OFFSET_Y = 30;

enum GamePhase {
	MAIN_MENU,
	SHOW_PLAYER,
	PLAYER1_MOVING,
	PLAYER1_ANGLE,
	PLAYER1_SHOOTING,
	WAIT_PLAYER1_PROJECTILE,
	PLAYER2_MOVING,
	PLAYER2_ANGLE,
	PLAYER2_SHOOTING,
	WAIT_PLAYER2_PROJECTILE,
	GAME_OVER
};

GamePhase currentPhase = MAIN_MENU;

// 포탄 관련

double gravity = -0.02;
double wind = -0.01;
bool isCharged = false;
double bulletHor = 0;
double bulletVer = 0;
double bulletTimer = 0;
double PI = 3.14159265358979323846;
double bulletCam = 0;

// 턴 관련

bool winner = true;
int turn = 0;


static int showPlayerSubPhase = 0;
static const int waitDuration = 2000;

static const ULONGLONG subPhaseDuration = 1000; // 1 second in ms
static ULONGLONG subPhaseStartTime = 0;

static int startX = 0, startY = 0;
static int targetX = 0, targetY = 0;

int main(void)
{
	CursorView(GetStdHandle(STD_OUTPUT_HANDLE), false);
	ScreenInit();
	PlayerInit();
	DrawScreen();

	// Main game loop with switch-case for phases
	while (true)
	{

		if (GetAsyncKeyState('P') & 0x8000 && turn % 2 == 0) {
			// Reinitialize non-constant variables for the player phase
			showPlayerSubPhase = 0;
			subPhaseStartTime = 0;
			startX = 0;
			startY = 0;
			targetX = 0;
			targetY = 0;
			turn = 0;
			CAMERA.x = 0;
			CAMERA.y = 0;

			currentPhase = MAIN_MENU;
			PlayerInit();
		}

		switch (currentPhase)
		{
		case MAIN_MENU:
			MainMenu();
			
			break;

		case SHOW_PLAYER:
		{
			PlayerInit();
			

			// Get current time in ms
			ULONGLONG now = GetTickCount64();



			switch (showPlayerSubPhase)
			{
			case 0:
				// Immediately position the camera on Player1 with offsets
				CAMERA.x = PLAYER[PLAYER1].xAxis - P1_OFFSET_X;
				CAMERA.y = PLAYER[PLAYER1].yAxis - P1_OFFSET_Y;

				// Initialize the timer on the first run
				if (subPhaseStartTime == 0)
					subPhaseStartTime = now;

				if (now - subPhaseStartTime >= waitDuration)
				{
					startX = CAMERA.x;
					startY = CAMERA.y;
					targetX = PLAYER[PLAYER2].xAxis - P2_OFFSET_X + 50;
					targetY = PLAYER[PLAYER2].yAxis - P2_OFFSET_Y;

					// Reset the timer and proceed to the next sub-phase
					subPhaseStartTime = now;
					showPlayerSubPhase++;
				}
				break;

			case 1:
			{
				ULONGLONG elapsed = now - subPhaseStartTime;
				float t = (float)elapsed / (float)subPhaseDuration; 

				if (t >= 1.0f)
				{
					t = 1.0f;

					showPlayerSubPhase++;

					startX = targetX;
					startY = targetY;

					targetX = PLAYER[PLAYER1].xAxis - P1_OFFSET_X;
					targetY = PLAYER[PLAYER1].yAxis - P1_OFFSET_Y;

					// Reset timer
					subPhaseStartTime = now;
				}

				// Linear interpolation for camera
				CAMERA.x = (int)(startX + t * (targetX - startX));
				CAMERA.y = (int)(startY + t * (targetY - startY));
			}
			break;

			case 2:
			{
				ULONGLONG elapsed = now - subPhaseStartTime;
				// Wait for 2 seconds at Player2 before starting transition to Player1
				if (elapsed < waitDuration)
				{
					// Keep camera at Player2 position during the wait
					CAMERA.x = startX;
					CAMERA.y = startY;
				}
				else
				{
					// Adjust elapsed time by subtracting the wait duration
					elapsed -= waitDuration;
					float t = (float)elapsed / (float)subPhaseDuration;
					if (t >= 1.0f)
					{
						t = 1.0f;
						showPlayerSubPhase++;
					}
					// Linear interpolation for camera position (from Player2 to Player1)
					CAMERA.x = (int)(startX + t * (targetX - startX));
					CAMERA.y = (int)(startY + t * (targetY - startY));
				}
			}
			break;

			case 3:
				// Move on to the next phase
				currentPhase = PLAYER1_MOVING;
				RenderStatusPanel(0, 44, PLAYER1);
				break;
			}

			// Draw both players with camera offset
			DrawTankCamera(PLAYER1);
			DrawTankCamera(PLAYER2);
			PrintFloor();

			// Since there's no Sleep(), the camera moves in real time,
			// completing in ~1 second for each segment (based on subPhaseDuration).
		}
		break;

		case PLAYER1_MOVING:
			// TODO: fill in player1 moving logic
			// Example usage:
			if (turn % 2)
			{
				if (PLAYER[1].energy <= 0)
				{
					winner = true;
					currentPhase = GAME_OVER;
				}
				HandleMainGamePlayerInput(PLAYER2);
				RenderStatusPanel(0, 44, PLAYER2);
			}
			else
			{
				if (PLAYER[0].energy <= 0)
				{
					winner = false;
					currentPhase = GAME_OVER;
				}
				HandleMainGamePlayerInput(PLAYER1);
				RenderStatusPanel(0, 44, PLAYER1);
			}
			AdjustCameraLocation(turn % 2);
			DrawTankCamera(PLAYER1);
			DrawTankCamera(PLAYER2);
			PrintFloor();
			break;

		case PLAYER1_ANGLE:
			// TODO: fill in adjusting angle logic
			break;

		case PLAYER1_SHOOTING:
			// TODO: fill in shooting logic
			break;

		case WAIT_PLAYER1_PROJECTILE:
			// TODO: fill in waiting for projectile logic
			break;

		case PLAYER2_MOVING:
			// TODO: fill in player2 turn logic
			break;

		case PLAYER2_ANGLE:
			break;

		case PLAYER2_SHOOTING:
			break;

		case WAIT_PLAYER2_PROJECTILE:
			// TODO: fill in waiting for player2 projectile logic
			break;

		case GAME_OVER:
			GameOverScreen();
			break;
		}

		// Render after each phase logic
		DrawScreen();
	}
	return 0;
}

void DrawTankCamera(int player)
{
	// We offset the position by CAMERA.x and CAMERA.y
	int drawX = PLAYER[player].xAxis - CAMERA.x - (bulletHor * bulletCam) - CAMERA2.x;
	int drawY = PLAYER[player].yAxis - CAMERA.y - (bulletVer * bulletCam);

	// Safety check to avoid drawing out of bounds
	if (drawX < 0 || drawX >= X_PIXELS || drawY < 0 || drawY >= Y_PIXELS) {
		return;
	}

	// For example, we use the angle to choose an index in 'tankUnicodeArt'
	int tankIndex = int(PLAYER[player].artillaryAngle / 15);

	// Make sure the index doesn't go out of bounds for your tankUnicodeArt array
	// (In your project, check the valid range. We'll assume it is safe for demonstration.)

	if (player == 0) DrawMultilineToMainScreen(drawX, drawY, tankUnicodeArt[tankIndex + int(6 * PLAYER[player].tankRotation)], MAGENTA);
	if (player == 1) DrawMultilineToMainScreen(drawX, drawY, tankUnicodeArt[tankIndex + int(6 * PLAYER[player].tankRotation)], GREEN);
}


void PlayerInit() {
	PLAYER[0].xAxis = 80;
	PLAYER[0].yAxis = 50;

	PLAYER[1].xAxis = 300;
	PLAYER[1].yAxis = 50;
	PLAYER[1].tankRotation = 1; 

	PLAYER[0].energy = DEFAULTENERGY;
	PLAYER[0].move = DEFAULTMOVE;
	PLAYER[0].artillaryPower = 0;
	PLAYER[0].artillaryAngle = 30;
	PLAYER[0].tankRotation = 0;
	PLAYER[0].ammoType = 0;

	PLAYER[1].energy = DEFAULTENERGY;
	PLAYER[1].move = DEFAULTMOVE;
	PLAYER[1].artillaryPower = 0;
	PLAYER[1].artillaryAngle = 30;
	PLAYER[1].ammoType = 0;
}


void HandleMainGamePlayerInput(int player) {

	int leftRange;
	int rightRange;

	if (player == 0) {
		leftRange = 30;
		rightRange = 150;
	}
	else {
		leftRange = 100;
		rightRange = 215;
	}

	if (GetAsyncKeyState(VK_LEFT) & 0x8000 && PLAYER[player].move > 0 && PLAYER[player].xAxis - CAMERA2.x > leftRange) {
		if (PLAYER[player].xAxis > 0) PLAYER[player].xAxis -= 0.2;
		if (PLAYER[player].move > 0) {
			PLAYER[player].move--;
			PLAYER[player].tankRotation = 1;
			SoundTankIdle();
		}
	}

	if (GetAsyncKeyState(VK_RIGHT) & 0x8000 && PLAYER[player].move > 0 && PLAYER[player].xAxis - CAMERA2.x < rightRange) {
		if (PLAYER[player].xAxis > 0) PLAYER[player].xAxis += 0.2;
		if (PLAYER[player].move > 0) {
			PLAYER[player].move--;
			PLAYER[player].tankRotation = 0;
			SoundTankIdle();
		}
	}

	// 탄 변경
	if (GetAsyncKeyState(0x31) & 0x8000)
	{
		PLAYER[player].ammoType = 0;
	}
	if (GetAsyncKeyState(0x32) & 0x8000)
	{
		PLAYER[player].ammoType = 1;
	}
	if (GetAsyncKeyState(0x33) & 0x8000)
	{
		PLAYER[player].ammoType = 2;
	}
	// 각도 조절
	if (GetAsyncKeyState(VK_UP) & 0x8000 && PLAYER[player].artillaryAngle < 75)
	{
		PLAYER[player].artillaryAngle += 0.15;
	}
	if (GetAsyncKeyState(VK_DOWN) & 0x8000 && PLAYER[player].artillaryAngle > 21)
	{
		PLAYER[player].artillaryAngle -= 0.15;
	}
	// 발사 준비
	if (GetAsyncKeyState(VK_SPACE) & 0x8000)
	{
		if (PLAYER[player].artillaryPower < MAXARTILLARYPOWER)
		{
			PLAYER[player].artillaryPower += 0.002;
			SoundCharging();
			isCharged = true;
		}
	}
	// 발사
	else if (isCharged)
	{
		SoundCannon();
		isCharged = false;
		ballistics(player);
		PLAYER[player].artillaryPower = 0;
		bulletTimer = 0;
		// 턴 바꾸는 코드
		PLAYER[player].move = DEFAULTMOVE;
		std::random_device rd;
		std::mt19937 gen(rd());
		std::uniform_real_distribution<double> dist(-MAXARTILLARYWIND, MAXARTILLARYWIND);
		wind = dist(gen);
		turn++;
	}
}

// 탄도
static int ballistics(int player)
{
	bulletCam = 5;
	bulletHor = 0;
	bulletVer = 0;
	if (PLAYER[player].ammoType == 0)
	{
		while (bulletVer < 22 - CAMERA.y - (bulletVer * bulletCam))
		{
			bulletTimer += 0.1;
			bulletHor = PLAYER[player].artillaryPower * (bulletTimer * cos(((PLAYER[player].tankRotation * 180) - PLAYER[player].artillaryAngle) * (PI / 180))) + ((wind * pow(bulletTimer, 2)) / 2);
			bulletVer = PLAYER[player].artillaryPower * (bulletTimer * sin((-PLAYER[player].artillaryAngle * (PI / 180)))) - ((gravity * pow(bulletTimer, 2)) / 2);
			DrawTankCamera(PLAYER1);
			DrawTankCamera(PLAYER2);
			if (PLAYER[player].tankRotation)
			{
				DrawMultilineToMainScreen(bulletHor + PLAYER[player].xAxis - 25 - CAMERA2.x, bulletVer + PLAYER[player].yAxis - 17, L"◀■■<", 0x0008);
			}
			else
			{
				DrawMultilineToMainScreen(bulletHor + PLAYER[player].xAxis - 25 - CAMERA2.x, bulletVer + PLAYER[player].yAxis - 17, L">■■▶", 0x0008);
			}
			if (isEnemyHit(bulletHor, bulletVer))
			{
				if (turn % 2)
				{
					PLAYER[0].energy -= 25;
				}
				else
				{
					PLAYER[1].energy -= 25;
				}
				break;
			}
			PrintFloor();
			DrawScreen();
		}
		PrintFloor();
		DrawTankCamera(PLAYER1);
		DrawTankCamera(PLAYER2);
		SoundExplosion();
		DrawMultilineToMainScreen(bulletHor + PLAYER[player].xAxis - 25 - CAMERA2.x, bulletVer + PLAYER[player].yAxis - 19, L"   ▲▲▲", RED);
		DrawMultilineToMainScreen(bulletHor + PLAYER[player].xAxis - 25 - CAMERA2.x, bulletVer + PLAYER[player].yAxis - 18, L" ◀█████▶", RED);
		DrawMultilineToMainScreen(bulletHor + PLAYER[player].xAxis - 25 - CAMERA2.x, bulletVer + PLAYER[player].yAxis - 17, L"◀███████▶", RED);
		DrawMultilineToMainScreen(bulletHor + PLAYER[player].xAxis - 25 - CAMERA2.x, bulletVer + PLAYER[player].yAxis - 16, L" ◀█████▶", RED);
		DrawMultilineToMainScreen(bulletHor + PLAYER[player].xAxis - 25 - CAMERA2.x, bulletVer + PLAYER[player].yAxis - 15, L"   ▼▼▼", RED);
	}
	else if (PLAYER[player].ammoType == 1)
	{
		bool bounce = true;
		double reverseBulletVer = 0;
		PLAYER[player].artillaryPower = PLAYER[player].artillaryPower * 0.5;
		while (bulletVer < 22 - CAMERA.y - (bulletVer * bulletCam) || bounce)
		{
			bulletTimer += 0.1;
			bulletHor = PLAYER[player].artillaryPower * (bulletTimer * cos(((PLAYER[player].tankRotation * 180) - (PLAYER[player].artillaryAngle - 15)) * (PI / 180))) + ((wind * pow(bulletTimer, 2)) / 2);
			if (bounce)
			{
				bulletVer = (PLAYER[player].artillaryPower * (bulletTimer * sin((-(PLAYER[player].artillaryAngle - 15) * (PI / 180))))) - ((gravity * pow(bulletTimer, 2)) / 2);
			}
			else
			{
				bulletVer = (PLAYER[player].artillaryPower * (bulletTimer * sin((-(PLAYER[player].artillaryAngle - 15) * (PI / 180))))) - ((gravity * pow(bulletTimer - reverseBulletVer, 2)) / 2);
			}
			if (bulletVer > 21 - CAMERA.y - (bulletVer * bulletCam) && bounce)
			{
				bounce = false;
				reverseBulletVer = bulletTimer * 1.99999;
			}
			DrawTankCamera(PLAYER1);
			DrawTankCamera(PLAYER2);
			if (isEnemyHit(bulletHor, bulletVer))
			{
				if (turn % 2)
				{
					PLAYER[0].energy -= 20;
				}
				else
				{
					PLAYER[1].energy -= 20;
				}
				break;
			}
			DrawMultilineToMainScreen(bulletHor + PLAYER[player].xAxis - 20 - CAMERA2.x, bulletVer + PLAYER[player].yAxis - 18, L"◢■█■◣", BLUE);
			DrawMultilineToMainScreen(bulletHor + PLAYER[player].xAxis - 20 - CAMERA2.x, bulletVer + PLAYER[player].yAxis - 17, L"◥■█■◤", BLUE);
			PrintFloor();
			DrawScreen();
		}
		PrintFloor();
		DrawTankCamera(PLAYER1);
		DrawTankCamera(PLAYER2);
		SoundExplosion();
		DrawMultilineToMainScreen(bulletHor + PLAYER[player].xAxis - 25 - CAMERA2.x, bulletVer + PLAYER[player].yAxis - 19, L"   ▲▲▲", CYAN);
		DrawMultilineToMainScreen(bulletHor + PLAYER[player].xAxis - 25 - CAMERA2.x, bulletVer + PLAYER[player].yAxis - 18, L" ◀█████▶", CYAN);
		DrawMultilineToMainScreen(bulletHor + PLAYER[player].xAxis - 25 - CAMERA2.x, bulletVer + PLAYER[player].yAxis - 17, L"◀███████▶", CYAN);
		DrawMultilineToMainScreen(bulletHor + PLAYER[player].xAxis - 25 - CAMERA2.x, bulletVer + PLAYER[player].yAxis - 16, L" ◀█████▶", CYAN);
		DrawMultilineToMainScreen(bulletHor + PLAYER[player].xAxis - 25 - CAMERA2.x, bulletVer + PLAYER[player].yAxis - 15, L"   ▼▼▼", CYAN);
	}
	else if (PLAYER[player].ammoType == 2)
	{
		PLAYER[player].artillaryPower = PLAYER[player].artillaryPower * 0.8;
		while (bulletVer < 22 - CAMERA.y - (bulletVer * bulletCam))
		{
			bulletTimer += 0.1;
			bulletHor = PLAYER[player].artillaryPower * (bulletTimer * cos(((PLAYER[player].tankRotation * 180) - PLAYER[player].artillaryAngle) * (PI / 180))) + (wind * pow(bulletTimer, 2));
			bulletVer = (PLAYER[player].artillaryPower * (bulletTimer * sin((-PLAYER[player].artillaryAngle * (PI / 180))))) - ((gravity * pow(bulletTimer, 2)) / 2);
			if (PLAYER[player].tankRotation)
			{
				DrawMultilineToMainScreen(bulletHor + PLAYER[player].xAxis - 25 - CAMERA2.x, bulletVer + PLAYER[player].yAxis - 17, L"▦████◀", WHITE);
			}
			else
			{
				DrawMultilineToMainScreen(bulletHor + PLAYER[player].xAxis - 25 - CAMERA2.x, bulletVer + PLAYER[player].yAxis - 17, L"▶████▦", WHITE);
			}
			DrawTankCamera(PLAYER1);
			DrawTankCamera(PLAYER2);
			if (isEnemyHit(bulletHor, bulletVer))
			{
				if (turn % 2)
				{
					PLAYER[0].energy -= 20;
					PLAYER[0].move = 0;
				}
				else
				{
					PLAYER[1].energy -= 20;
					PLAYER[1].move = 0;
				}
				break;
			}
			PrintFloor();
			DrawScreen();
		}
		DrawTankCamera(PLAYER1);
		DrawTankCamera(PLAYER2);
		PrintFloor();
		SoundExplosion();
		DrawMultilineToMainScreen(bulletHor + PLAYER[player].xAxis - 25 - CAMERA2.x, bulletVer + PLAYER[player].yAxis - 19, L"  ╬╬╬╬╬", WHITE);
		DrawMultilineToMainScreen(bulletHor + PLAYER[player].xAxis - 25 - CAMERA2.x, bulletVer + PLAYER[player].yAxis - 18, L" ╬╬╬╬╬╬╬", WHITE);
		DrawMultilineToMainScreen(bulletHor + PLAYER[player].xAxis - 25 - CAMERA2.x, bulletVer + PLAYER[player].yAxis - 17, L"╬╬╬╬╬╬╬╬╬", WHITE);
		DrawMultilineToMainScreen(bulletHor + PLAYER[player].xAxis - 25 - CAMERA2.x, bulletVer + PLAYER[player].yAxis - 16, L" ╬╬╬╬╬╬╬", WHITE);
		DrawMultilineToMainScreen(bulletHor + PLAYER[player].xAxis - 25 - CAMERA2.x, bulletVer + PLAYER[player].yAxis - 15, L"  ╬╬╬╬╬", WHITE);
	}
	DrawScreen();
	Sleep(1000);
	bulletCam = 0;
	bulletHor = 0;
	bulletVer = 0;
	SoundNULL();
	return 0;
}

static bool isEnemyHit(double bulletHor, double bulletVer)
{
	bool isHit = false;
	if (turn % 2)
	{
		if ((bulletHor + PLAYER[1].xAxis - 21 > PLAYER[0].xAxis - CAMERA.x - (bulletHor * bulletCam)) && (bulletHor + PLAYER[1].xAxis - 39 < PLAYER[0].xAxis - CAMERA.x - (bulletHor * bulletCam)) && (bulletVer + PLAYER[1].yAxis > 18 + PLAYER[0].yAxis - CAMERA.y - (bulletVer * bulletCam)))
		{
			isHit = true;
		}
	}
	else
	{
		if ((bulletHor + PLAYER[0].xAxis - 21 > PLAYER[1].xAxis - CAMERA.x - (bulletHor * bulletCam)) && (bulletHor + PLAYER[0].xAxis - 39 < PLAYER[1].xAxis - CAMERA.x - (bulletHor * bulletCam)) && (bulletVer + PLAYER[0].yAxis > 18 + PLAYER[1].yAxis - CAMERA.y - (bulletVer * bulletCam)))
		{
			isHit = true;
		}
	}
	return isHit;
}

void RenderStatusPanel(int x, int y, int player) {
	for (int i = 0; i < 2; i++) {
		if (player == i) {
			int angle = (int)(PLAYER[i].artillaryAngle);

			int tens = angle / 10;
			int ones = angle % 10;

			DrawMultilineToMainScreen(x, y - 4, uiBar, YELLOW);
			DrawMultilineToMainScreen(x, y + 4, uiBar, YELLOW);
			if (i == 0) {
				DrawToMainScreen(x + 3, y - 2, L"PLAYER1", MAGENTA);
			}
			if (i == 1) {
				DrawToMainScreen(x + 3, y - 2, L"PLAYER2", MAGENTA);
			}
			DrawToMainScreen(x + 4, y - 1, L"ANGLE");

			DrawMultilineToMainScreen(x + 2, y, numberUnicodeArt[tens], YELLOW);
			DrawMultilineToMainScreen(x + 2 + 5, y, numberUnicodeArt[ones], YELLOW);

			DrawToMainScreen(x + 20, y - 2, L"ENERGY", GREEN);
			DrawToMainScreen(x + 21, y, L"POWER", RED);
			DrawToMainScreen(x + 22, y + 2, L"MOVE", YELLOW);
			if (wind > 0)
			{
				DrawToMainScreen(x + 120, y - 2, L"WIND ->", CYAN);
			}
			else
			{
				DrawToMainScreen(x + 120, y - 2, L"WIND <-", CYAN);
			}
			DrawToMainScreen(x + 120, y + 1, L"AMMO", 0x0007);
			if (turn % 2)
			{
				if (PLAYER[1].ammoType == 0)
				{
					DrawToMainScreen(x + 120, y + 2, L"CANNON", 0x0008);
				}
				if (PLAYER[1].ammoType == 1)
				{
					DrawToMainScreen(x + 120, y + 2, L"BOUNCE", CYAN);
				}
				if (PLAYER[1].ammoType == 2)
				{
					DrawToMainScreen(x + 120, y + 2, L"NET", WHITE);
				}
			}
			else
			{
				if (PLAYER[0].ammoType == 0)
				{
					DrawToMainScreen(x + 120, y + 2, L"CANNON", 0x0008);
				}
				if (PLAYER[0].ammoType == 1)
				{
					DrawToMainScreen(x + 120, y + 2, L"BOUNCE", CYAN);
				}
				if (PLAYER[0].ammoType == 2)
				{
					DrawToMainScreen(x + 120, y + 2, L"NET", WHITE);
				}
			}

			const int SliderMaxSize = 75;

			int energySliderSize = (PLAYER[i].energy * SliderMaxSize) / DEFAULTENERGY;
			int moveSliderSize = (PLAYER[i].move * SliderMaxSize) / DEFAULTMOVE;
			int powerSliderSize = (PLAYER[i].artillaryPower * SliderMaxSize) / MAXARTILLARYPOWER;
			int windSliderSize = (fabs(wind) * 25) / MAXARTILLARYWIND;

			for (int i = 0; i < energySliderSize; i++) {
				DrawToMainScreen(x + 30 + i, y - 2, L"█", GREEN);
			}

			for (int i = 0; i < moveSliderSize; i++) {
				DrawToMainScreen(x + 30 + i, y + 2, L"█", YELLOW);
			}

			for (int i = 0; i < powerSliderSize; i++)
			{
				DrawToMainScreen(x + 30 + i, y, L"█", RED);
			}

			for (int i = 0; i < windSliderSize; i++)
			{
				DrawToMainScreen(x + 120 + i, y - 1, L"█", BLUE);
			}

		}
	}
	
}



void PrintPos(int x, int y, const char* s)
{
	HANDLE handle = g_hScreen[screen_cur];

	COORD Pos = { x * 2, y };
	//SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), Pos);
	SetConsoleCursorPosition(handle, Pos);

	DWORD dw;
	WriteFile(handle, s, strlen(s), &dw, NULL);
}

void ScreenClear(HANDLE hConsole)
{
	COORD coordScreen = { 0, 0 };
	DWORD cCharsWritten;
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	BOOL bSuccess;

	bSuccess = GetConsoleScreenBufferInfo(hConsole, &csbi);
	DWORD dwConSize = csbi.dwSize.X * csbi.dwSize.Y;

	// Fill the entire screen with wide spaces.
	FillConsoleOutputCharacterW(hConsole, L' ', dwConSize, coordScreen, &cCharsWritten);

	bSuccess = GetConsoleScreenBufferInfo(hConsole, &csbi);
	FillConsoleOutputAttribute(hConsole, csbi.wAttributes, dwConSize, coordScreen, &cCharsWritten);
	SetConsoleCursorPosition(hConsole, coordScreen);
}

static HANDLE myConsole;

void ScreenInit() {
	g_hScreen[0] = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);
	g_hScreen[1] = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, 0, NULL, CONSOLE_TEXTMODE_BUFFER, NULL);

	CursorView(g_hScreen[0], false);
	CursorView(g_hScreen[1], false);

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < Y_PIXELS; j++) {
			IS_FLOOR[i][j] = true;
		}
	}
}

void ScreenStart() {
	screen_cur = ++screen_cur % 2;
	myConsole = g_hScreen[screen_cur];		  //버퍼
	ScreenClear(myConsole);					  //버퍼 Clear
}

void ScreenEnd() {
	SetConsoleActiveScreenBuffer(myConsole);  //버퍼를 활성화 함으로써 화면에 버퍼의 내용을 한번에 출력
}

void CursorView(HANDLE hConsole, char show) //커서숨기기
{
	//hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_CURSOR_INFO ConsoleCursor;
	ConsoleCursor.bVisible = show;
	ConsoleCursor.dwSize = 1;
	SetConsoleCursorInfo(hConsole, &ConsoleCursor);
}

void DrawUnicodeFast(HANDLE hConsole, wchar_t picture[Y_PIXELS][X_PIXELS], WORD colors[Y_PIXELS][X_PIXELS]) {
	static CHAR_INFO consoleBuffer[X_PIXELS * Y_PIXELS];

	// Define the region for the console output.
	SMALL_RECT writeRegion = { 0, 0, X_PIXELS - 1, Y_PIXELS - 1 };

	// Fill the CHAR_INFO array with Unicode characters and their respective color attributes.
	for (int y = 0; y < Y_PIXELS; ++y) {
		for (int x = 0; x < X_PIXELS; ++x) {
			int idx = y * X_PIXELS + x;
			consoleBuffer[idx].Char.UnicodeChar = picture[y][x]; // Set Unicode character.
			consoleBuffer[idx].Attributes = colors[y][x];        // Set corresponding color attribute.
		}
	}

	// Set buffer size and starting coordinate.
	COORD bufSize = { (SHORT)X_PIXELS, (SHORT)Y_PIXELS };
	COORD bufCoord = { 0, 0 };

	// Write the full buffer to the console.
	WriteConsoleOutputW(hConsole, consoleBuffer, bufSize, bufCoord, &writeRegion);
}


void DrawScreen(void) {

	ScreenStart();

	DrawUnicodeFast(g_hScreen[screen_cur], MainScreen.unicode, MainScreen.color);

	ScreenEnd();

	initScreen(MainScreen);
}


void initScreen(Picture& screen) {
	for (int y = 0; y < Y_PIXELS; ++y) {
		for (int x = 0; x < X_PIXELS; ++x) {
			screen.unicode[y][x] = L' ';
			screen.color[y][x] = 0x000F;
		}
	}
}


void DrawToMainScreen(int x, int y, const wchar_t* s) {
	DrawToMainScreen(x, y, s, 0x000F);
}

void DrawToMainScreen(int x, int y, const wchar_t* s, WORD color) {
	if (y < 0 || y >= Y_PIXELS) return;

	int i = 0;
	while (s[i] != L'\0') {
		int pos = x + i;
		if (pos < 0 || pos >= X_PIXELS) break;
		MainScreen.unicode[y][pos] = s[i];
		MainScreen.color[y][pos] = color;
		i++;
	}
}

void ClearPictureCell(int x, int y) {
	// Check if the coordinates are within bounds.
	if (x < 0 || x >= X_PIXELS || y < 0 || y >= Y_PIXELS) {
		return;
	}
	// Clear the Unicode character by setting it to a space.
	MainScreen.unicode[y][x] = L' ';
	// Reset the color attribute to default (white on black).
	MainScreen.color[y][x] = 0x000F;
}
void DrawMultilineToMainScreen(int x, int y, const wchar_t* s) {
	DrawMultilineToMainScreen(x, y, s, 0x000F);
}
void DrawMultilineToMainScreen(int x, int y, const wchar_t* s, WORD color = 0x000F) {
	int cur_x = x;
	int cur_y = y;
	for (int i = 0; s[i] != L'\0'; i++) {
		if (s[i] == L'\n') {
			cur_y++;
			cur_x = x;  // Reset x to the starting column for a new line
		}
		else {
			if (cur_y >= 0 && cur_y < Y_PIXELS && cur_x >= 0 && cur_x < X_PIXELS) {
				MainScreen.unicode[cur_y][cur_x] = s[i];
				MainScreen.color[cur_y][cur_x] = color;
			}
			cur_x++;
		}
	}
}

void PrintFloor() {
	wchar_t line[Y_PIXELS + 1];

	for (int row = 0; row < 4; row++) {
		for (int col = 0; col < Y_PIXELS; col++) {
			line[col] = IS_FLOOR[row][col] ? L'█' : L' ';
		}
		line[Y_PIXELS] = L'\0';

		DrawMultilineToMainScreen(0, 6 + PLAYER[0].yAxis - CAMERA.y - (bulletVer * bulletCam) + row, line, GREEN);
	}
}

void AdjustCameraLocation(int player) {
	if (player == 0) {
		CAMERA2.x = 0;
		CAMERA2.y = 0;
	}

	else {
		CAMERA2.x = 180;
		CAMERA2.y = 0;
	}
}

void MainMenu()
{
	initScreen(MainScreen);

	int key = 0;
	int menuX = 145;
	int menuY = 22;
	int tankX = 150;
	int tankY = 10;
	int wordX = 5;
	int wordY = 52;
	ULONGLONG curTime = 0;
	ULONGLONG preTime = 0;
	bool blinkFlag = true;
	ULONGLONG loopBullet = 0;

	while (_kbhit()) _getch(); //버퍼에 있는 키값을 버림 

	while (1)
	{
		curTime = GetTickCount64();

		DrawMultilineToMainScreen(0, 0, mainMenuArt[5], WHITE);
		DrawMultilineToMainScreen(wordX, wordY, mainMenuArt[6], WHITE);

		if (curTime - preTime >= 400)
		{
			blinkFlag = !blinkFlag;
			preTime = curTime;

			DrawMultilineToMainScreen(tankX, tankY, tankUnicodeArt[4], GREEN);
			if (loopBullet % 3 == 0)
			{
				DrawMultilineToMainScreen(tankX + 15, tankY - 2, mainMenuArt[2], GREEN);
				loopBullet++;
			}
			else if (loopBullet % 3 == 1)
			{
				DrawMultilineToMainScreen(tankX + 30, tankY - 7, mainMenuArt[2], GREEN);
				loopBullet++;
			}
			else if (loopBullet % 3 == 2)
			{
				DrawMultilineToMainScreen(tankX + 45, tankY - 2, mainMenuArt[2], GREEN);
				loopBullet++;
			}

			if (blinkFlag)
			{
				DrawMultilineToMainScreen(menuX, menuY, mainMenuArt[0], YELLOW);
				DrawMultilineToMainScreen(menuX + 5, menuY + 3, mainMenuArt[3], YELLOW);

				DrawScreen();
			}

			else
			{
				DrawMultilineToMainScreen(menuX, menuY, mainMenuArt[1], YELLOW);
				DrawMultilineToMainScreen(menuX + 5, menuY + 3, mainMenuArt[4], YELLOW);

				DrawScreen();
			}



		}

		if (_kbhit()) {         //키입력받음 
			key = _getch();
			if (key == KEY_ESC)   exit(0);   // ESC키면 종료 
			else
			{
				currentPhase = SHOW_PLAYER;
				break;      // 아니면 그냥 계속 진행 
			}
		}
	}
}

void GameOverScreen()
{
	int key = 0;
	int textPlayerX = 30;
	int textPlayerY = 20;
	int textGameOverX = 30;
	int textGameOverY = 10;
	int textRestartX = 30;
	int textRestratY = 30;
	ULONGLONG curTime = 0;
	ULONGLONG preTime = 0;

	while (_kbhit()) _getch(); //버퍼에 있는 키값을 버림 

	while (1)
	{
		curTime = GetTickCount64();

		if (curTime - preTime >= 400)
		{
			preTime = curTime;

			DrawMultilineToMainScreen(textGameOverX, textGameOverY, mainMenuArt[9], YELLOW);
			DrawMultilineToMainScreen(textRestartX, textRestratY, mainMenuArt[10], YELLOW);
			DrawMultilineToMainScreen(textRestartX, textRestratY + 5, mainMenuArt[3], YELLOW);


			if (winner)
			{
				DrawMultilineToMainScreen(textPlayerX, textPlayerY, mainMenuArt[7], YELLOW);
				DrawScreen();
			}
			else if (!winner)
			{
				DrawMultilineToMainScreen(textPlayerX, textPlayerY, mainMenuArt[8], YELLOW);
				DrawScreen();
			}

		}

		if (_kbhit()) {         //키입력받음 
			key = _getch();
			if (key == KEY_ESC)   exit(0);   // ESC키면 종료 
			else
			{
				currentPhase = SHOW_PLAYER;
				break;      // 아니면 그냥 계속 진행 
			}
		}
	}
}

void SoundCannon()
{
	PlaySound(TEXT("cannon.wav"), NULL, SND_FILENAME | SND_ASYNC);
}

void SoundExplosion()
{
	PlaySound(TEXT("explosion.wav"), NULL, SND_FILENAME | SND_ASYNC);
}

void SoundTankIdle()
{
	PlaySound(TEXT("tankidle.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_NOSTOP);
}

void SoundCharging()
{
	PlaySound(TEXT("charging.wav"), NULL, SND_FILENAME | SND_ASYNC | SND_NOSTOP);
}

void SoundNULL()
{
	PlaySound(NULL, 0, 0);
}