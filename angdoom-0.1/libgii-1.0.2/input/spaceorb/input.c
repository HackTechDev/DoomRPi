/* $Id: input.c,v 1.17 2005/08/04 12:43:30 cegger Exp $
******************************************************************************

   SpaceOrb: input

   Copyright (C) 1998 Andrew Apted     [andrew@ggi-project.org]

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

   This code was derived from the following sources of information:

   [1] Brett Viren's "Unsupported Secrets of the SpaceOrb Protocol"
       web page and his driver code `sorb.c', `sorb.h' and `sorbT.c'
       files.  Damn good stuff.

******************************************************************************
*/

#include "config.h"
#include <ggi/internal/gii.h>
#include <ggi/internal/gii_debug.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <termios.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif


#define DEFAULT_WOBBLE  10

#ifndef ABS
#define ABS(n)	((n) < 0 ? -(n) : (n))
#endif


#define MAX_PACKET_BUF  256
#define MAX_GREETING	100


typedef struct orb_hook
{
	int fd;

	struct termios old_termios;

	int axes[6];
	int buttons;

	/* packet buffer */
	
	int packet_len;
	unsigned char packet_buf[MAX_PACKET_BUF];

	gii_event_mask sent;

} SpaceOrbHook;

#define SPACEORB_HOOK(inp)  ((SpaceOrbHook *) inp->priv)


/* ---------------------------------------------------------------------- */


/**
 **  Event dispatching code
 **/

static inline void
orb_send_axes(gii_input *inp, int axes[6], int last_axes[6], int wobble)
{
	SpaceOrbHook *orb = SPACEORB_HOOK(inp);

	gii_event ev;

	int i, num_changed=0;
	

	_giiEventBlank(&ev, sizeof(gii_val_event));

	ev.any.size   = sizeof(gii_val_event);
	ev.any.type   = evValAbsolute;
	ev.any.origin = inp->origin;

	ev.val.first  = 0;
	ev.val.count  = 6;

	/* check if wobble threshold exceeded */

	for (i=0; i < 6; i++) {

		if (ABS(axes[i] - last_axes[i]) >= wobble) {
			last_axes[i] = axes[i];
			num_changed++;
		}

		ev.val.value[i] = axes[i];
	}
	
	if (num_changed == 0) {
		return;		/* no change */
	}

	_giiEvQueueAdd(inp, &ev);

	orb->sent |= emValAbsolute;
}

static inline void
orb_send_buttons(gii_input *inp, int buttons, int last_buttons)
{
	SpaceOrbHook *orb = SPACEORB_HOOK(inp);
	gii_event ev;
	int i, changed_buttons = buttons ^ last_buttons;

	/* change in button state ? */
	for (i=0; i < 6; i++) {
		if (changed_buttons & (1 << i)) {
			int state = (buttons & (1 << i));
			
			_giiEventBlank(&ev, sizeof(gii_key_event));
			
			ev.any.size   = sizeof(gii_key_event);
			ev.any.type   = state ? evKeyPress : evKeyRelease;
			ev.any.origin = inp->origin;
			
			ev.key.modifiers = 0;
			ev.key.sym    = GIIK_VOID;
			ev.key.label  = GIIK_VOID;
			ev.key.button = 1 + i;
			
			_giiEvQueueAdd(inp, &ev);
			
			orb->sent |= (1 << ev.any.type);
		}
	}
}

/**
 **  SpaceOrb parser
 **/

static inline int
orb_parse_greeting(gii_input *inp, unsigned char *buf, int len)
{
	char name[MAX_GREETING];

	int i;
	int actual;


	DPRINT_EVENTS("spaceorb greeting packet (len=%d).\n", len);

	/* check for trailing CR */

	for (actual=0; actual < len; actual++) {
		if (buf[actual] == '\r')
			break;
	}
	
	if (actual >= (MAX_GREETING-2)) {

		/* Something is wrong, this greeting packet is way too
		 * long.  Maybe the serial port was setup incorrectly,
		 * and the 'R' header byte was just random.  We might
		 * as well ditch the whole packet.
		 */
		 
		return actual;
	}
		 
	if (actual == len) {
		DPRINT_EVENTS("spaceorb: short packet\n");
		return 0;  /* none */
	}
	

	/* show the greeting message */

	buf++;

	for (i=0; i < (actual-1); i++) {
		name[i] = isprint((uint8_t)(buf[i])) ? buf[i] : '.';
	}

	name[i] = 0;

	DPRINT_MISC("SpaceOrb: Device greeting is `%s'.\n", name);

	return actual+1;
}

