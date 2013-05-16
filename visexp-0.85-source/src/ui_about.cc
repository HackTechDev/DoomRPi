//------------------------------------------------------------------------
//  About Window
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


class UI_About : public Fl_Window
{
private:
	bool want_quit;

public:
	UI_About(const char *label = NULL);

	virtual ~UI_About()
	{
		// nothing needed
	}

	bool WantQuit() const
	{
		return want_quit;
	}

public:
	// FLTK virtual method for handling input events.
	int handle(int event)
	{
		if (event == FL_KEYDOWN || event == FL_SHORTCUT)
		{
			int key = Fl::event_key();

			if (key == FL_Escape)
			{
				want_quit = true;
				return 1;
			}

			// eat all other function keys
			if (FL_F+1 <= key && key <= FL_F+12)
				return 1;
		}

		return Fl_Window::handle(event);
	}

private:
	static void close_callback(Fl_Widget *w, void *data)
	{
		UI_About *that = (UI_About *)data;

		that->want_quit = true;
	}

	static const char *Logo;
	static const char *Text1;
	static const char *Text2;
	static const char *URL;
};


const char *UI_About::Logo = VISEXP_TITLE "\n v" VISEXP_VERSION;

const char *UI_About::Text1 =
	"Visplane Explorer is a tool for visualizing\n"
	"possible VPO errors in DOOM maps.";


const char *UI_About::Text2 =
	"Copyright (C) 2012-2013 Andrew Apted   \n"
	"Copyright (C) 2005-2008 Simon Howard   \n"
	"Copyright (C) 1993-1996 Id Software, Inc.\n"
	"\n"
	"This program is free software, and may be\n"
	"distributed and modified under the terms of\n"
	"the GNU General Public License\n"
	"\n"
	"There is ABSOLUTELY NO WARRANTY\n"
	"Use at your OWN RISK";


const char *UI_About::URL = "http://awwports.sf.net/vpo_zoom";


//
// Constructor
//
UI_About::UI_About(const char *label) :
    Fl_Window(366, 380, label),
    want_quit(false)
{
	callback(close_callback, this);

	// non-resizable
	size_range(w(), h(), w(), h());

	resizable(NULL);


	int cy = 12;

	// nice big logo text
	Fl_Box *box = new Fl_Box(0, cy, w(), 70, Logo);
	box->align(FL_ALIGN_INSIDE | FL_ALIGN_CENTER);
	box->labelcolor(fl_rgb_color(64, 160, 64));
	box->labelsize(24);

	cy += box->h() + 10;


	// the very informative text

	int pad = 24;

	box = new Fl_Box(FL_NO_BOX, pad, cy, w()-pad*2, 42, Text1);
	box->align(FL_ALIGN_INSIDE | FL_ALIGN_CENTER);
	box->labelfont(FL_HELVETICA_BOLD);

	cy += box->h();


	box = new Fl_Box(FL_NO_BOX, pad, cy, w()-pad*2, 186, Text2);
	box->align(FL_ALIGN_INSIDE | FL_ALIGN_CENTER);
	box->labelfont(FL_HELVETICA);

	cy += box->h();


	// finally add an "OK" button
	int bw = 64;
	int bh = 32;

	cy += (h() - cy - bh) /2;

	Fl_Color but_color = fl_rgb_color(128, 255, 128);

	Fl_Button *button = new Fl_Button((w()-10-bw)/2, cy, bw, bh, "OK!");
	button->color(but_color, but_color);
	button->callback(close_callback, this);

	end();
}


void DLG_AboutText()
{
	UI_About * dlg = new UI_About("About Me");

	dlg->set_modal();
	dlg->show();

	// run the GUI until the user closes
	while (! dlg->WantQuit())
	{
		Fl::wait();
	}

	// this deletes all the child widgets too...
	delete dlg;
}


//------------------------------------------------------------------------


class UI_CheatSheet : public Fl_Window
{
private:
	bool want_quit;

	static void close_callback(Fl_Widget *w, void *data);

public:
	UI_CheatSheet(const char *L);

	virtual ~ UI_CheatSheet()
	{ }

	bool WantQuit() const
	{
		return want_quit;
	}
};


void UI_CheatSheet::close_callback(Fl_Widget *w, void *data)
{
	UI_CheatSheet *that = (UI_CheatSheet *)data;

	that->want_quit = true;
}


UI_CheatSheet::UI_CheatSheet(const char *L) :
    Fl_Window(366, 415, L),
	want_quit(false)
{
//??	color(FL_LIGHT3);

	resizable(NULL);

	callback(close_callback, this);

	{ Fl_Box* o = new Fl_Box(10, 10, 345, 37, "Mouse Usage:");
	  o->labelfont(FL_HELVETICA_BOLD);
	  o->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
	}
	{ Fl_Box* o = new Fl_Box(36, 39, 320, 26, "RMB : hold down to scroll the map");
	  o->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
	}
	{ Fl_Box* o = new Fl_Box(36, 64, 320, 26, "wheel : zoom in and out ");
	  o->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
	}
	{ Fl_Box* o = new Fl_Box(10, 103, 345, 37, "Keyboard Usage:");
	  o->labelfont(FL_HELVETICA_BOLD);
	  o->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
	}
	{ Fl_Box* o = new Fl_Box(35, 133, 345, 26, "cursor keys : scroll the map");
	  o->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
	}
	{ Fl_Box* o = new Fl_Box(35, 158, 345, 26, "HOME : move to show the whole map");
	  o->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
	}
	{ Fl_Box* o = new Fl_Box(35, 183, 345, 26, "+ / - : zoom in and out");
	  o->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
	}
	{ Fl_Box* o = new Fl_Box(35, 219, 345, 26, "TAB : toggle bare map / analysis mode");
	  o->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
	}
	{ Fl_Box* o = new Fl_Box(35, 254, 345, 23, "V : switch to Visplane view");
	  o->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
	}
	{ Fl_Box* o = new Fl_Box(35, 277, 345, 23, "D : switch to Draw-segs view");
	  o->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
	}
	{ Fl_Box* o = new Fl_Box(35, 300, 345, 23, "S : switch to Solid-segs view");
	  o->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
	}
//--	{ Fl_Box* o = new Fl_Box(35, 322, 345, 23, "O : switch to Openings view");
//--	  o->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
//--	}
	{ Fl_Box* o = new Fl_Box(35, 323, 345, 23, "M : switch to Map view");
	  o->align(Fl_Align(FL_ALIGN_LEFT|FL_ALIGN_INSIDE));
	}

	Fl_Color but_color = fl_rgb_color(128, 255, 128);

	{ Fl_Button *ok_but = new Fl_Button(150, 370, 80, 32, "OK");
	  ok_but->color(but_color, but_color);
	  ok_but->callback(close_callback, this);
	}

	end();
}


void DLG_CheatSheet()
{
	UI_CheatSheet * dlg = new UI_CheatSheet("Cheat Sheet");

	dlg->set_modal();
	dlg->show();

	while (! dlg->WantQuit())
	{
		Fl::wait();
	}

	delete dlg;
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
