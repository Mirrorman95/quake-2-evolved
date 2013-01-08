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
// ui_main.c - Main UI
//

// TODO:
// - add a "update" option


#include "ui_local.h"


#define ART_BACKGROUND				"ui/assets/title_screen/title_backg"
#define ART_LOGO					"ui/assets/title_screen/q2e_logo"
#define ART_SINGLEPLAYER			"ui/assets/title_screen/singleplayer_n"
#define ART_SINGLEPLAYER2			"ui/assets/title_screen/singleplayer_s"
#define ART_MULTIPLAYER				"ui/assets/title_screen/multiplayer_n"
#define ART_MULTIPLAYER2			"ui/assets/title_screen/multiplayer_s"
#define ART_OPTIONS					"ui/assets/title_screen/options_n"
#define ART_OPTIONS2				"ui/assets/title_screen/options_s"
#define ART_CINEMATICS				"ui/assets/title_screen/cinematics_n"
#define ART_CINEMATICS2				"ui/assets/title_screen/cinematics_s"
#define ART_DEMOS					"ui/assets/title_screen/demos_n"
#define ART_DEMOS2					"ui/assets/title_screen/demos_s"
#define ART_MODS					"ui/assets/title_screen/mods_n"
#define ART_MODS2					"ui/assets/title_screen/mods_s"
#define ART_QUIT					"ui/assets/title_screen/quit_n"
#define ART_QUIT2					"ui/assets/title_screen/quit_s"
#define ART_IDLOGO					"ui/assets/title_screen/logo_id"
#define ART_IDLOGO2					"ui/assets/title_screen/logo_id_s"
#define ART_BLURLOGO				"ui/assets/title_screen/logo_blur"
#define ART_BLURLOGO2				"ui/assets/title_screen/logo_blur_s"
#define ART_COPYRIGHT				"ui/assets/title_screen/copyrights"

typedef enum {
	ID_BACKGROUND,
	ID_LOGO,

	ID_SINGLEPLAYER,
	ID_MULTIPLAYER,
	ID_OPTIONS,
	ID_CINEMATICS,
	ID_DEMOS,
	ID_MODS,
	ID_QUIT,

	ID_IDSOFTWARE,
	ID_TEAMBLUR,
	ID_COPYRIGHT,

	ID_ALLYOURBASE
} uiMainId_t;

typedef struct {
	menuFramework_t			menu;

	menuBitmap_t			background;
	menuBitmap_t			logo;

	menuBitmap_t			singlePlayer;
	menuBitmap_t			multiPlayer;
	menuBitmap_t			options;
	menuBitmap_t			cinematics;
	menuBitmap_t			demos;
	menuBitmap_t			mods;
	menuBitmap_t			quit;

	menuBitmap_t			idSoftware;
	menuBitmap_t			teamBlur;
	menuBitmap_t			copyright;

	menuBitmap_t			allYourBase;
} uiMain_t;

static uiMain_t				uiMain;

static void UI_AllYourBase_Menu ();


/*
 ==================
 UI_Main_KeyFunc
 ==================
*/
static const char *UI_Main_KeyFunc (int key){

	switch (key){
	case K_ESCAPE:
	case K_MOUSE2:
		if (cls.state == CA_ACTIVE){
			// This shouldn't be happening, but for some reason it is in
			// some mods
			UI_CloseMenu();
			return uiSoundNull;
		}

		return uiSoundNull;
	}

	return UI_DefaultKey(&uiMain.menu, key);
}

/*
 ==================
 UI_Main_Callback
 ==================
*/
static void UI_Main_Callback (void *self, int event){

	menuCommon_t	*item = (menuCommon_t *)self;

	if (event != QM_ACTIVATED)
		return;

	switch (item->id){
	case ID_SINGLEPLAYER:
		UI_SinglePlayer_Menu();
		break;
	case ID_MULTIPLAYER:
		UI_MultiPlayer_Menu();
		break;
	case ID_OPTIONS:
		UI_Options_Menu();
		break;
	case ID_CINEMATICS:
		UI_Cinematics_Menu();
		break;
	case ID_DEMOS:
		UI_Demos_Menu();
		break;
	case ID_MODS:
		UI_Mods_Menu();
		break;
	case ID_QUIT:
		UI_Quit_Menu();
		break;
	case ID_IDSOFTWARE:
		UI_GoToSite_Menu("http://www.idsoftware.com");
		break;
	case ID_TEAMBLUR:
		UI_GoToSite_Menu("http://www.team-blur-games.com/");
		break;
	case ID_ALLYOURBASE:
		UI_AllYourBase_Menu();
		break;
	}
}

