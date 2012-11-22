/*
 ------------------------------------------------------------------------------
 Copyright (C) 1997-2001 Id Software.

 This file is part of the Quake 2 source code.

 The Quake 2 source code is free software; you can redistribute it and/or
 modify it under the terms of the GNU General Public License as published by
 the Free Software Foundation; either version 2 of the License, or (at your
 option) any later version.

 The Quake 2 source code is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
 more details.

 You should have received a copy of the GNU General Public License along with
 the Quake 2 source code; if not, write to the Free Software Foundation, Inc.,
 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 ------------------------------------------------------------------------------
*/


//
// win_local.h - Win32-specific header
//


#ifndef __WIN_LOCAL_H__
#define __WIN_LOCAL_H__


#ifndef _WIN32
#error "You should not be including this file on this platform"
#endif


#include "resource.h"


typedef enum {
	CPUID_GENERIC					= BIT(0),	// Unrecognized processor
	CPUID_AMD						= BIT(1),	// AMD processor
	CPUID_INTEL						= BIT(2),	// Intel processor
	CPUID_POWERPC					= BIT(3),	// PowerPC processor
	CPUID_MMX						= BIT(4),	// MMX
	CPUID_MMX_EXT					= BIT(5),	// Extended MMX
	CPUID_3DNOW						= BIT(6),	// 3DNow!
	CPUID_3DNOW_EXT					= BIT(7),	// Extended 3DNow!
	CPUID_SSE						= BIT(8),	// Streaming SIMD Extensions
	CPUID_SSE2						= BIT(9),	// Streaming SIMD Extensions 2
	CPUID_SSE3						= BIT(10),	// Streaming SIMD Extensions 3
	CPUID_SSSE3						= BIT(11),	// Supplemental Streaming SIMD Extensions 3
	CPUID_SSE41						= BIT(12),	// Streaming SIMD Extensions 4.1
	CPUID_SSE42						= BIT(13),	// Streaming SIMD Extensions 4.2
	CPUID_SSE4A						= BIT(14),	// Streaming SIMD Extensions 4A
	CPUID_XOP						= BIT(15),	// XOP
	CPUID_FMA4						= BIT(16),	// Fused Multiply-Add 4
	CPUID_AVX						= BIT(17),	// Advanced Vector Extensions
	CPUID_ALTIVEC					= BIT(18)	// AltiVec
} cpuId_t;

typedef struct {
	HINSTANCE				hInstance;

	HICON					hIcon;

	HWND					hWndMain;

	bool					isActive;
	bool					isMinimized;
	bool					isFullscreen;

	int						msgTime;

	int						cpuId;
	int						cpuCount;
	char					cpuString[256];

	longlong				ticksPerSecond;

	char					currentDirectory[MAX_PATH_LENGTH];
} sysWin_t;

extern sysWin_t				sys;

void			Sys_CreateConsole ();
void			Sys_DestroyConsole ();

void			Sys_ConsolePrint (const char *text);
void			Sys_ConsoleError (const char *text);

LRESULT CALLBACK	WIN_MainWindowProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK	WIN_FakeWindowProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);


#endif	// __WIN_LOCAL_H__