/* $Id: xwin.c,v 1.1 2005/08/04 16:28:43 cegger Exp $
******************************************************************************

   Xwin inputlib - use an existing X window as a LibGII input source

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

#include <stdlib.h>
#include <string.h>
#include <X11/Xlib.h>

#include "config.h"
#include <ggi/gg.h>
#include <ggi/internal/gii-dl.h>
#include <ggi/internal/gii_debug.h>
#include <ggi/input/xwin.h>
#include "../x/xev.h"

#define RELPTR_KEYS { GIIK_CtrlL, GIIK_AltL, GIIUC_M }
#define RELPTR_KEYINUSE (1 | (1<<1) | (1<<2))
#define RELPTR_NUMKEYS  3

enum {
	XWIN_DEV_KEY=0,
	XWIN_DEV_MOUSE,

	XWIN_DEV_MAX
};

typedef struct {
	Display	       *disp;
	Window		win;
	Window		parentwin;
	XComposeStatus	compose_status;
	XIM		xim;
	XIC		xic;
	Cursor		cursor;
	unsigned int    oldcode;
	uint32_t       	symstat[0x60];
	int      width, height;
	int      oldx, oldy;
	int	 alwaysrel;
	int	 relptr;
	uint32_t relptr_keymask;
	gii_inputxwin_exposefunc *exposefunc;
	void	*exposearg;
	gii_inputxwin_resizefunc *resizefunc;
	void	*resizearg;
	gii_inputxwin_lockfunc *lockfunc;
	void	*lockarg;
	gii_inputxwin_unlockfunc *unlockfunc;
	void	*unlockarg;

	uint32_t origin[XWIN_DEV_MAX];
	char	 key_vector[32];
} xwin_priv;

#define XWIN_PRIV(inp)	((xwin_priv*)((inp)->priv))


/* These functions are only called from one place, and broken out of the
   code for readability. We inline them so we don't loose any performance. */

static inline Cursor make_cursor(Display *disp, Window win)
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


static void do_grab(xwin_priv *priv)
{
	XGrabPointer(priv->disp, priv->win, 1, 0,
		     GrabModeAsync, GrabModeAsync,
		     priv->win, priv->cursor, CurrentTime);
	XWarpPointer(priv->disp, None, priv->win, 0, 0, 0, 0,
		     priv->width/2, priv->height/2);
	priv->relptr = 1;
	priv->oldx = priv->width/2;
	priv->oldy = priv->height/2;
	DPRINT_EVENTS("GII_xwin: Using relative pointerevents\n");
}


static void do_ungrab(xwin_priv *priv)
{
	XUngrabPointer(priv->disp, CurrentTime);
	priv->relptr = 0;
	DPRINT_EVENTS("GII_xwin: Using absolute pointerevents\n");
}


static void handle_relptr(xwin_priv *priv)
{
	if (priv->relptr) {
		do_ungrab(priv);
	} else {
		do_grab(priv);
	}
}

static inline void center_pointer(xwin_priv *priv)
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

