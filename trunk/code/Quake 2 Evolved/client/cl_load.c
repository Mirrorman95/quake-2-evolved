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
// cl_load.c - Assets loading and caching
//

// TODO:
// - rewrite most of the mess
// - Replace Com_StripExtension


#include "client.h"


/*
 ==============================================================================

 UPDATE LOADING SCREEN

 ==============================================================================
*/


/*
 ==================
 CL_UpdateLoading
 ==================
*/
static void CL_UpdateLoading (const char *string){

	Str_Copy(cls.loadingInfo.string, string, sizeof(cls.loadingInfo.string));
	cls.loadingInfo.percent += 10;

	Sys_ProcessEvents();
	CL_UpdateScreen();
}


/*
 ==============================================================================

 MEDIA REGISTRATION

 ==============================================================================
*/


/*
 ==================
 CL_RegisterCollisionMap
 ==================
*/
static void CL_RegisterCollisionMap (){

	uint	checkCount;

	CL_UpdateLoading("COLLISION MAP");

	CM_LoadMap(cl.configStrings[CS_MODELS + 1], true, &checkCount);

	if (checkCount != Str_ToInteger(cl.configStrings[CS_MAPCHECKSUM]))
		Com_Error(ERR_DROP, "Local map version differs from server: %i != %s", checkCount, cl.configStrings[CS_MAPCHECKSUM]);
}

/*
 ==================
 CL_RegisterSounds
 ==================
*/
static void CL_RegisterSounds (){

	int		i;

	// Register sounds
	CL_UpdateLoading("SOUNDS");

	cl.media.sfxRichotecs[0] = S_RegisterSound("world/ric1.wav", 0);
	cl.media.sfxRichotecs[1] = S_RegisterSound("world/ric2.wav", 0);
	cl.media.sfxRichotecs[2] = S_RegisterSound("world/ric3.wav", 0);
	cl.media.sfxSparks[0] = S_RegisterSound("world/spark5.wav", 0);
	cl.media.sfxSparks[1] = S_RegisterSound("world/spark6.wav", 0);
	cl.media.sfxSparks[2] = S_RegisterSound("world/spark7.wav", 0);
	cl.media.sfxFootSteps[0] = S_RegisterSound("player/step1.wav", 0);
	cl.media.sfxFootSteps[1] = S_RegisterSound("player/step2.wav", 0);
	cl.media.sfxFootSteps[2] = S_RegisterSound("player/step3.wav", 0);
	cl.media.sfxFootSteps[3] = S_RegisterSound("player/step4.wav", 0);
	cl.media.sfxLaserHit = S_RegisterSound("weapons/lashit.wav", 0);
	cl.media.sfxRailgun = S_RegisterSound("weapons/railgf1a.wav", 0);
	cl.media.sfxRocketExplosion = S_RegisterSound("weapons/rocklx1a.wav", 0);
	cl.media.sfxGrenadeExplosion = S_RegisterSound("weapons/grenlx1a.wav", 0);
	cl.media.sfxWaterExplosion = S_RegisterSound("weapons/xpld_wat.wav", 0);
	cl.media.sfxMachinegunBrass = S_RegisterSound("weapons/brass_bullet.wav", 0);
	cl.media.sfxShotgunBrass = S_RegisterSound("weapons/brass_shell.wav", 0);

	if (!Str_ICompare(cl.gameDir, "rogue")){
		cl.media.sfxLightning = S_RegisterSound("weapons/tesla.wav", 0);
		cl.media.sfxDisruptorExplosion = S_RegisterSound("weapons/disrupthit.wav", 0);
	}

	S_RegisterSound("player/land1.wav", 0);
	S_RegisterSound("player/fall2.wav", 0);
	S_RegisterSound("player/fall1.wav", 0);

	// Register the sounds that the server references
	CL_UpdateLoading("GAME SOUNDS");

	for (i = 1; i < MAX_SOUNDS; i++){
		if (!cl.configStrings[CS_SOUNDS+i][0])
			break;
	
		cl.media.gameSounds[i] = S_RegisterSound(cl.configStrings[CS_SOUNDS+i], 0);
	}
}

