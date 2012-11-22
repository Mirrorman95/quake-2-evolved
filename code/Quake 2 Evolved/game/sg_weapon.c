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
// sg_weapon.c - Weapons
//


#include "g_local.h"
#include "m_player.h"


static qboolean	is_quad;
static byte		is_silenced;

static void SG_FireGrenadeWeapon (edict_t *entity, qboolean held);


/*
 ==================
 SG_ProjectWeaponSource

 The source origin from which to project
 ==================
*/
static void SG_ProjectWeaponSource (gclient_t *client, vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result){

	vec3_t	dist;

	VectorCopy(distance, dist);

	if (client->pers.hand == LEFT_HANDED)
		dist[1] *= -1.0f;
	else if (client->pers.hand == CENTER_HANDED)
		dist[1] = 0.0f;

	SG_ProjectSource(point, dist, forward, right, result);
}


/*
 ==============================================================================

 FRAMEWORK

 ==============================================================================
*/


/*
 ==================
 SG_WeaponThinkFramework

 Called by SG_ClientBeginServerFrame and SG_ClientThinkFramework
 ==================
*/
void SG_WeaponThinkFramework (edict_t *entity){

	// If just died, put the weapon away
	if (entity->health < 1){
		entity->client->newweapon = NULL;
		SG_ChangeWeapon(entity);
	}

	// Call active weapon think routine
	if (entity->client->pers.weapon && entity->client->pers.weapon->weaponthink){
		is_quad = (entity->client->quad_framenum > level.framenum);

		if (entity->client->silencer_shots)
			is_silenced = MZ_SILENCED;
		else
			is_silenced = 0;

		entity->client->pers.weapon->weaponthink(entity);
	}
}


/*
 ==============================================================================

 CLIENT WEAPON ACTIONS

 ==============================================================================
*/


/*
 ==================
 SG_PickupWeapon
 ==================
*/
qboolean SG_PickupWeapon (edict_t *entity, edict_t *other){

	int		index;
	gItem_t	*ammo;

	index = ITEM_INDEX(entity->item);

	if ((((int)(dmflags->value) & DF_WEAPONS_STAY) || coop->value) && other->client->pers.inventory[index]){
		if (!(entity->spawnflags & (DROPPED_ITEM | DROPPED_PLAYER_ITEM)))
			return false;	// Leave the weapon for others to pickup
	}

	other->client->pers.inventory[index]++;

	if (!(entity->spawnflags & DROPPED_ITEM)){
		// Give them some ammo with it
		ammo = SG_FindItem(entity->item->ammo);

		if ((int)dmflags->value & DF_INFINITE_AMMO)
			SG_Add_Ammo(other, ammo, 1000);
		else
			SG_Add_Ammo(other, ammo, ammo->quantity);

		if (!(entity->spawnflags & DROPPED_PLAYER_ITEM)){
			if (deathmatch->value){
				if ((int)(dmflags->value) & DF_WEAPONS_STAY)
					entity->flags |= FL_RESPAWN;
				else
					SG_SetRespawnTime(entity, 30.0f);
			}

			if (coop->value)
				entity->flags |= FL_RESPAWN;
		}
	}

	if (other->client->pers.weapon != entity->item && (other->client->pers.inventory[index] == 1) && (!deathmatch->value || other->client->pers.weapon == SG_FindItem("blaster")))
		other->client->newweapon = entity->item;

	return true;
}

/*
 ==================
 SG_ChangeWeapon

 The old weapon has been dropped all the way, so make the new one
 current
 ==================
*/
void SG_ChangeWeapon (edict_t *entity){

	int		i;

	if (entity->client->grenade_time){
		entity->client->grenade_time = level.time;
		entity->client->weapon_sound = 0;

		SG_FireGrenadeWeapon(entity, false);

		entity->client->grenade_time = 0.0f;
	}

	entity->client->pers.lastweapon = entity->client->pers.weapon;
	entity->client->pers.weapon = entity->client->newweapon;
	entity->client->newweapon = NULL;
	entity->client->machinegun_shots = 0;

	// Set visible model
	if (entity->s.modelindex == 255){
		if (entity->client->pers.weapon)
			i = ((entity->client->pers.weapon->weaponModel & 0xFF) << 8);
		else
			i = 0;

		entity->s.skinnum = (entity - g_edicts - 1) | i;
	}

	if (entity->client->pers.weapon && entity->client->pers.weapon->ammo)
		entity->client->ammo_index = ITEM_INDEX(SG_FindItem(entity->client->pers.weapon->ammo));
	else
		entity->client->ammo_index = 0;

	// Dead
	if (!entity->client->pers.weapon){
		entity->client->ps.gunindex = 0;
		return;
	}

	entity->client->weaponstate = WEAPON_ACTIVATING;
	entity->client->ps.gunframe = 0;
	entity->client->ps.gunindex = gi.modelindex(entity->client->pers.weapon->viewModel);

	entity->client->anim_priority = ANIM_PAIN;

	if (entity->client->ps.pmove.pm_flags & PMF_DUCKED){
		entity->s.frame = FRAME_crpain1;
		entity->client->anim_end = FRAME_crpain4;
	}
	else {
		entity->s.frame = FRAME_pain301;
		entity->client->anim_end = FRAME_pain304;	
	}
}

