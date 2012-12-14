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


#include "../renderer/r_local.h"
#include "../common/editor.h"
#include "win_local.h"


/*
 ==============================================================================

 INTEGRATED LIGHT EDITOR

 ==============================================================================
*/

#ifdef _WIN32


/*
 ==================
 
 ==================
*/
static void WIN_AddLightMaterial (const char *name){

}

/*
 ==================
 
 ==================
*/
void WIN_ApplyLightParameters (){

}

/*
 ==================
 
 ==================
*/
void WIN_ResetLightParameters (lightParms_t *parms){

}

/*
 ==================
 
 ==================
*/
void WIN_RemoveLightParameters (){

}

/*
 ==================
 
 ==================
*/
static LRESULT CALLBACK WIN_LightEditorWindowProc (HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam){

}

/*
 ==================
 
 ==================
*/
void *WIN_CreateLightEditorWindow (){

}

/*
 ==================
 
 ==================
*/
void WIN_DestroyLightEditorWindow (){

}

/*
 ==================
 
 ==================
*/
void WIN_EditLightParameters (int index, lightParms_t *parms){

}


#else

/*
 ==================
 WIN_CreateLightEditorWindow
 ==================
*/
void *WIN_CreateLightEditorWindow (){

	Com_Printf(S_COLOR_RED "The light editor is not currently supported on this platform");

	return NULL;
}

/*
 ==================
 WIN_DestroyLightEditorWindow
 ==================
*/
void WIN_DestroyLightEditorWindow (){

}

/*
 ==================
 WIN_EditLightParameters
 ==================
*/
void WIN_EditLightParameters (int index, lightParms_t *parms){

}

#endif