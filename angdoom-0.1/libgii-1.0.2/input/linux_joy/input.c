/* $Id: input.c,v 1.22 2005/08/04 12:43:28 cegger Exp $
******************************************************************************

   Linux_joy: input

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
*/

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <termios.h>

#include <sys/time.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <fcntl.h>

#include <linux/joystick.h>
#include "config.h"
#include <ggi/internal/gii.h>
#include <ggi/internal/gii_debug.h>
#include <ggi/gg.h>
#include <ggi/internal/gg_replace.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif


#define MAX_NR_JAXES		8
#define MAX_NR_JBUTTONS		32 


typedef struct joystick_hook
{
	int fd;

	unsigned char num_axes;
	unsigned char num_buttons;

	int axes[MAX_NR_JAXES];
	char buttons[MAX_NR_JBUTTONS];

} JoystickHook;

#define JOYSTICK_HOOK(inp)	((JoystickHook *) inp->priv)


/* ---------------------------------------------------------------------- */


static gii_cmddata_getdevinfo linux_joy_devinfo =
{
	"Linux Joystick",		/* long device name */
	"ljoy",				/* shorthand */
	emKey |	emValuator,		/* can_generate */
	0, 0				/* filled in later */
};

static gii_cmddata_getvalinfo linux_joy_valinfo[MAX_NR_JAXES] =
{
    {	0,				/* valuator number */
    	"Side to side",			/* long valuator name */
	"x",				/* shorthand */
	{ -32767, 0, +32767 },		/* range */
	GII_PT_LENGTH,			/* phystype */
	0, 1, 1, -8			/* SI constants (approx.) */
    },
    {	1,				/* valuator number */
    	"Forward and back",		/* long valuator name */
	"y",				/* shorthand */
	{ -32767, 0, +32767 },		/* range */
	GII_PT_LENGTH,			/* phystype */
	0, 1, 1, -8			/* SI constants (approx.) */
    },

    /* This last one is used for all extra axes */

    {	2,				/* changed later */
    	"Axis ",			/* appended to later */
	"n",				/* appended to later */
	{ -32767, 0, +32767 },		/* range */
	GII_PT_LENGTH,			/* phystype */
	0, 1, 1, -8			/* SI constants (approx.) */
    },
};



static int GII_joystick_init(gii_input *inp, const char *filename)
{
	JoystickHook *priv;
	int version;
	int fd;
	char name[128];

	/* open the device file */

	fd = open(filename, O_RDONLY);

	if (fd < 0) {
		perror("Linux_joy: Couldn't open joystick device");
		return GGI_ENODEVICE;
	}

	/* get version and get name */
	
	if (ioctl(fd, JSIOCGVERSION, &version) < 0) {
		perror("Linux_joy: Couldn't read version:");
		version=0;
	}

	DPRINT("Linux_joy: Joystick driver version %d.%d.%d\n",
		(version >> 16) & 0xff, (version >> 8) & 0xff,
		version & 0xff);

	if (version < 0x010000) {
		fprintf(stderr, "Linux_joy: Sorry, only driver versions "
			">= 1.0.0 supported.\n");
		close(fd);
		return GGI_ENODEVICE;
	}

	if (ioctl(fd, JSIOCGNAME(sizeof(name)), name) < 0) {
		strcpy(name, "Unknown");
	}

	DPRINT("Linux_joy: Joystick driver name `%s'.\n", name);

	/* allocate joystick hook */

	if ((priv = malloc(sizeof(JoystickHook))) == NULL) {
		close(fd);
		return GGI_ENOMEM;
	}
	
	
	/* get number of axes and buttons */

	if (ioctl(fd, JSIOCGAXES, &priv->num_axes) ||
	    ioctl(fd, JSIOCGBUTTONS, &priv->num_buttons)) {
		perror("Linux_joy: error getting axes/buttons");
		close(fd);
		free(priv);
		return GGI_ENODEVICE;
	}

	DPRINT("Linux_joy: Joystick has %d axes.\n", priv->num_axes);
	DPRINT("Linux_joy: Joystick has %d buttons.\n", priv->num_buttons);

	if (priv->num_axes > MAX_NR_JAXES) {
		priv->num_axes = MAX_NR_JAXES;
	}
	if (priv->num_buttons > MAX_NR_JBUTTONS) {
		priv->num_buttons = MAX_NR_JBUTTONS;
	}

	linux_joy_devinfo.num_axes   =priv->num_axes;
	linux_joy_devinfo.num_buttons=priv->num_buttons;
	
	priv->fd = fd;
	inp->priv = priv;

	DPRINT("Linux_joy: init OK.\n");

	return 0;
}

