/* $Id: input.c,v 1.14 2005/08/04 12:43:26 cegger Exp $
******************************************************************************

   fdselect inputlib - send information when filedescriptors become

   Copyright (C) 1999 Alexander Peuchert	[peuc@comnets.rwth-aachen.de]
   Copyright (C) 1999 Andreas Beck		[becka@ggi-project.org]

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
#include <ctype.h>

#include "config.h"
#include <ggi/gg.h>
#include <ggi/internal/gii-dl.h>
#include <ggi/internal/gii_debug.h>

#define FD_OPTIONS  3

static gg_option fd_options[] =
{
	{ "read",   "" },
	{ "write",  "" },
	{ "except", "" }
};

typedef struct fd_hook
{
	int    maxread,maxwrite,maxexcept;
        fd_set readfds,writefds,exceptfds;
        int    maxfd;
} fd_priv;

#define FD_PRIV(inp)  ((fd_priv *) inp->priv)

/* ---------------------------------------------------------------------- */

static gii_event_mask GII_fd_poll(gii_input *inp, void *arg)
{
	fd_priv	*priv = FD_PRIV(inp);
	struct timeval	t={0,0};
	gii_event ev;
	
	DPRINT_EVENTS("input-fdselect: poll(%p);\n",inp);
	
	if (select(priv->maxfd, &priv->readfds, &priv->writefds, 
		   &priv->exceptfds, &t) <= 0) {
		return 0;
	}

	/* at least one of the fds is readable -> send information event */

	_giiEventBlank(&ev, sizeof(gii_cmd_nodata_event));

	ev.any.size=sizeof(gii_cmd_nodata_event);
	ev.any.type=evInformation;
	ev.any.origin=inp->origin;
	ev.cmd.code   = GII_CMDFLAG_PRIVATE;
	/* fixme - be more precise ! */
	_giiEvQueueAdd(inp, &ev);

	return evInformation;
}


static int GII_fd_close(gii_input *inp)
{
	fd_priv *priv = FD_PRIV(inp);

	free(priv);

	DPRINT_MISC("input-fdselect: closed\n");

	return 0;
}


static gii_cmddata_getdevinfo devinfo =
{
	"File descriptor input",	/* long device name */
	"fd",				/* shorthand */
	emInformation,	                /* can_generate */
	0,				/* no buttons */
	0				/* no valuators */
};

static void send_devinfo(gii_input *inp)
{
	gii_event ev;
	gii_cmddata_getdevinfo *dinfo;
	int size = sizeof(gii_cmd_nodata_event)+sizeof(gii_cmddata_getdevinfo);

	_giiEventBlank(&ev, size);
	
	ev.any.size   = size;
	ev.any.type   = evCommand;
	ev.any.origin = inp->origin;
	ev.cmd.code   = GII_CMDCODE_GETDEVINFO;

	dinfo = (gii_cmddata_getdevinfo *) ev.cmd.data;
	*dinfo = devinfo;

	_giiEvQueueAdd(inp, &ev);
}


static int GII_fd_sendevent(gii_input *inp, gii_event *ev)
{
	if (ev->any.target != inp->origin && ev->any.target
	    != GII_EV_TARGET_ALL) {
		/* not for us */
		return GGI_EEVNOTARGET;
	}

	if (ev->any.type != evCommand) {
	        /* don't understand */
	        return GGI_EEVUNKNOWN;
	}

	if (ev->cmd.code == GII_CMDCODE_GETDEVINFO) {
		send_devinfo(inp);
		return 0;
	}

	return GGI_EEVUNKNOWN;	/* Unknown command */
}

#define SKIPNONNUM(str)	while(!isdigit((uint8_t)(*str)) && (*str) != '\0'){str++;}
static int dosetup(const char *result,fd_set *fdset,int *maxfd)
{
	SKIPNONNUM(result);

	while (*result != '\0') {
		int curfd;
		char *remain;
		
		curfd = strtol(result, &remain, 0);
		if (remain == result) {
			/* Parse error */
			return GGI_ENOMATCH;
		}

		FD_SET(curfd,fdset);
		if (curfd+1 > *maxfd) *maxfd = curfd+1;

		result = remain;
		SKIPNONNUM(result);
	}

	return 0;
}


EXPORTFUNC int GIIdl_fdselect(gii_input *inp, const char *args, void *argptr);

int GIIdl_fdselect(gii_input *inp, const char *args, void *argptr)
{
	fd_priv *priv;

	DPRINT_MISC("input-fdselect starting.(args=\"%s\",argptr=%p)\n",
		    args, argptr);
	
	/* handle args */
	if (args) {
		args = ggParseOptions((char *)args, fd_options, FD_OPTIONS);
		if (args == NULL) {
			fprintf(stderr, "input-fdselect: error in arguments.\n");
			return GGI_EARGINVAL;
		}
	}

	/* allocate private stuff */
	if ((priv = malloc(sizeof(fd_priv))) == NULL) {
		return GGI_ENOMEM;
	}
	
	if(_giiRegisterDevice(inp,&devinfo,NULL)==0) {
		free(priv);
		return GGI_ENOMEM;
	}

	inp->priv = priv;
	priv->maxread = priv->maxwrite = priv->maxexcept = priv->maxfd = 0;
        FD_ZERO(&priv->readfds);
        FD_ZERO(&priv->writefds);
        FD_ZERO(&priv->exceptfds);

	if (dosetup(fd_options[0].result, &priv->readfds, &priv->maxread)) {
		DPRINT_MISC("input-fdselect: error parsing -read arguments\n");
	}
	if (priv->maxread > priv->maxfd) priv->maxfd = priv->maxread;
	if (dosetup(fd_options[1].result, &priv->writefds, &priv->maxwrite)) {
		DPRINT_MISC("input-fdselect: error parsing -write arguments\n");
	}
	if (priv->maxwrite > priv->maxfd) priv->maxfd = priv->maxwrite;
	if (dosetup(fd_options[2].result, &priv->exceptfds,&priv->maxexcept)) {
		DPRINT_MISC("input-fdselect: error parsing -except arguments\n");
	}
	if (priv->maxexcept > priv->maxfd) priv->maxfd = priv->maxexcept;

	if (priv->maxfd < 1) {
		DPRINT_MISC("input-fdselect: no argument[s] given\n");
		free(priv);
		return GGI_EARGREQ;
	}

	inp->curreventmask = inp->targetcan = emInformation;
	dosetup(fd_options[0].result, &(inp->fdset), &inp->maxfd);
	/* Add read fds */

	if (priv->maxwrite || priv->maxexcept) {
		/* Oh - we have non-read FDs. Then we must poll. */
		inp->flags |= GII_FLAGS_HASPOLLED;
	}

	inp->GIIsendevent = GII_fd_sendevent;
	inp->GIIeventpoll = GII_fd_poll;
	inp->GIIclose = GII_fd_close;

	send_devinfo(inp);

	DPRINT_MISC("input-fdselect fully up\n");

	return 0;
}
