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
// sg_chaseCamera.c - Chase camera used by spectators
//


#include "g_local.h"


/*
 ==============================================================================

 FRAMEWORK

 ==============================================================================
*/


/*
 ==================
 SG_UpdateChaseCam
 ==================
*/
static void SG_UpdateChaseCam (edict_t *entity){

	edict_t *oldTarget, *target;
	vec3_t	ownerView, o;
	vec3_t	angles;
	vec3_t	forward, right;
	trace_t trace;
	vec3_t	goal;

	if (!entity->client->chaseTarget)
		return;

	// Switch to chase mode from free mode
	if (entity->client->chaseMode == SPM_FREE)
		entity->client->chaseMode = SPM_CHASE;

	// Is our chase target gone?
	if (!entity->client->chaseTarget->inuse || (entity->client->chaseTarget->client && entity->client->chaseTarget->client->resp.spectator) || ((entity->client->chaseTarget->svflags & SVF_MONSTER) && entity->client->chaseTarget->health <= 0)){
		oldTarget = entity->client->chaseTarget;

		SG_ChaseNext(entity);

		if (entity->client->chaseTarget == oldTarget){
			entity->client->chaseMode = SPM_FREE;
			entity->client->chaseTarget = NULL;
			entity->client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;

			return;
		}
	}

	// Get the current target
	target = entity->client->chaseTarget;

	// Compute the view origin and angels
	VectorCopy(target->s.origin, ownerView);
	ownerView[2] += target->viewheight;

	if (target->client)
		VectorCopy(target->client->v_angle, angles);
	else {
		ownerView[2] += (target->mins[2] / 2.0f) + (target->maxs[2] / 2.0f);
		ownerView[1] += (target->mins[1] / 2.0f) + (target->maxs[1] / 2.0f);
		ownerView[0] += (target->mins[0] / 2.0f) + (target->maxs[0] / 2.0f);

		VectorCopy(target->s.angles, angles);
	}

	if (entity->client->chaseMode == SPM_EYES && !target->client && target->enemy && SG_IsInFOV(target, target->enemy, 90)){
		VectorSubtract(target->enemy->s.origin, target->s.origin, forward);
		SG_VectorToAngles(forward, forward);

		angles[PITCH] = forward[PITCH];
	}

	if (angles[PITCH] > 56.0f)
		angles[PITCH] = 56.0f;

	AngleVectors(angles, forward, right, NULL);
	VectorNormalize(forward);

	if (entity->client->chaseMode == SPM_CHASE){
		if (target->client){
			ownerView[2] += 20.0f;
			VectorMA(ownerView, -50.0f, forward, o);
		}
		else {
			ownerView[2] += 40.0f;
			VectorMA(ownerView, -50.0f, forward, o);
		}

		if (o[2] < target->s.origin[2] + 20.0f)
			o[2] = target->s.origin[2] + 20.0f;
	}
	else {
		if (target->client){
			VectorMA(ownerView, target->maxs[0], forward, o);
			o[2] = target->s.origin[2] + target->viewheight;
		}
		else {
			VectorMA(ownerView, target->maxs[0] + 6, forward, o);
			o[2] = target->s.origin[2] + target->maxs[2] - 10;
		}
	}

	// Jump animation lifts
	if (!target->groundentity && target->client)
		o[2] += 16.0f;

	// Trace
	trace = gi.trace(target->s.origin, vec3_origin, vec3_origin, o, target, MASK_SOLID);

	VectorCopy(trace.endpos, goal);
	VectorMA(goal, 2.0f, forward, goal);

	// Pad for floors and ceilings
	VectorCopy(goal, o);
	o[2] += 6.0f;

	trace = gi.trace(goal, vec3_origin, vec3_origin, o, target, MASK_SOLID);

	if (trace.fraction < 1.0f){
		VectorCopy(trace.endpos, goal);
		goal[2] -= 6.0f;
	}

	VectorCopy(goal, o);
	o[2] -= 6.0f;

	trace = gi.trace(goal, vec3_origin, vec3_origin, o, target, MASK_SOLID);

	if (trace.fraction < 1.0f){
		VectorCopy(trace.endpos, goal);
		goal[2] += 6.0f;
	}

	if (target->deadflag)
		entity->client->ps.pmove.pm_type = PM_DEAD;
	else
		entity->client->ps.pmove.pm_type = PM_FREEZE;

	VectorCopy(goal, entity->s.origin);

	if (target->client){
		entity->client->ps.pmove.delta_angles[0] = ANGLE2SHORT(target->client->v_angle[0] - entity->client->resp.cmd_angles[0]);
		entity->client->ps.pmove.delta_angles[1] = ANGLE2SHORT(target->client->v_angle[1] - entity->client->resp.cmd_angles[1]);
		entity->client->ps.pmove.delta_angles[2] = ANGLE2SHORT(target->client->v_angle[2] - entity->client->resp.cmd_angles[2]);
	}

	if (target->deadflag){
		entity->client->ps.viewangles[ROLL] = 40.0f;
		entity->client->ps.viewangles[PITCH] = -15.0f;

		if (target->client)
			entity->client->ps.viewangles[YAW] = target->client->killer_yaw;
		else
			entity->client->ps.viewangles[YAW] = target->s.angles[YAW];
	}
	else {
		if (target->client){
			VectorCopy(target->client->v_angle, entity->client->ps.viewangles);
			VectorCopy(target->client->v_angle, entity->client->v_angle);
		}
		else {
			VectorCopy(angles, entity->client->ps.viewangles);
			VectorCopy(angles, entity->client->v_angle);

			if (entity->client->chaseMode == SPM_CHASE){
				entity->client->ps.viewangles[0] = 20.0f;
				entity->client->v_angle[0] = 20.0f;
			}
		}
	}

	entity->viewheight = 0;
	entity->client->ps.pmove.pm_flags |= PMF_NO_PREDICTION;

	gi.linkentity(entity);
}

