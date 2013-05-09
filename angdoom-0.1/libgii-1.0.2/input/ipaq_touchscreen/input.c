/* $Id: input.c,v 1.14 2005/08/04 16:49:56 cegger Exp $
******************************************************************************

   Input-ipaq-touchscreen: Driver for the iPaqs touchscreen
   
   This is a driver for the "null" device. It never generates any event 
   itself. However it might be useful for things that demand to be handed
   a gii_input_t.
   
   Copyright (C) 2001 Tobias Hunger [tobias@berlin-consortium.org]

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
#include "config.h"
#include <ggi/internal/gii-dl.h>
#include <ggi/internal/gii_debug.h>

#include <fcntl.h>

#include <linux/h3600_ts.h>

typedef struct {
    int fd;
    int readonly;
    gii_event_mask sent;

    int is_pressed;
} touchscreen_priv;

#define TOUCHSCREEN_PRIV(inp) ((touchscreen_priv *) inp->priv)

static gii_cmddata_getdevinfo touchscreen_devinfo = {
    "iPaq Touchscreen", /* long device name */
    "ipaq_ts", /* shorthand */
    emPointer, /* can generate */
    1, /* no. of buttons */
    2 /* no. of axes */
};

/* ---------------------------------------------------------------------- */

static void GII_touchscreen_send_devinfo(gii_input *inp) {
    gii_event ev;
    gii_cmddata_getdevinfo *dinfo;

    int size = sizeof(gii_cmd_nodata_event)+sizeof(gii_cmddata_getdevinfo);

    DPRINT_MISC("ipaq_touchscreen sending devinfo\n");
    
    _giiEventBlank(&ev, size);
    
    ev.any.size   = size;
    ev.any.type   = evCommand;
    ev.any.origin = inp->origin;
    ev.cmd.code   = GII_CMDCODE_GETDEVINFO;
    
    dinfo = (gii_cmddata_getdevinfo *) ev.cmd.data;
    *dinfo = touchscreen_devinfo;
    
    _giiEvQueueAdd(inp, &ev);
}

/* ---------------------------------------------------------------------- */

static inline void GII_touchscreen_handle_data(gii_input *inp) {
    touchscreen_priv * tshook = TOUCHSCREEN_PRIV(inp);
    int read_len;
    TS_EVENT new_event;
    gii_event ev;

    DPRINT_MISC("ipaq_touchscreen handling data...\n");

    read_len = read(tshook->fd, &new_event, sizeof(TS_EVENT));
    if (read_len != sizeof(TS_EVENT)) {
	perror("ipaq_touchscreen: Error reading touchscreen");
	return;
    }

    if (inp->curreventmask & emPtrAbsolute && new_event.pressure) {
	_giiEventBlank(&ev, sizeof(gii_pmove_event));
	
	ev.pmove.size   = sizeof(gii_pmove_event);
	ev.pmove.type   = evPtrAbsolute;
	ev.pmove.origin = inp->origin;

	ev.pmove.x      = new_event.x;
        ev.pmove.y      = new_event.y;
	ev.pmove.z      = 0;
	ev.pmove.wheel  = 0;

	DPRINT_EVENTS("ipaq_touchscreen: Generated pmove event (%d,%d)!\n",
		      ev.pmove.x, ev.pmove.y);
	_giiEvQueueAdd(inp, &ev);

	tshook->sent |= emPtrAbsolute;
    }

    if (inp->curreventmask & (emPtrButtonPress | emPtrButtonRelease)) {
	_giiEventBlank(&ev, sizeof(gii_pbutton_event));
	if (inp->curreventmask & emPtrButtonPress &&
	    new_event.pressure && !tshook->is_pressed) {
	    /* touches surface */
	    ev.pbutton.type = evPtrButtonPress;
	    tshook->sent |= emPtrButtonPress;
	    tshook->is_pressed = 1;

	    DPRINT_EVENTS("ipaq_touchscreen: Generated pbutton pressed event!\n");
	} else if (inp->curreventmask & emPtrButtonRelease && 
		   !new_event.pressure) {
	    /* pen leaves surface */
	    ev.pbutton.type = evPtrButtonRelease;
	    tshook->sent |= emPtrButtonRelease;
	    tshook->is_pressed = 0;

	    DPRINT_EVENTS("ipaq_touchscreen: Generated pbutton release event!\n");
	}
	ev.pbutton.size = sizeof(gii_pbutton_event);
	ev.pmove.origin = inp->origin;
	ev.pbutton.button = 1;

	_giiEvQueueAdd(inp, &ev);
    }
}

/* ---------------------------------------------------------------------- */

