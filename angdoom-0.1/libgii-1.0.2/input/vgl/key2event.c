/* $Id: key2event.c,v 1.5 2005/07/29 16:40:59 soyt Exp $
******************************************************************************

   FreeBSD vgl(3) inputlib key to event converter

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

#include <sys/kbio.h>
#include "config.h"
#include "input-vgl.h"
#include "keycodes.h"

static uint32_t ggi_labels[] = {
	GIIK_NIL, GIIUC_Escape, GIIUC_1, GIIUC_2, GIIUC_3, GIIUC_4, GIIUC_5,
	GIIUC_6, GIIUC_7, GIIUC_8, GIIUC_9, GIIUC_0, GIIUC_Minus, GIIUC_Equal,
	GIIUC_BackSpace, GIIUC_Tab, GIIUC_Q, GIIUC_W, GIIUC_E, GIIUC_R,
	GIIUC_T, GIIUC_Y, GIIUC_U, GIIUC_I, GIIUC_O, GIIUC_P, GIIUC_BraceLeft,
	GIIUC_BraceRight, GIIK_Enter, GIIK_CtrlL, GIIUC_A, GIIUC_S, GIIUC_D,
	GIIUC_F, GIIUC_G, GIIUC_H, GIIUC_J, GIIUC_K, GIIUC_L, GIIUC_Semicolon,
	GIIUC_Apostrophe, GIIUC_Grave, GIIK_ShiftL, GIIUC_BackSlash, GIIUC_Z,
	GIIUC_X, GIIUC_C, GIIUC_V, GIIUC_B, GIIUC_N, GIIUC_M, GIIUC_Comma,
	GIIUC_Period, GIIUC_Slash, GIIK_ShiftR, GIIK_PAsterisk, GIIK_AltL,
	GIIUC_Space, GIIK_CapsLock, GIIK_F1, GIIK_F2, GIIK_F3, GIIK_F4,
	GIIK_F5, GIIK_F6, GIIK_F7, GIIK_F8, GIIK_F9, GIIK_F10, GIIK_NumLock,
	GIIK_ScrollLock, GIIK_P7, GIIK_P8, GIIK_P9, GIIK_PMinus, GIIK_P4,
	GIIK_P5, GIIK_P6, GIIK_PPlus,
	GIIK_P1, GIIK_P2, GIIK_P3, GIIK_P0, GIIK_PDecimal, GIIK_NIL, GIIK_NIL,
	GIIK_NIL, GIIK_F11, GIIK_F12, GIIK_PEnter, GIIK_CtrlR, GIIK_PSlash,
	GIIK_SysRq, GIIK_AltR, GIIK_Home, GIIK_Up, GIIK_PageUp, GIIK_Left,
	GIIK_Right, GIIK_End, GIIK_Down, GIIK_PageDown, GIIK_Insert,
	GIIK_Delete, GIIK_Pause, GIIK_MetaL, GIIK_MetaR, GIIK_Compose, GIIK_NIL
};

#define set_lockkey_state(s, l)				\
	if (!((s) & l ## DOWN)) {			\
		int tmp;				\
		(s) |= l ## DOWN;			\
		(s) ^= l ## ED;				\
		tmp = (s) & LOCK_MASK;			\
		ioctl(0, KDSETLED, (caddr_t)&tmp);	\
	}

static u_int
save_accent_key(gii_vgl_priv *priv, u_int key, int *accents)
{
	int i;

	/* make an index into the accent map */
	i = key - F_ACC + 1;
	if ((i > priv->kbd_accentmap.n_accs)
	    || (priv->kbd_accentmap.acc[i - 1].accchar == 0)) {
		/* the index is out of range or pointing to an empty entry */
		*accents = 0;
		return ERRKEY;
	}

	/* 
	 * If the same accent key has been hit twice, produce the accent char
	 * itself.
	 */
	if (i == *accents) {
		key = priv->kbd_accentmap.acc[i - 1].accchar;
		*accents = 0;
		return key;
	}

	/* remember the index and wait for the next key  */
	*accents = i; 
	return NOKEY;
}

static u_int
make_accent_char(gii_vgl_priv *priv, u_int ch, int *accents)
{
	struct acc_t *acc;
	int i;

	acc = &priv->kbd_accentmap.acc[*accents - 1];
	*accents = 0;

	/* 
	 * If the accent key is followed by the space key,
	 * produce the accent char itself.
	 */
	if (ch == ' ')
		return acc->accchar;

	/* scan the accent map */
	for (i = 0; i < NUM_ACCENTCHARS; ++i) {
		if (acc->map[i][0] == 0)	/* end of table */
			break;
		if (acc->map[i][0] == ch)
			return acc->map[i][1];
	}
	/* this char cannot be accented... */
	return ERRKEY;
}

