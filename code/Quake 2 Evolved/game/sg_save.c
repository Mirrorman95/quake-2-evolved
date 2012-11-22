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
// sg_save.c - Data write and reading
//


#include "g_local.h"


#define Function(f)						{#f, f}

field_t					fields[] = {
	{"classname",			FOFS(className),				F_LSTRING},
	{"model",				FOFS(model),					F_LSTRING},
	{"spawnflags",			FOFS(spawnflags),				F_INT},
	{"speed",				FOFS(speed),					F_FLOAT},
	{"accel",				FOFS(accel),					F_FLOAT},
	{"decel",				FOFS(decel),					F_FLOAT},
	{"target",				FOFS(target),					F_LSTRING},
	{"targetname",			FOFS(targetname),				F_LSTRING},
	{"pathtarget",			FOFS(pathtarget),				F_LSTRING},
	{"deathtarget",			FOFS(deathtarget),				F_LSTRING},
	{"killtarget",			FOFS(killtarget),				F_LSTRING},
	{"combattarget",		FOFS(combattarget),				F_LSTRING},
	{"message",				FOFS(message),					F_LSTRING},
	{"team",				FOFS(team),						F_LSTRING},
	{"wait",				FOFS(wait),						F_FLOAT},
	{"delay",				FOFS(delay),					F_FLOAT},
	{"random",				FOFS(random),					F_FLOAT},
	{"move_origin",			FOFS(moveOrigin),				F_VECTOR},
	{"move_angles",			FOFS(moveAngles),				F_VECTOR},
	{"style",				FOFS(style),					F_INT},
	{"count",				FOFS(count),					F_INT},
	{"health",				FOFS(health),					F_INT},
	{"sounds",				FOFS(sounds),					F_INT},
	{"light",				0,								F_IGNORE},
	{"dmg",					FOFS(dmg),						F_INT},
	{"mass",				FOFS(mass),						F_INT},
	{"volume",				FOFS(volume),					F_FLOAT},
	{"attenuation",			FOFS(attenuation),				F_FLOAT},
	{"map",					FOFS(map),						F_LSTRING},
	{"origin",				FOFS(s.origin),					F_VECTOR},
	{"angles",				FOFS(s.angles),					F_VECTOR},
	{"angle",				FOFS(s.angles),					F_ANGLEHACK},
	{"goalentity",			FOFS(goalEntity),				F_EDICT,		FFL_NOSPAWN},
	{"movetarget",			FOFS(moveTarget),				F_EDICT,		FFL_NOSPAWN},
	{"enemy",				FOFS(enemy),					F_EDICT,		FFL_NOSPAWN},
	{"oldenemy",			FOFS(oldenemy),					F_EDICT,		FFL_NOSPAWN},
	{"activator",			FOFS(activator),				F_EDICT,		FFL_NOSPAWN},
	{"groundentity",		FOFS(groundentity),				F_EDICT,		FFL_NOSPAWN},
	{"teamchain",			FOFS(teamchain),				F_EDICT,		FFL_NOSPAWN},
	{"teammaster",			FOFS(teammaster),				F_EDICT,		FFL_NOSPAWN},
	{"owner",				FOFS(owner),					F_EDICT,		FFL_NOSPAWN},
	{"mynoise",				FOFS(mynoise),					F_EDICT,		FFL_NOSPAWN},
	{"mynoise2",			FOFS(mynoise2),					F_EDICT,		FFL_NOSPAWN},
	{"target_ent",			FOFS(target_ent),				F_EDICT,		FFL_NOSPAWN},
	{"chain",				FOFS(chain),					F_EDICT,		FFL_NOSPAWN},
	{"prethink",			FOFS(prethink),					F_FUNCTION,		FFL_NOSPAWN},
	{"think",				FOFS(think),					F_FUNCTION,		FFL_NOSPAWN},
	{"blocked",				FOFS(blocked),					F_FUNCTION,		FFL_NOSPAWN},
	{"touch",				FOFS(touch),					F_FUNCTION,		FFL_NOSPAWN},
	{"use",					FOFS(use),						F_FUNCTION,		FFL_NOSPAWN},
	{"pain",				FOFS(pain),						F_FUNCTION,		FFL_NOSPAWN},
	{"die",					FOFS(die),						F_FUNCTION,		FFL_NOSPAWN},
	{"stand",				FOFS(monsterinfo.stand),		F_FUNCTION,		FFL_NOSPAWN},
	{"idle",				FOFS(monsterinfo.idle),			F_FUNCTION,		FFL_NOSPAWN},
	{"search",				FOFS(monsterinfo.search),		F_FUNCTION,		FFL_NOSPAWN},
	{"walk",				FOFS(monsterinfo.walk),			F_FUNCTION,		FFL_NOSPAWN},
	{"run",					FOFS(monsterinfo.run),			F_FUNCTION,		FFL_NOSPAWN},
	{"dodge",				FOFS(monsterinfo.dodge),		F_FUNCTION,		FFL_NOSPAWN},
	{"attack",				FOFS(monsterinfo.attack),		F_FUNCTION,		FFL_NOSPAWN},
	{"melee",				FOFS(monsterinfo.melee),		F_FUNCTION,		FFL_NOSPAWN},
	{"sight",				FOFS(monsterinfo.sight),		F_FUNCTION,		FFL_NOSPAWN},
	{"checkattack",			FOFS(monsterinfo.checkattack),	F_FUNCTION,		FFL_NOSPAWN},
	{"currentmove",			FOFS(monsterinfo.currentmove),	F_MMOVE,		FFL_NOSPAWN},
	{"endfunc",				FOFS(moveInfo.endfunc),			F_FUNCTION,		FFL_NOSPAWN},
	// Temp spawn vars -- only valid when the spawn function is called
	{"lip",					STOFS(lip),						F_INT,			FFL_SPAWNTEMP},
	{"distance",			STOFS(distance),				F_INT,			FFL_SPAWNTEMP},
	{"height",				STOFS(height),					F_INT,			FFL_SPAWNTEMP},
	{"noise",				STOFS(noise),					F_LSTRING,		FFL_SPAWNTEMP},
	{"pausetime",			STOFS(pausetime),				F_FLOAT,		FFL_SPAWNTEMP},
	{"item",				STOFS(item),					F_LSTRING,		FFL_SPAWNTEMP},
	// Need for item field in edict struct, FFL_SPAWNTEMP item will be skipped on saves
	{"item",				FOFS(item),						F_ITEM},
	{"gravity",				STOFS(gravity),					F_LSTRING,		FFL_SPAWNTEMP},
	{"sky",					STOFS(sky),						F_LSTRING,		FFL_SPAWNTEMP},
	{"skyrotate",			STOFS(skyrotate),				F_FLOAT,		FFL_SPAWNTEMP},
	{"skyaxis",				STOFS(skyaxis),					F_VECTOR,		FFL_SPAWNTEMP},
	{"minyaw",				STOFS(minyaw),					F_FLOAT,		FFL_SPAWNTEMP},
	{"maxyaw",				STOFS(maxyaw),					F_FLOAT,		FFL_SPAWNTEMP},
	{"minpitch",			STOFS(minpitch),				F_FLOAT,		FFL_SPAWNTEMP},
	{"maxpitch",			STOFS(maxpitch),				F_FLOAT,		FFL_SPAWNTEMP},
	{"nextmap",				STOFS(nextmap),					F_LSTRING,		FFL_SPAWNTEMP},
	{0,						0,								(fieldType_t)0,	0}
};

