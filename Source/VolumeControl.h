/********************************************************************************
 Copyright (C) 2012 Hugh Bailey <obs.jim@gmail.com>

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
********************************************************************************/


#pragma once

#define VOLUME_CONTROL_CLASS TEXT("OBSVolumeControl")

void InitVolumeControl();
void SetVolumeControlIcons(HWND hwnd, HICON hiconPlay, HICON hiconMute);
float ToggleVolumeControlMute(HWND hwnd);
float SetVolumeControlValue(HWND hwnd, float fValue);
float GetVolumeControlValue(HWND hwnd);


#define VOLN_ADJUSTING  0x300
#define VOLN_FINALVALUE 0x301

/* 60 db dynamic range values for volume control scale*/
#define VOL_ALPHA .001f
#define VOL_BETA 6.908f
