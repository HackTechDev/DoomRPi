/* $Id: cpuid_ppc.c,v 1.8 2004/11/10 21:29:23 cegger Exp $
***************************************************************************

   LibGG - General runtime CPU-type detection code.

   Copyright (C) 2002  Brian S. Julin   [bri@tull.umassp.edu]

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

#include "cpuid.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_PARAM_H
#include <sys/param.h>
#endif
#ifdef HAVE_SYS_SYSCTL_H
#include <sys/sysctl.h>
#endif

/* State of CPU detection/selection is kept here. */
static int cpuid_tested = 0;
gg_swartype swars_detected = 0;
gg_swartype swars_enabled = GG_SWAR_ALL;


gg_swartype ggGetSwarType(void)
{
	if (!cpuid_tested) {

/* This Altivec detection has some major issues:
 * - CTL_HW only exists on BSD Unix
 * - HW_VECTORUNIT only exists on Darwin
 * - Motorola added two SIMD extensions:
 *        Altivec and SPE (Signal Processing Engine)
 *        HW_VECTORUNIT provides no way to distinguish
 *        between them.
 *        SPE is found in Motorola's E500 core. The
 *        first PowerPC to contain the E500 core is
 *        the MPC8540.
 * - sysctl(2) doesn't exist on System V (Solaris / AIX)
 */
#if defined(HAVE_SYS_SYSCTL_H) && defined(CTL_HW)
		int selectors[2] = { CTL_HW, };
		int error = 0;
		size_t length = 0;

#if defined(HW_VECTORUNIT)
		int hasVectorUnit = 0;
		selectors[1] = HW_VECTORUNIT;

		length = sizeof(hasVectorUnit);
		error = sysctl(selectors, 2, &hasVectorUnit,
				&length, NULL, 0);
		if (( 0 == error ) && (hasVectorUnit != 0)) {
			swars_detected |= GG_SWAR_ALTIVEC;
		}
#endif	/* HW_VECTORUNIT */

#endif	/* HAVE_SYS_SYSCTL_H && CTL_HW*/

		/* When 64bit CPUs actually run in 64bit mode,
		 * sizeof(void *) == 8
		 */
		if (sizeof(void *) == 8) {
			/* Yeah, we are on PPC64 in 64bit mode */
			swars_detected |= GG_SWAR_64BITC;
		}

		cpuid_tested = 1;
	}	/* if */

	return (swars_enabled &
		(swars_detected | GG_SWAR_32BITC | GG_SWAR_NONE));
}	/* ggGetSwarType */
