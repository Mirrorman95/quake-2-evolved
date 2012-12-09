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
// - add testSound
// - model testing and 3rd person view
// - view blends


#include "client.h"


/*
 ==============================================================================

 MODEL TESTING

 ==============================================================================
*/


/*
 ==================
 CL_ClearTestModel
 ==================
*/
void CL_ClearTestModel (){

	cl.testModel.isGun = false;
	cl.testModel.active = false;
	Mem_Fill(&cl.testModel.name, 0, sizeof(cl.testModel.name));
	Mem_Fill(&cl.testModel.renderEntity, 0, sizeof(cl.testModel.renderEntity));
}

/*
 ==================
 
 ==================
*/
void CL_TestModel_f (){

	if (Cmd_Argc() > 2){
		Com_Printf("Usage: testModel [name]\n");
		return;
	}

	if ((cls.state != CA_ACTIVE || cls.loading) || cls.playingCinematic || cl.demoPlayback){
		Com_Printf("You must be in a level to test a model\n");
		return;
	}

	if (!CVar_AllowCheats()){
		Com_Printf("You must enable cheats to test a model\n");
		return;
	}

	// Clear the old model, if any
	CL_ClearTestModel();

	if (Cmd_Argc() != 2)
		return;

	Str_Copy(cl.testModel.name, Cmd_Argv(1), sizeof(cl.testModel.name));

	cl.testModel.renderEntity.type = RE_MODEL;
	cl.testModel.renderEntity.model = R_RegisterModel(Cmd_Argv(1));

	cl.testModel.renderEntity.materialParms[MATERIALPARM_RED] = 1.0f;
	cl.testModel.renderEntity.materialParms[MATERIALPARM_GREEN] = 1.0f;
	cl.testModel.renderEntity.materialParms[MATERIALPARM_BLUE] = 1.0f;
	cl.testModel.renderEntity.materialParms[MATERIALPARM_ALPHA] = 1.0f;
	cl.testModel.renderEntity.materialParms[MATERIALPARM_TIMEOFFSET] = -MS2SEC(cl.time);
	cl.testModel.renderEntity.materialParms[MATERIALPARM_DIVERSITY] = crand();
	cl.testModel.renderEntity.materialParms[MATERIALPARM_MISC] = 0.0f;
	cl.testModel.renderEntity.materialParms[MATERIALPARM_MODE] = 0.0f;

	VectorMA(cl.renderView.origin, 100.0f, cl.renderView.axis[0], cl.testModel.renderEntity.origin);
	Matrix3_Identity(cl.testModel.renderEntity.axis);

	cl.testModel.active = true;
}

/*
 ==================
 
 ==================
*/
void CL_TestGun_f (){

	CL_TestModel_f();

	if (cl.testModel.active){
		cl.testModel.isGun = true;
		cl.testModel.renderEntity.renderFX = RF_MINLIGHT | RF_WEAPONMODEL;
		cl.testModel.renderEntity.depthHack = true;
	}
}

/*
 ==================
 CL_TestMaterial_f
 ==================
*/
void CL_TestMaterial_f (){

	if (Cmd_Argc() > 2){
		Com_Printf("Usage: testMaterial [name]\n");
		return;
	}

	if (!cl.testModel.active){
		Com_Printf("No active testModel\n");
		return;
	}

	// Clear the old material, if any
	cl.testModel.renderEntity.material = NULL;

	if (Cmd_Argc() != 2)
		return;

	// Load the material
	cl.testModel.renderEntity.material = R_RegisterMaterial(Cmd_Argv(1), false);
}

/*
 ==================
 CL_TestMaterialParm_f
 ==================
*/
void CL_TestMaterialParm_f (){

	int		index;

	if (Cmd_Argc() != 3){
		Com_Printf("Usage: testMaterialParm <index> <value | \"time\">\n");
		return;
	}

	if (!cl.testModel.active){
		Com_Printf("No active testModel\n");
		return;
	}

	// Set the specified material parm to the specified value
	index = Str_ToInteger(Cmd_Argv(1));
	if (index < 0 || index >= MAX_MATERIAL_PARMS){
		Com_Printf("Specified index is out of range\n");
		return;
	}

	if (!Str_ICompare(Cmd_Argv(2), "time"))
		cl.testModel.renderEntity.materialParms[index] = MS2SEC(cl.time);
	else
		cl.testModel.renderEntity.materialParms[index] = Str_ToFloat(Cmd_Argv(2));
}

