/* $Id: kii.c,v 1.17 2005/06/17 11:57:59 cegger Exp $
******************************************************************************

   Input-KII: Input for KII

   Copyright (C) 2002 Filip Spacek

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
#include <stdio.h>
#include <string.h>

#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include <ggi/internal/gii_debug.h>
#include <ggi/gg.h>
#include <ggi/internal/gg_replace.h>	/* for snprintf() */

#include <ggi/input/kii.h>

#ifndef PATH_MAX	/* GNU Hurd has no PATH_MAX */
# ifdef _POSIX_PATH_MAX
#  define PATH_MAX _POSIX_PATH_MAX
# else
#  define PATH_MAX 4096 /* Should be enough for most systems */
# endif
#endif

#if 0
static int open_event_dev(int flags)
{
	int fd;
	int kiidev;
	char devname[PATH_MAX];

	/* Find an event device to open */
	fd = open("/dev/event", flags);
	if (fd >= 0) return fd;

	/* Retry devfs version */
	fd = open("/dev/kii/event", flags);
	if (fd >= 0) return fd;

	/* Find an event device to open */
	for (kiidev = 0; kiidev < KII_MAX_NR_DEVICES; i++) {

		snprintf(devname, PATH_MAX, "/dev/event%d", kiidev);
		fd = open(devname, flags);
		if (fd >= 0) return fd;

		snprintf(devname, PATH_MAX, "/dev/kii/event%d", kiidev);
		fd = open(devname, flags);
		if (fd >= 0) return fd;
	}	/* for */

	/* return last error code*/
	return fd;
}	/* open_event_dev */
#endif

kii_error_t kiiInit(kii_context_t *ctx, const gg_option *options)
{
	char fopt[2048], *fname;
	union {
		kiic_mapper_identify_request_t	request;
		kiic_mapper_identify_result_t	result;
	} identify;
#ifdef __FreeBSD__
 	union {
		kiic_mapper_attach_request_t	request;
		kiic_mapper_attach_result_t	result;
	} attach;
	union {
		kiic_mapper_get_unit_request_t	request;
		kiic_mapper_get_unit_result_t	result;
	} get_unit;
#endif

	if (NULL == ctx) {
		return -KII_ERRNO(LIB, INVAL);
	}

#if 0
	ctx->mapper.fd = open_even_dev(O_RDWR | O_NONBLOCK);
#else
	ctx->mapper.fd = open("/dev/event", O_RDWR | O_NONBLOCK);
#endif
	if (ctx->mapper.fd < 0) {
		perror("failed to open /dev/eventX and /dev/kii/eventX: ");
		return -KII_ERRNO(LIB, INVAL);
	}

	ggstrlcpy(fopt, options[KII_OPT_DEVICE].result, sizeof(fopt));
	if (strlen(options[KII_OPT_DEVICE].result) > 2047) {
		fprintf(stderr, "option string too long for device\n");
	}
	fname = fopt;
	while (strchr(fname, ',') != NULL) {
		*(strchr(fname, ',')) = '\0';
		ctx->mapper.fd = open(fname, O_RDWR);
		if (ctx->mapper.fd >= 0) goto found;
		fprintf(stderr, "failed to open device %s\n", fname);
		fname += strlen(fname) + 1;
	}
#ifdef __FreeBSD__

	memset(&get_unit, 0, sizeof(get_unit));

	/* If the device id is specified, take it */
	if (sscanf(fname, "/dev/event%d", &get_unit.request.unit) != 1) {
		/* Pass an invalid device id to force auto attachement */
		get_unit.request.unit = -1;
	}

	if (ioctl(ctx->mapper.fd, KIIC_MAPPER_GET_UNIT, &get_unit)) {
		perror("failed to get free unit");
		return errno;
	}

	/* Close /dev/event then open the true one */
	close(ctx->mapper.fd);

	snprintf(fname, 16, "/dev/event%i", get_unit.result.unit);
	ctx->mapper.fd = open(fname, O_RDWR | O_NONBLOCK);
	if (ctx->mapper.fd < 0) {
		perror("failed to open /dev/eventX");
		return errno;
	}

	memset(&attach, 0, sizeof(attach));

	/* Pass an invalid device id to force auto attachement */
	attach.request.device_id = -1;
	if (ioctl(ctx->mapper.fd, KIIC_MAPPER_ATTACH, &attach)) {

		perror("failed to attach to device");
		return errno;
	}
#endif
 found:
	memset(&identify, 0, sizeof(identify));
	ggstrlcpy(identify.request.client, "libkii",
		sizeof(identify.request.client));
	identify.request.client_version.major = 0;
	identify.request.client_version.minor = 0;
	identify.request.client_version.patch = 0;
	identify.request.client_version.extra = 0;

	if (ioctl(ctx->mapper.fd, KIIC_MAPPER_IDENTIFY, &identify) < 0) {
		perror("failed to identify to mapper: ");
		return errno;
	}

	DPRINT("Identified to KII mapper %s-%i.%i.%i-%i.\n",
	       identify.result.mapper,
	       identify.result.mapper_version.major,
	       identify.result.mapper_version.minor,
	       identify.result.mapper_version.patch,
	       identify.result.mapper_version.extra);
	
#if 0
	memset(&ctx->keymap_info, 0, sizeof(ctx->keymap_info));
	if (ioctl(ctx->mapper.fd, KIIC_MAPPER_GET_KEYMAP_INFO,
		&ctx->keymap_info) < 0)
	{
		perror("failed to get keymap info: ");
		return errno;
	}

	DPRINT("keymap info: fn_buf_size %i, fn_str_size %i, "
	       "keymin %i, keymax %i, keymap_size %i, combine_size %i",
	       ctx->keymap_info.fn_buf_size,
	       ctx->keymap_info.fn_str_size,
	       ctx->keymap_info.keymin,
	       ctx->keymap_info.keymax,
	       ctx->keymap_info.keymap_size,
	       ctx->keymap_info.combine_size);
#endif

	return KII_EOK;
}

