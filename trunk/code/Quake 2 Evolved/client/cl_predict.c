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
// cl_predict.c - Local prediction of player movement
//


#include "client.h"


static entity_state_t *		cl_solidEntities[MAX_PARSE_ENTITIES];
static int					cl_numSolidEntities;


/*
 ==================
 CL_BuildSolidList
 ==================
*/
void CL_BuildSolidList (){

	entity_state_t	*entity;
	int				i;

	cl_numSolidEntities = 0;

	for (i = 0; i < cl.frame.numEntities; i++){
		entity = &cl.parseEntities[(cl.frame.parseEntitiesIndex + i) & (MAX_PARSE_ENTITIES - 1)];
		if (!entity->solid)
			continue;

		cl_solidEntities[cl_numSolidEntities++] = entity;
	}
}

/*
 ==================
 CL_Trace
 ==================
*/
trace_t CL_Trace (const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int skipNumber, int brushMask, bool brushOnly, int *entNumber){

	trace_t				trace, tmp;
	entity_state_t		*entity;
	clipInlineModel_t	*model;
	vec3_t				bmins, bmaxs;
	int					i, xy, zd, zu, headNode;

	// Check against the world
	trace = CM_BoxTrace(start, end, mins, maxs, 0, brushMask);
	if (trace.fraction < 1.0f){
		if (entNumber)
			*entNumber = 0;

		trace.ent = (struct edict_s *)1;
	}

	if (trace.allsolid || trace.fraction == 0.0f)
		return trace;

	// Check all other solid models
	for (i = 0; i < cl_numSolidEntities; i++){
		entity = cl_solidEntities[i];

		if (entity->number == skipNumber)
			continue;

		if (entity->solid == 31){
			// Special value for brush model
			model = cl.media.gameCModels[entity->modelindex];
			if (!model)
				continue;

			tmp = CM_TransformedBoxTrace(start, end, mins, maxs, model->headNode, brushMask, entity->origin, entity->angles);
		}
		else {
			if (brushOnly)
				continue;

			// Encoded a bounding box
			xy = 8 * (entity->solid & 31);
			zd = 8 * ((entity->solid >> 5) & 31);
			zu = 8 * ((entity->solid >> 10) & 63) - 32;

			bmins[0] = bmins[1] = -xy;
			bmaxs[0] = bmaxs[1] = xy;
			bmins[2] = -zd;
			bmaxs[2] = zu;

			headNode = CM_HeadNodeForBox(bmins, bmaxs);
			tmp = CM_TransformedBoxTrace(start, end, mins, maxs, headNode, brushMask, entity->origin, vec3_origin);
		}

		if (tmp.allsolid || tmp.startsolid || tmp.fraction < trace.fraction){
			if (entNumber)
				*entNumber = entity->number;

			tmp.ent = (struct edict_s *)entity;
			if (trace.startsolid){
				trace = tmp;
				trace.startsolid = true;
			}
			else
				trace = tmp;
		}
		else if (tmp.startsolid)
			trace.startsolid = true;

		if (trace.allsolid)
			break;
	}

	return trace;
}

/*
 ==================
 CL_PointContents
 ==================
*/
int	CL_PointContents (const vec3_t point, int skipNumber){

	entity_state_t		*entity;
	clipInlineModel_t	*model;
	int					i, contents;

	contents = CM_PointContents(point, 0);

	for (i = 0; i < cl_numSolidEntities; i++){
		entity = cl_solidEntities[i];

		if (entity->number == skipNumber)
			continue;

		if (entity->solid != 31)	// Special value for brush model
			continue;

		model = cl.media.gameCModels[entity->modelindex];
		if (!model)
			continue;

		contents |= CM_TransformedPointContents(point, model->headNode, entity->origin, entity->angles);
	}

	return contents;
}

/*
 ==================
 CL_PMTrace
 ==================
*/
static trace_t CL_PMTrace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end){

	return CL_Trace(start, mins, maxs, end, cl.clientNum, MASK_PLAYERSOLID, false, NULL);
}

/*
 ==================
 CL_PMPointContents
 ==================
*/
static int CL_PMPointContents (vec3_t point){

	return CL_PointContents(point, -1);
}

