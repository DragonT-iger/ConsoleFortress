#pragma once

// Foreground color definitions (lower 4 bits)
#define BLACK       0x0000
#define BLUE        0x0001
#define GREEN       0x0002
#define CYAN        0x0003
#define RED         0x0004
#define MAGENTA     0x0005
#define YELLOW      0x0006
#define WHITE       0x0007

// Brightness flag for foreground colors
#define BRIGHT      0x0008  // Use this flag to get a brighter version of any color.

// Background color definitions (upper 4 bits)
// These values are shifted left by 4 bits.
#define BG_BLACK    0x0000
#define BG_BLUE     0x0010
#define BG_GREEN    0x0020
#define BG_CYAN     0x0030
#define BG_RED      0x0040
#define BG_MAGENTA  0x0050
#define BG_YELLOW   0x0060
#define BG_WHITE    0x0070

// Brightness flag for background colors
#define BG_BRIGHT   0x0080  // Use this flag with any background color for a brighter version.

// Example combined colors:
// For a bright blue foreground: (BLUE | BRIGHT)
// For a bright white background: (BG_WHITE | BG_BRIGHT)
