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
// sg_client.c - Client back-end
//


#include "g_local.h"
#include "m_player.h"


void SG_MiscTeleporterDest_Spawn (edict_t *ent);
static void SG_PutClientInServer (edict_t *entity);

static edict_t *		pm_passent;


/*
 ==================
 SG_PlayerNoise

 Each player can have two noise objects associated with it:
 a personal noise (jumping, pain, weapon firing), and a weapon
 target noise (bullet wall impacts)

 Monsters that don't directly see the player can move
 to a noise in hopes of seeing the player from there.
 ==================
*/
void SG_PlayerNoise (edict_t *who, vec3_t where, int type){

	edict_t	*noise;

	if (type == PNOISE_WEAPON){
		if (who->client->silencer_shots){
			who->client->silencer_shots--;
			return;
		}
	}

	if (deathmatch->value)
		return;

	if (who->flags & FL_NOTARGET)
		return;

	if (!who->mynoise){
		noise = SG_AllocEntity();
		noise->className = "player_noise";
		VectorSet(noise->mins, -8.0f, -8.0f, -8.0f);
		VectorSet(noise->maxs, 8.0f, 8.0f, 8.0f);
		noise->owner = who;
		noise->svflags = SVF_NOCLIENT;
		who->mynoise = noise;

		noise = SG_AllocEntity();
		noise->className = "player_noise";
		VectorSet(noise->mins, -8.0f, -8.0f, -8.0f);
		VectorSet(noise->maxs, 8.0f, 8.0f, 8.0f);
		noise->owner = who;
		noise->svflags = SVF_NOCLIENT;
		who->mynoise2 = noise;
	}

	if (type == PNOISE_SELF || type == PNOISE_WEAPON){
		noise = who->mynoise;
		level.sound_entity = noise;
		level.sound_entity_framenum = level.framenum;
	}
	else {
		noise = who->mynoise2;
		level.sound2_entity = noise;
		level.sound2_entity_framenum = level.framenum;
	}

	VectorCopy(where, noise->s.origin);
	VectorSubtract(where, noise->maxs, noise->absmin);
	VectorAdd(where, noise->maxs, noise->absmax);

	noise->teleportTime = level.time;

	gi.linkentity(noise);
}


/*
 ==============================================================================

 COOP SPOT LOCATION HACK

 This function is an ugly as hell hack to fix some map flaws

 The coop spawn spots on some maps are SNAFU.  There are coop spots
 with the wrong targetname as well as spots with no name at all

 We use carnal knowledge of the maps to fix the coop spot targetnames to match
 that of the nearest named single player spot

 ==============================================================================
*/


/*
 ==================
 SP_FixCoopSpots
 ==================
*/
static void SP_FixCoopSpots (edict_t *self){

	edict_t	*spot;
	vec3_t	d;

	spot = NULL;

	while (1){
		spot = SG_FindEntity(spot, FOFS(className), "info_player_start");

		if (!spot)
			return;
		if (!spot->targetname)
			continue;

		VectorSubtract(self->s.origin, spot->s.origin, d);

		if (VectorLength(d) < 384.0f){
			if ((!self->targetname) || Q_stricmp(self->targetname, spot->targetname) != 0)
				self->targetname = spot->targetname;

			return;
		}
	}
}

/*
 ==================
 SP_CreateCoopSpots

 Now if that one wasn't ugly enough for you then try this one on for size
 some maps don't have any coop spots at all, so we need to create them
 where they should have been
 ==================
*/
static void SP_CreateCoopSpots (edict_t *self){

	edict_t	*spot;

	if (!Q_stricmp(level.mapname, "security")){
		spot = SG_AllocEntity();
		spot->className = "info_player_coop";
		spot->s.origin[0] = 188.0f - 64.0f;
		spot->s.origin[1] = -164.0f;
		spot->s.origin[2] = 80.0f;
		spot->targetname = "jail3";
		spot->s.angles[1] = 90.0f;

		spot = SG_AllocEntity();
		spot->className = "info_player_coop";
		spot->s.origin[0] = 188.0f + 64.0f;
		spot->s.origin[1] = -164.0f;
		spot->s.origin[2] = 80.0f;
		spot->targetname = "jail3";
		spot->s.angles[1] = 90.0f;

		spot = SG_AllocEntity();
		spot->className = "info_player_coop";
		spot->s.origin[0] = 188.0f + 128.0f;
		spot->s.origin[1] = -164.0f;
		spot->s.origin[2] = 80.0f;
		spot->targetname = "jail3";
		spot->s.angles[1] = 90.0f;

		return;
	}
}


/*
 ==============================================================================

 MAP EDITOR INFO FUNCTIONS

 ==============================================================================
*/


/*
 ==================
 SG_InfoPlayerStart_Spawn

 The normal starting point for a level
 ==================
*/
void SG_InfoPlayerStart_Spawn (edict_t *self){

	if (!coop->value)
		return;

	// Invoke one of our gross, ugly, disgusting hacks
	if (!Q_stricmp(level.mapname, "security")){
		self->think = SP_CreateCoopSpots;
		self->nextthink = level.time + FRAMETIME;
	}
}

/*
 ==================
 SG_InfoPlayerDeathmatch_Spawn

 Potential spawning position for deathmatch games
 ==================
*/
void SG_InfoPlayerDeathmatch_Spawn (edict_t *self){

	if (!deathmatch->value){
		SG_FreeEntity(self);
		return;
	}

	SG_MiscTeleporterDest_Spawn(self);
}

/*
 ==================
 SG_InfoPlayerCoop_Spawn

 Potential spawning position for coop games
 ==================
*/
void SG_InfoPlayerCoop_Spawn (edict_t *self){

	if (!coop->value){
		SG_FreeEntity(self);
		return;
	}

	// Invoke one of our gross, ugly, disgusting hacks
	if ((!Q_stricmp(level.mapname, "jail2"))   ||
		(!Q_stricmp(level.mapname, "jail4"))   ||
		(!Q_stricmp(level.mapname, "mine1"))   ||
		(!Q_stricmp(level.mapname, "mine2"))   ||
		(!Q_stricmp(level.mapname, "mine3"))   ||
		(!Q_stricmp(level.mapname, "mine4"))   ||
		(!Q_stricmp(level.mapname, "lab"))     ||
		(!Q_stricmp(level.mapname, "boss1"))   ||
		(!Q_stricmp(level.mapname, "fact3"))   ||
		(!Q_stricmp(level.mapname, "biggun"))  ||
		(!Q_stricmp(level.mapname, "space"))   ||
		(!Q_stricmp(level.mapname, "command")) ||
		(!Q_stricmp(level.mapname, "power2"))  ||
		(!Q_stricmp(level.mapname, "strike"))){
			self->think = SP_FixCoopSpots;
			self->nextthink = level.time + FRAMETIME;
	}
}