/*
 ==================
 CL_RegisterGraphics

 TODO: replace Com_StripExtension
 TODO: some material names needs to be changed
 ==================
*/
static void CL_RegisterGraphics (){

	int		i;
	float	skyRotate;
	vec3_t	skyAxis;
	char	name[MAX_QPATH];

	// Load the map
	CL_UpdateLoading("MAP");

	skyRotate = Str_ToFloat(cl.configStrings[CS_SKYROTATE]);
	sscanf(cl.configStrings[CS_SKYAXIS], "%f %f %f", &skyAxis[0], &skyAxis[1], &skyAxis[2]);

	R_LoadMap(cl.configStrings[CS_MODELS+1], cl.configStrings[CS_SKY], skyRotate, skyAxis);

	// Register models
	CL_UpdateLoading("MODELS");

	cl.media.parasiteBeamModel = R_RegisterModel("models/monsters/parasite/segment/tris.md2");
	cl.media.powerScreenShellModel = R_RegisterModel("models/items/armor/effect/tris.md2");
	cl.media.machinegunBrassModel = R_RegisterModel("models/misc/b_shell/tris.md3");
	cl.media.shotgunBrassModel = R_RegisterModel("models/misc/s_shell/tris.md3");

	R_RegisterModel("models/objects/laser/tris.md2");
	R_RegisterModel("models/objects/grenade2/tris.md2");
	R_RegisterModel("models/weapons/v_machn/tris.md2");
	R_RegisterModel("models/weapons/v_handgr/tris.md2");
	R_RegisterModel("models/weapons/v_shotg2/tris.md2");
	R_RegisterModel("models/objects/gibs/bone/tris.md2");
	R_RegisterModel("models/objects/gibs/sm_meat/tris.md2");
	R_RegisterModel("models/objects/gibs/bone2/tris.md2");

	// Register the models that the server references
	CL_UpdateLoading("GAME MODELS");

	Str_Copy(cl.weaponModels[0], "weapon", sizeof(cl.weaponModels[0]));
	cl.numWeaponModels = 1;

	for (i = 1; i < MAX_MODELS; i++){
		if (!cl.configStrings[CS_MODELS + i][0])
			break;

		if (cl.configStrings[CS_MODELS + i][0] == '#'){
			// Special player weapon model
			if (cl.numWeaponModels < MAX_CLIENTWEAPONMODELS){
				Com_StripExtension(cl.configStrings[CS_MODELS + i] + 1, cl.weaponModels[cl.numWeaponModels], sizeof(cl.weaponModels[cl.numWeaponModels]));
				cl.numWeaponModels++;
			}
		} 
		else {
			cl.media.gameModels[i] = R_RegisterModel(cl.configStrings[CS_MODELS + i]);

			if (cl.configStrings[CS_MODELS + i][0] == '*')
				cl.media.gameCModels[i] = CM_InlineModel(cl.configStrings[CS_MODELS + i]);
			else
				cl.media.gameCModels[i] = NULL;
		}
	}

	// Register materials
	CL_UpdateLoading("MATERIALS");

	cl.media.lagometerMaterial = R_RegisterMaterialNoMip("lagometer");
	cl.media.disconnectedMaterial = R_RegisterMaterialNoMip("disconnected");
	cl.media.backTileMaterial = R_RegisterMaterialNoMip("backTile");
	cl.media.pauseMaterial = R_RegisterMaterialNoMip("pause");

	for (i = 0; i < NUM_CROSSHAIRS; i++)
		cl.media.crosshairMaterials[i] = R_RegisterMaterialNoMip(Str_VarArgs("gfx/crosshairs/crosshair%i", i + 1));

	for (i = 0; i < 11; i++){
		if (i != 10){
			cl.media.hudNumberMaterials[0][i] = R_RegisterMaterialNoMip(Str_VarArgs("pics/num_%i", i));
			cl.media.hudNumberMaterials[1][i] = R_RegisterMaterialNoMip(Str_VarArgs("pics/anum_%i", i));
		}
		else {
			cl.media.hudNumberMaterials[0][i] = R_RegisterMaterialNoMip("pics/num_minus");
			cl.media.hudNumberMaterials[1][i] = R_RegisterMaterialNoMip("pics/anum_minus");
		}
	}

	cl.media.bloodBlendMaterial = R_RegisterMaterial("bloodBlend", false);

	cl.media.fireScreenMaterial = R_RegisterMaterialNoMip("fireScreen");
	cl.media.waterBlurMaterial = R_RegisterMaterialNoMip("waterBlur");
	cl.media.doubleVisionMaterial = R_RegisterMaterialNoMip("doubleVision");
	cl.media.underWaterVisionMaterial = R_RegisterMaterialNoMip("underWaterVision");
	cl.media.irGogglesMaterial = R_RegisterMaterialNoMip("irGoggles");

	cl.media.rocketExplosionMaterial = R_RegisterMaterial("rocketExplosion", false);
	cl.media.rocketExplosionWaterMaterial = R_RegisterMaterial("rocketExplosionWater", false);
	cl.media.grenadeExplosionMaterial = R_RegisterMaterial("grenadeExplosion", false);
	cl.media.grenadeExplosionWaterMaterial = R_RegisterMaterial("grenadeExplosionWater", false);
	cl.media.bfgExplosionMaterial = R_RegisterMaterial("bfgExplosion", false);
	cl.media.bfgBallMaterial = R_RegisterMaterial("bfgBall", false);
	cl.media.plasmaBallMaterial = R_RegisterMaterial("plasmaBall", false);
	cl.media.waterPlumeMaterial = R_RegisterMaterial("waterPlume", false);
	cl.media.waterSprayMaterial = R_RegisterMaterial("waterSpray", false);
	cl.media.waterWakeMaterial = R_RegisterMaterial("waterWake", false);
	cl.media.nukeShockwaveMaterial = R_RegisterMaterial("nukeShockwave", false);
	cl.media.bloodSplatMaterial[0] = R_RegisterMaterial("bloodSplat", false);
	cl.media.bloodSplatMaterial[1] = R_RegisterMaterial("greenBloodSplat", false);
	cl.media.bloodCloudMaterial[0] = R_RegisterMaterial("bloodCloud", false);
	cl.media.bloodCloudMaterial[1] = R_RegisterMaterial("greenBloodCloud", false);

	cl.media.powerScreenShellMaterial = R_RegisterMaterial("gfx/effects/shells/powerScreen", false);
	cl.media.invulnerabilityShellMaterial = R_RegisterMaterial("gfx/effects/shells/invulnerability", false);
	cl.media.quadDamageShellMaterial = R_RegisterMaterial("gfx/effects/shells/quadDamage", false);
	cl.media.doubleDamageShellMaterial = R_RegisterMaterial("gfx/effects/shells/doubleDamage", false);
	cl.media.halfDamageShellMaterial = R_RegisterMaterial("gfx/effects/shells/halfDamage", false);
	cl.media.genericShellMaterial = R_RegisterMaterial("gfx/effects/shells/generic", false);

	cl.media.laserBeamMaterial = R_RegisterMaterial("gfx/beams/laser", false);
	cl.media.grappleBeamMaterial = R_RegisterMaterial("gfx/beams/grapple", false);
	cl.media.lightningBeamMaterial = R_RegisterMaterial("gfx/beams/lightning", false);
	cl.media.heatBeamMaterial = R_RegisterMaterial("gfx/beams/heat", false);

	cl.media.energyParticleMaterial = R_RegisterMaterial("gfx/effects/particles/energy", false);
	cl.media.glowParticleMaterial = R_RegisterMaterial("gfx/effects/particles/glow", false);
	cl.media.flameParticleMaterial = R_RegisterMaterial("gfx/effects/particles/flame", false);
	cl.media.smokeParticleMaterial = R_RegisterMaterial("gfx/effects/particles/smoke", false);
	cl.media.liteSmokeParticleMaterial = R_RegisterMaterial("gfx/effects/particles/liteSmoke", false);
	cl.media.bubbleParticleMaterial = R_RegisterMaterial("gfx/effects/particles/bubble", false);
	cl.media.dropletParticleMaterial = R_RegisterMaterial("gfx/effects/particles/droplet", false);
	cl.media.steamParticleMaterial = R_RegisterMaterial("gfx/effects/particles/steam", false);
	cl.media.sparkParticleMaterial = R_RegisterMaterial("gfx/effects/particles/spark", false);
	cl.media.impactSparkParticleMaterial = R_RegisterMaterial("gfx/effects/particles/impactSpark", false);
	cl.media.trackerParticleMaterial = R_RegisterMaterial("gfx/effects/particles/tracker", false);
	cl.media.flyParticleMaterial = R_RegisterMaterial("gfx/effects/particles/fly", false);

	cl.media.energyMarkMaterial = R_RegisterMaterial("gfx/decals/energyMark", false);
	cl.media.bulletMarkMaterial = R_RegisterMaterial("gfx/decals/bulletMark", false);
	cl.media.burnMarkMaterial = R_RegisterMaterial("gfx/decals/burnMark", false);
	cl.media.bloodMarkMaterials[0][0] = R_RegisterMaterial("gfx/decals/bloodMark1", false);
	cl.media.bloodMarkMaterials[0][1] = R_RegisterMaterial("gfx/decals/bloodMark2", false);
	cl.media.bloodMarkMaterials[0][2] = R_RegisterMaterial("gfx/decals/bloodMark3", false);
	cl.media.bloodMarkMaterials[0][3] = R_RegisterMaterial("gfx/decals/bloodMark4", false);
	cl.media.bloodMarkMaterials[0][4] = R_RegisterMaterial("gfx/decals/bloodMark5", false);
	cl.media.bloodMarkMaterials[0][5] = R_RegisterMaterial("gfx/decals/bloodMark6", false);
	cl.media.bloodMarkMaterials[1][0] = R_RegisterMaterial("gfx/decals/greenBloodMark1", false);
	cl.media.bloodMarkMaterials[1][1] = R_RegisterMaterial("gfx/decals/greenBloodMark2", false);
	cl.media.bloodMarkMaterials[1][2] = R_RegisterMaterial("gfx/decals/greenBloodMark3", false);
	cl.media.bloodMarkMaterials[1][3] = R_RegisterMaterial("gfx/decals/greenBloodMark4", false);
	cl.media.bloodMarkMaterials[1][4] = R_RegisterMaterial("gfx/decals/greenBloodMark5", false);
	cl.media.bloodMarkMaterials[1][5] = R_RegisterMaterial("gfx/decals/greenBloodMark6", false);

	R_RegisterMaterialNoMip("pics/w_machinegun");
	R_RegisterMaterialNoMip("pics/a_bullets");
	R_RegisterMaterialNoMip("pics/i_health");
	R_RegisterMaterialNoMip("pics/a_grenades");

	// Register the materials that the server references
	CL_UpdateLoading("GAME MATERIALS");

	for (i = 1; i < MAX_IMAGES; i++){
		if (!cl.configStrings[CS_IMAGES + i][0])
			break;

		if (!Str_FindChar(cl.configStrings[CS_IMAGES + i], '/'))
			Str_SPrintf(name, sizeof(name), "pics/%s", cl.configStrings[CS_IMAGES + i]);
		else
			Com_StripExtension(cl.configStrings[CS_IMAGES + i], name, sizeof(name));

		cl.media.gameMaterials[i] = R_RegisterMaterialNoMip(name);
	}
}

