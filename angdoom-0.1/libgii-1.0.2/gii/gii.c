/* $Id: gii.c,v 1.38 2005/09/03 18:16:24 soyt Exp $
******************************************************************************

   Graphics library for GGI. General Input Interface.

   Copyright (C) 1997 Jason McMullan	[jmcc@ggi-project.org]
   Copyright (C) 1998 Andreas Beck	[becka@ggi-project.org]
   Copyright (C) 1998 Jim Ursetto	[jim.ursetto@ggi-project.org]
   Copyright (C) 1998 Andrew Apted	[andrew.apted@ggi-project.org]
   Copyright (C) 1999 Marcus Sundberg	[marcus@ggi-project.org]

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
#include <ggi/internal/gii-dl.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <ctype.h>

#ifdef HAVE_STRINGS_H
#include <strings.h>
#endif

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif


#define GII_VERSIONCODE		0x00000001
#define GII_COMPATIBLE_MASK	0xffffff00


/* Static variables */
static uint32_t	_gii_origin_count = GII_MAXSUBLIBS;


/*
******************************************************************************
 Internal helpers
******************************************************************************
*/

/* Make event timestamps strictly monotonic.
 */
static inline void
_GII_ev_timestamp(gii_event *ev)
{
	static struct timeval last_event = { 0, 0 };

	if(_gii_threadsafe)
		ggLock(_gii_event_lock);

	ggCurTime(&(ev->any.time));
	if((ev->any.time.tv_sec > last_event.tv_sec)
	   || ((ev->any.time.tv_sec == last_event.tv_sec)
	       && (ev->any.time.tv_usec > last_event.tv_usec)))
	{
		last_event = ev->any.time;
	}
	else {
		if(++last_event.tv_usec >= 1000000) {
			last_event.tv_usec -= 1000000;
			++last_event.tv_sec;
		}
		ev->any.time = last_event;
	}

	if(_gii_threadsafe)
		ggUnlock(_gii_event_lock);
}


/*
******************************************************************************
 Internal standard handlers
******************************************************************************
*/

static int _GIIstdseteventmask(struct gii_input *inp, gii_event_mask evm)
{
	inp->curreventmask=( evm & inp->targetcan );
	return 0;
}

static gii_event_mask _GIIstdgeteventmask(struct gii_input *inp)
{
	return inp->curreventmask;
}

static int _GIIstdgetselectfd(struct gii_input *inp, fd_set *readfds)
{
	memcpy(readfds, &inp->fdset, sizeof(fd_set));
	return inp->maxfd;
}


/*
******************************************************************************
 EvQueue mechanisms
******************************************************************************
*/

/* Allocate an event queue
 */
static gii_ev_queue *
_giiEvQueueSetup(void)
{
	gii_ev_queue *qp;
	
	DPRINT2_CORE("_giiEvQueueSetup() called\n");
	
	qp = malloc(sizeof(gii_ev_queue));
	if (qp == NULL) return NULL;
	memset(qp, 0, sizeof(gii_ev_queue));
	
	DPRINT_CORE("_giiEvQueueSetup alloced %p\n", qp);
	
	return qp;
}

/* Destroy the queue for that input. Set pointer to NULL to catch late calls.
 */
static void _giiEvQueueDestroy(gii_input *inp)
{
	int queue;

	DPRINT_CORE("_giiEvQueueDestroy(%p) called\n", inp);
	
	if (inp->queue) {
		DPRINT_CORE("Destroying %p, %p\n",
			    inp->queue, inp->queue->queues);
		for (queue=0; queue < evLast; queue++) {
			DPRINT2_CORE(
				"_giiEvQueueDestroy going %d, %p\n",
				queue, inp->queue->queues[queue]);
			if (inp->queue->queues[queue]) {
				free(inp->queue->queues[queue]);
			}
		}
		if (inp->queue->mutex) ggLockDestroy(inp->queue->mutex);
		free(inp->queue);
		inp->queue = NULL;	/* Assure segfault ... */
	}
	if (inp->safequeue) {
		free(inp->safequeue);
		inp->safequeue = NULL;
	}

	DPRINT_CORE("_giiEvQueueDestroy done\n");
}

/* Allocate the queue for that input. Individual queues are created on demand.
 */
static int _giiEvQueueAllocate(gii_input *inp)
{
	gii_ev_queue_set *qset;
	int i;

	DPRINT_EVENTS("_giiEvQueueAllocate(%p) called\n", inp);

	qset = malloc(sizeof(gii_ev_queue_set));
	if (qset == NULL) return GGI_ENOMEM;

	qset->mutex = ggLockCreate();
	if (qset->mutex == NULL) {
		free(qset);
		return GGI_EUNKNOWN;
	}
	qset->seen = 0;
	for (i = 0; i < evLast; i++) {
		qset->queues[i] = NULL;
	}

	inp->queue = qset;

	DPRINT_EVENTS("Got queue_set: %p\n", inp->queue);

	return 0;
}