/*
 ==================
 SG_InfoPlayerIntermission_Spawn

 The deathmatch intermission point will be at one of these
 Use 'angles' instead of 'angle', so you can set pitch or roll as well as yaw.  'pitch yaw roll'
 ==================
*/
void SG_InfoPlayerIntermission_Spawn (){

}


/*
 ==============================================================================

 OBITUARY MESSAGES

 ==============================================================================
*/


/*
 ==================
 SG_IsClientFemale
 ==================
*/
static qboolean SG_IsClientFemale (edict_t *entity){

	char	*info;

	if (!entity->client)
		return false;

	info = Info_ValueForKey(entity->client->pers.userinfo, "gender");

	if (info[0] == 'f' || info[0] == 'F')
		return true;

	return false;
}

/*
 ==================
 SG_IsClientNeutral
 ==================
*/
static qboolean SG_IsClientNeutral (edict_t *entity){

	char	*info;

	if (!entity->client)
		return false;

	info = Info_ValueForKey(entity->client->pers.userinfo, "gender");

	if (info[0] != 'f' && info[0] != 'F' && info[0] != 'm' && info[0] != 'M')
		return true;

	return false;
}

/*
 ==================
 SG_ClientObituary
 ==================
*/
static void SG_ClientObituary (edict_t *self, edict_t *inflictor, edict_t *attacker){

	int			mod;
	char		*message;
	char		*message2;
	qboolean	friendlyFire;

	if (coop->value && attacker->client)
		meansOfDeath |= MOD_FRIENDLY_FIRE;

	if (deathmatch->value || coop->value){
		friendlyFire = meansOfDeath & MOD_FRIENDLY_FIRE;
		mod = meansOfDeath & ~MOD_FRIENDLY_FIRE;
		message = NULL;
		message2 = "";

		switch (mod){
		case MOD_SUICIDE:
			message = "suicides";
			break;
		case MOD_FALLING:
			message = "cratered";
			break;
		case MOD_CRUSH:
			message = "was squished";
			break;
		case MOD_WATER:
			message = "sank like a rock";
			break;
		case MOD_SLIME:
			message = "melted";
			break;
		case MOD_LAVA:
			message = "does a back flip into the lava";
			break;
		case MOD_EXPLOSIVE:
		case MOD_BARREL:
			message = "blew up";
			break;
		case MOD_EXIT:
			message = "found a way out";
			break;
		case MOD_TARGET_LASER:
			message = "saw the light";
			break;
		case MOD_TARGET_BLASTER:
			message = "got blasted";
			break;
		case MOD_BOMB:
		case MOD_SPLASH:
		case MOD_TRIGGER_HURT:
			message = "was in the wrong place";
			break;
		}

		if (attacker == self){
			switch (mod){
			case MOD_HELD_GRENADE:
				message = "tried to put the pin back in";
				break;
			case MOD_HG_SPLASH:
			case MOD_G_SPLASH:
				if (SG_IsClientNeutral(self))
					message = "tripped on its own grenade";
				else if (SG_IsClientFemale(self))
					message = "tripped on her own grenade";
				else
					message = "tripped on his own grenade";
				break;
			case MOD_R_SPLASH:
				if (SG_IsClientNeutral(self))
					message = "blew itself up";
				else if (SG_IsClientFemale(self))
					message = "blew herself up";
				else
					message = "blew himself up";
				break;
			case MOD_BFG_BLAST:
				message = "should have used a smaller gun";
				break;
			default:
				if (SG_IsClientNeutral(self))
					message = "killed itself";
				else if (SG_IsClientFemale(self))
					message = "killed herself";
				else
					message = "killed himself";
				break;
			}
		}

		if (message){
			gi.bprintf(PRINT_MEDIUM, "%s %s.\n", self->client->pers.netname, message);

			if (deathmatch->value)
				self->client->resp.score--;

			self->enemy = NULL;
			return;
		}

		self->enemy = attacker;

		if (attacker && attacker->client){
			switch (mod){
			case MOD_BLASTER:
				message = "was blasted by";
				break;
			case MOD_SHOTGUN:
				message = "was gunned down by";
				break;
			case MOD_SSHOTGUN:
				message = "was blown away by";
				message2 = "'s super shotgun";
				break;
			case MOD_MACHINEGUN:
				message = "was machinegunned by";
				break;
			case MOD_CHAINGUN:
				message = "was cut in half by";
				message2 = "'s chaingun";
				break;
			case MOD_GRENADE:
				message = "was popped by";
				message2 = "'s grenade";
				break;
			case MOD_G_SPLASH:
				message = "was shredded by";
				message2 = "'s shrapnel";
				break;
			case MOD_ROCKET:
				message = "ate";
				message2 = "'s rocket";
				break;
			case MOD_R_SPLASH:
				message = "almost dodged";
				message2 = "'s rocket";
				break;
			case MOD_HYPERBLASTER:
				message = "was melted by";
				message2 = "'s hyperblaster";
				break;
			case MOD_RAILGUN:
				message = "was railed by";
				break;
			case MOD_BFG_LASER:
				message = "saw the pretty lights from";
				message2 = "'s BFG";
				break;
			case MOD_BFG_BLAST:
				message = "was disintegrated by";
				message2 = "'s BFG blast";
				break;
			case MOD_BFG_EFFECT:
				message = "couldn't hide from";
				message2 = "'s BFG";
				break;
			case MOD_HANDGRENADE:
				message = "caught";
				message2 = "'s handgrenade";
				break;
			case MOD_HG_SPLASH:
				message = "didn't see";
				message2 = "'s handgrenade";
				break;
			case MOD_HELD_GRENADE:
				message = "feels";
				message2 = "'s pain";
				break;
			case MOD_TELEFRAG:
				message = "tried to invade";
				message2 = "'s personal space";
				break;
			}

			if (message){
				gi.bprintf(PRINT_MEDIUM,"%s %s %s%s\n", self->client->pers.netname, message, attacker->client->pers.netname, message2);
				
				if (deathmatch->value){
					if (friendlyFire)
						attacker->client->resp.score--;
					else
						attacker->client->resp.score++;
				}

				return;
			}
		}
	}

	gi.bprintf(PRINT_MEDIUM,"%s died.\n", self->client->pers.netname);

	if (deathmatch->value)
		self->client->resp.score--;
}


/*
 ==============================================================================

 DEATH EVENTS

 ==============================================================================
*/


