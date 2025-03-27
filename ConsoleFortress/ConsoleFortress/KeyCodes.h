#pragma once

#include <conio.h>

 /**************************************************************
  * Arrow Keys (extended codes via _getch())
  *   - In _getch() context, if the first read is 0 or 224,
  *     the second read may yield these values.
  **************************************************************/
#define KEY_LEFT    75  // '←'
#define KEY_RIGHT   77  // '→'
#define KEY_UP      72  // '↑'
#define KEY_DOWN    80  // '↓'

  /**************************************************************
   * Additional Keys (extended codes)
   *   - These also come in via the second read if the first is
   *     0 or 224.
   **************************************************************/
#define KEY_HOME    71
#define KEY_END     79
#define KEY_PGUP    73
#define KEY_PGDN    81
#define KEY_INSERT  82
#define KEY_DELETE  83

   /**************************************************************
    * Function Keys (extended codes)
    *   - F1~F10 are 59~68
    *   - F11 = 133, F12 = 134
    *   - Note: These values can vary on different compilers or
    *     different environments. Commonly they are as below.
    **************************************************************/
#define KEY_F1      59
#define KEY_F2      60
#define KEY_F3      61
#define KEY_F4      62
#define KEY_F5      63
#define KEY_F6      64
#define KEY_F7      65
#define KEY_F8      66
#define KEY_F9      67
#define KEY_F10     68
#define KEY_F11     133
#define KEY_F12     134

    /**************************************************************
     * Common ASCII keys (first read from _getch() if < 128)
     **************************************************************/
#define KEY_ESC        27
#define KEY_ENTER      13
#define KEY_SPACE      32
#define KEY_TAB        9
#define KEY_BACKSPACE  8

     // Control key codes (Ctrl + C, Ctrl + D, etc.)
#define KEY_CTRL_C  3
#define KEY_CTRL_D  4
#define KEY_CTRL_Z  26

/**************************************************************
 * Getkey() function to handle arrow keys or function keys
 *   - If the key is an extended key (first read 0 or 224),
 *     we call _getch() again to get the real code.
 *   - Otherwise, we just return the ASCII code directly.
 **************************************************************/
int Getkey(void)
{
    int ch = _getch();
    if (ch == 0 || ch == 224) {
        ch = _getch();
    }
    return ch;
}
