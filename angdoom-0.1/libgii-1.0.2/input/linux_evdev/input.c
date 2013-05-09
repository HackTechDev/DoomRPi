/* $Id$
******************************************************************************

   Linux evdev inputlib

   Copyright (C) 2000 Marcus Sundberg	[marcus@ggi-project.org]
   Copyright (C) 2001 Stefan Seefeld	[stefan@berlin-consortium.org]

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
#include "linux_evdev.h"
#include <ggi/gg.h>
#include <ggi/internal/gii.h>

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <fcntl.h>
#include <unistd.h>

#define SKIPWHITE(str)	while (isspace((uint8_t)*(str))) (str)++

static const char *events[EV_MAX + 1] =
    { "Reset", "Key", "Relative", "Absolute", NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, "LED", "Sound", NULL, "Repeat"
};
static const char *keys[KEY_MAX + 1] =
    { "Reserved", "Esc", "1", "2", "3", "4", "5", "6", "7", "8", "9", "0",
	"Minus", "Equal", "Backspace",
	"Tab", "Q", "W", "E", "R", "T", "Y", "U", "I", "O", "P",
	    "LeftBrace",
	"RightBrace", "Enter", "LeftControl", "A", "S", "D", "F", "G",
	"H", "J", "K", "L", "Semicolon", "Apostrophe", "Grave",
	    "LeftShift",
	"BackSlash", "Z", "X", "C", "V", "B", "N", "M", "Comma", "Dot",
	"Slash", "RightShift", "KPAsterisk", "LeftAlt", "Space",
	    "CapsLock", "F1",
	"F2", "F3", "F4", "F5", "F6", "F7", "F8", "F9", "F10",
	"NumLock", "ScrollLock", "KP7", "KP8", "KP9", "KPMinus", "KP4",
	    "KP5",
	"KP6", "KPPlus", "KP1", "KP2", "KP3", "KP0", "KPDot", "103rd",
	"F13", "102nd", "F11", "F12", "F14", "F15", "F16", "F17", "F18",
	    "F19",
	"F20", "KPEnter", "RightCtrl", "KPSlash", "SysRq",
	"RightAlt", "LineFeed", "Home", "Up", "PageUp", "Left", "Right",
	    "End",
	"Down", "PageDown", "Insert", "Delete", "Macro", "Mute",
	"VolumeDown", "VolumeUp", "Power", "KPEqual", "KPPlusMinus",
	    "Pause", "F21",
	"F22", "F23", "F24", "KPComma", "LeftMeta", "RightMeta",
	"Compose", "Stop", "Again", "Props", "Undo", "Front", "Copy",
	    "Open",
	"Paste", "Find", "Cut", "Help", "Menu", "Calc", "Setup",
	"Sleep", "WakeUp", "File", "SendFile", "DeleteFile", "X-fer",
	    "Prog1",
	"Prog2", "WWW", "MSDOS", "Coffee", "Direction",
	"CycleWindows", "Mail", "Bookmarks", "Computer", "Back", "Forward",
	"CloseCD", "EjectCD", "EjectCloseCD", "NextSong", "PlayPause",
	"PreviousSong", "StopCD", "Record", "Rewind", "Phone", "ISOKey",
	    "Config",
	"HomePage", "Refresh", "Exit", "Move", "Edit", "ScrollUp",
	"ScrollDown", "KPLeftParenthesis", "KPRightParenthesis",
	"International1", "International2", "International3",
	    "International4",
	"International5",
	"International6", "International7", "International8",
	    "International9",
	"Language1", "Language2", "Language3", "Language4", "Language5",
	"Language6", "Language7", "Language8", "Language9",
	NULL,
	"PlayCD", "PauseCD", "Prog3", "Prog4", "Suspend", "Close",
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	    NULL,
	NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	    NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	    NULL,
	NULL, NULL,
	"Btn0", "Btn1", "Btn2", "Btn3", "Btn4", "Btn5", "Btn6", "Btn7",
	    "Btn8",
	"Btn9",
	NULL, NULL, NULL, NULL, NULL, NULL,
	"LeftBtn", "RightBtn", "MiddleBtn", "SideBtn", "ExtraBtn",
	    "ForwardBtn",
	"BackBtn",
	NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
	"Trigger", "ThumbBtn", "ThumbBtn2", "TopBtn", "TopBtn2",
	    "PinkieBtn",
	"BaseBtn", "BaseBtn2", "BaseBtn3", "BaseBtn4", "BaseBtn5",
	    "BaseBtn6",
	NULL, NULL, NULL, NULL,
	"BtnA", "BtnB", "BtnC", "BtnX", "BtnY", "BtnZ", "BtnTL", "BtnTR",
	    "BtnTL2",
	"BtnTR2", "BtnSelect", "BtnStart", "BtnMode",
	NULL, NULL, NULL,
	"ToolPen", "ToolRubber", "ToolBrush", "ToolPencil", "ToolAirbrush",
	"ToolFinger", "ToolMouse", "ToolLens", NULL, NULL,
	"Touch", "Stylus", "Stylus2"
};

#if 0	/* defined but not used */
static const char *absval[5] = { "Value", "Min  ", "Max  ", "Fuzz ", "Flat " };
#endif
static const char *relatives[REL_MAX + 1] =
    { "X", "Y", "Z", NULL, NULL, NULL, "HWheel", "Dial", "Wheel" };
