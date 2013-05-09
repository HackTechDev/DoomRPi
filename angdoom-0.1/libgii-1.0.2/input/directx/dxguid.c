/* $Id: dxguid.c,v 1.2 2005/01/21 11:58:39 pekberg Exp $
******************************************************************************

   Used DirectX IIDs and GUIDs

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

/* libtool fails to link a dll against libdxguid.a which
 * is what we really want to do here. When libtool can do
 * that, kill this file and add -ldxguid to the link
 * command line.
 * IMHO, this is a horrid workaround. But it works, and we
 * get a dll instead of a static lib.
 */

typedef struct _IID
{
    unsigned long  x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

typedef IID GUID;
typedef IID CLSID;

const CLSID CLSID_DirectInput       = {0x25E609E0,0xB259,0x11CF,{0xBF,0xC7,0x44,0x45,0x53,0x54,0x00,0x00}};
const IID  IID_IDirectInput2A       = {0x5944e662,0xaa8a,0x11cf,{0xbf,0xc7,0x44,0x45,0x53,0x54,0x00,0x00}};
const IID  IID_IDirectInputDevice2A = {0x5944e682,0xc92e,0x11cf,{0xbf,0xc7,0x44,0x45,0x53,0x54,0x00,0x00}};
const GUID GUID_Key                 = {0x55728220,0xd33c,0x11cf,{0xbf,0xc7,0x44,0x45,0x53,0x54,0x00,0x00}};
const GUID GUID_XAxis               = {0xa36d02e0,0xc9f3,0x11cf,{0xbf,0xc7,0x44,0x45,0x53,0x54,0x00,0x00}};
const GUID GUID_YAxis               = {0xa36d02e1,0xc9f3,0x11cf,{0xbf,0xc7,0x44,0x45,0x53,0x54,0x00,0x00}};
const GUID GUID_ZAxis               = {0xa36d02e2,0xc9f3,0x11cf,{0xbf,0xc7,0x44,0x45,0x53,0x54,0x00,0x00}};
const GUID GUID_RxAxis              = {0xa36d02f4,0xc9f3,0x11cf,{0xbf,0xc7,0x44,0x45,0x53,0x54,0x00,0x00}};
const GUID GUID_RyAxis              = {0xa36d02f5,0xc9f3,0x11cf,{0xbf,0xc7,0x44,0x45,0x53,0x54,0x00,0x00}};
const GUID GUID_RzAxis              = {0xa36d02e3,0xc9f3,0x11cf,{0xbf,0xc7,0x44,0x45,0x53,0x54,0x00,0x00}};
const GUID GUID_Slider              = {0xa36d02e4,0xc9f3,0x11cf,{0xbf,0xc7,0x44,0x45,0x53,0x54,0x00,0x00}};
const GUID GUID_POV                 = {0xa36d02f2,0xc9f3,0x11cf,{0xbf,0xc7,0x44,0x45,0x53,0x54,0x00,0x00}};
const GUID GUID_SysKeyboard         = {0x6f1d2b61,0xd5a0,0x11cf,{0xbf,0xc7,0x44,0x45,0x53,0x54,0x00,0x00}};
const GUID GUID_SysMouse            = {0x6f1d2b60,0xd5a0,0x11cf,{0xbf,0xc7,0x44,0x45,0x53,0x54,0x00,0x00}};
