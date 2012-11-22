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
// ui_gameoptions.c - Game options menu
//


#include "ui_local.h"


#define ART_BACKGROUND				"ui/assets/ui/ui_sub_options"
#define ART_BANNER					"ui/assets/banners/gameoptions_t"
#define ART_TEXT1					"ui/assets/text/gameoptions_text_p1"
#define ART_TEXT2					"ui/assets/text/gameoptions_text_p2"

typedef enum {
	ID_BACKGROUND,
	ID_BANNER,

	ID_TEXT1,
	ID_TEXT2,
	ID_TEXTSHADOW1,
	ID_TEXTSHADOW2,

	ID_BACK,

	ID_CROSSHAIR,
	ID_PREVCROSSHAIR,
	ID_NEXTCROSSHAIR,
	ID_CROSSHAIRSIZE,
	ID_IDENTIFYTARGET,
	ID_MAXFPS,
	ID_ENHANCEDHUD,
	ID_FOV,
	ID_HAND,
	ID_ALLOWDOWNLOAD
} uiGameOptionsId_t;

static const char *			uiGameOptionsYesNo[] = {
	"False",
	"True"
};
static const char *			uiGameOptionsHand[] = {
	"Right",
	"Left",
	"Hidden"
};

typedef struct {
	int						currentCrosshair;
	int						currentColor;

	char					hudNames[32][16];
	int						numHuds;

	menuFramework_t			menu;

	menuBitmap_t			background;
	menuBitmap_t			banner;

	menuBitmap_t			textShadow1;
	menuBitmap_t			textShadow2;
	menuBitmap_t			text1;
	menuBitmap_t			text2;

	menuBitmap_t			back;

	menuBitmap_t			crosshair;
	menuBitmap_t			prevCrosshair;
	menuBitmap_t			nextCrosshair;
	menuField_t				crosshairSize;
	menuSpinControl_t		identifyTarget;
	menuField_t				maxFPS;
	menuSpinControl_t		enhancedHUD;
	menuField_t				fov;
	menuSpinControl_t		hand;
	menuSpinControl_t		allowDownload;
} uiGameOptions_t;

static uiGameOptions_t		uiGameOptions;


/*
 ==================
 UI_GameOptions_GetConfig
 ==================
*/
static void UI_GameOptions_GetConfig (){

	uiGameOptions.currentCrosshair = CVar_GetVariableInteger("cl_drawCrosshair");
	if (uiGameOptions.currentCrosshair < 0)
		uiGameOptions.currentCrosshair = 0;
	else if (uiGameOptions.currentCrosshair > NUM_CROSSHAIRS)
		uiGameOptions.currentCrosshair = NUM_CROSSHAIRS;

	uiGameOptions.currentColor = CVar_GetVariableInteger("cl_crosshairColor");
	if (uiGameOptions.currentColor < 0)
		uiGameOptions.currentColor = 0;
	else if (uiGameOptions.currentColor > COLOR_MASK)
		uiGameOptions.currentColor = COLOR_MASK;

	Str_SPrintf(uiGameOptions.crosshairSize.buffer, sizeof(uiGameOptions.crosshairSize.buffer), "%i", CVar_GetVariableInteger("cl_crosshairSize"));

	if (CVar_GetVariableInteger("cl_crosshairNames"))
		uiGameOptions.identifyTarget.curValue = 1;

	Str_SPrintf(uiGameOptions.maxFPS.buffer, sizeof(uiGameOptions.maxFPS.buffer), "%i", CVar_GetVariableInteger("cl_maxFPS"));

	if (CVar_GetVariableInteger("cl_newHUD"))
		uiGameOptions.enhancedHUD.curValue = 1;

	Str_SPrintf(uiGameOptions.fov.buffer, sizeof(uiGameOptions.fov.buffer), "%i", CVar_GetVariableInteger("fov"));

	uiGameOptions.hand.curValue = CVar_GetVariableInteger("hand");
	if (uiGameOptions.hand.curValue < 0)
		uiGameOptions.hand.curValue = 0;
	else if (uiGameOptions.hand.curValue > 2)
		uiGameOptions.hand.curValue = 2;

	if (CVar_GetVariableInteger("cl_allowDownload"))
		uiGameOptions.allowDownload.curValue = 1;
}