static inline int
orb_parse_buttons(gii_input *inp, unsigned char *buf, int len)
{
	SpaceOrbHook *orb = SPACEORB_HOOK(inp);
	
	int time_ms;
	int buts;
	int checksum;
	
	DPRINT_EVENTS("spaceorb button packet (len=%d).\n", len);

	if (len < 5) {
		DPRINT_EVENTS("spaceorb: short packet\n");
		return 0;
	}
	
	time_ms  = buf[1];
	buts     = buf[2];
	checksum = buf[4];

	orb_send_buttons(inp, buts, orb->buttons);

	orb->buttons = buts;

	DPRINT_EVENTS("spaceorb button packet OK.\n");

	return 5;
}

static inline int
orb_parse_motion(gii_input *inp, unsigned char *buf, int len)
{
	const char SpaceWare[] = "SpaceWare!";

	SpaceOrbHook *orb = SPACEORB_HOOK(inp);

	int buts;
	int axes[6];
	int checksum;
	
	int i;


	DPRINT_EVENTS("spaceorb motion packet (len=%d).\n", len);

	if (len < 12) {
		DPRINT_EVENTS("spaceorb: short packet\n");
		return 0;
	}
	
	buts     = buf[1];
	checksum = buf[11];

	buf += 2;

	/* convert data */

	for (i=0; i < 9; i++) {
		buf[i] ^= SpaceWare[i];   /* What's this doing in there ? */
	}
	
	/* mask out the axis values */

	axes[0] = ((buf[0] & 0x7f) << 3) | ((buf[1] & 0x70) >> 4);
	axes[1] = ((buf[1] & 0x0f) << 6) | ((buf[2] & 0x7e) >> 1);
	axes[2] = ((buf[2] & 0x01) << 9) | ((buf[3] & 0x7f) << 2) |
			((buf[4] & 0x60) >> 5);
	axes[3] = ((buf[4] & 0x1f) << 5) | ((buf[5] & 0x7c) >> 2);
	axes[4] = ((buf[5] & 0x03) << 8) | ((buf[6] & 0x7f) << 1) |
			((buf[7] & 0x40) >> 6);
	axes[5] = ((buf[7] & 0x3f) << 4) | ((buf[8] & 0x78) >> 3);

	/* get the sign right */

	for (i=0; i < 6; i++) {
		if (axes[i] > 512) {
			axes[i] -= 1024;
		} else if (axes[i] == 512) {
			axes[i] = -511;
		}
		axes[i] *= 64;
	}

	orb_send_axes(inp, axes, orb->axes, DEFAULT_WOBBLE * 64);

	DPRINT_EVENTS("spaceorb motion packet OK.\n");

	return 12;
}

static int M_spaceorb(gii_input *inp, unsigned char *buf, int len)
{
	/* what kind of packet do we have ? */

	switch (buf[0])
	{
		case 'R': return orb_parse_greeting(inp, buf, len);
		case 'K': return orb_parse_buttons(inp,  buf, len);
		case 'D': return orb_parse_motion(inp,   buf, len);

		/* skip any intervening CRs */

		case '\r': return 1;

		default:
			break;
	}

	DPRINT_EVENTS("Invalid spaceorb packet (0x%02x).\n", buf[0]);

	return 1;
}


/* ---------------------------------------------------------------------- */


