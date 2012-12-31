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
// cl_userCmd.c - Builds an intended movement command to send to the server
//

// TODO:
// - CL_MouseMove


#include "client.h"


uint						cl_frameMsec;
static uint					cl_oldFrameTime;


/*
 ==================
 CL_AdjustAngles

 Moves the local angles positions
 ==================
*/
static void CL_AdjustAngles (){

	float	speed;
	
	if (in_speed.state & 1)
		speed = cls.frameTime * cl_angleSpeedKey->floatValue;
	else
		speed = cls.frameTime;

	if (!(in_strafe.state & 1)){
		cl.viewAngles[YAW] -= speed * cl_yawSpeed->floatValue * IN_KeyButtonState(&in_right);
		cl.viewAngles[YAW] += speed * cl_yawSpeed->floatValue * IN_KeyButtonState(&in_left);
	}

	if (in_kLook.state & 1){
		cl.viewAngles[PITCH] -= speed * cl_pitchSpeed->floatValue * IN_KeyButtonState(&in_forward);
		cl.viewAngles[PITCH] += speed * cl_pitchSpeed->floatValue * IN_KeyButtonState(&in_back);
	}
	
	cl.viewAngles[PITCH] -= speed * cl_pitchSpeed->floatValue * IN_KeyButtonState(&in_lookUp);
	cl.viewAngles[PITCH] += speed * cl_pitchSpeed->floatValue * IN_KeyButtonState(&in_lookDown);
}

/*
 ==================
 CL_KeyMove

 Send the intended movement message to the server
 ==================
*/
static void CL_KeyMove (usercmd_t *userCmd){

	if (in_strafe.state & 1){
		userCmd->sidemove += cl_sideSpeed->floatValue * IN_KeyButtonState(&in_right);
		userCmd->sidemove -= cl_sideSpeed->floatValue * IN_KeyButtonState(&in_left);
	}

	userCmd->sidemove += cl_sideSpeed->floatValue * IN_KeyButtonState(&in_moveRight);
	userCmd->sidemove -= cl_sideSpeed->floatValue * IN_KeyButtonState(&in_moveLeft);

	userCmd->upmove += cl_upSpeed->floatValue * IN_KeyButtonState(&in_up);
	userCmd->upmove -= cl_upSpeed->floatValue * IN_KeyButtonState(&in_down);

	if (!(in_kLook.state & 1)){
		userCmd->forwardmove += cl_forwardSpeed->floatValue * IN_KeyButtonState(&in_forward);
		userCmd->forwardmove -= cl_forwardSpeed->floatValue * IN_KeyButtonState(&in_back);
	}	

	// Adjust for speed key / running
	if ((in_speed.state & 1) ^ cl_run->integerValue){
		userCmd->forwardmove *= 2;
		userCmd->sidemove *= 2;
		userCmd->upmove *= 2;
	}	
}

static void CL_MouseMove (usercmd_t *userCmd){

}

/*
 ==================
 CL_ClampPitch
 ==================
*/
static void CL_ClampPitch (){

	float	pitch;

	pitch = SHORT2ANGLE(cl.frame.playerState.pmove.delta_angles[PITCH]);
	if (pitch > 180.0f)
		pitch -= 360.0f;

	if (cl.viewAngles[PITCH] + pitch < -360.0f)
		cl.viewAngles[PITCH] += 360.0f;	// Wrapped
	if (cl.viewAngles[PITCH] + pitch > 360.0f)
		cl.viewAngles[PITCH] -= 360.0f;	// Wrapped

	if (cl.viewAngles[PITCH] + pitch > 89.0f)
		cl.viewAngles[PITCH] = 89.0f - pitch;
	if (cl.viewAngles[PITCH] + pitch < -89.0f)
		cl.viewAngles[PITCH] = -89.0f - pitch;
}

/*
 ==================
 CL_GetLightLevel
 ==================
*/
static byte CL_GetLightLevel (){

	if ((cls.state != CA_ACTIVE || cls.loading) || cls.playingCinematic)
		return 0;

	return 150;
}

