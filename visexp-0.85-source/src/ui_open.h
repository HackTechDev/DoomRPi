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

#ifndef __VISEXP_UI_OPEN_H__
#define __VISEXP_UI_OPEN_H__


class UI_OpenMap : public Fl_Double_Window
{
private:
	Fl_Output *pwad_name;
	Fl_Input  *map_name;

	Fl_Group *button_grp;

	Fl_Return_Button *ok_but;

	enum
	{
		ACT_none = 0,
		ACT_CANCEL,
		ACT_ACCEPT
	};

	int action;

	void Populate();
	void PopulateButtons();

	void LoadFile();
	void SetPWAD(const char *name);
	void CheckMapName();

public:
	UI_OpenMap();
	virtual ~UI_OpenMap();

	void Run();

private:
	static void     ok_callback(Fl_Widget *, void *);
	static void  close_callback(Fl_Widget *, void *);
	static void button_callback(Fl_Widget *, void *);
	static void  input_callback(Fl_Widget *, void *);
	static void   load_callback(Fl_Widget *, void *);
};

void DLG_OpenMap();

#endif  /* __VISEXP_UI_OPEN_H__ */

//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
