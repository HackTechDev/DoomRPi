/* $Id$
******************************************************************************

   Mouse packet parsing

   Copyright (C) 1998 Andrew Apted	[andrew@ggi-project.org]
   Copyright (C) 1998 Marcus Sundberg	[marcus@ggi-project.org]

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
#include <string.h>
#include <unistd.h>
#include <errno.h>


/**
 **  Event dispatching routines
 **/

static void mouse_send_movement(gii_input *inp, int32_t dx, int32_t dy,
				int32_t dz, int32_t wheel)
{
	gii_event ev;

	if ((inp->curreventmask & emPtrRelative) &&
	    (dx || dy || dz || wheel)) {
		_giiEventBlank(&ev, sizeof(gii_pmove_event));

		ev.pmove.size   = sizeof(gii_pmove_event);
		ev.pmove.type   = evPtrRelative;
		ev.pmove.origin = inp->origin;

		ev.pmove.x = dx;
		ev.pmove.y = dy;
		ev.pmove.z = dz;
		ev.pmove.wheel = wheel;

		_giiEvQueueAdd(inp, &ev);

		MOUSE_PRIV(inp)->sent |= emPtrRelative;
	}
}

static void mouse_send_buttons(gii_input *inp, uint32_t buttons, uint32_t last)
{
	gii_event ev;
	uint32_t mask;
	uint32_t changed = buttons ^ last;
	int nr;

	for (nr = 1, mask = 1; mask != 0; nr++, mask <<= 1) {
		if (changed & mask) {
			_giiEventBlank(&ev, sizeof(gii_pbutton_event));

			if (buttons & mask) {
				if (! (inp->curreventmask
				       & emPtrButtonPress)) {
					continue;
				}
				ev.pbutton.type = evPtrButtonPress;
				MOUSE_PRIV(inp)->sent |= emPtrButtonPress;
			} else {
				if (! (inp->curreventmask
				       & emPtrButtonRelease)) {
					continue;
				}
				ev.pbutton.type = evPtrButtonRelease;
				MOUSE_PRIV(inp)->sent |= emPtrButtonRelease;
			}			

			ev.pbutton.size = sizeof(gii_pbutton_event);
			ev.pmove.origin = inp->origin;
			ev.pbutton.button = nr;
			
			_giiEvQueueAdd(inp, &ev);
		}
	}
}


/*
******************************************************************************
 Null/Do nothing protocol
******************************************************************************
*/

static int parse_null(gii_input *inp, uint8_t *buf, int len)
{
	return len;
}

/*
******************************************************************************
 Microsoft protocol
******************************************************************************
*/

static int parse_ms(gii_input *inp, uint8_t *buf, int len)
{
	int dx, dy;
	unsigned int buttons;
	mouse_priv *mpriv = MOUSE_PRIV(inp);

	/* check header */
	if (((buf[0] & 0x40) != 0x40) || ((buf[1] & 0x40) != 0x00)) {
		DPRINT_EVENTS("Invalid microsoft packet\n");
		return 1;
	}

	dx = (int8_t) (((buf[0] & 0x03) << 6) | (buf[1] & 0x3f));
	dy = (int8_t) (((buf[0] & 0x0c) << 4) | (buf[2] & 0x3f));

	/* Third button added by Christoph Egger
	   (Christoph_Egger@t-online.de) */
	if (buf[0] == 0x40 && !(mpriv->button_state|buf[1]|buf[2])) {
		buttons = 4; /* third button on MS compatible mouse */
	} else {
		buttons = ((buf[0] & 0x10) >> 3)
			| ((buf[0] & 0x20) >> 5);
	}
	
	/* Should allow motion _and_ button change */
	if ((dx == 0) && (dy == 0)
	    && (buttons == (mpriv->button_state & ~(4U)))) {
		/* No move or change: toggle middle */
		buttons = mpriv->button_state ^ 4U;
	} else {
		/* Change: preserve middle */
		buttons |= mpriv->button_state & 4U;
	}

	mouse_send_movement(inp, dx, dy, 0, 0);
	if (buttons != mpriv->button_state) {
		mouse_send_buttons(inp, buttons, mpriv->button_state);
		mpriv->button_state = buttons;
	}

	DPRINT_EVENTS("Got microsoft packet\n");

	return 3;
} 


