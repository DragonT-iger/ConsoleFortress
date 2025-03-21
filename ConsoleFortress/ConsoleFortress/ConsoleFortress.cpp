#include <windows.h>
#include <conio.h>
#include "KeyCodes.h"
#include "Color.h"

#define X_PIXELS 200
#define Y_PIXELS 50



struct Picture {
	wchar_t unicode[Y_PIXELS][X_PIXELS];
	WORD color[Y_PIXELS][X_PIXELS];
};

Picture MainScreen;

HANDLE g_hScreen[2]; //두 개의 화면버퍼를 담을 배열
int screen_cur = 0;

int Getkey(void);								//키입력
void PrintPos(int x, int y, const char* s);		//특정위치 출력

void CursorView(HANDLE hConsole, char show);	//커서숨기기

void ScreenInit();
void ScreenStart();								//스크린 
void ScreenEnd();								//스크린 
void ScreenClear(HANDLE hConsole);				//스크린 클리어
void DrawUnicodeFast(HANDLE hConsole, wchar_t picture[Y_PIXELS][X_PIXELS], WORD colors[Y_PIXELS][X_PIXELS]);
void DrawScreen(void);
void DrawToPicture(int x, int y, const wchar_t* s);
void DrawToPicture(int x, int y, const wchar_t* s, WORD color);
void ClearPictureCell(int x, int y);
void initScreen(Picture& screen);

int main(void)
{
	CursorView(GetStdHandle(STD_OUTPUT_HANDLE), false);

	ScreenInit();

	int x = 5, y = 5;

	DrawToPicture(x, y, L"★",0x0001);							//버퍼에 그리기
	DrawScreen();

	while (1)
	{
		//input -----------------------------------
		int key = Getkey();
		if (key == KEY_LEFT) { if (x > 0) x = x - 1; }	// 좌 방향키
		if (key == KEY_RIGHT) { if (x < 20) x = x + 1; }	// 우 방향키
		if (key == KEY_UP) { if (y > 0) y = y - 1; }	// 상 방향키
		if (key == KEY_DOWN) { if (y < 20) y = y + 1; }	// 하 방향키

		//-----------------------------------------


		//display ---------------------------------

		// HandleConsoleKeyBoardInput();

		DrawToPicture(x, y, L"★",BLUE);
		DrawScreen();	

	}
	return 0;
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


void DrawToPicture(int x, int y, const wchar_t* s) {
	DrawToPicture(x, y, s, 0x000F);
}

void DrawToPicture(int x, int y, const wchar_t* s, WORD color) {
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
