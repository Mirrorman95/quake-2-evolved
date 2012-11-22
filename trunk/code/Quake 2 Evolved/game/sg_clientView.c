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
// sg_clientView.c - View effects
//

// TODO:
// - SG_CalculateBlend, SG_WorldEffects, SG_ClientEndServerFrame


#include "g_local.h"
#include "m_player.h"



static edict_t *		current_player;
static gclient_t *		current_client;

static vec3_t			forward, right, up;
static float			xyspeed;

static float			bobmove;
static int				bobcycle;		// Odd cycles are right foot going forward
static float			bobfracsin;		// sin(bobfrac * M_PI)

static vec3_t			power_color = {0.0f, 1.0f, 0.0f};
static vec3_t			acolor = {1.0f, 1.0f, 1.0f};
static vec3_t			bcolor = {1.0f, 0.0f, 0.0f};


/*
 ==================
 SG_CalculateRoll
 ==================
*/
static float SG_CalculateRoll (vec3_t angles, vec3_t velocity){

	float	sign;
	float	side;
	float	value;

	side = DotProduct(velocity, right);
	sign = side < 0.0f ? -1.0f : 1.0f;
	side = fabs(side);

	value = sv_rollangle->value;

	if (side < sv_rollspeed->value)
		side = side * value / sv_rollspeed->value;
	else
		side = value;

	return side * sign;
}


/*
 ==============================================================================

 DAMAGE FEEDBACK

 ==============================================================================
*/


/*
 ==================
 SG_ClientDamageFeedback

 Handles color blends and view kicks
 ==================
*/
static void SG_ClientDamageFeedback (edict_t *player){

	static int	i;
	gclient_t	*client;
	float		side;
	float		realCount, count, kick;
	vec3_t		v;
	int			r, l;
	 
	client = player->client;

	// Flash the backgrounds behind the status numbers
	client->ps.stats[STAT_FLASHES] = 0;
	if (client->damage_blood)
		client->ps.stats[STAT_FLASHES] |= 1;
	if (client->damage_armor && !(player->flags & FL_GODMODE) && (client->invincible_framenum <= level.framenum))
		client->ps.stats[STAT_FLASHES] |= 2;

	// Total points of damage shot at the player this frame
	count = (client->damage_blood + client->damage_armor + client->damage_parmor);
	if (count == 0.0f)
		return;		// Didn't take any damage

	// start a pain animation if still in the player model
	if (client->anim_priority < ANIM_PAIN && player->s.modelindex == 255){
		client->anim_priority = ANIM_PAIN;

		if (client->ps.pmove.pm_flags & PMF_DUCKED){
			player->s.frame = FRAME_crpain1 - 1;
			client->anim_end = FRAME_crpain4;
		}
		else {
			i = (i + 1) % 3;

			switch (i){
			case 0:
				player->s.frame = FRAME_pain101 - 1;
				client->anim_end = FRAME_pain104;
				break;
			case 1:
				player->s.frame = FRAME_pain201 - 1;
				client->anim_end = FRAME_pain204;
				break;
			case 2:
				player->s.frame = FRAME_pain301 - 1;
				client->anim_end = FRAME_pain304;
				break;
			}
		}
	}

	// Always make a visible effect
	realCount = count;
	if (count < 10.0f)
		count = 10.0f;

	// Play an apropriate pain sound
	if ((level.time > player->pain_debounce_time) && !(player->flags & FL_GODMODE) && (client->invincible_framenum <= level.framenum)){
		r = 1 + (rand() & 1);

		player->pain_debounce_time = level.time + 0.7f;

		if (player->health < 25)
			l = 25;
		else if (player->health < 50)
			l = 50;
		else if (player->health < 75)
			l = 75;
		else
			l = 100;

		gi.sound(player, CHAN_VOICE, gi.soundindex(va("*pain%i_%i.wav", l, r)), 1.0f, ATTN_NORM, 0.0f);
	}

	// The total alpha of the blend is always proportional to count
	if (client->damage_alpha < 0.0f)
		client->damage_alpha = 0.0f;

	client->damage_alpha += count * 0.01f;

	if (client->damage_alpha < 0.2f)
		client->damage_alpha = 0.2f;
	if (client->damage_alpha > 0.6f)
		client->damage_alpha = 0.6f;		// Don't go too saturated

	// The color of the blend will vary based on how much was absorbed
	// by different armors
	VectorClear(v);

	if (client->damage_parmor)
		VectorMA(v, (float)client->damage_parmor / realCount, power_color, v);
	if (client->damage_armor)
		VectorMA(v, (float)client->damage_armor / realCount,  acolor, v);
	if (client->damage_blood)
		VectorMA(v, (float)client->damage_blood / realCount,  bcolor, v);

	VectorCopy(v, client->damage_blend);

	// Calculate view angle kicks
	kick = abs(client->damage_knockback);

	// Kick of 0 means no view adjust at all
	if (kick && player->health > 0){
		kick = kick * 100.0f / player->health;

		if (kick < count * 0.5f)
			kick = count * 0.5f;
		if (kick > 50.0f)
			kick = 50.0f;

		VectorSubtract(client->damage_from, player->s.origin, v);
		VectorNormalize(v);

		side = DotProduct(v, right);
		client->v_dmg_roll = kick * side * 0.3f;

		side = -DotProduct(v, forward);
		client->v_dmg_pitch = kick * side * 0.3f;

		client->v_dmg_time = level.time + DAMAGE_TIME;
	}

	// Clear totals
	client->damage_blood = 0;
	client->damage_armor = 0;
	client->damage_parmor = 0;
	client->damage_knockback = 0;
}


