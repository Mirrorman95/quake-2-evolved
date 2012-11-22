/*
Copyright (C) 1997-2001 Id Software, Inc.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  

See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

*/


#include "common.h"


// All of the locals will be zeroed before each pmove, just to make damn
// sure we don't have any differences when running on client or server
typedef struct {
	vec3_t		origin;
	vec3_t		velocity;
	vec3_t		forward, right, up;
	float		frameTime;
	csurface_t	*groundSurface;
	cplane_t	groundPlane;
	int			groundContents;
	vec3_t		previousOrigin;
	bool		ladder;
} pml_t;

static pml_t	pml;
static pmove_t	*pm;

// Movement parameters
float	pm_stopSpeed = 100;
float	pm_maxSpeed = 300;
float	pm_duckSpeed = 100;
float	pm_waterSpeed = 400;
float	pm_accelerate = 10;
float	pm_airAccelerate = 0;
float	pm_waterAccelerate = 10;
float	pm_friction = 6;
float	pm_waterFriction = 1;


/*
 =================
 PM_ClipVelocity

 Slide off of the impacting object
 =================
*/
static void PM_ClipVelocity (const vec3_t in, const vec3_t normal, vec3_t out, float overBounce){

	float	backOff;
	int		i;
	
	backOff = DotProduct(in, normal) * overBounce;

	for (i = 0; i < 3; i++){
		out[i] = in[i] - (normal[i] * backOff);
		if (out[i] > -0.1 && out[i] < 0.1)
			out[i] = 0;
	}
}

/*
 =================
 PM_SlideMove

 Each intersection will try to step over the obstruction instead of
 sliding along it.

 Returns a new origin, velocity, and contact entity.
 Does not modify any world state?
 =================
*/
static void PM_SlideMove (void){

#define	MAX_CLIP_PLANES	5

	int		bumpCount;
	vec3_t	primalVelocity, dir, end;
	float	d, timeLeft;
	int		numPlanes = 0;
	vec3_t	planes[MAX_CLIP_PLANES];
	int		i, j;
	trace_t	trace;
	
	VectorCopy(pml.velocity, primalVelocity);
	
	timeLeft = pml.frameTime;

	for (bumpCount = 0; bumpCount < 4; bumpCount++){
		VectorMA(pml.origin, timeLeft, pml.velocity, end);

		trace = pm->trace(pml.origin, pm->mins, pm->maxs, end);

		if (trace.allsolid){
			// Entity is trapped in another solid
			pml.velocity[2] = 0;	// Don't build up falling damage
			return;
		}

		if (trace.fraction > 0){
			// Actually covered some distance
			VectorCopy(trace.endpos, pml.origin);
			numPlanes = 0;
		}

		if (trace.fraction == 1)
			 break;		// Moved the entire distance

		// Save entity for contact
		if (pm->numtouch < MAXTOUCH && trace.ent){
			pm->touchents[pm->numtouch] = trace.ent;
			pm->numtouch++;
		}
		
		timeLeft -= timeLeft * trace.fraction;

		// Slide along this plane
		if (numPlanes >= MAX_CLIP_PLANES){
			// This shouldn't really happen
			VectorClear(pml.velocity);
			break;
		}

		VectorCopy(trace.plane.normal, planes[numPlanes]);
		numPlanes++;

		// Modify original velocity so it parallels all of the clip 
		// planes
		for (i = 0; i < numPlanes; i++){
			PM_ClipVelocity(pml.velocity, planes[i], pml.velocity, 1.01);

			for (j = 0; j < numPlanes; j++){
				if (j != i){
					if (DotProduct(pml.velocity, planes[j]) < 0)
						break;	// Not ok
				}
			}
			if (j == numPlanes)
				break;
		}

		if (i == numPlanes){
			// Go along the crease
			if (numPlanes != 2){
				VectorClear(pml.velocity);
				break;
			}

			CrossProduct(planes[0], planes[1], dir);
			d = DotProduct(dir, pml.velocity);
			VectorScale(dir, d, pml.velocity);
		}

		// If velocity is against the original velocity, stop dead to
		// avoid tiny occilations in sloping corners
		if (DotProduct(pml.velocity, primalVelocity) <= 0){
			VectorClear(pml.velocity);
			break;
		}
	}

	if (pm->s.pm_time)
		VectorCopy(primalVelocity, pml.velocity);
}

