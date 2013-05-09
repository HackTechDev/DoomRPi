/* $Id: input.c,v 1.15 2005/08/09 10:14:48 aldot Exp $
******************************************************************************

   Linux_kbd: input via linux MEDIUMRAW mode.

   Copyright (C) 1998-1999  Andrew Apted	[andrew@ggi-project.org]
   Copyright (C) 1999       Marcus Sundberg	[marcus@ggi-project.org]

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
#include <ggi/internal/gii.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <termios.h>

#include <signal.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

#ifdef HAVE_SYS_KD_H
#include <sys/kd.h>
#else
#include <linux/kd.h>
#endif
#ifdef HAVE_SYS_VT_H
#include <sys/vt.h>
#else
#include <linux/vt.h>
#endif
#include <linux/tty.h>
#include <linux/keyboard.h>


#include "linkey.h"

typedef struct keyboard_hook
{
	int fd;
	int eof;

	int old_mode;
	struct termios old_termios;
	char old_kbled;

	uint8_t  keydown_buf[128];
	uint32_t keydown_sym[128];
	uint32_t keydown_label[128];

	uint32_t  modifiers;
	uint32_t  normalmod;
	uint32_t  lockedmod;
	uint32_t  lockedmod2;

	unsigned char	accent;
	struct kbdiacrs	accent_table;

	int	call_vtswitch;
	int	needctrl2switch;
	int	ctrlstate;
} linkbd_priv;

#define LINKBD_PRIV(inp)	((linkbd_priv *) inp->priv)

#define LED2MASK(x)	(((x) & LED_CAP ? GII_MOD_CAPS   : 0) | \
			 ((x) & LED_NUM ? GII_MOD_NUM    : 0) | \
			 ((x) & LED_SCR ? GII_MOD_SCROLL : 0))

#define MASK2LED(x)	(((x) & GII_MOD_CAPS   ? LED_CAP : 0) | \
			 ((x) & GII_MOD_NUM    ? LED_NUM : 0) | \
			 ((x) & GII_MOD_SCROLL ? LED_SCR : 0))

/*
******************************************************************************
 Keyboard handling
******************************************************************************
*/

static int got_stopped;
static void sighandler(int unused)
{
	got_stopped = 1;
}

