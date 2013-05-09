/* $Id: builtins.c,v 1.14 2005/09/03 18:16:24 soyt Exp $
******************************************************************************

   Libgii builtin targets binding.

   Copyright (C) 2005 Eric Faurot	[eric.faurot@gmail.com]

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
#include <ggi/internal/gii_debug.h>
#include <string.h>

/* XXX in ggi/gii-input.h */
typedef int (giifunc_dlinit)(struct gii_input *inp,
			     const char *args, void *argptr,
			     const char *target);

#ifdef BUILTIN_FILTER_MOUSE
giifunc_dlinit GIIdl_filter_mouse;
#endif
#ifdef BUILTIN_FILTER_KEYTRANS
giifunc_dlinit GIIdl_filter_keytrans;
#endif
#ifdef BUILTIN_FILTER_SAVE
giifunc_dlinit GIIdl_filter_save;
#endif
#ifdef BUILTIN_FILTER_TCP
giifunc_dlinit GIIdl_filter_tcp;
#endif
#ifdef BUILTIN_INPUT_NULL
giifunc_dlinit GIIdl_null;
#endif
#ifdef BUILTIN_INPUT_FILE
giifunc_dlinit GIIdl_file;
#endif
#ifdef BUILTIN_INPUT_IPAQ_TS
giifunc_dlinit GIIdl_ipaq;
#endif
#ifdef BUILTIN_INPUT_ZAURUS_TS
giifunc_dlinit GIIdl_zaurus;
#endif
#ifdef BUILTIN_INPUT_TCP
giifunc_dlinit GIIdl_tcp;
#endif
#ifdef BUILTIN_INPUT_STDIN
giifunc_dlinit GIIdl_stdin;
#endif
#ifdef BUILTIN_INPUT_X
giifunc_dlinit GIIdl_x;
giifunc_dlinit GIIdl_xwin;
#ifdef HAVE_XF86DGA
giifunc_dlinit GIIdl_xf86dga;
#endif
#endif
#ifdef BUILTIN_INPUT_MOUSE
giifunc_dlinit GIIdl_mouse;
#endif
#ifdef BUILTIN_INPUT_LINUX_MOUSE
giifunc_dlinit GIIdl_linux_mouse;
#endif
#ifdef BUILTIN_INPUT_LINUX_KBD
giifunc_dlinit GIIdl_linux_kbd;
#endif
#ifdef BUILTIN_INPUT_LINUX_JOY
giifunc_dlinit GIIdl_linux_joy;
#endif
#ifdef BUILTIN_INPUT_LINUX_EVDEV
giifunc_dlinit GIIdl_linux_evdev;
#endif
#ifdef BUILTIN_INPUT_SPACEORB
giifunc_dlinit GIIdl_spaceorb;
#endif
#ifdef BUILTIN_INPUT_VGL
giifunc_dlinit GIIdl_vgl;
#endif
#ifdef BUILTIN_INPUT_DIRECTX
giifunc_dlinit GIIdl_directx;
#endif
#ifdef BUILTIN_INPUT_PCJOY
giifunc_dlinit GIIdl_pcjoy;
#endif
#ifdef BUILTIN_INPUT_LK201
giifunc_dlinit GIIdl_lk201;
#endif
#ifdef BUILTIN_INPUT_KII
giifunc_dlinit GIIdl_kii;
#endif
#ifdef BUILTIN_INPUT_COCOA
giifunc_dlinit GIIdl_cocoa;
#endif
#ifdef BUILTIN_INPUT_QUARTZ
giifunc_dlinit GIIdl_quartz;
#endif

struct target { 
	const char     *target;
	giifunc_dlinit *func;
};

