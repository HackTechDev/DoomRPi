/* $Id$
******************************************************************************
  
   LibGG - Configuration handling
  
   Copyright (C) 1997 Jason McMullan	[jmcc@ggi-project.org]
   Copyright (C) 1998 Marcus Sundberg	[marcus@ggi-project.org]
   Copyright (C) 2005 Eric Faurot	[eric.faurot@gmail.com]
   
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
#include <stdlib.h>
#include <string.h>
#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif
#include <ctype.h>

/* for stat */
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif


#include <ggi/gg.h>
#include <ggi/internal/gg_debug.h>

#define GG_ENTRY_MODULE  0
#define GG_ENTRY_ALIAS   1

/*
**  A module entry maps a pattern, matching canonical names to a
**  location (scope) and symbol within that location.
*/
struct _gg_entry_module {
	char *pattern;
	char *location;
	char *symbol;
};
/*
**  An alias maps simple names to a values, which are supposed to be
**  list of (canonical name, options) tuples.
*/
struct _gg_entry_alias {
	char *name;
	char *value;
};

struct _gg_entry {
	int type;
	GG_SLIST_ENTRY(_gg_entry) entries;
	union {
		struct _gg_entry_module module;
		struct _gg_entry_alias  alias;
	} _;
};

struct _gg_config {
	GG_SLIST_HEAD(_entry, _gg_entry) entries;
	struct _gg_entry * last;
};

struct _line_parser {
        struct gg_iter  iter;
	int             lineno;
	char            line[2048];
	void            *_state;
};

static int parse_file_next(struct _line_parser * iter) {
	if(fgets(iter->line, sizeof(iter->line), (FILE*)(iter->_state)) != NULL) {
		iter->lineno++;
		return ITER_YIELD;
	}
	return ITER_DONE;
}

static void parse_file_done(struct _line_parser *iter) {
	fclose((FILE*)(iter->_state));
}

static int parse_file_init(struct _line_parser *iter, const char * filename) {
	FILE *file;
	
	file = fopen(filename, "r");
	if (file == NULL) {
		DPRINT("! file not found : \"%s\"\n", filename);
		return GGI_ENOTFOUND;
	}
	iter->_state = file;
	iter->lineno = 0;
	GG_ITER_PREPARE(iter, parse_file_next, parse_file_done);
	return GGI_OK;
}


static int parse_array_next(struct _line_parser * iter) {
	char **array;

	array = (char **)iter->_state;
	if (*array != NULL) {
		ggstrlcpy(iter->line, *array, sizeof(iter->line));
		iter->_state = array + 1;
		iter->lineno++;
		return ITER_YIELD;
	}
	return ITER_DONE;
}

static void parse_array_done(struct _line_parser *iter) {
	return;
}

static int parse_array_init(struct _line_parser *iter, char ** array) {
	iter->_state = array;
	iter->lineno = 0;
	GG_ITER_PREPARE(iter, parse_array_next, parse_array_done);
	return GGI_OK;
}

static int parse_string_next(struct _line_parser * iter) {
	char *s = iter->_state;
	size_t l;
	while(*s) {
		if (*s == '\n' || *s == '\r') {
			l = s - ((char*)iter->_state);
			if(*s == '\r') s++;
			if(*s == '\n') s++;
			if(l >= sizeof(iter->line))
				continue; /* line too long */
			memcpy(iter->line, (char*)iter->_state, l);
			iter->line[l] = '\0';
			iter->_state = (void*)s;
			iter->lineno++;
			return ITER_YIELD;
		}
		s++;
	}
	return ITER_DONE;
}

static void parse_string_done(struct _line_parser *iter) {
	return;
}

static int parse_string_init(struct _line_parser *iter, char * buffer) {
	iter->_state = (void*)buffer;
	iter->lineno = 0;
	GG_ITER_PREPARE(iter, parse_string_next, parse_string_done);
	return GGI_OK;
}

#define BETWEEN(x,a,b) (((x) >= (a)) && ((x) <= (b)))
#define is_HTAB(c)     ((c) == 0x09)
#define is_SP(c)       ((c) == ' ')
#define is_WSP(c)      (is_HTAB(c) || is_SP(c))
/*#define is_CHAR(c)     (BETWEEN((c), 0x21, 0xff) && !is_WSP(c))*/
#define is_CHAR(c)     (((c) >= 0x21) && !is_WSP(c))


/* Split a string into at most n words. Put 0 at the relevant places
   and return the number of words */
