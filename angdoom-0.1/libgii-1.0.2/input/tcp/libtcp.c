/* $Id: libtcp.c,v 1.13 2005/07/29 16:40:58 soyt Exp $
******************************************************************************

   libtcp.c - functions to support GII events over TCP.

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
#include <ggi/internal/gii-dl.h>
#include <ggi/internal/gii_debug.h>
#include "libtcp.h"

#if defined(__WIN32__) && !defined(__CYGWIN__)
#  ifdef HAVE_WINSOCK2_H
#    include <winsock2.h>
#  endif
#  ifdef HAVE_WINSOCK_H
#    include <winsock.h>
#  endif
#endif

#ifdef HAVE_SYS_TYPES_H
# include <sys/types.h>
#endif

#if !defined(__WIN32__) || defined(__CYGWIN__)
# ifdef HAVE_SYS_SOCKET_H
#   include <sys/socket.h>
# endif
# ifdef HAVE_NETINET_IN_H
#   include <netinet/in.h>
# endif
# ifdef HAVE_ARPA_INET_H
#   include <arpa/inet.h>
# endif
# ifdef HAVE_NETDB_H
#   include <netdb.h>
# endif
#endif

#ifdef HAVE_SYS_UN_H
# include <sys/un.h>
#endif


#include <stdio.h>
#include <string.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif


int
_gii_tcp_htonev(gii_event *ev)
{
	int i, cnt;

	/* Convert to network byte order. */
	ev->any.error = htons(ev->any.error);
	ev->any.origin = htonl(ev->any.origin);
	ev->any.target = htonl(ev->any.target);
	/* 64-bit time values will be truncated, fix before 2038... */
	ev->any.time.tv_sec = htonl(((uint32_t)ev->any.time.tv_sec));
	ev->any.time.tv_usec = htonl(((uint32_t)ev->any.time.tv_usec));

	switch (ev->any.type) {
	case evKeyPress:
	case evKeyRelease:
	case evKeyRepeat:
		ev->key.modifiers = htonl(ev->key.modifiers);
		ev->key.sym = htonl(ev->key.sym);
		ev->key.label = htonl(ev->key.label);
		ev->key.button = htonl(ev->key.button);
		break;
	case evPtrRelative:
	case evPtrAbsolute:
		ev->pmove.x = htonl(ev->pmove.x);
		ev->pmove.y = htonl(ev->pmove.y);
		ev->pmove.z = htonl(ev->pmove.z);
		ev->pmove.wheel = htonl(ev->pmove.wheel);
		break;
	case evPtrButtonPress:
	case evPtrButtonRelease:
		ev->pbutton.button = htonl(ev->pbutton.button);
		break;
	case evValRelative:
	case evValAbsolute:
		ev->val.first = htonl(ev->val.first);
		cnt = ev->val.count;
		ev->val.count = htonl(cnt);
		for (i = 0; i < cnt; i++) {
			ev->val.value[i] = htonl(ev->val.value[i]);
		}
		break;
	default:
		/* Unable to convert this event. */
		return GGI_EEVUNKNOWN;
	}

	return 0;
}


int
_gii_tcp_ntohev(gii_event *ev)
{
	int i, cnt;

	/* Convert to network byte order. */
	ev->any.error = ntohs(ev->any.error);
	ev->any.origin = ntohl(ev->any.origin);
	ev->any.target = ntohl(ev->any.target);
	/* 64-bit time values will be truncated, fix before 2038... */
	ev->any.time.tv_sec = ntohl(((uint32_t)ev->any.time.tv_sec));
	ev->any.time.tv_usec = ntohl(((uint32_t)ev->any.time.tv_usec));

	switch (ev->any.type) {
	case evKeyPress:
	case evKeyRelease:
	case evKeyRepeat:
		ev->key.modifiers = ntohl(ev->key.modifiers);
		ev->key.sym = ntohl(ev->key.sym);
		ev->key.label = ntohl(ev->key.label);
		ev->key.button = ntohl(ev->key.button);
		break;
	case evPtrRelative:
	case evPtrAbsolute:
		ev->pmove.x = ntohl(ev->pmove.x);
		ev->pmove.y = ntohl(ev->pmove.y);
		ev->pmove.z = ntohl(ev->pmove.z);
		ev->pmove.wheel = ntohl(ev->pmove.wheel);
		break;
	case evPtrButtonPress:
	case evPtrButtonRelease:
		ev->pbutton.button = ntohl(ev->pbutton.button);
		break;
	case evValRelative:
	case evValAbsolute:
		ev->val.first = ntohl(ev->val.first);
		cnt = ev->val.count;
		ev->val.count = ntohl(cnt);
		for (i = 0; i < cnt; i++) {
			ev->val.value[i] = ntohl(ev->val.value[i]);
		}
		break;
	default:
		/* Unable to convert this event. */
		return GGI_EEVUNKNOWN;
	}

	return 0;
}


