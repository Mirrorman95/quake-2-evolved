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
// sg_clientHUD.c - Client HUD interface layout and updating
//

// TODO:
// - SG_DeathmatchScoreboardLayout, SG_GetMonsterStats
// - Move SG_MoveClientToIntermission into sg_client?
// - Move commands into their own file? (sg_cmds.c)


#include "g_local.h"



/*
 ==============================================================================

 INTERMISSION

 ==============================================================================
*/


/*
 ==================
 SG_MoveClientToIntermission
 ==================
*/
void SG_MoveClientToIntermission (edict_t *entity){

	if (deathmatch->value || coop->value)
		entity->client->showscores = true;

	VectorCopy(level.intermission_origin, entity->s.origin);
	entity->client->ps.pmove.origin[0] = level.intermission_origin[0] * 8.0f;
	entity->client->ps.pmove.origin[1] = level.intermission_origin[1] * 8.0f;
	entity->client->ps.pmove.origin[2] = level.intermission_origin[2] * 8.0f;

	VectorCopy(level.intermission_angle, entity->client->ps.viewangles);
	entity->client->ps.pmove.pm_type = PM_FREEZE;
	entity->client->ps.gunindex = 0;
	entity->client->ps.blend[3] = 0.0f;
	entity->client->ps.rdflags &= ~RDF_UNDERWATER;

	// Clean up powerup info
	entity->client->quad_framenum = 0.0f;
	entity->client->invincible_framenum = 0.0f;
	entity->client->breather_framenum = 0.0f;
	entity->client->enviro_framenum = 0.0f;
	entity->client->grenade_blew_up = false;
	entity->client->grenade_time = 0.0f;

	entity->viewheight = 0;
	entity->s.modelindex = 0;
	entity->s.modelindex2 = 0;
	entity->s.modelindex3 = 0;
	entity->s.modelindex = 0;
	entity->s.effects = 0;
	entity->s.sound = 0;
	entity->solid = SOLID_NOT;

	// Add the layout
	if (deathmatch->value || coop->value){
		SG_DeathmatchScoreboardLayout(entity, NULL);
		gi.unicast(entity, true);
	}
}

/*
 ==================
 SG_BeginIntermission
 ==================
*/
void SG_BeginIntermission (edict_t *target){

	int		i, n;
	edict_t	*entity, *client;

	if (level.intermissiontime)
		return;		// Already activated

	game.autosaved = false;

	// Respawn any dead clients
	for (i = 0; i < maxclients->value; i++){
		client = g_edicts + 1 + i;

		if (!client->inuse)
			continue;
		if (client->health <= 0)
			SG_ClientRespawn(client);
	}

	level.intermissiontime = level.time;
	level.changemap = target->map;

	if (strstr(level.changemap, "*")){
		if (coop->value){
			for (i = 0; i < maxclients->value; i++){
				client = g_edicts + 1 + i;

				if (!client->inuse)
					continue;

				// Strip players of all keys between units
				for (n = 0; n < MAX_ITEMS; n++){
					if (sg_itemList[n].flags & IT_KEY)
						client->client->pers.inventory[n] = 0;
				}
			}
		}
	}
	else {
		if (!deathmatch->value){
			// Go immediately to the next level
			level.exitintermission = 1;
			return;
		}
	}

	level.exitintermission = 0;

	// find an intermission spot
	entity = SG_FindEntity(NULL, FOFS(className), "info_player_intermission");

	if (!entity){
		// The map creator forgot to put in an intermission point...
		entity = SG_FindEntity(NULL, FOFS(className), "info_player_start");
		if (!entity)
			entity = SG_FindEntity(NULL, FOFS(className), "info_player_deathmatch");
	}
	else {
		// Chose one of four spots
		i = rand() & 3;

		while (i--){
			entity = SG_FindEntity(entity, FOFS(className), "info_player_intermission");	
			if (!entity)	// Wrap around the list
				entity = SG_FindEntity(entity, FOFS(className), "info_player_intermission");
		}
	}

	VectorCopy(entity->s.origin, level.intermission_origin);
	VectorCopy(entity->s.angles, level.intermission_angle);

	// Move all clients to the intermission point
	for (i = 0; i < maxclients->value; i++){
		client = g_edicts + 1 + i;

		if (!client->inuse)
			continue;

		SG_MoveClientToIntermission(client);
	}
}


/*
 ==============================================================================

 SCOREBOARD

 ==============================================================================
*/


