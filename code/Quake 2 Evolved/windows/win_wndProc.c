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
// win_wndProc.c - Windows message procedure
//


#include "../client/client.h"
#include "win_local.h"


#ifndef WM_MOUSEWHEEL
#define WM_MOUSEWHEEL				(WM_MOUSELAST+1)  // Message that will be supported by the OS 
#endif

#define CONSOLE_KEY_SCANCODE		41

static byte					win_scanCodeToKey[128] = {
	0           , K_ESCAPE    , '1'         , '2'         , '3'         , '4'         , '5'         , '6'         ,
	'7'         , '8'         , '9'         , '0'         , '-'         , '='         , K_BACKSPACE , K_TAB       ,
	'q'         , 'w'         , 'e'         , 'r'         , 't'         , 'y'         , 'u'         , 'i'         ,
	'o'         , 'p'         , '['         , ']'         , K_ENTER     , K_LCTRL     , 'a'         , 's'         ,
	'd'         , 'f'         , 'g'         , 'h'         , 'j'         , 'k'         , 'l'         , ';'         ,
	'\''        , '`'         , K_LSHIFT    , '\\'        , 'z'         , 'x'         , 'c'         , 'v'         ,
	'b'         , 'n'         , 'm'         , ','         , '.'         , '/'         , K_RSHIFT    , K_KP_STAR   ,
	K_LALT      , K_SPACE     , K_CAPSLOCK  , K_F1        , K_F2        , K_F3        , K_F4        , K_F5        ,
	K_F6        , K_F7        , K_F8        , K_F9        , K_F10       , K_PAUSE     , K_SCROLLLOCK, K_HOME      ,
	K_UPARROW   , K_PAGEUP    , K_KP_MINUS  , K_LEFTARROW , K_KP_5      , K_RIGHTARROW, K_KP_PLUS   , K_END       ,
	K_DOWNARROW , K_PAGEDOWN  , K_INSERT    , K_DELETE    , 0           , 0           , 0           , K_F11       ,
	K_F12       , 0           , 0           , 0           , 0           , K_MENU      , 0           , 0           ,
	0           , 0           , 0           , 0           , 0           , 0           , 0           , 0           ,
	0           , 0           , 0           , 0           , 0           , 0           , 0           , 0           ,
	0           , 0           , 0           , 0           , 0           , 0           , 0           , 0           ,
	0           , 0           , 0           , 0           , 0           , 0           , 0           , 0
};

static HHOOK				win_keyboardHook;

static bool					win_altTabDisabled;

static UINT					vid_msgMouseWheel;

static cvar_t *				win_xPos;
static cvar_t *				win_yPos;
static cvar_t *				win_allowAltTab;
static cvar_t *				win_allowPrintScreen;


/*
 ==================
 WIN_LowLevelKeyboardProc
 ==================
*/
static LRESULT CALLBACK WIN_LowLevelKeyboardProc (int nCode, WPARAM wParam, LPARAM lParam){

	KBDLLHOOKSTRUCT	*kbd = (KBDLLHOOKSTRUCT *)lParam;

	if (nCode != HC_ACTION)
		return CallNextHookEx(win_keyboardHook, nCode, wParam, lParam);

	switch (wParam){
	case WM_SYSKEYDOWN:
	case WM_KEYDOWN:
		switch (kbd->vkCode){
		case VK_LWIN:
			Key_Event(K_LWIN, kbd->time, true);
			return 1;
		case VK_RWIN:
			Key_Event(K_RWIN, kbd->time, true);
			return 1;
		case VK_SNAPSHOT:
			if (win_allowPrintScreen->integerValue)
				break;

			Key_Event(K_PRINTSCREEN, kbd->time, true);
			return 1;
		}

		break;
	case WM_SYSKEYUP:
	case WM_KEYUP:
		switch (kbd->vkCode){
		case VK_LWIN:
			Key_Event(K_LWIN, kbd->time, false);
			return 1;
		case VK_RWIN:
			Key_Event(K_RWIN, kbd->time, false);
			return 1;
		case VK_SNAPSHOT:
			if (win_allowPrintScreen->integerValue)
				break;

			Key_Event(K_PRINTSCREEN, kbd->time, false);
			return 1;
		}

		break;
	}

	return CallNextHookEx(win_keyboardHook, nCode, wParam, lParam);
}

