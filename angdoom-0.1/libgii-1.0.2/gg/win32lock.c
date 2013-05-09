/* $Id: win32lock.c,v 1.4 2004/03/08 07:59:46 pekberg Exp $
******************************************************************************

   LibGG - Mutex implementation using win32 semaphores

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
#include <windows.h>
#include <ggi/internal/gg.h>

void *ggLockCreate(void)
{
	HANDLE *ret;

	ret = CreateSemaphore(NULL, 1, 1, NULL);
	if (ret == INVALID_HANDLE_VALUE)
		/* Extra precaution, should not happen */
		return NULL;

	return (void *) ret;
}

int ggLockDestroy(void *lock)
{
	/* If lock is locked, or NULL, it's better to crash hard than allow
	   a broken app continue to run. */
	CloseHandle((HANDLE)lock);
	return 0;
}
	
void ggLock(void *lock)
{
	WaitForSingleObject((HANDLE)lock, INFINITE);
}

void ggUnlock(void *lock)
{
	ReleaseSemaphore((HANDLE)lock, 1, NULL);
}

int ggTryLock(void *lock)
{
	if (WaitForSingleObject((HANDLE)lock, 0) == WAIT_TIMEOUT)
		return GGI_EBUSY;

	return 0;
}

int _ggInitLocks(void)
{
	return 0;
}

void _ggExitLocks(void)
{
	return;
}
