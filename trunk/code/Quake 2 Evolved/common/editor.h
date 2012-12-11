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
// editor.h - Editors used by varius modules
//


#ifndef __EDITOR_H__
#define __EDITOR_H__


/*
 ==============================================================================

 INTEGRATED LIGHT EDITOR

 ==============================================================================
*/

#define MAX_LIGHT_NAME_LENGTH		64

typedef struct {
	char					name[MAX_LIGHT_NAME_LENGTH];

	int						type;

	// Transformation matrix
	vec3_t					origin;
	vec3_t					center;
	vec3_t					angles;

	// Bounding volume for point, cubic, and directional lights
	vec3_t					radius;

	// Frustum definition for projected lights
	float					xMin;
	float					xMax;

	float					yMin;
	float					yMax;

	float					zNear;
	float					zFar;

	// Shadowing parameters
	bool					noShadows;

	// Fogging parameters
	float					fogDistance;
	float					fogHeight;

	// Light attributes
	int						detailLevel;

	// Material
	char					material[MAX_PATH_LENGTH];
	float					materialParms[MAX_MATERIAL_PARMS];
} lightParms_t;

void			R_LightEditorUpdateCallback (int index, lightParms_t *parms);
void			R_LightEditorRemoveCallback (int index);
void			R_LightEditorSaveCallback ();
void			R_LightEditorCloseCallback ();

void *			WIN_CreateLightEditorWindow ();
void			WIN_DestroyLightEditorWindow ();
void			WIN_EditLightParameters (int index, lightParms_t *parms);


#endif	// __EDITOR_H__