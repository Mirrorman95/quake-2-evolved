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
// sg_spawn.c - Entity spawning and parsing
//

// TODO:
// - Fix the HUD worldspawn crap


#include "g_local.h"


extern void		SG_ItemHealth_Spawn (edict_t *self);
extern void		SG_ItemHealthSmall_Spawn (edict_t *self);
extern void		SG_ItemHealthLarge_Spawn (edict_t *self);
extern void		SG_ItemHealthMega_Spawn (edict_t *self);

extern void		SG_InfoPlayerStart_Spawn (edict_t *self);
extern void		SG_InfoPlayerDeathmatch_Spawn (edict_t *self);
extern void		SG_InfoPlayerCoop_Spawn (edict_t *self);
extern void		SG_InfoPlayerIntermission_Spawn (edict_t *self);

extern void		SG_FuncPlat_Spawn (edict_t *entity);
extern void		SG_FuncRotating_Spawn (edict_t *entity);
extern void		SG_FuncButton_Spawn (edict_t *entity);
extern void		SG_FuncDoor_Spawn (edict_t *entity);
extern void		SG_FuncDoorSecret_Spawn (edict_t *self);
extern void		SG_FuncDoorRotating_Spawn (edict_t *entity);
extern void		SG_FuncWater_Spawn (edict_t *entity);
extern void		SG_FuncTrain_Spawn (edict_t *self);
extern void		SG_FuncConveyor_Spawn (edict_t *self);
extern void		SG_FuncWall_Spawn (edict_t *self);
extern void		SG_FuncObject_Spawn (edict_t *self);
extern void		SG_FuncExplosive_Spawn (edict_t *self);
extern void		SG_FuncTimer_Spawn (edict_t *self);
extern void		SG_FuncAreaPortal_Spawn (edict_t *ent);
extern void		SG_FuncClock_Spawn (edict_t *ent);
extern void		SG_FuncKillbox_Spawn (edict_t *self);

extern void		SG_TriggerAlways_Spawn (edict_t *entity);
extern void		SG_TriggerOnce_Spawn (edict_t *entity);
extern void		SG_TriggerMultiple_Spawn (edict_t *entity);
extern void		SG_TriggerRelay_Spawn (edict_t *entity);
extern void		SG_TriggerPush_Spawn (edict_t *entity);
extern void		SG_TriggerHurt_Spawn (edict_t *entity);
extern void		SG_TriggerKey_Spawn (edict_t *entity);
extern void		SG_TriggerCounter_Spawn (edict_t *entity);
extern void		SG_TriggerElevator_Spawn (edict_t *entity);
extern void		SG_TriggerGravity_Spawn (edict_t *entity);
extern void		SG_TriggerMonsterJump_Spawn (edict_t *entity);

extern void		SG_TargetTempEntity_Spawn (edict_t *entity);
extern void		SG_TargetSpeaker_Spawn (edict_t *entity);
extern void		SG_TargetExplosion_Spawn (edict_t *entity);
extern void		SG_TargetChangeLevel_Spawn (edict_t *entity);
extern void		SG_TargetSecret_Spawn (edict_t *entity);
extern void		SG_TargetGoal_Spawn (edict_t *entity);
extern void		SG_TargetSplash_Spawn (edict_t *entity);
extern void		SG_TargetSpawner_Spawn (edict_t *entity);
extern void		SG_TargetBlaster_Spawn (edict_t *entity);
extern void		SG_TargetCrossLevel_Trigger_Spawn (edict_t *entity);
extern void		SG_TargetCrossLevel_Target_Spawn (edict_t *entity);
extern void		SG_TargetLaser_Spawn (edict_t *self);
extern void		SG_TargetHelp_Spawn (edict_t *entity);
extern void		SG_TargetActor_Spawn (edict_t *entity);
extern void		SG_TargetLightRamp_Spawn (edict_t *self);
extern void		SG_TargetEarthquake_Spawn (edict_t *entity);
extern void		SG_TargetCharacter_Spawn (edict_t *entity);
extern void		SG_TargetString_Spawn (edict_t *entity);

extern void		SG_WorldSpawn_Spawn (edict_t *entity);
extern void		SG_ViewThing_Spawn (edict_t *entity);

