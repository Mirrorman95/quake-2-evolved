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
// cl_effects.c - Client side effects parsing and management
//


#include "client.h"


/*
 ==============================================================================

 LIGHT STYLE MANAGEMENT

 ==============================================================================
*/

typedef struct {
	int					length;
	float				map[MAX_QPATH];
	vec3_t				rgb;
} cLightStyle_t;

static cLightStyle_t	cl_lightStyles[MAX_LIGHTSTYLES];
static int				cl_lastOfs;


/*
 ==================
 CL_ClearLightStyles
 ==================
*/
void CL_ClearLightStyles (){

	Mem_Fill(cl_lightStyles, 0, sizeof(cLightStyle_t));
	cl_lastOfs = -1;
}

/*
 ==================
 CL_RunLightStyles
 ==================
*/
void CL_RunLightStyles (){

	cLightStyle_t	*ls;
	int				i, offset;

	offset = cl.time / 100;
	if (offset == cl_lastOfs)
		return;
	cl_lastOfs = offset;

	for (i = 0, ls = cl_lightStyles; i < MAX_LIGHTSTYLES; i++, ls++){
		if (!ls->length){
			ls->rgb[0] = ls->rgb[1] = ls->rgb[2] = 1.0f;
			continue;
		}

		if (ls->length == 1)
			ls->rgb[0] = ls->rgb[1] = ls->rgb[2] = ls->map[0];
		else
			ls->rgb[0] = ls->rgb[1] = ls->rgb[2] = ls->map[offset % ls->length];
	}
}

/*
 ==================
 CL_AddLightStyles
 ==================
*/
void CL_AddLightStyles (){

	cLightStyle_t	*ls;
	int				i;

	for (i = 0, ls = cl_lightStyles; i < MAX_LIGHTSTYLES; i++, ls++)
		R_SetLightStyle(i, ls->rgb[0], ls->rgb[1], ls->rgb[2]);
}

/*
 ==================
 CL_SetLightStyle
 ==================
*/
void CL_SetLightStyle (int style){

	char	*string;
	int		i, length;

	if (style < 0 || style >= MAX_LIGHTSTYLES)
		Com_Error(ERR_DROP, "CL_SetLightStyle: style = %i", style);

	string = cl.configStrings[CS_LIGHTS + style];

	length = Str_Length(string);
	if (length >= MAX_QPATH)
		Com_Error(ERR_DROP, "CL_SetLightStyle: style length = %i", length);

	cl_lightStyles[style].length = length;

	for (i = 0; i < length; i++)
		cl_lightStyles[style].map[i] = (float)(string[i] - 'a') / (float)('m' - 'a');
}


/*
 ==============================================================================

 DYNAMIC LIGHT MANAGEMENT

 ==============================================================================
*/

#define MAX_DLIGHTS					32

typedef struct {
	bool					active;
	int						start;
	int						end;
	vec3_t					origin;
	float					intensity;
	vec3_t					color;
	bool					fade;
} dlight_t;

static dlight_t				cl_dynamicLights[MAX_DLIGHTS];


/*
 ==================
 CL_AllocDynamicLight
 ==================
*/
static dlight_t *CL_AllocDynamicLight (){

	int		i;
	int		time, index;

	for (i = 0; i < MAX_DLIGHTS; i++){
		if (!cl_dynamicLights[i].active){
			Mem_Fill(&cl_dynamicLights[i], 0, sizeof(cl_dynamicLights[i]));
			return &cl_dynamicLights[i];
		}
	}

	// Find the oldest light
	time = cl.time;
	index = 0;

	for (i = 0; i < MAX_DLIGHTS; i++){
		if (cl_dynamicLights[i].start < time){
			time = cl_dynamicLights[i].start;
			index = i;
		}
	}

	Mem_Fill(&cl_dynamicLights[index], 0, sizeof(cl_dynamicLights[index]));
	return &cl_dynamicLights[index];
}

/*
 ==================
 CL_ClearDynamicLights
 ==================
*/
void CL_ClearDynamicLights (){

	Mem_Fill(cl_dynamicLights, 0, sizeof(cl_dynamicLights));
}

