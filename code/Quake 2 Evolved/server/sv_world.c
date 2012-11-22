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


// world.c -- world query functions


#include "server.h"


/*
 =======================================================================

 ENTITY AREA CHECKING

 FIXME: this use of "area" is different from the BSP file use
 =======================================================================
*/

// FIXME: remove this mess!
#define	STRUCT_FROM_LINK(l,t,m) ((t *)((byte *)l - (int)&(((t *)0)->m)))
#define	EDICT_FROM_AREA(l)		STRUCT_FROM_LINK(l, edict_t, area)

#define	AREA_DEPTH				4
#define	AREA_NODES				32

typedef struct areaNode_s {
	struct	areaNode_s *children[2];

	int		axis;		// -1 = leaf node
	float	dist;

	link_t	triggerEdicts;
	link_t	solidEdicts;
} areaNode_t;

static areaNode_t	sv_areaNodes[AREA_NODES];
static int			sv_numAreaNodes;

static float		*sv_areaMins, *sv_areaMaxs;
static edict_t		**sv_areaList;
static int			sv_areaCount, sv_areaMaxCount;
static int			sv_areaType;


/*
 =================
 SV_ClearLink
 =================
*/
static void SV_ClearLink (link_t *l){

	l->prev = l->next = l;
}

/*
 =================
 SV_RemoveLink
 =================
*/
static void SV_RemoveLink (link_t *l){

	l->next->prev = l->prev;
	l->prev->next = l->next;
}

/*
 =================
 SV_InsertLinkBefore
 =================
*/
static void SV_InsertLinkBefore (link_t *l, link_t *before){

	l->next = before;
	l->prev = before->prev;
	l->prev->next = l;
	l->next->prev = l;
}

/*
 =================
 SV_CreateAreaNode

 Builds a uniformly subdivided tree for the given world size
 =================
*/
static areaNode_t *SV_CreateAreaNode (int depth, vec3_t mins, vec3_t maxs){

	areaNode_t	*areaNode;
	vec3_t		size;
	vec3_t		mins1, maxs1, mins2, maxs2;

	areaNode = &sv_areaNodes[sv_numAreaNodes];
	sv_numAreaNodes++;

	SV_ClearLink(&areaNode->triggerEdicts);
	SV_ClearLink(&areaNode->solidEdicts);

	if (depth == AREA_DEPTH){
		areaNode->axis = -1;
		areaNode->children[0] = areaNode->children[1] = NULL;
		return areaNode;
	}

	VectorSubtract(maxs, mins, size);
	if (size[0] > size[1])
		areaNode->axis = 0;
	else
		areaNode->axis = 1;

	areaNode->dist = 0.5 * (maxs[areaNode->axis] + mins[areaNode->axis]);

	VectorCopy(mins, mins1);
	VectorCopy(mins, mins2);
	VectorCopy(maxs, maxs1);
	VectorCopy(maxs, maxs2);

	maxs1[areaNode->axis] = mins2[areaNode->axis] = areaNode->dist;

	areaNode->children[0] = SV_CreateAreaNode(depth+1, mins2, maxs2);
	areaNode->children[1] = SV_CreateAreaNode(depth+1, mins1, maxs1);

	return areaNode;
}

/*
 =================
 SV_ClearWorld
 =================
*/
void SV_ClearWorld (void){

	Mem_Fill(sv_areaNodes, 0, sizeof(sv_areaNodes));
	sv_numAreaNodes = 0;

	SV_CreateAreaNode(0, sv.models[1]->mins, sv.models[1]->maxs);
}

/*
 =================
 SV_UnlinkEdict
 =================
*/
void SV_UnlinkEdict (edict_t *ent){

	if (!ent->area.prev)
		return;		// Not linked in anywhere

	SV_RemoveLink(&ent->area);

	ent->area.prev = ent->area.next = NULL;
}