/*
 ==================
 SG_UseWeapon

 Make the weapon ready if there is ammo
 ==================
*/
void SG_UseWeapon (edict_t *entity, gItem_t *item){

	gItem_t	*ammoItem;
	int		ammoIndex;

	// See if we're already using it
	if (item == entity->client->pers.weapon)
		return;

	if (item->ammo && !g_select_empty->value && !(item->flags & IT_AMMO)){
		ammoItem = SG_FindItem(item->ammo);
		ammoIndex = ITEM_INDEX(ammoItem);

		if (!entity->client->pers.inventory[ammoIndex]){
			gi.cprintf(entity, PRINT_HIGH, "No %s for %s.\n", ammoItem->pickupName, item->pickupName);
			return;
		}

		if (entity->client->pers.inventory[ammoIndex] < item->quantity){
			gi.cprintf(entity, PRINT_HIGH, "Not enough %s for %s.\n", ammoItem->pickupName, item->pickupName);
			return;
		}
	}

	// Change to this weapon when down
	entity->client->newweapon = item;
}

/*
 ==================
 SG_DropWeapon
 ==================
*/
void SG_DropWeapon (edict_t *entity, gItem_t *item){

	int		index;

	if ((int)(dmflags->value) & DF_WEAPONS_STAY)
		return;

	index = ITEM_INDEX(item);

	// See if we're already using it
	if (((item == entity->client->pers.weapon) || (item == entity->client->newweapon))&& (entity->client->pers.inventory[index] == 1)){
		gi.cprintf(entity, PRINT_HIGH, "Can't drop current weapon\n");
		return;
	}

	SG_Drop_Item(entity, item);

	entity->client->pers.inventory[index]--;
}


/*
 ==============================================================================

 GENERIC WEAPON ROUTINE

 ==============================================================================
*/

#define FRAME_FIRE_FIRST		(FRAME_ACTIVATE_LAST + 1)
#define FRAME_IDLE_FIRST		(FRAME_FIRE_LAST + 1)
#define FRAME_DEACTIVATE_FIRST	(FRAME_IDLE_LAST + 1)


/*
 ==================
 SG_NoAmmoWeaponChange
 ==================
*/
static void SG_NoAmmoWeaponChange (edict_t *entity){

	if (entity->client->pers.inventory[ITEM_INDEX(SG_FindItem("slugs"))] && entity->client->pers.inventory[ITEM_INDEX(SG_FindItem("railgun"))]){
		entity->client->newweapon = SG_FindItem("railgun");
		return;
	}

	if (entity->client->pers.inventory[ITEM_INDEX(SG_FindItem("cells"))] && entity->client->pers.inventory[ITEM_INDEX(SG_FindItem("hyperblaster"))]){
		entity->client->newweapon = SG_FindItem("hyperblaster");
		return;
	}

	if (entity->client->pers.inventory[ITEM_INDEX(SG_FindItem("bullets"))] && entity->client->pers.inventory[ITEM_INDEX(SG_FindItem("chaingun"))]){
		entity->client->newweapon = SG_FindItem("chaingun");
		return;
	}

	if (entity->client->pers.inventory[ITEM_INDEX(SG_FindItem("bullets"))] && entity->client->pers.inventory[ITEM_INDEX(SG_FindItem("machinegun"))]){
		entity->client->newweapon = SG_FindItem("machinegun");
		return;
	}

	if (entity->client->pers.inventory[ITEM_INDEX(SG_FindItem("shells"))] > 1 && entity->client->pers.inventory[ITEM_INDEX(SG_FindItem("super shotgun"))]){
		entity->client->newweapon = SG_FindItem("super shotgun");
		return;
	}

	if (entity->client->pers.inventory[ITEM_INDEX(SG_FindItem("shells"))] && entity->client->pers.inventory[ITEM_INDEX(SG_FindItem("shotgun"))]){
		entity->client->newweapon = SG_FindItem("shotgun");
		return;
	}

	entity->client->newweapon = SG_FindItem("blaster");
}