/*
 =================
 PM_StepSlideMove
 =================
*/
static void PM_StepSlideMove (void){

	vec3_t	startOrg, startVel;
	vec3_t	downOrg, downVel;
	vec3_t	up, down;
	float	upDist, downDist;
	trace_t	trace;

	VectorCopy(pml.origin, startOrg);
	VectorCopy(pml.velocity, startVel);

	PM_SlideMove();

	VectorCopy(pml.origin, downOrg);
	VectorCopy(pml.velocity, downVel);

	VectorCopy(startOrg, up);
	up[2] += 18;

	trace = pm->trace(up, pm->mins, pm->maxs, up);
	if (trace.allsolid)
		return;		// Can't step up

	// Try sliding above
	VectorCopy(up, pml.origin);
	VectorCopy(startVel, pml.velocity);

	PM_SlideMove();

	// Push down the final amount
	VectorCopy(pml.origin, down);
	down[2] -= 18;

	trace = pm->trace(pml.origin, pm->mins, pm->maxs, down);
	if (!trace.allsolid)
		VectorCopy(trace.endpos, pml.origin);

	VectorCopy(pml.origin, up);

	// Decide which one went farther
    downDist = (downOrg[0] - startOrg[0])*(downOrg[0] - startOrg[0]) + (downOrg[1] - startOrg[1])*(downOrg[1] - startOrg[1]);
    upDist = (up[0] - startOrg[0])*(up[0] - startOrg[0]) + (up[1] - startOrg[1])*(up[1] - startOrg[1]);

	if (downDist > upDist || trace.plane.normal[2] < 0.7){
		VectorCopy(downOrg, pml.origin);
		VectorCopy(downVel, pml.velocity);
		return;
	}
	
	// Special case
	// If we were walking along a plane, then we need to copy the Z over
	pml.velocity[2] = downVel[2];
}

/*
 =================
 PM_Friction

 Handles both ground friction and water friction
 =================
*/
static void PM_Friction (void){

	float	speed, newSpeed;
	float	control, friction, drop;
	
	speed = VectorLength(pml.velocity);
	if (speed < 1){
		pml.velocity[0] = 0;
		pml.velocity[1] = 0;
		return;
	}

	drop = 0;

	// Apply ground friction
	if ((pm->groundentity && pml.groundSurface && !(pml.groundSurface->flags & SURF_SLICK)) || (pml.ladder)){
		friction = pm_friction;
		control = (speed < pm_stopSpeed) ? pm_stopSpeed : speed;
		drop += control * friction * pml.frameTime;
	}

	// Apply water friction
	if (pm->waterlevel && !pml.ladder)
		drop += speed * pm_waterFriction * pm->waterlevel * pml.frameTime;

	// Scale the velocity
	newSpeed = speed - drop;
	if (newSpeed < 0)
		newSpeed = 0;

	VectorScale(pml.velocity, newSpeed / speed, pml.velocity);
}

/*
 =================
 PM_Accelerate

 Handles user intended acceleration
 =================
*/
static void PM_Accelerate (const vec3_t wishDir, float wishSpeed, float accel){

	float	addSpeed, accelSpeed;

	addSpeed = wishSpeed - DotProduct(pml.velocity, wishDir);
	if (addSpeed <= 0)
		return;

	accelSpeed = accel * wishSpeed * pml.frameTime;
	if (accelSpeed > addSpeed)
		accelSpeed = addSpeed;

	VectorMA(pml.velocity, accelSpeed, wishDir, pml.velocity);
}

/*
 =================
 PM_AirAccelerate
 =================
*/
static void PM_AirAccelerate (const vec3_t wishDir, float wishSpeed, float accel){

	float	addSpeed, accelSpeed;

	if (wishSpeed > 30)
		addSpeed = 30 - DotProduct(pml.velocity, wishDir);
	else
		addSpeed = wishSpeed - DotProduct(pml.velocity, wishDir);
		
	if (addSpeed <= 0)
		return;
	
	accelSpeed = accel * wishSpeed * pml.frameTime;
	if (accelSpeed > addSpeed)
		accelSpeed = addSpeed;

	VectorMA(pml.velocity, accelSpeed, wishDir, pml.velocity);
}

