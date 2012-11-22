//
// sg_physics.c - Physics behaviour
//


#include "g_local.h"


#define	STOP_EPSILON					0.1f
#define	MAX_CLIP_PLANES					5

typedef struct {
	edict_t *			ent;
	vec3_t				origin;
	vec3_t				angles;
	float				deltayaw;
} pushed_t;

static pushed_t			pushed[MAX_EDICTS], *pushed_p;

static edict_t *		obstacle;


/*
pushmove objects do not obey gravity, and do not interact with each other or trigger fields, but block normal movement and push normal objects when they move.

onground is set for toss objects when they come to a complete rest.  it is set for steping or walking objects 

doors, plats, etc are SOLID_BSP, and MOVETYPE_PUSH
bonus items are SOLID_TRIGGER touch, and MOVETYPE_TOSS
corpses are SOLID_NOT and MOVETYPE_TOSS
crates are SOLID_BBOX and MOVETYPE_TOSS
walking monsters are SOLID_SLIDEBOX and MOVETYPE_STEP
flying/floating monsters are SOLID_SLIDEBOX and MOVETYPE_FLY

solid_edge items only clip against bsp models.
*/


/*
 ==================
 
 ==================
*/
static edict_t *SV_TestEntityPosition (edict_t *entity){

	int		mask;
	trace_t	trace;

	if (entity->clipmask)
		mask = entity->clipmask;
	else
		mask = MASK_SOLID;

	trace = gi.trace(entity->s.origin, entity->mins, entity->maxs, entity->s.origin, entity, mask);
	
	if (trace.startsolid)
		return g_edicts;
		
	return NULL;
}

/*
 ==================
 SG_CheckVelocity

 Checks max server velocity
 ==================
*/
static void SG_CheckVelocity (edict_t *entity){

	int		i;

	// Bound velocity
	for (i = 0; i < 3; i++){
		if (entity->velocity[i] > sv_maxvelocity->value)
			entity->velocity[i] = sv_maxvelocity->value;
		else if (entity->velocity[i] < -sv_maxvelocity->value)
			entity->velocity[i] = -sv_maxvelocity->value;
	}
}

/*
 ==================
 
 Runs thinking code for this frame if necessary
 ==================
*/
static qboolean SV_RunThink (edict_t *entity){

	float	time;

	time = entity->nextthink;

	if (time <= 0.0f)
		return true;
	if (time > level.time + 0.001f)
		return true;

	entity->nextthink = 0.0f;

	if (!entity->think)
		gi.error("SV_RunThink: NULL ent->think");

	entity->think(entity);

	return false;
}

/*
 ==================
 SG_EntityImpact

 Two entities have touched, so run their touch functions
 ==================
*/
static void SG_EntityImpact (edict_t *e1, trace_t *trace){

	edict_t	*e2;

	e2 = trace->ent;

	if (e1->touch && e1->solid != SOLID_NOT)
		e1->touch(e1, e2, &trace->plane, trace->surface);

	if (e2->touch && e2->solid != SOLID_NOT)
		e2->touch(e2, e1, NULL, NULL);
}

/*
 ==================
 
 Slide off of the impacting object
 Returns the blocked flags (1 = floor, 2 = step / wall)
 ==================
*/
static int ClipVelocity (vec3_t in, vec3_t normal, vec3_t out, float overbounce){

	float	backOff;
	float	change;
	int		i, blocked;

	blocked = 0;

	// Floor
	if (normal[2] > 0.0f)
		blocked |= 1;

	// Step
	if (!normal[2])
		blocked |= 2;

	backOff = DotProduct (in, normal) * overbounce;

	for (i = 0; i < 3; i++){
		change = normal[i] * backOff;

		out[i] = in[i] - change;

		if (out[i] > -STOP_EPSILON && out[i] < STOP_EPSILON)
			out[i] = 0.0f;
	}

	return blocked;
}