/*
******************************************************************************
 IntelliMouse protocol
******************************************************************************
*/

static int parse_ms3(gii_input *inp, uint8_t *buf, int len)
{
	int wheel;
	unsigned int buttons;
	mouse_priv *mpriv = MOUSE_PRIV(inp);

	if (mpriv->parse_state == 0) {
		int dx, dy;
		/* check header */
		if (((buf[0] & 0x40) != 0x40) || ((buf[1] & 0x40) != 0x00)) {
			DPRINT_EVENTS("Invalid IntelliMouse packet\n");
			return 1;
		}
		
		dx = (int8_t) (((buf[0] & 0x03) << 6) | (buf[1] & 0x3f));
		dy = (int8_t) (((buf[0] & 0x0c) << 4) | (buf[2] & 0x3f));
		
		buttons = ((buf[0] & 0x10) >> 3)
			| ((buf[0] & 0x20) >> 5)
			| (mpriv->button_state & ~(0x03U));
		
		mouse_send_movement(inp, dx, dy, 0, 0);
		if (buttons != mpriv->button_state) {
			mouse_send_buttons(inp, buttons, mpriv->button_state);
			mpriv->button_state = buttons;
		}
		DPRINT_EVENTS("Got IntelliMouse base packet\n");
	}

	if (len < 4) {
		/* Wait for next byte before deciding anything */
		return 0;
	}
	mpriv->parse_state = 0;

	if ((buf[3] & 0x40) == 0x40) {
		/* 4th byte must be a header byte */
		DPRINT_EVENTS("Got 3-byte IntelliMouse packet\n");
		return 3;
	}

	if ((wheel = (buf[3] & 0x08) ? (buf[3] & 0x0f) - 16
	     : (buf[3] & 0x0f))) {
		mouse_send_movement(inp, 0, 0, 0, wheel);
	}
	buttons = ((int)(buf[3] & 0x30) >> 2)
		| (mpriv->button_state & 0x03);

	if (buttons != mpriv->button_state) {
		mouse_send_buttons(inp, buttons, mpriv->button_state);
		mpriv->button_state = buttons;
	}

	DPRINT_EVENTS("Got 4-byte IntelliMouse packet\n");
	
	return 4;
} 


/*
******************************************************************************
 MouseSystems protocol
******************************************************************************
*/

static int parse_msc(gii_input *inp, uint8_t *buf, int len)
{
	int dx, dy;
	unsigned int buttons;
	mouse_priv *mpriv = MOUSE_PRIV(inp);
	static uint32_t B_mousesys[] = {
		0x0, 0x2, 0x4, 0x6, 0x1, 0x3, 0x5, 0x7
	};

	/* check header */
	if ((buf[0] & 0xf8) != 0x80) {
		DPRINT_EVENTS("Invalid mousesys packet\n");
		return 1;
	}

	buttons = B_mousesys[(~buf[0] & 0x07)];

	dx =  (int8_t) buf[1] + (int8_t) buf[3];
	dy = -(int8_t) buf[2] - (int8_t) buf[4];

	mouse_send_movement(inp, dx, dy, 0, 0);
	if (buttons != mpriv->button_state) {
		mouse_send_buttons(inp, buttons, mpriv->button_state);
		mpriv->button_state = buttons;
	}

	DPRINT_EVENTS("Got mousesys packet\n");

	return 5;
}


/*
******************************************************************************
 Logitech protocol
******************************************************************************
*/

