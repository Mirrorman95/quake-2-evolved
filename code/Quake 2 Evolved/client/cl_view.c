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
// cl_view.c - View rendering
//

// TODO:
// - model testing and 3rd person view
// - view blends


#include "client.h"


/*
 ==================
 CL_CalcFov
 ==================
*/
static void CL_CalcFov (){

	float	f;

	// Interpolate field of view
    cl.renderView.fovX = cl.oldPlayerState->fov + (cl.playerState->fov - cl.oldPlayerState->fov) * cl.lerpFrac;
	
	if (cl.renderView.fovX < 1.0f)
		cl.renderView.fovX = 1.0f;
	else if (cl.renderView.fovX > 179.0f)
		cl.renderView.fovX = 179.0f;

	// Interpolate and account for zoom
	if (cl_zoomFov->integerValue < 1)
		CVar_SetVariableInteger("cl_zoomFov", 1, false);
	else if (cl_zoomFov->integerValue > 179)
		CVar_SetVariableInteger("cl_zoomFov", 179, false);

	if (cl.zooming){
		f = (cl.time - cl.zoomTime) / 250.0f;
		if (f > 1.0)
			cl.renderView.fovX = cl_zoomFov->floatValue;
		else
			cl.renderView.fovX = cl.renderView.fovX + f * (cl_zoomFov->floatValue - cl.renderView.fovX);
	}
	else {
		f = (cl.time - cl.zoomTime) / 250.0f;
		if (f > 1.0)
			cl.renderView.fovX = cl.renderView.fovX;
		else
			cl.renderView.fovX = cl_zoomFov->floatValue + f * (cl.renderView.fovX - cl_zoomFov->floatValue);
	}

	// Calculate Y field of view using a 640x480 virtual screen
	f = SCREEN_WIDTH / tan(cl.renderView.fovX / 360.0f * M_PI);
	cl.renderView.fovY = atan2(SCREEN_HEIGHT, f) * 360.0f / M_PI;

	if (cl.zooming)
		cl.zoomSensitivity = cl.renderView.fovY / 75.0f;

	// Warp if underwater
	if (cl.underwater){
		f = sin(MS2SEC(cl.time) * 0.4f * M_PI_TWO);

		cl.renderView.fovX += f;
		cl.renderView.fovY -= f;
	}
}

/*
 ==================
 CL_CalcFirstPersonView
 ==================
*/
static void CL_CalcFirstPersonView (){

	vec3_t	viewOffset, kickAngles;
	uint	delta;

    // Calculate the origin
	if (cl_predict->integerValue && !(cl.playerState->pmove.pm_flags & PMF_NO_PREDICTION)){
		// Use predicted values
		VectorLerp(cl.oldPlayerState->viewoffset, cl.playerState->viewoffset, cl.lerpFrac, viewOffset);
		VectorAdd(cl.predictedOrigin, viewOffset, cl.renderView.origin);
		VectorMA(cl.renderView.origin, -(1.0f - cl.lerpFrac), cl.predictedError, cl.renderView.origin);

        // Smooth out stair climbing
        delta = cls.realTime - cl.predictedStepTime;
        if (delta < 100)
            cl.renderView.origin[2] -= cl.predictedStep * (100 - delta) * 0.01f;
    }
    else {
		// Just use interpolated values
		cl.renderView.origin[0] = cl.oldPlayerState->pmove.origin[0]*0.125f + cl.oldPlayerState->viewoffset[0] + cl.lerpFrac * (cl.playerState->pmove.origin[0]*0.125f + cl.playerState->viewoffset[0] - (cl.oldPlayerState->pmove.origin[0]*0.125f + cl.oldPlayerState->viewoffset[0]));
		cl.renderView.origin[1] = cl.oldPlayerState->pmove.origin[1]*0.125f + cl.oldPlayerState->viewoffset[1] + cl.lerpFrac * (cl.playerState->pmove.origin[1]*0.125f + cl.playerState->viewoffset[1] - (cl.oldPlayerState->pmove.origin[1]*0.125f + cl.oldPlayerState->viewoffset[1]));
		cl.renderView.origin[2] = cl.oldPlayerState->pmove.origin[2]*0.125f + cl.oldPlayerState->viewoffset[2] + cl.lerpFrac * (cl.playerState->pmove.origin[2]*0.125f + cl.playerState->viewoffset[2] - (cl.oldPlayerState->pmove.origin[2]*0.125f + cl.oldPlayerState->viewoffset[2]));
    }

    // Calculate the angles
    if (cl.playerState->pmove.pm_type < PM_DEAD)
		// Use predicted values
		VectorCopy(cl.predictedAngles, cl.renderViewAngles);
    else
		// Just use interpolated values
		LerpAngles(cl.oldPlayerState->viewangles, cl.playerState->viewangles, cl.lerpFrac, cl.renderViewAngles);

	// Account for kick angles
	LerpAngles(cl.oldPlayerState->kick_angles, cl.playerState->kick_angles, cl.lerpFrac, kickAngles);
	VectorAdd(cl.renderViewAngles, kickAngles, cl.renderViewAngles);
}

