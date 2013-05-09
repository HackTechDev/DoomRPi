/*
******************************************************************************

   xf86dga inputlib - use DGA input events as a LibGII input source

   Copyright (C) 2005	Joseph Crayne		[oh.hello.joe@gmail.com]

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

#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <X11/extensions/xf86dga.h>

#include "config.h"
#include <ggi/gg.h>
#include <ggi/internal/gii-dl.h>
#include <ggi/internal/gii_debug.h>
#include <ggi/input/xf86dga.h>
#include "xev.h"


enum {
	XDGA_DEV_KEY=0,
	XDGA_DEV_MAX
};

typedef struct {
	Display	 *disp;
	int	 screen;
	XComposeStatus	compose_status;
	uint32_t origin[XDGA_DEV_MAX];
	char	 key_vector[32];
	int 	event_base;
	int	error_base;
} xdga_priv;

#define XDGA_PRIV(inp)	((xdga_priv*)((inp)->priv))

static inline void prepare_key_event( gii_input *inp, gii_event *giiev, 
				      int keycode, XDGAEvent *dgaev)
{
	xdga_priv  *priv = XDGA_PRIV(inp);
	XComposeStatus compose_status;
	XKeyEvent xkeyev;
	KeySym xsym;

	giiev->any.size = sizeof(gii_key_event);
	giiev->any.origin = priv->origin[XDGA_DEV_KEY];
	giiev->key.button = keycode - 8;
	XDGAKeyEventToXKeyEvent((XDGAKeyEvent*)dgaev, &xkeyev);
	XLookupString(&xkeyev, NULL, 0, &xsym, &compose_status);
	giiev->key.sym = basic_trans(xsym, 0);
	giiev->key.label = basic_trans(XLookupKeysym(&xkeyev, 0), 1);
	/*
	_gii_xev_trans( &xkeyev, &giiev.key, &compose_status,
			0, &oldcode_dummy );
			*/
}


/* Methods for a using an array of char as a packed binary
 * array */
#define SetFlag( packed, index ) packed[index/8]|=1<<(index&7)
#define ClearFlag( packed, index ) packed[index/8]&=~(1<<(index&7));
#define TestFlag( packed, index ) \
	(priv->key_vector[keycode/8] & (1<<(keycode&7))) 

static gii_event_mask GII_xdga_eventpoll(gii_input *inp, void *arg)
{
	xdga_priv  *priv = XDGA_PRIV(inp);
	XEvent xev;
	XDGAEvent *dgaev = (XDGAEvent *)&xev;
	gii_event giiev;
	int n;
	int rc = 0;
	int dga_event_base = priv->event_base;


	XSync( priv->disp, False );
	
	for(n=XEventsQueued(priv->disp, QueuedAfterReading); n ; n-- ) {

		int keycode;

		XNextEvent(priv->disp, &xev);
		keycode = dgaev->xkey.keycode;

		_giiEventBlank( &giiev, sizeof(gii_event) );

		switch(dgaev->type - dga_event_base ) {

		case KeyPress:
			prepare_key_event(inp, &giiev, keycode, dgaev);
			if( TestFlag( priv->key_vector, keycode) ) {
				giiev.any.type = evKeyRepeat;
				rc |= emKeyRepeat;
			} else {
				giiev.any.type = evKeyPress;
				rc |= emKeyPress;
			}
			/*DPRINT("press(%c)\n", giiev.key.sym);*/
			SetFlag(priv->key_vector, keycode);
			_giiEvQueueAdd(inp, &giiev );
			break;

		case KeyRelease:
			prepare_key_event(inp, &giiev, keycode, dgaev);
			giiev.any.type = evKeyRelease;
			rc |= emKeyRelease;
			/* DPRINT("release(%c)\n", giiev.key.sym);*/
			ClearFlag(priv->key_vector, keycode);
			_giiEvQueueAdd(inp, &giiev);
			break;
		}

	}

	return rc;
}


static int GII_xdga_close(gii_input *inp)
{
	xdga_priv  *priv = XDGA_PRIV(inp);
	
	free(priv);
	
	DPRINT_MISC("GII_xdga_close(%p) called\n", inp);

	return 0;
}

#if 0
static gii_cmddata_getdevinfo mouse_devinfo =
{
	"XFree86-DGA Mouse",/* long device name */
	"dgams",	/* shorthand */
	emPointer,	/* can_generate */
	0,		/* num_buttons	(filled in runtime) */
	0		/* num_axes 	(only for valuators) */
};
#endif

static gii_cmddata_getdevinfo key_devinfo =
{
	"XFree86-DGA Keyboard",/* long device name */
	"dgakb",	/* shorthand */
	emKey,		/* can_generate */
	0,		/* num_buttons	(filled in runtime) */
	0		/* num_axes 	(only for valuators) */
};

