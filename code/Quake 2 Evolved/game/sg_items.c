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
// sg_items.c - Items manager
//


#include "g_local.h"


#define HEALTH_IGNORE_MAX				1
#define HEALTH_TIMED					2

typedef struct {
	int					quadDropTimeoutHack;

	int					jacketArmorIndex;
	int					combatArmorIndex;
	int					bodyArmorIndex;
	int					powerScreenIndex;
	int					powerShieldIndex;
} itemLocals_t;

static itemLocals_t		itemLocals;

static gItemArmor_t		jacketArmorInfo	= {25, 50, 0.30f, 0.00f, ARMOR_JACKET};
static gItemArmor_t		combatArmorInfo	= {50, 100, 0.60f, 0.30f, ARMOR_COMBAT};
static gItemArmor_t		bodyArmorInfo	= {100, 200, 0.80f, 0.60f, ARMOR_BODY};

static void SG_Use_Quad (edict_t *entity, gItem_t *item);


/*
 ==============================================================================

 ITEM FINDING

 ==============================================================================
*/


/*
 ==================
 SG_GetItemByIndex
 ==================
*/
gItem_t	*SG_GetItemByIndex (int index){

	if (index == 0 || index >= game.num_items)
		return NULL;

	return &sg_itemList[index];
}

/*
 ==================
 SG_FindItemByClassname
 ==================
*/
gItem_t	*SG_FindItemByClassname (char *name){

	gItem_t	*item;
	int		i;

	item = sg_itemList;

	for (i = 0; i < game.num_items; i++, item++){
		if (!item->className)
			continue;
		if (!Q_stricmp(item->className, name))
			return item;
	}

	return NULL;
}

/*
 ==================
 SG_FindItem
 ==================
*/
gItem_t	*SG_FindItem (char *name){

	gItem_t	*item;
	int		i;

	item = sg_itemList;

	for (i = 0; i < game.num_items; i++, item++){
		if (!item->pickupName)
			continue;
		if (!Q_stricmp(item->pickupName, name))
			return item;
	}

	return NULL;
}


/*
 ==============================================================================

 ITEM RESPAWNING

 ==============================================================================
*/


/*
 ==================
 SG_RespawnItem
 ==================
*/
static void SG_RespawnItem (edict_t *entity){

	edict_t	*master;
	int		count;
	int		choice;

	if (entity->team){
		master = entity->teammaster;

		for (count = 0, entity = master; entity; entity = entity->chain, count++);

		choice = rand() % count;

		for (count = 0, entity = master; count < choice; entity = entity->chain, count++);
	}

	entity->svflags &= ~SVF_NOCLIENT;
	entity->solid = SOLID_TRIGGER;

	gi.linkentity(entity);

	// Send an effect
	entity->s.event = EV_ITEM_RESPAWN;
}

/*
 ==================
 SG_SetRespawnTime
 ==================
*/
void SG_SetRespawnTime (edict_t *entity, float delay){

	entity->flags |= FL_RESPAWN;
	entity->svflags |= SVF_NOCLIENT;
	entity->solid = SOLID_NOT;
	entity->nextthink = level.time + delay;
	entity->think = SG_RespawnItem;

	gi.linkentity(entity);
}


/*
 ==============================================================================

 ITEM PICKUP AND DROPPING

 ==============================================================================
*/


/*
 ==================
 SG_Drop_General
 ==================
*/
static void SG_Drop_General (edict_t *entity, gItem_t *item){

	SG_Drop_Item(entity, item);

	entity->client->pers.inventory[ITEM_INDEX(item)]--;

	SG_ValidateSelectedItem(entity);
}


/*
 ==================
 SG_Pickup_Powerup
 ==================
*/
static qboolean SG_Pickup_Powerup (edict_t *entity, edict_t *other){

	int		quantity;

	quantity = other->client->pers.inventory[ITEM_INDEX(entity->item)];

	if ((skill->value == 1 && quantity >= 2) || (skill->value >= 2 && quantity >= 1))
		return false;

	if ((coop->value) && (entity->item->flags & IT_STAY_COOP) && (quantity > 0))
		return false;

	other->client->pers.inventory[ITEM_INDEX(entity->item)]++;

	if (deathmatch->value){
		if (!(entity->spawnflags & DROPPED_ITEM))
			SG_SetRespawnTime(entity, entity->item->quantity);

		if (((int)dmflags->value & DF_INSTANT_ITEMS) || ((entity->item->use == SG_Use_Quad) && (entity->spawnflags & DROPPED_PLAYER_ITEM))){
			if ((entity->item->use == SG_Use_Quad) && (entity->spawnflags & DROPPED_PLAYER_ITEM))
				itemLocals.quadDropTimeoutHack = (entity->nextthink - level.time) / FRAMETIME;

			entity->item->use(other, entity->item);
		}
	}

	return true;
}

/*
 ==================
 SG_Pickup_Adrenaline
 ==================
*/
static qboolean SG_Pickup_Adrenaline (edict_t *entity, edict_t *other){

	if (!deathmatch->value)
		other->max_health += 1;

	if (other->health < other->max_health)
		other->health = other->max_health;

	if (!(entity->spawnflags & DROPPED_ITEM) && (deathmatch->value))
		SG_SetRespawnTime(entity, entity->item->quantity);

	return true;
}

/*
 ==================
 SG_Pickup_AncientHead
 ==================
*/
static qboolean SG_Pickup_AncientHead (edict_t *entity, edict_t *other){

	other->max_health += 2;

	if (!(entity->spawnflags & DROPPED_ITEM) && (deathmatch->value))
		SG_SetRespawnTime(entity, entity->item->quantity);

	return true;
}

/*
 ==================
 SG_Pickup_Bandolier
 ==================
*/
static qboolean SG_Pickup_Bandolier (edict_t *entity, edict_t *other){

	gItem_t	*item;
	int		index;

	if (other->client->pers.max_bullets < 250)
		other->client->pers.max_bullets = 250;
	if (other->client->pers.max_shells < 150)
		other->client->pers.max_shells = 150;
	if (other->client->pers.max_cells < 250)
		other->client->pers.max_cells = 250;
	if (other->client->pers.max_slugs < 75)
		other->client->pers.max_slugs = 75;

	item = SG_FindItem("Bullets");

	if (item){
		index = ITEM_INDEX(item);

		other->client->pers.inventory[index] += item->quantity;

		if (other->client->pers.inventory[index] > other->client->pers.max_bullets)
			other->client->pers.inventory[index] = other->client->pers.max_bullets;
	}

	item = SG_FindItem("Shells");

	if (item){
		index = ITEM_INDEX(item);

		other->client->pers.inventory[index] += item->quantity;

		if (other->client->pers.inventory[index] > other->client->pers.max_shells)
			other->client->pers.inventory[index] = other->client->pers.max_shells;
	}

	if (!(entity->spawnflags & DROPPED_ITEM) && (deathmatch->value))
		SG_SetRespawnTime(entity, entity->item->quantity);

	return true;
}