static int parse_logi(gii_input *inp, uint8_t *buf, int len)
{
	int dx, dy;
	unsigned int buttons;
	mouse_priv *mpriv = MOUSE_PRIV(inp);
	static uint32_t B_logitech[] = {
		0x0, 0x2, 0x4, 0x6, 0x1, 0x3, 0x5, 0x7
	};

	/* check header */
	if (((buf[0] & 0xe0) != 0x80) || ((buf[1] & 0x80) != 0x00)) {
		DPRINT_EVENTS("Invalid logitech packet\n");
		return 1;
	}

	buttons = B_logitech[(buf[0] & 0x07)];

	dx = (buf[0] & 0x10) ?	(int8_t)buf[1] : -(int8_t)buf[1];
	dy = (buf[0] & 0x08) ? -(int8_t)buf[2] :  (int8_t)buf[2];

	mouse_send_movement(inp, dx, dy, 0, 0);
	if (buttons != mpriv->button_state) {
		mouse_send_buttons(inp, buttons, mpriv->button_state);
		mpriv->button_state = buttons;
	}

	DPRINT_EVENTS("Got logitech packet\n");

	return 3;
}


/*
******************************************************************************
 Sun protocol
******************************************************************************
*/

static int parse_sun(gii_input *inp, uint8_t *buf, int len)
{
	int dx, dy;
	unsigned int buttons;
	mouse_priv *mpriv = MOUSE_PRIV(inp);
	static uint32_t B_sun[] = {
		0x0, 0x2, 0x4, 0x6, 0x1, 0x3, 0x5, 0x7
	};

	/* check header */
	if ((buf[0] & 0xf8) != 0x80) {
		DPRINT_EVENTS("Invalid sun packet\n");
		return 1;
	}

	buttons = B_sun[(~buf[0] & 0x07)];

	dx =  (int8_t) buf[1];
	dy = -(int8_t) buf[2];

	mouse_send_movement(inp, dx, dy, 0, 0);
	if (buttons != mpriv->button_state) {
		mouse_send_buttons(inp, buttons, mpriv->button_state);
		mpriv->button_state = buttons;
	}

	DPRINT_EVENTS("Got sun packet\n");

	return 3;
}


/*
******************************************************************************
 MouseMan protocol
******************************************************************************
*/

/*
  The damned MouseMan has 3/4 byte packets.  The extra byte
  is only there if the middle button is active.

  This is what we do: when we get the first 3 bytes, we parse
  the info and send off the events, and set a flag to say we
  have seen the first three bytes.

  When we get the fourth byte (maybe with the first three,
  or maybe later on), we check if it is a header byte.
  If so, we return 3, otherwise we parse the buttons in it,
  send off the events, and return 4.

  Note also that unlike the other mice, the mouseman parser
  stores the RAW buttons in priv->button_state.
*/

static int parse_mman(gii_input *inp, uint8_t *buf, int len)
{
	int dx, dy;
	unsigned int buttons;
	mouse_priv *mpriv = MOUSE_PRIV(inp);
	static uint32_t B_mouseman[] = {
		0x0, 0x2, 0x1, 0x3, 0x4, 0x6, 0x5, 0x7
	};

	/* check header */
	if (((buf[0] & 0x40) != 0x40) || ((buf[1] & 0x40) != 0x00)) {
		DPRINT_EVENTS("Invalid mouseman packet\n");
		return 1;
	}

	/* handle the common 3 bytes */
	if (mpriv->parse_state == 0) {
		buttons = (mpriv->button_state & 0x4) |
			((buf[0] & 0x30) >> 4);
		
		dx = (int8_t) (((buf[0] & 0x03) << 6) | (buf[1] & 0x3f));
		dy = (int8_t) (((buf[0] & 0x0c) << 4) | (buf[2] & 0x3f));

		mouse_send_movement(inp, dx, dy, 0, 0);
		mouse_send_buttons(inp, B_mouseman[buttons],
				   B_mouseman[mpriv->button_state]);

		mpriv->button_state = buttons;
		mpriv->parse_state  = 1;

		DPRINT_EVENTS("Got mouseman base packet\n");
	}

	/* now look for extension byte */
	if (len < 4) {
		return 0;
	}

	mpriv->parse_state = 0;

	if ((buf[3] & 0xc0) != 0) {
		/* 4th byte must be a header byte */
		return 3;
	}

	/* handle the extension byte */
	buttons = (mpriv->button_state & 0x3) | ((buf[3] & 0x20) >> 3);

	mouse_send_buttons(inp, B_mouseman[buttons],
			   B_mouseman[mpriv->button_state]);

	mpriv->button_state = buttons;

	DPRINT_EVENTS("Got mouseman extension packet\n");

	return 4;
}


