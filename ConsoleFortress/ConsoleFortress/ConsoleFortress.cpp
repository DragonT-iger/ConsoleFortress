#include <windows.h>
#include <conio.h>
#include <cmath>
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


// Function Prototypes

int Getkey(void);  // We'll leave this as a stub function (not used)
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
void PrintFloor();
void UpdatePlayerAngle(int player);
void UpdatePlayerPower(int player);
void ShootBullet(int player);

// GameManager Variables

const int DEFAULTENERGY = 100;
const int DEFAULTMOVE = 10;
const int MAXARTILLARYANGLE = 75;

struct Camera
{
    int x = 0;
    int y = 0;
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

// Adjust offsets if needed
const int P1_OFFSET_X = 50;
const int P1_OFFSET_Y = 30;
const int P2_OFFSET_X = 80;
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

GamePhase currentPhase = SHOW_PLAYER;

bool IS_FLOOR[4][Y_PIXELS];



int main(void)
{
    CursorView(GetStdHandle(STD_OUTPUT_HANDLE), false);
    ScreenInit();
    PlayerInit();
    DrawScreen();

    // Main game loop with switch-case for phases
    while (true)
    {
        // First, update asynchronous input states
        UpdateInput();

        switch (currentPhase)
        {
        case MAIN_MENU:
            // TODO: fill in main menu logic
            break;

        case SHOW_PLAYER:
        {
            static int showPlayerSubPhase = 0;
            static const int waitDuration = 2000;

            static const ULONGLONG subPhaseDuration = 1000; // 1 second in ms
            static ULONGLONG subPhaseStartTime = 0;

            static int startX = 0, startY = 0;
            static int targetX = 0, targetY = 0;

            // Get current time in ms
            ULONGLONG now = GetTickCount64();

            switch (showPlayerSubPhase)
            {
            case 0:
                CAMERA.x = PLAYER[PLAYER1].xAxis - P1_OFFSET_X;
                CAMERA.y = PLAYER[PLAYER1].yAxis - P1_OFFSET_Y;

                if (subPhaseStartTime == 0)
                    subPhaseStartTime = now;

                if (now - subPhaseStartTime >= waitDuration)
                {
                    startX = CAMERA.x;
                    startY = CAMERA.y;
                    targetX = PLAYER[PLAYER2].xAxis - P2_OFFSET_X;
                    targetY = PLAYER[PLAYER2].yAxis - P2_OFFSET_Y;

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
                RenderStatusPanel(0, 50, PLAYER1);
                break;
            }

            // Draw both players with camera offset

            // Since there's no Sleep(), the camera moves in real time
        }
        break;

        case PLAYER1_MOVING:
            HandleMainGamePlayerInput(PLAYER1);
            RenderStatusPanel(0, 50, PLAYER1);
            break;

        case PLAYER1_ANGLE:
        {
            HandleMainGamePlayerInput(PLAYER1);
            RenderStatusPanel(0, 50, PLAYER1);

            static ULONGLONG lastUpdateTime = 0;
            ULONGLONG currentTime = GetTickCount64();

            if (currentTime - lastUpdateTime >= 20)
            {
                UpdatePlayerAngle(PLAYER1);
                lastUpdateTime = currentTime;
            }
        }
        break;

        case PLAYER1_SHOOTING:
        {
            HandleMainGamePlayerInput(PLAYER1);
            RenderStatusPanel(0, 50, PLAYER1);
            static ULONGLONG lastUpdateTime = 0;
            ULONGLONG currentTime = GetTickCount64();

            if (currentTime - lastUpdateTime >= 20)
            {
                UpdatePlayerPower(PLAYER1);
                lastUpdateTime = currentTime;
            }
        }
        break;

        case WAIT_PLAYER1_PROJECTILE:
            RenderStatusPanel(0, 50, PLAYER1);
            ShootBullet(PLAYER1);
            // TODO: fill in waiting for projectile logic
            break;

        case PLAYER2_MOVING:
            // TODO: fill in player2 turn logic
            break;

        case WAIT_PLAYER2_PROJECTILE:
            // TODO: fill in waiting for player2 projectile logic
            break;

        case GAME_OVER:
            // TODO: fill in game over logic
            break;
        }

        // Render after each phase logic
        DrawTankCamera(PLAYER1);
        DrawTankCamera(PLAYER2);
        PrintFloor();
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
    DrawMultilineToMainScreen(drawX, drawY, tankUnicodeArt[tankIndex], GREEN);
}

void PlayerInit() {
    PLAYER[0].xAxis = 80;
    PLAYER[0].yAxis = 50;

    PLAYER[1].xAxis = 300;
    PLAYER[1].yAxis = 50;
}


// ===========================================================
// Modified: HandleMainGamePlayerInput uses async input
// ===========================================================
void HandleMainGamePlayerInput(int player)
{
    // We no longer use Getkey(), we check with IsKeyDown / IsKeyPressed
    if (currentPhase == PLAYER1_MOVING && PLAYER2_MOVING) {
        // Move left
        if (IsKeyReleased(VK_LEFT) && PLAYER[player].move > 0) {
            if (PLAYER[player].xAxis > 0) PLAYER[player].xAxis--;
            if (PLAYER[player].move > 0) {
                PLAYER[player].move--;
            }
        }
        // Move right
        if (IsKeyReleased(VK_RIGHT) && PLAYER[player].move > 0) {
            if (PLAYER[player].xAxis > 0) PLAYER[player].xAxis++;
            if (PLAYER[player].move > 0) {
                PLAYER[player].move--;
            }
        }
    }

    // Space pressed
    if (IsKeyPressed(VK_SPACE)) {
        switch (currentPhase)
        {
        case PLAYER1_MOVING:
            currentPhase = PLAYER1_ANGLE;
            break;
        case PLAYER1_ANGLE:
            currentPhase = PLAYER1_SHOOTING;
            break;
        case PLAYER1_SHOOTING:
            currentPhase = WAIT_PLAYER1_PROJECTILE;
            break;
        case WAIT_PLAYER1_PROJECTILE:
            break;
        case PLAYER2_MOVING:
            break;
        case PLAYER2_ANGLE:
            break;
        case PLAYER2_SHOOTING:
            break;
        case WAIT_PLAYER2_PROJECTILE:
            break;
        case GAME_OVER:
            break;
        default:
            break;
        }
    }
}
// ===========================================================


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
            DrawToMainScreen(x + 4, y - 1, L"angle");

            DrawMultilineToMainScreen(x + 2, y, numberUnicodeArt[tens], YELLOW);
            DrawMultilineToMainScreen(x + 2 + 5, y, numberUnicodeArt[ones], YELLOW);

            DrawToMainScreen(x + 20, y - 2, L"ENERGY", GREEN);
            DrawToMainScreen(x + 21, y, L"POWER", RED);
            DrawToMainScreen(x + 22, y + 2, L"MOVE", YELLOW);

            const int SliderMaxSize = 75;

            int energySliderSize = (PLAYER[i].energy * SliderMaxSize) / DEFAULTENERGY;
            int moveSliderSize = (PLAYER[i].move * SliderMaxSize) / DEFAULTMOVE;

            for (int j = 0; j < energySliderSize; j++) {
                DrawToMainScreen(x + 30 + j, y - 2, L"█", GREEN);
            }

            for (int j = 0; j < moveSliderSize; j++) {
                DrawToMainScreen(x + 30 + j, y + 2, L"█", YELLOW);
            }

            for (int j = 0; j < PLAYER[i].artillaryPower; j++) {
                DrawToMainScreen(x + 30 + j, y, L"█", RED);
            }
        }
    }
}