extern void		SG_Light_Spawn (edict_t *self);
extern void		SG_LightMine1_Spawn (edict_t *self);
extern void		SG_LightMine2_Spawn (edict_t *self);
extern void		SG_InfoNull_Spawn (edict_t *self);
extern void		SG_InfoNotNull_Spawn (edict_t *self);
extern void		SG_PathCorner_Spawn (edict_t *self);
extern void		SG_PointCombat_Spawn (edict_t *self);

extern void		SG_MiscBarrel_Spawn (edict_t *self);
extern void		SG_MiscBanner_Spawn (edict_t *self);
extern void		SG_MiscSatelliteDish_Spawn (edict_t *self);
extern void		SG_MonsterActor_Spawn (edict_t *self);
extern void		SG_MiscGibArm_Spawn (edict_t *self);
extern void		SG_MiscGibLeg_Spawn (edict_t *self);
extern void		SG_MiscGibHead_Spawn (edict_t *self);
extern void		SG_MiscInsane_Spawn (edict_t *self);
extern void		SG_MiscDeadSoldier_Spawn (edict_t *self);
extern void		SG_MiscViper_Spawn (edict_t *self);
extern void		SG_MiscViperBomb_Spawn (edict_t *self);
extern void		SG_MiscBigViper_Spawn (edict_t *self);
extern void		SG_MiscStroggShip_Spawn (edict_t *self);
extern void		SG_MiscTeleporter_Spawn (edict_t *self);
extern void		SG_MiscTeleporterDest_Spawn (edict_t *self);
extern void		SG_MiscBlackhole_Spawn (edict_t *self);
extern void		SG_MiscEastertank_Spawn (edict_t *self);
extern void		SG_MiscEasterchick_Spawn (edict_t *self);
extern void		SG_MiscEasterchick2_Spawn (edict_t *self);

extern void		SG_MonsterBerserk_Spawn (edict_t *self);
extern void		SG_MonsterGladiator_Spawn (edict_t *self);
extern void		SG_MonsterGunner_Spawn (edict_t *self);
extern void		SG_MonsterInfantry_Spawn (edict_t *self);
extern void		SG_MonsterSoldierLight_Spawn (edict_t *self);
extern void		SG_MonsterSoldier_Spawn (edict_t *self);
extern void		SG_MonsterSoldierSS_Spawn (edict_t *self);
extern void		SG_MonsterTank_Spawn (edict_t *self);
extern void		SG_MonsterMedic_Spawn (edict_t *self);
extern void		SG_MonsterFlipper_Spawn (edict_t *self);
extern void		SG_MonsterIronMaiden_Spawn (edict_t *self);
extern void		SG_MonsterParasite_Spawn (edict_t *self);
extern void		SG_MonsterFlyer_Spawn (edict_t *self);
extern void		SG_MonsterBrain_Spawn (edict_t *self);
extern void		SG_MonsterFloater_Spawn (edict_t *self);
extern void		SG_MonsterIcarus_Spawn (edict_t *self);
extern void		SG_MonsterMutant_Spawn (edict_t *self);
extern void		SG_MonsterSuperTank_Spawn (edict_t *self);
extern void		SG_MonsterHornet_Spawn (edict_t *self);
extern void		SG_MonsterJorg_Spawn (edict_t *self);
extern void		SG_MonsterBoss3Stand_Spawn (edict_t *self);
extern void		SG_MonsterMakron_Spawn (edict_t *self);

extern void		SG_MonsterCommanderBody_Spawn (edict_t *self);

extern void		Turret_Breach_Spawn (edict_t *self);
extern void		Turret_Base_Spawn (edict_t *self);
extern void		Turret_Driver_Spawn (edict_t *self);

typedef struct {
	const char *		name;
	void				(*spawn)(edict_t *entity);
} spawnFunction_t;