/*
 ==================
 UI_GameOptions_SetConfig
 ==================
*/
static void UI_GameOptions_SetConfig (){

	CVar_SetVariableInteger("cl_drawCrosshair", uiGameOptions.currentCrosshair, false);

	CVar_SetVariableInteger("cl_crosshairColor", uiGameOptions.currentColor, false);

	if (atoi(uiGameOptions.crosshairSize.buffer) >= 0)
		CVar_SetVariableInteger("cl_crosshairSize", atoi(uiGameOptions.crosshairSize.buffer), false);

	CVar_SetVariableInteger("cl_crosshairNames", (int)uiGameOptions.identifyTarget.curValue, false);

	if (atoi(uiGameOptions.maxFPS.buffer) >= 0)
		CVar_SetVariableInteger("cl_maxFPS", atoi(uiGameOptions.maxFPS.buffer), false);

	CVar_SetVariableInteger("cl_newHUD", (int)uiGameOptions.enhancedHUD.curValue, false);

	if (atoi(uiGameOptions.fov.buffer) >= 1 && atoi(uiGameOptions.fov.buffer) <= 179)
		CVar_SetVariableInteger("fov", atoi(uiGameOptions.fov.buffer), false);

	CVar_SetVariableInteger("hand", (int)uiGameOptions.hand.curValue, false);

	CVar_SetVariableInteger("cl_allowDownload", (int)uiGameOptions.allowDownload.curValue, false);
}

/*
 ==================
 UI_GameOptions_UpdateConfig
 ==================
*/
static void UI_GameOptions_UpdateConfig (){

	uiGameOptions.identifyTarget.generic.name = uiGameOptionsYesNo[(int)uiGameOptions.identifyTarget.curValue];

	uiGameOptions.enhancedHUD.generic.name = uiGameOptionsYesNo[(int)uiGameOptions.enhancedHUD.curValue];

	uiGameOptions.hand.generic.name = uiGameOptionsHand[(int)uiGameOptions.hand.curValue];

	uiGameOptions.allowDownload.generic.name = uiGameOptionsYesNo[(int)uiGameOptions.allowDownload.curValue];
}

/*
 ==================
 UI_GameOptions_Callback
 ==================
*/
static void UI_GameOptions_Callback (void *self, int event){

	menuCommon_t	*item = (menuCommon_t *)self;

	if (event == QM_CHANGED){
		UI_GameOptions_UpdateConfig();
		UI_GameOptions_SetConfig();
		return;
	}

	if (event != QM_ACTIVATED)
		return;

	switch (item->id){
	case ID_BACK:
		UI_PopMenu();

		break;
	case ID_CROSSHAIR:
		uiGameOptions.currentColor++;
		if (uiGameOptions.currentColor > COLOR_MASK)
			uiGameOptions.currentColor = 0;

		UI_GameOptions_UpdateConfig();
		UI_GameOptions_SetConfig();

		break;
	case ID_PREVCROSSHAIR:
		uiGameOptions.currentCrosshair--;
		if (uiGameOptions.currentCrosshair < 0)
			uiGameOptions.currentCrosshair = NUM_CROSSHAIRS;

		UI_GameOptions_UpdateConfig();
		UI_GameOptions_SetConfig();

		break;
	case ID_NEXTCROSSHAIR:
		uiGameOptions.currentCrosshair++;
		if (uiGameOptions.currentCrosshair > NUM_CROSSHAIRS)
			uiGameOptions.currentCrosshair = 0;

		UI_GameOptions_UpdateConfig();
		UI_GameOptions_SetConfig();

		break;
	}
}