/*
 ==================
 SG_FlyMovement

 The basic solid body movement clip that slides along multiple planes
 Returns the clipflags if the velocity was modified (hit something solid)
 1 = floor
 2 = wall / step
 4 = dead stop
 ==================
*/
static int SG_FlyMovement (edict_t *ent, float time, int mask){

	edict_t	*hit;
	int		bumpCount, numbumps;
	vec3_t	dir;
	float	d;
	int		numplanes;
	vec3_t	planes[MAX_CLIP_PLANES];
	vec3_t	primal_velocity, original_velocity, new_velocity;
	int		i, j;
	trace_t	trace;
	vec3_t	end;
	float	time_left;
	int		blocked;
	
	numbumps = 4;
	
	blocked = 0;

	VectorCopy(ent->velocity, original_velocity);
	VectorCopy(ent->velocity, primal_velocity);

	numplanes = 0;
	
	time_left = time;

	ent->groundentity = NULL;

	for (bumpCount = 0; bumpCount < numbumps; bumpCount++){
		end[0] = ent->s.origin[0] + time_left * ent->velocity[0];
		end[1] = ent->s.origin[1] + time_left * ent->velocity[1];
		end[2] = ent->s.origin[2] + time_left * ent->velocity[2];

		trace = gi.trace(ent->s.origin, ent->mins, ent->maxs, end, ent, mask);

		// Entity is trapped in another solid
		if (trace.allsolid){
			VectorCopy(vec3_origin, ent->velocity);
			return 3;
		}

		// Actually covered some distance
		if (trace.fraction > 0.0f){
			VectorCopy(trace.endpos, ent->s.origin);
			VectorCopy(ent->velocity, original_velocity);

			numplanes = 0;
		}

		if (trace.fraction == 1.0f)
			 break;		// Moved the entire distance

		hit = trace.ent;

		if (trace.plane.normal[2] > 0.7f){
			blocked |= 1;		// Floor

			if (hit->solid == SOLID_BSP){
				ent->groundentity = hit;
				ent->groundentity_linkcount = hit->linkcount;
			}
		}

		// Step
		if (!trace.plane.normal[2])
			blocked |= 2;

		// Run the impact function
		SG_EntityImpact(ent, &trace);

		if (!ent->inuse)
			break;		// Removed by the impact function

		
		time_left -= time_left * trace.fraction;
		
		// Cliped to another plane
		if (numplanes >= MAX_CLIP_PLANES){
			// This shouldn't really happen
			VectorCopy(vec3_origin, ent->velocity);
			return 3;
		}

		VectorCopy(trace.plane.normal, planes[numplanes]);
		numplanes++;

		// Modify original_velocity so it parallels all of the clip planes
		for (i = 0; i < numplanes; i++){
			ClipVelocity(original_velocity, planes[i], new_velocity, 1);

			for (j = 0; j < numplanes; j++)
				if ((j != i) && !VectorCompare(planes[i], planes[j])){
					if (DotProduct(new_velocity, planes[j]) < 0.0f)
						break;		// Not ok
				}

			if (j == numplanes)
				break;
		}
		
		// Go along this plane
		if (i != numplanes)
			VectorCopy(new_velocity, ent->velocity);
		else {
			// Go along the crease
			if (numplanes != 2){
				VectorCopy (vec3_origin, ent->velocity);
				return 7;
			}

			CrossProduct(planes[0], planes[1], dir);
			d = DotProduct(dir, ent->velocity);
			VectorScale(dir, d, ent->velocity);
		}

		// If original velocity is against the original velocity, stop dead
		// to avoid tiny occilations in sloping corners
		if (DotProduct(ent->velocity, primal_velocity) <= 0.0f){
			VectorCopy(vec3_origin, ent->velocity);
			return blocked;
		}
	}

	return blocked;
}

/*
 ==================
 SG_AddGravity
 ==================
*/
static void SG_AddGravity (edict_t *entity){

	entity->velocity[2] -= entity->gravity * sv_gravity->value * FRAMETIME;
}


/*
 ==============================================================================

 PUSHMOVE

 ==============================================================================
*/