/*
 ==================
 UI_Main_Init
 ==================
*/
static void UI_Main_Init (){

	Mem_Fill(&uiMain, 0, sizeof(uiMain_t));

	uiMain.menu.keyFunc							= UI_Main_KeyFunc;

	uiMain.background.generic.id				= ID_BACKGROUND;
	uiMain.background.generic.type				= QMTYPE_BITMAP;
	uiMain.background.generic.flags				= QMF_INACTIVE;
	uiMain.background.generic.x					= 0;
	uiMain.background.generic.y					= 0;
	uiMain.background.generic.width				= 1024;
	uiMain.background.generic.height			= 768;
	uiMain.background.pic						= ART_BACKGROUND;

	uiMain.logo.generic.id						= ID_LOGO;
	uiMain.logo.generic.type					= QMTYPE_BITMAP;
	uiMain.logo.generic.flags					= QMF_INACTIVE;
	uiMain.logo.generic.x						= 0;
	uiMain.logo.generic.y						= 0;
	uiMain.logo.generic.width					= 1024;
	uiMain.logo.generic.height					= 256;
	uiMain.logo.pic								= ART_LOGO;

	uiMain.singlePlayer.generic.id				= ID_SINGLEPLAYER;
	uiMain.singlePlayer.generic.type			= QMTYPE_BITMAP;
	uiMain.singlePlayer.generic.flags			= QMF_PULSEIFFOCUS | QMF_FOCUSBEHIND;
	uiMain.singlePlayer.generic.x				= 285;
	uiMain.singlePlayer.generic.y				= 300;
	uiMain.singlePlayer.generic.width			= 454;
	uiMain.singlePlayer.generic.height			= 42;
	uiMain.singlePlayer.generic.callback		= UI_Main_Callback;
	uiMain.singlePlayer.pic						= ART_SINGLEPLAYER;
	uiMain.singlePlayer.focusPic				= ART_SINGLEPLAYER2;

	uiMain.multiPlayer.generic.id				= ID_MULTIPLAYER;
	uiMain.multiPlayer.generic.type				= QMTYPE_BITMAP;
	uiMain.multiPlayer.generic.flags			= QMF_PULSEIFFOCUS | QMF_FOCUSBEHIND;
	uiMain.multiPlayer.generic.x				= 306;
	uiMain.multiPlayer.generic.y				= 342;
	uiMain.multiPlayer.generic.width			= 412;
	uiMain.multiPlayer.generic.height			= 42;
	uiMain.multiPlayer.generic.callback			= UI_Main_Callback;
	uiMain.multiPlayer.pic						= ART_MULTIPLAYER;
	uiMain.multiPlayer.focusPic					= ART_MULTIPLAYER2;

	uiMain.options.generic.id					= ID_OPTIONS;
	uiMain.options.generic.type					= QMTYPE_BITMAP;
	uiMain.options.generic.flags				= QMF_PULSEIFFOCUS | QMF_FOCUSBEHIND;
	uiMain.options.generic.x					= 367;
	uiMain.options.generic.y					= 384;
	uiMain.options.generic.width				= 290;
	uiMain.options.generic.height				= 42;
	uiMain.options.generic.callback				= UI_Main_Callback;
	uiMain.options.pic							= ART_OPTIONS;
	uiMain.options.focusPic						= ART_OPTIONS2;

	uiMain.cinematics.generic.id				= ID_CINEMATICS;
	uiMain.cinematics.generic.type				= QMTYPE_BITMAP;
	uiMain.cinematics.generic.flags				= QMF_PULSEIFFOCUS | QMF_FOCUSBEHIND;
	uiMain.cinematics.generic.x					= 328;
	uiMain.cinematics.generic.y					= 426;
	uiMain.cinematics.generic.width				= 368;
	uiMain.cinematics.generic.height			= 42;
	uiMain.cinematics.generic.callback			= UI_Main_Callback;
	uiMain.cinematics.pic						= ART_CINEMATICS;
	uiMain.cinematics.focusPic					= ART_CINEMATICS2;

	uiMain.demos.generic.id						= ID_DEMOS;
	uiMain.demos.generic.type					= QMTYPE_BITMAP;
	uiMain.demos.generic.flags					= QMF_PULSEIFFOCUS | QMF_FOCUSBEHIND;
	uiMain.demos.generic.x						= 386;
	uiMain.demos.generic.y						= 468;
	uiMain.demos.generic.width					= 252;
	uiMain.demos.generic.height					= 42;
	uiMain.demos.generic.callback				= UI_Main_Callback;
	uiMain.demos.pic							= ART_DEMOS;
	uiMain.demos.focusPic						= ART_DEMOS2;

	uiMain.mods.generic.id						= ID_MODS;
	uiMain.mods.generic.type					= QMTYPE_BITMAP;
	uiMain.mods.generic.flags					= QMF_PULSEIFFOCUS | QMF_FOCUSBEHIND;
	uiMain.mods.generic.x						= 291;
	uiMain.mods.generic.y						= 510;
	uiMain.mods.generic.width					= 442;
	uiMain.mods.generic.height					= 42;
	uiMain.mods.generic.callback				= UI_Main_Callback;
	uiMain.mods.pic								= ART_MODS;
	uiMain.mods.focusPic						= ART_MODS2;

	uiMain.quit.generic.id						= ID_QUIT;
	uiMain.quit.generic.type					= QMTYPE_BITMAP;
	uiMain.quit.generic.flags					= QMF_PULSEIFFOCUS | QMF_FOCUSBEHIND;
	uiMain.quit.generic.x						= 415;
	uiMain.quit.generic.y						= 594;
	uiMain.quit.generic.width					= 194;
	uiMain.quit.generic.height					= 42;
	uiMain.quit.generic.callback				= UI_Main_Callback;
	uiMain.quit.pic								= ART_QUIT;
	uiMain.quit.focusPic						= ART_QUIT2;

	uiMain.idSoftware.generic.id				= ID_IDSOFTWARE;
	uiMain.idSoftware.generic.type				= QMTYPE_BITMAP;
	uiMain.idSoftware.generic.flags				= QMF_PULSEIFFOCUS | QMF_MOUSEONLY;
	uiMain.idSoftware.generic.x					= 0;
	uiMain.idSoftware.generic.y					= 640;
	uiMain.idSoftware.generic.width				= 128;
	uiMain.idSoftware.generic.height			= 128;
	uiMain.idSoftware.generic.callback			= UI_Main_Callback;
	uiMain.idSoftware.pic						= ART_IDLOGO;
	uiMain.idSoftware.focusPic					= ART_IDLOGO2;

	uiMain.teamBlur.generic.id					= ID_TEAMBLUR;
	uiMain.teamBlur.generic.type				= QMTYPE_BITMAP;
	uiMain.teamBlur.generic.flags				= QMF_PULSEIFFOCUS | QMF_MOUSEONLY;
	uiMain.teamBlur.generic.x					= 896;
	uiMain.teamBlur.generic.y					= 640;
	uiMain.teamBlur.generic.width				= 128;
	uiMain.teamBlur.generic.height				= 128;
	uiMain.teamBlur.generic.callback			= UI_Main_Callback;
	uiMain.teamBlur.pic							= ART_BLURLOGO;
	uiMain.teamBlur.focusPic					= ART_BLURLOGO2;

	uiMain.copyright.generic.id					= ID_COPYRIGHT;
	uiMain.copyright.generic.type				= QMTYPE_BITMAP;
	uiMain.copyright.generic.flags				= QMF_INACTIVE;
	uiMain.copyright.generic.x					= 240;
	uiMain.copyright.generic.y					= 680;
	uiMain.copyright.generic.width				= 544;
	uiMain.copyright.generic.height				= 48;
	uiMain.copyright.pic						= ART_COPYRIGHT;

	uiMain.allYourBase.generic.id				= ID_ALLYOURBASE;
	uiMain.allYourBase.generic.type				= QMTYPE_BITMAP;
	uiMain.allYourBase.generic.flags			= QMF_MOUSEONLY;
	uiMain.allYourBase.generic.x				= 416;
	uiMain.allYourBase.generic.y				= 158;
	uiMain.allYourBase.generic.width			= 37;
	uiMain.allYourBase.generic.height			= 35;
	uiMain.allYourBase.generic.callback			= UI_Main_Callback;

	UI_AddItem(&uiMain.menu, (void *)&uiMain.background);
	UI_AddItem(&uiMain.menu, (void *)&uiMain.logo);
	UI_AddItem(&uiMain.menu, (void *)&uiMain.singlePlayer);
	UI_AddItem(&uiMain.menu, (void *)&uiMain.multiPlayer);
	UI_AddItem(&uiMain.menu, (void *)&uiMain.options);
	UI_AddItem(&uiMain.menu, (void *)&uiMain.cinematics);
	UI_AddItem(&uiMain.menu, (void *)&uiMain.demos);
	UI_AddItem(&uiMain.menu, (void *)&uiMain.mods);
	UI_AddItem(&uiMain.menu, (void *)&uiMain.quit);
	UI_AddItem(&uiMain.menu, (void *)&uiMain.idSoftware);
	UI_AddItem(&uiMain.menu, (void *)&uiMain.teamBlur);
	UI_AddItem(&uiMain.menu, (void *)&uiMain.copyright);
	UI_AddItem(&uiMain.menu, (void *)&uiMain.allYourBase);
}