/*
 * Generate the associated action and update the current state.
 * The result is the so-called 'symbol' in GII.
 *
 * Taken from /sys/dev/kbd/kbd.c
 */
static int
GII_vgl_genaction(gii_vgl_priv *priv, int up, int keycode)
{
	struct keyent_t *key;
	int state = priv->kbd_state;
	int action;
	int f;
	int i;

	i = keycode;
	f = state & (AGRS | ALKED);
	if ((f == AGRS1) || (f == AGRS2) || (f == ALKED))
		i += ALTGR_OFFSET;
	key = &priv->kbd_keymap.key[i];
	i = ((state & SHIFTS) ? 1 : 0)
	    | ((state & CTLS) ? 2 : 0)
	    | ((state & ALTS) ? 4 : 0);
	if (((key->flgs & FLAG_LOCK_C) && (state & CLKED))
		|| ((key->flgs & FLAG_LOCK_N) && (state & NLKED)) )
		i ^= 1;

	action = key->map[i];
	if (up) {	/* break: key released */
		if (key->spcl & (0x80 >> i)) {
			/* special keys */
			switch (action) {
			case LSHA:
				if (state & SHIFTAON) {
					set_lockkey_state(state, ALK);
					state &= ~ALKDOWN;
				}
				action = LSH;
				/* FALL THROUGH */
			case LSH:
				state &= ~SHIFTS1;
				break;
			case RSHA:
				if (state & SHIFTAON) {
					set_lockkey_state(state, ALK);
					state &= ~ALKDOWN;
				}
				action = RSH;
				/* FALL THROUGH */
			case RSH:
				state &= ~SHIFTS2;
				break;
			case LCTRA:
				if (state & SHIFTAON) {
					set_lockkey_state(state, ALK);
					state &= ~ALKDOWN;
				}
				action = LCTR;
				/* FALL THROUGH */
			case LCTR:
				state &= ~CTLS1;
				break;
			case RCTRA:
				if (state & SHIFTAON) {
					set_lockkey_state(state, ALK);
					state &= ~ALKDOWN;
				}
				action = RCTR;
				/* FALL THROUGH */
			case RCTR:
				state &= ~CTLS2;
				break;
			case LALTA:
				if (state & SHIFTAON) {
					set_lockkey_state(state, ALK);
					state &= ~ALKDOWN;
				}
				action = LALT;
				/* FALL THROUGH */
			case LALT:
				state &= ~ALTS1;
				break;
			case RALTA:
				if (state & SHIFTAON) {
					set_lockkey_state(state, ALK);
					state &= ~ALKDOWN;
				}
				action = RALT;
				/* FALL THROUGH */
			case RALT:
				state &= ~ALTS2;
				break;
			case ASH:
				state &= ~AGRS1;
				break;
			case META:
				state &= ~METAS1;
				break;
			case NLK:
				state &= ~NLKDOWN;
				break;
			case CLK:
#ifndef PC98
				state &= ~CLKDOWN;
#else
				state &= ~CLKED;
				i = state & LOCK_MASK;
				ioctl(0, KDSETLED, (caddr_t)&i); /* XXX */
#endif
				break;
			case SLK:
				state &= ~SLKDOWN;
				break;
			case ALK:
				state &= ~ALKDOWN;
				break;
			}
			priv->kbd_state = state & ~SHIFTAON;
			return (SPCLKEY | RELKEY | action);
		}
		/* release events of regular keys are not reported */
		priv->kbd_state &= ~SHIFTAON;
		return NOKEY;
	} else {	/* make: key pressed */
		state &= ~SHIFTAON;
		if (key->spcl & (0x80 >> i)) {
			/* special keys */
			switch (action) {
			/* LOCKING KEYS */
			case NLK:
				set_lockkey_state(state, NLK);
				break;
			case CLK:
#ifndef PC98
				set_lockkey_state(state, CLK);
#else
				state |= CLKED;
				i = state & LOCK_MASK;
				ioctl(0, KDSETLED, (caddr_t)&i); /* XXX */
#endif
				break;
			case SLK:
				set_lockkey_state(state, SLK);
				break;
			case ALK:
				set_lockkey_state(state, ALK);
				break;
			/* NON-LOCKING KEYS */
			case SPSC: case RBT:  case SUSP: case STBY:
			case DBG:  case NEXT: case PREV: case PNC:
#ifdef HALT
			case HALT:
#endif
#ifdef PDWN
			case PDWN:
#endif
#if defined(HALT) || defined(PDWN)
				priv->kbd_accents = 0;
				break;
#endif
			case BTAB:
				priv->kbd_accents = 0;
				action |= BKEY;
				break;
			case LSHA:
				state |= SHIFTAON;
				action = LSH;
				/* FALL THROUGH */
			case LSH:
				state |= SHIFTS1;
				break;
			case RSHA:
				state |= SHIFTAON;
				action = RSH;
				/* FALL THROUGH */
			case RSH:
				state |= SHIFTS2;
				break;
			case LCTRA:
				state |= SHIFTAON;
				action = LCTR;
				/* FALL THROUGH */
			case LCTR:
				state |= CTLS1;
				break;
			case RCTRA:
				state |= SHIFTAON;
				action = RCTR;
				/* FALL THROUGH */
			case RCTR:
				state |= CTLS2;
				break;
			case LALTA:
				state |= SHIFTAON;
				action = LALT;
				/* FALL THROUGH */
			case LALT:
				state |= ALTS1;
				break;
			case RALTA:
				state |= SHIFTAON;
				action = RALT;
				/* FALL THROUGH */
			case RALT:
				state |= ALTS2;
				break;
			case ASH:
				state |= AGRS1;
				break;
			case META:
				state |= METAS1;
				break;
			default:
				/* is this an accent (dead) key? */
				priv->kbd_state = state;
				if (action >= F_ACC && action <= L_ACC) {
					action = save_accent_key(priv, action,
							 &priv->kbd_accents);
					switch (action) {
					case NOKEY:
					case ERRKEY:
						return action;
					default:
						if (state & METAS)
							return (action | MKEY);
						else
							return action;
					}
					/* NOT REACHED */
				}
				/* other special keys */
				if (priv->kbd_accents > 0) {
					priv->kbd_accents = 0;
					return ERRKEY;
				}
				if (action >= F_FN && action <= L_FN)
					action |= FKEY;
				/* XXX: return fkey string for the FKEY? */
				return (SPCLKEY | action);
			}
			priv->kbd_state = state;
			return (SPCLKEY | action);
		} else {
			/* regular keys */
			priv->kbd_state = state;
			if (priv->kbd_accents > 0) {
				/* make an accented char */
				action = make_accent_char(priv, action,
							&priv->kbd_accents);
				if (action == ERRKEY)
					return action;
			}
			if (state & METAS)
				action |= MKEY;
			return action;
		}
	}
	/* NOT REACHED */
}