/*
 ==================
 SG_Pickup_Pack
 ==================
*/
static qboolean SG_Pickup_Pack (edict_t *entity, edict_t *other){

	gItem_t	*item;
	int		index;

	if (other->client->pers.max_bullets < 300)
		other->client->pers.max_bullets = 300;
	if (other->client->pers.max_shells < 200)
		other->client->pers.max_shells = 200;
	if (other->client->pers.max_rockets < 100)
		other->client->pers.max_rockets = 100;
	if (other->client->pers.max_grenades < 100)
		other->client->pers.max_grenades = 100;
	if (other->client->pers.max_cells < 300)
		other->client->pers.max_cells = 300;
	if (other->client->pers.max_slugs < 100)
		other->client->pers.max_slugs = 100;

	item = SG_FindItem("Bullets");

	if (item){
		index = ITEM_INDEX(item);

		other->client->pers.inventory[index] += item->quantity;

		if (other->client->pers.inventory[index] > other->client->pers.max_bullets)
			other->client->pers.inventory[index] = other->client->pers.max_bullets;
	}

	item = SG_FindItem("Shells");

	if (item){
		index = ITEM_INDEX(item);

		other->client->pers.inventory[index] += item->quantity;

		if (other->client->pers.inventory[index] > other->client->pers.max_shells)
			other->client->pers.inventory[index] = other->client->pers.max_shells;
	}

	item = SG_FindItem("Cells");

	if (item){
		index = ITEM_INDEX(item);

		other->client->pers.inventory[index] += item->quantity;

		if (other->client->pers.inventory[index] > other->client->pers.max_cells)
			other->client->pers.inventory[index] = other->client->pers.max_cells;
	}

	item = SG_FindItem("Grenades");

	if (item){
		index = ITEM_INDEX(item);

		other->client->pers.inventory[index] += item->quantity;

		if (other->client->pers.inventory[index] > other->client->pers.max_grenades)
			other->client->pers.inventory[index] = other->client->pers.max_grenades;
	}

	item = SG_FindItem("Rockets");

	if (item){
		index = ITEM_INDEX(item);

		other->client->pers.inventory[index] += item->quantity;

		if (other->client->pers.inventory[index] > other->client->pers.max_rockets)
			other->client->pers.inventory[index] = other->client->pers.max_rockets;
	}

	item = SG_FindItem("Slugs");

	if (item){
		index = ITEM_INDEX(item);

		other->client->pers.inventory[index] += item->quantity;

		if (other->client->pers.inventory[index] > other->client->pers.max_slugs)
			other->client->pers.inventory[index] = other->client->pers.max_slugs;
	}

	if (!(entity->spawnflags & DROPPED_ITEM) && (deathmatch->value))
		SG_SetRespawnTime (entity, entity->item->quantity);

	return true;
}


/*
 ==============================================================================

 ITEM USAGE

 ==============================================================================
*/