static spawnFunction_t	sg_spawnFunctions[] = {
	{"item_health",					SG_ItemHealth_Spawn},
	{"item_health_small",			SG_ItemHealthSmall_Spawn},
	{"item_health_large",			SG_ItemHealthLarge_Spawn},
	{"item_health_mega",			SG_ItemHealthMega_Spawn},
	{"info_player_start",			SG_InfoPlayerStart_Spawn},
	{"info_player_deathmatch",		SG_InfoPlayerDeathmatch_Spawn},
	{"info_player_coop",			SG_InfoPlayerCoop_Spawn},
	{"info_player_intermission",	SG_InfoPlayerIntermission_Spawn},
	{"func_plat",					SG_FuncPlat_Spawn},
	{"func_button",					SG_FuncButton_Spawn},
	{"func_door",					SG_FuncDoor_Spawn},
	{"func_door_secret",			SG_FuncDoorSecret_Spawn},
	{"func_door_rotating",			SG_FuncDoorRotating_Spawn},
	{"func_rotating",				SG_FuncRotating_Spawn},
	{"func_train",					SG_FuncTrain_Spawn},
	{"func_water",					SG_FuncWater_Spawn},
	{"func_conveyor",				SG_FuncConveyor_Spawn},
	{"func_areaportal",				SG_FuncAreaPortal_Spawn},
	{"func_clock",					SG_FuncClock_Spawn},
	{"func_wall",					SG_FuncWall_Spawn},
	{"func_object",					SG_FuncObject_Spawn},
	{"func_timer",					SG_FuncTimer_Spawn},
	{"func_explosive",				SG_FuncExplosive_Spawn},
	{"func_killbox",				SG_FuncKillbox_Spawn},
	{"trigger_always",				SG_TriggerAlways_Spawn},
	{"trigger_once",				SG_TriggerOnce_Spawn},
	{"trigger_multiple",			SG_TriggerMultiple_Spawn},
	{"trigger_relay",				SG_TriggerRelay_Spawn},
	{"trigger_push",				SG_TriggerPush_Spawn},
	{"trigger_hurt",				SG_TriggerHurt_Spawn},
	{"trigger_key",					SG_TriggerKey_Spawn},
	{"trigger_counter",				SG_TriggerCounter_Spawn},
	{"trigger_elevator",			SG_TriggerElevator_Spawn},
	{"trigger_gravity",				SG_TriggerGravity_Spawn},
	{"trigger_monsterjump",			SG_TriggerMonsterJump_Spawn},
	{"target_temp_entity",			SG_TargetTempEntity_Spawn},
	{"target_speaker",				SG_TargetSpeaker_Spawn},
	{"target_explosion",			SG_TargetExplosion_Spawn},
	{"target_changelevel",			SG_TargetChangeLevel_Spawn},
	{"target_secret",				SG_TargetSecret_Spawn},
	{"target_goal",					SG_TargetGoal_Spawn},
	{"target_splash",				SG_TargetSplash_Spawn},
	{"target_spawner",				SG_TargetSpawner_Spawn},
	{"target_blaster",				SG_TargetBlaster_Spawn},
	{"target_crosslevel_trigger",	SG_TargetCrossLevel_Trigger_Spawn},
	{"target_crosslevel_target",	SG_TargetCrossLevel_Target_Spawn},
	{"target_laser",				SG_TargetLaser_Spawn},
	{"target_help",					SG_TargetHelp_Spawn},
	{"target_actor",				SG_TargetActor_Spawn},
	{"target_lightramp",			SG_TargetLightRamp_Spawn},
	{"target_earthquake",			SG_TargetEarthquake_Spawn},
	{"target_character",			SG_TargetCharacter_Spawn},
	{"target_string",				SG_TargetString_Spawn},
	{"worldspawn",					SG_WorldSpawn_Spawn},
	{"viewthing",					SG_ViewThing_Spawn},
	{"light",						SG_Light_Spawn},
	{"light_mine1",					SG_LightMine1_Spawn},
	{"light_mine2",					SG_LightMine2_Spawn},
	{"info_null",					SG_InfoNull_Spawn},
	{"func_group",					SG_InfoNull_Spawn},
	{"info_notnull",				SG_InfoNotNull_Spawn},
	{"path_corner",					SG_PathCorner_Spawn},
	{"point_combat",				SG_PointCombat_Spawn},
	{"misc_explobox",				SG_MiscBarrel_Spawn},
	{"misc_banner",					SG_MiscBanner_Spawn},
	{"misc_satellite_dish",			SG_MiscSatelliteDish_Spawn},
	{"misc_actor",					SG_MonsterActor_Spawn},
	{"misc_gib_arm",				SG_MiscGibArm_Spawn},
	{"misc_gib_leg",				SG_MiscGibLeg_Spawn},
	{"misc_gib_head",				SG_MiscGibHead_Spawn},
	{"misc_insane",					SG_MiscInsane_Spawn},
	{"misc_deadsoldier",			SG_MiscDeadSoldier_Spawn},
	{"misc_viper",					SG_MiscViper_Spawn},
	{"misc_viper_bomb",				SG_MiscViperBomb_Spawn},
	{"misc_bigviper",				SG_MiscBigViper_Spawn},
	{"misc_strogg_ship",			SG_MiscStroggShip_Spawn},
	{"misc_teleporter",				SG_MiscTeleporter_Spawn},
	{"misc_teleporter_dest",		SG_MiscTeleporterDest_Spawn},
	{"misc_blackhole",				SG_MiscBlackhole_Spawn},
	{"misc_eastertank",				SG_MiscBlackhole_Spawn},
	{"misc_easterchick",			SG_MiscEastertank_Spawn},
	{"misc_easterchick2",			SG_MiscEasterchick2_Spawn},
	{"monster_berserk",				SG_MonsterBerserk_Spawn},
	{"monster_gladiator",			SG_MonsterGladiator_Spawn},
	{"monster_gunner",				SG_MonsterGunner_Spawn},
	{"monster_infantry",			SG_MonsterInfantry_Spawn},
	{"monster_soldier_light",		SG_MonsterSoldierLight_Spawn},
	{"monster_soldier",				SG_MonsterSoldier_Spawn},
	{"monster_soldier_ss",			SG_MonsterSoldierSS_Spawn},
	{"monster_tank",				SG_MonsterTank_Spawn},
	{"monster_tank_commander",		SG_MonsterTank_Spawn},
	{"monster_medic",				SG_MonsterMedic_Spawn},
	{"monster_flipper",				SG_MonsterFlipper_Spawn},
	{"monster_chick",				SG_MonsterIronMaiden_Spawn},
	{"monster_parasite",			SG_MonsterParasite_Spawn},
	{"monster_flyer",				SG_MonsterFlyer_Spawn},
	{"monster_brain",				SG_MonsterBrain_Spawn},
	{"monster_floater",				SG_MonsterFloater_Spawn},
	{"monster_hover",				SG_MonsterIcarus_Spawn},
	{"monster_mutant",				SG_MonsterMutant_Spawn},
	{"monster_supertank",			SG_MonsterSuperTank_Spawn},
	{"monster_boss2",				SG_MonsterHornet_Spawn},
	{"monster_boss3_stand",			SG_MonsterBoss3Stand_Spawn},
	{"monster_jorg",				SG_MonsterJorg_Spawn},
	{"monster_makron",				SG_MonsterMakron_Spawn},
	{"monster_commander_body",		SG_MonsterCommanderBody_Spawn},
	{"turret_breach",				Turret_Breach_Spawn},
	{"turret_base",					Turret_Base_Spawn},
	{"turret_driver",				Turret_Driver_Spawn},
	{NULL,							NULL}
};


