/* $Id: di.c,v 1.11 2005/07/29 16:40:55 soyt Exp $
******************************************************************************

   DirectX inputlib

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

#include "config.h"
#include <dxinput.h>

static BOOL CALLBACK
_gii_dx_dev_axis_callback(LPCDIDEVICEOBJECTINSTANCE lpddoi, void *pvRef)
{
	gii_di_priv_dev *dev = pvRef;
	gii_di_priv_obj **curr = &dev->objs;
	DIPROPRANGE dipr;
	HRESULT hr;

	dipr.diph.dwSize = sizeof(dipr);
	dipr.diph.dwHeaderSize = sizeof(dipr.diph);
	dipr.diph.dwObj = lpddoi->dwOfs;
	dipr.diph.dwHow = DIPH_BYOFFSET;
	hr = IDirectInputDevice2_GetProperty(dev->dev, DIPROP_RANGE,
						&dipr.diph);

	if(FAILED(hr))
		return DIENUM_CONTINUE;

	while(*curr)
		curr = (gii_di_priv_obj **)&((*curr)->next);

	*curr = calloc(1, sizeof(**curr));
	if(!*curr)
		return DIENUM_CONTINUE;
	(*curr)->type = lpddoi->dwType;
	(*curr)->ofs = lpddoi->dwOfs;
	(*curr)->valinfo.number = dev->devinfo.num_axes++;
	ggstrlcpy((*curr)->valinfo.longname,
		lpddoi->tszName,
		sizeof((*curr)->valinfo.longname));
	ggstrlcpy((*curr)->valinfo.shortname,
		lpddoi->tszName,
		sizeof((*curr)->valinfo.shortname));
	(*curr)->valinfo.range.min = dipr.lMin;
	(*curr)->valinfo.range.max = dipr.lMax;
	(*curr)->valinfo.range.center = (dipr.lMin + dipr.lMax) / 2;
	(*curr)->valinfo.phystype = GII_PT_UNKNOWN;

	return DIENUM_CONTINUE;
}

static BOOL CALLBACK
_gii_dx_dev_pov_callback(LPCDIDEVICEOBJECTINSTANCE lpddoi, void *pvRef)
{
	gii_di_priv_dev *dev = pvRef;
	gii_di_priv_obj **curr = &dev->objs;

	while(*curr)
		curr = (gii_di_priv_obj **)&((*curr)->next);

	*curr = calloc(1, sizeof(**curr));
	if(!*curr)
		goto fail_next;
	(*curr)->type = lpddoi->dwType;
	(*curr)->ofs = lpddoi->dwOfs;
	(*curr)->valinfo.number = dev->devinfo.num_axes++;
	sprintf((*curr)->valinfo.longname, "POV %d (btn %d)",
		++dev->povs, ++dev->devinfo.num_buttons);
	if(dev->devinfo.num_buttons < 10)
		sprintf((*curr)->valinfo.shortname, "pov%d",
			dev->devinfo.num_buttons);
	else if(dev->devinfo.num_buttons < 100)
		sprintf((*curr)->valinfo.shortname, "pv%d",
			dev->devinfo.num_buttons);
	else if(dev->devinfo.num_buttons < 1000)
		sprintf((*curr)->valinfo.shortname, "p%d",
			dev->devinfo.num_buttons);
	else
		goto fail_free;
	(*curr)->valinfo.range.min = 0;
	(*curr)->valinfo.range.max = 35999;
	(*curr)->valinfo.range.center = 18000;
	(*curr)->valinfo.phystype = GII_PT_ANGLE;
	(*curr)->valinfo.SI_add = 0;
	/* FIXME: Find a better fraction of pi/18000 */
	(*curr)->valinfo.SI_mul = 1686629713;
	(*curr)->valinfo.SI_div = 18000;
	(*curr)->valinfo.SI_shift = -29;
	return DIENUM_CONTINUE;

fail_free:
	free(*curr);
	*curr = NULL;
fail_next:
	return DIENUM_CONTINUE;
}

