/* $Id: quad.h,v 1.10 2005/07/31 15:31:10 soyt Exp $
***************************************************************************

   LibGG - 64bit arithmetic

***************************************************************************
*/


/* This code has been imported to GGI from NetBSD-current 2003-12-29 */

/*-
 * Copyright (c) 1992, 1993
 *	The Regents of the University of California.  All rights reserved.
 *
 * This software was developed by the Computer Systems Engineering group
 * at Lawrence Berkeley Laboratory under DARPA contract BG 91-66 and
 * contributed to Berkeley.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *	@(#)quad.h	8.1 (Berkeley) 6/4/93
 */


/*
 * Quad arithmetic.
 *
 * This library makes the following assumptions:
 *
 *  - The type long long (aka uint64) exists.
 *
 *  - A quad variable is exactly twice as long as `int'.
 *
 *  - The machine's arithmetic is two's complement.
 *
 * This library can provide 128-bit arithmetic on a machine with 128-bit
 * quads and 64-bit ints, for instance, or 96-bit arithmetic on machines
 * with 48-bit ints.
 */


/* Get LLONG_MIN and LLONG_MAX defined
 * in <limits.h>
 */
#ifndef __STDC_VERSION__
#define __STDC_VERSION__	199901L
#endif

#include "config.h"
#include "plat.h"

#include <sys/types.h>
#include <limits.h>


#if defined(GG_HAVE_INT64)

#if !defined(_QUAD_HIGHWORD) && !defined(_QUAD_LOWWORD)
#if defined(GGI_LITTLE_ENDIAN)
#define _QUAD_HIGHWORD 1
#define _QUAD_LOWWORD 0
#endif
#if defined(GGI_BIG_ENDIAN)
#define _QUAD_HIGHWORD 0
#define _QUAD_LOWWORD 1
#endif
#endif


#ifndef QUAD_MIN
#define QUAD_MIN	LLONG_MIN
#endif
#ifndef QUAD_MAX
#define QUAD_MAX	LLONG_MAX
#endif
#ifndef UQUAD_MAX
#define UQUAD_MAX	ULLONG_MAX
#endif

/*
 * Depending on the desired operation, we view a `long long' (aka quad_t) in
 * one or more of the following formats.
 */
union uu {
	int64_t  q;		/* as a (signed) quad */
	uint64_t uq;		/* as an unsigned quad */
	int	 sl[2];		/* as two signed ints */
	u_int	 ul[2];		/* as two unsigned ints */
};

/*
 * Define high and low parts of a quad_t.
 */
#define	H		_QUAD_HIGHWORD
#define	L		_QUAD_LOWWORD

/*
 * Total number of bits in a quad_t and in the pieces that make it up.
 * These are used for shifting, and also below for halfword extraction
 * and assembly.
 */
#define	QUAD_BITS	(sizeof(quad_t) * CHAR_BIT)
#define	INT_BITS	(sizeof(int) * CHAR_BIT)
#define	HALF_BITS	(sizeof(int) * CHAR_BIT / 2)

/*
 * Extract high and low shortwords from longword, and move low shortword of
 * longword to upper half of long, i.e., produce the upper longword of
 * ((quad_t)(x) << (number_of_bits_in_int/2)).  (`x' must actually be u_int.)
 *
 * These are used in the multiply code, to split a longword into upper
 * and lower halves, and to reassemble a product as a quad_t, shifted left
 * (sizeof(int)*CHAR_BIT/2).
 */
#define	HHALF(x)	((u_int)(x) >> HALF_BITS)
#define	LHALF(x)	((u_int)(x) & (((int)1 << HALF_BITS) - 1))
#define	LHUP(x)		((u_int)(x) << HALF_BITS)

typedef unsigned int	qshift_t;

__BEGIN_DECLS
int64_t __adddi3 (int64_t, int64_t);
int64_t __anddi3 (int64_t, int64_t);
int64_t __ashldi3 (int64_t, qshift_t);
int64_t __ashrdi3 (int64_t, qshift_t);
int __cmpdi2 (int64_t, int64_t);
int64_t __divdi3 (int64_t, int64_t);
int64_t __fixdfdi (double);
int64_t __fixsfdi (float);
uint64_t __fixunsdfdi (double);
uint64_t __fixunssfdi (float);
double __floatdidf (int64_t);
float __floatdisf (int64_t);
double __floatunsdidf (uint64_t);
int64_t __iordi3 (int64_t, int64_t);
int64_t __lshldi3 (int64_t, qshift_t);
int64_t __lshrdi3 (int64_t, qshift_t);
int64_t __moddi3 (int64_t, int64_t);
int64_t __muldi3 (int64_t, int64_t);
int64_t __negdi2 (int64_t);
int64_t __one_cmpldi2 (int64_t);
uint64_t __qdivrem (uint64_t, uint64_t, uint64_t *);
int64_t __subdi3 (int64_t, int64_t);
int __ucmpdi2 (uint64_t, uint64_t);
uint64_t __udivdi3 (uint64_t, uint64_t);
uint64_t __umoddi3 (uint64_t, uint64_t);
int64_t __xordi3 (int64_t, int64_t);
__END_DECLS

#endif	/* GG_HAVE_INT64 */
