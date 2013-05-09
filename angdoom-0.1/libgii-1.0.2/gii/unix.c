/* $Id: unix.c,v 1.9 2005/01/21 11:22:04 pekberg Exp $
******************************************************************************

   LibGII core - Unix specific stuff.

   Copyright (C) 1998 Andreas Beck	[becka@ggi-project.org]
   Copyright (C) 1999 Marcus Suneberg	[marcus@ggi-project.org]

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

#include "config.h"

#include <ggi/gg.h>
#include <ggi/internal/gii.h>
#include <ggi/internal/gii_debug.h>
#include <ggi/gii-unix.h>

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif
#include <sys/types.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif


#define GII_POLLINTERVAL 10000	/* Poll in 10ms intervals */


#ifdef HAVE_PIPE
#define USE_ASYNCPIPES
#endif


/*
  Helper functions
*/

static inline int
sub_tv_diff(struct timeval *timeout, struct timeval *origtv)
{
	struct timeval newtv;

	ggCurTime(&newtv);
	timeout->tv_usec -=
		(newtv.tv_usec - origtv->tv_usec);
	timeout->tv_sec -=
		(newtv.tv_sec - origtv->tv_sec);
	if (timeout->tv_usec > 1000000) {
		timeout->tv_usec -= 1000000;
		timeout->tv_sec++;
	} else {
		if (timeout->tv_usec < 0) {
			timeout->tv_usec += 1000000;
			timeout->tv_sec--;
		}
		if (timeout->tv_sec < 0) {
			/* Time is up, set to zero */
			timeout->tv_sec = 0;
			timeout->tv_usec = 0;
			return 1;
		}
	}
	*origtv = newtv;
	
	return 0;
}


static inline int
gii_select(int n, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
	   struct timeval *timeout)
{
	struct timeval tmptv, *ptmptv, origtv;
	int selectret;
	int zero_timeout = 0;
	
	if (timeout) {
		if (timeout->tv_usec == 0 && timeout->tv_sec == 0) {
			zero_timeout = 1;
		} else {
			ggCurTime(&origtv);
		}
		/* We need to use a copy ... */
		tmptv = *timeout;
		ptmptv = &tmptv;
	} else {
		ptmptv = NULL;
	}
	selectret = select(n, readfds, writefds, exceptfds, ptmptv);

	if (timeout && !zero_timeout) {
		if (selectret != 0) {
			int err = errno;
			sub_tv_diff(timeout, &origtv);
			errno = err;
		} else {
			timeout->tv_sec = timeout->tv_usec = 0;
		}
	}

	return selectret;
}
	

static inline void
addfds_from_inp(gii_input *inp, fd_set *dest)
{
	int i;
	for (i = 0; i < inp->cache->maxfd; i++) {
		if (FD_ISSET(i, &inp->cache->fdset)) {
			FD_SET((unsigned)(i), dest);
		}
	}
}


/*
  giiEventPoll()
*/

