/* $Id: gii_debug.h,v 1.13 2005/07/31 15:31:11 soyt Exp $
******************************************************************************

   LibGII debugging macros

   Copyright (C) 1998-1999 Marcus Sundberg	[marcus@ggi-project.org]

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

#ifndef _GGI_INTERNAL_GII_DEBUG_H
#define _GGI_INTERNAL_GII_DEBUG_H

#include <ggi/gii-defs.h>
#include <ggi/internal/debug_macros.h>

#define DEBUG_ISSYNC   (_giiDebug&DEBUG_SYNC)

#define DEBUG_CORE     (1<<1)	/*   2 */
#define DEBUG_MISC     (1<<5)	/*  32 */
#define DEBUG_LIBS     (1<<6)	/*  64 */
#define DEBUG_EVENTS   (1<<7)	/* 128 */

__BEGIN_DECLS

GIIAPIVAR uint32_t _giiDebug;

static inline void DPRINT(const char *form,...)        { DPRINTIF(_giiDebug,DEBUG_ALL);    }
static inline void DPRINT_CORE(const char *form,...)   { DPRINTIF(_giiDebug,DEBUG_CORE);   }
static inline void DPRINT_MISC(const char *form,...)   { DPRINTIF(_giiDebug,DEBUG_MISC);   }
static inline void DPRINT_LIBS(const char *form,...)   { DPRINTIF(_giiDebug,DEBUG_LIBS);   }
static inline void DPRINT_EVENTS(const char *form,...) { DPRINTIF(_giiDebug,DEBUG_EVENTS); }

static inline void DPRINT2_CORE(const char *form,...)   {}
static inline void DPRINT2_EVENTS(const char *form,...) {}

__END_DECLS

#endif /* _GGI_INTERNAL_GII_DEBUG_H */
