/* $Id: dl.c,v 1.13 2005/09/03 18:16:24 soyt Exp $
******************************************************************************

   Input library for GGI. Library extensions dynamic loading.

   Copyright (C) 1997 Jason McMullan	[jmcc@ggi-project.org]

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

#include <ggi/internal/gg.h>
#include <ggi/internal/gii.h>
#include <ggi/internal/gii_debug.h>
#include <ggi/internal/gg_replace.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Open the dynamic libary requested
 */
gii_dlhandle *_giiLoadDL(const char *name)
{
	gii_dlhandle hand,*hp;
	struct gg_location_iter match;
	
	DPRINT_LIBS("_giiLoadDL(\"%s\") called \n", name);
	
	hp = NULL;
	match.name = name;
	match.config = _giiconfhandle;
	ggConfigIterLocation(&match);
	GG_ITER_FOREACH(&match) {
		DPRINT_LIBS("match: location=\"%s\" symbol=\"%s\"\n",
			    match.location, match.symbol);
		if ((hand.handle = ggGetScope(match.location)) == NULL) {
			DPRINT_LIBS("cannot open bundle at \"%s\".\n",match.location);
			continue;
		}
		match.symbol = match.symbol ? match.symbol : GII_DLINIT_SYM;
		if ((hand.init = ggFromScope(hand.handle, match.symbol)) == NULL) {
			DPRINT_LIBS("symbol \"%s\" not found.\n", match.symbol);
			ggDelScope(hand.handle);
			continue;
		}
		if ((hp = malloc(sizeof(*hp))) == NULL) {
			DPRINT_LIBS("mem error.\n");
			ggDelScope(hand.handle);
			break;
		}
		memcpy(hp, &hand, sizeof(gii_dlhandle));
		break;
	}
	GG_ITER_DONE(&match);
	return hp;
}

int _giiCloseDL(gii_dlhandle *hand) {
	ggDelScope(hand->handle);
	return 0;
}
