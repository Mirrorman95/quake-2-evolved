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


#include "client.h"


/*
 ==============================================================================

 LOADING SCREEN

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

 LEVEL LOADING

 ==============================================================================
*/


/*
 ==================
 CL_RegisterLoadingInfo
 ==================
*/
static void CL_RegisterLoadingInfo (){

	char	levelshot[MAX_PATH_LENGTH];
	int		i, j;

	// Get the map name
	Str_Copy(cls.loadingInfo.map, cl.configStrings[CS_MODELS + 1] + 5, sizeof(cls.loadingInfo.map));
	Str_StripFileExtension(cls.loadingInfo.map);

	Str_Copy(cls.loadingInfo.name, cl.configStrings[CS_NAME], sizeof(cls.loadingInfo.name));

	// Check if a levelshot for this map exists
	Str_SPrintf(levelshot, sizeof(levelshot), "ui/assets/levelshots/%s.tga", cls.loadingInfo.map);

	if (FS_FileExists(levelshot))
		Str_SPrintf(levelshot, sizeof(levelshot), "ui/assets/levelshots/%s", cls.loadingInfo.map);
	else
		Str_SPrintf(levelshot, sizeof(levelshot), "ui/assets/levelshots/unknownmap");

	// Load a few needed materials for the loading screen
	cl.media.levelshot = R_RegisterMaterialNoMip(levelshot);
	cl.media.levelshotDetail = R_RegisterMaterialNoMip("ui/assets/loading/loading_detail");
	cl.media.loadingLogo = R_RegisterMaterialNoMip("ui/assets/title_screen/q2e_logo");
	cl.media.loadingDetail[0] = R_RegisterMaterialNoMip("ui/assets/loading/load_main2");
	cl.media.loadingDetail[1] = R_RegisterMaterialNoMip("ui/assets/loading/load_main");

	for (i = 0, j = 5; i < 20; i++, j += 5)
		cl.media.loadingPercent[i] = R_RegisterMaterialNoMip(Str_VarArgs("ui/assets/loading/percent/load_%i", j));
}

/*
 ==================
 CL_RegisterClipMap
 ==================
*/
static void CL_RegisterClipMap (){

	uint checkCount;

	CL_UpdateLoading("clip map");

	CM_LoadMap(cl.configStrings[CS_MODELS + 1], true, &checkCount);

	if (checkCount != Str_ToInteger(cl.configStrings[CS_MAPCHECKSUM]))
		Com_Error(ERR_DROP, "Local map version differs from server: %i != %s", checkCount, cl.configStrings[CS_MAPCHECKSUM]);
}

/*
 ==================
 CL_RegisterSoundShaders
 ==================
*/
static void CL_RegisterSoundShaders (){

	int		i;

	// Register sound shaders
	CL_UpdateLoading("sound shaders");

	cl.media.richotecSoundShaders[0] = S_RegisterSoundShader("world/ric1.wav");
	cl.media.richotecSoundShaders[1] = S_RegisterSoundShader("world/ric2.wav");
	cl.media.richotecSoundShaders[2] = S_RegisterSoundShader("world/ric3.wav");
	cl.media.sparkSoundShaders[0] = S_RegisterSoundShader("world/spark5.wav");
	cl.media.sparkSoundShaders[1] = S_RegisterSoundShader("world/spark6.wav");
	cl.media.sparkSoundShaders[2] = S_RegisterSoundShader("world/spark7.wav");
	cl.media.footStepSoundShaders[0] = S_RegisterSoundShader("player/footstep1");
	cl.media.footStepSoundShaders[1] = S_RegisterSoundShader("player/footstep2");
	cl.media.footStepSoundShaders[2] = S_RegisterSoundShader("player/footstep3");
	cl.media.footStepSoundShaders[3] = S_RegisterSoundShader("player/footstep4");
	cl.media.laserHitSoundShader = S_RegisterSoundShader("weapons/lashit.wav");
	cl.media.railgunSoundShader = S_RegisterSoundShader("weapons/railgun");
	cl.media.rocketExplosionSoundShader = S_RegisterSoundShader("weapons/rocklx1a.wav");
	cl.media.grenadeExplosionSoundShader = S_RegisterSoundShader("weapons/grenlx1a.wav");
	cl.media.waterExplosionSoundShader = S_RegisterSoundShader("weapons/xpld_wat.wav");
	cl.media.machinegunBrassSoundShader = S_RegisterSoundShader("weapons/brass_bullet.wav");
	cl.media.shotgunBrassSoundShader = S_RegisterSoundShader("weapons/brass_shell.wav");

	if (!Str_ICompare(cl.gameDir, "rogue")){
		cl.media.lightningSoundShader = S_RegisterSoundShader("weapons/tesla.wav");
		cl.media.disruptorExplosionSoundShader = S_RegisterSoundShader("weapons/disrupthit.wav");
	}

	S_RegisterSoundShader("player/land1.wav");
	S_RegisterSoundShader("player/fall2.wav");
	S_RegisterSoundShader("player/fall1.wav");

	// Register the sound shaders that the server references
	CL_UpdateLoading("game sound shaders");

	for (i = 1; i < MAX_SOUNDS; i++){
		if (!cl.configStrings[CS_SOUNDS + i][0])
			break;

		cl.media.gameSoundShaders[i] = S_RegisterSoundShader(cl.configStrings[CS_SOUNDS + i]);
	}
}

