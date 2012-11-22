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
// sg_main.c - Main file
//


#include "g_local.h"


gameLocals_t			game;
levelLocals_t			level;
game_import_t			gi;
game_export_t			globals;
spawn_temp_t			st;

int						sm_meat_index;
int						snd_fry;
int						meansOfDeath;

edict_t *				g_edicts;

cvar_t *				deathmatch;
cvar_t *				coop;
cvar_t *				dmflags;
cvar_t *				skill;
cvar_t *				fraglimit;
cvar_t *				timelimit;
cvar_t *				password;
cvar_t *				spectator_password;
cvar_t *				needpass;
cvar_t *				maxclients;
cvar_t *				maxspectators;
cvar_t *				maxentities;
cvar_t *				g_select_empty;
cvar_t *				dedicated;

cvar_t *				filterban;

cvar_t *				sv_maxvelocity;
cvar_t *				sv_gravity;

cvar_t *				sv_rollspeed;
cvar_t *				sv_rollangle;
cvar_t *				gun_x;
cvar_t *				gun_y;
cvar_t *				gun_z;

cvar_t *				run_pitch;
cvar_t *				run_roll;
cvar_t *				bob_up;
cvar_t *				bob_pitch;
cvar_t *				bob_roll;

cvar_t *				sv_cheats;

cvar_t *				flood_msgs;
cvar_t *				flood_persecond;
cvar_t *				flood_waitdelay;

cvar_t *				sv_maplist;

cvar_t *				sg_allowMonsterSpawn;

static void SG_ClientEndServerFrames ();


/*
 ==============================================================================

 EXPORTED FUNCTIONS

 ==============================================================================
*/


/*
 ==================
 GetGameAPI

 Returns a pointer to the structure with all entry points
 and global variables
 ==================
*/
game_export_t *GetGameAPI (game_import_t *import){

	gi = *import;

	globals.apiversion = GAME_API_VERSION;
	globals.Init = SG_InitGame;
	globals.Shutdown = SG_ShutdownGame;
	globals.SpawnEntities = SG_SpawnEntities;

	globals.WriteGame = SG_WriteGame;
	globals.ReadGame = SG_ReadGame;
	globals.WriteLevel = SG_WriteLevel;
	globals.ReadLevel = SG_ReadLevel;

	globals.ClientThink = SG_ClientThinkFramework;
	globals.ClientConnect = SG_ClientConnect;
	globals.ClientUserinfoChanged = SG_ClientUserInfoChanged;
	globals.ClientDisconnect = SG_ClientDisconnect;
	globals.ClientBegin = SG_ClientBegin;
	globals.ClientCommand = SG_ClientCommand;

	globals.RunFrame = SG_Framework;

	globals.ServerCommand = SG_ServerCommand;

	globals.edict_size = sizeof(edict_t);

	return &globals;
}

// This is only here so the functions in q_shared.c and q_shwin.c can link
#ifndef GAME_HARD_LINKED
void Sys_Error (char *error, ...){

	va_list		argptr;
	char		text[1024];

	va_start(argptr, error);
	vsprintf(text, error, argptr);
	va_end(argptr);

	gi.error(ERR_FATAL, "%s", text);
}

void Com_Printf (char *msg, ...){

	va_list		argptr;
	char		text[1024];

	va_start(argptr, msg);
	vsprintf(text, msg, argptr);
	va_end(argptr);

	gi.dprintf("%s", text);
}
#endif


// ============================================================================


/*
 ==================
 SG_ExitLevel
 ==================
*/
static void SG_ExitLevel (){

	char	cmd[256];
	int		i;
	edict_t	*entity;

	Com_sprintf(cmd, sizeof(cmd), "gamemap \"%s\"\n", level.changemap);
	gi.AddCommandString(cmd);

	level.changemap = NULL;
	level.exitintermission = 0;
	level.intermissiontime = 0.0f;

	SG_ClientEndServerFrames();

	// Clear some things before going to next level
	for (i = 0; i < maxclients->value; i++){
		entity = g_edicts + 1 + i;

		if (!entity->inuse)
			continue;

		if (entity->health > entity->client->pers.max_health)
			entity->health = entity->client->pers.max_health;
	}
}

/*
 ==================
 SG_CreateTargetChangeLevel

 Returns the created target changelevel
 ==================
*/
static edict_t *SG_CreateTargetChangeLevel (const char *map){

	edict_t *entity;

	entity = SG_AllocEntity();

	entity->className = "target_changelevel";
	Com_sprintf(level.nextmap, sizeof(level.nextmap), "%s", map);
	entity->map = level.nextmap;

	return entity;
}