static int strsplit(char *s, char **r, int n)
{
	int seen = 0;
	for (;;) {
		while (is_WSP((uint8_t) * s)) s++;
		if (*s == 0) break;
		r[seen++] = s;
		while (is_CHAR((uint8_t) * s)) s++;
		if (*s == 0 || seen == n) break;
		*s = 0;
		s++;
	}
	return seen;
}

#define FREE(p) if (p != NULL) free(p)

static int _doLoadFromFile(const char *filename,
			   struct _gg_config *cfg, const char *oroot, int depth);

#define NWORDS 4
#define MAX_INCLUDE_DEPTH 10
static int _doLoad(struct _line_parser *lines, const char *filename,
		   struct _gg_config *cfg, const char *oroot, int depth)
{
	char root[2048], *w[NWORDS], *c;
	int n, l;
	
	struct _gg_entry *entry;
		
	DPRINT("- adding config from \"%s\"\n", filename);
	
	ggstrlcpy(root, oroot, sizeof(root));
	
	GG_ITER_FOREACH(lines) {
		entry = NULL;
		
		n = strsplit(lines->line, w, NWORDS);
		/* count non-comments */
		l = 0;
		while (l < n && w[l][0] != '#')
			l++;
		switch (l) {
		case 2:
			if (!strcmp(w[0], ".root:")) {
				/* XXX detect truncation */
				if (w[1][0] == '/')
					ggstrlcpy(root, w[1],
						  sizeof(root));
				else
					ggstrlcat(root, w[1],
						  sizeof(root));
				n = strlen(root);
				if (root[n - 1] != '/')
					ggstrlcat(root, "/", sizeof(root));
				DPRINT("- new root: \"%s\"\n", root);
				continue;
			}

			if (!strcmp(w[0], ".include")) {
				if (depth < MAX_INCLUDE_DEPTH)
					_doLoadFromFile(w[1], cfg, root, depth + 1);
				else {
					DPRINT("! %s:%i: include recursion too deep.\n",
					       filename, lines->lineno);
				}
				continue;
			}
			
			/* assume it is a target definition */
			if ((entry = calloc(1, sizeof(*entry))) == NULL)
				goto module0;

			if ((entry->_.module.pattern = strdup(w[0])) == NULL)
				goto module1;
			entry->_.module.symbol = NULL;
			if ((c = strchr(w[1], ':')) != NULL) {
				*c = 0;
				if ((entry->_.module.symbol =
				     strdup(c + 1)) == NULL)
					goto module1;
			}
			if (w[1][0] == '/' || w[1][0] == '@') {
				/* absolute file path or builtin scope */
				if ((entry->_.module.location =
				     strdup(w[1])) == NULL)
					goto module1;
			} else {
				/* relative path */
				n = strlen(root) + strlen(w[1]) + 1;
				if ((entry->_.module.location = malloc(n)) == NULL)
					goto module1;
				ggstrlcpy(entry->_.module.location, root, n);
				ggstrlcat(entry->_.module.location, w[1], n);
			}
			DPRINT("- new module: \"%s@%s:%s\"\n",
			       entry->_.module.pattern,
			       entry->_.module.location,
			       entry->_.module.symbol);
			entry->type = GG_ENTRY_MODULE;
			break;
module1:
			FREE(entry->_.module.location);
			FREE(entry->_.module.symbol);
			FREE(entry->_.module.pattern);
			FREE(entry);
module0:
			DPRINT("! out of mem in _doLoad.\n");
			continue;
		case 3:
		  if (!strcmp(w[0], "alias")) {
				if ((entry =
				     calloc(1, sizeof(*entry))) == NULL)
					goto alias0;
				if ((entry->_.alias.name = strdup(w[1])) == NULL)
					goto alias1;
				if ((entry->_.alias.value = strdup(w[2])) == NULL)
					goto alias1;

				DPRINT("- new alias: \"%s\" as \"%s\"\n",
				       entry->_.alias.name,
				       entry->_.alias.value);
				entry->type = GG_ENTRY_ALIAS;
				break;
alias1:
				FREE(entry->_.alias.name);
				FREE(entry->_.alias.value);
				FREE(entry);
alias0:
				DPRINT("! out of mem in _doLoad.\n");
				continue;
			}
		default:
			continue;
		}
		if (entry) {
			if(cfg->last)
				GG_SLIST_INSERT_AFTER(cfg->last,entry,entries);
			else
				GG_SLIST_INSERT_HEAD(&cfg->entries,entry,entries);
			cfg->last = entry;
		}
	}
	GG_ITER_DONE(lines);
	DPRINT("- done with \"%s\"\n", filename);
	return GGI_OK;
}

