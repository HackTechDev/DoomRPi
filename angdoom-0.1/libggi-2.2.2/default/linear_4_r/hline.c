/* $Id$
******************************************************************************

   Graphics library for GGI. Horizontal lines.

   Copyright (C) 1995 Andreas Beck	[becka@ggi-project.org]
   Copyright (C) 1999 Marcus Sundberg	[marcus@ggi-project.org]
   Copyright (C) 2006 Peter Rosin	[peda@lysator.liu.se]

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

#include <string.h>

#include "lin4rlib.h"

/**********************************/
/* draw/get/put a horizontal line */
/**********************************/

static inline void
do_drawhline(ggi_visual *vis, int x, int y, int w)
{
	uint8_t *fb;
	uint8_t fg;

	fb = (uint8_t *) LIBGGI_CURWRITE(vis)+y*LIBGGI_FB_W_STRIDE(vis)+(x/2);
	fg = (uint8_t) LIBGGI_GC_FGCOLOR(vis) | (LIBGGI_GC_FGCOLOR(vis)<<4);
	
	PREPARE_FB(vis);

	/* x is odd.  Read-modify-write right pixel. */
	if (x & 0x01) {
		*fb = (*fb & 0x0f) | (fg & 0xf0);
		fb++;
		w--;
	}
	
	memset(fb, fg, (size_t)(w/2));

	/* Dangling right pixel. */
	if (w & 0x01) {
		fb += w/2;
		*fb = (*fb & 0xf0) | (fg & 0x0f);
	}
}


int GGI_lin4r_drawhline(ggi_visual *vis, int x, int y, int w)
{
	LIBGGICLIP_XYW(vis, x, y, w);

	do_drawhline(vis, x, y, w);
		
	return 0;
}

int GGI_lin4r_drawhline_nc(ggi_visual *vis, int x, int y, int w)
{
	do_drawhline(vis, x, y, w);
		
	return 0;
}

int GGI_lin4r_puthline(ggi_visual *vis, int x, int y, int w, const void *buffer)
{ 
	uint8_t *fb;
	uint16_t color;
	const uint8_t *buf = (const uint8_t *)buffer;

	LIBGGICLIP_XYW_BUFMOD(vis, x, y, w, buf, /2);
	PREPARE_FB(vis);
	
	fb = (uint8_t*)LIBGGI_CURWRITE(vis)+y*LIBGGI_FB_W_STRIDE(vis)+(x/2);
	
	/* Might as well take the memcpy speed up (multiple byte copy). */
	if (!(x & 0x01)) {
		memcpy(fb,buf, (size_t)(w/2));
		if (w & 0x01)
			*(fb+(w/2)) = ( *(fb+(w/2)) & 0xF0)
				| (*(buf+(w/2)) >> 4);
		return 0;
	}
	
	/* x is odd. */
	/* Could be optimized for multiple-byte copy. */

	color = *fb & 0x0f;
	
	for (; w > 0; w -= 2) {
		color |= *(buf++) << 4;
		/* Color is now a 12-bit quantity.  Lower 8 bits hold the
		 * memory to copy.  Upper 4 bits hold the upper 4 bits of the
		 * buffer byte read; we'll need it next loop. */
		*(fb++) = color;
		color >>= 8;
	}
	
	if (!w) {
		*fb = color | (*fb & 0xf0);
	}
	
	return 0;
}


int GGI_lin4r_gethline(ggi_visual *vis,int x,int y,int w,void *buffer)
{ 
	uint8_t *fb;
	uint16_t color;
	uint8_t *buf = (uint8_t *)buffer;

	PREPARE_FB(vis);

	fb = (uint8_t*)LIBGGI_CURREAD(vis)+y*LIBGGI_FB_R_STRIDE(vis)+(x/2);
	
	/* If width is odd, the last 4 bits of the buffer's last
	 * byte will hold an extraneous pixel.  No big deal. */
	
	if (!(x & 0x01)) {
		memcpy(buf, fb, (size_t)((w/2) + (w & 0x01)));
		return 0;
	}
	
	/* x is odd. */
	color = *fb & 0x0f;
	
	for (; w > 1; w -= 2) {
		color |= *(++fb) << 4;
		*(buf++) = color;
		color >>= 8;
	}

	if (w) {
		*(buf++) = color;
	}

	return 0;
}
