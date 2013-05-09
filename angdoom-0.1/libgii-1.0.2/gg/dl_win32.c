/* $Id: dl_win32.c,v 1.8 2005/08/01 09:18:24 pekberg Exp $
***************************************************************************

   LibGG - Module loading code for Win32/MinGW

   Copyright (C) 2002  Christoph Egger   [Christoph_Egger@t-online.de]
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

#include "config.h"
#include <stdlib.h>
#include <string.h>

#include <ggi/gg.h>
#include <ggi/internal/gg_replace.h>
#include "plat.h"

#include <windows.h>

static DWORD result = ERROR_SUCCESS;
static char error_str[PATH_MAX];

/* ggWin32DLOpen implements a "dlopen" wrapper
 */
gg_dlhand ggWin32DLOpen(const char *filename)
{
	gg_dlhand ret;
	static char message[PATH_MAX];
	
	result = ERROR_SUCCESS;
	ret = LoadLibrary(filename);

	if(ret == NULL) {
		result = GetLastError();

		memset(message, 0, sizeof(message));

		FormatMessage(
			FORMAT_MESSAGE_FROM_SYSTEM,
			NULL,
			result,
			0,
			message,
			sizeof(message),
			NULL);

		snprintf(error_str, PATH_MAX, "%s: %s", filename, message);
	}
	
	return ret;
}	/* ggWin32DLOpen */

/* ggWin32DLError implements a "dlerror()" wrapper
 */
const char *ggWin32DLError(void)
{
	if(result == ERROR_SUCCESS)
		return NULL;

	return error_str;
}	/* ggWin32DLError */