/*
 ==================
 
 Does not change the entities velocity at all
 ==================
*/
static trace_t SV_PushEntity (edict_t *entity, vec3_t push){

	trace_t	trace;
	vec3_t	start;
	vec3_t	end;
	int		mask;

	VectorCopy(entity->s.origin, start);
	VectorAdd(start, push, end);

retry:
	if (entity->clipmask)
		mask = entity->clipmask;
	else
		mask = MASK_SOLID;

	trace = gi.trace(start, entity->mins, entity->maxs, end, entity, mask);

	VectorCopy(trace.endpos, entity->s.origin);
	gi.linkentity(entity);

	if (trace.fraction != 1.0f){
		// Run the impact function
		SG_EntityImpact(entity, &trace);

		// If the pushed entity went away and the pusher is still there
		if (!trace.ent->inuse && entity->inuse){
			// Move the pusher back and try again
			VectorCopy(start, entity->s.origin);
			gi.linkentity(entity);

			goto retry;
		}
	}

	if (entity->inuse)
		SG_TouchTriggers(entity);

	return trace;
}

/*
 ==================
 
 Objects need to be moved back on a failed push,
 otherwise riders would continue to slide.
 ==================
*/
static qboolean SV_Push (edict_t *pusher, vec3_t move, vec3_t amove){

	int			i, e;
	edict_t		*check, *block;
	vec3_t		mins, maxs;
	pushed_t	*p;
	vec3_t		org, org2, move2, forward, right, up;
	float		temp;

	// Clamp the move to 1/8 units, so the position will
	// be accurate for client side prediction
	for (i = 0; i < 3; i++){
		temp = move[i] * 8.0f;

		if (temp > 0.0f)
			temp += 0.5f;
		else
			temp -= 0.5f;

		move[i] = 0.125f * (int)temp;
	}

	// Find the bounding box
	mins[0] = pusher->absmin[0] + move[0];
	mins[1] = pusher->absmin[1] + move[1];
	mins[2] = pusher->absmin[2] + move[2];

	maxs[0] = pusher->absmax[0] + move[0];
	maxs[1] = pusher->absmax[1] + move[1];
	maxs[2] = pusher->absmax[2] + move[2];

	// We need this for pushing things later
	VectorSubtract(vec3_origin, amove, org);
	AngleVectors(org, forward, right, up);

	// Save the pusher's original position
	pushed_p->ent = pusher;

	VectorCopy(pusher->s.origin, pushed_p->origin);
	VectorCopy(pusher->s.angles, pushed_p->angles);

	if (pusher->client)
		pushed_p->deltayaw = pusher->client->ps.pmove.delta_angles[YAW];

	pushed_p++;

	// Move the pusher to it's final position
	VectorAdd(pusher->s.origin, move, pusher->s.origin);
	VectorAdd(pusher->s.angles, amove, pusher->s.angles);

	gi.linkentity(pusher);

	// See if any solid entities are inside the final position
	check = g_edicts + 1;

	for (e = 1; e < globals.num_edicts; e++, check++){
		if (!check->inuse)
			continue;

		if (check->movetype == MOVETYPE_PUSH || check->movetype == MOVETYPE_STOP || check->movetype == MOVETYPE_NONE || check->movetype == MOVETYPE_NOCLIP)
			continue;

		if (!check->area.prev)
			continue;		// Not linked in anywhere

		// If the entity is standing on the pusher, it will definitely be moved
		if (check->groundentity != pusher){
			// See if the entity needs to be tested
			if (check->absmin[0] >= maxs[0]
			|| check->absmin[1] >= maxs[1]
			|| check->absmin[2] >= maxs[2]
			|| check->absmax[0] <= mins[0]
			|| check->absmax[1] <= mins[1]
			|| check->absmax[2] <= mins[2])
				continue;

			// See if the entity's bbox is inside the pusher's final position
			if (!SV_TestEntityPosition(check))
				continue;
		}

		if ((pusher->movetype == MOVETYPE_PUSH) || (check->groundentity == pusher)){
			// Move this entity
			pushed_p->ent = check;

			VectorCopy(check->s.origin, pushed_p->origin);
			VectorCopy(check->s.angles, pushed_p->angles);

			pushed_p++;

			// Try moving the contacted entity 
			VectorAdd(check->s.origin, move, check->s.origin);

			if (check->client)
				// FIXME: Doesn't rotate monsters?
				check->client->ps.pmove.delta_angles[YAW] += amove[YAW];

			// Figure movement due to the pusher's amove
			VectorSubtract(check->s.origin, pusher->s.origin, org);

			org2[0] = DotProduct(org, forward);
			org2[1] = -DotProduct(org, right);
			org2[2] = DotProduct(org, up);

			VectorSubtract(org2, org, move2);
			VectorAdd(check->s.origin, move2, check->s.origin);

			// May have pushed them off an edge
			if (check->groundentity != pusher)
				check->groundentity = NULL;

			block = SV_TestEntityPosition(check);

			if (!block){
				// Pushed ok
				gi.linkentity(check);

				// Impact?
				continue;
			}

			// If it is ok to leave in the old position, do it
			// this is only relevent for riding entities, not pushed

			// FIXME: This doesn't acount for rotation
			VectorSubtract(check->s.origin, move, check->s.origin);

			block = SV_TestEntityPosition(check);

			if (!block){
				pushed_p--;
				continue;
			}
		}

		// Save off the obstacle so we can call the block function
		obstacle = check;

		// Move back any entities we already moved
		// go backwards, so if the same entity was pushed
		// twice, it goes back to the original position
		for (p = pushed_p - 1; p >= pushed; p--)
		{
			VectorCopy(p->origin, p->ent->s.origin);
			VectorCopy(p->angles, p->ent->s.angles);

			if (p->ent->client)
				p->ent->client->ps.pmove.delta_angles[YAW] = p->deltayaw;

			gi.linkentity(p->ent);
		}

		return false;
	}

	// FIXME: Is there a better way to handle this?

	// See if anything we moved has touched a trigger
	for (p = pushed_p - 1 ; p >= pushed; p--)
		SG_TouchTriggers(p->ent);

	return true;
}