/*
 ==================
 SG_EndDeathMatchLevel

 The timelimit or fraglimit has been exceeded
 ==================
*/
static void SG_EndDeathMatchLevel (){

	char				*s, *t, *f;
	edict_t				*entity;
	static const char	*seps = " ,\n\r";

	// Stay on same level flag
	if ((int)dmflags->value & DF_SAME_LEVEL){
		SG_BeginIntermission(SG_CreateTargetChangeLevel(level.mapname));
		return;
	}

	// See if it's in the map list
	if (*sv_maplist->string){
		s = strdup(sv_maplist->string);
		f = NULL;
		t = strtok(s, seps);

		while (t != NULL){
			if (!Q_stricmp(t, level.mapname)){
				// It's in the list, go to the next one
				t = strtok(NULL, seps);

				if (t == NULL){	// End of list, go to first one
					if (f == NULL)	// There isn't a first one, same level
						SG_BeginIntermission(SG_CreateTargetChangeLevel(level.mapname));
					else
						SG_BeginIntermission(SG_CreateTargetChangeLevel(f));
				}
				else
					SG_BeginIntermission(SG_CreateTargetChangeLevel(t));

				free(s);
				return;
			}

			if (!f)
				f = t;

			t = strtok(NULL, seps);
		}

		free(s);
	}

	// Go to a specific map
	if (level.nextmap[0])
		SG_BeginIntermission(SG_CreateTargetChangeLevel(level.nextmap));
	else {
		// Search for a changelevel
		entity = SG_FindEntity(NULL, FOFS(className), "target_changelevel");
		if (!entity){
			// The map designer didn't include a changelevel,
			// so create a fake ent that goes back to the same level
			SG_BeginIntermission(SG_CreateTargetChangeLevel(level.mapname));
			return;
		}

		SG_BeginIntermission(entity);
	}
}

/*
 ==================
 SG_CheckDeathMatchRules
 ==================
*/
static void SG_CheckDeathMatchRules (){

	int			i;
	gclient_t	*client;

	if (level.intermissiontime)
		return;

	if (!deathmatch->value)
		return;

	if (timelimit->value){
		if (level.time >= timelimit->value * 60.0f){
			gi.bprintf(PRINT_HIGH, "Timelimit hit.\n");
			SG_EndDeathMatchLevel();
			return;
		}
	}

	if (fraglimit->value){
		for (i = 0; i < maxclients->value; i++){
			client = game.clients + i;

			if (!g_edicts[i + 1].inuse)
				continue;

			if (client->resp.score >= fraglimit->value)
			{
				gi.bprintf(PRINT_HIGH, "Fraglimit hit.\n");
				SG_EndDeathMatchLevel();
				return;
			}
		}
	}
}

/*
 ==================
 SG_CheckNeedPass
 ==================
*/
static void SG_CheckNeedPass (){

	int		need;

	// If password or spectator_password has changed, update needpass
	// as needed
	if (password->modified || spectator_password->modified) {
		password->modified = spectator_password->modified = false;

		need = 0;

		if (*password->string && Q_stricmp(password->string, "none"))
			need |= 1;
		if (*spectator_password->string && Q_stricmp(spectator_password->string, "none"))
			need |= 2;

		gi.cvar_set("needpass", va("%d", need));
	}
}

/*
 ==================
 SG_ClientEndServerFrames
 ==================
*/
static void SG_ClientEndServerFrames (){

	int		i;
	edict_t	*entity;

	// Calculate the player views now that all pushing
	// and damage has been added
	for (i = 0; i < maxclients->value; i++){
		entity = g_edicts + 1 + i;

		if (!entity->inuse || !entity->client)
			continue;

		SG_ClientEndServerFrame(entity);
	}
}


/*
 ==============================================================================

 FRAMEWORK

 ==============================================================================
*/


/*
 ==================
 SG_Framework

 Advances the world by 0.1 seconds
 ==================
*/
void SG_Framework (){

	int		i;
	edict_t	*entity;

	level.framenum++;
	level.time = level.framenum * FRAMETIME;

	// Choose a client for monsters to target this frame
	SG_AISetSightClient();

	// Exit intermissions
	if (level.exitintermission){
		SG_ExitLevel();
		return;
	}

	// Treat each object in turn even the world gets a chance to think
	entity = &g_edicts[0];

	for (i = 0; i < globals.num_edicts; i++, entity++){
		if (!entity->inuse)
			continue;

		level.current_entity = entity;

		VectorCopy(entity->s.origin, entity->s.old_origin);

		// If the ground entity moved, make sure we are still on it
		if ((entity->groundentity) && (entity->groundentity->linkcount != entity->groundentity_linkcount)){
			entity->groundentity = NULL;

			if (!(entity->flags & (FL_SWIM|FL_FLY)) && (entity->svflags & SVF_MONSTER))
				SG_CheckGround(entity);
		}

		if (i > 0 && i <= maxclients->value){
			SG_ClientBeginServerFrame(entity);
			continue;
		}

		G_RunEntity(entity);
	}

	// See if it is time to end a deathmatch
	SG_CheckDeathMatchRules();

	// See if needpass needs updated
	SG_CheckNeedPass();

	// Build the playerstate_t structures for all players
	SG_ClientEndServerFrames();
}


