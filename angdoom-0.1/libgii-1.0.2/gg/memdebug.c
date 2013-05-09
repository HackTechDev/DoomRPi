/* $Id: memdebug.c,v 1.3 2005/07/29 16:40:52 soyt Exp $
******************************************************************************

   LibGG - Functions for debugging memory usage

   Copyright (C) 1998 Marcus Sundberg	[marcus@ggi-project.org]

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
#include <stdio.h>
#include <ggi/gg.h>

#if defined(HAVE_MALLOC_HOOK) && defined(HAVE_SIGNAL)

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif
#ifdef HAVE_MALLOC_H
#include <malloc.h>
#endif

#define MEMTAG1 0x12345678
#define MEMTAG2 0x09876543
#define MEMTAG3 0x4321ffe4
#define set32idx8(ptr,i,val) *((uint32_t*)(((uint8_t*)(ptr))+(i)))=val
#define get32idx8(ptr,i) *((uint32_t*)(((uint8_t*)(ptr))+(i)))

static void *(*_gg_old_realloc) (void *ptr, size_t size);
static void *(*_gg_old_malloc)  (size_t);
static void  (*_gg_old_free)    (void *ptr);

static unsigned long _gg_alloced = 0;
static int _gg_memdebug_init = 0;

static void _gg_panic(char *arg)
{
	fflush(stdout);
	fprintf(stderr, arg);
	fflush(stderr);
	exit(5);
}


static void *
_gg_malloc_hook(size_t size)
{
	uint32_t *mem;
	__malloc_hook = _gg_old_malloc;
	if ((mem = malloc(size+16)) == NULL) {
		_gg_panic("_gg_malloc_hook failed!\n");
	}
	__malloc_hook = _gg_malloc_hook;
	_gg_alloced += size;
	
	set32idx8(mem,size+8,MEMTAG2);
	set32idx8(mem,size+12,MEMTAG3);
	*mem = size;
	mem++;
	*mem = MEMTAG1;
	mem++;

	return (void*)mem;
}

static void *
_gg_realloc_hook(void *ptr, size_t size)
{
	uint32_t *ret = ptr;

	if (ret != NULL) {
		ret--;
		if (*ret != MEMTAG1) {
			_gg_panic("Inconsistency 1 detected in realloc!\n");
		}
		ret--;
		if (get32idx8(ret,*ret+8) != MEMTAG2) {
			_gg_panic("Inconsistency 2 detected in realloc!\n");
		}
		if (get32idx8(ret,*ret+12) != MEMTAG3) {
			_gg_panic("Inconsistency 3 detected in realloc!\n");
		}
		
		_gg_alloced -= *ret;
	}
	__realloc_hook = _gg_old_realloc;
	if ((ret = realloc(ret, size+16)) == NULL) {
		_gg_panic("_gg_realloc_hook failed!\n");
	}
	__realloc_hook = _gg_realloc_hook;
	_gg_alloced += size;

	set32idx8(ret,size+8,MEMTAG2);
	set32idx8(ret,size+12,MEMTAG3);
	*ret = size;
	ret++;
	*ret = MEMTAG1;
	ret++;

	return ret;
}

static int outofbounds;

static void (*_gg_oldsighandler)(int);

static void 
_gg_freesighandler(int unused)
{
	outofbounds++;
	signal(SIGSEGV, _gg_freesighandler);
}

static void
_gg_free_hook(void *ptr)
{
	uint32_t *ptr32 = ptr;

	if (ptr32 == NULL) {
		_gg_panic("free() called with NULL argument!\n");
	}

	outofbounds = 0;
	_gg_oldsighandler = signal(SIGSEGV, _gg_freesighandler);
	ptr32--;
	if (*ptr32 != MEMTAG1 && !outofbounds) {
		_gg_panic("Inconsistency 1 detected in free!\n");
	}
	ptr32--;
	if (get32idx8(ptr32,*ptr32+8) != MEMTAG2 && !outofbounds) {
		_gg_panic("Inconsistency 2 detected in free!\n");
	}
	if (get32idx8(ptr32,*ptr32+12) != MEMTAG3 && !outofbounds) {
		_gg_panic("Inconsistency 3 detected in free!\n");
	}
	if (outofbounds) {
		puts("free() argument caused SIGSEGV!\n");
		exit(42);
	}
	signal(SIGSEGV, _gg_oldsighandler);
	_gg_alloced -= *ptr32;
	__free_hook = _gg_old_free;
	free(ptr32);
	__free_hook = _gg_free_hook;
}


unsigned long ggGetAlloced(void)
{
	return _gg_alloced;
}


void ggStartMemdebug(void)
{
	if (!_gg_memdebug_init) {
		_gg_old_realloc = __realloc_hook;
		__realloc_hook = _gg_realloc_hook;
		_gg_old_free = __free_hook;
		__free_hook = _gg_free_hook;
		_gg_old_malloc = __malloc_hook;
		__malloc_hook = _gg_malloc_hook;
		_gg_memdebug_init = 1;
	}
}


void ggStopMemdebug(void)
{
	if (_ggi_memdebug_init) {
		__realloc_hook = _ggi_old_realloc;
		__free_hook    = _ggi_old_free;
		__malloc_hook  = _ggi_old_malloc;
		_ggi_memdebug_init = 0;
	}
}

#else /* HAVE_MALLOC_HOOK && HAVE_SIGNAL */

unsigned long ggGetAlloced(void)
{
	return 0;
}

void ggStartMemdebug(void) { }
void ggStopMemdebug(void)  { }

#endif  /* HAVE_MALLOC_HOOK && HAVE_SIGNAL */