/*
 =================
 PM_AddCurrents
 =================
*/
static void PM_AddCurrents (vec3_t wishVel){

	vec3_t	v;
	float	s;

	// Account for ladders
	if (pml.ladder && fabs(pml.velocity[2]) <= 200){
		if ((pm->viewangles[PITCH] <= -15) && (pm->cmd.forwardmove > 0))
			wishVel[2] = 200;
		else if ((pm->viewangles[PITCH] >= 15) && (pm->cmd.forwardmove > 0))
			wishVel[2] = -200;
		else if (pm->cmd.upmove > 0)
			wishVel[2] = 200;
		else if (pm->cmd.upmove < 0)
			wishVel[2] = -200;
		else
			wishVel[2] = 0;

		// Limit horizontal speed when on a ladder
		if (wishVel[0] < -25)
			wishVel[0] = -25;
		else if (wishVel[0] > 25)
			wishVel[0] = 25;

		if (wishVel[1] < -25)
			wishVel[1] = -25;
		else if (wishVel[1] > 25)
			wishVel[1] = 25;
	}

	// Add water currents
	if (pm->watertype & MASK_CURRENT){
		VectorClear(v);

		if (pm->watertype & CONTENTS_CURRENT_0)
			v[0] += 1;
		if (pm->watertype & CONTENTS_CURRENT_90)
			v[1] += 1;
		if (pm->watertype & CONTENTS_CURRENT_180)
			v[0] -= 1;
		if (pm->watertype & CONTENTS_CURRENT_270)
			v[1] -= 1;
		if (pm->watertype & CONTENTS_CURRENT_UP)
			v[2] += 1;
		if (pm->watertype & CONTENTS_CURRENT_DOWN)
			v[2] -= 1;

		s = pm_waterSpeed;
		if ((pm->waterlevel == 1) && (pm->groundentity))
			s *= 0.5;

		VectorMA(wishVel, s, v, wishVel);
	}

	// Add conveyor belt velocities
	if (pm->groundentity){
		VectorClear(v);

		if (pml.groundContents & CONTENTS_CURRENT_0)
			v[0] += 1;
		if (pml.groundContents & CONTENTS_CURRENT_90)
			v[1] += 1;
		if (pml.groundContents & CONTENTS_CURRENT_180)
			v[0] -= 1;
		if (pml.groundContents & CONTENTS_CURRENT_270)
			v[1] -= 1;
		if (pml.groundContents & CONTENTS_CURRENT_UP)
			v[2] += 1;
		if (pml.groundContents & CONTENTS_CURRENT_DOWN)
			v[2] -= 1;

		VectorMA(wishVel, 100, v, wishVel);
	}
}

/*
 =================
 PM_WaterMove
 =================
*/
static void PM_WaterMove (void){

	vec3_t	wishVel, wishDir;
	float	wishSpeed;

	// User intentions
	wishVel[0] = (pml.forward[0] * pm->cmd.forwardmove) + (pml.right[0] * pm->cmd.sidemove);
	wishVel[1] = (pml.forward[1] * pm->cmd.forwardmove) + (pml.right[1] * pm->cmd.sidemove);
	wishVel[2] = (pml.forward[2] * pm->cmd.forwardmove) + (pml.right[2] * pm->cmd.sidemove);

	if (!pm->cmd.forwardmove && !pm->cmd.sidemove && !pm->cmd.upmove)
		wishVel[2] -= 60;		// Drift towards bottom
	else
		wishVel[2] += pm->cmd.upmove;

	PM_AddCurrents(wishVel);

	VectorCopy(wishVel, wishDir);
	wishSpeed = VectorNormalize(wishDir);

	if (wishSpeed > pm_maxSpeed){
		VectorScale(wishVel, pm_maxSpeed / wishSpeed, wishVel);
		wishSpeed = pm_maxSpeed;
	}
	wishSpeed *= 0.5;

	PM_Accelerate(wishDir, wishSpeed, pm_waterAccelerate);

	PM_StepSlideMove();
}

