/* $Id: input.c,v 1.21 2005/08/04 12:43:26 cegger Exp $
******************************************************************************

   DirectX inputlib

   Copyright (C) 1999-2000 John Fortin		[fortinj@ibm.net]
   Copyright (C) 2000      Marcus Sundberg	[marcus@ggi-project.org]
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

#include "config.h"
#include <ggi/gg.h>

#include <dxinput.h>
#include <wctype.h>
#include <ctype.h>

#include "dxinputkb.h"

#define GII_DI_LSHIFT (1<<0)
#define GII_DI_RSHIFT (1<<1)
#define GII_DI_LCTRL  (1<<2)
#define GII_DI_RCTRL  (1<<3)
#define GII_DI_LALT   (1<<4)
#define GII_DI_RALT   (1<<5)
#define GII_DI_SHIFT (GII_DI_LSHIFT | GII_DI_RSHIFT)
#define GII_DI_CTRL  (GII_DI_LCTRL  | GII_DI_RCTRL)
#define GII_DI_ALT   (GII_DI_LALT   | GII_DI_RALT)

typedef struct digii
{
	HANDLE hWnd;
	HINSTANCE hInstance;
} DIGII, *lpDIGII;		/* End structure */


static void
get_system_keyboard_repeat(gii_di_priv *priv)
{
	int iKeyDelay;
	DWORD dwKeySpeed;

	if(SystemParametersInfo(SPI_GETKEYBOARDDELAY, 0, &iKeyDelay, 0))
		priv->repeat_delay = 250000 * (iKeyDelay + 1);
	else
		/* Default to .5 seconds */
		priv->repeat_delay = 500000;

	if(SystemParametersInfo(SPI_GETKEYBOARDSPEED, 0, &dwKeySpeed, 0))
		priv->repeat_speed = 11825 * (31 - dwKeySpeed) + 33333;
	else
		/* Default to 25 Hz */
		priv->repeat_speed = 40000;

	DPRINT("input-directx: key delay %d us, key repeat %d us.\n",
	       priv->repeat_delay,
	       priv->repeat_speed);
}

/* Caps lock does not seem to work. Why the h*ll not? */

static void
set_key_state(BYTE *keystate, uint32_t modifiers)
{
	keystate[VK_SHIFT]   = (modifiers & GII_MOD_SHIFT) ? 0x80 : 0;
	keystate[VK_CONTROL] = (modifiers & GII_MOD_CTRL)  ? 0x80 : 0;
	keystate[VK_MENU]    = (modifiers & GII_MOD_ALT)   ? 0x80 : 0;
	keystate[VK_CAPITAL] = (modifiers & GII_MOD_CAPS)  ? 0x81 : 0;
}

/* Eliminate UTF-16 surrogate pairs. */

static int
utf16_to_utf32(WCHAR *utf16, int count16, uint32_t *utf32)
{
	int count32 = 0;

	if(count16 <= 0)
		return count16;

	while(count16 > 0) {
		if((*utf16 & 0xf800) != 0xd800)
			*utf32++ = *utf16++;
		else if((*utf16 & 0xfc00) == 0xd800) {
			/* high surrogate */
			*utf32 = (*utf16++ & 0x3ff) << 10;
			if(--count16 < 1)
				/* no low surrogate */
				return 0;
			if((*utf16 & 0xfc00) != 0xdc00)
				/* not a low surrogate */
				return 0;
			*utf32++ |= *utf16++ & 0x3ff;
		}
		else /* if((*utf16 & 0xfc00) == 0xdc00) */
			/* unexpected low surrogate */
			return 0;

		--count16;
		++count32;
	}

	return count32;
}

static gii_event_mask
send_synthetic_key(gii_input *inp, uint32_t sym)
{
	gii_di_priv *priv = DI_PRIV(inp);
	gii_event ev;

	_giiEventBlank(&ev, sizeof(gii_key_event));
	ev.any.size      = sizeof(gii_key_event);
	ev.any.origin    = priv->originkey;
	ev.key.modifiers = priv->modifiers;
	ev.key.button    = GII_BUTTON_NIL;
	ev.key.type      = evKeyPress;
	ev.key.sym       = sym;
	ev.key.label     = GIIK_VOID;
	_giiEvQueueAdd(inp, &ev);

	_giiEventBlank(&ev, sizeof(gii_key_event));
	ev.any.size      = sizeof(gii_key_event);
	ev.any.origin    = priv->originkey;
	ev.key.modifiers = priv->modifiers;
	ev.key.button    = GII_BUTTON_NIL;
	ev.key.type      = evKeyRelease;
	ev.key.sym       = sym;
	ev.key.label     = GIIK_VOID;
	_giiEvQueueAdd(inp, &ev);

	return emKeyPress | emKeyRelease;
}

