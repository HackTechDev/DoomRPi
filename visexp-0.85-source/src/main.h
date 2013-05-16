//------------------------------------------------------------------------
//  MAIN DEFINITIONS
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

#ifndef __VPOZOOM_MAIN_H__
#define __VPOZOOM_MAIN_H__


#define VISEXP_TITLE  "Visplane Explorer"

#define VISEXP_VERSION  "0.85"


#define CANVAS_MAX_W  (24 * 64)
#define CANVAS_MAX_H  (18 * 64)


#define MSG_BUF_LEN  2000


extern bool want_quit;

extern const char * current_file;
extern const char * current_map;

#define map_is_loaded  (current_file && current_map)


extern int limit_of_visplanes;
extern int limit_of_drawsegs;
extern int limit_of_openings;
extern int limit_of_solidsegs;


void Main_FatalError(const char *msg, ...);


bool Main_TryLoadWAD(const char *filename);
bool Main_TrySetMap (const char *map_name);


// test a spot using current settings.
// 
datum_t Main_TestSpot(int x, int y);


#endif  /* __VPOZOOM_MAIN_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