/*
 ==================
 SG_TossClientWeapon
 ==================
*/
static void SG_TossClientWeapon (edict_t *self){

	gItem_t		*item;
	edict_t		*drop;
	qboolean	quad;
	float		spread;

	if (!deathmatch->value)
		return;

	item = self->client->pers.weapon;

	if (!self->client->pers.inventory[self->client->ammo_index])
		item = NULL;
	if (item && (!strcmp(item->pickupName, "Blaster")))
		item = NULL;

	if (!((int)(dmflags->value) & DF_QUAD_DROP))
		quad = false;
	else
		quad = (self->client->quad_framenum > (level.framenum + 10));

	if (item && quad)
		spread = 22.5f;
	else
		spread = 0.0f;

	if (item){
		self->client->v_angle[YAW] -= spread;
		drop = SG_Drop_Item (self, item);
		self->client->v_angle[YAW] += spread;
		drop->spawnflags = DROPPED_PLAYER_ITEM;
	}

	if (quad){
		self->client->v_angle[YAW] += spread;
		drop = SG_Drop_Item(self, SG_FindItemByClassname("item_quad"));
		self->client->v_angle[YAW] -= spread;
		drop->spawnflags |= DROPPED_PLAYER_ITEM;

		drop->touch = SG_Touch_Item;
		drop->nextthink = level.time + (self->client->quad_framenum - level.framenum) * FRAMETIME;
		drop->think = SG_FreeEntity;
	}
}

/*
 ==================
 SG_LookAtKiller
 ==================
*/
static void SG_LookAtKiller (edict_t *self, edict_t *inflictor, edict_t *attacker){

	vec3_t	dir;

	if (attacker && attacker != world && attacker != self)
		VectorSubtract(attacker->s.origin, self->s.origin, dir);
	else if (inflictor && inflictor != world && inflictor != self)
		VectorSubtract(inflictor->s.origin, self->s.origin, dir);
	else {
		self->client->killer_yaw = self->s.angles[YAW];
		return;
	}

	if (dir[0])
		self->client->killer_yaw = 180.0f / M_PI * atan2(dir[1], dir[0]);
	else {
		self->client->killer_yaw = 0.0f;
		if (dir[1] > 0.0f)
			self->client->killer_yaw = 90.0f;
		else if (dir[1] < 0.0f)
			self->client->killer_yaw = -90.0f;
	}

	if (self->client->killer_yaw < 0.0f)
		self->client->killer_yaw += 360.0f;
}

/*
 ==================
 SG_Player_Pain
 ==================
*/
void SG_Player_Pain (edict_t *self, edict_t *other, float kick, int damage){

	// Player pain is handled at the end of the frame in P_DamageFeedback
}

/*
 ==================
 SG_Player_Die
 ==================
*/
void SG_Player_Die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point){

	int			j;
	static int	i;

	VectorClear(self->avelocity);

	self->takedamage = DAMAGE_YES;
	self->movetype = MOVETYPE_TOSS;

	self->s.modelindex2 = 0;	// Remove linked weapon model

	self->s.angles[0] = 0.0f;
	self->s.angles[2] = 0.0f;

	self->s.sound = 0;
	self->client->weapon_sound = 0;

	self->maxs[2] = -8.0f;

	self->svflags |= SVF_DEADMONSTER;

	if (!self->deadflag){
		self->client->respawn_time = level.time + 1.0f;

		SG_LookAtKiller(self, inflictor, attacker);

		self->client->ps.pmove.pm_type = PM_DEAD;

		SG_ClientObituary(self, inflictor, attacker);
		SG_TossClientWeapon(self);

		// Show scores
		if (deathmatch->value)
			SG_Help_f(self);

		// Clear inventory
		// This is kind of ugly, but it's how we want to handle keys in coop
		for (j = 0; j < game.num_items; j++){
			if (coop->value && sg_itemList[j].flags & IT_KEY)
				self->client->resp.coop_respawn.inventory[j] = self->client->pers.inventory[j];

			self->client->pers.inventory[j] = 0;
		}
	}

	// Remove powerups
	self->client->quad_framenum = 0;
	self->client->invincible_framenum = 0;
	self->client->breather_framenum = 0;
	self->client->enviro_framenum = 0;

	self->flags &= ~FL_POWER_ARMOR;

	// Gib death
	if (self->health < -40){
		gi.sound(self, CHAN_BODY, gi.soundindex("misc/udeath.wav"), 1.0f, ATTN_NORM, 0.0f);

		ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);

		ThrowClientHead(self, damage);

		self->takedamage = DAMAGE_NO;
	}
	else {
		// Normal death
		if (!self->deadflag){
			i = (i + 1) % 3;

			// Start a death animation
			self->client->anim_priority = ANIM_DEATH;

			if (self->client->ps.pmove.pm_flags & PMF_DUCKED){
				self->s.frame = FRAME_crdeath1 - 1;
				self->client->anim_end = FRAME_crdeath5;
			}
			else {
				switch (i){
				case 0:
					self->s.frame = FRAME_death101 - 1;
					self->client->anim_end = FRAME_death106;
					break;
				case 1:
					self->s.frame = FRAME_death201 - 1;
					self->client->anim_end = FRAME_death206;
					break;
				case 2:
					self->s.frame = FRAME_death301 - 1;
					self->client->anim_end = FRAME_death308;
					break;
				}

				gi.sound(self, CHAN_VOICE, gi.soundindex(va("*death%i.wav", (rand() % 4) + 1)), 1.0f, ATTN_NORM, 0.0f);
			}
		}

		self->deadflag = DEAD_DEAD;

		gi.linkentity(self);
	}
}


/*
 ==============================================================================

 CLIENT DATA

 ==============================================================================
*/


/*
 ==================
 SG_InitClientPersistantData

 This is only called when the game first initializes in single player,
 but is called after each death and level change in deathmatch
 ==================
*/
static void SG_InitClientPersistantData (gclient_t *client){

	gItem_t	*item;

	memset(&client->pers, 0, sizeof(client->pers));

	item = SG_FindItem("Blaster");
	client->pers.selected_item = ITEM_INDEX(item);
	client->pers.inventory[client->pers.selected_item] = 1;

	client->pers.weapon = item;

	client->pers.health = 100;
	client->pers.max_health = 100;

	client->pers.max_bullets = 200;
	client->pers.max_shells = 100;
	client->pers.max_rockets = 50;
	client->pers.max_grenades = 50;
	client->pers.max_cells = 200;
	client->pers.max_slugs = 50;

	client->pers.connected = true;
}

/*
 ==================
 SG_InitClientRespawn
 ==================
*/
static void SG_InitClientRespawn (gclient_t *client){

	memset(&client->resp, 0, sizeof(client->resp));

	client->resp.enterframe = level.framenum;
	client->resp.coop_respawn = client->pers;
}

