/* $Id: input.c,v 1.25 2005/08/04 12:43:31 cegger Exp $
******************************************************************************

   Standalone X inputlib

   Copyright (C) 1998-1999 Marcus Sundberg	[marcus@ggi-project.org]

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
#include <ggi/internal/gii-dl.h>
#include <ggi/internal/gii_debug.h>
#include "xev.h"

#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>
#include <ctype.h>

static const gg_option optlist[] =
{
	{ "nokeyfocus",   "no" }
};

#define OPT_NOKEYFOCUS	0

#define NUM_OPTS	(sizeof(optlist)/sizeof(gg_option))


enum {
	X_DEV_KEY=0,
	X_DEV_MOUSE,

	X_DEV_MAX
};


typedef struct {
	Display	       *disp;
	Window		win;
	XComposeStatus	compose_status;
	XIM		xim;
	XIC		xic;
	unsigned int    oldcode;
	uint32_t       	symstat[0x60];
	int      width, height;
	int      oldx, oldy;

	uint32_t origin[X_DEV_MAX];

	int	keyfocus;
} x_priv;

#define X_PRIV(inp)	((x_priv*)((inp)->priv))

static Cursor make_cursor(Display *disp, Window win)
{
	char emptybm[] = {0};
	Pixmap crsrpix;
	XColor nocol;
	Cursor mycrsr;

	crsrpix = XCreateBitmapFromData(disp, win, emptybm, 1, 1);
	mycrsr = XCreatePixmapCursor(disp, crsrpix, crsrpix,
				     &nocol, &nocol, 0, 0);
	XFreePixmap(disp, crsrpix);

	return mycrsr;
}

static int do_ungrab(Display *disp, Window win)
{
	XUngrabKeyboard(disp, win);
	XUngrabPointer(disp, win);
	return 0;
}

static int do_grab(Display *disp, Window win, Cursor crsr)
{
	if (XGrabKeyboard(disp, win, True, GrabModeAsync, GrabModeAsync,
			  CurrentTime)
	    != GrabSuccess ||
	    XGrabPointer(disp, win, True,
			 PointerMotionMask|ButtonPressMask|ButtonReleaseMask,
			 GrabModeAsync, GrabModeAsync, win, crsr, CurrentTime)
	    != GrabSuccess) {
		DPRINT_LIBS("input-X: Unable to grab input\n");
		return GGI_ENODEVICE;
	}

	return 0;
}

static inline void center_pointer(x_priv *priv)
{
	XEvent event;
	event.type = MotionNotify;
	event.xmotion.display = priv->disp;
	event.xmotion.window = priv->win;
	event.xmotion.x = priv->width/2;
	event.xmotion.y = priv->height/2;
	XSendEvent(priv->disp, priv->win, False, PointerMotionMask, &event);
	XWarpPointer(priv->disp, None, priv->win, 0, 0, 0, 0,
		     priv->width/2, priv->height/2);
}

static gii_event_mask GII_x_eventpoll(gii_input *inp, void *arg)
{
	x_priv  *priv = X_PRIV(inp);
	int n;
	int rc = 0;
	gii_event releasecache;
	Time      releasetime = 0;
	int       havecached = 0;

	n = 0;
	while (n || (n = XPending(priv->disp))) {
		XEvent xev;
		gii_event giiev;
		unsigned int keycode;
		
		n--;

		XNextEvent(priv->disp, &xev);
		keycode = xev.xkey.keycode;

		if (XFilterEvent(&xev, None)) {
			priv->oldcode = keycode;
			if (xev.xkey.keycode == 0) continue;
		}

		_giiEventBlank(&giiev, sizeof(gii_event));

		switch(xev.type) { 
		case KeyPress:
			giiev.any.size = sizeof(gii_key_event);
			giiev.any.type = evKeyPress;
			giiev.any.origin = priv->origin[X_DEV_KEY];
			giiev.key.button = keycode - 8;

			if (havecached &&
			    releasecache.key.button == giiev.key.button) {
				if (xev.xkey.time == releasetime) {
					giiev.any.type = evKeyRepeat;
					rc |= emKeyRepeat;
				} else {
					_giiEvQueueAdd(inp, &releasecache);
					rc |= emKeyRelease;
					rc |= emKeyPress;
					if (releasecache.key.label < 0x60) {
						priv->symstat[releasecache.key.label] = 0;
					}
				}
				havecached = 0;
			} else {
				rc |= emKeyPress;
			}
			_gii_xev_trans(&xev.xkey, &giiev.key,
				       &priv->compose_status, priv->xic,
				       &priv->oldcode);
			if (giiev.any.type == evKeyPress &&
			    giiev.key.label < 0x60) {
				priv->symstat[giiev.key.label] = giiev.key.sym;
			}

			DPRINT_EVENTS("GII_x_eventpoll: KeyPress\n");
			break;

		case KeyRelease: 
			if (havecached) {
				_giiEvQueueAdd(inp, &releasecache);
				rc |= emKeyRelease;
			}

			_giiEventBlank(&releasecache, sizeof(gii_key_event));
			releasecache.any.size = sizeof(gii_key_event);
			releasecache.any.type = evKeyRelease;
			releasecache.any.origin = priv->origin[X_DEV_KEY];
			releasecache.key.button = keycode - 8;

			_gii_xev_trans(&xev.xkey, &releasecache.key,
				       &priv->compose_status, NULL, NULL);
			if (releasecache.key.label < 0x60 &&
			    priv->symstat[releasecache.key.label] != 0) {
				releasecache.key.sym
				      = priv->symstat[releasecache.key.label];
			}
			havecached = 1;
			releasetime = xev.xkey.time;

			DPRINT_EVENTS("GII_x_eventpoll: KeyRelease\n");
			break;

		case ButtonPress:
			giiev.any.type = evPtrButtonPress;
			giiev.any.origin = priv->origin[X_DEV_MOUSE];
			rc |= emPtrButtonPress;
			DPRINT_EVENTS("GII_x_eventpoll: ButtonPress %x\n",
				      xev.xbutton.state); 
			break;

		case ButtonRelease:
			giiev.any.type = evPtrButtonRelease;
			giiev.any.origin = priv->origin[X_DEV_MOUSE];
			rc |= emPtrButtonRelease;
			DPRINT_EVENTS("GII_x_eventpoll: ButtonRelease %x\n",
					xev.xbutton.state);
			break;

		case EnterNotify:
			if (priv->keyfocus) {
				/* active window get the key input focus */
				XSetInputFocus(priv->disp, priv->win,
						RevertToParent, CurrentTime);
			}	/* if */
		case LeaveNotify:
			break;

		case FocusIn:
		case FocusOut:
			break;

		case MotionNotify:
		{
			int realmove = 0;

			if (!xev.xmotion.send_event) {
				giiev.pmove.x =
					xev.xmotion.x - priv->oldx;
				giiev.pmove.y =
					xev.xmotion.y - priv->oldy;
				realmove = 1;
#undef ABS
#define ABS(a) (((int)(a)<0) ? -(a) : (a) )
				if (ABS(priv->width/2 - xev.xmotion.x)
				    > priv->width / 4
				    || ABS(priv->height/2 - xev.xmotion.y)
				    > priv->height / 4) {
#undef ABS	
					center_pointer(priv);
				}
			}
			priv->oldx = xev.xmotion.x;
			priv->oldy = xev.xmotion.y;
			if (!realmove
			    || (giiev.pmove.x == 0 && giiev.pmove.y == 0)) {
				goto dont_queue_this_event;
			}
			giiev.any.size = sizeof(gii_pmove_event);
			giiev.any.type = evPtrRelative;
			giiev.any.origin = priv->origin[X_DEV_MOUSE];
			rc |= emPtrRelative;
			DPRINT_EVENTS("GII_x_eventpoll: MouseMove %d,%d\n",
				      giiev.pmove.x, giiev.pmove.y);
			break;
		}
		}	/* switch */
		switch(giiev.any.type) {
		case evPtrButtonPress:
		case evPtrButtonRelease:
			giiev.any.size = sizeof(gii_pbutton_event);
			giiev.pbutton.button =
				_gii_xev_buttontrans(xev.xbutton.button);
			break;
		}
		if (giiev.any.type) {
			_giiEvQueueAdd(inp, &giiev);
		}
		dont_queue_this_event:
		/* "ANSI C forbids label at end of compound statement"
		   Well, this makes GCC happy at least... */
		while(0){};
	}
	if (havecached) {
		_giiEvQueueAdd(inp, &releasecache);
		rc |= emKeyRelease;
		if (releasecache.key.label < 0x60) {
			priv->symstat[releasecache.key.label] = 0;
		}
	}

	return rc;
}