gii_event_mask
giiEventPoll(gii_input *inp, gii_event_mask mask, struct timeval *timeout)
{
	struct timeval origtv;
	int	maxallfd;
	int	zero_timeout = 0;
	gii_event_mask	tmpmask;
	fd_set	allfds;
	
	DPRINT_EVENTS("giiEventPoll(%p, 0x%x, %p) called\n",
		      inp, mask, timeout);
	
	if (inp->cache->havesafe) {
		_giiSafeMove(inp, inp);
	}
	
	/* Do we have something already queued ? */
	tmpmask = (mask & inp->queue->seen);
	if (tmpmask) {
		return tmpmask;
	}

	if (timeout) {
		if (timeout->tv_usec == 0 && timeout->tv_sec == 0) {
			zero_timeout = 1;
		} else {
			ggCurTime(&origtv);
		}
	}

	/* Give the sources a try. */
	tmpmask = _giiPollall(inp, mask, NULL);
	if (tmpmask) {
		return tmpmask;
	}

	/* Catch common case, avoid underflow and select() */
	if (zero_timeout) return 0;

	maxallfd = inp->cache->maxfd;
	allfds   = inp->cache->fdset;
	
	if (! (inp->cache->flags & GII_FLAGS_HASPOLLED)) {
		/* No polled drivers - good! So we just select() */

		if (maxallfd <= 0) {
			/* Nothing here at all ... */
			return 0;
		}

		DPRINT_EVENTS("giiEventPoll: starting non-polled loop\n");
		while (1) {
			struct timeval tmptv, *ptmptv;
			int selectret;

			if (timeout) {
				/* We need to use a copy ... */
				tmptv = *timeout;
				ptmptv = &tmptv;
			} else {
				ptmptv = NULL;
			}
			selectret = select(maxallfd, &allfds, NULL, NULL,
					   ptmptv);
			if (selectret == 0) {
				/* Time is up */
				if (timeout) {
					timeout->tv_sec = timeout->tv_usec = 0;
				}
				return 0;
			} else if (selectret < 0) {
				/* Error */
				int time_up = 0;
				if (timeout) {
					time_up = sub_tv_diff(timeout,&origtv);
				}
				/* Check if we got something in the safequeue*/
				if (inp->cache->havesafe) {
					_giiSafeMove(inp, inp);
				}
				if ((inp->queue->seen & mask)) {
					return inp->queue->seen & mask;
				}
				if (time_up) return 0;
				continue;
			}

#ifdef USE_ASYNCPIPES
			if (FD_ISSET(inp->cache->asyncpipes[0], &allfds)) {
				/* Got async event */
				char dummy;
				read(inp->cache->asyncpipes[0], &dummy, 1);
				inp->cache->haveasync = 0;
				tmpmask = (inp->queue->seen & mask);
				if (tmpmask) return tmpmask;
			}
#endif

			/* Give the sources a try. */
			tmpmask = _giiPollall(inp, mask, &allfds);
			
			/* We check sub_tv_diff() before tmpmask because
			   timeout must be updated before return */
			if ((timeout && sub_tv_diff(timeout, &origtv)) ||
			    tmpmask) {
				return tmpmask;
			}

			/* No wanted events, so we keep on looping until we
			   time out.
			   fdset are most probably corrupted now, so we have
			   to restore the copy */
			allfds = inp->cache->fdset;
		}
	}

	/* We have polled drivers, so we have to split the select and
	   poll at regular intervals. */
	DPRINT_EVENTS("giiEventPoll: starting polled loop\n");
	while (1) {
		struct timeval tv;
		int selectret, real_timeout;

		if (timeout && timeout->tv_sec == 0 &&
		    timeout->tv_usec < GII_POLLINTERVAL) {
			tv = *timeout;
			real_timeout = 1;
		} else {
			tv.tv_sec  = 0;
			tv.tv_usec = GII_POLLINTERVAL;
			real_timeout = 0;
		}


		/* On some systems a select() with zero fds always blocks, so
		   we just take a short nap instead. */
		if (maxallfd <= 0) {
			ggUSleep(GII_POLLINTERVAL);
			selectret = 0;
		} else
			selectret = select(maxallfd, &allfds, NULL, NULL, &tv);
		
		if (real_timeout && selectret == 0) {
			/* Time is up */
			if (timeout) {
				timeout->tv_sec = timeout->tv_usec = 0;
			}
			return 0;
		}
		if (selectret < 0) {
			/* Error */
			int time_up = 0;
			if (timeout) {
				time_up = sub_tv_diff(timeout,&origtv);
			}
			/* Check if we got something in the safequeue*/
			if (inp->cache->havesafe) {
				_giiSafeMove(inp, inp);
			}
			if ((inp->queue->seen & mask)) {
				return inp->queue->seen & mask;
			}
			if (time_up) return 0;
			continue;
		}

#ifdef USE_ASYNCPIPES
		if (FD_ISSET(inp->cache->asyncpipes[0], &allfds)) {
			/* Got async event */
			char dummy;
			read(inp->cache->asyncpipes[0], &dummy, 1);
			inp->cache->haveasync = 0;
			tmpmask = (inp->queue->seen & mask);
			if (tmpmask) return tmpmask;
		}
#endif

		/* Time is up or we have something */
		tmpmask = _giiPollall(inp, mask,
				      (selectret > 0) ? &allfds : NULL);
		
		/* We check sub_tv_diff() before tmpmask because
		   timeout must be updated before return */
		if ((timeout && sub_tv_diff(timeout, &origtv)) ||
		    tmpmask) {
			return tmpmask;
		}
		
		/* No wanted events, so we keep on looping until we
		   time out.
		   fdset are most probably corrupted now, so we have
		   to restore the copy */
		allfds = inp->cache->fdset;
	}

	/* Never reached, but makes compilers happy */
	return 0;
}


