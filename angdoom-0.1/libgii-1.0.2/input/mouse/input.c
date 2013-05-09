/* $Id: input.c,v 1.14 2005/08/04 12:43:29 cegger Exp $
******************************************************************************

   Mouse inputlib init function

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
#include "mouse.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#define SKIPWHITE(str)	while (isspace((uint8_t)*(str))) (str)++

static gii_cmddata_getdevinfo devinfo =
{
	"Raw Mouse",	/* long device name */
	"rmse",		/* shorthand */
	emPointer,	/* can_generate */
	4,		/* num_buttons	(no supported device have more) */
	0		/* num_axes 	(only for valuators) */
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


static inline parser_type *match_mtype(char *mtype) {
	int i, j;

	SKIPWHITE(mtype);
	if (*mtype == ',') mtype++;
	SKIPWHITE(mtype);

	for (i = 0; _gii_mouse_parsers[i] != NULL; i++) {
		for (j = 0; _gii_mouse_parsers[i]->names[j] != NULL; j++) {
			if (strcasecmp(mtype,
				       _gii_mouse_parsers[i]->names[j]) == 0) {
				return _gii_mouse_parsers[i];
			}
		}
	}
	return NULL;
}


EXPORTFUNC int GIIdl_mouse(gii_input *inp, const char *args, void *argptr);

int GIIdl_mouse(gii_input *inp, const char *args, void *argptr)
{
	char *mtype;
	parser_type *parser;
	mouse_priv *mpriv;
	int fd, fallback_parser = 0;
	
	if (! (args && *args
	  && (fd = strtol(args, &mtype, 0)) >= 0 && mtype != args
	  /* FIXME - we should allow PnP for the mousetype */
	  && *mtype != '\0'))
	{
		return GGI_EARGREQ;
	}

	if ((parser = match_mtype(mtype)) == NULL) {
		return GGI_EARGINVAL;
	}
	if (parser->init_data) {
		if (write(fd, parser->init_data, (unsigned)parser->init_len)
		    != parser->init_len)
		{
			switch(parser->init_type) {
			case GII_MIT_DONTCARE:
				break;
			case GII_MIT_MUST:
				return GGI_ENODEVICE;
			case GII_MIT_FALLBACK:
				fallback_parser = 1;
				break;
			}
		}
	}

	

	if ((mpriv = malloc(sizeof(mouse_priv))) == NULL) {
		return GGI_ENOMEM;
	}

	if(_giiRegisterDevice(inp,&devinfo,NULL)==0) {
		free(mpriv);
		return GGI_ENOMEM;
	}

	inp->GIIsendevent = GIIsendevent;
	inp->GIIeventpoll = GII_mouse_poll;
	inp->GIIclose = NULL;

	inp->targetcan = emPointer | emCommand;
	inp->curreventmask = emPointer | emCommand;

	inp->maxfd = fd + 1;
	FD_SET((unsigned)(fd), &inp->fdset);

	if (fallback_parser) {
		mpriv->parser = parser->fbparser->parser;
	} else {
		mpriv->parser = parser->parser;
	}
	mpriv->min_packet_len = parser->min_packet_len;
	mpriv->fd = fd;
	mpriv->eof = 0;

	mpriv->parse_state = mpriv->button_state = 0;
	mpriv->packet_len = 0;
	mpriv->sent = 0;

	inp->priv = mpriv;

	send_devinfo(inp);

	DPRINT_LIBS("mouse fully up\n");

	return 0;
}
