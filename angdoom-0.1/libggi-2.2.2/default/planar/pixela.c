/* $Id: pixela.c,v 1.2 2005/07/30 11:40:02 cegger Exp $
******************************************************************************

   Planar pixel handling.

   Copyright (C) 1998 Andrew Apted   [andrew@ggi-project.org]

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

#include "pllib.h"


#define PIXEL_WADDR(vis,x,y)  \
	((uint16_t *) ((uint8_t *) LIBGGI_CURWRITE(vis) +  \
		(y) * LIBGGI_W_PLAN(vis).next_line) + ((x) >> 4))

#define PIXEL_RADDR(vis,x,y)  \
	((uint16_t *) ((uint8_t *) LIBGGI_CURREAD(vis) +  \
		(y) * LIBGGI_R_PLAN(vis).next_line) + ((x) >> 4))


int GGI_pl_putpixel_nca(ggi_visual *vis, int x, int y, ggi_pixel col)
{
	uint16_t *dest;
	uint16_t mask = 0x8000 >> (x & 15);
	int plane_add = LIBGGI_W_PLAN(vis).next_plane / 2;
	int i;

	PREPARE_FB(vis);

	dest = PIXEL_WADDR(vis, x, y);

	for (i=GT_DEPTH(LIBGGI_GT(vis)); i > 0; i--) {

		if (col & 1) {
			*dest |=  mask;
		} else {
			*dest &= ~mask;
		}

		col >>= 1; dest += plane_add;
	}
	
	return 0;
}


int GGI_pl_getpixela(ggi_visual *vis, int x, int y, ggi_pixel *pixel)
{
	ggi_pixel col = 0;
	uint16_t *dest;
	uint16_t shift = 15 - (x & 15);
	int plane_add = LIBGGI_R_PLAN(vis).next_plane / 2;
	int i, depth = GT_DEPTH(LIBGGI_GT(vis));

	PREPARE_FB(vis);

	dest = PIXEL_RADDR(vis, x, y);

	for (i=0; i < depth; i++) {

		col |= ((*dest >> shift) & 1) << i;

		dest += plane_add;
	}

	*pixel = col;

	return 0;
}