/*
******************************************************************************
 PS/2 protocol
******************************************************************************
*/

static int parse_ps2(gii_input *inp, uint8_t *buf, int len)
{
	int dx, dy;
	unsigned int buttons;
	mouse_priv *mpriv = MOUSE_PRIV(inp);

	/* Check header byte. */
	if ((buf[0] & 0xc0) != 0) {
		DPRINT_EVENTS("Invalid PS/2 packet\n");
		return 1;
	}

	buttons = (buf[0] & 0x07);

	dx = (buf[0] & 0x10) ? buf[1] - 256 : buf[1];
	dy = (buf[0] & 0x20) ? -(buf[2] - 256) : -buf[2];

	mouse_send_movement(inp, dx, dy, 0, 0);
	if (buttons != mpriv->button_state) {
		mouse_send_buttons(inp, buttons, mpriv->button_state);
		mpriv->button_state = buttons;
	}

	DPRINT_EVENTS("Got PS/2 packet\n");

	return 3;
}


/*
******************************************************************************
 MouseMan+ PS/2 protocol
******************************************************************************
*/

static int parse_mmanps2(gii_input *inp, uint8_t *buf, int len)
{
	int dx = 0, dy = 0, wheel = 0;
	unsigned int buttons;
	mouse_priv *mpriv = MOUSE_PRIV(inp);

	/* Check header byte. */
	if ((buf[0] & ~0x07) == 0xc8) {
		/* Extended packet */
		buttons = (buf[0] & 0x07)
			| ((buf[2] & 0x10) ? 0x08 : 0); /* Fourth button */
		wheel = ((buf[2] & 0x0f) < 0x08) ? (buf[2] & 0x0f)
			: (buf[2] & 0x0f) - 16;
	} else {
		if ((buf[0] & 0xc0) != 0) {
			DPRINT_EVENTS("Invalid MouseMan+ PS/2 packet\n");
			return 1;
		}
		buttons = (buf[0] & 0x07)
			| (mpriv->button_state & ~(0x07U)); /* Fourth button */
		dx = (buf[0] & 0x10) ? buf[1] - 256 : buf[1];
		dy = (buf[0] & 0x20) ? -(buf[2] - 256) : -buf[2];
	}

	mouse_send_movement(inp, dx, dy, 0, wheel);
	if (buttons != mpriv->button_state) {
		mouse_send_buttons(inp, buttons, mpriv->button_state);
		mpriv->button_state = buttons;
	}

	DPRINT_EVENTS("Got MouseMan+ PS/2 packet\n");

	return 3;
}


/*
******************************************************************************
 IntelliMouse PS/2 protocol
******************************************************************************
*/

static int parse_imps2(gii_input *inp, uint8_t *buf, int len)
{
	int dx, dy, wheel;
	unsigned int buttons;
	mouse_priv *mpriv = MOUSE_PRIV(inp);

	if ((buf[0] & 0xc0) != 0) {
		DPRINT_EVENTS("Invalid IMPS/2 packet\n");
		return 1;
	}

	buttons = buf[0] & 0x07;
	dx = (buf[0] & 0x10) ? buf[1] - 256 : buf[1];
	dy = (buf[0] & 0x20) ? -(buf[2] - 256) : -buf[2];
	wheel = (int8_t)buf[3];

	mouse_send_movement(inp, dx, dy, 0, wheel);
	if (buttons != mpriv->button_state) {
		mouse_send_buttons(inp, buttons, mpriv->button_state);
		mpriv->button_state = buttons;
	}

	DPRINT_EVENTS("Got IMPS/2 packet\n");

	return 4;
}


