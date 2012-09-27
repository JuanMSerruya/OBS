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

#define WINVER         0x0600
#define _WIN32_WINDOWS 0x0600
#define _WIN32_WINNT   0x0600
#define WIN32_LEAN_AND_MEAN
#define ISOLATION_AWARE_ENABLED 1
#include <windows.h>
#include <commctrl.h>
#include <commdlg.h>
#include <math.h>

#include <xmmintrin.h>
#include <emmintrin.h>



//-------------------------------------------
// direct3d 10.1

#include <D3D10_1.h>
#include <D3DX10.h>
#include <DXGI.h>


//-------------------------------------------
// API DLL

#include "OBSApi.h"


//-------------------------------------------
// application globals

class OBS;

extern HWND         hwndMain;
extern HWND         hwndRenderFrame;
extern HINSTANCE    hinstMain;
extern ConfigFile   *GlobalConfig;
extern ConfigFile   *AppConfig;
extern OBS          *App;
extern TCHAR        lpAppDataPath[MAX_PATH];

#define OBS_VERSION_STRING_ANSI "Open Broadcaster Software v0.381a"
#define OBS_VERSION_STRING TEXT(OBS_VERSION_STRING_ANSI)

#define OBS_WINDOW_CLASS      TEXT("OBSWindowClass")
#define OBS_RENDERFRAME_CLASS TEXT("RenderFrame")

inline UINT ConvertMSTo100NanoSec(UINT ms)
{
    return ms*1000*10; //1000 microseconds, then 10 "100nanosecond" segments
}

//big endian conversion functions
#define QWORD_BE(val) (((val>>56)&0xFF) | (((val>>48)&0xFF)<<8) | (((val>>40)&0xFF)<<16) | (((val>>32)&0xFF)<<24) | \
    (((val>>24)&0xFF)<<32) | (((val>>16)&0xFF)<<40) | (((val>>8)&0xFF)<<48) | ((val&0xFF)<<56))
#define DWORD_BE(val) (((val>>24)&0xFF) | (((val>>16)&0xFF)<<8) | (((val>>8)&0xFF)<<16) | ((val&0xFF)<<24))
#define WORD_BE(val)  (((val>>8)&0xFF) | ((val&0xFF)<<8))

__forceinline QWORD fastHtonll(QWORD qw) {return QWORD_BE(qw);}
__forceinline DWORD fastHtonl (DWORD dw) {return DWORD_BE(dw);}
__forceinline  WORD fastHtons (WORD  w)  {return  WORD_BE(w);}


//-------------------------------------------
// application headers

#include "../resource.h"
#include "VolumeControl.h"
#include "BandwidthMeter.h"
#include "OBS.h"
#include "WindowStuff.h"
#include "CodeTokenizer.h"
#include "D3D10System.h"
