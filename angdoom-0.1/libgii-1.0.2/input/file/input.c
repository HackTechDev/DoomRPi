/* $Id: input.c,v 1.14 2005/08/04 12:43:26 cegger Exp $
******************************************************************************

   Input-file: Read events from a file saved by filter-save

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

#include "config.h"

#include <ggi/gg.h>
#include <ggi/internal/gg_replace.h>
#include <ggi/internal/gii-dl.h>
#include <ggi/internal/gii_debug.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>


enum outtype { STDIN, FIL, PIPE };

typedef struct {
	enum outtype	 type;
	FILE		*fil;
	struct timeval   start_here;
	struct timeval   start_file;
	gii_event	 event;
	uint8_t		*datastart;
} file_priv;

#define FILE_PRIV(inp)  ((file_priv *) inp->priv)

static inline int
read_event(file_priv *priv)
{
	if (fread(&priv->event, 1, 1, priv->fil) != 1) return 0;
	DPRINT_EVENTS("input-file: got event of size: %d\n",
		      priv->event.size);
	if (fread(priv->datastart, (priv->event.size - 1U), 1, priv->fil)
	    != 1) {
		return 0;
	}

	return 1;
}

static inline int is_event_ready(gii_input *inp)
{
	file_priv *priv = inp->priv;
	struct timeval tv;
	long milli_here, milli_file;

	ggCurTime(&tv);

	milli_here = (tv.tv_sec  - priv->start_here.tv_sec)  * 1000 +
	             (tv.tv_usec - priv->start_here.tv_usec) / 1000;

	milli_file = (priv->event.any.time.tv_sec  - 
			priv->start_file.tv_sec)  * 1000 +
	             (priv->event.any.time.tv_usec - 
			priv->start_file.tv_usec) / 1000;

	if (milli_here >= milli_file) {
		priv->event.any.time = tv;
		return 1;
	}

	return 0;
}

static gii_event_mask GII_file_poll(gii_input *inp, void *arg)
{
	file_priv *priv = inp->priv;
	int rc = 0;

	DPRINT_EVENTS("GII_file_poll(%p, %p) called\n", inp, arg);
	
	while (is_event_ready(inp)) {

		rc |= (1 << priv->event.any.type);
		_giiEvQueueAdd(inp, &priv->event);

		if (!read_event(priv)) {
			/* No more events to be read */
			inp->curreventmask = inp->targetcan = 0;
			inp->flags = 0;
			inp->GIIeventpoll = NULL;
			_giiUpdateCache(inp);

			return rc;
		}
	}

	return rc;
}

static int GII_file_close(gii_input *inp)
{
	file_priv *priv = inp->priv;
	
	DPRINT_LIBS("GII_file_close(%p) called\n", inp);
	
	fflush(priv->fil);
	
	switch (priv->type) {
	case FIL:
		fclose(priv->fil);
		break;
	case PIPE:
		pclose(priv->fil);
		break;
	default: 
		break;
	}
	free(priv);

	DPRINT_LIBS("GII_file_close done\n");

	return 0;
}


static gii_cmddata_getdevinfo devinfo =
{
	"File",				/* long device name */
	"file",				/* shorthand */
	emAll,				/* all event types */
	~(0U),				/* all buttons */
	~(0U),				/* all valuators */
};

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


EXPORTFUNC int GIIdl_file(gii_input *inp, const char *args, void *argptr);

int GIIdl_file(gii_input *inp, const char *args, void *argptr)
{
	struct timeval tv;
	file_priv *priv;

	DPRINT_LIBS("input-file init(%p, \"%s\") called\n", inp,
		    args ? args : "");

	priv = malloc(sizeof(file_priv));
	if (priv == NULL) {
		return GGI_ENOMEM;
	}

	if(_giiRegisterDevice(inp,&devinfo,NULL)==0) {
		free(priv);
		return GGI_ENOMEM;
	}

	if (args == NULL || *args == '\0') {
		priv->type = STDIN;
		priv->fil = stdin;
	} else {
		if (*args == '|') {
			DPRINT_LIBS("input-file: pipe\n");
			fflush(stdin);
			priv->fil = popen(args+1, "rb");
			priv->type = PIPE;
		} else {
			DPRINT_LIBS("input-file: file\n");
			priv->fil = fopen(args, "rb");
			priv->type = FIL;
		}
		if (priv->fil == NULL) {
			free(priv);
			
			return GGI_ENODEVICE;
		}
	}
	
	priv->datastart = ((uint8_t*)(&priv->event)) + 1;
	
	inp->priv = priv;

	DPRINT_EVENTS("input-file: reading first event\n");

	if (!read_event(priv)) {
		GII_file_close(inp);
		return GGI_ENODEVICE;
	}
	
	ggCurTime(&tv);

	priv->start_here = tv;
	priv->start_file = priv->event.any.time;

	DPRINT_EVENTS("input-file: start_here=(%d,%d) start_file=(%d,%d)", 
		      priv->start_here.tv_sec, priv->start_here.tv_usec,
		      priv->start_file.tv_sec, priv->start_file.tv_usec);

	inp->maxfd = 0;
	inp->curreventmask = inp->targetcan = emAll;
	inp->flags = GII_FLAGS_HASPOLLED;

	inp->GIIsendevent = GIIsendevent;
	inp->GIIeventpoll = GII_file_poll;
	inp->GIIclose     = GII_file_close;

	send_devinfo(inp);

	DPRINT_LIBS("input-file fully up\n");

	return 0;
}
