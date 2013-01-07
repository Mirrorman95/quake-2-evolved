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

// TODO:
// - light index is removed or something in the light editor code
// - drawing


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

	lightData_t	*visibleLights[MAX_STATIC_LIGHTS];
	int			numVisibleLights;
	lightData_t *lightData;
	trace_t		trace;
	vec3_t		origin, textOrigin, forward, angles, axis[3];
	vec3_t		tmp, tmp2;
	vec3_t		mins = {-5.0f, -5.0f, -5.0f}, maxs = {5.0f, 5.0f, 5.0f};
	char		string[MAX_STRING_LENGTH];
	float		fraction = 1.0f;
	int			headNode;
	int			i;

	if (!r_lightEditor.active)
		return;			// Not active

	r_lightEditor.focusLight = NULL;

	// Get the current view origin, forward vector, and angles
	VectorCopy(rg.renderView.origin, origin);
	VectorMA(origin, 8192.0f, rg.renderView.axis[0], forward);
	VectorClear(angles);

	// Find visible lights
	numVisibleLights = 0;

	for (i = 0, lightData = rg.staticLights; i < rg.numStaticLights; i++, lightData++){
		// Always add the light that is being edited
		if (lightData == r_lightEditor.editLight){
			visibleLights[numVisibleLights++] = lightData;
			continue;
		}

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
	headNode = CM_SetupBoxModel(mins, maxs);

	for (i = 0; i < numVisibleLights; i++){
		lightData = visibleLights[i];

		trace = CM_TransformedBoxTrace(origin, forward, vec3_origin, vec3_origin, headNode, MASK_ALL, lightData->origin, angles);

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
			// If a point or cubic light, draw a box at the center
			if (lightData->type == RL_POINT || lightData->type == RL_CUBIC){
				ClearBounds(mins, maxs);

				VectorSet(mins, -1.0f, -1.0f, -1.0f);
				VectorSet(maxs, 1.0f, 1.0f, 1.0f);

				// TODO: origin = axis * center + origin ?
				// or origin = axis * center

				R_DebugBox(colorRed, origin, axis, mins, maxs, false, VIEW_MAIN);
			}

			// If a projected light, draw an arrow pointing to the far plane
			if (lightData->type == RL_PROJECTED){

			}

			// If a directional light, draw an arrow pointing in the direction
			// of the light
			if (lightData->type == RL_DIRECTIONAL){

			}
		}

		// Draw the light and material names
		if (!lightData->material)
			Str_SPrintf(string, sizeof(string), "%s", lightData->name);
		else
			Str_SPrintf(string, sizeof(string), "%s (%s)", lightData->name, lightData->material->name);

		VectorMA(origin, 10.0f, forward, textOrigin);

//		R_DebugText(colorWhite, true, textOrigin, 2.0f, 4.0f, string, false, VIEW_MAIN);
	}
}


/*
 ==============================================================================

 CALLBACK FUNCTIONS

 ==============================================================================
*/


/*
 ==================
 R_LightEditorUpdateCallback
 ==================
*/
void R_LightEditorUpdateCallback (int index, lightParms_t *parms){

	lightData_t	*lightData;

	if (!r_lightEditor.active)
		return;			// Not active

	if (index < 0 || index >= MAX_STATIC_LIGHTS)
		return;			// Out of range

	lightData = &rg.staticLights[index];

	lightData->valid = false;

	// Copy the parameters and clamp as needed
	lightData->parms = *parms;

	lightData->parms.type = ClampInt(parms->type, 0, 3);

	lightData->parms.radius[0] = Max(parms->radius[0], 1.0f);
	lightData->parms.radius[1] = Max(parms->radius[1], 1.0f);
	lightData->parms.radius[2] = Max(parms->radius[2], 1.0f);

	lightData->parms.zNear = Max(parms->zNear, 1.0f);
	lightData->parms.zFar = Max(parms->zFar, 2.0f);

	lightData->parms.fogDistance = Max(parms->fogDistance, 1.0f);
	lightData->parms.fogHeight = Max(parms->fogHeight, 1.0f);

	lightData->parms.detailLevel = ClampInt(parms->detailLevel, 0, 2);

	// Validate
	if (lightData->parms.type == RL_DIRECTIONAL && lightData->parms.center[0] != 0.0f || lightData->parms.center[1] != 0.0f || lightData->parms.center[2] != 0.0f)
		VectorSet(lightData->parms.center, 0.0f, 0.0f, 1.0f);

	if (lightData->parms.xMin >= lightData->parms.xMax){
		lightData->parms.xMin = -0.5f;
		lightData->parms.xMax = 0.5f;
	}

	if (lightData->parms.yMin >= lightData->parms.yMax){
		lightData->parms.yMin = -0.5f;
		lightData->parms.yMax = 0.5f;
	}

	if (lightData->parms.zNear >= lightData->parms.zFar){
		lightData->parms.zNear = 1.0f;
		lightData->parms.zFar = 200.0f;
	}
}