static int do_spaceorb_open(gii_input *inp, char *filename,
			    int dtr, int rts, int baud)
{
	SpaceOrbHook *orb = SPACEORB_HOOK(inp);
	struct termios tio;

	orb->fd = open(filename, O_RDWR | O_NOCTTY);

	if (orb->fd < 0) {
		perror("SpaceOrb: Failed to open spaceorb device");
		return GGI_ENODEVICE;
	}

	/* set up the termios state and baud rate */

	tcflush(orb->fd, TCIOFLUSH);

	if (tcgetattr(orb->fd, &orb->old_termios) < 0) {
		DPRINT("tcgetattr failed.\n");
/*		close(orb->fd);		*/
/*		return GGI_ENODEVICE;		*/
	}

	tio = orb->old_termios;

	if (baud < 0) {
		baud = B9600;
	}
	
	tio.c_cflag = CREAD | CLOCAL | HUPCL | CS7 | baud;
	tio.c_iflag = IGNBRK;
	tio.c_oflag = 0;
	tio.c_lflag = 0;
	tio.c_cc[VMIN]  = 1;
	tio.c_cc[VTIME] = 0;

	if (tcsetattr(orb->fd, TCSANOW, &tio) < 0) {
		DPRINT("tcsetattr failed.\n");
/*		close(orb->fd);		*/
/*		return GGI_ENODEVICE;		*/
	}

	/* set up RTS and DTR modem lines */
	if ((dtr >= 0) || (rts >= 0)) {
#ifdef HAVE_TIOCMSET
		unsigned int modem_lines;

		if (ioctl(orb->fd, TIOCMGET, &modem_lines) == 0) {

			if (dtr == 0) modem_lines &= ~TIOCM_DTR;
			if (rts == 0) modem_lines &= ~TIOCM_RTS;

			if (dtr > 0) modem_lines |= TIOCM_DTR;
			if (rts > 0) modem_lines |= TIOCM_RTS;
			
			ioctl(orb->fd, TIOCMSET, &modem_lines);
		}
#else /* HAVE_TIOCMSET */
		fprintf(stderr,
			"input-spaceorb: warning, this system does not"
			" support TIOCMSET\n"
			"        device may not work as expected\n");
#endif /* HAVE_TIOCMSET */
	}

	return 0;
}

/* !!! All this parsing stuff is probably best done with the
 * ggParseOption() code, with things like "-file=/dev/spaceorb",
 * "-baud=9600", and that sort of thing...
 */
 
static const char *parse_field(char *dst, int max, const char *src)
{
	int len=1;   /* includes trailing NUL */

	for (; *src && (*src != ','); src++) {

		if (len < max) {
			*dst++ = *src;
			len++;
		}
	}

	*dst = 0;

	if (*src == ',') {
		src++;
	}
	return src;
}

static inline void
parse_spaceorb_specifier(const char *spec, char *_devname, char *options)
{
	*_devname = *options = 0;

	if (spec) {
		parse_field(options, 255,
			    parse_field(_devname, 255, spec));
	}
	
	/* supply defaults for missing bits */
	if (*_devname == 0) {
		strcpy(_devname, "/dev/spaceorb");
	}
}

static char *parse_opt_int(char *opt, int *val)
{
	*val = 0;

	for (; *opt && isdigit((uint8_t)*opt); opt++) {
		*val = ((*val) * 10) + ((*opt) - '0');
	}

	return opt;
}

static void parse_options(char *opt, int *baud, int *dtr, int *rts)
{
	while (*opt) switch (*opt++) {

		case 'b': case 'B':    /* baud */
			opt = parse_opt_int(opt, baud);
			break;

		case 'd': case 'D':    /* dtr */
			opt = parse_opt_int(opt, dtr);
			break;

		case 'r': case 'R':    /* rts */
			opt = parse_opt_int(opt, rts);
			break;

		default:
			fprintf(stderr, "Unknown spaceorb option "
				"'%c' -- rest ignored.\n", *opt);
			return;
	}
}


/* ---------------------------------------------------------------------- */


static inline int
GII_spaceorb_init(gii_input *inp, const char *typname)
{
	SpaceOrbHook *orb = SPACEORB_HOOK(inp);
	char _devname[256];
	char options[256];
	int dtr=-1, rts=-1, baud=-1;
	int ret;

	/* allocate spaceorb private structure */

	if ((orb = inp->priv = malloc(sizeof(SpaceOrbHook))) == NULL) {
		return GGI_ENOMEM;
	}

	/* parse the spaceorb specifier */
	
	parse_spaceorb_specifier(typname, _devname, options);
	parse_options(options, &baud, &dtr, &rts);

	if (strcmp(_devname, "none") == 0) {
		return GGI_ENODEVICE;
	}

 	/* open spaceorb */
	
	if ((ret = do_spaceorb_open(inp, _devname, dtr, rts, baud)) < 0) {
		free(orb);
	}
	return ret;
}

static void GII_spaceorb_exit(gii_input *inp)
{
	SpaceOrbHook *orb = SPACEORB_HOOK(inp);

	if (tcsetattr(orb->fd, TCSANOW, &orb->old_termios) < 0) {
		DPRINT("tcsetattr failed.\n");
	}

	close(orb->fd);
	orb->fd = -1;

	free(orb);
	inp->priv = NULL;

	DPRINT("SpaceOrb: exit OK.\n");
}


