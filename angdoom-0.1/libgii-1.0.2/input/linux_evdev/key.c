/* $Id: key.c,v 1.8 2005/07/29 16:40:56 soyt Exp $
******************************************************************************

   Linux evdev EV_KEY mapper

   Copyright (C) 2000 Marcus Sundberg	[marcus@ggi-project.org]

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   THE AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************************
*/

#include "config.h"
#include "linux_evdev.h"




uint32_t
GII_levdev_key2label(struct gii_input *inp, unsigned short key)
{
	uint32_t label = GIIK_NIL;

	if ((key >= KEY_1) && key <= KEY_9)
		label = GIIUC_1 + (key - KEY_1);
	if ((key >= KEY_F1) && key <= KEY_F10)
		label = GIIK_F1 + (key - KEY_F1);
	if ((key >= KEY_F14) && (key <= KEY_F20))
		label = GIIK_F14 + (key - KEY_F14);
#ifndef KEY_MEDIA
#define KEY_MEDIA 226
#endif
	if ((key >= KEY_STOP) && (key <= KEY_MEDIA)) {
		switch (key) {
		case KEY_STOP:
			label = GIIK_Stop;
			break;
		case KEY_UNDO:
			label = GIIK_Undo;
			break;
		case KEY_FIND:
			label = GIIK_Find;
			break;
		case KEY_HELP:
			label = GIIK_Help;
			break;
		case KEY_MENU:
			label = GIIK_Menu;
			break;
#ifdef KEY_CANCEL
		case KEY_CANCEL:
			label = GIIK_Cancel;
			break;
#endif
		default:
			label = GIIK_Stop + (key - KEY_STOP);
			break;
		}
	}
#ifndef KEY_TWEN
#define KEY_TWEN 0x19f
#endif
	if ((key >= BTN_DIGI) && (key <= KEY_TWEN)) {
		switch (key) {
#ifdef KEY_SELECT
		case KEY_SELECT:
			label = GIIK_Select;
			break;
#endif
#ifdef KEY_CLEAR
		case KEY_CLEAR:
			label = GIIK_Clear;
			break;
#endif
#ifdef KEY_NEXT
		case KEY_NEXT:
			label = GIIK_Next;
			break;
#endif
#ifdef KEY_BREAK
		case KEY_BREAK:
			label = GIIK_Break;
			break;
#endif
		default:
			label = GIIK_Digi + (key - BTN_DIGI);
			break;
		}
	}
	if (label != GIIK_NIL) return label;
	switch (key) {
	case KEY_RESERVED:
		label = GIIK_NIL;
		break;
	case KEY_ESC:
		label = GIIUC_Escape;
		break;
	/* KEY_1 to KEY_9 handled above */
	case KEY_0:
		label = GIIUC_0;
		break;
	case KEY_MINUS:
		label = GIIUC_Minus;
		break;
	case KEY_EQUAL:
		label = GIIUC_Equal;
		break;
	case KEY_BACKSPACE:
		label = GIIUC_BackSpace;
		break;
	case KEY_TAB:
		label = GIIUC_Tab;
		break;
	case KEY_Q:
		label = GIIUC_Q;
		break;
	case KEY_W:
		label = GIIUC_W;
		break;
	case KEY_E:
		label = GIIUC_E;
		break;
	case KEY_R:
		label = GIIUC_R;
		break;
	case KEY_T:
		label = GIIUC_T;
		break;
	case KEY_Y:
		label = GIIUC_Y;
		break;
	case KEY_U:
		label = GIIUC_U;
		break;
	case KEY_I:
		label = GIIUC_I;
		break;
	case KEY_O:
		label = GIIUC_O;
		break;
	case KEY_P:
		label = GIIUC_P;
		break;
	case KEY_LEFTBRACE:
		label = GIIUC_BraceLeft;
		break;
	case KEY_RIGHTBRACE:
		label = GIIUC_BraceRight;
		break;
	case KEY_ENTER:
		label = GIIK_Enter;
		break;
	case KEY_LEFTCTRL:
		label = GIIK_CtrlL;
		break;
	case KEY_A:
		label = GIIUC_A;
		break;
	case KEY_S:
		label = GIIUC_S;
		break;
	case KEY_D:
		label = GIIUC_D;
		break;
	case KEY_F:
		label = GIIUC_F;
		break;
	case KEY_G:
		label = GIIUC_G;
		break;
	case KEY_H:
		label = GIIUC_H;
		break;
	case KEY_J:
		label = GIIUC_J;
		break;
	case KEY_K:
		label = GIIUC_K;
		break;
	case KEY_L:
		label = GIIUC_L;
		break;
	case KEY_SEMICOLON:
		label = GIIUC_Semicolon;
		break;
	case KEY_APOSTROPHE:
		label = GIIUC_Apostrophe;
		break;
	case KEY_GRAVE:
		label = GIIUC_Grave;
		break;
	case KEY_LEFTSHIFT:
		label = GIIK_ShiftL;
		break;
	case KEY_BACKSLASH:
		label = GIIUC_BackSlash;
		break;
	case KEY_Z:
		label = GIIUC_Z;
		break;
	case KEY_X:
		label = GIIUC_X;
		break;
	case KEY_C:
		label = GIIUC_C;
		break;
	case KEY_V:
		label = GIIUC_V;
		break;
	case KEY_B:
		label = GIIUC_B;
		break;
	case KEY_N:
		label = GIIUC_N;
		break;
	case KEY_M:
		label = GIIUC_M;
		break;
	case KEY_COMMA:
		label = GIIUC_Comma;
		break;
	case KEY_DOT:
		label = GIIUC_Period;
		break;
	case KEY_SLASH:
		label = GIIUC_Slash;
		break;
	case KEY_RIGHTSHIFT:
		label = GIIK_ShiftR;
		break;
	case KEY_KPASTERISK:
		label = GIIK_PAsterisk;
		break;
	case KEY_LEFTALT:
		label = GIIK_AltL;
		break;
	case KEY_SPACE:
		label = GIIUC_Space;
		break;
	case KEY_CAPSLOCK:
		label = GIIK_CapsLock;
		break;
	case KEY_1:
		label = GIIUC_1;
		break;
	case KEY_2:
		label = GIIUC_2;
		break;
	case KEY_3:
		label = GIIUC_3;
		break;
	case KEY_4:
		label = GIIUC_4;
		break;
	case KEY_5:
		label = GIIUC_5;
		break;
	case KEY_6:
		label = GIIUC_6;
		break;
	case KEY_7:
		label = GIIUC_7;
		break;
	case KEY_8:
		label = GIIUC_8;
		break;
	case KEY_9:
		label = GIIUC_9;
		break;
	/* KEY_F1 to KEY_F10 handled above */
	case KEY_NUMLOCK:
		label = GIIK_NumLock;
		break;
	case KEY_SCROLLLOCK:
		label = GIIK_ScrollLock;
		break;
	case KEY_KP7:
		label = GIIK_P7;
		break;
	case KEY_KP8:
		label = GIIK_P8;
		break;
	case KEY_KP9:
		label = GIIK_P9;
		break;
	case KEY_KPMINUS:
		label = GIIK_PMinus;
		break;
	case KEY_KP4:
		label = GIIK_P4;
		break;
	case KEY_KP5:
		label = GIIK_P5;
		break;
	case KEY_KP6:
		label = GIIK_P6;
		break;
	case KEY_KPPLUS:
		label = GIIK_PPlus;
		break;
	case KEY_KP1:
		label = GIIK_P1;
		break;
	case KEY_KP2:
		label = GIIK_P2;
		break;
	case KEY_KP3:
		label = GIIK_P3;
		break;
	case KEY_KP0:
		label = GIIK_P0;
		break;
	case KEY_KPDOT:
		label = GIIK_PDecimal;
		break;
#ifdef KEY_103RD
	case KEY_103RD:
		printf("KEY_103RD\n");
#if 0
		label = GIIK_103RD;
#endif
		break;
#endif
	case KEY_F13:
		label = GIIK_F13;
		break;
	case KEY_102ND:
		printf("KEY_102ND\n");
#if 0
		label = GIIK_102ND;
#endif
		break;
	case KEY_F11:
		label = GIIK_F11;
		break;
	case KEY_F12:
		label = GIIK_F12;
		break;
	/* KEY_F14 to KEY_F20 handled above */
	case KEY_KPENTER:
		label = GIIK_PEnter;
		break;
	case KEY_RIGHTCTRL:
		label = GIIK_CtrlR;
		break;
	case KEY_KPSLASH:
		label = GIIK_PSlash;
		break;
	case KEY_SYSRQ:
		label = GIIK_SysRq;
		break;
	case KEY_RIGHTALT:
		label = GIIK_AltR;
		break;
	case KEY_LINEFEED:
		label = GIIUC_Linefeed;
		break;
	case KEY_HOME:
		label = GIIK_Home;
		break;
	case KEY_UP:
		label = GIIK_Up;
		break;
	case KEY_PAGEUP:
		label = GIIK_PageUp;
		break;
	case KEY_LEFT:
		label = GIIK_Left;
		break;
	case KEY_RIGHT:
		label = GIIK_Right;
		break;
	case KEY_END:
		label = GIIK_End;
		break;
	case KEY_DOWN:
		label = GIIK_Down;
		break;
	case KEY_PAGEDOWN:
		label = GIIK_PageDown;
		break;
	case KEY_INSERT:
		label = GIIK_Insert;
		break;
	case KEY_DELETE:
		label = GIIK_Delete;
		break;
	case KEY_MACRO:
		label = GIIK_Macro;
		break;
	case KEY_MUTE:
		label = GIIK_Mute;
		break;
	case KEY_VOLUMEDOWN:
		label = GIIK_VolumeDown;
		break;
	case KEY_VOLUMEUP:
		label = GIIK_VolumeUp;
		break;
	case KEY_POWER:
		label = GIIK_Power;
		break;
	case KEY_KPEQUAL:
		label = GIIK_PEqual;
		break;
	case KEY_KPPLUSMINUS:
		label = GIIK_PPlusMinus;
		break;
	case KEY_PAUSE:
		label = GIIK_Pause;
		break;
	case KEY_F21:
		label = GIIK_F21;
		break;
	case KEY_F22:
		label = GIIK_F22;
		break;
	case KEY_F23:
		label = GIIK_F23;
		break;
	case KEY_F24:
		label = GIIK_F24;
		break;
#if 0
	case KEY_JPN:
		label = GIIK_JPN;
		break;
#endif
	case KEY_LEFTMETA:
		label = GIIK_MetaL;
		break;
	case KEY_RIGHTMETA:
		label = GIIK_MetaR;
		break;
	case KEY_COMPOSE:
		/* This is actually the Windows Menu key on PC-keyobards... */
		label = GIIK_Compose;
		break;
	case KEY_UNKNOWN:
		label = GIIK_NIL;
		break;
	default:
		DPRINT_EVENTS("GII_levdev_key2label: Unknown keycode 0x%x.\n",
			      key);
		break;
	}

	return label;
}