/* A mess since ToUnicode keeps state with regard to dead keys,
 * and is also used by the windows message translation code.
 */

/* Define MYWAY to get dead keys with unmodified not dead variant
 * in label and dead key in sym, instead of unmodified dead variant
 * in label and GIIK_VOID in sym.
 */
/*#define MYWAY*/

static gii_event_mask
map_key(gii_input *inp, uint32_t scan, uint32_t *sym, uint32_t *label)
{
	gii_di_priv *priv = DI_PRIV(inp);
	UINT vk;
	static BYTE keystate[256];
	WCHAR utf16[6];
	uint32_t utf32[2];
	int res;
	gii_event_mask ret = 0;

	set_key_state(keystate, priv->modifiers);

	vk = MapVirtualKey(scan, 1);
	res = ToUnicode(vk, scan, keystate, utf16, 5, 0);
	res = utf16_to_utf32(utf16, res, utf32);
	switch(res) {
	case 0: /* fall back to hard coded values... */
		/*printf("no unicode translation\n");fflush(stdout);*/
		*sym = dx_kb[scan];
		*label = dx_kb[scan];
		break;
	case -1: /* We're not in sync. Sync up. */
		res = ToUnicode(vk, scan, keystate, utf16, 5, 0);
		res = utf16_to_utf32(utf16, res, utf32);
		/*if(res <= 0) {
			if(res == 0)
				printf("What? Suddenly no translation\n");
			else
				printf("What? Dead followed by dead...\n");
			fflush(stdout);
		}*/
		/* Should be in sync now. */
		/* Fall through... */
	default: /* We're in sync. Good. */
		if(priv->dead) {
			/* former key was dead. Don't trust initial call. */
			/* Replay the dead key. */
			set_key_state(keystate, priv->dead_mod);
			res = ToUnicode(priv->dead_vk, priv->dead_scan,
					keystate, utf16, 5, 0);
			res = utf16_to_utf32(utf16, res, utf32);
			/*if(res != -1) {
				printf("not dead, racy as hell.\n");
				fflush(stdout);
			}*/

			/* And restore the keystate for this key. */
			set_key_state(keystate, priv->modifiers);
		}
		/* We're in sync, and any former dead key is replayed */
		priv->dead = 0;
		res = ToUnicode(vk, scan, keystate, utf16, 5, 0);
		res = utf16_to_utf32(utf16, res, utf32);
		switch(res) {
		case -1: /* Dead */
			priv->dead = 1;
			priv->dead_vk = vk;
			priv->dead_scan = scan;
			priv->dead_mod = priv->modifiers;
			*sym = GIIK_VOID;
			break;
		case 0: /* Aiee, suddely no translation */
			/*printf("What? Suddely no translation.\n");
			fflush(stdout);*/
			*sym = GIIK_VOID;
			break;
		case 2:
			ret |= send_synthetic_key(inp, utf32[0]);
			/* fall through */
		case 1:
			*sym = utf32[res - 1];
			break;
		}
		/* again to get unshifted variant / no dead key variant. */
		set_key_state(keystate, 0);
		res = ToUnicode(vk, scan, keystate, utf16, 5, 0);
		res = utf16_to_utf32(utf16, res, utf32);
		if(res == -1) {
			/* dead, do it once more... */
			res = ToUnicode(vk, scan, keystate, utf16, 5, 0);
			res = utf16_to_utf32(utf16, res, utf32);
		}
		if(res <= 0) {
			/*if(res == 0)
				printf("What? Suddenly no translation...\n");
			else
				printf("What? Dead followed by dead.\n");
			fflush(stdout);*/
			*label = GIIK_VOID;
			break;
		}
		*label = towupper(utf32[res - 1]);

		if(priv->dead) {
#ifdef MYWAY
			*sym = utf32[0];
			switch(*sym)
#else
//			*label = utf32[res - 1];
			switch(*label)
#endif
			{
			case GIIUC_Acute:
			case GIIUC_Cedilla:
			case GIIUC_Circumflex:
			case GIIUC_Diaeresis:
			case GIIUC_Grave:
			case GIIUC_Tilde:
			case GIIUC_Macron:
#ifdef MYWAY
				*sym = GII_KEY(GII_KT_DEAD, GII_KVAL(*sym));
#else
				*label = GII_KEY(GII_KT_DEAD,
						 GII_KVAL(*label));
				*sym = GIIK_VOID;
#endif
				break;
			}
		}
		break;
	}

	/* Don't care whether modifier syms are left or right */
	if(GII_KTYP(*sym) == GII_KT_MOD)
		*sym &= ~GII_KM_RIGHT;

	/* Fix caps lock modifier */
	if(!(priv->modifiers & GII_MOD_CAPS)
	   ^ !(priv->modifiers & GII_MOD_SHIFT))
		*sym = towupper(*sym);

	return ret;
}

