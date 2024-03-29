/* $Id: vline.c,v 1.3 2004/12/01 23:08:24 cegger Exp $
******************************************************************************

   SVGAlib target vgagl helper: vertical lines

   Copyright (C) 1998 Marcus Sundberg   [marcus@ggi-project.org]
   Copyright (C) 1997 Jason McMullan    [jmcc@ggi-project.org]
   Copyright (C) 1995 Andreas Beck      [becka@ggi-project.org]

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
#include <ggi/internal/ggi-dl.h>
#include "vgaglvis.h"

/********************************/
/* draw a vertical line */
/********************************/

int GGI_vgagl_drawvline(ggi_visual *vis,int x,int y,int height)
{
	/* Clipping */
	if (x< (LIBGGI_GC(vis)->cliptl.x) ||
	    x>=(LIBGGI_GC(vis)->clipbr.x)) return 0;
	if (y< (LIBGGI_GC(vis)->cliptl.y)) {
		int diff=(LIBGGI_GC(vis)->cliptl.y)-y;
		y     +=diff;
		height-=diff;
	}
	if (y+height>(LIBGGI_GC(vis)->clipbr.y)) {
		height=(LIBGGI_GC(vis)->clipbr.y)-y;
	}
	if (height<1)
		return 0;
	
	gl_line(x,y,x,y+height-1,LIBGGI_GC_FGCOLOR(vis));
	return 0;
}

int GGI_vgagl_drawvline_nc(ggi_visual *vis,int x,int y,int height)
{
	gl_line(x,y,x,y+height-1,LIBGGI_GC_FGCOLOR(vis));	
	return 0;
}

int GGI_vgagl_putvline(ggi_visual *vis,int x,int y,int height,const void *buffer)
{
        /* Clipping */
	if (x< (LIBGGI_GC(vis)->cliptl.x) ||
		x>=(LIBGGI_GC(vis)->clipbr.x)) return 0;
	if (y< (LIBGGI_GC(vis)->cliptl.y)) {
		int diff=(LIBGGI_GC(vis)->cliptl.y)-y;
		y     +=diff;
		height-=diff;
		buffer =((const char *)buffer)+diff;
	}
	if (y+height>(LIBGGI_GC(vis)->clipbr.y)) {
		height=(LIBGGI_GC(vis)->clipbr.y)-y;
	}
	
	if (height>0) {
        gl_putbox(x, y, 1, height, buffer);
	}
	return 0;
}

int GGI_vgagl_getvline(ggi_visual *vis,int x,int y,int height,void *buffer)
{
	gl_getbox(x, y, 1, height, buffer);

	return 0;
}
