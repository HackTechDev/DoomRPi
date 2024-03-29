/* $Id: mansync.c,v 1.2 2004/10/11 19:12:03 cegger Exp $
******************************************************************************

   This is a regression-test for LibGGI display-tile - mansync usage.

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
#include <ggi/display/tile.h>
#include <ggi/ggi.h>


#define DISPLAYSTR	"display-tile:0,0,320,200,(display-memory)"

#include "../display.mansync/mansync.inc.c"