/*
 ==============================================================================

 VIEW AND GUN OFFSETS

 ==============================================================================
*/


/*
 ==================
 SG_CalculateViewOffset

 Fall from 128: 400 = 160000
 Fall from 256: 580 = 336400
 Fall from 384: 720 = 518400
 Fall from 512: 800 = 640000
 Fall from 640: 960 = 

 Damage = deltavelocity * deltavelocity  * 0.0001

 FIXME: Auto pitching on slopes?
 ==================
*/
static void SG_CalculateViewOffset (edict_t *entity){

	float	*angles;
	float	bob;
	float	ratio;
	float	delta;
	vec3_t	v;

	// Base angles
	angles = entity->client->ps.kick_angles;

	// If dead, fix the angle and don't add any kick
	if (entity->deadflag){
		VectorClear(angles);

		entity->client->ps.viewangles[ROLL] = 40.0f;
		entity->client->ps.viewangles[PITCH] = -15.0f;
		entity->client->ps.viewangles[YAW] = entity->client->killer_yaw;
	}
	else {
		// Add angles based on weapon kick
		VectorCopy(entity->client->kick_angles, angles);

		// Add angles based on damage kick
		ratio = (entity->client->v_dmg_time - level.time) / DAMAGE_TIME;
		if (ratio < 0.0f){
			ratio = 0.0f;

			entity->client->v_dmg_pitch = 0.0f;
			entity->client->v_dmg_roll = 0.0f;
		}

		angles[PITCH] += ratio * entity->client->v_dmg_pitch;
		angles[ROLL] += ratio * entity->client->v_dmg_roll;

		// Add pitch based on fall kick
		ratio = (entity->client->fall_time - level.time) / FALL_TIME;
		if (ratio < 0.0f)
			ratio = 0.0f;

		angles[PITCH] += ratio * entity->client->fall_value;

		// Add angles based on velocity
		delta = DotProduct(entity->velocity, forward);
		angles[PITCH] += delta * run_pitch->value;

		delta = DotProduct(entity->velocity, right);
		angles[ROLL] += delta * run_roll->value;

		// Add angles based on bob
		delta = bobfracsin * bob_pitch->value * xyspeed;
		if (entity->client->ps.pmove.pm_flags & PMF_DUCKED)
			delta *= 6.0f;		// crouching

		angles[PITCH] += delta;

		delta = bobfracsin * bob_roll->value * xyspeed;
		if (entity->client->ps.pmove.pm_flags & PMF_DUCKED)
			delta *= 6.0f;		// crouching
		if (bobcycle & 1)
			delta = -delta;

		angles[ROLL] += delta;
	}

	// base Origin
	VectorClear(v);

	// Add view height
	v[2] += entity->viewheight;

	// Add fall height
	ratio = (entity->client->fall_time - level.time) / FALL_TIME;
	if (ratio < 0.0f)
		ratio = 0.0f;

	v[2] -= ratio * entity->client->fall_value * 0.4f;

	// Add bob height
	bob = bobfracsin * xyspeed * bob_up->value;
	if (bob > 6.0f)
		bob = 6.0f;

	v[2] += bob;

	// Add kick offset
	VectorAdd(v, entity->client->kick_origin, v);

	// Absolutely bound offsets
	// so the view can never be outside the player box
	if (v[0] < -14.0f)
		v[0] = -14.0f;
	else if (v[0] > 14.0f)
		v[0] = 14.0f;

	if (v[1] < -14.0f)
		v[1] = -14.0f;
	else if (v[1] > 14.0f)
		v[1] = 14.0f;

	if (v[2] < -22.0f)
		v[2] = -22.0f;
	else if (v[2] > 30.0f)
		v[2] = 30.0f;

	VectorCopy(v, entity->client->ps.viewoffset);
}

