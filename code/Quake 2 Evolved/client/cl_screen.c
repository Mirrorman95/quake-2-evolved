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
// cl_screen.c - Screen updating
//


#include "client.h"


/*
 ==============================================================================

 CINEMATIC FUNCTIONS

 ==============================================================================
*/


/*
 ==================
 CL_PlayCinematic
 ==================
*/
void CL_PlayCinematic (const char *name){

	char	path[MAX_PATH_LENGTH];

	// Stop the cinematic
	CL_StopCinematic();

	Com_DPrintf("CL_PlayCinematic( %s )\n", name);

	// Get the extension name
	if (!Str_ICompare(name+Str_Length(name)-4, ".pcx")){
		Str_Copy(path, name, sizeof(path));
		Com_DefaultPath(path, sizeof(path), "pics");
	}
	else {
		Str_Copy(path, name, sizeof(path));
		Com_DefaultPath(path, sizeof(path), "video");
		Com_DefaultExtension(path, sizeof(path), ".cin");
	}

	// Play the cinematic
	cls.cinematicHandle = CIN_PlayCinematic(path, CIN_SYSTEM);
}

/*
 ==================
 CL_StopCinematic
 ==================
*/
void CL_StopCinematic (){

	if (!cls.playingCinematic)
		return;

	Com_DPrintf("CL_StopCinematic()\n");

	CIN_StopCinematic(cls.cinematicHandle);
	cls.cinematicHandle = 0;
}

/*
 ==================
 CL_FinishCinematic

 Called when either the cinematic completes, or it is aborted
 ==================
*/
void CL_FinishCinematic (){

	Com_DPrintf("CL_FinishCinematic()\n");

	if (cls.state == CA_DISCONNECTED){
		UI_SetActiveMenu(UI_MAINMENU);
		return;
	}

	// Not active anymore, but not disconnected
	cls.state = CA_CONNECTED;

	// Draw the loading screen
	CL_LoadingState();

	// Tell the server to advance to the next map / cinematic
	MSG_WriteByte(&cls.netChan.message, CLC_STRINGCMD);
	MSG_Print(&cls.netChan.message, Str_VarArgs("nextserver %i\n", cl.serverCount));
}

/*
 ==================
 
 ==================
*/
static bool CL_DrawCinematic (){

	if (!cls.playingCinematic)
		return false;

	return true;
}


// ============================================================================


/*
 ==================
 CL_UpdateScreen
 ==================
*/
void CL_UpdateScreen (){

	// Begin a new frame
	R_BeginFrame(cls.realTime);

	switch (cls.state){
	case CA_DISCONNECTED:
		// If playing a cinematic, draw it
		if (CL_DrawCinematic())
			break;

		// Draw the main menu
		UI_UpdateMenu(cls.realTime);

		break;
	case CA_CONNECTING:
	case CA_CHALLENGING:
	case CA_CONNECTED:
	case CA_LOADING:
	case CA_PRIMED:
		// Draw the loading screen and connection information
		CL_DrawLoading();

		break;
	case CA_ACTIVE:
		// Only draw the main menu UI if it covers the entire screen
		if (UI_IsFullscreen()){
			UI_UpdateMenu(cls.realTime);
			break;
		}

		// If playing a cinematic, draw it
		if (CL_DrawCinematic())
			break;

		// Update the in-game view
		CL_RenderActiveFrame();

		// Draw the main menu UI on top of the game view
		UI_UpdateMenu(cls.realTime);

		break;
	}

	// The console will be drawn on top of everything
	Con_Redraw();

	// End the frame
	R_EndFrame();
}