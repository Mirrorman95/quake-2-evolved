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
// ui_advanced.c - Advanced menu
//

// TODO!!!


#include "ui_local.h"


#define ART_BACKGROUND				"ui/assets/ui/ui_sub_options"
#define ART_BANNER					"ui/assets/banners/advanced_t"
#define ART_TEXT1					"ui/assets/text/advanced_text_p1"
#define ART_TEXT2					"ui/assets/text/advanced_text_p2"

typedef enum {
	ID_BACKGROUND,
	ID_BANNER,

	ID_TEXT1,
	ID_TEXT2,
	ID_TEXTSHADOW1,
	ID_TEXTSHADOW2,

	ID_CANCEL,
	ID_APPLY,
} uiAdvancedId_t;

static const char *			uiAdvancedYesNo[] = {
	"False",
	"True"
};

typedef struct {
	int todo;
} uiAdvancedValues_t;

static uiAdvancedValues_t	uiAdvancedInitial;

typedef struct {
	menuFramework_t			menu;

	menuBitmap_t			background;
	menuBitmap_t			banner;

	menuBitmap_t			textShadow1;
	menuBitmap_t			textShadow2;
	menuBitmap_t			text1;
	menuBitmap_t			text2;

	menuBitmap_t			cancel;
	menuBitmap_t			apply;
} uiAdvanced_t;

static uiAdvanced_t			uiAdvanced;


/*
 ==================
 
 ==================
*/
static void UI_Advanced_GetConfig (){

}

/*
 ==================
 
 ==================
*/
static void UI_Advanced_SetConfig (){

	// Restart video subsystem
	Cmd_ExecuteText(CMD_EXEC_NOW, "restartVideo\n");
}

/*
 ==================
 
 ==================
*/
static void UI_Advanced_UpdateConfig (){

	// Some settings can be updated here

	// See if the apply button should be enabled or disabled
	uiAdvanced.apply.generic.flags |= QMF_GRAYED;
}

/*
 ==================
 UI_Advanced_Callback
 ==================
*/
static void UI_Advanced_Callback (void *self, int event){

	menuCommon_t	*item = (menuCommon_t *)self;

	if (event == QM_CHANGED){
		UI_Advanced_UpdateConfig();
		return;
	}

	if (event != QM_ACTIVATED)
		return;

	switch (item->id){
	case ID_CANCEL:
		UI_PopMenu();
		break;
	case ID_APPLY:
		UI_Advanced_SetConfig();
		break;
	}
}

/*
 ==================
 UI_Advanced_Ownerdraw
 ==================
*/
static void UI_Advanced_Ownerdraw (void *self){

	menuCommon_t	*item = (menuCommon_t *)self;

	if (uiAdvanced.menu.items[uiAdvanced.menu.cursor] == self)
		UI_DrawPic(item->x, item->y, item->width, item->height, colorWhite, UI_MOVEBOXFOCUS);
	else
		UI_DrawPic(item->x, item->y, item->width, item->height, colorWhite, UI_MOVEBOX);

	if (item->flags & QMF_GRAYED)
		UI_DrawPic(item->x, item->y, item->width, item->height, uiColorDarkGray, ((menuBitmap_t *)self)->pic);
	else
		UI_DrawPic(item->x, item->y, item->width, item->height, colorWhite, ((menuBitmap_t *)self)->pic);
}