/*
 ==================
 SG_DeathmatchScoreboardLayout

 FIXME: picNum is dead code
 ==================
*/
void SG_DeathmatchScoreboardLayout (edict_t *entity, edict_t *killer){

	char		entry[1024];
	char		string[1400];
	int			length;
	int			i, j, k;
	int			sorted[MAX_CLIENTS];
	int			sortedScores[MAX_CLIENTS];
	int			score, total;
	int			picNum;
	int			x, y;
	gclient_t	*client;
	edict_t		*clientEntity;
	char		*dogTag;

	// Sort the clients by score
	total = 0;

	for (i = 0; i < game.maxclients; i++){
		clientEntity = g_edicts + 1 + i;

		if (!clientEntity->inuse || game.clients[i].resp.spectator)
			continue;

		score = game.clients[i].resp.score;

		for (j = 0; j < total; j++){
			if (score > sortedScores[j])
				break;
		}

		for (k = total; k > j; k--){
			sorted[k] = sorted[k - 1];
			sortedScores[k] = sortedScores[k - 1];
		}

		sorted[j] = i;
		sortedScores[j] = score;

		total++;
	}

	// Print level name and exit rules
	string[0] = 0;

	length = strlen(string);

	// Add the clients in sorted order
	if (total > 12)
		total = 12;

	for (i = 0; i < total; i++){
		client = &game.clients[sorted[i]];
		clientEntity = g_edicts + 1 + sorted[i];

		picNum = gi.imageindex("i_fixme");

		x = (i >= 6) ? 160 : 0;
		y = 32 + 32 * (i % 6);

		// Add a dogtag
		if (clientEntity == entity)
			dogTag = "tag1";
		else if (clientEntity == killer)
			dogTag = "tag2";
		else
			dogTag = NULL;

		if (dogTag){
			Com_sprintf(entry, sizeof(entry), "xv %i yv %i picn %s ", x + 32, y, dogTag);

			j = strlen(entry);

			if (length + j > 1024)
				break;

			strcpy(string + length, entry);
			length += j;
		}

		// Send the layout
		Com_sprintf(entry, sizeof(entry), "client %i %i %i %i %i %i ", x, y, sorted[i], client->resp.score, client->ping, (level.framenum - client->resp.enterframe) / 600);

		j = strlen(entry);

		if (length + j > 1024)
			break;

		strcpy(string + length, entry);
		length += j;
	}

	gi.WriteByte(svc_layout);
	gi.WriteString(string);
}

/*
 ==================
 SG_DeathmatchScoreboard

 Draw instead of help message.
 Note that it isn't that hard to overflow the 1400 byte message limit!
 ==================
*/
static void SG_DeathmatchScoreboard (edict_t *entity){

	SG_DeathmatchScoreboardLayout(entity, entity->enemy);
	gi.unicast(entity, true);
}

/*
 ==================
 SG_Score_f

 Display the scoreboard
 ==================
*/
void SG_Score_f (edict_t *entity){

	entity->client->showinventory = false;
	entity->client->showhelp = false;

	if (!deathmatch->value && !coop->value)
		return;

	if (entity->client->showscores){
		entity->client->showscores = false;
		return;
	}

	entity->client->showscores = true;
	SG_DeathmatchScoreboard(entity);
}


/*
 ==============================================================================

 HELP COMPUTER

 ==============================================================================
*/


/*
 ==================
 SG_HelpComputerLayout

 Draw help computer
 ==================
*/
static void SG_HelpComputerLayout (edict_t *entity){

	char	*sk;
	char	string[1024];

	if (skill->value == 0)
		sk = "easy";
	else if (skill->value == 1)
		sk = "medium";
	else if (skill->value == 2)
		sk = "hard";
	else
		sk = "hard+";

	// Send the layout
	Com_sprintf(string, sizeof(string),
		"xv 32 yv 8 picn help "				// background
		"xv 202 yv 12 string2 \"%s\" "		// skill
		"xv 0 yv 24 cstring2 \"%s\" "		// level name
		"xv 0 yv 54 cstring2 \"%s\" "		// help 1
		"xv 0 yv 110 cstring2 \"%s\" "		// help 2
		"xv 50 yv 164 string2 \" kills     goals    secrets\" "
		"xv 50 yv 172 string2 \"%3i/%3i     %i/%i       %i/%i\" ", 
		sk,
		level.level_name,
		game.helpmessage1,
		game.helpmessage2,
		level.killed_monsters, level.total_monsters, 
		level.found_goals, level.total_goals,
		level.found_secrets, level.total_secrets);

	gi.WriteByte(svc_layout);
	gi.WriteString(string);
	gi.unicast(entity, true);
}