/*
 ==================
 CL_AddDynamicLights
 ==================
*/
void CL_AddDynamicLights (){

	dlight_t		*dl;
	renderLight_t	light;
	float			intensity;
	int				i;

	for (i = 0, dl = cl_dynamicLights; i < MAX_DLIGHTS; i++, dl++){
		if (!dl->active)
			continue;

		if (cl.time >= dl->end){
			dl->active = false;
			continue;
		}

		if (!dl->fade)
			intensity = dl->intensity;
		else {
			intensity = (float)(cl.time - dl->start) / (dl->end - dl->start);
			intensity = dl->intensity * (1.0f - intensity);
		}

		// Create a new light
		VectorCopy(dl->origin, light.origin);

		light.material = NULL;

		light.materialParms[MATERIALPARM_RED] = dl->color[0];
		light.materialParms[MATERIALPARM_GREEN] = dl->color[1];
		light.materialParms[MATERIALPARM_BLUE] = dl->color[2];
		light.materialParms[MATERIALPARM_ALPHA] = 1.0f;
		light.materialParms[MATERIALPARM_TIMEOFFSET] = MS2SEC(dl->start);
		light.materialParms[MATERIALPARM_DIVERSITY] = 0.0f;
		light.materialParms[MATERIALPARM_MISC] = 0.0f;
		light.materialParms[MATERIALPARM_MODE] = 1.0f;

		// Send it to the renderer
		R_AddLightToScene(&light);
	}
}

/*
 ==================
 CL_DynamicLight
 ==================
*/
void CL_DynamicLight (const vec3_t origin, float intensity, float r, float g, float b, bool fade, int duration){

	dlight_t		*dl;
	renderLight_t	light;

	// A duration of 0 means a temporary light, so we should just send
	// it to the renderer and forget about it
	if (duration == 0){
		VectorCopy(origin, light.origin);

		light.material = NULL;

		light.materialParms[MATERIALPARM_RED] = r;
		light.materialParms[MATERIALPARM_GREEN] = g;
		light.materialParms[MATERIALPARM_BLUE] = b;
		light.materialParms[MATERIALPARM_ALPHA] = 1.0f;
		light.materialParms[MATERIALPARM_TIMEOFFSET] = 0.0f;
		light.materialParms[MATERIALPARM_DIVERSITY] = 0.0f;
		light.materialParms[MATERIALPARM_MISC] = 0.0f;
		light.materialParms[MATERIALPARM_MODE] = 1.0f;

		// Send it to the renderer
		R_AddLightToScene(&light);

		return;
	}

	// Add a new dynamic light
	dl = CL_AllocDynamicLight();
	dl->active = true;

	dl->start = cl.time;
	dl->end = dl->start + duration;
	VectorCopy(origin, dl->origin);
	dl->intensity = intensity;
	dl->color[0] = r;
	dl->color[1] = g;
	dl->color[2] = b;
	dl->fade = fade;
}


/*
 ==============================================================================

 MUZZLE FLASH PARSING

 ==============================================================================
*/