void PrintPos(int x, int y, const char* s)
{
    HANDLE handle = g_hScreen[screen_cur];

    COORD Pos = { (SHORT)(x * 2), (SHORT)y };
    SetConsoleCursorPosition(handle, Pos);

    DWORD dw;
    WriteFile(handle, s, (DWORD)strlen(s), &dw, NULL);
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
    myConsole = g_hScreen[screen_cur];
    ScreenClear(myConsole);
}

void ScreenEnd() {
    SetConsoleActiveScreenBuffer(myConsole);
}

void CursorView(HANDLE hConsole, char show) // hide or show cursor
{
    CONSOLE_CURSOR_INFO ConsoleCursor;
    ConsoleCursor.bVisible = show;
    ConsoleCursor.dwSize = 1;
    SetConsoleCursorInfo(hConsole, &ConsoleCursor);
}

void DrawUnicodeFast(HANDLE hConsole, wchar_t picture[Y_PIXELS][X_PIXELS], WORD colors[Y_PIXELS][X_PIXELS]) {
    static CHAR_INFO consoleBuffer[X_PIXELS * Y_PIXELS];

    SMALL_RECT writeRegion = { 0, 0, X_PIXELS - 1, Y_PIXELS - 1 };

    for (int y = 0; y < Y_PIXELS; ++y) {
        for (int x = 0; x < X_PIXELS; ++x) {
            int idx = y * X_PIXELS + x;
            consoleBuffer[idx].Char.UnicodeChar = picture[y][x];
            consoleBuffer[idx].Attributes = colors[y][x];
        }
    }

    COORD bufSize = { (SHORT)X_PIXELS, (SHORT)Y_PIXELS };
    COORD bufCoord = { 0, 0 };

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
    if (x < 0 || x >= X_PIXELS || y < 0 || y >= Y_PIXELS) {
        return;
    }
    MainScreen.unicode[y][x] = L' ';
    MainScreen.color[y][x] = 0x000F;
}