static inline gii_event_mask
GII_spaceorb_handle_data(gii_input *inp)
{
	SpaceOrbHook *orb = SPACEORB_HOOK(inp);
	size_t buflen;
	int read_len;

	/* read the spaceorb data */
	buflen = MAX_PACKET_BUF - orb->packet_len - 1;

	/* ASSERT(read_len >= 1) */
	read_len = read(orb->fd, orb->packet_buf + orb->packet_len, 
			buflen);

	if (read_len < 1) {
		perror("SpaceOrb: Error reading spaceorb");
		return 0;
	}
	orb->packet_len += read_len;

	/* parse any packets */
	while (orb->packet_len > 0) {
		int used;

		used = M_spaceorb(inp, orb->packet_buf, orb->packet_len);

		if (used <= 0) {
			break;	 /* not enough data yet */
		}

		orb->packet_len -= used;

		if (orb->packet_len > 0) {
			memmove(orb->packet_buf, orb->packet_buf + used,
				(unsigned)orb->packet_len);
		} else {
			orb->packet_len = 0;
		}
	}
	
	if (buflen == (size_t)read_len) {
		/* Filled the buffer - see if there's more data */
		return 1;
	} else {
		/* Short read - no need to select again */
		return 0;
	}
}


/* ---------------------------------------------------------------------- */


static gii_event_mask GII_spaceorb_poll(gii_input *inp, void *arg)
{
	SpaceOrbHook *priv = SPACEORB_HOOK(inp);
	int doselect = 1;

	DPRINT_EVENTS("GII_spaceorb_poll(%p, %p) called\n", inp, arg);

	if (arg != NULL) {
		if (! FD_ISSET(priv->fd, ((fd_set*)arg))) {
			/* Nothing to read on our fd */
			DPRINT_EVENTS("GII_spaceorb_poll: dummypoll\n");
			return 0;
		}
		doselect = 0;
	}
	
	priv->sent = 0;

	do {
		fd_set fds = inp->fdset;
		struct timeval tv = { 0, 0 };

		if (doselect) {
			if (select(inp->maxfd, &fds, NULL, NULL, &tv) <= 0) {
				return priv->sent;
			}
		} else {
			doselect = 1;
		}
	} while (GII_spaceorb_handle_data(inp));

	return priv->sent;
}


/* ---------------------------------------------------------------------- */


static gii_cmddata_getdevinfo spaceorb_devinfo =
{
	"SpaceOrb 360",			/* long device name */
	"sorb",				/* shorthand */
	emKey |	emValuator,		/* can_generate */
	8,				/* num_buttons */
	6				/* num_axes */
};

static gii_cmddata_getvalinfo spaceorb_valinfo[6] =
{
    {	0,				/* valuator number */
    	"Side to side",			/* long valuator name */
	"tx",				/* shorthand */
	{ -32767, 0, +32767 },		/* range */
	GII_PT_FORCE,			/* phystype */
	0, 256, 328, -8			/* SI constants (bogus!) */
    },
    {	1,				/* valuator number */
    	"Up and down",			/* long valuator name */
	"ty",				/* shorthand */
	{ -32767, 0, +32767 },		/* range */
	GII_PT_FORCE,			/* phystype */
	0, 256, 328, -8			/* SI constants (bogus!) */
    },
    {	2,				/* valuator number */
    	"Forward and back",		/* long valuator name */
	"tz",				/* shorthand */
	{ -32767, 0, +32767 },		/* range */
	GII_PT_FORCE,			/* phystype */
	0, 256, 328, -8			/* SI constants (bogus!) */
    },
    {	3,				/* valuator number */
    	"Rotate about z",		/* long valuator name */
	"rz",				/* shorthand */
	{ -32767, 0, +32767 },		/* range */
	GII_PT_TORQUE,			/* phystype */
	0, 256, 328, -8			/* SI constants (bogus!) */
    },
    {	4,				/* valuator number */
    	"Rotate about x",		/* long valuator name */
	"rx",				/* shorthand */
	{ -32767, 0, +32767 },		/* range */
	GII_PT_TORQUE,			/* phystype */
	0, 256, 328, -8			/* SI constants (bogus!) */
    },
    {	5,				/* valuator number */
    	"Rotate about y",		/* long valuator name */
	"ry",				/* shorthand */
	{ -32767, 0, +32767 },		/* range */
	GII_PT_TORQUE,			/* phystype */
	0, 256, 328, -8			/* SI constants (bogus!) */
    }
};