static gii_event_mask
GII_send_key(gii_input *inp, uint32_t scan, uint8_t press)
{
	gii_di_priv *priv = DI_PRIV(inp);
	gii_event ev;
	gii_event_mask ret = 0;

	_giiEventBlank(&ev, sizeof(gii_key_event));

	ev.any.size = sizeof(gii_key_event);
	ev.any.origin = priv->originkey;
	ev.key.modifiers = priv->modifiers;
	ev.key.button = scan;
	if (press) {
		if (press == 2) {
			ev.any.type = evKeyRepeat;
			ret |= emKeyRepeat;
		}
		else {
			ev.any.type = evKeyPress;
			ret |= emKeyPress;
		}
		/* Store sym & label for release event */
		ret |= map_key(inp, scan, &ev.key.sym, &ev.key.label);
		priv->symlabel[scan][0] = ev.key.sym;
		priv->symlabel[scan][1] = ev.key.label;

		if(ev.key.label == GIIK_CapsLock)
			priv->modifiers ^= GII_MOD_CAPS;
	} else {
		ev.any.type = evKeyRelease;
		if(priv->symlabel[scan][0] != GIIK_NIL) {
			/* Retreive stored sym & label */
			ev.key.sym   = priv->symlabel[scan][0];
			ev.key.label = priv->symlabel[scan][1];
			priv->symlabel[scan][0] = GIIK_NIL;
			priv->symlabel[scan][1] = GIIK_NIL;
			ret |= emKeyRelease;
		}
		/* else, Eeek, double release */
	}

	if(ret)
		_giiEvQueueAdd(inp, &ev);

	return ret;
}


static gii_event_mask
GII_send_ptr(gii_input *inp, int relative,
	     uint32_t type,  uint32_t data,
	     uint32_t type2, uint32_t data2)
{
	gii_di_priv *priv = DI_PRIV(inp);
	gii_event ev;

	switch (type) {
	case DIMOFS_X:		/* Mouse X movement */
	case DIMOFS_Y:		/* Mouse Y Movement */
	case DIMOFS_Z:		/* Mouse Z Movement (wheel?) */
		_giiEventBlank(&ev, sizeof(gii_pmove_event));
		ev.any.size = sizeof(gii_pmove_event);
		ev.any.type = relative ? evPtrRelative : evPtrAbsolute;
		ev.any.origin = priv->originptr;
		if(type == DIMOFS_X)
			ev.pmove.x = data;
		else if(type == DIMOFS_Y)
			ev.pmove.y = data;
		else
			ev.pmove.wheel = data;
		if((type == DIMOFS_X) && (type2 == DIMOFS_Y))
			ev.pmove.y = data2;
		_giiEvQueueAdd(inp, &ev);
		return relative ? emPtrRelative : emPtrAbsolute;

	case DIMOFS_BUTTON0:	/* Key Press/Release */
	case DIMOFS_BUTTON1:
	case DIMOFS_BUTTON2:
	case DIMOFS_BUTTON3:
#ifdef DIMOFS_BUTTON4
	case DIMOFS_BUTTON4:
	case DIMOFS_BUTTON5:
	case DIMOFS_BUTTON6:
	case DIMOFS_BUTTON7:
#endif /* DIMOFS_BUTTON4 */
		_giiEventBlank(&ev, sizeof(gii_pbutton_event));
		ev.any.size = sizeof(gii_pbutton_event);
		ev.any.type = data ? evPtrButtonPress : evPtrButtonRelease;
		ev.any.origin = priv->originptr;
		ev.pbutton.button = GII_PBUTTON_(1 + type - DIMOFS_BUTTON0);
		_giiEvQueueAdd(inp, &ev);
		return data ? emPtrButtonPress : emPtrButtonRelease;
	}			/* End switch */

	return 0;		/* Should never get here */
}