/* Add event to a queue
 */
static inline int 
_giiAddEvent(gii_ev_queue *qp, gii_event *ev)
{
	if (qp->head < qp->tail) {
		if ((unsigned)(qp->tail - qp->head - 1) < ev->size) {
			return GGI_EEVOVERFLOW;
		}	
	} else if (qp->head > qp->tail) {
		if ((qp->head + ev->size) > GII_Q_THRESHOLD) {
			/* Event crosses threshold, thus we need space
			   at start of buffer to put head (tail may be
			   at 0, but head == tail means an empty buffer).
			*/
			if (qp->tail == 0) {
				return GGI_EEVOVERFLOW;
			}
		}
	}

	/* Add the event, and mark that we've seen it.. */
	memcpy(qp->buf + qp->head, ev, ev->size);
	
	qp->count++;
	qp->head += ev->size;

	if (qp->head > GII_Q_THRESHOLD) {
		qp->head = 0;
	}

	return 0;
}


/* Peek at event at tail of queue
 * WARNING: The returned pointer may be misaligned.
 *   Use it in conjunction with memcpy(3) to ensure
 *   aligned memory access.
 *   Some platforms (i.e. NetBSD/sparc64) rely on this.
 */
static inline gii_event *
_giiPeekEvent(gii_ev_queue *qp)
{
	return (gii_event*) (qp->buf + qp->tail);
}


/* Delete event at tail of queue
 */
static inline void
_giiDeleteEvent(gii_ev_queue *qp)
{
	qp->count--;
	qp->tail += qp->buf[qp->tail];

	if (qp->tail > GII_Q_THRESHOLD) {
		qp->tail = 0;
	}
}	


/* Get event at tail of queue
 */
static inline void
_giiGetEvent(gii_ev_queue *qp, gii_event *ev)
{
	uint8_t size = qp->buf[qp->tail];

	/* Pull event out of queue.. */
	memcpy(ev, qp->buf + qp->tail, size);

	qp->count--;
	qp->tail += size;

	if (qp->tail > GII_Q_THRESHOLD) {
		qp->tail = 0;
	}
}


/* Release an event from the queue set. Select the earliest one.
   Returns event-size. 0 = fail.
*/
static int
_giiEvQueueRelease(gii_input *inp, gii_event *ev, gii_event_mask mask)
{
	gii_ev_queue *qp = NULL;
	gii_event_mask evm;
	struct timeval t_min;
	int queue;

	DPRINT_EVENTS("_giiEvQueueRelease(%p, %p, 0x%x) called\n",
		      inp, ev, mask);
	
	if (_gii_threadsafe) ggLock(inp->queue->mutex);

	evm = mask & inp->queue->seen;

	/* Got nothing.. */
	if (evm == 0) {
		if (_gii_threadsafe) ggUnlock(inp->queue->mutex);
		return 0;
	}

	/* Max timestamp.. */
	t_min.tv_sec= 0x7FFFFFFF;
	t_min.tv_usec=0x7FFFFFFF;

	/* Get the specified event out of the queue.  If the user asks
	 * for more than one event type, return the one that has been
	 * waiting the longest.
	 */
	for (queue=0; queue < evLast; queue++) {
		struct gii_ev_queue *qp_tmp;
		
		DPRINT2_EVENTS("queue = %p, queue->queues = %p, "
			       "queue->queues[queue] = %p\n",
			       inp->queue, inp->queue->queues,
			       inp->queue->queues[queue]);
		qp_tmp = inp->queue->queues[queue];
		if (qp_tmp && qp_tmp->count && (evm & (1 << queue))) {
			uint8_t *e_tmp = (uint8_t *)_giiPeekEvent(qp_tmp);
			struct timeval t_tmp;

			/* Assure aligned memory access. Some platforms
			 * (i.e. NetBSD/sparc64) rely on this.
			 */
			memcpy(&t_tmp,
				e_tmp + ((int)&(((gii_event*)0)->any.time)),
				sizeof(struct timeval));

			if (t_tmp.tv_sec < t_min.tv_sec || 
			    (t_tmp.tv_sec == t_min.tv_sec &&
                             t_tmp.tv_usec < t_min.tv_usec)) {
				DPRINT2_EVENTS("_giiEvQueueRelease: Plausible found.\n");
				qp = qp_tmp;
				t_min = t_tmp;
			}
		}
	}

	/* Shouldn't happen.. */
	LIB_ASSERT(qp != NULL, "_giiEvQueueRelease: Arrgghh!! Nothing plausible");
	if (qp == NULL) {
		if (_gii_threadsafe) ggUnlock(inp->queue->mutex);
		return 0;
	}

	_giiGetEvent(qp, ev);

	if (qp->count == 0) {
		inp->queue->seen &= ~(1 << ev->any.type);
	}

	if (_gii_threadsafe) ggUnlock(inp->queue->mutex);

	DPRINT_EVENTS("Retrieved event type %d, size %d.\n", 
		      ev->any.type, ev->size);
	
	return ev->size;
}