/*
 ==================
 SG_SaveClientData

 Some information that should be persistant, like health,
 is still stored in the edict structure, so it needs to
 be mirrored out to the client structure before all the
 edicts are wiped.
 ==================
*/
void SG_SaveClientData (){

	int		i;
	edict_t	*entity;

	for (i = 0; i < game.maxclients; i++){
		entity = &g_edicts[1 + i];

		if (!entity->inuse)
			continue;

		game.clients[i].pers.health = entity->health;
		game.clients[i].pers.max_health = entity->max_health;
		game.clients[i].pers.savedFlags = (entity->flags & (FL_GODMODE|FL_NOTARGET|FL_POWER_ARMOR));

		if (coop->value)
			game.clients[i].pers.score = entity->client->resp.score;
	}
}

/*
 ==================
 SG_GetClientEntityData
 ==================
*/
static void SG_GetClientEntityData (edict_t *entity){

	entity->health = entity->client->pers.health;
	entity->max_health = entity->client->pers.max_health;
	entity->flags |= entity->client->pers.savedFlags;

	if (coop->value)
		entity->client->resp.score = entity->client->pers.score;
}


/*
 ==============================================================================

 SPAWN POINT SELECTION

 ==============================================================================
*/


/*
 ==================
 SG_PlayersRangeFromSpot

 Returns the distance to the nearest player from the given spot
 ==================
*/
static float SG_PlayersRangeFromSpot (edict_t *spot){

	float	bestDistance;
	int		i;
	edict_t	*player;
	vec3_t	v;
	float	playerDistance;


	bestDistance = 9999999.0f;

	for (i = 1; i <= maxclients->value; i++){
		player = &g_edicts[i];

		if (!player->inuse)
			continue;

		if (player->health <= 0)
			continue;

		VectorSubtract(spot->s.origin, player->s.origin, v);
		playerDistance = VectorLength(v);

		if (playerDistance < bestDistance)
			bestDistance = playerDistance;
	}

	return bestDistance;
}

/*
 ==================
 SG_SelectRandomDeathmatchSpawnPoint

 Go to a random point, but NOT the two points closest
 to other players
 ==================
*/
static edict_t *SG_SelectRandomDeathmatchSpawnPoint (){

	edict_t	*spot, *spot1, *spot2;
	int		count = 0;
	int		selection;
	float	range, range1, range2;

	spot = NULL;
	range1 = range2 = 99999.0f;
	spot1 = spot2 = NULL;

	while ((spot = SG_FindEntity(spot, FOFS(className), "info_player_deathmatch")) != NULL){
		count++;

		range = SG_PlayersRangeFromSpot(spot);

		if (range < range1){
			range1 = range;
			spot1 = spot;
		}
		else if (range < range2){
			range2 = range;
			spot2 = spot;
		}
	}

	if (!count)
		return NULL;

	if (count <= 2)
		spot1 = spot2 = NULL;
	else
		count -= 2;

	selection = rand() % count;

	spot = NULL;
	do {
		spot = SG_FindEntity(spot, FOFS(className), "info_player_deathmatch");

		if (spot == spot1 || spot == spot2)
			selection++;
	} while (selection--);

	return spot;
}

/*
 ==================
 SG_SelectFarthestDeathmatchSpawnPoint
 ==================
*/
static edict_t *SG_SelectFarthestDeathmatchSpawnPoint (){

	edict_t	*spot;
	edict_t	*bestSpot;
	float	bestDistance, bestPlayerDistance;

	spot = NULL;
	bestSpot = NULL;
	bestDistance = 0;

	while ((spot = SG_FindEntity(spot, FOFS(className), "info_player_deathmatch")) != NULL){
		bestPlayerDistance = SG_PlayersRangeFromSpot(spot);

		if (bestPlayerDistance > bestDistance){
			bestSpot = spot;
			bestDistance = bestPlayerDistance;
		}
	}

	if (bestSpot)
		return bestSpot;

	// If there is a player just spawned on each and every start spot
	// we have no choice to turn one into a telefrag meltdown
	spot = SG_FindEntity(NULL, FOFS(className), "info_player_deathmatch");

	return spot;
}

/*
 ==================
 SG_SelectDeathmatchSpawnPoint
 ==================
*/
static edict_t *SG_SelectDeathmatchSpawnPoint (){

	if ((int)(dmflags->value) & DF_SPAWN_FARTHEST)
		return SG_SelectFarthestDeathmatchSpawnPoint();
	else
		return SG_SelectRandomDeathmatchSpawnPoint();
}

/*
 ==================
 SG_SelectCoopSpawnPoint
 ==================
*/
static edict_t *SG_SelectCoopSpawnPoint (edict_t *entity){

	int		index;
	edict_t	*spot = NULL;
	char	*target;

	index = entity->client - game.clients;

	// Player 0 starts in normal player spawn point
	if (!index)
		return NULL;

	spot = NULL;

	// assume there are four coop spots at each spawnpoint
	while (1){
		spot = SG_FindEntity (spot, FOFS(className), "info_player_coop");

		if (!spot)
			return NULL;	// We didn't have enough...

		target = spot->targetname;
		if (!target)
			target = "";

		// This is a coop spawn point for one of the clients here
		if (!Q_stricmp(game.spawnpoint, target)){
			index--;
			if (!index)
				return spot;	// This is it
		}
	}

	return spot;
}

/*
 ==================
 SG_SelectSpawnPoint

 Chooses a player start, deathmatch start, coop start, etc
 ==================
*/
static void SG_SelectSpawnPoint (edict_t *entity, vec3_t origin, vec3_t angles){

	edict_t	*spot = NULL;

	if (deathmatch->value)
		spot = SG_SelectDeathmatchSpawnPoint();
	else if (coop->value)
		spot = SG_SelectCoopSpawnPoint(entity);

	// Find a single player start spot
	if (!spot){
		while ((spot = SG_FindEntity(spot, FOFS(className), "info_player_start")) != NULL){
			if (!game.spawnpoint[0] && !spot->targetname)
				break;

			if (!game.spawnpoint[0] || !spot->targetname)
				continue;

			if (!Q_stricmp(game.spawnpoint, spot->targetname))
				break;
		}

		// There wasn't a spawnpoint without a target, so use any
		if (!spot){
			if (!game.spawnpoint[0])
				spot = SG_FindEntity(spot, FOFS(className), "info_player_start");

			if (!spot)
				gi.error("Couldn't find spawn point %s\n", game.spawnpoint);
		}
	}

	VectorCopy(spot->s.origin, origin);
	origin[2] += 9.0f;
	VectorCopy(spot->s.angles, angles);
}


/*
 ==============================================================================

 BODY QUE AND RESPAWNING

 ==============================================================================
*/