/*
 ==================
 UI_Main_Precache
 ==================
*/
void UI_Main_Precache (){

	R_RegisterMaterialNoMip(ART_BACKGROUND);
	R_RegisterMaterialNoMip(ART_LOGO);
	R_RegisterMaterialNoMip(ART_SINGLEPLAYER);
	R_RegisterMaterialNoMip(ART_SINGLEPLAYER2);
	R_RegisterMaterialNoMip(ART_MULTIPLAYER);
	R_RegisterMaterialNoMip(ART_MULTIPLAYER2);
	R_RegisterMaterialNoMip(ART_OPTIONS);
	R_RegisterMaterialNoMip(ART_OPTIONS2);
	R_RegisterMaterialNoMip(ART_CINEMATICS);
	R_RegisterMaterialNoMip(ART_CINEMATICS2);
	R_RegisterMaterialNoMip(ART_DEMOS);
	R_RegisterMaterialNoMip(ART_DEMOS2);
	R_RegisterMaterialNoMip(ART_MODS);
	R_RegisterMaterialNoMip(ART_MODS2);
	R_RegisterMaterialNoMip(ART_QUIT);
	R_RegisterMaterialNoMip(ART_QUIT2);
	R_RegisterMaterialNoMip(ART_IDLOGO);
	R_RegisterMaterialNoMip(ART_IDLOGO2);
	R_RegisterMaterialNoMip(ART_BLURLOGO);
	R_RegisterMaterialNoMip(ART_BLURLOGO2);
	R_RegisterMaterialNoMip(ART_COPYRIGHT);
}