static inline int
GII_keyboard_init(gii_input *inp, const char *filename)
{
	int fd;
	struct termios tio;
	linkbd_priv *priv;
	void (*oldinhandler)(int);
	void (*oldouthandler)(int);

	DPRINT_MISC("Linux-kbd: opening tty\n");

	/* open the tty */
	fd = open(filename, O_RDWR);

	if (fd < 0) {
		perror("Linux-kbd: Couldn't open TTY");
		return GGI_ENODEVICE;
	}

	/* allocate keyboard hook */
	if ((priv = malloc(sizeof(linkbd_priv))) == NULL) {
		close(fd);
		return GGI_ENOMEM;
	}

	DPRINT_MISC("Linux-kbd: calling tcgetattr()\n");

	/* put tty into "straight through" mode.
	 */
	if (tcgetattr(fd, &priv->old_termios) < 0) {
		perror("Linux-kbd: tcgetattr failed");
	}

	tio = priv->old_termios;

	tio.c_lflag &= ~(ICANON | ECHO  | ISIG);
	tio.c_iflag &= ~(ISTRIP | IGNCR | ICRNL | INLCR | IXOFF | IXON);
	tio.c_iflag |= IGNBRK;
	tio.c_cc[VMIN]  = 0;
	tio.c_cc[VTIME] = 0;

	DPRINT_MISC("Linux-kbd: calling tcsetattr()\n");

	got_stopped = 0;
	oldinhandler  = signal(SIGTTIN, sighandler);
	oldouthandler = signal(SIGTTOU, sighandler);
	if (tcsetattr(fd, TCSANOW, &tio) < 0) {
		perror("Linux-kbd: tcsetattr failed");
	}
	signal(SIGTTIN, oldinhandler);
	signal(SIGTTOU, oldouthandler);

	if (got_stopped) {
		/* We're a background process on this tty... */
		fprintf(stderr,
			"Linux-kbd: can't be run in the background!\n");
		free(priv);
		close(fd);
		return GGI_EUNKNOWN;
	}

	/* Put the keyboard into MEDIUMRAW mode.  Despite the name, this
	 * is really "mostly raw", with the kernel just folding long
	 * scancode sequences (e.g. E0 XX) onto single keycodes.
	 */
	DPRINT_MISC("Linux-kbd: going to MEDIUMRAW mode\n");
	if (ioctl(fd, KDGKBMODE, &priv->old_mode) < 0) {
		perror("Linux-kbd: couldn't get mode");
		priv->old_mode = K_XLATE;
	}
	if (ioctl(fd, KDSKBMODE, K_MEDIUMRAW) < 0) {
		perror("Linux-kbd: couldn't set raw mode");
		tcsetattr(fd, TCSANOW, &(priv->old_termios));
		close(fd);
		free(priv);
		return GGI_ENODEVICE;
	}

	priv->fd = fd;
	priv->eof = 0;
	priv->call_vtswitch = 1;
	memset(priv->keydown_buf, 0, sizeof(priv->keydown_buf));

	if (ioctl(fd, KDGKBLED, &priv->old_kbled) != 0) {
		perror("Linux-kbd: unable to get keylock status");
		priv->old_kbled = 0x7f;
		priv->lockedmod = 0;
	} else {
		priv->lockedmod = LED2MASK(priv->old_kbled);
	}
	/* Make sure LEDs match the flags */
	ioctl(priv->fd, KDSETLED, 0x80);

	priv->normalmod = 0;
	priv->modifiers = priv->lockedmod | priv->normalmod;
	priv->lockedmod2 = priv->lockedmod;

	if (ioctl(fd, KDGKBDIACR, &priv->accent_table) != 0) {
		priv->accent_table.kb_cnt = 0;
	} else {
		unsigned int i;
		for (i = 0; i < priv->accent_table.kb_cnt; i++) {
			switch (priv->accent_table.kbdiacr[i].diacr) {
			case '"':
				priv->accent_table.kbdiacr[i].diacr =
					GIIUC_Diaeresis;
				break;
			case '\'':
				priv->accent_table.kbdiacr[i].diacr =
					GIIUC_Acute;
				break;
			}
		}
	}
	priv->accent = 0;

	if (getenv("GII_CTRLALT_VTSWITCH")) {
		priv->needctrl2switch = 1;
		priv->ctrlstate = 0;
	} else {
		priv->needctrl2switch = 0;
		priv->ctrlstate = 1;
	}
	inp->priv = priv;

	DPRINT_MISC("Linux-kbd: init OK.\n");

	return 0;
}


static inline gii_event_mask
GII_keyboard_flush_keys(gii_input *inp)
{
	linkbd_priv *priv = LINKBD_PRIV(inp);
	gii_event_mask rc = 0;
	gii_event ev;
	int code;

	for (code=0; code < 128; code++) {

		if (! priv->keydown_buf[code]) continue;
		priv->keydown_buf[code] = 0;

		if (! (inp->curreventmask & emKeyRelease)) continue;

		/* Send a key-release event */
		_giiEventBlank(&ev, sizeof(gii_key_event));

		ev.any.type   = evKeyRelease;
		ev.any.size   = sizeof(gii_key_event);
		ev.any.origin = inp->origin;

		ev.key.button = code;
		ev.key.sym    = priv->keydown_sym[code];
		ev.key.label  = priv->keydown_label[code];
		ev.key.modifiers = priv->modifiers;

		DPRINT_EVENTS("Linux-kbd: flushing key: button=0x%02x "
			"modifiers=0x%02x sym=0x%04x label=0x%04x.\n",
			ev.key.button, ev.key.modifiers, ev.key.sym,
			ev.key.label);

		_giiEvQueueAdd(inp, &ev);

		rc |= emKeyRelease;
	}

	priv->normalmod = 0;
	priv->modifiers = priv->lockedmod;

	if (priv->needctrl2switch) {
		priv->ctrlstate = 0;
	}

	return rc;
}


static inline void
handle_accent(linkbd_priv *priv, int symval, gii_event *ev)
{
	unsigned char diacr = priv->accent;
	unsigned int i;

	for (i = 0; i < priv->accent_table.kb_cnt; i++) {
		if (priv->accent_table.kbdiacr[i].diacr == diacr &&
		    priv->accent_table.kbdiacr[i].base
		    == ev->key.sym) {
			ev->key.sym
				= priv->accent_table.kbdiacr[i].result;
				break;
		}
	}
	if (ev->key.sym == GIIUC_Space) ev->key.sym = priv->accent;

	priv->accent = 0;
}