static void GII_joystick_exit(gii_input *inp)
{
	JoystickHook *priv = JOYSTICK_HOOK(inp);

	close(priv->fd);
	priv->fd = -1;

	free(priv);
	inp->priv = NULL;

	DPRINT("Linux_joy: exit OK.\n");
}

static inline gii_event_mask
GII_joystick_handle_data(gii_input *inp)
{
	JoystickHook *priv = JOYSTICK_HOOK(inp);

	struct js_event js;

	gii_event ev;
	gii_event_mask result=0;

	unsigned int i;


	/* read the joystick packet */

	if (read(priv->fd, &js, sizeof(js)) != sizeof(js)) {
		perror("Linux_joy: Error reading joystick");
		return 0;
	}

	switch (js.type & ~JS_EVENT_INIT) {

	  case JS_EVENT_AXIS:

		if (js.number > priv->num_axes)
			break;
		if (priv->axes[js.number] == js.value)
			break;

		priv->axes[js.number] = js.value;

		DPRINT_EVENTS("JOY-AXIS[%d] -> %d.\n", js.number,
				js.value);

		_giiEventBlank(&ev, sizeof(gii_val_event));

		ev.any.type   = evValAbsolute;
		ev.any.size   = sizeof(gii_val_event);
		ev.any.origin = inp->origin;
		ev.val.first  = 0;
		ev.val.count  = priv->num_axes;

		for (i=0; i < ev.val.count; i++) {
			ev.val.value[i] = priv->axes[i];
		}

		_giiEvQueueAdd(inp, &ev);

		result |= emValAbsolute;
		break;
	
	  case JS_EVENT_BUTTON:

		if (js.number > priv->num_buttons)
			break;
		if (priv->buttons[js.number] == js.value)
			break;

		priv->buttons[js.number] = js.value;

		DPRINT_EVENTS("JOY-BUTTON[%d] -> %d.\n", js.number,
				js.value);

		_giiEventBlank(&ev, sizeof(gii_key_event));

		ev.any.type   = js.value ? evKeyPress : evKeyRelease;
		ev.any.size   = sizeof(gii_key_event);
		ev.any.origin = inp->origin;
		ev.key.modifiers = 0;
		ev.key.button = 1 + js.number;
		ev.key.sym    = GIIK_VOID;
		ev.key.label  = GIIK_VOID;

		_giiEvQueueAdd(inp, &ev);

		result |= (1 << ev.any.type);
		break;

	  default:
		DPRINT_EVENTS("JOY: unknown event from driver "
				"(0x%02x)\n", js.type);
		break;
	}
	
	return result;
}

static gii_event_mask GII_joystick_poll(gii_input *inp, void *arg)
{
	JoystickHook *priv = JOYSTICK_HOOK(inp);
	gii_event_mask result = 0;

	DPRINT_EVENTS("GII_joystick_poll(%p, %p) called\n", inp, arg);
	
	if (arg != NULL) {
		if (! FD_ISSET(priv->fd, ((fd_set*)arg))) {
			/* Nothing to read on our fd */
			DPRINT_EVENTS("GII_joystick_poll: dummypoll\n");
			return 0;
		}
	}

	while (1) {
		fd_set readset = inp->fdset;
		struct timeval t = {0,0};
		int rc;

		rc = select(inp->maxfd, &readset, NULL, NULL, &t);

		if (rc <= 0) break;

		result |= GII_joystick_handle_data(inp);
	}

	return result;
}


/* ---------------------------------------------------------------------- */


static int GII_linux_joy_senddevinfo(gii_input *inp)
{
	JoystickHook *priv = JOYSTICK_HOOK(inp);
	
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

	*DI = linux_joy_devinfo;
	
	DI->num_axes    = priv->num_axes;
	DI->num_buttons = priv->num_buttons;

	return _giiEvQueueAdd(inp, &ev);
}

