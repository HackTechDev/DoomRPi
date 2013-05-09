/* $Id: filter.c,v 1.15 2005/08/04 16:36:37 soyt Exp $
******************************************************************************

   Filter-tcp: Repeat events to a TCP-socket

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

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

static int
GII_tcp_handler(gii_input *inp, gii_event *event)
{
	gii_tcp_priv *priv = inp->priv;
	struct timeval tv = { 0, 0 };
	gii_event ev;
	fd_set fds;
	int cnt;

	DPRINT_EVENTS("GII_tcp_handler(%p) called (fd: %d)\n",
		      inp, priv->fd);

	if (!priv->state) return 0;

	FD_ZERO(&fds);
	if (priv->state == GIITCP_LISTEN) {
		FD_SET((unsigned)(priv->listenfd), &fds);
		if (select(priv->listenfd + 1, &fds, NULL, NULL, &tv) <= 0) {
			return 0;
		}
		if (_gii_tcp_accept(priv) == 0) {
			fprintf(stderr, "filter-tcp: accepted connection\n");
		} else {
			DPRINT_MISC("GII_tcp_handler: failed to accept connection\n");
		}
		return 0;
	}

	FD_SET((unsigned)(priv->fd), &fds);
	if (select(priv->fd + 1, NULL, &fds, NULL, &tv) <= 0) {
		DPRINT_EVENTS("filter-tcp: unable to write event\n");
		return 0;
	}

	memcpy(&ev, event, event->any.size);

	if (_gii_tcp_htonev(&ev) != 0) {
		/* Unsupported event. */
		return 0;
	}

	cnt = write(priv->fd, &ev, ev.any.size);

	/* Note, casting cnt to unsigned is wrong here, since
	 * write() can return negative value.
	 */
	if (cnt != (signed)(ev.any.size)) {
		if (cnt >= 0) {
			fprintf(stderr,
				"filter-tcp: only wrote %d of %u bytes\n",
				cnt, ev.any.size);
			return 0;
		}
		_gii_tcp_close(priv->fd);
		priv->fd = -1;
		if (priv->listenfd != -1) {
			priv->state = GIITCP_LISTEN;
			fprintf(stderr,
				"filter-tcp: starting to listen again\n");
		} else {
			priv->state = GIITCP_NOCONN;
			fprintf(stderr, "filter-tcp: connection closed\n");
		}
	}

	return 0;
}


static int
GII_tcp_close(gii_input *inp)
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
EXPORTFUNC int GIIdl_filter_tcp(gii_input *inp, const char *args, void *argptr);

int GIIdl_filter_tcp(gii_input *inp, const char *args, void *argptr)
{
	char host[MAX_HLEN];
	const char *portstr;
	size_t hlen;
	int port, err;
	gii_tcp_priv *priv;
	
	DPRINT_LIBS("filter-tcp init(%p, \"%s\") called\n", inp,
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
	} else {
		err = _gii_tcp_connect(priv, host, port);
	}
	if (err) return err;

	inp->priv = priv;

	inp->GIIhandler   = GII_tcp_handler;
	inp->GIIclose     = GII_tcp_close;
	
	DPRINT_LIBS("filter-tcp fully up\n");
	
	return 0;
}