static gii_event_mask
GII_send_joy(gii_input *inp, gii_di_priv_dev *dev, uint32_t type, uint32_t data)
{
	gii_event ev;

	if (type >= DIJOFS_BUTTON(0) && type <= DIJOFS_BUTTON(31)) {
		_giiEventBlank(&ev, sizeof(gii_key_event));
		ev.any.size = sizeof(gii_key_event);
		ev.any.type = data ? evKeyPress : evKeyRelease;
		ev.any.origin = dev->origin;
		ev.key.modifiers = 0;
		ev.key.button = 1 + type - DIJOFS_BUTTON(0);
		ev.key.sym = GIIK_VOID;
		ev.key.label = GIIK_VOID;
		_giiEvQueueAdd(inp, &ev);
		return data ? emKeyPress : emKeyRelease;
	}

	if (type >= DIJOFS_POV(0) && type <= DIJOFS_POV(3)) {
		int povnr = (type - DIJOFS_POV(0)) / sizeof(DWORD);
		int povbtn = dev->devinfo.num_buttons - dev->povs + povnr;
		int povaxis = dev->devinfo.num_axes - dev->povs + povnr;
		if ((data & 0xffff) == 0xffff) {
			/* POV released */
			if (!dev->btn[povbtn])
				return 0;
			dev->btn[povbtn] = 0;
			_giiEventBlank(&ev, sizeof(gii_key_event));
			ev.any.size = sizeof(gii_key_event);
			ev.any.type = evKeyRelease;
			ev.any.origin = dev->origin;
			ev.key.modifiers = 0;
			ev.key.button = 1 + povbtn;
			ev.key.sym = GIIK_VOID;
			ev.key.label = GIIK_VOID;
			_giiEvQueueAdd(inp, &ev);
			return emKeyRelease;
		}
		_giiEventBlank(&ev, sizeof(gii_val_event));
		ev.any.size = sizeof(gii_val_event);
		ev.any.type = evValAbsolute;
		ev.any.origin = dev->origin;
		ev.val.first = povaxis;
		ev.val.count = 1;
		ev.val.value[0] = data;
		_giiEvQueueAdd(inp, &ev);
		if (!dev->btn[povbtn]) {
			/* First POV direction */
			dev->btn[povbtn] = 128;
			_giiEventBlank(&ev, sizeof(gii_key_event));
			ev.any.size = sizeof(gii_key_event);
			ev.any.type = evKeyPress;
			ev.any.origin = dev->origin;
			ev.key.modifiers = 0;
			ev.key.button = 1 + povbtn;
			ev.key.sym = GIIK_VOID;
			ev.key.label = GIIK_VOID;
			_giiEvQueueAdd(inp, &ev);
			return emValAbsolute | emKeyPress;
		}
		return emValAbsolute;
	}

	_giiEventBlank(&ev, sizeof(gii_val_event));
	ev.any.size = sizeof(gii_val_event);
	ev.any.type = evValAbsolute;
	ev.any.origin = dev->origin;
	ev.val.first = type;
	ev.val.count = 1;
	ev.val.value[0] = data;
	_giiEvQueueAdd(inp, &ev);
	return emValAbsolute;
}

/* Don't add more than 1000000 microseconds */
static inline void
tv_add_usec(struct timeval *tv, uint32_t usec)
{
	tv->tv_usec += usec;
	if (tv->tv_usec >= 1000000) {
		++tv->tv_sec;
		tv->tv_usec -= 1000000;
	}
}

/* return 1 if tv1 is later than tv2, -1 if tv2 is later,
 * and 0 if equal
 */
static inline int
tv_compare(struct timeval *tv1, struct timeval *tv2)
{
	if (tv1->tv_sec > tv2->tv_sec)
		return 1;
	if (tv1->tv_sec < tv2->tv_sec)
		return -1;
	if (tv1->tv_usec > tv2->tv_usec)
		return 1;
	if (tv1->tv_usec < tv2->tv_usec)
		return -1;
	return 0;
}

