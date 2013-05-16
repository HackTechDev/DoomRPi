//------------------------------------------------------------------------
//  Open map dialog
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


bool ValidateMapName(const char *p)
{
#if 0 // lax

	int len = strlen(p);

	if (len == 0 || len > 8)
		return false;

	if (! isalpha(*p))
		return false;

	for ( ; *p ; p++)
		if (! (isalnum(*p) || *p == '_'))
			return false;
	return true;

#else  // strict

	if (toupper(p[0]) == 'E' && isdigit(p[1]) &&
	    toupper(p[2]) == 'M' && isdigit(p[3]) && ! p[4])
		return true;
	
	if (toupper(p[0]) == 'M' && toupper(p[1]) == 'A' &&
		toupper(p[2]) == 'P' && isdigit(p[3]) && isdigit(p[4]) && ! p[5])
		return true;

	return false;
#endif
}


UI_OpenMap::UI_OpenMap() :
	Fl_Double_Window(395, 420, "Open Map"),
	action(ACT_none)
{
	resizable(NULL);

	callback(close_callback, this);

	{
		Fl_Box* o = new Fl_Box(10, 10, 300, 20, "Current WAD file:");
		o->labelfont(FL_HELVETICA_BOLD);
		o->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	}

	pwad_name = new Fl_Output(25, 35, 265, 26);

	Fl_Button *load_but = new Fl_Button(305, 34, 70, 28, "Load");
	load_but->callback(load_callback, this);

	map_name = new Fl_Input(94, 72, 100, 26, "Map slot: ");
	map_name->labelfont(FL_HELVETICA_BOLD);
	map_name->when(FL_WHEN_CHANGED);
	map_name->callback(input_callback, this);

	{
		Fl_Box *o = new Fl_Box(10, 105, 180, 26, "Available maps:");
		o->labelfont(FL_HELVETICA_BOLD);
		o->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
	}


	// all the map buttons go into this group
	
	button_grp = new Fl_Group(0, 135, 395, 210, "\n\nNone Found");
	button_grp->align(FL_ALIGN_TOP | FL_ALIGN_INSIDE);
	button_grp->end();

	{
		Fl_Group* o = new Fl_Group(0, 360, 395, 60);
		o->box(FL_FLAT_BOX);
		o->color(WINDOW_BG, WINDOW_BG);

		ok_but = new Fl_Return_Button(280, 376, 85, 32, "OK");
		ok_but->labelfont(FL_HELVETICA_BOLD);
		ok_but->callback(ok_callback, this);

		Fl_Button * cancel = new Fl_Button(155, 376, 95, 32, "Cancel");
		cancel->callback(close_callback, this);

		o->end();
	}

	end();

	CheckMapName();
}


UI_OpenMap::~UI_OpenMap()
{
}


void UI_OpenMap::Run()
{
	if (current_file)
		SetPWAD(current_file);

	Populate();

	set_modal();

	show();

	while (action == ACT_none)
	{
		Fl::wait(0.2);
	}

	if (action == ACT_ACCEPT)
	{
		Main_TrySetMap(map_name->value());
	}
}


void UI_OpenMap::CheckMapName()
{
	bool was_valid = ok_but->active();

	bool  is_valid = current_file &&
	                 ValidateMapName(map_name->value());
					 // TODO: VPO_CheckLevelExists(map_name->value());

	if (was_valid == is_valid)
		return;

	if (is_valid)
	{
		ok_but->activate();
		map_name->textcolor(FL_FOREGROUND_COLOR);
	}
	else
	{
		ok_but->deactivate();
		map_name->textcolor(FL_RED);
	}

	map_name->redraw();
}


void UI_OpenMap::Populate()
{
	button_grp->label("\n\nNone Found");
	button_grp->clear();

	if (current_file)
		PopulateButtons();
}


