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
// win_input.c - DirectInput and Windows mouse code
//


#include "../client/client.h"
#include "win_local.h"


static bool				in_activeApp;
static bool				in_mLooking;

cvar_t *				in_mouse;
cvar_t *				in_initMouse;


/*
 ==============================================================================

 MOUSE CONTROL

 ==============================================================================
*/

#define MAX_MOUSE_BUTTONS			3

cvar_t *				m_showMouse;
cvar_t *				m_filter;
cvar_t *				m_pitch;
cvar_t *				m_yaw;
cvar_t *				m_forward;
cvar_t *				m_side;
cvar_t *				sensitivity;

typedef struct {
	bool				initialized;
	bool				active;

	int					numButtons;
	int					oldButtonState;
	int					oldMouseX;
	int					oldMouseY;

	int					originalAccel[3];
	bool				restoreAccel;
	int					windowCenterX;
	int					windowCenterY;
} mouse_t;

static mouse_t			mouse;


/*
 ==================
 IN_MLookDown_f
 ==================
*/
static void IN_MLookDown_f (){
	
	in_mLooking = true;
}

/*
 ==================
 IN_MLookUp_f
 ==================
*/
static void IN_MLookUp_f (){

	in_mLooking = false;

	if (!cl_freeLook->integerValue && cl_lookSpring->integerValue)
		CL_CenterView_f();
}

/*
 ==================
 IN_ActivateMouse

 Called when the window gains focus or changes in some way
 ==================
*/
static void IN_ActivateMouse (){

	int		width, height;
	RECT	windowRect;
	int		accel[3] = {0, 0, 1};

	if (!mouse.initialized)
		return;

	if (mouse.active)
		return;
	mouse.active = true;

	width = GetSystemMetrics(SM_CXSCREEN);
	height = GetSystemMetrics(SM_CYSCREEN);

	GetWindowRect(sys.hWndMain, &windowRect);
	if (windowRect.left < 0)
		windowRect.left = 0;
	if (windowRect.top < 0)
		windowRect.top = 0;
	if (windowRect.right >= width)
		windowRect.right = width - 1;
	if (windowRect.bottom >= height)
		windowRect.bottom = height - 1;

	mouse.windowCenterX = (windowRect.right + windowRect.left) / 2;
	mouse.windowCenterY = (windowRect.top + windowRect.bottom) / 2;

	SetCursorPos(mouse.windowCenterX, mouse.windowCenterY);

#ifndef _DEBUG
	if (mouse.restoreAccel)
		SystemParametersInfo(SPI_SETMOUSE, 0, accel, 0);

	ClipCursor(&windowRect);
#endif

	SetCapture(sys.hWndMain);

	while (ShowCursor(FALSE) >= 0)
		;
}

/*
 ==================
 IN_DeactivateMouse

 Called when the window loses focus
 ==================
*/
static void IN_DeactivateMouse (){

	if (!mouse.initialized)
		return;

	if (!mouse.active)
		return;
	mouse.active = false;

	while (ShowCursor(TRUE) < 0)
		;

	ReleaseCapture();

	ClipCursor(NULL);

	if (mouse.restoreAccel)
		SystemParametersInfo(SPI_SETMOUSE, 0, mouse.originalAccel, 0);
}

/*
 ==================
 IN_StartupMouse
 ==================
*/
static void IN_StartupMouse (){

	Mem_Fill(&mouse, 0, sizeof(mouse_t));

	if (!in_initMouse->integerValue){
		Com_Printf("Skipping mouse initialization\n");
		return; 
	}

	if (!in_mouse->integerValue){
		Com_Printf("Mouse is not active\n");
		return;
	}

	if (!GetSystemMetrics(SM_MOUSEPRESENT)){
		Com_Printf(S_COLOR_YELLOW "WARNING: no mouse device installed\n");
		return;
	}

	mouse.numButtons = GetSystemMetrics(SM_CMOUSEBUTTONS);
	if (mouse.numButtons > MAX_MOUSE_BUTTONS)
		mouse.numButtons = MAX_MOUSE_BUTTONS;

	mouse.restoreAccel = SystemParametersInfo(SPI_GETMOUSE, 0, mouse.originalAccel, 0);

	Com_Printf("...mouse detected\n");

	mouse.initialized = true;

	IN_ActivateMouse();
}

/*
 ==================
 IN_MouseEvent
 ==================
*/
void IN_MouseEvent (int state){

	int		i;

	if (!mouse.initialized)
		return;

	if (!mouse.active)
		return;

	// Perform button actions
	for (i = 0; i < mouse.numButtons; i++){
		if ((state & (1<<i)) && !(mouse.oldButtonState & (1<<i)))
			Key_Event(K_MOUSE1 + i, sys.msgTime, true);
		
		if (!(state & (1<<i)) && (mouse.oldButtonState & (1<<i)))
			Key_Event(K_MOUSE1 + i, sys.msgTime, false);
	}	
		
	mouse.oldButtonState = state;
}