int
_gii_tcp_connect(gii_tcp_priv *priv, const char *host, int port)
{
	struct hostent *hent;
	struct sockaddr_in addr;
	struct in_addr in;
	int sockfd;

	/* Find out IP address.
	   gethostbyname() is not threadsafe, so we need to lock here. */
	ggLock(priv->lock);
	hent = gethostbyname(host);
	if (hent) {
		int type;

		type = hent->h_addrtype;
		if (type != AF_INET) {
			ggUnlock(priv->lock);
#ifdef AF_INET6
			if (type == AF_INET6) {
				fprintf(stderr, "giitcp: IPV6 addresses not supported yet\n");
			} else
#endif
				fprintf(stderr,
					"giitcp: Unknown address type: %d\n",
					type);
			return GGI_ENOTFOUND;
		}
		memcpy(&in, hent->h_addr, sizeof(in));
		ggUnlock(priv->lock);
	} else {
		ggUnlock(priv->lock);
#ifdef HAVE_INET_ATON
		if (!inet_aton(host, &in)) {
#else
		if ((signed)(in.s_addr = inet_addr(host)) == -1) {
#endif
			fprintf(stderr,
				"giitcp: Unknown or invalid address: %s\n",
				host);
			return GGI_EUNKNOWN;
		}
	}

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("giitcp");
		return GGI_ENOFILE;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr = in;
	addr.sin_port = htons(port);
	if (connect(sockfd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
		perror("giitcp: connection failed");
		return GGI_ENODEVICE;
	}
	priv->fd = sockfd;
	priv->state = GIITCP_CONNECTED;

	return 0;
}


int
_gii_tcp_listen(gii_tcp_priv *priv, int port)
{
	struct sockaddr_in addr;
	int sockfd;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd < 0) {
		perror("giitcp: unable to create socket");
		return GGI_ENODEVICE;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port=htons(port);
	if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) != 0) {
		perror("giitcp: unable to bind socket");
		_gii_tcp_close(sockfd);
		return GGI_ENODEVICE;
	}

	if (listen(sockfd, 1) != 0) {
		perror("giitcp: unable to listen to socket");
		_gii_tcp_close(sockfd);
		return GGI_ENODEVICE;
	}

	priv->listenfd = sockfd;
	priv->state = GIITCP_LISTEN;

	return 0;
}


int
_gii_tcp_accept(gii_tcp_priv *priv)
{
#ifdef __WIN32__
	struct sockaddr addr;
#else
	struct sockaddr_in addr;
#endif
#ifdef HAVE_SOCKLEN_T
	socklen_t size = sizeof(addr);
#else
	int size = sizeof(addr);
#endif

	int fd;

	fd = accept(priv->listenfd, (struct sockaddr *)&addr, &size);
	if (fd < 0) {
		perror("giitcp: unable to accept connection");
		return GGI_ENODEVICE;
	}

	priv->fd = fd;
	priv->state = GIITCP_CONNECTED;

	return 0;
}


int
_gii_tcp_close(int fd)
{
#if defined(__WIN32__) && !defined(__CYGWIN__)
	return closesocket(fd);
#else
	return close(fd);
#endif
}
