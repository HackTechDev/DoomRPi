/* $Id$
***************************************************************************

   LibGG - General Module loading code and scope abstraction

   Copyright (C) 2004-2005  Eric Faurot [eric.faurot@gmail.com]
   Copyright (C) 1998  MenTaLguY   [mentalg@geocities.com]

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

#include "plat.h"
#include <stdlib.h>
#include <string.h>

#include <ggi/gg-queue.h>
#include <ggi/internal/gg.h>
#include <ggi/internal/gg_debug.h>

/* 
**  Implementaton notes: The number of live scopes can become rather
**  high, considering that they can be used for a lot of things. The
**  list implementation provided here is not really efficient. If it
**  is heavily used, it might be a good idea to reimplement it with a
**  hash table.
*/

/*
**  XXX: move this to configure.in or plat.h. The idea is to be able
**  to disable dynamic libraries completely when they are not
**  available/needed. Define this here for now, build will fail in
**  plat.h anyway.
*/
/*
** Don't allow loading of dynamic modules when linking statically.
** I.e. require builtin modules in that case. Solves problem where
** the app is linked agains the static lib, but dynamic modules are
** linked against the dynamic lib.
*/
#ifdef PIC
#define GG_SCOPE_USE_DYNOBJ
#endif

#define	GG_SCOPE_CUSTOM 0
#define	GG_SCOPE_DYNOBJ 1

#define GG_SCOPE_HEADER \
        char *location; \
	int   type;     \
	int   refcount; \
	void *handle;   \
        GG_LIST_ENTRY(_gg_scope) entries;


struct _gg_scope {
	GG_SCOPE_HEADER
};

struct _gg_dynobj_scope {
	GG_SCOPE_HEADER
};

struct _gg_custom_scope {
	GG_SCOPE_HEADER
	
	ggfunc_scope_get  *get;
	ggfunc_scope_del  *del;
};

#define FOREACH_SCOPE(n) GG_LIST_FOREACH(n, &scopes, entries)
#define REMOVE_SCOPE(n)  GG_LIST_REMOVE(n, entries)
#define APPEND_SCOPE(n)  GG_LIST_INSERT_HEAD(&scopes, n, entries)

static GG_LIST_HEAD(scope_list, _gg_scope) scopes;
static void * scopes_lock;

int _ggScopeInit(void)
{
	if ((scopes_lock = ggLockCreate()) == NULL) {
		DPRINT_CORE("! could not allocate scope mutex.\n");
		return GGI_EUNKNOWN;
	}
	
	GG_LIST_INIT(&scopes);
	
	return GGI_OK;
}

int _ggScopeExit(void)
{
	struct _gg_scope   * scope;

	FOREACH_SCOPE(scope) {
		ggDelScope(scope);
	}

	ggLockDestroy(scopes_lock);
	return GGI_OK;
}


static struct _gg_scope *
_new_scope(int type, const char * location, void * handle)
{
	struct _gg_scope   * res;
	
	switch(type) {
#ifdef GG_SCOPE_USE_DYNOBJ
	case GG_SCOPE_DYNOBJ:
		res = calloc(1, sizeof(struct _gg_dynobj_scope));
		break;
#endif /* GG_SCOPE_USE_DYNOBJ */
	case GG_SCOPE_CUSTOM:
		res = calloc(1, sizeof(struct _gg_custom_scope));
		break;
	default:
		DPRINT_SCOPE("! unknow scope type %i\n", type);
		return NULL;
	}
	if (res == NULL) {
		DPRINT_SCOPE("! out of memory in _ggNewScope()\n");
		return NULL;
	}
	
	res->location = strdup(location);
	
	if (res->location == NULL) {
		DPRINT_SCOPE("! out of memory in _ggNewScope()\n");
		free(res);
		return NULL;
	}
	res->type = type;
	res->refcount = 0;
	res->handle = handle;
	APPEND_SCOPE(res);
	return res;
}

gg_scope ggGetScope(const char *location)
{
	struct _gg_scope   * scope;
#ifdef GG_SCOPE_USE_DYNOBJ
	void * ret;
#endif /* GG_SCOPE_USE_DYNOBJ */
	DPRINT_SCOPE("ggGetScope(\"%s\")\n", location);

	if (location == NULL) {
		DPRINT_SCOPE("invalid or unknown location\n");
		return NULL;
	}

        /* 
	**  Protect to ensure that:
	**  - a scope will not arrive after we found it is not there,
	**  - the selected scope is not destroyed before we can refcount it.
	*/
	
	ggLock(scopes_lock);
	
	FOREACH_SCOPE(scope) {
		if (strcmp(scope->location, location) == 0) {
			DPRINT_SCOPE("! scope \"%s\" already loaded\n", location);
			goto ok;
		}
	}

#ifdef GG_SCOPE_USE_DYNOBJ
	/* Fallback to system dl */
	
	ret = GGOPENLIB(location);
	/*
	** NOTE: GGOPENLIBGLOBAL is never used.
	*/
	if (ret == NULL) {
		const char *err = GGDLERROR();
		if (err != NULL) {
			DPRINT_SCOPE("! unable to open lib: %s\n", err);
		}
		ggUnlock(scopes_lock);
		return NULL;
	}
	
	DPRINT_SCOPE("- new scope \"%s\" from library\n", location);

	if ((scope = _new_scope(GG_SCOPE_DYNOBJ, location, ret)) == NULL) {
		DPRINT_SCOPE("! could not allocate scope structure.\n");
		GGCLOSELIB(ret);
		ggUnlock(scopes_lock);
		return NULL;
	}
#else
	DPRINT_SCOPE("! scope not found\n");
	ggUnlock(scopes_lock);
	return NULL;
#endif /* GG_SCOPE_USE_DYNOBJ */
	
ok:
	scope->refcount++;
	ggUnlock(scopes_lock);
	return (gg_scope)scope;
}


