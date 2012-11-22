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
// ui_public.h - Public header file to all other systems
//


#ifndef __UI_PUBLIC_H__
#define __UI_PUBLIC_H__


typedef enum {
	UI_CLOSEMENU,
	UI_MAINMENU,
	UI_INGAMEMENU
} uiActiveMenu_t;

// Handles key events
void			UI_KeyEvent (int key, bool down);

// Handles char events on the active UI, if any
void			UI_CharEvent (int ch);

// Handles mouse movement on the active UI, if any
void			UI_MouseMove (float x, float y);

// Redraws the given UI
void			UI_UpdateMenu (int realTime);

// Sets the given UI to active
void			UI_SetActiveMenu (uiActiveMenu_t activeMenu);

// Adds the server address to the UI address book
void			UI_AddServerToList (netAdr_t adr, const char *info);

// Returns true if the given UI is fully visible
bool			UI_IsVisible ();

// Returns true if the given UI covers the entire screen
bool			UI_IsFullscreen ();

// Pre-caches all media assets
void			UI_Precache ();

// Initializes the UI subsystem
void			UI_Init ();

// Shuts down the UI subsystem
void			UI_Shutdown ();


#endif	// __UI_PUBLIC_H__