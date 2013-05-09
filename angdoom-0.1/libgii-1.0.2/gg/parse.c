/* $Id: parse.c,v 1.14 2005/07/29 16:40:52 soyt Exp $
******************************************************************************

   LibGG - Parsing code

   Copyright (C) 1998  Andrew Apted     [andrew.apted@ggi-project.org]

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
#include <string.h>
#include <ctype.h>

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#include <ggi/gg.h>

#define MAX_LINELEN	2048
#define COMMENT_CHAR	'#'
#define QUOTE_CHARS     "'\""

#define SKIPWHITE(str)	{while (isspace((uint8_t)*(str)) && *(str) != '\0' \
				&& *(str) != COMMENT_CHAR) { (str)++; }}
#define SKIPNONWHITE(str) {while (!isspace((uint8_t)*(str)) && *(str) != '\0' \
				&& *(str) != COMMENT_CHAR) { (str)++; }}
#define SKIPTOEND(str)	{while (*(str) != '\0' && *(str) != '\n' \
				&& *(str) != COMMENT_CHAR) { (str)++; }}

int ggGetFileOpt(FILE *fp, const char **optnames, char **results, int ressize)
{
	char line[MAX_LINELEN];

	while (fgets(line, MAX_LINELEN, fp) != NULL) {
		int i;
		for (i = 0; optnames[i] != NULL; i++) {
			char *tmpptr, *endptr, *lineptr = line;
			size_t len = strlen(optnames[i]);

			SKIPWHITE(lineptr);
#ifdef HAVE_STRCASECMP
			if (strncasecmp(lineptr, optnames[i], len) != 0) {
#else
			if (strncmp(lineptr, optnames[i], len) != 0) {
#endif
				continue;
			}
			lineptr += len;
			if (! isspace((uint8_t)*lineptr)) continue;
			SKIPWHITE(lineptr);
			if (*lineptr == '\0' || *lineptr == COMMENT_CHAR) {
				continue;
			}

			endptr = tmpptr = lineptr;
			while (*tmpptr != '\0' && *tmpptr != COMMENT_CHAR) {
				SKIPNONWHITE(tmpptr);
				endptr = tmpptr;
				SKIPWHITE(tmpptr);
			}

			*endptr = '\0';
			if ((endptr - lineptr) + 1 > ressize) {
				return GGI_ENOMEM;
			}

			ggstrlcpy(results[i], lineptr, MAX_LINELEN);
			return i;
		}
	}

	return GGI_ENOTFOUND;
}


/*
******************************************************************************

   const char *
   ggParseTarget(const char *str, char *target, int max);
 
 	Parses a target descriptor out of a string, and returns a
 	pointer to after the last character of the target.  Handles
 	bracketized forms appropriately.  Returns NULL if an error
 	occurs.
 
 	For example:
 
               "display-multi:(display-kgi:/dev/head1):display-x\0"
                              ^                       ^
                              |__str                  |__result
 
               target: "display-kgi:/dev/head1"
	       
******************************************************************************
*/

const char *ggParseTarget(const char *str, char *target, int max)
{
	int bracketized = 0;
	int bracket_count = 0;

	while (*str != '\0' && isspace((uint8_t)*str)) {
		str++;
	}

	if (*str == '\0') {
		*target = '\0';
		return str;
	}

	if (*str == '(') {
		bracketized=1;
		bracket_count++;
		str++;
	}

	while (*str != '\0') {
		if (*str == '(') {
			bracket_count++;
		} else if (*str == ')') {
		
			if (bracket_count == 0) {
				fprintf(stderr, "libgg: Bad target "
					"descriptor : unexpected ')'\n");
				*target = '\0';
				return NULL;
			}

			bracket_count--;

			if (bracketized && (bracket_count == 0)) {
				str++;
				break;
			}
		}

		if (max <= 2) {
			fprintf(stderr, "libgg: target descriptor "
				"too long\n");
			*target = '\0';
			return NULL;
		}
		
		*target++ = *str++; max--;
	}

	*target = '\0';

	if (bracket_count > 0) {
		fprintf(stderr, "libgg: Bad target descriptor : "
			"missing ')'\n");
		return NULL;
	}

	return str;
}

