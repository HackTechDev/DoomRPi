/* $Id: hline.c,v 1.13 2005/07/30 10:58:22 cegger Exp $
******************************************************************************

   LibGGI - horizontal lines for display-x

   Copyright (C) 1998-1999 Marcus Sundberg	[marcus@ggi-project.org]
   Copyright (C) 2002 Brian S. Julin	        [bri@tull.umassp.edu]

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
#include <ggi/internal/ggi-dl.h>
#include <ggi/display/x.h>

int GGI_X_drawhline_slave(ggi_visual *vis, int x, int y, int w)
{
	ggi_x_priv *priv;
	priv = GGIX_PRIV(vis);

        LIBGGICLIP_XYW(vis, x, y, w);
	priv->slave->opdraw->drawhline_nc(priv->slave, x, y, w);
	GGI_X_DIRTY(vis, x, y, w, 1);
	return 0;
}

int GGI_X_drawhline_nc_slave(ggi_visual *vis, int x, int y, int w)
{
	ggi_x_priv *priv;
	priv = GGIX_PRIV(vis);

	priv->slave->opdraw->drawhline_nc(priv->slave, x, y, w);
	GGI_X_DIRTY(vis, x, y, w, 1);
	return 0;
}

int GGI_X_puthline_slave(ggi_visual *vis, int x, int y, int w, const void *data)
{
	ggi_x_priv *priv;
	priv = GGIX_PRIV(vis);

	/* Use slave's clipping for depth adjustment */
	priv->slave->opdraw->puthline(priv->slave, x, y, w, data);
        LIBGGICLIP_XYW(vis, x, y, w);
	GGI_X_DIRTY(vis, x, y, w, 1);
	return 0;
}

int GGI_X_gethline_slave(ggi_visual *vis, int x, int y, int w, void *data)
{	
	ggi_x_priv *priv;
	priv = GGIX_PRIV(vis);

	/* Slave is always up to date */
	return (priv->slave->opdraw->gethline(priv->slave, x, y, w, data));
}

int GGI_X_drawhline_slave_draw(ggi_visual *vis, int x, int y, int w)
{
	ggi_x_priv *priv;
	priv = GGIX_PRIV(vis);

        LIBGGICLIP_XYW(vis, x, y, w);
	GGI_X_CLEAN(vis, x, y, w, 1);
	priv->slave->opdraw->drawhline_nc(priv->slave, x, y, w);
	y = GGI_X_WRITE_Y;
	GGI_X_LOCK_XLIB(vis);
	XDrawLine(priv->disp, priv->drawable, priv->gc, x, y, x+w-1, y);
	GGI_X_MAYBE_SYNC(vis);
	GGI_X_UNLOCK_XLIB(vis);
	return 0;
}

int GGI_X_drawhline_nc_slave_draw(ggi_visual *vis, int x, int y, int w)
{
	ggi_x_priv *priv;
	priv = GGIX_PRIV(vis);

	GGI_X_CLEAN(vis, x, y, w, 1);
	priv->slave->opdraw->drawhline_nc(priv->slave, x, y, w);
	y = GGI_X_WRITE_Y;
	GGI_X_LOCK_XLIB(vis);
	XDrawLine(priv->disp, priv->drawable, priv->gc, x, y, x+w-1, y);
	GGI_X_MAYBE_SYNC(vis);
	GGI_X_UNLOCK_XLIB(vis);
	return 0;
}

int GGI_X_drawhline_draw(ggi_visual *vis, int x, int y, int w)
{
	ggi_x_priv *priv;
	priv = GGIX_PRIV(vis);

	y = GGI_X_WRITE_Y;
	GGI_X_LOCK_XLIB(vis);
	XDrawLine(priv->disp, priv->drawable, priv->gc, x, y, x+w-1, y);
	GGI_X_MAYBE_SYNC(vis);
	GGI_X_UNLOCK_XLIB(vis);
	return 0;
}

int GGI_X_puthline_draw(ggi_visual *vis, int x, int y, int w, const void *data)
{
        XImage *ximg;
	ggi_x_priv *priv;
	priv = GGIX_PRIV(vis);

	ximg = _ggi_x_create_ximage( vis, (char *)data, w, 1 );
	if (ximg == NULL) return GGI_ENOMEM;

	GGI_X_LOCK_XLIB(vis);
	XPutImage(priv->disp, priv->drawable, priv->gc, ximg,
                  0, 0, x, GGI_X_WRITE_Y, (unsigned)w, 1);
#ifndef HAVE_XINITIMAGE
	XFree(ximg);
#else
	free(ximg);
#endif

	GGI_X_MAYBE_SYNC(vis);
	GGI_X_UNLOCK_XLIB(vis);
        return 0;
}

static int geterror;

static int errorhandler (Display * disp, XErrorEvent * event)
{
	if (event->error_code == BadMatch) geterror = 1;

	return 0;
}

int GGI_X_gethline_draw(ggi_visual *vis, int x, int y, int w, void *data)
{	
	ggi_x_priv *priv;
	XImage *ximg;
	int     (*olderrorhandler) (Display *, XErrorEvent *);
	int ret = 0;
	priv = GGIX_PRIV(vis);

	GGI_X_LOCK_XLIB(vis);
	XSync(priv->disp,0);
	ggLock(_ggi_global_lock);
	geterror = 0;
	olderrorhandler = XSetErrorHandler(errorhandler);
	/* This will cause a BadMatch error when the window is
	   iconified or on another virtual screen... */
#warning honor various ximage format fields here.
#warning 1,2,4-bit support needed.
	ximg = XGetImage(priv->disp, priv->drawable, x, GGI_X_READ_Y,
			 (unsigned)w, 1, AllPlanes, ZPixmap);
	XSync(priv->disp, 0);
	XSetErrorHandler(olderrorhandler);

	if (geterror) {
		ret = -1;
		goto out;
	}

	if (ximg->byte_order == 
#ifdef GGI_LITTLE_ENDIAN
	    LSBFirst
#else
	    MSBFirst
#endif
	    ) goto noswab;

	if (ximg->bits_per_pixel == 16) {
		int j;
		uint8_t *ximgptr;
		ximgptr = (uint8_t *)(ximg->data) +(ximg->xoffset * ximg->bits_per_pixel)/8;
		for (j = 0; j < w * 2; j += 2) {
		  *((uint8_t *)data + j) = *(ximgptr + j + 1);
		  *((uint8_t *)data + j + 1) = *(ximgptr + j);
		}
	}
	else if (ximg->bits_per_pixel == 32) {	
		int j;
		uint8_t *ximgptr;
		ximgptr = (uint8_t *)(ximg->data) +(ximg->xoffset * ximg->bits_per_pixel)/8;
		for (j = 0; j < w * 4; j += 4) {
			*((uint8_t *)data + j) = *(ximgptr + j + 3);
			*((uint8_t *)data + j + 1) = *(ximgptr + j + 2);
			*((uint8_t *)data + j + 2) = *(ximgptr + j + 1);
			*((uint8_t *)data + j + 3) = *(ximgptr + j);
		}
	}
	else {
	noswab:
		memcpy(data, ximg->data,
		       (size_t)((long)w * LIBGGI_PIXFMT(vis)->size)/8);
	}

	XDestroyImage(ximg);
out:
	ggUnlock(_ggi_global_lock);
	GGI_X_UNLOCK_XLIB(vis);

	return ret;
}