/*
 =================
 PM_AirMove
 =================
*/
static void PM_AirMove (void){

	vec3_t		wishVel, wishDir;
	float		wishSpeed, maxSpeed;

	wishVel[0] = (pml.forward[0] * pm->cmd.forwardmove) + (pml.right[0] * pm->cmd.sidemove);
	wishVel[1] = (pml.forward[1] * pm->cmd.forwardmove) + (pml.right[1] * pm->cmd.sidemove);
	wishVel[2] = 0;

	PM_AddCurrents(wishVel);

	maxSpeed = (pm->s.pm_flags & PMF_DUCKED) ? pm_duckSpeed : pm_maxSpeed;

	VectorCopy(wishVel, wishDir);
	wishSpeed = VectorNormalize(wishDir);

	if (wishSpeed > maxSpeed){
		VectorScale(wishVel, maxSpeed / wishSpeed, wishVel);
		wishSpeed = maxSpeed;
	}
	
	if (pml.ladder){
		PM_Accelerate(wishDir, wishSpeed, pm_accelerate);

		if (!wishVel[2]){
			if (pml.velocity[2] > 0){
				pml.velocity[2] -= pm->s.gravity * pml.frameTime;
				if (pml.velocity[2] < 0)
					pml.velocity[2] = 0;
			}
			else {
				pml.velocity[2] += pm->s.gravity * pml.frameTime;
				if (pml.velocity[2] > 0)
					pml.velocity[2] = 0;
			}
		}
	}
	else if (pm->groundentity){
		// Walking on ground
		pml.velocity[2] = 0;	// This is before the accel
		
		PM_Accelerate(wishDir, wishSpeed, pm_accelerate);

		// Fix for negative trigger_gravity fields
		if (pm->s.gravity > 0)
			pml.velocity[2] = 0;
		else
			pml.velocity[2] -= pm->s.gravity * pml.frameTime;

		if (!pml.velocity[0] && !pml.velocity[1])
			return;
	}
	else {
		// Not on ground, so little effect on velocity
		if (pm_airAccelerate)
			PM_AirAccelerate(wishDir, wishSpeed, pm_accelerate);
		else
			PM_Accelerate(wishDir, wishSpeed, 1);
		
		// Add gravity
		pml.velocity[2] -= pm->s.gravity * pml.frameTime;
	}

	PM_StepSlideMove();
}

/*
 =================
 PM_CategorizePosition
 =================
*/
static void PM_CategorizePosition (void){

	vec3_t		point;
	int			contents;
	trace_t		trace;
	int			sample1, sample2;

	// If the player hull point one-quarter unit down is solid, the
	// player is on ground

	// See if standing on something solid	
	point[0] = pml.origin[0];
	point[1] = pml.origin[1];
	point[2] = pml.origin[2] - 0.25;

	if (pml.velocity[2] > 180){		// Changed from 100 to 180 (ramp accel)
		pm->s.pm_flags &= ~PMF_ON_GROUND;
		pm->groundentity = NULL;
	}
	else {
		trace = pm->trace(pml.origin, pm->mins, pm->maxs, point);
		
		pml.groundPlane = trace.plane;
		pml.groundSurface = trace.surface;
		pml.groundContents = trace.contents;

		if (!trace.ent || (trace.plane.normal[2] < 0.7 && !trace.startsolid)){
			pm->groundentity = NULL;
			pm->s.pm_flags &= ~PMF_ON_GROUND;
		}
		else {
			pm->groundentity = trace.ent;

			// Hitting solid ground will end a waterjump
			if (pm->s.pm_flags & PMF_TIME_WATERJUMP){
				pm->s.pm_flags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND | PMF_TIME_TELEPORT);
				pm->s.pm_time = 0;
			}

			if (!(pm->s.pm_flags & PMF_ON_GROUND)){
				// Just hit the ground
				pm->s.pm_flags |= PMF_ON_GROUND;

				// Don't do landing time if we were just going down a 
				// slope
				if (pml.velocity[2] < -200){
					pm->s.pm_flags |= PMF_TIME_LAND;
					// Don't allow another jump for a little while
					if (pml.velocity[2] < -400)
						pm->s.pm_time = 25;	
					else
						pm->s.pm_time = 18;
				}
			}
		}

		if (pm->numtouch < MAXTOUCH && trace.ent){
			pm->touchents[pm->numtouch] = trace.ent;
			pm->numtouch++;
		}
	}

	// Get waterlevel, accounting for ducking
	pm->waterlevel = 0;
	pm->watertype = 0;

	sample2 = pm->viewheight - pm->mins[2];
	sample1 = sample2 / 2;

	point[2] = pml.origin[2] + pm->mins[2] + 1;	
	contents = pm->pointcontents(point);
	
	if (contents & MASK_WATER){
		pm->watertype = contents;
		pm->waterlevel = 1;

		point[2] = pml.origin[2] + pm->mins[2] + sample1;
		contents = pm->pointcontents(point);

		if (contents & MASK_WATER){
			pm->waterlevel = 2;
		
			point[2] = pml.origin[2] + pm->mins[2] + sample2;
			contents = pm->pointcontents(point);
			
			if (contents & MASK_WATER)
				pm->waterlevel = 3;
		}
	}
}