static BOOL CALLBACK
_gii_dx_dev_callback(LPCDIDEVICEINSTANCE lpddi, void *pvRef)
{
	gii_di_priv *priv = pvRef;
	gii_di_priv_dev **curr = &priv->devs;
	HRESULT hr;
	IDirectInputDevice *tmpdev;
	IDirectInputDevice2 *dev;
	DIPROPDWORD dipdw;
	DIDEVCAPS dicaps;
	int devnum = 0;

	hr = IDirectInput2_CreateDevice(priv->pDI, &lpddi->guidInstance,
					     &tmpdev, NULL);
	if (FAILED(hr))
		goto fail_next;

	hr = IDirectInputDevice_QueryInterface(tmpdev, &IID_IDirectInputDevice2,
						(LPVOID *)(void *)&dev);
	IDirectInputDevice_Release(tmpdev);
	tmpdev = NULL;
	if (FAILED(hr))
		goto fail_next;

	hr = IDirectInputDevice2_SetDataFormat(dev, &c_dfDIJoystick);
	if (FAILED(hr))
		goto fail_release;

	hr = IDirectInputDevice2_SetCooperativeLevel(dev, priv->hWnd,
				  DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
	if (FAILED(hr))
		goto fail_release;

	dipdw.diph.dwSize = sizeof(DIPROPDWORD);
	dipdw.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	dipdw.diph.dwObj = 0;
	dipdw.diph.dwHow = DIPH_DEVICE;
	dipdw.dwData = GII_DX_BUFFER_SIZE;

	hr = IDirectInputDevice2_SetProperty(dev, DIPROP_BUFFERSIZE,
					    &dipdw.diph);
	if (FAILED(hr))
		goto fail_release;

	dicaps.dwSize = sizeof(dicaps);
	hr = IDirectInputDevice2_GetCapabilities(dev, &dicaps);
	if (FAILED(hr))
		goto fail_release;

	if(!dicaps.dwButtons && !dicaps.dwAxes && !dicaps.dwPOVs)
		goto fail_release;

	while(*curr) {
		curr = (gii_di_priv_dev **)&((*curr)->next);
		++devnum;
	}

	*curr = calloc(1, sizeof(**curr));
	if(!*curr)
		goto fail_release;
	(*curr)->dev = dev;
	ggstrlcpy((*curr)->devinfo.longname,
		lpddi->tszInstanceName,
		sizeof((*curr)->devinfo.longname));
	if(++devnum < 10)
		sprintf((*curr)->devinfo.shortname, "joy%d", devnum);
	else if(devnum < 100)
		sprintf((*curr)->devinfo.shortname, "jo%d", devnum);
	else if(devnum < 1000)
		sprintf((*curr)->devinfo.shortname, "j%d", devnum);
	else
		goto fail_free;
	(*curr)->devinfo.num_buttons = dicaps.dwButtons;
	(*curr)->devinfo.num_axes = 0;

	hr = IDirectInputDevice2_EnumObjects(dev, _gii_dx_dev_axis_callback,
						*curr, DIDFT_ABSAXIS);
	if(FAILED(hr))
		goto fail_free;

	hr = IDirectInputDevice2_EnumObjects(dev, _gii_dx_dev_pov_callback,
						*curr, DIDFT_POV);
	if(FAILED(hr))
		goto fail_free;

	if(!dicaps.dwButtons && !(*curr)->devinfo.num_axes)
		goto fail_free;

	if(dicaps.dwButtons) {
		(*curr)->btn = calloc(dicaps.dwButtons, 1);
		if(!(*curr)->btn)
			goto fail_free;
		(*curr)->devinfo.can_generate |= emKeyPress | emKeyRelease;
	}
	if((*curr)->devinfo.num_axes)
		(*curr)->devinfo.can_generate |= emValAbsolute;

	return DIENUM_CONTINUE;

fail_free:
	free(*curr);
	*curr = NULL;
fail_release:
	IDirectInputDevice2_Release(dev);
fail_next:
	return DIENUM_CONTINUE;
}

HRESULT
_gii_dx_InitDirectInput(gii_di_priv *priv, HWND hWnd, HINSTANCE hInstance)
{
	HRESULT hr;
	DIPROPDWORD dipdw_k;
	DIPROPDWORD dipdw_m;
	IDirectInputDevice *tmpdev;

	CoInitialize(NULL);

	hr = CoCreateInstance(&CLSID_DirectInput, NULL, CLSCTX_INPROC_SERVER,
			      &IID_IDirectInput2, (void**)&priv->pDI);
	if (FAILED(hr))	return hr;

	IDirectInput2_Initialize(priv->pDI, hInstance, DIRECTINPUT_VERSION);
	if (FAILED(hr))	return hr;

	hr = IDirectInput2_CreateDevice(priv->pDI, &GUID_SysKeyboard,
					&tmpdev, NULL);
	if (FAILED(hr))	return hr;

	hr = IDirectInputDevice_QueryInterface(tmpdev, &IID_IDirectInputDevice2,
					(void**)&priv->pKeyboard);
	IDirectInputDevice_Release(tmpdev);
	tmpdev = NULL;
	if (FAILED(hr)) return hr;

	hr = IDirectInputDevice2_SetDataFormat(priv->pKeyboard,
					      &c_dfDIKeyboard);
	if (FAILED(hr))	return hr;

	hr = IDirectInputDevice2_SetCooperativeLevel(priv->pKeyboard, hWnd,
				  DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
	if (FAILED(hr))	return hr;

	dipdw_k.diph.dwSize = sizeof(DIPROPDWORD);
	dipdw_k.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	dipdw_k.diph.dwObj = 0;
	dipdw_k.diph.dwHow = DIPH_DEVICE;
	dipdw_k.dwData = GII_DX_BUFFER_SIZE;

	hr = IDirectInputDevice2_SetProperty(priv->pKeyboard,
						DIPROP_BUFFERSIZE,
						&dipdw_k.diph);
	if (FAILED(hr)) return hr;

	hr = IDirectInput2_CreateDevice(priv->pDI, &GUID_SysMouse,
					     &tmpdev, NULL);
	if (FAILED(hr)) return hr;

	hr = IDirectInputDevice_QueryInterface(tmpdev, &IID_IDirectInputDevice2,
						(void**)&priv->pMouse);
	IDirectInputDevice_Release(tmpdev);
	tmpdev = NULL;
	if (FAILED(hr)) return hr;

	hr = IDirectInputDevice2_SetDataFormat(priv->pMouse, &c_dfDIMouse);
	if (FAILED(hr)) return hr;
	
	hr = IDirectInputDevice2_SetCooperativeLevel(priv->pMouse, hWnd,
				  DISCL_NONEXCLUSIVE | DISCL_FOREGROUND);
	if (FAILED(hr)) return hr;
	
	dipdw_m.diph.dwSize = sizeof(DIPROPDWORD);
	dipdw_m.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	dipdw_m.diph.dwObj = 0;
	dipdw_m.diph.dwHow = DIPH_DEVICE;
	dipdw_m.dwData = GII_DX_BUFFER_SIZE;

	hr = IDirectInputDevice2_SetProperty(priv->pMouse, DIPROP_BUFFERSIZE,
					    &dipdw_m.diph);
	if (FAILED(hr)) return hr;

	hr = IDirectInput_EnumDevices(priv->pDI, DIDEVTYPE_JOYSTICK,
				(LPDIENUMDEVICESCALLBACK)_gii_dx_dev_callback,
				priv, DIEDFL_ALLDEVICES);
	if (FAILED(hr)) return hr;

	return S_OK;
}


HRESULT
_gii_dx_SetAcquire(gii_di_priv *priv)
{
	gii_di_priv_dev *dev = priv->devs;

	if (NULL == priv->pKeyboard) return S_FALSE;

	IDirectInputDevice2_Acquire(priv->pKeyboard);
	IDirectInputDevice2_Acquire(priv->pMouse);

	for(dev = priv->devs; dev; dev = dev->next)
		IDirectInputDevice2_Acquire(dev->dev);

	return S_OK;
}


void
_gii_dx_release(gii_di_priv *priv)
{
	gii_di_priv_dev *dev;
	for (dev = priv->devs; dev; dev = dev->next) {
		if (!dev->dev)
			continue;
		IDirectInputDevice2_Release(dev->dev);
		dev->dev = NULL;
	}
	if (priv->pMouse) {
		IDirectInputDevice2_Release(priv->pMouse);
		priv->pMouse = NULL;
	}
	if (priv->pKeyboard) {
		IDirectInputDevice2_Release(priv->pKeyboard);
		priv->pKeyboard = NULL;
	}
	if (priv->pDI) {
		IDirectInput2_Release(priv->pDI);
		priv->pDI = NULL;
	}
}


HRESULT
_gii_dx_GetKBInput(gii_di_priv *priv, DWORD *dwElements, char *kb_buff)
{
	DWORD i, hr;
	DIDEVICEOBJECTDATA didod[GII_DX_BUFFER_SIZE];

	if (NULL == priv->pKeyboard) {
		*dwElements = 0;
		return S_OK;
	}

	do {
		short state;
		*dwElements = GII_DX_BUFFER_SIZE;
		hr = IDirectInputDevice2_GetDeviceData(priv->pKeyboard,
				sizeof(DIDEVICEOBJECTDATA),
				didod, dwElements, 0);
		if(hr == DI_OK)
			break;

		*dwElements = 0;

		hr = IDirectInputDevice2_Acquire(priv->pKeyboard);
		if(FAILED(hr)) {
			priv->repeat_key = -1;
			return hr;
		}
		if(hr != DI_OK)
			/* was already acquired, don't retry */
			return hr;

		/* Keyboard was not acquired previously. */
		priv->di_modifiers = 0;
		priv->modifiers = 0;

		state = GetKeyState(VK_CAPITAL);
		if(state & 1)
			priv->modifiers |= GII_MOD_CAPS;
		if(state & 0x80)
			/* Toggle the modifier so that the state will be
			 * correct when the keypress is received that is
			 * generated for keys pressed down at the time the
			 * keyboard is acquired.
			 */
			priv->modifiers ^= GII_MOD_CAPS;

		for(i=0; i<255; ++i) {
			if(priv->symlabel[i][0] == GIIK_NIL)
				/* Not recorded down, ignore */
				continue;

			/* fake key release */
			kb_buff[2 * *dwElements] = i;
			kb_buff[2 * (*dwElements)++ + 1] = 'U';
		}
		/* GetDeviceData returns strange stuff if
		 * called immediately, return and wait for next
		 * poll.
		 */
		return DI_OK;
	} while (hr != DI_OK);

	for (i = 0; i < *dwElements; i++) {
		kb_buff[2 * i] = didod[i].dwOfs;
		kb_buff[2 * i + 1]
			= (didod[i].dwData & 0x80) ? 'D' : 'U';
	}

	return S_OK;
}


HRESULT
_gii_dx_GetPtrInput(gii_di_priv *priv, DWORD *dwElements, DWORD *ptr_buff)
{
	DWORD i, hr;
	DIDEVICEOBJECTDATA didod[GII_DX_BUFFER_SIZE];
	int start = 0;

	if (NULL == priv->pMouse) {
		*dwElements = 0;
		return DI_OK;
	}
	
	do {
		*dwElements = GII_DX_BUFFER_SIZE;
		hr = IDirectInputDevice2_GetDeviceData(priv->pMouse,
					sizeof(DIDEVICEOBJECTDATA),
					didod, dwElements, 0);
		if(SUCCEEDED(hr))
			break;

		/* Failed, assume no elements returned */
		*dwElements = 0;

		hr = IDirectInputDevice2_Acquire(priv->pMouse);
		if(FAILED(hr))
			return hr;
		if(hr != DI_OK)
			/* was already acquired, don't retry */
			return hr;
	} while(hr != DI_OK);

	for (i = 0; i < *dwElements; i++) {
		int btn;
		ptr_buff[2 * (start + i)] = didod[i].dwOfs;
		ptr_buff[2 * (start + i) + 1] = didod[i].dwData;
		if((int)didod[i].dwOfs < DIMOFS_BUTTON0)
			continue;
	   	/* button, handle double events */
		btn = didod[i].dwOfs - DIMOFS_BUTTON0;
		if(priv->ptrbtn[btn] == didod[i].dwData) {
			if(didod[i].dwData) {
				/* btn down *again*, fake up first */
				ptr_buff[2*(start+i)+1] = 0;
				ptr_buff[2*(++start+i)] = didod[i].dwOfs;
				ptr_buff[2*(start+i)+1] = didod[i].dwData;
			}
			else
				/* btn up *again*, ignore */
				--start;
		}
		priv->ptrbtn[btn] = didod[i].dwData;
	}
	*dwElements += start;

	return hr;
}

HRESULT
_gii_dx_GetJoyInput(gii_di_priv_dev *dev, DWORD *dwElements, DWORD *joy_buff)
{
	DWORD i, hr;
	DIDEVICEOBJECTDATA didod[GII_DX_BUFFER_SIZE];
	int start = 0;

	if (!dev->dev) {
		*dwElements = 0;
		return DI_OK;
	}
	
	do {
		IDirectInputDevice2_Poll(dev->dev);
		*dwElements = GII_DX_BUFFER_SIZE;
		hr = IDirectInputDevice2_GetDeviceData(dev->dev,
					sizeof(DIDEVICEOBJECTDATA),
					didod, dwElements, 0);
		if(SUCCEEDED(hr))
			break;

		/* Failed, assume no elements returned */
		*dwElements = 0;

		hr = IDirectInputDevice2_Acquire(dev->dev);
		if(FAILED(hr))
			return hr;
		if(hr != DI_OK)
			/* was already acquired, don't retry */
			return hr;
	} while(hr != DI_OK);

	for (i = 0; i < *dwElements; i++) {
		int btn;
		joy_buff[2 * (start + i)] = didod[i].dwOfs;
		joy_buff[2 * (start + i) + 1] = didod[i].dwData;
		if(didod[i].dwOfs < DIJOFS_BUTTON(0))
			continue;
		if(didod[i].dwOfs > sizeof(DIJOYSTATE))
			continue;
	   	/* button, handle double events */
		btn = didod[i].dwOfs - DIJOFS_BUTTON(0);
		if(dev->btn[btn] == (uint8_t)didod[i].dwData) {
			if(didod[i].dwData) {
				/* btn down *again*, fake up first */
				joy_buff[2*(start+i)+1] = 0;
				joy_buff[2*(++start+i)] = didod[i].dwOfs;
				joy_buff[2*(start+i)+1] = didod[i].dwData;
			}
			else
				/* btn up *again*, ignore */
				--start;
		}
		dev->btn[btn] = (uint8_t)didod[i].dwData;
	}
	*dwElements += start;

	return hr;
}


HRESULT
_gii_dx_FreeDirectInput(gii_di_priv *priv)
{
	gii_di_priv_dev *dev;
	if (NULL != priv->pKeyboard) {
		IDirectInputDevice2_Unacquire(priv->pKeyboard);
		IDirectInputDevice2_Release(priv->pKeyboard);
		priv->pKeyboard = NULL;
	}
	if (NULL != priv->pMouse) {
		IDirectInputDevice2_Unacquire(priv->pMouse);
		IDirectInputDevice2_Release(priv->pMouse);
		priv->pMouse = NULL;
	}
	for (dev = priv->devs; dev; dev = dev->next) {
		if (dev->dev) {
			IDirectInputDevice2_Unacquire(dev->dev);
			IDirectInputDevice2_Release(dev->dev);
			dev->dev = NULL;
		}
	}
	if (NULL != priv->pDI) {
		IDirectInput2_Release(priv->pDI);
		priv->pDI = NULL;
	}

	return 0;
}


typedef struct DDMessageBoxStruct
{
	HWND hWnd;
	LPCTSTR text;
	LPCTSTR caption;
	UINT type;
} DDMBS, *LPDDMBS;


HRESULT
_gii_dx_DDMessageBox(HWND hWnd, LPCTSTR text, LPCTSTR caption)
{
	DDMBS MessageData;

	MessageData.hWnd = hWnd;
	MessageData.text = text;
	MessageData.caption = caption;
	MessageData.type = MB_OK;
	SendMessage(hWnd, WM_DDMESSAGEBOX, 0, (LPARAM) & MessageData);

	return 0;
}
