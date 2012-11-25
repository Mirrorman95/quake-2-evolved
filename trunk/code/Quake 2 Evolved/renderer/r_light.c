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
// r_light.c - Light management
//


#include "r_local.h"


#define MAX_STATIC_LIGHTS			4096

typedef struct {
	const char *			name;
} lightSource_t;

typedef struct {
	bool					active;

	lightSource_t *			focusLight;
	lightSource_t *			editLight;
} lightEditor_t;

static lightSource_t		r_staticLights[MAX_STATIC_LIGHTS];
static int					r_numStaticLights;

static lightEditor_t		r_lightEditor;


/*
 ==============================================================================

 LIGHT PARSING

 ==============================================================================
*/


/*
 ==================
 
 ==================
*/
static bool R_ParseLight (script_t *script){

	token_t			token;
	lightSource_t	*lightSource;
	int				number;

	// Parse the index number
	if (!PS_ReadInteger(script, &number)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing index number in light file\n");
		return false;
	}

	if (number < 0 || number >= MAX_STATIC_LIGHTS){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid value of %i for index number in light file\n", number);
		return false;
	}

	// Parse the light parameters
	lightSource = &r_staticLights[r_numStaticLights++];

	if (!PS_ExpectTokenString(script, &token, "{", true)){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected '{', found '%s' instead in light file (number %i)\n", token.string, number);
		return false;
	}

	while (1){
		if (!PS_ReadToken(script, &token)){
			Com_Printf(S_COLOR_YELLOW "WARNING: no concluding '}' in light file (number %i)\n", number);
			return false;	// End of script
		}

		if (!Str_ICompare(token.string, "}"))
			break;			// End of light

		// Parse the parameter
	}

	return true;
}

/*
 ==================
 
 ==================
*/
void R_LoadLights (const char *name){

	script_t	*script;
	token_t		token;

	// Copy the name

	// Load the script file
	script = PS_LoadScriptFile(name);
	if (!script){
		Com_Printf(S_COLOR_RED "Light file %s not found\n", name);
		return;
	}

	PS_SetScriptFlags(script, SF_NOWARNINGS | SF_NOERRORS | SF_ALLOWPATHNAMES);

	// Parse it
	while (1){
		if (!PS_ReadToken(script, &token))
			break;		// End of script

		if (!Str_ICompare(token.string, "light")){
			if (!R_ParseLight(script))
				break;
		}
		else {
			Com_Printf(S_COLOR_YELLOW "WARNING: expected 'light', found '%s' instead in light file\n", token.string);
			break;
		}
	}

	// Free the script file
	PS_FreeScript(script);
}


// ============================================================================


/*
 ==================
 
 ==================
*/
static void R_AddLight (){

}

/*
 ==================
 
 ==================
*/
static void R_AddLights (){

	if (r_skipLights->integerValue)
		return;

	// Add all the lights
}


// ============================================================================


/*
 ==================
 R_AllocLights
 ==================
*/
void R_AllocLights (){

	rg.maxLights[0] = MAX_LIGHTS;
	rg.maxLights[1] = MAX_LIGHTS >> 4;
	rg.maxLights[2] = MAX_LIGHTS >> 4;
	rg.maxLights[3] = MAX_LIGHTS >> 4;

	rg.lights[0] = (light_t *)Mem_ClearedAlloc(rg.maxLights[0] * sizeof(light_t), TAG_RENDERER);
	rg.lights[1] = (light_t *)Mem_ClearedAlloc(rg.maxLights[1] * sizeof(light_t), TAG_RENDERER);
	rg.lights[2] = (light_t *)Mem_ClearedAlloc(rg.maxLights[2] * sizeof(light_t), TAG_RENDERER);
	rg.lights[3] = (light_t *)Mem_ClearedAlloc(rg.maxLights[3] * sizeof(light_t), TAG_RENDERER);
}