void ggDelScope(gg_scope scope)
{
	struct _gg_scope *s = (struct _gg_scope *)scope;
	
	DPRINT_SCOPE("ggDelScope(%p)\n", scope);
	
	/*
	**  Protect to ensure that the scope is not freed twice
	*/
	ggLock(scopes_lock);
	
	if(--s->refcount == 0) {
		switch(s->type) {
#ifdef GG_SCOPE_USE_DYNOBJ
		case GG_SCOPE_DYNOBJ:
			DPRINT_SCOPE("- closing dynamic scope \"%s\"\n", s->location);
			GGCLOSELIB(s->handle);
			break;
#endif
		case GG_SCOPE_CUSTOM:
			DPRINT_SCOPE("- closing custom scope \"%s\"\n", s->location);
			if (((struct _gg_custom_scope *)s)->del)
			    ((struct _gg_custom_scope *)s)->del(s->handle);
			break;
		default:
			DPRINT_SCOPE("! unknown scope type %i\n", s->type);
			break;
		}
		REMOVE_SCOPE(s);
		free(s->location);
		free(s);
	}
	
	ggUnlock(scopes_lock);
}



void * ggFromScope(gg_scope scope, const char *symbol)
{
	struct _gg_scope *s = (struct _gg_scope *)scope;
	
	DPRINT_SCOPE("ggFromScope(%p, \"%s\")\n", scope, symbol);
	
	switch(s->type) {
#ifdef GG_SCOPE_USE_DYNOBJ
	case GG_SCOPE_DYNOBJ:
		DPRINT_SCOPE("- from dynamic scope \"%s\"\n", s->location);
#ifdef GG_SYMPREFIX
		do {
			void *ret;
			char *buffer;
			int buffer_len;
		
			buffer_len = strlen(GG_SYMPREFIX) + strlen(symbol) + 1;
			buffer = malloc(buffer_len);
			ggstrlcpy(buffer, GG_SYMPREFIX, buffer_len);
			ggstrlcat(buffer, symbol, buffer_len);
			ret = GGGETSYM(s->handle, buffer);
			free(buffer);
			return ret;
		} while(0);
#else
		return GGGETSYM(s->handle, symbol);
#endif /* GG_SYMPREFIX */
#endif /* GG_SCOPE_USE_DYNOBJ */
	case GG_SCOPE_CUSTOM:
		DPRINT_SCOPE("- from custom scope \"%s\"\n", s->location);
		return ((struct _gg_custom_scope *)s)->get(s->handle, symbol);
	default:
		DPRINT_SCOPE("! unknown scope type %i\n", s->type);
		break;
	}
	return NULL;
}

gg_scope ggNewScope(const char * location, void * handle,
		    ggfunc_scope_get get, ggfunc_scope_del del)
{
	struct _gg_scope * scope;
	
	DPRINT_SCOPE("ggNewScope(\"%s\", %p, %p, %p)\n", location, handle, get, del);
	
	/* 
	**  Protect to ensure that:
	**  - a scope will not arrive after we found it is not there,
	**  - the selected scope is not destroyed before we can refcount it.
	*/
	
	ggLock(scopes_lock);
	
	FOREACH_SCOPE(scope) {
		if (strcmp(scope->location, location) == 0) {
			DPRINT_SCOPE("- scope \"%s\" exists\n", location);
			ggUnlock(scopes_lock);
			return NULL;
		}
	}
	
	if((scope = _new_scope(GG_SCOPE_CUSTOM, location, handle)) == NULL) {
		return NULL;
	}
	((struct _gg_custom_scope*)scope)->get = get;
	((struct _gg_custom_scope*)scope)->del = del;
	scope->refcount++;
	ggUnlock(scopes_lock);
	
	return scope;
}

/*
***********************************************************
Legacy API
***********************************************************
*/


gg_module ggLoadModule(const char *filename, int flags)
{
	DPRINT("*** ggLoadModule is deprecated\n");
	return (gg_module)ggGetScope(filename);
	
}

gg_module ggMLoadModule(const void *conf, const char *name,
			const char *version, int flags)
{
	const char *filename;
	DPRINT("*** ggMLoadModule is deprecated\n");
	if ((filename = ggMatchConfig(conf, name, version)) == NULL) {
		return GG_MODULE_NULL;
	}
	
	return ggLoadModule(filename, flags);
}

void *ggGetSymbolAddress(gg_module module, const char *symbol)
{
	DPRINT("*** ggGetSymbolAddress is deprecated\n");
	return ggFromScope((gg_scope)module, symbol);
}

void ggFreeModule(gg_module module)
{
	DPRINT("*** ggFreeModule is deprecated\n");
	ggDelScope((gg_scope)module);
}
