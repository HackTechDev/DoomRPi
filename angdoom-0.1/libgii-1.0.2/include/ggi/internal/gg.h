/* $Id: gg.h,v 1.13 2005/08/15 21:54:37 cegger Exp $
******************************************************************************

   LibGG - internal header file

   Copyright (C) 2003 Christoph Egger	[Christoph_Egger@gmx.de]

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

#ifndef _GGI_INTERNAL_GG_H
#define _GGI_INTERNAL_GG_H

#include <ggi/gg.h>


__BEGIN_DECLS

/*
******************************************************************************
 Options as per GG_OPTS environment variable
******************************************************************************
*/

extern gg_option _gg_optlist[];
#define _GG_OPT_SIGNUM       0
#define _GG_OPT_SCHEDTHREADS 1
#define _GG_OPT_SCHEDHZ      2
#define _GG_OPT_BANSWAR      3

/* Parsed result of -banswar flag */
extern gg_swartype swars_enabled;

/*
******************************************************************************
 Cleanup functions
******************************************************************************
*/

extern int _gg_signum_dead;       /* Reserved signal also used for scheduler */
void _gg_sigfunc_dead(int);       /* Unique handle installed when dying      */
void _gg_init_cleanups(void);
int _gg_do_graceful_cleanup(void);



/*
******************************************************************************
 Mutex locking
******************************************************************************
*/

int _ggInitLocks(void);

void _ggExitLocks(void);

void _gg_death_spiral(void);


/*
******************************************************************************
 Task scheduler
******************************************************************************
*/
int _ggTaskInit(void);
void _ggTaskExit(void);
typedef int (*_gg_task_fn)(void);

int _gg_task_tock(void);
int _gg_task_tick(void);
int _gg_task_tick_finish(void);
int _gg_task_driver_init(_gg_task_fn *start, _gg_task_fn *stop,
			 _gg_task_fn *xit, int rate);


/*
******************************************************************************
 Dynamic module and other scope look-up abstraction
******************************************************************************
*/

int _ggScopeInit(void);
int _ggScopeExit(void);

__END_DECLS

#endif /* _GGI_INTERNAL_GG_H */