/*
 ==================
 CL_RegisterClients
 ==================
*/
static void CL_RegisterClients (){

	int		i;

	// Register all the clients in the server
	CL_UpdateLoading("CLIENTS");

	CL_LoadClientInfo(&cl.baseClientInfo, "unnamed\\male/grunt");

	for (i = 0; i < MAX_CLIENTS; i++){
		if (!cl.configStrings[CS_PLAYERSKINS + i][0])
			continue;

		CL_LoadClientInfo(&cl.clientInfo[i], cl.configStrings[CS_PLAYERSKINS + i]);
	}
}


/*
 ==============================================================================

 LOADING AND DRAWING

 ==============================================================================
*/


/*
 ==================
 CL_Loading
 ==================
*/
void CL_Loading (){

	if (cls.loading)
		return;

	cls.loading = true;
	Mem_Fill(&cls.loadingInfo, 0, sizeof(loadingInfo_t));

	// Make sure sounds aren't playing
	S_StopAllSounds();

	// Force menu and console off
	UI_SetActiveMenu(UI_CLOSEMENU);
	Con_Close();
}

/*
 ==================
 CL_LoadClientInfo
 ==================
*/
void CL_LoadClientInfo (clientInfo_t *clientInfo, const char *string){

	char	model[MAX_QPATH], skin[MAX_QPATH], name[MAX_QPATH];
	char	checkMD3[MAX_QPATH], checkMD2[MAX_QPATH];
	char	checkTGA[MAX_QPATH], checkPCX[MAX_QPATH];
	char	*ch;
	int		i;

	Mem_Fill(clientInfo, 0, sizeof(clientInfo_t));

	// Isolate the player name
	Str_Copy(clientInfo->name, string, sizeof(clientInfo->name));

	ch = Str_FindChar(string, '\\');
	if (ch){
		clientInfo->name[ch - string] = 0;
		string = ch + 1;
	}

	if (cl_noSkins->integerValue || *string == 0){
		// No custom skins or bad info string, so just use male/grunt
		clientInfo->model = R_RegisterModel("players/male/tris.md2");
		clientInfo->skin = R_RegisterMaterial("players/male/grunt", true);
		clientInfo->icon = R_RegisterMaterialNoMip("players/male/grunt_i");
		clientInfo->weaponModel[0] = R_RegisterModel("players/male/weapon.md2");

		// Save model/skin in the info string
		Str_SPrintf(clientInfo->info, sizeof(clientInfo->info), "male/grunt");

		clientInfo->valid = true;
		return;
	}

	// Isolate model and skin name
	Str_Copy(model, string, sizeof(model));

	ch = Str_FindChar(model, '/');
	if (!ch)
		ch = Str_FindChar(model, '\\');
	if (ch){
		*ch++ = 0;
		Str_Copy(skin, ch, sizeof(skin));
	}
	else
		skin[0] = 0;

	// If the model doesn't exist, default to male
	Str_SPrintf(checkMD3, sizeof(checkMD3), "players/%s/tris.md3", model);
	Str_SPrintf(checkMD2, sizeof(checkMD2), "players/%s/tris.md2", model);
	if (!FS_FileExists(checkMD3) && !FS_FileExists(checkMD2))
		Str_Copy(model, "male", sizeof(model));

	// If the skin doesn't exist, default to male/grunt
	Str_SPrintf(checkTGA, sizeof(checkTGA), "players/%s/%s.tga", model, skin);
	Str_SPrintf(checkPCX, sizeof(checkPCX), "players/%s/%s.pcx", model, skin);
	if (!FS_FileExists(checkTGA) && !FS_FileExists(checkPCX)){
		Str_Copy(model, "male", sizeof(model));
		Str_Copy(skin, "grunt", sizeof(skin));
	}

	// If the icon doesn't exist, default to male/grunt
	Str_SPrintf(checkTGA, sizeof(checkTGA), "players/%s/%s_i.tga", model, skin);
	Str_SPrintf(checkPCX, sizeof(checkPCX), "players/%s/%s_i.pcx", model, skin);
	if (!FS_FileExists(checkTGA) && !FS_FileExists(checkPCX)){
		Str_Copy(model, "male", sizeof(model));
		Str_Copy(skin, "grunt", sizeof(skin));
	}

	// If a weapon model doesn't exist, default to male/grunt
	for (i = 0; i < cl.numWeaponModels; i++){
		Str_SPrintf(checkMD3, sizeof(checkMD3), "players/%s/%s.md3", model, cl.weaponModels[i]);
		Str_SPrintf(checkMD2, sizeof(checkMD2), "players/%s/%s.md2", model, cl.weaponModels[i]);
		if (!FS_FileExists(checkMD3) && !FS_FileExists(checkMD2)){
			Str_Copy(model, "male", sizeof(model));
			Str_Copy(skin, "grunt", sizeof(skin));
			break;
		}

		if (!cl_visibleWeapons->integerValue)
			break;		// Only one if no visible weapons
	}

	// We can now load everything
	Str_SPrintf(name, sizeof(name), "players/%s/tris.md2", model);
	clientInfo->model = R_RegisterModel(name);

	Str_SPrintf(name, sizeof(name), "players/%s/%s", model, skin);
	clientInfo->skin = R_RegisterMaterial(name, true);

	Str_SPrintf(name, sizeof(name), "players/%s/%s_i", model, skin);
	clientInfo->icon = R_RegisterMaterialNoMip(name);

	for (i = 0; i < cl.numWeaponModels; i++){
		Str_SPrintf(name, sizeof(name), "players/%s/%s.md2", model, cl.weaponModels[i]);
		clientInfo->weaponModel[i] = R_RegisterModel(name);

		if (!cl_visibleWeapons->integerValue)
			break;		// Only one if no visible weapons
	}

	// Save model/skin in the info string
	Str_SPrintf(clientInfo->info, sizeof(clientInfo->info), "%s/%s", model, skin);

	clientInfo->valid = true;
}