static field_t			sg_levelFields[] = {
	{"changemap",			LLOFS(changemap),				F_LSTRING},                  
	{"sight_client",		LLOFS(sight_client),			F_EDICT},
	{"sight_entity",		LLOFS(sight_entity),			F_EDICT},
	{"sound_entity",		LLOFS(sound_entity),			F_EDICT},
	{"sound2_entity",		LLOFS(sound2_entity),			F_EDICT},
	{NULL,					0,								F_INT}
};

static field_t			sg_clientFields[] = {
	{"pers.weapon",			CLOFS(pers.weapon),				F_ITEM},
	{"pers.lastweapon",		CLOFS(pers.lastweapon),			F_ITEM},
	{"newweapon",			CLOFS(newweapon),				F_ITEM},
	{NULL,					0,								F_INT}
};

static mmove_t			mmove_reloc;


/*
 ==============================================================================

 FIELD WRITE/READ

 ==============================================================================
*/


/*
 ==================
 SG_WriteField1
 ==================
*/
static void SG_WriteField1 (FILE *f, field_t *field, byte *base){

	void	*p;
	int		length;
	int		index;

	if (field->flags & FFL_SPAWNTEMP)
		return;

	p = (void *)(base + field->ofs);

	switch (field->type){
	case F_INT:
	case F_FLOAT:
	case F_ANGLEHACK:
	case F_VECTOR:
	case F_IGNORE:
		break;
	case F_LSTRING:
	case F_GSTRING:
		if ( *(char **)p )
			length = strlen(*(char **)p) + 1;
		else
			length = 0;

		*(int *)p = length;
		break;
	case F_EDICT:
		if ( *(edict_t **)p == NULL)
			index = -1;
		else
			index = *(edict_t **)p - g_edicts;

		*(int *)p = index;
		break;
	case F_CLIENT:
		if ( *(gclient_t **)p == NULL)
			index = -1;
		else
			index = *(gclient_t **)p - game.clients;

		*(int *)p = index;
		break;
	case F_ITEM:
		if ( *(edict_t **)p == NULL)
			index = -1;
		else
			index = *(gItem_t **)p - sg_itemList;

		*(int *)p = index;
		break;
	case F_FUNCTION:	// Relative to code segment
		if (*(byte **)p == NULL)
			index = 0;
		else
			index = *(byte **)p - ((byte *)SG_InitGame);

		*(int *)p = index;
		break;
	case F_MMOVE:	// Relative to data segment
		if (*(byte **)p == NULL)
			index = 0;
		else
			index = *(byte **)p - (byte *)&mmove_reloc;

		*(int *)p = index;
		break;
	default:
		gi.error("SG_WriteField1: unknown field type");
	}
}