/*
 ==================
 UI_Main_Menu
 ==================
*/
void UI_Main_Menu (){

	if (cls.state == CA_ACTIVE){
		// This shouldn't be happening, but for some reason it is in
		// some mods
		UI_InGame_Menu();
		return;
	}

	UI_Main_Precache();
	UI_Main_Init();

	UI_PushMenu(&uiMain.menu);
}


// ============================================================================

#define ART_BANNER					"ui/assets/banners/aybabtu_t"
#define ART_MOO						"ui/assets/misc/moo"
#define SOUND_MOO					"ui/moo"

#define ID_BANNER					1
#define ID_BACK						2
#define ID_MOO						3

typedef struct {
	menuFramework_t			menu;

	menuBitmap_t			background;
	menuBitmap_t			banner;

	menuBitmap_t			back;

	menuBitmap_t			moo;
} uiAllYourBase_t;

static uiAllYourBase_t		uiAllYourBase;


/*
 ==================
 UI_AllYourBase_Callback
 ==================
*/
static void UI_AllYourBase_Callback (void *self, int event){

	menuCommon_t	*item = (menuCommon_t *)self;

	if (event != QM_ACTIVATED)
		return;

	switch (item->id){
	case ID_BACK:
		UI_PopMenu();
		break;
	}
}