/* Set all queue entries to the queue. Used when joining inputs.
 */
static void _giiSetQueue(struct gii_input *inp, struct gii_input *set)
{
	struct gii_input *curr = inp;
	
	DPRINT_EVENTS("_giiSetQueue(%p, %p) called\n", inp, set);
	
	do {
		curr->queue     = set->queue;
		curr->safequeue = set->safequeue;
		curr = curr->next;
	} while (curr != inp);	/* looped through once. */
}


/*
******************************************************************************
 These are externally callable by GII modules
******************************************************************************
*/

/* Set up an event
 */
void _giiEventBlank(gii_event *ev, size_t size)
{
	memset(ev, 0, size);

	ev->any.error  = 0;
	ev->any.origin = GII_EV_ORIGIN_NONE;
	ev->any.target = GII_EV_TARGET_ALL;

	_GII_ev_timestamp(ev);
}



/* Default devinfo sender
 * Most input targets implement the devinfo sender in this way.
 */
int _giiStdSendDevInfo(gii_input *inp, gii_cmddata_getdevinfo *data)
{
	gii_event ev;
	gii_cmddata_getdevinfo *dinfo;

	int size = sizeof(gii_cmd_nodata_event) +
			sizeof(gii_cmddata_getdevinfo);

	DPRINT_EVENTS("_giiStdSendDevInfo(%p, %p\n)", inp, data);
	
	_giiEventBlank(&ev, size);

	ev.any.size	= size;
	ev.any.type	= evCommand;
	ev.any.origin	= inp->origin;
	ev.cmd.code	= GII_CMDCODE_GETDEVINFO;

	dinfo = (gii_cmddata_getdevinfo *) ev.cmd.data;
	*dinfo = *data;

	return _giiEvQueueAdd(inp, &ev);
}	/* _giiStdSendDevInfo */



/* Default valuator sender
 * Most input targets implement the valuator sender in this way.
 */
int _giiStdSendValEvent(gii_input *inp, gii_cmddata_getvalinfo *vi,
			int val, int maxval)
{
	gii_cmddata_getvalinfo *VI = NULL;
	gii_event ev;
	size_t size;

	DPRINT_EVENTS("_giiStdSendValEvent(%p, %p, %i, %i) called\n",
		      inp, VI, val, maxval);
	
	if (val >= maxval) return GGI_EARGINVAL;

	size = sizeof(gii_cmd_nodata_event) +
			sizeof(gii_cmddata_getvalinfo);

	_giiEventBlank(&ev, size);

	ev.any.size	= size;
	ev.any.type	= evCommand;
	ev.any.origin	= inp->origin;
	ev.cmd.code	= GII_CMDCODE_GETVALINFO;

	VI = (gii_cmddata_getvalinfo *) ev.cmd.data;

	*VI = vi[val];

	return _giiEvQueueAdd(inp, &ev);
}	/* _giiStdSendValEvent */



/* Add an event. Return 0 on either success or unrecognized event type,
   return GGI_ENOMEM if the queue can't be allocated, and GGI_EEVOVERFLOW if
   the queue is full.
*/
int _giiEvQueueAdd(gii_input *inp, gii_event *ev)
{
	gii_ev_queue *qp;
	struct gii_input *curr = inp;
	int ret;

	DPRINT_EVENTS("_giiEvQueueAdd(%p, %p) called\n", inp, ev);
	
	if (! curr) return GGI_EARGINVAL;

	/* Check if type is in range */
	if (ev->any.type >= evLast) {
		DPRINT_EVENTS("_giiEvQueueAdd: bad type: 0x%x\n",
			      ev->any.type );
		return 0;
	}

	/* Tell all the filters */
	do {
		if (curr->GIIhandler)
			if (curr->GIIhandler(curr, ev)) {
				/* Event eaten by filter */
				return 0;
			}
		curr = curr->next;
	} while (curr != inp);	/* looped through once. */

	if (_gii_threadsafe) ggLock(inp->queue->mutex);

	if (inp->queue->queues[ev->any.type]) {
		qp = inp->queue->queues[ev->any.type];
	} else {
		qp = _giiEvQueueSetup();
		if (qp == NULL) {
			if (_gii_threadsafe) ggUnlock(inp->queue->mutex);
			return GGI_ENOMEM;
		}
		inp->queue->queues[ev->any.type] = qp;
	}

	DPRINT_EVENTS("Adding event type %d, size %d at pos %d\n",
		      ev->any.type, ev->size, qp->count);
	
	ret = _giiAddEvent(qp, ev);
	if (ret != 0) {
		if (_gii_threadsafe) ggUnlock(inp->queue->mutex);
		return ret;
	}

	inp->queue->seen |= (1 << ev->any.type);

	if (_gii_threadsafe) {
		/* Notify any other threads which may be blocking on
		   this input */
		_giiAsyncNotify(inp);
		ggUnlock(inp->queue->mutex);
	}

	return 0;
}


