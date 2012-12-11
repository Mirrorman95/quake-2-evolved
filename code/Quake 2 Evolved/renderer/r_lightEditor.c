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

	lightData_t		*visibleLights[MAX_STATIC_LIGHTS];
	int				numVisibleLights;
	lightData_t		*lightData;
	trace_t			trace;
	vec3_t			start, end;
	vec3_t			angles, axis[3];
	vec3_t			mins = {-5, -5, -5}, maxs = {5, 5, 5};
	float			fraction = 1.0f;
	int				headNode;
	int				i;

	vec3_t origin;

	if (!r_lightEditor.active)
		return;			// Not active

	r_lightEditor.focusLight = NULL;

	// Get the current view origin and up vector
	VectorCopy(rg.renderView.origin, origin);

	// Find visible lights
	numVisibleLights = 0;

	for (i = 0, lightData = r_staticLights; i < r_numStaticLights; i++, lightData++){

		// Always add the light that is being edited
		if (lightData == r_lightEditor.editLight){
			visibleLights[numVisibleLights++] = lightData;
			continue;
		}

		// TODO: frustum cull?

		// Cull lights by distance
		if (Distance(lightData->origin, origin) > 1000.0f){
			// Check if the view is inside the light bounds. This is so that
			// huge lights are added no matter how far their origin is.
			if (BoundsContainsPoint(lightData->mins, lightData->maxs, origin))
				continue;
		}

		// Add it to the list
		visibleLights[numVisibleLights++] = lightData;
	}

	if (!numVisibleLights)
		return;		// Nothing more to do here

	// Find the closest light that has focus, if any
	VectorCopy(rg.renderView.origin, start);
	VectorMA(rg.renderView.origin, 8192, rg.renderView.axis[0], end);

	VectorClear(angles);

	headNode = CM_HeadNodeForBox(mins, maxs);

	for (i = 0; i < numVisibleLights; i++){
		lightData = visibleLights[i];

		trace = CM_TransformedBoxTrace(start, end, vec3_origin, vec3_origin, headNode, MASK_ALL, lightData->origin, angles);

		if (trace.fraction < fraction){
			fraction = trace.fraction;

			// This light has focus
			r_lightEditor.focusLight = lightData;
		}
	}

	// Draw all visible lights
	for (i = 0; i < numVisibleLights; i++){
		lightData = visibleLights[i];

		VectorCopy(lightData->origin, origin);
		Matrix3_Copy(lightData->axis, axis);

		// Draw a box at the origin
		if (lightData != r_lightEditor.focusLight && lightData != r_lightEditor.editLight)
			R_DebugBox(colorWhite, origin, axis, mins, maxs, false, VIEW_MAIN);
		else
			R_DebugBox(colorRed, origin, axis, mins, maxs, false, VIEW_MAIN);

		// If editing this light
		if (lightData == r_lightEditor.editLight){

		}

		// Draw the entity and material names
	}
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

	lightData_t		*lightData;

	if (!r_lightEditor.active)
		return;			// Not active

	// Copy the parameters and clamp as needed
}

/*
 ==================
 
 ==================
*/
void R_LightEditorRemoveCallback (int index){

	if (!r_lightEditor.active)
		return;			// Not active

	// TODO: remove the static light

	// Force a reset
	r_lightEditor.focusLight = NULL;
	r_lightEditor.editLight = NULL;
}

/*
 ==================
 
 ==================
*/
void R_LightEditorSaveCallback (){

}

/*
 ==================
 R_LightEditorCloseCallback
 ==================
*/
void R_LightEditorCloseCallback (){

	if (!r_lightEditor.active)
		return;			// Not active

	r_lightEditor.active = false;

	r_lightEditor.focusLight = NULL;
	r_lightEditor.editLight = NULL;

	// Close the editor
	Com_CloseEditor();
}

/*
 ==================
 
 ==================
*/
static bool R_LightEditorMouseEvent (){

	lightData_t		*lightData;
	lightParms_t	parms;

	if (!r_lightEditor.active)
		return false;	// Not active

	if (!r_lightEditor.focusLight)
		return false;	// No light has focus

	// Edit the light that has focus
	lightData = r_lightEditor.editLight = r_lightEditor.focusLight;

	// TODO: copy lightData_t parms into lightParms_t

	WIN_EditLightParameters(lightData->index, &parms);

	return true;
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
 R_AddLight_f
 ==================
*/
static void R_AddLight_f (){

	lightData_t		*lightData;
	lightParms_t	parms;
	vec3_t			origin;

	if (!r_lightEditor.active){
		Com_Printf("You must launch the light editor to use this command\n");
		return;
	}

	// Add a new light with default parameters in front of the view
	if (r_numStaticLights == MAX_STATIC_LIGHTS){
		Com_Printf("Too many lights!\n");
		return;
	}

	lightData = &r_staticLights[r_numStaticLights++];	
	
	VectorMA(rg.renderView.origin, 100.0f, rg.renderView.axis[0], origin);
	SnapVector(origin);

	// TODO: set the light data

	// Edit the new light
	r_lightEditor.editLight = lightData;

	Str_Copy(parms.name, "light", sizeof(parms.name));

	parms.type = RL_POINT;

	VectorCopy(origin, parms.origin);
	VectorClear(parms.center);
	VectorClear(parms.angles);

	VectorSet(parms.radius, 100.0f, 100.0f, 100.0f);

	parms.xMin = 1.0f;
	parms.xMax = 1.0f;

	parms.yMin = 1.0f;
	parms.yMax = 1.0f;

	parms.zNear = 1.0f;
	parms.zFar = 1.0f;

	parms.noShadows = false;

	parms.fogDistance = 500.0f;
	parms.fogHeight = 500.0f;

	parms.detailLevel = 0;

	Str_Copy(parms.material, "_defaultLight", sizeof(parms.material));

	parms.materialParms[0] = 1.0f;
	parms.materialParms[1] = 1.0f;
	parms.materialParms[2] = 1.0f;
	parms.materialParms[3] = 1.0f;
	parms.materialParms[4] = 0.0f;
	parms.materialParms[5] = 0.0f;
	parms.materialParms[6] = 0.0f;
	parms.materialParms[7] = 0.0f;

	WIN_EditLightParameters(r_numStaticLights, &parms);
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