/*
 ==================
 UI_GameOptions_Ownerdraw
 ==================
*/
static void UI_GameOptions_Ownerdraw (void *self){

	menuCommon_t	*item = (menuCommon_t *)self;

	if (self == (void *)&uiGameOptions.crosshair){
		if (uiGameOptions.currentCrosshair >= 1 && uiGameOptions.currentCrosshair <= NUM_CROSSHAIRS){
			vec4_t	color;

			VectorCopy(color_table[Clamp(CVar_GetVariableInteger("cl_crosshairColor"), 0, 63)], color);
			color[3] = 1.0f * Clamp(CVar_GetVariableFloat("cl_crosshairAlpha"), 0.0f, 1.0f);

			UI_DrawPic(item->x, item->y, item->width, item->height, color, Str_VarArgs("pics/crosshair%i", uiGameOptions.currentCrosshair));
		}
	}
	else {
		if (uiGameOptions.menu.items[uiGameOptions.menu.cursor] == self)
			UI_DrawPic(item->x, item->y, item->width, item->height, colorWhite, UI_MOVEBOXFOCUS);
		else
			UI_DrawPic(item->x, item->y, item->width, item->height, colorWhite, UI_MOVEBOX);

		UI_DrawPic(item->x, item->y, item->width, item->height, colorWhite, ((menuBitmap_t *)self)->pic);
	}
}

