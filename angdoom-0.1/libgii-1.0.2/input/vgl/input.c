/* $Id: input.c,v 1.10 2005/08/04 12:43:31 cegger Exp $
******************************************************************************

   FreeBSD vgl(3) inputlib

   Copyright (C) 2000 Alcove - Nicolas Souchu <nsouch@freebsd.org>

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

#include <sys/kbio.h>
#include <sys/fbio.h>
#include <vgl.h>

#include "config.h"
#include "input-vgl.h"

static gii_cmddata_getdevinfo devinfo =
{
	"FreeBSD vgl",	/* long device name */
	"vgl",		/* shorthand */
	emAll,		/* can_generate */
	GII_NUM_UNKNOWN,/* num_buttons */
	GII_NUM_UNKNOWN,/* num_axes */
};

#define MAX_INPUTS 64

gii_event_mask
GII_vgl_poll(struct gii_input *inp, void *arg)
{
	int buf[MAX_INPUTS];
	int read_len;
	int ret, i;

	DPRINT_EVENTS("GII_vgl_poll(%p, %p) called\n", inp, arg);

	read_len = 0;
	while (read_len < MAX_INPUTS) {
		if (!(buf[read_len] = VGLKeyboardGetCh()))
			break;
		read_len ++;
	}

	/* Dispatch all keys and return queued events. */
	ret = 0;
	for (i = 0; i < read_len; i++) {
		ret |= GII_vgl_key2event(inp, buf[i]);
	}

	return ret;
}

static void send_devinfo(gii_input *inp)
{
	gii_event ev;
	gii_cmddata_getdevinfo *dinfo;
	size_t size;

	size = sizeof(gii_cmd_nodata_event)+sizeof(gii_cmddata_getdevinfo);

	_giiEventBlank(&ev, size);
	
	ev.any.size   = size;
	ev.any.type   = evCommand;
	ev.any.origin = inp->origin;
	ev.cmd.code   = GII_CMDCODE_GETDEVINFO;

	dinfo = (gii_cmddata_getdevinfo *) ev.cmd.data;
	*dinfo = devinfo;

	_giiEvQueueAdd(inp, &ev);
}


static int
GIIsendevent(gii_input *inp, gii_event *ev)
{
	if (ev->any.target != inp->origin &&
	    ev->any.target != GII_EV_TARGET_ALL) {
		/* not for us */
		return GGI_EEVNOTARGET;
	}

	if (ev->any.type != evCommand) {
		return GGI_EEVUNKNOWN;
	}

	if (ev->cmd.code == GII_CMDCODE_GETDEVINFO) {
		send_devinfo(inp);
		return 0;
	}

	return GGI_EEVUNKNOWN;	/* Unknown command */
}


static int
GIIclose(struct gii_input *inp)
{
	gii_vgl_priv *priv = VGL_PRIV(inp);

	VGLKeyboardEnd();
	free(priv);

	DPRINT_LIBS("FreeBSD vgl closing.\n");

	return 0;
}


int GIIdl_vgl(gii_input *inp, const char *args, void *argptr)
{
	gii_vgl_priv *priv;
	int error = 0;
	
	DPRINT_LIBS("FreeBSD vgl starting.\n");

	/* XXX Only on per link */
	VGLKeyboardInit(VGL_CODEKEYS);

	priv = malloc(sizeof(*priv));
	if (priv == NULL) {
		error = GGI_ENOMEM;
		goto error;
	}
	memset((void *)priv, 0, sizeof(*priv));

	if(_giiRegisterDevice(inp,&devinfo,NULL)==0) {
		free(priv);
		error = GGI_ENOMEM;
		goto error;
	}


	/* Retrieve the current keymap */
	if (ioctl(0, GIO_KEYMAP, &priv->kbd_keymap) < 0) {
		free(priv);
		error = GGI_ENODEVICE;
		goto error;
	}

	if (ioctl(0, GIO_DEADKEYMAP, &priv->kbd_accentmap) < 0)
		memset(&priv->kbd_accentmap, 0, sizeof(priv->kbd_accentmap));

	inp->GIIsendevent = GIIsendevent;
	inp->GIIeventpoll = GII_vgl_poll;
	inp->GIIclose = GIIclose;

	inp->targetcan = emAll;
	inp->curreventmask = emAll;

	inp->flags |= GII_FLAGS_HASPOLLED;
	inp->maxfd = 0;

	priv->prev_keycode = 0;
	inp->priv = priv;

	send_devinfo(inp);

	DPRINT_LIBS("FreeBSD vgl up.\n");

	return 0;

error:
	VGLKeyboardEnd();
	return error;
}