/*
 ==================
 CL_NextFrame_f
 ==================
*/
void CL_NextFrame_f (){

	if (!cl.testModel.active){
		Com_Printf("No active testModel\n");
		return;
	}

	if (cl_testModelAnimate->integerValue)
		return;

	cl.testModel.renderEntity.frame++;

	Com_Printf("Frame %i\n", cl.testModel.renderEntity.frame);
}

/*
 ==================
 CL_PrevFrame_f
 ==================
*/
void CL_PrevFrame_f (){

	if (!cl.testModel.active){
		Com_Printf("No active testModel\n");
		return;
	}

	if (cl_testModelAnimate->integerValue)
		return;

	cl.testModel.renderEntity.frame--;
	if (cl.testModel.renderEntity.frame < 0)
		cl.testModel.renderEntity.frame = 0;

	Com_Printf("Frame %i\n", cl.testModel.renderEntity.frame);
}

/*
 ==================
 CL_NextSkin_f
 ==================
*/
void CL_NextSkin_f (){

	if (!cl.testModel.active){
		Com_Printf("No active testModel\n");
		return;
	}

	cl.testModel.renderEntity.skinIndex++;

	Com_Printf("Skin %i\n", cl.testModel.renderEntity.skinIndex);
}

/*
 ==================
 CL_PrevSkin_f
 ==================
*/
void CL_PrevSkin_f (){

	if (!cl.testModel.active){
		Com_Printf("No active testModel\n");
		return;
	}

	cl.testModel.renderEntity.skinIndex--;
	if (cl.testModel.renderEntity.skinIndex < 0)
		cl.testModel.renderEntity.skinIndex = 0;

	Com_Printf("Skin %i\n", cl.testModel.renderEntity.skinIndex);
}

/*
 ==================

 ==================
*/
static void CL_UpdateTestModel (){

}


/*
 ==============================================================================

 SPRITE TESTING

 ==============================================================================
*/


/*
 ==================
 CL_ClearTestSprite
 ==================
*/
void CL_ClearTestSprite (){

	// Clear the test beam
	Mem_Fill(&cl.testSprite, 0, sizeof(testSprite_t));
}

/*
 ==================
 CL_TestSprite_f
 ==================
*/
void CL_TestSprite_f (){

	if (Cmd_Argc() > 3){
		Com_Printf("Usage: testSprite [material] [radius]\n");
		return;
	}

	if ((cls.state != CA_ACTIVE || cls.loading) || cls.playingCinematic || cl.demoPlayback){
		Com_Printf("You must be in a level to test a beam\n");
		return;
	}

	if (!CVar_AllowCheats()){
		Com_Printf("You must enable cheats to test a beam\n");
		return;
	}

	// Clear the old sprite, if any
	CL_ClearTestSprite();

	if (Cmd_Argc() < 2)
		return;

	// Test the specified material on a sprite
	cl.testSprite.active = true;

	// Set up the render entity
	cl.testSprite.renderEntity.type = RE_SPRITE;

	VectorMA(cl.renderView.origin, 100.0f, cl.renderView.axis[0], cl.testSprite.renderEntity.origin);
	Matrix3_Identity(cl.testSprite.renderEntity.axis);

	cl.testSprite.renderEntity.spriteOriented = false;

	if (Cmd_Argc() == 2)
		cl.testSprite.renderEntity.spriteRadius = 25.0f;
	else {
		cl.testSprite.renderEntity.spriteRadius = Str_ToFloat(Cmd_Argv(2));
		if (cl.testSprite.renderEntity.spriteRadius <= 0.0f)
			cl.testSprite.renderEntity.spriteRadius = 25.0f;
	}

	cl.testSprite.renderEntity.spriteRotation = 0.0f;

	cl.testSprite.renderEntity.material = R_RegisterMaterial(Cmd_Argv(1), false);

	cl.testSprite.renderEntity.materialParms[MATERIALPARM_RED] = 1.0f;
	cl.testSprite.renderEntity.materialParms[MATERIALPARM_GREEN] = 1.0f;
	cl.testSprite.renderEntity.materialParms[MATERIALPARM_BLUE] = 1.0f;
	cl.testSprite.renderEntity.materialParms[MATERIALPARM_ALPHA] = 1.0f;
	cl.testSprite.renderEntity.materialParms[MATERIALPARM_TIMEOFFSET] = -MS2SEC(cl.time);
	cl.testSprite.renderEntity.materialParms[MATERIALPARM_DIVERSITY] = crand();
	cl.testSprite.renderEntity.materialParms[MATERIALPARM_MISC] = 0.0f;
	cl.testSprite.renderEntity.materialParms[MATERIALPARM_MODE] = 0.0f;
}

