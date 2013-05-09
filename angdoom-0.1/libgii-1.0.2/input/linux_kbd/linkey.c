/* $Id: linkey.c,v 1.4 2005/07/29 16:40:57 soyt Exp $
******************************************************************************

   Convert Linux keysyms into LibGII syms and labels

   Copyright (C) 1998  Andrew Apted  [andrew@ggi-project.org]

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
#include "linkey.h"
#include <ctype.h>

static uint32_t basic_trans(int sym, int islabel, uint32_t *modifiers,
			    uint32_t label, int keycode)
{
	int typ = KTYP(sym);
	int val = KVAL(sym);


	/* Handle some special cases */
	switch (sym) {
	case 0x1c:
		return GIIK_PrintScreen;
	}

	/* Handle the rest */
	switch (typ) {
	case KT_LATIN: 
	case KT_LETTER: 
		if (val == 0x7f || val == 0x08) {
			/* Backspace and delete mapping is a mess under Linux.
			   We simply use the keycode to make it right.
			*/
#ifdef __i386__ /* Keycodes for other platforms are welcome. */
			if (keycode == 0x0e) return GIIUC_BackSpace;
			if (keycode == 0x6f) return GIIUC_Delete;
#endif
			return val;
		}
		if (islabel || (*modifiers & GII_MOD_CAPS)) {
			if ((val >= 'a' && val <= 'z') ||
			    (val >= GIIUC_agrave &&
			     val <= GIIUC_ydiaeresis &&
			     val !=  GIIUC_Division)) {
				return (val - 0x20);
			}
		}
		return val;

	case KT_FN:
		if (val < 20) {
			return GII_KEY(GII_KT_FN, val+1);
		} else if (val >= 30) {
#ifdef K_UNDO
			if (val == K_UNDO) return GIIK_Undo;
#endif
			return GII_KEY(GII_KT_FN, val-9);
		}
		break;

	case KT_META:
		*modifiers = *modifiers | GII_MOD_META;
		return val;
	case KT_PAD:
		if (val <= 9) {
			if (islabel) {
				return GII_KEY(GII_KT_PAD, '0' + val);
			} else if ((*modifiers & GII_MOD_NUM)) {
				return '0' + val;
			}
			switch (val) {
			case 0:
				return GIIK_Insert;
			case 1:
				return GIIK_End;
			case 2:
				return GIIK_Down;
			case 3:
				return GIIK_PageDown;
			case 4:
				return GIIK_Left;
			case 5:
				/* X does this, so why not...*/
				return GIIK_Begin;
			case 6:
				return GIIK_Right;
			case 7:
				return GIIK_Home;
			case 8:
				return GIIK_Up;
			case 9:
				return GIIK_PageUp;
			}
		}
		switch (sym) {
		case K_PPLUS:
			if (islabel)	return GIIK_PPlus;
			else 		return GIIUC_Plus;
		case K_PMINUS:
			if (islabel)	return GIIK_PMinus;
			else 		return GIIUC_Minus;
		case K_PSTAR:
			if (islabel)	return GIIK_PAsterisk;
			else 		return GIIUC_Asterisk;
		case K_PSLASH:
			if (islabel)	return GIIK_PSlash;
			else 		return GIIUC_Slash;
		case K_PENTER:
			if (islabel)	return GIIK_PEnter;
			else 		return GIIUC_Return;
			/* Label won't work on keyboards which has both, but
			   it's the best we can do. */
		case K_PCOMMA:
			if (islabel)	return GIIK_PDecimal;
			else if ((*modifiers & GII_MOD_NUM)) {
				return GIIUC_Comma;
			} else {
				return GIIUC_Delete;
			}
		case K_PDOT:
			if (islabel)	return GIIK_PDecimal;
			else if ((*modifiers & GII_MOD_NUM)) {
				return GIIUC_Period;
			} else {
				return GIIUC_Delete;
			}
		case K_PPLUSMINUS:
			if (islabel)	return GIIK_PPlusMinus;
			else 		return GIIUC_PlusMinus;
#ifdef K_PPARENL
		case K_PPARENL:
			if (islabel)	return GIIK_PParenLeft;
			else 		return GIIUC_ParenLeft;
		case K_PPARENR:
			if (islabel)	return GIIK_PParenRight;
			else 		return GIIUC_ParenRight;
#endif
		default:
				/* Unhandled PAD key */
			return GIIK_VOID;
		}
		break;

	case KT_CONS:
		return GIIK_VOID;

	case KT_DEAD:
		switch (sym) {
		case K_DGRAVE:
			return GIIK_DeadGrave;
		case K_DACUTE:
			return GIIK_DeadAcute;
		case K_DCIRCM:
			return GIIK_DeadCircumflex;
		case K_DTILDE:
			return GIIK_DeadTilde;
		case K_DDIERE:
			return GIIK_DeadDiaeresis;
#ifdef K_DCEDIL
		case K_DCEDIL:
			return GIIK_DeadCedilla;
#endif
		}
		return GIIK_VOID;

	case KT_SPEC:
	case KT_CUR:
		break;

	default:  /* The rest not handled yet */
		return GIIK_VOID;
	}

	switch (sym) {
	case K_HOLE:		return GIIK_VOID;

	case K_FIND:		return GIIK_Home;
	case K_SELECT:		return GIIK_End;
	case K_INSERT:		return GIIK_Insert;
	case K_REMOVE:		return GIIK_Delete;
	case K_PGUP:		return GIIK_PageUp;
	case K_PGDN:		return GIIK_PageDown;
	case K_MACRO:		return GIIK_Macro;
	case K_HELP:		return GIIK_Help;
	case K_DO:		return GIIK_Do;
	case K_PAUSE:		return GIIK_Pause;
	case K_ENTER:		return GIIK_Enter;
#ifdef GIIK_ShowRegs
	case K_SH_REGS:		return GIIK_ShowRegs;
	case K_SH_MEM:		return GIIK_ShowMem;
	case K_SH_STAT:		return GIIK_ShowStat;
	case K_CONS:		return GIIK_LastConsole;
	case K_DECRCONSOLE:	return GIIK_PrevConsole;
	case K_INCRCONSOLE:	return GIIK_NextConsole;
	case K_SPAWNCONSOLE:	return GIIK_SpawnConsole;
#endif
	case K_BREAK:		return GIIK_Break;
	case K_CAPS:		return GIIK_CapsLock;
	case K_NUM:		return GIIK_NumLock;
	case K_HOLD:		return GIIK_ScrollLock;
	case K_BOOT:		return GIIK_Boot;
	case K_CAPSON:		return GIIK_CapsLock;
	case K_COMPOSE:		return GIIK_Compose;
	case K_SAK:		return GIIK_SAK;

	case K_SCROLLFORW:
		if (!islabel && label == GIIK_PageDown &&
		    (*modifiers & GII_MOD_SHIFT)) {
			return GIIK_PageDown;
		}
		return GIIK_ScrollForw;
	case K_SCROLLBACK:
		if (!islabel && label == GIIK_PageUp &&
		    (*modifiers & GII_MOD_SHIFT)) {
			return GIIK_PageUp;
		}
		return GIIK_ScrollBack;
	case K_BARENUMLOCK: 	return GIIK_NumLock;

	case K_DOWN:		return GIIK_Down;
	case K_LEFT:		return GIIK_Left;
	case K_RIGHT:		return GIIK_Right;
	case K_UP:		return GIIK_Up;
	}

	/* Undo some Linux braindamage */

	if (sym >= 0x2000) {
		return sym ^ 0xf000;
	}

	return GIIK_VOID;
}
	