/*
******************************************************************************
 Common code
******************************************************************************
*/

#define PS2_SCALE11	230	/* Set 1:1 scale factor */
#define PS2_SCALE21	231	/* Set 2:1 scale factor */
#define PS2_SETRES	232	/* Set resolution */
#define PS2_GETSCALE	233	/* Get scale factor */
#define PS2_SETSTREAM	234	/* Set stream mode */
#define PS2_SETSAMPLE	243	/* Set sample rate */
#define PS2_ENABLE	244	/* Enable PS/2 device */
#define PS2_DISABLE	245	/* Disable PS/2 device */
#define PS2_DEFAULT	246	/* Set default settings */
#define PS2_RESET	255	/* Reset PS/2 device */
 
/* Init data
******************************************************************************
*/
static uint8_t initdata_ps2[] = 
/* Make sure the mouse is enabled and in a sane state */
{ PS2_DEFAULT, PS2_SCALE11, PS2_ENABLE };

static uint8_t initdata_imps2[] =
/* This is a "magic" sequence turning on native mode */
{ PS2_SETSAMPLE, 200, PS2_SETSAMPLE, 100, PS2_SETSAMPLE, 80,
/* Make sure the mouse is enabled and in a sane state */
  PS2_DEFAULT, PS2_SCALE11, PS2_ENABLE };

static uint8_t initdata_mmanps2[] =
/* This is a "magic" sequence turning on native mode */
{ PS2_SCALE11, PS2_SETRES, 0, PS2_SETRES, 3, PS2_SETRES, 2, PS2_SETRES, 1,
  PS2_SCALE11, PS2_SETRES, 3, PS2_SETRES, 1, PS2_SETRES, 2, PS2_SETRES, 3,
/* Make sure the mouse is enabled and in a sane state */
  PS2_DEFAULT, PS2_SCALE11, PS2_ENABLE,
/* The MouseMan+ is really slow by default, so we increase the resolution */
  PS2_SETRES, 3 };

/* Mouse parsers
******************************************************************************
*/

static parser_type pp_null =
{ { "null", NULL }, parse_null, 0,
  NULL, 0, 0, NULL };
static parser_type pp_ms =
{ { "ms", "Microsoft", NULL }, parse_ms, 3,
  NULL, 0, 0, NULL };
static parser_type pp_msc =
{ { "msc", "MouseSystems", NULL }, parse_msc, 5,
  NULL, 0, 0, NULL };
/* Logitech and MMSeries use the same protocol */
static parser_type pp_logi =
{ { "logi", "Logitech", "mm", "MMSeries", NULL }, parse_logi, 3,
  NULL, 0, 0, NULL };
/* Sun and busmice use the same protocol */
static parser_type pp_sun =
{ { "sun", "bm", "BusMouse", NULL }, parse_sun, 3,
  NULL, 0, 0, NULL };
static parser_type pp_mman =
{ { "mman", "MouseMan", NULL }, parse_mman, 3,
  NULL, 0, 0, NULL };
/* Serial IntelliMouse or MouseMan+ */
static parser_type pp_ms3 =
{ { "ms3", "IntelliMouse", "mman+", NULL }, parse_ms3, 3,
  NULL, 0, 0, NULL };

static parser_type pp_ps2 =
{ { "ps2", "PS/2", NULL }, parse_ps2, 3,
  initdata_ps2, sizeof(initdata_ps2), GII_MIT_DONTCARE, NULL };
/* PS/2 MouseMan+ */
static parser_type pp_mmanps2 =
{ { "mmanps2", "MouseManPlusPS/2", NULL }, parse_mmanps2, 3,
  initdata_mmanps2, sizeof(initdata_mmanps2), GII_MIT_FALLBACK, &pp_ps2 };
