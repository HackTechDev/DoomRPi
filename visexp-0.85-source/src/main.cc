//------------------------------------------------------------------------
//  MAIN PROGRAM
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


bool want_quit;

const char * current_file;
const char * current_map;


bool new_color_scheme = true;

#define REFRESH_TIME  1.0  // seconds


static const int angles_to_check[8] =
{
	0, 90, 180, 270,   /* four main compass directions: E, N, W, S */

	45, 135, 225, 315  /* the other compass dirs: NE, NW, SW, SE */

	/* 22, 67, 112, 157, 202, 247, 292, 337 */
};


int limit_of_visplanes = 128;
int limit_of_drawsegs  = 256;
int limit_of_openings  = 320 * 64;
int limit_of_solidsegs = 32;


#define VIEWHEIGHT  41
#define MAXBOB       8


// test a spot using current settings.
// returns a packed datum_t value (or a special DATUM_XXX value),
// but never DATUM_UNSET.
datum_t Main_TestSpot(int x, int y)
{
	int height = VIEWHEIGHT + MAXBOB;

	int num_visplanes = 0;
	int num_drawsegs  = 0;
	int num_openings  = 0;
	int num_solidsegs = 0;

	int result = VPO_TestSpot(x, y, height, angles_to_check[0],
	                          &num_visplanes, &num_drawsegs,
							  &num_openings,  &num_solidsegs);

	if (result == RESULT_IN_VOID)
		return DATUM_VOID;

	// closed door (or low window) -- use black for this
	if (result == RESULT_BAD_Z)
		return DATUM_CLOSED;

	if (result == RESULT_OVERFLOW)
		return DATUM_OVERFLOW;

	for (int a = 1 ; a < 8 ; a++)
	{
		result = VPO_TestSpot(x, y, height, angles_to_check[a],
							  &num_visplanes, &num_drawsegs,
							  &num_openings,  &num_solidsegs);

		if (result == RESULT_OVERFLOW)
			return DATUM_OVERFLOW;
	}

	// TODO: if (game == hexen)
	//           num_visplanes = num_visplanes * 128 / 160;

	// clamp values and pack into a datum_t
	num_visplanes = MIN(250, 1 + num_visplanes);
	num_drawsegs  = MIN(250, 1 + (num_drawsegs + 1) / 2);
	num_openings  = MIN(250, 1 + (num_openings + 159) / 160);
	num_solidsegs = MIN(250, 1 + num_solidsegs * 4);

	datum_t d =  num_visplanes |
	            (num_drawsegs  << 8)  |
	            (num_openings  << 16) |
				(num_solidsegs << 24);

	return d;
}


void Main_Shutdown(bool error)
{
	if (main_win)
	{
		delete main_win;

		main_win = NULL;
	}

	current_file = NULL;
	current_map  = NULL;
}


void Main_FatalError(const char *msg, ...)
{
	static char buffer[MSG_BUF_LEN];

	va_list arg_pt;

	va_start(arg_pt, msg);
	vsnprintf(buffer, MSG_BUF_LEN-1, msg, arg_pt);
	va_end(arg_pt);

	buffer[MSG_BUF_LEN-2] = 0;

#if 0  // FIXME
	DLG_ShowError("%s", buffer);
#else
	fprintf(stderr, "ERROR: %s\n", buffer);
#endif

	Main_Shutdown(true);

	exit(9);
}


void Main_OpenWindow()
{
	Fl::visual(FL_RGB);

	if (new_color_scheme)
	{
		Fl::background(236, 232, 228);
		Fl::background2(255, 255, 255);
		Fl::foreground(0, 0, 0);

		Fl::scheme("plastic");
	}

#if 0  // debug
	int screen_w = Fl::w();
	int screen_h = Fl::h();

	fprintf(stderr, "Screen dimensions = %dx%d\n", screen_w, screen_h);
#endif

	fl_message_font(FL_HELVETICA /* _BOLD */, 18);

	// load icons for file chooser
#ifndef WIN32
	Fl_File_Icon::load_system_icons();
#endif


	M_CreatePalettes();


	// create the FLTK window

	main_win = new UI_MainWin();


	// show the window (pass some dummy arguments)
	{
		char *argv[2];

		argv[0] = strdup("visplane-explorer.exe");
		argv[1] = NULL;

		main_win->show(1 /* argc */, argv);
	}

    // kill the stupid bright background of the "plastic" scheme
	if (new_color_scheme)
    {
		delete Fl::scheme_bg_;
		Fl::scheme_bg_ = NULL;

		main_win->image(NULL);
    }

	main_win->UpdateMemory(0);
	main_win->UpdateProgress(0);

	// run GUI for half a sec
	Fl::wait(0.1); Fl::wait(0.1);
	Fl::wait(0.1); Fl::wait(0.1);
}