/*
 ==================
 SG_InitClientBodyQue
 ==================
*/
void SG_InitClientBodyQue (){

	int		i;
	edict_t	*entity;

	level.body_que = 0;

	for (i = 0; i < BODY_QUEUE_SIZE; i++){
		entity = SG_AllocEntity();
		entity->className = "bodyque";
	}
}

/*
 ==================
 SG_ClientBody_Die
 ==================
*/
static void SG_ClientBody_Die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point){

	if (self->health < -40){
		gi.sound(self, CHAN_BODY, gi.soundindex("misc/udeath.wav"), 1.0f, ATTN_NORM, 0.0f);

		ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);
		ThrowGib(self, "models/objects/gibs/sm_meat/tris.md2", damage, GIB_ORGANIC);

		self->s.origin[2] -= 48.0f;

		ThrowClientHead(self, damage);

		self->takedamage = DAMAGE_NO;
	}
}

/*
 ==================
 SG_CopyClientToBodyQue
 ==================
*/
static void SG_CopyClientToBodyQue (edict_t *entity){

	edict_t	*body;

	// Grab a body que and cycle to the next one
	body = &g_edicts[(int)maxclients->value + level.body_que + 1];
	level.body_que = (level.body_que + 1) % BODY_QUEUE_SIZE;

	// FIXME: Send an effect on the removed body

	gi.unlinkentity(entity);

	gi.unlinkentity(body);
	body->s = entity->s;
	body->s.number = body - g_edicts;

	body->svflags = entity->svflags;
	VectorCopy(entity->mins, body->mins);
	VectorCopy(entity->maxs, body->maxs);
	VectorCopy(entity->absmin, body->absmin);
	VectorCopy(entity->absmax, body->absmax);
	VectorCopy(entity->size, body->size);
	body->solid = entity->solid;
	body->clipmask = entity->clipmask;
	body->owner = entity->owner;
	body->movetype = entity->movetype;

	body->die = SG_ClientBody_Die;
	body->takedamage = DAMAGE_YES;

	gi.linkentity(body);
}

/*
 ==================
 SG_ClientRespawn
 ==================
*/
void SG_ClientRespawn (edict_t *self){

	if (deathmatch->value || coop->value){
		// Spectator's don't leave bodies
		if (self->movetype != MOVETYPE_NOCLIP)
			SG_CopyClientToBodyQue(self);

		self->svflags &= ~SVF_NOCLIENT;
		SG_PutClientInServer(self);

		// Aadd a teleportation effect
		self->s.event = EV_PLAYER_TELEPORT;

		// Hold in place briefly
		self->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
		self->client->ps.pmove.pm_time = 14;

		self->client->respawn_time = level.time;

		return;
	}

	// Restart the entire server
	gi.AddCommandString("menu_loadgame\n");
}

/*
 ==================
 SG_SpectatorRespawn

 Only called when pers.spectator changes
 Note that resp.spectator should be the opposite of pers.spectator here
 ==================
*/
static void SG_SpectatorRespawn (edict_t *entity){

	char	*value;
	int		i, numSpectators;

	// If the user wants to become a spectator, make sure he doesn't
	// exceed max_spectators
	if (entity->client->pers.spectator){
		value = Info_ValueForKey(entity->client->pers.userinfo, "spectator");

		if (*spectator_password->string && strcmp(spectator_password->string, "none") && strcmp(spectator_password->string, value)){
			gi.cprintf(entity, PRINT_HIGH, "Spectator password incorrect.\n");
			entity->client->pers.spectator = false;
			
			gi.WriteByte(svc_stufftext);
			gi.WriteString("spectator 0\n");	
			gi.unicast(entity, true);
			return;
		}

		// Count spectators
		for (i = 1, numSpectators = 0; i <= maxclients->value; i++){
			if (g_edicts[i].inuse && g_edicts[i].client->pers.spectator)
				numSpectators++;
		}

		if (numSpectators >= maxspectators->value) {
			gi.cprintf(entity, PRINT_HIGH, "Server spectator limit is full.");
			entity->client->pers.spectator = false;

			// Reset his spectator var
			gi.WriteByte(svc_stufftext);
			gi.WriteString("spectator 0\n");
			gi.unicast(entity, true);
			return;
		}
	}
	else {
		// He was a spectator and wants to join the game
		// he must have the right password
		value = Info_ValueForKey(entity->client->pers.userinfo, "password");

		if (*password->string && strcmp(password->string, "none") && strcmp(password->string, value)){
			gi.cprintf(entity, PRINT_HIGH, "Password incorrect.\n");
			entity->client->pers.spectator = true;
		
			gi.WriteByte(svc_stufftext);
			gi.WriteString("spectator 1\n");
			gi.unicast(entity, true);
			return;
		}
	}

	// Clear client on respawn
	entity->client->resp.score = entity->client->pers.score = 0;

	entity->svflags &= ~SVF_NOCLIENT;
	SG_PutClientInServer(entity);

	// Add a teleportation effect
	if (!entity->client->pers.spectator) {
		// Send effect
		gi.WriteByte(svc_muzzleflash);
		gi.WriteShort(entity - g_edicts);
		gi.WriteByte(MZ_LOGIN);
		gi.multicast(entity->s.origin, MULTICAST_PVS);

		// Hold in place briefly
		entity->client->ps.pmove.pm_flags = PMF_TIME_TELEPORT;
		entity->client->ps.pmove.pm_time = 14;
	}

	entity->client->respawn_time = level.time;

	if (entity->client->pers.spectator) 
		gi.bprintf(PRINT_HIGH, "%s has moved to the sidelines\n", entity->client->pers.netname);
	else
		gi.bprintf(PRINT_HIGH, "%s joined the game\n", entity->client->pers.netname);
}


/*
 ==============================================================================

 CLIENT/SERVER CONNECTION

 ==============================================================================
*/