/*
 ==================
 SG_Use_Quad
 ==================
*/
static void SG_Use_Quad (edict_t *entity, gItem_t *item){

	int		timeOut;

	entity->client->pers.inventory[ITEM_INDEX(item)]--;

	SG_ValidateSelectedItem(entity);

	if (itemLocals.quadDropTimeoutHack){
		timeOut = itemLocals.quadDropTimeoutHack;
		itemLocals.quadDropTimeoutHack = 0;
	}
	else
		timeOut = 300;

	if (entity->client->quad_framenum > level.framenum)
		entity->client->quad_framenum += timeOut;
	else
		entity->client->quad_framenum = level.framenum + timeOut;

	gi.sound(entity, CHAN_ITEM, gi.soundindex("items/damage.wav"), 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 SG_Use_Breather
 ==================
*/
static void SG_Use_Breather (edict_t *entity, gItem_t *item){

	entity->client->pers.inventory[ITEM_INDEX(item)]--;

	SG_ValidateSelectedItem(entity);

	if (entity->client->breather_framenum > level.framenum)
		entity->client->breather_framenum += 300.0f;
	else
		entity->client->breather_framenum = level.framenum + 300.0f;
}

/*
 ==================
 SG_Use_EnvironmentSuit
 ==================
*/
static void SG_Use_EnvironmentSuit (edict_t *entity, gItem_t *item){

	entity->client->pers.inventory[ITEM_INDEX(item)]--;

	SG_ValidateSelectedItem(entity);

	if (entity->client->enviro_framenum > level.framenum)
		entity->client->enviro_framenum += 300.0f;
	else
		entity->client->enviro_framenum = level.framenum + 300.0f;
}

/*
 ==================
 SG_Use_Invulnerability
 ==================
*/
static void	SG_Use_Invulnerability (edict_t *entity, gItem_t *item){

	entity->client->pers.inventory[ITEM_INDEX(item)]--;

	SG_ValidateSelectedItem(entity);

	if (entity->client->invincible_framenum > level.framenum)
		entity->client->invincible_framenum += 300.0f;
	else
		entity->client->invincible_framenum = level.framenum + 300.0f;

	gi.sound(entity, CHAN_ITEM, gi.soundindex("items/protect.wav"), 1.0f, ATTN_NORM, 0.0f);
}

/*
 ==================
 SG_Use_Silencer
 ==================
*/
static void SG_Use_Silencer (edict_t *entity, gItem_t *item){

	entity->client->pers.inventory[ITEM_INDEX(item)]--;

	SG_ValidateSelectedItem(entity);

	entity->client->silencer_shots += 30;
}

/*
 ==================
 SG_Pickup_Key
 ==================
*/
static qboolean SG_Pickup_Key (edict_t *entity, edict_t *other){

	if (coop->value){
		if (!strcmp(entity->className, "key_power_cube")){
			if (other->client->pers.power_cubes & ((entity->spawnflags & 0x0000FF00)>> 8))
				return false;

			other->client->pers.inventory[ITEM_INDEX(entity->item)]++;
			other->client->pers.power_cubes |= ((entity->spawnflags & 0x0000FF00) >> 8);
		}
		else {
			if (other->client->pers.inventory[ITEM_INDEX(entity->item)])
				return false;

			other->client->pers.inventory[ITEM_INDEX(entity->item)] = 1;
		}

		return true;
	}

	other->client->pers.inventory[ITEM_INDEX(entity->item)]++;

	return true;
}


/*
 ==============================================================================

 TODO

 ==============================================================================
*/


/*
 ==================
 SG_Add_Ammo
 ==================
*/
qboolean SG_Add_Ammo (edict_t *entity, gItem_t *item, int count){

	int		max;
	int		index;

	if (!entity->client)
		return false;

	if (item->tag == AMMO_BULLETS)
		max = entity->client->pers.max_bullets;
	else if (item->tag == AMMO_SHELLS)
		max = entity->client->pers.max_shells;
	else if (item->tag == AMMO_ROCKETS)
		max = entity->client->pers.max_rockets;
	else if (item->tag == AMMO_GRENADES)
		max = entity->client->pers.max_grenades;
	else if (item->tag == AMMO_CELLS)
		max = entity->client->pers.max_cells;
	else if (item->tag == AMMO_SLUGS)
		max = entity->client->pers.max_slugs;
	else
		return false;

	index = ITEM_INDEX(item);

	if (entity->client->pers.inventory[index] == max)
		return false;

	entity->client->pers.inventory[index] += count;

	if (entity->client->pers.inventory[index] > max)
		entity->client->pers.inventory[index] = max;

	return true;
}

/*
 ==================
 SG_Pickup_Ammo
 ==================
*/
static qboolean SG_Pickup_Ammo (edict_t *entity, edict_t *other){

	qboolean	weapon;
	int			count;
	int			oldcount;

	weapon = (entity->item->flags & IT_WEAPON);

	if ((weapon) && ((int)dmflags->value & DF_INFINITE_AMMO))
		count = 1000;
	else if (entity->count)
		count = entity->count;
	else
		count = entity->item->quantity;

	oldcount = other->client->pers.inventory[ITEM_INDEX(entity->item)];

	if (!SG_Add_Ammo(other, entity->item, count))
		return false;

	if (weapon && !oldcount){
		if (other->client->pers.weapon != entity->item && (!deathmatch->value || other->client->pers.weapon == SG_FindItem("blaster")))
			other->client->newweapon = entity->item;
	}

	if (!(entity->spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM)) && (deathmatch->value))
		SG_SetRespawnTime(entity, 30);

	return true;
}

/*
 ==================
 SG_Drop_Ammo
 ==================
*/
static void SG_Drop_Ammo (edict_t *entity, gItem_t *item){

	int		index;
	edict_t	*dropped;

	index = ITEM_INDEX(item);
	dropped = SG_Drop_Item(entity, item);

	if (entity->client->pers.inventory[index] >= item->quantity)
		dropped->count = item->quantity;
	else
		dropped->count = entity->client->pers.inventory[index];

	if (entity->client->pers.weapon && entity->client->pers.weapon->tag == AMMO_GRENADES && item->tag == AMMO_GRENADES && entity->client->pers.inventory[index] - dropped->count <= 0){
		gi.cprintf(entity, PRINT_HIGH, "Can't drop current weapon\n");
		SG_FreeEntity(dropped);
		return;
	}

	entity->client->pers.inventory[index] -= dropped->count;
	SG_ValidateSelectedItem(entity);
}


/*
 ==================
 SG_MegaHealth_Think
 ==================
*/
static void SG_MegaHealth_Think (edict_t *self){

	if (self->owner->health > self->owner->max_health){
		self->nextthink = level.time + 1.0f;
		self->owner->health -= 1;
		return;
	}

	if (!(self->spawnflags & DROPPED_ITEM) && (deathmatch->value))
		SG_SetRespawnTime(self, 20);
	else
		SG_FreeEntity(self);
}

/*
 ==================
 SG_Pickup_Health
 ==================
*/
static qboolean SG_Pickup_Health (edict_t *entity, edict_t *other){

	if (!(entity->style & HEALTH_IGNORE_MAX)){
		if (other->health >= other->max_health)
			return false;
	}

	other->health += entity->count;

	if (!(entity->style & HEALTH_IGNORE_MAX)){
		if (other->health > other->max_health)
			other->health = other->max_health;
	}

	if (entity->style & HEALTH_TIMED){
		entity->think = SG_MegaHealth_Think;
		entity->nextthink = level.time + 5;
		entity->owner = other;
		entity->flags |= FL_RESPAWN;
		entity->svflags |= SVF_NOCLIENT;
		entity->solid = SOLID_NOT;
	}
	else {
		if (!(entity->spawnflags & DROPPED_ITEM) && (deathmatch->value))
			SG_SetRespawnTime(entity, 30.0f);
	}

	return true;
}


/*
 ==================
 SG_FindArmorByIndex
 ==================
*/
int SG_FindArmorByIndex (edict_t *entity){

	if (!entity->client)
		return 0;

	if (entity->client->pers.inventory[itemLocals.jacketArmorIndex] > 0)
		return itemLocals.jacketArmorIndex;

	if (entity->client->pers.inventory[itemLocals.combatArmorIndex] > 0)
		return itemLocals.combatArmorIndex;

	if (entity->client->pers.inventory[itemLocals.bodyArmorIndex] > 0)
		return itemLocals.bodyArmorIndex;

	return 0;
}

/*
 ==================
 SG_Pickup_Armor
 ==================
*/
static qboolean SG_Pickup_Armor (edict_t *entity, edict_t *other){

	gItemArmor_t	*newInfo;
	int				oldArmorIndex;
	gItemArmor_t	*oldInfo;
	float			salvage;
	int				salvageCount;
	int				newCount;

	// Get info on the new armor
	newInfo = (gItemArmor_t *)entity->item->info;

	oldArmorIndex = SG_FindArmorByIndex(other);

	// Handle armor shards specially
	if (entity->item->tag == ARMOR_SHARD){
		if (!oldArmorIndex)
			other->client->pers.inventory[itemLocals.jacketArmorIndex] = 2;
		else
			other->client->pers.inventory[oldArmorIndex] += 2;
	}
	else if (!oldArmorIndex)		// If player has no armor, just use it
		other->client->pers.inventory[ITEM_INDEX(entity->item)] = newInfo->baseCount;
	else {		// Use the better armor
		// Get info on the old armor
		if (oldArmorIndex == itemLocals.jacketArmorIndex)
			oldInfo = &jacketArmorInfo;
		else if (oldArmorIndex == itemLocals.combatArmorIndex)
			oldInfo = &combatArmorInfo;
		else
			oldInfo = &bodyArmorInfo;

		if (newInfo->normalProtection > oldInfo->normalProtection){
			// Calculate the new armor values
			salvage = oldInfo->normalProtection / newInfo->normalProtection;
			salvageCount = salvage * other->client->pers.inventory[oldArmorIndex];

			newCount = newInfo->baseCount + salvageCount;

			if (newCount > newInfo->maxCount)
				newCount = newInfo->maxCount;

			// Zero count of old armor so it goes away
			other->client->pers.inventory[oldArmorIndex] = 0;

			// Change armor to new item with computed value
			other->client->pers.inventory[ITEM_INDEX(entity->item)] = newCount;
		}
		else {
			// Calculate the new armor values
			salvage = newInfo->normalProtection / oldInfo->normalProtection;
			salvageCount = salvage * newInfo->baseCount;

			newCount = other->client->pers.inventory[oldArmorIndex] + salvageCount;

			if (newCount > oldInfo->maxCount)
				newCount = oldInfo->maxCount;

			// If we're already maxed out then we don't need the new armor
			if (other->client->pers.inventory[oldArmorIndex] >= newCount)
				return false;

			// Update current armor value
			other->client->pers.inventory[oldArmorIndex] = newCount;
		}
	}

	if (!(entity->spawnflags & DROPPED_ITEM) && (deathmatch->value))
		SG_SetRespawnTime(entity, 20.0f);

	return true;
}


/*
 ==================
 SG_FindPowerArmorByType
 ==================
*/
int SG_FindPowerArmorByType (edict_t *entity){

	if (!entity->client)
		return POWER_ARMOR_NONE;

	if (!(entity->flags & FL_POWER_ARMOR))
		return POWER_ARMOR_NONE;

	if (entity->client->pers.inventory[itemLocals.powerShieldIndex] > 0)
		return POWER_ARMOR_SHIELD;

	if (entity->client->pers.inventory[itemLocals.powerScreenIndex] > 0)
		return POWER_ARMOR_SCREEN;

	return POWER_ARMOR_NONE;
}

/*
 ==================
 SG_Use_PowerArmor
 ==================
*/
static void SG_Use_PowerArmor (edict_t *ent, gItem_t *item){

	int		index;

	if (ent->flags & FL_POWER_ARMOR){
		ent->flags &= ~FL_POWER_ARMOR;
		gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/power2.wav"), 1.0f, ATTN_NORM, 0.0f);
	}
	else {
		index = ITEM_INDEX(SG_FindItem("cells"));

		if (!ent->client->pers.inventory[index]){
			gi.cprintf(ent, PRINT_HIGH, "No cells for power armor.\n");
			return;
		}

		ent->flags |= FL_POWER_ARMOR;

		gi.sound(ent, CHAN_AUTO, gi.soundindex("misc/power1.wav"), 1.0f, ATTN_NORM, 0.0f);
	}
}

/*
 ==================
 SG_Pickup_PowerArmor
 ==================
*/
static qboolean SG_Pickup_PowerArmor (edict_t *entity, edict_t *other){

	int		quantity;

	quantity = other->client->pers.inventory[ITEM_INDEX(entity->item)];

	other->client->pers.inventory[ITEM_INDEX(entity->item)]++;

	if (deathmatch->value){
		if (!(entity->spawnflags & DROPPED_ITEM) )
			SG_SetRespawnTime(entity, entity->item->quantity);

		// Auto-use for DM only if we didn't already have one
		if (!quantity)
			entity->item->use(other, entity->item);
	}

	return true;
}

/*
 ==================
 SG_Drop_PowerArmor
 ==================
*/
static void SG_Drop_PowerArmor (edict_t *entity, gItem_t *item){

	if ((entity->flags & FL_POWER_ARMOR) && (entity->client->pers.inventory[ITEM_INDEX(item)] == 1))
		SG_Use_PowerArmor(entity, item);

	SG_Drop_General(entity, item);
}


/*
 ==============================================================================

 ITEM INTERACTION

 ==============================================================================
*/


/*
 ==================
 SG_Touch_Item
 ==================
*/
void SG_Touch_Item (edict_t *entity, edict_t *other, cplane_t *plane, csurface_t *surface){

	qboolean	taken;

	if (!other->client)
		return;
	if (other->health < 1)
		return;		// Dead people can't pickup
	if (!entity->item->pickup)
		return;		// Not a grabbable item?

	taken = entity->item->pickup(entity, other);

	if (taken){
		// Flash the screen
		other->client->bonus_alpha = 0.25f;	

		// Show icon and name on status bar
		other->client->ps.stats[STAT_PICKUP_ICON] = gi.imageindex(entity->item->icon);
		other->client->ps.stats[STAT_PICKUP_STRING] = CS_ITEMS + ITEM_INDEX(entity->item);
		other->client->pickup_msg_time = level.time + 3.0f;

		// Change selected item
		if (entity->item->use)
			other->client->pers.selected_item = other->client->ps.stats[STAT_SELECTED_ITEM] = ITEM_INDEX(entity->item);

		if (entity->item->pickup == SG_Pickup_Health){
			if (entity->count == 2)
				gi.sound(other, CHAN_ITEM, gi.soundindex("items/s_health.wav"), 1.0f, ATTN_NORM, 0.0f);
			else if (entity->count == 10)
				gi.sound(other, CHAN_ITEM, gi.soundindex("items/n_health.wav"), 1.0f, ATTN_NORM, 0.0f);
			else if (entity->count == 25)
				gi.sound(other, CHAN_ITEM, gi.soundindex("items/l_health.wav"), 1.0f, ATTN_NORM, 0.0f);
			else
				gi.sound(other, CHAN_ITEM, gi.soundindex("items/m_health.wav"), 1.0f, ATTN_NORM, 0.0f);
		}
		else if (entity->item->pickupSound)
			gi.sound(other, CHAN_ITEM, gi.soundindex(entity->item->pickupSound), 1.0f, ATTN_NORM, 0.0f);
	}

	if (!(entity->spawnflags & ITEM_TARGETS_USED)){
		SG_UseTargets(entity, other);
		entity->spawnflags |= ITEM_TARGETS_USED;
	}

	if (!taken)
		return;

	if (!((coop->value) && (entity->item->flags & IT_STAY_COOP)) || (entity->spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM))){
		if (entity->flags & FL_RESPAWN)
			entity->flags &= ~FL_RESPAWN;
		else
			SG_FreeEntity(entity);
	}
}

