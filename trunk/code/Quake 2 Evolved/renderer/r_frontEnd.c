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
// r_frontEnd.c - Front-end renderer
//


#include "r_local.h"


/*
 ==============================================================================

 VIEW FRUSTUM CULLING

 ==============================================================================
*/


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

	for (i = 0, plane = rg.viewParms.frustum; i < 4; i++, plane++){
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

	for (i = 0, plane = rg.viewParms.frustum; i < 4; i++, plane++){
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

 COORDINATE SCALING AND ASPECT RATIO CORRECTION

 ==============================================================================
*/


/*
 ==================
 R_AdjustHorzCoords
 ==================
*/
void R_AdjustHorzCoords (horzAdjust_t adjust, float percent, float xIn, float wIn, float *xOut, float *wOut){

	renderCrop_t	*renderCrop;

	if (adjust == H_NONE){
		*xOut = xIn * r_screenFraction->floatValue;
		*wOut = wIn * r_screenFraction->floatValue;

		return;
	}

	renderCrop = &rg.renderCrops[rg.currentRenderCrop];

	if (renderCrop->aspectRatio != ASPECT_WIDE){
		*xOut = xIn * renderCrop->xScale;
		*wOut = wIn * renderCrop->xScale;

		return;
	}

	switch (adjust){
	case H_SCALE:
		*xOut = xIn * renderCrop->xScale;
		*wOut = wIn * renderCrop->xScale;

		break;
	case H_ALIGN_LEFT:
		*xOut = xIn * renderCrop->aspectScale + renderCrop->aspectBias * (1.0f - percent);
		*wOut = wIn * renderCrop->aspectScale;

		break;
	case H_ALIGN_CENTER:
		*xOut = xIn * renderCrop->aspectScale + renderCrop->aspectBias;
		*wOut = wIn * renderCrop->aspectScale;

		break;
	case H_ALIGN_RIGHT:
		*xOut = xIn * renderCrop->aspectScale + renderCrop->aspectBias * (1.0f + percent);
		*wOut = wIn * renderCrop->aspectScale;

		break;
	case H_STRETCH_LEFT:
		*xOut = xIn * renderCrop->aspectScale + renderCrop->aspectBias * (1.0f - percent);
		*wOut = wIn * renderCrop->aspectScale + renderCrop->aspectBias * percent;

		break;
	case H_STRETCH_WIDTH:
		*xOut = xIn * renderCrop->aspectScale + renderCrop->aspectBias * (1.0f - percent);
		*wOut = wIn * renderCrop->aspectScale + renderCrop->aspectBias * percent * 2.0f;

		break;
	case H_STRETCH_RIGHT:
		*xOut = xIn * renderCrop->aspectScale + renderCrop->aspectBias;
		*wOut = wIn * renderCrop->aspectScale + renderCrop->aspectBias * percent;

		break;
	}
}

/*
 ==================
 R_AdjustVertCoords
 ==================
*/
void R_AdjustVertCoords (vertAdjust_t adjust, float percent, float yIn, float hIn, float *yOut, float *hOut){

	renderCrop_t	*renderCrop;

	if (adjust == V_NONE){
		*yOut = yIn * r_screenFraction->floatValue;
		*hOut = hIn * r_screenFraction->floatValue;

		return;
	}

	renderCrop = &rg.renderCrops[rg.currentRenderCrop];

	if (renderCrop->aspectRatio != ASPECT_HIGH){
		*yOut = yIn * renderCrop->yScale;
		*hOut = hIn * renderCrop->yScale;

		return;
	}

	switch (adjust){
	case V_SCALE:
		*yOut = yIn * renderCrop->yScale;
		*hOut = hIn * renderCrop->yScale;

		break;
	case V_ALIGN_TOP:
		*yOut = yIn * renderCrop->aspectScale + renderCrop->aspectBias * (1.0f - percent);
		*hOut = hIn * renderCrop->aspectScale;

		break;
	case V_ALIGN_CENTER:
		*yOut = yIn * renderCrop->aspectScale + renderCrop->aspectBias;
		*hOut = hIn * renderCrop->aspectScale;

		break;
	case V_ALIGN_BOTTOM:
		*yOut = yIn * renderCrop->aspectScale + renderCrop->aspectBias * (1.0f + percent);
		*hOut = hIn * renderCrop->aspectScale;

		break;
	case V_STRETCH_TOP:
		*yOut = yIn * renderCrop->aspectScale + renderCrop->aspectBias * (1.0f - percent);
		*hOut = hIn * renderCrop->aspectScale + renderCrop->aspectBias * percent;

		break;
	case V_STRETCH_HEIGHT:
		*yOut = yIn * renderCrop->aspectScale + renderCrop->aspectBias * (1.0f - percent);
		*hOut = hIn * renderCrop->aspectScale + renderCrop->aspectBias * percent * 2.0f;

		break;
	case V_STRETCH_BOTTOM:
		*yOut = yIn * renderCrop->aspectScale + renderCrop->aspectBias;
		*hOut = hIn * renderCrop->aspectScale + renderCrop->aspectBias * percent;

		break;
	}
}

/*
 ==================
 R_AdjustHorzCoordsInt
 ==================
*/
void R_AdjustHorzCoordsInt (horzAdjust_t adjust, float percent, int xIn, int wIn, int *xOut, int *wOut){

	renderCrop_t	*renderCrop;

	if (adjust == H_NONE){
		*xOut = FloatToInt(xIn * r_screenFraction->floatValue);
		*wOut = FloatToInt(wIn * r_screenFraction->floatValue);

		return;
	}

	renderCrop = &rg.renderCrops[rg.currentRenderCrop];

	if (renderCrop->aspectRatio != ASPECT_WIDE){
		*xOut = FloatToInt(xIn * renderCrop->xScale);
		*wOut = FloatToInt(wIn * renderCrop->xScale);

		return;
	}

	switch (adjust){
	case H_SCALE:
		*xOut = FloatToInt(xIn * renderCrop->xScale);
		*wOut = FloatToInt(wIn * renderCrop->xScale);

		break;
	case H_ALIGN_LEFT:
		*xOut = FloatToInt(xIn * renderCrop->aspectScale + renderCrop->aspectBias * (1.0f - percent));
		*wOut = FloatToInt(wIn * renderCrop->aspectScale);

		break;
	case H_ALIGN_CENTER:
		*xOut = FloatToInt(xIn * renderCrop->aspectScale + renderCrop->aspectBias);
		*wOut = FloatToInt(wIn * renderCrop->aspectScale);

		break;
	case H_ALIGN_RIGHT:
		*xOut = FloatToInt(xIn * renderCrop->aspectScale + renderCrop->aspectBias * (1.0f + percent));
		*wOut = FloatToInt(wIn * renderCrop->aspectScale);

		break;
	case H_STRETCH_LEFT:
		*xOut = FloatToInt(xIn * renderCrop->aspectScale + renderCrop->aspectBias * (1.0f - percent));
		*wOut = FloatToInt(wIn * renderCrop->aspectScale + renderCrop->aspectBias * percent);

		break;
	case H_STRETCH_WIDTH:
		*xOut = FloatToInt(xIn * renderCrop->aspectScale + renderCrop->aspectBias * (1.0f - percent));
		*wOut = FloatToInt(wIn * renderCrop->aspectScale + renderCrop->aspectBias * percent * 2.0f);

		break;
	case H_STRETCH_RIGHT:
		*xOut = FloatToInt(xIn * renderCrop->aspectScale + renderCrop->aspectBias);
		*wOut = FloatToInt(wIn * renderCrop->aspectScale + renderCrop->aspectBias * percent);

		break;
	}
}

/*
 ==================
 R_AdjustVertCoordsInt
 ==================
*/
void R_AdjustVertCoordsInt (vertAdjust_t adjust, float percent, int yIn, int hIn, int *yOut, int *hOut){

	renderCrop_t	*renderCrop;

	if (adjust == V_NONE){
		*yOut = FloatToInt(yIn * r_screenFraction->floatValue);
		*hOut = FloatToInt(hIn * r_screenFraction->floatValue);

		return;
	}

	renderCrop = &rg.renderCrops[rg.currentRenderCrop];

	if (renderCrop->aspectRatio != ASPECT_HIGH){
		*yOut = FloatToInt(yIn * renderCrop->yScale);
		*hOut = FloatToInt(hIn * renderCrop->yScale);

		return;
	}

	switch (adjust){
	case V_SCALE:
		*yOut = FloatToInt(yIn * renderCrop->yScale);
		*hOut = FloatToInt(hIn * renderCrop->yScale);

		break;
	case V_ALIGN_TOP:
		*yOut = FloatToInt(yIn * renderCrop->aspectScale + renderCrop->aspectBias * (1.0f - percent));
		*hOut = FloatToInt(hIn * renderCrop->aspectScale);

		break;
	case V_ALIGN_CENTER:
		*yOut = FloatToInt(yIn * renderCrop->aspectScale + renderCrop->aspectBias);
		*hOut = FloatToInt(hIn * renderCrop->aspectScale);

		break;
	case V_ALIGN_BOTTOM:
		*yOut = FloatToInt(yIn * renderCrop->aspectScale + renderCrop->aspectBias * (1.0f + percent));
		*hOut = FloatToInt(hIn * renderCrop->aspectScale);

		break;
	case V_STRETCH_TOP:
		*yOut = FloatToInt(yIn * renderCrop->aspectScale + renderCrop->aspectBias * (1.0f - percent));
		*hOut = FloatToInt(hIn * renderCrop->aspectScale + renderCrop->aspectBias * percent);

		break;
	case V_STRETCH_HEIGHT:
		*yOut = FloatToInt(yIn * renderCrop->aspectScale + renderCrop->aspectBias * (1.0f - percent));
		*hOut = FloatToInt(hIn * renderCrop->aspectScale + renderCrop->aspectBias * percent * 2.0f);

		break;
	case V_STRETCH_BOTTOM:
		*yOut = FloatToInt(yIn * renderCrop->aspectScale + renderCrop->aspectBias);
		*hOut = FloatToInt(hIn * renderCrop->aspectScale + renderCrop->aspectBias * percent);

		break;
	}
}


/*
 ==============================================================================

 VIEW RENDERING

 ==============================================================================
*/


/*
 ==================

 ==================
*/
static void R_SetupViewportAndScissor (){

	renderCrop_t	*renderCrop;
	int				x, y, w, h;

	if (rg.envShotRendering){
		rg.viewParms.viewport.x = 0;
		rg.viewParms.viewport.y = 0;
		rg.viewParms.viewport.width = rg.envShotSize;
		rg.viewParms.viewport.height = rg.envShotSize;

		rg.viewParms.scissor.x = 0;
		rg.viewParms.scissor.y = 0;
		rg.viewParms.scissor.width = rg.envShotSize;
		rg.viewParms.scissor.height = rg.envShotSize;

		return;
	}

	renderCrop = &rg.renderCrops[rg.currentRenderCrop];

	// Scale the coordinates and correct the aspect ratio if needed
	R_AdjustHorzCoordsInt(rg.renderView.horzAdjust, rg.renderView.horzPercent, rg.renderView.x, rg.renderView.width, &x, &w);
	R_AdjustVertCoordsInt(rg.renderView.vertAdjust, rg.renderView.vertPercent, rg.renderView.y, rg.renderView.height, &y, &h);

	// Convert origin from top-left to bottom-left
	y = renderCrop->height - (y + h);

	// Set up the viewport
	rg.viewParms.viewport.x = x;
	rg.viewParms.viewport.y = y;
	rg.viewParms.viewport.width = x + w;
	rg.viewParms.viewport.height = y + h;

	// TODO: clamp

	// Set up the scissor
	if (r_skipScissors->integerValue){
		rg.viewParms.scissor.x = rg.viewParms.viewport.x;
		rg.viewParms.scissor.y = rg.viewParms.viewport.y;
		rg.viewParms.scissor.width = rg.viewParms.viewport.width;
		rg.viewParms.scissor.height = rg.viewParms.viewport.height;

		return;
	}

	if (rg.viewParms.viewType == VIEW_MAIN || rg.viewParms.viewType == VIEW_REMOTE){
		rg.viewParms.scissor.x = rg.viewParms.viewport.x;
		rg.viewParms.scissor.y = rg.viewParms.viewport.y;
		rg.viewParms.scissor.width = rg.viewParms.viewport.width;
		rg.viewParms.scissor.height = rg.viewParms.viewport.height;
	}
	else {

	}
}

/*
 ==================
 R_SetupFOV
 ==================
*/
static void R_SetupFOV (){

	float	f;

	if (rg.envShotRendering){
		rg.viewParms.fovX = 90.0f;
		rg.viewParms.fovY = 90.0f;
		rg.viewParms.fovScale = 1.0f;

		return;
	}

	rg.viewParms.fovX = rg.renderView.fovX;
	rg.viewParms.fovY = rg.renderView.fovY;
	rg.viewParms.fovScale = rg.renderView.fovScale;

	// Correct the aspect ratio if needed
	if (rg.viewParms.viewType != VIEW_MAIN)
		return;

	if (rg.viewParms.viewport.width * rg.renderView.height > rg.viewParms.viewport.height * rg.renderView.width){
		f = rg.viewParms.viewport.height / Tan(rg.viewParms.fovY / 360.0f * M_PI);
		rg.viewParms.fovX = ATan(rg.viewParms.viewport.width, f) * 360.0f / M_PI;
	}
	else if (rg.viewParms.viewport.width * rg.renderView.height < rg.viewParms.viewport.height * rg.renderView.width){
		f = rg.viewParms.viewport.width / Tan(rg.viewParms.fovX / 360.0f * M_PI);
		rg.viewParms.fovY = ATan(rg.viewParms.viewport.height, f) * 360.0f / M_PI;
	}
}

/*
 ==================

 ==================
*/
static void R_SetupFrustum (){

	int		i;

	// Set up the left/right clip planes
	RotatePointAroundVector(rg.viewParms.frustum[FRUSTUM_LEFT].normal, rg.renderView.axis[2], rg.renderView.axis[0], -(90.0f - rg.viewParms.fovX / 2.0f));
	RotatePointAroundVector(rg.viewParms.frustum[FRUSTUM_RIGHT].normal, rg.renderView.axis[2], rg.renderView.axis[0], 90.0f - rg.viewParms.fovX / 2.0f);
	
	// Set up the top/bottom clip planes
	RotatePointAroundVector(rg.viewParms.frustum[FRUSTUM_TOP].normal, rg.renderView.axis[1], rg.renderView.axis[0], 90.0f - rg.viewParms.fovY / 2.0f);
	RotatePointAroundVector(rg.viewParms.frustum[FRUSTUM_BOTTOM].normal, rg.renderView.axis[1], rg.renderView.axis[0], -(90.0f - rg.viewParms.fovY / 2.0f));

	for (i = 0; i < 4; i++){
		rg.viewParms.frustum[i].type = PLANE_NON_AXIAL;
		rg.viewParms.frustum[i].dist = DotProduct(rg.renderView.origin, rg.viewParms.frustum[i].normal);
		SetPlaneSignbits(&rg.viewParms.frustum[i]);
	}
}

/*
 ==================
 R_SetupMatrices
 ==================
*/
static void R_SetupMatrices (){

	float	xMax, xMin, yMax, yMin;
	float	xDiv, yDiv, zDiv;

	// Compute the projection matrix
	xMax = r_zNear->floatValue * tan(rg.viewParms.fovX * M_PI / 360.0);
	xMin = -xMax;

	yMax = r_zNear->floatValue * tan(rg.viewParms.fovY * M_PI / 360.0);
	yMin = -yMax;

	xDiv = 1.0 / (xMax - xMin);
	yDiv = 1.0 / (yMax - yMin);
	zDiv = 1.0 / (r_zFar->floatValue - r_zNear->floatValue);

	rg.viewParms.projectionMatrix[ 0] = 2.0f * r_zNear->floatValue * xDiv;
	rg.viewParms.projectionMatrix[ 1] = 0.0f;
	rg.viewParms.projectionMatrix[ 2] = 0.0f;
	rg.viewParms.projectionMatrix[ 3] = 0.0f;
	rg.viewParms.projectionMatrix[ 4] = 0.0f;
	rg.viewParms.projectionMatrix[ 5] = 2.0f * r_zNear->floatValue * yDiv;
	rg.viewParms.projectionMatrix[ 6] = 0.0f;
	rg.viewParms.projectionMatrix[ 7] = 0.0f;
	rg.viewParms.projectionMatrix[ 8] = (xMax + xMin) * xDiv;
	rg.viewParms.projectionMatrix[ 9] = (yMax + yMin) * yDiv;
	rg.viewParms.projectionMatrix[10] = -0.9f;
	rg.viewParms.projectionMatrix[11] = -1.0f;
	rg.viewParms.projectionMatrix[12] = 0.0f;
	rg.viewParms.projectionMatrix[13] = 0.0f;
	rg.viewParms.projectionMatrix[14] = -2.0f * r_zNear->floatValue * 0.9f;
	rg.viewParms.projectionMatrix[15] = 0.0f;

	if (r_zFar->floatValue > r_zNear->floatValue){
		rg.viewParms.projectionMatrix[10] = -(r_zNear->floatValue + r_zFar->floatValue) * zDiv;
		rg.viewParms.projectionMatrix[14] = -2.0f * r_zNear->floatValue * r_zFar->floatValue * zDiv;
	}

	// Compute the modelview matrix
	rg.viewParms.modelviewMatrix[ 0] = -rg.renderView.axis[1][0];
	rg.viewParms.modelviewMatrix[ 1] = rg.renderView.axis[2][0];
	rg.viewParms.modelviewMatrix[ 2] = -rg.renderView.axis[0][0];
	rg.viewParms.modelviewMatrix[ 3] = 0.0f;
	rg.viewParms.modelviewMatrix[ 4] = -rg.renderView.axis[1][1];
	rg.viewParms.modelviewMatrix[ 5] = rg.renderView.axis[2][1];
	rg.viewParms.modelviewMatrix[ 6] = -rg.renderView.axis[0][1];
	rg.viewParms.modelviewMatrix[ 7] = 0.0f;
	rg.viewParms.modelviewMatrix[ 8] = -rg.renderView.axis[1][2];
	rg.viewParms.modelviewMatrix[ 9] = rg.renderView.axis[2][2];
	rg.viewParms.modelviewMatrix[10] = -rg.renderView.axis[0][2];
	rg.viewParms.modelviewMatrix[11] = 0.0f;
	rg.viewParms.modelviewMatrix[12] = DotProduct(rg.renderView.origin, rg.renderView.axis[1]);
	rg.viewParms.modelviewMatrix[13] = -DotProduct(rg.renderView.origin, rg.renderView.axis[2]);
	rg.viewParms.modelviewMatrix[14] = DotProduct(rg.renderView.origin, rg.renderView.axis[0]);
	rg.viewParms.modelviewMatrix[15] = 1.0f;

	// Compute the modelview-projection matrix
	Matrix4_Multiply(rg.viewParms.projectionMatrix, rg.viewParms.modelviewMatrix, rg.viewParms.modelviewProjectionMatrix);

	// Compute sky box texture matrix
	rg.viewParms.skyBoxMatrix[ 0] = 1.0f;
	rg.viewParms.skyBoxMatrix[ 1] = 0.0f;
	rg.viewParms.skyBoxMatrix[ 2] = 0.0f;
	rg.viewParms.skyBoxMatrix[ 3] = -rg.renderView.origin[0];
	rg.viewParms.skyBoxMatrix[ 4] = 0.0f;
	rg.viewParms.skyBoxMatrix[ 5] = 1.0f;
	rg.viewParms.skyBoxMatrix[ 6] = 0.0f;
	rg.viewParms.skyBoxMatrix[ 7] = -rg.renderView.origin[1];
	rg.viewParms.skyBoxMatrix[ 8] = 0.0f;
	rg.viewParms.skyBoxMatrix[ 9] = 0.0f;
	rg.viewParms.skyBoxMatrix[10] = 1.0f;
	rg.viewParms.skyBoxMatrix[11] = -rg.renderView.origin[2];
	rg.viewParms.skyBoxMatrix[12] = 0.0f;
	rg.viewParms.skyBoxMatrix[13] = 0.0f;
	rg.viewParms.skyBoxMatrix[14] = 0.0f;
	rg.viewParms.skyBoxMatrix[15] = 1.0f;

	if (rg.worldModel && rg.worldModel->sky->rotate)
		Matrix4_Rotate(rg.viewParms.skyBoxMatrix, rg.renderView.time * rg.worldModel->sky->rotate, rg.worldModel->sky->axis[0], rg.worldModel->sky->axis[1], rg.worldModel->sky->axis[2]);
}

/*
 ==================
 
 ==================
*/
void R_RenderView (bool primaryView, int viewType){

	// Bump view count
	rg.viewCount++;

	// Set up the view
	rg.viewParms.primaryView = primaryView;
	rg.viewParms.viewType = viewType;

	// Set up the viewport and scissor
	R_SetupViewportAndScissor();

	// Set up FOV, frustum, and matrices
	R_SetupFOV();
	R_SetupFrustum();
	R_SetupMatrices();

	// Generate mesh and light lists
	R_GenerateMeshes();
	R_GenerateLights();

	// Render a subview if needed
	if (R_RenderSubview()){
		if (r_subviewOnly->integerValue)
			return;
	}

	// Add the render view command
	R_AddRenderViewCommand();
}