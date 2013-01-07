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
// r_cull.c - Culling functions
//


#include "r_local.h"


/*
 ==============================================================================

 VIEW FRUSTUM CULLING

 ==============================================================================
*/


/*
 ==================
 R_CullBounds

 TODO: make sure this is valid
 ==================
*/
int R_CullBounds (const vec3_t mins, const vec3_t maxs, int planeBits){

	cplane_t	*plane;
	int			side, cullBits;
	int			i;

	cullBits = CULL_IN;

	// Check against frustum planes
	for (i = 0, plane = rg.viewParms.frustum; i < NUM_FRUSTUM_PLANES; i++, plane++){
		if (!(planeBits & BIT(i)))
			continue;

		side = BoxOnPlaneSide(mins, maxs, plane);

		if (side == PLANESIDE_BACK){
			rg.pc.cullBoundsOut++;
			return CULL_OUT;
		}

		if (side == PLANESIDE_CROSS)
			cullBits |= BIT(i);
	}

	if (cullBits != CULL_IN)
		rg.pc.cullBoundsClip++;
	else
		rg.pc.cullBoundsIn++;

	return cullBits;
}

/*
 ==================
 R_CullLocalBounds

 TODO: make sure this is valid
 ==================
*/
int R_CullLocalBounds (const vec3_t mins, const vec3_t maxs, const vec3_t origin, const vec3_t axis[3], int planeBits){

	cplane_t	*plane;
	vec3_t		corner, worldCorners[8];
	int			side, cullBits;
	int			i, j;

	cullBits = CULL_IN;

	// Compute the corner points
	for (i = 0; i < 8; i++){
		corner[0] = (i & 1) ? mins[0] : maxs[0];
		corner[1] = (i & 2) ? mins[1] : maxs[1];
		corner[2] = (i & 4) ? mins[2] : maxs[2];

		// Transform into world space
		R_LocalPointToWorld(corner, worldCorners[i], origin, axis);
	}

	// Check against frustum planes
	for (i = 0, plane = rg.viewParms.frustum; i < NUM_FRUSTUM_PLANES; i++, plane++){
		if (!(planeBits & BIT(i)))
			continue;

		side = 0;

		for (j = 0; j < 8; j++){
			side |= PointOnPlaneSide(worldCorners[j], 0.0f, plane);

			if (side == PLANESIDE_CROSS)
				break;
		}

		if (side == PLANESIDE_BACK){
			rg.pc.cullBoundsOut++;
			return CULL_OUT;
		}

		if (side == PLANESIDE_CROSS)
			cullBits |= BIT(i);
	}

	if (cullBits != CULL_IN)
		rg.pc.cullBoundsClip++;
	else
		rg.pc.cullBoundsIn++;

	return cullBits;
}

/*
 ==================
 R_CullBox

 Returns true if the box is completely outside the frustum
 ==================
*/
bool R_CullBox (const vec3_t mins, const vec3_t maxs, int clipFlags){

	cplane_t	*plane;
	int			i;

	if (r_skipCulling->integerValue)
		return false;

	for (i = 0, plane = rg.viewParms.frustum; i < NUM_FRUSTUM_PLANES; i++, plane++){
		if (!(clipFlags & BIT(i)))
			continue;

		switch (plane->signbits){
		case 0:
			if (plane->normal[0] * maxs[0] + plane->normal[1] * maxs[1] + plane->normal[2] * maxs[2] < plane->dist){
				rg.pc.cullBoundsOut++;
				return true;
			}

			break;
		case 1:
			if (plane->normal[0] * mins[0] + plane->normal[1] * maxs[1] + plane->normal[2] * maxs[2] < plane->dist){
				rg.pc.cullBoundsOut++;
				return true;
			}

			break;
		case 2:
			if (plane->normal[0] * maxs[0] + plane->normal[1] * mins[1] + plane->normal[2] * maxs[2] < plane->dist){
				rg.pc.cullBoundsOut++;
				return true;
			}

			break;
		case 3:
			if (plane->normal[0] * mins[0] + plane->normal[1] * mins[1] + plane->normal[2] * maxs[2] < plane->dist){
				rg.pc.cullBoundsOut++;
				return true;
			}

			break;
		case 4:
			if (plane->normal[0] * maxs[0] + plane->normal[1] * maxs[1] + plane->normal[2] * mins[2] < plane->dist){
				rg.pc.cullBoundsOut++;
				return true;
			}

			break;
		case 5:
			if (plane->normal[0] * mins[0] + plane->normal[1] * maxs[1] + plane->normal[2] * mins[2] < plane->dist){
				rg.pc.cullBoundsOut++;
				return true;
			}

			break;
		case 6:
			if (plane->normal[0] * maxs[0] + plane->normal[1] * mins[1] + plane->normal[2] * mins[2] < plane->dist){
				rg.pc.cullBoundsOut++;
				return true;
			}

			break;
		case 7:
			if (plane->normal[0] * mins[0] + plane->normal[1] * mins[1] + plane->normal[2] * mins[2] < plane->dist){
				rg.pc.cullBoundsOut++;
				return true;
			}

			break;
		default:
			rg.pc.cullBoundsIn++;
			return false;
		}
	}

	rg.pc.cullBoundsIn++;

	return false;
}