static inline void update_winparam(xwin_priv *priv)
{
	if (!priv->alwaysrel) {
		Window dummywin;
		unsigned int w, h, dummy;

		if (priv->cursor == None) {
			DPRINT_MISC("update_winparam: call make_cursor(%p,%i)\n",
					priv->disp, priv->win);
			priv->cursor = make_cursor(priv->disp, priv->win);
		}
		DPRINT_MISC("update_winparam: call XGetGeometry with disp=%p, win=%i\n",
				priv->disp, priv->win);
		XGetGeometry(priv->disp, priv->win, &dummywin,
			     (int *)&dummy, (int *)&dummy,
			     &w, &h, &dummy, &dummy);
		DPRINT_MISC("update_winparam: XGetGeometry() done, w=%u, h=%u\n",
				w, h);
		priv->width = w;
		priv->height = h;
		priv->oldx = w/2;
		priv->oldy = h/2;
	}
	if (priv->xim) {
		XDestroyIC(priv->xic);
		XCloseIM(priv->xim);
	}
	priv->xim = XOpenIM(priv->disp, NULL, NULL, NULL);
	if (priv->xim) {
		DPRINT_MISC("update_winparam: call XCreateIC with priv->win = %i\n",
				priv->win);
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
}

static gii_event_mask GII_xwin_eventpoll(gii_input *inp, void *arg)
{
	xwin_priv  *priv = XWIN_PRIV(inp);
	int n, i;
	int rc = 0;
	gii_event releasecache;
	Time      releasetime = 0;
	int       havecached = 0;
	
	DPRINT_EVENTS("GII_xwin_eventpoll(%p) called\n", inp);

	if (priv->lockfunc) priv->lockfunc(priv->lockarg);

	n = 0;
	while (n || (n = XEventsQueued(priv->disp, QueuedAfterReading))) {
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
			giiev.any.origin = priv->origin[XWIN_DEV_KEY];
			giiev.key.button = keycode - 8;

			priv->key_vector[keycode/8]|=1<<(keycode&7);

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

			if (!priv->alwaysrel) {
				uint32_t relsyms[RELPTR_NUMKEYS] = RELPTR_KEYS;
				for (i=0; i < RELPTR_NUMKEYS; i++) {
					if (giiev.key.label == relsyms[i]) {
						priv->relptr_keymask &= ~(1<<i);
						break;
					}
				}
				if (priv->relptr_keymask == 0) {
					handle_relptr(priv);
				}
			}
			DPRINT_EVENTS("GII_xwin_eventpoll: KeyPress\n");
			break;

		case KeyRelease:
			priv->key_vector[keycode/8]&=~(1<<(keycode&7));
			if (havecached) {
				_giiEvQueueAdd(inp, &releasecache);
				rc |= emKeyRelease;
			}
			_giiEventBlank(&releasecache, sizeof(gii_key_event));
			releasecache.any.size = sizeof(gii_key_event);
			releasecache.any.type = evKeyRelease;
			releasecache.any.origin = priv->origin[XWIN_DEV_KEY];
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

			if (!priv->alwaysrel) {
				uint32_t relsyms[RELPTR_NUMKEYS] = RELPTR_KEYS;
				for (i=0; i < RELPTR_NUMKEYS; i++) {
					if (releasecache.key.label
					    == relsyms[i]) {
						priv->relptr_keymask |= (1<<i);
						break;
					}
				}
			}
			DPRINT_EVENTS("GII_xwin_eventpoll: KeyRelease\n");
			break;

		case KeymapNotify: {
			unsigned char delta;
			int j;
			XEvent fake_xev;
			
			for(i=1;i<32;i++) {
				delta=priv->key_vector[i]^xev.xkeymap.key_vector[i];
				if (!delta) continue;
				for(j=0;j<8;j++) {
					if (0==(delta&(1<<j))) continue;
					keycode=i*8+j;
					fake_xev.xkey.serial	=xev.xkeymap.serial;
					fake_xev.xkey.send_event=xev.xkeymap.send_event;
					fake_xev.xkey.display	=xev.xkeymap.display;
					fake_xev.xkey.window	=xev.xkeymap.window;
					fake_xev.xkey.root	=xev.xkeymap.window;
					fake_xev.xkey.subwindow	=xev.xkeymap.window;
					fake_xev.xkey.time	=0;
					fake_xev.xkey.x		=0;
					fake_xev.xkey.y		=0;
					fake_xev.xkey.x_root	=0;
					fake_xev.xkey.y_root	=0;
					fake_xev.xkey.state	=0;
					fake_xev.xkey.keycode	=keycode;
					fake_xev.xkey.same_screen=0;
					
					if (xev.xkeymap.key_vector[i]&(1<<j)) {
						giiev.any.size = sizeof(gii_key_event);
						giiev.any.type = evKeyPress;
						giiev.any.origin = priv->origin[XWIN_DEV_KEY];
						giiev.key.button = keycode - 8;
						rc |= emKeyPress;
						fake_xev.xkey.type      =KeyPress;
						_gii_xev_trans(&fake_xev.xkey, &giiev.key,
								&priv->compose_status, priv->xic,
								&priv->oldcode);
						if (giiev.any.type == evKeyPress &&
						    giiev.key.label < 0x60) {
							priv->symstat[giiev.key.label] = giiev.key.sym;
						}
						_giiEvQueueAdd(inp, &giiev);
					} else {
						giiev.any.size = sizeof(gii_key_event);
						giiev.any.type = evKeyRelease;
						giiev.any.origin = priv->origin[XWIN_DEV_KEY];
						giiev.key.button = keycode - 8;
						rc |= emKeyRelease;
						fake_xev.xkey.type      =KeyRelease;
						_gii_xev_trans(&fake_xev.xkey, &giiev.key,
								&priv->compose_status, NULL, NULL);
						if (giiev.key.label < 0x60 &&
						    priv->symstat[giiev.key.label] != 0) {
							giiev.key.sym=priv->symstat[giiev.key.label];
						}
						_giiEvQueueAdd(inp, &giiev);
					}
				}
				priv->key_vector[i]=xev.xkeymap.key_vector[i];
			}
			} goto dont_queue_this_event;

		case ButtonPress:
			giiev.any.type = evPtrButtonPress;
			giiev.any.origin = priv->origin[XWIN_DEV_MOUSE];
			rc |= emPtrButtonPress;
			DPRINT_EVENTS("GII_xwin_eventpoll: ButtonPress %x\n",
				      xev.xbutton.state); 
			break;

		case ButtonRelease:
			giiev.any.type = evPtrButtonRelease;
			giiev.any.origin = priv->origin[XWIN_DEV_MOUSE];
			rc |= emPtrButtonRelease;
			DPRINT_EVENTS("GII_xwin_eventpoll: ButtonRelease %x\n",
				      xev.xbutton.state); 
			break;

		case MotionNotify:
			if (priv->relptr) {
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
					    > priv->width / 4 ||
					    ABS(priv->height/2 - xev.xmotion.y)
					    > priv->height / 4)
					{
						center_pointer(priv);
					}
#undef ABS			
				}
				priv->oldx = xev.xmotion.x;
				priv->oldy = xev.xmotion.y;
				if (!realmove ||
				    (giiev.pmove.x == 0 && giiev.pmove.y == 0))
					goto dont_queue_this_event;
				giiev.any.type = evPtrRelative;
				rc |= emPtrRelative;
			} else {
				if (priv->alwaysrel) {
					giiev.any.type = evPtrRelative;
					rc |= emPtrRelative;
				} else {
					giiev.any.type = evPtrAbsolute;
					rc |= emPtrAbsolute;
				}
				giiev.pmove.x = xev.xmotion.x;
				giiev.pmove.y = xev.xmotion.y;
			}
			giiev.any.size = sizeof(gii_pmove_event);
			giiev.any.origin = priv->origin[XWIN_DEV_MOUSE];
			DPRINT_EVENTS("GII_xwin_eventpoll: MouseMove %d,%d\n",
					 giiev.pmove.x, giiev.pmove.y);
			break;

		case EnterNotify:
			/* We used to pull focus here using
			 * XSetInputFocus(priv->disp, priv->win,
			 *		RevertToParent, CurrentTime);
			 * We now leave that to the window manager.
			 */
		case LeaveNotify:
			break;

		case FocusIn:
		case FocusOut:
			break;

		case Expose:
			DPRINT_EVENTS("GII_xwin_eventpoll: Expose\n");
			if (priv->exposefunc != NULL) {
				if (! priv->exposefunc(priv->exposearg,
						       xev.xexpose.x,
						       xev.xexpose.y,
						       xev.xexpose.width,
						       xev.xexpose.height))
				{
					goto dont_queue_this_event;
				}
			}
			giiev.any.size = sizeof(gii_expose_event);
			giiev.any.type = evExpose;
			giiev.any.origin = inp->origin;
			giiev.expose.x = xev.xexpose.x;
			giiev.expose.y = xev.xexpose.y;
			giiev.expose.w = xev.xexpose.width;
			giiev.expose.h = xev.xexpose.height;
			break;

		case GraphicsExpose:
			DPRINT_EVENTS("GII_xwin_eventpoll: Expose dest\n");
			if (priv->exposefunc != NULL) {
				if (! priv->exposefunc(priv->exposearg,
						     xev.xgraphicsexpose.x,
						     xev.xgraphicsexpose.y,
						     xev.xgraphicsexpose.width,
						     xev.xgraphicsexpose.height
						     ))
				{
					goto dont_queue_this_event;
				}
			}
			giiev.any.size = sizeof(gii_expose_event);
			giiev.any.type = evExpose;
			giiev.any.origin = inp->origin;
			giiev.expose.x = xev.xgraphicsexpose.x;
			giiev.expose.y = xev.xgraphicsexpose.y;
			giiev.expose.w = xev.xgraphicsexpose.width;
			giiev.expose.h = xev.xgraphicsexpose.height;
			break;

		case ResizeRequest:
			DPRINT_EVENTS("GII_xwin_eventpoll: Resize\n");
			giiev.any.origin = inp->origin;
			if (priv->resizefunc != NULL) {
				if (priv->resizefunc(priv->resizearg,
						       xev.xresizerequest.width,
						       xev.xresizerequest.height,
						       &giiev)) {
					goto dont_queue_this_event;
				}
			}
			break;

		default: 
			DPRINT_EVENTS("GII_xwin_eventpoll: Other Event (%d)\n",
				      xev.type);
			break;
			
		}
		switch(giiev.any.type) {
		case evPtrButtonPress:
		case evPtrButtonRelease:
			giiev.any.size = sizeof(gii_pbutton_event);
			giiev.pbutton.button =
				_gii_xev_buttontrans(xev.xbutton.button);
			break;
		}
		if (giiev.any.type) _giiEvQueueAdd(inp, &giiev);
		dont_queue_this_event:
		/* "ANSI C forbids label at end of compound statement"
		   Well, this makes GCC happy at least... */
		while(0){};
	}
	if (priv->unlockfunc) priv->unlockfunc(priv->unlockarg);

	if (havecached) {
		_giiEvQueueAdd(inp, &releasecache);
		rc |= emKeyRelease;
		if (releasecache.key.label < 0x60) {
			priv->symstat[releasecache.key.label] = 0;
		}
	}

	return rc;
}


