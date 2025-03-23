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


// GameManager Variables

const int DEFAULTENERGY = 100;
const int DEFAULTMOVE = 10;
const int MAXARTILLARYANGLE = 75;

struct Player {
	int energy = DEFAULTENERGY;
	int move = DEFAULTMOVE;
	int artillaryPower = 0;
	float artillaryAngle = 0.0f;
};
Player PLAYER[2];

const int PLAYER1 = 0;
const int PLAYER2 = 1;



int main(void)
{
	CursorView(GetStdHandle(STD_OUTPUT_HANDLE), false);

	ScreenInit();

	int x = 5, y = 40;
	

	DrawMultilineToMainScreen(x, y, tankUnicodeArt[int(PLAYER[0].artillaryAngle / 15)], GREEN);
	DrawScreen();

	

	while (1)
	{
		//input -----------------------------------
		int key = Getkey();


		if (key == KEY_LEFT) { if (x > 0) x = x - 1; }	// 좌 방향키
		if (key == KEY_RIGHT) { if (x < Y_PIXELS) x = x + 1; }	// 우 방향키
		if (key == KEY_UP) {
			PLAYER[0].artillaryAngle += 1;
			if (PLAYER[0].artillaryAngle > MAXARTILLARYANGLE) 	PLAYER[0].artillaryAngle = MAXARTILLARYANGLE;
			if (PLAYER[0].energy > 0) PLAYER[0].energy--;
		}	


		if (key == KEY_DOWN) {
			PLAYER[0].artillaryAngle -= 1; 
			if (PLAYER[0].artillaryAngle < 0) PLAYER[0].artillaryAngle = 0; 
			if (PLAYER[0].energy > 0) PLAYER[0].energy--;
		}	// 하 방향키

		//-----------------------------------------

		DrawMultilineToMainScreen(x, y, tankUnicodeArt[int(PLAYER[0].artillaryAngle / 15)], GREEN);

		RenderStatusPanel(0 , 50, PLAYER1);

		//display ---------------------------------

		// HandleConsoleKeyBoardInput();
		DrawScreen();

	}
	return 0;
}

void RenderStatusPanel(int x, int y, int player) {
	if (player == PLAYER1) {
		int angle = (int)(PLAYER[PLAYER1].artillaryAngle);

		int tens = angle / 10;
		int ones = angle % 10;

		DrawMultilineToMainScreen(x, y - 4, uiBar, YELLOW);
		DrawMultilineToMainScreen(x, y + 4, uiBar, YELLOW);
		DrawToMainScreen(x + 4, y - 2, L"angle");

		DrawMultilineToMainScreen(x + 2, y, numberUnicodeArt[tens], YELLOW);
		DrawMultilineToMainScreen(x + 2 + 5, y, numberUnicodeArt[ones], YELLOW);

		DrawToMainScreen(x + 20, y - 2, L"ENERGY", GREEN);
		DrawToMainScreen(x + 21, y, L"POWER", RED);
		DrawToMainScreen(x + 22, y + 2, L"MOVE", YELLOW);

		int SliderMaxSize = 75;

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

	RenderStatusPanel(0, 50, PLAYER1);
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
