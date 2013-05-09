/* $Id: input.c,v 1.8 2005/08/04 12:43:29 cegger Exp $
******************************************************************************

   Input-null: all-in-one-file.
   
   This is a driver for the "null" device. It never generates any event 
   itself. However it might be useful for things that demand to be handed
   a gii_input_t.
   
   Copyright (C) 1998 Andreas Beck      [becka@ggi-project.org]

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
#include "config.h"
#include <ggi/internal/gii-dl.h>
#include <ggi/internal/gii_debug.h>


EXPORTFUNC int GIIdl_null(gii_input *inp,const char *args,void *argptr);

int GIIdl_null(gii_input *inp,const char *args,void *argptr)
{
	DPRINT_MISC("input-null starting. (args=%s,argptr=%p)\n",
		    args, argptr);
	inp->targetcan=emAll;		/* This should actually be 0. */
	inp->GIIseteventmask(inp,emAll);
	inp->maxfd = 0;
	inp->flags = 0;
	DPRINT_MISC("input-null fully up\n");

	return 0;
}
