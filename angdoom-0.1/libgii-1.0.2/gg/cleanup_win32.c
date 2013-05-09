/* $Id: cleanup_win32.c,v 1.2 2005/01/20 10:34:57 pekberg Exp $
******************************************************************************

   LibGG - Functions for win32 cleanup

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
#include <ggi/internal/gg.h>
#include "cleanup.h"

static LPTOP_LEVEL_EXCEPTION_FILTER PrevUnhandledExceptionFilter;

/* If an UnhandledExceptionFilter (UEF) is installed after ours,
 * libgg introduces the rule that it should call us if it doesn't
 * handle the exception and check for EXCEPTION_CONTINUE_SEARCH as
 * we do if there is a PrevUnhandledExceptionFilter.
 * TODO: What to do if the previous UEF is uninstalled after
 * we have installed ours? That's a big no-no.
 */
static LONG WINAPI ggUnhandledExceptionFilter(LPEXCEPTION_POINTERS ep)
{
	if(PrevUnhandledExceptionFilter) {
		/* If there was a previous UEF, run it. */
		long res = PrevUnhandledExceptionFilter(ep);
		if(res != EXCEPTION_CONTINUE_SEARCH)
			/* The previous UEF said it handled it, trust it */
			return res;
	}

	if(ep->ExceptionRecord->ExceptionCode == EXCEPTION_STACK_OVERFLOW)
		/* Stack overflow, aiee...
		 * Don't do cleanups, that uses stack.
		 */
		return EXCEPTION_CONTINUE_SEARCH;

	_gg_do_graceful_cleanup();
	/* Didn't handle the exception, tell the OS so. */
	return EXCEPTION_CONTINUE_SEARCH;
}

int _gg_register_os_cleanup(void)
{
	PrevUnhandledExceptionFilter
		= SetUnhandledExceptionFilter(ggUnhandledExceptionFilter);
	return 0;
}

void _gg_unregister_os_cleanup(void)
{
	SetUnhandledExceptionFilter(PrevUnhandledExceptionFilter);
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpvReserved);

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD dwReason, LPVOID lpvReserved)
{
	if(dwReason == DLL_PROCESS_DETACH)
		/* This is perhaps a wee bit late?
		 * Might be worth a shot though.
		 */
		_gg_do_graceful_cleanup();

	return TRUE;
}
