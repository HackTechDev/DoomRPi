/* $Id: misc_win32.c,v 1.3 2004/10/05 07:42:06 pekberg Exp $
******************************************************************************

   LibGG - Misc utility functions for win32

   Copyright (C) 2004 Peter Ekberg	[peda@lysator.liu.se]

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

#include "plat.h"

#ifdef HAVE_SHLOBJ_H

#include <shlobj.h>

static BOOL WINAPI
ProbeSHGetSpecialFolderPathA(HWND hwndOwner, LPSTR lpszPath,
				int nFolder, BOOL fCreate);
static BOOL (WINAPI *SHGetSpecialFolderPathAnsi)(HWND, LPSTR, int, BOOL)
	= ProbeSHGetSpecialFolderPathA;

/*
 * The version to use if we are forced to emulate.
 */
static BOOL WINAPI
EmulateSHGetSpecialFolderPathA(HWND hwndOwner, LPSTR lpszPath,
				int nFolder, BOOL fCreate)
{
	return FALSE;
}

/*
 * Stub that probes to decide which version to use.
 * Probing needed since the Internet Explorer 4.0 Desktop Update
 * is required on NT4 and Win95.
 */
static BOOL WINAPI
ProbeSHGetSpecialFolderPathA(HWND hwndOwner, LPSTR lpszPath,
				int nFolder, BOOL fCreate)
{
	HINSTANCE hinst;

	hinst = LoadLibrary("Shell32");
	*(FARPROC *)(void *)&SHGetSpecialFolderPathAnsi = hinst ?
		GetProcAddress(hinst, "SHGetSpecialFolderPathA") : NULL;

	if(!SHGetSpecialFolderPathAnsi)
		SHGetSpecialFolderPathAnsi = EmulateSHGetSpecialFolderPathA;

	return SHGetSpecialFolderPathAnsi(hwndOwner, lpszPath,
					nFolder, fCreate);
}

#endif /* HAVE_SHLOBJ_H */

static char *get_personal_folder_path(void)
{
#ifdef HAVE_SHLOBJ_H
	static char path[PATH_MAX];
	if(SHGetSpecialFolderPathAnsi(NULL, path, CSIDL_PERSONAL, FALSE))
		return path;
#endif /* HAVE_SHLOBJ_H */
	return NULL;
}
