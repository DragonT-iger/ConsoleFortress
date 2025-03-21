#pragma once
/**************************************************************
 * File: KeyCodes.h
 * Description: Header file defining arrow key codes (etc.)
 **************************************************************/
#ifndef KEYCODES_H
#define KEYCODES_H

 // Arrow Keys
#define KEY_LEFT  75
#define KEY_RIGHT 77
#define KEY_UP    72
#define KEY_DOWN  80


int Getkey(void)
{
	int ch = _getch();
	if (ch == 0 || ch == 224)	// 방향키의 경우 0 또는 224의 값이 먼저 입력됨
		ch = _getch();	// 그 다음에 해당 방향키에 따라 72(Up),80(Down),75(Left),77(Right) 값이 입력됨
	return ch;
}

#endif // KEYCODES_H
