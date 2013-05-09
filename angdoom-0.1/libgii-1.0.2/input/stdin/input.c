/* $Id: input.c,v 1.17 2005/08/04 12:43:30 cegger Exp $
******************************************************************************

   Input-stdin: initialization

   Copyright (C) 1998 Andreas Beck      [becka@ggi-project.org]

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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_TERMIOS_H
#include <termios.h>
#define USE_TERMIOS
#endif

static const gg_option optlist[] =
{
	{ "noraw",   "no" },
	{ "ansikey", "no" },
};

#define OPT_NORAW	0
#define OPT_ANSIKEY	1

#define NUM_OPTS	(sizeof(optlist)/sizeof(gg_option))


typedef struct stdin_hook
{
	int rawmode, ansikey;

#ifdef USE_TERMIOS
	struct termios old_termios;
#endif
} stdin_priv;

#define STDIN_PRIV(inp)  ((stdin_priv *) inp->priv)

#define STDIN_FD	0


/* ---------------------------------------------------------------------- */


static gii_event_mask GII_send_key(gii_input *inp, uint32_t sym)
{
	gii_event ev;
	
	_giiEventBlank(&ev, sizeof(gii_key_event));

	ev.any.size=sizeof(gii_key_event);
	ev.any.type=evKeyPress;
	ev.any.origin=inp->origin;
	ev.key.modifiers=0;
	ev.key.sym   =sym;
	ev.key.label =sym;
	ev.key.button=sym;
	_giiEvQueueAdd(inp, &ev);

	ev.key.type=evKeyRelease;
	_giiEvQueueAdd(inp, &ev);

	return emKeyPress | emKeyRelease;
}

static gii_event_mask GII_stdin_poll(gii_input *inp, void *arg)
{
	stdin_priv	*priv = STDIN_PRIV(inp);
	fd_set		readset = inp->fdset;
	struct timeval	t={0,0};
	unsigned char	buf[6];
	
	DPRINT_EVENTS("input-stdin: poll(%p);\n",inp);
	
	if (select(inp->maxfd, &readset, NULL, NULL, &t) <= 0) {
		return 0;
	}

	read(STDIN_FD, buf, 1);

	if (! priv->ansikey || (buf[0] != 27 /* escape */)) {
		return GII_send_key(inp, (uint32_t) buf[0]);
	}

	/* Wait a bit, to see if the character following escape was '['
	 * which signals an ANSI key.
	 */

	if (select(inp->maxfd, &readset, NULL, NULL, &t) <= 0) {
		ggUSleep(100 * 1000);  /* wait 1/10th of a second */
	}

	if (select(inp->maxfd, &readset, NULL, NULL, &t) <= 0) {
		/* Timed out : must have been plain escape key */
		return GII_send_key(inp, (uint32_t) buf[0]);
	}
	
	read(STDIN_FD, buf+1, 1);

	if (buf[1] != '[') {
		/* Nope, not an ANSI key sequence */
		GII_send_key(inp, (uint32_t) buf[0]);
		return GII_send_key(inp, (uint32_t) buf[1]);
	}
	
	/* handle the ANSI key sequences */

	read(STDIN_FD, buf+2, 1);

	buf[3] = buf[4] = buf[5] = 0;

	if (isdigit((uint8_t)(buf[2])) || (buf[2] == '[')) {
	    read(STDIN_FD, buf+3, 1);
	}
	if (isdigit((uint8_t)(buf[3]))) {
	    read(STDIN_FD, buf+4, 1);
	}
	
#define CMP_KEY(S,K) if (strcmp((const char*)buf+2, (S)) == 0) {GII_send_key(inp, (K));}
		
	CMP_KEY("A", GIIK_Up);    CMP_KEY("B", GIIK_Down);
	CMP_KEY("C", GIIK_Right); CMP_KEY("D", GIIK_Left);

	CMP_KEY("1~", GIIK_Home);   CMP_KEY("4~", GIIK_End);
	CMP_KEY("2~", GIIK_Insert); CMP_KEY("3~", GIIUC_Delete);
	CMP_KEY("5~", GIIK_PageUp); CMP_KEY("6~", GIIK_PageDown);

	CMP_KEY("[A",  GIIK_F1); CMP_KEY("[B",  GIIK_F2); 
	CMP_KEY("[C",  GIIK_F3); CMP_KEY("[D",  GIIK_F4); 
	CMP_KEY("[E",  GIIK_F5); CMP_KEY("17~", GIIK_F6); 
	CMP_KEY("18~", GIIK_F7); CMP_KEY("19~", GIIK_F8); 
	CMP_KEY("20~", GIIK_F9); CMP_KEY("21~", GIIK_F10); 

#undef CMP_KEY
	
	return 0;
}