static int GII_xwin_close(gii_input *inp)
{
	xwin_priv  *priv = XWIN_PRIV(inp);
	
	if (priv->cursor != None) XFreeCursor(priv->disp, priv->cursor);

	if (priv->xim) {
		XDestroyIC(priv->xic);
		XCloseIM(priv->xim);
	}
	
	free(priv);
	
	DPRINT_MISC("GII_xwin_close(%p) called\n", inp);

	return 0;
}


static gii_cmddata_getdevinfo mouse_devinfo =
{
	"Xwin Mouse",	/* long device name */
	"xwms",		/* shorthand */
	emPointer,	/* can_generate */
	0,		/* num_buttons	(filled in runtime) */
	0		/* num_axes 	(only for valuators) */
};

static gii_cmddata_getdevinfo key_devinfo =
{
	"Xwin Keyboard",/* long device name */
	"xwkb",		/* shorthand */
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
	xwin_priv  *priv = XWIN_PRIV(inp);

	size = sizeof(gii_cmd_nodata_event)+sizeof(gii_cmddata_getdevinfo);

	_giiEventBlank(&ev, size);
	
	ev.any.size   = size;
	ev.any.type   = evCommand;
	ev.any.origin = priv->origin[dev];
	ev.cmd.code   = GII_CMDCODE_GETDEVINFO;

	dinfo = (gii_cmddata_getdevinfo *) ev.cmd.data;
	switch (dev) {
	case XWIN_DEV_MOUSE:
		*dinfo = mouse_devinfo;
		break;
	case XWIN_DEV_KEY:
		*dinfo = key_devinfo;
		break;
	default:
		return;
	}

	_giiEvQueueAdd(inp, &ev);
}