/*
 ==================
 SG_WriteField2
 ==================
*/
static void SG_WriteField2 (FILE *f, field_t *field, byte *base){

	void	*p;
	int		length;

	if (field->flags & FFL_SPAWNTEMP)
		return;

	p = (void *)(base + field->ofs);

	switch (field->type){
	case F_LSTRING:
		if (*(char **)p){
			length = strlen(*(char **)p) + 1;
			fwrite(*(char **)p, length, 1, f);
		}
		break;
	}
}

/*
 ==================
 SG_ReadField
 ==================
*/
static void SG_ReadField (FILE *f, field_t *field, byte *base){

	void	*p;
	int		length;
	int		index;

	if (field->flags & FFL_SPAWNTEMP)
		return;

	p = (void *)(base + field->ofs);

	switch (field->type){
	case F_INT:
	case F_FLOAT:
	case F_ANGLEHACK:
	case F_VECTOR:
	case F_IGNORE:
		break;
	case F_LSTRING:
		length = *(int *)p;

		if (!length)
			*(char **)p = NULL;
		else {
			*(char **)p = (char *)gi.TagMalloc(length, TAG_LEVEL);
			fread (*(char **)p, length, 1, f);
		}
		break;
	case F_EDICT:
		index = *(int *)p;

		if (index == -1)
			*(edict_t **)p = NULL;
		else
			*(edict_t **)p = &g_edicts[index];
		break;
	case F_CLIENT:
		index = *(int *)p;

		if (index == -1)
			*(gclient_t **)p = NULL;
		else
			*(gclient_t **)p = &game.clients[index];
		break;
	case F_ITEM:
		index = *(int *)p;

		if (index == -1)
			*(gItem_t **)p = NULL;
		else
			*(gItem_t **)p = &sg_itemList[index];
		break;
	case F_FUNCTION:	// Relative to code segment
		index = *(int *)p;

		if (index == 0)
			*(byte **)p = NULL;
		else
			*(byte **)p = ((byte *)SG_InitGame) + index;
		break;
	case F_MMOVE:	// Relative to data segment
		index = *(int *)p;

		if (index == 0)
			*(byte **)p = NULL;
		else
			*(byte **)p = (byte *)&mmove_reloc + index;
		break;
	default:
		gi.error("SG_ReadField: unknown field type");
	}
}


/*
 ==============================================================================

 CLIENT WRITE/READ

 ==============================================================================
*/


