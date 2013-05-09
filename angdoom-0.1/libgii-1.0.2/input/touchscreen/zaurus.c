/* $Id: zaurus.c,v 1.6 2005/08/04 12:43:31 cegger Exp $
******************************************************************************

   Input-zaurus-touchscreen: Driver for the Zaurus' touchscreen
   
   Copyright (C) 2001 Tobias Hunger [tobias@fresco.org]

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

/* These headers are from the Zaurus' kernel source */
#include <asm/sharp_ts.h>

/* touchscreen internals: */
#include <common.h>


/* ----------------------------------------------------------------------
   Structures:
   ---------------------------------------------------------------------- */

static gii_cmddata_getdevinfo zts_devinfo =
{
  "Zaurus Touchscreen",           /* long device name */
  "zts",                          /* shorthand */
  emPointer | emValuator,         /* can_generate */
  1,                              /* num_buttons */
  2                               /* num_axes */
};



/* ----------------------------------------------------------------------
   Helper functions (INTernal):
   ---------------------------------------------------------------------- */

static int INT_zaurus_open(gii_input * inp, char *filename) {
    touchscreen_priv * tshook = TOUCHSCREEN_PRIV(inp);
    tshook->readonly = 0;
    
    tshook->fd = open(filename, O_RDWR | O_NOCTTY | O_NONBLOCK);
    
    if (tshook->fd < 0) {
	tshook->readonly = 1;
	tshook->fd = open(filename, O_RDONLY | O_NOCTTY | O_NONBLOCK);
    }
    
    if (tshook->fd < 0) {
	DPRINT_MISC("zaurus-ts: Failed to open '%s'.\n",
		       filename);
	return GGI_ENODEVICE;
    }

    tshook->is_calibrated = touchscreen_init();
    tshook->dev_info = &zts_devinfo;

    DPRINT_MISC("zaurus-ts: Opened touchscreen file '%s'(%d) %s %s.\n",
		   filename, tshook->fd,
		   tshook->readonly ? "ReadOnly" : "Read/Write",
		   tshook->is_calibrated ? "calibrated" : "uncalibrated");

    return 0;
}


static inline gii_event_mask INT_zaurus_handle_data(gii_input *inp) {
    touchscreen_priv * tshook = TOUCHSCREEN_PRIV(inp);
    int read_len;
    tsEvent event;

    read_len = read(tshook->fd, &event, sizeof(event));
    if (read_len != sizeof(event))
      {
        perror("zaurus-ts: Error reading touchscreen");
        return 0; /* empty event mask */
      }

    return touchscreen_handledata(inp,
				  event.x, event.y, event.pressure,
				  tshook->is_calibrated);
}



/* ----------------------------------------------------------------------
   functions visible to GGI:
   ---------------------------------------------------------------------- */

static gii_event_mask GII_zaurus_poll(gii_input *inp, void *arg) {
    touchscreen_priv * tshook = TOUCHSCREEN_PRIV(inp);
    int doselect = 1;
    fd_set fds = inp->fdset;
    struct timeval tv = {0, 0};

    if (arg != NULL) {
	if (!FD_ISSET(tshook->fd, ((fd_set*)arg))) {
	    /* Nothing to read on our fd */
	    DPRINT_EVENTS("zaurus-ts: dummypoll\n");
	    return 0;
	}
	doselect = 0;
    }

    if (doselect)
      {
	if (select(inp->maxfd, &fds, NULL, NULL, &tv) <= 0)
	  return 0; /* empty event mask */
      } else
	doselect = 1;
    tshook->sent = INT_zaurus_handle_data(inp);
    
    return tshook->sent;
}


static int GII_zaurus_close(gii_input *inp) {
    touchscreen_priv *tshook = TOUCHSCREEN_PRIV(inp);
    
    close(tshook->fd);
    free(tshook);

    DPRINT_MISC("zaurus-ts: exit OK.\n");

    return 0;
}



/* ----------------------------------------------------------------------
   GGI init
   ---------------------------------------------------------------------- */

EXPORTFUNC int GIIdl_zaurus(gii_input *inp, const char *args, void *argptr);

int GIIdl_zaurus(gii_input *inp, const char *args, void *argptr)
{
    char             * devname = "/dev/sharp_ts";
    touchscreen_priv * tshook;
    int                ret;

    DPRINT_MISC("zaurus-ts: starting. (args=%s,argptr=%p)\n",
		   args, argptr);
    
    /* allocate touchscreens private structure */
    if ((tshook = inp->priv = malloc(sizeof(touchscreen_priv))) == NULL) {
	return GGI_ENOMEM;
    }

    if(_giiRegisterDevice(inp, &zts_devinfo,NULL)==0) {
	    free(tshook);
	    return GGI_ENOMEM;
    }

    if ((ret = INT_zaurus_open(inp, devname)) < 0) {
	free(tshook);
	return ret;
    }

    inp->GIIsendevent = touchscreen_sendevent;
    inp->GIIeventpoll = GII_zaurus_poll;
    inp->GIIclose     = GII_zaurus_close;
   
    inp->targetcan = zts_devinfo.can_generate | emCommand;
    inp->curreventmask = inp->targetcan;
    
    inp->maxfd = tshook->fd + 1;
    FD_SET(tshook->fd, &inp->fdset);

    DPRINT_MISC("zaurus-ts: fully up\n");

    return 0;
}