static int GIIsendevent(gii_input *inp, gii_event *ev)
{
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
			send_devinfo(inp, XWIN_DEV_KEY);
			send_devinfo(inp, XWIN_DEV_MOUSE);
			return 0;
		} else {
			xwin_priv  *priv = XWIN_PRIV(inp);
			if(ev->any.target==priv->origin[XWIN_DEV_KEY]) {
				send_devinfo(inp, XWIN_DEV_KEY);
				return 0;
			}
			if(ev->any.target==priv->origin[XWIN_DEV_MOUSE]) {
				send_devinfo(inp, XWIN_DEV_MOUSE);
				return 0;
			}
			return GGI_EEVNOTARGET;
		}

	} else if (ev->cmd.code == GII_CMDCODE_XWINSETPARAM) {
		xwin_priv  *priv = XWIN_PRIV(inp);
		gii_xwin_cmddata_setparam data;

		/* Assure aligned memory access. Some platforms
		 * (i.e. NetBSD/sparc64) rely on this.
		 */
		memcpy(&data, (gii_xwin_cmddata_setparam *)ev->cmd.data,
			sizeof(gii_xwin_cmddata_setparam));

		priv->win = data.win;
		priv->parentwin = data.parentwin;
		priv->alwaysrel = data.ptralwaysrel;

		update_winparam(priv);

		return 0;
	} else if (ev->cmd.code == GII_CMDCODE_PREFER_ABSPTR) {
		xwin_priv  *priv = XWIN_PRIV(inp);
		if (priv->relptr) do_ungrab(priv);
		return 0;
	} else if (ev->cmd.code == GII_CMDCODE_PREFER_RELPTR) {
		xwin_priv  *priv = XWIN_PRIV(inp);
		if (!priv->relptr) do_grab(priv);
		return 0;
	}

	return GGI_EEVUNKNOWN;
}