/*
 ==================
 SG_Drop_Temp_Touch
 ==================
*/
static void SG_Drop_Temp_Touch (edict_t *entity, edict_t *other, cplane_t *plane, csurface_t *surface){

	if (other == entity->owner)
		return;

	SG_Touch_Item(entity, other, plane, surface);
}

/*
 ==================
 SG_Drop_Make_Touchable
 ==================
*/
static void SG_Drop_Make_Touchable (edict_t *entity){

	entity->touch = SG_Touch_Item;

	if (deathmatch->value){
		entity->nextthink = level.time + 29.0f;
		entity->think = SG_FreeEntity;
	}
}

/*
 ==================
 SG_Drop_Item
 ==================
*/
edict_t *SG_Drop_Item (edict_t *entity, gItem_t *item){

	edict_t	*dropped;
	trace_t	trace;
	vec3_t	forward, right;
	vec3_t	offset;

	dropped = SG_AllocEntity();

	dropped->className = item->className;
	dropped->item = item;
	dropped->spawnflags = DROPPED_ITEM;
	dropped->s.effects = item->worldWodelFlags;
	dropped->s.renderfx = RF_GLOW;
	VectorSet(dropped->mins, -15.0f, -15.0f, -15.0f);
	VectorSet(dropped->maxs, 15.0f, 15.0f, 15.0f);
	gi.setmodel(dropped, dropped->item->worldModel);
	dropped->solid = SOLID_TRIGGER;
	dropped->movetype = MOVETYPE_TOSS;  
	dropped->touch = SG_Drop_Temp_Touch;
	dropped->owner = entity;

	if (entity->client){
		AngleVectors(entity->client->v_angle, forward, right, NULL);
		VectorSet(offset, 24.0f, 0.0f, -16.0f);

		SG_ProjectSource(entity->s.origin, offset, forward, right, dropped->s.origin);

		trace = gi.trace(entity->s.origin, dropped->mins, dropped->maxs, dropped->s.origin, entity, CONTENTS_SOLID);
		VectorCopy(trace.endpos, dropped->s.origin);
	}
	else {
		AngleVectors(entity->s.angles, forward, right, NULL);
		VectorCopy(entity->s.origin, dropped->s.origin);
	}

	VectorScale(forward, 100.0f, dropped->velocity);
	dropped->velocity[2] = 300.0f;

	dropped->think = SG_Drop_Make_Touchable;
	dropped->nextthink = level.time + 1.0f;

	gi.linkentity(dropped);

	return dropped;
}

/*
 ==================
 SG_Use_Item
 ==================
*/
static void SG_Use_Item (edict_t *entity, edict_t *other, edict_t *activator){

	entity->svflags &= ~SVF_NOCLIENT;
	entity->use = NULL;

	if (entity->spawnflags & ITEM_NO_TOUCH){
		entity->solid = SOLID_BBOX;
		entity->touch = NULL;
	}
	else {
		entity->solid = SOLID_TRIGGER;
		entity->touch = SG_Touch_Item;
	}

	gi.linkentity(entity);
}


/*
 ==============================================================================

 TODO

 ==============================================================================
*/


/*
 ==================
 SG_PrecacheItem

 Precaches all data needed for a given item.

 This will be called for each item spawned in a level,
 and for each item in each client's inventory.
 ==================
*/
void SG_PrecacheItem (gItem_t *item){

	char	*string, *start;
	char	data[MAX_QPATH];
	int		length;
	gItem_t	*ammo;

	if (!item)
		return;

	if (item->pickupSound)
		gi.soundindex(item->pickupSound);
	if (item->worldModel)
		gi.modelindex(item->worldModel);
	if (item->viewModel)
		gi.modelindex(item->viewModel);
	if (item->icon)
		gi.imageindex(item->icon);

	// Parse everything for its ammo
	if (item->ammo && item->ammo[0]){
		ammo = SG_FindItem(item->ammo);

		if (ammo != item)
			SG_PrecacheItem(ammo);
	}

	// Parse the space seperated precache string for other items
	string = item->precaches;

	if (!string || !string[0])
		return;

	while (*string){
		start = string;

		while (*string && *string != ' ')
			string++;

		length = string - start;

		if (length >= MAX_QPATH || length < 5)
			gi.error("SG_PrecacheItem: %s has bad precache string", item->className);

		memcpy(data, start, length);
		data[length] = 0;

		if (*string)
			string++;

		// Determine type based on extension
		if (!strcmp(data + length - 3, "md2"))
			gi.modelindex(data);
		else if (!strcmp(data + length - 3, "sp2"))
			gi.modelindex(data);
		else if (!strcmp(data + length - 3, "wav"))
			gi.soundindex(data);
		if (!strcmp(data + length - 3, "pcx"))
			gi.imageindex(data);
	}
}