static gii_event_mask
GII_di_poll(gii_input *inp, void *arg)
{
	gii_di_priv *priv = DI_PRIV(inp);
	gii_di_priv_dev *dev;
	gii_di_priv_obj *obj;
	DWORD KB_Elements;
	DWORD Ptr_Elements;
	DWORD Joy_Elements;
	POINT CurPos;
	RECT WinSize;
	uint32_t i, j;
	uint8_t KB_Buffer[2 * GII_DX_BUFFER_SIZE];
	DWORD Ptr_Buffer[2 * GII_DX_BUFFER_SIZE];
	DWORD Joy_Buffer[2 * GII_DX_BUFFER_SIZE];
	gii_event_mask return_mask = 0;
	struct timeval now = { 0, 0 };
	int sent_abs;

	_gii_dx_GetKBInput(priv, &KB_Elements, (char *) KB_Buffer);
	if (priv->repeat_key != -1)
		ggCurTime(&now);
	for (i = 0; i < KB_Elements; i++) {
		if (KB_Buffer[2 * i + 1] == 'D') {
			if (now.tv_sec == 0)
				ggCurTime(&now);
			DPRINT("gii_send_key: poll(%p);\n", inp);
			return_mask |= GII_send_key(inp, KB_Buffer[2 * i], 1);

			priv->repeat_key = -1;
			switch (dx_kb[KB_Buffer[2 * i]]) {
			case GIIK_ShiftR: priv->di_modifiers |= GII_DI_RSHIFT;
				break;
			case GIIK_ShiftL: priv->di_modifiers |= GII_DI_LSHIFT;
				break;
			case GIIK_CtrlR:  priv->di_modifiers |= GII_DI_RCTRL;
				break;
			case GIIK_CtrlL:  priv->di_modifiers |= GII_DI_LCTRL;
				break;
			case GIIK_AltR:   priv->di_modifiers |= GII_DI_RALT;
				break;
			case GIIK_AltL:   priv->di_modifiers |= GII_DI_LALT;
				break;
			}
			if (GII_KTYP(dx_kb[KB_Buffer[2 * i]]) != GII_KT_MOD) {
				priv->repeat_key = KB_Buffer[2 * i];
				priv->repeat_at = now;
				tv_add_usec(&priv->repeat_at,
					priv->repeat_delay);
			}
		} else {
			return_mask |= GII_send_key(inp, KB_Buffer[2 * i], 0);

			switch (dx_kb[KB_Buffer[2 * i]]) {
			case GIIK_ShiftR: priv->di_modifiers &= ~GII_DI_RSHIFT;
				break;
			case GIIK_ShiftL: priv->di_modifiers &= ~GII_DI_LSHIFT;
				break;
			case GIIK_CtrlR:  priv->di_modifiers &= ~GII_DI_RCTRL;
				break;
			case GIIK_CtrlL:  priv->di_modifiers &= ~GII_DI_LCTRL;
				break;
			case GIIK_AltR:   priv->di_modifiers &= ~GII_DI_RALT;
				break;
			case GIIK_AltL:   priv->di_modifiers &= ~GII_DI_LALT;
				break;
			}
			if (priv->repeat_key == KB_Buffer[2 * i])
				priv->repeat_key = -1;
		}

		if(priv->di_modifiers & GII_DI_SHIFT)
			priv->modifiers |= GII_MOD_SHIFT;
		else
			priv->modifiers &= ~GII_MOD_SHIFT;
		if(priv->di_modifiers & GII_DI_CTRL)
			priv->modifiers |= GII_MOD_CTRL;
		else
			priv->modifiers &= ~GII_MOD_CTRL;
		if(priv->di_modifiers & GII_DI_ALT)
			priv->modifiers |= GII_MOD_ALT;
		else
			priv->modifiers &= ~GII_MOD_ALT;
	}
	if ((inp->curreventmask & emKeyRepeat) && priv->repeat_key != -1) {
		i = 10;
		while (--i && tv_compare(&now, &priv->repeat_at) >= 0) {
			return_mask |= GII_send_key(inp, priv->repeat_key, 2);
			tv_add_usec(&priv->repeat_at, priv->repeat_speed);
		}
		if (!i) {
			/* Don't loop too much here in case someone
			 * changed the system clock or something...
			 */
			priv->repeat_at = now;
			tv_add_usec(&priv->repeat_at, priv->repeat_speed);
		}
	}

	_gii_dx_GetPtrInput(priv, &Ptr_Elements, Ptr_Buffer);
	if(Ptr_Elements && (inp->curreventmask & emPtrAbsolute)) {
		GetCursorPos(&CurPos);
		ScreenToClient(priv->hWnd, &CurPos);
		GetClientRect(priv->hWnd, &WinSize);
		if (CurPos.x < 0)
			CurPos.x = 0;
		if (CurPos.y < 0)
			CurPos.y = 0;
		if (CurPos.x >= WinSize.right)
			CurPos.x = WinSize.right - 1;
		if (CurPos.y >= WinSize.bottom)
			CurPos.y = WinSize.bottom - 1;
	}
	sent_abs = 0;
	for (i = 0; i < Ptr_Elements; i++) {
		switch (Ptr_Buffer[2 * i]) {
		case DIMOFS_X:
		case DIMOFS_Y:
			if (!sent_abs
			    && (inp->curreventmask & emPtrAbsolute)) {
				/* abs; must send both x and y */
				return_mask |= GII_send_ptr(inp, 0,
					DIMOFS_X, CurPos.x,
					DIMOFS_Y, CurPos.y);
				sent_abs = 1;
			}

			if(!(inp->curreventmask & emPtrRelative))
				break;
			if((i+1 < Ptr_Elements)
			   && (Ptr_Buffer[2*i] == DIMOFS_X)
			   && (Ptr_Buffer[2*i+2] == DIMOFS_Y)) {
			   	/* consolidate dx and dy */
				return_mask |= GII_send_ptr(inp, 1,
					DIMOFS_X, Ptr_Buffer[2*i+1],
					DIMOFS_Y, Ptr_Buffer[2*i+3]);
				++i;
			}
			else
				return_mask |= GII_send_ptr(inp, 1,
					Ptr_Buffer[2*i], Ptr_Buffer[2*i+1],
					DIMOFS_X, 0);
			break;
		case DIMOFS_Z:
			if(inp->curreventmask & emPtrRelative) {
				return_mask |= GII_send_ptr(inp, 1,
					DIMOFS_Z, Ptr_Buffer[2*i+1],
					DIMOFS_X, 0);
			}
			break;
		case DIMOFS_BUTTON0:
		case DIMOFS_BUTTON1:
		case DIMOFS_BUTTON2:
		case DIMOFS_BUTTON3:
#ifdef DIMOFS_BUTTON4
		case DIMOFS_BUTTON4:
		case DIMOFS_BUTTON5:
		case DIMOFS_BUTTON6:
		case DIMOFS_BUTTON7:
#endif /* DIMOFS_BUTTON4 */
			if(Ptr_Buffer[2 * i + 1] & 0x80) {
				if(!(inp->curreventmask & emPtrButtonPress))
					break;
			}
			else
				if(!(inp->curreventmask & emPtrButtonRelease))
					break;
			return_mask |= GII_send_ptr(inp, 0, Ptr_Buffer[2 * i],
					Ptr_Buffer[2 * i + 1] & 0x80,
					DIMOFS_Z, 0);
			break;
		}
	}

	for(dev = priv->devs; dev; dev = dev->next) {
		_gii_dx_GetJoyInput(dev, &Joy_Elements, Joy_Buffer);
		for (i = 0; i < Joy_Elements; i++) {
			DWORD ofs = Joy_Buffer[2 * i];
			if (ofs >= DIJOFS_BUTTON(0)
			    && ofs <= DIJOFS_BUTTON(31)) {
				/* Button */
				return_mask |= GII_send_joy(inp, dev, ofs,
						Joy_Buffer[2 * i + 1] & 0x80);
				continue;
			}
			for (j = 0, obj = dev->objs; obj; ++j, obj = obj->next) {
				if (ofs != obj->ofs)
					continue;
				if (ofs < DIJOFS_POV(0)
				    || ofs > DIJOFS_POV(3))
					ofs = j;
				return_mask |= GII_send_joy(inp, dev, ofs,
						Joy_Buffer[2 * i + 1]);
			}
		}
	}

	return return_mask;
}