/*
 =================
 PM_CheckJump
 =================
*/
static void PM_CheckJump (void){

	if (pm->s.pm_flags & PMF_TIME_LAND)
		// Hasn't been long enough since landing to jump again
		return;

	if (pm->cmd.upmove < 10){
		// Not holding jump
		pm->s.pm_flags &= ~PMF_JUMP_HELD;
		return;
	}

	// Must wait for jump to be released
	if (pm->s.pm_flags & PMF_JUMP_HELD)
		return;

	if (pm->s.pm_type == PM_DEAD)
		return;

	if (pm->waterlevel >= 2){
		// Swimming, not jumping
		pm->groundentity = NULL;

		if (pml.velocity[2] <= -300)
			return;

		if (pm->watertype == CONTENTS_WATER)
			pml.velocity[2] = 100;
		else if (pm->watertype == CONTENTS_SLIME)
			pml.velocity[2] = 80;
		else
			pml.velocity[2] = 50;

		return;
	}

	if (pm->groundentity == NULL)
		return;		// In air, so no effect

	pm->s.pm_flags |= PMF_JUMP_HELD;

	pm->groundentity = NULL;
	pml.velocity[2] += 270;
	if (pml.velocity[2] < 270)
		pml.velocity[2] = 270;
}

/*
 =================
 PM_CheckSpecialMovement
 =================
*/
static void PM_CheckSpecialMovement (void){

	vec3_t	point, flatForward;
	int		contents;
	trace_t	trace;

	if (pm->s.pm_time)
		return;

	flatForward[0] = pml.forward[0];
	flatForward[1] = pml.forward[1];
	flatForward[2] = 0;

	VectorNormalize(flatForward);

	// Check for ladder
	pml.ladder = false;
	VectorAdd(pml.origin, flatForward, point);

	trace = pm->trace(pml.origin, pm->mins, pm->maxs, point);
	if ((trace.fraction < 1) && (trace.contents & CONTENTS_LADDER))
		pml.ladder = true;

	// Check for water jump
	if (pm->waterlevel != 2)
		return;

	VectorMA(pml.origin, 30, flatForward, point);
	point[2] += 4;
	
	contents = pm->pointcontents(point);
	if (!(contents & CONTENTS_SOLID))
		return;

	point[2] += 16;
	
	contents = pm->pointcontents(point);
	if (contents)
		return;
	
	// Jump out of water
	VectorScale(flatForward, 50, pml.velocity);
	pml.velocity[2] = 350;

	pm->s.pm_flags |= PMF_TIME_WATERJUMP;
	pm->s.pm_time = 255;
}