/*
 ==================
 SG_GenericWeapon

 A generic function to handle the basics of weapon thinking
 ==================
*/
static void SG_GenericWeapon (edict_t *entity, int FRAME_ACTIVATE_LAST, int FRAME_FIRE_LAST, int FRAME_IDLE_LAST, int FRAME_DEACTIVATE_LAST, int *pauseFrames, int *fireFrames, void (*fire)(edict_t *entity)){

	int		i;

	if (entity->deadflag || entity->s.modelindex != 255)
		return;		// View weapon animations screw up corpses

	if (entity->client->weaponstate == WEAPON_DROPPING){
		if (entity->client->ps.gunframe == FRAME_DEACTIVATE_LAST){
			SG_ChangeWeapon (entity);
			return;
		}
		else if ((FRAME_DEACTIVATE_LAST - entity->client->ps.gunframe) == 4){
			entity->client->anim_priority = ANIM_REVERSE;

			if (entity->client->ps.pmove.pm_flags & PMF_DUCKED){
				entity->s.frame = FRAME_crpain4 + 1;
				entity->client->anim_end = FRAME_crpain1;
			}
			else {
				entity->s.frame = FRAME_pain304 + 1;
				entity->client->anim_end = FRAME_pain301;
			}
		}

		entity->client->ps.gunframe++;
		return;
	}

	if (entity->client->weaponstate == WEAPON_ACTIVATING){
		if (entity->client->ps.gunframe == FRAME_ACTIVATE_LAST){
			entity->client->weaponstate = WEAPON_READY;
			entity->client->ps.gunframe = FRAME_IDLE_FIRST;
			return;
		}

		entity->client->ps.gunframe++;
		return;
	}

	if ((entity->client->newweapon) && (entity->client->weaponstate != WEAPON_FIRING)){
		entity->client->weaponstate = WEAPON_DROPPING;
		entity->client->ps.gunframe = FRAME_DEACTIVATE_FIRST;

		if ((FRAME_DEACTIVATE_LAST - FRAME_DEACTIVATE_FIRST) < 4){
			entity->client->anim_priority = ANIM_REVERSE;

			if (entity->client->ps.pmove.pm_flags & PMF_DUCKED){
				entity->s.frame = FRAME_crpain4 + 1;
				entity->client->anim_end = FRAME_crpain1;
			}
			else {
				entity->s.frame = FRAME_pain304 + 1;
				entity->client->anim_end = FRAME_pain301;
			}
		}

		return;
	}

	if (entity->client->weaponstate == WEAPON_READY){
		if (((entity->client->latched_buttons|entity->client->buttons) & BUTTON_ATTACK)){
			entity->client->latched_buttons &= ~BUTTON_ATTACK;

			if ((!entity->client->ammo_index) || (entity->client->pers.inventory[entity->client->ammo_index] >= entity->client->pers.weapon->quantity)){
				entity->client->ps.gunframe = FRAME_FIRE_FIRST;
				entity->client->weaponstate = WEAPON_FIRING;

				// Start the animation
				entity->client->anim_priority = ANIM_ATTACK;

				if (entity->client->ps.pmove.pm_flags & PMF_DUCKED){
					entity->s.frame = FRAME_crattak1 - 1;
					entity->client->anim_end = FRAME_crattak9;
				}
				else {
					entity->s.frame = FRAME_attack1 - 1;
					entity->client->anim_end = FRAME_attack8;
				}
			}
			else {
				if (level.time >= entity->pain_debounce_time){
					gi.sound(entity, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1.0f, ATTN_NORM, 0.0f);
					entity->pain_debounce_time = level.time + 1.0f;
				}

				SG_NoAmmoWeaponChange(entity);
			}
		}
		else {
			if (entity->client->ps.gunframe == FRAME_IDLE_LAST){
				entity->client->ps.gunframe = FRAME_IDLE_FIRST;
				return;
			}

			if (pauseFrames){
				for (i = 0; pauseFrames[i]; i++){
					if (entity->client->ps.gunframe == pauseFrames[i]){
						if (rand() & 15)
							return;
					}
				}
			}

			entity->client->ps.gunframe++;
			return;
		}
	}

	if (entity->client->weaponstate == WEAPON_FIRING){
		for (i = 0; fireFrames[i]; i++){
			if (entity->client->ps.gunframe == fireFrames[i]){
				if (entity->client->quad_framenum > level.framenum)
					gi.sound(entity, CHAN_ITEM, gi.soundindex("items/damage3.wav"), 1.0f, ATTN_NORM, 0.0f);

				fire(entity);
				break;
			}
		}

		if (!fireFrames[i])
			entity->client->ps.gunframe++;

		if (entity->client->ps.gunframe == FRAME_IDLE_FIRST + 1)
			entity->client->weaponstate = WEAPON_READY;
	}
}


/*
 ==============================================================================

 GRENADE WEAPON

 ==============================================================================
*/

#define GRENADE_TIMER		3.0f
#define GRENADE_MIN_SPEED	400
#define GRENADE_MAX_SPEED	800


/*
 ==================
 SG_FireGrenadeWeapon
 ==================
*/
static void SG_FireGrenadeWeapon (edict_t *entity, qboolean held){

	vec3_t	offset;
	vec3_t	forward, right;
	vec3_t	start;
	int		damage = 125;
	float	timer;
	int		speed;
	float	radius;

	radius = damage + 40;

	if (is_quad)
		damage *= 4;

	VectorSet(offset, 8.0f, 8.0f, entity->viewheight - 8.0f);
	AngleVectors(entity->client->v_angle, forward, right, NULL);
	SG_ProjectWeaponSource(entity->client, entity->s.origin, offset, forward, right, start);

	timer = entity->client->grenade_time - level.time;
	speed = GRENADE_MIN_SPEED + (GRENADE_TIMER - timer) * ((GRENADE_MAX_SPEED - GRENADE_MIN_SPEED) / GRENADE_TIMER);

	SG_FireGrenadeProjectile2(entity, start, forward, damage, speed, timer, radius, held);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		entity->client->pers.inventory[entity->client->ammo_index]--;

	entity->client->grenade_time = level.time + 1.0f;

	if (entity->deadflag || entity->s.modelindex != 255)
		return;		// View weapon animations screw up corpses

	if (entity->health <= 0)
		return;

	if (entity->client->ps.pmove.pm_flags & PMF_DUCKED){
		entity->client->anim_priority = ANIM_ATTACK;
		entity->s.frame = FRAME_crattak1 - 1;
		entity->client->anim_end = FRAME_crattak3;
	}
	else {
		entity->client->anim_priority = ANIM_REVERSE;
		entity->s.frame = FRAME_wave08;
		entity->client->anim_end = FRAME_wave01;
	}
}

