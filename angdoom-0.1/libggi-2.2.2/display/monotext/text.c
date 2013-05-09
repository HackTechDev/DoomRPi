/* $Id: text.c,v 1.3 2004/09/12 20:13:40 cegger Exp $
******************************************************************************

   Display-monotext: displaying text

   Copyright (C) 1998 Andrew Apted    [andrew@ggi-project.org]

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
#include <ggi/display/monotext.h>

#include <stdio.h>
#include <stdlib.h>

#undef putc


int GGI_monotext_putc(ggi_visual *vis, int x, int y, char c)
{
	ggi_monotext_priv *priv = MONOTEXT_PRIV(vis);

	int char_w, char_h;
	int err;
	
	ggiGetCharSize(vis, &char_w, &char_h);

	UPDATE_MOD(priv, x, y, char_w, char_h);
	
	if ((err = priv->mem_opdraw->putc(vis, x, y, c)) < 0) {
		return err;
	}

	UPDATE_SYNC;
	return 0;
}