int _giiSafeAdd(struct gii_input *inp, gii_event *ev)
{
	int ret = 0;
#ifdef HAVE_SIGPROCMASK
	sigset_t blockset, oldset;
	
	/* Signals? We don't need no stinking signals! */
	sigfillset(&blockset);
	sigprocmask(SIG_BLOCK, &blockset, &oldset);
#endif
	ggLock(_gii_safe_lock);

	if (inp->safequeue == NULL && 
	    (inp->safequeue = _giiEvQueueSetup()) == NULL) {
		ret = GGI_ENOMEM;
	} else {
		_giiAddEvent(inp->safequeue, ev);
		inp->cache->havesafe = 1;
		DPRINT_EVENTS("_giiSafeAdd added event type: 0x%x, size: %d"
			      " at: %p, %p\n",
			      ev->any.type, ev->size, inp->safequeue->head,
			      inp->safequeue->tail);
	}
	
	ggUnlock(_gii_safe_lock);
#ifdef HAVE_SIGPROCMASK
	sigprocmask(SIG_SETMASK, &oldset, NULL);
#endif
	return ret;
}


int _giiSafeMove(struct gii_input *toinp, struct gii_input *frominp)
{
	int ret = 0;
#ifdef HAVE_SIGPROCMASK
	sigset_t blockset, oldset;
	
	/* Signals? We don't need no stinking signals! */
	sigfillset(&blockset);
	sigprocmask(SIG_BLOCK, &blockset, &oldset);
#endif
	ggLock(_gii_safe_lock);
	
	DPRINT_EVENTS("_giiSafeMove moving %d events\n",
		      frominp->safequeue->count);
	while (frominp->safequeue->count) {
		ret = _giiEvQueueAdd(toinp, _giiPeekEvent(frominp->safequeue));
		if (ret != 0) {
			goto safemove_finish;
		}
		_giiDeleteEvent(frominp->safequeue);
		DPRINT_EVENTS("_giiSafeMove stored event\n");
	}
	frominp->cache->havesafe = 0;

  safemove_finish:
	ggUnlock(_gii_safe_lock);
#ifdef HAVE_SIGPROCMASK
	sigprocmask(SIG_SETMASK, &oldset, NULL);
#endif
	return ret;
}


/* Allocate memory for an input descriptor and initialize it.
   It also allocates an event queue.
 */
struct gii_input *_giiInputAlloc(void)
{
	struct gii_input *ret;

	/* Allocate memory for input descriptor.
	 */
	ret = malloc(sizeof(gii_input));
	if (ret == NULL) return ret;
	ret->cache = _giiCacheAlloc();
	if (ret->cache == NULL) {
		free(ret);
		return NULL;
	}
	if (_giiEvQueueAllocate(ret) != 0) {
		_giiCacheFree(ret->cache);
		free(ret);
		return NULL;
	}

	ret->version = GII_VERSIONCODE;
	if (_gii_threadsafe) {
		ret->mutex = ggLockCreate();
	} else {
		ret->mutex = NULL;
	}
	ret->next = ret->prev = ret;	/* Ring structure ... self<->self */
	ret->dlhand = NULL;
	GG_SLIST_INIT(&ret->devinfo);

	ret->origin = _gii_origin_count++;

	ret->maxfd = 0;
	FD_ZERO(&(ret->fdset));
	ret->curreventmask = 0;		/* The target should set those. */
	ret->targetcan     = 0;		/* If it doesn't, it is broken. */
	ret->flags         = 0;
	ret->safequeue = NULL;

	ret->GIIeventpoll = NULL;
	ret->GIIsendevent = NULL;
	ret->GIIsendevent = NULL;
	ret->GIIhandler   = NULL;
	ret->GIIseteventmask   = _GIIstdseteventmask;
	ret->GIIgeteventmask   = _GIIstdgeteventmask;
	ret->GIIgetselectfdset = _GIIstdgetselectfd;
	ret->GIIclose = NULL;
	
	return ret;
}

