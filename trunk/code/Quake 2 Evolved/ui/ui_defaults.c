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
// ui_defaults.c - Defaults menu
//


#include "ui_local.h"


#define ART_BACKGROUND				"ui/assets/ui/ui_sub_options"
#define ART_MSGBOX					"ui/assets/ui/defaults_warn"

typedef enum {
	ID_BACKGROUND,

	ID_MSGBOX,
	ID_YES,
	ID_NO
} uiDefaultsId_t;

typedef struct {
	menuFramework_t			menu;

	menuBitmap_t			background;

	menuBitmap_t			msgBox;
	menuBitmap_t			no;
	menuBitmap_t			yes;
} uiDefaults_t;

static uiDefaults_t			uiDefaults;


/*
 ==================
 UI_Defaults_Callback
 ==================
*/ 
static void UI_Defaults_Callback (void *self, int event){

	menuCommon_t	*item = (menuCommon_t *)self;

	if (event != QM_ACTIVATED)
		return;

	switch (item->id){
	case ID_YES:
		// Set default bindings and restart all cvars
		Cmd_ExecuteText(CMD_EXEC_APPEND, "exec default.cfg\n");
		Cmd_ExecuteText(CMD_EXEC_APPEND, "restartCVars\n");

		// Restart all the subsystems for the changes to take effect
		Cmd_ExecuteText(CMD_EXEC_APPEND, "restartNetwork\n");
		Cmd_ExecuteText(CMD_EXEC_APPEND, "restartInput\n");
		Cmd_ExecuteText(CMD_EXEC_APPEND, "restartVideo\n");

		break;
	case ID_NO:
		UI_PopMenu();
		break;
	}
}

/*
 ==================
 UI_Defaults_Ownerdraw
 ==================
*/ 
static void UI_Defaults_Ownerdraw (void *self){

	menuCommon_t	*item = (menuCommon_t *)self;

	if (uiDefaults.menu.items[uiDefaults.menu.cursor] == self)
		UI_DrawPic(item->x, item->y, item->width, item->height, colorWhite, UI_MOVEBOXFOCUS);
	else
		UI_DrawPic(item->x, item->y, item->width, item->height, colorWhite, UI_MOVEBOX);
	
	UI_DrawPic(item->x, item->y, item->width, item->height, colorWhite, ((menuBitmap_t *)self)->pic);
}

/*
 ==================
 UI_Defaults_Init
 ==================
*/ 
static void UI_Defaults_Init (){

	Mem_Fill(&uiDefaults, 0, sizeof(uiDefaults_t));

	uiDefaults.background.generic.id			= ID_BACKGROUND;
	uiDefaults.background.generic.type			= QMTYPE_BITMAP;
	uiDefaults.background.generic.flags			= QMF_INACTIVE | QMF_GRAYED;
	uiDefaults.background.generic.x				= 0;
	uiDefaults.background.generic.y				= 0;
	uiDefaults.background.generic.width			= 1024;
	uiDefaults.background.generic.height		= 768;
	uiDefaults.background.pic					= ART_BACKGROUND;

	uiDefaults.msgBox.generic.id				= ID_MSGBOX;
	uiDefaults.msgBox.generic.type				= QMTYPE_BITMAP;
	uiDefaults.msgBox.generic.flags				= QMF_INACTIVE;
	uiDefaults.msgBox.generic.x					= 174;
	uiDefaults.msgBox.generic.y					= 256;
	uiDefaults.msgBox.generic.width				= 676;
	uiDefaults.msgBox.generic.height			= 256;
	uiDefaults.msgBox.pic						= ART_MSGBOX;

	uiDefaults.no.generic.id					= ID_NO;
	uiDefaults.no.generic.type					= QMTYPE_BITMAP;
	uiDefaults.no.generic.flags					= 0;
	uiDefaults.no.generic.x						= 310;
	uiDefaults.no.generic.y						= 434;
	uiDefaults.no.generic.width					= 198;
	uiDefaults.no.generic.height				= 38;
	uiDefaults.no.generic.callback				= UI_Defaults_Callback;
	uiDefaults.no.generic.ownerdraw				= UI_Defaults_Ownerdraw;
	uiDefaults.no.pic							= UI_CANCELBUTTON;

	uiDefaults.yes.generic.id					= ID_YES;
	uiDefaults.yes.generic.type					= QMTYPE_BITMAP;
	uiDefaults.yes.generic.flags				= 0;
	uiDefaults.yes.generic.x					= 516;
	uiDefaults.yes.generic.y					= 434;
	uiDefaults.yes.generic.width				= 198;
	uiDefaults.yes.generic.height				= 38;
	uiDefaults.yes.generic.callback				= UI_Defaults_Callback;
	uiDefaults.yes.generic.ownerdraw			= UI_Defaults_Ownerdraw;
	uiDefaults.yes.pic							= UI_ACCEPTBUTTON;

	UI_AddItem(&uiDefaults.menu, (void *)&uiDefaults.background);
	UI_AddItem(&uiDefaults.menu, (void *)&uiDefaults.msgBox);
	UI_AddItem(&uiDefaults.menu, (void *)&uiDefaults.no);
	UI_AddItem(&uiDefaults.menu, (void *)&uiDefaults.yes);
}

/*
 ==================
 UI_Defaults_Precache
 ==================
*/ 
void UI_Defaults_Precache (){

	R_RegisterMaterialNoMip(ART_BACKGROUND);
	R_RegisterMaterialNoMip(ART_MSGBOX);
}

/*
 ==================
 UI_Defaults_Menu
 ==================
*/ 
void UI_Defaults_Menu (){

	UI_Defaults_Precache();
	UI_Defaults_Init();

	UI_PushMenu(&uiDefaults.menu);
}