/*
 =================
 PM_FlyMove
 =================
*/
static void PM_FlyMove (bool doClip){

	float		speed, wishSpeed, newSpeed, addSpeed, accelSpeed;
	float		drop, friction, control;
	vec3_t		wishVel, wishDir;
	vec3_t		end;
	trace_t		trace;

	pm->viewheight = 22;

	// Friction
	speed = VectorLength(pml.velocity);
	if (speed < 1)
		VectorClear(pml.velocity);
	else {
		drop = 0;

		friction = pm_friction * 1.5;	// Extra friction
		control = (speed < pm_stopSpeed) ? pm_stopSpeed : speed;
		drop += control * friction * pml.frameTime;

		// Scale the velocity
		newSpeed = speed - drop;
		if (newSpeed < 0)
			newSpeed = 0;

		VectorScale(pml.velocity, newSpeed / speed, pml.velocity);
	}

	// Accelerate
	VectorNormalize(pml.forward);
	VectorNormalize(pml.right);

	wishVel[0] = (pml.forward[0] * pm->cmd.forwardmove) + (pml.right[0] * pm->cmd.sidemove);
	wishVel[1] = (pml.forward[1] * pm->cmd.forwardmove) + (pml.right[1] * pm->cmd.sidemove);
	wishVel[2] = (pml.forward[2] * pm->cmd.forwardmove) + (pml.right[2] * pm->cmd.sidemove) + pm->cmd.upmove;

	VectorCopy(wishVel, wishDir);
	wishSpeed = VectorNormalize(wishDir);

	if (wishSpeed > pm_maxSpeed){
		VectorScale(wishVel, pm_maxSpeed / wishSpeed, wishVel);
		wishSpeed = pm_maxSpeed;
	}

	addSpeed = wishSpeed - DotProduct(pml.velocity, wishDir);
	if (addSpeed <= 0)
		return;

	accelSpeed = pm_accelerate * pml.frameTime * wishSpeed;
	if (accelSpeed > addSpeed)
		accelSpeed = addSpeed;

	VectorMA(pml.velocity, accelSpeed, wishDir, pml.velocity);

	if (doClip){
		VectorMA(pml.origin, pml.frameTime, pml.velocity, end);

		trace = pm->trace(pml.origin, pm->mins, pm->maxs, end);

		VectorCopy(trace.endpos, pml.origin);
	} 
	else 
		VectorMA(pml.origin, pml.frameTime, pml.velocity, pml.origin);
}

/*
 =================
 PM_CheckDuck

 Sets mins, maxs, and pm->viewheight
 =================
*/
static void PM_CheckDuck (void){

	trace_t	trace;

	pm->mins[0] = -16;
	pm->mins[1] = -16;

	pm->maxs[0] = 16;
	pm->maxs[1] = 16;

	if (pm->s.pm_type == PM_GIB){
		pm->mins[2] = 0;
		pm->maxs[2] = 16;
		pm->viewheight = 8;
		return;
	}

	pm->mins[2] = -24;

	if (pm->s.pm_type == PM_DEAD)
		pm->s.pm_flags |= PMF_DUCKED;
	else if (pm->cmd.upmove < 0 && (pm->s.pm_flags & PMF_ON_GROUND))
		// Duck
		pm->s.pm_flags |= PMF_DUCKED;
	else {
		// Stand up if possible
		if (pm->s.pm_flags & PMF_DUCKED){
			// Try to stand up
			pm->maxs[2] = 32;
			trace = pm->trace(pml.origin, pm->mins, pm->maxs, pml.origin);
			if (!trace.allsolid)
				pm->s.pm_flags &= ~PMF_DUCKED;
		}
	}

	if (pm->s.pm_flags & PMF_DUCKED){
		pm->maxs[2] = 4;
		pm->viewheight = -2;
	}
	else {
		pm->maxs[2] = 32;
		pm->viewheight = 22;
	}
}

/*
 =================
 PM_DeadMove
 =================
*/
static void PM_DeadMove (void){

	float	forward;

	if (!pm->groundentity)
		return;

	// Extra friction
	forward = VectorLength(pml.velocity);
	forward -= 20;

	if (forward <= 0)
		VectorClear(pml.velocity);
	else {
		VectorNormalize(pml.velocity);
		VectorScale(pml.velocity, forward, pml.velocity);
	}
}