/*
 ==================
 UI_AllYourBase_Ownerdraw
 ==================
*/
static void UI_AllYourBase_Ownerdraw (void *self){

	menuCommon_t	*item = (menuCommon_t *)self;

	if (uiAllYourBase.menu.items[uiAllYourBase.menu.cursor] == self)
		UI_DrawPicOLD(item->x, item->y, item->width, item->height, colorWhite, UI_MOVEBOXFOCUS);
	else
		UI_DrawPicOLD(item->x, item->y, item->width, item->height, colorWhite, UI_MOVEBOX);

	UI_DrawPicOLD(item->x, item->y, item->width, item->height, colorWhite, ((menuBitmap_t *)self)->pic);
}

/*
 ==================
 UI_AllYourBase_Init
 ==================
*/
static void UI_AllYourBase_Init (void){

	Mem_Fill(&uiAllYourBase, 0, sizeof(uiAllYourBase_t));

	uiAllYourBase.background.generic.id			= ID_BACKGROUND;
	uiAllYourBase.background.generic.type		= QMTYPE_BITMAP;
	uiAllYourBase.background.generic.flags		= QMF_INACTIVE | QMF_GRAYED;
	uiAllYourBase.background.generic.x			= 0;
	uiAllYourBase.background.generic.y			= 0;
	uiAllYourBase.background.generic.width		= 1024;
	uiAllYourBase.background.generic.height		= 768;
	uiAllYourBase.background.pic				= ART_BACKGROUND;

	uiAllYourBase.banner.generic.id				= ID_BANNER;
	uiAllYourBase.banner.generic.type			= QMTYPE_BITMAP;
	uiAllYourBase.banner.generic.flags			= QMF_INACTIVE;
	uiAllYourBase.banner.generic.x				= 0;
	uiAllYourBase.banner.generic.y				= 66;
	uiAllYourBase.banner.generic.width			= 1024;
	uiAllYourBase.banner.generic.height			= 46;
	uiAllYourBase.banner.pic					= ART_BANNER;

	uiAllYourBase.back.generic.id				= ID_BACK;
	uiAllYourBase.back.generic.type				= QMTYPE_BITMAP;
	uiAllYourBase.back.generic.x				= 413;
	uiAllYourBase.back.generic.y				= 656;
	uiAllYourBase.back.generic.width			= 198;
	uiAllYourBase.back.generic.height			= 38;
	uiAllYourBase.back.generic.callback			= UI_AllYourBase_Callback;
	uiAllYourBase.back.generic.ownerdraw		= UI_AllYourBase_Ownerdraw;
	uiAllYourBase.back.pic						= UI_BACKBUTTON;

	uiAllYourBase.moo.generic.id				= ID_MOO;
	uiAllYourBase.moo.generic.type				= QMTYPE_BITMAP;
	uiAllYourBase.moo.generic.flags				= QMF_INACTIVE;
	uiAllYourBase.moo.generic.x					= 256;
	uiAllYourBase.moo.generic.y					= 128;
	uiAllYourBase.moo.generic.width				= 512;
	uiAllYourBase.moo.generic.height			= 512;
	uiAllYourBase.moo.pic						= ART_MOO;

	UI_AddItem(&uiAllYourBase.menu, (void **)&uiAllYourBase.background);
	UI_AddItem(&uiAllYourBase.menu, (void **)&uiAllYourBase.banner);
	UI_AddItem(&uiAllYourBase.menu, (void **)&uiAllYourBase.back);
	UI_AddItem(&uiAllYourBase.menu, (void **)&uiAllYourBase.moo);
}

/*
 ==================
 UI_AllYourBase_Precache
 ==================
*/
static void UI_AllYourBase_Precache (){

	R_RegisterMaterialNoMip(ART_BACKGROUND);
	R_RegisterMaterialNoMip(ART_BANNER);
	R_RegisterMaterialNoMip(ART_MOO);

	S_RegisterSound(SOUND_MOO, 0);
}

/*
 ==================
 UI_AllYourBase_Menu
 ==================
*/
static void UI_AllYourBase_Menu (){

	UI_AllYourBase_Precache();
	UI_AllYourBase_Init();

	UI_PushMenu(&uiAllYourBase.menu);

	UI_StartSound(SOUND_MOO);
}