static const char *absolutes[ABS_MAX + 1] =
    { "X", "Y", "Z", "Rx", "Ry", "Rz", "Throttle", "Rudder", 
      "Wheel", "Gas", "Brake", NULL, NULL, NULL, NULL, NULL,
      "Hat0X", "Hat0Y", "Hat1X", "Hat1Y", "Hat2X", "Hat2Y", "Hat3X", "Hat 3Y",
      "Pressure", "Distance", "XTilt", "YTilt"
};
static const char *leds[LED_MAX + 1] =
    { "NumLock", "CapsLock", "ScrollLock", "Compose", "Kana", "Sleep",
	"Suspend", "Mute"
};
static const char *repeats[REP_MAX + 1] = { "Delay", "Period" };
static const char *sounds[SND_MAX + 1] = { "Bell", "Click" };

static const char **names[EV_MAX + 1] =
	{ events, keys, relatives, absolutes, NULL, NULL, NULL, NULL, 
	  NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
	  NULL, leds, sounds, repeats
	};

static void init_devinfo(gii_input * inp)
{
	unsigned int i, n = 0;
	int _abs[5];

	gii_levdev_priv *priv = inp->priv;

	memset(&(priv->devinfo), 0, sizeof(priv->devinfo));
	ioctl(priv->fd, EVIOCGNAME(sizeof(priv->devinfo.longname)),
	      priv->devinfo.longname);

	memset(priv->bit, 0, sizeof(priv->bit));
	ioctl(priv->fd, EVIOCGBIT(0, EV_MAX), priv->bit[0]);
	/* key events */
	if (test_bit(1, priv->bit[0])) {
		ioctl(priv->fd, EVIOCGBIT(1, KEY_MAX), priv->bit[1]);
		for (i = 0; i < KEY_MAX; i++)
			if (test_bit(i, priv->bit[1]))
				n++;
	}
	priv->devinfo.num_buttons = n;

	/* find highest indexed valuator */
	n = 0;
	if (test_bit(3, priv->bit[0])) {
		ioctl(priv->fd, EVIOCGBIT(3, KEY_MAX), priv->bit[3]);
		for (i = 0; i < KEY_MAX; i++)
			if (test_bit(i, priv->bit[3]))
				n = n > i ? n : i;
	}
	/* FIXME:
	 * 
	 * This is actually not the number of axes, as the variable name 
	 * suggests, but the highest index of a valuator.  There can be
	 * dead valuator indices.
	 * 
	 * Likely the best "fix" is to simply document this behavior and
	 * for the apps to probe each valuator up to num_axes to ensure 
	 * it is actually present.
	 *
	 */
	priv->devinfo.num_axes = n + 1;

	priv->devinfo.can_generate = emAll;  /* FIXME: be more accurate */

	/* now find valuators */
	if (test_bit(EV_ABS, priv->bit[0])) {
		ioctl(priv->fd, EVIOCGBIT(EV_ABS, KEY_MAX), priv->bit[EV_ABS]);
		/*        for (i = 0; i < KEY_MAX; i++)  */
		for (i = 0; i < KEY_MAX; i++) {
			if (test_bit(i, priv->bit[EV_ABS])) {
				const char *name =
				    names[EV_ABS] ? (names[EV_ABS][i] ?
						     names[EV_ABS][i] :
						     "?") : "?";
				ioctl(priv->fd, EVIOCGABS(i), _abs);
				priv->valinfo[i].number = i;
				priv->valinfo[i].range.min = _abs[1];
				priv->valinfo[i].range.max = _abs[2];
				/* FIXME: enumerate shortnames above.
				 * For now, we truncate the longname.
				 */
				ggstrlcpy(priv->valinfo[i].shortname, name,
					(size_t)(4));
				ggstrlcpy(priv->valinfo[i].longname, name,
					(size_t)(75));
			}
		}
	}
}