/*
 ==================
 SG_CalculateGunOffset
 ==================
*/
static void SG_CalculateGunOffset (edict_t *entity){

	int		i;
	float	delta;

	// Gun angles from bobbing
	entity->client->ps.gunangles[ROLL] = xyspeed * bobfracsin * 0.005f;
	entity->client->ps.gunangles[YAW] = xyspeed * bobfracsin * 0.01f;

	if (bobcycle & 1){
		entity->client->ps.gunangles[ROLL] = -entity->client->ps.gunangles[ROLL];
		entity->client->ps.gunangles[YAW] = -entity->client->ps.gunangles[YAW];
	}

	entity->client->ps.gunangles[PITCH] = xyspeed * bobfracsin * 0.005;

	// Gun angles from delta movement
	for (i = 0; i < 3; i++){
		delta = entity->client->oldviewangles[i] - entity->client->ps.viewangles[i];

		if (delta > 180.0f)
			delta -= 360.0f;
		if (delta < -180.0f)
			delta += 360.0f;
		if (delta > 45.0f)
			delta = 45.0f;
		if (delta < -45.0f)
			delta = -45.0f;

		if (i == YAW)
			entity->client->ps.gunangles[ROLL] += 0.1f * delta;

		entity->client->ps.gunangles[i] += 0.2f * delta;
	}

	// Gun height
	VectorClear(entity->client->ps.gunoffset);

	// gun_x / gun_y / gun_z are development tools
	for (i = 0; i < 3; i++){
		entity->client->ps.gunoffset[i] += forward[i] * (gun_y->value);
		entity->client->ps.gunoffset[i] += right[i] * gun_x->value;
		entity->client->ps.gunoffset[i] += up[i] * (-gun_z->value);
	}
}


/*
 ==============================================================================

 VIEW BLEND

 ==============================================================================


*/
/*
 ==================
 SG_AddBlend
 ==================
*/
static void SG_AddBlend (float r, float g, float b, float a, float *blend){

	float	a2, a3;

	if (a <= 0)
		return;

	// New total alpha
	a2 = blend[3] + (1 - blend[3]) * a;

	// Fraction of color from old
	a3 = blend[3] / a2;

	blend[0] = blend[0] * a3 + r * (1 - a3);
	blend[1] = blend[1] * a3 + g * (1 - a3);
	blend[2] = blend[2] * a3 + b * (1 - a3);
	blend[3] = a2;
}

