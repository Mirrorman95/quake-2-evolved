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
// cl_commands.c - client commands
//


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
static void CL_ClearTestModel (){

	cl.testModel.isGun = false;
	cl.testModel.active = false;
	Mem_Fill(&cl.testModel.name, 0, sizeof(cl.testModel.name));
	Mem_Fill(&cl.testModel.renderEntity, 0, sizeof(cl.testModel.renderEntity));
}

/*
 ==================
 CL_TestModel_f
 ==================
*/
static void CL_TestModel_f (){

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

	// Test the specified model
	cl.testModel.active = true;

	cl.testModel.isGun = false;

	Str_Copy(cl.testModel.name, Cmd_Argv(1), sizeof(cl.testModel.name));

	// Set up the render entity
	cl.testModel.renderEntity.type = RE_MODEL;

	VectorMA(cl.renderView.origin, 100.0f, cl.renderView.axis[0], cl.testModel.renderEntity.origin);
	Matrix3_Identity(cl.testModel.renderEntity.axis);

	cl.testModel.renderEntity.model = R_RegisterModel(Cmd_Argv(1));

	cl.testModel.renderEntity.hasSubview = false;
	VectorCopy(cl.renderView.origin, cl.testModel.renderEntity.subviewOrigin);
	VectorCopy(cl.renderViewAngles, cl.testModel.renderEntity.subviewAngles);
	cl.testModel.renderEntity.subviewFovX = 90.0f;
	cl.testModel.renderEntity.subviewFovY = 90.0f;

	cl.testModel.renderEntity.depthHack = false;

	cl.testModel.renderEntity.allowInView = VIEW_ALL;
	cl.testModel.renderEntity.allowShadowInView = VIEW_ALL;

	cl.testModel.renderEntity.material = NULL;

	cl.testModel.renderEntity.materialParms[MATERIALPARM_RED] = 1.0f;
	cl.testModel.renderEntity.materialParms[MATERIALPARM_GREEN] = 1.0f;
	cl.testModel.renderEntity.materialParms[MATERIALPARM_BLUE] = 1.0f;
	cl.testModel.renderEntity.materialParms[MATERIALPARM_ALPHA] = 1.0f;
	cl.testModel.renderEntity.materialParms[MATERIALPARM_TIMEOFFSET] = -MS2SEC(cl.time);
	cl.testModel.renderEntity.materialParms[MATERIALPARM_DIVERSITY] = crand();
	cl.testModel.renderEntity.materialParms[MATERIALPARM_MISC] = 0.0f;
	cl.testModel.renderEntity.materialParms[MATERIALPARM_MODE] = 0.0f;

	cl.testModel.active = true;
}

/*
 ==================
 CL_TestGun_f
 ==================
*/
static void CL_TestGun_f (){

	if (!cl.testModel.active){
		Com_Printf("No active testModel\n");
		return;
	}

	// Toggle gun testing
	if (cl.testModel.isGun){
		cl.testModel.isGun = false;

		VectorMA(cl.renderView.origin, 100.0f, cl.renderView.axis[0], cl.testModel.renderEntity.origin);
		Matrix3_Identity(cl.testModel.renderEntity.axis);

		cl.testModel.renderEntity.allowInView = VIEW_ALL;
		cl.testModel.renderEntity.allowShadowInView = VIEW_ALL;

		cl.testModel.renderEntity.depthHack = false;
	}
	else {
		cl.testModel.isGun = false;

		VectorCopy(cl.renderView.origin, cl.testModel.renderEntity.origin);
		Matrix3_Copy(cl.renderView.axis, cl.testModel.renderEntity.axis);

		cl.testModel.renderEntity.allowInView = VIEW_MAIN;
		cl.testModel.renderEntity.allowShadowInView = VIEW_NONE;

		cl.testModel.renderEntity.depthHack = true;
	}
}