/*
 ==================
 WIN_AddKeyboardHook
 ==================
*/
static void WIN_AddKeyboardHook (){

	if (win_keyboardHook)
		return;

	win_keyboardHook = SetWindowsHookEx(WH_KEYBOARD_LL, WIN_LowLevelKeyboardProc, sys.hInstance, 0);
}

/*
 ==================
 WIN_RemoveKeyboardHook
 ==================
*/
static void WIN_RemoveKeyboardHook (){

	if (!win_keyboardHook)
		return;

	UnhookWindowsHookEx(win_keyboardHook);
	win_keyboardHook = NULL;
}

/*
 ==================
 WIN_EnableAltTab
 ==================
*/
static void WIN_EnableAltTab (){

	if (!win_altTabDisabled)
		return;
	win_altTabDisabled = false;

	UnregisterHotKey(NULL, 0);
}

/*
 ==================
 WIN_DisableAltTab
 ==================
*/
static void WIN_DisableAltTab (){

	if (win_altTabDisabled)
		return;
	win_altTabDisabled = true;

	RegisterHotKey(NULL, 0, MOD_ALT, VK_TAB);
}

/*
 ==================
 WIN_AppActivate
 ==================
*/
static void WIN_AppActivate (bool active, bool minimized){

	Com_DPrintf("WIN_AppActivate( %i, %i )\n", active, minimized);

	// We don't want to act like we're active if we're minimized
	if (active && !minimized)
		sys.isActive = true;
	else
		sys.isActive = false;

	sys.isMinimized = minimized;

	// Clear key and input states
	Key_ClearStates();
	IN_ClearStates();

	// Restore or minimize on demand
	if (sys.isActive){
		R_Activate(true);
		S_Activate(true);
		IN_Activate(true);

		WIN_AddKeyboardHook();

		if (sys.isFullscreen && !win_allowAltTab->integerValue)
			WIN_DisableAltTab();
	}
	else {
		if (sys.isFullscreen && !win_allowAltTab->integerValue)
			WIN_EnableAltTab();

		WIN_RemoveKeyboardHook();

		IN_Activate(false);
		S_Activate(false);
		R_Activate(false);
	}
}

/*	
 ==================
 WIN_MapKey
 ==================
*/
static int WIN_MapKey (int keyCode){

	int		scanCode;
	int		key;

	scanCode = (keyCode >> 16) & 255;
	if (scanCode > 127)
		return 0;

	key = win_scanCodeToKey[scanCode];

	if (keyCode & BIT(24)){
		switch (key){
		case K_LALT:
			return K_RALT;
		case K_LCTRL:
			return K_RCTRL;
		case K_ENTER:
			return K_KP_ENTER;
		case K_PAUSE:
			return K_KP_NUMLOCK;
		case '/':
			return K_KP_SLASH;
		}
	}
	else {
		switch (key){
		case K_UPARROW:
			return K_KP_UPARROW;
		case K_DOWNARROW:
			return K_KP_DOWNARROW;
		case K_LEFTARROW:
			return K_KP_LEFTARROW;
		case K_RIGHTARROW:
			return K_KP_RIGHTARROW;
		case K_INSERT:
			return K_KP_INSERT;
		case K_DELETE:
			return K_KP_DELETE;
		case K_HOME:
			return K_KP_HOME;
		case K_END:
			return K_KP_END;
		case K_PAGEUP:
			return K_KP_PAGEUP;
		case K_PAGEDOWN:
			return K_KP_PAGEDOWN;
		}
	}

	return key;
}

