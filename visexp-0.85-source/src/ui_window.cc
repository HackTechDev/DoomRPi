//------------------------------------------------------------------------
//  Main Window
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

#if (FL_MAJOR_VERSION != 1 ||  \
     FL_MINOR_VERSION != 3 ||  \
     FL_PATCH_VERSION < 0)
#error "Require FLTK version 1.3.0 or later"
#endif


UI_MainWin *main_win;


#define PANEL_W  2

#define MENU_W   230
#define MENU_H   200

#define WINDOW_MIN_W  630
#define WINDOW_MIN_H  450

#define WINDOW_MAX_W  (CANVAS_MAX_W + PANEL_W)
#define WINDOW_MAX_H  (CANVAS_MAX_H)

#define BG_COLOR  fl_rgb_color(0x66, 0x5E, 0x55)

#define BARE_TITLE  VISEXP_TITLE " " VISEXP_VERSION


static void main_win_close_CB(Fl_Widget *w, void *data)
{
	want_quit = true;
}


//
// MainWin Constructor
//
UI_MainWin::UI_MainWin() :
    Fl_Double_Window(WINDOW_MIN_W, WINDOW_MIN_H, BARE_TITLE),
    cursor_shape(FL_CURSOR_DEFAULT)
{
	size_range(WINDOW_MIN_W, WINDOW_MIN_H,
	           WINDOW_MAX_W, WINDOW_MAX_H);

	callback((Fl_Callback *) main_win_close_CB);

	color(BG_COLOR, BG_COLOR);


	int cy = 0;
	int ey = h() - 28;


	menu = Menu_Create(0, 0, MENU_W, 28);


	int stat_w = (w() - MENU_W) / 2;

	status1 = new Fl_Box(FL_FLAT_BOX, w() - stat_w * 2 + 2, 0, stat_w - 2, 28, "");
	status1->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	status1->color(FL_DARK2);

	status2 = new Fl_Box(FL_FLAT_BOX, w() - stat_w + 2, 0, stat_w - 2, 28, "");
	status2->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	status2->color(FL_DARK2);

	cy += 28;


	canvas = new UI_Canvas(0, cy, w(), ey - cy);

	infobar = new UI_InfoBar(0, ey, w(), 28);


	end();

	resizable(canvas);
}


//
// MainWin Destructor
//
UI_MainWin::~UI_MainWin()
{ }


void UI_MainWin::SetCursor(Fl_Cursor shape)
{
	if (shape == cursor_shape)
		return;

	cursor_shape = shape;

	cursor(shape);
}


void UI_MainWin::UpdateTitle()
{
	if (! current_file)
	{
		label(BARE_TITLE);
		return;
	}

	static char map_buf[64];
	static char title_buf[FL_PATH_MAX];

	const char *filename = fl_filename_name(current_file);

	/* upper case map name */
	if (current_map)
	{
		const char *map = current_map;

		char * p = map_buf;

		*p++ = ' ';
		*p++ = '(';

		while (*map)
			*p++ = toupper(*map++);

		*p++ = ')';
		*p   = 0;
	}

	sprintf(title_buf, "%s%s - %s", filename, current_map ? map_buf : "", VISEXP_TITLE);

	label(title_buf);
}


void UI_MainWin::UpdateProgress(float prog)
{
	char buffer[200];

	if (! map_is_loaded)
		sprintf(buffer, " No Map Loaded");
	else
		sprintf(buffer, " Completed: %1.2f%%", prog);

	status1->copy_label(buffer);
}


void UI_MainWin::UpdateMemory(int bytes)
{
	bytes /= 1024;

	char buffer[200];

	sprintf(buffer, " Tile memory: %1.1f MB", bytes / 1024.0);

	status2->copy_label(buffer);
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