/*
 ==================
 SG_CalculateBlend

 FIXME: With client prediction, the contents
 should be determined by the client
 ==================
*/
static void SG_CalculateBlend (edict_t *entity){

	int		contents;
	vec3_t	viewOrigin;
	int		remaining;

	entity->client->ps.blend[0] = entity->client->ps.blend[1] = entity->client->ps.blend[2] = entity->client->ps.blend[3] = 0.0f;

	// Add for contents
	VectorAdd(entity->s.origin, entity->client->ps.viewoffset, viewOrigin);

	contents = gi.pointcontents(viewOrigin);

	if (contents & (CONTENTS_LAVA|CONTENTS_SLIME|CONTENTS_WATER))
		entity->client->ps.rdflags |= RDF_UNDERWATER;
	else
		entity->client->ps.rdflags &= ~RDF_UNDERWATER;

	if (contents & (CONTENTS_SOLID|CONTENTS_LAVA))
		SG_AddBlend(1.0f, 0.3f, 0.0f, 0.6f, entity->client->ps.blend);
	else if (contents & CONTENTS_SLIME)
		SG_AddBlend(0.0f, 0.1f, 0.05f, 0.6f, entity->client->ps.blend);
	else if (contents & CONTENTS_WATER)
		SG_AddBlend(0.5f, 0.3f, 0.2f, 0.4f, entity->client->ps.blend);

	// Add for powerups
	if (entity->client->quad_framenum > level.framenum){
		remaining = entity->client->quad_framenum - level.framenum;

		if (remaining == 30)	// Beginning to fade
			gi.sound(entity, CHAN_ITEM, gi.soundindex("items/damage2.wav"), 1.0f, ATTN_NORM, 0.0f);
		if (remaining > 30 || (remaining & 4))
			SG_AddBlend(0.0f, 0.0f, 1.0f, 0.08f, entity->client->ps.blend);
	}
	else if (entity->client->invincible_framenum > level.framenum){
		remaining = entity->client->invincible_framenum - level.framenum;

		if (remaining == 30)	// Beginning to fade
			gi.sound(entity, CHAN_ITEM, gi.soundindex("items/protect2.wav"), 1.0f, ATTN_NORM, 0.0f);
		if (remaining > 30 || (remaining & 4))
			SG_AddBlend(1.0f, 1.0f, 0.0f, 0.08f, entity->client->ps.blend);
	}
	else if (entity->client->enviro_framenum > level.framenum){
		remaining = entity->client->enviro_framenum - level.framenum;

		if (remaining == 30)	// Beginning to fade
			gi.sound(entity, CHAN_ITEM, gi.soundindex("items/airout.wav"), 1.0f, ATTN_NORM, 0.0f);
		if (remaining > 30 || (remaining & 4))
			SG_AddBlend(0.0f, 1.0f, 0.0f, 0.08f, entity->client->ps.blend);
	}
	else if (entity->client->breather_framenum > level.framenum){
		remaining = entity->client->breather_framenum - level.framenum;

		if (remaining == 30)	// Beginning to fade
			gi.sound(entity, CHAN_ITEM, gi.soundindex("items/airout.wav"), 1.0f, ATTN_NORM, 0.0f);
		if (remaining > 30 || (remaining & 4))
			SG_AddBlend(0.4f, 1.0f, 0.4f, 0.04f, entity->client->ps.blend);
	}

	// Add for damage
	if (entity->client->damage_alpha > 0.0f)
		SG_AddBlend(entity->client->damage_blend[0], entity->client->damage_blend[1], entity->client->damage_blend[2], entity->client->damage_alpha, entity->client->ps.blend);

	if (entity->client->bonus_alpha > 0.0f)
		SG_AddBlend(0.85f, 0.7f, 0.3f, entity->client->bonus_alpha, entity->client->ps.blend);

	// Drop the damage value
	entity->client->damage_alpha -= 0.06f;
	if (entity->client->damage_alpha < 0.0f)
		entity->client->damage_alpha = 0.0f;

	// Drop the bonus value
	entity->client->bonus_alpha -= 0.1f;
	if (entity->client->bonus_alpha < 0.0f)
		entity->client->bonus_alpha = 0.0f;
}

/*
 ==================
 SG_FallingDamage
 ==================
*/
static void SG_FallingDamage (edict_t *entity){

	float	delta;
	int		damage;
	vec3_t	dir;

	if (entity->s.modelindex != 255)
		return;		// Not in the player model

	if (entity->movetype == MOVETYPE_NOCLIP)
		return;

	if ((entity->client->oldvelocity[2] < 0.0f) && (entity->velocity[2] > entity->client->oldvelocity[2]) && (!entity->groundentity))
		delta = entity->client->oldvelocity[2];
	else {
		if (!entity->groundentity)
			return;

		delta = entity->velocity[2] - entity->client->oldvelocity[2];
	}

	delta = delta * delta * 0.0001f;

	// Never take falling damage if completely underwater
	if (entity->waterLevel == WATERLEVEL_HEAD)
		return;
	if (entity->waterLevel == WATERLEVEL_WAIST)
		delta *= 0.25f;
	if (entity->waterLevel == WATERLEVEL_FEET)
		delta *= 0.5f;

	if (delta < 1.0f)
		return;

	if (delta < 15.0f){
		entity->s.event = EV_FOOTSTEP;
		return;
	}

	entity->client->fall_value = delta * 0.5f;
	if (entity->client->fall_value > 40.0f)
		entity->client->fall_value = 40.0f;

	entity->client->fall_time = level.time + FALL_TIME;

	if (delta > 30.0f){
		if (entity->health > 0){
			if (delta >= 55)
				entity->s.event = EV_FALLFAR;
			else
				entity->s.event = EV_FALL;
		}

		// No normal pain sound
		entity->pain_debounce_time = level.time;

		damage = (delta - 30) / 2;
		if (damage < 1)
			damage = 1;

		VectorSet(dir, 0.0f, 0.0f, 1.0f);

		if (!deathmatch->value || !((int)dmflags->value & DF_NO_FALLING))
			SG_TargetDamage(entity, world, world, dir, entity->s.origin, vec3_origin, damage, 0, 0, MOD_FALLING);
	}
	else {
		entity->s.event = EV_FALLSHORT;
		return;
	}
}

