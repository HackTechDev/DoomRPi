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

#ifndef __VPOZOOM_UI_WINDOW_H__
#define __VPOZOOM_UI_WINDOW_H__


#define WINDOW_BG  fl_gray_ramp(3)

#define BUILD_BG   fl_gray_ramp(9)


class UI_MainWin : public Fl_Double_Window
{
public:
	// main child widgets

	Fl_Sys_Menu_Bar *menu;

	UI_Canvas  *canvas;
	UI_InfoBar *infobar;

	Fl_Box  *status1;
	Fl_Box  *status2;

private:
	Fl_Cursor cursor_shape;

public:
	UI_MainWin();
	virtual ~UI_MainWin();

public:
	// this is a wrapper around the FLTK cursor() method which
	// prevents the possibly expensive call when the shape hasn't
	// changed.
	void SetCursor(Fl_Cursor shape);

	void UpdateTitle();
	void UpdateProgress(float prog);
	void UpdateMemory(int bytes);
};


extern UI_MainWin * main_win;


#endif  /* __VPOZOOM_UI_WINDOW_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
