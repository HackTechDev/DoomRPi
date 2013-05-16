//------------------------------------------------------------------------
//  ALL HEADERS
//------------------------------------------------------------------------
//
//  Visplane Explorer
//
//  Copyright (C) 2012 Andrew Apted
//
//  This program is free software; you can redistribute it and/or
//  modify it under the terms of the GNU General Public License
//  as published by the Free Software Foundation; either version 2
//  of the License, or (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU General Public License for more details.
//
//------------------------------------------------------------------------

#ifndef __VPOZOOM_HEADERS_H__
#define __VPOZOOM_HEADERS_H__

/* OS specifics */
#ifdef WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#endif

/* C library */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>
#include <errno.h>
#include <time.h>

/* STL goodies */

#include <string>
#include <vector>
#include <list>
#include <map>

/* Our own system defs */

#include "sys_type.h"
#include "sys_macro.h"
#include "sys_endian.h"
//FIXME !!!! #include "sys_assert.h"

/* FLTK */
#include "hdr_fltk.h"

/* VPO-LIB */
#include "vpo_api.h"

/* Main */
#include "m_util.h"
#include "m_tiles.h"
#include "main.h"

/* UI Widgets */
#include "ui_about.h"
#include "ui_canvas.h"
#include "ui_infobar.h"
#include "ui_menubar.h"
#include "ui_open.h"
#include "ui_window.h"


#endif // __VPOZOOM_HEADERS_H__

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