static int
GII_di_close(gii_input *inp)
{
	gii_di_priv *priv = DI_PRIV(inp);
	gii_di_priv_dev *dev;
	gii_di_priv_obj *obj;
	void *tmp;

	_gii_dx_release(priv);
	for (dev = priv->devs; dev; dev = tmp) {
		if (dev->btn)
			free(dev->btn);
		for (obj = dev->objs; obj; obj = tmp) {
			tmp = obj->next;
			free(obj);
		}
		/* AIEE! The allvalinfos memory is still referred to by the
		 * gii core. Pretty rude/dangerous to free it, but there are
		 * no later notifications so this is the last point at which
		 * it can be freed.
		 */
		if (dev->allvalinfos)
			free(dev->allvalinfos);
		tmp = dev->next;
		free(dev);
	}
	free(priv);

	DPRINT_MISC("input-directx: closed\n");

	return 0;
}


static gii_cmddata_getdevinfo di_devinfo_key =
{
	"Direct Input Keyboard",/* long device name */
	"dik",			/* shorthand */
	emKey,			/* can_generate */
	256,			/* 256 pseudo buttons */
	0			/* no valuators */
};

static gii_cmddata_getdevinfo di_devinfo_ptr =
{
	"Direct Input Mouse",	/* long device name */
	"dim",			/* shorthand */
	emPtrButtonRelease | emPtrButtonPress |
	    emPtrRelative,	/* can_generate */
	GII_NUM_UNKNOWN,	/* number of buttons */
	0			/* no valuators */
};