gii_event_mask
GII_vgl_key2event(struct gii_input *inp, int keycode)
{
	gii_vgl_priv *priv = VGL_PRIV(inp);
	gii_event ev;
	int ret, up, action;

	_giiEventBlank(&ev, sizeof(gii_key_event));

	if ((up = keycode & KBD_KEY_RELEASED)) {
		ev.any.type = evKeyRelease;
		priv->prev_keycode = 0;
		ret = emKeyRelease;
	} else {
		if (priv->prev_keycode == keycode) {
			ev.any.type = evKeyRepeat;
			ret = emKeyRepeat;
		} else {
			ev.any.type = evKeyPress;
			priv->prev_keycode = keycode;
			ret = emKeyPress;
		}
	}
	keycode &= ~KBD_KEY_RELEASED;

	ev.any.size = sizeof(gii_key_event);
	ev.any.origin = inp->origin;

	/* The scancode is a unique number */
	ev.key.button = keycode;
	ev.key.label = ggi_labels[keycode];

	if ((GII_KTYP(ev.key.label) == GII_KT_MOD) &&
		(ev.key.type == evKeyRepeat)) {
			return evNothing;
	}

	switch ((action = GII_vgl_genaction(priv, up, keycode))) {
	case NOKEY:
	case ERRKEY:			/* Errorneous keys are ignored */
		return evNothing;
	default:
		break;
	}

	ev.key.sym = (u_char)action;
	ev.key.modifiers = (((priv->kbd_state & SHIFTS) ? GII_MOD_SHIFT : 0) |
			((priv->kbd_state & CTLS) ? GII_MOD_CTRL : 0) |
			((priv->kbd_state & ALTS1) ? GII_MOD_ALT : 0) |
			((priv->kbd_state & ALTS2) ? GII_MOD_ALTGR : 0) |
			((priv->kbd_state & METAS) ? GII_MOD_META : 0) |
			((priv->kbd_state & NLKDOWN) ? GII_MOD_NUM : 0) |
			((priv->kbd_state & SLKDOWN) ? GII_MOD_SCROLL : 0) |
			((priv->kbd_state & CLKDOWN) ? GII_MOD_CAPS : 0));

	_giiEvQueueAdd(inp, &ev);
	
	return ret;
}