static int GII_linux_joy_sendvalinfo(gii_input *inp, uint32_t val)
{
	JoystickHook *priv = JOYSTICK_HOOK(inp);
	
	gii_cmddata_getvalinfo *VI;

	gii_event ev;


	if (val >= priv->num_axes) return GGI_ENOSPACE;
	
	_giiEventBlank(&ev, sizeof(gii_cmd_nodata_event) +
		       sizeof(gii_cmddata_getvalinfo));
	
	ev.any.size   = sizeof(gii_cmd_nodata_event) +
		         sizeof(gii_cmddata_getvalinfo);
	ev.any.type   = evCommand;
	ev.any.origin = inp->origin;
	ev.cmd.code   = GII_CMDCODE_GETVALINFO;


	VI = (gii_cmddata_getvalinfo *) ev.cmd.data;

	*VI = linux_joy_valinfo[(val < 2) ? val : 2];
	
	if (val >= 2) {
		char buf[20];
	
		snprintf(buf, 20, "%u", val);

		VI->number = val;
		ggstrlcat(VI->longname, buf, sizeof(VI->longname));
		ggstrlcat(VI->shortname, buf, sizeof(VI->shortname));
	}

	return _giiEvQueueAdd(inp, &ev);
}

static int GII_linux_joy_sendevent(gii_input *inp, gii_event *ev)
{
	JoystickHook *priv = JOYSTICK_HOOK(inp);
	
	if ((ev->any.target != inp->origin) &&
	    (ev->any.target != GII_EV_TARGET_ALL)) {
		/* not for us */
		return GGI_EEVNOTARGET;
	}

	if (ev->any.type != evCommand) {
		return GGI_EEVUNKNOWN;
	}

	if (ev->cmd.code == GII_CMDCODE_GETDEVINFO) {
		return GII_linux_joy_senddevinfo(inp);
	}

	if (ev->cmd.code == GII_CMDCODE_GETVALINFO) {
	
		uint32_t i;
		gii_cmddata_getvalinfo *vi;
		
		vi = (gii_cmddata_getvalinfo *) ev->cmd.data;
		
		if (vi->number == GII_VAL_QUERY_ALL) {
			for (i=0; i < priv->num_axes; i++) {
				GII_linux_joy_sendvalinfo(inp, i);
			}
			return 0;
		}

		return GII_linux_joy_sendvalinfo(inp, vi->number);
	}

	return GGI_EEVUNKNOWN;  /* Unknown command */
}


/* ---------------------------------------------------------------------- */


static int GII_linux_joy_close(gii_input *inp)
{
	DPRINT_MISC("Linux_joy close\n");

	if (JOYSTICK_HOOK(inp)) {
		GII_joystick_exit(inp);
	}

	return 0;
}


EXPORTFUNC int GIIdl_linux_joy(gii_input *inp, const char *args, void *argptr);

int GIIdl_linux_joy(gii_input *inp, const char *args, void *argptr)
{
	int ret;
	const char *filename = "/dev/js0";

	DPRINT_MISC("linux_joy starting.(args=\"%s\",argptr=%p)\n",args,argptr);

	/* Initialize */

	if (args && *args) {
		filename = args;
	}

	if(_giiRegisterDevice(inp,&linux_joy_devinfo,linux_joy_valinfo)==0) {
		return GGI_ENOMEM;
	}

	if ((ret = GII_joystick_init(inp, filename)) < 0) {
		return ret;
	}
	
	/* We leave these on the default handlers
	 *	inp->GIIseteventmask = _GIIstdseteventmask;
	 *	inp->GIIgeteventmask = _GIIstdgeteventmask;
	 *	inp->GIIgetselectfdset = _GIIstdgetselectfd;
	 */
	inp->GIIeventpoll = GII_joystick_poll;
	inp->GIIclose = GII_linux_joy_close;
	inp->GIIsendevent = GII_linux_joy_sendevent;

	inp->targetcan = emKey | emValuator;
	inp->GIIseteventmask(inp, emKey | emValuator);

	/*
	  inp->devinfo    = &xdevinfo;	 FIXME ! Length ! 
	  xdevinfo.origin = inp->origin;
	*/

	inp->maxfd = JOYSTICK_HOOK(inp)->fd + 1;
	FD_SET(JOYSTICK_HOOK(inp)->fd, &inp->fdset);

	/* Send initial cmdDevInfo event */
	GII_linux_joy_senddevinfo(inp);
	
	DPRINT_MISC("linux_joy fully up\n");

	return 0;
}