/*
 ==================
 
 ==================
*/
static void UI_Advanced_Init (){

	Mem_Fill(&uiAdvanced, 0, sizeof(uiAdvanced_t));

	uiAdvanced.background.generic.id			= ID_BACKGROUND;
	uiAdvanced.background.generic.type			= QMTYPE_BITMAP;
	uiAdvanced.background.generic.flags			= QMF_INACTIVE;
	uiAdvanced.background.generic.x				= 0;
	uiAdvanced.background.generic.y				= 0;
	uiAdvanced.background.generic.width			= 1024;
	uiAdvanced.background.generic.height		= 768;
	uiAdvanced.background.pic					= ART_BACKGROUND;

	uiAdvanced.banner.generic.id				= ID_BANNER;
	uiAdvanced.banner.generic.type				= QMTYPE_BITMAP;
	uiAdvanced.banner.generic.flags				= QMF_INACTIVE;
	uiAdvanced.banner.generic.x					= 0;
	uiAdvanced.banner.generic.y					= 66;
	uiAdvanced.banner.generic.width				= 1024;
	uiAdvanced.banner.generic.height			= 46;
	uiAdvanced.banner.pic						= ART_BANNER;

	uiAdvanced.textShadow1.generic.id			= ID_TEXTSHADOW1;
	uiAdvanced.textShadow1.generic.type			= QMTYPE_BITMAP;
	uiAdvanced.textShadow1.generic.flags		= QMF_INACTIVE;
	uiAdvanced.textShadow1.generic.x			= 182;
	uiAdvanced.textShadow1.generic.y			= 170;
	uiAdvanced.textShadow1.generic.width		= 256;
	uiAdvanced.textShadow1.generic.height		= 256;
	uiAdvanced.textShadow1.generic.color		= colorBlack;
	uiAdvanced.textShadow1.pic					= ART_TEXT1;

	uiAdvanced.textShadow2.generic.id			= ID_TEXTSHADOW2;
	uiAdvanced.textShadow2.generic.type			= QMTYPE_BITMAP;
	uiAdvanced.textShadow2.generic.flags		= QMF_INACTIVE;
	uiAdvanced.textShadow2.generic.x			= 182;
	uiAdvanced.textShadow2.generic.y			= 426;
	uiAdvanced.textShadow2.generic.width		= 256;
	uiAdvanced.textShadow2.generic.height		= 256;
	uiAdvanced.textShadow2.generic.color		= colorBlack;
	uiAdvanced.textShadow2.pic					= ART_TEXT2;

	uiAdvanced.text1.generic.id					= ID_TEXT1;
	uiAdvanced.text1.generic.type				= QMTYPE_BITMAP;
	uiAdvanced.text1.generic.flags				= QMF_INACTIVE;
	uiAdvanced.text1.generic.x					= 180;
	uiAdvanced.text1.generic.y					= 168;
	uiAdvanced.text1.generic.width				= 256;
	uiAdvanced.text1.generic.height				= 256;
	uiAdvanced.text1.pic						= ART_TEXT1;

	uiAdvanced.text2.generic.id					= ID_TEXT2;
	uiAdvanced.text2.generic.type				= QMTYPE_BITMAP;
	uiAdvanced.text2.generic.flags				= QMF_INACTIVE;
	uiAdvanced.text2.generic.x					= 180;
	uiAdvanced.text2.generic.y					= 424;
	uiAdvanced.text2.generic.width				= 256;
	uiAdvanced.text2.generic.height				= 256;
	uiAdvanced.text2.pic						= ART_TEXT2;

	uiAdvanced.cancel.generic.id				= ID_CANCEL;
	uiAdvanced.cancel.generic.type				= QMTYPE_BITMAP;
	uiAdvanced.cancel.generic.x					= 310;
	uiAdvanced.cancel.generic.y					= 656;
	uiAdvanced.cancel.generic.width				= 198;
	uiAdvanced.cancel.generic.height			= 38;
	uiAdvanced.cancel.generic.callback			= UI_Advanced_Callback;
	uiAdvanced.cancel.generic.ownerdraw			= UI_Advanced_Ownerdraw;
	uiAdvanced.cancel.pic						= UI_CANCELBUTTON;

	uiAdvanced.apply.generic.id					= ID_APPLY;
	uiAdvanced.apply.generic.type				= QMTYPE_BITMAP;
	uiAdvanced.apply.generic.x					= 516;
	uiAdvanced.apply.generic.y					= 656;
	uiAdvanced.apply.generic.width				= 198;
	uiAdvanced.apply.generic.height				= 38;
	uiAdvanced.apply.generic.callback			= UI_Advanced_Callback;
	uiAdvanced.apply.generic.ownerdraw			= UI_Advanced_Ownerdraw;
	uiAdvanced.apply.pic						= UI_APPLYBUTTON;

	UI_Advanced_GetConfig();

	UI_AddItem(&uiAdvanced.menu, (void *)&uiAdvanced.background);
	UI_AddItem(&uiAdvanced.menu, (void *)&uiAdvanced.banner);
	UI_AddItem(&uiAdvanced.menu, (void *)&uiAdvanced.textShadow1);
	UI_AddItem(&uiAdvanced.menu, (void *)&uiAdvanced.textShadow2);
	UI_AddItem(&uiAdvanced.menu, (void *)&uiAdvanced.text1);
	UI_AddItem(&uiAdvanced.menu, (void *)&uiAdvanced.text2);
	UI_AddItem(&uiAdvanced.menu, (void *)&uiAdvanced.cancel);
	UI_AddItem(&uiAdvanced.menu, (void *)&uiAdvanced.apply);
}

/*
 ==================
 UI_Advanced_Precache
 ==================
*/
void UI_Advanced_Precache (){

	R_RegisterMaterialNoMip(ART_BACKGROUND);
	R_RegisterMaterialNoMip(ART_BANNER);
	R_RegisterMaterialNoMip(ART_TEXT1);
	R_RegisterMaterialNoMip(ART_TEXT2);
}

/*
 ==================
 UI_Advanced_Menu
 ==================
*/
void UI_Advanced_Menu (){

	UI_Advanced_Precache();
	UI_Advanced_Init();

	UI_Advanced_UpdateConfig();

	UI_PushMenu(&uiAdvanced.menu);
}