void Main_ProcessTiles()
{
	main_win->canvas->ProcessOneTile();
}


void Main_Refresh(void *)
{
	if (map_is_loaded)
	{
		main_win->canvas->Refresh();

		int bytes = main_win->canvas->calcTileMemory();

		main_win->UpdateMemory(bytes);

		float prog = main_win->canvas->calcProgress();

		main_win->UpdateProgress(prog);
	}

	Fl::repeat_timeout(REFRESH_TIME, Main_Refresh);
}


void Main_Loop()
{
	try
	{
		// run the GUI until the user quits
		while (! want_quit)
		{
			Main_ProcessTiles();

			Fl::wait(map_is_loaded ? 0 : 0.2);
		}
	}
#if 0  // FIXME
	catch (assert_fail_c err)
	{
		Main_FatalError("Sorry, an internal error occurred:\n%s", err.GetMessage());
	}
#endif
	catch (...)
	{
//!!!!! FIXME FIXME		Main_FatalError("An unknown problem occurred (UI code)");
	}
}


bool Main_TryLoadWAD(const char *filename)
{
	VPO_FreeWAD();

	current_file = NULL;
	current_map  = NULL;

	main_win->canvas->KillTiles();

	if (VPO_LoadWAD(filename) != 0)
	{
		fl_alert("%s", VPO_GetError());
		return false;
	}

	if (! VPO_GetMapName(0))
	{
		fl_alert("No maps found in this WAD.");
		return false;
	}

	// can now load a map
	current_file = strdup(filename);

	main_win->UpdateTitle();

	return true;
}


bool Main_TrySetMap(const char *map_name)
{
	VPO_CloseMap();

	current_map = NULL;

	main_win->canvas->KillTiles();

	if (VPO_OpenMap(map_name) != 0)
	{
		fl_alert("%s", VPO_GetError());
		return false;
	}

	current_map = strdup(map_name);

	main_win->UpdateTitle();

	main_win->canvas->GoHome();
	main_win->canvas->SetMode('m');

	return true;
}


static void ShowHelp()
{
	printf(
		"*\n"
		"**\n"
		"*** " VISEXP_TITLE " " VISEXP_VERSION " (C) 2009-2013 Andrew Apted\n"
		"**\n"
		"*\n"
	);

	printf(
		"This program is free software,  under the terms of the GNU General\n"
		"Public License (GPL), and comes with ABSOLUTELY NO WARRANTY.\n"
		"\n"
		"Home page: http://vis-explorer.sourceforge.net/\n"
		"\n"
/*		"\n"	*/
	);

	printf(
		"USAGE: visplane-explorer [options...] [FILE [MAP]]\n"
		"\n"

/*
		"Available options are:\n"
		"  -f  -file    <pwad>        Wad file to load\n"
		"  -w  -warp    <map>         Select level to view\n"
		"\n"
*/
	);
}


int main(int argc, char **argv)
{
	if (argc >= 2 &&
	    ( strcmp(argv[1], "/?") == 0 ||
	      strcmp(argv[1], "-h") == 0 ||
	      strcmp(argv[1], "-help") == 0 ||
	      strcmp(argv[1], "--help") == 0 ||
	      strcmp(argv[1], "-version") == 0 ||
	      strcmp(argv[1], "--version") == 0 ))
	{
		ShowHelp();
		return 0;
	}


	Main_OpenWindow();

	// first argument should be a wad filename (drag'n'drop)
	// second argument, if present, should be a map name

	// TODO: support -file and -warp too

	if (argc >= 2 && argv[1][0] != '-')
	{
		if (Main_TryLoadWAD(argv[1]))
		{
			// only one map?  may as well open it
			if (! VPO_GetMapName(1))
				Main_TrySetMap(VPO_GetMapName(0));

			else if (argc >= 3 && argv[2][0] != '-')
				Main_TrySetMap(argv[2]);
		}
	}

	Fl::add_timeout(REFRESH_TIME, Main_Refresh);

	if (! map_is_loaded)
		DLG_OpenMap();

	Main_Loop();

	return 0;
}


//--- editor settings ---
// vi:ts=4:sw=4:noexpandtab