static int _doLoadFromFile(const char *filename,
			   struct _gg_config *cfg, const char *oroot, int depth) {
	int err;
	struct _line_parser lines;
	struct stat sb;
	
#if defined(S_IFMT) && defined(S_IFREG)
	/* make sure that file is a regular file */
	if ((err = stat(filename, &sb))) {
		DPRINT("! could not stat(2) %s (err %i).\n", filename, err);
		return GGI_EARGINVAL;
	}

	if ((sb.st_mode & S_IFMT) != S_IFREG) {
		DPRINT("! %s is not a regular file\n", filename);
		return GGI_EARGINVAL;
	}
#endif
	if ((err = parse_file_init(&lines, filename)) != GGI_OK) {
		DPRINT("! cannot initialize file parser\n");
		return err;
	}
	
	return _doLoad(&lines, filename, cfg, oroot, depth);
}

/*
 * array MUST end with NULL: { "line1", "line2", "line3", NULL }
 */
static int _doLoadFromArray(char **array,
			    struct _gg_config *cfg, const char *oroot, int depth) {
	int err;
	char filename[64];
	struct _line_parser lines;
	snprintf(filename, sizeof(filename), "array@%p", (void *)array);
	if ((err = parse_array_init(&lines, array)) != GGI_OK) {
		DPRINT("! cannot initialize file parser\n");
		return err;
	}
	return _doLoad(&lines, filename, cfg, oroot, depth);
}

/*
 * string must and with '\0'. Line separator is either LF or CR+LF
 */
static int _doLoadFromString(char *string,
			     struct _gg_config *cfg, const char *oroot, int depth) {
	int err;
	char filename[64];
	struct _line_parser lines;
	snprintf(filename, sizeof(filename), "string@%p", (void *)string);
	if ((err = parse_string_init(&lines, string)) != GGI_OK) {
		DPRINT("! cannot initialize string parser\n");
		return err;
	}
	return _doLoad(&lines, filename, cfg, oroot, depth);
}

static void _dumpConfig(struct _gg_config *c) {
	struct _gg_entry *e;
	
	GG_SLIST_FOREACH(e, &c->entries, entries)
		switch (e->type) {
		case GG_ENTRY_MODULE:
			printf("MODULE \"%s\" at \"%s\" symbol \"%s\"\n",
			       e->_.module.pattern,
			       e->_.module.location,
			       e->_.module.symbol);
			break;
		case GG_ENTRY_ALIAS:
			printf("ALIAS \"%s\" for \"%s\"\n",
			       e->_.alias.name,
			       e->_.alias.value);
			break;
		default:
			printf("UNKNOWN type %i.\n", e->type);
			break;
		}
}

int ggLoadConfig(const char *filename, gg_config *confptr)
{
	struct _gg_config *res;
	char **array;
	char *string;
	int ret;

	DPRINT("ggLoadConfig(\"%s\", %p)\n", filename, confptr);
	
	res = (struct _gg_config *) (*confptr);

	/* res != NULL mostly comes from re-instantiating libg* again
	 * w/o re-initializing the confhandles (= global variables)
	 */
	LIB_ASSERT(res == NULL, "res != NULL causes memory corruption later\n");

	if (res == NULL) {
		res = malloc(sizeof(*res));
		if (res == NULL) {
			DPRINT("- out of mem in ggLoadConfig.\n");
			return GGI_ENOMEM;
		}
		GG_SLIST_INIT(&res->entries);
		res->last = NULL;
		*confptr = res;
	}

	if (sscanf(filename, "array@%p", (void **)&array) == 1)
		return _doLoadFromArray(array, res, "", 0);

	if (sscanf(filename, "string@%p", (void **)&string) == 1)
		return _doLoadFromString(string, res, "", 0);

	ret = _doLoadFromFile(filename, res, "", 0);
	/* _dumpConfig(res); */
	return ret;
}


void ggFreeConfig(gg_config cfg)
{
	struct _gg_entry *entry;
	struct _gg_config *_cfg = (struct _gg_config *)cfg;
	
	DPRINT("ggFreeConfig(%p)\n", cfg);
	
	while ((entry = GG_SLIST_FIRST(&_cfg->entries))) {
		GG_SLIST_REMOVE_HEAD(&_cfg->entries, entries);
		switch (entry->type) {
		case GG_ENTRY_MODULE:
			FREE(entry->_.module.pattern);
			FREE(entry->_.module.location);
			FREE(entry->_.module.symbol);
			break;
		case GG_ENTRY_ALIAS:
			FREE(entry->_.alias.name);
			FREE(entry->_.alias.value);
			break;
		default:
			DPRINT("! unknown entry type %i.\n", entry->type);
			break;
		}
		FREE(entry);
	}
	FREE(_cfg);
}