/*
 ==================
 CL_LoadGameMedia
 ==================
*/
void CL_LoadGameMedia (){

	char	checkTGA[MAX_QPATH];
	char	levelshot[MAX_QPATH];
	int		i, n;
	int		time;

	time = Sys_Milliseconds();

	// Need to precache files
	cls.state = CA_LOADING;

	// Clear all local effects (except for light styles) because they
	// now point to invalid files
	CL_ClearTempEntities();
	CL_ClearLocalEntities();
	CL_ClearDynamicLights();
	CL_ClearParticles();

	// Get map name
	Com_StripExtension(cl.configStrings[CS_MODELS + 1] + 5, cls.loadingInfo.map, sizeof(cls.loadingInfo.map));
	Str_Copy(cls.loadingInfo.name, cl.configStrings[CS_NAME], sizeof(cls.loadingInfo.name));

	// Check if a levelshot for this map exists
	Str_SPrintf(checkTGA, sizeof(checkTGA), "ui/assets/levelshots/%s.tga", cls.loadingInfo.map);
	if (!FS_FileExists(checkTGA))
		Str_SPrintf(levelshot, sizeof(levelshot), "ui/assets/levelshots/unknownmap");
	else
		Str_SPrintf(levelshot, sizeof(levelshot), "ui/assets/levelshots/%s", cls.loadingInfo.map);

	// Load a few needed shaders for the loading screen
	cl.media.levelshot = R_RegisterMaterialNoMip(levelshot);
	cl.media.levelshotDetail = R_RegisterMaterialNoMip("ui/assets/loading/loading_detail");
	cl.media.loadingLogo = R_RegisterMaterialNoMip("ui/assets/title_screen/q2e_logo");
	cl.media.loadingDetail[0] = R_RegisterMaterialNoMip("ui/assets/loading/load_main2");
	cl.media.loadingDetail[1] = R_RegisterMaterialNoMip("ui/assets/loading/load_main");

	for (i = 0, n = 5; i < 20; i++, n += 5)
		cl.media.loadingPercent[i] = R_RegisterMaterialNoMip(Str_VarArgs("ui/assets/loading/percent/load_%i", n));

	// Register all the files for this level
	CL_RegisterCollisionMap();
	CL_RegisterSounds();
	CL_RegisterGraphics();
	CL_RegisterClients();

	// Start the background track
	CL_PlayBackgroundTrack();

	// All precaches are now complete
	cls.state = CA_PRIMED;

	Com_Printf("CL_LoadGameMedia: %.2f seconds\n", (float)(Sys_Milliseconds() - time) / 1000.0f);

	CL_UpdateLoading("");

	// Touch all the memory used for this level
	Mem_TouchMemory();

	// Force menu and console off
	UI_SetActiveMenu(UI_CLOSEMENU);
	Con_Close();
}