/* dev 0 is keyboard, dev 1 is mouse */
static void send_devinfo(gii_input *inp, int dev)
{
	gii_event ev;
	gii_cmddata_getdevinfo *dinfo;
	size_t size;
	xdga_priv  *priv = XDGA_PRIV(inp);

	size = sizeof(gii_cmd_nodata_event)+sizeof(gii_cmddata_getdevinfo);

	_giiEventBlank(&ev, size);
	
	ev.any.size   = size;
	ev.any.type   = evCommand;
	ev.any.origin = priv->origin[dev];
	ev.cmd.code   = GII_CMDCODE_GETDEVINFO;

	dinfo = (gii_cmddata_getdevinfo *) ev.cmd.data;
	switch (dev) {
		/*
	case XDGA_DEV_MOUSE:
		*dinfo = mouse_devinfo;
		break;
		*/
	case XDGA_DEV_KEY:
		*dinfo = key_devinfo;
		break;
	default:
		return;
	}

	_giiEvQueueAdd(inp, &ev);
}


static int GIIsendevent(gii_input *inp, gii_event *ev)
{
	xdga_priv  *priv = XDGA_PRIV(inp);

	if ((ev->any.target & 0xffffff00) != inp->origin &&
	    ev->any.target != GII_EV_TARGET_ALL)
	{
		/* not for us */
		return GGI_EEVNOTARGET;
	}

	if (ev->any.type != evCommand) {
		return GGI_EEVUNKNOWN;
	}

	if (ev->cmd.code == GII_CMDCODE_GETDEVINFO) {

		if (ev->any.target == GII_EV_TARGET_ALL) {
			send_devinfo(inp, XDGA_DEV_KEY);
			/*
			send_devinfo(inp, XDGA_DEV_MOUSE);
			*/

			return 0;
		} else if(ev->any.target==priv->origin[XDGA_DEV_KEY]) {
			send_devinfo(inp, XDGA_DEV_KEY);
			return 0;
		/*
		} else if(ev->any.target==priv->origin[XDGA_DEV_MOUSE]) {
			send_devinfo(inp, XDGA_DEV_MOUSE);
			return 0;
		*/
		} else
			return GGI_EEVNOTARGET;

	}

	return GGI_EEVUNKNOWN;
}


EXPORTFUNC int GIIdl_xf86dga(gii_input *inp, const char *args, void *argptr);

int GIIdl_xf86dga(gii_input *inp, const char *args, void *argptr)
{
	gii_inputxf86dga_arg *xdgaarg = argptr;
	xdga_priv  *priv;
	int minkey, maxkey;

	DPRINT_MISC("GIIdlinit(%p) called for input-dga\n", inp);


	/* TODO: use XOpenDisplay to get a display if one wasnt
	 * provided? */
	if (xdgaarg == NULL || xdgaarg->disp == NULL) {
		return GGI_EARGREQ;
	}

	if ((priv = malloc(sizeof(xdga_priv))) == NULL) {
		return GGI_ENOMEM;
	}


	priv->disp = xdgaarg->disp;
	priv->screen = xdgaarg->screen;

	memset(priv->key_vector,0,sizeof(priv->key_vector));

	inp->priv = priv;

	inp->GIIsendevent = GIIsendevent;
	inp->GIIeventpoll = GII_xdga_eventpoll;
	inp->GIIclose = GII_xdga_close;

	if (0 == (priv->origin[XDGA_DEV_KEY] =
		_giiRegisterDevice(inp,&key_devinfo,NULL)))
	{
		GII_xdga_close(inp);
		return GGI_ENOMEM;
	}

	/*
	if (0 == (priv->origin[XDGA_DEV_MOUSE] =
		_giiRegisterDevice(inp,&mouse_devinfo,NULL)))
	{
		GII_xdga_close(inp);
		return GGI_ENOMEM;
	}
	*/


	inp->targetcan = emKey; /* | emPointer; */
	inp->curreventmask = emKey; /* | emPointer;*/

	inp->maxfd = ConnectionNumber(priv->disp) + 1;
	FD_SET(ConnectionNumber(priv->disp), &inp->fdset);

	/*
	mouse_devinfo.num_buttons = XGetPointerMapping(priv->disp, NULL, 0);
	*/
	XDisplayKeycodes(priv->disp, &minkey, &maxkey);
	key_devinfo.num_buttons = (maxkey - minkey) + 1;

	send_devinfo(inp, XDGA_DEV_KEY);
	/*
	send_devinfo(inp, XDGA_DEV_MOUSE);
	*/

	XDGAQueryExtension( priv->disp, &priv->event_base, &priv->error_base);

	XSync( priv->disp, True );
	XDGASelectInput( priv->disp, priv->screen, KeyPressMask|KeyReleaseMask);

	return 0;
}