/*
 ==================
 SG_SpawnItem_Think

 Drops an item to the floor
 ==================
*/
static void SG_SpawnItem_Think (edict_t *entity){

	float	*v;
	vec3_t	dest;
	trace_t	trace;

	v = SG_TempVector(-15.0f, -15.0f, -15.0f);
	VectorCopy(v, entity->mins);
	v = SG_TempVector(15.0f, 15.0f, 1.0f);
	VectorCopy(v, entity->maxs);

	if (entity->model)
		gi.setmodel(entity, entity->model);
	else
		gi.setmodel(entity, entity->item->worldModel);

	entity->solid = SOLID_TRIGGER;
	entity->movetype = MOVETYPE_TOSS;  
	entity->touch = SG_Touch_Item;

	v = SG_TempVector(0.0f, 0.0f, -128.0f);
	VectorAdd(entity->s.origin, v, dest);

	trace = gi.trace(entity->s.origin, entity->mins, entity->maxs, dest, entity, MASK_SOLID);
	
	if (trace.startsolid){
		gi.dprintf("droptofloor: %s startsolid at %s\n", entity->className, SG_VectorToString(entity->s.origin));
		SG_FreeEntity(entity);
		return;
	}

	VectorCopy(trace.endpos, entity->s.origin);

	if (entity->team){
		entity->flags &= ~FL_TEAMSLAVE;
		entity->chain = entity->teamchain;
		entity->teamchain = NULL;

		entity->svflags |= SVF_NOCLIENT;
		entity->solid = SOLID_NOT;

		if (entity == entity->teammaster){
			entity->nextthink = level.time + FRAMETIME;
			entity->think = SG_RespawnItem;
		}
	}

	if (entity->spawnflags & ITEM_NO_TOUCH){
		entity->solid = SOLID_BBOX;
		entity->touch = NULL;
		entity->s.effects &= ~EF_ROTATE;
		entity->s.renderfx &= ~RF_GLOW;
	}

	if (entity->spawnflags & ITEM_TRIGGER_SPAWN){
		entity->svflags |= SVF_NOCLIENT;
		entity->solid = SOLID_NOT;
		entity->use = SG_Use_Item;
	}

	gi.linkentity(entity);
}

/*
 ==================
 SG_SpawnItem

 Sets the clipping size and plants the object on the floor.

 Items can't be immediately dropped to the floor, because they might
 be on an entity that hasn't spawned yet.
 ==================
*/
void SG_SpawnItem (edict_t *entity, gItem_t *item){

	SG_PrecacheItem(item);

	if (entity->spawnflags){
		if (strcmp(entity->className, "key_power_cube") != 0){
			entity->spawnflags = 0;
			gi.dprintf("%s at %s has invalid spawnflags set\n", entity->className, SG_VectorToString(entity->s.origin));
		}
	}

	// Some items will be prevented in deathmatch
	if (deathmatch->value){
		if ((int)dmflags->value & DF_NO_ARMOR){
			if (item->pickup == SG_Pickup_Armor || item->pickup == SG_Pickup_PowerArmor){
				SG_FreeEntity(entity);
				return;
			}
		}

		if ((int)dmflags->value & DF_NO_ITEMS){
			if (item->pickup == SG_Pickup_Powerup){
				SG_FreeEntity(entity);
				return;
			}
		}

		if ((int)dmflags->value & DF_NO_HEALTH){
			if (item->pickup == SG_Pickup_Health || item->pickup == SG_Pickup_Adrenaline || item->pickup == SG_Pickup_AncientHead){
				SG_FreeEntity(entity);
				return;
			}
		}

		if ((int)dmflags->value & DF_INFINITE_AMMO){
			if ( (item->flags == IT_AMMO) || (!strcmp(entity->className, "weapon_bfg"))){
				SG_FreeEntity(entity);
				return;
			}
		}
	}

	if (coop->value && (!strcmp(entity->className, "key_power_cube"))){
		entity->spawnflags |= (1 << (8 + level.power_cubes));
		level.power_cubes++;
	}

	// Don't let them drop items that stay in a coop game
	if ((coop->value) && (item->flags & IT_STAY_COOP))
		item->drop = NULL;

	entity->item = item;
	entity->nextthink = level.time + 2.0f * FRAMETIME;		// Items start after other solids
	entity->think = SG_SpawnItem_Think;
	entity->s.effects = item->worldWodelFlags;
	entity->s.renderfx = RF_GLOW;

	if (entity->model)
		gi.modelindex(entity->model);
}


/*
 ==============================================================================

 ITEM LIST

 ==============================================================================
*/

gItem_t	sg_itemList[] = {
	{
		NULL
	},	// leave index 0 alone

	//
	// ARMOR
	//

/*QUAKED item_armor_body (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_armor_body", 
		SG_Pickup_Armor,
		NULL,
		NULL,
		NULL,
		"misc/ar1_pkup.wav",
		"models/items/armor/body/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"i_bodyarmor",
/* pickup */	"Body Armor",
/* width */		3,
		0,
		NULL,
		IT_ARMOR,
		0,
		&bodyArmorInfo,
		ARMOR_BODY,
/* precache */ ""
	},

/*QUAKED item_armor_combat (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_armor_combat", 
		SG_Pickup_Armor,
		NULL,
		NULL,
		NULL,
		"misc/ar1_pkup.wav",
		"models/items/armor/combat/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"i_combatarmor",
/* pickup */	"Combat Armor",
/* width */		3,
		0,
		NULL,
		IT_ARMOR,
		0,
		&combatArmorInfo,
		ARMOR_COMBAT,
/* precache */ ""
	},

/*QUAKED item_armor_jacket (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_armor_jacket", 
		SG_Pickup_Armor,
		NULL,
		NULL,
		NULL,
		"misc/ar1_pkup.wav",
		"models/items/armor/jacket/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"i_jacketarmor",
/* pickup */	"Jacket Armor",
/* width */		3,
		0,
		NULL,
		IT_ARMOR,
		0,
		&jacketArmorInfo,
		ARMOR_JACKET,
/* precache */ ""
	},

/*QUAKED item_armor_shard (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_armor_shard", 
		SG_Pickup_Armor,
		NULL,
		NULL,
		NULL,
		"misc/ar2_pkup.wav",
		"models/items/armor/shard/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"i_jacketarmor",
/* pickup */	"Armor Shard",
/* width */		3,
		0,
		NULL,
		IT_ARMOR,
		0,
		NULL,
		ARMOR_SHARD,
/* precache */ ""
	},


/*QUAKED item_power_screen (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_power_screen", 
		SG_Pickup_PowerArmor,
		SG_Use_PowerArmor,
		SG_Drop_PowerArmor,
		NULL,
		"misc/ar3_pkup.wav",
		"models/items/armor/screen/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"i_powerscreen",
/* pickup */	"Power Screen",
/* width */		0,
		60,
		NULL,
		IT_ARMOR,
		0,
		NULL,
		0,
/* precache */ ""
	},

/*QUAKED item_power_shield (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_power_shield",
		SG_Pickup_PowerArmor,
		SG_Use_PowerArmor,
		SG_Drop_PowerArmor,
		NULL,
		"misc/ar3_pkup.wav",
		"models/items/armor/shield/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"i_powershield",
/* pickup */	"Power Shield",
/* width */		0,
		60,
		NULL,
		IT_ARMOR,
		0,
		NULL,
		0,
/* precache */ "misc/power2.wav misc/power1.wav"
	},


	//
	// WEAPONS 
	//