/*
 ==================
 
 Inline model objects don't interact with each other, but
 push all box objects
 ==================
*/
static void SV_Physics_Pusher (edict_t *entity){

	vec3_t	move, amove;
	edict_t	*part, *mv;

	// If not a team captain, so movement will be handled elsewhere
	if (entity->flags & FL_TEAMSLAVE)
		return;

	// Make sure all team slaves can move before commiting
	// any moves or calling any think functions
	// If the move is blocked, all moved objects will be backed out
	pushed_p = pushed;

	for (part = entity; part; part = part->teamchain){
		if (part->velocity[0] || part->velocity[1] || part->velocity[2] || part->avelocity[0] || part->avelocity[1] || part->avelocity[2]){
			// Object is moving
			VectorScale(part->velocity, FRAMETIME, move);
			VectorScale(part->avelocity, FRAMETIME, amove);

			if (!SV_Push(part, move, amove))
				break;		// Move was blocked
		}
	}

	if (pushed_p > &pushed[MAX_EDICTS])
		gi.error(ERR_FATAL, "pushed_p > &pushed[MAX_EDICTS], memory corrupted");

	if (part){
		// The move failed, bump all nextthink times and back out moves
		for (mv = entity; mv; mv = mv->teamchain){
			if (mv->nextthink > 0.0f)
				mv->nextthink += FRAMETIME;
		}

		// if the pusher has a "blocked" function, call it
		// otherwise, just stay in place until the obstacle is gone
		if (part->blocked)
			part->blocked(part, obstacle);
	}
	else {
		// the move succeeded, so call all think functions
		for (part = entity; part; part = part->teamchain)
			SV_RunThink(part);
	}
}

//==================================================================

/*
 ==================
 
 Non moving objects can only think
 ==================
*/
static void SV_Physics_None (edict_t *entity){

	// Regular thinking
	SV_RunThink(entity);
}

/*
 ==================
 
 A moving object that doesn't obey physics
 ==================
*/
static void SV_Physics_Noclip (edict_t *entity){

	// Regular thinking
	if (!SV_RunThink(entity))
		return;

	VectorMA(entity->s.angles, FRAMETIME, entity->avelocity, entity->s.angles);
	VectorMA(entity->s.origin, FRAMETIME, entity->velocity, entity->s.origin);

	gi.linkentity(entity);
}


/*
 ==============================================================================

 TOSS/BOUNCE

 ==============================================================================
*/