/*
 ==================
 SG_WorldEffects
 ==================
*/
static void SG_WorldEffects (){

	qboolean	breather;
	qboolean	environmentSuit;
	int			waterLevel, oldWaterLevel;

	if (current_player->movetype == MOVETYPE_NOCLIP){
		// Don't need air
		current_player->air_finished = level.time + 12.0f;
		return;
	}

	waterLevel = current_player->waterLevel;
	oldWaterLevel = current_client->old_waterlevel;
	current_client->old_waterlevel = waterLevel;

	breather = current_client->breather_framenum > level.framenum;
	environmentSuit = current_client->enviro_framenum > level.framenum;

	// If just entered a water volume, play a sound
	if (!oldWaterLevel && waterLevel){
		SG_PlayerNoise(current_player, current_player->s.origin, PNOISE_SELF);

		if (current_player->waterType & CONTENTS_LAVA)
			gi.sound (current_player, CHAN_BODY, gi.soundindex("player/lava_in.wav"), 1.0f, ATTN_NORM, 0.0f);
		else if (current_player->waterType & CONTENTS_SLIME)
			gi.sound (current_player, CHAN_BODY, gi.soundindex("player/watr_in.wav"), 1.0f, ATTN_NORM, 0.0f);
		else if (current_player->waterType & CONTENTS_WATER)
			gi.sound (current_player, CHAN_BODY, gi.soundindex("player/watr_in.wav"), 1.0f, ATTN_NORM, 0.0f);
		
		current_player->flags |= FL_INWATER;

		// Clear damage_debounce, so the pain sound will play immediately
		current_player->damage_debounce_time = level.time - 1.0f;
	}

	// If just completely exited a water volume, play a sound
	if (oldWaterLevel && ! waterLevel){
		SG_PlayerNoise(current_player, current_player->s.origin, PNOISE_SELF);

		gi.sound(current_player, CHAN_BODY, gi.soundindex("player/watr_out.wav"), 1.0f, ATTN_NORM, 0.0f);

		current_player->flags &= ~FL_INWATER;
	}

	// Check for head just going under water
	if (oldWaterLevel != WATERLEVEL_HEAD && waterLevel == WATERLEVEL_HEAD)
		gi.sound(current_player, CHAN_BODY, gi.soundindex("player/watr_un.wav"), 1.0f, ATTN_NORM, 0.0f);

	// Check for head just coming out of water
	if (oldWaterLevel == WATERLEVEL_HEAD && waterLevel != WATERLEVEL_HEAD){
		if (current_player->air_finished < level.time){		// Gasp for air
			gi.sound (current_player, CHAN_VOICE, gi.soundindex("player/gasp1.wav"), 1.0f, ATTN_NORM, 0.0f);
			SG_PlayerNoise(current_player, current_player->s.origin, PNOISE_SELF);
		}
		else  if (current_player->air_finished < level.time + 11)	// Just break surface
			gi.sound (current_player, CHAN_VOICE, gi.soundindex("player/gasp2.wav"), 1.0f, ATTN_NORM, 0.0f);
	}

	// Check for drowning
	if (waterLevel == WATERLEVEL_HEAD){
		// Breather or envirosuit give air
		if (breather || environmentSuit){
			current_player->air_finished = level.time + 10.0f;

			if (((int)(current_client->breather_framenum - level.framenum) % 25) == 0){
				if (!current_client->breather_sound)
					gi.sound (current_player, CHAN_AUTO, gi.soundindex("player/u_breath1.wav"), 1.0f, ATTN_NORM, 0.0f);
				else
					gi.sound (current_player, CHAN_AUTO, gi.soundindex("player/u_breath2.wav"), 1.0f, ATTN_NORM, 0.0f);

				current_client->breather_sound ^= 1;

				SG_PlayerNoise(current_player, current_player->s.origin, PNOISE_SELF);

				// FIXME: Release a bubble?
			}
		}

		// If out of air, start drowning
		if (current_player->air_finished < level.time){
			if (current_player->client->next_drown_time < level.time && current_player->health > 0){
				current_player->client->next_drown_time = level.time + 1.0f;

				// Take more damage the longer underwater
				current_player->dmg += 2;
				if (current_player->dmg > 15)
					current_player->dmg = 15;

				// Play a gurp sound instead of a normal pain sound
				if (current_player->health <= current_player->dmg)
					gi.sound(current_player, CHAN_VOICE, gi.soundindex("player/drown1.wav"), 1.0f, ATTN_NORM, 0.0f);
				else if (rand() & 1)
					gi.sound(current_player, CHAN_VOICE, gi.soundindex("*gurp1.wav"), 1.0f, ATTN_NORM, 0.0f);
				else
					gi.sound(current_player, CHAN_VOICE, gi.soundindex("*gurp2.wav"), 1.0f, ATTN_NORM, 0.0f);

				current_player->pain_debounce_time = level.time;

				SG_TargetDamage(current_player, world, world, vec3_origin, current_player->s.origin, vec3_origin, current_player->dmg, 0, DAMAGE_NO_ARMOR, MOD_WATER);
			}
		}
	}
	else {
		current_player->air_finished = level.time + 12;
		current_player->dmg = 2;
	}

	// Check for sizzle damage
	if (waterLevel && (current_player->waterType&(CONTENTS_LAVA|CONTENTS_SLIME))){
		if (current_player->waterType & CONTENTS_LAVA){
			if (current_player->health > 0 && current_player->pain_debounce_time <= level.time && current_client->invincible_framenum < level.framenum){
				if (rand() & 1)
					gi.sound(current_player, CHAN_VOICE, gi.soundindex("player/burn1.wav"), 1.0f, ATTN_NORM, 0.0f);
				else
					gi.sound(current_player, CHAN_VOICE, gi.soundindex("player/burn2.wav"), 1.0f, ATTN_NORM, 0.0f);

				current_player->pain_debounce_time = level.time + 1.0f;
			}

			// Take 1/3 damage with environment suit
			if (environmentSuit)
				SG_TargetDamage(current_player, world, world, vec3_origin, current_player->s.origin, vec3_origin, 1*waterLevel, 0, 0, MOD_LAVA);
			else
				SG_TargetDamage(current_player, world, world, vec3_origin, current_player->s.origin, vec3_origin, 3*waterLevel, 0, 0, MOD_LAVA);
		}

		if (current_player->waterType & CONTENTS_SLIME){
			// No damage from slime with environment suit
			if (!environmentSuit)
				SG_TargetDamage(current_player, world, world, vec3_origin, current_player->s.origin, vec3_origin, 1*waterLevel, 0, 0, MOD_SLIME);
		}
	}
}