/*
 ==================
 CL_ParsePlayerMuzzleFlash

 TODO: muzzle flash sprite effect similar to Doom 3
 ==================
*/
void CL_ParsePlayerMuzzleFlash (){

	int			entity;
	centity_t	*centity;
	vec3_t		forward, right;
	int			weapon;
	float		volume = 1.0f;
	char		name[MAX_QPATH];
	vec3_t		origin;

	entity = MSG_ReadShort(&net_message);
	if (entity < 1 || entity >= MAX_EDICTS)
		Com_Error(ERR_DROP, "CL_ParsePlayerMuzzleFlash: entity = %i", entity);

	weapon = MSG_ReadByte(&net_message);
	
	if (weapon & MZ_SILENCED){
		weapon &= ~MZ_SILENCED;
		volume = 0.2f;
	}

	centity = &cl.entities[entity];

	AngleToVectors(centity->current.angles, forward, right, NULL);

	origin[0] = centity->current.origin[0] + forward[0] * 18.0f + right[0] * 16.0f;
	origin[1] = centity->current.origin[1] + forward[1] * 18.0f + right[1] * 16.0f;
	origin[2] = centity->current.origin[2] + forward[2] * 18.0f + right[2] * 16.0f;

	switch (weapon){
	case MZ_BLASTER:
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 1.0f, 1.0f, 0.0f, false, 1);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound("weapons/blastf1a.wav", 0), volume, ATTN_NORM, 0.0f);

		break;
	case MZ_BLUEHYPERBLASTER:
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 0.0f, 0.0f, 1.0f, false, 1);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound("weapons/hyprbf1a.wav", 0), volume, ATTN_NORM, 0.0f);

		break;
	case MZ_HYPERBLASTER:
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 1.0f, 1.0f, 0.0f, false, 1);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound("weapons/hyprbf1a.wav", 0), volume, ATTN_NORM, 0.0f);

		break;
	case MZ_MACHINEGUN:
		CL_MachinegunEjectBrass(centity, 1, 15.0f, -8.0f, 18.0f);
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 1.0f, 1.0f, 0.0f, false, 1);
		Str_SPrintf(name, sizeof(name), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound(name, 0), volume, ATTN_NORM, 0.0f);

		break;
	case MZ_SHOTGUN:
		CL_ShotgunEjectBrass(centity, 1, 12.0f, -6.0f, 16.0f);
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 1.0f, 1.0f, 0.0f, false, 1);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound("weapons/shotgf1b.wav", 0), volume, ATTN_NORM, 0.0f);
		S_PlaySound(NULL, entity, CHAN_AUTO, S_RegisterSound("weapons/shotgr1b.wav", 0), volume, ATTN_NORM, 100.0f);

		break;
	case MZ_SSHOTGUN:
		CL_ShotgunEjectBrass(centity, 2, 8.0f, -8.0f, 16.0f);
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 1.0f, 1.0f, 0.0f, false, 1);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound("weapons/sshotf1b.wav", 0), volume, ATTN_NORM, 0.0f);

		break;
	case MZ_CHAINGUN1:
		CL_MachinegunEjectBrass(centity, 1, 10.0f, -8.0f, 18.0f);
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 1.0f, 0.25f, 0.0f, false, 1);
		Str_SPrintf(name, sizeof(name), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound(name, 0), volume, ATTN_NORM, 0.0f);

		break;
	case MZ_CHAINGUN2:
		CL_MachinegunEjectBrass(centity, 2, 10.0f, -8.0f, 18.0f);
		CL_DynamicLight(origin, 255.0f + (rand() & 31), 1.0f, 0.5f, 0.0f, false, 1);
		Str_SPrintf(name, sizeof(name), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound(name, 0), volume, ATTN_NORM, 0.0f);
		Str_SPrintf(name, sizeof(name), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound(name, 0), volume, ATTN_NORM, 50.0f);

		break;
	case MZ_CHAINGUN3:
		CL_MachinegunEjectBrass(centity, 3, 10.0f, -8.0f, 18.0f);
		CL_DynamicLight(origin, 250.0f + (rand() & 31), 1.0f, 1.0f, 0.0f, false, 1);
		Str_SPrintf(name, sizeof(name), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound(name, 0), volume, ATTN_NORM, 0.0f);
		Str_SPrintf(name, sizeof(name), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound(name, 0), volume, ATTN_NORM, 33.0f);
		Str_SPrintf(name, sizeof(name), "weapons/machgf%ib.wav", (rand() % 5) + 1);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound(name, 0), volume, ATTN_NORM, 66.0f);

		break;
	case MZ_RAILGUN:
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 0.5f, 0.5f, 1.0f, false, 1);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound("weapons/railgf1a.wav", 0), volume, ATTN_NORM, 0.0f);

		break;
	case MZ_ROCKET:
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 1.0f, 0.5f, 0.2f, false, 1);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound("weapons/rocklf1a.wav", 0), volume, ATTN_NORM, 0.0f);
		S_PlaySound(NULL, entity, CHAN_AUTO, S_RegisterSound("weapons/rocklr1b.wav", 0), volume, ATTN_NORM, 100.0f);

		break;
	case MZ_GRENADE:
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 1.0f, 0.5f, 0.0f, false, 1);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound("weapons/grenlf1a.wav", 0), volume, ATTN_NORM, 0.0f);
		S_PlaySound(NULL, entity, CHAN_AUTO, S_RegisterSound("weapons/grenlr1b.wav", 0), volume, ATTN_NORM, 100.0f);

		break;
	case MZ_BFG:
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 0.0f, 1.0f, 0.0f, false, 1);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound("weapons/bfg__f1y.wav", 0), volume, ATTN_NORM, 0.0f);

		break;
	case MZ_LOGIN:
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 0.0f, 1.0f, 0.0f, false, 1);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound("weapons/grenlf1a.wav", 0), 1.0f, ATTN_NORM, 0.0f);
		CL_LogParticles(centity->current.origin, 0.0f, 1.0f, 0.0f);

		break;
	case MZ_LOGOUT:
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 1.0f, 0.0f, 0.0f, false, 1);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound("weapons/grenlf1a.wav", 0), 1.0f, ATTN_NORM, 0.0f);
		CL_LogParticles(centity->current.origin, 1.0f, 0.0f, 0.0f);

		break;
	case MZ_PHALANX:
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 1.0f, 0.5f, 0.5f, false, 1);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound("weapons/plasshot.wav", 0), volume, ATTN_NORM, 0.0f);

		break;
	case MZ_IONRIPPER:
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 1.0f, 0.5f, 0.5f, false, 1);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound("weapons/rippfire.wav", 0), volume, ATTN_NORM, 0.0f);

		break;
	case MZ_ETF_RIFLE:
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 0.9f, 0.7f, 0.0f, false, 1);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound("weapons/nail1.wav", 0), volume, ATTN_NORM, 0.0f);

		break;
	case MZ_HEATBEAM:
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 1.0f, 1.0f, 0.0f, false, 100);

		break;
	case MZ_BLASTER2:
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 0.0f, 1.0f, 0.0f, false, 1);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound("weapons/blastf1a.wav", 0), volume, ATTN_NORM, 0.0f);

		break;
	case MZ_TRACKER:
		CL_DynamicLight(origin, 200.0f + (rand() & 31), -1.0f, -1.0f, -1.0f, false, 1);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound("weapons/disint2.wav", 0), volume, ATTN_NORM, 0.0f);

		break;		
	case MZ_NUKE1:
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 1.0f, 0.0f, 0.0f, false, 100);

		break;
	case MZ_NUKE2:
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 1.0f, 1.0f, 0.0f, false, 100);

		break;
	case MZ_NUKE4:
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 0.0f, 0.0f, 1.0f, false, 100);

		break;
	case MZ_NUKE8:
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 0.0f, 1.0f, 1.0f, false, 100);

		break;
	}
}

