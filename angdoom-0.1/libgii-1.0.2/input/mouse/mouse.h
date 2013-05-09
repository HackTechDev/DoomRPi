/* $Id: mouse.h,v 1.5 2005/07/31 15:31:12 soyt Exp $
******************************************************************************

   Mouse inputlib header

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

#include <ggi/internal/gii-dl.h>
#include <ggi/internal/gii_debug.h>

#define MAX_PACKET_BUF	128


/* Mouse init types */
#define GII_MIT_DONTCARE	0	/* Continue if init fails. */
#define GII_MIT_MUST		1	/* Fail if init fails. */
#define GII_MIT_FALLBACK	2	/* Fall back to fbparser. */

typedef struct parser_type {
	const char *names[8];			  /* Case insensitive names */
	int (*parser)(gii_input *, uint8_t *, int); /* Parser function */
	int	 min_packet_len;
	uint8_t	*init_data;
	int	 init_len;
	int	 init_type;	/* Kind of init (see GII_MIT_*) */
	struct parser_type *fbparser; /* Fallback parser type */
} parser_type;

typedef struct {
	int	(*parser)(gii_input *, uint8_t *, int); /* Parser function */
	int	min_packet_len;
	int	fd;
	int	eof;	/* Non-zero when end-of-file has occured */

	uint32_t button_state;
	uint32_t parse_state;

	int		packet_len;
	uint8_t        	packet_buf[MAX_PACKET_BUF];
	gii_event_mask	sent;
} mouse_priv;

#define MOUSE_PRIV(inp)  ((mouse_priv *) inp->priv)

extern parser_type *_gii_mouse_parsers[];

extern giifunc_eventpoll	GII_mouse_poll;