/*
 ==================
 WIN_MainWindowProc

 Main window procedure
 ==================
*/
LRESULT CALLBACK WIN_MainWindowProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){

	if (uMsg == vid_msgMouseWheel){
		// For Win95
		if (((int)wParam) > 0){
			Key_Event(K_MWHEELUP, sys.msgTime, true);
			Key_Event(K_MWHEELUP, sys.msgTime, false);
		}
		else {
			Key_Event(K_MWHEELDOWN, sys.msgTime, true);
			Key_Event(K_MWHEELDOWN, sys.msgTime, false);
		}

		return DefWindowProc(hWnd, uMsg, wParam, lParam);
	}

	switch (uMsg){
	case WM_CREATE:
		sys.hWndMain = hWnd;

		vid_msgMouseWheel = RegisterWindowMessage("MSWHEEL_ROLLMSG");

		// Register variables
		win_xPos = CVar_Register("win_xPos", "10", CVAR_INTEGER, CVAR_ARCHIVE | CVAR_LATCH, "Horizontal window position", 0, 65535);
		win_yPos = CVar_Register("win_yPos", "30", CVAR_INTEGER, CVAR_ARCHIVE | CVAR_LATCH, "Vertical window position", 0, 65535);
		win_allowAltTab = CVar_Register("win_allowAltTab", "0", CVAR_BOOL, CVAR_ARCHIVE | CVAR_LATCH, "Allow Alt-Tab when fullscreen", 0, 0);
		win_allowPrintScreen = CVar_Register("win_allowPrintScreen", "0", CVAR_BOOL, CVAR_ARCHIVE | CVAR_LATCH, "Allow capturing the screen", 0, 0);

		break;
	case WM_DESTROY:
		sys.hWndMain = NULL;

		// Enable Alt-Tab and remove keyboard hook if needed
		WIN_EnableAltTab();
		WIN_RemoveKeyboardHook();

		break;
	case WM_ACTIVATE:
		WIN_AppActivate(LOWORD(wParam) != WA_INACTIVE, HIWORD(wParam) != 0);

		break;
	case WM_CLOSE:
		PostQuitMessage(0);

		return 0;
	case WM_MOVE:
		if (!sys.isFullscreen){
			RECT	rect;
			int		style, exStyle;
			int		x, y;

			x = (int)((short)LOWORD(lParam));	// Horizontal position 
			y = (int)((short)HIWORD(lParam));	// Vertical position 

			style = GetWindowLong(hWnd, GWL_STYLE);
			exStyle = GetWindowLong(hWnd, GWL_EXSTYLE);

			rect.left = 0;
			rect.top = 0;
			rect.right = 1;
			rect.bottom = 1;

			AdjustWindowRectEx(&rect, style, FALSE, exStyle);

			CVar_SetInteger(win_xPos, x + rect.left);
			CVar_SetInteger(win_yPos, y + rect.top);

			if (sys.isActive)
				IN_Activate(false);
		}

		break;
	case WM_ERASEBKGND:		// TODO!!!

		break;
	case WM_LBUTTONDOWN:
	case WM_LBUTTONUP:
	case WM_RBUTTONDOWN:
	case WM_RBUTTONUP:
	case WM_MBUTTONDOWN:
	case WM_MBUTTONUP:
	case WM_XBUTTONDOWN:
	case WM_XBUTTONUP:
		// This is complicated because Win32 seems to pack multiple 
		// mouse events into one update sometimes, so we always check 
		// all states and look for events
		{
			int		state = 0x00;

			if (wParam & MK_LBUTTON)
				state |= 0x01;
			if (wParam & MK_RBUTTON)
				state |= 0x02;
			if (wParam & MK_MBUTTON)
				state |= 0x04;
			if (wParam & MK_XBUTTON1)
				state |= 0x08;
			if (wParam & MK_XBUTTON2)
				state |= 0x10;

			IN_MouseEvent(state);
		}

		break;
	case WM_MOUSEWHEEL:
		if ((short)HIWORD(wParam) > 0){
			Key_Event(K_MWHEELUP, sys.msgTime, true);
			Key_Event(K_MWHEELUP, sys.msgTime, false);
		}
		else {
			Key_Event(K_MWHEELDOWN, sys.msgTime, true);
			Key_Event(K_MWHEELDOWN, sys.msgTime, false);
		}

		break;
	case WM_SYSCOMMAND:
		if (wParam == SC_KEYMENU || wParam == SC_SCREENSAVE || wParam == SC_MONITORPOWER)
			return 0;

		break;
	case WM_SYSKEYDOWN:
		// Alt-Enter toggles fullscreen mode
		if (wParam == VK_RETURN){
			CVar_SetVariableInteger("r_fullscreen", !sys.isFullscreen, true);
			Cmd_AppendText("restartVideo\n");

			return 0;
		}

		// Fall through

	case WM_KEYDOWN:
		Key_Event(WIN_MapKey(lParam), sys.msgTime, true);

		break;
	case WM_SYSKEYUP:
	case WM_KEYUP:
		Key_Event(WIN_MapKey(lParam), sys.msgTime, false);

		break;
	case WM_CHAR:
		if (((lParam >> 16) & 255) == CONSOLE_KEY_SCANCODE)
			break;		// Ignore the console key

		Key_CharEvent(wParam);

		break;
	}

	// Pass all unhandled messages to DefWindowProc
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/*
 ==================
 WIN_FakeWindowProc

 Fake window procedure
 ==================
*/
LRESULT CALLBACK WIN_FakeWindowProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){

	// Pass all messages to DefWindowProc
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}