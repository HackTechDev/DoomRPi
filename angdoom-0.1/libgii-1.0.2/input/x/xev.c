/* $Id: xev.c,v 1.6 2005/07/29 16:40:59 soyt Exp $
******************************************************************************

   X to GII event conversion - implementation

   Copyright (C) 1998	   Steve Cheng		[steve@ggi-project.org]
   Copyright (C) 1998-1999 Marcus Sundberg	[marcus@ggi-project.org]

   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included in
   all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
   THE AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
   IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
   CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

******************************************************************************
*/

#include "config.h"
#include "xev.h"
#include <X11/keysym.h>
#include <X11/Xutil.h>

#include <ctype.h>

uint32_t basic_trans(KeySym sym, int islabel)
{
	if (sym < 256) {  /* Latin1 - map through */
		if (islabel &&
		    ((sym >= 'a' && sym <= 'z') ||
		     (sym >= GIIUC_agrave && sym <= GIIUC_ydiaeresis &&
		      sym !=  GIIUC_Division))) {
			return (sym - 0x20);
		}
		return sym;
	}
		
	switch(sym) { 
	case XK_VoidSymbol : return GIIK_VOID;

	case XK_space      : return GIIUC_Space;
	case XK_BackSpace  : return GIIUC_BackSpace;
	case XK_Tab        : return GIIUC_Tab;
	case XK_Clear	   : return GIIK_Clear;
	case XK_Linefeed   : return GIIUC_Linefeed;
	case XK_Return     : return GIIUC_Return;
	case XK_Pause	   : return GIIK_Pause;
	case XK_Scroll_Lock: return GIIK_ScrollLock;
	case XK_Sys_Req    : return GIIK_SysRq;
	case XK_Escape     : return GIIUC_Escape;
	case XK_Delete     : return GIIUC_Delete;

		/* Japanese Language shifts not yet */

		/* Cursor control & motion */
	case XK_Home	: return GIIK_Home;
	case XK_Left    : return GIIK_Left;
	case XK_Up      : return GIIK_Up;
	case XK_Right   : return GIIK_Right;
	case XK_Down    : return GIIK_Down;
	case XK_Prior	: return GIIK_PageUp;
	case XK_Next	: return GIIK_PageDown;
	case XK_End	: return GIIK_End;
	case XK_Begin	: return GIIK_Begin;

	/* Misc Functions */
	case XK_Select	: return GIIK_Select;
	case XK_Print	: return GIIK_PrintScreen;
	case XK_Execute	: return GIIK_Execute;
	case XK_Insert	: return GIIK_Insert;
	case XK_Undo	: return GIIK_Undo;
	case XK_Redo	: return GIIK_Redo;
	case XK_Menu	: return GIIK_Menu;
	case XK_Cancel	: return GIIK_Cancel;
	case XK_Find	: return GIIK_Find;
	case XK_Help	: return GIIK_Help;
	case XK_Break	: return GIIK_Break;
	case XK_Num_Lock: return GIIK_NumLock;
	case XK_Multi_key   : return GIIK_Compose;

	case XK_Mode_switch :
		/* Is this true in all cases? */
		if (islabel) return GIIK_AltR;
		else return GIIK_AltGr;

	/* Keypad Functions */
	case XK_KP_Insert: 
		if (islabel) return GIIK_P0;
		else return GIIK_Insert;
	case XK_KP_End:
		if (islabel) return GIIK_P1;
		else return GIIK_End;
	case XK_KP_Down:
		if (islabel) return GIIK_P2;
		else return GIIK_Down;
	case XK_KP_Page_Down:
		if (islabel) return GIIK_P3;
		else return GIIK_PageDown;
	case XK_KP_Left:
		if (islabel) return GIIK_P4;
		else return GIIK_Left;
	case XK_KP_Begin:
		if (islabel) return GIIK_P5;
		else return GIIK_Begin;
	case XK_KP_Right:
		if (islabel) return GIIK_P6;
		else return GIIK_Right;
	case XK_KP_Home:
		if (islabel) return GIIK_P7;
		else return GIIK_Home;
	case XK_KP_Up:
		if (islabel) return GIIK_P8;
		else return GIIK_Up;
	case XK_KP_Page_Up :
		if (islabel) return GIIK_P9;
		else return GIIK_PageUp;
	case XK_KP_Decimal:
		if (islabel) return GIIK_PDecimal;
		else return '.';
	case XK_KP_Separator:
		if (islabel) return GIIK_PSeparator;
		else return ',';
	case XK_KP_Delete: /* Hmm... */
		if (islabel) return GIIK_PDecimal;
		else return GIIUC_Delete;

	case XK_KP_0	: return GIIK_P0;
	case XK_KP_1	: return GIIK_P1;
	case XK_KP_2	: return GIIK_P2;
	case XK_KP_3	: return GIIK_P3;
	case XK_KP_4	: return GIIK_P4;
	case XK_KP_5	: return GIIK_P5;
	case XK_KP_6	: return GIIK_P6;
	case XK_KP_7	: return GIIK_P7;
	case XK_KP_8	: return GIIK_P8;
	case XK_KP_9	: return GIIK_P9;

	case XK_KP_F1	 : return GIIK_PF1;
	case XK_KP_F2	 : return GIIK_PF2;
	case XK_KP_F3	 : return GIIK_PF3;
	case XK_KP_F4	 : return GIIK_PF4;

	case XK_KP_Space : return GIIK_PSpace;
	case XK_KP_Tab	 : return GIIK_PTab;
	case XK_KP_Enter : return GIIK_PEnter;

	case XK_KP_Equal : return GIIK_PEqual;
	case XK_KP_Multiply : return GIIK_PStar;
	case XK_KP_Add	    : return GIIK_PPlus;
	case XK_KP_Subtract : return GIIK_PMinus;
	case XK_KP_Divide: return GIIK_PSlash;

		/* function keys */
	case XK_F1	: return GIIK_F1;
	case XK_F2	: return GIIK_F2;
	case XK_F3	: return GIIK_F3;
	case XK_F4	: return GIIK_F4;
	case XK_F5	: return GIIK_F5;
	case XK_F6	: return GIIK_F6;
	case XK_F7	: return GIIK_F7;
	case XK_F8	: return GIIK_F8;
	case XK_F9	: return GIIK_F9;
	case XK_F10	: return GIIK_F10;
	case XK_F11	: return GIIK_F11;
	case XK_F12	: return GIIK_F12;
	case XK_F13	: return GIIK_F13;
	case XK_F14	: return GIIK_F14;
	case XK_F15	: return GIIK_F15;
	case XK_F16	: return GIIK_F16;
	case XK_F17	: return GIIK_F17;
	case XK_F18	: return GIIK_F18;
	case XK_F19	: return GIIK_F19;
	case XK_F20	: return GIIK_F20;
	case XK_F21	: return GIIK_F21;
	case XK_F22	: return GIIK_F22;
	case XK_F23	: return GIIK_F23;
	case XK_F24	: return GIIK_F24;
	case XK_F25	: return GIIK_F25;
	case XK_F26	: return GIIK_F26;
	case XK_F27	: return GIIK_F27;
	case XK_F28	: return GIIK_F28;
	case XK_F29	: return GIIK_F29;
	case XK_F30	: return GIIK_F30;
	case XK_F31	: return GIIK_F31;
	case XK_F32	: return GIIK_F32;
	case XK_F33	: return GIIK_F33;
	case XK_F34	: return GIIK_F34;
	case XK_F35	: return GIIK_F35;
		
		/* Modifiers */

	case XK_Shift_L   : return GIIK_ShiftL;
	case XK_Shift_R   : return GIIK_ShiftR;
	case XK_Control_L : return GIIK_CtrlL;
	case XK_Control_R : return GIIK_CtrlR;
	case XK_Caps_Lock : return GIIK_CapsLock;
	case XK_Shift_Lock: return GIIK_CapsLock;  /* FIXME (in ggi/keyboard.h)*/

	case XK_Meta_L : return GIIK_MetaL;
	case XK_Meta_R : return GIIK_MetaR;
	case XK_Alt_L  : return GIIK_AltL;
	case XK_Alt_R  : return GIIK_AltR;
	case XK_Super_L: return GIIK_SuperL;
	case XK_Super_R: return GIIK_SuperR;
	case XK_Hyper_L: return GIIK_HyperL;
	case XK_Hyper_R: return GIIK_HyperR;

#ifdef XK_XKB_KEYS
	/* Most of theISO 9995 Function and Modifier Keys missing */
	case XK_dead_grave:		return GIIK_DeadGrave;
	case XK_dead_acute:		return GIIK_DeadAcute;
	case XK_dead_circumflex:	return GIIK_DeadCircumflex;
	case XK_dead_tilde:		return GIIK_DeadTilde;
	case XK_dead_macron:		return GIIK_DeadMacron;
	case XK_dead_breve:		return GIIK_DeadBreve;
	case XK_dead_abovedot:		return GIIK_DeadAboveDot;
	case XK_dead_diaeresis:		return GIIK_DeadDiaeresis;
	case XK_dead_abovering:		return GIIK_DeadRing;
	case XK_dead_doubleacute:	return GIIK_DeadDoubleAcute;
	case XK_dead_caron:		return GIIK_DeadCaron;
	case XK_dead_cedilla:		return GIIK_DeadCedilla;
	case XK_dead_ogonek:		return GIIK_DeadOgonek;
	case XK_dead_iota:		return GIIK_DeadIota;
	case XK_dead_voiced_sound:	return GIIK_DeadVoicedSound;
	case XK_dead_semivoiced_sound:	return GIIK_DeadSemiVoicedSound;
#ifdef XK_dead_belowdot
	case XK_dead_belowdot:		return GIIK_DeadBelowDot;
#endif
#endif

	/* 3270 Terminal Keys missing */

#ifdef XK_LATIN2
	case XK_Aogonek		: return 0x0104;
	case XK_breve		: return 0x02D8;
	case XK_Lstroke		: return 0x0141;
	case XK_Lcaron		: return 0x013D;
	case XK_Sacute		: return 0x015A;
	case XK_Scaron		: return 0x0160;
	case XK_Scedilla	: return 0x015E;
	case XK_Tcaron		: return 0x0164;
	case XK_Zacute		: return 0x0179;
	case XK_Zcaron		: return 0x017D;
	case XK_Zabovedot	: return 0x017B;
	case XK_aogonek		: return 0x0105;
	case XK_ogonek		: return 0x02DB;
	case XK_lstroke		: return 0x0142;
	case XK_lcaron		: return 0x013E;
	case XK_sacute		: return 0x015B;
	case XK_caron		: return 0x02C7;
	case XK_scaron		: return 0x0161;
	case XK_scedilla	: return 0x015F;
	case XK_tcaron		: return 0x0165;
	case XK_zacute		: return 0x017A;
	case XK_doubleacute	: return 0x02DD;
	case XK_zcaron		: return 0x017E;
	case XK_zabovedot	: return 0x017C;
	case XK_Racute		: return 0x0154;
	case XK_Abreve		: return 0x0102;
	case XK_Lacute		: return 0x0139;
	case XK_Cacute		: return 0x0106;
	case XK_Ccaron		: return 0x010C;
	case XK_Eogonek		: return 0x0118;
	case XK_Ecaron		: return 0x011A;
	case XK_Dcaron		: return 0x010E;
	case XK_Dstroke		: return 0x0110;
	case XK_Nacute		: return 0x0143;
	case XK_Ncaron		: return 0x0147;
	case XK_Odoubleacute	: return 0x0150;
	case XK_Rcaron		: return 0x0158;
	case XK_Uring		: return 0x016E;
	case XK_Udoubleacute	: return 0x0170;
	case XK_Tcedilla	: return 0x0162;
	case XK_racute		: return 0x0155;
	case XK_abreve		: return 0x0103;
	case XK_lacute		: return 0x013A;
	case XK_cacute		: return 0x0107;
	case XK_ccaron		: return 0x010D;
	case XK_eogonek		: return 0x0119;
	case XK_ecaron		: return 0x011B;
	case XK_dcaron		: return 0x010F;
	case XK_dstroke		: return 0x0111;
	case XK_nacute		: return 0x0144;
	case XK_ncaron		: return 0x0148;
	case XK_odoubleacute	: return 0x0151;
	case XK_udoubleacute	: return 0x0171;
	case XK_rcaron		: return 0x0159;
	case XK_uring		: return 0x016F;
	case XK_tcedilla	: return 0x0163;
	case XK_abovedot	: return 0x02D9;
#endif /* XK_LATIN2 */

#ifdef XK_LATIN3
	case XK_Hstroke		: return 0x0126;
	case XK_Hcircumflex	: return 0x0124;
	case XK_Iabovedot	: return 0x0130;
	case XK_Gbreve		: return 0x011E;
	case XK_Jcircumflex	: return 0x0134;
	case XK_hstroke		: return 0x0127;
	case XK_hcircumflex	: return 0x0125;
	case XK_idotless	: return 0x0131;
	case XK_gbreve		: return 0x011F;
	case XK_jcircumflex	: return 0x0135;
	case XK_Cabovedot	: return 0x010A;
	case XK_Ccircumflex	: return 0x0108;
	case XK_Gabovedot	: return 0x0120;
	case XK_Gcircumflex	: return 0x011C;
	case XK_Ubreve		: return 0x016C;
	case XK_Scircumflex	: return 0x015C;
	case XK_cabovedot	: return 0x010B;
	case XK_ccircumflex	: return 0x0109;
	case XK_gabovedot	: return 0x0121;
	case XK_gcircumflex	: return 0x011D;
	case XK_ubreve		: return 0x016D;
	case XK_scircumflex	: return 0x015D;
#endif /* XK_LATIN3 */

#ifdef XK_LATIN4
	case XK_kra		: return 0x0138;
	case XK_Rcedilla	: return 0x0156;
	case XK_Itilde		: return 0x0128;
	case XK_Lcedilla	: return 0x013B;
	case XK_Emacron		: return 0x0112;
	case XK_Gcedilla	: return 0x0122;
	case XK_Tslash		: return 0x0166;
	case XK_rcedilla	: return 0x0157;
	case XK_itilde		: return 0x0129;
	case XK_lcedilla	: return 0x013C;
	case XK_emacron		: return 0x0113;
	case XK_gcedilla	: return 0x0123;
	case XK_tslash		: return 0x0167;
	case XK_ENG		: return 0x014A;
	case XK_eng		: return 0x014B;
	case XK_Amacron		: return 0x0100;
	case XK_Iogonek		: return 0x012E;
	case XK_Eabovedot	: return 0x0116;
	case XK_Imacron		: return 0x012A;
	case XK_Ncedilla	: return 0x0145;
	case XK_Omacron		: return 0x014C;
	case XK_Kcedilla	: return 0x0136;
	case XK_Uogonek		: return 0x0172;
	case XK_Utilde		: return 0x0168;
	case XK_Umacron		: return 0x016A;
	case XK_amacron		: return 0x0101;
	case XK_iogonek		: return 0x012F;
	case XK_eabovedot	: return 0x0117;
	case XK_imacron		: return 0x012B;
	case XK_ncedilla	: return 0x0146;
	case XK_omacron		: return 0x014D;
	case XK_kcedilla	: return 0x0137;
	case XK_uogonek		: return 0x0173;
	case XK_utilde		: return 0x0169;
	case XK_umacron		: return 0x016B;
#endif /* XK_LATIN4 */

#ifdef XK_KATAKANA
	case XK_overline		: break;
	case XK_kana_fullstop		: break;
	case XK_kana_openingbracket	: break;
	case XK_kana_closingbracket	: break;
	case XK_kana_comma		: break;
	case XK_kana_conjunctive	: return 0x30FB;
	case XK_kana_WO			: return 0x30F2;
	case XK_kana_a			: return 0x30A1;
	case XK_kana_i			: return 0x30A3;
	case XK_kana_u			: return 0x30A5;
	case XK_kana_e			: return 0x30A7;
	case XK_kana_o			: return 0x30A9;
	case XK_kana_ya			: return 0x30E3;
	case XK_kana_yu			: return 0x30E5;
	case XK_kana_yo			: return 0x30E7;
	case XK_kana_tsu		: return 0x30C3;
	case XK_prolongedsound		: return 0x30FC;
	case XK_kana_A			: return 0x30A2;
	case XK_kana_I			: return 0x30A4;
	case XK_kana_U			: return 0x30A6;
	case XK_kana_E			: return 0x30A8;
	case XK_kana_O			: return 0x30AA;
	case XK_kana_KA			: return 0x30AB;
	case XK_kana_KI			: return 0x30AD;
	case XK_kana_KU			: return 0x30AF;
	case XK_kana_KE			: return 0x30B1;
	case XK_kana_KO			: return 0x30B3;
	case XK_kana_SA			: return 0x30B5;
	case XK_kana_SHI		: break;
	case XK_kana_SU			: return 0x30B9;
	case XK_kana_SE			: return 0x30BB;
	case XK_kana_SO			: return 0x30BD;
	case XK_kana_TA			: return 0x30BF;
	case XK_kana_CHI		: return 0x30C1;
	case XK_kana_TSU		: return 0x30C4;
	case XK_kana_TE			: return 0x30C6;
	case XK_kana_TO			: return 0x30C8;
	case XK_kana_NA			: return 0x30CA;
	case XK_kana_NI			: return 0x30CB;
	case XK_kana_NU			: return 0x30CC;
	case XK_kana_NE			: return 0x30CD;
	case XK_kana_NO			: return 0x30CE;
	case XK_kana_HA			: return 0x30CF;
	case XK_kana_HI			: return 0x30D2;
	case XK_kana_FU			: return 0x30D5;
	case XK_kana_HE			: return 0x30D8;
	case XK_kana_HO			: return 0x30DB;
	case XK_kana_MA			: return 0x30DE;
	case XK_kana_MI			: return 0x30DF;
	case XK_kana_MU			: return 0x30E0;
	case XK_kana_ME			: return 0x30E1;
	case XK_kana_MO			: return 0x30E2;
	case XK_kana_YA			: return 0x30E4;
	case XK_kana_YU			: return 0x30E6;
	case XK_kana_YO			: return 0x30E8;
	case XK_kana_RA			: return 0x30E9;
	case XK_kana_RI			: return 0x30EA;
	case XK_kana_RU			: return 0x30EB;
	case XK_kana_RE			: return 0x30EC;
	case XK_kana_RO			: return 0x30ED;
	case XK_kana_WA			: return 0x30EF;
	case XK_kana_N			: return 0x30F3;
	case XK_voicedsound		: return 0x309B;
	case XK_semivoicedsound		: return 0x309C;
		/* case XK_kana_switch          0xFF7E  Alias for mode_switch */
#endif /* XK_KATAKANA */

#ifdef XK_ARABIC
	case XK_Arabic_comma		: return 0x060C;
	case XK_Arabic_semicolon	: return 0x061B;
	case XK_Arabic_question_mark	: return 0x061F;
	case XK_Arabic_hamza		: return 0x0621;
	case XK_Arabic_maddaonalef	: return 0x0622;
	case XK_Arabic_hamzaonalef	: return 0x0623;
	case XK_Arabic_hamzaonwaw	: return 0x0624;
	case XK_Arabic_hamzaunderalef	: return 0x0625;
	case XK_Arabic_hamzaonyeh	: return 0x0626;
	case XK_Arabic_alef		: return 0x0627;
	case XK_Arabic_beh		: return 0x0628;
	case XK_Arabic_tehmarbuta	: return 0x0629;
	case XK_Arabic_teh		: return 0x062a;
	case XK_Arabic_theh		: return 0x062b;
	case XK_Arabic_jeem		: return 0x062c;
	case XK_Arabic_hah		: return 0x062d;
	case XK_Arabic_khah		: return 0x062e;
	case XK_Arabic_dal		: return 0x062f;
	case XK_Arabic_thal		: return 0x0630;
	case XK_Arabic_ra		: return 0x0631;
	case XK_Arabic_zain		: return 0x0632;
	case XK_Arabic_seen		: return 0x0633;
	case XK_Arabic_sheen		: return 0x0634;
	case XK_Arabic_sad		: return 0x0635;
	case XK_Arabic_dad		: return 0x0636;
	case XK_Arabic_tah		: return 0x0637;
	case XK_Arabic_zah		: return 0x0638;
	case XK_Arabic_ain		: return 0x0639;
	case XK_Arabic_ghain		: return 0x063A;
	case XK_Arabic_tatweel		: return 0x0640;
	case XK_Arabic_feh		: return 0x0641;
	case XK_Arabic_qaf		: return 0x0642;
	case XK_Arabic_kaf		: return 0x0643;
	case XK_Arabic_lam		: return 0x0644;
	case XK_Arabic_meem		: return 0x0645;
	case XK_Arabic_noon		: return 0x0646;
	case XK_Arabic_ha		: return 0x0647;
	case XK_Arabic_waw		: return 0x0648;
	case XK_Arabic_alefmaksura	: return 0x0649;
	case XK_Arabic_yeh		: return 0x064A;
	case XK_Arabic_fathatan		: return 0x064B;
	case XK_Arabic_dammatan		: return 0x064C;
	case XK_Arabic_kasratan		: return 0x064D;
	case XK_Arabic_fatha		: return 0x064E;
	case XK_Arabic_damma		: return 0x064F;
	case XK_Arabic_kasra		: return 0x0650;
	case XK_Arabic_shadda		: return 0x0651;
	case XK_Arabic_sukun		: return 0x0652;
		/* case XK_Arabic_switch        0xFF7E  Alias for mode_switch */
#endif /* XK_ARABIC */

#ifdef XK_CYRILLIC
	case XK_Serbian_dje	: return 0x0452;
	case XK_Macedonia_gje	: return 0x0453;
	case XK_Cyrillic_io	: return 0x0451;
	case XK_Ukrainian_ie	: return 0x0458;
	case XK_Macedonia_dse	: return 0x0455;
	case XK_Ukrainian_i	: return 0x0456;
	case XK_Ukrainian_yi	: return 0x0457;
	case XK_Cyrillic_je	: return 0x0454;
	case XK_Cyrillic_lje	: return 0x0459;
	case XK_Cyrillic_nje	: return 0x045A;
	case XK_Serbian_tshe	: return 0x045B;
	case XK_Macedonia_kje	: return 0x045C;
	case XK_Byelorussian_shortu : return 0x045E;
	case XK_Cyrillic_dzhe	: return 0x045F;
	case XK_numerosign	: return 0x2116;
	case XK_Serbian_DJE	: return 0x0402;
	case XK_Macedonia_GJE	: return 0x0403;
	case XK_Cyrillic_IO	: return 0x042E;
	case XK_Ukrainian_IE	: return 0x0404;
	case XK_Macedonia_DSE	: return 0x0405;
	case XK_Ukrainian_I	: return 0x0406;
	case XK_Ukrainian_YI	: return 0x0407;
	case XK_Cyrillic_JE	: return 0x0408;
	case XK_Cyrillic_LJE	: return 0x0409;
	case XK_Cyrillic_NJE	: return 0x040A;
	case XK_Serbian_TSHE	: return 0x040B;
	case XK_Macedonia_KJE	: return 0x040C;
	case XK_Byelorussian_SHORTU	: return 0x040E;
	case XK_Cyrillic_DZHE	: return 0x040F;
	case XK_Cyrillic_yu	: return 0x044E;
	case XK_Cyrillic_a	: return 0x0430;
	case XK_Cyrillic_be	: return 0x0431;
	case XK_Cyrillic_tse	: return 0x0446;
	case XK_Cyrillic_de	: return 0x0434;
	case XK_Cyrillic_ie	: return 0x0435;
	case XK_Cyrillic_ef	: return 0x0444;
	case XK_Cyrillic_ghe	: return 0x0433;
	case XK_Cyrillic_ha	: return 0x0445;
	case XK_Cyrillic_i	: return 0x0438;
	case XK_Cyrillic_shorti	: return 0x0439;
	case XK_Cyrillic_ka	: return 0x043A;
	case XK_Cyrillic_el	: return 0x043B;
	case XK_Cyrillic_em	: return 0x043C;
	case XK_Cyrillic_en	: return 0x043D;
	case XK_Cyrillic_o	: return 0x043E;
	case XK_Cyrillic_pe	: return 0x043F;
	case XK_Cyrillic_ya	: return 0x044F;
	case XK_Cyrillic_er	: return 0x0440;
	case XK_Cyrillic_es	: return 0x0441;
	case XK_Cyrillic_te	: return 0x0442;
	case XK_Cyrillic_u	: return 0x0443;
	case XK_Cyrillic_zhe	: return 0x0436;
	case XK_Cyrillic_ve	: return 0x0432;
	case XK_Cyrillic_softsign	: return 0x044C;
	case XK_Cyrillic_yeru	: return 0x044B;
	case XK_Cyrillic_ze	: return 0x0437;
	case XK_Cyrillic_sha	: return 0x0448;
	case XK_Cyrillic_e	: return 0x044D;
	case XK_Cyrillic_shcha	: return 0x0449;
	case XK_Cyrillic_che	: return 0x0447;
	case XK_Cyrillic_hardsign	: return 0x044A;

	case XK_Cyrillic_YU	: return 0x0401;
	case XK_Cyrillic_A	: return 0x0410;
	case XK_Cyrillic_BE	: return 0x0411;
	case XK_Cyrillic_TSE	: return 0x0426;
	case XK_Cyrillic_DE	: return 0x0414;
	case XK_Cyrillic_IE	: return 0x0415;
	case XK_Cyrillic_EF	: return 0x0424;
	case XK_Cyrillic_GHE	: return 0x0413;
	case XK_Cyrillic_HA	: return 0x0425;
	case XK_Cyrillic_I	: return 0x0418;
	case XK_Cyrillic_SHORTI	: return 0x0419;
	case XK_Cyrillic_KA	: return 0x041A;
	case XK_Cyrillic_EL	: return 0x041B;
	case XK_Cyrillic_EM	: return 0x041C;
	case XK_Cyrillic_EN	: return 0x041D;
	case XK_Cyrillic_O	: return 0x041E;
	case XK_Cyrillic_PE	: return 0x041F;
	case XK_Cyrillic_YA	: return 0x042F;
	case XK_Cyrillic_ER	: return 0x0420;
	case XK_Cyrillic_ES	: return 0x0421;
	case XK_Cyrillic_TE	: return 0x0422;
	case XK_Cyrillic_U	: return 0x0423;
	case XK_Cyrillic_ZHE	: return 0x0416;
	case XK_Cyrillic_VE	: return 0x0412;
	case XK_Cyrillic_SOFTSIGN	: return 0x042C;
	case XK_Cyrillic_YERU	: return 0x042B;
	case XK_Cyrillic_ZE	: return 0x0417;
	case XK_Cyrillic_SHA	: return 0x0428;
	case XK_Cyrillic_E	: return 0x042D;
	case XK_Cyrillic_SHCHA	: return 0x0429;
	case XK_Cyrillic_CHE	: return 0x0427;
	case XK_Cyrillic_HARDSIGN	: return 0x042A;
#endif

		/* Technical, Special missing*/

	case XK_Greek_ALPHAaccent	: return 0x0386;
	case XK_Greek_EPSILONaccent	: return 0x0388;
	case XK_Greek_ETAaccent		: return 0x0389;
	case XK_Greek_IOTAaccent	: return 0x038A;
	case XK_Greek_IOTAdiaeresis	: return 0x03AA;
	case XK_Greek_OMICRONaccent	: return 0x038C;
	case XK_Greek_UPSILONaccent	: return 0x038E;
	case XK_Greek_UPSILONdieresis	: return 0x03AB;
	case XK_Greek_OMEGAaccent	: return 0x038F;
	case XK_Greek_accentdieresis	: return 0x0385;
	case XK_Greek_horizbar		: return 0x2015;
	case XK_Greek_alphaaccent	: return 0x03AC;
	case XK_Greek_epsilonaccent	: return 0x03AD;
	case XK_Greek_etaaccent		: return 0x03AE;
	case XK_Greek_iotaaccent	: return 0x03AF;
	case XK_Greek_iotadieresis	: return 0x03CA;
	case XK_Greek_iotaaccentdieresis: return 0x0390;
	case XK_Greek_omicronaccent	: return 0x03CC;
	case XK_Greek_upsilonaccent	: return 0x03CD;
	case XK_Greek_upsilondieresis	: return 0x03CB;
	case XK_Greek_upsilonaccentdieresis: return 0x03B0;
	case XK_Greek_omegaaccent	: return 0x03CE;
	case XK_Greek_ALPHA		: return 0x0391;
	case XK_Greek_BETA		: return 0x0392;
	case XK_Greek_GAMMA		: return 0x0393;
	case XK_Greek_DELTA		: return 0x0394;
	case XK_Greek_EPSILON		: return 0x0395;
	case XK_Greek_ZETA		: return 0x0396;
	case XK_Greek_ETA		: return 0x0397;
	case XK_Greek_THETA		: return 0x0398;
	case XK_Greek_IOTA		: return 0x0399;
	case XK_Greek_KAPPA		: return 0x039A;
	case XK_Greek_LAMBDA		: return 0x039B;
	case XK_Greek_MU		: return 0x039C;
	case XK_Greek_NU		: return 0x039D;
	case XK_Greek_XI		: return 0x039E;
	case XK_Greek_OMICRON		: return 0x039F;
	case XK_Greek_PI		: return 0x03A0;
	case XK_Greek_RHO		: return 0x03A1;
	case XK_Greek_SIGMA		: return 0x03A3;
	case XK_Greek_TAU		: return 0x03A4;
	case XK_Greek_UPSILON		: return 0x03A5;
	case XK_Greek_PHI		: return 0x03A6;
	case XK_Greek_CHI		: return 0x03A7;
	case XK_Greek_PSI		: return 0x03A8;
	case XK_Greek_OMEGA		: return 0x03A9;
	case XK_Greek_alpha		: return 0x03B1;
	case XK_Greek_beta		: return 0x03B2;
	case XK_Greek_gamma		: return 0x03B3;
	case XK_Greek_delta		: return 0x03B4;
	case XK_Greek_epsilon		: return 0x03B5;
	case XK_Greek_zeta		: return 0x03B6;
	case XK_Greek_eta		: return 0x03B7;
	case XK_Greek_theta		: return 0x03B8;
	case XK_Greek_iota		: return 0x03B9;
	case XK_Greek_kappa		: return 0x03BA;
	case XK_Greek_lambda		: return 0x03BB;
	case XK_Greek_mu		: return 0x03BC;
	case XK_Greek_nu		: return 0x03BD;
	case XK_Greek_xi		: return 0x03BE;
	case XK_Greek_omicron		: return 0x03BF;
	case XK_Greek_pi		: return 0x03C0;
	case XK_Greek_rho		: return 0x03C1;
	case XK_Greek_sigma		: return 0x03C3;
	case XK_Greek_finalsmallsigma	: return 0x03C2;
	case XK_Greek_tau		: return 0x03C4;
	case XK_Greek_upsilon		: return 0x03C5;
	case XK_Greek_phi		: return 0x03C6;
	case XK_Greek_chi		: return 0x03C7;
	case XK_Greek_psi		: return 0x03C8;
	case XK_Greek_omega		: return 0x03C9;
		/* case XK_Greek_switch         0xFF7E  Alias for mode_switch */

		/* Publishing, APL missing*/

#ifdef XK_HEBREW
	case XK_hebrew_doublelowline: return 0x2017;
	case XK_hebrew_aleph	: return 0x05D0;
	case XK_hebrew_bet     	: return 0x05D1;
	case XK_hebrew_gimel	: return 0x05D2;
	case XK_hebrew_dalet	: return 0x05D3;
	case XK_hebrew_he	: return 0x05D4;
	case XK_hebrew_waw	: return 0x05D5;
	case XK_hebrew_zain	: return 0x05D6;
	case XK_hebrew_chet	: return 0x05D7;
	case XK_hebrew_tet	: return 0x05D8;
	case XK_hebrew_yod	: return 0x05D9;
	case XK_hebrew_finalkaph: return 0x05DA;
	case XK_hebrew_kaph	: return 0x05DB;
	case XK_hebrew_lamed	: return 0x05DC;
	case XK_hebrew_finalmem	: return 0x05DD;
	case XK_hebrew_mem	: return 0x05DE;
	case XK_hebrew_finalnun	: return 0x05DF;
	case XK_hebrew_nun	: return 0x05E0;
	case XK_hebrew_samech	: return 0x05E1;
	case XK_hebrew_ayin	: return 0x05E2;
	case XK_hebrew_finalpe	: return 0x05E3;
	case XK_hebrew_pe	: return 0x05E4;
	case XK_hebrew_finalzade: return 0x05E5;
	case XK_hebrew_zade	: return 0x05E6;
	case XK_hebrew_qoph	: return 0x05E7;
	case XK_hebrew_resh	: return 0x05E8;
	case XK_hebrew_shin	: return 0x05E9;
	case XK_hebrew_taw	: return 0x05EA;
		/* case XK_Hebrew_switch 0xFF7E  Alias for mode_switch */
#endif
		/* Thai, Korean missing*/
	}

	return GIIK_VOID;
}


