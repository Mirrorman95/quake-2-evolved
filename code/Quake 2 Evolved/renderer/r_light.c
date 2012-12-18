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
// r_light.c - Light management
//

// TODO:
// - R_ClusterPVS is messed up for static lights
// - culling
// - fog and the other light stuff
// - make sure the frustum is right for R_SetupNearClipVolume
// - light scissor, seems like it clips the entire screen and not just the light


#include "r_local.h"


/*
 ==============================================================================

 LIGHT PARSING

 ==============================================================================
*/


/*
 ==================
 R_ParseLight
 ==================
*/
static bool R_ParseLight (script_t *script){

	token_t		token;
	lightData_t	*lightData;
	vec3_t		color;
	int			index;

	// Parse the index number
	if (!PS_ReadInteger(script, &index)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing index number in light file\n");
		return false;
	}

	if (index < 0 || index >= MAX_STATIC_LIGHTS){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid value of %i for index number in light file\n", index);
		return false;
	}

	lightData = &rg.staticLights[rg.numStaticLights++];

	// Set detault values
	Str_Copy(lightData->parms.name, "light", sizeof(lightData->parms.name));

	lightData->parms.index = rg.numStaticLights - 1;

	lightData->parms.type = RL_POINT;

	VectorClear(lightData->parms.origin);
	VectorClear(lightData->parms.center);
	VectorClear(lightData->parms.angles);

	VectorSet(lightData->parms.radius, 100.0f, 100.0f, 100.0f);

	lightData->parms.xMin = 0.0f;
	lightData->parms.xMax = 0.0f;

	lightData->parms.yMin = 0.0f;
	lightData->parms.yMax = 0.0f;

	lightData->parms.zFar = 0.0f;
	lightData->parms.zNear = 0.0;

	lightData->parms.noShadows = false;

	lightData->parms.fogDistance = 500.0f;
	lightData->parms.fogHeight = 500.0f;

	lightData->parms.style = 0;
	lightData->parms.detailLevel = 0;

	Str_Copy(lightData->parms.material, "lights/default", sizeof(lightData->parms.material));

	lightData->parms.materialParms[MATERIALPARM_RED] = 1.0f;
	lightData->parms.materialParms[MATERIALPARM_GREEN] = 1.0f;
	lightData->parms.materialParms[MATERIALPARM_BLUE] = 1.0f;
	lightData->parms.materialParms[MATERIALPARM_ALPHA] = 1.0f;
	lightData->parms.materialParms[MATERIALPARM_TIMEOFFSET] = 0.0f;
	lightData->parms.materialParms[MATERIALPARM_DIVERSITY] = 0.0f;
	lightData->parms.materialParms[MATERIALPARM_MISC] = 0.0f;
	lightData->parms.materialParms[MATERIALPARM_MODE] = 0.0f;

	// Parse the light parameters
	if (!PS_ExpectTokenString(script, &token, "{", true)){
		Com_Printf(S_COLOR_YELLOW "WARNING: expected '{', found '%s' instead in light file (index %i)\n", token.string, index);
		return false;
	}

	while (1){
		if (!PS_ReadToken(script, &token)){
			Com_Printf(S_COLOR_YELLOW "WARNING: no concluding '}' in light file (index %i)\n", index);
			return false;	// End of script
		}

		if (!Str_ICompare(token.string, "}"))
			break;			// End of light

		// Parse the parameter
		if (!Str_ICompare(token.string, "name")){
			if (!PS_ReadToken(script, &token)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'name' in light file (index %i)\n", index);
				return false;
			}

			Str_Copy(lightData->parms.name, token.string, sizeof(lightData->parms.name));
		}
		else if (!Str_ICompare(token.string, "type")){
			if (!PS_ReadInteger(script, &lightData->parms.type)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'type' in light file (index %i)\n", index);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "origin")){
			if (!PS_ReadMatrix1D(script, 3, lightData->parms.origin)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'origin' in light file (index %i)\n", index);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "center")){
			if (!PS_ReadMatrix1D(script, 3, lightData->parms.center)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'center' in light file (index %i)\n", index);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "angles")){
			if (!PS_ReadMatrix1D(script, 3, lightData->parms.angles)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'angles' in light file (index %i)\n", index);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "radius")){
			if (!PS_ReadMatrix1D(script, 3, lightData->parms.radius)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'radius' in light file (index %i)\n", index);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "xMin")){
			if (!PS_ReadFloat(script, &lightData->parms.xMin)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'xMin' in light file (index %i)\n", index);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "xMax")){
			if (!PS_ReadFloat(script, &lightData->parms.xMax)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'xMax' in light file (index %i)\n", index);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "yMin")){
			if (!PS_ReadFloat(script, &lightData->parms.yMin)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'yMin' in light file (index %i)\n", index);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "yMax")){
			if (!PS_ReadFloat(script, &lightData->parms.yMax)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'yMax' in light file (index %i)\n", index);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "zNear")){
			if (!PS_ReadFloat(script, &lightData->parms.zNear)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'zNear' in light file (index %i)\n", index);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "zFar")){
			if (!PS_ReadFloat(script, &lightData->parms.zFar)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'zFar' in light file (index %i)\n", index);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "noShadows")){
			if (!PS_ReadBool(script, &lightData->parms.noShadows)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'noShadows' in light file (index %i)\n", index);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "fogDistance")){
			if (!PS_ReadFloat(script, &lightData->parms.fogDistance)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'fogDistance' in light file (index %i)\n", index);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "fogHeight")){
			if (!PS_ReadFloat(script, &lightData->parms.fogHeight)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'fogHeight' in light file (index %i)\n", index);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "style")){
			if (!PS_ReadInteger(script, &lightData->parms.style)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'style' in light file (index %i)\n", index);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "detailLevel")){
			if (!PS_ReadInteger(script, &lightData->parms.detailLevel)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'detailLevel' in light file (index %i)\n", index);
				return false;
			}
		}
		else if (!Str_ICompare(token.string, "material")){
			if (!PS_ReadToken(script, &token)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'material' in light file (index %i)\n", index);
				return false;
			}

			Str_Copy(lightData->parms.material, token.string, sizeof(lightData->parms.material));
		}
		else if (!Str_ICompare(token.string, "color")){
			if (!PS_ReadMatrix1D(script, 3, color)){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'color' in light file (index %i)\n", index);
				return false;
			}

			lightData->parms.materialParms[MATERIALPARM_RED] = color[0];
			lightData->parms.materialParms[MATERIALPARM_GREEN] = color[1];
			lightData->parms.materialParms[MATERIALPARM_BLUE] = color[2];
		}
		else if (!Str_ICompare(token.string, "alpha")){
			if (!PS_ReadFloat(script, &lightData->parms.materialParms[MATERIALPARM_ALPHA])){
				Com_Printf(S_COLOR_YELLOW "WARNING: missing parameter for 'alpha' in light file (index %i)\n", index);
				return false;
			}
		}
		else {
			Com_Printf(S_COLOR_YELLOW "WARNING: unknown parameter '%s' in light file (index %i)\n", token.string, index);
			return false;
		}
	}

	return true;
}

/*
 ==================
 R_LoadLights
 ==================
*/
void R_LoadLights (const char *name){

	script_t	*script;
	token_t		token;

	// Copy the name
	Str_Copy(rg.staticLights->fileName, name, sizeof(rg.staticLights->fileName));

	// Load the script file
	script = PS_LoadScriptFile(name);
	if (!script){
		Com_Printf(S_COLOR_RED "Light file %s not found\n", name);
		return;
	}

	PS_SetScriptFlags(script, SF_NOWARNINGS | SF_NOERRORS | SF_ALLOWPATHNAMES);

	// Parse it
	while (1){
		if (!PS_ReadToken(script, &token))
			break;		// End of script

		if (!Str_ICompare(token.string, "light")){
			if (!R_ParseLight(script))
				break;
		}
		else {
			Com_Printf(S_COLOR_YELLOW "WARNING: expected 'light', found '%s' instead in light file\n", token.string);
			break;
		}
	}

	// Free the script file
	PS_FreeScript(script);
}


/*
 ==============================================================================

 LIGHT FRUSTUM CULLING

 ==============================================================================
*/


/*
 ==================
 
 ==================
*/
void R_LightCullBounds (){

}

/*
 ==================
 
 ==================
*/
void R_LightCullLocalBounds (){

}

/*
 ==================
 
 ==================
*/
void R_LightCullSphere (){

}

/*
 ==================
 
 ==================
*/
void R_LightCullLocalSphere (){

}


// ============================================================================


/*
 ==================
 
 ==================
*/
static void R_SetStaticLightFrustum (lightData_t *lightData, vec3_t axis[3]){

	float	dot;
	int		i;

	if (lightData->type != RL_PROJECTED){
		for (i = 0; i < 3; i++){
			dot = DotProduct(lightData->parms.origin, axis[i]);

			VectorCopy(axis[i], lightData->frustum[i*2+0].normal);
			lightData->frustum[i*2+0].dist = dot - lightData->parms.radius[i];
			lightData->frustum[i*2+0].type = PlaneTypeForNormal(lightData->frustum[i*2+0].normal);
			SetPlaneSignbits(&lightData->frustum[i*2+0]);

			VectorNegate(axis[i], lightData->frustum[i*2+1].normal);
			lightData->frustum[i*2+1].dist = -dot - lightData->parms.radius[i];
			lightData->frustum[i*2+1].type = PlaneTypeForNormal(lightData->frustum[i*2+1].normal);
			SetPlaneSignbits(&lightData->frustum[i*2+1]);
		}
	}
	else {

	}
}

/*
 ==================
 
 ==================
*/
static void R_SetDynamicLightFrustum (const renderLight_t *renderLight, lightData_t *lightData){

	float	dot;
	int		i;

	if (lightData->type != RL_PROJECTED){
		for (i = 0; i < 3; i++){
			dot = DotProduct(renderLight->origin, renderLight->axis[i]);

			VectorCopy(renderLight->axis[i], lightData->frustum[i*2+0].normal);
			lightData->frustum[i*2+0].dist = dot - renderLight->radius[i];
			lightData->frustum[i*2+0].type = PlaneTypeForNormal(lightData->frustum[i*2+0].normal);
			SetPlaneSignbits(&lightData->frustum[i*2+0]);

			VectorNegate(renderLight->axis[i], lightData->frustum[i*2+1].normal);
			lightData->frustum[i*2+1].dist = -dot - renderLight->radius[i];
			lightData->frustum[i*2+1].type = PlaneTypeForNormal(lightData->frustum[i*2+1].normal);
			SetPlaneSignbits(&lightData->frustum[i*2+1]);
		}
	}
	else {

	}
}

/*
 ==================
 
 ==================
*/
static void R_SetupStaticLightData (lightData_t *lightData, bool inWorld){

	material_t	*material;
	leaf_t		*leaf;
	byte		pvs[MAX_MAP_LEAFS/8];
	vec3_t		origin, axis[3];
	vec3_t		direction, corner;
	vec3_t		lVector, rVector;
	vec3_t		dVector, uVector;
	vec3_t		nVector, fVector;
	float		ratio;
	float		distance, maxDistance;
	int			i;

	if (lightData->valid)
		return;		// Already computed the light data

	lightData->valid = true;
	lightData->precached = false;

	// If the light data was precached, copy it and return immediately
	if (!r_skipLightCache->integerValue){

	}

	// Set the name
	Str_Copy(lightData->name, lightData->parms.name, sizeof(lightData->name));

	// Set the index
	lightData->index = lightData->parms.index;

	// Set the type
	lightData->type = lightData->parms.type;

	// Set the material
	material = R_RegisterMaterialLight(lightData->parms.material);

	if (material)
		lightData->material = material;
	else {
		if (lightData->type != RL_PROJECTED)
			lightData->material = rg.defaultLightMaterial;
		else
			lightData->material = rg.defaultProjectedLightMaterial;
	}

	// Set the material parameters
	lightData->materialParms[MATERIALPARM_RED] = lightData->parms.materialParms[MATERIALPARM_RED];
	lightData->materialParms[MATERIALPARM_GREEN] = lightData->parms.materialParms[MATERIALPARM_GREEN];
	lightData->materialParms[MATERIALPARM_BLUE] = lightData->parms.materialParms[MATERIALPARM_BLUE];
	lightData->materialParms[MATERIALPARM_ALPHA] = lightData->parms.materialParms[MATERIALPARM_ALPHA];
	lightData->materialParms[MATERIALPARM_TIMEOFFSET] = lightData->parms.materialParms[MATERIALPARM_TIMEOFFSET];
	lightData->materialParms[MATERIALPARM_DIVERSITY] = lightData->parms.materialParms[MATERIALPARM_DIVERSITY];
	lightData->materialParms[MATERIALPARM_MISC] = lightData->parms.materialParms[MATERIALPARM_MISC];
	lightData->materialParms[MATERIALPARM_MODE] = lightData->parms.materialParms[MATERIALPARM_MODE];

	// Compute the origin, direction, and axis
	AnglesToMat3(lightData->parms.angles, axis);

	if (lightData->type == RL_POINT || lightData->type == RL_CUBIC){
		VectorRotate(lightData->parms.center, axis, origin);
		VectorAdd(lightData->parms.origin, origin, lightData->origin);
		VectorClear(lightData->direction);
		Matrix3_Identity(lightData->axis);
	}
	else if (lightData->type == RL_PROJECTED){
		VectorNegate(axis[2], direction);

		VectorCopy(lightData->parms.origin, lightData->origin);
		VectorCopy(direction, lightData->direction);
		VectorToMatrix(direction, lightData->axis);
	}
	else {

	}

	// Compute the bounding volume
	if (lightData->type != RL_PROJECTED){
		// Compute the corner points
		for (i = 0; i < 8; i++){
			corner[0] = (i & 1) ? -lightData->parms.radius[0] : lightData->parms.radius[0];
			corner[1] = (i & 2) ? -lightData->parms.radius[1] : lightData->parms.radius[1];
			corner[2] = (i & 4) ? -lightData->parms.radius[2] : lightData->parms.radius[2];

			// Transform into world space
			R_LocalPointToWorld(corner, lightData->corners[i], lightData->parms.origin, axis);
		}

		// Compute the bounding box
		BoundsFromPoints(lightData->bounds[0], lightData->bounds[1], lightData->corners);

		// Compute the frustum planes
		R_SetStaticLightFrustum(lightData, axis);
	}
	else {
		// Compute the corner points
		ratio = lightData->parms.zFar / lightData->parms.zNear;

		VectorScale(axis[0], lightData->parms.xMin, lVector);
		VectorScale(axis[0], lightData->parms.xMax, rVector);

		VectorScale(axis[1], lightData->parms.yMin, dVector);
		VectorScale(axis[1], lightData->parms.yMax, uVector);

		VectorScale(axis[2], lightData->parms.zNear, nVector);
		VectorScale(axis[2], lightData->parms.zFar, fVector);

		// TODO: corners

		// Compute the bounding box
		BoundsFromPoints(lightData->bounds[0], lightData->bounds[1], lightData->corners);

		// Compute the frustum planes
	}

	// Compute the light range
	maxDistance = 0.0f;

	if (lightData->type != RL_DIRECTIONAL){
		for (i = 0; i < 8; i++){
			distance = Distance(lightData->origin, lightData->corners[i]);

			if (distance > maxDistance)
				maxDistance = distance;
		}
	}
	else {
		for (i = 0; i < 8; i++){
			distance = DotProduct(lightData->direction, lightData->corners[i]) - DotProduct(lightData->direction, lightData->origin);

			if (distance > maxDistance)
				maxDistance = distance;
		}
	}

	lightData->lightRange = maxDistance + 8.0f;

	// Compute the fog plane

	// Compute the transformation matrices
	if (lightData->type != RL_PROJECTED){
		lightData->projectionMatrix[ 0] = 0.5f / lightData->parms.radius[0];
		lightData->projectionMatrix[ 1] = 0.0f;
		lightData->projectionMatrix[ 2] = 0.0f;
		lightData->projectionMatrix[ 3] = 0.0f;
		lightData->projectionMatrix[ 4] = 0.0f;
		lightData->projectionMatrix[ 5] = 0.5f / lightData->parms.radius[1];
		lightData->projectionMatrix[ 6] = 0.0f;
		lightData->projectionMatrix[ 7] = 0.0f;
		lightData->projectionMatrix[ 8] = 0.0f;
		lightData->projectionMatrix[ 9] = 0.0f;
		lightData->projectionMatrix[10] = 0.5f / lightData->parms.radius[2];
		lightData->projectionMatrix[11] = 0.0f;
		lightData->projectionMatrix[12] = 0.5f;
		lightData->projectionMatrix[13] = 0.5f;
		lightData->projectionMatrix[14] = 0.5f;
		lightData->projectionMatrix[15] = 1.0f;

		lightData->modelviewMatrix[ 0] = -axis[0][0];
		lightData->modelviewMatrix[ 1] = axis[1][0];
		lightData->modelviewMatrix[ 2] = -axis[2][0];
		lightData->modelviewMatrix[ 3] = 0.0f;
		lightData->modelviewMatrix[ 4] = -axis[0][1];
		lightData->modelviewMatrix[ 5] = axis[1][1];
		lightData->modelviewMatrix[ 6] = -axis[2][1];
		lightData->modelviewMatrix[ 7] = 0.0f;
		lightData->modelviewMatrix[ 8] = -axis[0][2];
		lightData->modelviewMatrix[ 9] = axis[1][2];
		lightData->modelviewMatrix[10] = -axis[2][2];
		lightData->modelviewMatrix[11] = 0.0f;
		lightData->modelviewMatrix[12] = DotProduct(lightData->parms.origin, axis[0]);
		lightData->modelviewMatrix[13] = -DotProduct(lightData->parms.origin, axis[1]);
		lightData->modelviewMatrix[14] = DotProduct(lightData->parms.origin, axis[2]);
		lightData->modelviewMatrix[15] = 1.0f;

		Matrix4_Multiply(lightData->projectionMatrix, lightData->modelviewMatrix, lightData->modelviewProjectionMatrix);
	}
	else {
		lightData->projectionMatrix[ 0] = 0.5f * (2.0f * lightData->parms.zNear / (lightData->parms.xMax - lightData->parms.xMin));
		lightData->projectionMatrix[ 1] = 0.0f;
		lightData->projectionMatrix[ 2] = 0.0f;
		lightData->projectionMatrix[ 3] = 0.0f;
		lightData->projectionMatrix[ 4] = 0.0f;
		lightData->projectionMatrix[ 5] = -0.5f * (2.0f * lightData->parms.zNear / (lightData->parms.yMax - lightData->parms.yMin));
		lightData->projectionMatrix[ 6] = 0.0f;
		lightData->projectionMatrix[ 7] = 0.0f;
		lightData->projectionMatrix[ 8] = 0.5f * ((lightData->parms.xMax + lightData->parms.xMin) / (lightData->parms.xMax - lightData->parms.xMin)) - 0.5f;
		lightData->projectionMatrix[ 9] = -0.5f * ((lightData->parms.yMax + lightData->parms.yMin) / (lightData->parms.yMax - lightData->parms.yMin)) - 0.5f;
		lightData->projectionMatrix[10] = 1.0f / (lightData->parms.zFar - lightData->parms.zNear);
		lightData->projectionMatrix[11] = -1.0f;
		lightData->projectionMatrix[12] = 0.0f;
		lightData->projectionMatrix[13] = 0.0f;
		lightData->projectionMatrix[14] = -0.5f * ((lightData->parms.zFar + lightData->parms.zNear) / (lightData->parms.zFar - lightData->parms.zNear)) + 0.5f;
		lightData->projectionMatrix[15] = 0.0f;

		lightData->modelviewMatrix[ 0] = axis[0][0];
		lightData->modelviewMatrix[ 1] = axis[1][0];
		lightData->modelviewMatrix[ 2] = -axis[2][0];
		lightData->modelviewMatrix[ 3] = 0.0f;
		lightData->modelviewMatrix[ 4] = axis[0][1];
		lightData->modelviewMatrix[ 5] = axis[1][1];
		lightData->modelviewMatrix[ 6] = -axis[2][1];
		lightData->modelviewMatrix[ 7] = 0.0f;
		lightData->modelviewMatrix[ 8] = axis[0][2];
		lightData->modelviewMatrix[ 9] = axis[1][2];
		lightData->modelviewMatrix[10] = -axis[2][2];
		lightData->modelviewMatrix[11] = 0.0f;
		lightData->modelviewMatrix[12] = -DotProduct(lightData->parms.origin, axis[0]);
		lightData->modelviewMatrix[13] = -DotProduct(lightData->parms.origin, axis[1]);
		lightData->modelviewMatrix[14] = DotProduct(lightData->parms.origin, axis[2]);
		lightData->modelviewMatrix[15] = 1.0f;

		Matrix4_Multiply(lightData->projectionMatrix, lightData->modelviewMatrix, lightData->modelviewProjectionMatrix);
	}

	// Set no shadows
	lightData->noShadows = lightData->parms.noShadows;

	// Set style
	lightData->style = lightData->parms.style;

	// TODO: R_ClusterPVS makes black surfaces..?

	// Set up the PVS and area
	if (lightData->type == RL_DIRECTIONAL || lightData->material->lightType != LT_GENERIC || !inWorld){
		lightData->pvs = NULL;
		lightData->area = -1;
	}
	else {
		leaf = R_PointInLeaf(lightData->origin);

		if (leaf->cluster == -1 || leaf->area == -1){
			lightData->pvs = NULL;
			lightData->area = -1;
		}
		else {
			R_ClusterPVS(leaf->cluster, pvs);

			lightData->pvs = pvs;
			lightData->area = leaf->area;
		}
	}
}

/*
 ==================
 
 ==================
*/
static void R_SetupDynamicLightData (const renderLight_t *renderLight, lightData_t *lightData, bool inWorld){

	leaf_t	*leaf;
	byte	pvs[MAX_MAP_LEAFS/8];
	vec3_t	origin;
	vec3_t	direction, corner;
	vec3_t	lVector, rVector;
	vec3_t	dVector, uVector;
	vec3_t	nVector, fVector;
	float	ratio;
	float	distance, maxDistance;
	int		i;

	lightData->valid = true;
	lightData->precached = false;

	// Set the name
	Str_SPrintf(lightData->name, sizeof(lightData->name), "dynamicLight_%i", renderLight->index);

	// Set the index
	lightData->index = renderLight->index;

	// Set the type
	lightData->type = renderLight->type;

	// Set the material
	if (renderLight->material)
		lightData->material = renderLight->material;
	else {
		if (lightData->type != RL_PROJECTED)
			lightData->material = rg.defaultLightMaterial;
		else
			lightData->material = rg.defaultProjectedLightMaterial;
	}

	// Set the material parameters
	lightData->materialParms[MATERIALPARM_RED] = renderLight->materialParms[MATERIALPARM_RED];
	lightData->materialParms[MATERIALPARM_GREEN] = renderLight->materialParms[MATERIALPARM_GREEN];
	lightData->materialParms[MATERIALPARM_BLUE] = renderLight->materialParms[MATERIALPARM_BLUE];
	lightData->materialParms[MATERIALPARM_ALPHA] = renderLight->materialParms[MATERIALPARM_ALPHA];
	lightData->materialParms[MATERIALPARM_TIMEOFFSET] = renderLight->materialParms[MATERIALPARM_TIMEOFFSET];
	lightData->materialParms[MATERIALPARM_DIVERSITY] = renderLight->materialParms[MATERIALPARM_DIVERSITY];
	lightData->materialParms[MATERIALPARM_MISC] = renderLight->materialParms[MATERIALPARM_MISC];
	lightData->materialParms[MATERIALPARM_MODE] = renderLight->materialParms[MATERIALPARM_MODE];

	// Compute the origin, direction, and axis
	if (lightData->type == RL_POINT || lightData->type == RL_CUBIC){
		VectorRotate(renderLight->center, renderLight->axis, origin);
		VectorAdd(renderLight->origin, origin, lightData->origin);
		VectorClear(lightData->direction);
		Matrix3_Identity(lightData->axis);
	}
	else if (lightData->type == RL_PROJECTED){
		VectorNegate(renderLight->axis[2], direction);

		VectorCopy(renderLight->origin, lightData->origin);
		VectorCopy(direction, lightData->direction);
		VectorToMatrix(direction, lightData->axis);
	}
	else {

	}

	// Compute the bounding volume
	if (lightData->type != RL_PROJECTED){
		// Compute the corner points
		for (i = 0; i < 8; i++){
			corner[0] = (i & 1) ? -renderLight->radius[0] : renderLight->radius[0];
			corner[1] = (i & 2) ? -renderLight->radius[1] : renderLight->radius[1];
			corner[2] = (i & 4) ? -renderLight->radius[2] : renderLight->radius[2];

			// Transform into world space
			R_LocalPointToWorld(corner, lightData->corners[i], renderLight->origin, renderLight->axis);
		}

		// Compute the bounding box
		BoundsFromPoints(lightData->bounds[0], lightData->bounds[1], lightData->corners);

		// Compute the frustum planes
		R_SetDynamicLightFrustum(renderLight, lightData);
	}
	else {
		// Compute the corner points
		ratio = renderLight->zFar / renderLight->zNear;

		VectorScale(renderLight->axis[0], renderLight->xMin, lVector);
		VectorScale(renderLight->axis[0], renderLight->xMax, rVector);

		VectorScale(renderLight->axis[1], renderLight->yMin, dVector);
		VectorScale(renderLight->axis[1], renderLight->yMax, uVector);

		VectorScale(renderLight->axis[2], renderLight->zNear, nVector);
		VectorScale(renderLight->axis[2], renderLight->zFar, fVector);

		// TODO: corners

		// Compute the bounding box
		BoundsFromPoints(lightData->bounds[0], lightData->bounds[1], lightData->corners);

		// Compute the frustum planes
	}

	// Compute the light range
	maxDistance = 0.0f;

	if (lightData->type != RL_DIRECTIONAL){
		for (i = 0; i < 8; i++){
			distance = Distance(lightData->origin, lightData->corners[i]);

			if (distance > maxDistance)
				maxDistance = distance;
		}
	}
	else {
		for (i = 0; i < 8; i++){
			distance = DotProduct(lightData->direction, lightData->corners[i]) - DotProduct(lightData->direction, lightData->origin);

			if (distance > maxDistance)
				maxDistance = distance;
		}
	}

	lightData->lightRange = maxDistance + 8.0f;

	// Compute the fog plane

	// Compute the transformation matrices
	if (lightData->type != RL_PROJECTED){
		lightData->projectionMatrix[ 0] = 0.5f / renderLight->radius[0];
		lightData->projectionMatrix[ 1] = 0.0f;
		lightData->projectionMatrix[ 2] = 0.0f;
		lightData->projectionMatrix[ 3] = 0.0f;
		lightData->projectionMatrix[ 4] = 0.0f;
		lightData->projectionMatrix[ 5] = 0.5f / renderLight->radius[1];
		lightData->projectionMatrix[ 6] = 0.0f;
		lightData->projectionMatrix[ 7] = 0.0f;
		lightData->projectionMatrix[ 8] = 0.0f;
		lightData->projectionMatrix[ 9] = 0.0f;
		lightData->projectionMatrix[10] = 0.5f / renderLight->radius[2];
		lightData->projectionMatrix[11] = 0.0f;
		lightData->projectionMatrix[12] = 0.5f;
		lightData->projectionMatrix[13] = 0.5f;
		lightData->projectionMatrix[14] = 0.5f;
		lightData->projectionMatrix[15] = 1.0f;

		lightData->modelviewMatrix[ 0] = -renderLight->axis[0][0];
		lightData->modelviewMatrix[ 1] = renderLight->axis[1][0];
		lightData->modelviewMatrix[ 2] = -renderLight->axis[2][0];
		lightData->modelviewMatrix[ 3] = 0.0f;
		lightData->modelviewMatrix[ 4] = -renderLight->axis[0][1];
		lightData->modelviewMatrix[ 5] = renderLight->axis[1][1];
		lightData->modelviewMatrix[ 6] = -renderLight->axis[2][1];
		lightData->modelviewMatrix[ 7] = 0.0f;
		lightData->modelviewMatrix[ 8] = -renderLight->axis[0][2];
		lightData->modelviewMatrix[ 9] = renderLight->axis[1][2];
		lightData->modelviewMatrix[10] = -renderLight->axis[2][2];
		lightData->modelviewMatrix[11] = 0.0f;
		lightData->modelviewMatrix[12] = DotProduct(renderLight->origin, renderLight->axis[0]);
		lightData->modelviewMatrix[13] = -DotProduct(renderLight->origin, renderLight->axis[1]);
		lightData->modelviewMatrix[14] = DotProduct(renderLight->origin, renderLight->axis[2]);
		lightData->modelviewMatrix[15] = 1.0f;

		Matrix4_Multiply(lightData->projectionMatrix, lightData->modelviewMatrix, lightData->modelviewProjectionMatrix);
	}
	else {
		lightData->projectionMatrix[ 0] = 0.5f * (2.0f * renderLight->zNear / (renderLight->xMax - renderLight->xMin));
		lightData->projectionMatrix[ 1] = 0.0f;
		lightData->projectionMatrix[ 2] = 0.0f;
		lightData->projectionMatrix[ 3] = 0.0f;
		lightData->projectionMatrix[ 4] = 0.0f;
		lightData->projectionMatrix[ 5] = -0.5f * (2.0f * renderLight->zNear / (renderLight->yMax - renderLight->yMin));
		lightData->projectionMatrix[ 6] = 0.0f;
		lightData->projectionMatrix[ 7] = 0.0f;
		lightData->projectionMatrix[ 8] = 0.5f * ((renderLight->xMax + renderLight->xMin) / (renderLight->xMax - renderLight->xMin)) - 0.5f;
		lightData->projectionMatrix[ 9] = -0.5f * ((renderLight->yMax + renderLight->yMin) / (renderLight->yMax - renderLight->yMin)) - 0.5f;
		lightData->projectionMatrix[10] = 1.0f / (renderLight->zFar - renderLight->zNear);
		lightData->projectionMatrix[11] = -1.0f;
		lightData->projectionMatrix[12] = 0.0f;
		lightData->projectionMatrix[13] = 0.0f;
		lightData->projectionMatrix[14] = -0.5f * ((renderLight->zFar + renderLight->zNear) / (renderLight->zFar - renderLight->zNear)) + 0.5f;
		lightData->projectionMatrix[15] = 0.0f;

		lightData->modelviewMatrix[ 0] = renderLight->axis[0][0];
		lightData->modelviewMatrix[ 1] = renderLight->axis[1][0];
		lightData->modelviewMatrix[ 2] = -renderLight->axis[2][0];
		lightData->modelviewMatrix[ 3] = 0.0f;
		lightData->modelviewMatrix[ 4] = renderLight->axis[0][1];
		lightData->modelviewMatrix[ 5] = renderLight->axis[1][1];
		lightData->modelviewMatrix[ 6] = -renderLight->axis[2][1];
		lightData->modelviewMatrix[ 7] = 0.0f;
		lightData->modelviewMatrix[ 8] = renderLight->axis[0][2];
		lightData->modelviewMatrix[ 9] = renderLight->axis[1][2];
		lightData->modelviewMatrix[10] = -renderLight->axis[2][2];
		lightData->modelviewMatrix[11] = 0.0f;
		lightData->modelviewMatrix[12] = -DotProduct(renderLight->origin, renderLight->axis[0]);
		lightData->modelviewMatrix[13] = -DotProduct(renderLight->origin, renderLight->axis[1]);
		lightData->modelviewMatrix[14] = DotProduct(renderLight->origin, renderLight->axis[2]);
		lightData->modelviewMatrix[15] = 1.0f;

		Matrix4_Multiply(lightData->projectionMatrix, lightData->modelviewMatrix, lightData->modelviewProjectionMatrix);
	}

	// Set no shadows
	lightData->noShadows = renderLight->noShadows;

	// Set style
	lightData->style = renderLight->style;

	// Set up the PVS and area
	if (lightData->type == RL_DIRECTIONAL || lightData->material->lightType != LT_GENERIC || !inWorld){
		lightData->pvs = NULL;
		lightData->area = -1;
	}
	else {
		leaf = R_PointInLeaf(lightData->origin);

		if (leaf->cluster == -1 || leaf->area == -1){
			lightData->pvs = NULL;
			lightData->area = -1;
		}
		else {
			R_ClusterPVS(leaf->cluster, pvs);

			lightData->pvs = pvs;
			lightData->area = leaf->area;
		}
	}
}

/*
 ==================
 
 ==================
*/
static int R_CullLightBounds (lightData_t *lightData, int planeBits){

}

/*
 ==================
 
 ==================
*/
static int R_CullLightVolume (lightData_t *lightData, int planeBits){

}

/*
 ==================
 
 ==================
*/
static bool R_ViewInLightVolume (){

	return true;
}

/*
 ==================

 TODO: for some reason the coords are scaled down to a 640x480 viewport which is totally wrong
 ==================
*/
static void R_SetupScissor (light_t *light){

	r_skipLightScissors->integerValue = 1;

	// If scissor testing is disabled
	if (r_skipLightScissors->integerValue){
		light->scissor = rg.viewParms.scissor;
		return;
	}
}

/*
 ==================
 
 ==================
*/
static void R_SetupDepthBounds (light_t *light){

}

/*
 ==================
 R_SetupNearClipVolume
 ==================
*/
static void R_SetupNearClipVolume (light_t *light){

	mat4_t	invMatrix;
	vec3_t	eyeCorners[4], worldCorners[4];
	int		side;
	float	x, y;
	int		i;

	// See which side of the near plane the light is on
	side = PointOnPlaneSide(light->data.origin, ON_EPSILON, &rg.viewParms.frustum[FRUSTUM_NEAR]);

	if (side == PLANESIDE_ON){
		light->ncv.degenerate = true;
		return;
	}

	light->ncv.degenerate = false;

	// Compute the eye space corners of the viewport
	x = r_zNear->floatValue * Tan(rg.renderView.fovX * M_PI / 360.0f);
	y = r_zNear->floatValue * Tan(rg.renderView.fovY * M_PI / 360.0f);

	VectorSet(eyeCorners[0], x, y, -r_zNear->floatValue);
	VectorSet(eyeCorners[1], -x, y, -r_zNear->floatValue);
	VectorSet(eyeCorners[2], -x, -y, -r_zNear->floatValue);
	VectorSet(eyeCorners[3], x, -y, -r_zNear->floatValue);

	// Transform the corners to world space
	Matrix4_AffineInverse(rg.viewParms.modelviewMatrix, invMatrix);

	Matrix4_TransformVector(invMatrix, eyeCorners[0], worldCorners[0]);
	Matrix4_TransformVector(invMatrix, eyeCorners[1], worldCorners[1]);
	Matrix4_TransformVector(invMatrix, eyeCorners[2], worldCorners[2]);
	Matrix4_TransformVector(invMatrix, eyeCorners[3], worldCorners[3]);

	// Set up the frustum planes
	if (side == PLANESIDE_FRONT){
		PlaneFromPoints(&light->ncv.frustum[FRUSTUM_LEFT], light->data.origin, worldCorners[1], worldCorners[0]);
		PlaneFromPoints(&light->ncv.frustum[FRUSTUM_RIGHT], light->data.origin, worldCorners[2], worldCorners[1]);
		PlaneFromPoints(&light->ncv.frustum[FRUSTUM_TOP], light->data.origin, worldCorners[3], worldCorners[2]);
		PlaneFromPoints(&light->ncv.frustum[FRUSTUM_BOTTOM], light->data.origin, worldCorners[0], worldCorners[3]);

		VectorCopy(rg.viewParms.frustum[FRUSTUM_NEAR].normal, light->ncv.frustum[FRUSTUM_NEAR].normal);
		light->ncv.frustum[FRUSTUM_NEAR].dist = rg.viewParms.frustum[FRUSTUM_NEAR].dist;
	}
	else {
		PlaneFromPoints(&light->ncv.frustum[FRUSTUM_LEFT], light->data.origin, worldCorners[0], worldCorners[1]);
		PlaneFromPoints(&light->ncv.frustum[FRUSTUM_RIGHT], light->data.origin, worldCorners[1], worldCorners[2]);
		PlaneFromPoints(&light->ncv.frustum[FRUSTUM_TOP], light->data.origin, worldCorners[2], worldCorners[3]);
		PlaneFromPoints(&light->ncv.frustum[FRUSTUM_BOTTOM], light->data.origin, worldCorners[3], worldCorners[0]);

		VectorNegate(rg.viewParms.frustum[FRUSTUM_NEAR].normal, light->ncv.frustum[FRUSTUM_NEAR].normal);
		light->ncv.frustum[FRUSTUM_NEAR].dist = -rg.viewParms.frustum[FRUSTUM_NEAR].dist;
	}

	for (i = 0; i < NUM_FRUSTUM_PLANES; i++){
		light->ncv.frustum[i].type = PLANE_NON_AXIAL;
		SetPlaneSignbits(&light->ncv.frustum[i]);
	}
}

/*
 ==================
 
 TODO: fog plane
 ==================
*/
static void R_AddLight (lightData_t *lightData, material_t *material, bool viewInLight){

	light_t	*light;
	int		index;

	if (material->lightType == LT_GENERIC || material->lightType == LT_AMBIENT)
		index = 0;
	else if (material->lightType == LT_BLEND)
		index = 1;
	else {
		if (viewInLight)
			index = 3;
		else
			index = 2;
	}

	// Add a new light
	if (rg.numLights[index] == rg.maxLights[index]){
		Com_DPrintf(S_COLOR_YELLOW "R_AddLight: overflow\n");
		return;
	}

	light = &rg.lights[index][rg.numLights[index]];

	// Fill it in
	light->data = *lightData;
	light->material = material;
	light->materialParms[MATERIALPARM_RED] = lightData->materialParms[MATERIALPARM_RED] * rg.lightStyles[lightData->style].rgb[0];
	light->materialParms[MATERIALPARM_GREEN] = lightData->materialParms[MATERIALPARM_GREEN] * rg.lightStyles[lightData->style].rgb[1];
	light->materialParms[MATERIALPARM_BLUE] = lightData->materialParms[MATERIALPARM_BLUE] * rg.lightStyles[lightData->style].rgb[2];
	light->materialParms[MATERIALPARM_ALPHA] = lightData->materialParms[MATERIALPARM_ALPHA];
	light->materialParms[MATERIALPARM_TIMEOFFSET] = lightData->materialParms[MATERIALPARM_TIMEOFFSET];
	light->materialParms[MATERIALPARM_DIVERSITY] = lightData->materialParms[MATERIALPARM_DIVERSITY];
	light->materialParms[MATERIALPARM_MISC] = lightData->materialParms[MATERIALPARM_MISC];
	light->materialParms[MATERIALPARM_MODE] = lightData->materialParms[MATERIALPARM_MODE];

	// Set up the scissor
	R_SetupScissor(light);

	// Set up the depth bounds
	R_SetupDepthBounds(light);

	// Set up the near clip volume
	R_SetupNearClipVolume(light);

	// Determine fog plane visibility

	// Generate light interaction meshes
	R_GenerateLightMeshes(light);

	if (light->data.precached)
		rg.pc.staticLights++;
	else
		rg.pc.dynamicLights++;

	rg.numLights[index]++;
}

/*
 ==================
 
 TODO: culling
 ==================
*/
static void R_AddLights (){

	renderLight_t	*renderLight;
	lightData_t		*lightData;
	int				cullBits;
	bool			viewInLight;
	int				i;

	// Add static lights
	for (i = 0, lightData = rg.staticLights; i < rg.numStaticLights; i++, lightData++){
		// Development tool
		if (r_singleLight->integerValue != -1){
			if (r_singleLight->integerValue != lightData->parms.index)
				continue;
		}

		// Check the detail level
		if (lightData->parms.detailLevel > r_lightDetailLevel->integerValue)
			continue;

		// Set up the light data
		R_SetupStaticLightData(lightData, rg.viewParms.primaryView);

		// Check for door connected areas if rendering a primary view
		if (!r_skipVisibility->integerValue && rg.viewParms.primaryView){
			if (lightData->area != -1){
				if (!(rg.renderView.areaBits[lightData->area >> 3] & (1 << (lightData->area & 7))))
					continue;
			}
		}

		// Cull
		if (!r_skipLightCulling->integerValue){
			cullBits = R_CullLightBounds(lightData, rg.viewParms.planeBits);

			if (cullBits == CULL_OUT)
				continue;

			if (cullBits != CULL_IN){
				if (R_CullLightVolume(lightData, cullBits) == CULL_OUT)
					continue;
			}
		}

		// Determine if the view is inside the light volume
		viewInLight = R_ViewInLightVolume();

		// Add the light
		R_AddLight(lightData, lightData->material, viewInLight);
	}

	// Add dynamic lights
	for (i = 0, renderLight = rg.viewParms.renderLights; i < rg.viewParms.numRenderLights; i++, renderLight++){
		lightData_t newLightData;

		lightData = &newLightData;

		// Development tool
		if (r_singleLight->integerValue != -1){
			if (r_singleLight->integerValue != renderLight->index)
				continue;
		}

		// Check for view suppression
		if (!r_skipSuppress->integerValue){
			if (!(renderLight->allowInView & rg.viewParms.viewType))
				continue;
		}

		// Check the detail level
		if (renderLight->detailLevel > r_lightDetailLevel->integerValue)
			continue;

		// Set up the light data
		R_SetupDynamicLightData(renderLight, lightData, rg.viewParms.primaryView);

		// Check for door connected areas if rendering a primary view
		if (!r_skipVisibility->integerValue && rg.viewParms.primaryView){
			if (lightData->area != -1){
				if (!(rg.renderView.areaBits[lightData->area >> 3] & (1 << (lightData->area & 7))))
					continue;
			}
		}

		// Cull
		if (!r_skipLightCulling->integerValue){

		}

		// Determine if the view is inside the light volume
		viewInLight = R_ViewInLightVolume();

		// Add the light
		R_AddLight(lightData, lightData->material, viewInLight);
	}
}


// ============================================================================


/*
 ==================
 R_AllocLights
 ==================
*/
void R_AllocLights (){

	rg.maxLights[0] = MAX_LIGHTS;
	rg.maxLights[1] = MAX_LIGHTS >> 4;
	rg.maxLights[2] = MAX_LIGHTS >> 4;
	rg.maxLights[3] = MAX_LIGHTS >> 4;

	rg.lights[0] = (light_t *)Mem_Alloc(rg.maxLights[0] * sizeof(light_t), TAG_RENDERER);
	rg.lights[1] = (light_t *)Mem_Alloc(rg.maxLights[1] * sizeof(light_t), TAG_RENDERER);
	rg.lights[2] = (light_t *)Mem_Alloc(rg.maxLights[2] * sizeof(light_t), TAG_RENDERER);
	rg.lights[3] = (light_t *)Mem_Alloc(rg.maxLights[3] * sizeof(light_t), TAG_RENDERER);
}

/*
 ==================
 R_GenerateLights
 ==================
*/
void R_GenerateLights (){

	// Add the draw lights
	R_AddLights();

	// Set up the draw lights
	rg.viewParms.numLights[0] = rg.numLights[0] - rg.firstLight[0];
	rg.viewParms.numLights[1] = rg.numLights[1] - rg.firstLight[1];
	rg.viewParms.numLights[2] = rg.numLights[2] - rg.firstLight[2];
	rg.viewParms.numLights[3] = rg.numLights[3] - rg.firstLight[3];

	rg.viewParms.lights[0] = &rg.lights[0][rg.firstLight[0]];
	rg.viewParms.lights[1] = &rg.lights[1][rg.firstLight[1]];
	rg.viewParms.lights[2] = &rg.lights[2][rg.firstLight[2]];
	rg.viewParms.lights[3] = &rg.lights[3][rg.firstLight[3]];

	rg.pc.lights += rg.viewParms.numLights[0] + rg.viewParms.numLights[1] + rg.viewParms.numLights[2] + rg.viewParms.numLights[3];

	// The next view rendered in this frame will tack on after this one
	rg.firstLight[0] = rg.numLights[0];
	rg.firstLight[1] = rg.numLights[1];
	rg.firstLight[2] = rg.numLights[2];
	rg.firstLight[3] = rg.numLights[3];
}

/*
 ==================
 R_ClearLights
 ==================
*/
void R_ClearLights (){

	rg.numLights[0] = rg.firstLight[0] = 0;
	rg.numLights[1] = rg.firstLight[1] = 0;
	rg.numLights[2] = rg.firstLight[2] = 0;
	rg.numLights[3] = rg.firstLight[3] = 0;
}