/* $Id: color.c,v 1.12 2005/07/30 10:58:27 cegger Exp $
******************************************************************************

   SVGAlib target: palette driver

   Copyright (C) 1997 Steve Cheng	[steve@ggi-project.org]
   Copyright (C) 1997 Andreas Beck	[becka@ggi-project.org]
   Copyright (C) 1999 Marcus Sundberg	[marcus@ggi-project.org]

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

#include "config.h"
#include <ggi/internal/ggi_debug.h>
#include <ggi/display/svgalib.h>


int
GGI_svga_setPalette(ggi_visual *vis, size_t start, size_t len, const ggi_color *colormap)
{
	int *vgaptr;
	size_t i;

	APP_ASSERT(colormap != NULL,
			 "GGI_svga_setPalette() - colormap == NULL");

	memcpy(LIBGGI_PAL(vis)->clut.data, colormap, len*sizeof(ggi_color));

	vgaptr = (int *)((uint8_t *)LIBGGI_PAL(vis)->priv + start*3);

	/* vga_setPalette() takes 6-bit r,g,b,
	   so we need to scale ggi_color's 16-bit values. */
	for (i = 0; i < len; i++) {
		*vgaptr = colormap->r >> 10;
		vgaptr++;
		*vgaptr = colormap->g >> 10;
		vgaptr++;
		*vgaptr = colormap->b >> 10;
		vgaptr++;
		colormap++;
	}

	if (!SVGA_PRIV(vis)->ismapped) return 0;

	vga_setpalvec(start, len, ((int*)LIBGGI_PAL(vis)->priv) + start*3);

	return 0;
}

size_t GGI_svga_getPrivSize(ggi_visual_t vis)
{
  return (3 * LIBGGI_PAL(vis)->clut.size * sizeof(int));
}