/*
 ==================
 SG_Help_f

 Display the current help message
 ==================
*/
void SG_Help_f (edict_t *entity){

	// This is for backwards compatability
	if (deathmatch->value){
		SG_Score_f(entity);
		return;
	}

	entity->client->showinventory = false;
	entity->client->showscores = false;

	if (entity->client->showhelp && (entity->client->pers.game_helpchanged == game.helpchanged)){
		entity->client->showhelp = false;
		return;
	}

	entity->client->showhelp = true;
	entity->client->pers.helpchanged = 0;

	SG_HelpComputerLayout(entity);
}


/*
 ==============================================================================

 STATISTICS

 ==============================================================================
*/


/*
 ==================
 SG_SetStats
 ==================
*/
void SG_SetStats (edict_t *entity){

	gItem_t	*item;
	int		index, cells;
	int		powerArmorType;

	// Health
	entity->client->ps.stats[STAT_HEALTH_ICON] = level.pic_health;
	entity->client->ps.stats[STAT_HEALTH] = entity->health;

	// Ammo
	if (!entity->client->ammo_index){
		entity->client->ps.stats[STAT_AMMO_ICON] = 0;
		entity->client->ps.stats[STAT_AMMO] = 0;
	}
	else {
		item = &sg_itemList[entity->client->ammo_index];
		entity->client->ps.stats[STAT_AMMO_ICON] = gi.imageindex(item->icon);
		entity->client->ps.stats[STAT_AMMO] = entity->client->pers.inventory[entity->client->ammo_index];
	}

	// Armor
	powerArmorType = SG_FindPowerArmorByType(entity);

	if (powerArmorType){
		cells = entity->client->pers.inventory[ITEM_INDEX(SG_FindItem("cells"))];

		// Ran out of cells for the power armor
		if (cells == 0){
			entity->flags &= ~FL_POWER_ARMOR;
			gi.sound(entity, CHAN_ITEM, gi.soundindex("misc/power2.wav"), 1.0f, ATTN_NORM, 0.0f);
			powerArmorType = 0;;
		}
	}

	index = SG_FindArmorByIndex(entity);

	// Flash between power armor and other armor icon
	if (powerArmorType && (!index || (level.framenum & 8))){
		entity->client->ps.stats[STAT_ARMOR_ICON] = gi.imageindex("i_powershield");
		entity->client->ps.stats[STAT_ARMOR] = cells;
	}
	else if (index){
		item = SG_GetItemByIndex(index);
		entity->client->ps.stats[STAT_ARMOR_ICON] = gi.imageindex(item->icon);
		entity->client->ps.stats[STAT_ARMOR] = entity->client->pers.inventory[index];
	}
	else {
		entity->client->ps.stats[STAT_ARMOR_ICON] = 0;
		entity->client->ps.stats[STAT_ARMOR] = 0;
	}

	// Pickup message
	if (level.time > entity->client->pickup_msg_time){
		entity->client->ps.stats[STAT_PICKUP_ICON] = 0;
		entity->client->ps.stats[STAT_PICKUP_STRING] = 0;
	}

	// Timers
	if (entity->client->quad_framenum > level.framenum){
		entity->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex("p_quad");
		entity->client->ps.stats[STAT_TIMER] = (entity->client->quad_framenum - level.framenum) / 10;
	}
	else if (entity->client->invincible_framenum > level.framenum){
		entity->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex("p_invulnerability");
		entity->client->ps.stats[STAT_TIMER] = (entity->client->invincible_framenum - level.framenum) / 10;
	}
	else if (entity->client->enviro_framenum > level.framenum){
		entity->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex("p_envirosuit");
		entity->client->ps.stats[STAT_TIMER] = (entity->client->enviro_framenum - level.framenum) / 10;
	}
	else if (entity->client->breather_framenum > level.framenum){
		entity->client->ps.stats[STAT_TIMER_ICON] = gi.imageindex("p_rebreather");
		entity->client->ps.stats[STAT_TIMER] = (entity->client->breather_framenum - level.framenum) / 10;
	}
	else {
		entity->client->ps.stats[STAT_TIMER_ICON] = 0;
		entity->client->ps.stats[STAT_TIMER] = 0;
	}

	// Selected item
	if (entity->client->pers.selected_item == -1)
		entity->client->ps.stats[STAT_SELECTED_ICON] = 0;
	else
		entity->client->ps.stats[STAT_SELECTED_ICON] = gi.imageindex(sg_itemList[entity->client->pers.selected_item].icon);

	entity->client->ps.stats[STAT_SELECTED_ITEM] = entity->client->pers.selected_item;

	// Layouts
	entity->client->ps.stats[STAT_LAYOUTS] = 0;

	if (deathmatch->value){
		if (entity->client->pers.health <= 0 || level.intermissiontime || entity->client->showscores)
			entity->client->ps.stats[STAT_LAYOUTS] |= 1;
		if (entity->client->showinventory && entity->client->pers.health > 0)
			entity->client->ps.stats[STAT_LAYOUTS] |= 2;
	}
	else {
		if (entity->client->showscores || entity->client->showhelp)
			entity->client->ps.stats[STAT_LAYOUTS] |= 1;
		if (entity->client->showinventory && entity->client->pers.health > 0)
			entity->client->ps.stats[STAT_LAYOUTS] |= 2;
	}

	// Frags
	entity->client->ps.stats[STAT_FRAGS] = entity->client->resp.score;

	// Help icon / current weapon if not shown
	if (entity->client->pers.helpchanged && (level.framenum & 8))
		entity->client->ps.stats[STAT_HELPICON] = gi.imageindex("i_help");
	else if ((entity->client->pers.hand == CENTER_HANDED || entity->client->ps.fov > 91.0f) && entity->client->pers.weapon)
		entity->client->ps.stats[STAT_HELPICON] = gi.imageindex(entity->client->pers.weapon->icon);
	else
		entity->client->ps.stats[STAT_HELPICON] = 0;

	entity->client->ps.stats[STAT_SPECTATOR] = 0;
}

