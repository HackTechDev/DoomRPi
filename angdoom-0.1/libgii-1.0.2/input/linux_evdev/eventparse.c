/* $Id$
******************************************************************************

   Linux evdev parser

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
#include "linux_evdev.h"
#include <errno.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

static gii_event_mask
dispatch_pmove(struct gii_input *inp, struct input_event *event,
	       gii_event *ev)
{
	_giiEventBlank(ev, sizeof(gii_pmove_event));

	switch (event->code) {
	case REL_X:
		ev->pmove.x = event->value;
		break;
	case REL_Y:
		ev->pmove.y = event->value;
		break;
	case REL_Z:
		ev->pmove.z = event->value;
		break;
	case REL_WHEEL:
		ev->pmove.wheel = event->value;
		break;
	default:
		return 0;
	}

	ev->any.size   = sizeof(gii_pmove_event);
	ev->any.type   = evPtrRelative;
	ev->any.origin = inp->origin;

	return emPtrRelative;
}


static gii_event_mask
dispatch_pbutton(struct gii_input *inp, struct input_event *event,
		 gii_event *ev)
{
	int ret;

	_giiEventBlank(ev, sizeof(gii_pbutton_event));

	switch (event->value) {
	case 0:
		ev->any.type = evPtrButtonRelease;
		ret = emPtrButtonRelease;
		break;
	case 1:
		ev->any.type = evPtrButtonPress;
		ret = emPtrButtonPress;
		break;
	case 2:
		/* Should not happen... */
		ev->any.type = evPtrButtonPress;
		ret = emPtrButtonPress;
		break;
	default:
		return 0;
	}

	ev->any.size = sizeof(gii_pbutton_event);
	ev->any.origin = inp->origin;
	ev->pbutton.button = event->code + 1 - BTN_MOUSE;

	return ret;
}


static gii_event_mask
dispatch_key(struct gii_input *inp, struct input_event *event, gii_event *ev)
{
	int ret;

	_giiEventBlank(ev, sizeof(gii_key_event));

	switch (event->value) {
	case 0:
		ev->any.type = evKeyRelease;
		ret = emKeyRelease;
		break;
	case 1:
		ev->any.type = evKeyPress;
		ret = emKeyPress;
		break;
	case 2:
		ev->any.type = evKeyRepeat;
		ret = emKeyRepeat;
		break;
	default:
		return 0;
	}

	ev->any.size = sizeof(gii_key_event);
	ev->any.origin = inp->origin;

	ev->key.modifiers = 0;
	ev->key.button = event->code;
	if (event->code < 0x100) {
		/* Keyboard key. */

		/* Attention: gcc bug:
		 * passing arg 2 of `GII_levdev_key2label' with different 
		 * width due to prototype
		 */
		ev->key.label = GII_levdev_key2label(inp, event->code);

		ev->key.sym = ev->key.label;
	} else {
		/* Other button. */
		ev->key.label = ev->key.sym = GIIK_VOID;
	}

	return ret;
}


static gii_event_mask
dispatch_rel(struct gii_input *inp, struct input_event *event, gii_event *ev)
{
	_giiEventBlank(ev, sizeof(gii_val_event));

	ev->any.type   = evValRelative;
	ev->any.size   = sizeof(gii_val_event);
	ev->any.origin = inp->origin;
	ev->val.first  = event->code;
	ev->val.count  = 1;
	ev->val.value[0] = event->value;

	return emValRelative;
}


static gii_event_mask
dispatch_abs(struct gii_input *inp, struct input_event *event, gii_event *ev)
{
	_giiEventBlank(ev, sizeof(gii_val_event));

	ev->any.type   = evValAbsolute;
	ev->any.size   = sizeof(gii_val_event);
	ev->any.origin = inp->origin;
	ev->val.first  = event->code;
	ev->val.count  = 1;
	ev->val.value[0] = event->value;

	return emValAbsolute;
}


typedef gii_event_mask (func_dispatcher)(struct gii_input *inp,
				    struct input_event *event,
				    gii_event *ev);

static gii_event_mask
dispatch_event(struct gii_input *inp, struct input_event *event)
{
	func_dispatcher	*dispatcher;
	gii_event ev;
	int ret;

	switch (event->type) {
	case EV_KEY:
		if (event->code >= BTN_MOUSE && event->code < BTN_JOYSTICK) {
			/* Pointer button */
			dispatcher = dispatch_pbutton;
		} else {
			/* Other button/key */
			dispatcher = dispatch_key;
		}
		break;
	case EV_REL:
		switch (event->code) {
		case REL_X:
		case REL_Y:
		case REL_Z:
		case REL_WHEEL:
			/* Pointer move */
			dispatcher = dispatch_pmove;
			break;
		default:
			dispatcher = dispatch_rel;
			break;
		}
		break;
	case EV_ABS:
		dispatcher = dispatch_abs;
		break;
#ifdef EV_SYN
	case EV_SYN:
#endif
#ifdef EV_MSC
	case EV_MSC:
#endif
#ifdef EV_LED
	case EV_LED:
#endif
#ifdef EV_SND
	case EV_SND:
#endif
#ifdef EV_REP
	case EV_REP:
#endif
#ifdef EV_FF
	case EV_FF:
#endif
#ifdef EV_PWR
	case EV_PWR:
#endif
#ifdef EV_FF_STATUS
	case EV_FF_STATUS:
#endif
	default:
		return 0;
	}

	ret = dispatcher(inp, event, &ev);

	if (ret) _giiEvQueueAdd(inp, &ev);

	return ret;
}


#define MAX_EVENTS	64

gii_event_mask
GII_levdev_poll(struct gii_input *inp, void *arg)
{
	gii_levdev_priv *priv = inp->priv;
	struct input_event evbuf[MAX_EVENTS];
	unsigned int i;
	int read_len;
	int ret;

	DPRINT_EVENTS("GII_levdev_poll(%p, %p) called\n", inp, arg);

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
			DPRINT_EVENTS("GII_levdev_poll: dummypoll\n");
			return 0;
		}
	}

	/* The evdev docs guarantee that only complete events will be read. */
	read_len = read(priv->fd, evbuf,
			MAX_EVENTS*sizeof(struct input_event));

	if (read_len <= 0) {
		if (read_len == 0) {
			/* End-of-file occured */
			priv->eof = 1;
			DPRINT_EVENTS("Levdev: EOF occured on fd: %d\n",
				      priv->fd);
			return 0;
		}
		/* Error, we try again next time */
		if (errno == EAGAIN) return 0; /* Don't print warning here */
		perror("Levdev: Error reading events");
		return 0;
	}

	/* Dispatch all events and return queued events. */
	ret = 0;
	for (i = 0; i < read_len / sizeof(struct input_event); i++) {
		ret |= dispatch_event(inp, evbuf+i);
	}

	return ret;
}
