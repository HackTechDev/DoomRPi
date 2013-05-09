/* $Id: input.c,v 1.6 2005/08/31 17:41:43 cegger Exp $
******************************************************************************

   Quartz: Input driver

   Copyright (C) 2004 Christoph Egger

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
#include <ggi/internal/gii-dl.h>
#include <ggi/internal/gii_debug.h>

#include <ggi/input/quartz.h>
#include "quartz.h"


static gii_cmddata_getdevinfo mouse_devinfo =
{
	"Quartz Mouse",				/* long name */
	"QZMS",					/* short name */
	emPointer,				/* can_generate (targetcan) */
	5,					/* num_buttons  (filled in runtime) */
	0					/* num_axes     (only for valuators) */
};

static gii_cmddata_getdevinfo key_devinfo =
{
	"Quartz Keyboard",	/* long name */
	"QZKB",			/* short name */
	emKey,			/* can_generate (targetcan) */
	100,			/* num_buttons (filled in runtime) */
	0			/* num_axes    (only for valuators) */
};


static int update_winparam(gii_input *inp)
{
	QuartzUninitEventHandler(inp);
	QuartzInitEventHandler(inp);

	return GGI_OK;
}	/* update_winparam */


static int send_devinfo(gii_input *inp, enum quartz_devtype devtype)
{
	gii_event ev;
	gii_cmddata_getdevinfo *dinfo;
	size_t size;
	quartz_priv *priv = QUARTZ_PRIV(inp);

	size = sizeof(gii_cmd_nodata_event) + sizeof(gii_cmddata_getdevinfo);

	_giiEventBlank(&ev, size);

	ev.any.size = size;
	ev.any.type = evCommand;
	ev.any.origin = priv->origin[devtype];
	ev.cmd.code = GII_CMDCODE_GETDEVINFO;

	dinfo = (gii_cmddata_getdevinfo *) ev.cmd.data;
	switch (devtype) {
	case QZ_DEV_MOUSE:
		*dinfo = mouse_devinfo;
		break;
	case QZ_DEV_KEY:
		*dinfo = key_devinfo;
		break;
	default:
		return GGI_EEVNOTARGET;
	}

	return _giiEvQueueAdd(inp, &ev);
}	/* send_devinfo */


static int GII_quartz_send_event(gii_input *inp, gii_event *ev)
{
	quartz_priv *priv = QUARTZ_PRIV(inp);

	if ((ev->any.target & 0xffffff00) != inp->origin &&
	    ev->any.target != GII_EV_TARGET_ALL)
	{
		/* not for us */
		return GGI_EEVNOTARGET;
	}

	if (ev->any.type != evCommand) {
		return GGI_EEVUNKNOWN;
	}

	switch (ev->cmd.code) {
	case GII_CMDCODE_GETDEVINFO:
		if (ev->any.target == GII_EV_TARGET_ALL) {
			send_devinfo(inp, QZ_DEV_KEY);
			send_devinfo(inp, QZ_DEV_MOUSE);
			return GGI_OK;
		} else {
			if (ev->any.target == priv->origin[QZ_DEV_KEY]) {
				send_devinfo(inp, QZ_DEV_KEY);
				return GGI_OK;
			}
			if (ev->any.target == priv->origin[QZ_DEV_MOUSE]) {
				send_devinfo(inp, QZ_DEV_MOUSE);
				return GGI_OK;
			}
			return GGI_EEVNOTARGET;
		}
		break;

	case GII_CMDCODE_QZSETPARAM:
		do {
			int err = GGI_OK;
			gii_quartz_cmddata_setparam data;

			/* Assure aligned memory access. */
			memcpy(&data, (gii_quartz_cmddata_setparam *)ev->cmd.data,
				sizeof(gii_quartz_cmddata_setparam));

			if (data.flags & GII_QZFLAG_UPDATE_WINDOW) {
				priv->theWindow = data.theWindow;

				err = update_winparam(inp);
			} else if (data.flags & GII_QZFLAG_UPDATE_RESIZEFUNC) {
				priv->resizefunc = data.resizefunc;
			}
			return err;
		} while(0);
		break;

	default:
		break;
	}

	return GGI_EEVUNKNOWN;
}	/* GII_quartz_send_event */


static int GII_quartz_close(gii_input *inp)
{
	quartz_priv *priv = QUARTZ_PRIV(inp);

	QuartzExit(priv);
	free(priv);

	DPRINT_LIBS("exit ok.\n");

	return 0;
}	/* GII_quartz_close */


EXPORTFUNC int GIIdl_quartz(gii_input *inp, const char *args, void *argptr);

int GIIdl_quartz(gii_input *inp, const char *args, void *argptr)
{
	int rc = 0;
	gii_inputquartz_arg *quartzarg = argptr;
	quartz_priv *priv;

	DPRINT_MISC("quartz input starting. (args=%s,argptr=%p)\n",
		args, argptr);

	priv = calloc(1, sizeof(quartz_priv));
	if (priv == NULL) {
		rc = GGI_ENOMEM;
		goto err0;
	}

	/* Attention: quartzarg->theWindow is invalid at this point.
	 * It becomes valid once a mode has been set up.
	 */
	priv->theWindow = quartzarg->theWindow;
#if 0
	LIB_ASSERT(priv->theWindow != NULL, "quartzarg->theWindow is NULL\n");
#endif

	if (0 == (priv->origin[QZ_DEV_KEY] =
		_giiRegisterDevice(inp, &key_devinfo, NULL)))
	{
		rc = GGI_ENODEVICE;
		goto err1;
	}	/* if */

	if (0 == (priv->origin[QZ_DEV_MOUSE] =
		_giiRegisterDevice(inp, &mouse_devinfo, NULL)))
	{
		rc = GGI_ENODEVICE;
		goto err1;
	}	/* if */


	if (GGI_OK != QuartzInit(priv)) {
		rc = GGI_ENODEVICE;
		goto err2;
	}

	inp->GIIsendevent    = GII_quartz_send_event;
	inp->GIIeventpoll    = GII_quartz_eventpoll;
#if 0
	inp->GIIseteventmask = GII_quartz_seteventmask;
	inp->GIIgeteventmask = GII_quartz_geteventmaks;
#endif
	inp->GIIclose        = GII_quartz_close;

	inp->targetcan = emKey | emPointer;
	inp->curreventmask = emKey | emPointer;

	inp->maxfd = 0; /* We poll from an event queue - ouch! */
	inp->flags |= GII_FLAGS_HASPOLLED;
	inp->priv = priv;

	inp->GIIseteventmask(inp, inp->targetcan);

	if (GGI_OK != QuartzFinishLaunch(inp)) {
		rc = GGI_ENODEVICE;
		goto err2;
	}

	send_devinfo(inp, QZ_DEV_KEY);
	send_devinfo(inp, QZ_DEV_MOUSE);

	DPRINT_MISC("quartz input fully up\n");

	return GGI_OK;

err2:
err1:
	GII_quartz_close(inp);
err0:
	return rc;
}	/* GIIdlinit */
