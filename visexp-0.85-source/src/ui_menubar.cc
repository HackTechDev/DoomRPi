//------------------------------------------------------------------------
//  Menu Bar
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

#include "headers.h"


//------------------------------------------------------------------------
//  FILE MENU
//------------------------------------------------------------------------

static void file_do_quit(Fl_Widget *w, void * data)
{
	want_quit = true;
}

static void file_do_open(Fl_Widget *w, void * data)
{
	DLG_OpenMap();

}

static void file_do_save(Fl_Widget *w, void * data)
{
	// TODO
}


//------------------------------------------------------------------------
//  VIEW MENU
//------------------------------------------------------------------------

static void view_do_heat_map(Fl_Widget *w, void * data)
{
	const Fl_Menu_Item *item = ((Fl_Menu_*)w)->mvalue();

	M_CreatePalettes(item->value() ? true : false);
}

static void view_do_seg_splits(Fl_Widget *w, void * data)
{
	// TODO
}


static void view_do_zoom_in(Fl_Widget *w, void * data)
{
	main_win->canvas->Zoom(+1);
}

static void view_do_zoom_out(Fl_Widget *w, void * data)
{
	main_win->canvas->Zoom(-1);
}

static void view_do_whole_map(Fl_Widget *w, void * data)
{
	main_win->canvas->GoHome();
}

static void view_do_goto_loc(Fl_Widget *w, void * data)
{
	// TODO
}


//------------------------------------------------------------------------
//  MODE MENU
//------------------------------------------------------------------------

static void mode_do_map(Fl_Widget *w, void * data)
{
	main_win->canvas->SetMode('m');
}

static void mode_do_visplanes(Fl_Widget *w, void * data)
{
	main_win->canvas->SetMode('v');
}

static void mode_do_drawsegs(Fl_Widget *w, void * data)
{
	main_win->canvas->SetMode('d');
}

static void mode_do_solidsegs(Fl_Widget *w, void * data)
{
	main_win->canvas->SetMode('s');
}


//------------------------------------------------------------------------
//  HELP MENU
//------------------------------------------------------------------------

void help_do_about(Fl_Widget *w, void * data)
{
	DLG_AboutText();
}


void help_do_cheatsheet(Fl_Widget *w, void * data)
{
	DLG_CheatSheet();
}


//------------------------------------------------------------------------

#undef  FCAL
#define FCAL  (Fl_Callback *)

static Fl_Menu_Item menu_items[] = 
{
	{ "&File", 0, 0, 0, FL_SUBMENU },

		{ "&Open Map",       FL_COMMAND + 'o', FCAL file_do_open },
		{ "&Save Image   ",  FL_COMMAND + 's', FCAL file_do_save },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "&Quit",      FL_COMMAND + 'q', FCAL file_do_quit },
		{ 0 },

	{ "&View", 0, 0, 0, FL_SUBMENU },

		{ "2D &Map",      0, FCAL mode_do_map },
		{ "&Visplanes",   0, FCAL mode_do_visplanes },
		{ "&Draw-segs",   0, FCAL mode_do_drawsegs },
		{ "&Solid-segs",  0, FCAL mode_do_solidsegs },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "&Heat colors",  0, FCAL view_do_heat_map,   0, FL_MENU_TOGGLE },
		{ "Seg s&plits",   0, FCAL view_do_seg_splits, 0, FL_MENU_TOGGLE },

		{ "", 0, 0, 0, FL_MENU_DIVIDER|FL_MENU_INACTIVE },

		{ "Zoom In",     0, FCAL view_do_zoom_in },
		{ "Zoom Out",    0, FCAL view_do_zoom_out },
		{ "&Whole Map",  0, FCAL view_do_whole_map },
//!! TODO	{ "&Go to Location...  ", 0, FCAL view_do_goto_loc },

		{ 0 },

	{ "&Help", 0, 0, 0, FL_SUBMENU },
		{ "&About...  ",           0,  FCAL help_do_about },
		{ "&Cheat Sheet   ",  FL_F+1,  FCAL help_do_cheatsheet },
		{ 0 },

	{ 0 }
};


#ifdef __APPLE__
Fl_Sys_Menu_Bar * Menu_Create(int x, int y, int w, int h)
{
	Fl_Sys_Menu_Bar *bar = new Fl_Sys_Menu_Bar(x, y, w, h);
	bar->menu(menu_items);
	return bar;
}
#else
Fl_Menu_Bar * Menu_Create(int x, int y, int w, int h)
{
	Fl_Menu_Bar *bar = new Fl_Menu_Bar(x, y, w, h);
	bar->menu(menu_items);
	return bar;
}
#endif


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