/*
 ==============================================================================

 INITIALIZATION AND SHUTDOWN

 ==============================================================================
*/


/*
 ==================
 SG_InitGame

 This will be called when the dll is first loaded, which
 only happens when a new game is started or a save game is loaded.
 ==================
*/
void SG_InitGame (){

	gi.dprintf("--------- Game Initialization ---------\n");
	gi.dprintf("Game DLL version: %s\n", GAME_VERSION);
	gi.dprintf("Game date: %s\n", __DATE__);

	gun_x = gi.cvar("gun_x", "0", 0);
	gun_y = gi.cvar("gun_y", "0", 0);
	gun_z = gi.cvar("gun_z", "0", 0);

	// FIXME: sv_ prefix is wrong for these
	sv_rollspeed = gi.cvar("sv_rollspeed", "200", 0);
	sv_rollangle = gi.cvar("sv_rollangle", "2", 0);
	sv_maxvelocity = gi.cvar("sv_maxvelocity", "2000", 0);
	sv_gravity = gi.cvar("sv_gravity", "800", 0);

	// noset vars
	dedicated = gi.cvar("dedicated", "0", CVAR_NOSET);

	// latched vars
	sv_cheats = gi.cvar("cheats", "0", CVAR_SERVERINFO|CVAR_LATCH);
	gi.cvar("gamename", GAMEVERSION , CVAR_SERVERINFO | CVAR_LATCH);
	gi.cvar("gamedate", __DATE__ , CVAR_SERVERINFO | CVAR_LATCH);

	maxclients = gi.cvar("maxclients", "4", CVAR_SERVERINFO | CVAR_LATCH);
	maxspectators = gi.cvar("maxspectators", "4", CVAR_SERVERINFO);
	deathmatch = gi.cvar("deathmatch", "0", CVAR_LATCH);
	coop = gi.cvar("coop", "0", CVAR_LATCH);
	skill = gi.cvar("skill", "1", CVAR_LATCH);
	maxentities = gi.cvar("maxentities", "1024", CVAR_LATCH);

	// change anytime vars
	dmflags = gi.cvar("dmflags", "0", CVAR_SERVERINFO);
	fraglimit = gi.cvar("fraglimit", "0", CVAR_SERVERINFO);
	timelimit = gi.cvar("timelimit", "0", CVAR_SERVERINFO);
	password = gi.cvar("password", "", CVAR_USERINFO);
	spectator_password = gi.cvar("spectator_password", "", CVAR_USERINFO);
	needpass = gi.cvar("needpass", "0", CVAR_SERVERINFO);
	filterban = gi.cvar("filterban", "1", 0);

	g_select_empty = gi.cvar("g_select_empty", "0", CVAR_ARCHIVE);

	run_pitch = gi.cvar("run_pitch", "0.002", 0);
	run_roll = gi.cvar("run_roll", "0.005", 0);
	bob_up  = gi.cvar("bob_up", "0.005", 0);
	bob_pitch = gi.cvar("bob_pitch", "0.002", 0);
	bob_roll = gi.cvar("bob_roll", "0.002", 0);

	// flood control
	flood_msgs = gi.cvar("flood_msgs", "4", 0);
	flood_persecond = gi.cvar("flood_persecond", "4", 0);
	flood_waitdelay = gi.cvar("flood_waitdelay", "10", 0);

	// dm map list
	sv_maplist = gi.cvar("sv_maplist", "", 0);

	sg_allowMonsterSpawn = gi.cvar("sg_allowMonsterSpawn", "0", 0);

	// Initialize items
	SG_InitItems();

	Com_sprintf(game.helpmessage1, sizeof(game.helpmessage1), "");

	Com_sprintf(game.helpmessage2, sizeof(game.helpmessage2), "");

	// Initialize all entities for this game
	game.maxentities = maxentities->value;
	g_edicts =  (edict_t *)gi.TagMalloc(game.maxentities * sizeof(g_edicts[0]), TAG_GAME);
	globals.edicts = g_edicts;
	globals.max_edicts = game.maxentities;

	// Initialize all clients for this game
	game.maxclients = maxclients->value;
	game.clients = (gclient_t *)gi.TagMalloc(game.maxclients * sizeof(game.clients[0]), TAG_GAME);
	globals.num_edicts = game.maxclients + 1;
}

/*
 ==================
 SG_ShutdownGame
 ==================
*/
void SG_ShutdownGame (){

	gi.dprintf("-------- Game Shutdown --------\n");

	gi.FreeTags(TAG_LEVEL);
	gi.FreeTags(TAG_GAME);

	gi.dprintf("---------------------------------\n");
}