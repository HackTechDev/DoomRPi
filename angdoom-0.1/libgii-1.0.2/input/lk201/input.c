/* $Id: input.c,v 1.13 2005/08/04 12:43:29 cegger Exp $
******************************************************************************

   lk201: input

   Copyright (C) 1998 Andrew Apted	[andrew@ggi-project.org]
   Copyright (C) 1999 Marcus Sundberg	[marcus@ggi-project.org]
   Copyright (C) 1999 John Weismiller	[johnweis@home.com]

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
#include <ggi/internal/gii-dl.h>
#include <ggi/internal/gii_debug.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include <unistd.h>
#include <termios.h>

#include <sys/types.h>
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#include <sys/ioctl.h>
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_LINUX_KDEV_T_H
#ifdef HAVE_LINUX_MAJOR_H
#define HAVE_LINUX_DEVICE_CHECK
#include <linux/kdev_t.h>   /* only needed for MAJOR() macro */
#include <linux/major.h>    /* only needed for MISC_MAJOR */
#endif
#endif

#define LK_POWER_UP	(0x01)
#define LK_CMD_POWER_UP (0xfd)
#define LK_CMD_REQ_ID	(0xab)
#define LK_CMD_SET_DEFAULTS (0xd3)
#define LK_CMD_TEST_MODE	(0xcb)
#define LK_TESTMODE_ACK	(0xb8)
#define LK_CMD_INHIBIT	(0xb9)
#define LK_KDBLOCK_ACK	(0xb7)
#define LK_CMD_RESUME	(0x8b)
#define LK_OUTPUT_ERROR (0xb5)
#define LK_ALLUP	(0xb3)
#define LK_KDOWN_ERROR	(0x3d)
#define LK_POWER_ERROR	(0x3e)
#define LK_MODECHG_ACK	(0xba)
#define LK_REPEAT	(0xb4)
#define LK_LED_ENABLE	(0x13)
#define LK_LED_DISABLE	(0x11)

#define LED_WAIT	(0x1)
#define LED_COMP	(0x2)
#define LED_LOCK	(0x4)
#define LED_HOLD	(0x8)
#define LED_ALL	(0xf)

#define LK_LED(x)	(0x80 | x)

#define LK_CL_ENABLE	(0x1b)
#define LK_CCL_ENABLE	(0xbb)
#define LK_CL_DISABLE	(0x99)
#define LK_CCL_DISABLE	(0xb9)
#define LK_SOUND_CLICK	(0x9f)
#define LK_PARAM_VOLUME(v)	(0x80|((v)&0x7))
#define LK_BELL_ENABLE	(0x23)
#define LK_BELL_DISABLE (0xa1)
#define LK_RING_BELL	(0xa7)
#define LK_UPDOWN	(0x86)
#define LK_AUTODOWN	(0x82)
#define LK_DOWN	(0x80)
#define LK_CMD_MODE(m,div)	((m)|((div)<<3))
#define LK_MODECHG_ACK	(0xba)
#define LK_PFX_KEYDOWN	(0xb9)
#define LK_CMD_RPT_TO_DOWN	(0xd9)
#define LK_CMD_ENB_RPT	(0xe3)
#define LK_CMD_DIS_RPT	(0xe1)
#define LK_CMD_TMP_NORPT	(0xd1)
#define LK_INPUT_ERROR	(0xb6)
#define LK_ENABLE_401	(0xe9)

#define C_NORM	(CREAD | CS8 | CLOCAL | B4800)
#define MAX_KEYS (256)

/* sorry for you non-gcc folks... will rewrite this sometime... */
static uint32_t keylabel[]={
#include "keytable.dat"
[0]=GIIK_VOID };

static uint32_t keysymShift[]={
#include "keysymShift.dat"
[0]=GIIK_VOID };

typedef enum
{
	RDST_IDLE,
	RDST_KEYBOARDID,
	RDST_ERRORCODE,
	RDST_KEYCODE
	
} RDSTATE;