kii_error_t kiiMapDevice(kii_context_t *ctx)
{
	return (ioctl(ctx->mapper.fd, KIIC_MAPPER_MAP_DEVICE, NULL) == 0)
		? KII_EOK : errno;
}

inline kii_u_t kiiEventAvailable(kii_context_t *ctx)
{
	kii_event_t *event;
	ssize_t count;

	event = (kii_event_t *) (ctx->evbuf.buffer + ctx->evbuf.curr);

	if (ctx->evbuf.size) {

		/*	if the next event is completely in buffer, we have a
		**	event available.
		*/
		if (event->size <= ctx->evbuf.size - ctx->evbuf.curr) {

			return 1;
		}

		/*	if not, move the event fraction to the start of 
		**	the buffer
		*/
		memmove(ctx->evbuf.buffer, 
			ctx->evbuf.buffer + ctx->evbuf.curr,
			ctx->evbuf.size - ctx->evbuf.curr);
		ctx->evbuf.size -= ctx->evbuf.curr;
		ctx->evbuf.curr = 0;
		event = (kii_event_t *) ctx->evbuf.buffer;
	}

	/*	attempt to read new event data
	*/
	count = read(ctx->mapper.fd,
			ctx->evbuf.buffer + ctx->evbuf.size,
			sizeof(ctx->evbuf.buffer) - ctx->evbuf.size);

	if (0 < count) {

		ctx->evbuf.size += count;
	}

	return ctx->evbuf.size &&
		(event->size <= ctx->evbuf.size - ctx->evbuf.curr);
}

int kiiEventDeviceFD(kii_context_t *ctx)
{
	return ctx->mapper.fd;
}

const kii_event_t *kiiNextEvent(kii_context_t *ctx)
{
	kii_event_t *event;

	event = (kii_event_t *) (ctx->evbuf.buffer + ctx->evbuf.curr);

	if (ctx->evbuf.size &&
		(event->size <= ctx->evbuf.size - ctx->evbuf.curr)) {

		ctx->evbuf.curr += event->size;
		if (ctx->evbuf.curr == ctx->evbuf.size) {
			ctx->evbuf.size = ctx->evbuf.curr = 0;
		}

		return event;

	}

	return NULL;
}