/*
 ==================
 SG_CallSpawn

 Finds the spawn function for the entity and calls it
 ==================
*/
void SG_CallSpawn (edict_t *entity){

	gItem_t			*item;
	spawnFunction_t	*sf;
	int				i;

	if (!entity->className){
		gi.dprintf("SG_SpawnEntity: NULL class name\n");
		return;
	}

	// Check item spawn functions
	for (i = 0, item = sg_itemList; i < game.num_items; i++, item++){
		if (!item->className)
			continue;

		if (!strcmp(item->className, entity->className)){
			// Found it
			SG_SpawnItem(entity, item);
			return;
		}
	}

	// Check normal spawn functions
	for (sf = sg_spawnFunctions; sf->name; sf++){
		if (!strcmp(sf->name, entity->className)){
			// Found it
			sf->spawn(entity);
			return;
		}
	}

	gi.dprintf("Entity class '%s' doesn't have a spawn function\n", entity->className);
}


/*
 ==============================================================================

 ENTITY PARSING

 ==============================================================================
*/


/*
 ==================
 SG_NewString
 ==================
*/
static char *SG_NewString (const char *string){

	char *newb, *new_p;
	int i, length;

	length = strlen(string) + 1;

	newb = (char *)gi.TagMalloc(length, TAG_LEVEL);

	new_p = newb;

	for (i = 0; i < length; i++){
		if (string[i] == '\\' && i < length - 1){
			i++;

			if (string[i] == 'n')
				*new_p++ = '\n';
			else
				*new_p++ = '\\';
		}
		else
			*new_p++ = string[i];
	}

	return newb;
}