/*
 ==================
 
 Toss, bounce, and fly movement. When onground, do nothing.
 ==================
*/
static void SV_Physics_Toss (edict_t *entity){

	trace_t		trace;
	vec3_t		move;
	float		backOff;
	edict_t		*slave;
	qboolean	wasInwater;
	qboolean	isInWater;
	vec3_t		oldOrigin;

	// Regular thinking
	SV_RunThink(entity);

	// If not a team captain, so movement will be handled elsewhere
	if (entity->flags & FL_TEAMSLAVE)
		return;

	if (entity->velocity[2] > 0.0f)
		entity->groundentity = NULL;

	// Check for the groundentity going away
	if (entity->groundentity){
		if (!entity->groundentity->inuse)
			entity->groundentity = NULL;
	}

	// If onground, return without moving
	if (entity->groundentity)
		return;

	VectorCopy(entity->s.origin, oldOrigin);

	// Check max velocity
	SG_CheckVelocity(entity);

	// Add gravity
	if (entity->movetype != MOVETYPE_FLY && entity->movetype != MOVETYPE_FLYMISSILE)
		SG_AddGravity(entity);

	// Move angles
	VectorMA(entity->s.angles, FRAMETIME, entity->avelocity, entity->s.angles);

	// Move origin
	VectorScale(entity->velocity, FRAMETIME, move);

	trace = SV_PushEntity(entity, move);

	if (!entity->inuse)
		return;

	if (trace.fraction < 1.0f){
		if (entity->movetype == MOVETYPE_BOUNCE)
			backOff = 1.5f;
		else
			backOff = 1.0f;

		ClipVelocity(entity->velocity, trace.plane.normal, entity->velocity, backOff);

		// Stop if on ground
		if (trace.plane.normal[2] > 0.7f){		
			if (entity->velocity[2] < 60.0f || entity->movetype != MOVETYPE_BOUNCE){
				entity->groundentity = trace.ent;
				entity->groundentity_linkcount = trace.ent->linkcount;
				VectorCopy(vec3_origin, entity->velocity);
				VectorCopy(vec3_origin, entity->avelocity);
			}
		}
	}

	// Check for water transition
	wasInwater = (entity->waterType & MASK_WATER);
	entity->waterType = gi.pointcontents(entity->s.origin);
	isInWater = entity->waterType & MASK_WATER;

	if (isInWater)
		entity->waterLevel = WATERLEVEL_FEET;
	else
		entity->waterLevel = WATERLEVEL_NONE;

	if (!wasInwater && isInWater)
		gi.positioned_sound(oldOrigin, g_edicts, CHAN_AUTO, gi.soundindex("misc/h2ohit1.wav"), 1.0f, 1.0f, 0.0f);
	else if (wasInwater && !isInWater)
		gi.positioned_sound(entity->s.origin, g_edicts, CHAN_AUTO, gi.soundindex("misc/h2ohit1.wav"), 1.0f, 1.0f, 0.0f);

	// Move team slaves
	for (slave = entity->teamchain; slave; slave = slave->teamchain){
		VectorCopy(entity->s.origin, slave->s.origin);
		gi.linkentity(slave);
	}
}


/*
 ==============================================================================

 STEPPING MOVEMENT

 ==============================================================================
*/

//FIXME: hacked in for E3 demo
#define	sv_stopspeed		100
#define sv_friction			6
#define sv_waterfriction	1


/*
 ==================
 
 ==================
*/
static void SV_AddRotationalFriction (edict_t *entity){

	float	adjustment;
	int		i;

	VectorMA(entity->s.angles, FRAMETIME, entity->avelocity, entity->s.angles);
	adjustment = FRAMETIME * sv_stopspeed * sv_friction;

	for (i = 0; i < 3; i++){
		if (entity->avelocity[i] > 0.0f){
			entity->avelocity[i] -= adjustment;

			if (entity->avelocity[i] < 0.0f)
				entity->avelocity[i] = 0.0f;
		}
		else {
			entity->avelocity[i] += adjustment;

			if (entity->avelocity[i] > 0.0f)
				entity->avelocity[i] = 0.0f;
		}
	}
}