EXPORTFUNC int GIIdl_xwin(gii_input *inp, const char *args, void *argptr);

int GIIdl_xwin(gii_input *inp, const char *args, void *argptr)
{
	XComposeStatus dummyxcs = { NULL, 0 };
	gii_inputxwin_arg *xwinarg = argptr;
	xwin_priv  *priv;
	int minkey, maxkey;

	DPRINT_MISC("GIIdlinit(%p) called for input-xwin\n", inp);

	if (xwinarg == NULL || xwinarg->disp == NULL) {
		return GGI_EARGREQ;
	}

	if ((priv = malloc(sizeof(xwin_priv))) == NULL) {
		return GGI_ENOMEM;
	}


	/* Attention: priv->win is NOT valid at this
	 * point. It becomes valid once a mode has
	 * been set up by libggi's X-target.
	 */
	priv->disp = xwinarg->disp;
	priv->win  = xwinarg->win;
	priv->parentwin  = xwinarg->win;
	priv->compose_status = dummyxcs;
	priv->xim = NULL;
	priv->xic = NULL;
	priv->cursor = None;
	priv->oldcode = 0;
	memset(priv->symstat, 0, sizeof(priv->symstat));

	priv->alwaysrel		= xwinarg->ptralwaysrel;
	priv->relptr		= 0;
	priv->relptr_keymask	= RELPTR_KEYINUSE;
	priv->exposefunc	= xwinarg->exposefunc;
	priv->exposearg		= xwinarg->exposearg;
	priv->resizefunc	= xwinarg->resizefunc;
	priv->resizearg		= xwinarg->resizearg;
	priv->lockfunc		= xwinarg->lockfunc;
	priv->lockarg		= xwinarg->lockarg;
	priv->unlockfunc	= xwinarg->unlockfunc;
	priv->unlockarg		= xwinarg->unlockarg;

	memset(priv->key_vector,0,sizeof(priv->key_vector));

	if (!xwinarg->wait) {
		update_winparam(priv);
	} else {
		priv->cursor = None;
	}

	inp->priv = priv;

	inp->GIIsendevent = GIIsendevent;
	inp->GIIeventpoll = GII_xwin_eventpoll;
	inp->GIIclose = GII_xwin_close;

	if (0 == (priv->origin[XWIN_DEV_KEY] =
		_giiRegisterDevice(inp,&key_devinfo,NULL)))
	{
		GII_xwin_close(inp);
		return GGI_ENOMEM;
	}

	if (0 == (priv->origin[XWIN_DEV_MOUSE] =
		_giiRegisterDevice(inp,&mouse_devinfo,NULL)))
	{
		GII_xwin_close(inp);
		return GGI_ENOMEM;
	}


	inp->targetcan = emKey | emPointer | emExpose;
	inp->curreventmask = emKey | emPointer | emExpose;

	inp->maxfd = ConnectionNumber(priv->disp) + 1;
	FD_SET(ConnectionNumber(priv->disp), &inp->fdset);

	mouse_devinfo.num_buttons = XGetPointerMapping(priv->disp, NULL, 0);
	XDisplayKeycodes(priv->disp, &minkey, &maxkey);
	key_devinfo.num_buttons = (maxkey - minkey) + 1;

	send_devinfo(inp, XWIN_DEV_KEY);
	send_devinfo(inp, XWIN_DEV_MOUSE);

	return 0;
}
