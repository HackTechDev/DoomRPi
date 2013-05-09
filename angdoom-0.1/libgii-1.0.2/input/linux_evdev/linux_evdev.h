/* $Id$
******************************************************************************

   Linux evdev inputlib header

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

#include <ggi/internal/gii-dl.h>
#include <ggi/internal/gii_debug.h>

#include <linux/input.h>

/* Compatibility for Linux 2.4
 * EV_SYN not available in 2.4
 */
#if !defined(EV_SYN) && defined(EV_RST)
#define EV_SYN EV_RST
#endif
/* End compat */

#define BITS_PER_LONG (sizeof(long) * 8)
#define NBITS(x) ((((x)-1)/BITS_PER_LONG)+1)
#define OFF(x)  ((x)%BITS_PER_LONG)
#define BIT(x)  (1UL<<OFF(x))
#define LONG(x) ((x)/BITS_PER_LONG)
#define test_bit(bit, array)    ((array[LONG(bit)] >> OFF(bit)) & 1)

typedef struct {
	int fd;
	int eof;
	unsigned long bit[EV_MAX][NBITS(KEY_MAX)];
	gii_cmddata_getdevinfo devinfo;
	gii_cmddata_getvalinfo valinfo[KEY_MAX];
} gii_levdev_priv;

#define LEVDEV_PRIV(inp)  ((gii_levdev_priv *) inp->priv)

gii_event_mask GII_levdev_poll(struct gii_input *inp, void *arg);

uint32_t GII_levdev_key2label(struct gii_input *inp, unsigned short key);