void _giiInputFree(struct gii_input *inp)
{
	if (inp->queue) _giiEvQueueDestroy(inp);
	if (inp->cache) {
		inp->cache->count--;
		if (inp->cache->count == 0) {
			_giiCacheFree(inp->cache);
		}
	}

	/* Free the associated device info structures */
	while(!GG_SLIST_EMPTY(&inp->devinfo)) {
		_giiUnregisterDevice(inp, GG_SLIST_FIRST(&inp->devinfo)->origin);
	}
	if (inp->mutex) 
		ggLockDestroy(inp->mutex);

	free(inp);
}


/* Tests all possible sources for events that match mask.
   It returns a mask of all newly acquired events.
 */
gii_event_mask
_giiPollall(struct gii_input *inp, gii_event_mask mask, void *arg)
{
	struct gii_input *curr = inp;
	gii_event_mask retmask = 0;
	
	DPRINT_EVENTS("_giiPollAll(%p, 0x%x, %p) called\n", inp, mask, arg);
	
	if (! curr || ! (curr->cache->eventmask & mask)) {
		return 0;
	}

	do {
		if ((curr->curreventmask & mask) && curr->GIIeventpoll) {
			/* This is expected to do the following:
			   1. queue all pending events.
			   2. return the or-ed mask of all queued events
			*/
			retmask |= (curr->GIIeventpoll(curr, arg) & mask);
		}
		curr = curr->next;

	} while (curr != inp);	/* looped through once. */

	return retmask;
}

/* Add a new device to an input source and
 * return its origin, or 0 on error.
 */
uint32_t _giiRegisterDevice(gii_input *inp,
			    gii_cmddata_getdevinfo  *dev,
			    gii_cmddata_getvalinfo  *val) {
	gii_deviceinfo * ret;

	if (!GG_SLIST_EMPTY(&inp->devinfo))
		if ((GG_SLIST_FIRST(&inp->devinfo)->origin & GII_SUBLIBMASK) == GII_SUBLIBMASK)
			/* 
			   Too many devices registered.
			   
			   FIXME: This can actually be incorrect,
			   since other devices may have been
			   unregistered, leaving space in the device
			   count for this one.  We should scan the
			   devices and try to insert the new one.
			   (devices *must* then be sorted).
			   
			   But hey, do we really expect to exhaust 256
			   devices this way?
			*/
			return 0;
	
	ret = calloc(1, sizeof(*ret));
	if (ret == NULL) return 0;
	
	ret->dev = dev;
	ret->val = val;
	
	if (GG_SLIST_EMPTY(&inp->devinfo)) {
		/* Device count start as 1, to keep origin designate
		   the whole input. */
		ret->origin = inp->origin + 1;
	} else {
		ret->origin = GG_SLIST_FIRST(&inp->devinfo)->origin + 1;
	}
	GG_SLIST_INSERT_HEAD(&inp->devinfo, ret, devlist);
	
	return ret->origin;
}


/*
  Remove a device given by its origin.
  
  return GGI_OK, or GGI_ENOTFOUND if no device match the origin.
  
 */
int _giiUnregisterDevice(gii_input *inp, uint32_t origin) {

	gii_deviceinfo *t, *curr;
	
	if (GG_SLIST_EMPTY(&inp->devinfo)) return GGI_ENOTFOUND;
	
	t = GG_SLIST_FIRST(&inp->devinfo);
	if (t->origin == origin) {
		GG_SLIST_REMOVE_HEAD(&inp->devinfo, devlist);
		free(t);
		return GGI_OK;
	}

	GG_SLIST_FOREACH(curr, &inp->devinfo, devlist) {
		t = GG_SLIST_NEXT(curr, devlist);
		if (t->origin == origin) {
			GG_SLIST_NEXT(curr, devlist) =
				GG_SLIST_NEXT(GG_SLIST_NEXT(curr, devlist),
						devlist);
			free(t);
			return GGI_OK;
		}
	}
	return GGI_ENOTFOUND;
}

/*
 * ** Opening/Closing/Joining of sources **
 */

struct gii_input *giiOpen(const char *input,...)
{
	struct gii_input *ret = NULL;
	struct gii_input *inp;
	struct gg_target_iter match;
	giifunc_inputinit *init;
	va_list	drivers;
	void *argptr;
	int err;
	
	
	if (input == NULL) {
		input = getenv("GII_INPUT");
		if (input == NULL)
			return NULL;
		argptr = NULL;
	} else {
		va_start(drivers,input);
		argptr = va_arg(drivers,void *);
		va_end(drivers);
	}
	
