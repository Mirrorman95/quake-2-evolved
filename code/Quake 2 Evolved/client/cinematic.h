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
// cinematic.h - Cinematic playback
//


#ifndef __CINEMATIC_H__
#define __CINEMATIC_H__


#define MAX_CINEMATICS				8

typedef enum {
	CIN_SYSTEM				= BIT(0),	// A cinematic handled by the client system
	CIN_LOOPING				= BIT(1),	// Looped playback
	CIN_SILENT				= BIT(2)	// Don't play audio
} cinFlags_t;

typedef int cinHandle_t;

// Will run a frame of the cinematic but will not draw it. Will return
// false if the end of the cinematic has been reached.
bool			CIN_UpdateCinematic ();

// Draws the current frame
void			CIN_DrawCinematic ();

// Returns a handle to the cinematic
bool			CIN_PlayCinematic (const char *name, int flags);

// Stops playing the cinematic
void			CIN_StopCinematic ();

// Initializes the cinematic module
void			CIN_Init ();

// Shuts down the cinematic module
void			CIN_Shutdown ();


#endif	// __CINEMATIC_H__