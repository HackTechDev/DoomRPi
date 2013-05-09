/* $Id: cpuid.c,v 1.14 2005/01/11 09:40:24 pekberg Exp $
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

#ifdef HAVE_NO_CPUID
#warning You must implement cpuid for this architecture to use SWAR.
#endif

#ifdef HAVE_NO_SWARTYPE_IMPLEMENTATION
/* State of CPU detection/selection is kept here. */
static int cpuid_tested = 0;
gg_swartype swars_detected = 0;
gg_swartype swars_enabled = GG_SWAR_ALL;


gg_swartype ggGetSwarType(void)
{
	/* By default we assume the C SWARs all are faster if the arch
	 * has the types. This may not be true everywhere, but those archs 
	 * can define their own implementation.
	 */

	/* 64bit CPUs can run in 32bit (emulation) mode and 64bit mode.
	 * When 64bit CPUs actually run in 64bit mode,
	 * sizeof(void *) == 8  
         */
	if (sizeof(void *) == 8) {
		/* Yeah, we are in 64bit mode */
		swars_detected |= GG_SWAR_64BITC;
	}

	return (swars_enabled &
		(swars_detected | GG_SWAR_NONE | GG_SWAR_32BITC));
}
#endif