/*
 ==================
 SG_ParseField

 Takes a key/value pair and sets the binary values in an edict
 ==================
*/
static void SG_ParseField (char *key, char *value, edict_t *entity){

	field_t	*f;
	byte	*b;
	float	v;
	vec3_t	vec;

	for (f = fields; f->name; f++){
		if (!(f->flags & FFL_NOSPAWN) && !Q_stricmp(f->name, key)){
			// Found it
			if (f->flags & FFL_SPAWNTEMP)
				b = (byte *)&st;
			else
				b = (byte *)entity;

			switch (f->type){
			case F_LSTRING:
				*(char **)(b + f->ofs) = SG_NewString(value);
				break;
			case F_VECTOR:
				sscanf (value, "%f %f %f", &vec[0], &vec[1], &vec[2]);
				((float *)(b + f->ofs))[0] = vec[0];
				((float *)(b + f->ofs))[1] = vec[1];
				((float *)(b + f->ofs))[2] = vec[2];
				break;
			case F_INT:
				*(int *)(b + f->ofs) = atoi(value);
				break;
			case F_FLOAT:
				*(float *)(b + f->ofs) = atof(value);
				break;
			case F_ANGLEHACK:
				v = atof(value);
				((float *)(b + f->ofs))[0] = 0;
				((float *)(b + f->ofs))[1] = v;
				((float *)(b + f->ofs))[2] = 0;
				break;
			case F_IGNORE:
				break;
			}

			return;
		}
	}

	gi.dprintf("%s is not a field\n", key);
}

/*
 ==================
 SG_ParseEdict

 Parses an edict out of the given string, returning the new position
 ed should be a properly initialized empty edict.
 ==================
*/
static char *SG_ParseEdict (char *data, edict_t *entity){

	qboolean	init;
	char		keyname[256];
	char		*token;

	init = false;
	memset(&st, 0, sizeof(st));

	// Go through all the dictionary pairs
	while (1){	
		// Parse key
		token = COM_Parse(&data);

		if (token[0] == '}')
			break;
		if (!data)
			gi.error("ED_ParseEntity: EOF without closing brace");

		strncpy(keyname, token, sizeof(keyname) - 1);

		// Parse value	
		token = COM_Parse(&data);

		if (!data)
			gi.error("ED_ParseEntity: EOF without closing brace");
		if (token[0] == '}')
			gi.error("ED_ParseEntity: closing brace without data");

		init = true;	

		// Keynames with a leading underscore are used for utility comments,
		// and are immediately discarded by quake
		if (keyname[0] == '_')
			continue;

		SG_ParseField(keyname, token, entity);
	}

	if (!init)
		memset(entity, 0, sizeof(*entity));

	return data;
}

/*
 ==================
 SG_FindTeams

 Chain together all entities with a matching team field.

 All but the first will have the FL_TEAMSLAVE flag set.
 All but the last will have the teamchain field set to the next one
 ==================
*/
static void SG_FindTeams (){

	int		c, c2;
	edict_t	*e, *e2, *chain;
	int		i, j;

	c = 0;
	c2 = 0;

	for (i = 1, e = g_edicts+i; i < globals.num_edicts; i++, e++){
		if (!e->inuse)
			continue;
		if (!e->team)
			continue;
		if (e->flags & FL_TEAMSLAVE)
			continue;

		chain = e;
		e->teammaster = e;
		c++;
		c2++;

		for (j = i + 1, e2 = e + 1; j < globals.num_edicts; j++, e2++){
			if (!e2->inuse)
				continue;
			if (!e2->team)
				continue;
			if (e2->flags & FL_TEAMSLAVE)
				continue;

			if (!strcmp(e->team, e2->team)){
				c2++;
				chain->teamchain = e2;
				e2->teammaster = e;
				chain = e2;
				e2->flags |= FL_TEAMSLAVE;
			}
		}
	}

	gi.dprintf("%i teams with %i entities\n", c, c2);
}

