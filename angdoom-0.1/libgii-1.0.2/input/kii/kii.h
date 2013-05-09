/* $Id: kii.h,v 1.3 2005/07/31 15:31:12 soyt Exp $
** ----------------------------------------------------------------------------
**	KII manager interface definitions
** ----------------------------------------------------------------------------
**	Copyright (C)	1995-1996	Andreas Beck
**	Copyright (C)	1995-2000	Steffen Seeger
**
**	This file is distributed under the terms and conditions of the
**	MIT/X public license. Please see the file COPYRIGHT.MIT included
**	with this software for details of these terms and conditions.
**
** ----------------------------------------------------------------------------
*/
#ifndef _GGI_KII_H
#define _GGI_KII_H

#include "kgi/config.h"
#include <kgi/system.h>
#define KII_NEED_MODIFIER_KEYSYMS
#include <kii/kii.h>

typedef struct kii_context_s kii_context_t;

extern kii_error_t kiiInit(kii_context_t **ctx);
extern kii_error_t kiiMapDevice(kii_context_t *ctx);
extern int kiiEventDeviceFD(kii_context_t *ctx);

#define	KII_DEVICE_KEYBOARD	1
#define	KII_DEVICE_POINTER	2
extern kii_u_t kiiLegalModifier(kii_context_t *ctx, kii_u_t device, kii_u32_t key);

extern void kiiGetu(kii_context_t *ctx, kii_enum_t var, kii_u_t *val);
extern kii_error_t kiiGetKeymap(kii_context_t *ctx, kii_unicode_t *map,
	kii_u_t keymap, kii_u_t keymin, kii_u_t keymax);

extern kii_u_t kiiEventAvailable(kii_context_t *ctx);
extern const kii_event_t *kiiNextEvent(kii_context_t *ctx);

extern void kiiPrintEvent(kii_context_t *ctx, FILE *f, const kii_event_t *ev);

#endif