/*
 ==================
 R_GenerateLights
 ==================
*/
void R_GenerateLights (){

	// Add the draw lights
	R_AddLights();

	// Set up the draw lights
	rg.viewParms.numLights[0] = rg.numLights[0] - rg.firstLight[0];
	rg.viewParms.numLights[1] = rg.numLights[1] - rg.firstLight[1];
	rg.viewParms.numLights[2] = rg.numLights[2] - rg.firstLight[2];
	rg.viewParms.numLights[3] = rg.numLights[3] - rg.firstLight[3];

	rg.viewParms.lights[0] = &rg.lights[0][rg.firstLight[0]];
	rg.viewParms.lights[1] = &rg.lights[1][rg.firstLight[1]];
	rg.viewParms.lights[2] = &rg.lights[2][rg.firstLight[2]];
	rg.viewParms.lights[3] = &rg.lights[3][rg.firstLight[3]];

	rg.pc.lights += rg.viewParms.numLights[0] + rg.viewParms.numLights[1] + rg.viewParms.numLights[2] + rg.viewParms.numLights[3];

	// The next view rendered in this frame will tack on after this one
	rg.firstLight[0] = rg.numLights[0];
	rg.firstLight[1] = rg.numLights[1];
	rg.firstLight[2] = rg.numLights[2];
	rg.firstLight[3] = rg.numLights[3];
}

/*
 ==================
 R_ClearLights
 ==================
*/
void R_ClearLights (){

	rg.numLights[0] = rg.firstLight[0] = 0;
	rg.numLights[1] = rg.firstLight[1] = 0;
	rg.numLights[2] = rg.firstLight[2] = 0;
	rg.numLights[3] = rg.firstLight[3] = 0;
}


/*
 ==============================================================================

 CONSOLE COMMANDS

 ==============================================================================
*/


/*
 ==================
 
 ==================
*/
static void R_EditLights_f (){

}

/*
 ==================
 
 ==================
*/
static void R_AddLight_f (){

	lightSource_t	*lightSource;
	lightParms_t	parms;
	vec3_t			origin;

	if (!r_lightEditor.active){
		Com_Printf("You must launch the light editor to use this command\n");
		return;
	}

	if (r_numStaticLights == MAX_STATIC_LIGHTS){
		Com_Printf("Too many lights!\n");
		return;
	}

	lightSource = &r_staticLights[r_numStaticLights++];

	// Add a new light with default parameters in front of the view
	VectorMA(rg.renderView.origin, 100.0f, rg.renderView.axis[0], origin);
	SnapVector(origin);

	// Edit the new light
}

/*
 ==================
 
 ==================
*/
static void R_SaveLights_f (){

}

/*
 ==================
 
 ==================
*/
static void R_ClearLights_f (){

}


/*
 ==============================================================================

 INITIALIZATION AND SHUTDOWN

 ==============================================================================
*/


/*
 ==================
 R_InitLights
 ==================
*/
void R_InitLights (){

	// Add commands
	Cmd_AddCommand("editLights", R_EditLights_f, "Launches the integrated light editor", NULL);
	Cmd_AddCommand("addLight", R_AddLight_f, "Adds a new static light", NULL);
	Cmd_AddCommand("saveLight", R_SaveLights_f, "Saves all static lights", NULL);
	Cmd_AddCommand("clearLights", R_SaveLights_f, "Clears all static lights", NULL);
}

/*
 ==================
 R_ShutdownLights
 ==================
*/
void R_ShutdownLights (){

	// Remove commands
	Cmd_RemoveCommand("editLights");
	Cmd_RemoveCommand("addLight");
	Cmd_RemoveCommand("saveLight");
	Cmd_RemoveCommand("clearLights");

	// Close the light editor if active
	if (r_lightEditor.active)
		Com_CloseEditor();

	Mem_Fill(&r_lightEditor, 0, sizeof(lightEditor_t));
}