/*
 ==================
 IN_MouseMove
 ==================
*/
static void IN_MouseMove (usercmd_t *userCmd){

	POINT	currentPos;
	int		mx, my;
	int		mouseX, mouseY;

	if (!mouse.initialized)
		return;

	if (!mouse.active)
		return;

	if (!GetCursorPos(&currentPos))
		return;

	mx = currentPos.x - mouse.windowCenterX;
	my = currentPos.y - mouse.windowCenterY;

	// Force the mouse to the center, so there's room to move
	if (mx || my)
		SetCursorPos(mouse.windowCenterX, mouse.windowCenterY);

	// If the menu is visible, move the menu cursor
	if (UI_IsVisible()){
		UI_MouseMove(mx, my);
		return;
	}

	// If game is not active or paused, don't move
	if ((cls.state != CA_ACTIVE || cls.loading) || cls.playingCinematic || paused->integerValue)
		return;

	if (m_filter->integerValue){
		mouseX = (mx + mouse.oldMouseX) * 0.5f;
		mouseY = (my + mouse.oldMouseY) * 0.5f;
	}
	else {
		mouseX = mx;
		mouseY = my;
	}

	mouse.oldMouseX = mx;
	mouse.oldMouseY = my;

	mouseX *= sensitivity->floatValue;
	mouseY *= sensitivity->floatValue;

	if (cl.zooming){
		mouseX *= cl.zoomSensitivity;
		mouseY *= cl.zoomSensitivity;
	}

	if (m_showMouse->integerValue && (mouseX || mouseY))
		Com_Printf("%i %i\n", mouseX, mouseY);

	// Add mouse X/Y movement to cmd
	if ((in_strafe.state & 1) || (cl_lookStrafe->integerValue && in_mLooking))
		userCmd->sidemove += m_side->floatValue * mouseX;
	else
		cl.viewAngles[YAW] -= m_yaw->floatValue * mouseX;

	if (!(in_strafe.state & 1) && (cl_freeLook->integerValue || in_mLooking))
		cl.viewAngles[PITCH] += m_pitch->floatValue * mouseY;
	else
		userCmd->forwardmove -= m_forward->floatValue * mouseY;
}


/*
 ==============================================================================

 FUNCTIONS USED ELSEWHERE

 ==============================================================================
*/


/*
 ==================
 IN_Move
 ==================
*/
void IN_Move (usercmd_t *userCmd){

	if (!in_activeApp)
		return;

	IN_MouseMove(userCmd);
}

/*
 ==================
 IN_Frame

 Called every frame, even if not generating commands
 ==================
*/
void IN_Frame (){

	if (!in_activeApp || (!cls.glConfig.isFullscreen && Key_GetKeyDest() == KEY_CONSOLE))
		IN_DeactivateMouse();
	else
		IN_ActivateMouse();
}

/*
 ==================
 IN_ClearStates
 ==================
*/
void IN_ClearStates (){

	mouse.oldButtonState = 0;
	mouse.oldMouseX = 0;
	mouse.oldMouseY = 0;
}

/*
 ==================
 IN_Activate

 Called when the main window gains or loses focus.
 The window may have been destroyed and recreated between a deactivate and an
 activate.
 ==================
*/
void IN_Activate (bool active){

	in_activeApp = active;

	if (active)
		IN_ActivateMouse();
	else
		IN_DeactivateMouse();
}


/*
 ==============================================================================

 CONSOLE COMMANDS

 ==============================================================================
*/


/*
 ==================
 IN_Restart_f
 ==================
*/
static void IN_Restart_f (){

	IN_Shutdown();
	IN_Init();
}

/*
 ==============================================================================

 INITIALIZATION AND SHUTDOWN

 ==============================================================================
*/


/*
 ==================
 IN_Init
 ==================
*/
void IN_Init (){

	Com_Printf("-------- Input Initialization --------\n");

	// General variables
    in_mouse = CVar_Register("in_mouse", "1", CVAR_BOOL, CVAR_ARCHIVE | CVAR_LATCH, "Enable mouse input", 0, 0);
	in_initMouse = CVar_Register("in_initMouse", "1", CVAR_BOOL, CVAR_INIT, NULL, 0, 0);

	// Mouse variables
	m_showMouse = CVar_Register("m_showMouse", "0", CVAR_BOOL, CVAR_CHEAT, NULL, 0, 0);
	m_filter = CVar_Register("m_filter", "0", CVAR_INTEGER, CVAR_ARCHIVE, NULL, 0, 100);
	m_pitch = CVar_Register("m_pitch", "0.022", CVAR_FLOAT, CVAR_ARCHIVE, "Mouse pitch scale", 0.0f, 0.0f);
	m_yaw = CVar_Register("m_yaw", "0.022", CVAR_FLOAT, CVAR_ARCHIVE, "Mouse yaw scale", 0.0f, 0.0f);
	m_forward = CVar_Register("m_forward", "1.0", CVAR_FLOAT, CVAR_ARCHIVE, "Mouse forward/back speed", 0.0f, 0.0f);
	m_side = CVar_Register("m_side", "1.0", CVAR_FLOAT, CVAR_ARCHIVE, NULL, 0.0f, 100.0f);
	sensitivity = CVar_Register("sensitivity", "5", CVAR_FLOAT, CVAR_ARCHIVE, "Mouse view sensitivity", 0.0f, 30.0f);

	// Add commands
	Cmd_AddCommand("+mlook", IN_MLookDown_f, NULL, NULL);
	Cmd_AddCommand("-mlook", IN_MLookUp_f, NULL, NULL);
	Cmd_AddCommand("restartInput", IN_Restart_f, "Restarts the input subsystem", NULL);

	IN_StartupMouse();

	Com_Printf("--------------------------------------\n");
}

/*
 ==================
 IN_Shutdown
 ==================
*/
void IN_Shutdown (){

	// Remove commands
	Cmd_RemoveCommand("+mlook");
	Cmd_RemoveCommand("-mlook");
	Cmd_RemoveCommand("joy_advancedupdate");
	Cmd_RemoveCommand("restartInput");

	// Shutdown mouse controller
	IN_DeactivateMouse();
}