	match.input  = input;
	match.config = _giiconfhandle;
	ggConfigIterTarget(&match);
	GG_ITER_FOREACH(&match) {
		
		DPRINT_CORE("Allocating input structure\n");
		
		inp = _giiInputAlloc();
		if (inp == NULL) break;
		
		DPRINT_LIBS("giiOpen adding \"%s\", \"%s\", %p\n",
			    match.target, match.options, argptr);
		
		/* Start at next boundary.  This leaves space for up 
		 * to 256 subsystems ...  I don't think we will 
		 * wrap around - will we ? */
		_gii_origin_count += GII_MAXSUBLIBS; 
		_gii_origin_count &= GII_MAINMASK;
		_gii_origin_count &= ~GII_EV_ORIGIN_SENDEVENT;
		
		inp->dlhand = _giiLoadDL(match.target);
		DPRINT_LIBS("_giiLoadDL returned %p\n", inp->dlhand);
		
		if (inp->dlhand == NULL) {
			_giiInputFree(inp);
			continue;
		}
		
		init = (giifunc_inputinit *)inp->dlhand->init;
		err = init(inp, match.options, argptr, match.target);
		DPRINT_LIBS("%d=dlh->init(%p,\"%s\",%p,\"%s\") - %s\n",
			    err, inp, match.options, argptr, match.target, match.target);
		if (err) {
			_giiCloseDL(inp->dlhand);
			free(inp->dlhand);
			_giiInputFree(inp);
			continue;
		}
		inp->dlhand->identifier = ret;
		if (ret == NULL)
			ret = inp;
		else 
			ret = giiJoinInputs(ret, inp);
	}
	GG_ITER_DONE(&match);
	
	if (ret)
		_giiUpdateCache(ret);
	
	return ret;
}

int giiClose(struct gii_input *inp)
{
	struct gii_input *curr = inp;
	int rc = -1;

	DPRINT_LIBS("giiClose(%p) called\n", inp);

	if (! curr) return GGI_EARGINVAL;

	_giiEvQueueDestroy(inp);	/* This destroys _all_ queues ! */

	do {
		struct gii_input *prev;

		curr->queue = NULL;	/* For better error catching. */

		if (curr->GIIclose) {
			rc = curr->GIIclose(curr);
		}

		if (curr->dlhand) {
			_giiCloseDL(curr->dlhand);
			free(curr->dlhand);
		}
		prev = curr;
		curr = curr->next;
		_giiInputFree(prev);
	} while (curr != inp);	/* looped through once. */

	return rc;
}

/* Take two inputs and merge them together. For the program, this is as if
   inp2 has been giiClosed() and inp has taken over all of its properties.
 */
struct gii_input *giiJoinInputs(struct gii_input *inp, struct gii_input *inp2)
{
	struct gii_input *curr;
	struct timeval tv={ 0,0 };
	 
	DPRINT_EVENTS("giiJoinInputs(%p, %p) called\n", inp, inp2);

	if (inp == NULL) {
		if (inp2) _giiUpdateCache(inp2);
		return inp2;
	}
	if (inp2 == NULL) {
		if (inp) _giiUpdateCache(inp);
		return inp;
	}
	
	/* Catch weird case */
	if (inp == inp2) return inp;

	/* First propagate pending events from the second source to the
	   primary queue. */
	while (giiEventPoll(inp2, emAll, &tv)) {
		gii_event ev;
		DPRINT_CORE("Fetching event from %p\n", inp2);
		giiEventRead(inp2, &ev, emAll);
		DPRINT_CORE("Storing event in %p\n", inp);
		_giiEvQueueAdd(inp, &ev);        /* Post it */
	}
	if (inp2->safequeue && inp2->safequeue->count) {
		_giiSafeMove(inp, inp2);
	}
		
	_giiEvQueueDestroy(inp2);	/* Destroy inp2 queue */
	_giiSetQueue(inp2, inp);	/* Set inp queue instead */

	inp2->prev->next = inp ->next;
	inp ->next->prev = inp2->prev;
	inp ->next = inp2;
	inp2->prev = inp;

	/* Merge caches together */
	curr = inp->next;
	do {
		if (curr->cache != inp->cache) {
			curr->cache->count--;
			if (curr->cache->count == 0) {
				_giiCacheFree(curr->cache);
			}
			curr->cache = inp->cache;
			inp->cache->count++;
		}
		curr = curr->next;
	} while (curr != inp);
	_giiUpdateCache(inp);

	return inp;
}