/*
 ==================
 R_CullSphere

 Returns true if the sphere is completely outside the frustum
 ==================
*/
bool R_CullSphere (const vec3_t origin, float radius, int clipFlags){

	cplane_t	*plane;
	int			i;

	if (r_skipCulling->integerValue)
		return false;

	for (i = 0, plane = rg.viewParms.frustum; i < NUM_FRUSTUM_PLANES; i++, plane++){
		if (!(clipFlags & BIT(i)))
			continue;

		if (DotProduct(origin, plane->normal) - plane->dist <= -radius){
			rg.pc.cullSphereOut++;
			return true;
		}
	}

	rg.pc.cullSphereIn++;

	return false;
}


/*
 ==============================================================================

 LIGHT FRUSTUM CULLING

 ==============================================================================
*/


/*
 ==================
 R_LightCullBounds

 TODO: make sure this is valid
 ==================
*/
int R_LightCullBounds (lightData_t *lightData, const vec3_t mins, const vec3_t maxs, int planeBits){

	cplane_t	*plane;
	int			side, cullBits;
	int			i;

	cullBits = CULL_IN;

	// Check against frustum planes
	for (i = 0, plane = lightData->frustum; i < 6; i++, plane++){
		if (!(planeBits & BIT(i)))
			continue;

		side = BoxOnPlaneSide(mins, maxs, plane);

		if (side == PLANESIDE_BACK){
			rg.pc.cullBoundsOut++;
			return CULL_OUT;
		}

		if (side == PLANESIDE_CROSS)
			cullBits |= BIT(i);
	}

	if (cullBits != CULL_IN)
		rg.pc.cullBoundsClip++;
	else
		rg.pc.cullBoundsIn++;

	return cullBits;
}

/*
 ==================
 R_LightCullLocalBounds

 TODO: make sure this is valid
 ==================
*/
int R_LightCullLocalBounds (lightData_t *lightData, const vec3_t mins, const vec3_t maxs, const vec3_t origin, const vec3_t axis[2], int planeBits){

	cplane_t	*plane;
	vec3_t		corner, worldCorners[8];
	int			side, cullBits;
	int			i, j;

	cullBits = CULL_IN;

	// Compute the corner points
	for (i = 0; i < 8; i++){
		corner[0] = (i & 1) ? mins[0] : maxs[0];
		corner[1] = (i & 2) ? mins[1] : maxs[1];
		corner[2] = (i & 4) ? mins[2] : maxs[2];

		// Transform into world space
		R_LocalPointToWorld(corner, worldCorners[i], origin, axis);
	}

	// Check against frustum planes
	for (i = 0, plane = lightData->frustum; i < 6; i++, plane++){
		if (!(planeBits & BIT(i)))
			continue;

		side = 0;

		for (j = 0; j < 8; j++){
			side |= PointOnPlaneSide(worldCorners[j], 0.0f, plane);

			if (side == PLANESIDE_CROSS)
				break;
		}

		if (side == PLANESIDE_BACK){
			rg.pc.cullBoundsOut++;
			return CULL_OUT;
		}

		if (side == PLANESIDE_CROSS)
			cullBits |= BIT(i);
	}

	if (cullBits != CULL_IN)
		rg.pc.cullBoundsClip++;
	else
		rg.pc.cullBoundsIn++;

	return cullBits;
}

/*
 ==================
 
 ==================
*/
int R_LightCullSphere (){

}

/*
 ==================
 
 ==================
*/
int R_LightCullLocalSphere (lightData_t *lightData, float radius, const vec3_t origin, const vec3_t axis[3], int planeBits){

}

/*
 ==================
 R_CullLightBounds

 TODO: make sure this is valid
 ==================
*/
int R_CullLightBounds (lightData_t *lightData, int planeBits){

	cplane_t	*plane;
	int			side, cullBits;
	int			i;

	cullBits = CULL_IN;

	// Check against frustum planes
	for (i = 0, plane = rg.viewParms.frustum; i < NUM_FRUSTUM_PLANES; i++, plane++){
		if (!(planeBits & BIT(i)))
			continue;

		side = BoxOnPlaneSide(lightData->mins, lightData->maxs, plane);

		if (side == PLANESIDE_BACK){
			rg.pc.cullBoundsOut++;
			return CULL_OUT;
		}

		if (side == PLANESIDE_CROSS)
			cullBits |= BIT(i);
	}

	if (cullBits != CULL_IN)
		rg.pc.cullBoundsClip++;
	else
		rg.pc.cullBoundsIn++;

	return cullBits;
}

/*
 ==================
 R_CullLightVolume

 FIXME: never returns CULL_OUT, because side always == 3 which renders the volume even if
 it's out side the frustum
 ==================
*/
int R_CullLightVolume (lightData_t *lightData, int planeBits){

	cplane_t	*plane;
	int			side, cullBits;
	int			i, j;

	cullBits = CULL_IN;

	// Check against frustum planes
	for (i = 0, plane = rg.viewParms.frustum; i < NUM_FRUSTUM_PLANES; i++, plane++){
		if (!(planeBits & BIT(i)))
			continue;

		side = 0;

		for (j = 0; j < 8; j++){
			side |= PointOnPlaneSide(lightData->corners[j], 0.0f, plane);

			if (side == PLANESIDE_CROSS)
				break;
		}

		if (side == PLANESIDE_BACK){
			rg.pc.cullBoundsOut++;
			return CULL_OUT;
		}

		if (side == PLANESIDE_CROSS)
			cullBits |= BIT(i);
	}

	if (cullBits != CULL_IN)
		rg.pc.cullBoundsClip++;
	else
		rg.pc.cullBoundsIn++;

	return cullBits;
}