/*
 ==================
 SG_GrenadeWeapon
 ==================
*/
void SG_GrenadeWeapon (edict_t *entity){

	if ((entity->client->newweapon) && (entity->client->weaponstate == WEAPON_READY)){
		SG_ChangeWeapon(entity);
		return;
	}

	if (entity->client->weaponstate == WEAPON_ACTIVATING){
		entity->client->weaponstate = WEAPON_READY;
		entity->client->ps.gunframe = 16;
		return;
	}

	if (entity->client->weaponstate == WEAPON_READY){
		if (((entity->client->latched_buttons|entity->client->buttons) & BUTTON_ATTACK)){
			entity->client->latched_buttons &= ~BUTTON_ATTACK;

			if (entity->client->pers.inventory[entity->client->ammo_index]){
				entity->client->ps.gunframe = 1;
				entity->client->weaponstate = WEAPON_FIRING;
				entity->client->grenade_time = 0;
			}
			else {
				if (level.time >= entity->pain_debounce_time){
					gi.sound(entity, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1.0f, ATTN_NORM, 0.0f);
					entity->pain_debounce_time = level.time + 1.0f;
				}

				SG_NoAmmoWeaponChange(entity);
			}

			return;
		}

		if ((entity->client->ps.gunframe == 29) || (entity->client->ps.gunframe == 34) || (entity->client->ps.gunframe == 39) || (entity->client->ps.gunframe == 48)){
			if (rand() & 15)
				return;
		}

		if (++entity->client->ps.gunframe > 48)
			entity->client->ps.gunframe = 16;

		return;
	}

	if (entity->client->weaponstate == WEAPON_FIRING){
		if (entity->client->ps.gunframe == 5)
			gi.sound(entity, CHAN_WEAPON, gi.soundindex("weapons/hgrena1b.wav"), 1.0f, ATTN_NORM, 0.0f);

		if (entity->client->ps.gunframe == 11){
			if (!entity->client->grenade_time){
				entity->client->grenade_time = level.time + GRENADE_TIMER + 0.2f;
				entity->client->weapon_sound = gi.soundindex("weapons/hgrenc1b.wav");
			}

			// They waited too long, detonate it in their hand
			if (!entity->client->grenade_blew_up && level.time >= entity->client->grenade_time){
				entity->client->weapon_sound = 0;
				SG_FireGrenadeWeapon(entity, true);
				entity->client->grenade_blew_up = true;
			}

			if (entity->client->buttons & BUTTON_ATTACK)
				return;

			if (entity->client->grenade_blew_up){
				if (level.time >= entity->client->grenade_time){
					entity->client->ps.gunframe = 15;
					entity->client->grenade_blew_up = false;
				}
				else
					return;
			}
		}

		if (entity->client->ps.gunframe == 12){
			entity->client->weapon_sound = 0;
			SG_FireGrenadeWeapon(entity, false);
		}

		if ((entity->client->ps.gunframe == 15) && (level.time < entity->client->grenade_time))
			return;

		entity->client->ps.gunframe++;

		if (entity->client->ps.gunframe == 16){
			entity->client->grenade_time = 0;
			entity->client->weaponstate = WEAPON_READY;
		}
	}
}


/*
 ==============================================================================

 GRENADE LAUNCHER WEAPON

 ==============================================================================
*/


/*
 ==================
 SG_FireGrenadeLauncherWeapon
 ==================
*/
static void SG_FireGrenadeLauncherWeapon (edict_t *entity){

	vec3_t	offset;
	vec3_t	forward, right;
	vec3_t	start;
	int		damage = 120;
	float	radius;

	radius = damage + 40;

	if (is_quad)
		damage *= 4;

	VectorSet(offset, 8.0f, 8.0f, entity->viewheight - 8.0f);
	AngleVectors(entity->client->v_angle, forward, right, NULL);
	SG_ProjectWeaponSource(entity->client, entity->s.origin, offset, forward, right, start);

	VectorScale(forward, -2.0f, entity->client->kick_origin);
	entity->client->kick_angles[0] = -1.0f;

	SG_FireGrenadeProjectile(entity, start, forward, damage, 600, 2.5f, radius);

	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(entity - g_edicts);
	gi.WriteByte(MZ_GRENADE | is_silenced);
	gi.multicast(entity->s.origin, MULTICAST_PVS);

	entity->client->ps.gunframe++;

	SG_PlayerNoise(entity, start, PNOISE_WEAPON);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		entity->client->pers.inventory[entity->client->ammo_index]--;
}

/*
 ==================
 SG_GrenadeLauncherWeapon
 ==================
*/
void SG_GrenadeLauncherWeapon (edict_t *entity){

	static int pauseFrames[] = {34, 51, 59, 0};
	static int fireFrames[] = {6, 0};

	SG_GenericWeapon(entity, 5, 16, 59, 64, pauseFrames, fireFrames, SG_FireGrenadeLauncherWeapon);
}


/*
 ==============================================================================

 ROCKET LAUNCHER WEAPON

 ==============================================================================
*/


/*
 ==================
 SG_FireRocketLauncherWeapon
 ==================
*/
static void SG_FireRocketLauncherWeapon (edict_t *entity){

	vec3_t	offset, start;
	vec3_t	forward, right;
	int		damage;
	float	damageRadius;
	int		radiusDamage;

	damage = 100 + (int)(random() * 20.0f);
	radiusDamage = 120;
	damageRadius = 120.0f;

	if (is_quad){
		damage *= 4;
		radiusDamage *= 4;
	}

	AngleVectors(entity->client->v_angle, forward, right, NULL);

	VectorScale(forward, -2.0f, entity->client->kick_origin);
	entity->client->kick_angles[0] = -1.0f;

	VectorSet(offset, 8.0f, 8.0f, entity->viewheight - 8.0f);
	SG_ProjectWeaponSource(entity->client, entity->s.origin, offset, forward, right, start);

	SG_FireRocketProjectile(entity, start, forward, damage, 650, damageRadius, radiusDamage);

	// Send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(entity-g_edicts);
	gi.WriteByte(MZ_ROCKET | is_silenced);
	gi.multicast(entity->s.origin, MULTICAST_PVS);

	entity->client->ps.gunframe++;

	SG_PlayerNoise(entity, start, PNOISE_WEAPON);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		entity->client->pers.inventory[entity->client->ammo_index]--;
}