/*
 ==================
 SG_CheckChaseStats
 ==================
*/
void SG_CheckChaseStats (edict_t *entity){

	int			i;
	gclient_t	*client;

	for (i = 1; i <= maxclients->value; i++){
		client = g_edicts[i].client;

		if (!g_edicts[i].inuse || client->chaseTarget != entity)
			continue;

		memcpy(client->ps.stats, entity->client->ps.stats, sizeof(client->ps.stats));

		SG_SetSpectatorStats(g_edicts + i);
	}
}

/*
 ==================
 SG_GetMonsterStats
 ==================
*/
static void SG_GetMonsterStats (edict_t *entity){

	edict_t *monster = entity->client->chaseTarget;

	if (monster && monster->inuse){
		entity->client->ps.stats[STAT_AMMO] = 0;
		entity->client->ps.stats[STAT_AMMO_ICON] = 0;
		entity->client->ps.stats[STAT_ARMOR] = 0;
		entity->client->ps.stats[STAT_ARMOR_ICON] = ARMOR_INDEX;
//		entity->client->ps.stats[STAT_FRAGS] = monster->character->kills;
		entity->client->ps.stats[STAT_HEALTH] = monster->health;
		entity->client->ps.stats[STAT_HEALTH_ICON] = level.pic_health;
		entity->client->ps.stats[STAT_PICKUP_ICON] = 0;
		entity->client->ps.stats[STAT_PICKUP_STRING] = 0;
		entity->client->ps.stats[STAT_SELECTED_ICON] = 0;
		entity->client->ps.stats[STAT_SELECTED_ITEM] = 0;
		entity->client->ps.stats[STAT_TIMER] = 0;
		entity->client->ps.stats[STAT_TIMER_ICON] = 0;
		entity->client->ps.stats[STAT_CHASE] = CS_MODELS + monster->s.modelindex;
	}
}

/*
 ==================
 SG_SetSpectatorStats
 ==================
*/
void SG_SetSpectatorStats (edict_t *entity){

	gclient_t	*client = entity->client;

	if (!client->chaseTarget)
		SG_SetStats(entity);

	client->ps.stats[STAT_SPECTATOR] = 1;

	// Layouts are independant in spectator
	client->ps.stats[STAT_LAYOUTS] = 0;

	if (client->pers.health <= 0 || level.intermissiontime || client->showscores)
		client->ps.stats[STAT_LAYOUTS] |= 1;
	if (client->showinventory && client->pers.health > 0)
		client->ps.stats[STAT_LAYOUTS] |= 2;

	if (client->chaseTarget && client->chaseTarget->inuse)
		client->ps.stats[STAT_CHASE] = CS_PLAYERSKINS + (client->chaseTarget - g_edicts) - 1;
	else
		client->ps.stats[STAT_CHASE] = 0;

	// Show stats for monsters when in spectator chasing mode
	SG_GetMonsterStats(entity);
}