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


#include "r_local.h"


#define MAX_STATIC_LIGHTS			4096


/*
 ==============================================================================

 LIGHT PARSING

 ==============================================================================
*/


/*
 ==================
 
 ==================
*/
static bool R_ParseLight (script_t *script){

	token_t	token;
	int		index;

	// Parse the index number
	if (!PS_ReadInteger(script, &index)){
		Com_Printf(S_COLOR_YELLOW "WARNING: missing index number in light file\n");
		return false;
	}

	if (index < 0 || index >= MAX_STATIC_LIGHTS){
		Com_Printf(S_COLOR_YELLOW "WARNING: invalid value of %i for index number in light file\n", index);
		return false;
	}

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

		}
		else if (!Str_ICompare(token.string, "type")){

		}
		else if (!Str_ICompare(token.string, "origin")){

		}
		else if (!Str_ICompare(token.string, "center")){

		}
		else if (!Str_ICompare(token.string, "angles")){

		}
		else if (!Str_ICompare(token.string, "radius")){

		}
		else if (!Str_ICompare(token.string, "xMin")){

		}
		else if (!Str_ICompare(token.string, "xMax")){

		}
		else if (!Str_ICompare(token.string, "yMin")){

		}
		else if (!Str_ICompare(token.string, "yMax")){

		}
		else if (!Str_ICompare(token.string, "zNear")){

		}
		else if (!Str_ICompare(token.string, "zFar")){

		}
		else if (!Str_ICompare(token.string, "noShadows")){

		}
		else if (!Str_ICompare(token.string, "fogDistance")){

		}
		else if (!Str_ICompare(token.string, "fogHeight")){

		}
		else if (!Str_ICompare(token.string, "style")){

		}
		else if (!Str_ICompare(token.string, "detailLevel")){

		}
		else if (!Str_ICompare(token.string, "material")){

		}
		else if (!Str_ICompare(token.string, "color")){

		}
		else if (!Str_ICompare(token.string, "alpha")){

		}
		else {
			Com_Printf(S_COLOR_YELLOW "WARNING: unknown parameter '%s' in light file (index %i)\n", token.string, index);
			return false;
		}
	}

	// Fill it in

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
static void R_LightCullBounds (){

}

/*
 ==================
 
 ==================
*/
static void R_LightCullLocalBounds (){

}

/*
 ==================
 
 ==================
*/
static void R_LightCullSphere (){

}

/*
 ==================
 
 ==================
*/
static void R_LightCullLocalSphere (){

}


/*
 ==============================================================================

 LIGHT CULLING

 ==============================================================================
*/


/*
 ==================
 
 ==================
*/
static void R_CullLightBounds (){

}

/*
 ==================
 
 ==================
*/
static void R_CullLightVolume (){

}


// ============================================================================


/*
 ==================
 
 ==================
*/
static void R_SetStaticLightFrustum (){

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
static void R_SetupStaticLightData (){

}

/*
 ==================
 
 ==================
*/
static void R_SetupDynamicLightData (const renderLight_t *renderLight, lightData_t *lightData, bool inWorld){

	vec3_t	origin;
	vec3_t	direction, corner;
	float	distance, maxDistance;
	int		i;

	lightData->valid = true;
	lightData->precached = false;

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
	lightData->materialParms[0] = renderLight->materialParms[0];
	lightData->materialParms[1] = renderLight->materialParms[1];
	lightData->materialParms[2] = renderLight->materialParms[2];
	lightData->materialParms[3] = renderLight->materialParms[3];
	lightData->materialParms[4] = renderLight->materialParms[4];
	lightData->materialParms[5] = renderLight->materialParms[5];
	lightData->materialParms[6] = renderLight->materialParms[6];
	lightData->materialParms[7] = renderLight->materialParms[7];

	// Compute the origin, direction, and axis
	if (lightData->type == RL_POINT || renderLight->type == RL_CUBIC){
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

		// Compute the bounding box

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
	if (renderLight->type == RL_DIRECTIONAL || lightData->material->lightType != LT_GENERIC || !inWorld){

	}
	else {

	}
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
 
 ==================
*/
static void R_SetupScissor (light_t *light){

	light->scissor = rg.viewParms.scissor;
}

/*
 ==================
 
 ==================
*/
static void R_SetupDepthBounds (light_t *light){

}

/*
 ==================
 
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
	light->materialParms[0] = lightData->materialParms[0] * rg.lightStyles[lightData->style].rgb[0];
	light->materialParms[1] = lightData->materialParms[1] * rg.lightStyles[lightData->style].rgb[1];
	light->materialParms[2] = lightData->materialParms[2] * rg.lightStyles[lightData->style].rgb[2];
	light->materialParms[3] = lightData->materialParms[3];
	light->materialParms[4] = lightData->materialParms[4];
	light->materialParms[5] = lightData->materialParms[5];
	light->materialParms[6] = lightData->materialParms[6];
	light->materialParms[7] = lightData->materialParms[7];

	// Set up the scissor
	R_SetupScissor(light);

	// Set up the depth bounds
	R_SetupDepthBounds(light);

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
 
 ==================
*/
static void R_AddLights (){

	renderLight_t	*renderLight;
	lightData_t		*lightData;
	bool			viewInLight;
	int				i;

	// Add static lights

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

		// Check area connection if rendering a primary view
		if (!r_skipVisibility->integerValue && rg.viewParms.primaryView){

		}

		// Cull

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