static int _wildcardMatch(const char *pattern, const char *name)
{
	int la, lb, lp, ln;
	const char *c;

	if (pattern == NULL) {
		DPRINT("_wildcardMatch: invalid or unknown pattern\n");
		return 0;
	}

	for (lp = strlen(pattern), la = 0, c = pattern; *c; c++, la++)
		if (*c == '*') {
			if (memcmp(name, pattern, la))
				return 0;
			ln = strlen(name);
			lb = lp - la - 1;
			if (memcmp(name + ln - lb, pattern + la + 1, lb))
				return 0;
			return 1;
		}
	return !strcmp(name, pattern);
}

static int _location_next(struct gg_location_iter *iter)
{
	struct _gg_entry *entry;
		
	entry = iter->_state;
	
	while (entry) {
		if (entry->type == GG_ENTRY_MODULE) {
			if (_wildcardMatch(entry->_.module.pattern, iter->name)) {
				iter->location = entry->_.module.location;
				iter->symbol = entry->_.module.symbol;
				iter->_state = GG_SLIST_NEXT(entry, entries);
				return ITER_YIELD;
			}
		}
		entry = GG_SLIST_NEXT(entry, entries);
	}
	return ITER_DONE;
}

int ggConfigIterLocation(struct gg_location_iter *iter)
{
	struct _gg_config * cfg;
	
	DPRINT("ggConfigIterLocation(%p)\n", iter);
	
	cfg = (struct _gg_config *)(iter->config);
	GG_ITER_PREPARE(iter, _location_next, NULL);
	iter->location = NULL;
	iter->symbol = NULL;
	iter->_state = GG_SLIST_FIRST(&cfg->entries);
	return GGI_OK;
}

static char *_getAlias(struct _gg_config *cfg, const char *alias)
{
	struct _gg_entry *entry;
	
	GG_SLIST_FOREACH(entry, &cfg->entries, entries)
		if (entry->type == GG_ENTRY_ALIAS &&
		    !strcmp(alias, entry->_.alias.name))
			return entry->_.alias.value;
	
	return NULL;
}

/*
***********************************************************
Legacy API
***********************************************************
*/

const char *ggMatchConfig(const void *conf, const char *name,
			  const char *vers)
{
	struct gg_location_iter match;

	DPRINT("*** ggMatchConfig is deprecated.\n");
	match.name = name;
	match.config = conf;
	ggConfigIterLocation(&match);
	GG_ITER_FOREACH(&match)
	    break;		/* stop here */
	GG_ITER_DONE(&match);
	return match.location;
}

int ggConfigExpandAlias(const void *confhandle, const char *list_in,
			char *list_out, size_t outmax)
{
	struct gg_target_iter match;
	int count = 0;
	
	DPRINT("*** ggConfigExpandAlias is deprecated.\n");
	*list_out = '\0';
	match.config = confhandle;
	match.input = list_in;
	ggConfigIterTarget(&match);
	GG_ITER_FOREACH(&match) {
		if(count++)
			ggstrlcat(list_out,":",outmax);
		ggstrlcat(list_out,"(",outmax);
		ggstrlcat(list_out,match.target,outmax);
		if (match.options != "") {
			ggstrlcat(list_out,":",outmax);
			ggstrlcat(list_out,match.options,outmax);
		}
		ggstrlcat(list_out,")",outmax);
	}
	GG_ITER_DONE(&match);
	return 0;
}

/*
******************************************************************************
Target spec parsing
******************************************************************************
*/

#define MAXTGTLEN 1024
#define MAXALIASDEPTH 20
struct _nested_target_iter {
	struct gg_target_iter *main;
	struct _nested_target_iter *nested;
	const char *input;
	char target[MAXTGTLEN];
	char *options;
	const char *upopts;
	int depth;
};

static void _free_nested(struct _nested_target_iter *iter)
{
	if (iter->nested) {
		_free_nested(iter->nested);
		free(iter->nested);
		iter->nested = NULL;
	}
}

