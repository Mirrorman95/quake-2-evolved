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
// r_lightEditor.c - Light editor
//


#include "r_local.h"
#include "../common/editor.h"


typedef struct {
	bool					active;

	lightData_t *			focusLight;
	lightData_t *			editLight;
} lightEditor_t;

static lightEditor_t		r_lightEditor;


/*
 ==================
 
 ==================
*/
void R_RefreshLightEditor (){

}


/*
 ==============================================================================

 CALLBACK FUNCTIONS

 ==============================================================================
*/


/*
 ==================
 
 ==================
*/
void R_LightEditorUpdateCallback (int index, lightParms_t *parms){

}

/*
 ==================
 
 ==================
*/
void R_LightEditorRemoveCallback (int index){

}

/*
 ==================
 
 ==================
*/
void R_LightEditorSaveCallback (){

}

/*
 ==================
 
 ==================
*/
void R_LightEditorCloseCallback (){

}

/*
 ==================
 
 ==================
*/
static bool R_LightEditorMouseEvent (){

}


/*
 ==============================================================================

 CONSOLE COMMANDS

 ==============================================================================
*/


/*
 ==================
 R_EditLights_f
 ==================
*/
static void R_EditLights_f (){

	editorCallbacks_t	callbacks;

	// Launch the light editor
	callbacks.createWindow = WIN_CreateLightEditorWindow;
	callbacks.destroyWindow = WIN_DestroyLightEditorWindow;
	callbacks.mouseEvent = R_LightEditorMouseEvent;

	if (!Com_LaunchEditor("light", &callbacks))
		return;

	r_lightEditor.active = true;

	r_lightEditor.focusLight = NULL;
	r_lightEditor.editLight = NULL;

	// Finish setting up, but don't edit anything yet
	WIN_EditLightParameters(-1, NULL);
}

/*
 ==================
 
 ==================
*/
static void R_AddLight_f (){

}


/*
 ==============================================================================

 INITIALIZATION AND SHUTDOWN

 ==============================================================================
*/


/*
 ==================
 R_InitLightEditor
 ==================
*/
void R_InitLightEditor (){

	// Add commands
	Cmd_AddCommand("editLights", R_EditLights_f, "Launches the integrated light editor", NULL);
	Cmd_AddCommand("addLight", R_AddLight_f, "Adds a new light", NULL);
}

/*
 ==================
 R_ShutdownLightEditor
 ==================
*/
void R_ShutdownLightEditor (){

	// Remove commands
	Cmd_RemoveCommand("editLights");
	Cmd_RemoveCommand("addLight");

	// Close the light editor if active
	if (r_lightEditor.active)
		Com_CloseEditor();

	Mem_Fill(&r_lightEditor, 0, sizeof(lightEditor_t));
}