/*
 ==================
 SG_ChaseCamThinkFramework
 ==================
*/
void SG_ChaseCamThinkFramework (usercmd_t *userCmd, gclient_t *client, edict_t *entity){

	int		i;
	edict_t	*other;

	// Fire weapon from final position if needed
	if (client->latched_buttons & BUTTON_ATTACK){
		if (client->resp.spectator){
			client->latched_buttons = 0;

			switch (client->chaseMode){
			case SPM_EYES:
				gi.cprintf(entity, PRINT_MEDIUM, "Free look mode.\n");

				client->chaseMode = SPM_FREE;
				client->chaseTarget = NULL;
				client->ps.pmove.pm_flags &= ~PMF_NO_PREDICTION;
				break;
			case SPM_CHASE:
				gi.cprintf(entity, PRINT_MEDIUM, "In-eyes camera mode.\n");

				client->chaseMode = SPM_EYES;
				SG_UpdateChaseCam(entity);
				break;
			case SPM_FREE:
				client->chaseMode = SPM_CHASE;
				SG_GetChaseTarget(entity);

				// May not have anyone to chase so we have to check again
				if (client->chaseMode == SPM_CHASE)
					gi.cprintf(entity, PRINT_MEDIUM, "Chase camera mode.\n");
				break;
			}
		}
		else if (!client->weaponThink){
			client->weaponThink = true;
			SG_WeaponThinkFramework(entity);
		}
	}

	if (client->resp.spectator){
		if (userCmd->upmove >= 10){
			if (!(client->ps.pmove.pm_flags & PMF_JUMP_HELD)){
				client->ps.pmove.pm_flags |= PMF_JUMP_HELD;

				if (client->chaseTarget)
					SG_ChaseNext(entity);
				else
					SG_GetChaseTarget(entity);
			}
		}
		else
			client->ps.pmove.pm_flags &= ~PMF_JUMP_HELD;
	}

	// Update chase cam if being followed
	for (i = 1; i <= maxclients->value; i++){
		other = g_edicts + i;

		if (other->inuse && other->client->chaseTarget == entity)
			SG_UpdateChaseCam(other);
	}

	SG_UpdateChaseCam(entity);
}


/*
 ==============================================================================

 TARGET SELECTION

 ==============================================================================
*/


/*
 ==================
 SG_ChaseNext
 ==================
*/
void SG_ChaseNext (edict_t *entity){

	int		i;
	edict_t	*e;

	if (!entity->client->chaseTarget)
		return;

	i = entity->client->chaseTarget - g_edicts;
	do {
		i++;

		if (i > maxentities->value)
			i = 1;

		e = g_edicts + i;

		if (!e->inuse)
			continue;
		if (e->client && !e->client->resp.spectator)
			break;
		if ((e->svflags & SVF_MONSTER) && !e->deadflag)
			break;
	} while (e != entity->client->chaseTarget);

	entity->client->chaseTarget = e;
	entity->client->updateChase = true;
}

/*
 ==================
 SG_ChasePrev
 ==================
*/
void SG_ChasePrev (edict_t *entity){

	int		i;
	edict_t	*e;

	if (!entity->client->chaseTarget)
		return;

	i = entity->client->chaseTarget - g_edicts;
	do {
		i--;

		if (i < 1)
			i = maxentities->value - 1;

		e = g_edicts + i;

		if (!e->inuse)
			continue;
		if (e->client && !e->client->resp.spectator)
			break;
		if ((e->svflags & SVF_MONSTER) && !e->deadflag)
			break;
	} while (e != entity->client->chaseTarget);

	entity->client->chaseTarget = e;
	entity->client->updateChase = true;
}

/*
 ==================
 SG_GetChaseTarget
 ==================
*/
void SG_GetChaseTarget (edict_t *entity){

	int		i;
	edict_t	*other;

	for (i = 1; i <= maxclients->value; i++){
		other = g_edicts + i;

		if (other->inuse && !other->client->resp.spectator){
			entity->client->chaseTarget = other;
			entity->client->updateChase = true;

			SG_UpdateChaseCam(entity);
			return;
		}
	}

	for ( ; i < maxentities->value; i++){
		other = g_edicts + i;

		if (other->inuse && (other->svflags & SVF_MONSTER) && !other->deadflag){
			entity->client->chaseTarget = other;
			entity->client->updateChase = true;

			SG_UpdateChaseCam(entity);
			return;
		}
	}

	entity->client->chaseMode = SPM_FREE;

	gi.centerprintf(entity, "No other players to chase.");
}