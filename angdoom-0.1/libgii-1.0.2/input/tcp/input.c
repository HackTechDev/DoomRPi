/* $Id: input.c,v 1.17 2005/08/04 12:43:31 cegger Exp $
******************************************************************************

   Input-tcp: Read events from a TCP-socket

   Copyright (C) 2000 Marcus Sundberg	[marcus@ggi-project.org]

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

#include "libtcp.h"

#include <stdlib.h>
#include <string.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif


static gii_cmddata_getdevinfo devinfo =
{
	"TCP",				/* long device name */
	"tcp",				/* shorthand */
	emAll,				/* all event types */
	GII_NUM_UNKNOWN,		/* all buttons */
	GII_NUM_UNKNOWN,		/* all valuators */
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


static int GII_tcp_sendevent(gii_input *inp, gii_event *ev)
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


static gii_event_mask handle_packets(gii_input *inp)
{
	gii_tcp_priv *priv = inp->priv;
	gii_event *ev;
	int rc = 0;

	ev = (gii_event*)priv->buf;
	while ((priv->count) && (priv->count >= ev->any.size)) {
		if (_gii_tcp_ntohev(ev) == 0) {
			rc |= (1 << ev->any.type);
			DPRINT_EVENTS("input-tcp: Got event type %d, size %d\n",
				      ev->any.type, ev->any.size);

			ev->any.origin = inp->origin;
			_giiEvQueueAdd(inp, ev);
		} else {
			DPRINT_EVENTS("input-tcp: Got UNSUPPORTED event type %d, size %d\n",
				      ev->any.type, ev->any.size);
		}

		priv->count -= ev->any.size;
		ev = (gii_event*)((uint8_t*)ev + ev->any.size);
	}
	if (priv->count) {
		memmove(priv->buf, ev, priv->count);
	}

	return rc;
}


static gii_event_mask GII_tcp_poll(gii_input *inp, void *arg)
{
	gii_tcp_priv *priv = inp->priv;
	size_t read_len;

	DPRINT_EVENTS("GII_tcp_eventpoll(%p) called\n", inp);

	if (!priv->state) return 0;

	if (arg == NULL) {
		fd_set fds = inp->fdset;
		struct timeval tv = { 0, 0 };
		if (select(inp->maxfd, &fds, NULL, NULL, &tv) <= 0) {
			DPRINT_EVENTS("GII_tcp_poll: dummypoll 1\n");
			return 0;
		}
	} else {
		int fd;

		if (priv->state == GIITCP_LISTEN) {
			fd = priv->listenfd;
		} else {
			fd = priv->fd;
		}
		if (! FD_ISSET(fd, ((fd_set*)arg))) {
			/* Nothing to read on our fd */
			DPRINT_EVENTS("GII_tcp_poll: dummypoll 2\n");
			return 0;
		}
	}

	if (priv->state == GIITCP_LISTEN) {
		if (_gii_tcp_accept(priv) == 0) {
			inp->maxfd = priv->fd + 1;
			FD_CLR((unsigned)(priv->listenfd), &inp->fdset);
			FD_SET((unsigned)(priv->fd), &inp->fdset);
			_giiUpdateCache(inp);
			fprintf(stderr, "input-tcp: accepted connection\n");
		} else {
			DPRINT_MISC("GII_tcp_poll: failed to accept connection\n");
		}
		return 0;
	}

	read_len = GIITCP_BUFSIZE - priv->count;

	read_len = read(priv->fd, priv->buf + priv->count, read_len);

	if (read_len <= 0) {
		if (read_len == 0) {
			/* Connection has probably been broken. */
			_gii_tcp_close(priv->fd);
			FD_CLR((unsigned)(priv->fd), &inp->fdset);
			if (priv->listenfd != -1) {
				/* Start listening again. */
				priv->state = GIITCP_LISTEN;
				inp->maxfd = priv->listenfd + 1;
				FD_SET((unsigned)(priv->listenfd), &inp->fdset);
				fprintf(stderr,	"input-tcp: starting to listen again\n");
			} else {
				/* Just close the fd. */
				priv->state = GIITCP_NOCONN;
				inp->maxfd = 0;
				fprintf(stderr, "input-tcp: connection closed\n");
			}
			priv->fd = -1;
			_giiUpdateCache(inp);
		}
		return 0;
	}

	priv->count += read_len;

	return handle_packets(inp);
}


static int GII_tcp_close(gii_input *inp)
{
	gii_tcp_priv *priv = inp->priv;

	DPRINT_LIBS("GII_tcp_close(%p) called\n", inp);

	if (priv->fd != -1) _gii_tcp_close(priv->fd);
	if (priv->listenfd != -1) _gii_tcp_close(priv->listenfd);
	if (priv->lock) ggLockDestroy(priv->lock);
	free(priv);

	DPRINT_LIBS("GII_tcp_close done\n");

	return 0;
}


#define MAX_HLEN  256

EXPORTFUNC int GIIdl_tcp(gii_input *inp, const char *args, void *argptr);

int GIIdl_tcp(gii_input *inp, const char *args, void *argptr)
{
	char host[MAX_HLEN];
	const char *portstr;
	size_t hlen;
	int port, err, fd;
	gii_tcp_priv *priv;

	DPRINT_LIBS("input-tcp init(%p, \"%s\") called\n", inp,
		    args ? args : "");

	if (!args || *args == '\0') return GGI_EARGREQ;

	portstr = strchr(args, ':');
	if (!portstr) return GGI_EARGREQ;

	hlen = portstr - args;
	if (hlen >= MAX_HLEN) return GGI_EARGINVAL;
	memcpy(host, args, hlen);
	host[hlen] = '\0';

	portstr++;
	port = strtoul(portstr, NULL, 0);
	if (!port) return GGI_EARGINVAL;

	priv = malloc(sizeof(*priv));
	if (priv == NULL) {
		return GGI_ENOMEM;
	}

	if (_giiRegisterDevice(inp,&devinfo,NULL) == 0) {
		free(priv);
		return GGI_ENOMEM;
	}

	priv->lock = ggLockCreate();
	if (priv->lock == NULL) {
		free(priv);
		return GGI_ENOMEM;
	}
	priv->state = GIITCP_NOCONN;
	priv->listenfd = priv->fd = -1;
	priv->count = 0;

	if (strcasecmp(host, "listen") == 0) {
		err = _gii_tcp_listen(priv, port);
		fd = priv->listenfd;
	} else {
		err = _gii_tcp_connect(priv, host, port);
		fd = priv->fd;
	}
	if (err) return err;

	inp->priv = priv;

	inp->maxfd = fd + 1;
	FD_SET((unsigned)(fd), &inp->fdset);

	inp->curreventmask = inp->targetcan = emAll;

	inp->GIIsendevent = GII_tcp_sendevent;
	inp->GIIeventpoll = GII_tcp_poll;
	inp->GIIclose     = GII_tcp_close;
	
	send_devinfo(inp);

	DPRINT_LIBS("input-tcp fully up\n");

	return 0;
}