/*
 ==================
 SG_SetClientEvent
 ==================
*/
static void SG_SetClientEvent (edict_t *entity){

	if (entity->s.event)
		return;

	if (entity->groundentity && xyspeed > 225){
		if ((int)(current_client->bobtime + bobmove) != bobcycle)
			entity->s.event = EV_FOOTSTEP;
	}
}

/*
 ==================
 SG_SetClientEffects
 ==================
*/
static void SG_SetClientEffects (edict_t *entity){

	int		powerArmorType;
	int		remaining;

	entity->s.effects = 0;
	entity->s.renderfx = 0;

	if (entity->health <= 0 || level.intermissiontime)
		return;

	if (entity->powerarmor_time > level.time){
		powerArmorType = SG_FindPowerArmorByType(entity);

		if (powerArmorType == POWER_ARMOR_SCREEN)
			entity->s.effects |= EF_POWERSCREEN;
		else if (powerArmorType == POWER_ARMOR_SHIELD){
			entity->s.effects |= EF_COLOR_SHELL;
			entity->s.renderfx |= RF_SHELL_GREEN;
		}
	}

	if (entity->client->quad_framenum > level.framenum){
		remaining = entity->client->quad_framenum - level.framenum;

		if (remaining > 30 || (remaining & 4))
			entity->s.effects |= EF_QUAD;
	}

	if (entity->client->invincible_framenum > level.framenum){
		remaining = entity->client->invincible_framenum - level.framenum;

		if (remaining > 30 || (remaining & 4))
			entity->s.effects |= EF_PENT;
	}

	// Show cheaters!!!
	if (entity->flags & FL_GODMODE){
		entity->s.effects |= EF_COLOR_SHELL;
		entity->s.renderfx |= (RF_SHELL_RED|RF_SHELL_GREEN|RF_SHELL_BLUE);
	}
}

