/* $Id: cleanup.h,v 1.1 2004/03/19 09:19:56 pekberg Exp $
***************************************************************************

   LibGG - OS specific cleanup.

   Copyright (C) 2004  Peter Ekberg      [peda@lysator.liu.se]

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

***************************************************************************
*/

#ifndef _GG_INTERNAL_CLEANUP_H
#define _GG_INTERNAL_CLEANUP_H

#include <ggi/gg.h>
#include "config.h"

int _gg_register_os_cleanup(void);
void _gg_unregister_os_cleanup(void);

#endif	/* _GG_INTERNAL_CLEANUP_H */
