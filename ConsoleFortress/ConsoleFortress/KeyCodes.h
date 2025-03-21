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
	if (ch == 0 || ch == 224)	// ����Ű�� ��� 0 �Ǵ� 224�� ���� ���� �Էµ�
		ch = _getch();	// �� ������ �ش� ����Ű�� ���� 72(Up),80(Down),75(Left),77(Right) ���� �Էµ�
	return ch;
}

#endif // KEYCODES_H