static int GII_x_close(gii_input *inp)
{
	x_priv  *priv = X_PRIV(inp);

	if (priv->xim) {
		XDestroyIC(priv->xic);
		XCloseIM(priv->xim);
	}

	do_ungrab(priv->disp, priv->win);

	XDestroyWindow(priv->disp, priv->win);
	XCloseDisplay(priv->disp);
	free(priv);
	
	return 0;
}


static gii_cmddata_getdevinfo mouse_devinfo =
{
	"X Mouse",	/* long device name */
	"xmse",		/* shorthand */
	emPointer,	/* can_generate */
	0,		/* num_buttons	(filled in runtime) */
	0		/* num_axes 	(only for valuators) */
};

static gii_cmddata_getdevinfo key_devinfo =
{
	"X Keyboard",/* long device name */
	"xkbd",		/* shorthand */
	emKey,		/* can_generate */
	0,		/* num_buttons	(filled in runtime) */
	0		/* num_axes 	(only for valuators) */
};

/* dev 0 is keyboard, dev 1 is mouse */
static void send_devinfo(gii_input *inp, int dev)
{
	
	gii_event ev;
	gii_cmddata_getdevinfo *dinfo;
	x_priv  *priv = X_PRIV(inp);
	size_t size;

	size = sizeof(gii_cmd_nodata_event)+sizeof(gii_cmddata_getdevinfo);

	_giiEventBlank(&ev, size);
	
	ev.any.size   = size;
	ev.any.type   = evCommand;
	ev.any.origin = priv->origin[dev];
	ev.cmd.code   = GII_CMDCODE_GETDEVINFO;

	dinfo = (gii_cmddata_getdevinfo *) ev.cmd.data;
	switch (dev) {
	case X_DEV_MOUSE:
		*dinfo = mouse_devinfo;
		break;
	case X_DEV_KEY:
		*dinfo = key_devinfo;
		break;
	default:
		return;
	}

	_giiEvQueueAdd(inp, &ev);
}


