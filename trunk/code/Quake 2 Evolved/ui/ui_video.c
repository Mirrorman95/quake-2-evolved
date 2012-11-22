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
// ui_video.c - Video menu
//

// TODO!!!


#include "ui_local.h"


#define ART_BACKGROUND				"ui/assets/ui/ui_sub_options"
#define ART_BANNER					"ui/assets/banners/video_t"
#define ART_TEXT1					"ui/assets/text/video_text_p1"
#define ART_TEXT2					"ui/assets/text/video_text_p2"
#define ART_ADVANCED				"ui/assets/buttons/advanced_b"

typedef enum {
	ID_BACKGROUND,
	ID_BANNER,

	ID_TEXT1,
	ID_TEXT2,
	ID_TEXTSHADOW1,
	ID_TEXTSHADOW2,

	ID_CANCEL,
	ID_ADVANCED,
	ID_APPLY,
} uiVideoId_t;

static const char *			uiVideoSettings[] = {
	"Custom",
	"Fastest",
	"Fast",
	"Normal",
	"High Quality"
};

static const char *			uiVideoYesNo[] = {
	"False",
	"True"
};

static const char *			uiVideoModes[] = {
	"Custom",
	"640x480 (4:3)",
	"800x600 (4:3)",
	"960x720 (4:3)",
	"1024x768 (4:3)",
	"1152x864 (4:3)",
	"1280x960 (4:3)",
	"1400x1050 (4:3)",
	"1600x1200 (4:3)",
	"2048x1536 (4:3)",
	"720x576 (5:4)",
	"1280x1024 (5:4)",
	"640x360 (16:9)",
	"800x450 (16:9)",
	"960x540 (16:9)",
	"1024x576 (16:9)",
	"1088x612 (16:9)",
	"1280x720 (16:9)",
	"1600x900 (16:9)",
	"1920x1080 (16:9)",
	"2048x1152 (16:9)",
	"640x400 (16:10)",
	"800x500 (16:10)",
	"960x600 (16:10)",
	"1024x640 (16:10)",
	"1152x720 (16:10)",
	"1280x800 (16:10)",
	"1440x900 (16:10)",
	"1680x1050 (16:10)",
	"1920x1200 (16:10)",
};

static const char *			uiVideoDetail[] = {
	"Low",
	"Medium",
	"High",
	"Max"
};

static const char *			uiVideoTextureFilters[] = {
	"Bilinear",
	"Trilinear"
};

typedef struct {
	int todo;
} uiVideoValues_t;

static uiVideoValues_t		uiVideoInitial;

typedef struct {
	menuFramework_t			menu;

	menuBitmap_t			background;
	menuBitmap_t			banner;

	menuBitmap_t			textShadow1;
	menuBitmap_t			textShadow2;
	menuBitmap_t			text1;
	menuBitmap_t			text2;

	menuBitmap_t			cancel;
	menuBitmap_t			advanced;
	menuBitmap_t			apply;
} uiVideo_t;

static uiVideo_t			uiVideo;


/*
 ==================
 
 ==================
*/
static void UI_Video_GetConfig (){

	// Save initial values
}

/*
 ==================
 
 ==================
*/
static void UI_Video_SetConfig (){

	// Restart video subsystem
	Cmd_ExecuteText(CMD_EXEC_NOW, "restartVideo\n");
}

/*
 ==================
 
 ==================
*/
static void UI_Video_UpdateConfig (){

	// Some settings can be updated here

	// See if the apply button should be enabled or disabled
	uiVideo.apply.generic.flags |= QMF_GRAYED;
}

/*
 ==================
 UI_Video_Callback
 ==================
*/
static void UI_Video_Callback (void *self, int event){

	menuCommon_t	*item = (menuCommon_t *)self;

	if (event == QM_CHANGED){
		UI_Video_UpdateConfig();
		return;
	}

	if (event != QM_ACTIVATED)
		return;

	switch (item->id){
	case ID_CANCEL:
		UI_PopMenu();
		break;
	case ID_ADVANCED:
		UI_Advanced_Menu();
		break;
	case ID_APPLY:
		UI_Video_SetConfig();
		break;
	}
}

