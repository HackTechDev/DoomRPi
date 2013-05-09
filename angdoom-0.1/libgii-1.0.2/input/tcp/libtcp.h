/* $Id: libtcp.h,v 1.3 2005/07/29 16:40:58 soyt Exp $
******************************************************************************

   libtcp.h - functions to support GII events over TCP.

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

/* Connection states. */
#define GIITCP_NOCONN		0
#define GIITCP_LISTEN		1
#define GIITCP_CONNECTED	2

#define GIITCP_BUFSIZE	512
typedef struct {
	int state;
	int listenfd;
	int fd;
	void *lock;
	uint8_t buf[GIITCP_BUFSIZE];
	size_t count;
} gii_tcp_priv;

#define GII_TCP_PRIV(inp)  ((gii_tcp_priv *) (inp)->priv)


int _gii_tcp_htonev(gii_event *ev);

int _gii_tcp_ntohev(gii_event *ev);

int _gii_tcp_connect(gii_tcp_priv *priv, const char *host, int port);

int _gii_tcp_listen(gii_tcp_priv *priv, int port);

int _gii_tcp_accept(gii_tcp_priv *priv);

int _gii_tcp_close(int fd);