/*
 ==================
 SG_RocketLauncherWeapon
 ==================
*/
void SG_RocketLauncherWeapon (edict_t *entity){

	static int pauseFrames[] = {25, 33, 42, 50, 0};
	static int fireFrames[] = {5, 0};

	SG_GenericWeapon(entity, 4, 12, 50, 54, pauseFrames, fireFrames, SG_FireRocketLauncherWeapon);
}


/*
 ==============================================================================

 BLASTER WEAPON

 ==============================================================================
*/


/*
 ==================
 SG_BlasterFire
 ==================
*/
static void SG_BlasterFire (edict_t *entity, vec3_t g_offset, int damage, qboolean hyper, int effect){

	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	offset;

	if (is_quad)
		damage *= 4;

	AngleVectors(entity->client->v_angle, forward, right, NULL);
	VectorSet(offset, 24.0f, 8.0f, entity->viewheight - 8.0f);
	VectorAdd(offset, g_offset, offset);

	SG_ProjectWeaponSource(entity->client, entity->s.origin, offset, forward, right, start);

	VectorScale(forward, -2.0f, entity->client->kick_origin);
	entity->client->kick_angles[0] = -1.0f;

	SG_FireBlasterProjectile(entity, start, forward, damage, 1000, effect, hyper);

	// Send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(entity-g_edicts);

	if (hyper)
		gi.WriteByte(MZ_HYPERBLASTER | is_silenced);
	else
		gi.WriteByte(MZ_BLASTER | is_silenced);

	gi.multicast(entity->s.origin, MULTICAST_PVS);

	SG_PlayerNoise(entity, start, PNOISE_WEAPON);
}

/*
 ==================
 SG_FireBlasterWeapon
 ==================
*/
static void SG_FireBlasterWeapon (edict_t *entity){

	int		damage;

	if (deathmatch->value)
		damage = 15;
	else
		damage = 10;

	SG_BlasterFire(entity, vec3_origin, damage, false, EF_BLASTER);

	entity->client->ps.gunframe++;
}

/*
 ==================
 SG_BlasterWeapon
 ==================
*/
void SG_BlasterWeapon (edict_t *entity){

	static int pauseFrames[] = {19, 32, 0};
	static int fireFrames[] = {5, 0};

	SG_GenericWeapon(entity, 4, 8, 52, 55, pauseFrames, fireFrames, SG_FireBlasterWeapon);
}


/*
 ==============================================================================

 HYPERBLASTER WEAPON

 ==============================================================================
*/


/*
 ==================
 SG_FireHyperBlasterWeapon
 ==================
*/
static void SG_FireHyperBlasterWeapon (edict_t *entity){

	float	rotation;
	vec3_t	offset;
	int		effect;
	int		damage;

	entity->client->weapon_sound = gi.soundindex("weapons/hyprbl1a.wav");

	if (!(entity->client->buttons & BUTTON_ATTACK))
		entity->client->ps.gunframe++;
	else {
		if (!entity->client->pers.inventory[entity->client->ammo_index]){
			if (level.time >= entity->pain_debounce_time){
				gi.sound(entity, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1.0f, ATTN_NORM, 0.0f);
				entity->pain_debounce_time = level.time + 1.0f;
			}

			SG_NoAmmoWeaponChange(entity);
		}
		else {
			rotation = (entity->client->ps.gunframe - 5) * 2.0f * M_PI / 6.0f;
			offset[0] = -4.0f * sin(rotation);
			offset[1] = 0.0f;
			offset[2] = 4.0f * cos(rotation);

			if ((entity->client->ps.gunframe == 6) || (entity->client->ps.gunframe == 9))
				effect = EF_HYPERBLASTER;
			else
				effect = 0;

			if (deathmatch->value)
				damage = 15;
			else
				damage = 20;

			SG_BlasterFire(entity, offset, damage, true, effect);

			if (!((int)dmflags->value & DF_INFINITE_AMMO))
				entity->client->pers.inventory[entity->client->ammo_index]--;

			entity->client->anim_priority = ANIM_ATTACK;

			if (entity->client->ps.pmove.pm_flags & PMF_DUCKED){
				entity->s.frame = FRAME_crattak1 - 1;
				entity->client->anim_end = FRAME_crattak9;
			}
			else {
				entity->s.frame = FRAME_attack1 - 1;
				entity->client->anim_end = FRAME_attack8;
			}
		}

		entity->client->ps.gunframe++;

		if (entity->client->ps.gunframe == 12 && entity->client->pers.inventory[entity->client->ammo_index])
			entity->client->ps.gunframe = 6;
	}

	if (entity->client->ps.gunframe == 12){
		gi.sound(entity, CHAN_AUTO, gi.soundindex("weapons/hyprbd1a.wav"), 1.0f, ATTN_NORM, 0.0f);
		entity->client->weapon_sound = 0;
	}
}

/*
 ==================
 SG_HyperBlasterWeapon
 ==================
*/
void SG_HyperBlasterWeapon (edict_t *entity){

	static int pauseFrames[] = {0};
	static int fireFrames[] = {6, 7, 8, 9, 10, 11, 0};

	SG_GenericWeapon(entity, 5, 20, 49, 53, pauseFrames, fireFrames, SG_FireHyperBlasterWeapon);
}


/*
 ==============================================================================

 MACHINEGUN WEAPON

 ==============================================================================
*/