static int GII_spaceorb_senddevinfo(gii_input *inp)
{
	gii_cmddata_getdevinfo *DI;
	
	gii_event ev;

	_giiEventBlank(&ev, sizeof(gii_cmd_nodata_event) +
		       sizeof(gii_cmddata_getdevinfo));

	ev.any.size   = sizeof(gii_cmd_nodata_event) +
			sizeof(gii_cmddata_getdevinfo);
	ev.any.type   = evCommand;
	ev.any.origin = inp->origin;
	ev.cmd.code   = GII_CMDCODE_GETDEVINFO;

	DI = (gii_cmddata_getdevinfo *) ev.cmd.data;

	*DI = spaceorb_devinfo;

	return _giiEvQueueAdd(inp, &ev);
}

static int GII_spaceorb_sendvalinfo(gii_input *inp, uint32_t val)
{
	gii_cmddata_getvalinfo *VI;

	gii_event ev;

	if (val > 5) return GGI_ENOSPACE;

	_giiEventBlank(&ev, sizeof(gii_cmd_nodata_event) +
		       sizeof(gii_cmddata_getvalinfo));

	ev.any.size   = sizeof(gii_cmd_nodata_event) +
		         sizeof(gii_cmddata_getvalinfo);
	ev.any.type   = evCommand;
	ev.any.origin = inp->origin;
	ev.cmd.code   = GII_CMDCODE_GETVALINFO;

	VI = (gii_cmddata_getvalinfo *) ev.cmd.data;

	*VI = spaceorb_valinfo[val];

	return _giiEvQueueAdd(inp, &ev);
}

static int GII_spaceorb_sendevent(gii_input *inp, gii_event *ev)
{
	if ((ev->any.target != inp->origin) &&
	    (ev->any.target != GII_EV_TARGET_ALL)) {
		/* not for us */
		return GGI_EEVNOTARGET;
	}

	if (ev->any.type != evCommand) {
		return GGI_EEVUNKNOWN;
	}

	if (ev->cmd.code == GII_CMDCODE_GETDEVINFO) {
		return GII_spaceorb_senddevinfo(inp);
	}

	if (ev->cmd.code == GII_CMDCODE_GETVALINFO) {
	
		uint32_t i;
		gii_cmddata_getvalinfo *vi;
		
		vi = (gii_cmddata_getvalinfo *) ev->cmd.data;
		
		if (vi->number == GII_VAL_QUERY_ALL) {
			for (i=0; i < 6; i++) {
				GII_spaceorb_sendvalinfo(inp, i);
			}
			return 0;
		}

		return GII_spaceorb_sendvalinfo(inp, vi->number);
	}

	return GGI_EEVUNKNOWN;  /* Unknown command */
}


/* ---------------------------------------------------------------------- */


static int GII_spaceorb_close(gii_input *inp)
{
	DPRINT_MISC("SpaceOrb cleanup\n");

	if (SPACEORB_HOOK(inp)) {
		GII_spaceorb_exit(inp);
	}

	return 0;
}


EXPORTFUNC int GIIdl_spaceorb(gii_input *inp, const char *args, void *argptr);

int GIIdl_spaceorb(gii_input *inp, const char *args, void *argptr)
{
	int ret;
	const char *spec = "";
	
	DPRINT_MISC("SpaceOrb starting.(args=\"%s\",argptr=%p)\n",args,argptr);

	/* Initialize */

	if (args && *args) {
		spec = args;
	}
	
	if(_giiRegisterDevice(inp,&spaceorb_devinfo,spaceorb_valinfo)==0) {
		return GGI_ENOMEM;
	}

	if ((ret = GII_spaceorb_init(inp, spec)) < 0) {
		return ret;
	}
	
	/* We leave these on the default handlers
	 *	inp->GIIseteventmask = _GIIstdseteventmask;
	 *	inp->GIIgeteventmask = _GIIstdgeteventmask;
	 *	inp->GIIgetselectfdset = _GIIstdgetselectfd;
	 */
	
	inp->GIIeventpoll = GII_spaceorb_poll;
	inp->GIIclose = GII_spaceorb_close;
	inp->GIIsendevent = GII_spaceorb_sendevent;

	inp->targetcan = emKey | emValuator;
	inp->GIIseteventmask(inp, emKey | emValuator);

	inp->maxfd = SPACEORB_HOOK(inp)->fd + 1;
	FD_SET((unsigned)(SPACEORB_HOOK(inp)->fd), &inp->fdset);

	/* Send initial cmdDevInfo event */
	GII_spaceorb_senddevinfo(inp);
	
	DPRINT_MISC("SpaceOrb fully up\n");

	return 0;
}