/*
 ==================
 SG_SetClientSound
 ==================
*/
static void G_SetClientSound (edict_t *entity){

	char	*name;

	if (entity->client->pers.game_helpchanged != game.helpchanged){
		entity->client->pers.game_helpchanged = game.helpchanged;
		entity->client->pers.helpchanged = 1;
	}

	// Help beep (no more than three times)
	if (entity->client->pers.helpchanged && entity->client->pers.helpchanged <= 3 && !(level.framenum & 63)){
		entity->client->pers.helpchanged++;
		gi.sound(entity, CHAN_VOICE, gi.soundindex("misc/pc_up.wav"), 1.0f, ATTN_STATIC, 0.0f);
	}


	if (entity->client->pers.weapon)
		name = entity->client->pers.weapon->className;
	else
		name = "";

	if (entity->waterLevel && (entity->waterType & (CONTENTS_LAVA|CONTENTS_SLIME)))
		entity->s.sound = snd_fry;
	else if (!strcmp(name, "weapon_railgun"))
		entity->s.sound = gi.soundindex("weapons/rg_hum.wav");
	else if (!strcmp(name, "weapon_bfg"))
		entity->s.sound = gi.soundindex("weapons/bfg_hum.wav");
	else if (entity->client->weapon_sound)
		entity->s.sound = entity->client->weapon_sound;
	else
		entity->s.sound = 0;
}

/*
 ==================
 SG_SetClientFrame
 ==================
*/
void SG_SetClientFrame (edict_t *entity){

	gclient_t	*client;
	qboolean	duck, run;

	if (entity->s.modelindex != 255)
		return;		// Not in the player model

	client = entity->client;

	if (client->ps.pmove.pm_flags & PMF_DUCKED)
		duck = true;
	else
		duck = false;

	if (xyspeed)
		run = true;
	else
		run = false;

	// Check for stand/duck and stop/go transitions
	if (duck != client->anim_duck && client->anim_priority < ANIM_DEATH)
		goto newAnimation;
	if (run != client->anim_run && client->anim_priority == ANIM_BASIC)
		goto newAnimation;
	if (!entity->groundentity && client->anim_priority <= ANIM_WAVE)
		goto newAnimation;

	if (client->anim_priority == ANIM_REVERSE){
		if (entity->s.frame > client->anim_end){
			entity->s.frame--;
			return;
		}
	}
	else if (entity->s.frame < client->anim_end){
		// Continue an animation
		entity->s.frame++;
		return;
	}

	if (client->anim_priority == ANIM_DEATH)
		return;		// Stay there

	if (client->anim_priority == ANIM_JUMP){
		if (!entity->groundentity)
			return;	// Stay there

		entity->client->anim_priority = ANIM_WAVE;
		entity->s.frame = FRAME_jump3;
		entity->client->anim_end = FRAME_jump6;
		return;
	}

newAnimation:
	// Return to either a running or standing frame
	client->anim_priority = ANIM_BASIC;
	client->anim_duck = duck;
	client->anim_run = run;

	if (!entity->groundentity){
		client->anim_priority = ANIM_JUMP;
		if (entity->s.frame != FRAME_jump2)
			entity->s.frame = FRAME_jump1;

		client->anim_end = FRAME_jump2;
	}
	else if (run){
		// Running
		if (duck){
			entity->s.frame = FRAME_crwalk1;
			client->anim_end = FRAME_crwalk6;
		}
		else {
			entity->s.frame = FRAME_run1;
			client->anim_end = FRAME_run6;
		}
	}
	else {
		// Standing
		if (duck){
			entity->s.frame = FRAME_crstnd01;
			client->anim_end = FRAME_crstnd19;
		}
		else {
			entity->s.frame = FRAME_stand01;
			client->anim_end = FRAME_stand40;
		}
	}
}