static void send_devinfo(gii_input * inp)
{
	gii_event ev;
	gii_cmddata_getdevinfo *dinfo;
	size_t size =
	    sizeof(gii_cmd_nodata_event) + sizeof(gii_cmddata_getdevinfo);

	gii_levdev_priv *priv = inp->priv;

	_giiEventBlank(&ev, size);

	ev.any.size = size;
	ev.any.type = evCommand;
	ev.any.origin = inp->origin;
	ev.cmd.code = GII_CMDCODE_GETDEVINFO;

	dinfo = (gii_cmddata_getdevinfo *) ev.cmd.data;
	*dinfo = priv->devinfo;

	_giiEvQueueAdd(inp, &ev);
}

static void send_valinfo(gii_input * inp, uint32_t n)
{
	gii_event ev;
	gii_cmddata_getvalinfo *vinfo;
	size_t size =
	    sizeof(gii_cmd_nodata_event) + sizeof(gii_cmddata_getvalinfo);
	gii_levdev_priv *priv = inp->priv;

	_giiEventBlank(&ev, size);

	ev.any.size = size;
	ev.any.type = evCommand;
	ev.any.origin = inp->origin;
	ev.cmd.code = GII_CMDCODE_GETVALINFO;

	vinfo = (gii_cmddata_getvalinfo *) ev.cmd.data;
	*vinfo = priv->valinfo[n];
	_giiEvQueueAdd(inp, &ev);
}


static int GIIsendevent(gii_input * inp, gii_event * ev)
{
	gii_levdev_priv *priv = inp->priv;

	if (ev->any.target != inp->origin
	    && ev->any.target != GII_EV_TARGET_ALL)
		return GGI_EEVNOTARGET;	/* not for us */

	if (ev->any.type != evCommand)
		return GGI_EEVUNKNOWN;

	if (ev->cmd.code == GII_CMDCODE_GETDEVINFO) {
		send_devinfo(inp);
		return 0;
	}

	if (ev->cmd.code == GII_CMDCODE_GETVALINFO) {
		gii_cmddata_getvalinfo *vi =
		    (gii_cmddata_getvalinfo *) ev->cmd.data;
		if (vi->number == GII_VAL_QUERY_ALL) {
			if (test_bit(3, priv->bit[0])) {
				uint32_t i;
				for (i = 0; i < KEY_MAX; i++)
					if (test_bit(i, priv->bit[3]))
						send_valinfo(inp, i);
			}
			return 0;
		}

		send_valinfo(inp, vi->number);
		return 0;
	}

	return GGI_EEVUNKNOWN;	/* Unknown command */
}


static int GIIclose(struct gii_input *inp)
{
	gii_levdev_priv *priv = inp->priv;

	if (priv->fd)
		close(priv->fd);
	free(priv);

	return 0;
}


EXPORTFUNC int GIIdl_linux_evdev(gii_input * inp,
				const char *args, void *argptr);

int GIIdl_linux_evdev(gii_input * inp, const char *args, void *argptr)
{
	const char *devname = "/dev/input/event0";
	gii_levdev_priv *priv;
	int fd;

	DPRINT_LIBS("Linux evdev starting.\n");

	if (args && *args)
		devname = args;

	fd = open(devname, O_RDONLY);
	if (fd < 0)
		return GGI_ENODEVICE;

	priv = malloc(sizeof(*priv));
	if (priv == NULL) {
		close(fd);
		return GGI_ENOMEM;
	}

	if (_giiRegisterDevice(inp, &priv->devinfo, &priv->valinfo[0]) == 0) {
		free(priv);
		close(fd);
		return GGI_ENOMEM;
	}


	inp->GIIsendevent = GIIsendevent;
	inp->GIIeventpoll = GII_levdev_poll;
	inp->GIIclose = GIIclose;

	inp->targetcan = emAll;
	inp->curreventmask = emAll;

	inp->maxfd = fd + 1;
	FD_SET(fd, &inp->fdset);

	priv->fd = fd;
	priv->eof = 0;

	inp->priv = priv;

	init_devinfo(inp);
	send_devinfo(inp);

	DPRINT_LIBS("Linux evdev up.\n");

	return 0;
}