/*
 ==================
 CL_CalcThirdPersonView

 TODO: this is kind jerky, maybe there is a faster way of doing it?
 TODO: add a camera height variable
 ==================
*/
static void CL_CalcThirdPersonView (){

	trace_t	trace;
	vec3_t	forward, right, spot;
	vec3_t	origin, angles;
	vec3_t	mins = {-4.0f, -4.0f, -4.0f}, maxs = {4.0f, 4.0f, 4.0f};
	float	dist, rad;

	// Calculate the origin
	cl.renderView.origin[0] = cl.oldPlayerState->pmove.origin[0]*0.125f + cl.oldPlayerState->viewoffset[0] + cl.lerpFrac * (cl.playerState->pmove.origin[0]*0.125f + cl.playerState->viewoffset[0] - (cl.oldPlayerState->pmove.origin[0]*0.125f + cl.oldPlayerState->viewoffset[0]));
	cl.renderView.origin[1] = cl.oldPlayerState->pmove.origin[1]*0.125f + cl.oldPlayerState->viewoffset[1] + cl.lerpFrac * (cl.playerState->pmove.origin[1]*0.125f + cl.playerState->viewoffset[1] - (cl.oldPlayerState->pmove.origin[1]*0.125f + cl.oldPlayerState->viewoffset[1]));
	cl.renderView.origin[2] = cl.oldPlayerState->pmove.origin[2]*0.125f + cl.oldPlayerState->viewoffset[2] + cl.lerpFrac * (cl.playerState->pmove.origin[2]*0.125f + cl.playerState->viewoffset[2] - (cl.oldPlayerState->pmove.origin[2]*0.125f + cl.oldPlayerState->viewoffset[2]));

	// Calculate the angles
	LerpAngles(cl.oldPlayerState->viewangles, cl.playerState->viewangles, cl.lerpFrac, cl.renderViewAngles);

	VectorCopy(cl.renderViewAngles, angles);
	if (angles[PITCH] > 45.0f)
		angles[PITCH] = 45.0f;

	AngleToVectors(angles, forward, NULL, NULL);
	VectorMA(cl.renderView.origin, 512.0f, forward, spot);

	// Calculate exact origin
	VectorCopy(cl.renderView.origin, origin);
	origin[2] += 8.0f;

	cl.renderViewAngles[PITCH] *= 0.5f;
	AngleToVectors(cl.renderViewAngles, forward, right, NULL);

	rad = DEG2RAD(cl_thirdPersonAngle->floatValue);
	VectorMA(origin, -cl_thirdPersonRange->floatValue * cos(rad), forward, origin);
	VectorMA(origin, -cl_thirdPersonRange->floatValue * sin(rad), right, origin);

	// Trace a line to make sure the view isn't inside solid geometry
	trace = CL_Trace(cl.renderView.origin, mins, maxs, origin, cl.clientNum, MASK_PLAYERSOLID, false, NULL);
	if (trace.fraction != 1.0f){
		VectorCopy(trace.endpos, origin);
		origin[2] += (1.0f - trace.fraction) * 32.0f;

		trace = CL_Trace(cl.renderView.origin, mins, maxs, origin, cl.clientNum, MASK_PLAYERSOLID, false, NULL);
		VectorCopy(trace.endpos, origin);
	}

	VectorCopy(origin, cl.renderView.origin);

	// Calculate pitch to look at spot from camera
	VectorSubtract(spot, cl.renderView.origin, spot);
	dist = sqrt(spot[0] * spot[0] + spot[1] * spot[1]);
	if (dist < 1.0f)
		dist = 1.0f;

	cl.renderViewAngles[PITCH] = -RAD2DEG(atan2(spot[2], dist));
	cl.renderViewAngles[YAW] -= cl_thirdPersonAngle->floatValue;
}

/*
 ==================
 CL_CheckContentBlends
 ==================
*/
static void CL_CheckContentBlends (){

	trace_t	trace;
	vec3_t	mins = {-16.0f, -16.0f, -24.0f}, maxs = {16.0f, 16.0f, 32.0f};

	// Check if underwater
	if (CL_PointContents(cl.renderView.origin, -1) & MASK_WATER){
		cl.underwater = true;

		// Set the end time for the underwater vision effect
		cl.underwaterVisionEndTime = cl.time + 1000;
	}
	else
		cl.underwater = false;

	// Check if the player is touching lava
	if (cl.playerState->pmove.pm_flags & PMF_DUCKED){
		mins[2] = -22.0f;
		maxs[2] = 6.0f;
	}
	else {
		mins[2] = -46.0f;
		maxs[2] = 10.0f;
	}

	trace = CL_Trace(cl.renderView.origin, mins, maxs, cl.renderView.origin, -1, CONTENTS_LAVA, false, NULL);
	if (trace.contents & CONTENTS_LAVA)
		cl.fireScreenEndTime = cl.time + 1000;
}

