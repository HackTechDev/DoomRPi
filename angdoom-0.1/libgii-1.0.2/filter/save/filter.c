/* $Id: filter.c,v 1.11 2005/08/04 16:36:37 soyt Exp $
******************************************************************************

   Filter-save - save away an eventstream for later playback

   Copyright (C) 1998 Andreas Beck	[becka@ggi-project.org]
   Copyright (C) 1999 Marcus Sundberg	[marcus@ggi-project.org]

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
#include <ggi/internal/gg_replace.h>
#include <ggi/internal/gii-dl.h>
#include <ggi/internal/gii_debug.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


enum outtype { STDOUT,FIL,PIPE };

typedef struct save_state {
	enum outtype	 type;
	FILE		*output;
} save_priv;


static int
GII_save_handler(gii_input *inp, gii_event *event)
{
	save_priv *priv = inp->priv;
	DPRINT_LIBS("filter-save handler,priv=%p file=%p\n",
		    priv,priv->output);
	
	fwrite(event, event->any.size, 1, priv->output);
	
	return 0;
}


static int
GII_save_close(gii_input *inp)
{
	save_priv *priv = inp->priv;

	DPRINT_LIBS("GII_save_close(%p) called\n", inp);

	fflush(priv->output);
	
	switch (priv->type) {
	case FIL:
		fclose(priv->output);
		break;
	case PIPE:
		pclose(priv->output);
		break;
	default: 
		break;
	}
	free(priv);

	DPRINT_LIBS("GII_save_close done\n");

	return 0;
}


EXPORTFUNC int GIIdl_filter_save(gii_input *inp, const char *args, void *argptr);

int GIIdl_filter_save(gii_input *inp, const char *args, void *argptr)
{
	save_priv   *priv;

	DPRINT_LIBS("filter-save init(%p, \"%s\") called\n", inp,
		    args ? args : "");

	priv = malloc(sizeof(save_priv));
	if (priv == NULL) return GGI_ENOMEM;

	priv->output = stdout;
	priv->type = STDOUT;
	
	if (args && *args != '\0') {
		if (*args=='|') {
			fflush(stdout);
			fflush(stderr);
			priv->output = popen(args+1, "wb");
			priv->type = PIPE;
		} else {
			priv->output = fopen(args, "wb");
			priv->type = FIL;
		}
		if (priv->output == NULL) {
			fprintf(stderr, "filter-save: unable to open %s\n",
				args);
			free(priv);
			return GGI_ENODEVICE;
		}
	}

	inp->priv       = priv;
	inp->GIIhandler = GII_save_handler;
	inp->GIIclose   = GII_save_close;
	
	DPRINT_LIBS("filter-save fully up, priv=%p file=%p\n",
		    priv,priv->output);
	
	return 0;
}