/*
 ==================
 R_LightEditorRemoveCallback
 ==================
*/
void R_LightEditorRemoveCallback (int index){

	lightData_t	*lightData;
	int			i, j;

	if (!r_lightEditor.active)
		return;			// Not active

	// Remove the light
	for (i = 0, lightData = rg.staticLights; i < rg.numStaticLights; i++, lightData++){
		if (lightData != r_lightEditor.editLight)
			continue;

		for (j = i; j < rg.numStaticLights - 1; j++)
			Mem_Copy(&rg.staticLights[j], &rg.staticLights[j+1], sizeof(lightData_t));

		rg.numStaticLights--;
		break;
	}

	for (i = 0, lightData = rg.staticLights; i < rg.numStaticLights; i++, lightData++)
		lightData->index = i;

	// Force a reset
	r_lightEditor.focusLight = NULL;
	r_lightEditor.editLight = NULL;
}

/*
 ==================
 R_LightEditorSaveCallback
 ==================
*/
void R_LightEditorSaveCallback (){

	fileHandle_t	f;
	lightData_t		*lightData;
	int				i;

	if (!r_lightEditor.active)
		return;			// Not active

	if (!rg.numStaticLights){
		Com_Printf("Could not find any static lights\n");
		return;
	}

	// Write the light file
	FS_OpenFile(rg.staticLights->fileName, FS_WRITE, &f);
	if (!f){
		Com_Printf("Couldn't write light file %s\n", rg.staticLights->fileName);
		return;
	}

	for (i = 0, lightData = rg.staticLights; i < rg.numStaticLights; i++, lightData++){
		FS_Printf(f, "light %i" NEWLINE, i);
		FS_Printf(f, "{" NEWLINE);

		FS_Printf(f, "name                %s" NEWLINE, lightData->parms.name);

		if (lightData->parms.type != RL_POINT)
			FS_Printf(f, "type                %g" NEWLINE, lightData->parms.type);

		FS_Printf(f, "origin              ( %g %g %g )" NEWLINE, lightData->parms.origin[0], lightData->parms.origin[1], lightData->parms.origin[2]);

		if (lightData->parms.center[0] != 0.0f || lightData->parms.center[1] != 0.0f || lightData->parms.center[2] != 0.0f)
			FS_Printf(f, "center              ( %g %g %g )" NEWLINE, lightData->parms.center[0], lightData->parms.center[1], lightData->parms.center[2]);

		if (lightData->parms.angles[0] != 0.0f || lightData->parms.angles[1] != 0.0f || lightData->parms.angles[2] != 0.0f)
			FS_Printf(f, "angles              ( %g %g %g )" NEWLINE, lightData->parms.angles[0], lightData->parms.angles[1], lightData->parms.angles[2]);

		if (lightData->parms.radius[0] != 100.0f || lightData->parms.radius[1] != 100.0f || lightData->parms.radius[2] != 100.0f)
			FS_Printf(f, "radius              ( %g %g %g )" NEWLINE, lightData->parms.radius[0], lightData->parms.radius[1], lightData->parms.radius[2]);

		if (lightData->parms.xMin != -0.5f)
			FS_Printf(f, "xMin                %g" NEWLINE, lightData->parms.xMin);

		if (lightData->parms.xMax != 0.5f)
			FS_Printf(f, "xMax                %g" NEWLINE, lightData->parms.xMax);

		if (lightData->parms.yMin != -0.5f)
			FS_Printf(f, "yMin                %g" NEWLINE, lightData->parms.yMin);

		if (lightData->parms.yMax != 0.5f)
			FS_Printf(f, "yMax                %g" NEWLINE, lightData->parms.yMax);

		if (lightData->parms.zNear != 1.0f)
			FS_Printf(f, "zNear               %g" NEWLINE, lightData->parms.zNear);

		if (lightData->parms.zFar != 200.0f)
			FS_Printf(f, "zFar                %g" NEWLINE, lightData->parms.zFar);

		if (lightData->parms.noShadows)
			FS_Printf(f, "noShadows           %g" NEWLINE, lightData->parms.noShadows);

		if (lightData->parms.fogDistance != 500.0f)
			FS_Printf(f, "fogDistance         %g" NEWLINE, lightData->parms.fogDistance);

		if (lightData->parms.fogHeight != 500.0f)
			FS_Printf(f, "fogHeight           %g" NEWLINE, lightData->parms.fogHeight);

		if (lightData->parms.style != 0)
			FS_Printf(f, "style               %g" NEWLINE, lightData->parms.style);

		if (lightData->parms.detailLevel)
			FS_Printf(f, "detailLevel         %g" NEWLINE, lightData->parms.detailLevel);

		if (lightData->parms.material)
			FS_Printf(f, "material            %s" NEWLINE, lightData->parms.material);

		if (lightData->parms.materialParms[MATERIALPARM_RED] != 1.0f || lightData->parms.materialParms[MATERIALPARM_GREEN] != 1.0f || lightData->parms.materialParms[MATERIALPARM_BLUE] != 1.0f)
			FS_Printf(f, "color               ( %g %g %g )" NEWLINE, lightData->parms.materialParms[MATERIALPARM_RED], lightData->parms.materialParms[MATERIALPARM_GREEN], lightData->parms.materialParms[MATERIALPARM_BLUE]);

		if (lightData->parms.materialParms[MATERIALPARM_ALPHA] != 1.0f)
			FS_Printf(f, "alpha               %g" NEWLINE, lightData->parms.materialParms[MATERIALPARM_ALPHA]);

		FS_Printf(f, "}" NEWLINE);
	}

	FS_CloseFile(f);

	Com_Printf("Wrote light file %s with %i lights\n", rg.staticLights->fileName, rg.numStaticLights);
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
 R_LightEditorMouseEvent
 ==================
*/
static bool R_LightEditorMouseEvent (){

	lightData_t	*lightData;

	if (!r_lightEditor.active)
		return false;	// Not active

	if (!r_lightEditor.focusLight)
		return false;	// No light has focus

	// Edit the light that has focus
	lightData = r_lightEditor.editLight = r_lightEditor.focusLight;

	WIN_EditLightParameters(lightData->index, &lightData->parms);

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

	lightData_t	*lightData;
	vec3_t		origin;

	if (!r_lightEditor.active){
		Com_Printf("You must launch the light editor to use this command\n");
		return;
	}

	// Add a new light with default parameters in front of the view
	if (rg.numStaticLights == MAX_STATIC_LIGHTS){
		Com_Printf("R_AddLight_f: MAX_STATIC_LIGHTS hit\n");
		return;
	}

	lightData = &rg.staticLights[rg.numStaticLights++];

	VectorMA(rg.renderView.origin, 100.0f, rg.renderView.axis[0], origin);
	SnapVector(origin);

	// Edit the new light
	r_lightEditor.editLight = lightData;

	Str_SPrintf(lightData->parms.name, sizeof(lightData->parms.name), "light_%i", rg.numStaticLights - 1);

	lightData->parms.index = rg.numStaticLights - 1;

	lightData->parms.type = RL_POINT;

	VectorCopy(origin, lightData->parms.origin);
	VectorClear(lightData->parms.center);
	VectorClear(lightData->parms.angles);

	VectorSet(lightData->parms.radius, 100.0f, 100.0f, 100.0f);

	lightData->parms.xMin = 0.0f;
	lightData->parms.xMax = 0.0f;

	lightData->parms.yMin = 0.0f;
	lightData->parms.yMax = 0.0f;

	lightData->parms.zFar = 0.0f;
	lightData->parms.zNear = 0.0f;

	lightData->parms.noShadows = false;

	lightData->parms.fogDistance = 500.0f;
	lightData->parms.fogHeight = 500.0f;

	lightData->parms.style = 0;
	lightData->parms.detailLevel = 0;

	Str_Copy(lightData->parms.material, "lights/default", sizeof(lightData->parms.material));

	lightData->parms.materialParms[MATERIALPARM_RED] = 1.0f;
	lightData->parms.materialParms[MATERIALPARM_GREEN] = 1.0f;
	lightData->parms.materialParms[MATERIALPARM_BLUE] = 1.0f;
	lightData->parms.materialParms[MATERIALPARM_ALPHA] = 1.0f;
	lightData->parms.materialParms[MATERIALPARM_TIMEOFFSET] = 0.0f;
	lightData->parms.materialParms[MATERIALPARM_DIVERSITY] = 0.0f;
	lightData->parms.materialParms[MATERIALPARM_MISC] = 0.0f;
	lightData->parms.materialParms[MATERIALPARM_MODE] = 0.0f;

	WIN_EditLightParameters(lightData->parms.index, &lightData->parms);
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