/*
 ==================
 SG_FireMachinegunWeapon
 ==================
*/
static void SG_FireMachinegunWeapon (edict_t *entity){

	vec3_t	start;
	vec3_t	forward, right;
	vec3_t	angles;
	int		damage = 8;
	int		kick = 2;
	vec3_t	offset;

	if (!(entity->client->buttons & BUTTON_ATTACK)){
		entity->client->machinegun_shots = 0;
		entity->client->ps.gunframe++;
		return;
	}

	if (entity->client->ps.gunframe == 5)
		entity->client->ps.gunframe = 4;
	else
		entity->client->ps.gunframe = 5;

	if (entity->client->pers.inventory[entity->client->ammo_index] < 1){
		entity->client->ps.gunframe = 6;

		if (level.time >= entity->pain_debounce_time){
			gi.sound(entity, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1.0f, ATTN_NORM, 0.0f);
			entity->pain_debounce_time = level.time + 1.0f;
		}

		SG_NoAmmoWeaponChange(entity);
		return;
	}

	if (is_quad){
		damage *= 4;
		kick *= 4;
	}

	entity->client->kick_origin[0] = crandom() * 0.35f;
	entity->client->kick_origin[1] = crandom() * 0.35f;
	entity->client->kick_origin[2] = crandom() * 0.35f;

	entity->client->kick_angles[0] = crandom() * 0.7f;
	entity->client->kick_angles[1] = crandom() * 0.7f;
	entity->client->kick_angles[2] = crandom() * 0.7f;

	entity->client->kick_origin[0] = crandom() * 0.35f;
	entity->client->kick_angles[0] = entity->client->machinegun_shots * -1.5f;

	// Raise the gun as it is firing
	if (!deathmatch->value){
		entity->client->machinegun_shots++;

		if (entity->client->machinegun_shots > 9)
			entity->client->machinegun_shots = 9;
	}

	// Get start / end positions
	VectorAdd(entity->client->v_angle, entity->client->kick_angles, angles);
	AngleVectors(angles, forward, right, NULL);

	VectorSet(offset, 0.0f, 8.0f, entity->viewheight - 8.0f);
	SG_ProjectWeaponSource(entity->client, entity->s.origin, offset, forward, right, start);

	SG_FireBullet(entity, start, forward, damage, kick, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MOD_MACHINEGUN);

	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(entity - g_edicts);
	gi.WriteByte(MZ_MACHINEGUN | is_silenced);
	gi.multicast(entity->s.origin, MULTICAST_PVS);

	SG_PlayerNoise(entity, start, PNOISE_WEAPON);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		entity->client->pers.inventory[entity->client->ammo_index]--;

	entity->client->anim_priority = ANIM_ATTACK;

	if (entity->client->ps.pmove.pm_flags & PMF_DUCKED){
		entity->s.frame = FRAME_crattak1 - (int)(random() + 0.25f);
		entity->client->anim_end = FRAME_crattak9;
	}
	else {
		entity->s.frame = FRAME_attack1 - (int)(random() + 0.25f);
		entity->client->anim_end = FRAME_attack8;
	}
}

/*
 ==================
 SG_MachinegunWeapon
 ==================
*/
void SG_MachinegunWeapon (edict_t *entity){

	static int pauseFrames[] = {23, 45, 0};
	static int fireFrames[] = {4, 5, 0};

	SG_GenericWeapon(entity, 3, 5, 45, 49, pauseFrames, fireFrames, SG_FireMachinegunWeapon);
}


/*
 ==============================================================================

 CHAINGUN WEAPON

 ==============================================================================
*/


/*
 ==================
 SG_FireChaingunWeapon
 ==================
*/
static void SG_FireChaingunWeapon (edict_t *entity){

	int			damage;
	int			shots;
	int			i;
	int			kick = 2;
	vec3_t		start;
	vec3_t		forward, right, up;
	float		r, u;
	vec3_t		offset;

	if (deathmatch->value)
		damage = 6;
	else
		damage = 8;

	if (entity->client->ps.gunframe == 5)
		gi.sound(entity, CHAN_AUTO, gi.soundindex("weapons/chngnu1a.wav"), 1.0f, ATTN_IDLE, 0.0f);

	if ((entity->client->ps.gunframe == 14) && !(entity->client->buttons & BUTTON_ATTACK)){
		entity->client->ps.gunframe = 32;
		entity->client->weapon_sound = 0;

		return;
	}
	else if ((entity->client->ps.gunframe == 21) && (entity->client->buttons & BUTTON_ATTACK) && entity->client->pers.inventory[entity->client->ammo_index])
		entity->client->ps.gunframe = 15;
	else
		entity->client->ps.gunframe++;

	if (entity->client->ps.gunframe == 22){
		entity->client->weapon_sound = 0;
		gi.sound(entity, CHAN_AUTO, gi.soundindex("weapons/chngnd1a.wav"), 1.0f, ATTN_IDLE, 0.0f);
	}
	else
		entity->client->weapon_sound = gi.soundindex("weapons/chngnl1a.wav");

	entity->client->anim_priority = ANIM_ATTACK;

	if (entity->client->ps.pmove.pm_flags & PMF_DUCKED){
		entity->s.frame = FRAME_crattak1 - (entity->client->ps.gunframe & 1);
		entity->client->anim_end = FRAME_crattak9;
	}
	else {
		entity->s.frame = FRAME_attack1 - (entity->client->ps.gunframe & 1);
		entity->client->anim_end = FRAME_attack8;
	}

	if (entity->client->ps.gunframe <= 9)
		shots = 1;
	else if (entity->client->ps.gunframe <= 14){
		if (entity->client->buttons & BUTTON_ATTACK)
			shots = 2;
		else
			shots = 1;
	}
	else
		shots = 3;

	if (entity->client->pers.inventory[entity->client->ammo_index] < shots)
		shots = entity->client->pers.inventory[entity->client->ammo_index];

	if (!shots){
		if (level.time >= entity->pain_debounce_time){
			gi.sound(entity, CHAN_VOICE, gi.soundindex("weapons/noammo.wav"), 1.0f, ATTN_NORM, 0.0f);
			entity->pain_debounce_time = level.time + 1;
		}

		SG_NoAmmoWeaponChange(entity);
		return;
	}

	if (is_quad){
		damage *= 4;
		kick *= 4;
	}

	entity->client->kick_origin[0] = crandom() * 0.35f;
	entity->client->kick_origin[1] = crandom() * 0.35f;
	entity->client->kick_origin[2] = crandom() * 0.35f;

	entity->client->kick_angles[0] = crandom() * 0.7f;
	entity->client->kick_angles[1] = crandom() * 0.7f;
	entity->client->kick_angles[2] = crandom() * 0.7f;

	for (i = 0; i < shots; i++){
		// Get start / end positions
		AngleVectors(entity->client->v_angle, forward, right, up);
		r = 7.0f + crandom() * 4.0f;
		u = crandom() * 4.0f;

		VectorSet(offset, 0.0f, r, u + entity->viewheight - 8.0f);
		SG_ProjectWeaponSource(entity->client, entity->s.origin, offset, forward, right, start);

		SG_FireBullet(entity, start, forward, damage, kick, DEFAULT_BULLET_HSPREAD, DEFAULT_BULLET_VSPREAD, MOD_CHAINGUN);
	}

	// Send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(entity - g_edicts);
	gi.WriteByte((MZ_CHAINGUN1 + shots - 1) | is_silenced);
	gi.multicast(entity->s.origin, MULTICAST_PVS);

	SG_PlayerNoise(entity, start, PNOISE_WEAPON);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		entity->client->pers.inventory[entity->client->ammo_index] -= shots;
}