typedef struct {
	int fd;
	struct termios old_termios;
	int readonly;
	int have_old_termios;
	int eof;
	char keydown[MAX_KEYS+1];
	RDSTATE rd_state;
	int nextdiv;
	uint32_t modifiers;
	uint32_t repeatkey; /* last key pressed, used for repeat */
	unsigned int leds;
} l_lk201_priv;

#define PRIVP(inp)	((l_lk201_priv*)(inp->priv))

static const char *tcgetattrfailstr =
	"Warning: failed to get serial parameters for lk201 device.\n"
	"         (Is it really a character device?)\n"
	"         Your keyboard may not work as expected.\n";

static const char *tcsetattrfailstr =
	"Warning: failed to set serial parameters for lk201 device.\n"
	"         (Proper access permisions?)\n"
	"         Your keyboard may not work as expected.\n";


static gii_cmddata_getdevinfo devinfo =
{
	"DEC LK201 Keyboard",		/* long device name */
	"lkkb",				/* shorthand */
	emKey,				/* can_generate */
	MAX_KEYS,			/* num_buttons */
	0				/* num_axes */
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

static int 
do_lk201_open(gii_input *inp, const char *filename)
{
	l_lk201_priv *priv = (l_lk201_priv*)inp->priv;
	uint8_t tmp[256];
	
	priv->readonly = 0;
	priv->fd = open(filename, O_RDWR | O_NOCTTY | O_NONBLOCK);
	
	if (priv->fd < 0) 
	{
		priv->readonly = 1;
		priv->fd = open(filename, O_RDONLY | O_NOCTTY | O_NONBLOCK);
	}
	
	if (priv->fd < 0) 
	{
		DPRINT_MISC("lk201: Failed to open '%s'.\n",
			    filename);
		return GGI_ENODEVICE;
	}
	
	if (priv->fd>=inp->maxfd)
	{
		inp->maxfd=priv->fd+1;
	}
	FD_SET(priv->fd, &inp->fdset);
	
	DPRINT_MISC("lk201: Opened serial port '%s' %s (fd=%d).\n",
		    filename, priv->readonly ? "ReadOnly" : "Read/Write", priv->fd);
	
	/* Set up the termios state and baud rate */
	tcflush(priv->fd, TCIOFLUSH);
	if (tcgetattr(priv->fd, &priv->old_termios) == 0) {
		struct termios tio = priv->old_termios;
		
		tio.c_cflag = C_NORM;
		tio.c_iflag = IGNBRK;
		tio.c_oflag = 0;
		tio.c_lflag = 0;

		/* What the heck are these? man page only says control chars.
		   Because I don't know what they are, I'm commenting them out.
		   So there. */
#if 0
		tio.c_cc[VMIN]  = 1;
		tio.c_cc[VTIME] = 0;
#endif

		if (tcsetattr(priv->fd, TCSANOW, &tio) == 0) 
		{
			priv->have_old_termios = 1;
		}
		else 
		{
			fprintf(stderr, tcsetattrfailstr);
		}
	} 
	else 
	{
		fprintf(stderr, tcgetattrfailstr);
	}
	
	/* dump and characters currently in the buffer */
	while (read(priv->fd, tmp, sizeof(tmp))>0);
	
	/* send the power-up reset command */
	tmp[0]=LK_CMD_POWER_UP;
	write(priv->fd, tmp, 1);
	
	return 0;
}

static int GII_lk201_close(gii_input *inp)
{
	l_lk201_priv *priv = (l_lk201_priv*)inp->priv;
	
	DPRINT_MISC("lk201 cleanup\n");
	
	if (priv->have_old_termios) {
		if (tcsetattr(priv->fd, TCSANOW, &priv->old_termios) < 0) {
			perror("Error restoring serial parameters");
		}
	}
	
	close(priv->fd);
	free(priv);
	
	DPRINT_MISC("lk201: exit OK.\n");
	
	return 0;
}

static void lk201_sendbyte(gii_input *inp, unsigned char byte)
{
	l_lk201_priv *priv=PRIVP(inp);
	int fdflags=fcntl(priv->fd, F_GETFL);
	
	DPRINT_MISC("lk201_sendbyte: Sending 0x%02x on fd=%d\n", byte, PRIVP(inp)->fd);
	
	fcntl(priv->fd, F_SETFL, fdflags & ~O_NONBLOCK);
	write(priv->fd, (char*)&byte, sizeof(byte));
	fcntl(priv->fd, F_SETFL, fdflags);
}

static void lk201_modechange(gii_input *inp, int mode)
{
	DPRINT_MISC("Mode change requested, mode=%d, div=%d, fd=%d\n", mode, PRIVP(inp)->nextdiv, PRIVP(inp)->fd);
	
	lk201_sendbyte(inp, LK_CMD_MODE(mode, PRIVP(inp)->nextdiv--));
}

/* this function combines the ev.key.modifiers field with the label to create the symbol */
static inline void lookup_symbol(gii_input *inp, gii_event *ev, uint32_t button)
{
	if (ev->key.modifiers & GII_MOD_SHIFT)
	{
		ev->key.sym=keysymShift[button];
		
	}
	else if (ev->key.modifiers & GII_MOD_CAPS)
	{
		ev->key.sym=ev->key.label;
	}
	else if ((ev->key.modifiers & GII_MOD_CTRL) && GII_KVAL(ev->key.label)>='A' && GII_KVAL(ev->key.label)<='Z')
	{
		ev->key.sym=ev->key.label-'A'+1;
		
	}
	else if ((ev->key.modifiers && GII_MOD_CTRL) && ev->key.label==GIIUC_Grave)
	{
		/* Escape?  I think this is how it is accessed */
		
		ev->key.sym=GIIUC_Escape;
	}
	else if ((ev->key.modifiers && GII_MOD_CTRL) && ev->key.label==GIIUC_3)
	{
		/* Escape?  I think this is how it is accessed */
		
		ev->key.sym=GIIUC_Pound;
	}
	else if ((ev->key.modifiers && GII_MOD_CTRL) && ev->key.label==GIIUC_BackSpace)
	{
		ev->key.sym=GIIUC_Delete;
	}
	/* FIXME: add checks for Ctrl-\, etc here */
	else if (GII_KVAL(ev->key.label)>='A' && GII_KVAL(ev->key.label)<='Z')
	{
		/* if this is the unshifted character, then use label+32 */
		ev->key.sym=ev->key.label+32;
	}
	else if (!ev->key.modifiers)
	{
		ev->key.sym=ev->key.label;
		
	}
	else
	{
		ev->key.sym=GIIK_VOID;
	}
}

static void lk201_updateleds(gii_input *inp)
{
	lk201_sendbyte(inp, PRIVP(inp)->leds & LED_LOCK ? LK_LED_ENABLE : LK_LED_DISABLE);
	lk201_sendbyte(inp, LK_LED(LED_LOCK));
}

static inline void handle_modifier(gii_input *inp, gii_event *ev)
{
	l_lk201_priv *priv=PRIVP(inp);
	uint32_t modifier=ev->key.label & GII_KM_MASK;
	
	/* update the sym field */
	ev->key.sym=GII_KEY(GII_KT_MOD, modifier);
	
	/* if this is not a locking modifier, then clear the bit.  This way, if we are suppost to
	set the bit, then the toggle will always set it to true */
	if (!(ev->key.label & GII_KM_LOCK))
	{
		priv->modifiers&=~(1 << modifier);
		
	}
	
	/* if this is a keypress event, then toggle the bit */
	if (ev->any.type==evKeyPress)
	{
		priv->modifiers^=1 << modifier;
		
	}
	
	/* check if we need to update the keyboard leds */
	if (modifier==GII_KM_CAPS)
	{
		priv->leds&=~LED_LOCK;
		
		if (priv->modifiers & GII_MOD_CAPS)
		{
			priv->leds|=LED_LOCK;
		}
		
		lk201_updateleds(inp);
	}
}

static inline gii_event_mask
GII_create_key_event(gii_input *inp, uint8_t evtype, uint32_t button)
{
	gii_event ev;
	gii_event_mask retval=0;
	l_lk201_priv *priv=PRIVP(inp);
	
	/* setup fields in event structure */
	_giiEventBlank(&ev, sizeof(gii_key_event));
	ev.any.type   = evtype;
	ev.any.size   = sizeof(gii_key_event);
	ev.any.origin = inp->origin;
	ev.key.button=button;
	ev.key.modifiers=priv->modifiers;
	ev.key.label=keylabel[button];
	
	/* check if this key is a modifier; we need to handle it specially */
	if (GII_KTYP(ev.key.label)==GII_KT_MOD)
	{
		/* this is a modifier then update the modifier field */
		handle_modifier(inp, &ev);
	}
	else /* this is a normal key, so update the sym field */
	{
		lookup_symbol(inp, &ev, button);
		
		if (ev.any.type==evKeyPress)
		{
			priv->repeatkey=ev.key.button;
			
			/* FIXME: somehow process the time at this point to
			   allow initial delay before key repeat. ie,
			   set/reset an alarm */
		}
		else if (ev.any.type==evKeyRelease)
		{
			priv->repeatkey=0;
			
			/* FIXME: if there is an alarm set for delay before
			   key repeat, clear it now */
		}
	}
	
	DPRINT_EVENTS("KEY-%s(0x%02x) button=0x%02x modifiers=0x%02x "
		"sym=0x%04x label=0x%04x\n",
		(ev.key.type == evKeyRelease) ? "UP" : 
		((ev.key.type == evKeyPress) ? "DN" : "RP"), ev.key.type,
		ev.key.button, ev.key.modifiers, 
		ev.key.sym,  ev.key.label);
	
	/* check if the application actually requested this evcent */
	if ((inp->curreventmask & (1<<evtype)))
	{
		/* we are suppost to send this event
		   finally queue the key event */
		_giiEvQueueAdd(inp, &ev);
		
		retval|=1<<evtype;
	}
	
	return retval;
}

static inline gii_event_mask
GII_keyboard_handle_data(gii_input *inp, unsigned char key)
{
	l_lk201_priv *priv=(l_lk201_priv*)inp->priv;
	gii_event_mask mask=0;
	int loop;
	
	DPRINT_MISC("GII_keyboard_handle_data: Byte 0x%2x (%3d) received, state=%d\n", key, key, priv->rd_state);
	
	switch (priv->rd_state)	{
	case RDST_IDLE:

		switch (key) {
		case LK_MODECHG_ACK:
			if (priv->nextdiv>0) {
				/* there are still modes to change */
				lk201_modechange(inp, LK_UPDOWN);
			} else if (priv->nextdiv==0) {
				/* there are still modes to change */
				/* send the enable 401 command. if we get
				   another successful mode change message,
				   we have a 401. I think.
				   Otherwise, we should receive an invalid
				   mode command (I have not a lk201. Is this
				   true?) */
				lk201_sendbyte(inp, LK_ENABLE_401);

				priv->nextdiv--;

				DPRINT_MISC("GII_keyboard_handle_data: Keyboard reset successfully\n");
				/* ring the bell */
				/* lk201_sendbyte(inp, LK_RING_BELL); */
			} else {
				/* all of the keyboard modes have been set */
				DPRINT_MISC("GII_keyboard_handle_data: lk401 keyboard detected\n");
			}
			break;
					
		case LK_POWER_UP: /* keyboard has been restarted? */
			priv->rd_state=RDST_KEYBOARDID;
					
			/* fall through */
					
		case LK_ALLUP:
			for (loop=0; loop<=MAX_KEYS; loop++) {
				if (priv->keydown[loop]) {
					DPRINT_MISC("GII_keyboard_handle_data: Key 0x%02x released\n", loop);

					priv->keydown[loop]=0;

					mask |= GII_create_key_event(inp, evKeyRelease, loop);
				}
			}
			break;
			
		case LK_INPUT_ERROR:
			DPRINT_MISC("GII_keyboard_handle_data: Keyboard has indicated an input error. (lk201 keyboard detected?)\n");
			
			break;
			
		default:
			if (keylabel[key]) {
				if (priv->keydown[key]) {
					DPRINT_MISC("GII_keyboard_handle_data: Key 0x%02x released\n", key);
							
					priv->keydown[key]=0;
					
					mask|=GII_create_key_event(inp, evKeyRelease, key);
							
				} else {
					DPRINT_MISC("GII_keyboard_handle_data: Key 0x%02x pressed\n", key);

					priv->keydown[key]=1;
					mask|=GII_create_key_event(inp, evKeyPress, key);
							
				}
						
						
			} else {
				DPRINT_MISC("GII_keyboard_handle_data: Unknown value %d (0x%x) received from keyboard\n", key, key);
			}
			break;
		}
			
		break;
			
	case RDST_KEYBOARDID:
		priv->rd_state=RDST_ERRORCODE;
		break;
			
	case RDST_ERRORCODE:
		priv->rd_state=RDST_KEYCODE;
		break;
			
	case RDST_KEYCODE:
		priv->rd_state=RDST_IDLE;
			
		priv->nextdiv=14;
			
		/* reset the keyboard and put the mode change events in
		   motion */
		lk201_sendbyte(inp, LK_CMD_SET_DEFAULTS);
		break;

		default:
			DPRINT_MISC("GII_keyboard_handle_data: Unknown state\n");

			priv->rd_state=RDST_IDLE;
			break;
	}
	
	return mask;
}

static gii_event_mask GII_keyboard_poll(gii_input *inp, void *arg)
{
	l_lk201_priv *priv = (l_lk201_priv*)inp->priv;
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


EXPORTFUNC int GIIdl_lk201(gii_input *inp, const char *args, void *argptr);

int GIIdl_lk201(gii_input *inp, const char *args, void *argptr)
{
	l_lk201_priv *priv;
	const char *_devname = getenv("GII_LK201_OPTIONS");
	int i;
	
	DPRINT_MISC("lk201 starting.(args=\"%s\",argptr=%p)\n",
		args, argptr);
	
	/* Initialize */
	if (args && *args) {
		_devname = args;
	}
	
	/* parse the device options here */
	
	DPRINT_MISC("lk201: dev=`%s'\n", _devname);
	
	if (!_devname || *_devname == '\0') {
		return GGI_EARGINVAL;
	}
	
	/* allocate lk201 private structure */
	if ((priv = inp->priv = malloc(sizeof(l_lk201_priv))) == NULL) {
		return GGI_ENOMEM;
	}

	if(_giiRegisterDevice(inp,&devinfo,NULL)==0) {
		free(priv);
		return GGI_ENOMEM;
	}
	
	inp->maxfd = 0;
	priv->have_old_termios = 0;
	priv->eof=0;
	priv->rd_state=RDST_IDLE;
	priv->modifiers=0;
	priv->leds=0;
	
	memset(priv->keydown, 0, sizeof(priv->keydown));
	
	if ((i = do_lk201_open(inp, _devname)) < 0) {
		free(priv);
		return i;
	}
	
	inp->GIIsendevent = GIIsendevent;
	inp->GIIclose = GII_lk201_close;
	inp->GIIeventpoll = GII_keyboard_poll;
	
	inp->targetcan = emKey;
	inp->GIIseteventmask(inp, inp->targetcan);

	send_devinfo(inp);
	
	DPRINT_MISC("lk201 fully up\n");
	
	return 0;
}
