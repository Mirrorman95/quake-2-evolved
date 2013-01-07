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
// r_postProcess.c - Post-Processing effects editor
//


#include "r_local.h"


#define POST_PROCESS_EQUAL_EPSILON	0.00001f

static postProcessParms_t	r_postProcessParmsGeneric = {1.0f, 0.5f, 1.0f, 1.0f, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {0.0f, 0.0f, 0.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {1.0f, 1.0f, 1.0f}, {0}, "_default"};


/*
 ==================
 R_ComparePostProcess
 ==================
*/
static bool R_ComparePostProcess (const postProcessParms_t *postProcessParms1, const postProcessParms_t *postProcessParms2){

	if (FAbs(postProcessParms1->bloomContrast - postProcessParms2->bloomContrast) > POST_PROCESS_EQUAL_EPSILON)
		return false;
	if (FAbs(postProcessParms1->bloomThreshold - postProcessParms2->bloomThreshold) > POST_PROCESS_EQUAL_EPSILON)
		return false;
	if (FAbs(postProcessParms1->baseIntensity - postProcessParms2->baseIntensity) > POST_PROCESS_EQUAL_EPSILON)
		return false;
	if (FAbs(postProcessParms1->glowIntensity - postProcessParms2->glowIntensity) > POST_PROCESS_EQUAL_EPSILON)
		return false;
	if (!VectorCompareEpsilon(postProcessParms1->colorShadows, postProcessParms2->colorShadows, POST_PROCESS_EQUAL_EPSILON))
		return false;
	if (!VectorCompareEpsilon(postProcessParms1->colorHighlights, postProcessParms2->colorHighlights, POST_PROCESS_EQUAL_EPSILON))
		return false;
	if (!VectorCompareEpsilon(postProcessParms1->colorMidtones, postProcessParms2->colorMidtones, POST_PROCESS_EQUAL_EPSILON))
		return false;
	if (!VectorCompareEpsilon(postProcessParms1->colorMinOutput, postProcessParms2->colorMinOutput, POST_PROCESS_EQUAL_EPSILON))
		return false;
	if (!VectorCompareEpsilon(postProcessParms1->colorMaxOutput, postProcessParms2->colorMaxOutput, POST_PROCESS_EQUAL_EPSILON))
		return false;
	if (!VectorCompareEpsilon(postProcessParms1->colorSaturation, postProcessParms2->colorSaturation, POST_PROCESS_EQUAL_EPSILON))
		return false;
	if (!VectorCompareEpsilon(postProcessParms1->colorTint, postProcessParms2->colorTint, POST_PROCESS_EQUAL_EPSILON))
		return false;
	if (Str_ICompare(postProcessParms1->colorTableName, postProcessParms2->colorTableName))
		return false;

	return true;
}

/*
 ==================
 R_UpdatePostProcess
 ==================
*/
void R_UpdatePostProcess (){

	postProcessParms_t	*postProcessParms;
	postProcessParms_t	*previous, *current;
	float				frac;

	if (!r_postProcess->integerValue)
		return;

	if (!rg.viewParms.primaryView || rg.viewParms.viewType != VIEW_MAIN)
		return;

	// Update post-process parameters
	if (rg.viewArea == -1)
		postProcessParms = &rg.postProcess.postProcessGeneric;
	else
		postProcessParms = &rg.postProcess.postProcessList[rg.viewArea];

	if (!rg.postProcess.current){
		rg.postProcess.time = rg.renderView.time;

		Mem_Copy(&rg.postProcess.previous, postProcessParms, sizeof(postProcessParms_t));
		rg.postProcess.current = postProcessParms;
	}
	else if (rg.postProcess.current != postProcessParms){
		if (R_ComparePostProcess(rg.postProcess.current, postProcessParms))
			rg.postProcess.current = postProcessParms;
		else {
			rg.postProcess.time = rg.renderView.time;

			Mem_Copy(&rg.postProcess.previous, &rg.postProcess.postProcessParms, sizeof(postProcessParms_t));
			rg.postProcess.current = postProcessParms;
		}
	}

	// Interpolate
	postProcessParms = &rg.postProcess.postProcessParms;

	previous = &rg.postProcess.previous;
	current = rg.postProcess.current;

	if (!r_postProcessTime->floatValue)
		frac = 1.0f;
	else
		frac = MS2SEC(rg.renderView.time - rg.postProcess.time) / r_postProcessTime->floatValue;

	if (frac <= 0.0f)
		Mem_Copy(postProcessParms, previous, sizeof(postProcessParms_t));
	else if (frac >= 1.0f)
		Mem_Copy(postProcessParms, current, sizeof(postProcessParms_t));
	else {
		postProcessParms->bloomContrast = LerpFast(previous->bloomContrast, current->bloomContrast, frac);
		postProcessParms->bloomThreshold = LerpFast(previous->bloomThreshold, current->bloomThreshold, frac);
		postProcessParms->baseIntensity = LerpFast(previous->baseIntensity, current->baseIntensity, frac);
		postProcessParms->glowIntensity = LerpFast(previous->glowIntensity, current->glowIntensity, frac);
		VectorLerpFast(previous->colorShadows, current->colorShadows, frac, postProcessParms->colorShadows);
		VectorLerpFast(previous->colorHighlights, current->colorHighlights, frac, postProcessParms->colorHighlights);
		VectorLerpFast(previous->colorMidtones, current->colorMidtones, frac, postProcessParms->colorMidtones);
		VectorLerpFast(previous->colorMinOutput, current->colorMinOutput, frac, postProcessParms->colorMinOutput);
		VectorLerpFast(previous->colorMaxOutput, current->colorMaxOutput, frac, postProcessParms->colorMaxOutput);
		VectorLerpFast(previous->colorSaturation, current->colorSaturation, frac, postProcessParms->colorSaturation);
		VectorLerpFast(previous->colorTint, current->colorTint, frac, postProcessParms->colorTint);

		// Blend the color tables
		R_BlendColorTables(postProcessParms->colorTable, previous->colorTable, current->colorTable, frac);
	}

	// Update the color table texture
	R_UploadTextureImage(rg.colorTableTexture, 0, (const byte *)postProcessParms->colorTable, 256, 1);
}


/*
 ==============================================================================

 CALLBACK FUNCTIONS

 ==============================================================================
*/


/*
 ==================
 R_EditAreaPostProcess
 ==================
*/
void R_EditAreaPostProcess (int area){

	if (!rg.postProcess.editing)
		return;		// Not active

	if (rg.postProcess.editingArea == area)
		return;		// Area hasn't changed
	rg.postProcess.editingArea = area;

	// Edit the specified area post-process parameters
	if (area == -1){
		WIN_EditPostProcessParameters(-1, NULL);
		return;
	}

	WIN_EditPostProcessParameters(area, &rg.postProcess.postProcessList[area]);
}

/*
 ==================
 R_PostProcessEditorUpdateCallback
 ==================
*/
void R_PostProcessEditorUpdateCallback (int index, postProcessParms_t *parms){

	postProcessParms_t	*postProcessParms;
	int					i;

	if (!rg.postProcess.editing)
		return;		// Not active

	if (index < 0 || index >= CM_NumAreas())
		return;		// Out of range

	postProcessParms = &rg.postProcess.postProcessList[index];

	// Copy the parameters
	Mem_Copy(postProcessParms, parms, sizeof(postProcessParms_t));

	// Clamp the parameters
	postProcessParms->bloomContrast = ClampFloat(postProcessParms->bloomContrast, 0.0f, 4.0f);
	postProcessParms->bloomThreshold = ClampFloat(postProcessParms->bloomThreshold, 0.0f, 1.0f);
	postProcessParms->baseIntensity = ClampFloat(postProcessParms->baseIntensity, 0.0f, 4.0f);
	postProcessParms->glowIntensity = ClampFloat(postProcessParms->glowIntensity, 0.0f, 4.0f);
	VectorClamp(0.0f, 1.0f, postProcessParms->colorShadows);
	VectorClamp(0.0f, 1.0f, postProcessParms->colorHighlights);
	VectorClamp(0.1f, 9.99f, postProcessParms->colorMidtones);
	VectorClamp(0.0f, 1.0f, postProcessParms->colorMinOutput);
	VectorClamp(0.0f, 1.0f, postProcessParms->colorMaxOutput);
	VectorClamp(0.0f, 4.0f, postProcessParms->colorSaturation);
	VectorClamp(0.0f, 1.0f, postProcessParms->colorTint);

	for (i = 0; i < 3; i++){
		if (postProcessParms->colorShadows[i] >= postProcessParms->colorHighlights[i]){
			postProcessParms->colorShadows[i] = 0.0f;
			postProcessParms->colorHighlights[i] = 1.0f;
		}

		if (postProcessParms->colorMinOutput[i] >= postProcessParms->colorMaxOutput[i]){
			postProcessParms->colorMinOutput[i] = 0.0f;
			postProcessParms->colorMaxOutput[i] = 1.0f;
		}
	}

	// Load the color table
	R_LoadColorTable(postProcessParms->colorTableName, postProcessParms->colorTable);

	// Force an update
	rg.postProcess.time = 0;

	Mem_Copy(&rg.postProcess.previous, &r_postProcessParmsGeneric, sizeof(postProcessParms_t));
	rg.postProcess.current = NULL;
}

/*
 ==================
 R_PostProcessEditorSaveCallback
 ==================
*/
void R_PostProcessEditorSaveCallback (){

	fileHandle_t		f;
	postProcessParms_t	*postProcessParms;
	int					i;

	if (!rg.postProcess.editing)
		return;		// Not active

	if (!rg.postProcess.postProcessName[0]){
		Com_Printf("No post-process file name available\n");
		return;
	}

	// Write the post-process file
	FS_OpenFile(rg.postProcess.postProcessName, FS_WRITE, &f);
	if (!f){
		Com_Printf("Couldn't write post-process file %s\n", rg.postProcess.postProcessName);
		return;
	}

	for (i = 0, postProcessParms = rg.postProcess.postProcessList; i < CM_NumAreas(); i++, postProcessParms++){
		FS_Printf(f, "postProcess %i" NEWLINE, i);
		FS_Printf(f, "{" NEWLINE);
		FS_Printf(f, "bloomContrast       %g" NEWLINE, postProcessParms->bloomContrast);
		FS_Printf(f, "bloomThreshold      %g" NEWLINE, postProcessParms->bloomThreshold);
		FS_Printf(f, "baseIntensity       %g" NEWLINE, postProcessParms->baseIntensity);
		FS_Printf(f, "glowIntensity       %g" NEWLINE, postProcessParms->glowIntensity);
		FS_Printf(f, "colorShadows        ( %g %g %g )" NEWLINE, postProcessParms->colorShadows[0], postProcessParms->colorShadows[1], postProcessParms->colorShadows[2]);
		FS_Printf(f, "colorHighlights     ( %g %g %g )" NEWLINE, postProcessParms->colorHighlights[0], postProcessParms->colorHighlights[1], postProcessParms->colorHighlights[2]);
		FS_Printf(f, "colorMidtones       ( %g %g %g )" NEWLINE, postProcessParms->colorMidtones[0], postProcessParms->colorMidtones[1], postProcessParms->colorMidtones[2]);
		FS_Printf(f, "colorMinOutput      ( %g %g %g )" NEWLINE, postProcessParms->colorMinOutput[0], postProcessParms->colorMinOutput[1], postProcessParms->colorMinOutput[2]);
		FS_Printf(f, "colorMaxOutput      ( %g %g %g )" NEWLINE, postProcessParms->colorMaxOutput[0], postProcessParms->colorMaxOutput[1], postProcessParms->colorMaxOutput[2]);
		FS_Printf(f, "colorSaturation     ( %g %g %g )" NEWLINE, postProcessParms->colorSaturation[0], postProcessParms->colorSaturation[1], postProcessParms->colorSaturation[2]);
		FS_Printf(f, "colorTint           ( %g %g %g )" NEWLINE, postProcessParms->colorTint[0], postProcessParms->colorTint[1], postProcessParms->colorTint[2]);
		FS_Printf(f, "colorTable          %s" NEWLINE, postProcessParms->colorTableName);
		FS_Printf(f, "}" NEWLINE);

		if (i < CM_NumAreas() - 1)
			FS_Printf(f, NEWLINE);
	}

	FS_CloseFile(f);

	Com_Printf("Wrote post-process file %s with %i areas\n", rg.postProcess.postProcessName, CM_NumAreas());
}

/*
 ==================
R_PostProcessEditorCloseCallback
 ==================
*/
void R_PostProcessEditorCloseCallback (){

	if (!rg.postProcess.editing)
		return;		// Not active

	rg.postProcess.editing = false;
	rg.postProcess.editingArea = -1;

	// Close the post-process editor
	Com_CloseEditor();
}


/*
 ==============================================================================

 CONSOLE COMMANDS

 ==============================================================================
*/


/*
 ==================
 R_EditPostProcess_f
 ==================
*/
static void R_EditPostProcess_f (){

	editorCallbacks_t	callbacks;

	if (!r_postProcess->integerValue){
		Com_Printf("You must enable post-process effects to launch the post-process editor\n");
		return;
	}

	// Launch the post-process editor
	callbacks.createWindow = WIN_CreatePostProcessEditorWindow;
	callbacks.destroyWindow = WIN_DestroyPostProcessEditorWindow;
	callbacks.mouseEvent = NULL;

	if (!Com_LaunchEditor("post-process", &callbacks))
		return;

	rg.postProcess.editing = true;
	rg.postProcess.editingArea = rg.viewArea;

	// Edit the current area post-process parameters
	if (rg.viewArea == -1){
		WIN_EditPostProcessParameters(-1, NULL);
		return;
	}

	WIN_EditPostProcessParameters(rg.viewArea, &rg.postProcess.postProcessList[rg.viewArea]);
}


/*
 ==============================================================================

 INITIALIZATION AND SHUTDOWN

 ==============================================================================
*/


/*
 ==================
 R_InitPostProcessEditor
 ==================
*/
void R_InitPostProcessEditor (){

	int		i;

	// Add commands
	Cmd_AddCommand("editPostProcess", R_EditPostProcess_f, "Launches the integrated post-process editor", NULL);

	// Load the default color table
	R_LoadColorTable(r_postProcessParmsGeneric.colorTableName, r_postProcessParmsGeneric.colorTable);

	// Fill in defaults
	Mem_Copy(&rg.postProcess.postProcessGeneric, &r_postProcessParmsGeneric, sizeof(postProcessParms_t));

	for (i = 0; i < MAX_POST_PROCESS_AREAS; i++)
		Mem_Copy(&rg.postProcess.postProcessList[i], &r_postProcessParmsGeneric, sizeof(postProcessParms_t));

	Mem_Copy(&rg.postProcess.postProcessParms, &r_postProcessParmsGeneric, sizeof(postProcessParms_t));

	// Clear interpolation parameters
	rg.postProcess.time = 0;

	Mem_Copy(&rg.postProcess.previous, &r_postProcessParmsGeneric, sizeof(postProcessParms_t));
	rg.postProcess.current = NULL;
}

/*
 ==================
 R_ShutdownPostProcessEditor
 ==================
*/
void R_ShutdownPostProcessEditor (){

	// Remove commands
	Cmd_RemoveCommand("editPostProcess");

	// Close the post-process editor if active
	if (rg.postProcess.editing)
		Com_CloseEditor();
}