/*
 ==================
 CL_TestMaterial_f
 ==================
*/
static void CL_TestMaterial_f (){

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
static void CL_TestMaterialParm_f (){

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
		cl.testModel.renderEntity.materialParms[index] = -MS2SEC(cl.time);
	else
		cl.testModel.renderEntity.materialParms[index] = Str_ToFloat(Cmd_Argv(2));
}

/*
 ==================
 CL_TestRemoteView_f
 ==================
*/
static void CL_TestRemoteView_f (){

	if (Cmd_Argc() != 4 && Cmd_Argc() != 7 && Cmd_Argc() != 9){
		Com_Printf("Usage: testRemoteView <origin x y z> [angles x y z] [fov x y]\n");
		return;
	}

	if (!cl.testModel.active){
		Com_Printf("No active testModel\n");
		return;
	}

	// Set up the subview with the specified values
	cl.testModel.renderEntity.hasSubview = true;

	cl.testModel.renderEntity.subviewOrigin[0] = Str_ToFloat(Cmd_Argv(1));
	cl.testModel.renderEntity.subviewOrigin[1] = Str_ToFloat(Cmd_Argv(2));
	cl.testModel.renderEntity.subviewOrigin[2] = Str_ToFloat(Cmd_Argv(3));

	if (Cmd_Argc() < 5)
		VectorClear(cl.testModel.renderEntity.subviewAngles);
	else {
		cl.testModel.renderEntity.subviewAngles[0] = Str_ToFloat(Cmd_Argv(4));
		cl.testModel.renderEntity.subviewAngles[1] = Str_ToFloat(Cmd_Argv(5));
		cl.testModel.renderEntity.subviewAngles[2] = Str_ToFloat(Cmd_Argv(6));

		AnglesNormalize360(cl.testModel.renderEntity.subviewAngles[PITCH], cl.testModel.renderEntity.subviewAngles[YAW], cl.testModel.renderEntity.subviewAngles[ROLL]);
	}

	if (Cmd_Argc() < 8){
		cl.testModel.renderEntity.subviewFovX = 90.0f;
		cl.testModel.renderEntity.subviewFovY = 90.0f;
	}
	else {
		cl.testModel.renderEntity.subviewFovX = ClampFloat(Str_ToFloat(Cmd_Argv(7)), 1.0f, 160.0f);
		cl.testModel.renderEntity.subviewFovY = ClampFloat(Str_ToFloat(Cmd_Argv(8)), 1.0f, 160.0f);
	}
}

/*
 ==================
 CL_PrevFrame_f
 ==================
*/
static void CL_PrevFrame_f (){

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
 CL_NextFrame_f
 ==================
*/
static void CL_NextFrame_f (){

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
 CL_PrevSkin_f
 ==================
*/
static void CL_PrevSkin_f (){

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
 CL_NextSkin_f
 ==================
*/
static void CL_NextSkin_f (){

	if (!cl.testModel.active){
		Com_Printf("No active testModel\n");
		return;
	}

	cl.testModel.renderEntity.skinIndex++;

	Com_Printf("Skin %i\n", cl.testModel.renderEntity.skinIndex);
}

/*
 ==================
 CL_UpdateTestModel

 TODO: animation does not work
 ==================
*/
static void CL_UpdateTestModel (){

	vec3_t	angles;

	if (!cl.testModel.active)
		return;

	// Update origin and axis
	if (cl.testModel.isGun){
		VectorCopy(cl.renderView.origin, cl.testModel.renderEntity.origin);
		Matrix3_Copy(cl.renderView.axis, cl.testModel.renderEntity.axis);

		VectorMA(cl.testModel.renderEntity.origin, cl_testGunX->floatValue, cl.testModel.renderEntity.axis[0], cl.testModel.renderEntity.origin);
		VectorMA(cl.testModel.renderEntity.origin, cl_testGunY->floatValue, cl.testModel.renderEntity.axis[1], cl.testModel.renderEntity.origin);
		VectorMA(cl.testModel.renderEntity.origin, cl_testGunZ->floatValue, cl.testModel.renderEntity.axis[2], cl.testModel.renderEntity.origin);
	}
	else {
		VectorSet(angles, cl_testModelPitch->floatValue, cl_testModelYaw->floatValue, cl_testModelRoll->floatValue);

		if (cl_testModelRotatePitch->floatValue)
			angles[PITCH] = cl_testModelRotatePitch->floatValue * MS2SEC(cl.time);
		if (cl_testModelRotateYaw->floatValue)
			angles[YAW] = cl_testModelRotateYaw->floatValue * MS2SEC(cl.time);
		if (cl_testModelRotateRoll->floatValue)
			angles[ROLL] = cl_testModelRotateRoll->floatValue * MS2SEC(cl.time);

		AnglesNormalize360(angles[PITCH], angles[YAW], angles[ROLL]);

		AnglesToMat3(angles, cl.testModel.renderEntity.axis);
	}

	// Update animation if needed
	if (cl_testModelAnimate->integerValue){
		if (cl.time - cl.testModel.time >= 100){
			cl.testModel.time = cl.time;

			cl.testModel.renderEntity.oldFrame = cl.testModel.renderEntity.frame;

			cl.testModel.renderEntity.frame++;
			if (cl.testModel.renderEntity.frame >= cl.testModel.frames)
				cl.testModel.renderEntity.frame = 0;
		}

		cl.testModel.renderEntity.backLerp = 1.0f - (cl.time - cl.testModel.time) * 0.01f;
	}
	else {
		cl.testModel.time = cl.time;

		cl.testModel.renderEntity.backLerp = 0.0f;
	}

	// Add or update the render entity
	R_AddEntityToScene(&cl.testModel.renderEntity);
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
static void CL_ClearTestSprite (){

	// Clear the test beam
	Mem_Fill(&cl.testSprite, 0, sizeof(testSprite_t));
}

/*
 ==================
 CL_TestSprite_f
 ==================
*/
static void CL_TestSprite_f (){

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

	cl.testSprite.renderEntity.hasSubview = false;

	cl.testSprite.renderEntity.depthHack = false;

	cl.testSprite.renderEntity.allowInView = VIEW_ALL;
	cl.testSprite.renderEntity.allowShadowInView = VIEW_NONE;

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
 CL_TestSpriteParm_f
 ==================
*/
static void CL_TestSpriteParm_f (){

	int		index;

	if (Cmd_Argc() != 3){
		Com_Printf("Usage: testSpriteParm <index> <value | \"time\">\n");
		return;
	}

	if (!cl.testSprite.active){
		Com_Printf("No active testSprite\n");
		return;
	}

	// Set the specified material parm to the specified value
	index = Str_ToInteger(Cmd_Argv(1));
	if (index < 0 || index >= MAX_MATERIAL_PARMS){
		Com_Printf("Specified index is out of range\n");
		return;
	}

	if (!Str_ICompare(Cmd_Argv(2), "time"))
		cl.testSprite.renderEntity.materialParms[index] = -MS2SEC(cl.time);
	else
		cl.testSprite.renderEntity.materialParms[index] = Str_ToFloat(Cmd_Argv(2));
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
static void CL_ClearTestBeam (){

	// Clear the test beam
	Mem_Fill(&cl.testBeam, 0, sizeof(testBeam_t));
}

/*
 ==================
 CL_TestBeam_f
 ==================
*/
static void CL_TestBeam_f (){

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

	cl.testBeam.renderEntity.hasSubview = false;

	cl.testBeam.renderEntity.depthHack = false;

	cl.testBeam.renderEntity.allowInView = VIEW_ALL;
	cl.testBeam.renderEntity.allowShadowInView = VIEW_NONE;

	cl.testBeam.renderEntity.material = R_RegisterMaterial(Cmd_Argv(1), false);

	cl.testBeam.renderEntity.materialParms[MATERIALPARM_RED] = 1.0f;
	cl.testBeam.renderEntity.materialParms[MATERIALPARM_GREEN] = 1.0f;
	cl.testBeam.renderEntity.materialParms[MATERIALPARM_BLUE] = 1.0f;
	cl.testBeam.renderEntity.materialParms[MATERIALPARM_ALPHA] = 1.0f;
	cl.testBeam.renderEntity.materialParms[MATERIALPARM_TIMEOFFSET] = -MS2SEC(cl.time);
	cl.testBeam.renderEntity.materialParms[MATERIALPARM_DIVERSITY] = crand();
	cl.testBeam.renderEntity.materialParms[MATERIALPARM_MISC] = 0.0f;
	cl.testBeam.renderEntity.materialParms[MATERIALPARM_MODE] = 0.0f;
}

/*
 ==================
 CL_TestBeamParm_f
 ==================
*/
static void CL_TestBeamParm_f (){

	int		index;

	if (Cmd_Argc() != 3){
		Com_Printf("Usage: testBeamParm <index> <value | \"time\">\n");
		return;
	}

	if (!cl.testBeam.active){
		Com_Printf("No active testBeam\n");
		return;
	}

	// Set the specified material parm to the specified value
	index = Str_ToInteger(Cmd_Argv(1));
	if (index < 0 || index >= MAX_MATERIAL_PARMS){
		Com_Printf("Specified index is out of range\n");
		return;
	}

	if (!Str_ICompare(Cmd_Argv(2), "time"))
		cl.testBeam.renderEntity.materialParms[index] = -MS2SEC(cl.time);
	else
		cl.testBeam.renderEntity.materialParms[index] = Str_ToFloat(Cmd_Argv(2));
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

#define ENTITYNUM_TEST 1024


/*
 ==================
 CL_ClearTestSound
 ==================
*/
static void CL_ClearTestSound (){

	// Remove the sound emitter
	if (cl.testSound.emitterHandle)
		S_RemoveSoundEmitter(cl.testSound.emitterHandle, true);

	// Clear the test sound
	Mem_Fill(&cl.testSound, 0, sizeof(testSound_t));
}

/*
 ==================
 CL_TestSound_f
 ==================
*/
static void CL_TestSound_f (){

	if (Cmd_Argc() != 1 && Cmd_Argc() != 2 && Cmd_Argc() != 5){
		Com_Printf("Usage: testSound [soundShader] [direction x y z]\n");
		return;
	}

	if ((cls.state != CA_ACTIVE || cls.loading) || cls.playingCinematic || cl.demoPlayback){
		Com_Printf("You must be in a level to test a sound\n");
		return;
	}

	if (!CVar_AllowCheats()){
		Com_Printf("You must enable cheats to test a sound\n");
		return;
	}

	// Clear the old sound, if any
	CL_ClearTestSound();

	if (Cmd_Argc() < 2)
		return;

	// Test the specified sound
	cl.testSound.active = true;

	cl.testSound.played = false;

	// Load the sound shader
	cl.testSound.soundShader = S_RegisterSoundShader(Cmd_Argv(1));

	// Set up the sound emitter
	cl.testSound.soundEmitter.emitterId = ENTITYNUM_TEST;

	VectorMA(cl.renderView.origin, 100.0f, cl.renderView.axis[0], cl.testSound.soundEmitter.origin);
	VectorClear(cl.testSound.soundEmitter.velocity);

	if (Cmd_Argc() == 2)
		VectorClear(cl.testSound.soundEmitter.direction);
	else {
		cl.testSound.soundEmitter.direction[0] = Str_ToFloat(Cmd_Argv(2));
		cl.testSound.soundEmitter.direction[1] = Str_ToFloat(Cmd_Argv(3));
		cl.testSound.soundEmitter.direction[2] = Str_ToFloat(Cmd_Argv(4));

		VectorNormalize(cl.testSound.soundEmitter.direction);
	}

	Matrix3_Identity(cl.testSound.soundEmitter.axis);

	cl.testSound.soundEmitter.area = CM_PointInArea(cl.testSound.soundEmitter.origin, 0);
	cl.testSound.soundEmitter.underwater = false;

	cl.testSound.soundEmitter.soundParms[SOUNDPARM_VOLUME] = 1.0f;
	cl.testSound.soundEmitter.soundParms[SOUNDPARM_PITCH] = 1.0f;
	cl.testSound.soundEmitter.soundParms[SOUNDPARM_DRYFILTER] = 1.0f;
	cl.testSound.soundEmitter.soundParms[SOUNDPARM_DRYFILTERHF] = 1.0f;
	cl.testSound.soundEmitter.soundParms[SOUNDPARM_WETFILTER] = 1.0f;
	cl.testSound.soundEmitter.soundParms[SOUNDPARM_WETFILTERHF] = 1.0f;
	cl.testSound.soundEmitter.soundParms[SOUNDPARM_MINDISTANCE] = 0.0f;
	cl.testSound.soundEmitter.soundParms[SOUNDPARM_MAXDISTANCE] = 0.0f;
}

/*
 ==================
 CL_TestSoundParm_f
 ==================
*/
static void CL_TestSoundParm_f (){

	int		index;

	if (Cmd_Argc() != 3){
		Com_Printf("Usage: testSoundParm <index> <value>\n");
		return;
	}

	if (!cl.testSound.active){
		Com_Printf("No active testSound\n");
		return;
	}

	// Set the specified sound parm to the specified value
	index = Str_ToInteger(Cmd_Argv(1));
	if (index < 0 || index >= MAX_SOUND_PARMS){
		Com_Printf("Specified index is out of range\n");
		return;
	}

	cl.testSound.soundEmitter.soundParms[index] = Str_ToFloat(Cmd_Argv(2));
}

/*
 ==================
 CL_UpdateTestSound
 ==================
*/
static void CL_UpdateTestSound (){

	if (!cl.testSound.active)
		return;

	// Update underwater
	if (CL_PointContents(cl.testSound.soundEmitter.origin, -1) & MASK_WATER)
		cl.testSound.soundEmitter.underwater = true;
	else
		cl.testSound.soundEmitter.underwater = false;

	// Add or update the sound emitter
	if (!cl.testSound.emitterHandle)
		cl.testSound.emitterHandle = S_AddSoundEmitter(&cl.testSound.soundEmitter);
	else
		S_UpdateSoundEmitter(cl.testSound.emitterHandle, &cl.testSound.soundEmitter);

	if (!cl.testSound.emitterHandle)
		return;

	// If currently playing, do nothing
	if (S_PlayingFromEmitter(cl.testSound.emitterHandle, SOUND_CHANNEL_ANY))
		return;

	// If done playing, remove it
	if (cl.testSound.played){
		CL_ClearTestSound();
		return;
	}

	// Play the sound
	cl.testSound.played = true;

	S_PlaySoundFromEmitter(cl.testSound.emitterHandle, SOUND_CHANNEL_ANY, cl.time, cl.testSound.soundShader);
}


/*
 ==============================================================================

 POST-PROCESS MATERIAL TESTING

 ==============================================================================
*/


/*
 ==================
 CL_ClearTestPostProcess
 ==================
*/
static void CL_ClearTestPostProcess (){

	// Clear the test post-process
	Mem_Fill(&cl.testPostProcess, 0, sizeof(testPostProcess_t));
}

/*
 ==================
 CL_TestPostProcess_f
 ==================
*/
static void CL_TestPostProcess_f (){

	if (Cmd_Argc() > 2){
		Com_Printf("Usage: testPostProcess [material]\n");
		return;
	}

	if ((cls.state != CA_ACTIVE || cls.loading) || cls.playingCinematic || cl.demoPlayback){
		Com_Printf("You must be in a level to test a post-process material\n");
		return;
	}

	if (!CVar_AllowCheats()){
		Com_Printf("You must enable cheats to test a post-process material\n");
		return;
	}

	// Clear the old post-process material, if any
	CL_ClearTestPostProcess();

	if (Cmd_Argc() != 2)
		return;

	// Test the specified post-process material
	cl.testPostProcess.active = true;

	// Set up the post-process material
	cl.testPostProcess.material = R_RegisterMaterialNoMip(Cmd_Argv(1));

	cl.testPostProcess.materialParms[MATERIALPARM_RED] = 1.0f;
	cl.testPostProcess.materialParms[MATERIALPARM_GREEN] = 1.0f;
	cl.testPostProcess.materialParms[MATERIALPARM_BLUE] = 1.0f;
	cl.testPostProcess.materialParms[MATERIALPARM_ALPHA] = 1.0f;
	cl.testPostProcess.materialParms[MATERIALPARM_TIMEOFFSET] = -MS2SEC(cl.time);
	cl.testPostProcess.materialParms[MATERIALPARM_DIVERSITY] = crand();
	cl.testPostProcess.materialParms[MATERIALPARM_MISC] = 0.0f;
	cl.testPostProcess.materialParms[MATERIALPARM_MODE] = 0.0f;
}

/*
 ==================
 CL_TestPostProcessParm_f
 ==================
*/
static void CL_TestPostProcessParm_f (){

	int		index;

	if (Cmd_Argc() != 3){
		Com_Printf("Usage: testPostProcessParm <index> <value | \"time\">\n");
		return;
	}

	if (!cl.testPostProcess.active){
		Com_Printf("No active testPostProcess\n");
		return;
	}

	// Set the specified material parm to the specified value
	index = Str_ToInteger(Cmd_Argv(1));
	if (index < 0 || index >= MAX_MATERIAL_PARMS){
		Com_Printf("Specified index is out of range\n");
		return;
	}

	if (!Str_ICompare(Cmd_Argv(2), "time"))
		cl.testPostProcess.materialParms[index] = -MS2SEC(cl.time);
	else
		cl.testPostProcess.materialParms[index] = Str_ToFloat(Cmd_Argv(2));
}


/*
 ==================
 CL_UpdateTestPostProcess
 ==================
*/
static void CL_UpdateTestPostProcess (){

	if (!cl.testPostProcess.active)
		return;

	// Draw the material
	R_SetColor1(1.0f);
	R_SetParameters(cl.testPostProcess.materialParms);
	R_DrawStretchPic(0.0f, 0.0f, SCREEN_WIDTH, SCREEN_HEIGHT, 0.0f, 0.0f, 1.0f, 1.0f, H_SCALE, 1.0f, V_SCALE, 1.0f, cl.testPostProcess.material);
}


/*
 ==============================================================================

 DECAL TESTING

 ==============================================================================
*/


/*
 ==================
 
 ==================
*/
static void CL_TestDecal_f (){

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
static void CL_Where_f (){

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
static void CL_SizeUp_f (){

	CVar_SetVariableInteger("cl_viewSize", cl_viewSize->integerValue + 10, false);
	if (cl_viewSize->integerValue > 100)
		CVar_SetVariableInteger("cl_viewSize", 100, false);
}

/*
 ==================
 
 ==================
*/
static void CL_SizeDown_f (){

	CVar_SetVariableInteger("cl_viewSize", cl_viewSize->integerValue - 10, false);
	if (cl_viewSize->integerValue < 30)
		CVar_SetVariableInteger("cl_viewSize", 30, false);
}


// ============================================================================


/*
 ==================
 CL_UpdateTestTools
 ==================
*/
void CL_UpdateTestTools (){

	CL_UpdateTestModel();
	CL_UpdateTestSprite();
	CL_UpdateTestBeam();
	CL_UpdateTestSound();
	CL_UpdateTestPostProcess();
}

/*
 ==================
 CL_ClearTestTools
 ==================
*/
void CL_ClearTestTools (){

	CL_ClearTestModel();
	CL_ClearTestSprite();
	CL_ClearTestBeam();
	CL_ClearTestSound();
	CL_ClearTestPostProcess();
}

/*
 ==================
 CL_AddCommands
 ==================
*/
void CL_AddCommands (){

	Cmd_AddCommand("testModel", CL_TestModel_f, "Tests a model", Cmd_ArgCompletion_ModelName);
	Cmd_AddCommand("testGun", CL_TestGun_f, "Tests the current test model as a gun model", NULL);
	Cmd_AddCommand("testMaterial", CL_TestMaterial_f, "Tests a material on the current test model", Cmd_ArgCompletion_MaterialName);
	Cmd_AddCommand("testMaterialParm", CL_TestMaterialParm_f, "Tests a material parm on the current test model", NULL);
	Cmd_AddCommand("testRemoteView", CL_TestRemoteView_f, "Tests a remote view on the current test model", NULL);
	Cmd_AddCommand("nextFrame", CL_NextFrame_f, NULL, NULL);
	Cmd_AddCommand("prevFrame", CL_PrevFrame_f, NULL, NULL);
	Cmd_AddCommand("nextSkin", CL_NextSkin_f, NULL, NULL);
	Cmd_AddCommand("prevSkin", CL_PrevSkin_f, NULL, NULL);
	Cmd_AddCommand("testSprite", CL_TestSprite_f, "Tests a sprite", Cmd_ArgCompletion_MaterialName);
	Cmd_AddCommand("testSpriteParm", CL_TestSpriteParm_f, "Tests a material parm on the current test sprite", NULL);
	Cmd_AddCommand("testBeam", CL_TestBeam_f, "Tests a beam", Cmd_ArgCompletion_MaterialName);
	Cmd_AddCommand("testBeamParm", CL_TestBeamParm_f, "Tests a material parm on the current test beam", NULL);
	Cmd_AddCommand("testSound", CL_TestSound_f, "Tests a sound", NULL);
	Cmd_AddCommand("testSoundParm", CL_TestSoundParm_f, "Tests a sound parm on the current test sound", NULL);
	Cmd_AddCommand("testPostProcess", CL_TestPostProcess_f, "Tests a post-process material", Cmd_ArgCompletion_MaterialName);
	Cmd_AddCommand("testPostProcessParm", CL_TestPostProcessParm_f, "Tests a post-process parm on the current test post-process", NULL);
	Cmd_AddCommand("testDecal", CL_TestDecal_f, "Tests a decal", Cmd_ArgCompletion_MaterialName);	
	Cmd_AddCommand("where", CL_Where_f, "Shows the current view position and angles", NULL);
	Cmd_AddCommand("sizeUp", CL_SizeUp_f, NULL, NULL);
	Cmd_AddCommand("sizeDown", CL_SizeDown_f, NULL, NULL);
}

/*
 ==================
 CL_ShutdownCommands
 ==================
*/
void CL_ShutdownCommands (){

	Cmd_RemoveCommand("testModel");
	Cmd_RemoveCommand("testGun");
	Cmd_RemoveCommand("testMaterial");
	Cmd_RemoveCommand("testMaterialParm");
	Cmd_RemoveCommand("testRemoteView");
	Cmd_RemoveCommand("nextFrame");
	Cmd_RemoveCommand("prevFrame");
	Cmd_RemoveCommand("nextSkin");
	Cmd_RemoveCommand("prevSkin");
	Cmd_RemoveCommand("testSprite");
	Cmd_RemoveCommand("testSpriteParm");
	Cmd_RemoveCommand("testBeam");
	Cmd_RemoveCommand("testBeamParm");
	Cmd_RemoveCommand("testSound");
	Cmd_RemoveCommand("testSoundParm");
	Cmd_RemoveCommand("testPostProcess");
	Cmd_RemoveCommand("testPostProcessParm");
	Cmd_RemoveCommand("testDecal");
	Cmd_RemoveCommand("where");
	Cmd_RemoveCommand("sizeIp");
	Cmd_RemoveCommand("sizeDown");
}