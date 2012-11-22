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
// ui_gotosite.c - Go-to-site menu
//


#include "ui_local.h"


#define ART_BACKGROUND				"ui/assets/title_screen/title_backg"
#define ART_MSGBOX					"ui/assets/title_screen/site_load"

typedef enum {
	ID_BACKGROUND,

	ID_MSGBOX,
	ID_YES,
	ID_NO
} uiGoToSiteId_t;

typedef struct {
	char					webAddress[256];

	menuFramework_t			menu;

	menuBitmap_t			background;

	menuBitmap_t			msgBox;
	menuBitmap_t			no;
	menuBitmap_t			yes;
} uiGoToSite_t;

static uiGoToSite_t			uiGoToSite;


/*
 ==================
 UI_GoToSite_Callback
 ==================
*/
static void UI_GoToSite_Callback (void *self, int event){

	menuCommon_t	*item = (menuCommon_t *)self;

	if (event != QM_ACTIVATED)
		return;

	switch (item->id){
	case ID_YES:
		Sys_OpenURL(uiGoToSite.webAddress, true);
		break;
	case ID_NO:
		UI_PopMenu();
		break;
	}
}

/*
 ==================
 UI_GoToSite_Ownerdraw
 ==================
*/
static void UI_GoToSite_Ownerdraw (void *self){

	menuCommon_t	*item = (menuCommon_t *)self;

	if (uiGoToSite.menu.items[uiGoToSite.menu.cursor] == self)
		UI_DrawPic(item->x, item->y, item->width, item->height, colorWhite, UI_MOVEBOXFOCUS);
	else
		UI_DrawPic(item->x, item->y, item->width, item->height, colorWhite, UI_MOVEBOX);
	
	UI_DrawPic(item->x, item->y, item->width, item->height, colorWhite, ((menuBitmap_t *)self)->pic);
}

/*
 ==================
 UI_GoToSite_Init
 ==================
*/
static void UI_GoToSite_Init (){

	Mem_Fill(&uiGoToSite, 0, sizeof(uiGoToSite_t));

	uiGoToSite.background.generic.id			= ID_BACKGROUND;
	uiGoToSite.background.generic.type			= QMTYPE_BITMAP;
	uiGoToSite.background.generic.flags			= QMF_INACTIVE | QMF_GRAYED;
	uiGoToSite.background.generic.x				= 0;
	uiGoToSite.background.generic.y				= 0;
	uiGoToSite.background.generic.width			= 1024;
	uiGoToSite.background.generic.height		= 768;
	uiGoToSite.background.pic					= ART_BACKGROUND;

	uiGoToSite.msgBox.generic.id				= ID_MSGBOX;
	uiGoToSite.msgBox.generic.type				= QMTYPE_BITMAP;
	uiGoToSite.msgBox.generic.flags				= QMF_INACTIVE;
	uiGoToSite.msgBox.generic.x					= 174;
	uiGoToSite.msgBox.generic.y					= 256;
	uiGoToSite.msgBox.generic.width				= 676;
	uiGoToSite.msgBox.generic.height			= 256;
	uiGoToSite.msgBox.pic						= ART_MSGBOX;

	uiGoToSite.no.generic.id					= ID_NO;
	uiGoToSite.no.generic.type					= QMTYPE_BITMAP;
	uiGoToSite.no.generic.flags					= 0;
	uiGoToSite.no.generic.x						= 310;
	uiGoToSite.no.generic.y						= 434;
	uiGoToSite.no.generic.width					= 198;
	uiGoToSite.no.generic.height				= 38;
	uiGoToSite.no.generic.callback				= UI_GoToSite_Callback;
	uiGoToSite.no.generic.ownerdraw				= UI_GoToSite_Ownerdraw;
	uiGoToSite.no.pic							= UI_CANCELBUTTON;

	uiGoToSite.yes.generic.id					= ID_YES;
	uiGoToSite.yes.generic.type					= QMTYPE_BITMAP;
	uiGoToSite.yes.generic.flags				= 0;
	uiGoToSite.yes.generic.x					= 516;
	uiGoToSite.yes.generic.y					= 434;
	uiGoToSite.yes.generic.width				= 198;
	uiGoToSite.yes.generic.height				= 38;
	uiGoToSite.yes.generic.callback				= UI_GoToSite_Callback;
	uiGoToSite.yes.generic.ownerdraw			= UI_GoToSite_Ownerdraw;
	uiGoToSite.yes.pic							= UI_ACCEPTBUTTON;

	UI_AddItem(&uiGoToSite.menu, (void *)&uiGoToSite.background);
	UI_AddItem(&uiGoToSite.menu, (void *)&uiGoToSite.msgBox);
	UI_AddItem(&uiGoToSite.menu, (void *)&uiGoToSite.no);
	UI_AddItem(&uiGoToSite.menu, (void *)&uiGoToSite.yes);
}

/*
 ==================
 UI_GoToSite_Precache
 ==================
*/
void UI_GoToSite_Precache (){

	R_RegisterMaterialNoMip(ART_BACKGROUND);
	R_RegisterMaterialNoMip(ART_MSGBOX);
}

/*
 ==================
 UI_GoToSite_Menu
 ==================
*/
void UI_GoToSite_Menu (const char *webAddress){

	UI_GoToSite_Precache();
	UI_GoToSite_Init();

	Str_Copy(uiGoToSite.webAddress, webAddress, sizeof(uiGoToSite.webAddress));

	UI_PushMenu(&uiGoToSite.menu);
}