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
// r_subview.c - Subview rendering
//


#include "r_local.h"


/*
 ==============================================================================

 SUBVIEW SETUP

 ==============================================================================
*/


/*
 ==================
 
 ==================
*/
static void R_PlaneForSurface (){

}

/*
 ==================
 
 ==================
*/
static bool R_PreciseCullSurface (){

	return true;
}

/*
 ==================
 
 ==================
*/
static bool R_MirrorViewForSurface (meshType_t type,  meshData_t *data, renderEntity_t *entity, material_t *material){

	return true;
}

/*
 ==================
 
 ==================
*/
static bool R_RemoteViewForSurface (meshType_t type,  meshData_t *data, renderEntity_t *entity, material_t *material){

	return true;
}

/*
 ==================
 R_AddSubviewSurface
 ==================
*/
bool R_AddSubviewSurface (meshType_t type,  meshData_t *data, renderEntity_t *entity, material_t *material){

	if (r_skipSubviews->integerValue)
		return true;

	if (rg.viewParms.viewType != VIEW_MAIN)
		return false;		// Avoid recursion

	switch (material->subviewType){
	case ST_MIRROR:
		if (!R_MirrorViewForSurface(type, data, entity, material))
			return false;

		break;
	case ST_REMOTE:
		if (!R_RemoteViewForSurface(type, data, entity, material))
			return false;

		break;
	default:
		Com_Error(ERR_DROP, "R_AddSubviewSurface: bad subview type (%i)", material->subviewType);
	}

	return true;
}


/*
 ==============================================================================

 SUBVIEW RENDERING

 ==============================================================================
*/


/*
 ==================
 R_RenderSubview
 ==================
*/
bool R_RenderSubview (){

	material_t		*material;
	renderView_t	oldRenderView;
	viewParms_t		oldViewParms;

	if (rg.viewParms.viewType != VIEW_MAIN)
		return false;		// Avoid recursion

	if (rg.subviewParms.viewType == VIEW_NONE)
		return false;		// No subview available

	material = rg.subviewParms.material;

	// Backup main view
	Mem_Copy(&oldRenderView, &rg.renderView, sizeof(renderView_t));
	Mem_Copy(&oldViewParms, &rg.viewParms, sizeof(viewParms_t));

	// Set up the view
	VectorCopy(rg.subviewParms.origin, rg.renderView.origin);
	Matrix3_Copy(rg.subviewParms.axis, rg.renderView.axis);

	rg.renderView.fovX = rg.subviewParms.fovX;
	rg.renderView.fovY = rg.subviewParms.fovY;
	rg.renderView.fovScale = rg.subviewParms.fovScale;

	// Render the subview to a texture
	R_CropRenderSize(material->subviewWidth, material->subviewHeight, false);
	R_RenderView(rg.viewParms.primaryView, rg.subviewParms.viewType);
	R_CaptureRenderToTexture(material->subviewTexture->name);
	R_UnCropRenderSize();

	// Restore main view
	Mem_Copy(&rg.renderView, &oldRenderView, sizeof(renderView_t));
	Mem_Copy(&rg.viewParms, &oldViewParms, sizeof(viewParms_t));

	// Clear subview
	Mem_Fill(&rg.subviewParms, 0, sizeof(subviewParms_t));

	return true;
}