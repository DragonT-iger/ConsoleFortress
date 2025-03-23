#include <windows.h>
#include <conio.h>
#include "KeyCodes.h"
#include "Color.h"
#include "Tank.h"
#include "Numbers.h"
#include "UI.h"




#define X_PIXELS 2000
#define Y_PIXELS 500




// Panel Rendering


struct Picture {
	wchar_t unicode[Y_PIXELS][X_PIXELS];
	WORD color[Y_PIXELS][X_PIXELS];
};

Picture MainScreen;


HANDLE g_hScreen[2];
int screen_cur = 0;


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


// GameManager Variables

const int DEFAULTENERGY = 100;
const int DEFAULTMOVE = 10;
const int MAXARTILLARYANGLE = 75;

struct Camera
{
	int x = -80;
	int y = 50;
};

Camera CAMERA;

struct Player {
	int xAxis;
	int yAxis;
	int energy = DEFAULTENERGY;
	int move = DEFAULTMOVE;
	int artillaryPower = 0;
	float artillaryAngle = 0.0f;
};
Player PLAYER[2];

const int PLAYER1 = 0;
const int PLAYER2 = 1;

const int P1_OFFSET_X = 0;
const int P1_OFFSET_Y = 0;
const int P2_OFFSET_X = 0;
const int P2_OFFSET_Y = 0;

enum GamePhase {
	MAIN_MENU,
	SHOW_PLAYER,
	PLAYER1_MOVING,
	PLAYER1_SHOOTING,
	PLAYER1_ANGLE,
	WAIT_PLAYER1_PROJECTILE,
	PLAYER2_MOVING,
	PLAYER2_SHOOTING,
	PLAYER2_TURN,
	WAIT_PLAYER2_PROJECTILE,
	GAME_OVER
};

GamePhase currentPhase = SHOW_PLAYER;