/*
 ==================
 CL_CheckPredictionError
 ==================
*/
void CL_CheckPredictionError (){

	int		frame;
	int		delta[3];

	if (!cl_predict->integerValue || (cl.frame.playerState.pmove.pm_flags & PMF_NO_PREDICTION))
		return;

	// Calculate the last usercmd_t we sent that the server has
	// processed
	frame = cls.netChan.incomingAcknowledged & CMD_MASK;

	// Compare what the server returned with what we had predicted it to
	// be
	delta[0] = cl.frame.playerState.pmove.origin[0] - cl.predictedOrigins[frame][0];
	delta[1] = cl.frame.playerState.pmove.origin[1] - cl.predictedOrigins[frame][1];
	delta[2] = cl.frame.playerState.pmove.origin[2] - cl.predictedOrigins[frame][2];

	// Save the prediction error for interpolation
	if (abs(delta[0]) + abs(delta[1]) + abs(delta[2]) > 640.0f)
		// A teleport or something
		VectorClear(cl.predictedError);
	else {
		if (cl_showMiss->integerValue && (delta[0] || delta[1] || delta[2]))
			Com_Printf("Prediction miss on %i: %i\n", cl.frame.serverFrame, delta[0] + delta[1] + delta[2]);

		cl.predictedOrigins[frame][0] = cl.frame.playerState.pmove.origin[0];
		cl.predictedOrigins[frame][1] = cl.frame.playerState.pmove.origin[1];
		cl.predictedOrigins[frame][2] = cl.frame.playerState.pmove.origin[2];

		// Save for error interpolation
		cl.predictedError[0] = delta[0] * 0.125f;
		cl.predictedError[1] = delta[1] * 0.125f;
		cl.predictedError[2] = delta[2] * 0.125f;
	}
}

/*
 ==================
 CL_PredictMovement

 Sets cl.predictedOrigin and cl.predictedAngles
 ==================
*/
void CL_PredictMovement (){

	int		ack, current;
	int		frame, step;
	pmove_t	pm;

	if ((cls.state != CA_ACTIVE || cls.loading) || cls.playingCinematic || com_paused->integerValue)
		return;

	if (!cl_predict->integerValue || (cl.frame.playerState.pmove.pm_flags & PMF_NO_PREDICTION)){
		// Just set angles
		cl.predictedAngles[0] = cl.viewAngles[0] + SHORT2ANGLE(cl.frame.playerState.pmove.delta_angles[0]);
		cl.predictedAngles[1] = cl.viewAngles[1] + SHORT2ANGLE(cl.frame.playerState.pmove.delta_angles[1]);
		cl.predictedAngles[2] = cl.viewAngles[2] + SHORT2ANGLE(cl.frame.playerState.pmove.delta_angles[2]);
		
		return;
	}

	ack = cls.netChan.incomingAcknowledged;
	current = cls.netChan.outgoingSequence;

	// If we are too far out of date, just freeze
	if (current - ack >= CMD_BACKUP){
		if (cl_showMiss->integerValue)
			Com_Printf("CL_PredictMovement: exceeded CMD_BACKUP\n");

		return;	
	}

	// Copy current state to pmove
	Mem_Fill(&pm, 0, sizeof(pm));

	pm.trace = CL_PMTrace;
	pm.pointcontents = CL_PMPointContents;
	pm_airAccelerate = atof(cl.configStrings[CS_AIRACCEL]);
	pm.s = cl.frame.playerState.pmove;

	// Run frames
	while (++ack < current){
		frame = ack & CMD_MASK;
		pm.cmd = cl.cmds[frame];

		PMove(&pm);

		// Save for debug checking
		cl.predictedOrigins[frame][0] = pm.s.origin[0];
		cl.predictedOrigins[frame][1] = pm.s.origin[1];
		cl.predictedOrigins[frame][2] = pm.s.origin[2];
	}

	// Smooth out stair climbing
	if (pm.s.pm_flags & PMF_ON_GROUND){
		step = pm.s.origin[2] - cl.predictedOrigins[(ack - 2) & CMD_MASK][2];
		if (step > 63 && step < 160){
			cl.predictedStep = step * 0.125f;
			cl.predictedStepTime = cls.realTime - cls.frameTime * 500.0f;
		}
	}

	// Copy results out for rendering
	cl.predictedOrigin[0] = pm.s.origin[0] * 0.125f;
	cl.predictedOrigin[1] = pm.s.origin[1] * 0.125f;
	cl.predictedOrigin[2] = pm.s.origin[2] * 0.125f;

	VectorCopy(pm.viewangles, cl.predictedAngles);
}