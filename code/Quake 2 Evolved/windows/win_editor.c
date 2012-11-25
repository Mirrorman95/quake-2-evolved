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
// win_editor.c - Editors used by varius modules
//


#include "../common/common.h"
#include "win_local.h"


/*
 ==============================================================================

 INTEGRATED LIGHT EDITOR

 ==============================================================================
*/

#ifdef _WIN32

#define LIGHT_EDITOR_WINDOW_NAME	GAME_NAME " Light Editor"
#define LIGHT_EDITOR_WINDOW_CLASS	GAME_NAME " Light Editor"
#define LIGHT_EDITOR_WINDOW_STYLE	(WS_OVERLAPPED | WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX)

#define LIGHT_COLOR_BITMAP_SIZE		(80 * 20)

typedef struct {
	// Window stuff
	HINSTANCE				hInstance;

	HWND					hWndMaterialValue;
} lightParmsEditor_t;

static lightParmsEditor_t	win_lightParmsEditor;


/*
 ==================
 WIN_AddLightMaterial
 ==================
*/
static void WIN_AddLightMaterial (const char *name){

	if (Str_ICompareChars("lights/", name, 7))
		return;

	if (SendMessage(win_lightParmsEditor.hWndMaterialValue, CB_FINDSTRINGEXACT, (WPARAM)-1, (LPARAM)name) != CB_ERR)
		return;

	SendMessage(win_lightParmsEditor.hWndMaterialValue, CB_ADDSTRING, 0, (LPARAM)name);
}

/*
 ==================
 
 ==================
*/
static void WIN_ApplyLightParameters (){

}

/*
 ==================
 
 ==================
*/
static void WIN_ResetLightParameters (lightParms_t *parms){

}

/*
 ==================
 
 ==================
*/
static void WIN_RemoveLightParameters (){

}

/*
 ==================
 
 ==================
*/
static LRESULT CALLBACK WIN_LightEditorWindowProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){

	switch (uMsg){
	case WM_CLOSE:
		break;
	case WM_SYSCOMMAND:
		break;
	case WM_COMMAND:
		break;
	case WM_NOTIFY:
		break;
	}

	// Pass all unhandled messages to DefWindowProc
	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/*
 ==================
 
 ==================
*/
static void *WIN_CreateLightEditorWindow (){

	return NULL;
}

/*
 ==================
 
 ==================
*/
static void WIN_DestroyLightEditorWindow (){

}

/*
 ==================
 
 ==================
*/
static void WIN_EditLightParameters (int index, lightParms_t *parms){

}

#else

/*
 ==================
 WIN_CreateLightEditorWindow
 ==================
*/
static void *WIN_CreateLightEditorWindow (){

	Com_Printf(S_COLOR_RED "The light editor is not currently supported on this platform");

	return NULL;
}

/*
 ==================
 WIN_DestroyLightEditorWindow
 ==================
*/
static void WIN_DestroyLightEditorWindow (){

}

/*
 ==================
 WIN_EditLightParameters
 ==================
*/
static void WIN_EditLightParameters (int index, lightParms_t *parms){

}

#endif