void UI_OpenMap::PopulateButtons()
{
	int num_levels = 0;

	while (VPO_GetMapName(num_levels))
		num_levels++;

	if (num_levels == 0)
		return;

	button_grp->label("");

	// limit the number based on available space
	int max_rows = 8;
	int max_cols = 5;

	num_levels = MIN(num_levels, max_rows * max_cols);

	std::map<std::string, int> level_names;
	std::map<std::string, int>::iterator IT;

	for (int lev = 0 ; lev < num_levels ; lev++)
	{
		const char *name = VPO_GetMapName(lev);

		level_names[std::string(name)] = 1;
	}


	// determine how many rows and columns, and adjust layout

	int row = 0;
	int col = 0;

	if (num_levels <= 24) max_rows = 6;
	if (num_levels <=  9) max_rows = 3;
	if (num_levels <=  4) max_rows = 2;
	if (num_levels <=  2) max_rows = 1;

	max_cols = (num_levels + (max_rows - 1)) / max_rows;

	int cx_base = button_grp->x() + 30;
	int cy_base = button_grp->y() + 5;
	int but_W   = 56;

	if (max_cols <= 4) { cx_base += 20; but_W += 12; }
	if (max_cols <= 2) { cx_base += 40; but_W += 12; }
	if (max_cols <= 1) { cx_base += 40; but_W += 12; }


	// create them buttons!!

	Fl_Color but_col = fl_rgb_color(0xff, 0xee, 0x80);

	for (IT = level_names.begin() ; IT != level_names.end() ; IT++)
	{
		int cx = cx_base + col * (but_W + but_W / 5);
		int cy = cy_base + row * 25;

		Fl_Button * but = new Fl_Button(cx, cy, but_W, 20);
		but->copy_label(IT->first.c_str());
		but->labelsize(12);
		but->color(but_col);
		but->callback(button_callback, this);

		button_grp->add(but);

		row++;
		if (row >= max_rows)
		{
			row = 0;
			col++;
		}
	}

	redraw();
}


void UI_OpenMap::SetPWAD(const char *name)
{
	pwad_name->value(fl_filename_name(name));
}


void UI_OpenMap::close_callback(Fl_Widget *w, void *data)
{
	UI_OpenMap * that = (UI_OpenMap *)data;

	that->action = ACT_CANCEL;
}


void UI_OpenMap::ok_callback(Fl_Widget *w, void *data)
{
	UI_OpenMap * that = (UI_OpenMap *)data;

	// santify check
	if (current_file && ValidateMapName(that->map_name->value()))
		that->action = ACT_ACCEPT;
	else
		fl_beep();
}


void UI_OpenMap::button_callback(Fl_Widget *w, void *data)
{
	UI_OpenMap * that = (UI_OpenMap *)data;

	// sanity check
	if (! current_file)
		return;

	that->map_name->value(w->label());
	that->action = ACT_ACCEPT;
}


void UI_OpenMap::input_callback(Fl_Widget *w, void *data)
{
	UI_OpenMap * that = (UI_OpenMap *)data;

	that->CheckMapName();
}


void UI_OpenMap::load_callback(Fl_Widget *w, void *data)
{
	UI_OpenMap * that = (UI_OpenMap *)data;

	that->LoadFile();
	that->CheckMapName();
}


void UI_OpenMap::LoadFile()
{
	Fl_Native_File_Chooser chooser;

	chooser.title("Pick file to open");
	chooser.type(Fl_Native_File_Chooser::BROWSE_FILE);
	chooser.filter("Wads\t*.wad");

	//??  chooser.directory("xxx");

	// Show native chooser
	switch (chooser.show())
	{
		case -1:
			fl_alert("Unable to open the map:\n\n%s",
					 chooser.errmsg());
			return;

		case 1: // cancelled by user
			return;

		default:
			break;  // OK
	}


	if (! Main_TryLoadWAD(chooser.filename()))
	{
		pwad_name->value("");
		return;
	}

	SetPWAD(current_file);

	Populate();


#if 0
	// only one map?  may as well open it

	if (! VPO_GetMapName(1))
	{
		map_name->value(VPO_GetMapName(0));

		action = ACT_ACCEPT;
	}
#endif
}


void DLG_OpenMap()
{
	UI_OpenMap * dialog = new UI_OpenMap();

	dialog->Run();

	delete dialog;

	main_win->UpdateTitle();
}

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