int giiSplitInputs(struct gii_input *inp, struct gii_input **newhand,
		   uint32_t origin, uint32_t flags)
{
	struct gii_input *idx;
	gii_deviceinfo   *di;

	flags = 0; /* Silence, GCC!  Flags is reserved for later use 
		      when asking an input driver-lib to split off an 
		      individual origin. */

	if (inp == NULL) return GGI_EARGINVAL;

	/* Request is bogus - just one input in there. Abort.
	 */
	if (inp->next == inp) return GGI_ENOTFOUND;

	/* Make sure noone messes with the queue or the input while we
	 * dissect it.
	 */
	if (_gii_threadsafe) {
		ggLock(inp->mutex);
		ggLock(inp->queue->mutex);
	}

	idx = inp;
	if (origin == GII_EV_ORIGIN_NONE) goto haveit;
	do {
		/* walk the queue until the right origin is found. */

		if ( (idx->origin & GII_MAINMASK) != (origin & GII_MAINMASK) ) 
			goto trynextone;

		if (origin == idx->origin) 
			goto haveit;

		GG_SLIST_FOREACH(di, &idx->devinfo, devlist) {
			if (di->origin == origin)
				goto haveit;
		}

	trynextone:
		idx = idx->next;
		continue;

	haveit:
		if (inp == idx) {
			/* Ooops - we try to split off the _first_ input. This is a special case.
			 */

			/* Make a new queue for me. Old is 
			 * retained due to pointers in other inputs. 
			 */
			_giiEvQueueAllocate(inp); 
			inp->safequeue = NULL;
			if (inp->cache != NULL) {
				inp->cache->count--;
				if (inp->cache->count == 0) {
					_giiCacheFree(inp->cache);
					inp->cache = _giiCacheAlloc();
				}
				inp->cache->count++;
			}

			/* lock adjacent inputs as they will be changed now (relink) */
			if (_gii_threadsafe) {
				ggLock(inp->next->mutex);
				if (inp->next != inp->prev)
					ggLock(inp->prev->mutex);
			}
			inp->next->prev = inp->prev;
			inp->prev->next = inp->next;
			*newhand = inp->next;
			_giiUpdateCache(*newhand);
			/* unlock adjacent inputs again. */
			if (_gii_threadsafe) {
				if (inp->next != inp->prev)
					ggUnlock(inp->prev->mutex);
				ggUnlock(inp->next->mutex);
				/* Look closely:
				 * inp->queue has been overwritten during 
				 * _giiEvQueueAllocate(inp); However 
				 * inp->queue->mutex ist still locked.
				 * inp->next->queue has a copy of the old 
				 * contents of inp->queue, so we use that.
				 */
				ggUnlock(inp->next->queue->mutex);
			}
			/* selflink myself. */
			inp->next = inp;
			inp->prev = inp;
			_giiUpdateCache(inp);
			if (_gii_threadsafe) {
				ggUnlock(inp->mutex);
			}
			/* newhand may still be a joined input,
			 * so we inform the user of that. */
			return 1;
		}
		/* inp!=idx */
		if (_gii_threadsafe) {
			ggLock(idx->mutex); /* no check needed as idx!=inp by definition */
		}
		_giiEvQueueAllocate(idx);
		inp->safequeue = NULL;
		if (idx->cache != NULL) {
			idx->cache->count--;
			if (idx->cache->count == 0) 
				_giiCacheFree(idx->cache);
			idx->cache = _giiCacheAlloc();
			idx->cache->count++;
		}
		if (_gii_threadsafe) {
			if (idx->next != inp)
				ggLock(idx->next->mutex);
			if (idx->prev != inp)
				ggLock(idx->prev->mutex);
			/* the case idx->prev==idx->next cannot happen, 
			 * because that would have to be inp, then. 
			 * Otherwise the circular list would be 
			 * corrupted already anyway.
			 */
		}
		idx->prev->next = idx->next;
		idx->next->prev = idx->prev;
		if (_gii_threadsafe) {
			if (idx->prev != inp)
				ggUnlock(idx->prev->mutex);
			if (idx->next != inp)
				ggUnlock(idx->next->mutex);
		}
		idx->next = idx;
		idx->prev = idx;
		*newhand = idx;
		_giiUpdateCache(*newhand);
		_giiUpdateCache(inp);
		if (_gii_threadsafe) {
			ggUnlock(idx->mutex);
			ggUnlock(inp->queue->mutex);
			ggUnlock(inp->mutex);
		}
		return GGI_OK;

	} while (idx != inp);

	if (_gii_threadsafe) {
		ggUnlock(inp->queue->mutex);
		ggUnlock(inp->mutex);
	}

	return GGI_ENOTFOUND;
}

int giiEventsQueued(struct gii_input *inp, gii_event_mask mask)
{
	int count = 0;
	int i;
	
	if (_gii_threadsafe) ggLock(inp->queue->mutex);

	mask &= inp->queue->seen;
	for (i = 0; mask; i++, mask >>= 1) {
		if ((mask & 1)) {
			count += inp->queue->queues[i]->count;
		}
	}

	if (_gii_threadsafe) ggUnlock(inp->queue->mutex);
	
	return count;
}


int giiEventRead(struct gii_input *inp, gii_event *ev, gii_event_mask mask)
{
	if (! (mask & inp->queue->seen)) {
		/* Block until an event comes in */
		giiEventPoll(inp, mask, NULL);
	}
	return _giiEvQueueRelease(inp, ev, mask);
}


/*
 * ** Event Masks **
 */