/*
 ==================
 SG_ClientUserInfoChanged

 Called from ClientConnect when the player first connects and directly by the
 server system when the player updates a user info variable.

 The game can override any of the settings if desired.
 ==================
*/
void SG_ClientUserInfoChanged (edict_t *entity, char *userInfo){

	char	*string;
	int		playernum;

	// Check for malformed or illegal info strings
	if (!Info_Validate(userInfo))
		strcpy(userInfo, "\\name\\badinfo\\skin\\male/grunt");

	// Set the new player name
	string = Info_ValueForKey(userInfo, "name");

	strncpy(entity->client->pers.netname, string, sizeof(entity->client->pers.netname) - 1);

	// Set the new spectator
	string = Info_ValueForKey(userInfo, "spectator");

	// Spectators are only supported in deathmatch
	if (deathmatch->value && *string && strcmp(string, "0"))
		entity->client->pers.spectator = true;
	else
		entity->client->pers.spectator = false;

	// Set the new player skin
	string = Info_ValueForKey(userInfo, "skin");

	playernum = entity - g_edicts - 1;

	// Combine name and skin into a configstring
	gi.configstring(CS_PLAYERSKINS + playernum, va("%s\\%s", entity->client->pers.netname, string));

	// FOV
	if (deathmatch->value && ((int)dmflags->value & DF_FIXED_FOV))
		entity->client->ps.fov = 90.0f;
	else {
		entity->client->ps.fov = atoi(Info_ValueForKey(userInfo, "fov"));

		if (entity->client->ps.fov < 1.0f)
			entity->client->ps.fov = 90.0f;
		else if (entity->client->ps.fov > 160.0f)
			entity->client->ps.fov = 160.0f;
	}

	// Handedness
	string = Info_ValueForKey(userInfo, "hand");

	if (strlen(string))
		entity->client->pers.hand = atoi(string);

	// Save off the userinfo in case we want to check something later
	strncpy(entity->client->pers.userinfo, userInfo, sizeof(entity->client->pers.userinfo) - 1);
}

/*
 ==================
 SG_ClientConnect

 Called when a player begins connecting to the server and for every map change.

 The game can refuse entrance to a client by returning false.

 If the client is allowed, the connection process will continue and eventually
 get to SG_ClientBegin()

 Changing levels will NOT cause this to be called again, but loadgames will.
 ==================
*/
qboolean SG_ClientConnect (edict_t *entity, char *userInfo){

	char	*value;
	int		i, numSpectators;

	// Check to see if they are on the banned IP list
	value = Info_ValueForKey(userInfo, "ip");

	if (SG_FilterPacket(value)){
		Info_SetValueForKey(userInfo, "rejmsg", "Banned.");
		return false;
	}

	// Check for a spectator
	value = Info_ValueForKey(userInfo, "spectator");

	if (deathmatch->value && *value && strcmp(value, "0")){
		if (*spectator_password->string && strcmp(spectator_password->string, "none") && strcmp(spectator_password->string, value)){
			Info_SetValueForKey(userInfo, "rejmsg", "Spectator password required or incorrect.");
			return false;
		}

		// Count spectators
		for (i = numSpectators = 0; i < maxclients->value; i++){
			if (g_edicts[i + 1].inuse && g_edicts[i + 1].client->pers.spectator)
				numSpectators++;
		}

		if (numSpectators >= maxspectators->value){
			Info_SetValueForKey(userInfo, "rejmsg", "Server spectator limit is full.");
			return false;
		}
	}
	else {
		// Check for a password
		value = Info_ValueForKey(userInfo, "password");

		if (*password->string && strcmp(password->string, "none") && strcmp(password->string, value)){
			Info_SetValueForKey(userInfo, "rejmsg", "Password required or incorrect.");
			return false;
		}
	}

	// They can connect
	entity->client = game.clients + (entity - g_edicts - 1);

	// If there is already a body waiting for us (a loadgame), just
	// take it, otherwise spawn one from scratch
	if (entity->inuse == false){
		// Clear the respawning variables
		SG_InitClientRespawn(entity->client);

		if (!game.autosaved || !entity->client->pers.weapon)
			SG_InitClientPersistantData(entity->client);
	}

	SG_ClientUserInfoChanged(entity, userInfo);

	if (game.maxclients > 1)
		gi.dprintf("%s connected\n", entity->client->pers.netname);

	entity->svflags = 0;	// Make sure we start with known default
	entity->client->pers.connected = true;

	return true;
}

/*
 ==================
 SG_ClientDisconnect

 Called when a player drops from the server.
 Will not be called between levels.
 ==================
*/
void SG_ClientDisconnect (edict_t *entity){

	int		playernum;

	if (!entity->client)
		return;

	gi.bprintf(PRINT_HIGH, "%s disconnected\n", entity->client->pers.netname);

	// Send effect
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(entity - g_edicts);
	gi.WriteByte(MZ_LOGOUT);
	gi.multicast(entity->s.origin, MULTICAST_PVS);

	// Clear the entity
	gi.unlinkentity(entity);

	entity->s.modelindex = 0;
	entity->solid = SOLID_NOT;
	entity->inuse = false;
	entity->className = "disconnected";
	entity->client->pers.connected = false;

	playernum = entity - g_edicts - 1;
	gi.configstring(CS_PLAYERSKINS + playernum, "");
}

/*
 ==================
 SG_ClientBeginDeathmatch

 A client has just connected to the server in deathmatch mode,
 so clear everything out before starting them.
 ==================
*/
static void SG_ClientBeginDeathmatch (edict_t *entity){

	SG_InitEntity(entity);

	SG_InitClientRespawn(entity->client);

	// Locate ent at a spawn point
	SG_PutClientInServer(entity);

	if (level.intermissiontime)
		SG_MoveClientToIntermission(entity);
	else {
		// Send effect
		gi.WriteByte(svc_muzzleflash);
		gi.WriteShort(entity - g_edicts);
		gi.WriteByte(MZ_LOGIN);
		gi.multicast(entity->s.origin, MULTICAST_PVS);
	}

	gi.bprintf(PRINT_HIGH, "%s entered the game\n", entity->client->pers.netname);

	// Make sure all view stuff is valid
	SG_ClientEndServerFrame(entity);
}

/*
 ==================
 SG_ClientBegin

 Called when a player has finished connecting, and is ready to be placed into
 the level.
 
 This will happen every level load, and on transition between teams, but
 doesn't happen on respawns.
 ==================
*/
void SG_ClientBegin (edict_t *entity){

	entity->client = game.clients + (entity - g_edicts - 1);

	if (deathmatch->value){
		SG_ClientBeginDeathmatch(entity);
		return;
	}

	// If there is already a body waiting for us (a loadgame), just
	// take it, otherwise spawn one from scratch
	if (entity->inuse == true){
		// The client has cleared the client side viewangles upon
		// connecting to the server, which is different than the
		// state when the game is saved, so we need to compensate
		// with deltaangles
		entity->client->ps.pmove.delta_angles[0] = ANGLE2SHORT(entity->client->ps.viewangles[0]);
		entity->client->ps.pmove.delta_angles[1] = ANGLE2SHORT(entity->client->ps.viewangles[1]);
		entity->client->ps.pmove.delta_angles[2] = ANGLE2SHORT(entity->client->ps.viewangles[2]);
	}
	else {
		// A spawn point will completely reinitialize the entity
		// except for the persistant data that was initialized at
		// ClientConnect() time
		SG_InitEntity(entity);

		entity->className = "player";

		SG_InitClientRespawn(entity->client);
		SG_PutClientInServer(entity);
	}

	if (level.intermissiontime)
		SG_MoveClientToIntermission(entity);
	else {
		// Send effect if in a multiplayer game
		if (game.maxclients > 1){
			gi.WriteByte(svc_muzzleflash);
			gi.WriteShort(entity - g_edicts);
			gi.WriteByte(MZ_LOGIN);
			gi.multicast(entity->s.origin, MULTICAST_PVS);

			gi.bprintf(PRINT_HIGH, "%s entered the game\n", entity->client->pers.netname);
		}
	}

	// Make sure all view stuff is valid
	SG_ClientEndServerFrame(entity);
}