/*
  giiEventSelect()
*/

int giiEventSelect(gii_input *inp, gii_event_mask *mask, int n,
		   fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
		   struct timeval *timeout)
{
	struct timeval origtv;
	int	maxallfd;
	int	zero_timeout = 0;
	fd_set	giifds;
	gii_event_mask	tmpmask;
	fd_set	allfds;
	fd_set	fdcopyall, fdcopyread, fdcopywrite, fdcopyexcept;

	if (mask == NULL || *mask == 0) {
		return gii_select(n, readfds, writefds, exceptfds, timeout);
	}

	if (inp->cache->havesafe) {
		_giiSafeMove(inp, inp);
	}

	/* Do we have something already queued ? */
	tmpmask = (*mask & inp->queue->seen);
	if (tmpmask) {
		*mask = tmpmask;
		/* Call select with zero timeout so fds are returned too */
		origtv.tv_sec = origtv.tv_usec = 0;
		return select(n, readfds, writefds, exceptfds, &origtv);
	}
	        
	if (timeout) {
		if (timeout->tv_usec == 0 && timeout->tv_sec == 0) {
			zero_timeout = 1;
		} else {
			ggCurTime(&origtv);
		}
	}

	/* Give the sources a try. */
	tmpmask = _giiPollall(inp, *mask, NULL);
	if (tmpmask) {
		*mask = tmpmask;
		/* Call select with zero timeout so fds are returned too */
		origtv.tv_sec = origtv.tv_usec = 0;
		return select(n, readfds, writefds, exceptfds, &origtv);
	}

	/* Catch common case, avoid underflow and select() */
	if (zero_timeout) {
		*mask = 0;
		return select(n, readfds, writefds, exceptfds, timeout);
	}

	giifds = inp->cache->fdset;

	if (readfds) {
		fdcopyread = *readfds;
		/* We set FDs here later from allfds */
		FD_ZERO(readfds);
	} else {
		FD_ZERO(&fdcopyread);
	}
	if (writefds) {
		fdcopywrite = *writefds;
	}
	if (exceptfds) {
		fdcopyexcept = *exceptfds;
	}

	allfds = fdcopyread;
 	addfds_from_inp(inp, &allfds);
	fdcopyall = allfds;

	if (inp->cache->maxfd > n) {
		maxallfd = inp->cache->maxfd;
	} else {
		maxallfd = n;
	}