/*
 ==================
 CL_UpdateTestSprite
 ==================
*/
static void CL_UpdateTestSprite (){

	if (!cl.testSprite.active)
		return;

	// Add or update the render entity
	R_AddEntityToScene(&cl.testSprite.renderEntity);
}


/*
 ==============================================================================

 BEAM TESTING

 ==============================================================================
*/


/*
 ==================
 CL_ClearTestBeam
 ==================
*/
void CL_ClearTestBeam (){

	// Clear the test beam
	Mem_Fill(&cl.testBeam, 0, sizeof(testBeam_t));
}

/*
 ==================
 CL_TestBeam_f
 ==================
*/
void CL_TestBeam_f (){

	float	length;
	int		axis, segments;

	if (Cmd_Argc() > 6){
		Com_Printf("Usage: testBeam [material] [axis] [length] [width] [segments]\n");
		return;
	}

	if ((cls.state != CA_ACTIVE || cls.loading) || cls.playingCinematic || cl.demoPlayback){
		Com_Printf("You must be in a level to test a beam\n");
		return;
	}

	if (!CVar_AllowCheats()){
		Com_Printf("You must enable cheats to test a beam\n");
		return;
	}

	// Clear the old beam, if any
	CL_ClearTestBeam();

	if (Cmd_Argc() < 2)
		return;

	// Test the specified material on a beam
	cl.testBeam.active = true;

	// Set up the render entity
	cl.testBeam.renderEntity.type = RE_BEAM;

	VectorMA(cl.renderView.origin, 100.0f, cl.renderView.axis[0], cl.testBeam.renderEntity.origin);
	Matrix3_Identity(cl.testBeam.renderEntity.axis);

	if (Cmd_Argc() < 3)
		axis = 0;
	else
		axis = Clamp(Str_ToInteger(Cmd_Argv(2)), 0, 2);

	if (Cmd_Argc() < 4)
		VectorMA(cl.testBeam.renderEntity.origin, 100.0f, cl.renderView.axis[0], cl.testBeam.renderEntity.beamEnd);
	else {
		length = Str_ToFloat(Cmd_Argv(3));
		if (length <= 0.0f)
			length = 100.0f;

		VectorMA(cl.testBeam.renderEntity.origin, length, cl.renderView.axis[axis], cl.testBeam.renderEntity.beamEnd);
	}

	if (Cmd_Argc() < 5)
		cl.testBeam.renderEntity.beamWidth = 5.0f;
	else {
		cl.testBeam.renderEntity.beamWidth = Str_ToFloat(Cmd_Argv(4));
		if (cl.testBeam.renderEntity.beamWidth <= 0.0f)
			cl.testBeam.renderEntity.beamWidth = 5.0f;
	}

	if (Cmd_Argc() < 6)
		cl.testBeam.renderEntity.beamLength = 0.0f;
	else {
		segments = Str_ToInteger(Cmd_Argv(5));

		if (segments <= 0)
			cl.testBeam.renderEntity.beamLength = 0.0f;
		else
			cl.testBeam.renderEntity.beamLength = length / segments;
	}

	cl.testBeam.renderEntity.material = R_RegisterMaterial(Cmd_Argv(1), false);

	cl.testBeam.renderEntity.materialParms[MATERIALPARM_RED] = 1.0f;
	cl.testBeam.renderEntity.materialParms[MATERIALPARM_GREEN] = 1.0f;
	cl.testBeam.renderEntity.materialParms[MATERIALPARM_BLUE] = 1.0f;
	cl.testBeam.renderEntity.materialParms[MATERIALPARM_ALPHA] = 1.0f;
	cl.testBeam.renderEntity.materialParms[MATERIALPARM_TIMEOFFSET] = MS2SEC(cl.time);
	cl.testBeam.renderEntity.materialParms[MATERIALPARM_DIVERSITY] = crand();
	cl.testBeam.renderEntity.materialParms[MATERIALPARM_MISC] = 0.0f;
	cl.testBeam.renderEntity.materialParms[MATERIALPARM_MODE] = 0.0f;
}