static inline void
handle_modifier(linkbd_priv *priv, gii_event *ev)
{
	uint32_t mask, old_label;

	/* Handle AltGr properly */
	if (ev->key.label == GIIK_AltR) {
		if (ev->key.sym == GIIK_VOID) {
			ev->key.sym = GIIK_AltGr;
		}
		mask = 1 << (ev->key.sym & GII_KM_MASK);
	} else {
		mask = 1 << (ev->key.label & GII_KM_MASK);
	}
	/* Handle CapsShift properly: shift keys undo CapsLock */
	if ((ev->key.label == GIIK_ShiftL || ev->key.label == GIIK_ShiftR) &&
	    ev->key.sym == GIIK_CapsLock) {
		if (priv->lockedmod & GII_MOD_CAPS) {
			old_label = ev->key.label;
			ev->key.label = GIIK_CapsLock;
			handle_modifier(priv, ev);
			ev->key.label = old_label;
		}
		ev->key.sym = GIIK_Shift;
	}

	if (GII_KVAL(ev->key.label) & GII_KM_LOCK) {
		if (ev->key.type == evKeyPress) {
			if (!(priv->lockedmod & mask)) {
				priv->lockedmod |= mask;
				ioctl(priv->fd, KDSKBLED,
				      MASK2LED(priv->lockedmod));
			} else {
				ev->key.sym = GIIK_VOID;
			}
		} else {
			if ((priv->lockedmod & mask)) {
				if (!(priv->lockedmod2 & mask)) {
					priv->lockedmod2 |= mask;
					ev->key.sym = GIIK_VOID;
				} else {
					priv->lockedmod2 &= ~mask;
					priv->lockedmod &= ~mask;
					ioctl(priv->fd, KDSKBLED,
					      MASK2LED(priv->lockedmod));
				}
			}
		}
	} else {
		if (ev->key.type == evKeyRelease) {
			priv->normalmod &= ~mask;
		} else {
			priv->normalmod |= mask;
		}
	}
	priv->modifiers = priv->lockedmod | priv->normalmod;
}