/*
 ==================
 CL_ParseMonsterMuzzleFlash

 TODO: muzzle flash sprite effect similar to Doom 3
 ==================
*/
void CL_ParseMonsterMuzzleFlash (){

	int			entity;
	centity_t	*centity;
	vec3_t		forward, right;
	int			flash;
	char		name[MAX_QPATH];
	vec3_t		origin;

	entity = MSG_ReadShort(&net_message);
	if (entity < 1 || entity >= MAX_EDICTS)
		Com_Error(ERR_DROP, "CL_ParseMonsterMuzzleFlash: entity = %i", entity);

	flash = MSG_ReadByte(&net_message);

	centity = &cl.entities[entity];

	AngleToVectors(centity->current.angles, forward, right, NULL);
	
	origin[0] = centity->current.origin[0] + forward[0] * monster_flash_offset[flash][0] + right[0] * monster_flash_offset[flash][1];
	origin[1] = centity->current.origin[1] + forward[1] * monster_flash_offset[flash][0] + right[1] * monster_flash_offset[flash][1];
	origin[2] = centity->current.origin[2] + forward[2] * monster_flash_offset[flash][0] + right[2] * monster_flash_offset[flash][1] + monster_flash_offset[flash][2];
	
	switch (flash){
	case MZ2_INFANTRY_MACHINEGUN_1:
	case MZ2_INFANTRY_MACHINEGUN_2:
	case MZ2_INFANTRY_MACHINEGUN_3:
	case MZ2_INFANTRY_MACHINEGUN_4:
	case MZ2_INFANTRY_MACHINEGUN_5:
	case MZ2_INFANTRY_MACHINEGUN_6:
	case MZ2_INFANTRY_MACHINEGUN_7:
	case MZ2_INFANTRY_MACHINEGUN_8:
	case MZ2_INFANTRY_MACHINEGUN_9:
	case MZ2_INFANTRY_MACHINEGUN_10:
	case MZ2_INFANTRY_MACHINEGUN_11:
	case MZ2_INFANTRY_MACHINEGUN_12:
	case MZ2_INFANTRY_MACHINEGUN_13:
		CL_MachinegunEjectBrass(centity, 1, monster_flash_offset[flash][0], monster_flash_offset[flash][1], monster_flash_offset[flash][2]);
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 1.0f, 1.0f, 0.0f, false, 1);
		CL_SmokePuffParticles(origin, 3, 1);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound("infantry/infatck1.wav", 0), 1.0f, ATTN_NORM, 0.0f);

		break;
	case MZ2_SOLDIER_MACHINEGUN_1:
	case MZ2_SOLDIER_MACHINEGUN_2:
	case MZ2_SOLDIER_MACHINEGUN_3:
	case MZ2_SOLDIER_MACHINEGUN_4:
	case MZ2_SOLDIER_MACHINEGUN_5:
	case MZ2_SOLDIER_MACHINEGUN_6:
	case MZ2_SOLDIER_MACHINEGUN_7:
	case MZ2_SOLDIER_MACHINEGUN_8:
		CL_MachinegunEjectBrass(centity, 1, monster_flash_offset[flash][0], monster_flash_offset[flash][1], monster_flash_offset[flash][2]);
		CL_DynamicLight(origin, 200 + (rand() & 31), 1.0f, 1.0f, 0.0f, false, 1);
		CL_SmokePuffParticles(origin, 3.0f, 1);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound("soldier/solatck3.wav", 0), 1.0f, ATTN_NORM, 0.0f);

		break;
	case MZ2_GUNNER_MACHINEGUN_1:
	case MZ2_GUNNER_MACHINEGUN_2:
	case MZ2_GUNNER_MACHINEGUN_3:
	case MZ2_GUNNER_MACHINEGUN_4:
	case MZ2_GUNNER_MACHINEGUN_5:
	case MZ2_GUNNER_MACHINEGUN_6:
	case MZ2_GUNNER_MACHINEGUN_7:
	case MZ2_GUNNER_MACHINEGUN_8:
		CL_MachinegunEjectBrass(centity, 1, monster_flash_offset[flash][0], monster_flash_offset[flash][1], monster_flash_offset[flash][2]);
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 1.0f, 1.0f, 0.0f, false, 1);
		CL_SmokePuffParticles(origin, 3.0f, 1);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound("gunner/gunatck2.wav", 0), 1.0f, ATTN_NORM, 0.0f);

		break;
	case MZ2_ACTOR_MACHINEGUN_1:
	case MZ2_SUPERTANK_MACHINEGUN_1:
	case MZ2_SUPERTANK_MACHINEGUN_2:
	case MZ2_SUPERTANK_MACHINEGUN_3:
	case MZ2_SUPERTANK_MACHINEGUN_4:
	case MZ2_SUPERTANK_MACHINEGUN_5:
	case MZ2_SUPERTANK_MACHINEGUN_6:
	case MZ2_TURRET_MACHINEGUN:
		CL_MachinegunEjectBrass(centity, 1, monster_flash_offset[flash][0], monster_flash_offset[flash][1], monster_flash_offset[flash][2]);
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 1.0f, 1.0f, 0.0f, false, 1);
		CL_SmokePuffParticles(origin, 3.0f, 1);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound("infantry/infatck1.wav", 0), 1.0f, ATTN_NORM, 0.0f);

		break;
	case MZ2_BOSS2_MACHINEGUN_L1:
	case MZ2_BOSS2_MACHINEGUN_L2:
	case MZ2_BOSS2_MACHINEGUN_L3:
	case MZ2_BOSS2_MACHINEGUN_L4:
	case MZ2_BOSS2_MACHINEGUN_L5:
	case MZ2_CARRIER_MACHINEGUN_L1:
	case MZ2_CARRIER_MACHINEGUN_L2:
		CL_MachinegunEjectBrass(centity, 1, monster_flash_offset[flash][0], monster_flash_offset[flash][1], monster_flash_offset[flash][2]);
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 1.0f, 1.0f, 0.0f, false, 1);
		CL_SmokePuffParticles(origin, 3.0f, 1);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound("infantry/infatck1.wav", 0), 1.0f, ATTN_NONE, 0.0f);

		break;
	case MZ2_SOLDIER_BLASTER_1:
	case MZ2_SOLDIER_BLASTER_2:
	case MZ2_SOLDIER_BLASTER_3:
	case MZ2_SOLDIER_BLASTER_4:
	case MZ2_SOLDIER_BLASTER_5:
	case MZ2_SOLDIER_BLASTER_6:
	case MZ2_SOLDIER_BLASTER_7:
	case MZ2_SOLDIER_BLASTER_8:
	case MZ2_TURRET_BLASTER:
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 1.0f, 1.0f, 0.0f, false, 1);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound("soldier/solatck2.wav", 0), 1.0f, ATTN_NORM, 0.0f);

		break;
	case MZ2_FLYER_BLASTER_1:
	case MZ2_FLYER_BLASTER_2:
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 1.0f, 1.0f, 0.0f, false, 1);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound("flyer/flyatck3.wav", 0), 1.0f, ATTN_NORM, 0.0f);

		break;
	case MZ2_MEDIC_BLASTER_1:
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 1.0f, 1.0f, 0.0f, false, 1);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound("medic/medatck1.wav", 0), 1.0f, ATTN_NORM, 0.0f);

		break;
	case MZ2_HOVER_BLASTER_1:
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 1.0f, 1.0f, 0.0f, false, 1);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound("hover/hovatck1.wav", 0), 1.0f, ATTN_NORM, 0.0f);

		break;
	case MZ2_FLOAT_BLASTER_1:
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 1.0f, 1.0f, 0.0f, false, 1);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound("floater/fltatck1.wav", 0), 1.0f, ATTN_NORM, 0.0f);

		break;
	case MZ2_SOLDIER_SHOTGUN_1:
	case MZ2_SOLDIER_SHOTGUN_2:
	case MZ2_SOLDIER_SHOTGUN_3:
	case MZ2_SOLDIER_SHOTGUN_4:
	case MZ2_SOLDIER_SHOTGUN_5:
	case MZ2_SOLDIER_SHOTGUN_6:
	case MZ2_SOLDIER_SHOTGUN_7:
	case MZ2_SOLDIER_SHOTGUN_8:
		CL_ShotgunEjectBrass(centity, 1, monster_flash_offset[flash][0], monster_flash_offset[flash][1], monster_flash_offset[flash][2]);
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 1.0f, 1.0f, 0.0f, false, 1);
		CL_SmokePuffParticles(origin, 3.0f, 1);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound("soldier/solatck1.wav", 0), 1.0f, ATTN_NORM, 0.0f);

		break;
	case MZ2_TANK_BLASTER_1:
	case MZ2_TANK_BLASTER_2:
	case MZ2_TANK_BLASTER_3:
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 1.0f, 1.0f, 0.0f, false, 1);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound("tank/tnkatck3.wav", 0), 1.0f, ATTN_NORM, 0.0f);

		break;
	case MZ2_TANK_MACHINEGUN_1:
	case MZ2_TANK_MACHINEGUN_2:
	case MZ2_TANK_MACHINEGUN_3:
	case MZ2_TANK_MACHINEGUN_4:
	case MZ2_TANK_MACHINEGUN_5:
	case MZ2_TANK_MACHINEGUN_6:
	case MZ2_TANK_MACHINEGUN_7:
	case MZ2_TANK_MACHINEGUN_8:
	case MZ2_TANK_MACHINEGUN_9:
	case MZ2_TANK_MACHINEGUN_10:
	case MZ2_TANK_MACHINEGUN_11:
	case MZ2_TANK_MACHINEGUN_12:
	case MZ2_TANK_MACHINEGUN_13:
	case MZ2_TANK_MACHINEGUN_14:
	case MZ2_TANK_MACHINEGUN_15:
	case MZ2_TANK_MACHINEGUN_16:
	case MZ2_TANK_MACHINEGUN_17:
	case MZ2_TANK_MACHINEGUN_18:
	case MZ2_TANK_MACHINEGUN_19:
		CL_MachinegunEjectBrass(centity, 1, monster_flash_offset[flash][0], monster_flash_offset[flash][1], monster_flash_offset[flash][2]);
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 1.0f, 1.0f, 0.0f, false, 1);
		CL_SmokePuffParticles(origin, 3.0f, 1);
		Str_SPrintf(name, sizeof(name), "tank/tnkatk2%c.wav", 'a' + rand() % 5);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound(name, 0), 1.0f, ATTN_NORM, 0.0f);

		break;
	case MZ2_CHICK_ROCKET_1:
	case MZ2_TURRET_ROCKET:
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 1.0f, 0.5f, 0.2f, false, 1);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound("chick/chkatck2.wav", 0), 1.0f, ATTN_NORM, 0.0f);

		break;
	case MZ2_TANK_ROCKET_1:
	case MZ2_TANK_ROCKET_2:
	case MZ2_TANK_ROCKET_3:
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 1.0f, 0.5f, 0.2f, false, 1);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound("tank/tnkatck1.wav", 0), 1.0f, ATTN_NORM, 0.0f);

		break;
	case MZ2_SUPERTANK_ROCKET_1:
	case MZ2_SUPERTANK_ROCKET_2:
	case MZ2_SUPERTANK_ROCKET_3:
	case MZ2_BOSS2_ROCKET_1:
	case MZ2_BOSS2_ROCKET_2:
	case MZ2_BOSS2_ROCKET_3:
	case MZ2_BOSS2_ROCKET_4:
	case MZ2_CARRIER_ROCKET_1:
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 1.0f, 0.5f, 0.2f, false, 1);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound("tank/rocket.wav", 0), 1.0f, ATTN_NORM, 0.0f);

		break;
	case MZ2_GUNNER_GRENADE_1:
	case MZ2_GUNNER_GRENADE_2:
	case MZ2_GUNNER_GRENADE_3:
	case MZ2_GUNNER_GRENADE_4:
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 1.0f, 0.5f, 0.0f, false, 1);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound("gunner/gunatck3.wav", 0), 1.0f, ATTN_NORM, 0.0f);

		break;
	case MZ2_GLADIATOR_RAILGUN_1:
	case MZ2_CARRIER_RAILGUN:
	case MZ2_WIDOW_RAIL:
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 0.5f, 0.5f, 1.0f, false, 1);

		break;
	case MZ2_MAKRON_BFG:
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 0.5f, 1.0f, 0.5f, false, 1);

		break;
	case MZ2_MAKRON_BLASTER_1:
	case MZ2_MAKRON_BLASTER_2:
	case MZ2_MAKRON_BLASTER_3:
	case MZ2_MAKRON_BLASTER_4:
	case MZ2_MAKRON_BLASTER_5:
	case MZ2_MAKRON_BLASTER_6:
	case MZ2_MAKRON_BLASTER_7:
	case MZ2_MAKRON_BLASTER_8:
	case MZ2_MAKRON_BLASTER_9:
	case MZ2_MAKRON_BLASTER_10:
	case MZ2_MAKRON_BLASTER_11:
	case MZ2_MAKRON_BLASTER_12:
	case MZ2_MAKRON_BLASTER_13:
	case MZ2_MAKRON_BLASTER_14:
	case MZ2_MAKRON_BLASTER_15:
	case MZ2_MAKRON_BLASTER_16:
	case MZ2_MAKRON_BLASTER_17:
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 1.0f, 1.0f, 0.0f, false, 1);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound("makron/blaster.wav", 0), 1.0f, ATTN_NORM, 0.0f);

		break;
	case MZ2_JORG_MACHINEGUN_L1:
	case MZ2_JORG_MACHINEGUN_L2:
	case MZ2_JORG_MACHINEGUN_L3:
	case MZ2_JORG_MACHINEGUN_L4:
	case MZ2_JORG_MACHINEGUN_L5:
	case MZ2_JORG_MACHINEGUN_L6:
		CL_MachinegunEjectBrass(centity, 1, monster_flash_offset[flash][0], monster_flash_offset[flash][1], monster_flash_offset[flash][2]);
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 1.0f, 1.0f, 0.0f, false, 1);
		CL_SmokePuffParticles(origin, 3.0f, 1);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound("boss3/xfire.wav", 0), 1.0f, ATTN_NORM, 0.0f);

		break;
	case MZ2_JORG_MACHINEGUN_R1:
	case MZ2_JORG_MACHINEGUN_R2:
	case MZ2_JORG_MACHINEGUN_R3:
	case MZ2_JORG_MACHINEGUN_R4:
	case MZ2_JORG_MACHINEGUN_R5:
	case MZ2_JORG_MACHINEGUN_R6:
		CL_MachinegunEjectBrass(centity, 1, monster_flash_offset[flash][0], monster_flash_offset[flash][1], monster_flash_offset[flash][2]);
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 1.0f, 1.0f, 0.0f, false, 1);
		CL_SmokePuffParticles(origin, 3.0f, 1);

		break;
	case MZ2_JORG_BFG_1:
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 0.5f, 1.0f, 0.5f, false, 1);

		break;
	case MZ2_BOSS2_MACHINEGUN_R1:
	case MZ2_BOSS2_MACHINEGUN_R2:
	case MZ2_BOSS2_MACHINEGUN_R3:
	case MZ2_BOSS2_MACHINEGUN_R4:
	case MZ2_BOSS2_MACHINEGUN_R5:
	case MZ2_CARRIER_MACHINEGUN_R1:
	case MZ2_CARRIER_MACHINEGUN_R2:
		CL_MachinegunEjectBrass(centity, 1, monster_flash_offset[flash][0], monster_flash_offset[flash][1], monster_flash_offset[flash][2]);
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 1.0f, 1.0f, 0.0f, false, 1);
		CL_SmokePuffParticles(origin, 3.0f, 1);

		break;
	case MZ2_STALKER_BLASTER:
	case MZ2_DAEDALUS_BLASTER:
	case MZ2_MEDIC_BLASTER_2:
	case MZ2_WIDOW_BLASTER:
	case MZ2_WIDOW_BLASTER_SWEEP1:
	case MZ2_WIDOW_BLASTER_SWEEP2:
	case MZ2_WIDOW_BLASTER_SWEEP3:
	case MZ2_WIDOW_BLASTER_SWEEP4:
	case MZ2_WIDOW_BLASTER_SWEEP5:
	case MZ2_WIDOW_BLASTER_SWEEP6:
	case MZ2_WIDOW_BLASTER_SWEEP7:
	case MZ2_WIDOW_BLASTER_SWEEP8:
	case MZ2_WIDOW_BLASTER_SWEEP9:
	case MZ2_WIDOW_BLASTER_100:
	case MZ2_WIDOW_BLASTER_90:
	case MZ2_WIDOW_BLASTER_80:
	case MZ2_WIDOW_BLASTER_70:
	case MZ2_WIDOW_BLASTER_60:
	case MZ2_WIDOW_BLASTER_50:
	case MZ2_WIDOW_BLASTER_40:
	case MZ2_WIDOW_BLASTER_30:
	case MZ2_WIDOW_BLASTER_20:
	case MZ2_WIDOW_BLASTER_10:
	case MZ2_WIDOW_BLASTER_0:
	case MZ2_WIDOW_BLASTER_10L:
	case MZ2_WIDOW_BLASTER_20L:
	case MZ2_WIDOW_BLASTER_30L:
	case MZ2_WIDOW_BLASTER_40L:
	case MZ2_WIDOW_BLASTER_50L:
	case MZ2_WIDOW_BLASTER_60L:
	case MZ2_WIDOW_BLASTER_70L:
	case MZ2_WIDOW_RUN_1:
	case MZ2_WIDOW_RUN_2:
	case MZ2_WIDOW_RUN_3:
	case MZ2_WIDOW_RUN_4:
	case MZ2_WIDOW_RUN_5:
	case MZ2_WIDOW_RUN_6:
	case MZ2_WIDOW_RUN_7:
	case MZ2_WIDOW_RUN_8:
		CL_DynamicLight(origin, 200.0f + (rand() & 31), 0.0f, 1.0f, 0.0f, false, 1);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound("tank/tnkatck3.wav", 0), 1.0f, ATTN_NORM, 0.0f);

		break;
	case MZ2_WIDOW_DISRUPTOR:
		CL_DynamicLight(origin, 200.0f + (rand() & 31), -1.0f, -1.0f, -1.0f, false, 1);
		S_PlaySound(NULL, entity, CHAN_WEAPON, S_RegisterSound("weapons/disint2.wav", 0), 1.0f, ATTN_NORM, 0.0f);

		break;
	case MZ2_WIDOW_PLASMABEAM:
	case MZ2_WIDOW2_BEAMER_1:
	case MZ2_WIDOW2_BEAMER_2:
	case MZ2_WIDOW2_BEAMER_3:
	case MZ2_WIDOW2_BEAMER_4:
	case MZ2_WIDOW2_BEAMER_5:
	case MZ2_WIDOW2_BEAM_SWEEP_1:
	case MZ2_WIDOW2_BEAM_SWEEP_2:
	case MZ2_WIDOW2_BEAM_SWEEP_3:
	case MZ2_WIDOW2_BEAM_SWEEP_4:
	case MZ2_WIDOW2_BEAM_SWEEP_5:
	case MZ2_WIDOW2_BEAM_SWEEP_6:
	case MZ2_WIDOW2_BEAM_SWEEP_7:
	case MZ2_WIDOW2_BEAM_SWEEP_8:
	case MZ2_WIDOW2_BEAM_SWEEP_9:
	case MZ2_WIDOW2_BEAM_SWEEP_10:
	case MZ2_WIDOW2_BEAM_SWEEP_11:
		CL_DynamicLight(origin, 300.0f + (rand() & 100), 1.0f, 1.0f, 0.0f, false, 200);

		break;
	}
}