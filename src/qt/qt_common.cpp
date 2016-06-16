// GB Enhanced+ Copyright Daniel Baxter 2015
// Licensed under the GPLv2
// See LICENSE.txt for full license text

// File : qt_common.h
// Date : July 15, 2015
// Description : Common functions and definitions for Qt

#include <SDL2/SDL.h>

#include <QtGui>

#include "qt_common.h"

/****** Translate Qt keys into SDL keycodes ******/
int qtkey_to_sdlkey(int key)
{
	//Cycle through keys that need special translation
	switch(key)
	{
		case Qt::Key_Escape: return SDLK_ESCAPE; break;
		case Qt::Key_Tab: return SDLK_TAB; break;
		case Qt::Key_Backspace: return SDLK_BACKSPACE; break;
		case Qt::Key_Enter: return SDLK_KP_ENTER; break;
		case Qt::Key_Return: return SDLK_RETURN; break;
		case Qt::Key_Insert: return SDLK_INSERT; break;
		case Qt::Key_Delete: return SDLK_DELETE; break;
		case Qt::Key_Pause: return SDLK_PAUSE; break;

		case Qt::Key_Home: return SDLK_HOME; break;
		case Qt::Key_End: return SDLK_END; break;
		case Qt::Key_Left: return SDLK_LEFT; break;
		case Qt::Key_Right: return SDLK_RIGHT; break;
		case Qt::Key_Up: return SDLK_UP; break;
		case Qt::Key_Down: return SDLK_DOWN; break;
		case Qt::Key_PageUp: return SDLK_PAGEUP; break;
		case Qt::Key_PageDown: return SDLK_PAGEDOWN; break;
		case Qt::Key_Shift: return SDLK_LSHIFT; break;
		case Qt::Key_Control: return SDLK_LCTRL; break;
		//case Qt::Key_Meta: return SDLK_LSUPER; break;
		case Qt::Key_Alt: return SDLK_LALT; break;
		case Qt::Key_AltGr: return SDLK_MODE; break;
		case Qt::Key_CapsLock: return SDLK_CAPSLOCK; break;
		//case Qt::Key_NumLock: return SDLK_NUMLOCK; break;
		//case Qt::Key_ScrollLock: return SDLK_SCROLLOCK; break;

		case Qt::Key_F1: return SDLK_F1; break;
		case Qt::Key_F2: return SDLK_F2; break;
		case Qt::Key_F3: return SDLK_F3; break;
		case Qt::Key_F4: return SDLK_F4; break;
		case Qt::Key_F5: return SDLK_F5; break;
		case Qt::Key_F6: return SDLK_F6; break;
		case Qt::Key_F7: return SDLK_F7; break;
		case Qt::Key_F8: return SDLK_F8; break;
		case Qt::Key_F9: return SDLK_F9; break;
		case Qt::Key_F10: return SDLK_F10; break;
		case Qt::Key_F11: return SDLK_F11; break;
		case Qt::Key_F12: return SDLK_F12; break;
	}

	//ASCII mapped keys are the same for 0x20 to 0x40
	if((key >= 0x20) && (key <= 0x40)) { return key; }

	//ASCII mapped lower-case keys get translated in upper-case
	//SDL only recognizes the upper-case ones
	else if((key >= 0x41) && (key <= 0x5A)) { return (key + 32); }

	//Other ASCII mapped keys are mostly the same
	else if(key <= 0xFF) { return key; }

	else { return -1; }
} 
