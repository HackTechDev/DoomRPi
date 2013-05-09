/* $Id: dxinputkb.h,v 1.3 2005/07/29 16:40:55 soyt Exp $
******************************************************************************

   DirectX inputlib keyboard mapping

   Copyright (C) 1999-2000 John Fortin		[fortinj@ibm.net]
   Copyright (C) 2004      Peter Ekberg		[peda@lysator.liu.se]

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

#include <dinput.h>
#include <ggi/keyboard.h>

static uint16_t dx_kb[256];

static void
initkb(void)
{
	dx_kb[DIK_1] = (uint16_t) GIIUC_1;
	dx_kb[DIK_2] = (uint16_t) GIIUC_2;
	dx_kb[DIK_3] = (uint16_t) GIIUC_3;
	dx_kb[DIK_4] = (uint16_t) GIIUC_4;
	dx_kb[DIK_5] = (uint16_t) GIIUC_5;
	dx_kb[DIK_6] = (uint16_t) GIIUC_6;
	dx_kb[DIK_7] = (uint16_t) GIIUC_7;
	dx_kb[DIK_8] = (uint16_t) GIIUC_8;
	dx_kb[DIK_9] = (uint16_t) GIIUC_9;
	dx_kb[DIK_0] = (uint16_t) GIIUC_0;
	dx_kb[DIK_A] = (uint16_t) GIIUC_A;
	dx_kb[DIK_B] = (uint16_t) GIIUC_B;
	dx_kb[DIK_C] = (uint16_t) GIIUC_C;
	dx_kb[DIK_D] = (uint16_t) GIIUC_D;
	dx_kb[DIK_E] = (uint16_t) GIIUC_E;
	dx_kb[DIK_F] = (uint16_t) GIIUC_F;
	dx_kb[DIK_G] = (uint16_t) GIIUC_G;
	dx_kb[DIK_H] = (uint16_t) GIIUC_H;
	dx_kb[DIK_I] = (uint16_t) GIIUC_I;
	dx_kb[DIK_J] = (uint16_t) GIIUC_J;
	dx_kb[DIK_K] = (uint16_t) GIIUC_K;
	dx_kb[DIK_L] = (uint16_t) GIIUC_L;
	dx_kb[DIK_M] = (uint16_t) GIIUC_M;
	dx_kb[DIK_N] = (uint16_t) GIIUC_N;
	dx_kb[DIK_O] = (uint16_t) GIIUC_O;
	dx_kb[DIK_P] = (uint16_t) GIIUC_P;
	dx_kb[DIK_Q] = (uint16_t) GIIUC_Q;
	dx_kb[DIK_R] = (uint16_t) GIIUC_R;
	dx_kb[DIK_S] = (uint16_t) GIIUC_S;
	dx_kb[DIK_T] = (uint16_t) GIIUC_T;
	dx_kb[DIK_U] = (uint16_t) GIIUC_U;
	dx_kb[DIK_V] = (uint16_t) GIIUC_V;
	dx_kb[DIK_W] = (uint16_t) GIIUC_W;
	dx_kb[DIK_X] = (uint16_t) GIIUC_X;
	dx_kb[DIK_Y] = (uint16_t) GIIUC_Y;
	dx_kb[DIK_Z] = (uint16_t) GIIUC_Z;
	dx_kb[DIK_ESCAPE] = (uint16_t) GIIUC_Escape;
	dx_kb[DIK_MINUS] = (uint16_t) GIIUC_Minus;
	dx_kb[DIK_EQUALS] = (uint16_t) GIIUC_Equal;
	dx_kb[DIK_BACK] = (uint16_t) GIIUC_BackSpace;
	dx_kb[DIK_TAB] = (uint16_t) GIIUC_Tab;
	dx_kb[DIK_LBRACKET] = (uint16_t) GIIUC_BracketLeft;
	dx_kb[DIK_RBRACKET] = (uint16_t) GIIUC_BracketRight;
	dx_kb[DIK_RETURN] = (uint16_t) GIIUC_Return;
	dx_kb[DIK_SEMICOLON] = (uint16_t) GIIUC_Semicolon;
	dx_kb[DIK_APOSTROPHE] = (uint16_t) GIIUC_Apostrophe;
	dx_kb[DIK_GRAVE] = (uint16_t) GIIUC_Grave;
	dx_kb[DIK_BACKSLASH] = (uint16_t) GIIUC_BackSlash;
	dx_kb[DIK_COMMA] = (uint16_t) GIIUC_Comma;
	dx_kb[DIK_PERIOD] = (uint16_t) GIIUC_Period;
	dx_kb[DIK_SLASH] = (uint16_t) GIIUC_Slash;
	dx_kb[DIK_SPACE] = (uint16_t) GIIUC_Space;
	dx_kb[DIK_AT] = (uint16_t) GIIUC_At;
	dx_kb[DIK_COLON] = (uint16_t) GIIUC_Colon;
	dx_kb[DIK_UNDERLINE] = (uint16_t) GIIUC_Underscore;

	dx_kb[DIK_LSHIFT] = (uint16_t) GIIK_ShiftL;
	dx_kb[DIK_RSHIFT] = (uint16_t) GIIK_ShiftR;
	dx_kb[DIK_LCONTROL] = (uint16_t) GIIK_CtrlL;
	dx_kb[DIK_RCONTROL] = (uint16_t) GIIK_CtrlR;
	dx_kb[DIK_LMENU] = (uint16_t) GIIK_AltL;
	dx_kb[DIK_RMENU] = (uint16_t) GIIK_AltR;
	dx_kb[DIK_CAPITAL] = (uint16_t) GIIK_CapsLock;
	dx_kb[DIK_NUMLOCK] = (uint16_t) GIIK_NumLock;
	dx_kb[DIK_SCROLL] = (uint16_t) GIIK_ScrollLock;
	dx_kb[DIK_SUBTRACT] = (uint16_t) GIIK_PMinus;
	dx_kb[DIK_ADD] = (uint16_t) GIIK_PPlus;
	dx_kb[DIK_MULTIPLY] = (uint16_t) GIIK_PAsterisk;
	dx_kb[DIK_DIVIDE] = (uint16_t) GIIK_PSlash;
	dx_kb[DIK_NUMPADEQUALS] = (uint16_t) GIIK_PEqual;
	dx_kb[DIK_NUMPADENTER] = (uint16_t) GIIK_PEnter;
	dx_kb[DIK_NUMPADCOMMA] = (uint16_t) GIIK_PSeparator;
	dx_kb[DIK_NUMPAD1] = (uint16_t) GIIK_P1;
	dx_kb[DIK_NUMPAD2] = (uint16_t) GIIK_P2;
	dx_kb[DIK_NUMPAD3] = (uint16_t) GIIK_P3;
	dx_kb[DIK_NUMPAD4] = (uint16_t) GIIK_P4;
	dx_kb[DIK_NUMPAD5] = (uint16_t) GIIK_P5;
	dx_kb[DIK_NUMPAD6] = (uint16_t) GIIK_P6;
	dx_kb[DIK_NUMPAD7] = (uint16_t) GIIK_P7;
	dx_kb[DIK_NUMPAD8] = (uint16_t) GIIK_P8;
	dx_kb[DIK_NUMPAD9] = (uint16_t) GIIK_P9;
	dx_kb[DIK_NUMPAD0] = (uint16_t) GIIK_P0;
	dx_kb[DIK_DECIMAL] = (uint16_t) GIIK_PDecimal;
	dx_kb[DIK_F1] = (uint16_t) GIIK_F1;
	dx_kb[DIK_F2] = (uint16_t) GIIK_F2;
	dx_kb[DIK_F3] = (uint16_t) GIIK_F3;
	dx_kb[DIK_F4] = (uint16_t) GIIK_F4;
	dx_kb[DIK_F5] = (uint16_t) GIIK_F5;
	dx_kb[DIK_F6] = (uint16_t) GIIK_F6;
	dx_kb[DIK_F7] = (uint16_t) GIIK_F7;
	dx_kb[DIK_F8] = (uint16_t) GIIK_F8;
	dx_kb[DIK_F9] = (uint16_t) GIIK_F9;
	dx_kb[DIK_F10] = (uint16_t) GIIK_F10;
	dx_kb[DIK_F11] = (uint16_t) GIIK_F11;
	dx_kb[DIK_F12] = (uint16_t) GIIK_F12;
	dx_kb[DIK_F13] = (uint16_t) GIIK_F13;
	dx_kb[DIK_F14] = (uint16_t) GIIK_F14;
	dx_kb[DIK_F15] = (uint16_t) GIIK_F15;
	dx_kb[DIK_SYSRQ] = (uint16_t) GIIK_SysRq;
	dx_kb[DIK_HOME] = (uint16_t) GIIK_Home;
	dx_kb[DIK_END] = (uint16_t) GIIK_End;
	dx_kb[DIK_PRIOR] = (uint16_t) GIIK_Prior;
	dx_kb[DIK_NEXT] = (uint16_t) GIIK_Next;
	dx_kb[DIK_UP] = (uint16_t) GIIK_Up;
	dx_kb[DIK_DOWN] = (uint16_t) GIIK_Down;
	dx_kb[DIK_LEFT] = (uint16_t) GIIK_Left;
	dx_kb[DIK_RIGHT] = (uint16_t) GIIK_Right;
	dx_kb[DIK_INSERT] = (uint16_t) GIIK_Insert;
	dx_kb[DIK_DELETE] = (uint16_t) GIIK_Delete;
}