/*
 ==================
 SG_SpawnEntities

 Parses textual entity definitions out of a file and spawns entities
 ==================
*/
void SG_SpawnEntities (char *mapName, char *entities, char *spawnPoint){

	edict_t		*entity;
	int			spawned, inhibited;
	char		*com_token;
	int			i;
	float		skill_level;

	gi.dprintf("Spawning entities\n");

	skill_level = floor(skill->value);

	if (skill_level < 0)
		skill_level = 0;
	if (skill_level > 3)
		skill_level = 3;
	if (skill->value != skill_level)
		gi.cvar_forceset("skill", va("%f", skill_level));

	SG_SaveClientData();

	gi.FreeTags(TAG_LEVEL);

	memset(&level, 0, sizeof(level));
	memset(g_edicts, 0, game.maxentities * sizeof (g_edicts[0]));

	strncpy(level.mapname, mapName, sizeof(level.mapname) - 1);
	strncpy(game.spawnpoint, spawnPoint, sizeof(game.spawnpoint) - 1);

	// Set client fields on player ents
	for (i = 0; i < game.maxclients; i++)
		g_edicts[i + 1].client = game.clients + i;

	entity = NULL;
	spawned = 1;
	inhibited = 0;

	// Parse entities
	while (1){
		// Parse the opening brace	
		com_token = COM_Parse(&entities);

		if (!entities)
			break;
		if (com_token[0] != '{')
			gi.error("ED_LoadFromFile: found %s when expecting {", com_token);

		if (!entity)
			entity = g_edicts;
		else
			entity = SG_AllocEntity();

		entities = SG_ParseEdict(entities, entity);

		// Yet another map hack
		if (!Q_stricmp(level.mapname, "command") && !Q_stricmp(entity->className, "trigger_once") && !Q_stricmp(entity->model, "*27"))
			entity->spawnflags &= ~SPAWNFLAG_NOT_HARD;

		// Remove things (except the world) from different skill levels or deathmatch
		if (entity != g_edicts){
			if (deathmatch->value){
				if (entity->spawnflags & SPAWNFLAG_NOT_DEATHMATCH){
					SG_FreeEntity(entity);	
					inhibited++;
					continue;
				}
			}
			else {
				if (((skill->value == 0) && (entity->spawnflags & SPAWNFLAG_NOT_EASY)) ||
					((skill->value == 1) && (entity->spawnflags & SPAWNFLAG_NOT_MEDIUM)) ||
					(((skill->value == 2) || (skill->value == 3)) && (entity->spawnflags & SPAWNFLAG_NOT_HARD)))
				{
					SG_FreeEntity(entity);	
					inhibited++;
					continue;
				}
			}

			entity->spawnflags &= ~(SPAWNFLAG_NOT_EASY|SPAWNFLAG_NOT_MEDIUM|SPAWNFLAG_NOT_HARD|SPAWNFLAG_NOT_COOP|SPAWNFLAG_NOT_DEATHMATCH);
		}

		SG_CallSpawn(entity);

		spawned++;
	}	

	gi.dprintf("...%i entities spawned, %i inhibited\n", spawned, inhibited);

	SG_FindTeams();

	SG_InitClientTrail();
}


/*
 ==============================================================================

 WORLDSPAWN

 Only used for the world.

 "sky"			environment map name
 "skyaxis"		vector axis for rotating sky
 "skyrotate"	speed of rotation in degrees/second
 "sounds"		music cd track number
 "gravity"		800 is default gravity
 "message"		text to print at user logon

 // Cursor positioning
 xl <value>
 xr <value>
 yb <value>
 yt <value>
 xv <value>
 yv <value>

 // Drawing
 statpic <name>
 pic <stat>
 num <fieldwidth> <stat>
 string <stat>

 // Control
 if <stat>
 ifeq <stat> <value>
 ifbit <stat> <value>
 endif
 ==============================================================================
*/

char *single_statusbar = 
"yb	-24 "

// health
"xv	0 "
"hnum "
"xv	50 "
"pic 0 "

// ammo
"if 2 "
"	xv	100 "
"	anum "
"	xv	150 "
"	pic 2 "
"endif "

// armor
"if 4 "
"	xv	200 "
"	rnum "
"	xv	250 "
"	pic 4 "
"endif "

// selected item
"if 6 "
"	xv	296 "
"	pic 6 "
"endif "

"yb	-50 "

// picked up item
"if 7 "
"	xv	0 "
"	pic 7 "
"	xv	26 "
"	yb	-42 "
"	stat_string 8 "
"	yb	-50 "
"endif "

// timer
"if 9 "
"	xv	262 "
"	num	2	10 "
"	xv	296 "
"	pic	9 "
"endif "

//  help / weapon icon 
"if 11 "
"	xv	148 "
"	pic	11 "
"endif "
;

char *dm_statusbar =
"yb	-24 "

// health
"xv	0 "
"hnum "
"xv	50 "
"pic 0 "