/*
 ==================
 SG_PutClientInServer

 Called when a player connects to a server or respawns in
 a deathmatch
 ==================
*/
static void SG_PutClientInServer (edict_t *entity){

	vec3_t				spawnOrigin, spawnAngles;
	char				userInfo[MAX_INFO_STRING];
	int					index;
	gclient_t			*client;
	client_respawn_t	resp;
	client_persistant_t	saved;
	vec3_t				mins = {-16.0f, -16.0f, -24.0f};
	vec3_t				maxs = {16.0f, 16.0f, 32.0f};

	// Find a spawn point
	// Do it before setting health back up, so farthest
	// ranging doesn't count this client
	SG_SelectSpawnPoint(entity, spawnOrigin, spawnAngles);

	index = entity-g_edicts - 1;
	client = entity->client;

	// Deathmatch wipes most client data every spawn
	if (deathmatch->value){
		resp = client->resp;

		memcpy(userInfo, client->pers.userinfo, sizeof(userInfo));

		SG_InitClientPersistantData(client);
		SG_ClientUserInfoChanged(entity, userInfo);
	}
	else if (coop->value){
		resp = client->resp;

		memcpy(userInfo, client->pers.userinfo, sizeof(userInfo));

		resp.coop_respawn.game_helpchanged = client->pers.game_helpchanged;
		resp.coop_respawn.helpchanged = client->pers.helpchanged;

		client->pers = resp.coop_respawn;

		SG_ClientUserInfoChanged(entity, userInfo);

		if (resp.score > client->pers.score)
			client->pers.score = resp.score;
	}
	else
		memset(&resp, 0, sizeof(resp));

	// Clear everything but the persistant data
	saved = client->pers;

	memset(client, 0, sizeof(*client));
	client->pers = saved;

	if (client->pers.health <= 0)
		SG_InitClientPersistantData(client);

	client->resp = resp;

	// Copy some data from the client to the entity
	SG_GetClientEntityData(entity);

	// Clear entity values
	entity->groundentity = NULL;
	entity->client = &game.clients[index];
	entity->takedamage = DAMAGE_AIM;
	entity->movetype = MOVETYPE_WALK;
	entity->viewheight = 22;
	entity->inuse = true;
	entity->className = "player";
	entity->mass = 200;
	entity->solid = SOLID_BBOX;
	entity->deadflag = DEAD_NO;
	entity->air_finished = level.time + 12.0f;
	entity->clipmask = MASK_PLAYERSOLID;
	entity->model = "players/male/tris.md2";
	entity->pain = SG_Player_Pain;
	entity->die = SG_Player_Die;
	entity->waterLevel = WATERLEVEL_NONE;
	entity->waterType = 0;
	entity->flags &= ~FL_NO_KNOCKBACK;
	entity->svflags &= ~SVF_DEADMONSTER;

	VectorCopy(mins, entity->mins);
	VectorCopy(maxs, entity->maxs);
	VectorClear(entity->velocity);

	// Clear playerstate values
	memset(&entity->client->ps, 0, sizeof(client->ps));

	client->ps.pmove.origin[0] = spawnOrigin[0] * 8.0f;
	client->ps.pmove.origin[1] = spawnOrigin[1] * 8.0f;
	client->ps.pmove.origin[2] = spawnOrigin[2] * 8.0f;

	if (deathmatch->value && ((int)dmflags->value & DF_FIXED_FOV))
		client->ps.fov = 90.0f;
	else {
		client->ps.fov = atoi(Info_ValueForKey(client->pers.userinfo, "fov"));

		if (client->ps.fov < 1.0f)
			client->ps.fov = 90.0f;
		else if (client->ps.fov > 160.0f)
			client->ps.fov = 160.0f;
	}

	client->ps.gunindex = gi.modelindex(client->pers.weapon->viewModel);

	// Clear entity state values
	entity->s.effects = 0;
	entity->s.modelindex = 255;		// Will use the skin specified model
	entity->s.modelindex2 = 255;	// Custom gun model

	// sknum is player number and weapon number
	// weapon number will be added in changeweapon
	entity->s.skinnum = entity - g_edicts - 1;

	entity->s.frame = 0;
	VectorCopy(spawnOrigin, entity->s.origin);
	entity->s.origin[2] += 1.0f;	// Make sure off ground
	VectorCopy(entity->s.origin, entity->s.old_origin);

	// Set the delta angle
	client->ps.pmove.delta_angles[0] = ANGLE2SHORT(spawnAngles[0] - client->resp.cmd_angles[0]);
	client->ps.pmove.delta_angles[1] = ANGLE2SHORT(spawnAngles[1] - client->resp.cmd_angles[1]);
	client->ps.pmove.delta_angles[2] = ANGLE2SHORT(spawnAngles[2] - client->resp.cmd_angles[2]);

	entity->s.angles[PITCH] = 0.0f;
	entity->s.angles[YAW] = spawnAngles[YAW];
	entity->s.angles[ROLL] = 0.0f;
	VectorCopy(entity->s.angles, client->ps.viewangles);
	VectorCopy(entity->s.angles, client->v_angle);

	// Spawn a spectator
	if (client->pers.spectator){
		client->chaseTarget = NULL;

		client->resp.spectator = true;

		entity->movetype = MOVETYPE_NOCLIP;
		entity->solid = SOLID_NOT;
		entity->svflags |= SVF_NOCLIENT;
		entity->client->ps.gunindex = 0;

		gi.linkentity(entity);

		return;
	}
	else
		client->resp.spectator = false;

	// Could't spawn in?
	if (!SG_KillBox(entity)){
	}

	gi.linkentity(entity);

	// Force the current weapon up
	client->newweapon = client->pers.weapon;

	SG_ChangeWeapon(entity);
}


/*
 ==============================================================================

 FRAMEWORK

 ==============================================================================
*/


/*
 ==================
 PM_Trace

 pmove doesn't need to know about passent and contentmask
 ==================
*/
static trace_t PM_Trace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end){

	if (pm_passent->health > 0)
		return gi.trace(start, mins, maxs, end, pm_passent, MASK_PLAYERSOLID);
	else
		return gi.trace(start, mins, maxs, end, pm_passent, MASK_DEADSOLID);
}