/*
 =================
 PM_GoodPosition
 =================
*/
static bool PM_GoodPosition (void){

	trace_t	trace;
	vec3_t	start, end;

	if (pm->s.pm_type == PM_SPECTATOR)
		return true;

	start[0] = end[0] = pm->s.origin[0] * 0.125;
	start[1] = end[1] = pm->s.origin[1] * 0.125;
	start[2] = end[2] = pm->s.origin[2] * 0.125;

	trace = pm->trace(start, pm->mins, pm->maxs, end);

	return !trace.allsolid;
}

/*
 =================
 PM_SnapPosition

 On exit, the origin will have a value that is pre-quantized to the 
 0.125 precision of the network channel and in a valid position
 =================
*/
static void PM_SnapPosition (void){

	static int	jitterBits[8] = {0, 4, 1, 2, 3, 5, 6, 7};
	int			sign[3], bits;
	short		base[3];
	int			i, j;

	// Snap velocity to eights
	pm->s.velocity[0] = (int)(pml.velocity[0] * 8);
	pm->s.velocity[1] = (int)(pml.velocity[1] * 8);
	pm->s.velocity[2] = (int)(pml.velocity[2] * 8);

	for (i = 0; i < 3; i++){
		if (pml.origin[i] >= 0)
			sign[i] = 1;
		else 
			sign[i] = -1;

		pm->s.origin[i] = (int)(pml.origin[i] * 8);
		if (pm->s.origin[i] * 0.125 == pml.origin[i])
			sign[i] = 0;
	}

	base[0] = pm->s.origin[0];
	base[1] = pm->s.origin[1];
	base[2] = pm->s.origin[2];

	// Try all combinations
	for (j = 0; j < 8; j++){
		bits = jitterBits[j];

		pm->s.origin[0] = base[0];
		pm->s.origin[1] = base[1];
		pm->s.origin[2] = base[2];

		for (i = 0; i < 3; i++){
			if (bits & (1<<i))
				pm->s.origin[i] += sign[i];
		}

		if (PM_GoodPosition())
			return;
	}

	// Go back to the last position
	pm->s.origin[0] = pml.previousOrigin[0];
	pm->s.origin[1] = pml.previousOrigin[1];
	pm->s.origin[2] = pml.previousOrigin[2];
}

/*
 =================
 PM_InitialSnapPosition
 =================
*/
static void PM_InitialSnapPosition (void){

	static int	offset[3] = {0, -1, 1};
	short		base[3];
	int			x, y, z;
	
	base[0] = pm->s.origin[0];
	base[1] = pm->s.origin[1];
	base[2] = pm->s.origin[2];

	for (z = 0; z < 3; z++){
		pm->s.origin[2] = base[2] + offset[z];

		for (y = 0; y < 3; y++){
			pm->s.origin[1] = base[1] + offset[y];

			for (x = 0; x < 3; x++){
				pm->s.origin[0] = base[0] + offset[x];

				if (PM_GoodPosition()){
					pml.origin[0] = pm->s.origin[0] * 0.125;
					pml.origin[1] = pm->s.origin[1] * 0.125;
					pml.origin[2] = pm->s.origin[2] * 0.125;

					pml.previousOrigin[0] = pm->s.origin[0];
					pml.previousOrigin[1] = pm->s.origin[1];
					pml.previousOrigin[2] = pm->s.origin[2];

					return;
				}
			}
		}
	}

	Com_DPrintf(S_COLOR_YELLOW "Bad PM_InitialSnapPosition\n");
}

/*
 =================
 PM_ClampAngles
 =================
*/
static void PM_ClampAngles (void){

	short	temp;
	int		i;

	if (pm->s.pm_flags & PMF_TIME_TELEPORT){
		pm->viewangles[PITCH] = 0;
		pm->viewangles[YAW] = SHORT2ANGLE(pm->cmd.angles[YAW] + pm->s.delta_angles[YAW]);
		pm->viewangles[ROLL] = 0;
	}
	else {
		// Circularly clamp the angles with deltas
		for (i = 0; i < 3; i++){
			temp = pm->cmd.angles[i] + pm->s.delta_angles[i];
			pm->viewangles[i] = SHORT2ANGLE(temp);
		}

		// Don't let the player look up or down more than 90 degrees
		if (pm->viewangles[PITCH] > 89 && pm->viewangles[PITCH] < 180)
			pm->viewangles[PITCH] = 89;
		else if (pm->viewangles[PITCH] < 271 && pm->viewangles[PITCH] >= 180)
			pm->viewangles[PITCH] = 271;
	}

	AngleToVectors(pm->viewangles, pml.forward, pml.right, pml.up);
}