/* weapon_blaster (.3 .3 1) (-16 -16 -16) (16 16 16)
always owned, never in the world
*/
	{
		"weapon_blaster", 
		NULL,
		SG_UseWeapon,
		NULL,
		SG_BlasterWeapon,
		"misc/w_pkup.wav",
		NULL, 0,
		"models/weapons/v_blast/tris.md2",
/* icon */		"w_blaster",
/* pickup */	"Blaster",
		0,
		0,
		NULL,
		IT_WEAPON|IT_STAY_COOP,
		WEAP_BLASTER,
		NULL,
		0,
/* precache */ "weapons/blastf1a.wav misc/lasfly.wav"
	},

/*QUAKED weapon_shotgun (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_shotgun", 
		SG_PickupWeapon,
		SG_UseWeapon,
		SG_DropWeapon,
		SG_ShotgunWeapon,
		"misc/w_pkup.wav",
		"models/weapons/g_shotg/tris.md2", EF_ROTATE,
		"models/weapons/v_shotg/tris.md2",
/* icon */		"w_shotgun",
/* pickup */	"Shotgun",
		0,
		1,
		"Shells",
		IT_WEAPON|IT_STAY_COOP,
		WEAP_SHOTGUN,
		NULL,
		0,
/* precache */ "weapons/shotgf1b.wav weapons/shotgr1b.wav"
	},

/*QUAKED weapon_supershotgun (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_supershotgun", 
		SG_PickupWeapon,
		SG_UseWeapon,
		SG_DropWeapon,
		SG_SupershotgunWeapon,
		"misc/w_pkup.wav",
		"models/weapons/g_shotg2/tris.md2", EF_ROTATE,
		"models/weapons/v_shotg2/tris.md2",
/* icon */		"w_sshotgun",
/* pickup */	"Super Shotgun",
		0,
		2,
		"Shells",
		IT_WEAPON|IT_STAY_COOP,
		WEAP_SUPERSHOTGUN,
		NULL,
		0,
/* precache */ "weapons/sshotf1b.wav"
	},

/*QUAKED weapon_machinegun (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_machinegun", 
		SG_PickupWeapon,
		SG_UseWeapon,
		SG_DropWeapon,
		SG_MachinegunWeapon,
		"misc/w_pkup.wav",
		"models/weapons/g_machn/tris.md2", EF_ROTATE,
		"models/weapons/v_machn/tris.md2",
/* icon */		"w_machinegun",
/* pickup */	"Machinegun",
		0,
		1,
		"Bullets",
		IT_WEAPON|IT_STAY_COOP,
		WEAP_MACHINEGUN,
		NULL,
		0,
/* precache */ "weapons/machgf1b.wav weapons/machgf2b.wav weapons/machgf3b.wav weapons/machgf4b.wav weapons/machgf5b.wav"
	},

/*QUAKED weapon_chaingun (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_chaingun", 
		SG_PickupWeapon,
		SG_UseWeapon,
		SG_DropWeapon,
		SG_ChaingunWeapon,
		"misc/w_pkup.wav",
		"models/weapons/g_chain/tris.md2", EF_ROTATE,
		"models/weapons/v_chain/tris.md2",
/* icon */		"w_chaingun",
/* pickup */	"Chaingun",
		0,
		1,
		"Bullets",
		IT_WEAPON|IT_STAY_COOP,
		WEAP_CHAINGUN,
		NULL,
		0,
/* precache */ "weapons/chngnu1a.wav weapons/chngnl1a.wav weapons/machgf3b.wav` weapons/chngnd1a.wav"
	},

/*QUAKED ammo_grenades (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"ammo_grenades",
		SG_Pickup_Ammo,
		SG_UseWeapon,
		SG_Drop_Ammo,
		SG_GrenadeWeapon,
		"misc/am_pkup.wav",
		"models/items/ammo/grenades/medium/tris.md2", 0,
		"models/weapons/v_handgr/tris.md2",
/* icon */		"a_grenades",
/* pickup */	"Grenades",
/* width */		3,
		5,
		"grenades",
		IT_AMMO|IT_WEAPON,
		WEAP_GRENADES,
		NULL,
		AMMO_GRENADES,
/* precache */ "weapons/hgrent1a.wav weapons/hgrena1b.wav weapons/hgrenc1b.wav weapons/hgrenb1a.wav weapons/hgrenb2a.wav "
	},

/*QUAKED weapon_grenadelauncher (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_grenadelauncher",
		SG_PickupWeapon,
		SG_UseWeapon,
		SG_DropWeapon,
		SG_GrenadeLauncherWeapon,
		"misc/w_pkup.wav",
		"models/weapons/g_launch/tris.md2", EF_ROTATE,
		"models/weapons/v_launch/tris.md2",
/* icon */		"w_glauncher",
/* pickup */	"Grenade Launcher",
		0,
		1,
		"Grenades",
		IT_WEAPON|IT_STAY_COOP,
		WEAP_GRENADELAUNCHER,
		NULL,
		0,
/* precache */ "models/objects/grenade/tris.md2 weapons/grenlf1a.wav weapons/grenlr1b.wav weapons/grenlb1b.wav"
	},

/*QUAKED weapon_rocketlauncher (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_rocketlauncher",
		SG_PickupWeapon,
		SG_UseWeapon,
		SG_DropWeapon,
		SG_RocketLauncherWeapon,
		"misc/w_pkup.wav",
		"models/weapons/g_rocket/tris.md2", EF_ROTATE,
		"models/weapons/v_rocket/tris.md2",
/* icon */		"w_rlauncher",
/* pickup */	"Rocket Launcher",
		0,
		1,
		"Rockets",
		IT_WEAPON|IT_STAY_COOP,
		WEAP_ROCKETLAUNCHER,
		NULL,
		0,
/* precache */ "models/objects/rocket/tris.md2 weapons/rockfly.wav weapons/rocklf1a.wav weapons/rocklr1b.wav models/objects/debris2/tris.md2"
	},

/*QUAKED weapon_hyperblaster (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_hyperblaster", 
		SG_PickupWeapon,
		SG_UseWeapon,
		SG_DropWeapon,
		SG_HyperBlasterWeapon,
		"misc/w_pkup.wav",
		"models/weapons/g_hyperb/tris.md2", EF_ROTATE,
		"models/weapons/v_hyperb/tris.md2",
/* icon */		"w_hyperblaster",
/* pickup */	"HyperBlaster",
		0,
		1,
		"Cells",
		IT_WEAPON|IT_STAY_COOP,
		WEAP_HYPERBLASTER,
		NULL,
		0,
/* precache */ "weapons/hyprbu1a.wav weapons/hyprbl1a.wav weapons/hyprbf1a.wav weapons/hyprbd1a.wav misc/lasfly.wav"
	},

/*QUAKED weapon_railgun (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_railgun", 
		SG_PickupWeapon,
		SG_UseWeapon,
		SG_DropWeapon,
		SG_RailgunWeapon,
		"misc/w_pkup.wav",
		"models/weapons/g_rail/tris.md2", EF_ROTATE,
		"models/weapons/v_rail/tris.md2",
/* icon */		"w_railgun",
/* pickup */	"Railgun",
		0,
		1,
		"Slugs",
		IT_WEAPON|IT_STAY_COOP,
		WEAP_RAILGUN,
		NULL,
		0,
/* precache */ "weapons/rg_hum.wav"
	},

/*QUAKED weapon_bfg (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"weapon_bfg",
		SG_PickupWeapon,
		SG_UseWeapon,
		SG_DropWeapon,
		SG_BFGWeapon,
		"misc/w_pkup.wav",
		"models/weapons/g_bfg/tris.md2", EF_ROTATE,
		"models/weapons/v_bfg/tris.md2",
/* icon */		"w_bfg",
/* pickup */	"BFG10K",
		0,
		50,
		"Cells",
		IT_WEAPON|IT_STAY_COOP,
		WEAP_BFG,
		NULL,
		0,
