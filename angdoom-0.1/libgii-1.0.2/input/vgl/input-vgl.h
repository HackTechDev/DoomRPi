/* $Id: input-vgl.h,v 1.2 2001/06/17 08:34:31 cegger Exp $
******************************************************************************

   FreeBSD vgl(3) inputlib header

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

#ifndef __INPUT_VGL_H
#define __INPUT_VGL_H

#include <ggi/internal/gii-dl.h>
#include <ggi/internal/gii_debug.h>

/* shift key state */
#define SHIFTS1		(1 << 16)
#define SHIFTS2		(1 << 17)
#define SHIFTS		(SHIFTS1 | SHIFTS2)
#define CTLS1		(1 << 18)
#define CTLS2		(1 << 19)
#define CTLS		(CTLS1 | CTLS2)
#define ALTS1		(1 << 20)
#define ALTS2		(1 << 21)
#define ALTS		(ALTS1 | ALTS2)
#define AGRS1		(1 << 22)
#define AGRS2		(1 << 23)
#define AGRS		(AGRS1 | AGRS2)
#define METAS1		(1 << 24)
#define METAS2		(1 << 25)
#define METAS		(METAS1 | METAS2)
#define NLKDOWN		(1 << 26)
#define SLKDOWN		(1 << 27)
#define CLKDOWN		(1 << 28)
#define ALKDOWN		(1 << 29)
#define SHIFTAON	(1 << 30)

typedef struct {
	int prev_keycode;		/* last pressed keycode */
	int kbd_state;			/* shift/lock key state */
	int kbd_accents;		/* accent key index (> 0) */

	keymap_t kbd_keymap;
	accentmap_t kbd_accentmap;
} gii_vgl_priv;

#define VGL_PRIV(inp)  ((gii_vgl_priv *) inp->priv)

gii_event_mask GII_vgl_poll(struct gii_input *inp, void *arg);
gii_event_mask GII_vgl_key2event(struct gii_input *inp, int keycode);

#endif /* __INPUT_VGL_H */