/*
 ==================
 CL_CalcViewValues
 ==================
*/
static void CL_CalcViewValues (){

	// Clamp time
	if (cl.time > cl.frame.serverTime){
		cl.time = cl.frame.serverTime;
		cl.lerpFrac = 1.0f;
	}
	else if (cl.time < cl.frame.serverTime - 100){
		cl.time = cl.frame.serverTime - 100;
		cl.lerpFrac = 0.0f;
	}
	else
		cl.lerpFrac = 1.0f - (cl.frame.serverTime - cl.time) * 0.01f;

	if (com_timeDemo->integerValue)
        cl.lerpFrac = 1.0f;

	// Calculate view origin and angles
	if (!cl_thirdPerson->integerValue)
		CL_CalcFirstPersonView();
	else
		CL_CalcThirdPersonView();

	// Never let it sit exactly on a node line, because a water plane
	// can disappear when viewed with the eye exactly on it. The server
	// protocol only specifies to 1/8 pixel, so add 1/16 in each axis.
	cl.renderView.origin[0] += 1.0f/16;
	cl.renderView.origin[1] += 1.0f/16;
	cl.renderView.origin[2] += 1.0f/16;

	AnglesToMat3(cl.renderViewAngles, cl.renderView.axis);

	// Check contents for view blending effects
	CL_CheckContentBlends();

	// Set up the render view
	cl.renderView.width = SCREEN_WIDTH * cl_viewSize->integerValue / 100;
	cl.renderView.width &= ~1;

	cl.renderView.height = SCREEN_HEIGHT * cl_viewSize->integerValue / 100;
	cl.renderView.height &= ~1;

	cl.renderView.x = (SCREEN_WIDTH - cl.renderView.width) >> 1;
	cl.renderView.y = (SCREEN_HEIGHT - cl.renderView.height) >> 1;

	cl.renderView.horzAdjust = H_SCALE;
	cl.renderView.horzPercent = 1.0f;

	cl.renderView.vertAdjust = V_SCALE;
	cl.renderView.vertPercent = 1.0f;

	// Calculate field of view
	CL_CalcFov();

	// Finish up the rest of the renderView
	cl.renderView.time = MS2SEC(cl.time);
	cl.renderView.areaBits = cl.frame.areaBits;

	cl.renderView.materialParms[0] = 1.0f;
	cl.renderView.materialParms[1] = 1.0f;
	cl.renderView.materialParms[2] = 1.0f;
	cl.renderView.materialParms[3] = 1.0f;
	cl.renderView.materialParms[4] = -0.50f * cl.renderView.time;
	cl.renderView.materialParms[5] = -0.25f * cl.renderView.time;
	cl.renderView.materialParms[6] = (cl.playerState->rdflags & RDF_IRGOGGLES) ? 1.0f : 0.0f;
	cl.renderView.materialParms[7] = 0.0f;
}

/*
 ==================
 
 ==================
*/
static void CL_DrawViewBlends (){

	if (!cl_viewBlend->integerValue || cl_thirdPerson->integerValue)
		return;

	// This is just the old poly blend
	if (cl_viewBlend->integerValue == 1){
		if (!cl.playerState->blend[3])
			return;

		R_SetColor4(cl.playerState->blend[0], cl.playerState->blend[1], cl.playerState->blend[2], cl.playerState->blend[3]);
		CL_FillRect(cl.renderView.x, cl.renderView.y, cl.renderView.width, cl.renderView.height, H_SCALE, 1.0f, V_SCALE, 1.0f);

		return;
	}

	// Fire screen

	// Underwater blur

	// Double vision

	// Underwater vision

	// Draw IR Goggles
}

/*
 ==================
 CL_RenderView
 ==================
*/
static void CL_RenderView (){

	// Render the scene
	R_RenderScene(&cl.renderView, true);

	// Draw screen blends on top of the game view
	CL_DrawViewBlends();
}

/*
 ==================
 CL_RenderActiveFrame
 ==================
*/
void CL_RenderActiveFrame (){

	if (cl_skipRendering->integerValue)
		return;

	if (!cl.frame.valid){
		R_SetColor(colorBlack);
		CL_FillRect(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, H_NONE, 1.0f, V_NONE, 1.0f);
		return;
	}

	// Clear render lists
	R_ClearScene();

	// Build renderView
	CL_CalcViewValues();

	// Build renderer lists
	CL_AddViewWeapon();
	CL_AddPacketEntities();
	CL_AddTempEntities();
	CL_AddLocalEntities();
	CL_AddDynamicLights();
	CL_AddParticles();
	CL_AddLightStyles();

	// Update test tools
	CL_UpdateTestTools();

	// Render the game view
	CL_RenderView();

	// Draw all the 2D elements
	CL_Draw2D();
}