void DrawMultilineToMainScreen(int x, int y, const wchar_t* s) {
    DrawMultilineToMainScreen(x, y, s, 0x000F);
}

void DrawMultilineToMainScreen(int x, int y, const wchar_t* s, WORD color) {
    int cur_x = x;
    int cur_y = y;
    for (int i = 0; s[i] != L'\0'; i++) {
        if (s[i] == L'\n') {
            cur_y++;
            cur_x = x;
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

        DrawMultilineToMainScreen(0, 37 + row, line, GREEN);
    }
}

void UpdatePlayerAngle(int player) {
    static bool increasing = true;
    // Rate of change per update; adjust as needed
    const float delta = 1.0f;

    if (increasing) {
        PLAYER[player].artillaryAngle += delta;
        // When the angle reaches or exceeds 90, clamp it and change direction
        if (PLAYER[player].artillaryAngle >= 75.0f) {
            PLAYER[player].artillaryAngle = 75.0f;
            increasing = false;
        }
    }
    else {
        PLAYER[player].artillaryAngle -= delta;
        // When the angle goes below 0, clamp it and change direction
        if (PLAYER[player].artillaryAngle <= 0.0f) {
            PLAYER[player].artillaryAngle = 0.0f;
            increasing = true;
        }
    }
}

void UpdatePlayerPower(int player) {
    static bool increasing = true; 
    const int delta = 1;      

    if (increasing) {
        PLAYER[player].artillaryPower += delta;
        if (PLAYER[player].artillaryPower >= 75) {
            PLAYER[player].artillaryPower = 75;
            increasing = false;
        }
    }
    else {
        PLAYER[player].artillaryPower -= delta;
        if (PLAYER[player].artillaryPower <= 0) {
            PLAYER[player].artillaryPower = 0;
            increasing = true;
        }
    }
}

void ShootBullet(int player) {
    float angleRad = PLAYER[player].artillaryAngle * (3.1415926f / 180.0f);

    static float velocityX = PLAYER[player].artillaryPower * cos(angleRad);
    velocityX += velocityX;
    static float velocityY = PLAYER[player].artillaryPower * sin(angleRad);
    velocityY += velocityY;

    if (player == PLAYER1) {
        float bulletStartX = (float)PLAYER[player].xAxis - P1_OFFSET_X;
        float bulletStartY = (float)PLAYER[player].yAxis - P1_OFFSET_Y;
        DrawMultilineToMainScreen(bulletStartX + velocityX, bulletStartY - velocityY, bullet);
    }
    if (player == PLAYER2) {
        float bulletStartX = (float)PLAYER[player].xAxis - P2_OFFSET_X;
        float bulletStartY = (float)PLAYER[player].yAxis - P2_OFFSET_Y;
        DrawMultilineToMainScreen(bulletStartX - velocityX, bulletStartY - velocityY, bullet);
    }
}