/*
 =================
 PMove

 Can be called by either the server or the client
 =================
*/
void PMove (pmove_t *pmove){

	pm = pmove;

	// Clear results
	pm->numtouch = 0;
	VectorClear(pm->viewangles);
	pm->viewheight = 0;
	pm->groundentity = 0;
	pm->watertype = 0;
	pm->waterlevel = 0;

	// Clear all pmove local vars
	Mem_Fill(&pml, 0, sizeof(pml));

	// Convert origin and velocity to float values
	pml.origin[0] = pm->s.origin[0] * 0.125;
	pml.origin[1] = pm->s.origin[1] * 0.125;
	pml.origin[2] = pm->s.origin[2] * 0.125;

	pml.velocity[0] = pm->s.velocity[0] * 0.125;
	pml.velocity[1] = pm->s.velocity[1] * 0.125;
	pml.velocity[2] = pm->s.velocity[2] * 0.125;

	// Save old origin in case we get stuck
	pml.previousOrigin[0] = pm->s.origin[0];
	pml.previousOrigin[1] = pm->s.origin[1];
	pml.previousOrigin[2] = pm->s.origin[2];

	pml.frameTime = pm->cmd.msec * 0.001;

	PM_ClampAngles();

	if (pm->s.pm_type == PM_SPECTATOR){
		PM_FlyMove(false);
		PM_SnapPosition();
		return;
	}

	if (pm->s.pm_type >= PM_DEAD){
		pm->cmd.forwardmove = 0;
		pm->cmd.sidemove = 0;
		pm->cmd.upmove = 0;
	}

	if (pm->s.pm_type == PM_FREEZE)
		return;		// No movement at all

	// Set mins, maxs, and viewheight
	PM_CheckDuck();

	if (pm->snapinitial)
		PM_InitialSnapPosition();

	// Set groundentity, watertype, and waterlevel
	PM_CategorizePosition();

	if (pm->s.pm_type == PM_DEAD)
		PM_DeadMove();

	PM_CheckSpecialMovement();

	// Drop timing counter
	if (pm->s.pm_time){
		int		msec;

		msec = pm->cmd.msec >> 3;
		if (!msec)
			msec = 1;
		if (msec >= pm->s.pm_time){
			pm->s.pm_flags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND | PMF_TIME_TELEPORT);
			pm->s.pm_time = 0;
		}
		else
			pm->s.pm_time -= msec;
	}

	if (pm->s.pm_flags & PMF_TIME_TELEPORT)
		; // Teleport pause stays exactly in place
	else if (pm->s.pm_flags & PMF_TIME_WATERJUMP){
		// Waterjump has no control, but falls
		pml.velocity[2] -= pm->s.gravity * pml.frameTime;
		if (pml.velocity[2] < 0){
			// Cancel as soon as we are falling down again
			pm->s.pm_flags &= ~(PMF_TIME_WATERJUMP | PMF_TIME_LAND | PMF_TIME_TELEPORT);
			pm->s.pm_time = 0;
		}

		PM_StepSlideMove();
	}
	else {
		PM_CheckJump();

		PM_Friction();

		if (pm->waterlevel >= 2)
			PM_WaterMove();
		else {
			vec3_t	angles;

			VectorCopy(pm->viewangles, angles);
			if (angles[PITCH] > 180)
				angles[PITCH] = angles[PITCH] - 360;
			angles[PITCH] /= 3;

			AngleToVectors(angles, pml.forward, pml.right, pml.up);

			PM_AirMove();
		}
	}

	// Set groundentity, watertype, and waterlevel for final spot
	PM_CategorizePosition();

	PM_SnapPosition();
}