int main(void)
{
	CursorView(GetStdHandle(STD_OUTPUT_HANDLE), false);
	ScreenInit();
	PlayerInit();
	DrawScreen();

	// Main game loop with switch-case for phases
	while (true)
	{
		switch (currentPhase)
		{
		case MAIN_MENU:
			// TODO: fill in main menu logic
			// Example:
			// HandlePlayerInput(0);
			// RenderStatusPanel(...);
			break;

		case SHOW_PLAYER:
		{
			/*
			 * Sub-phase approach (time-based):
			 *   0) Instantly place camera on Player1
			 *   1) Animate camera from P1 -> P2 over 1 second
			 *   2) Animate camera from P2 -> P1 over 1 second
			 *   3) Move to next phase
			 */

			static int showPlayerSubPhase = 0;

			// We'll use these to store animation info
			static const ULONGLONG subPhaseDuration = 1000; // 1 second in ms
			static ULONGLONG subPhaseStartTime = 0;

			// Start/end positions for camera each sub-phase
			static int startX = 0, startY = 0;
			static int targetX = 0, targetY = 0;

			// Get current time in ms
			ULONGLONG now = GetTickCount64();

			switch (showPlayerSubPhase)
			{
			case 0:
				// Instantly place camera on Player1, applying Player1's offsets
				CAMERA.x = PLAYER[PLAYER1].xAxis - P1_OFFSET_X;
				CAMERA.y = PLAYER[PLAYER1].yAxis - P1_OFFSET_Y;

				// Prepare for next sub-phase: animate from P1 -> P2
				startX = CAMERA.x;
				startY = CAMERA.y;
				targetX = PLAYER[PLAYER2].xAxis - P2_OFFSET_X;
				targetY = PLAYER[PLAYER2].yAxis - P2_OFFSET_Y;

				subPhaseStartTime = now;
				showPlayerSubPhase++;
				break;

			case 1:
			{
				// Animate camera from (startX, startY) to (targetX, targetY)
				// over subPhaseDuration ms
				ULONGLONG elapsed = now - subPhaseStartTime;
				float t = (float)elapsed / (float)subPhaseDuration; // 0 to 1

				if (t >= 1.0f)
				{
					// Clamp to final position
					t = 1.0f;

					// Now set up next sub-phase (P2 -> P1)
					showPlayerSubPhase++;

					// Start is the camera's current final position
					startX = targetX;
					startY = targetY;

					// Target is Player1 again (with P1 offsets)
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
				// Animate camera from P2 -> P1
				ULONGLONG elapsed = now - subPhaseStartTime;
				float t = (float)elapsed / (float)subPhaseDuration;

				if (t >= 1.0f)
				{
					t = 1.0f;
					// Done with showing players
					showPlayerSubPhase++;
				}

				CAMERA.x = (int)(startX + t * (targetX - startX));
				CAMERA.y = (int)(startY + t * (targetY - startY));
			}
			break;

			case 3:
				// Move on to the next phase
				currentPhase = PLAYER1_MOVING;
				break;
			}

			// Draw both players with camera offset
			DrawTankCamera(PLAYER1);
			DrawTankCamera(PLAYER2);

			// Since there's no Sleep(), the camera moves in real time,
			// completing in ~1 second for each segment (based on subPhaseDuration).
		}
		break;

		case PLAYER1_MOVING:
			// TODO: fill in player1 moving logic
			// Example usage:
			HandleMainGamePlayerInput(PLAYER1);
			RenderStatusPanel(0, 50, PLAYER1);
			DrawTankCamera(PLAYER1);
			DrawTankCamera(PLAYER2);
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

		case PLAYER2_TURN:
			// TODO: fill in player2 turn logic
			break;

		case WAIT_PLAYER2_PROJECTILE:
			// TODO: fill in waiting for player2 projectile logic
			break;

		case GAME_OVER:
			// TODO: fill in game over logic
			// Possibly break out of the loop or wait for input
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
	int drawX = PLAYER[player].xAxis - CAMERA.x;
	int drawY = PLAYER[player].yAxis - CAMERA.y;

	// Safety check to avoid drawing out of bounds
	if (drawX < 0 || drawX >= X_PIXELS || drawY < 0 || drawY >= Y_PIXELS) {
		return;
	}

	// For example, we use the angle to choose an index in 'tankUnicodeArt'
	int tankIndex = int(PLAYER[player].artillaryAngle / 15);

	// Make sure the index doesn't go out of bounds for your tankUnicodeArt array
	// (In your project, check the valid range. We'll assume it is safe for demonstration.)
	DrawMultilineToMainScreen(drawX, drawY, tankUnicodeArt[tankIndex], GREEN);
}


void PlayerInit() {
	PLAYER[0].xAxis = 80;
	PLAYER[0].yAxis = 50;

	PLAYER[1].xAxis = 200;
	PLAYER[1].yAxis = 50;
}

void HandleMainGamePlayerInput(int player) {

	int key = Getkey();

	if (key == KEY_LEFT && PLAYER[player].move > 0) {
		if (PLAYER[player].xAxis > 0) PLAYER[player].xAxis--;
		if (PLAYER[player].move > 0) {
			PLAYER[player].move--;
		}

	}	// 좌 방향키
	if (key == KEY_RIGHT && PLAYER[player].move > 0) {
		if (PLAYER[player].xAxis > 0) PLAYER[player].xAxis++;
		if (PLAYER[player].move > 0) {
			PLAYER[player].move--;
		}

	}	// 우 방향키
	if (key == KEY_SPACE) {
		
	}
}

void RenderStatusPanel(int x, int y, int player) {
	if (player == PLAYER1) {
		int angle = (int)(PLAYER[PLAYER1].artillaryAngle);

		int tens = angle / 10;
		int ones = angle % 10;

		DrawMultilineToMainScreen(x, y - 4, uiBar, YELLOW);
		DrawMultilineToMainScreen(x, y + 4, uiBar, YELLOW);
		DrawToMainScreen(x + 3, y - 2, L"PLAYER1",MAGENTA);
		DrawToMainScreen(x + 4, y - 1, L"angle");

		DrawMultilineToMainScreen(x + 2, y, numberUnicodeArt[tens], YELLOW);
		DrawMultilineToMainScreen(x + 2 + 5, y, numberUnicodeArt[ones], YELLOW);

		DrawToMainScreen(x + 20, y - 2, L"ENERGY", GREEN);
		DrawToMainScreen(x + 21, y, L"POWER", RED);
		DrawToMainScreen(x + 22, y + 2, L"MOVE", YELLOW);

		const int SliderMaxSize = 75;

		int energySliderSize = (PLAYER[PLAYER1].energy * SliderMaxSize) / DEFAULTENERGY;
		int moveSliderSize = (PLAYER[PLAYER1].move * SliderMaxSize) / DEFAULTMOVE;

		for (int i = 0; i < energySliderSize; i++) {
			DrawToMainScreen(x + 30 + i, y - 2, L"█", GREEN);
		}

		for (int i = 0; i < moveSliderSize; i++) {
			DrawToMainScreen(x + 30 + i, y + 2, L"█", YELLOW);
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