/*
 ==================
 CL_UpdateTestBeam
 ==================
*/
static void CL_UpdateTestBeam (){

	if (!cl.testBeam.active)
		return;

	// Add or update the render entity
	R_AddEntityToScene(&cl.testBeam.renderEntity);
}


/*
 ==============================================================================

 SOUND TESTING

 ==============================================================================
*/


/*
 ==================
 
 ==================
*/
void CL_ClearTestSound (){

}

/*
 ==================
 
 ==================
*/
void CL_TestSound_f (){

}

/*
 ==================
 
 ==================
*/
static void CL_UpdateTestSound (){

}


/*
 ==============================================================================

 DECAL TESTING

 ==============================================================================
*/


/*
 ==================
 CL_TestDecal_f
 ==================
*/
void CL_TestDecal_f (){

	material_t	*material;
	trace_t		trace;
	vec3_t		point;
	float		radius;

	if (Cmd_Argc() != 2 && Cmd_Argc() != 3){
		Com_Printf("Usage: testDecal <material> [radius]\n");
		return;
	}

	if ((cls.state != CA_ACTIVE || cls.loading) || cls.playingCinematic || cl.demoPlayback){
		Com_Printf("You must be in a level to test a decal\n");
		return;
	}

	if (!CVar_AllowCheats()){
		Com_Printf("You must enable cheats to test a decal\n");
		return;
	}

	// Load the material
	material = R_RegisterMaterial(Cmd_Argv(1), false);

	// Get the radius
	if (Cmd_Argc() < 3)
		radius = 10.0f;
	else {
		radius = Str_ToFloat(Cmd_Argv(2));
		if (radius < 1.0f)
			radius = 1.0f;
	}

	// Trace to find an impact point
	VectorMA(cl.renderView.origin, 1000.0f, cl.renderView.axis[0], point);

	trace = CM_BoxTrace(cl.renderView.origin, point, vec3_origin, vec3_origin, 0, MASK_SOLID);

	if (trace.allsolid || trace.fraction == 1.0f)
		return;

	// Project a decal onto the world
//	R_ProjectDecalOntoWorld(trace.endpos, trace.plane.normal, rand() % 360, radius, 1.0f, 1.0f, 1.0f, 1.0f, false, material, false);
}


// ============================================================================


/*
 ==================
 CL_UpdateTestTools
 ==================
*/
static void CL_UpdateTestTools (){

	CL_UpdateTestModel();
	CL_UpdateTestSprite();
	CL_UpdateTestBeam();
	CL_UpdateTestSound();
}


/*
 ==============================================================================

 CLIENT VIEW

 ==============================================================================
*/


/*
 ==================
 CL_Where_f
 ==================
*/
void CL_Where_f (){

	if ((cls.state != CA_ACTIVE || cls.loading) || cls.playingCinematic || cl.demoPlayback){
		Com_Printf("You must be in a map to view the current position\n");
		return;
	}

	Com_Printf("(%i %i %i) : %i\n", (int)cl.renderView.origin[0], (int)cl.renderView.origin[1], (int)cl.renderView.origin[2], (int)cl.renderViewAngles[YAW]);
}

/*
 ==================
 
 ==================
*/
void CL_SizeUp_f (){

	CVar_SetVariableInteger("cl_viewSize", cl_viewSize->integerValue + 10, false);
	if (cl_viewSize->integerValue > 100)
		CVar_SetVariableInteger("cl_viewSize", 100, false);
}

/*
 ==================
 
 ==================
*/
void CL_SizeDown_f (){

	CVar_SetVariableInteger("cl_viewSize", cl_viewSize->integerValue - 10, false);
	if (cl_viewSize->integerValue < 30)
		CVar_SetVariableInteger("cl_viewSize", 30, false);
}


// ============================================================================


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