/*
 =================
 SV_LinkEdict
 =================
*/
void SV_LinkEdict (edict_t *ent){

#define MAX_ENT_LEAFS	128

	areaNode_t	*node;
	float		max, v;
	int			leafs[MAX_ENT_LEAFS];
	int			clusters[MAX_ENT_LEAFS];
	int			numLeafs;
	int			i, j, k;
	int			area;
	int			topNode;

	if (ent->area.prev)
		SV_UnlinkEdict (ent);	// Unlink from old position

	if (ent == ge->edicts)
		return;		// Don't add the world

	if (!ent->inuse)
		return;

	// Set the size
	VectorSubtract(ent->maxs, ent->mins, ent->size);

	// Encode the size into the entity_state for client prediction
	if (ent->solid == SOLID_BBOX && !(ent->svflags & SVF_DEADMONSTER)){
		// Assume that x/y are equal and symetric
		i = ent->maxs[0]/8;
		if (i < 1)
			i = 1;
		if (i > 31)
			i = 31;

		// Z is not symetric
		j = (-ent->mins[2])/8;
		if (j < 1)
			j = 1;
		if (j > 31)
			j = 31;

		// And Z maxs can be negative...
		k = (ent->maxs[2]+32)/8;
		if (k < 1)
			k = 1;
		if (k > 63)
			k = 63;

		ent->s.solid = (k<<10) | (j<<5) | i;
	}
	else if (ent->solid == SOLID_BSP)
		ent->s.solid = 31;		// A SOLID_BBOX will never create this value
	else
		ent->s.solid = 0;

	// Set the abs box
	if (ent->solid == SOLID_BSP && (ent->s.angles[0] || ent->s.angles[1] || ent->s.angles[2])){
		// Expand for rotation
		max = 0;
		for (i = 0; i < 3; i++){
			v = fabs(ent->mins[i]);
			if (v > max)
				max = v;
			v = fabs(ent->maxs[i]);
			if (v > max)
				max = v;
		}

		for (i = 0; i < 3; i++){
			ent->absmin[i] = ent->s.origin[i] - max;
			ent->absmax[i] = ent->s.origin[i] + max;
		}
	}
	else {
		// Normal
		VectorAdd(ent->s.origin, ent->mins, ent->absmin);	
		VectorAdd(ent->s.origin, ent->maxs, ent->absmax);
	}

	// Because movement is clipped an epsilon away from an actual edge,
	// we must fully check even when bounding boxes don't quite touch
	ent->absmin[0] -= 1;
	ent->absmin[1] -= 1;
	ent->absmin[2] -= 1;
	ent->absmax[0] += 1;
	ent->absmax[1] += 1;
	ent->absmax[2] += 1;

	// Link to PVS leafs
	ent->num_clusters = 0;
	ent->areanum = 0;
	ent->areanum2 = 0;

	// Get all leafs, including solids
	numLeafs = CM_BoxLeafNums(ent->absmin, ent->absmax, leafs, MAX_ENT_LEAFS, &topNode);

	// Set areas
	for (i = 0; i < numLeafs; i++){
		clusters[i] = CM_LeafCluster(leafs[i]);
		area = CM_LeafArea(leafs[i]);
		if (area){
			// Doors may legally straggle two areas, but nothing should 
			// ever need more than that
			if (ent->areanum && ent->areanum != area){
				if (ent->areanum2 && ent->areanum2 != area && sv.state == SS_LOADING)
					Com_DPrintf(S_COLOR_YELLOW "Object touching 3 areas at %f %f %f\n", ent->absmin[0], ent->absmin[1], ent->absmin[2]);

				ent->areanum2 = area;
			}
			else
				ent->areanum = area;
		}
	}

	if (numLeafs >= MAX_ENT_LEAFS){
		// Assume we missed some leafs, and mark by headNode
		ent->num_clusters = -1;
		ent->headnode = topNode;
	}
	else {
		ent->num_clusters = 0;
		for (i = 0; i < numLeafs; i++){
			if (clusters[i] == -1)
				continue;		// Not a visible leaf

			for (j = 0; j < i; j++){
				if (clusters[j] == clusters[i])
					break;
			}

			if (j == i){
				if (ent->num_clusters == MAX_ENT_CLUSTERS){
					// Assume we missed some leafs, and mark by headNode
					ent->num_clusters = -1;
					ent->headnode = topNode;
					break;
				}

				ent->clusternums[ent->num_clusters++] = clusters[i];
			}
		}
	}

	// If first time, make sure old_origin is valid
	if (!ent->linkcount)
		VectorCopy(ent->s.origin, ent->s.old_origin);

	ent->linkcount++;

	if (ent->solid == SOLID_NOT)
		return;

	// Find the first node that the entity's box crosses
	node = sv_areaNodes;
	while (1){
		if (node->axis == -1)
			break;

		if (ent->absmin[node->axis] > node->dist)
			node = node->children[0];
		else if (ent->absmax[node->axis] < node->dist)
			node = node->children[1];
		else
			break;		// Crosses the node
	}

	// Link it in	
	if (ent->solid == SOLID_TRIGGER)
		SV_InsertLinkBefore(&ent->area, &node->triggerEdicts);
	else
		SV_InsertLinkBefore(&ent->area, &node->solidEdicts);
}

