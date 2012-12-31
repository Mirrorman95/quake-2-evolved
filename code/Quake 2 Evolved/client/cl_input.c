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
// cl_input.c - Input button events
//

// TODO:
// - FINISH!!!


#include "client.h"


extern uint				cl_frameMsec;


/*
 ==============================================================================

 KEY BUTTONS

 Continuous button event tracking is complicated by the fact that two different
 input sources (say, left mouse button and the control key) can both press the
 same button, but the button should only be released when both of the pressing
 keys have been released.

 When a key event issues a key button command (+forward, +attack, etc...), it
 sends its key number as a parameter so it can be matched up with the release.

 It also sends the time the event happened, which allows exact control even at
 low framerates when the down and up events may both get queued in the same
 frame.

 State bit 0 is the current state of the key
 State bit 1 is edge triggered on the up to down transition
 State bit 2 is edge triggered on the down to up transition

 ==============================================================================
*/

keyButton_t				in_up;
keyButton_t				in_down;
keyButton_t				in_left;
keyButton_t				in_right;
keyButton_t				in_forward;
keyButton_t				in_back;
keyButton_t				in_lookUp;
keyButton_t				in_lookDown;
keyButton_t				in_strafe;
keyButton_t				in_moveLeft;
keyButton_t				in_moveRight;
keyButton_t				in_speed;
keyButton_t				in_attack;
keyButton_t				in_use;
keyButton_t				in_kLook;
int						in_impulse;


/*
 ==================
 
 ==================
*/
static void IN_KeyButtonDown (keyButton_t *kb){

	int		key;
	char	*cmd;

	cmd = Cmd_Argv(1);
	if (cmd[0])
		key = Str_ToInteger(cmd);
	else
		key = -1;		// Typed manually at the console for continuous down

	if (key == kb->down[0] || key == kb->down[1])
		return;		// Repeating key

	if (!kb->down[0])
		kb->down[0] = key;
	else if (!kb->down[1])
		kb->down[1] = key;
	else {
		Com_Printf("Three keys down for a button!\n");
		return;
	}

	if (kb->state & 1)
		return;		// Still down

	// Save timestamp
	kb->downTime = Str_ToInteger(Cmd_Argv(2));
	if (!kb->downTime)
		kb->downTime = com_frameTime - 100;

	kb->state |= 1 + 2;	// Down + impulse down
}

/*
 ==================
 
 ==================
*/
static void IN_KeyButtonUp (keyButton_t *kb){

	int		key;
	char	*cmd;
	uint	upTime;

	cmd = Cmd_Argv(1);
	if (cmd[0])
		key = Str_ToInteger(cmd);
	else {
		// Typed manually at the console, assume for unsticking, so 
		// clear all
		kb->down[0] = kb->down[1] = 0;
		kb->state = 4;	// Impulse up
		return;
	}

	if (kb->down[0] == key)
		kb->down[0] = 0;
	else if (kb->down[1] == key)
		kb->down[1] = 0;
	else
		return;		// Key up without coresponding down (menu pass through)

	if (kb->down[0] || kb->down[1])
		return;		// Some other key is still holding it down

	if (!(kb->state & 1))
		return;		// Still up (this should not happen)

	// Save timestamp
	upTime = Str_ToInteger(Cmd_Argv(2));
	if (upTime)
		kb->msec += upTime - kb->downTime;
	else
		kb->msec += 10;

	kb->state &= ~1;	// Now up
	kb->state |= 4;		// Impulse up
}

/*
 ==================
 
 Returns the fraction of the frame that the key was down
 ==================
*/
float IN_KeyButtonState (keyButton_t *kb){

	float	frac;
	int		msec;

	kb->state &= 1;		// Clear impulses

	msec = kb->msec;
	kb->msec = 0;

	if (kb->state){
		// Still down
		msec += com_frameTime - kb->downTime;
		kb->downTime = com_frameTime;
	}

	frac = (float)msec / cl_frameMsec;
	if (frac < 0.0f)
		frac = 0.0f;
	else if (frac > 1.0f)
		frac = 1.0f;

	return frac;
}


// =====================================================================


/*
 ==================
 
 ==================
*/
void CL_CenterView_f (){

	cl.viewAngles[PITCH] = -SHORT2ANGLE(cl.frame.playerState.pmove.delta_angles[PITCH]);
}

/*
 ==================
 
 ==================
*/
void CL_ZoomDown_f (){

	if (cl.zooming)
		return;

	cl.zooming = true;
	cl.zoomTime = cl.time;
}