/*
 ==================
 CL_FinishMove
 ==================
*/
static void CL_FinishMove (usercmd_t *userCmd){

	int		ms;

	// Figure button bits
	if (in_attack.state & 3)
		userCmd->buttons |= BUTTON_ATTACK;
	in_attack.state &= ~2;
	
	if (in_use.state & 3)
		userCmd->buttons |= BUTTON_USE;
	in_use.state &= ~2;

	if (Key_IsAnyDown())
		userCmd->buttons |= BUTTON_ANY;

	// Send milliseconds of time to apply to the move
	ms = cls.frameTime * 1000.0f;
	if (ms > 250)
		ms = 100;		// Time was unreasonable
	userCmd->msec = ms;

	CL_ClampPitch();

	userCmd->angles[0] = ANGLE2SHORT(cl.viewAngles[0]);
	userCmd->angles[1] = ANGLE2SHORT(cl.viewAngles[1]);
	userCmd->angles[2] = ANGLE2SHORT(cl.viewAngles[2]);

	userCmd->impulse = in_impulse;
	in_impulse = 0;

	userCmd->lightlevel = CL_GetLightLevel();
}

/*
 ==================
 CL_CreateCmd
 ==================
*/
static void CL_CreateCmd (){

	usercmd_t	*userCmd;
	int			index;

	index = cls.netChan.outgoingSequence & CMD_MASK;
	userCmd = &cl.cmds[index];

	cl_frameMsec = com_frameTime - cl_oldFrameTime;
	if (cl_frameMsec < 1)
		cl_frameMsec = 1;
	else if (cl_frameMsec > 200)
		cl_frameMsec = 200;

	cl_oldFrameTime = com_frameTime;

	Mem_Fill(userCmd, 0, sizeof(usercmd_t));

	// Keyboard angle adjustment
	CL_AdjustAngles();

	// Get movement from keyboard
	CL_KeyMove(userCmd);

	// Get movement from mouse
	IN_Move(userCmd);

	// Finish movement
	CL_FinishMove(userCmd);

	// Save the time for ping calculations
	cl.cmdTime[index] = cls.realTime;
}

/*
 ==================
 CL_BuildUserCmd

 Build a command even if not connected
 ==================
*/
void CL_BuildUserCmd (){

	msg_t		msg;
	byte		data[128];
	usercmd_t	*userCmd, *oldUserCmd, nullUserCmd;
	int			checksumIndex;

	CL_CreateCmd();

	if (cls.state < CA_CONNECTED)
		return;

	if (cls.state < CA_ACTIVE){
		if (cls.netChan.message.curSize	|| Sys_Milliseconds() - cls.netChan.lastSent > 1000)
			NetChan_Transmit(&cls.netChan, NULL, 0);

		return;
	}

	// Send a user info update if needed
	if (CVar_GetModifiedFlags() & CVAR_USERINFO){
		CVar_ClearModifiedFlags(CVAR_USERINFO);

		MSG_WriteByte(&cls.netChan.message, CLC_USERINFO);
		MSG_WriteString(&cls.netChan.message, CVar_InfoString(CVAR_USERINFO));
	}

	MSG_Init(&msg, data, sizeof(data), false);

	// Begin a client move command
	MSG_WriteByte(&msg, CLC_MOVE);

	// Save the position for a checksum byte
	checksumIndex = msg.curSize;
	MSG_WriteByte(&msg, 0);

	// Let the server know what the last frame we got was, so the next 
	// message can be delta compressed
	if (cl_noDelta->integerValue || !cl.frame.valid || cls.demoWaiting)
		MSG_WriteLong(&msg, -1);	// No compression
	else
		MSG_WriteLong(&msg, cl.frame.serverFrame);

	// Send this and the previous cmds in the message, so if the last 
	// packet was dropped, it can be recovered
	Mem_Fill(&nullUserCmd, 0, sizeof(nullUserCmd));

	userCmd = &cl.cmds[(cls.netChan.outgoingSequence - 2) & CMD_MASK];
	MSG_WriteDeltaUserCmd(&msg, &nullUserCmd, userCmd);

	oldUserCmd = userCmd;
	userCmd = &cl.cmds[(cls.netChan.outgoingSequence - 1) & CMD_MASK];
	MSG_WriteDeltaUserCmd(&msg, oldUserCmd, userCmd);

	oldUserCmd = userCmd;
	userCmd = &cl.cmds[(cls.netChan.outgoingSequence) & CMD_MASK];
	MSG_WriteDeltaUserCmd(&msg, oldUserCmd, userCmd);

	// Calculate a checksum over the move commands
	msg.data[checksumIndex] = Com_BlockSequenceCRCByte(msg.data + checksumIndex + 1, msg.curSize - checksumIndex - 1, cls.netChan.outgoingSequence);

	// Deliver the message
	NetChan_Transmit(&cls.netChan, msg.data, msg.curSize);
}