static int GIIsendevent(gii_input *inp, gii_event *ev)
{
	x_priv  *priv = X_PRIV(inp);
	if ((ev->any.target & 0x100) != inp->origin &&
	    ev->any.target != GII_EV_TARGET_ALL) {
		/* not for us */
		return GGI_EEVNOTARGET;
	}

	if (ev->any.type != evCommand) {
		return GGI_EEVUNKNOWN;
	}

	if (ev->cmd.code == GII_CMDCODE_GETDEVINFO) {
		if (ev->any.target == GII_EV_TARGET_ALL) {
			send_devinfo(inp, X_DEV_KEY);
			send_devinfo(inp, X_DEV_MOUSE);
			return 0;
		} else {
			if(ev->any.target==priv->origin[X_DEV_KEY]) {
				send_devinfo(inp, X_DEV_KEY);
				return 0;
			}
			if(ev->any.target==priv->origin[X_DEV_MOUSE]) {
				send_devinfo(inp, X_DEV_MOUSE);
				return 0;
			}
			return GGI_EEVNOTARGET;
		}
	}

	return GGI_EEVUNKNOWN;	/* Unknown command */
}


EXPORTFUNC int GIIdl_x(gii_input *inp, const char *args, void *argptr);

int GIIdl_x(gii_input *inp, const char *args, void *argptr)
{
	Display *disp;
	Window   win;
	Screen  *sc;
	XSetWindowAttributes attr;
	XComposeStatus dummyxcs = { NULL, 0 };
	int      scr;
	Cursor   crsr;
	XEvent   xev;
	x_priv  *priv;
	int minkey, maxkey;
	gg_option options[NUM_OPTS];


	/* handle args */
	memcpy(options, optlist, sizeof(options));

	if (args) {
		args = ggParseOptions(args, options, NUM_OPTS);
		if (args == NULL) {
			fprintf(stderr, "input-x: error in "
				"arguments.\n");
			return GGI_EARGINVAL;
		}
	}


	if ((disp = XOpenDisplay(NULL)) == NULL) {
		DPRINT_LIBS("input-X: Unable to open display\n");
		return GGI_ENODEVICE;
	}
 
	scr = XScreenNumberOfScreen(sc=DefaultScreenOfDisplay(disp));

	attr.event_mask =
		KeyPressMask | KeyReleaseMask |
		ButtonPressMask | ButtonReleaseMask | PointerMotionMask |
		FocusChangeMask;
	win = XCreateWindow(disp, RootWindow(disp, scr), 0, 0,
			    WidthOfScreen(sc)/2U,  HeightOfScreen(sc)/2U,
			    0, 0, InputOnly, CopyFromParent,
			    CWEventMask, &attr);

	XMapRaised(disp, win);
	XSync(disp, 0);

	/* Wait until window is mapped */
	XNextEvent(disp, &xev);

	/* Move to top left corner in case the WM uses interactive placement */
	XMoveWindow(disp, win, 0, 0);

	crsr = make_cursor(disp, win);

	if (do_grab(disp, win, crsr) != 0) {
		XDestroyWindow(disp, win);
		XCloseDisplay(disp);
		return GGI_ENODEVICE;
	}
	
	if ((priv = malloc(sizeof(x_priv))) == NULL) {
		XDestroyWindow(disp, win);
		XCloseDisplay(disp);
		return GGI_ENOMEM;
	}

	priv->disp = disp;
	priv->win  = win;
	priv->xim = NULL;
	priv->xic = NULL;
	priv->oldcode = 0;
	priv->compose_status = dummyxcs;
	memset(priv->symstat, 0, sizeof(priv->symstat));

	{
		Window dummywin;
		unsigned int w, h, dummy;

		XGetGeometry(disp, win, &dummywin, (int *)&dummy, (int *)&dummy,
			     &w, &h, &dummy, &dummy);
		priv->width = w;
		priv->height = h;
		priv->oldx = w/2;
		priv->oldy = h/2;
		center_pointer(priv);
	}

	priv->xim = XOpenIM(priv->disp, NULL, NULL, NULL);
	if (priv->xim) {
		priv->xic = XCreateIC(priv->xim, XNInputStyle,
				      XIMPreeditNothing | XIMStatusNothing,
				      XNClientWindow, priv->win,
				      XNFocusWindow, priv->win,
				      NULL);
		if (!priv->xic) {
			XCloseIM(priv->xim);
			priv->xim = NULL;
		}
	} else {
		priv->xic = NULL;
	}

	inp->priv = priv;

	inp->GIIsendevent = GIIsendevent;
	inp->GIIeventpoll = GII_x_eventpoll;
	inp->GIIclose = GII_x_close;

	inp->targetcan = emKey | emPointer;
	inp->curreventmask = emKey | emPointer;

	if (tolower((uint8_t)options[OPT_NOKEYFOCUS].result[0]) == 'n') {
		priv->keyfocus = 1;
	} else {
		priv->keyfocus = 0;
	}


	if (0 == (priv->origin[X_DEV_KEY] =
		_giiRegisterDevice(inp,&key_devinfo,NULL)))
	{
		GII_x_close(inp);
		return GGI_ENOMEM;
		
	}
	
	if (0 == (priv->origin[X_DEV_MOUSE] =
		_giiRegisterDevice(inp,&mouse_devinfo,NULL)))
	{
		GII_x_close(inp);
		return GGI_ENOMEM;
	}

	inp->maxfd = ConnectionNumber(disp) + 1;
	FD_SET((unsigned)(ConnectionNumber(disp)), &inp->fdset);

	mouse_devinfo.num_buttons = XGetPointerMapping(priv->disp, NULL, 0);
	XDisplayKeycodes(priv->disp, &minkey, &maxkey);
	key_devinfo.num_buttons = (maxkey - minkey) + 1;
	
	send_devinfo(inp, X_DEV_KEY);
	send_devinfo(inp, X_DEV_MOUSE);

	return 0;
}