	if (! (inp->cache->flags & GII_FLAGS_HASPOLLED)) {
		/* No polled drivers - good! So we just select() */

		if (maxallfd <= 0) {
			/* Nothing here at all ... */
			*mask = 0;
			return 0;
		}
		while (1) {
			struct timeval tmptv, *ptmptv;
			int selectret, poll_for_gii_events, matched_fds, i;

			if (timeout) {
				/* We need to use a copy ... */
				tmptv = *timeout;
				ptmptv = &tmptv;
			} else {
				ptmptv = NULL;
			}
			selectret = select(maxallfd, &allfds, writefds,
					   exceptfds, ptmptv);
			if (selectret == 0) {
				/* Time is up */
				*mask = 0;
				if (timeout) {
					timeout->tv_sec = timeout->tv_usec = 0;
				}
				return 0;
			} else if (selectret < 0) {
				/* Error */
				int time_up = 0;
				int err = errno;

				if (timeout) {
					time_up = sub_tv_diff(timeout,&origtv);
				}
				/* Check if we got something in the safequeue*/
				if (inp->cache->havesafe) {
					_giiSafeMove(inp, inp);
				}
				if ((inp->queue->seen & *mask)) {
					*mask = (inp->queue->seen & *mask);
					return 0;
				}
				*mask = 0;
				if (time_up) return 0;
				errno = err;
				return -1;
			}

			poll_for_gii_events = 0;
			matched_fds = selectret;
			for (i = 0; i < maxallfd && matched_fds; i++) {
				if (FD_ISSET(i, &allfds)) {
					if (FD_ISSET(i, &fdcopyread)) {
						/* fd was passed by app */
						FD_SET((unsigned)(i), readfds);
					} else {
						selectret--;
					}
					if (FD_ISSET(i, &giifds)) {
						/* fd belongs to gii source */
						poll_for_gii_events = 1;
					}
					/* Keeping track of this should improve
					   performance for the average
					   application... */
					matched_fds--;
				}
			}

#ifdef USE_ASYNCPIPES
			if (FD_ISSET(inp->cache->asyncpipes[0], &allfds)) {
				/* Got async event */
				char dummy;
				read(inp->cache->asyncpipes[0], &dummy, 1);
				inp->cache->haveasync = 0;
				tmpmask = (inp->queue->seen & *mask);
				if (tmpmask) {
					*mask = tmpmask;
					return selectret;
				}
			}
#endif
			
			if (poll_for_gii_events) {
				/* Give the sources a try. */
				tmpmask = _giiPollall(inp, *mask, &allfds);
			} else {
				tmpmask = 0;
			}
			
			if ((timeout && sub_tv_diff(timeout, &origtv)) ||
			    tmpmask || selectret) {
				*mask = tmpmask;
				return selectret;
			}

			/* None of the application specified fds matched, and
			   no wanted events, so we keep on looping until we
			   time out.
			   fdsets are most probably corrupted now, so we have
			   to restore the copies */
			allfds = fdcopyall;
			if (writefds) {
				*writefds = fdcopywrite;
			}
			if (exceptfds) {
				*exceptfds = fdcopyexcept;
			}
		}
	}

	/* We have polled drivers, so we have to split the select and
	   poll at regular intervals. */
	while (1) {
		struct timeval tv;
		int selectret, real_timeout, matched_fds, have_fds, i;

		if (timeout && timeout->tv_sec == 0 &&
		    timeout->tv_usec < GII_POLLINTERVAL) {
			tv = *timeout;
			real_timeout = 1;
		} else {
			tv.tv_sec  = 0;
			tv.tv_usec = GII_POLLINTERVAL;
			real_timeout = 0;
		}
		selectret = select(maxallfd, &allfds, writefds,
				   exceptfds, &tv);
		
		if (real_timeout && selectret == 0) {
			/* Time is up */
			*mask = 0;
			if (timeout) {
				timeout->tv_sec = timeout->tv_usec = 0;
			}
			return 0;
		}
		if (selectret < 0) {
			/* Error */
			int time_up = 0;
			int err = errno;

			if (timeout) {
				time_up = sub_tv_diff(timeout,&origtv);
			}
			/* Check if we got something in the safequeue*/
			if (inp->cache->havesafe) {
				_giiSafeMove(inp, inp);
			}
			if ((inp->queue->seen & *mask)) {
				*mask = (inp->queue->seen & *mask);
				return 0;
			}
			*mask = 0;
			if (time_up) return 0;
			errno = err;
			return -1;
		}

		/* Time is up or we have something */
		have_fds = matched_fds = selectret;
		for (i = 0; i < maxallfd && matched_fds; i++) {
			if (FD_ISSET(i, &allfds)) {
				if (FD_ISSET(i, &fdcopyread)) {
					/* fd was passed by app */
					FD_SET((unsigned)(i), readfds);
				} else {
					/* fd belongs only to a gii source */
					selectret--;
				}
				/* Keeping track of this should improve
				   performance for the average
				   application... */
				matched_fds--;
			}
		}
		
#ifdef USE_ASYNCPIPES
		if (FD_ISSET(inp->cache->asyncpipes[0], &allfds)) {
			/* Got async event */
			char dummy;
			read(inp->cache->asyncpipes[0], &dummy, 1);
			inp->cache->haveasync = 0;
			tmpmask = (inp->queue->seen & *mask);
			if (tmpmask) {
				*mask = tmpmask;
				return selectret;
			}
		}
#endif

		tmpmask = _giiPollall(inp, *mask, have_fds ? &allfds : NULL);
		
		if ((timeout && sub_tv_diff(timeout, &origtv)) ||
		    tmpmask || selectret) {
			*mask = tmpmask;
			return selectret;
		}
		
		/* None of the application-specified fds matched, and no
		   wanted events, so we keep on looping until we time out.
		   fdsets are most probably corrupted now, so we have
		   to restore the copies */
		allfds = fdcopyall;
		if (writefds) {
			*writefds = fdcopywrite;
		}
		if (exceptfds) {
			*exceptfds = fdcopyexcept;
		}
	}