/* precache */ "sprites/s_bfg1.sp2 sprites/s_bfg2.sp2 sprites/s_bfg3.sp2 weapons/bfg__f1y.wav weapons/bfg__l1a.wav weapons/bfg__x1b.wav weapons/bfg_hum.wav"
	},

	//
	// AMMO ITEMS
	//

/*QUAKED ammo_shells (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"ammo_shells",
		SG_Pickup_Ammo,
		NULL,
		SG_Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/items/ammo/shells/medium/tris.md2", 0,
		NULL,
/* icon */		"a_shells",
/* pickup */	"Shells",
/* width */		3,
		10,
		NULL,
		IT_AMMO,
		0,
		NULL,
		AMMO_SHELLS,
/* precache */ ""
	},

/*QUAKED ammo_bullets (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"ammo_bullets",
		SG_Pickup_Ammo,
		NULL,
		SG_Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/items/ammo/bullets/medium/tris.md2", 0,
		NULL,
/* icon */		"a_bullets",
/* pickup */	"Bullets",
/* width */		3,
		50,
		NULL,
		IT_AMMO,
		0,
		NULL,
		AMMO_BULLETS,
/* precache */ ""
	},

/*QUAKED ammo_cells (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"ammo_cells",
		SG_Pickup_Ammo,
		NULL,
		SG_Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/items/ammo/cells/medium/tris.md2", 0,
		NULL,
/* icon */		"a_cells",
/* pickup */	"Cells",
/* width */		3,
		50,
		NULL,
		IT_AMMO,
		0,
		NULL,
		AMMO_CELLS,
/* precache */ ""
	},

/*QUAKED ammo_rockets (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"ammo_rockets",
		SG_Pickup_Ammo,
		NULL,
		SG_Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/items/ammo/rockets/medium/tris.md2", 0,
		NULL,
/* icon */		"a_rockets",
/* pickup */	"Rockets",
/* width */		3,
		5,
		NULL,
		IT_AMMO,
		0,
		NULL,
		AMMO_ROCKETS,
/* precache */ ""
	},

/*QUAKED ammo_slugs (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"ammo_slugs",
		SG_Pickup_Ammo,
		NULL,
		SG_Drop_Ammo,
		NULL,
		"misc/am_pkup.wav",
		"models/items/ammo/slugs/medium/tris.md2", 0,
		NULL,
/* icon */		"a_slugs",
/* pickup */	"Slugs",
/* width */		3,
		10,
		NULL,
		IT_AMMO,
		0,
		NULL,
		AMMO_SLUGS,
/* precache */ ""
	},


	//
	// POWERUP ITEMS
	//
/*QUAKED item_quad (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_quad", 
		SG_Pickup_Powerup,
		SG_Use_Quad,
		SG_Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/quaddama/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"p_quad",
/* pickup */	"Quad Damage",
/* width */		2,
		60,
		NULL,
		IT_POWERUP,
		0,
		NULL,
		0,
/* precache */ "items/damage.wav items/damage2.wav items/damage3.wav"
	},

/*QUAKED item_invulnerability (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_invulnerability",
		SG_Pickup_Powerup,
		SG_Use_Invulnerability,
		SG_Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/invulner/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"p_invulnerability",
/* pickup */	"Invulnerability",
/* width */		2,
		300,
		NULL,
		IT_POWERUP,
		0,
		NULL,
		0,
/* precache */ "items/protect.wav items/protect2.wav items/protect4.wav"
	},

/*QUAKED item_silencer (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_silencer",
		SG_Pickup_Powerup,
		SG_Use_Silencer,
		SG_Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/silencer/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"p_silencer",
/* pickup */	"Silencer",
/* width */		2,
		60,
		NULL,
		IT_POWERUP,
		0,
		NULL,
		0,
/* precache */ ""
	},

/*QUAKED item_breather (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_breather",
		SG_Pickup_Powerup,
		SG_Use_Breather,
		SG_Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/breather/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"p_rebreather",
/* pickup */	"Rebreather",
/* width */		2,
		60,
		NULL,
		IT_STAY_COOP|IT_POWERUP,
		0,
		NULL,
		0,
/* precache */ "items/airout.wav"
	},

/*QUAKED item_enviro (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_enviro",
		SG_Pickup_Powerup,
		SG_Use_EnvironmentSuit,
		SG_Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/enviro/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"p_envirosuit",
/* pickup */	"Environment Suit",
/* width */		2,
		60,
		NULL,
		IT_STAY_COOP|IT_POWERUP,
		0,
		NULL,
		0,
/* precache */ "items/airout.wav"
	},

/*QUAKED item_ancient_head (.3 .3 1) (-16 -16 -16) (16 16 16)
Special item that gives +2 to maximum health
*/
	{
		"item_ancient_head",
		SG_Pickup_AncientHead,
		NULL,
		NULL,
		NULL,
		"items/pkup.wav",
		"models/items/c_head/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"i_fixme",
/* pickup */	"Ancient Head",
/* width */		2,
		60,
		NULL,
		0,
		0,
		NULL,
		0,
/* precache */ ""
	},

/*QUAKED item_adrenaline (.3 .3 1) (-16 -16 -16) (16 16 16)
gives +1 to maximum health
*/
	{
		"item_adrenaline",
		SG_Pickup_Adrenaline,
		NULL,
		NULL,
		NULL,
		"items/pkup.wav",
		"models/items/adrenal/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"p_adrenaline",
/* pickup */	"Adrenaline",
/* width */		2,
		60,
		NULL,
		0,
		0,
		NULL,
		0,
/* precache */ ""
	},

/*QUAKED item_bandolier (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_bandolier",
		SG_Pickup_Bandolier,
		NULL,
		NULL,
		NULL,
		"items/pkup.wav",
		"models/items/band/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"p_bandolier",
/* pickup */	"Bandolier",
/* width */		2,
		60,
		NULL,
		0,
		0,
		NULL,
		0,
/* precache */ ""
	},

/*QUAKED item_pack (.3 .3 1) (-16 -16 -16) (16 16 16)
*/
	{
		"item_pack",
		SG_Pickup_Pack,
		NULL,
		NULL,
		NULL,
		"items/pkup.wav",
		"models/items/pack/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"i_pack",
/* pickup */	"Ammo Pack",
/* width */		2,
		180,
		NULL,
		0,
		0,
		NULL,
		0,
/* precache */ ""
	},

	//
	// KEYS
	//
/*QUAKED key_data_cd (0 .5 .8) (-16 -16 -16) (16 16 16)
key for computer centers
*/
	{
		"key_data_cd",
		SG_Pickup_Key,
		NULL,
		SG_Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/data_cd/tris.md2", EF_ROTATE,
		NULL,
		"k_datacd",
		"Data CD",
		2,
		0,
		NULL,
		IT_STAY_COOP|IT_KEY,
		0,
		NULL,
		0,
/* precache */ ""
	},

/*QUAKED key_power_cube (0 .5 .8) (-16 -16 -16) (16 16 16) TRIGGER_SPAWN NO_TOUCH
warehouse circuits
*/
	{
		"key_power_cube",
		SG_Pickup_Key,
		NULL,
		SG_Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/power/tris.md2", EF_ROTATE,
		NULL,
		"k_powercube",
		"Power Cube",
		2,
		0,
		NULL,
		IT_STAY_COOP|IT_KEY,
		0,
		NULL,
		0,
/* precache */ ""
	},