/*
 ==================
 CL_RegisterGraphics
 ==================
*/
static void CL_RegisterGraphics (){

	char	name[MAX_PATH_LENGTH];
	float	skyRotate;
	vec3_t	skyAxis;
	int		i;

	// Load the world map
	CL_UpdateLoading("world map");

	skyRotate = Str_ToFloat(cl.configStrings[CS_SKYROTATE]);
	sscanf(cl.configStrings[CS_SKYAXIS], "%f %f %f", &skyAxis[0], &skyAxis[1], &skyAxis[2]);

	R_LoadMap(cl.configStrings[CS_MODELS+1], cl.configStrings[CS_SKY], skyRotate, skyAxis);		// FIXME: change the first paramter to cls.loadingInfo.name ?

	// Load the post-process effects
	CL_UpdateLoading("post-process effects");

	Str_SPrintf(name, sizeof(name), "maps/%s.postProcess", cls.loadingInfo.map);
	R_LoadPostProcess(name);

	// Load the reverb effects
	CL_UpdateLoading("reverb effects");

	Str_SPrintf(name, sizeof(name), "maps/%s.reverb", cls.loadingInfo.map);
	S_LoadReverb(name);

	// Register models
	CL_UpdateLoading("models");

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
	CL_UpdateLoading("game models");

	Str_Copy(cl.weaponModels[0], "weapon", sizeof(cl.weaponModels[0]));
	cl.numWeaponModels = 1;

	for (i = 1; i < MAX_MODELS; i++){
		if (!cl.configStrings[CS_MODELS + i][0])
			break;

		if (cl.configStrings[CS_MODELS + i][0] == '#'){
			// Special player weapon model
			if (cl.numWeaponModels < MAX_CLIENTWEAPONMODELS){
				Str_Copy(cl.weaponModels[cl.numWeaponModels], cl.configStrings[CS_MODELS + i] + 1, sizeof(cl.weaponModels[cl.numWeaponModels]));
				Str_StripFileExtension(cl.weaponModels[cl.numWeaponModels]);

				cl.numWeaponModels++;
			}
		}
		else {
			cl.media.gameModels[i] = R_RegisterModel(cl.configStrings[CS_MODELS + i]);

			if (cl.configStrings[CS_MODELS + i][0] == '*')
				cl.media.gameCModels[i] = CM_LoadInlineModel(cl.configStrings[CS_MODELS + i]);
			else
				cl.media.gameCModels[i] = NULL;
		}
	}

	// Register materials
	CL_UpdateLoading("materials");

	cl.media.lagometerMaterial = R_RegisterMaterialNoMip("lagometer");
	cl.media.disconnectedMaterial = R_RegisterMaterialNoMip("disconnected");
	cl.media.backTileMaterial = R_RegisterMaterialNoMip("backTile");
	cl.media.pauseMaterial = R_RegisterMaterialNoMip("pause");
	cl.media.logoMaterial = R_RegisterMaterialNoMip("logo");

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

	cl.media.fireScreenMaterial = R_RegisterMaterialNoMip("gfx/screen/fireScreen");
	cl.media.waterBlurMaterial = R_RegisterMaterialNoMip("gfx/screen/waterBlur");
	cl.media.doubleVisionMaterial = R_RegisterMaterialNoMip("gfx/screen/doubleVision");
	cl.media.underWaterVisionMaterial = R_RegisterMaterialNoMip("gfx/screen/underWaterVision");
	cl.media.irGogglesMaterial = R_RegisterMaterialNoMip("gfx/screen/irGoggles");

	cl.media.rocketExplosionMaterial = R_RegisterMaterial("gfx/effects/explosions/rocketExplosion", false);
	cl.media.rocketExplosionWaterMaterial = R_RegisterMaterial("gfx/effects/explosions/rocketExplosionWater", false);
	cl.media.grenadeExplosionMaterial = R_RegisterMaterial("gfx/effects/explosions/grenadeExplosion", false);
	cl.media.grenadeExplosionWaterMaterial = R_RegisterMaterial("gfx/effects/explosions/grenadeExplosionWater", false);
	cl.media.bfgExplosionMaterial = R_RegisterMaterial("gfx/effects/explosions/bfgExplosion", false);
	cl.media.bfgBallMaterial = R_RegisterMaterial("gfx/sprites/bfg", false);
	cl.media.plasmaBallMaterial = R_RegisterMaterial("gfx/sprites/plasma", false);
	cl.media.waterPlumeMaterial = R_RegisterMaterial("gfx/effects/water/waterPlume", false);
	cl.media.waterSprayMaterial = R_RegisterMaterial("gfx/effects/water/waterSpray", false);
	cl.media.waterWakeMaterial = R_RegisterMaterial("gfx/effects/water/waterWake", false);
	cl.media.nukeShockwaveMaterial = R_RegisterMaterial("nukeShockwave", false);
	cl.media.bloodSplatMaterial[0] = R_RegisterMaterial("bloodSplat", false);
	cl.media.bloodSplatMaterial[1] = R_RegisterMaterial("greenBloodSplat", false);
	cl.media.bloodCloudMaterial[0] = R_RegisterMaterial("bloodCloud", false);
	cl.media.bloodCloudMaterial[1] = R_RegisterMaterial("greenBloodCloud", false);

	cl.media.powerScreenShellMaterial = R_RegisterMaterial("gfx/shells/powerScreen", false);
	cl.media.invulnerabilityShellMaterial = R_RegisterMaterial("gfx/shells/invulnerability", false);
	cl.media.quadDamageShellMaterial = R_RegisterMaterial("gfx/shells/quadDamage", false);
	cl.media.doubleDamageShellMaterial = R_RegisterMaterial("gfx/shells/doubleDamage", false);
	cl.media.halfDamageShellMaterial = R_RegisterMaterial("gfx/shells/halfDamage", false);
	cl.media.genericShellMaterial = R_RegisterMaterial("gfx/shells/generic", false);

	cl.media.laserBeamMaterial = R_RegisterMaterial("gfx/beams/laser", false);
	cl.media.laserBeamBFGMaterial = R_RegisterMaterial("gfx/beams/laserBFG", false);
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
	CL_UpdateLoading("game materials");

	for (i = 1; i < MAX_IMAGES; i++){
		if (!cl.configStrings[CS_IMAGES + i][0])
			break;

		if (!Str_FindChar(cl.configStrings[CS_IMAGES + i], '/'))
			Str_SPrintf(name, sizeof(name), "pics/%s", cl.configStrings[CS_IMAGES + i]);
		else {
			Str_Copy(name, cl.configStrings[CS_IMAGES + i], sizeof(name));
			Str_StripFileExtension(name);
		}

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

	// Register all the clients that are present on the server
	CL_UpdateLoading("clients");

	CL_LoadClientInfo(&cl.baseClientInfo, "unnamed\\male/grunt");

	for (i = 0; i < MAX_CLIENTS; i++){
		if (!cl.configStrings[CS_PLAYERSKINS + i][0])
			continue;

		CL_LoadClientInfo(&cl.clientInfo[i], cl.configStrings[CS_PLAYERSKINS + i]);
	}
}

/*
 ==================
 CL_LoadAssets
 ==================
*/
static void CL_LoadAssets (){

	int		time;

	time = Sys_Milliseconds();

	Com_Printf("------------ Level Loading ------------\n");
	Com_Printf("Loading %s\n", cls.loadingInfo.name);

	CL_RegisterLoadingInfo();
	CL_RegisterClipMap();
	CL_RegisterSoundShaders();
	CL_RegisterGraphics();
	CL_RegisterClients();

	Com_Printf("---------------------------------------\n");
	Com_Printf("Level loaded in %.2f seconds\n", MS2SEC(Sys_Milliseconds() - time));
}

/*
 ==================

 TODO: get the static light data from the parsed lights, then copy the
 data into renderLight_t which sends it to the renderer for pre-cacheing
 ==================
*/
static void CL_PrecacheLights (){

	char		name[MAX_PATH_LENGTH];

	// Load and parse the lights
	Str_SPrintf(name, sizeof(name), "maps/%s.light", cls.loadingInfo.map);

	R_LoadLights(name);
}

/*
 ==================
 CL_Load
 ==================
*/
static void CL_Load (){

	// Load the assets
	CL_LoadAssets();

	// Precache all the lights
	CL_PrecacheLights();
}

/*
 ==================
 CL_LoadLevel
 ==================
*/
void CL_LoadLevel (){

	// Need to precache files
	cls.state = CA_LOADING;

	// Free any loaded data
	CL_FreeLevel();

	// Load the level
	CL_Load();

	// All precaches are now complete
	cls.state = CA_PRIMED;

	// Free all temporary allocations
	Mem_FreeAll(TAG_TEMPORARY, true);

	// Make sure everything is paged in
//	Mem_TouchMemory();

	// Force menu and console off
	UI_SetActiveMenu(UI_CLOSEMENU);
	Con_Close();
}

/*
 ==================
 CL_FreeLevel
 ==================
*/
void CL_FreeLevel (){

	// Clear local effects because they now point to invalid files
	CL_ClearTempEntities();
	CL_ClearLocalEntities();
	CL_ClearDynamicLights();
	CL_ClearParticles();

	// Clear the testing utilities
	CL_ClearTestTools();
}


// ============================================================================


/*
 ==================
 CL_LoadingState
 ==================
*/
void CL_LoadingState (){

	if (cls.loading)
		return;

	cls.loading = true;

	// Clear loading information
	Mem_Fill(&cls.loadingInfo, 0, sizeof(loadingInfo_t));

	// If playing a cinematic, stop it
	CL_StopCinematic();

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

	char	*ch;
	char	model[MAX_PATH_LENGTH], skin[MAX_PATH_LENGTH], name[MAX_PATH_LENGTH];
	char	checkMD3[MAX_PATH_LENGTH], checkMD2[MAX_PATH_LENGTH];
	char	checkTGA[MAX_PATH_LENGTH], checkPCX[MAX_PATH_LENGTH];
	int		i;

	Mem_Fill(clientInfo, 0, sizeof(clientInfo_t));

	// Isolate the player name
	Str_Copy(clientInfo->name, string, sizeof(clientInfo->name));

	ch = Str_FindChar(string, '\\');
	if (ch){
		clientInfo->name[ch-string] = 0;
		string = ch+1;
	}

	// If no custom skins or bad info string, so just use male/grunt
	if (cl_noSkins->integerValue || *string == 0){
		clientInfo->model = R_RegisterModel("players/male/tris.md2");
		clientInfo->skin = R_RegisterMaterial("players/male/grunt", true);
		clientInfo->icon = R_RegisterMaterialNoMip("players/male/grunt_i");
		clientInfo->weaponModel[0] = R_RegisterModel("players/male/weapon.md2");

		// Save the model/skin in the info string
		Str_SPrintf(clientInfo->info, sizeof(clientInfo->info), "male/grunt");

		clientInfo->valid = true;

		return;
	}

	// Isolate the model and skin name
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
	}

	// Save model/skin in the info string
	Str_SPrintf(clientInfo->info, sizeof(clientInfo->info), "%s/%s", model, skin);

	clientInfo->valid = true;
}