static gii_event_mask GII_touchscreen_poll(gii_input *inp, void *arg) {
    touchscreen_priv * tshook = TOUCHSCREEN_PRIV(inp);
    int doselect = 1;
    fd_set fds = inp->fdset;
    struct timeval tv = {0, 0};

    DPRINT_EVENTS("ipaq_touchscreen: poll(%p, %p) called\n", inp, arg);

    if (arg != NULL) {
	if (!FD_ISSET(tshook->fd, ((fd_set*)arg))) {
	    /* Nothing to read on our fd */
	    DPRINT_EVENTS("GII_touchscreen_poll: dummypoll\n");
	    return 0;
	}
	doselect = 0;
    }

    tshook->sent = 0;

    if (doselect) {
	if (select(inp->maxfd, &fds, NULL, NULL, &tv) <= 0)
	    return tshook->sent;
    } else
	doselect = 1;
    GII_touchscreen_handle_data(inp);

    return tshook->sent;
}

static int GII_touchscreen_send_event(gii_input *inp, gii_event *ev) {
    DPRINT_MISC("ipaq_touchscreen send event\n");

    if ((ev->any.target != inp->origin) &&
	(ev->any.target != GII_EV_TARGET_ALL))
	/* not for us */
	return GGI_EEVNOTARGET;
    
    if (ev->any.type != evCommand)
	return GGI_EEVUNKNOWN;

    if (ev->cmd.code == GII_CMDCODE_GETDEVINFO) {
	GII_touchscreen_send_devinfo(inp);
	return 0;
    }
    /*    
    if (ev->cmd.code == GII_CMDCODE_GETVALINFO) {
	int i;
	gii_cmddata_getvalinfo *vi;
	
	vi = (gii_cmddata_getvalinfo *) ev->cmd.data;
	
	if (vi->number == GII_VAL_QUERY_ALL) {
	    for (i=0; i < 6; i++)
		GII_spaceorb_send_valinfo(inp, i);
	    return 0;
	}
	
	return GII_spaceorb_send_valinfo(inp, vi->number);
    }
    */
    
    DPRINT_MISC("ipaq_touchscreen: event sent.\n");

    return GGI_EEVUNKNOWN; /* unknown command */
}

/* ---------------------------------------------------------------------- */

static int do_touchscreen_open(gii_input * inp, char *filename) {
    touchscreen_priv * tshook = TOUCHSCREEN_PRIV(inp);
    tshook->readonly = 0;
    
    DPRINT_MISC("ipaq_touchscreen opening...\n");

    tshook->fd = open(filename, O_RDWR | O_NOCTTY | O_NONBLOCK);
    
    if (tshook->fd < 0) {
	tshook->readonly = 1;
	tshook->fd = open(filename, O_RDONLY | O_NOCTTY | O_NONBLOCK);
    }
    
    if (tshook->fd < 0) {
	DPRINT_MISC("ipaq_touchscreen: Failed to open '%s'.\n",
		       filename);
	return GGI_ENODEVICE;
    }

    DPRINT_MISC("ipaq_touchscreen: Opened touchscreen file '%s'(%d) %s.\n",
		   filename, tshook->fd,
		   tshook->readonly ? "ReadOnly" : "Read/Write");

    return 0;
}

static int GII_touchscreen_close(gii_input *inp) {
    touchscreen_priv *tshook = TOUCHSCREEN_PRIV(inp);
    
    DPRINT_MISC("ipaq_touchscreen cleanup\n");
    
    close(tshook->fd);
    free(tshook);

    DPRINT_MISC("ipaq_touchscreen: exit OK.\n");

    return 0;
}

/* ---------------------------------------------------------------------- */


EXPORTFUNC int GIIdl_ipaq(gii_input *inp, const char *args, void *argptr);

int GIIdl_ipaq(gii_input *inp, const char *args, void *argptr)
{
    char             * devname = "/dev/ts";
    touchscreen_priv * tshook;
    int                ret;

    DPRINT_MISC("ipaq_touchscreen starting. (args=%s,argptr=%p)\n",
		   args, argptr);
    
    /* allocate touchscreens private structure */
    if ((tshook = inp->priv = malloc(sizeof(touchscreen_priv))) == NULL) {
	return GGI_ENOMEM;
    }

    if(_giiRegisterDevice(inp,&touchscreen_devinfo,NULL)==0) {
	    free(tshook);
	    return GGI_ENOMEM;
    }

    if ((ret = do_touchscreen_open(inp, devname)) < 0) {
	free(tshook);
	return ret;
    }

    inp->GIIsendevent = GII_touchscreen_send_event;
    inp->GIIeventpoll = GII_touchscreen_poll;
    inp->GIIclose     = GII_touchscreen_close;
   
    inp->targetcan = emPointer;
    inp->GIIseteventmask(inp, emPointer);

    inp->targetcan = emPointer | emCommand;
    inp->curreventmask = emPointer | emCommand;
    
    inp->maxfd = tshook->fd + 1;
    FD_SET(tshook->fd, &inp->fdset);
    DPRINT_MISC("ipaq_touchscreen: maxfd: %d\n", inp->maxfd);

    DPRINT_MISC("ipaq_touchscreen fully up\n");

    return 0;
}