/*QUAKED key_pyramid (0 .5 .8) (-16 -16 -16) (16 16 16)
key for the entrance of jail3
*/
	{
		"key_pyramid",
		SG_Pickup_Key,
		NULL,
		SG_Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/pyramid/tris.md2", EF_ROTATE,
		NULL,
		"k_pyramid",
		"Pyramid Key",
		2,
		0,
		NULL,
		IT_STAY_COOP|IT_KEY,
		0,
		NULL,
		0,
/* precache */ ""
	},

/*QUAKED key_data_spinner (0 .5 .8) (-16 -16 -16) (16 16 16)
key for the city computer
*/
	{
		"key_data_spinner",
		SG_Pickup_Key,
		NULL,
		SG_Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/spinner/tris.md2", EF_ROTATE,
		NULL,
		"k_dataspin",
		"Data Spinner",
		2,
		0,
		NULL,
		IT_STAY_COOP|IT_KEY,
		0,
		NULL,
		0,
/* precache */ ""
	},

/*QUAKED key_pass (0 .5 .8) (-16 -16 -16) (16 16 16)
security pass for the security level
*/
	{
		"key_pass",
		SG_Pickup_Key,
		NULL,
		SG_Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/pass/tris.md2", EF_ROTATE,
		NULL,
		"k_security",
		"Security Pass",
		2,
		0,
		NULL,
		IT_STAY_COOP|IT_KEY,
		0,
		NULL,
		0,
/* precache */ ""
	},

/*QUAKED key_blue_key (0 .5 .8) (-16 -16 -16) (16 16 16)
normal door key - blue
*/
	{
		"key_blue_key",
		SG_Pickup_Key,
		NULL,
		SG_Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/key/tris.md2", EF_ROTATE,
		NULL,
		"k_bluekey",
		"Blue Key",
		2,
		0,
		NULL,
		IT_STAY_COOP|IT_KEY,
		0,
		NULL,
		0,
/* precache */ ""
	},

/*QUAKED key_red_key (0 .5 .8) (-16 -16 -16) (16 16 16)
normal door key - red
*/
	{
		"key_red_key",
		SG_Pickup_Key,
		NULL,
		SG_Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/red_key/tris.md2", EF_ROTATE,
		NULL,
		"k_redkey",
		"Red Key",
		2,
		0,
		NULL,
		IT_STAY_COOP|IT_KEY,
		0,
		NULL,
		0,
/* precache */ ""
	},

/*QUAKED key_commander_head (0 .5 .8) (-16 -16 -16) (16 16 16)
tank commander's head
*/
	{
		"key_commander_head",
		SG_Pickup_Key,
		NULL,
		SG_Drop_General,
		NULL,
		"items/pkup.wav",
		"models/monsters/commandr/head/tris.md2", EF_GIB,
		NULL,
/* icon */		"k_comhead",
/* pickup */	"Commander's Head",
/* width */		2,
		0,
		NULL,
		IT_STAY_COOP|IT_KEY,
		0,
		NULL,
		0,
/* precache */ ""
	},

/*QUAKED key_airstrike_target (0 .5 .8) (-16 -16 -16) (16 16 16)
tank commander's head
*/
	{
		"key_airstrike_target",
		SG_Pickup_Key,
		NULL,
		SG_Drop_General,
		NULL,
		"items/pkup.wav",
		"models/items/keys/target/tris.md2", EF_ROTATE,
		NULL,
/* icon */		"i_airstrike",
/* pickup */	"Airstrike Marker",
/* width */		2,
		0,
		NULL,
		IT_STAY_COOP|IT_KEY,
		0,
		NULL,
		0,
/* precache */ ""
	},

	{
		NULL,
		SG_Pickup_Health,
		NULL,
		NULL,
		NULL,
		"items/pkup.wav",
		NULL, 0,
		NULL,
/* icon */		"i_health",
/* pickup */	"Health",
/* width */		3,
		0,
		NULL,
		0,
		0,
		NULL,
		0,
/* precache */ "items/s_health.wav items/n_health.wav items/l_health.wav items/m_health.wav"
	},

	// end of list marker
	{NULL}
};


/*
 ==================
 SG_SetItemNames

 Called by worldspawn
 ==================
*/
void SG_SetItemNames (){

	int		i;
	gItem_t	*item;

	for (i = 0; i < game.num_items; i++){
		item = &sg_itemList[i];
		gi.configstring(CS_ITEMS + i, item->pickupName);
	}

	itemLocals.jacketArmorIndex = ITEM_INDEX(SG_FindItem("Jacket Armor"));
	itemLocals.combatArmorIndex = ITEM_INDEX(SG_FindItem("Combat Armor"));
	itemLocals.bodyArmorIndex   = ITEM_INDEX(SG_FindItem("Body Armor"));
	itemLocals.powerScreenIndex = ITEM_INDEX(SG_FindItem("Power Screen"));
	itemLocals.powerShieldIndex = ITEM_INDEX(SG_FindItem("Power Shield"));
}

/*
 ==============================================================================

 ITEM_HEALTH

 ==============================================================================
*/


/*
 ==================
 SG_ItemHealth_Spawn
 ==================
*/
void SG_ItemHealth_Spawn (edict_t *self){

	if (deathmatch->value && ((int)dmflags->value & DF_NO_HEALTH)){
		SG_FreeEntity(self);
		return;
	}

	self->model = "models/items/healing/medium/tris.md2";
	self->count = 10;
	SG_SpawnItem(self, SG_FindItem("Health"));
	gi.soundindex("items/n_health.wav");
}


/*
 ==============================================================================

 ITEM_HEALTH_SMALL

 ==============================================================================
*/


/*
 ==================
 SG_ItemHealthSmall_Spawn
 ==================
*/
void SG_ItemHealthSmall_Spawn (edict_t *self){

	if (deathmatch->value && ((int)dmflags->value & DF_NO_HEALTH)){
		SG_FreeEntity(self);
		return;
	}

	self->model = "models/items/healing/stimpack/tris.md2";
	self->count = 2;
	SG_SpawnItem(self, SG_FindItem("Health"));
	self->style = HEALTH_IGNORE_MAX;
	gi.soundindex("items/s_health.wav");
}


/*
 ==============================================================================

 ITEM_HEALTH_LARGE

 ==============================================================================
*/


/*
 ==================
 SG_ItemHealthLarge_Spawn
 ==================
*/
void SG_ItemHealthLarge_Spawn (edict_t *self){

	if (deathmatch->value && ((int)dmflags->value & DF_NO_HEALTH)){
		SG_FreeEntity(self);
		return;
	}

	self->model = "models/items/healing/large/tris.md2";
	self->count = 25;
	SG_SpawnItem(self, SG_FindItem("Health"));
	gi.soundindex("items/l_health.wav");
}

/*
 ==============================================================================

 ITEM_HEALTH_MEGA

 ==============================================================================
*/


/*
 ==================
 SG_ItemHealthMega_Spawn
 ==================
*/
void SG_ItemHealthMega_Spawn (edict_t *self){

	if (deathmatch->value && ((int)dmflags->value & DF_NO_HEALTH)){
		SG_FreeEntity(self);
		return;
	}

	self->model = "models/items/mega_h/tris.md2";
	self->count = 100;
	SG_SpawnItem(self, SG_FindItem("Health"));
	gi.soundindex("items/m_health.wav");
	self->style = HEALTH_IGNORE_MAX|HEALTH_TIMED;
}


/*
 ==============================================================================

 INITIALIZATION AND SHUTDOWN

 ==============================================================================
*/


/*
 ==================
 SG_InitItems
 ==================
*/
void SG_InitItems (){

	game.num_items = sizeof(sg_itemList) / sizeof(sg_itemList[0]) - 1;
}