/*
 ==================
 
 Monsters freefall when they don't have a ground entity, otherwise
 all movement is done with discrete steps.

 This is also used for objects that have become still on the ground, but
 will fall if the floor is pulled out from under them.

 FIXME: Is this true?
 ==================
*/
static void SV_Physics_Step (edict_t *ent){

	qboolean	wasonground;
	qboolean	hitsound = false;
	float		*vel;
	float		speed, newspeed, control;
	float		friction;
	edict_t		*groundentity;
	int			mask;

	// Airborn monsters should always check for ground
	if (!ent->groundentity)
		SG_CheckGround(ent);

	groundentity = ent->groundentity;

	// Check max velocity
	SG_CheckVelocity(ent);

	if (groundentity)
		wasonground = true;
	else
		wasonground = false;

	if (ent->avelocity[0] || ent->avelocity[1] || ent->avelocity[2])
		SV_AddRotationalFriction(ent);

	// Add gravity except for flying monsters and swimming monsters who are in the water
	if (! wasonground){
		if (!(ent->flags & FL_FLY)){
			if (!((ent->flags & FL_SWIM) && (ent->waterLevel > WATERLEVEL_WAIST))){
				if (ent->velocity[2] < sv_gravity->value * -0.1f)
					hitsound = true;
				if (ent->waterLevel == WATERLEVEL_NONE)
					SG_AddGravity(ent);
			}
		}
	}

	// Friction for flying monsters that have been given vertical velocity
	if ((ent->flags & FL_FLY) && (ent->velocity[2] != 0.0f)){
		speed = fabs(ent->velocity[2]);
		control = speed < sv_stopspeed ? sv_stopspeed : speed;
		friction = sv_friction / 3.0f;
		newspeed = speed - (FRAMETIME * control * friction);

		if (newspeed < 0.0f)
			newspeed = 0.0f;

		newspeed /= speed;
		ent->velocity[2] *= newspeed;
	}

	// Friction for flying monsters that have been given vertical velocity
	if ((ent->flags & FL_SWIM) && (ent->velocity[2] != 0.0f)){
		speed = fabs(ent->velocity[2]);
		control = speed < sv_stopspeed ? sv_stopspeed : speed;
		newspeed = speed - (FRAMETIME * control * sv_waterfriction * ent->waterLevel);

		if (newspeed < 0.0f)
			newspeed = 0.0f;

		newspeed /= speed;
		ent->velocity[2] *= newspeed;
	}

	if (ent->velocity[2] || ent->velocity[1] || ent->velocity[0]){
		// Apply friction
		// Let dead monsters who aren't completely onground slide
		if ((wasonground) || (ent->flags & (FL_SWIM|FL_FLY))){
			if (!(ent->health <= 0.0f && !SG_MonsterCheckBottom(ent))){
				vel = ent->velocity;
				speed = sqrt(vel[0] * vel[0] + vel[1] * vel[1]);

				if (speed){
					friction = sv_friction;

					control = speed < sv_stopspeed ? sv_stopspeed : speed;
					newspeed = speed - FRAMETIME*control*friction;

					if (newspeed < 0.0f)
						newspeed = 0.0f;

					newspeed /= speed;

					vel[0] *= newspeed;
					vel[1] *= newspeed;
				}
			}
		}

		if (ent->svflags & SVF_MONSTER)
			mask = MASK_MONSTERSOLID;
		else
			mask = MASK_SOLID;

		// Flying movement behaviour
		SG_FlyMovement(ent, FRAMETIME, mask);

		gi.linkentity(ent);

		SG_TouchTriggers(ent);

		if (!ent->inuse)
			return;

		if (ent->groundentity){
			if (!wasonground){
				if (hitsound)
					gi.sound(ent, 0, gi.soundindex("world/land.wav"), 1.0f, 1.0f, 0.0f);
			}
		}
	}

	// Regular thinking
	SV_RunThink(ent);
}


//============================================================================


void G_RunEntity (edict_t *ent){

	if (ent->prethink)
		ent->prethink(ent);

	switch ((int)ent->movetype){
	case MOVETYPE_PUSH:
	case MOVETYPE_STOP:
		SV_Physics_Pusher(ent);
		break;
	case MOVETYPE_NONE:
		SV_Physics_None(ent);
		break;
	case MOVETYPE_NOCLIP:
		SV_Physics_Noclip(ent);
		break;
	case MOVETYPE_STEP:
		SV_Physics_Step(ent);
		break;
	case MOVETYPE_TOSS:
	case MOVETYPE_BOUNCE:
	case MOVETYPE_FLY:
	case MOVETYPE_FLYMISSILE:
		SV_Physics_Toss(ent);
		break;
	default:
		gi.error("SV_Physics: bad movetype %i", (int)ent->movetype);			
	}
}