static int _iter_nested(struct _nested_target_iter *iter)
{

	char *alias;
start:
	/*
	 * First, exhaust nested iterators if any.
	 */
	if (iter->nested) {
		if (_iter_nested(iter->nested) == ITER_YIELD)
			return ITER_YIELD;
		/* this one is done */
		_free_nested(iter->nested);
		free(iter->nested);
		iter->nested = NULL;
	}

	/*
	 * 2) Parse next target
	 */
	iter->input = ggParseTarget(iter->input, iter->target, MAXTGTLEN);

	if (*iter->input == ':')	/* target separator */
		iter->input++;

	/* No target found, we're done. */
	if (iter->target[0] == '\0')
		return ITER_DONE;

	/*
	 * append all options specified upstream
	 */
	if (ggstrlcat(iter->target, ":", MAXTGTLEN) >= MAXTGTLEN) {
		DPRINT("! target buffer overflow\n");
		goto start;
	}
	if (ggstrlcat(iter->target, iter->upopts, MAXTGTLEN) >= MAXTGTLEN) {
		DPRINT("! target buffer overflow\n");
		goto start;
	}

	/* Find options specified in this entry. ':' always exists */
	iter->options = strchr(iter->target, ':');
	*iter->options++ = '\0';

	/* Now, if the target is an alias:
	   - resolve it (it is a target spec)
	   - nest a new target spec iteration where iter->options will be
	   appended to options found there */

	if ((alias = _getAlias(iter->main->config, iter->target)) != NULL) {

		DPRINT("- expanding alias to \"%s\".\n", alias);

		if (iter->depth == MAXALIASDEPTH) {
			DPRINT("! too many nested aliases.\n", alias);
			goto start;
		}

		iter->nested =
		    calloc(1, sizeof(struct _nested_target_iter));
		if (iter->nested == NULL) {
			DPRINT("! out of mem for expanding alias \"%s\"\n",
			       iter->target);
		} else {
			iter->nested->main = iter->main;
			iter->nested->upopts = iter->options;
			iter->nested->input = alias;
			iter->nested->depth = iter->depth + 1;
		}
		/* twisted, isn't it? */
		goto start;
	}

	/* Eliminate the last ':' in the options */
	if (strcmp(iter->options, "") != 0) {
		alias = strrchr(iter->options, ':');
		*alias = '\0';
	}


	iter->main->target = iter->target;
	iter->main->options = iter->options;
	DPRINT("- next match: target=\"%s\", options=\"%s\".\n",
	       iter->target, iter->options);
	return ITER_YIELD;
}

static int _target_next(struct gg_target_iter *iter)
{
	return _iter_nested((struct _nested_target_iter *) (iter->nested));
}

static void _target_done(struct gg_target_iter *iter)
{
	_free_nested((struct _nested_target_iter *) (iter->nested));
	free(iter->nested);
}


int ggConfigIterTarget(struct gg_target_iter *iter)
{
	struct _nested_target_iter *n;

	DPRINT("ggConfigIterTarget(%p)\n", iter);

	GG_ITER_PREPARE(iter, _target_next, _target_done);

	n = calloc(1, sizeof(struct _nested_target_iter));
	if (n == NULL) {
		DPRINT("! out of mem\n");
		return GGI_ENOMEM;
	}
	n->main = iter;
	n->input = iter->input;
	n->upopts = "";
	n->depth = 0;
	iter->nested = (void *) n;

	return GGI_OK;
}

/*
int ggOpenTarget(const char *targetspec, gg_config conf, const char *symbol,
		 ggfunc_target_open *func, void *arg, void* tgtargptr,
		 int flags)
{
	gg_scope scope;
	struct gg_target_iter   target;
	struct gg_location_iter location;
	int count, ret;
	void *hook;
	
	DPRINT("ggOpenTarget(target=\"%s\", conf=%p, symbol=\"%s\",...)\n",
	       targetspec, conf, symbol);
	
	count = 0;
	target.config = conf;
	location.config = conf;
	
	target.input  = targetspec;
	ggConfigIterTarget(&target);
	GG_ITER_FOREACH(&target) {
		DPRINT("- adding \"%s\", \"%s\", %p\n",
		       target.target, target.options, arg);
		
		location.name = target.target;
		ggConfigIterLocation(&location);
		GG_ITER_FOREACH(&location) {
			DPRINT("- trying location \"%s:%s\"\n",
			       location.location, location.symbol);
			if ((scope = ggGetScope(location.location)) == NULL) {
				DPRINT("! cannot open location \"%s\".\n",
				       location.location);
				continue;
			}
			if (location.symbol == NULL)
				location.symbol = symbol;
			hook = ggFromScope(scope, location.symbol);
			if (hook == NULL) {
				DPRINT("! symbol \"%s\" not found.\n",
				       location.symbol);
				ggDelScope(scope);
				continue;
			}
			if ((ret = func(arg, scope, hook,
					target.target,
					target.options,
					tgtargptr)) == GGI_OK) {
				count += 1;
				break;
			}
			DPRINT("! provided init callback failed with errcode %i.\n", ret);
			ggDelScope(scope);
		}
		GG_ITER_DONE(&location);
		DPRINT("- no more locations\n");
	}
	GG_ITER_DONE(&target);
	
	return count;
}
*/