/*
 ==================
 SG_ClientEndServerFrame

 Called for each player at the end of the server frame
 and right after spawning
 ==================
*/
void SG_ClientEndServerFrame (edict_t *entity){

	float	bobTime;
	int		i;

	current_player = entity;
	current_client = entity->client;

	// If the origin or velocity have changed since SG_ClientThinkFramework(),
	// update the pmove values.  This will happen when the client
	// is pushed by a bmodel or kicked by an explosion.

	// If it wasn't updated here, the view position would lag a frame
	// behind the body position when pushed -- "sinking into plats"
	for (i = 0; i < 3; i++){
		current_client->ps.pmove.origin[i] = entity->s.origin[i] * 8.0f;
		current_client->ps.pmove.velocity[i] = entity->velocity[i] * 8.0f;
	}

	// If the end of unit layout is displayed, don't give
	// the player any normal movement attributes
	if (level.intermissiontime){
		// FIXME: Add view drifting here?
		current_client->ps.blend[3] = 0.0f;
		current_client->ps.fov = 90.0f;

		SG_SetStats(entity);
		return;
	}

	AngleVectors(entity->client->v_angle, forward, right, up);

	// Burn from lava, etc
	SG_WorldEffects();

	// Set model angles from view angles so other things in
	// the world can tell which direction you are looking
	if (entity->client->v_angle[PITCH] > 180.0f)
		entity->s.angles[PITCH] = (-360.0f + entity->client->v_angle[PITCH]) / 3.0f;
	else
		entity->s.angles[PITCH] = entity->client->v_angle[PITCH] / 3.0f;

	entity->s.angles[YAW] = entity->client->v_angle[YAW];
	entity->s.angles[ROLL] = 0.0f;
	entity->s.angles[ROLL] = SG_CalculateRoll(entity->s.angles, entity->velocity) * 4.0f;

	// Calculate speed and cycle to be used for
	// all cyclic walking effects
	xyspeed = sqrt(entity->velocity[0] * entity->velocity[0] + entity->velocity[1] * entity->velocity[1]);

	if (xyspeed < 5){
		bobmove = 0.0f;
		current_client->bobtime = 0.0f;		// Start at beginning of cycle again
	}
	else if (entity->groundentity){
		// So bobbing only cycles when on ground
		if (xyspeed > 210.0f)
			bobmove = 0.25f;
		else if (xyspeed > 100.0f)
			bobmove = 0.125f;
		else
			bobmove = 0.0625f;
	}

	bobTime = (current_client->bobtime += bobmove);

	if (current_client->ps.pmove.pm_flags & PMF_DUCKED)
		bobTime *= 4.0f;

	bobcycle = (int)bobTime;
	bobfracsin = fabs(sin(bobTime * M_PI));

	// Detect hitting the floor
	SG_FallingDamage(entity);

	// Apply all the damage taken this frame
	SG_ClientDamageFeedback(entity);

	// Determine the view and gun offsets
	SG_CalculateViewOffset(entity);
	SG_CalculateGunOffset(entity);

	// Determine the full screen color blend
	// must be after viewoffset, so eye contents can be
	// accurately determined
	SG_CalculateBlend(entity);

	// Chase camera
	if (entity->client->resp.spectator)
		SG_SetSpectatorStats(entity);
	else
		SG_SetStats(entity);

	SG_CheckChaseStats(entity);
	SG_SetClientEvent(entity);
	SG_SetClientEffects(entity);
	G_SetClientSound(entity);
	SG_SetClientFrame(entity);

	VectorCopy(entity->velocity, entity->client->oldvelocity);
	VectorCopy(entity->client->ps.viewangles, entity->client->oldviewangles);

	// Clear weapon kicks
	VectorClear(entity->client->kick_origin);
	VectorClear(entity->client->kick_angles);

	// If the scoreboard is up, update it
	if (entity->client->showscores && !(level.framenum & 31)){
		SG_DeathmatchScoreboardLayout(entity, entity->enemy);
		gi.unicast(entity, false);
	}
}