int giiSetEventMask(struct gii_input *inp, gii_event_mask evm)
{
	struct gii_input *curr = inp;
	int rc = -1;
	int i;

	DPRINT_EVENTS("GIIseteventmask(%p, 0x%x) called\n", inp, evm);

	if (! curr) return GGI_EARGINVAL;

	/* Tell all the sources
	 */
	do {
		if (curr->GIIseteventmask)
			rc = curr->GIIseteventmask(curr, evm);
		curr = curr->next;

	} while (curr != inp);	/* looped through once. */

	/* Update the cache */
	_giiUpdateCache(inp);

	/* Flush any events whose bit is 0 in the new mask.
	 * Is that really desireable ? Andy
	 */
	if (_gii_threadsafe) ggLock(inp->queue->mutex);
	for (i=0; i < evLast; i++) {
		if (((evm & (1 << i)) == 0) && inp->queue->queues[i]) {
			inp->queue->queues[i]->head  = 0;
			inp->queue->queues[i]->tail  = 0;
			inp->queue->queues[i]->count = 0;
			inp->queue->seen&=~(1 << i);	/* Not seen ! */
		}
	}
	if (_gii_threadsafe) ggUnlock(inp->queue->mutex);

	return rc;
}

gii_event_mask giiGetEventMask(struct gii_input *inp)
{
	return inp->cache->eventmask;
}

int giiEventSend(struct gii_input *inp, gii_event *event)
{
	struct gii_input *curr = inp;
	
	APP_ASSERT(inp != NULL, "giiEventSend: inp is NULL");
	
	_GII_ev_timestamp(event);

	/* FIXME! We should allow this to be or-ed in */
	event->any.origin = GII_EV_ORIGIN_SENDEVENT;

	if (event->any.target == GII_EV_TARGET_QUEUE) {
		return _giiEvQueueAdd(inp, event);
	}

	do {
		/* Notify all clients that care */
		if (curr->GIIsendevent) {
			if (event->any.target == GII_EV_TARGET_ALL) {
				curr->GIIsendevent(curr, event);
			} else if ((event->any.target & GII_MAINMASK)
				   == (curr->origin & GII_MAINMASK)) {
				return curr->GIIsendevent(curr, event);
			}
		}
		curr = curr->next;
	} while(curr != inp);	/* looped through once. */

	if (event->any.target != GII_EV_TARGET_ALL) {
		return GGI_EEVNOTARGET;
	}

	return _giiEvQueueAdd(inp, event);
}

void giiPanic(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	vfprintf(stderr, format, ap);
	fflush(stderr);
	va_end(ap);

	while(giiExit() > 0);	/* kill all instances ! */
	exit(1);
}

static gii_deviceinfo *giiFindDeviceInfo(gii_input_t inp, uint32_t origin)
{
	struct gii_input *curr = inp;
	gii_deviceinfo   *di;

	do {
		if ( (curr->origin&GII_MAINMASK) == (origin&GII_MAINMASK) ) {

			GG_SLIST_FOREACH(di, &curr->devinfo, devlist) {
				if (di->origin==origin)
					return di;
			}
			break;	/* The queried device _must_ be in that queue. */
		}
		curr = curr->next;
	} while(curr != inp);	/* looped through once. */
	return NULL;
}

int giiQueryDeviceInfo(gii_input_t inp, uint32_t origin,
		       gii_cmddata_getdevinfo *info)
{
	gii_deviceinfo *di=giiFindDeviceInfo(inp,origin);
	if (di) {
		*info = *di->dev;
		return 0;
	}
	return GGI_ENOMATCH;
}

static gii_deviceinfo *
giiFindDeviceInfoByNumber(gii_input_t inp, uint32_t number, uint32_t *origin)
{
	struct gii_input *curr = inp;
	gii_deviceinfo   *di;

	do {
		GG_SLIST_FOREACH(di, &curr->devinfo, devlist) {
			if (number--==0) {
				if (origin) *origin=di->origin;
				return di;
			}
		}
		curr = curr->next;
	} while(curr != inp);	/* looped through once. */
	return NULL;
}

int giiQueryDeviceInfoByNumber(gii_input_t inp, uint32_t number, uint32_t *origin,
			       gii_cmddata_getdevinfo *info)
{
	gii_deviceinfo *di=giiFindDeviceInfoByNumber(inp,number,origin);
	if (di) {
		*info = *di->dev;
		return 0;
	}
	return GGI_ENOMATCH;
}

int giiQueryValInfo(gii_input_t inp, uint32_t origin, uint32_t valnumber,
		    gii_cmddata_getvalinfo *info)
{
	gii_deviceinfo *di=giiFindDeviceInfo(inp,origin);
	if (di) {
		if (valnumber >= di->dev->num_axes) return GGI_ENOSPACE;
		*info = di->val[valnumber];return 0;
	}
	return GGI_ENOMATCH;
}