static struct target _targets[] = {
#ifdef BUILTIN_FILTER_MOUSE
	{ "filter-mouse",             &GIIdl_filter_mouse },
#endif
#ifdef BUILTIN_FILTER_KEYTRANS
	{ "filter-keytrans",          &GIIdl_filter_keytrans },
#endif
#ifdef BUILTIN_FILTER_SAVE
	{ "filter-save",              &GIIdl_filter_save },
#endif
#ifdef BUILTIN_FILTER_TCP
	{ "filter-tcp",               &GIIdl_filter_tcp },
#endif
#ifdef BUILTIN_INPUT_NULL
	{ "input-null",               &GIIdl_null },
#endif
#ifdef BUILTIN_INPUT_FILE
	{ "input-file",               &GIIdl_file },
#endif
#ifdef BUILTIN_INPUT_IPAQ_TS
	{ "input-ipaq-touchscreen",   &GIIdl_ipaq },
#endif
#ifdef BUILTIN_INPUT_ZAURUS_TS
	{ "input-zaurus-touchscreen", &GIIdl_zaurus },
#endif
#ifdef BUILTIN_INPUT_TCP
	{ "input-tcp",                &GIIdl_tcp },
#endif
#ifdef BUILTIN_INPUT_STDIN
	{ "input-stdin",              &GIIdl_stdin },
#endif
#ifdef BUILTIN_INPUT_X
        { "input-x",                  &GIIdl_x },
	{ "input-xwin",               &GIIdl_xwin },
#ifdef HAVE_XF86DGA
	{ "input-xf86dga",            &GIIdl_xf86dga },
#endif
#endif
#ifdef BUILTIN_INPUT_MOUSE
	{ "input-mouse",              &GIIdl_mouse },
#endif
#ifdef BUILTIN_INPUT_LINUX_MOUSE
	{ "input-linux-mouse",        &GIIdl_linux_mouse },
#endif
#ifdef BUILTIN_INPUT_LINUX_KBD
	{ "input-linux-kbd",          &GIIdl_linux_kbd },
#endif
#ifdef BUILTIN_INPUT_LINUX_JOY
	{ "input-linux-joy",          &GIIdl_linux_joy },
#endif
#ifdef BUILTIN_INPUT_LINUX_EVDEV
	{ "input-linux-evdev",        &GIIdl_linux_evdev },
#endif
#ifdef BUILTIN_INPUT_SPACEORB
	{ "input-spaceorb",           &GIIdl_spaceorb },
#endif
#ifdef BUILTIN_INPUT_VGL
	{ "input-vgl",                &GIIdl_vgl },
#endif
#ifdef BUILTIN_INPUT_DIRECTX
	{ "input-directx",            &GIIdl_directx },
#endif
#ifdef BUILTIN_INPUT_PCJOY
	{ "input-pcjoy",              &GIIdl_pcjoy },
#endif
#ifdef BUILTIN_INPUT_LK201
	{ "input-lk201",              &GIIdl_lk201 },
#endif
#ifdef BUILTIN_INPUT_KII
	{ "input-kii",                &GIIdl_kii },
#endif
#ifdef BUILTIN_INPUT_COCOA
	{ "input-cocoa",              &GIIdl_cocoa },
#endif
#ifdef BUILTIN_INPUT_QUARTZ
	{ "input-quartz",             &GIIdl_quartz },
#endif
	{ NULL, NULL }
};


static int GIIdlinit(struct gii_input *inp, const char *args, void *argptr,
		     const char *target) {
	struct target * t;
	
	for(t = _targets; t->target != NULL; t++) {
		if(!strcmp(t->target, target)) {
			DPRINT_LIBS("- try to launch builtin target \"%s\"\n", target);
			return t->func(inp, args, argptr, target);
		}
	}
	
	DPRINT_LIBS("! unknown builtin target \"%s\"\n", target);
	return GGI_ENOTFOUND;
}

static void * _builtins_get(void * _, const char *symbol) {
	if(!strcmp(symbol, GII_DLINIT_SYM))
		return (void *)GIIdlinit;
	DPRINT_LIBS("! builtin symbol '%s' not found\n",symbol);
	return NULL;
}

static gg_scope _builtins;

void _giiInitBuiltins(void);
void _giiExitBuiltins(void);

void _giiInitBuiltins(void)
{
	_builtins = ggNewScope("@libgii", NULL, &_builtins_get,  NULL);
}

void _giiExitBuiltins(void)
{
	ggDelScope(_builtins);
}
