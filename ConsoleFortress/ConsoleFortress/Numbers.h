#pragma once
#ifndef NUMBERS_H
#define NUMBERS_H

// ASCII art representation for digits 0 to 9 using block characters.
// Each digit is stored as a multi-line wide string literal.

// You can adjust the art below as needed. The current design uses a 5-line format.

static const wchar_t* const numberUnicodeArt[10] = {
    // Digit 0
    L"▄▄▄▄\n"
    L"█  █\n"
    L"█▄▄█\n",

    // Digit 1
    L"   ▄\n"
    L"   █\n"
    L"   █\n",

    // Digit 2
    L"▄▄▄▄\n"
    L"▄▄▄█\n"
    L"█▄▄▄\n",

    // Digit 3
    L"▄▄▄▄\n"
    L"▄▄▄█\n"
    L"▄▄▄█\n",

    // Digit 4
    L"▄  ▄\n"
    L"█▄▄█\n"
    L"   █\n",

    // Digit 5
    L"▄▄▄▄\n"
    L"█▄▄▄\n"
    L"▄▄▄█\n",

    // Digit 6
    L"▄▄▄▄\n"
    L"█▄▄▄\n"
    L"█▄▄█\n",

    // Digit 7
    L"▄▄▄▄\n"
    L"   █\n"
    L"   █\n",

    // Digit 8
    L"▄▄▄▄\n"
    L"█▄▄█\n"
    L"█▄▄█\n",

    // Digit 9
    L"▄▄▄▄\n"
    L"█▄▄█\n"
    L"▄▄▄█\n"
};

#endif // NUMBERS_H