static int GII_stdin_close(gii_input *inp)
{
	stdin_priv *priv = STDIN_PRIV(inp);

#ifdef USE_TERMIOS
	if (priv->rawmode) {
		if (tcsetattr(STDIN_FD, TCSANOW, &priv->old_termios) < 0) {
			perror("input-stdin: tcsetattr failed");
		}
		ggUnregisterCleanup((ggcleanup_func *)GII_stdin_close,
				    (void *)inp);
	}
#endif
	free(priv);

	DPRINT_MISC("input-stdin: closed\n");

	return 0;
}


#ifdef USE_TERMIOS
static void GII_stdin_setraw(gii_input *inp)
{
	stdin_priv *priv = STDIN_PRIV(inp);

	struct termios newt;

	/* put the tty into "straight through" mode. */
	if (tcgetattr(STDIN_FD, &priv->old_termios) < 0) {
		perror("input-stdin: tcgetattr failed");
	}

	newt = priv->old_termios;

	newt.c_lflag &= ~(ICANON | ECHO  | ISIG);
	newt.c_iflag &= ~(ISTRIP | IGNCR | ICRNL | INLCR | IXOFF | IXON);
	newt.c_cc[VMIN]  = 0;
	newt.c_cc[VTIME] = 0;

	if (tcsetattr(STDIN_FD, TCSANOW, &newt) < 0) {
		priv->rawmode = 0;
		perror("input-stdin: tcsetattr failed");
	} else {
		ggRegisterCleanup((ggcleanup_func *)GII_stdin_close,
				  (void *)inp);
	}
}
#endif


static gii_cmddata_getdevinfo devinfo =
{
	"Standard input",		/* long device name */
	"stin",				/* shorthand */
	emKeyPress | emKeyRelease,	/* can_generate */
	256,				/* 256 pseudo buttons */
	0				/* no valuators */
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


static int GIIsendevent(gii_input *inp, gii_event *ev)
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


EXPORTFUNC int GIIdl_stdin(gii_input *inp, const char *args, void *argptr);

int GIIdl_stdin(gii_input *inp, const char *args, void *argptr)
{
	const char *str;
	stdin_priv *priv;
	gg_option options[NUM_OPTS];

	DPRINT_MISC("input-stdin starting.(args=\"%s\",argptr=%p)\n",
			args, argptr);

	memcpy(options, optlist, sizeof(options));

	/* handle args */
	str = getenv("GII_STDIN_OPTIONS");
	if (str != NULL) {
		str = ggParseOptions(str, options, NUM_OPTS);
		if (str == NULL) {
			fprintf(stderr, "input-stdin: error in "
				"$GII_STDIN_OPTIONS.\n");
			return GGI_EARGINVAL;
		}
	}
	
	if (args) {
		args = ggParseOptions(args, options, NUM_OPTS);
		if (args == NULL) {
			fprintf(stderr, "input-stdin: error in "
				"arguments.\n");
			return GGI_EARGINVAL;
		}
	}

	/* allocate private stuff */
	if ((priv = malloc(sizeof(stdin_priv))) == NULL) {
		return GGI_ENOMEM;
	}

	if(_giiRegisterDevice(inp,&devinfo,NULL)==0) {
		free(priv);
		return GGI_ENOMEM;
	}

	inp->priv = priv;
        
	if (tolower((uint8_t)options[OPT_ANSIKEY].result[0]) != 'n') {
		priv->ansikey=1;
	} else {
		priv->ansikey = 0;
	}

	priv->rawmode = 0;
#ifdef USE_TERMIOS
	if (tolower((uint8_t)options[OPT_NORAW].result[0]) == 'n') {
		/* turn on `raw' mode (i.e. non-canonical mode) */
		priv->rawmode = 1;
		GII_stdin_setraw(inp);
	}
#endif

	inp->curreventmask = inp->targetcan = emKeyPress | emKeyRelease;
	inp->maxfd = 1;
	FD_SET(STDIN_FD, &(inp->fdset));	/* Add stdin */

	inp->GIIsendevent = GIIsendevent;
	inp->GIIeventpoll = GII_stdin_poll;
	inp->GIIclose = GII_stdin_close;

	send_devinfo(inp);

	DPRINT_MISC("input-stdin fully up\n");

	return 0;
}