static inline gii_event_mask
GII_keyboard_handle_data(gii_input *inp, int code)
{
	linkbd_priv *priv = LINKBD_PRIV(inp);
	gii_event ev;
	struct kbentry entry;
	int symval, labelval;
	gii_event_mask mask;

	_giiEventBlank(&ev, sizeof(gii_key_event));

	if (code & 0x80) {
		code &= 0x7f;
		ev.key.type = evKeyRelease;
		priv->keydown_buf[code] = 0;
	} else if (priv->keydown_buf[code] == 0) {
		ev.key.type = evKeyPress;
		priv->keydown_buf[code] = 1;

	} else {
		ev.key.type = evKeyRepeat;
	}
	ev.key.button = code;
	/* First update modifiers here so linkey.c can use the value */
	ev.key.modifiers = priv->modifiers;

	if (ev.key.type == evKeyRelease &&
	    GII_KTYP(priv->keydown_sym[code]) != GII_KT_MOD &&
	    priv->keydown_sym[code] != GIIK_VOID) {
		/* We can use the cached values */
		ev.key.sym   = priv->keydown_sym[code];
		ev.key.label = priv->keydown_label[code];
	} else {
		/* Temporarily switch back to the old mode because
		   unicodes aren't available through table lookup in MEDIUMRAW
		*/
		if (ioctl(priv->fd, KDSKBMODE, priv->old_mode) < 0) {
			DPRINT_MISC("Linux-kbd: ioctl(KDSKBMODE) failed.\n");
			return 0;
		}
		/* Look up the keysym without modifiers, which will give us
		 * the key label (more or less).
		 */
		entry.kb_table = 0;
		entry.kb_index = code;
		if (ioctl(priv->fd, KDGKBENT, &entry) < 0) {
			DPRINT_MISC("Linux-kbd: ioctl(KDGKBENT) failed.\n");
			return 0;
		}
		labelval = entry.kb_value;
		if (priv->old_mode == K_UNICODE) labelval &= 0x0fff;

		/* Now look up the full keysym in the kernel's table */

		/* calculate kernel-like shift value */
		entry.kb_table =
		  ((priv->modifiers & GII_MOD_SHIFT) ? (1<<KG_SHIFT)     : 0) |
		  ((priv->modifiers & GII_MOD_CTRL)  ? (1<<KG_CTRL)      : 0) |
		  ((priv->modifiers & GII_MOD_ALT)   ? (1<<KG_ALT)       : 0) |
		  ((priv->modifiers & GII_MOD_ALTGR) ? (1<<KG_ALTGR)     : 0) |
		  ((priv->modifiers & GII_MOD_META)  ? (1<<KG_ALT)       : 0) |
		  ((priv->modifiers & GII_MOD_CAPS)  ? (1<<KG_CAPSSHIFT) : 0);

		entry.kb_index = code;
		if (ioctl(priv->fd, KDGKBENT, &entry) < 0) {
			DPRINT_MISC("Linux-kbd: ioctl(KDGKBENT) failed.\n");
			return 0;
		}

		/* Switch back to MEDIUMRAW */
		if (ioctl(priv->fd, KDSKBMODE, K_MEDIUMRAW) < 0) {
			DPRINT_MISC("Linux-kbd: ioctl(KDSKBMODE) failed.\n");
			return 0;
		}

		switch (entry.kb_value) {

		case K_HOLE:
			DPRINT_EVENTS("Linux-kbd: NOSUCHKEY\n");
			break;

		case K_NOSUCHMAP:
			DPRINT_EVENTS("Linux-kbd: NOSUCHMAP\n");
			entry.kb_value = K_HOLE;
			break;
		}
		symval = entry.kb_value;
		if (priv->old_mode == K_UNICODE) symval &= 0x0fff;

		_gii_linkey_trans(code, labelval, symval, &ev.key);

		if (ev.key.type == evKeyPress) {
			if (priv->accent) {
				if (GII_KTYP(ev.key.sym) != GII_KT_MOD) {
					handle_accent(priv, symval, &ev);
				}
			} else if (KTYP(symval) == KT_DEAD) {
				priv->accent = GII_KVAL(ev.key.sym);
			}
		}
		if (GII_KTYP(ev.key.sym) == GII_KT_DEAD) {
			ev.key.sym = GIIK_VOID;
		}
	}

	/* Keep track of modifier state */
	if (GII_KTYP(ev.key.label) == GII_KT_MOD) {
		/* Modifers don't repeat */
		if (ev.key.type == evKeyRepeat) return 0;

		handle_modifier(priv, &ev);
	}
	/* Now update modifiers again to get the value _after_ the current
	   event */
	ev.key.modifiers = priv->modifiers;

	if (ev.any.type == evKeyPress) {
		priv->keydown_sym[code]    = ev.key.sym;
		priv->keydown_label[code]  = ev.key.label;
	}

	DPRINT_EVENTS("KEY-%s button=0x%02x modifiers=0x%02x "
		"sym=0x%04x label=0x%04x\n",
		(ev.key.type == evKeyRelease) ? "UP" :
		((ev.key.type == evKeyPress) ? "DN" : "RP"),
		ev.key.button, ev.key.modifiers,
		ev.key.sym,  ev.key.label);

	if (priv->call_vtswitch) {
		if (ev.key.label == GIIK_CtrlL && priv->needctrl2switch) {
			if (ev.key.type == evKeyRelease) {
				priv->ctrlstate = 0;
			} else if (ev.key.type == evKeyPress) {
				priv->ctrlstate = 1;
			}
		}
		/* Check for console switch.  Unfortunately, the kernel doesn't
		 * recognize KT_CONS when the keyboard is in RAW or MEDIUMRAW
		 * mode, so _we_ have to.  Sigh.
		 */
		if ((ev.key.type == evKeyPress) &&
		    (KTYP(entry.kb_value) == KT_CONS) && priv->ctrlstate) {
			int rc;
			int new_cons = 1+KVAL(entry.kb_value);

			/* Clear the keydown buffer, since we'll never know
			   what keys the user pressed (or released) while they
			   were away.
			 */
			DPRINT_MISC("Flushing all keys.\n");
			rc = GII_keyboard_flush_keys(inp);

			DPRINT_MISC("Switching to console %d.\n", new_cons);

			if (ioctl(priv->fd, VT_ACTIVATE, new_cons) < 0) {
				perror("ioctl(VT_ACTIVATE)");
			}

			return rc;
		}
	}

	mask = (1 << ev.any.type);

	if (! (inp->curreventmask & mask)) return 0;

	/* finally queue the key event */
	ev.any.size   = sizeof(gii_key_event);
	ev.any.origin = inp->origin;

	_giiEvQueueAdd(inp, &ev);

	return (1 << ev.any.type);
}


