/* $Id: dxinput.h,v 1.12 2005/07/29 16:40:55 soyt Exp $
******************************************************************************

   DirectX inputlib internal header

   Copyright (C) 1999-2000 John Fortin		[fortinj@ibm.net]
   Copyright (C) 2000      Marcus Sundberg	[marcus@ggi-project.org]
   Copyright (C) 2004      Peter Ekberg		[peda@lysator.liu.se]

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

#include <stdlib.h>
#include <windows.h>

#define DIRECTINPUT_VERSION         0x0700
#include <dinput.h>
#ifdef DIRECTINPUT_HEADER_VERSION
#if DIRECTINPUT_HEADER_VERSION < DIRECTINPUT_VERSION
#error dinput.h too old
#endif
#endif

#include <ggi/gg.h>
#include <ggi/internal/gii-dl.h>
#include <ggi/internal/gii_debug.h>
#include <ggi/input/directx.h>


#define WM_DDMESSAGEBOX         0x7FFF

#define GII_DX_BUFFER_SIZE 1024	/* number of buffer elements */

typedef struct {
	DWORD			type;
	DWORD			ofs;
	gii_cmddata_getvalinfo  valinfo;
	void			*next;
} gii_di_priv_obj;

typedef struct {
	IDirectInputDevice2	*dev;
	DIJOYSTATE		dij;
	int			dij_filled;
	gii_di_priv_obj		*objs;
	gii_cmddata_getdevinfo  devinfo;
	gii_cmddata_getvalinfo  *allvalinfos;
	uint32_t       		origin;
	int			povs;
	BYTE			*btn;
	void			*next;
} gii_di_priv_dev;

typedef struct {
	HWND			hWnd;
	IDirectInput2		*pDI;
	IDirectInputDevice2	*pKeyboard;
	IDirectInputDevice2	*pMouse;
	gii_di_priv_dev		*devs;
	/* Keyboard */
	uint32_t       		originkey;
	uint32_t       		modifiers;
	uint32_t       		di_modifiers;
	uint32_t       		symlabel[256][2];
	int			repeat_key;
	struct timeval		repeat_at;
	uint32_t       		repeat_delay;
	uint32_t       		repeat_speed;
	int			dead;
	UINT			dead_vk;
	uint32_t       		dead_scan;
	uint32_t       		dead_mod;
	/* Mouse */
	uint32_t       		originptr;
	BYTE			ptrbtn[8];
} gii_di_priv;


HRESULT _gii_dx_InitDirectInput(gii_di_priv *priv, HWND hDlg,
				HINSTANCE hInstance);
HRESULT _gii_dx_SetAcquire(gii_di_priv *priv);
HRESULT _gii_dx_FreeDirectInput(gii_di_priv *priv);
HRESULT _gii_dx_GetKBInput(gii_di_priv *priv, DWORD * dwElements,
			   char *kb_buff);
HRESULT _gii_dx_GetPtrInput(gii_di_priv *priv, DWORD * dwElements,
			    DWORD *ptr_buff);
HRESULT _gii_dx_GetJoyInput(gii_di_priv_dev *dev, DWORD * dwElements,
			    DWORD *joy_buff);
HRESULT _gii_dx_DDMessageBox(HWND hWnd, LPCTSTR text, LPCTSTR caption);
void _gii_dx_release(gii_di_priv *priv);

#define DI_PRIV(inp)  ((gii_di_priv *)(inp)->priv)