static void
send_devinfo(gii_input *inp, uint32_t origin, gii_cmddata_getdevinfo *devinfo)
{
	gii_event ev;
	gii_cmddata_getdevinfo *dinfo;
	size_t size = sizeof(gii_cmd_nodata_event)
		+ sizeof(gii_cmddata_getdevinfo);

	_giiEventBlank(&ev, size);

	ev.any.size = size;
	ev.any.type = evCommand;
	ev.any.origin = origin;
	ev.cmd.code = GII_CMDCODE_GETDEVINFO;

	dinfo = (gii_cmddata_getdevinfo *) ev.cmd.data;
	*dinfo = *devinfo;

	_giiEvQueueAdd(inp, &ev);
}

static int
send_valinfo(gii_input *inp, uint32_t origin, gii_di_priv_dev *dev, uint32_t val)
{
	gii_event ev;
	gii_cmddata_getvalinfo *vinfo;
	gii_di_priv_obj *obj;
	uint32_t i;

	if (val == GII_VAL_QUERY_ALL) {
		for (i = 0; i < dev->devinfo.num_axes; ++i)
			send_valinfo(inp, origin, dev, i);
		return 0;
	}

	if (val >= dev->devinfo.num_axes)
		return GGI_EARGINVAL;

	_giiEventBlank(&ev, sizeof(gii_cmd_nodata_event) +
		       sizeof(gii_cmddata_getvalinfo));
	
	ev.any.size   = sizeof(gii_cmd_nodata_event) +
		         sizeof(gii_cmddata_getvalinfo);
	ev.any.type   = evCommand;
	ev.any.origin = origin;
	ev.cmd.code   = GII_CMDCODE_GETVALINFO;

	vinfo = (gii_cmddata_getvalinfo *) ev.cmd.data;

	obj = dev->objs;
	for (i = 0, obj = dev->objs; obj; ++i, obj = obj->next) {
		if (i == val)
			break;
	}
	if (!obj)
		return GGI_ENOTFOUND;

	*vinfo = obj->valinfo;

	return _giiEvQueueAdd(inp, &ev);
}


static int
GIIsendevent(gii_input *inp, gii_event *ev)
{
	gii_di_priv *priv = DI_PRIV(inp);
	gii_di_priv_dev *dev = NULL;

	do {
		if ((ev->any.target & GII_MAINMASK) == inp->origin)
			break;
		if (ev->any.target == GII_EV_TARGET_ALL)
			break;
		if (ev->any.target == priv->originkey)
			break;
		if (ev->any.target == priv->originptr)
			break;

		for (dev = priv->devs; dev; dev = dev->next) {
			if(ev->any.target == dev->origin)
				break;
		}
		if (!dev)
			/* not for us */
			return GGI_EEVNOTARGET;
	} while(0);

	if (ev->any.type != evCommand) {
		return GGI_EEVUNKNOWN;
	}
	if (ev->cmd.code == GII_CMDCODE_GETDEVINFO) {
		if (ev->any.target == priv->originkey) {
			send_devinfo(inp, priv->originkey, &di_devinfo_key);
			return 0;
		}
		if (ev->any.target == priv->originptr) {
			send_devinfo(inp, priv->originptr, &di_devinfo_ptr);
			return 0;
		}
		if (dev) {
			send_devinfo(inp, dev->origin, &dev->devinfo);
			return 0;
		}
		/* GII_EV_TARGET_ALL */
		send_devinfo(inp, priv->originkey, &di_devinfo_key);
		send_devinfo(inp, priv->originptr, &di_devinfo_ptr);
		for(dev = priv->devs; dev; dev = dev->next)
			send_devinfo(inp, dev->origin, &dev->devinfo);
		return 0;
	}
	if (ev->cmd.code == GII_CMDCODE_GETVALINFO) {
		gii_cmddata_getvalinfo *vi;
		vi = (gii_cmddata_getvalinfo *) ev->cmd.data;
		if (ev->any.target == priv->originkey)
			return GGI_EEVNOTARGET;
		if (ev->any.target == priv->originptr)
			return GGI_EEVNOTARGET;
		if (dev)
			return send_valinfo(inp,
					dev->origin, dev, vi->number);
		/* GII_EV_TARGET_ALL */
		for (dev = priv->devs; dev; dev = dev->next)
			send_valinfo(inp, dev->origin, dev, vi->number);
		return 0;
	}
	if (ev->cmd.code == GII_CMDCODE_DXSETTINGCHANGE) {
		if (ev->any.target == priv->originptr)
			return GGI_EEVNOTARGET;
		if (dev)
			return GGI_EEVNOTARGET;
		/* keyboard or GII_EV_TARGET_ALL */
		get_system_keyboard_repeat(priv);
		return 0;
	}
	return GGI_EEVUNKNOWN;	/* Unknown command */
}