/*
 ==================
 SG_WriteClient

 All pointer variables (except function pointers) must be handled specially
 ==================
*/
static void SG_WriteClient (FILE *f, gclient_t *client){

	gclient_t	temp;
	field_t		*field;
	
	// All of the ints, floats, and vectors stay as they are
	temp = *client;

	// Change the pointers to lengths or indicies
	for (field = sg_clientFields; field->name; field++)
		SG_WriteField1(f, field, (byte *)&temp);

	// Write the block
	fwrite(&temp, sizeof(temp), 1, f);

	// Now write any allocated data following the edict
	for (field = sg_clientFields; field->name; field++)
		SG_WriteField2(f, field, (byte *)client);
}

/*
 ==================
 SG_ReadClient

 All pointer variables (except function pointers) must be handled specially.
 ==================
*/
static void SG_ReadClient (FILE *f, gclient_t *client){

	field_t	*field;

	fread(client, sizeof(*client), 1, f);

	for (field = sg_clientFields; field->name; field++)
		SG_ReadField(f, field, (byte *)client);
}


/*
 ==============================================================================

 SAVEGAME WRITE/READ

 ==============================================================================
*/


/*
 ==================
 SG_WriteGame

 This will be called whenever the game goes to a new level,
 and when the user explicitly saves the game.

 Game information include cross level data, like multi level
 triggers, help computer info, and all client states.

 A single player death will automatically restore from the
 last save position.
 ==================
*/
void SG_WriteGame (char *name, qboolean autoSave){

	FILE	*f;
	char	string[16];
	int		i;

	if (!autoSave)
		SG_SaveClientData();

	f = fopen(name, "wb");
	if (!f)
		gi.error("Couldn't open %s", name);

	memset(string, 0, sizeof(string));

	strcpy(string, __DATE__);
	fwrite(string, sizeof(string), 1, f);

	game.autosaved = autoSave;
	fwrite(&game, sizeof(game), 1, f);
	game.autosaved = false;

	for (i = 0; i < game.maxclients; i++)
		SG_WriteClient(f, &game.clients[i]);

	fclose(f);
}

/*
 ==================
 SG_ReadGame
 ==================
*/
void SG_ReadGame (char *name){

	FILE	*f;
	char	string[16];
	int		i;

	gi.FreeTags(TAG_GAME);

	f = fopen(name, "rb");
	if (!f)
		gi.error("Couldn't open %s", name);

	fread(string, sizeof(string), 1, f);

	if (strcmp(string, __DATE__)){
		fclose(f);
		gi.error("Savegame from an older version.\n");
	}

	g_edicts =  (edict_t *)gi.TagMalloc(game.maxentities * sizeof(g_edicts[0]), TAG_GAME);
	globals.edicts = g_edicts;

	fread(&game, sizeof(game), 1, f);

	game.clients = (gclient_t *)gi.TagMalloc(game.maxclients * sizeof(game.clients[0]), TAG_GAME);

	for (i = 0; i < game.maxclients; i++)
		SG_ReadClient(f, &game.clients[i]);

	fclose(f);
}


/*
 ==============================================================================

 EDICT AND LEVELOCALS WRITE/READ

 ==============================================================================
*/


/*
 ==================
 SG_WriteEdict

 All pointer variables (except function pointers) must be handled specially.
 ==================
*/
static void SG_WriteEdict (FILE *f, edict_t *entity){

	edict_t	temp;
	field_t	*field;

	// All of the ints, floats, and vectors stay as they are
	temp = *entity;

	// Change the pointers to lengths or indexes
	for (field = fields; field->name; field++)
		SG_WriteField1(f, field, (byte *)&temp);

	// Write the block
	fwrite (&temp, sizeof(temp), 1, f);

	// Now write any allocated data following the edict
	for (field = fields; field->name; field++)
		SG_WriteField2(f, field, (byte *)entity);
}

/*
 ==================
 SG_WriteLevelLocals

 All pointer variables (except function pointers) must be handled specially.
 ==================
*/
static void SG_WriteLevelLocals (FILE *f){

	levelLocals_t	temp;
	field_t			*field;

	// All of the ints, floats, and vectors stay as they are
	temp = level;

	// Change the pointers to lengths or indexes
	for (field = sg_levelFields; field->name; field++)
		SG_WriteField1(f, field, (byte *)&temp);

	// Write the block
	fwrite(&temp, sizeof(temp), 1, f);

	// Now write any allocated data following the edict
	for (field = sg_levelFields; field->name; field++)
		SG_WriteField2(f, field, (byte *)&level);
}