/*
******************************************************************************

   char *
   ggParseOptions(const char *str, gg_option *optlist, int count);

 	Parses the target options out of the argument string.  Each
 	option begins with a '-' characters, and can be given a value
 	using the '=' characters.  Some examples "-nodb", "-parent=8".
 	Options are separated by ':' characters or whitespace.  
 
 	The recognizable options are stored in the array of gg_options:
 	`optlist'.  Matching is case sensitive, but options can be
 	abbreviated right down to a single letter.  Returns NULL if an
 	error occurred.
 	
	For example:
 
               "trueemu:-parent=8 -d=4:display-fbdev"
                        ^              ^
                        |__str         |__result
 	
 		optlist[0].name: "parent"     (in)
 		optlist[1].name: "dither"     (in)
 
 		optlist[0].result: "8"        (out)
 		optlist[1].result: "4"        (out)

	Unnamed/ordered options support:

	If in the optlist an option name is prefixed with a ":",
	it is an eligible unnamed/ordered option.  The options
	are first processed as described above, and if any of these
	unnamed/ordered options are found in named (-optname=optvalue) 
	form, the ":" is changed to a "-", and it is processed as a normal
	named option.

	After all named options have been processed, any leftover
	fields are assigned to any unnamed/ordered options which
	were not found in named form.  This is done in the order 
	that these options appear in the optlist, and the prepended
	':' is not changed in this case.  Options that were found in
	named form are skipped.  Thus, it is possible to figure out 
	if an option was found in named form, or was given a value 
	from the fields left over after the named options, by checking 
	if the first character of the option name has been changed 
	to '-', or if it is still ':'.

	For Example:

		"terminfo:-physz==240,180:-path=/dev/ttyS0:vt100"
			  ^					^
			  |__str				|__result

		... supposing that the option names are as follows:

		optlist[0].name: ":path"
		optlist[1].name: ":term"
		optlist[2].name: "physz"

		The option "physz" is a named option, and is 
		found normally.  The option ":path" is an unnamed/ordered
                option, but it is found in named form in the string 
		"-path=/dev/ttyS0", and so the first unnamed/ordered
		option belongs to the only remaining unnamed/ordered
		option ":term".  The option names after ggParseOptions
		returns are thus:

		optlist[0].name: "-path"
		optlist[1].name: ":term"
		optlist[2].name: "physz"

		...and the values that are retrieved for the options
		are, respectively:
 
		optlist[0].result: "/dev/ttyS0"
		optlist[1].result: "vt100"
		optlist[2].result: "=240,180"

******************************************************************************
*/

#define TERMINATOR(c)  (((c) == '\0') || isspace((uint8_t)c) || ((c) == ':'))

static inline const char *
ggParseOptionValue(const char *str, char *buf)
{
	int len = 0;
	int quote_c = 0;

	if ((*str != '\0') && (strchr(QUOTE_CHARS, *str) != NULL)) {
		/* get quote character */
		quote_c = *str++;
	}

	for (; *str != '\0'; str++) {

		if (quote_c) {
			if (*str == quote_c) {
				str++;
				break;
			}
		} else {
			if (TERMINATOR(*str)) {
				break;
			}
		}

		if ((*str == '\\') && (str[1] != '\0')) {
			/* handle escapes */
			str++;
		}

		if (len < GG_MAX_OPTION_RESULT-1) {
			buf[len++] = *str;
		}
	}

	buf[len] = '\0';
	return str;
}

const char *ggParseOptions(const char *str, gg_option *optlist, int count)
{
	char name[GG_MAX_OPTION_NAME];
	size_t len;

	gg_option *cur;
	int i;

	for (i = 0; i < count; i++) 
		if (optlist[i].name[0] == '-') optlist[i].name[0] = ':';

	for (;;) {

		while (*str != '\0' && isspace((uint8_t)*str)) 
			str++;

		if (*str != '-')
			break;

		/* parse option's name */

		len = 0;

		for (str++; ! TERMINATOR(*str) && (*str != '='); str++) {
			if (len < GG_MAX_OPTION_NAME-1) {
				name[len++] = *str;
			}
		}

		if (len == 0) {
			fprintf(stderr, "libgg: Bad target options : "
				"missing option name\n");
			return NULL;
		}

		name[len] = '\0';

		/* EXPERIMENTAL FEATURE: shows config */

		if (strcmp(name, "showconfig") == 0) {
			fprintf(stderr, "libgg: CONFIG has %d options%s\n",
				count, count ? ":" : ".");
			for (i=0; i < count; i++) {
				fprintf(stderr, "libgg: CONFIG option -%s "
					"= \"%s\".\n", optlist[i].name, 
					optlist[i].result);
			}
			return NULL;
		}
		
		/* find option */

		cur = NULL;

		for (i = 0; i < count; i++) {
			if (strncmp(optlist[i].name, name, len) == 0) {
				/* found it */
				cur = optlist + i;
			}
			else if (*(optlist[i].name) == ':' &&
				strncmp(optlist[i].name + 1, name, len) == 0) {
				/* found it */
				cur = optlist + i;
				*(optlist[i].name) = '-';
			}
		}

		if (cur == NULL) {
			fprintf(stderr, "libgg: Unknown target option "
				"'%s'\n", name);
			return NULL;
		}

		/* store value */

		if (*str != '=') {
			strcpy(cur->result, "y");
		} else {
			str = ggParseOptionValue(str+1, cur->result);
		}

		if (*str == ':')
			str++;
	}

	/* Now fill in any unfound eligible unnamed options from excess */
	for (i=0; i < count; i++) {
		if (*(optlist[i].name) == ':') {
			cur = optlist + i;
			str = ggParseOptionValue(str, cur->result);
			if (*str == ':')
				str++;
		}
	}

	return str;
}