int _gii_xev_trans(XKeyEvent *xev, gii_key_event *giiev,
		   XComposeStatus *compose_status, XIC xic,
		   unsigned int *oldcode)
{
	KeySym xsym;
	uint32_t label, sym = GIIK_VOID, modifiers = 0;

	if (xic) {
		Status status;
		char buf[32];
		int n;

		n = XmbLookupString(xic, xev, buf, 32, &xsym, &status);
		switch (status) {
		case XLookupKeySym:
		case XLookupBoth:
			sym = basic_trans(xsym, 0);
			break;
		case XLookupChars:
			sym = *buf;
			break;
		case XBufferOverflow:
			DPRINT_CORE("can't fit %i bytes into buffer!\n", n);
			break;
		}
	} else {
		XLookupString(xev, NULL, 0, &xsym, compose_status);
		sym = basic_trans(xsym, 0);
	}

	if (xev->keycode == 0 && oldcode && *oldcode) {
		/* Composed key */
		xev->keycode = *oldcode;
		giiev->button = *oldcode - 8;
		*oldcode = 0;
	}
	label = basic_trans(XLookupKeysym(xev, 0), 1);
	
	if (xev->state & ShiftMask) modifiers |= GII_MOD_SHIFT;
	if (xev->state & LockMask)  modifiers |= GII_MOD_CAPS;
	if (xev->state & ControlMask) {
		modifiers |= GII_MOD_CTRL;

		/* Translate control charactrers */
		if (sym >= '@' && sym <= '_') {
			sym -= '@';
		} else if (sym >= 'a' && sym <= 'z') {
			sym -= ('a'-1);
		}
	}
	if (xev->state & Mod1Mask) {
		modifiers |= GII_MOD_ALT;
		modifiers |= GII_MOD_META;
	}
	if (xev->state & Mod2Mask) modifiers |= GII_MOD_NUM;
	if (xev->state & Mod3Mask) modifiers |= GII_MOD_ALTGR;
	if (xev->state & Mod5Mask) modifiers |= GII_MOD_SCROLL;
	
	if (GII_KTYP(sym) == GII_KT_MOD) {
		sym &= ~GII_KM_RIGHT;
	} else if (GII_KTYP(sym) == GII_KT_PAD) {
		if (GII_KVAL(sym) < 0x80) {
			sym = GII_KVAL(sym);
		}
	} else if (GII_KTYP(sym) == GII_KT_DEAD) {
		sym = GIIK_VOID;
	}

	giiev->label = label;
	giiev->sym = sym;
	giiev->modifiers = modifiers;

	return 0;
}


uint32_t _gii_xev_buttontrans(unsigned int button)
{
	switch(button) {
	/* X assumes a middle button, while GGI goes in order
	   of availability. */
	case Button1:
		return GII_PBUTTON_LEFT;
	case Button2:
		return GII_PBUTTON_MIDDLE;
	case Button3:
		return GII_PBUTTON_RIGHT;
	case Button4:
		return GII_PBUTTON_(4);
	case Button5:
		return GII_PBUTTON_(5);
	}

	return GII_PBUTTON_(button);
}