/*
 ==================
 SG_ReadEdict

 All pointer variables (except function pointers) must be handled specially.
 ==================
*/
static void SG_ReadEdict (FILE *f, edict_t *entity){

	field_t	*field;

	fread(entity, sizeof(*entity), 1, f);

	for (field = fields; field->name; field++)
		SG_ReadField(f, field, (byte *)entity);
}

/*
 ==================
 SG_ReadLevelLocals

 All pointer variables (except function pointers) must be handled specially.
 ==================
*/
static void SG_ReadLevelLocals (FILE *f){

	field_t	*field;

	fread(&level, sizeof(level), 1, f);

	for (field = sg_levelFields; field->name; field++)
		SG_ReadField(f, field, (byte *)&level);
}

/*
 ==================
 SG_WriteLevel
 ==================
*/
void SG_WriteLevel (char *name){

	FILE	*f;
	int		i;
	void	*base;
	edict_t	*entity;

	f = fopen(name, "wb");
	if (!f)
		gi.error("Couldn't open %s", name);

	// Write out edict size for checking
	i = sizeof(edict_t);
	fwrite(&i, sizeof(i), 1, f);

	// Write out a function pointer for checking
	base = (void *)SG_InitGame;
	fwrite(&base, sizeof(base), 1, f);

	// Write out levelLocals_t
	SG_WriteLevelLocals(f);

	// Write out all the entities
	for (i = 0; i < globals.num_edicts; i++){
		entity = &g_edicts[i];

		if (!entity->inuse)
			continue;

		fwrite(&i, sizeof(i), 1, f);
		SG_WriteEdict(f, entity);
	}

	i = -1;
	fwrite(&i, sizeof(i), 1, f);

	fclose(f);
}

/*
 ==================
 SG_ReadLevel

 SG_SpawnEntities will allready have been called on the
 level the same way it was when the level was saved.

 That is necessary to get the baselines
 set up identically.

 The server will have cleared all of the world links before
 calling ReadLevel.

 No clients are connected yet.
 ==================
*/
void SG_ReadLevel (char *name){

	FILE	*f;
	int		i;
	void	*base;
	int		entityNum;
	edict_t	*entity;

	f = fopen (name, "rb");
	if (!f)
		gi.error("Couldn't open %s", name);

	// Free any dynamic memory allocated by loading the level
	// base state
	gi.FreeTags(TAG_LEVEL);

	// Wipe all the entities
	memset(g_edicts, 0, game.maxentities * sizeof(g_edicts[0]));
	globals.num_edicts = maxclients->value + 1;

	// Check edict size
	fread(&i, sizeof(i), 1, f);

	if (i != sizeof(edict_t)){
		fclose(f);
		gi.error("SG_ReadLevel: mismatched edict size");
	}

	// Check function pointer base address
	fread(&base, sizeof(base), 1, f);
#ifdef _WIN32
	if (base != (void *)SG_InitGame){
		fclose(f);
		gi.error("ReadLevel: function pointers have moved");
	}
#else
	gi.dprintf("Function offsets %d\n", ((byte *)base) - ((byte *)SG_InitGame));
#endif

	// Load the level locals
	SG_ReadLevelLocals(f);

	// Load all the entities
	while (1){
		if (fread(&entityNum, sizeof(entityNum), 1, f) != 1){
			fclose(f);
			gi.error("ReadLevel: failed to read entnum");
		}

		if (entityNum == -1)
			break;
		if (entityNum >= globals.num_edicts)
			globals.num_edicts = entityNum + 1;

		entity = &g_edicts[entityNum];
		SG_ReadEdict(f, entity);

		// Let the server rebuild world links for this ent
		memset(&entity->area, 0, sizeof(entity->area));

		gi.linkentity(entity);
	}

	fclose(f);

	// Mark all clients as unconnected
	for (i = 0; i < maxclients->value; i++){
		entity = &g_edicts[i + 1];

		entity->client = game.clients + i;
		entity->client->pers.connected = false;
	}

	// Do any load time things at this point
	for (i = 0; i < globals.num_edicts; i++){
		entity = &g_edicts[i];

		if (!entity->inuse)
			continue;

		// Fire any cross-level triggers
		if (entity->className){
			if (!strcmp(entity->className, "target_crosslevel_target"))
				entity->nextthink = level.time + entity->delay;
		}
	}
}