// ammo
"if 2 "
"	xv	100 "
"	anum "
"	xv	150 "
"	pic 2 "
"endif "

// armor
"if 4 "
"	xv	200 "
"	rnum "
"	xv	250 "
"	pic 4 "
"endif "

// selected item
"if 6 "
"	xv	296 "
"	pic 6 "
"endif "

"yb	-50 "

// picked up item
"if 7 "
"	xv	0 "
"	pic 7 "
"	xv	26 "
"	yb	-42 "
"	stat_string 8 "
"	yb	-50 "
"endif "

// timer
"if 9 "
"	xv	246 "
"	num	2	10 "
"	xv	296 "
"	pic	9 "
"endif "

//  help / weapon icon 
"if 11 "
"	xv	148 "
"	pic	11 "
"endif "

//  frags
"xr	-50 "
"yt 2 "
"num 3 14 "

// spectator
"if 17 "
  "xv 0 "
  "yb -58 "
  "string2 \"SPECTATOR MODE\" "
"endif "

// chase camera
"if 16 "
  "xv 0 "
  "yb -68 "
  "string \"Chasing\" "
  "xv 64 "
  "stat_string 16 "
"endif "
;

/*
 ==================
 SG_WorldSpawn_Spawn
 ==================
*/
void SG_WorldSpawn_Spawn (edict_t *entity){

	entity->movetype = MOVETYPE_PUSH;
	entity->solid = SOLID_BSP;
	entity->inuse = true;			// Since the world doesn't use SG_AllocEntity()
	entity->s.modelindex = 1;		// World model is always index 1

	// Reserve some spots for dead player bodies for coop / deathmatch
	SG_InitClientBodyQue();

	// Set configstrings for items
	SG_SetItemNames();

	if (st.nextmap)
		strcpy(level.nextmap, st.nextmap);

	// Make some data visible to the server

	if (entity->message && entity->message[0]){
		gi.configstring(CS_NAME, entity->message);
		strncpy(level.level_name, entity->message, sizeof(level.level_name));
	}
	else
		strncpy(level.level_name, level.mapname, sizeof(level.level_name));

	if (st.sky && st.sky[0])
		gi.configstring(CS_SKY, st.sky);
	else
		gi.configstring(CS_SKY, "unit1_");

	gi.configstring(CS_SKYROTATE, va("%f", st.skyrotate));
	gi.configstring(CS_SKYAXIS, va("%f %f %f", st.skyaxis[0], st.skyaxis[1], st.skyaxis[2]));
	gi.configstring(CS_CDTRACK, va("%i", entity->sounds));
	gi.configstring(CS_MAXCLIENTS, va("%i", (int)(maxclients->value)));

	// Status bar program
	if (deathmatch->value)
		gi.configstring(CS_STATUSBAR, dm_statusbar);
	else
		gi.configstring(CS_STATUSBAR, single_statusbar);

	// Help icon for statusbar
	gi.imageindex("i_help");
	level.pic_health = gi.imageindex("i_health");
	gi.imageindex("help");
	gi.imageindex("field_3");

	if (!st.gravity)
		gi.cvar_set("sv_gravity", "800");
	else
		gi.cvar_set("sv_gravity", st.gravity);

	snd_fry = gi.soundindex("player/fry.wav");	// Standing in lava / slime

	SG_PrecacheItem(SG_FindItem("Blaster"));

	gi.soundindex("player/lava1.wav");
	gi.soundindex("player/lava2.wav");

	gi.soundindex("misc/pc_up.wav");
	gi.soundindex("misc/talk1.wav");

	gi.soundindex("misc/udeath.wav");

	// Gibs
	gi.soundindex("items/respawn1.wav");

	// Sexed sounds
	gi.soundindex("*death1.wav");
	gi.soundindex("*death2.wav");
	gi.soundindex("*death3.wav");
	gi.soundindex("*death4.wav");
	gi.soundindex("*fall1.wav");
	gi.soundindex("*fall2.wav");	
	gi.soundindex("*gurp1.wav");		// Drowning damage
	gi.soundindex("*gurp2.wav");	
	gi.soundindex("*jump1.wav");		// Player jump
	gi.soundindex("*pain25_1.wav");
	gi.soundindex("*pain25_2.wav");
	gi.soundindex("*pain50_1.wav");
	gi.soundindex("*pain50_2.wav");
	gi.soundindex("*pain75_1.wav");
	gi.soundindex("*pain75_2.wav");
	gi.soundindex("*pain100_1.wav");
	gi.soundindex("*pain100_2.wav");

	// Sexed models
	// THIS ORDER MUST MATCH THE DEFINES IN g_local.h
	// you can add more, max 15
	gi.modelindex("#w_blaster.md2");
	gi.modelindex("#w_shotgun.md2");
	gi.modelindex("#w_sshotgun.md2");
	gi.modelindex("#w_machinegun.md2");
	gi.modelindex("#w_chaingun.md2");
	gi.modelindex("#a_grenades.md2");
	gi.modelindex("#w_glauncher.md2");
	gi.modelindex("#w_rlauncher.md2");
	gi.modelindex("#w_hyperblaster.md2");
	gi.modelindex("#w_railgun.md2");
	gi.modelindex("#w_bfg.md2");

	gi.soundindex("player/gasp1.wav");		// Gasping for air
	gi.soundindex("player/gasp2.wav");		// Head breaking surface, not gasping

	gi.soundindex("player/watr_in.wav");	// Feet hitting water
	gi.soundindex("player/watr_out.wav");	// Feet leaving water

	gi.soundindex("player/watr_un.wav");	// Head going underwater

	gi.soundindex("player/u_breath1.wav");
	gi.soundindex("player/u_breath2.wav");

	gi.soundindex("items/pkup.wav");		// Bonus item pickup
	gi.soundindex("world/land.wav");		// Landing thud
	gi.soundindex("misc/h2ohit1.wav");		// Landing splash

	gi.soundindex("items/damage.wav");
	gi.soundindex("items/protect.wav");
	gi.soundindex("items/protect4.wav");
	gi.soundindex("weapons/noammo.wav");

	gi.soundindex("infantry/inflies1.wav");

	sm_meat_index = gi.modelindex("models/objects/gibs/sm_meat/tris.md2");
	gi.modelindex("models/objects/gibs/arm/tris.md2");
	gi.modelindex("models/objects/gibs/bone/tris.md2");
	gi.modelindex("models/objects/gibs/bone2/tris.md2");
	gi.modelindex("models/objects/gibs/chest/tris.md2");
	gi.modelindex("models/objects/gibs/skull/tris.md2");
	gi.modelindex("models/objects/gibs/head2/tris.md2");


	// Setup light animation tables. 'a' is total darkness, 'z' is doublebright.

	// 0 normal
	gi.configstring(CS_LIGHTS + 0, "m");

	// 1 FLICKER (first variety)
	gi.configstring(CS_LIGHTS + 1, "mmnmmommommnonmmonqnmmo");

	// 2 SLOW STRONG PULSE
	gi.configstring(CS_LIGHTS + 2, "abcdefghijklmnopqrstuvwxyzyxwvutsrqponmlkjihgfedcba");

	// 3 CANDLE (first variety)
	gi.configstring(CS_LIGHTS + 3, "mmmmmaaaaammmmmaaaaaabcdefgabcdefg");

	// 4 FAST STROBE
	gi.configstring(CS_LIGHTS + 4, "mamamamamama");

	// 5 GENTLE PULSE 1
	gi.configstring(CS_LIGHTS + 5,"jklmnopqrstuvwxyzyxwvutsrqponmlkj");

	// 6 FLICKER (second variety)
	gi.configstring(CS_LIGHTS + 6, "nmonqnmomnmomomno");

	// 7 CANDLE (second variety)
	gi.configstring(CS_LIGHTS + 7, "mmmaaaabcdefgmmmmaaaammmaamm");

	// 8 CANDLE (third variety)
	gi.configstring(CS_LIGHTS + 8, "mmmaaammmaaammmabcdefaaaammmmabcdefmmmaaaa");

	// 9 SLOW STROBE (fourth variety)
	gi.configstring(CS_LIGHTS + 9, "aaaaaaaazzzzzzzz");

	// 10 FLUORESCENT FLICKER
	gi.configstring(CS_LIGHTS + 10, "mmamammmmammamamaaamammma");

	// 11 SLOW PULSE NOT FADE TO BLACK
	gi.configstring(CS_LIGHTS + 11, "abcdefghijklmnopqrrqponmlkjihgfedcba");

	// Styles 32-62 are assigned by the light program for switchable lights

	// 63 testing
	gi.configstring(CS_LIGHTS + 63, "a");
}