static gii_event_mask GII_keyboard_poll(gii_input *inp, void *arg)
{
	linkbd_priv *priv = LINKBD_PRIV(inp);
	unsigned char buf[256];
	gii_event_mask result = 0;
	int readlen, i;

	DPRINT_EVENTS("GII_keyboard_poll(%p, %p) called\n", inp, arg);

	if (priv->eof) {
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
		if (! FD_ISSET(priv->fd, ((fd_set*)arg))) {
			/* Nothing to read on our fd */
			DPRINT_EVENTS("GII_keyboard_poll: dummypoll\n");
			return 0;
		}
	}

	/* Read keyboard data */
	while ((readlen = read(priv->fd, buf, 256)) > 0) {
		for (i = 0; i < readlen; i++) {
			result |= GII_keyboard_handle_data(inp, buf[i]);
		}
		if (readlen != 256) break;
		else {
			fd_set fds = inp->fdset;
			struct timeval tv = { 0, 0 };
			if (select(inp->maxfd, &fds, NULL, NULL, &tv) <= 0) {
				return 0;
			}
		}
	}

	if (readlen == 0) {
		/* End-of-file occured */
		if (errno != EINTR) {
			priv->eof = 1;
		}
		DPRINT_EVENTS("Linux-kbd: EOF occured on fd: %d\n",
				 priv->fd);
	} else if (readlen < 0) {
		/* Error, we try again next time */
		perror("Linux-kbd: Error reading keyboard");
	}

	return result;
}


/* ---------------------------------------------------------------------- */

static gii_cmddata_getdevinfo devinfo =
{
	"Linux Keyboard",		/* long device name */
	"lkbd",				/* shorthand */
	emKey,				/* can_generate */
	128,				/* num_buttons */
	0				/* num_axes */
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

/* ---------------------------------------------------------------------- */


static int GII_lin_kbd_close(gii_input *inp)
{
	linkbd_priv *priv = LINKBD_PRIV(inp);

	DPRINT_MISC("Linux-kbd cleanup\n");

	if (priv == NULL) {
		/* Make sure we're only called once */
		return 0;
	}

	if (priv->old_kbled != 0x7f) {
		ioctl(priv->fd, KDSKBLED, priv->old_kbled);
	}

	if (ioctl(priv->fd, KDSKBMODE, priv->old_mode) < 0) {
		perror("Linux-kbd: couldn't restore mode");
	}

	if (tcsetattr(priv->fd, TCSANOW, &priv->old_termios) < 0) {
		perror("Linux-kbd: tcsetattr failed");
	}

	close(priv->fd);

	free(priv);
	inp->priv = NULL;

	ggUnregisterCleanup((ggcleanup_func *)GII_lin_kbd_close, inp);

	fputc('\n', stderr);
	fflush(stderr);

	DPRINT("Linux-kbd: exit OK.\n");

	return 0;
}


EXPORTFUNC int GIIdl_linux_kbd(gii_input *inp, const char *args, void *argptr);

int GIIdl_linux_kbd(gii_input *inp, const char *args, void *argptr)
{
	const char *filename = "/dev/tty";

	DPRINT_MISC("linux_kbd starting.(args=\"%s\",argptr=%p)\n",
		    args, argptr);

	/* Initialize */
	if (args && *args) {
		filename = args;
	}

	if(_giiRegisterDevice(inp,&devinfo,NULL)==0) {
		return GGI_ENOMEM;
	}

	if (GII_keyboard_init(inp, filename) < 0) {
		return GGI_ENODEVICE;
	}

	/* Make sure the keyboard is reset when the app terminates */
	ggRegisterCleanup((ggcleanup_func *)GII_lin_kbd_close, inp);

	inp->GIIsendevent = GIIsendevent;
	inp->GIIeventpoll = GII_keyboard_poll;
	inp->GIIclose = GII_lin_kbd_close;

	inp->targetcan = emKey;
	inp->GIIseteventmask(inp, inp->targetcan);

	inp->maxfd = LINKBD_PRIV(inp)->fd + 1;
	FD_SET(LINKBD_PRIV(inp)->fd, &inp->fdset);

	/* Send initial cmdDevInfo event */
	send_devinfo(inp);

	DPRINT_MISC("linux_kbd fully up\n");

	return 0;
}