EXPORTFUNC int GIIdl_directx(gii_input * inp, const char *args, void *argptr);

int GIIdl_directx(gii_input * inp, const char *args, void *argptr)
{
	gii_di_priv *priv;
	gii_di_priv_dev *dev;
	lpDIGII digii = (lpDIGII) argptr;
	int ret;
	int i;

	DPRINT_LIBS("DXINPUT HWND=%ld hInstance=%ld\n",
		       (long)digii->hWnd, (long)digii->hInstance);

	priv = malloc(sizeof(gii_di_priv));
	if (priv == NULL) {
		return GGI_ENOMEM;
	}

	priv->originkey = _giiRegisterDevice(inp, &di_devinfo_key, NULL);
	if(priv->originkey == 0) {
		free(priv);
		return GGI_ENOMEM;
	}

	priv->originptr = _giiRegisterDevice(inp, &di_devinfo_ptr, NULL);
	if(priv->originptr == 0) {
		free(priv);
		return GGI_ENOMEM;
	}

	priv->hWnd = digii->hWnd;
	priv->devs = NULL;
	priv->pDI = NULL;
	priv->pKeyboard = NULL;
	priv->pMouse = NULL;
	priv->modifiers = 0;
	priv->di_modifiers = 0;
	priv->repeat_key = -1;
	for(i = 0; i < 256; ++i) {
		priv->symlabel[i][0] = GIIK_NIL;
		priv->symlabel[i][1] = GIIK_NIL;
	}

	ret = _gii_dx_InitDirectInput(priv, digii->hWnd, digii->hInstance);
	if (ret != 0) {
		DPRINT_LIBS("input-directx: Unable to grab keyboard\n");
		DPRINT_LIBS("HWND=%ld rc=%d\n", (long)digii->hWnd, ret);
		free(priv);
		return GGI_ENODEVICE;
	}

	inp->targetcan = 0;
	inp->curreventmask = 0;

	for (dev = priv->devs; dev; dev = dev->next) {
		gii_di_priv_obj *obj;

		if (dev->objs) {
			dev->allvalinfos = calloc(dev->devinfo.num_axes,
					sizeof(*dev->allvalinfos));
			if(!dev->allvalinfos)
				continue;
		}

		for (i = 0, obj = dev->objs; obj; ++i, obj = obj->next)
			dev->allvalinfos[i] = obj->valinfo;

		dev->origin = _giiRegisterDevice(inp, &dev->devinfo,
						 dev->allvalinfos);
		if (dev->origin == 0) {
			if (dev->allvalinfos)
				free(dev->allvalinfos);
			free(priv);
			return GGI_ENOMEM;
		}

		inp->targetcan |= dev->devinfo.can_generate;
		inp->curreventmask |= dev->devinfo.can_generate;
	}

	inp->priv = priv;

	inp->GIIsendevent = GIIsendevent;
	inp->GIIeventpoll = GII_di_poll;
	inp->GIIclose = GII_di_close;

	inp->targetcan |= emKey | emPointer;

	/* Don't say both emPtrRelative and emPtrAbsolute... */
	inp->curreventmask |= emKey |
	    emPtrAbsolute |
	    emPtrButtonPress | emPtrButtonRelease;

	inp->flags = GII_FLAGS_HASPOLLED;
	inp->maxfd = 0;

	initkb();

	di_devinfo_ptr.num_buttons = GetSystemMetrics(SM_CMOUSEBUTTONS);

	send_devinfo(inp, priv->originkey, &di_devinfo_key);
	send_devinfo(inp, priv->originptr, &di_devinfo_ptr);
	for (dev = priv->devs; dev; dev = dev->next)
		send_devinfo(inp, dev->origin, &dev->devinfo);

	get_system_keyboard_repeat(priv);

	return 0;
}
