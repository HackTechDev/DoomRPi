/* $Id: keycodes.h,v 1.2 2005/07/31 15:31:13 soyt Exp $
******************************************************************************

   FreeBSD vgl(3) keycodes

   Copyright (C) 2000 Alcove - Nicolas Souchu <nsouch@freebsd.org>

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

/* Tested on a 105 keys French keyboard with fr.iso.acc loaded */

#ifndef __KEYCODES_H
#define __KEYCODES_H

#define KEY_NOKEY		0
#define KEY_ESC			1
#define KEY_1			2
#define KEY_2			3
#define KEY_3			4
#define KEY_4			5
#define KEY_5			6
#define KEY_6			7
#define KEY_7			8
#define KEY_8			9
#define KEY_9			10
#define KEY_0			11
#define KEY_MINUS		12
#define KEY_EQUAL		13
#define KEY_BACKSPACE		14
#define KEY_TAB			15
#define KEY_Q			16
#define KEY_W			17
#define KEY_E			18
#define KEY_R			19
#define KEY_T			20
#define KEY_Y			21
#define KEY_U			22
#define KEY_I			23
#define KEY_O			24
#define KEY_P			25
#define KEY_LEFTBRACE		26
#define KEY_RIGHTBRACE		27
#define KEY_ENTER		28
#define KEY_LEFTCTRL		29
#define KEY_A			30
#define KEY_S			31
#define KEY_D			32
#define KEY_F			33
#define KEY_G			34
#define KEY_H			35
#define KEY_J			36
#define KEY_K			37
#define KEY_L			38
#define KEY_SEMICOLON		39
#define KEY_APOSTROPHE		40
#define KEY_GRAVE		41
#define KEY_LEFTSHIFT		42
#define KEY_BACKSLASH		43
#define KEY_Z			44
#define KEY_X			45
#define KEY_C			46
#define KEY_V			47
#define KEY_B			48
#define KEY_N			49
#define KEY_M			50
#define KEY_COMMA		51
#define KEY_DOT			52
#define KEY_SLASH		53
#define KEY_RIGHTSHIFT		54
#define KEY_KPASTERISK		55
#define KEY_LEFTALT		56
#define KEY_SPACE		57
#define KEY_CAPSLOCK		58
#define KEY_F1			59
#define KEY_F2			60
#define KEY_F3			61
#define KEY_F4			62
#define KEY_F5			63
#define KEY_F6			64
#define KEY_F7			65
#define KEY_F8			66
#define KEY_F9			67
#define KEY_F10			68
#define KEY_NUMLOCK		69
#define KEY_SCROLLLOCK		70
#define KEY_KP7			71
#define KEY_KP8			72
#define KEY_KP9			73
#define KEY_KPMINUS		74
#define KEY_KP4			75
#define KEY_KP5			76
#define KEY_KP6			77
#define KEY_KPPLUS		78
#define KEY_KP1			79
#define KEY_KP2			80
#define KEY_KP3			81
#define KEY_KP0			82
#define KEY_KPDOT		83
#define KEY_103RD		84
#define KEY_F13			85
#define KEY_102ND		86
#define KEY_F11			87
#define KEY_F12			88
#define KEY_KPENTER		89
#define KEY_RIGHTCTRL		90
#define KEY_KPSLASH		91
#define KEY_SYSRQ		92
#define KEY_RIGHTALT		93
#define KEY_HOME		94
#define KEY_UP			95
#define KEY_PAGEUP		96
#define KEY_LEFT		97
#define KEY_RIGHT		98
#define KEY_END			99
#define KEY_DOWN		100
#define KEY_PAGEDOWN		101
#define KEY_INSERT		102
#define KEY_DELETE		103
#define KEY_PAUSE		104
#define KEY_LEFTMETA		105
#define KEY_RIGHTMETA		106
#define KEY_COMPOSE		107		/* Windows <MENU> key */

/* Press/Release bit in the scancode */
#define KBD_KEY_RELEASED	0x80

/* State indexes in the keymap array of each key */
#define KBD_STATE_BASE			0
#define KBD_STATE_SHIFT			1
#define KBD_STATE_CNTRL			2
#define KBD_STATE_CNTRL_SHIFT		3
#define KBD_STATE_ALT			4
#define KBD_STATE_ALT_SHIFT		5
#define KBD_STATE_ALT_CNTRL		6
#define KBD_STATE_ALT_CNTRL_SHIFT	7

/* Special bits */
#define KBD_SPCL_BASE			(0x80 >> KBD_STATE_BASE)
#define KBD_SPCL_SHIFT			(0x80 >> KBD_STATE_SHIFT)
#define KBD_SPCL_CNTRL			(0x80 >> KBD_STATE_CNTRL)
#define KBD_SPCL_CNTRL_SHIFT		(0x80 >> KBD_STATE_CNTRL_SHIFT)
#define KBD_SPCL_ALT			(0x80 >> KBD_STATE_ALT)
#define KBD_SPCL_ALT_SHIFT		(0x80 >> KBD_STATE_ALT_SHIFT)
#define KBD_SPCL_ALT_CNTRL		(0x80 >> KBD_STATE_ALT_CNTRL)
#define KBD_SPCL_ALT_CNTRL_SHIFT	(0x80 >> KBD_STATE_ALT_CNTRL_SHIFT)

#define KBD_SPCL_MASK 	(KBD_SPCL_BASE | KBD_SPCL_SHIFT | KBD_SPCL_CNTRL | \
			KBD_SPCL_CNTRL_SHIFT | KBD_SPCL_ALT | \
			KBD_SPCL_ALT_SHIFT | KBD_SPCL_ALT_CNTRL | \
			KBD_SPCL_ALT_CNTRL_SHIFT)

#endif /* __KEYCODES_H */
