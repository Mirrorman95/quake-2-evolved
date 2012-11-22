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
// ui_audio.c - Audio menu
//

// TODO!!!


#include "ui_local.h"


#define ART_BACKGROUND				"ui/assets/ui/ui_sub_options"
#define ART_BANNER					"ui/assets/banners/audio_t"
#define ART_TEXT1					"ui/assets/text/audio_text_p1"
#define ART_TEXT2					"ui/assets/text/audio_text_p2"

typedef enum {
	ID_BACKGROUND,
	ID_BANNER,

	ID_TEXT1,
	ID_TEXT2,
	ID_TEXTSHADOW1,
	ID_TEXTSHADOW2,

	ID_CANCEL,
	ID_APPLY,
} uiAudioId_t;

static const char *			uiAudioYesNo[] = {
	"False",
	"True"
};

typedef struct {
	int todo;
} uiAudioValues_t;

static uiAudioValues_t		uiAudioInitial;

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
} uiAudio_t;

static uiAudio_t			uiAudio;


/*
 ==================
 
 ==================
*/
static void UI_Audio_GetDeviceList (){

}

/*
 ==================
 
 ==================
*/
static void UI_Audio_GetConfig (){

	UI_Audio_GetDeviceList();

	// Save initial values
}

/*
 ==================
 
 ==================
*/
static void UI_Audio_SetConfig (){

	// Restart sound subsystem
	Cmd_ExecuteText(CMD_EXEC_NOW, "restartSound\n");
}

/*
 ==================
 
 ==================
*/
static void UI_Audio_UpdateConfig (){

	// Some settings can be updated here

	// See if the apply button should be enabled or disabled
	uiAudio.apply.generic.flags |= QMF_GRAYED;
}

/*
 ==================
 UI_Audio_Callback
 ==================
*/
static void UI_Audio_Callback (void *self, int event){

	menuCommon_t	*item = (menuCommon_t *)self;

	if (event == QM_CHANGED){
		UI_Audio_UpdateConfig();
		return;
	}

	if (event != QM_ACTIVATED)
		return;

	switch (item->id){
	case ID_CANCEL:
		UI_PopMenu();
		break;
	case ID_APPLY:
		UI_Audio_SetConfig();
		UI_Audio_GetConfig();
		UI_Audio_UpdateConfig();
		break;
	}
}