/*
 =================
 SV_AreaEdicts_r
 =================
*/
static void SV_AreaEdicts_r (areaNode_t *node){

	link_t		*l, *next, *start;
	edict_t		*check;
	int			count = 0;

	// Touch linked edicts
	if (sv_areaType == AREA_SOLID)
		start = &node->solidEdicts;
	else
		start = &node->triggerEdicts;

	for (l = start->next; l != start; l = next){
		next = l->next;
		check = EDICT_FROM_AREA(l);

		if (check->solid == SOLID_NOT)
			continue;		// Deactivated

		if (check->absmin[0] > sv_areaMaxs[0] || check->absmin[1] > sv_areaMaxs[1] || check->absmin[2] > sv_areaMaxs[2] || check->absmax[0] < sv_areaMins[0] || check->absmax[1] < sv_areaMins[1] || check->absmax[2] < sv_areaMins[2])
			continue;		// Not touching

		if (sv_areaCount == sv_areaMaxCount){
			Com_DPrintf(S_COLOR_YELLOW "SV_AreaEdicts_r: MAXCOUNT\n");
			return;
		}

		sv_areaList[sv_areaCount] = check;
		sv_areaCount++;
	}

	if (node->axis == -1)
		return;		// Terminal node

	// Recurse down both sides
	if (sv_areaMaxs[node->axis] > node->dist)
		SV_AreaEdicts_r(node->children[0]);
	if (sv_areaMins[node->axis] < node->dist)
		SV_AreaEdicts_r(node->children[1]);
}

/*
 =================
 SV_AreaEdicts
 =================
*/
int SV_AreaEdicts (vec3_t mins, vec3_t maxs, edict_t **list, int maxCount, int areaType){

	sv_areaMins = mins;
	sv_areaMaxs = maxs;
	sv_areaList = list;
	sv_areaCount = 0;
	sv_areaMaxCount = maxCount;
	sv_areaType = areaType;

	SV_AreaEdicts_r(sv_areaNodes);

	return sv_areaCount;
}


// =====================================================================

typedef struct {
	vec3_t		boxMins, boxMaxs;	// Enclose the test object along entire move
	float		*mins, *maxs;		// Size of the moving object
	vec3_t		mins2, maxs2;		// Size when clipping against mosnters
	float		*start, *end;
	trace_t		trace;
	edict_t		*passEdict;
	int			contentMask;
} moveClip_t;


/*
 =================
 SV_HullForEntity

 Returns a headNode that can be used for testing or clipping an object 
 of mins/maxs size.
 Offset is filled in to contain the adjustment that must be added to the
 testing object's origin to get a point to use with the returned hull.
 =================
*/
static int SV_HullForEntity (edict_t *ent){

	clipInlineModel_t	*model;

	// Decide which clipping hull to use, based on the size
	if (ent->solid == SOLID_BSP){
		// Explicit hulls in the BSP model
		model = sv.models[ent->s.modelindex];
		if (!model)
			Com_Error(ERR_DROP, "SV_HullForEntity: SOLID_BSP with a non BSP model");

		return model->headNode;
	}

	// Create a temp hull from bounding box sizes
	return CM_HeadNodeForBox(ent->mins, ent->maxs);
}