void kiiPrintEvent(kii_context_t *kii, FILE *f, const kii_event_t *e)
{
	fprintf(f, "event: size %i, focus %i, device %i, time %i",
		e->any.size, e->any.focus, e->any.device, (int)e->any.time);

	switch (e->any.type) {
	case KII_EV_COMMAND:
		fprintf(f, " COMMAND\n");
		break;

	case KII_EV_BROADCAST:
		fprintf(f, " BROADCAST\n");
		break;

	case KII_EV_DEVICE_INFO:
		fprintf(f, " DEVICE_INFO\n");
		break;

	case KII_EV_RAW_DATA:
		fprintf(f, " RAW_DATA\n");
		break;

	case KII_EV_KEY_PRESS:
		fprintf(f, " KEY_PRESS\n");
		break;

	case KII_EV_KEY_RELEASE:
		fprintf(f, " KEY_RELEASE\n");
		break;

	case KII_EV_KEY_REPEAT:
		fprintf(f, " KEY_REPEAT\n");
		break;

	case KII_EV_KEY_STATE:
		fprintf(f, " KEY_STATE\n");
		break;

	case KII_EV_PTR_RELATIVE:
		fprintf(f, " PTR_RELATIVE\n");
		break;

	case KII_EV_PTR_ABSOLUTE:
		fprintf(f, " PTR_ABSOLUTE\n");
		break;

	case KII_EV_PTR_BUTTON_PRESS:
		fprintf(f, " PTR_BUTTON_PRESS\n");
		break;

	case KII_EV_PTR_BUTTON_RELEASE:
		fprintf(f, " PTR_BUTTON_RELEASE\n");
		break;

	case KII_EV_PTR_STATE:
		fprintf(f, " PTR_STATE\n");
		break;

	case KII_EV_VAL_RELATIVE:
		fprintf(f, " PTR_VAL_RELATIVE\n");
		break;

	case KII_EV_VAL_ABSOLUTE:
		fprintf(f, " PTR_VAL_ABSOLUTE\n");
		break;

	case KII_EV_VAL_STATE:
		fprintf(f, " VAL_STATE\n");
		break;

	case KII_EV_NOTHING:
		fprintf(f, " NOTHING\n");
		break;
	}
	
}

kii_u_t kiiLegalModifier(kii_context_t *kii, kii_u_t device, kii_u32_t key)
{
	return 1;
}

void kiiGetu(kii_context_t *kii, kii_enum_t var, kii_u_t *val)
{
	switch (var) {

	case KII_KBD_MIN_KEYCODE:
		*val = kii->keymap_info.keymin;
		return;

	case KII_KBD_MAX_KEYCODE:
		*val = kii->keymap_info.keymax;
		return;

	case KII_KBD_MAX_MAPSIZE:
		*val = kii->keymap_info.keymap_size;
		return;

	default:
		*val = 0;
		return;
	}
}

kii_error_t kiiGetKeymap(kii_context_t *kii, kii_unicode_t *map,
	kii_u_t keymap, kii_u_t keymin, kii_u_t keymax)
{
	union {
		kiic_mapper_get_keymap_request_t	request;
		kiic_mapper_get_keymap_result_t		result;
	} get_keymap;
	kii_u_t key;

	if ((NULL == map) || (keymax < keymin)) {
		return -KII_ERRNO(LIB, INVAL);
	}

	get_keymap.request.keymap = keymap;
	get_keymap.request.keymin = keymin;
	get_keymap.request.keymax = keymax;

	if (ioctl(kii->mapper.fd, KIIC_MAPPER_GET_KEYMAP, &get_keymap) != 0) {

		return errno;
	}

	for (key = keymin; key <= keymax; key++) {

		if ((key < get_keymap.result.keymin) ||
			(key > get_keymap.result.keymax)) {

			map[key - keymin] = K_VOID;
			continue;
		}
		map[key - keymin] = 
			get_keymap.result.map[key - get_keymap.result.keymin];
	}
	return KII_EOK;
}