/*
 ==================
 UI_Video_Ownerdraw
 ==================
*/
static void UI_Video_Ownerdraw (void *self){

	menuCommon_t	*item = (menuCommon_t *)self;

	if (uiVideo.menu.items[uiVideo.menu.cursor] == self)
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
static void UI_Video_Init (){

	Mem_Fill(&uiVideo, 0, sizeof(uiVideo_t));

	uiVideo.background.generic.id				= ID_BACKGROUND;
	uiVideo.background.generic.type				= QMTYPE_BITMAP;
	uiVideo.background.generic.flags			= QMF_INACTIVE;
	uiVideo.background.generic.x				= 0;
	uiVideo.background.generic.y				= 0;
	uiVideo.background.generic.width			= 1024;
	uiVideo.background.generic.height			= 768;
	uiVideo.background.pic						= ART_BACKGROUND;

	uiVideo.banner.generic.id					= ID_BANNER;
	uiVideo.banner.generic.type					= QMTYPE_BITMAP;
	uiVideo.banner.generic.flags				= QMF_INACTIVE;
	uiVideo.banner.generic.x					= 0;
	uiVideo.banner.generic.y					= 66;
	uiVideo.banner.generic.width				= 1024;
	uiVideo.banner.generic.height				= 46;
	uiVideo.banner.pic							= ART_BANNER;

	uiVideo.textShadow1.generic.id				= ID_TEXTSHADOW1;
	uiVideo.textShadow1.generic.type			= QMTYPE_BITMAP;
	uiVideo.textShadow1.generic.flags			= QMF_INACTIVE;
	uiVideo.textShadow1.generic.x				= 182;
	uiVideo.textShadow1.generic.y				= 170;
	uiVideo.textShadow1.generic.width			= 256;
	uiVideo.textShadow1.generic.height			= 256;
	uiVideo.textShadow1.generic.color			= colorBlack;
	uiVideo.textShadow1.pic						= ART_TEXT1;

	uiVideo.textShadow2.generic.id				= ID_TEXTSHADOW2;
	uiVideo.textShadow2.generic.type			= QMTYPE_BITMAP;
	uiVideo.textShadow2.generic.flags			= QMF_INACTIVE;
	uiVideo.textShadow2.generic.x				= 182;
	uiVideo.textShadow2.generic.y				= 426;
	uiVideo.textShadow2.generic.width			= 256;
	uiVideo.textShadow2.generic.height			= 256;
	uiVideo.textShadow2.generic.color			= colorBlack;
	uiVideo.textShadow2.pic						= ART_TEXT2;

	uiVideo.text1.generic.id					= ID_TEXT1;
	uiVideo.text1.generic.type					= QMTYPE_BITMAP;
	uiVideo.text1.generic.flags					= QMF_INACTIVE;
	uiVideo.text1.generic.x						= 180;
	uiVideo.text1.generic.y						= 168;
	uiVideo.text1.generic.width					= 256;
	uiVideo.text1.generic.height				= 256;
	uiVideo.text1.pic							= ART_TEXT1;

	uiVideo.text2.generic.id					= ID_TEXT2;
	uiVideo.text2.generic.type					= QMTYPE_BITMAP;
	uiVideo.text2.generic.flags					= QMF_INACTIVE;
	uiVideo.text2.generic.x						= 180;
	uiVideo.text2.generic.y						= 424;
	uiVideo.text2.generic.width					= 256;
	uiVideo.text2.generic.height				= 256;
	uiVideo.text2.pic							= ART_TEXT2;

	uiVideo.cancel.generic.id					= ID_CANCEL;
	uiVideo.cancel.generic.type					= QMTYPE_BITMAP;
	uiVideo.cancel.generic.x					= 206;
	uiVideo.cancel.generic.y					= 656;
	uiVideo.cancel.generic.width				= 198;
	uiVideo.cancel.generic.height				= 38;
	uiVideo.cancel.generic.callback				= UI_Video_Callback;
	uiVideo.cancel.generic.ownerdraw			= UI_Video_Ownerdraw;
	uiVideo.cancel.pic							= UI_CANCELBUTTON;

	uiVideo.advanced.generic.id					= ID_ADVANCED;
	uiVideo.advanced.generic.type				= QMTYPE_BITMAP;
	uiVideo.advanced.generic.x					= 413;
	uiVideo.advanced.generic.y					= 656;
	uiVideo.advanced.generic.width				= 198;
	uiVideo.advanced.generic.height				= 38;
	uiVideo.advanced.generic.callback			= UI_Video_Callback;
	uiVideo.advanced.generic.ownerdraw			= UI_Video_Ownerdraw;
	uiVideo.advanced.pic						= ART_ADVANCED;

	uiVideo.apply.generic.id					= ID_APPLY;
	uiVideo.apply.generic.type					= QMTYPE_BITMAP;
	uiVideo.apply.generic.x						= 620;
	uiVideo.apply.generic.y						= 656;
	uiVideo.apply.generic.width					= 198;
	uiVideo.apply.generic.height				= 38;
	uiVideo.apply.generic.callback				= UI_Video_Callback;
	uiVideo.apply.generic.ownerdraw				= UI_Video_Ownerdraw;
	uiVideo.apply.pic							= UI_APPLYBUTTON;

	UI_Video_GetConfig();

	UI_AddItem(&uiVideo.menu, (void *)&uiVideo.background);
	UI_AddItem(&uiVideo.menu, (void *)&uiVideo.banner);
	UI_AddItem(&uiVideo.menu, (void *)&uiVideo.textShadow1);
	UI_AddItem(&uiVideo.menu, (void *)&uiVideo.textShadow2);
	UI_AddItem(&uiVideo.menu, (void *)&uiVideo.text1);
	UI_AddItem(&uiVideo.menu, (void *)&uiVideo.text2);
	UI_AddItem(&uiVideo.menu, (void *)&uiVideo.cancel);
	UI_AddItem(&uiVideo.menu, (void *)&uiVideo.advanced);
	UI_AddItem(&uiVideo.menu, (void *)&uiVideo.apply);
}

/*
 ==================
 UI_Video_Precache
 ==================
*/
void UI_Video_Precache (){

	R_RegisterMaterialNoMip(ART_BACKGROUND);
	R_RegisterMaterialNoMip(ART_BANNER);
	R_RegisterMaterialNoMip(ART_TEXT1);
	R_RegisterMaterialNoMip(ART_TEXT2);
	R_RegisterMaterialNoMip(ART_ADVANCED);
}

/*
 ==================
 UI_Video_Menu
 ==================
*/
void UI_Video_Menu (){

	UI_Video_Precache();
	UI_Video_Init();

	UI_Video_UpdateConfig();

	UI_PushMenu(&uiVideo.menu);
}