/*
 ==================
 
 ==================
*/
void CL_ZoomUp_f (){

	if (!cl.zooming)
		return;

	cl.zooming = false;
	cl.zoomTime = cl.time;
}

/*
 ==================
 
 ==================
*/
void CL_UpDown_f (){
	
	IN_KeyButtonDown(&in_up);
}

/*
 ==================
 
 ==================
*/
void CL_UpUp_f (){
	
	IN_KeyButtonUp(&in_up);
}

/*
 ==================
 
 ==================
*/
void CL_DownDown_f (){
	
	IN_KeyButtonDown(&in_down);
}

/*
 ==================
 
 ==================
*/
void CL_DownUp_f (){
	
	IN_KeyButtonUp(&in_down);
}

/*
 ==================
 
 ==================
*/
void CL_LeftDown_f (){
	
	IN_KeyButtonDown(&in_left);
}

/*
 ==================
 
 ==================
*/
void CL_LeftUp_f (){
	
	IN_KeyButtonUp(&in_left);
}

/*
 ==================
 
 ==================
*/
void CL_RightDown_f (){
	
	IN_KeyButtonDown(&in_right);
}

/*
 ==================
 
 ==================
*/
void CL_RightUp_f (){
	
	IN_KeyButtonUp(&in_right);
}

/*
 ==================
 
 ==================
*/
void CL_ForwardDown_f (){
	
	IN_KeyButtonDown(&in_forward);
}

/*
 ==================
 
 ==================
*/
void CL_ForwardUp_f (){
	
	IN_KeyButtonUp(&in_forward);
}

/*
 ==================
 
 ==================
*/
void CL_BackDown_f (){
	
	IN_KeyButtonDown(&in_back);
}

/*
 ==================
 
 ==================
*/
void CL_BackUp_f (){
	
	IN_KeyButtonUp(&in_back);
}

/*
 ==================
 
 ==================
*/
void CL_LookUpDown_f (){
	
	IN_KeyButtonDown(&in_lookUp);
}

/*
 ==================
 
 ==================
*/
void CL_LookUpUp_f (){
	
	IN_KeyButtonUp(&in_lookUp);
}

/*
 ==================
 
 ==================
*/
void CL_LookDownDown_f (){
	
	IN_KeyButtonDown(&in_lookDown);
}

/*
 ==================
 
 ==================
*/
void CL_LookDownUp_f (){
	
	IN_KeyButtonUp(&in_lookDown);
}

/*
 ==================
 
 ==================
*/
void CL_StrafeDown_f (){
	
	IN_KeyButtonDown(&in_strafe);
}

/*
 ==================
 
 ==================
*/
void CL_StrafeUp_f (){
	
	IN_KeyButtonUp(&in_strafe);
}

/*
 ==================
 
 ==================
*/
void CL_MoveLeftDown_f (){
	
	IN_KeyButtonDown(&in_moveLeft);
}

/*
 ==================
 
 ==================
*/
void CL_MoveLeftUp_f (){
	
	IN_KeyButtonUp(&in_moveLeft);
}

/*
 ==================
 
 ==================
*/
void CL_MoveRightDown_f (){
	
	IN_KeyButtonDown(&in_moveRight);
}

/*
 ==================
 
 ==================
*/
void CL_MoveRightUp_f (){
	
	IN_KeyButtonUp(&in_moveRight);
}

/*
 ==================
 
 ==================
*/
void CL_SpeedDown_f (){
	
	IN_KeyButtonDown(&in_speed);
}

/*
 ==================
 
 ==================
*/
void CL_SpeedUp_f (){
	
	IN_KeyButtonUp(&in_speed);
}

/*
 ==================
 
 ==================
*/
void CL_AttackDown_f (){
	
	IN_KeyButtonDown(&in_attack);
}

/*
 ==================
 
 ==================
*/
void CL_AttackUp_f (){
	
	IN_KeyButtonUp(&in_attack);
}

/*
 ==================
 
 ==================
*/
void CL_UseDown_f (){
	
	IN_KeyButtonDown(&in_use);
}

/*
 ==================
 
 ==================
*/
void CL_UseUp_f (){
	
	IN_KeyButtonUp(&in_use);
}

/*
 ==================
 
 ==================
*/
void CL_KLookDown_f (){
	
	IN_KeyButtonDown(&in_kLook);
}

/*
 ==================
 
 ==================
*/
void CL_KLookUp_f (){
	
	IN_KeyButtonUp(&in_kLook);
}

/*
 ==================
 
 ==================
*/
void CL_Impulse_f (){

	in_impulse = Str_ToInteger(Cmd_Argv(1));
}