/*
 ==================
 SG_ChaingunWeapon
 ==================
*/
void SG_ChaingunWeapon (edict_t *entity){

	static int pauseFrames[] = {38, 43, 51, 61, 0};
	static int fireFrames[] = {5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 0};

	SG_GenericWeapon(entity, 4, 31, 61, 64, pauseFrames, fireFrames, SG_FireChaingunWeapon);
}


/*
 ==============================================================================

 SHOTGUN WEAPON

 ==============================================================================
*/


/*
 ==================
 SG_FireShotgunWeapon
 ==================
*/
static void SG_FireShotgunWeapon (edict_t *entity){

	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	offset;
	int		damage = 4;
	int		kick = 8;

	if (entity->client->ps.gunframe == 9){
		entity->client->ps.gunframe++;
		return;
	}

	AngleVectors(entity->client->v_angle, forward, right, NULL);

	VectorScale(forward, -2.0f, entity->client->kick_origin);
	entity->client->kick_angles[0] = -2.0f;

	VectorSet(offset, 0.0f, 8.0f,  entity->viewheight - 8.0f);
	SG_ProjectWeaponSource(entity->client, entity->s.origin, offset, forward, right, start);

	if (is_quad){
		damage *= 4;
		kick *= 4;
	}

	if (deathmatch->value)
		SG_FirePellet(entity, start, forward, damage, kick, 500, 500, DEFAULT_DEATHMATCH_SHOTGUN_COUNT, MOD_SHOTGUN);
	else
		SG_FirePellet(entity, start, forward, damage, kick, 500, 500, DEFAULT_SHOTGUN_COUNT, MOD_SHOTGUN);

	// Send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(entity - g_edicts);
	gi.WriteByte(MZ_SHOTGUN | is_silenced);
	gi.multicast(entity->s.origin, MULTICAST_PVS);

	entity->client->ps.gunframe++;
	SG_PlayerNoise(entity, start, PNOISE_WEAPON);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		entity->client->pers.inventory[entity->client->ammo_index]--;
}

/*
 ==================
 SG_ShotgunWeapon
 ==================
*/
void SG_ShotgunWeapon (edict_t *entity){

	static int pauseFrames[] = {22, 28, 34, 0};
	static int fireFrames[] = {8, 9, 0};

	SG_GenericWeapon(entity, 7, 18, 36, 39, pauseFrames, fireFrames, SG_FireShotgunWeapon);
}


/*
 ==============================================================================

 SUPERSHOTGUN WEAPON

 ==============================================================================
*/


/*
 ==================
 SG_FireSupershotgunWeapon
 ==================
*/
static void SG_FireSupershotgunWeapon (edict_t *entity){

	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	offset;
	vec3_t	v;
	int		damage = 6;
	int		kick = 12;

	AngleVectors(entity->client->v_angle, forward, right, NULL);

	VectorScale(forward, -2.0f, entity->client->kick_origin);
	entity->client->kick_angles[0] = -2;

	VectorSet(offset, 0.0f, 8.0f,  entity->viewheight - 8.0f);
	SG_ProjectWeaponSource(entity->client, entity->s.origin, offset, forward, right, start);

	if (is_quad){
		damage *= 4;
		kick *= 4;
	}

	v[PITCH] = entity->client->v_angle[PITCH];
	v[YAW] = entity->client->v_angle[YAW] - 5.0f;
	v[ROLL] = entity->client->v_angle[ROLL];
	AngleVectors(v, forward, NULL, NULL);
	SG_FirePellet(entity, start, forward, damage, kick, DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD, DEFAULT_SSHOTGUN_COUNT / 2, MOD_SSHOTGUN);
	v[YAW] = entity->client->v_angle[YAW] + 5.0f;
	AngleVectors(v, forward, NULL, NULL);
	SG_FirePellet(entity, start, forward, damage, kick, DEFAULT_SHOTGUN_HSPREAD, DEFAULT_SHOTGUN_VSPREAD, DEFAULT_SSHOTGUN_COUNT / 2, MOD_SSHOTGUN);

	// Send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(entity - g_edicts);
	gi.WriteByte(MZ_SSHOTGUN | is_silenced);
	gi.multicast(entity->s.origin, MULTICAST_PVS);

	entity->client->ps.gunframe++;
	SG_PlayerNoise(entity, start, PNOISE_WEAPON);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		entity->client->pers.inventory[entity->client->ammo_index] -= 2;
}