/*
 ==================
 UI_Audio_Ownerdraw
 ==================
*/
static void UI_Audio_Ownerdraw (void *self){

	menuCommon_t	*item = (menuCommon_t *)self;

	if (uiAudio.menu.items[uiAudio.menu.cursor] == self)
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
static void UI_Audio_Init (void){

	Mem_Fill(&uiAudio, 0, sizeof(uiAudio_t));

	uiAudio.background.generic.id				= ID_BACKGROUND;
	uiAudio.background.generic.type				= QMTYPE_BITMAP;
	uiAudio.background.generic.flags			= QMF_INACTIVE;
	uiAudio.background.generic.x				= 0;
	uiAudio.background.generic.y				= 0;
	uiAudio.background.generic.width			= 1024;
	uiAudio.background.generic.height			= 768;
	uiAudio.background.pic						= ART_BACKGROUND;

	uiAudio.banner.generic.id					= ID_BANNER;
	uiAudio.banner.generic.type					= QMTYPE_BITMAP;
	uiAudio.banner.generic.flags				= QMF_INACTIVE;
	uiAudio.banner.generic.x					= 0;
	uiAudio.banner.generic.y					= 66;
	uiAudio.banner.generic.width				= 1024;
	uiAudio.banner.generic.height				= 46;
	uiAudio.banner.pic							= ART_BANNER;

	uiAudio.textShadow1.generic.id				= ID_TEXTSHADOW1;
	uiAudio.textShadow1.generic.type			= QMTYPE_BITMAP;
	uiAudio.textShadow1.generic.flags			= QMF_INACTIVE;
	uiAudio.textShadow1.generic.x				= 182;
	uiAudio.textShadow1.generic.y				= 170;
	uiAudio.textShadow1.generic.width			= 256;
	uiAudio.textShadow1.generic.height			= 256;
	uiAudio.textShadow1.generic.color			= colorBlack;
	uiAudio.textShadow1.pic						= ART_TEXT1;

	uiAudio.textShadow2.generic.id				= ID_TEXTSHADOW2;
	uiAudio.textShadow2.generic.type			= QMTYPE_BITMAP;
	uiAudio.textShadow2.generic.flags			= QMF_INACTIVE;
	uiAudio.textShadow2.generic.x				= 182;
	uiAudio.textShadow2.generic.y				= 426;
	uiAudio.textShadow2.generic.width			= 256;
	uiAudio.textShadow2.generic.height			= 256;
	uiAudio.textShadow2.generic.color			= colorBlack;
	uiAudio.textShadow2.pic						= ART_TEXT2;

	uiAudio.text1.generic.id					= ID_TEXT1;
	uiAudio.text1.generic.type					= QMTYPE_BITMAP;
	uiAudio.text1.generic.flags					= QMF_INACTIVE;
	uiAudio.text1.generic.x						= 180;
	uiAudio.text1.generic.y						= 168;
	uiAudio.text1.generic.width					= 256;
	uiAudio.text1.generic.height				= 256;
	uiAudio.text1.pic							= ART_TEXT1;

	uiAudio.text2.generic.id					= ID_TEXT2;
	uiAudio.text2.generic.type					= QMTYPE_BITMAP;
	uiAudio.text2.generic.flags					= QMF_INACTIVE;
	uiAudio.text2.generic.x						= 180;
	uiAudio.text2.generic.y						= 424;
	uiAudio.text2.generic.width					= 256;
	uiAudio.text2.generic.height				= 256;
	uiAudio.text2.pic							= ART_TEXT2;

	uiAudio.cancel.generic.id					= ID_CANCEL;
	uiAudio.cancel.generic.type					= QMTYPE_BITMAP;
	uiAudio.cancel.generic.x					= 310;
	uiAudio.cancel.generic.y					= 656;
	uiAudio.cancel.generic.width				= 198;
	uiAudio.cancel.generic.height				= 38;
	uiAudio.cancel.generic.callback				= UI_Audio_Callback;
	uiAudio.cancel.generic.ownerdraw			= UI_Audio_Ownerdraw;
	uiAudio.cancel.pic							= UI_CANCELBUTTON;

	uiAudio.apply.generic.id					= ID_APPLY;
	uiAudio.apply.generic.type					= QMTYPE_BITMAP;
	uiAudio.apply.generic.x						= 516;
	uiAudio.apply.generic.y						= 656;
	uiAudio.apply.generic.width					= 198;
	uiAudio.apply.generic.height				= 38;
	uiAudio.apply.generic.callback				= UI_Audio_Callback;
	uiAudio.apply.generic.ownerdraw				= UI_Audio_Ownerdraw;
	uiAudio.apply.pic							= UI_APPLYBUTTON;

	UI_Audio_GetConfig();

	UI_AddItem(&uiAudio.menu, (void *)&uiAudio.background);
	UI_AddItem(&uiAudio.menu, (void *)&uiAudio.banner);
	UI_AddItem(&uiAudio.menu, (void *)&uiAudio.textShadow1);
	UI_AddItem(&uiAudio.menu, (void *)&uiAudio.textShadow2);
	UI_AddItem(&uiAudio.menu, (void *)&uiAudio.text1);
	UI_AddItem(&uiAudio.menu, (void *)&uiAudio.text2);
	UI_AddItem(&uiAudio.menu, (void *)&uiAudio.cancel);
	UI_AddItem(&uiAudio.menu, (void *)&uiAudio.apply);
}

/*
 ==================
 UI_Audio_Precache
 ==================
*/
void UI_Audio_Precache (){

	R_RegisterMaterialNoMip(ART_BACKGROUND);
	R_RegisterMaterialNoMip(ART_BANNER);
	R_RegisterMaterialNoMip(ART_TEXT1);
	R_RegisterMaterialNoMip(ART_TEXT2);
}

/*
 ==================
 UI_Audio_Menu
 ==================
*/
void UI_Audio_Menu (){

	UI_Audio_Precache();
	UI_Audio_Init();

	UI_Audio_UpdateConfig();

	UI_PushMenu(&uiAudio.menu);
}