/*
 ==================
 SG_ClientThinkFramework

 This will be called once for each client frame, which will
 usually be a couple times for each server frame.
 ==================
*/
void SG_ClientThinkFramework (edict_t *entity, usercmd_t *userCmd){

	gclient_t	*client;
	edict_t		*other;
	int			i, j;
	pmove_t		pm;

	level.current_entity = entity;
	client = entity->client;

	if (level.intermissiontime){
		client->ps.pmove.pm_type = PM_FREEZE;

		// Can exit intermission after five seconds
		if (level.time > level.intermissiontime + 5.0f && (userCmd->buttons & BUTTON_ANY))
			level.exitintermission = true;

		return;
	}

	pm_passent = entity;

	if (entity->client->chaseTarget){
		client->resp.cmd_angles[0] = SHORT2ANGLE(userCmd->angles[0]);
		client->resp.cmd_angles[1] = SHORT2ANGLE(userCmd->angles[1]);
		client->resp.cmd_angles[2] = SHORT2ANGLE(userCmd->angles[2]);
	}
	else {
		// Set up for pmove
		memset(&pm, 0, sizeof(pm));

		if (entity->movetype == MOVETYPE_NOCLIP)
			client->ps.pmove.pm_type = PM_SPECTATOR;
		else if (entity->s.modelindex != 255)
			client->ps.pmove.pm_type = PM_GIB;
		else if (entity->deadflag)
			client->ps.pmove.pm_type = PM_DEAD;
		else
			client->ps.pmove.pm_type = PM_NORMAL;

		client->ps.pmove.gravity = sv_gravity->value;
		pm.s = client->ps.pmove;

		pm.s.origin[0] = entity->s.origin[0] * 8.0f;
		pm.s.origin[1] = entity->s.origin[1] * 8.0f;
		pm.s.origin[2] = entity->s.origin[2] * 8.0f;

		pm.s.velocity[0] = entity->velocity[0] * 8.0f;
		pm.s.velocity[1] = entity->velocity[1] * 8.0f;
		pm.s.velocity[2] = entity->velocity[2] * 8.0f;

		if (memcmp(&client->old_pmove, &pm.s, sizeof(pm.s)))
			pm.snapinitial = true;

		pm.cmd = *userCmd;

		pm.trace = PM_Trace;	// Adds default parms
		pm.pointcontents = gi.pointcontents;

		// Perform a pmove
		gi.Pmove(&pm);

		// Save results of pmove
		client->ps.pmove = pm.s;
		client->old_pmove = pm.s;

		entity->s.origin[0] = pm.s.origin[0] * 0.125f;
		entity->s.origin[1] = pm.s.origin[1] * 0.125f;
		entity->s.origin[2] = pm.s.origin[2] * 0.125f;

		entity->velocity[0] = pm.s.velocity[0] * 0.125f;
		entity->velocity[1] = pm.s.velocity[1] * 0.125f;
		entity->velocity[2] = pm.s.velocity[2] * 0.125f;

		VectorCopy(pm.mins, entity->mins);
		VectorCopy(pm.maxs, entity->maxs);

		client->resp.cmd_angles[0] = SHORT2ANGLE(userCmd->angles[0]);
		client->resp.cmd_angles[1] = SHORT2ANGLE(userCmd->angles[1]);
		client->resp.cmd_angles[2] = SHORT2ANGLE(userCmd->angles[2]);

		if (entity->groundentity && !pm.groundentity && (pm.cmd.upmove >= 10) && (pm.waterlevel == 0)){
			gi.sound(entity, CHAN_VOICE, gi.soundindex("*jump1.wav"), 1.0f, ATTN_NORM, 0.0f);
			SG_PlayerNoise(entity, entity->s.origin, PNOISE_SELF);
		}

		entity->viewheight = pm.viewheight;
		entity->waterLevel = (waterLevel_t)pm.waterlevel;
		entity->waterType = pm.watertype;
		entity->groundentity = pm.groundentity;

		if (pm.groundentity)
			entity->groundentity_linkcount = pm.groundentity->linkcount;

		if (entity->deadflag){
			client->ps.viewangles[ROLL] = 40.0f;
			client->ps.viewangles[PITCH] = -15.0f;
			client->ps.viewangles[YAW] = client->killer_yaw;
		}
		else {
			VectorCopy(pm.viewangles, client->v_angle);
			VectorCopy(pm.viewangles, client->ps.viewangles);
		}

		gi.linkentity(entity);

		if (entity->movetype != MOVETYPE_NOCLIP)
			SG_TouchTriggers(entity);

		// Touch other objects
		for (i = 0; i < pm.numtouch; i++){
			other = pm.touchents[i];

			for (j = 0; j < i; j++){
				if (pm.touchents[j] == other)
					break;
			}

			if (j != i)
				continue;	// Duplicated
			if (!other->touch)
				continue;

			other->touch(other, entity, NULL, NULL);
		}

	}

	client->oldbuttons = client->buttons;
	client->buttons = userCmd->buttons;
	client->latched_buttons |= client->buttons & ~client->oldbuttons;

	// Save light level the player is standing on for
	// monster sighting AI
	entity->lightLevel = userCmd->lightlevel;

	// Chase camera
	SG_ChaseCamThinkFramework(userCmd, client, entity);
}

/*
 ==================
 SG_ClientBeginServerFrame

 This will be called once for each server frame, before running
 any other entities in the world
 ==================
*/
void SG_ClientBeginServerFrame (edict_t *entity){

	gclient_t	*client;
	int			buttonMask;

	if (level.intermissiontime)
		return;

	client = entity->client;

	if (deathmatch->value && client->pers.spectator != client->resp.spectator && (level.time - client->respawn_time) >= 5){
		SG_SpectatorRespawn(entity);
		return;
	}

	// Run weapon animations if it hasn't been done by a ucmd_t
	if (!client->weaponThink && !client->resp.spectator)
		SG_WeaponThinkFramework(entity);
	else
		client->weaponThink = false;

	if (entity->deadflag){
		// Wait for any button just going down
		if (level.time > client->respawn_time){
			// In deathmatch, only wait for attack button
			if (deathmatch->value)
				buttonMask = BUTTON_ATTACK;
			else
				buttonMask = -1;

			if ((client->latched_buttons & buttonMask ) || (deathmatch->value && ((int)dmflags->value & DF_FORCE_RESPAWN))){
				SG_ClientRespawn(entity);
				client->latched_buttons = 0;
			}
		}

		return;
	}

	// Add player trail so monsters can follow
	if (!deathmatch->value){
		if (!SG_IsEntityVisible(entity, SG_ClientTrail_LastSpot()))
			SG_AddClientTrail(entity->s.old_origin);
	}

	client->latched_buttons = 0;
}