/*
 ==================
 UI_GameOptions_Init
 ==================
*/
static void UI_GameOptions_Init (){

	Mem_Fill(&uiGameOptions, 0, sizeof(uiGameOptions_t));

	uiGameOptions.background.generic.id			= ID_BACKGROUND;
	uiGameOptions.background.generic.type		= QMTYPE_BITMAP;
	uiGameOptions.background.generic.flags		= QMF_INACTIVE;
	uiGameOptions.background.generic.x			= 0;
	uiGameOptions.background.generic.y			= 0;
	uiGameOptions.background.generic.width		= 1024;
	uiGameOptions.background.generic.height		= 768;
	uiGameOptions.background.pic				= ART_BACKGROUND;

	uiGameOptions.banner.generic.id				= ID_BANNER;
	uiGameOptions.banner.generic.type			= QMTYPE_BITMAP;
	uiGameOptions.banner.generic.flags			= QMF_INACTIVE;
	uiGameOptions.banner.generic.x				= 0;
	uiGameOptions.banner.generic.y				= 66;
	uiGameOptions.banner.generic.width			= 1024;
	uiGameOptions.banner.generic.height			= 46;
	uiGameOptions.banner.pic					= ART_BANNER;

	uiGameOptions.textShadow1.generic.id		= ID_TEXTSHADOW1;
	uiGameOptions.textShadow1.generic.type		= QMTYPE_BITMAP;
	uiGameOptions.textShadow1.generic.flags		= QMF_INACTIVE;
	uiGameOptions.textShadow1.generic.x			= 182;
	uiGameOptions.textShadow1.generic.y			= 170;
	uiGameOptions.textShadow1.generic.width		= 256;
	uiGameOptions.textShadow1.generic.height	= 256;
	uiGameOptions.textShadow1.generic.color		= colorBlack;
	uiGameOptions.textShadow1.pic				= ART_TEXT1;

	uiGameOptions.textShadow2.generic.id		= ID_TEXTSHADOW2;
	uiGameOptions.textShadow2.generic.type		= QMTYPE_BITMAP;
	uiGameOptions.textShadow2.generic.flags		= QMF_INACTIVE;
	uiGameOptions.textShadow2.generic.x			= 182;
	uiGameOptions.textShadow2.generic.y			= 426;
	uiGameOptions.textShadow2.generic.width		= 256;
	uiGameOptions.textShadow2.generic.height	= 256;
	uiGameOptions.textShadow2.generic.color		= colorBlack;
	uiGameOptions.textShadow2.pic				= ART_TEXT2;

	uiGameOptions.text1.generic.id				= ID_TEXT1;
	uiGameOptions.text1.generic.type			= QMTYPE_BITMAP;
	uiGameOptions.text1.generic.flags			= QMF_INACTIVE;
	uiGameOptions.text1.generic.x				= 180;
	uiGameOptions.text1.generic.y				= 168;
	uiGameOptions.text1.generic.width			= 256;
	uiGameOptions.text1.generic.height			= 256;
	uiGameOptions.text1.pic						= ART_TEXT1;

	uiGameOptions.text2.generic.id				= ID_TEXT2;
	uiGameOptions.text2.generic.type			= QMTYPE_BITMAP;
	uiGameOptions.text2.generic.flags			= QMF_INACTIVE;
	uiGameOptions.text2.generic.x				= 180;
	uiGameOptions.text2.generic.y				= 424;
	uiGameOptions.text2.generic.width			= 256;
	uiGameOptions.text2.generic.height			= 256;
	uiGameOptions.text2.pic						= ART_TEXT2;

	uiGameOptions.back.generic.id				= ID_BACK;
	uiGameOptions.back.generic.type				= QMTYPE_BITMAP;
	uiGameOptions.back.generic.x				= 413;
	uiGameOptions.back.generic.y				= 656;
	uiGameOptions.back.generic.width			= 198;
	uiGameOptions.back.generic.height			= 38;
	uiGameOptions.back.generic.callback			= UI_GameOptions_Callback;
	uiGameOptions.back.generic.ownerdraw		= UI_GameOptions_Ownerdraw;
	uiGameOptions.back.pic						= UI_BACKBUTTON;

	uiGameOptions.crosshair.generic.id			= ID_CROSSHAIR;
	uiGameOptions.crosshair.generic.type		= QMTYPE_BITMAP;
	uiGameOptions.crosshair.generic.flags		= 0;
	uiGameOptions.crosshair.generic.x			= 646;
	uiGameOptions.crosshair.generic.y			= 144;
	uiGameOptions.crosshair.generic.width		= 64;
	uiGameOptions.crosshair.generic.height		= 64;
	uiGameOptions.crosshair.generic.callback	= UI_GameOptions_Callback;
	uiGameOptions.crosshair.generic.ownerdraw	= UI_GameOptions_Ownerdraw;
	uiGameOptions.crosshair.generic.statusText	= "Click to change crosshair color";

	uiGameOptions.prevCrosshair.generic.id		= ID_PREVCROSSHAIR;
	uiGameOptions.prevCrosshair.generic.type	= QMTYPE_BITMAP;
	uiGameOptions.prevCrosshair.generic.flags	= QMF_PULSEIFFOCUS;
	uiGameOptions.prevCrosshair.generic.x		= 534;
	uiGameOptions.prevCrosshair.generic.y		= 160;
	uiGameOptions.prevCrosshair.generic.width	= 30;
	uiGameOptions.prevCrosshair.generic.height	= 30;
	uiGameOptions.prevCrosshair.generic.callback	= UI_GameOptions_Callback;
	uiGameOptions.prevCrosshair.generic.statusText	= "Select your crosshair graphic";
	uiGameOptions.prevCrosshair.pic				= UI_LEFTARROW;
	uiGameOptions.prevCrosshair.focusPic		= UI_LEFTARROWFOCUS;

	uiGameOptions.nextCrosshair.generic.id		= ID_NEXTCROSSHAIR;
	uiGameOptions.nextCrosshair.generic.type	= QMTYPE_BITMAP;
	uiGameOptions.nextCrosshair.generic.flags	= QMF_PULSEIFFOCUS;
	uiGameOptions.nextCrosshair.generic.x		= 794;
	uiGameOptions.nextCrosshair.generic.y		= 160;
	uiGameOptions.nextCrosshair.generic.width	= 30;
	uiGameOptions.nextCrosshair.generic.height	= 30;
	uiGameOptions.nextCrosshair.generic.callback	= UI_GameOptions_Callback;
	uiGameOptions.nextCrosshair.generic.statusText	= "Select your crosshair graphic";
	uiGameOptions.nextCrosshair.pic				= UI_RIGHTARROW;
	uiGameOptions.nextCrosshair.focusPic		= UI_RIGHTARROWFOCUS;

	uiGameOptions.crosshairSize.generic.id		= ID_CROSSHAIRSIZE;
	uiGameOptions.crosshairSize.generic.type	= QMTYPE_FIELD;
	uiGameOptions.crosshairSize.generic.flags	= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW | QMF_NUMBERSONLY;
	uiGameOptions.crosshairSize.generic.x		= 580;
	uiGameOptions.crosshairSize.generic.y		= 224;
	uiGameOptions.crosshairSize.generic.width	= 198;
	uiGameOptions.crosshairSize.generic.height	= 30;
	uiGameOptions.crosshairSize.generic.callback	= UI_GameOptions_Callback;
	uiGameOptions.crosshairSize.generic.statusText	= "Set your crosshair size";
	uiGameOptions.crosshairSize.maxLenght		= 3;

	uiGameOptions.identifyTarget.generic.id		= ID_IDENTIFYTARGET;
	uiGameOptions.identifyTarget.generic.type	= QMTYPE_SPINCONTROL;
	uiGameOptions.identifyTarget.generic.flags	= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiGameOptions.identifyTarget.generic.x		= 580;
	uiGameOptions.identifyTarget.generic.y		= 256;
	uiGameOptions.identifyTarget.generic.width	= 198;
	uiGameOptions.identifyTarget.generic.height	= 30;
	uiGameOptions.identifyTarget.generic.callback	= UI_GameOptions_Callback;
	uiGameOptions.identifyTarget.generic.statusText	= "Show target player names";
	uiGameOptions.identifyTarget.minValue		= 0;
	uiGameOptions.identifyTarget.maxValue		= 1;
	uiGameOptions.identifyTarget.range			= 1;

	uiGameOptions.maxFPS.generic.id				= ID_MAXFPS;
	uiGameOptions.maxFPS.generic.type			= QMTYPE_FIELD;
	uiGameOptions.maxFPS.generic.flags			= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW | QMF_NUMBERSONLY;
	uiGameOptions.maxFPS.generic.x				= 580;
	uiGameOptions.maxFPS.generic.y				= 288;
	uiGameOptions.maxFPS.generic.width			= 198;
	uiGameOptions.maxFPS.generic.height			= 30;
	uiGameOptions.maxFPS.generic.callback		= UI_GameOptions_Callback;
	uiGameOptions.maxFPS.generic.statusText		= "Cap your game frame rate";
	uiGameOptions.maxFPS.maxLenght				= 3;

	uiGameOptions.enhancedHUD.generic.id		= ID_ENHANCEDHUD;
	uiGameOptions.enhancedHUD.generic.type		= QMTYPE_SPINCONTROL;
	uiGameOptions.enhancedHUD.generic.flags		= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiGameOptions.enhancedHUD.generic.x			= 580;
	uiGameOptions.enhancedHUD.generic.y			= 320;
	uiGameOptions.enhancedHUD.generic.width		= 198;
	uiGameOptions.enhancedHUD.generic.height	= 30;
	uiGameOptions.enhancedHUD.generic.callback	= UI_GameOptions_Callback;
	uiGameOptions.enhancedHUD.generic.statusText	= "Enable/Disable new HUD layout";
	uiGameOptions.enhancedHUD.minValue			= 0;
	uiGameOptions.enhancedHUD.maxValue			= 1;
	uiGameOptions.enhancedHUD.range				= 1;
	
	uiGameOptions.fov.generic.id				= ID_FOV;
	uiGameOptions.fov.generic.type				= QMTYPE_FIELD;
	uiGameOptions.fov.generic.flags				= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW | QMF_NUMBERSONLY;
	uiGameOptions.fov.generic.x					= 580;
	uiGameOptions.fov.generic.y					= 352;
	uiGameOptions.fov.generic.width				= 198;
	uiGameOptions.fov.generic.height			= 30;
	uiGameOptions.fov.generic.callback			= UI_GameOptions_Callback;
	uiGameOptions.fov.generic.statusText		= "Set your field of view";
	uiGameOptions.fov.maxLenght					= 3;

	uiGameOptions.hand.generic.id				= ID_HAND;
	uiGameOptions.hand.generic.type				= QMTYPE_SPINCONTROL;
	uiGameOptions.hand.generic.flags			= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiGameOptions.hand.generic.x				= 580;
	uiGameOptions.hand.generic.y				= 384;
	uiGameOptions.hand.generic.width			= 198;
	uiGameOptions.hand.generic.height			= 30;
	uiGameOptions.hand.generic.callback			= UI_GameOptions_Callback;
	uiGameOptions.hand.generic.statusText		= "Choose your gun hand preference";
	uiGameOptions.hand.minValue					= 0;
	uiGameOptions.hand.maxValue					= 2;
	uiGameOptions.hand.range					= 1;

	uiGameOptions.allowDownload.generic.id		= ID_ALLOWDOWNLOAD;
	uiGameOptions.allowDownload.generic.type	= QMTYPE_SPINCONTROL;
	uiGameOptions.allowDownload.generic.flags	= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiGameOptions.allowDownload.generic.x		= 580;
	uiGameOptions.allowDownload.generic.y		= 416;
	uiGameOptions.allowDownload.generic.width	= 198;
	uiGameOptions.allowDownload.generic.height	= 30;
	uiGameOptions.allowDownload.generic.callback	= UI_GameOptions_Callback;
	uiGameOptions.allowDownload.generic.statusText	= "Allow download of files from servers";
	uiGameOptions.allowDownload.minValue		= 0;
	uiGameOptions.allowDownload.maxValue		= 1;
	uiGameOptions.allowDownload.range			= 1;

	UI_GameOptions_GetConfig();

	UI_AddItem(&uiGameOptions.menu, (void *)&uiGameOptions.background);
	UI_AddItem(&uiGameOptions.menu, (void *)&uiGameOptions.banner);
	UI_AddItem(&uiGameOptions.menu, (void *)&uiGameOptions.textShadow1);
	UI_AddItem(&uiGameOptions.menu, (void *)&uiGameOptions.textShadow2);
	UI_AddItem(&uiGameOptions.menu, (void *)&uiGameOptions.text1);
	UI_AddItem(&uiGameOptions.menu, (void *)&uiGameOptions.text2);
	UI_AddItem(&uiGameOptions.menu, (void *)&uiGameOptions.back);
	UI_AddItem(&uiGameOptions.menu, (void *)&uiGameOptions.crosshair);
	UI_AddItem(&uiGameOptions.menu, (void *)&uiGameOptions.prevCrosshair);
	UI_AddItem(&uiGameOptions.menu, (void *)&uiGameOptions.nextCrosshair);
	UI_AddItem(&uiGameOptions.menu, (void *)&uiGameOptions.crosshairSize);
	UI_AddItem(&uiGameOptions.menu, (void *)&uiGameOptions.identifyTarget);
	UI_AddItem(&uiGameOptions.menu, (void *)&uiGameOptions.maxFPS);
	UI_AddItem(&uiGameOptions.menu, (void *)&uiGameOptions.enhancedHUD);
	UI_AddItem(&uiGameOptions.menu, (void *)&uiGameOptions.fov);
	UI_AddItem(&uiGameOptions.menu, (void *)&uiGameOptions.hand);
	UI_AddItem(&uiGameOptions.menu, (void *)&uiGameOptions.allowDownload);
}

/*
 ==================
 UI_GameOptions_Precache
 ==================
*/
void UI_GameOptions_Precache (){

	R_RegisterMaterialNoMip(ART_BACKGROUND);
	R_RegisterMaterialNoMip(ART_BANNER);
	R_RegisterMaterialNoMip(ART_TEXT1);
	R_RegisterMaterialNoMip(ART_TEXT2);
}

/*
 ==================
 UI_GameOptions_Menu
 ==================
*/
void UI_GameOptions_Menu (){

	UI_GameOptions_Precache();
	UI_GameOptions_Init();

	UI_GameOptions_UpdateConfig();

	UI_PushMenu(&uiGameOptions.menu);
}