/*
 ==================
 SG_SupershotgunWeapon
 ==================
*/
void SG_SupershotgunWeapon (edict_t *entity){

	static int pauseFrames[] = {29, 42, 57, 0};
	static int fireFrames[] = {7, 0};

	SG_GenericWeapon(entity, 6, 17, 57, 61, pauseFrames, fireFrames, SG_FireSupershotgunWeapon);
}


/*
 ==============================================================================

 RAILGUN WEAPON

 ==============================================================================
*/


/*
 ==================
 SG_FireRailgunWeapon
 ==================
*/
static void SG_FireRailgunWeapon (edict_t *entity){

	int		damage;
	int		kick;
	vec3_t	forward, right;
	vec3_t	start;
	vec3_t	offset;

	if (deathmatch->value){
		// Normal damage is too extreme in dm
		damage = 100;
		kick = 200;
	}
	else {
		damage = 150;
		kick = 250;
	}

	if (is_quad){
		damage *= 4;
		kick *= 4;
	}

	AngleVectors(entity->client->v_angle, forward, right, NULL);

	VectorScale(forward, -3.0f, entity->client->kick_origin);
	entity->client->kick_angles[0] = -3.0f;

	VectorSet(offset, 0.0f, 7.0f,  entity->viewheight - 8.0f);
	SG_ProjectWeaponSource(entity->client, entity->s.origin, offset, forward, right, start);
	SG_FireRailProjectile(entity, start, forward, damage, kick);

	// Send muzzle flash
	gi.WriteByte(svc_muzzleflash);
	gi.WriteShort(entity - g_edicts);
	gi.WriteByte(MZ_RAILGUN | is_silenced);
	gi.multicast(entity->s.origin, MULTICAST_PVS);

	entity->client->ps.gunframe++;
	SG_PlayerNoise(entity, start, PNOISE_WEAPON);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		entity->client->pers.inventory[entity->client->ammo_index]--;
}

/*
 ==================
 SG_RailgunWeapon
 ==================
*/
void SG_RailgunWeapon (edict_t *entity){

	static int pauseFrames[] = {56, 0};
	static int fireFrames[] = {4, 0};

	SG_GenericWeapon(entity, 3, 18, 56, 61, pauseFrames, fireFrames, SG_FireRailgunWeapon);
}


/*
 ==============================================================================

 BFG10K WEAPON

 ==============================================================================
*/


/*
 ==================
 SG_FireBFGWeapon
 ==================
*/
static void SG_FireBFGWeapon (edict_t *entity){

	int		damage;
	vec3_t	forward, right;
	vec3_t	offset, start;
	float	damage_radius = 1000;

	if (deathmatch->value)
		damage = 200;
	else
		damage = 500;

	if (entity->client->ps.gunframe == 9){
		// Send muzzle flash
		gi.WriteByte(svc_muzzleflash);
		gi.WriteShort(entity - g_edicts);
		gi.WriteByte(MZ_BFG | is_silenced);
		gi.multicast(entity->s.origin, MULTICAST_PVS);

		entity->client->ps.gunframe++;

		SG_PlayerNoise(entity, start, PNOISE_WEAPON);
		return;
	}

	// Cells can go down during windup (from power armor hits), so
	// check again and abort firing if we don't have enough now
	if (entity->client->pers.inventory[entity->client->ammo_index] < 50){
		entity->client->ps.gunframe++;
		return;
	}

	if (is_quad)
		damage *= 4;

	AngleVectors(entity->client->v_angle, forward, right, NULL);

	VectorScale(forward, -2.0f, entity->client->kick_origin);

	// Make a big pitch kick with an inverse fall
	entity->client->v_dmg_pitch = -40.0f;
	entity->client->v_dmg_roll = crandom() * 8.0f;
	entity->client->v_dmg_time = level.time + DAMAGE_TIME;

	VectorSet(offset, 8.0f, 8.0f, entity->viewheight - 8.0f);
	SG_ProjectWeaponSource(entity->client, entity->s.origin, offset, forward, right, start);
	SG_FireBFGProjectile(entity, start, forward, damage, 400, damage_radius);

	entity->client->ps.gunframe++;

	SG_PlayerNoise(entity, start, PNOISE_WEAPON);

	if (!((int)dmflags->value & DF_INFINITE_AMMO))
		entity->client->pers.inventory[entity->client->ammo_index] -= 50;
}

/*
 ==================
 SG_BFGWeapon
 ==================
*/
void SG_BFGWeapon (edict_t *entity){

	static int	pauseFrames[] = {39, 45, 50, 55, 0};
	static int	fireFrames[] = {9, 17, 0};

	SG_GenericWeapon(entity, 8, 32, 55, 58, pauseFrames, fireFrames, SG_FireBFGWeapon);
}