int _gii_linkey_trans(int keycode, int labelval, int symval, gii_key_event *ev)
{

	/* Set label field */
	switch (labelval) {
	case K_ALT:		ev->label = GIIK_AltL;		break;
	case K_ALTGR:		ev->label = GIIK_AltR;		break;
#ifdef K_CAPSSHIFT
	case K_CAPSSHIFT:
#endif
	case K_SHIFT:
#ifdef __i386__
		/* If you map shift keys to K_SHIFTL and K_SHIFTR they won't
		   work as shift keys, aieee what braindamage!
		   We try using the keycode to detect whether the user
		   actually pressed right shift.
		   (Non-ix86 platforms are very probably just as broken, but
		   they	are likely to have other keycodes) */
		if (keycode == 0x36) {
				ev->label = GIIK_ShiftR;	break;
		}
#endif
	case K_SHIFTL:		ev->label = GIIK_ShiftL;	break;
	case K_SHIFTR:		ev->label = GIIK_ShiftR;	break;
	case K_CTRL:
#ifdef __i386__
		/* Same thing with control keys... */
		if (keycode == 0x61) {
				ev->label = GIIK_CtrlR;		break;
		}
#endif
	case K_CTRLL:		ev->label = GIIK_CtrlL;		break;
	case K_CTRLR:		ev->label = GIIK_CtrlR;		break;
		
#ifdef K_SHIFTLOCK
		/* What are these things??? */
	case K_ALTLOCK:		ev->label = GIIK_AltL;		break;
	case K_ALTGRLOCK:	ev->label = GIIK_AltR;		break;
	case K_SHIFTLOCK:
	case K_SHIFTLLOCK:	ev->label = GIIK_ShiftL;	break;
	case K_SHIFTRLOCK:	ev->label = GIIK_ShiftR;	break;
	case K_CTRLLOCK:
	case K_CTRLLLOCK:	ev->label = GIIK_CtrlL;		break;
	case K_CTRLRLOCK:	ev->label = GIIK_CtrlR;		break;
#endif

#ifdef K_SHIFT_SLOCK
		/* What are these things??? */
	case K_ALT_SLOCK:	ev->label = GIIK_AltL;		break;
	case K_ALTGR_SLOCK:	ev->label = GIIK_AltR;		break;
	case K_SHIFT_SLOCK:
	case K_SHIFTL_SLOCK:	ev->label = GIIK_ShiftL;	break;
	case K_SHIFTR_SLOCK:	ev->label = GIIK_ShiftR;	break;
	case K_CTRL_SLOCK:
	case K_CTRLL_SLOCK:	ev->label = GIIK_CtrlL;		break;
	case K_CTRLR_SLOCK:	ev->label = GIIK_CtrlR;		break;
#endif
	default:
		ev->label = basic_trans(labelval, 1, &ev->modifiers, 0,
					keycode);
		break;
	}

	/* Set sym field */
	switch (symval) {
	case K_ALT:		ev->sym = GIIK_Alt;	break;
	case K_ALTGR:		ev->sym = GIIK_AltGr;	break;
	case K_SHIFT:
	case K_SHIFTL:
	case K_SHIFTR:		ev->sym = GIIK_Shift;	break;
	case K_CTRL:
	case K_CTRLL:
	case K_CTRLR:		ev->sym = GIIK_Ctrl;	break;

#ifdef K_CAPSSHIFT
	case K_CAPSSHIFT:
			ev->sym = GIIK_CapsLock;	break;
#endif
		
#ifdef K_SHIFTLOCK
	case K_ALTLOCK:		ev->sym = GIIK_Alt;	break;
	case K_ALTGRLOCK:	ev->sym = GIIK_AltGr;	break;
	case K_SHIFTLOCK:
	case K_SHIFTLLOCK:
	case K_SHIFTRLOCK:	ev->sym = GIIK_Shift;	break;
	case K_CTRLLOCK:
	case K_CTRLLLOCK:
	case K_CTRLRLOCK:	ev->sym = GIIK_Ctrl;	break;
#endif

#ifdef K_SHIFT_SLOCK
	case K_ALT_SLOCK:	ev->sym = GIIK_Alt;	break;
	case K_ALTGR_SLOCK:	ev->sym = GIIK_AltGr;	break;
	case K_SHIFT_SLOCK:
	case K_SHIFTL_SLOCK:
	case K_SHIFTR_SLOCK:	ev->sym = GIIK_Shift;	break;
	case K_CTRL_SLOCK:
	case K_CTRLL_SLOCK:
	case K_CTRLR_SLOCK:	ev->sym = GIIK_Ctrl;	break;
#endif
	default:
		ev->sym = basic_trans(symval, 0, &ev->modifiers, ev->label,
				      keycode);
		break;
	}


	/* Handle keys not recognized by Linux keymaps */
	if (ev->label == GIIK_VOID && ev->sym == GIIK_VOID) {
		switch (keycode) {
		case 0x7d:
			/* Left Windows key */
			ev->label = GIIK_MetaL;
			ev->sym = GIIK_Meta;
			break;
		case 0x7e:
			/* Right Windows key */
			ev->label = GIIK_MetaR;
			ev->sym = GIIK_Meta;
			break;
		case 0x7f:
			/* Menu key */
			ev->label = GIIK_Menu;
			ev->sym = GIIK_Meta;
			break;
		}
	}

	return 0;
}