	/* Never reached, but makes compilers happy */
	return 0;
}


gii_inputchain_cache *_giiCacheAlloc(void)
{
	gii_inputchain_cache *ret;

	ret = malloc(sizeof(gii_inputchain_cache));
	if (ret == NULL) return NULL;

#ifdef HAVE_PIPE
	if (pipe(ret->asyncpipes) != 0) {
		free(ret);
		return NULL;
	}
#endif

	ret->count = 1;		/* Usage count */
	ret->maxfd = 0;
	FD_ZERO(&ret->fdset);
	ret->eventmask = 0;
	ret->inputcan  = 0;
	ret->flags     = 0;
	ret->havesafe  = 0;
	ret->haveasync = 0;

	return ret;
}


void _giiCacheFree(gii_inputchain_cache *cache)
{
#ifdef HAVE_PIPE
	close(cache->asyncpipes[0]);
	close(cache->asyncpipes[1]);
#endif
	free(cache);
}	


void _giiAsyncNotify(gii_input *inp)
{
#ifdef USE_ASYNCPIPES
	char dummy;

	if (inp->cache->haveasync) {
		/* Only having one byte in the pipe will make things easier */
		return;
	}

	inp->cache->haveasync = 1;
	write(inp->cache->asyncpipes[1], &dummy, 1);
#endif
}


/* Update cache data for an input chain
 */
void _giiUpdateCache(gii_input *inp)
{
	struct gii_input *curr = inp;
	fd_set	hlpfd;
	int i;
	
	DPRINT_CORE("_giiUpdateCache(%p) called\n", inp);

	FD_ZERO(&inp->cache->fdset);
#ifdef USE_ASYNCPIPES
	FD_SET((unsigned)(inp->cache->asyncpipes[0]), &inp->cache->fdset);
	inp->cache->maxfd = inp->cache->asyncpipes[0] + 1;
#else
	inp->cache->maxfd = 0;
#endif
	inp->cache->eventmask = 0;
	inp->cache->inputcan  = 0;
	inp->cache->flags     = 0;
	inp->cache->havesafe  = 0;

	do {
		if (curr->GIIgetselectfdset) {
			int sourcemax = curr->GIIgetselectfdset(curr, &hlpfd);

			for (i = 0; i < sourcemax; i++) {
				if (FD_ISSET(i, &hlpfd)) {
					DPRINT_EVENTS("Found fd: %d \n", i);
					FD_SET((unsigned)(i), &inp->cache->fdset);
				}
			}
			if (sourcemax > inp->cache->maxfd) {
				inp->cache->maxfd = sourcemax;
			}
		}

		if (curr->GIIgeteventmask) {
			inp->cache->eventmask |= curr->GIIgeteventmask(curr);
		}

		inp->cache->inputcan |= curr->targetcan;
		inp->cache->flags |= curr->flags;

		if (curr->safequeue && curr->safequeue->count) {
			inp->cache->havesafe = 1;
		}

		curr = curr->next;
	} while (curr != inp);	/* looped through once. */
}
