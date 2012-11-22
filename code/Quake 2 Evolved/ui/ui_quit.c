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
// ui_quit.c - Quit menu
//


#include "ui_local.h"


#define ART_BACKGROUND				"ui/assets/title_screen/title_backg"
#define ART_MSGBOX					"ui/assets/quit_screen/quit"

typedef enum {
	ID_BACKGROUND,

	ID_MSGBOX,
	ID_YES,
	ID_NO
} uiQuitId_t;

typedef struct {
	menuFramework_t			menu;

	menuBitmap_t			background;

	menuBitmap_t			msgBox;
	menuBitmap_t			no;
	menuBitmap_t			yes;
} uiQuit_t;

static uiQuit_t				uiQuit;


/*
 ==================
 UI_Quit_Callback
 ==================
*/
static void UI_Quit_Callback (void *self, int event){

	menuCommon_t	*item = (menuCommon_t *)self;

	if (event != QM_ACTIVATED)
		return;

	switch (item->id){
	case ID_YES:
		UI_Credits_Menu();
		break;
	case ID_NO:
		UI_PopMenu();
		break;
	}
}

/*
 ==================
 UI_Quit_Ownerdraw
 ==================
*/
static void UI_Quit_Ownerdraw (void *self){

	menuCommon_t	*item = (menuCommon_t *)self;

	if (uiQuit.menu.items[uiQuit.menu.cursor] == self)
		UI_DrawPic(item->x, item->y, item->width, item->height, colorWhite, UI_MOVEBOXFOCUS);
	else
		UI_DrawPic(item->x, item->y, item->width, item->height, colorWhite, UI_MOVEBOX);
	
	UI_DrawPic(item->x, item->y, item->width, item->height, colorWhite, ((menuBitmap_t *)self)->pic);
}

/*
 ==================
 UI_Quit_Init
 ==================
*/
static void UI_Quit_Init (){

	Mem_Fill(&uiQuit, 0, sizeof(uiQuit_t));

	uiQuit.background.generic.id				= ID_BACKGROUND;
	uiQuit.background.generic.type				= QMTYPE_BITMAP;
	uiQuit.background.generic.flags				= QMF_INACTIVE | QMF_GRAYED;
	uiQuit.background.generic.x					= 0;
	uiQuit.background.generic.y					= 0;
	uiQuit.background.generic.width				= 1024;
	uiQuit.background.generic.height			= 768;
	uiQuit.background.pic						= ART_BACKGROUND;

	uiQuit.msgBox.generic.id					= ID_MSGBOX;
	uiQuit.msgBox.generic.type					= QMTYPE_BITMAP;
	uiQuit.msgBox.generic.flags					= QMF_INACTIVE;
	uiQuit.msgBox.generic.x						= 174;
	uiQuit.msgBox.generic.y						= 284;
	uiQuit.msgBox.generic.width					= 676;
	uiQuit.msgBox.generic.height				= 202;
	uiQuit.msgBox.pic							= ART_MSGBOX;

	uiQuit.no.generic.id						= ID_NO;
	uiQuit.no.generic.type						= QMTYPE_BITMAP;
	uiQuit.no.generic.x							= 310;
	uiQuit.no.generic.y							= 410;
	uiQuit.no.generic.width						= 198;
	uiQuit.no.generic.height					= 38;
	uiQuit.no.generic.callback					= UI_Quit_Callback;
	uiQuit.no.generic.ownerdraw					= UI_Quit_Ownerdraw;
	uiQuit.no.pic								= UI_CANCELBUTTON;

	uiQuit.yes.generic.id						= ID_YES;
	uiQuit.yes.generic.type						= QMTYPE_BITMAP;
	uiQuit.yes.generic.x						= 516;
	uiQuit.yes.generic.y						= 410;
	uiQuit.yes.generic.width					= 198;
	uiQuit.yes.generic.height					= 38;
	uiQuit.yes.generic.callback					= UI_Quit_Callback;
	uiQuit.yes.generic.ownerdraw				= UI_Quit_Ownerdraw;
	uiQuit.yes.pic								= UI_ACCEPTBUTTON;

	UI_AddItem(&uiQuit.menu, (void *)&uiQuit.background);
	UI_AddItem(&uiQuit.menu, (void *)&uiQuit.msgBox);
	UI_AddItem(&uiQuit.menu, (void *)&uiQuit.no);
	UI_AddItem(&uiQuit.menu, (void *)&uiQuit.yes);
}

/*
 ==================
 UI_Quit_Precache
 ==================
*/
void UI_Quit_Precache (){

	R_RegisterMaterialNoMip(ART_BACKGROUND);
	R_RegisterMaterialNoMip(ART_MSGBOX);
}

/*
 ==================
 UI_Quit_Menu
 ==================
*/
void UI_Quit_Menu (){

	UI_Quit_Precache();
	UI_Quit_Init();

	UI_PushMenu(&uiQuit.menu);
}