/*
 =================
 SV_ClipMoveToEntities
 =================
*/
static void SV_ClipMoveToEntities (moveClip_t *clip){

	trace_t		trace;
	int			i, num, headNode;
	edict_t		*touchList[MAX_EDICTS], *touch;
	float		*angles;

	num = SV_AreaEdicts(clip->boxMins, clip->boxMaxs, touchList, MAX_EDICTS, AREA_SOLID);

	// Be careful, it is possible to have an entity in this list removed 
	// before we get to it (killtriggered)
	for (i = 0; i < num; i++){
		touch = touchList[i];

		if (touch->solid == SOLID_NOT)
			continue;

		if (touch == clip->passEdict)
			continue;

		if (clip->trace.allsolid)
			return;

		if (clip->passEdict){
		 	if (touch->owner == clip->passEdict)
				continue;	// Don't clip against own missiles
			if (clip->passEdict->owner == touch)
				continue;	// Don't clip against owner
		}

		if (!(clip->contentMask & CONTENTS_DEADMONSTER) && (touch->svflags & SVF_DEADMONSTER))
			continue;

		// Might intersect, so do an exact clip
		headNode = SV_HullForEntity(touch);
		angles = touch->s.angles;
		if (touch->solid != SOLID_BSP)
			angles = vec3_origin;	// Boxes don't rotate

		if (touch->svflags & SVF_MONSTER)
			trace = CM_TransformedBoxTrace(clip->start, clip->end, clip->mins2, clip->maxs2, headNode, clip->contentMask, touch->s.origin, angles);
		else
			trace = CM_TransformedBoxTrace(clip->start, clip->end, clip->mins, clip->maxs, headNode,  clip->contentMask, touch->s.origin, angles);

		if (trace.allsolid || trace.startsolid || trace.fraction < clip->trace.fraction){
			trace.ent = touch;
			if (clip->trace.startsolid){
				clip->trace = trace;
				clip->trace.startsolid = true;
			}
			else
				clip->trace = trace;
		}
		else if (trace.startsolid)
			clip->trace.startsolid = true;
	}
}

/*
 =================
 SV_Trace

 Moves the given mins/maxs volume through the world from start to end.
 Passedict and edicts owned by passedict are explicitly not checked.
 =================
*/
trace_t SV_Trace (vec3_t start, vec3_t mins, vec3_t maxs, vec3_t end, edict_t *passEdict, int contentMask){

	moveClip_t	clip;
	int			i;

	if (!mins)
		mins = vec3_origin;
	if (!maxs)
		maxs = vec3_origin;

	Mem_Fill(&clip, 0, sizeof(moveClip_t));

	// Clip to world
	clip.trace = CM_BoxTrace(start, end, mins, maxs, 0, contentMask);
	clip.trace.ent = ge->edicts;
	if (clip.trace.fraction == 0)
		return clip.trace;		// Blocked by the world

	clip.contentMask = contentMask;
	clip.start = start;
	clip.end = end;
	clip.mins = mins;
	clip.maxs = maxs;
	clip.passEdict = passEdict;

	VectorCopy(mins, clip.mins2);
	VectorCopy(maxs, clip.maxs2);

	// Create the bounding box of the entire move
	for (i = 0; i < 3; i++){
		if (end[i] > start[i]){
			clip.boxMins[i] = start[i] + clip.mins2[i] - 1;
			clip.boxMaxs[i] = end[i] + clip.maxs2[i] + 1;
		}
		else {
			clip.boxMins[i] = end[i] + clip.mins2[i] - 1;
			clip.boxMaxs[i] = start[i] + clip.maxs2[i] + 1;
		}
	}

	// Clip to other solid entities
	SV_ClipMoveToEntities(&clip);

	return clip.trace;
}

/*
 =================
 SV_PointContents
 =================
*/
int SV_PointContents (vec3_t p){

	edict_t		*touch[MAX_EDICTS], *hit;
	int			i, num, headNode;
	int			contents;
	float		*angles;

	// Get base contents from world
	contents = CM_PointContents(p, sv.models[1]->headNode);

	// Or in contents from all the other entities
	num = SV_AreaEdicts(p, p, touch, MAX_EDICTS, AREA_SOLID);

	for (i = 0; i < num; i++){
		hit = touch[i];

		// Might intersect, so do an exact clip
		headNode = SV_HullForEntity(hit);
		angles = hit->s.angles;
		if (hit->solid != SOLID_BSP)
			angles = vec3_origin;	// Boxes don't rotate

		contents |= CM_TransformedPointContents(p, headNode, hit->s.origin, hit->s.angles);
	}

	return contents;
}