/* PS/2 IntelliMouse */
static parser_type pp_imps2 =
{ { "imps2", "IMPS/2", NULL }, parse_imps2, 4,
  initdata_imps2 , sizeof(initdata_imps2), GII_MIT_FALLBACK, &pp_ps2 };
/* Linux USB mouse - imps2 protocol without init string. */
static parser_type pp_lnxusb =
{ { "lnxusb", "LinuxUSB", NULL }, parse_imps2, 4,
  NULL, 0, 0, NULL };

parser_type *_gii_mouse_parsers[] = {
	&pp_null,
	&pp_ms,
	&pp_msc,
	&pp_logi,
	&pp_sun,
	&pp_mman,
	&pp_ms3,

	&pp_ps2,
	&pp_mmanps2,
	&pp_imps2,
	&pp_lnxusb,

	NULL	/* Terminator */
};

static int do_parse_packet(gii_input *inp)
{
	mouse_priv *mpriv = MOUSE_PRIV(inp);

	int used;

#if 0
	{	int i;

		fprintf(stderr, "Mouse: do_parse_packet [");

		for (i=0; i < (MOUSE_PRIV(inp)->packet_len - 1); i++) {
			fprintf(stderr, "%02x ", 
				MOUSE_PRIV(inp)->packet_buf[i]);
		}

		fprintf(stderr, "%02x]\n", MOUSE_PRIV(inp)->packet_buf[i]);
	}
#endif

	/* call parser function */
	
	used = mpriv->parser(inp, mpriv->packet_buf, 
			     mpriv->packet_len);

	DPRINT_EVENTS("packet used %d bytes\n", used);

	return used;
}


gii_event_mask GII_mouse_poll(gii_input *inp, void *arg)
{
	mouse_priv *mpriv = MOUSE_PRIV(inp);
	int read_len;

	DPRINT_EVENTS("GII_mouse_poll(%p, %p) called\n", inp, arg);
	
	if (mpriv->eof) {
		/* End-of-file, don't do any polling */
		return 0;
	}

	if (arg == NULL) {
		fd_set fds = inp->fdset;
		struct timeval tv = { 0, 0 };
		if (select(inp->maxfd, &fds, NULL, NULL, &tv) <= 0) {
			return 0;
		}
	} else {
		if (! FD_ISSET(mpriv->fd, ((fd_set*)arg))) {
			/* Nothing to read on our fd */
			DPRINT_EVENTS("GII_mouse_poll: dummypoll\n");
			return 0;
		}
	}
		
	mpriv->sent = 0;

	/* read the mouse data */
	read_len = MAX_PACKET_BUF - mpriv->packet_len - 1;

	/* ASSERT(read_len >= 1) */
	read_len = read(mpriv->fd, mpriv->packet_buf + mpriv->packet_len, 
			(unsigned)read_len);

	if (read_len <= 0) {
		if (read_len == 0) {
			/* End-of-file occured */
			mpriv->eof = 1;
			DPRINT_EVENTS("Mouse: EOF occured on fd: %d\n",
					 mpriv->fd);
			return 0;
		}
		/* Error, we try again next time */
		if (errno == EAGAIN) return 0; /* Don't print warning here */
		perror("Mouse: Error reading mouse");
		return 0;
	}

	mpriv->packet_len += read_len;
	
	/* parse any packets */
	while (mpriv->packet_len >= mpriv->min_packet_len) {
		int used;

		used = do_parse_packet(inp);

		if (used <= 0) {
			break;	 /* not enough data yet */
		}

		mpriv->packet_len -= used;

		if (mpriv->packet_len > 0) {
			memmove(mpriv->packet_buf, mpriv->packet_buf + used,
				(unsigned)mpriv->packet_len);
		} else {
			mpriv->packet_len = 0;
		}
	}

	DPRINT_EVENTS("GII_mouse_poll: done\n");

	return mpriv->sent;
}
