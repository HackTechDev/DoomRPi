/* $Id: mansync.c,v 1.1 2004/10/10 12:18:48 cegger Exp $
******************************************************************************

   This is a regression-test for LibGGI display-trueemu - mansync usage.

   Written in 2004 by Christoph Egger

   This software is placed in the public domain and can be used freely
   for any purpose. It comes without any kind of warranty, either
   expressed or implied, including, but not limited to the implied
   warranties of merchantability or fitness for a particular purpose.
   Use it at your own risk. the author is not responsible for any damage
   or consequences raised by use or inability to use this program.

******************************************************************************
*/


#include "config.h"
#include <ggi/internal/internal.h>
#include <ggi/display/trueemu.h>
#include <ggi/ggi.h>


#define DISPLAYSTR	"display-trueemu"

#include "../display.mansync/mansync.inc.c"
