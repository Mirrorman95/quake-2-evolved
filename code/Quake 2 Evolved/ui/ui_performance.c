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
// ui_performance.c - Performance menu
//


#include "ui_local.h"


#define ART_BACKGROUND				"ui/assets/ui/ui_sub_options"
#define ART_BANNER					"ui/assets/banners/performance_t"
#define ART_TEXT1					"ui/assets/text/performance_text_p1"
#define ART_TEXT2					"ui/assets/text/performance_text_p2"

typedef enum {
	ID_BACKGROUND,
	ID_BANNER,

	ID_TEXT1,
	ID_TEXT2,
	ID_TEXTSHADOW1,
	ID_TEXTSHADOW2,

	ID_BACK,

	ID_SCREENSIZE,
	ID_SYNCEVERYFRAME,
	ID_INGAMEVIDEO,
	ID_DYNAMICLIGHTS,
	ID_SHADOWS,
	ID_CAUSTICS,
	ID_PARTICLES,
	ID_DECALS,
	ID_EJECTINGBRASS,
	ID_SHELLEFFECTS,
	ID_SCREENBLENDS,
	ID_BLOOD
} uiMainId_t;

static const char *			uiPerformanceYesNo[] = {
	"False",
	"True"
};
static const char *			uiPerformanceShadows[] = {
	"None",
	"Planar"
};
static const char *			uiPerformanceDecals[] = {
	"None",
	"Short",
	"Long"
};
static const char *			uiPerformanceEjectBrass[] = {
	"None",
	"Low",
	"High"
};
static const char *			uiPerformanceScreenBlends[] = {
	"None",
	"Flash",
	"Material"
};

typedef struct {
	menuFramework_t			menu;

	menuBitmap_t			background;
	menuBitmap_t			banner;

	menuBitmap_t			textShadow1;
	menuBitmap_t			textShadow2;
	menuBitmap_t			text1;
	menuBitmap_t			text2;

	menuBitmap_t			back;

	menuSpinControl_t		screenSize;
	menuSpinControl_t		syncEveryFrame;
	menuSpinControl_t		inGameVideo;
	menuSpinControl_t		dynamicLights;
	menuSpinControl_t		shadows;
	menuSpinControl_t		caustics;
	menuSpinControl_t		particles;
	menuSpinControl_t		decals;
	menuSpinControl_t		ejectingBrass;
	menuSpinControl_t		shellEffects;
	menuSpinControl_t		screenBlends;
	menuSpinControl_t		blood;
} uiPerformance_t;

static uiPerformance_t		uiPerformance;


/*
 ==================
 UI_Performance_GetConfig
 ==================
*/
static void UI_Performance_GetConfig (){

	uiPerformance.screenSize.curValue = CVar_GetVariableInteger("cl_viewSize");
	if (uiPerformance.screenSize.curValue < 30)
		uiPerformance.screenSize.curValue = 30;
	else if (uiPerformance.screenSize.curValue > 100)
		uiPerformance.screenSize.curValue = 100;

	if (CVar_GetVariableInteger("r_finish"))
		uiPerformance.syncEveryFrame.curValue = 1;

	if (CVar_GetVariableInteger("r_inGameVideo"))
		uiPerformance.inGameVideo.curValue = 1;

	if (CVar_GetVariableInteger("r_dynamicLights"))
		uiPerformance.dynamicLights.curValue = 1;

	uiPerformance.shadows.curValue = CVar_GetVariableInteger("r_shadows");
	if (uiPerformance.shadows.curValue < 0)
		uiPerformance.shadows.curValue = 0;
	else if (uiPerformance.shadows.curValue > 1)
		uiPerformance.shadows.curValue = 1;

	if (CVar_GetVariableInteger("r_caustics"))
		uiPerformance.caustics.curValue = 1;

	if (CVar_GetVariableInteger("cl_particles"))
		uiPerformance.particles.curValue = 1;

	switch (CVar_GetVariableInteger("cl_markTime")){
	case 30000:
		uiPerformance.decals.curValue = 2;
		break;
	case 15000:
		uiPerformance.decals.curValue = 1;
		break;
	default:
		uiPerformance.decals.curValue = 0;
		break;
	}

	switch (CVar_GetVariableInteger("cl_brassTime")){
	case 5000:
		uiPerformance.ejectingBrass.curValue = 2;
		break;
	case 2500:
		uiPerformance.ejectingBrass.curValue = 1;
		break;
	default:
		uiPerformance.ejectingBrass.curValue = 0;
		break;
	}

	if (CVar_GetVariableInteger("cl_drawShells"))
		uiPerformance.shellEffects.curValue = 1;

	uiPerformance.screenBlends.curValue = CVar_GetVariableInteger("cl_viewBlend");
	if (uiPerformance.screenBlends.curValue < 0)
		uiPerformance.screenBlends.curValue = 0;
	else if (uiPerformance.screenBlends.curValue > 2)
		uiPerformance.screenBlends.curValue = 2;

	if (CVar_GetVariableInteger("cl_blood"))
		uiPerformance.blood.curValue = 1;
}

/*
 ==================
 UI_Performance_SetConfig
 ==================
*/
static void UI_Performance_SetConfig (){

	CVar_SetVariableInteger("cl_viewSize", (int)uiPerformance.screenSize.curValue, false);

	CVar_SetVariableInteger("r_finish", (int)uiPerformance.syncEveryFrame.curValue, false);

	CVar_SetVariableInteger("r_inGameVideo", (int)uiPerformance.inGameVideo.curValue, false);

	CVar_SetVariableInteger("r_dynamicLights", (int)uiPerformance.dynamicLights.curValue, false);

	CVar_SetVariableInteger("r_shadows", (int)uiPerformance.shadows.curValue, false);

	CVar_SetVariableInteger("r_caustics", (int)uiPerformance.caustics.curValue, false);

	CVar_SetVariableInteger("cl_particles", (int)uiPerformance.particles.curValue, false);

	CVar_SetVariableInteger("cl_markTime", (int)uiPerformance.decals.curValue * 15000, false);

	CVar_SetVariableInteger("cl_brassTime", (int)uiPerformance.ejectingBrass.curValue * 2500, false);

	CVar_SetVariableInteger("cl_drawShells", (int)uiPerformance.shellEffects.curValue, false);

	CVar_SetVariableInteger("cl_viewBlend", (int)uiPerformance.screenBlends.curValue, false);

	CVar_SetVariableInteger("cl_blood", (int)uiPerformance.blood.curValue, false);
}

/*
 ==================
 UI_Performance_UpdateConfig
 ==================
*/
static void UI_Performance_UpdateConfig (){

	static char	sizeText[32];

	Str_SPrintf(sizeText, sizeof(sizeText), "%i", (int)uiPerformance.screenSize.curValue);
	uiPerformance.screenSize.generic.name = sizeText;

	uiPerformance.syncEveryFrame.generic.name = uiPerformanceYesNo[(int)uiPerformance.syncEveryFrame.curValue];

	uiPerformance.inGameVideo.generic.name = uiPerformanceYesNo[(int)uiPerformance.inGameVideo.curValue];

	uiPerformance.dynamicLights.generic.name = uiPerformanceYesNo[(int)uiPerformance.dynamicLights.curValue];

	uiPerformance.shadows.generic.name = uiPerformanceShadows[(int)uiPerformance.shadows.curValue];

	uiPerformance.caustics.generic.name = uiPerformanceYesNo[(int)uiPerformance.caustics.curValue];

	uiPerformance.particles.generic.name = uiPerformanceYesNo[(int)uiPerformance.particles.curValue];

	uiPerformance.decals.generic.name = uiPerformanceDecals[(int)uiPerformance.decals.curValue];

	uiPerformance.ejectingBrass.generic.name = uiPerformanceEjectBrass[(int)uiPerformance.ejectingBrass.curValue];

	uiPerformance.shellEffects.generic.name = uiPerformanceYesNo[(int)uiPerformance.shellEffects.curValue];

	uiPerformance.screenBlends.generic.name = uiPerformanceScreenBlends[(int)uiPerformance.screenBlends.curValue];

	uiPerformance.blood.generic.name = uiPerformanceYesNo[(int)uiPerformance.blood.curValue];
}

/*
 ==================
 UI_Performance_Callback
 ==================
*/
static void UI_Performance_Callback (void *self, int event){

	menuCommon_t	*item = (menuCommon_t *)self;

	if (event == QM_CHANGED){
		UI_Performance_UpdateConfig();
		UI_Performance_SetConfig();
		return;
	}

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
 UI_Performance_Ownerdraw
 ==================
*/
static void UI_Performance_Ownerdraw (void *self){

	menuCommon_t	*item = (menuCommon_t *)self;

	if (uiPerformance.menu.items[uiPerformance.menu.cursor] == self)
		UI_DrawPic(item->x, item->y, item->width, item->height, colorWhite, UI_MOVEBOXFOCUS);
	else
		UI_DrawPic(item->x, item->y, item->width, item->height, colorWhite, UI_MOVEBOX);

	UI_DrawPic(item->x, item->y, item->width, item->height, colorWhite, ((menuBitmap_t *)self)->pic);
}

/*
 ==================
 UI_Performance_Init
 ==================
*/
static void UI_Performance_Init (){

	Mem_Fill(&uiPerformance, 0, sizeof(uiPerformance_t));

	uiPerformance.background.generic.id			= ID_BACKGROUND;
	uiPerformance.background.generic.type		= QMTYPE_BITMAP;
	uiPerformance.background.generic.flags		= QMF_INACTIVE;
	uiPerformance.background.generic.x			= 0;
	uiPerformance.background.generic.y			= 0;
	uiPerformance.background.generic.width		= 1024;
	uiPerformance.background.generic.height		= 768;
	uiPerformance.background.pic				= ART_BACKGROUND;

	uiPerformance.banner.generic.id				= ID_BANNER;
	uiPerformance.banner.generic.type			= QMTYPE_BITMAP;
	uiPerformance.banner.generic.flags			= QMF_INACTIVE;
	uiPerformance.banner.generic.x				= 0;
	uiPerformance.banner.generic.y				= 66;
	uiPerformance.banner.generic.width			= 1024;
	uiPerformance.banner.generic.height			= 46;
	uiPerformance.banner.pic					= ART_BANNER;

	uiPerformance.textShadow1.generic.id		= ID_TEXTSHADOW1;
	uiPerformance.textShadow1.generic.type		= QMTYPE_BITMAP;
	uiPerformance.textShadow1.generic.flags		= QMF_INACTIVE;
	uiPerformance.textShadow1.generic.x			= 182;
	uiPerformance.textShadow1.generic.y			= 170;
	uiPerformance.textShadow1.generic.width		= 256;
	uiPerformance.textShadow1.generic.height	= 256;
	uiPerformance.textShadow1.generic.color		= colorBlack;
	uiPerformance.textShadow1.pic				= ART_TEXT1;

	uiPerformance.textShadow2.generic.id		= ID_TEXTSHADOW2;
	uiPerformance.textShadow2.generic.type		= QMTYPE_BITMAP;
	uiPerformance.textShadow2.generic.flags		= QMF_INACTIVE;
	uiPerformance.textShadow2.generic.x			= 182;
	uiPerformance.textShadow2.generic.y			= 426;
	uiPerformance.textShadow2.generic.width		= 256;
	uiPerformance.textShadow2.generic.height	= 256;
	uiPerformance.textShadow2.generic.color		= colorBlack;
	uiPerformance.textShadow2.pic				= ART_TEXT2;

	uiPerformance.text1.generic.id				= ID_TEXT1;
	uiPerformance.text1.generic.type			= QMTYPE_BITMAP;
	uiPerformance.text1.generic.flags			= QMF_INACTIVE;
	uiPerformance.text1.generic.x				= 180;
	uiPerformance.text1.generic.y				= 168;
	uiPerformance.text1.generic.width			= 256;
	uiPerformance.text1.generic.height			= 256;
	uiPerformance.text1.pic						= ART_TEXT1;

	uiPerformance.text2.generic.id				= ID_TEXT2;
	uiPerformance.text2.generic.type			= QMTYPE_BITMAP;
	uiPerformance.text2.generic.flags			= QMF_INACTIVE;
	uiPerformance.text2.generic.x				= 180;
	uiPerformance.text2.generic.y				= 424;
	uiPerformance.text2.generic.width			= 256;
	uiPerformance.text2.generic.height			= 256;
	uiPerformance.text2.pic						= ART_TEXT2;

	uiPerformance.back.generic.id				= ID_BACK;
	uiPerformance.back.generic.type				= QMTYPE_BITMAP;
	uiPerformance.back.generic.x				= 413;
	uiPerformance.back.generic.y				= 656;
	uiPerformance.back.generic.width			= 198;
	uiPerformance.back.generic.height			= 38;
	uiPerformance.back.generic.callback			= UI_Performance_Callback;
	uiPerformance.back.generic.ownerdraw		= UI_Performance_Ownerdraw;
	uiPerformance.back.pic						= UI_BACKBUTTON;

	uiPerformance.screenSize.generic.id			= ID_SCREENSIZE;
	uiPerformance.screenSize.generic.type		= QMTYPE_SPINCONTROL;
	uiPerformance.screenSize.generic.flags		= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiPerformance.screenSize.generic.x			= 580;
	uiPerformance.screenSize.generic.y			= 160;
	uiPerformance.screenSize.generic.width		= 198;
	uiPerformance.screenSize.generic.height		= 30;
	uiPerformance.screenSize.generic.callback	= UI_Performance_Callback;
	uiPerformance.screenSize.generic.statusText	= "Set your game window's screen size";
	uiPerformance.screenSize.minValue			= 30;
	uiPerformance.screenSize.maxValue			= 100;
	uiPerformance.screenSize.range				= 10;

	uiPerformance.syncEveryFrame.generic.id		= ID_SYNCEVERYFRAME;
	uiPerformance.syncEveryFrame.generic.type	= QMTYPE_SPINCONTROL;
	uiPerformance.syncEveryFrame.generic.flags	= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiPerformance.syncEveryFrame.generic.x		= 580;
	uiPerformance.syncEveryFrame.generic.y		= 192;
	uiPerformance.syncEveryFrame.generic.width	= 198;
	uiPerformance.syncEveryFrame.generic.height	= 30;
	uiPerformance.syncEveryFrame.generic.callback	= UI_Performance_Callback;
	uiPerformance.syncEveryFrame.generic.statusText	= "Synchronize video frames";
	uiPerformance.syncEveryFrame.minValue		= 0;
	uiPerformance.syncEveryFrame.maxValue		= 1;
	uiPerformance.syncEveryFrame.range			= 1;

	uiPerformance.inGameVideo.generic.id		= ID_INGAMEVIDEO;
	uiPerformance.inGameVideo.generic.type		= QMTYPE_SPINCONTROL;
	uiPerformance.inGameVideo.generic.flags		= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiPerformance.inGameVideo.generic.x			= 580;
	uiPerformance.inGameVideo.generic.y			= 224;
	uiPerformance.inGameVideo.generic.width		= 198;
	uiPerformance.inGameVideo.generic.height	= 30;
	uiPerformance.inGameVideo.generic.callback	= UI_Performance_Callback;
	uiPerformance.inGameVideo.generic.statusText	= "Play in-game cinematics";
	uiPerformance.inGameVideo.minValue			= 0;
	uiPerformance.inGameVideo.maxValue			= 1;
	uiPerformance.inGameVideo.range				= 1;

	uiPerformance.dynamicLights.generic.id		= ID_DYNAMICLIGHTS;
	uiPerformance.dynamicLights.generic.type	= QMTYPE_SPINCONTROL;
	uiPerformance.dynamicLights.generic.flags	= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiPerformance.dynamicLights.generic.x		= 580;
	uiPerformance.dynamicLights.generic.y		= 256;
	uiPerformance.dynamicLights.generic.width	= 198;
	uiPerformance.dynamicLights.generic.height	= 30;
	uiPerformance.dynamicLights.generic.callback	= UI_Performance_Callback;
	uiPerformance.dynamicLights.generic.statusText	= "Render dynamic lights on map and models";
	uiPerformance.dynamicLights.minValue		= 0;
	uiPerformance.dynamicLights.maxValue		= 1;
	uiPerformance.dynamicLights.range			= 1;

	uiPerformance.shadows.generic.id			= ID_SHADOWS;
	uiPerformance.shadows.generic.type			= QMTYPE_SPINCONTROL;
	uiPerformance.shadows.generic.flags			= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiPerformance.shadows.generic.x				= 580;
	uiPerformance.shadows.generic.y				= 288;
	uiPerformance.shadows.generic.width			= 198;
	uiPerformance.shadows.generic.height		= 30;
	uiPerformance.shadows.generic.callback		= UI_Performance_Callback;
	uiPerformance.shadows.generic.statusText	= "Render shadows cast by models";
	uiPerformance.shadows.minValue				= 0;
	uiPerformance.shadows.maxValue				= 1;
	uiPerformance.shadows.range					= 1;

	uiPerformance.caustics.generic.id			= ID_CAUSTICS;
	uiPerformance.caustics.generic.type			= QMTYPE_SPINCONTROL;
	uiPerformance.caustics.generic.flags		= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiPerformance.caustics.generic.x			= 580;
	uiPerformance.caustics.generic.y			= 320;
	uiPerformance.caustics.generic.width		= 198;
	uiPerformance.caustics.generic.height		= 30;
	uiPerformance.caustics.generic.callback		= UI_Performance_Callback;
	uiPerformance.caustics.generic.statusText	= "Render caustics on submerged surfaces";
	uiPerformance.caustics.minValue				= 0;
	uiPerformance.caustics.maxValue				= 1;
	uiPerformance.caustics.range				= 1;

	uiPerformance.particles.generic.id			= ID_PARTICLES;
	uiPerformance.particles.generic.type		= QMTYPE_SPINCONTROL;
	uiPerformance.particles.generic.flags		= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiPerformance.particles.generic.x			= 580;
	uiPerformance.particles.generic.y			= 352;
	uiPerformance.particles.generic.width		= 198;
	uiPerformance.particles.generic.height		= 30;
	uiPerformance.particles.generic.callback	= UI_Performance_Callback;
	uiPerformance.particles.generic.statusText	= "Enable/Disable particle effects";
	uiPerformance.particles.minValue			= 0;
	uiPerformance.particles.maxValue			= 1;
	uiPerformance.particles.range				= 1;

	uiPerformance.decals.generic.id				= ID_DECALS;
	uiPerformance.decals.generic.type			= QMTYPE_SPINCONTROL;
	uiPerformance.decals.generic.flags			= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiPerformance.decals.generic.x				= 580;
	uiPerformance.decals.generic.y				= 384;
	uiPerformance.decals.generic.width			= 198;
	uiPerformance.decals.generic.height			= 30;
	uiPerformance.decals.generic.callback		= UI_Performance_Callback;
	uiPerformance.decals.generic.statusText		= "Enable/Disable marks on walls";
	uiPerformance.decals.minValue				= 0;
	uiPerformance.decals.maxValue				= 2;
	uiPerformance.decals.range					= 1;

	uiPerformance.ejectingBrass.generic.id		= ID_EJECTINGBRASS;
	uiPerformance.ejectingBrass.generic.type	= QMTYPE_SPINCONTROL;
	uiPerformance.ejectingBrass.generic.flags	= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiPerformance.ejectingBrass.generic.x		= 580;
	uiPerformance.ejectingBrass.generic.y		= 416;
	uiPerformance.ejectingBrass.generic.width	= 198;
	uiPerformance.ejectingBrass.generic.height	= 30;
	uiPerformance.ejectingBrass.generic.callback	= UI_Performance_Callback;
	uiPerformance.ejectingBrass.generic.statusText	= "Enable/Disable ejecting bullet casings from weapons";
	uiPerformance.ejectingBrass.minValue		= 0;
	uiPerformance.ejectingBrass.maxValue		= 2;
	uiPerformance.ejectingBrass.range			= 1;

	uiPerformance.shellEffects.generic.id		= ID_SHELLEFFECTS;
	uiPerformance.shellEffects.generic.type		= QMTYPE_SPINCONTROL;
	uiPerformance.shellEffects.generic.flags	= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiPerformance.shellEffects.generic.x		= 580;
	uiPerformance.shellEffects.generic.y		= 448;
	uiPerformance.shellEffects.generic.width	= 198;
	uiPerformance.shellEffects.generic.height	= 30;
	uiPerformance.shellEffects.generic.callback	= UI_Performance_Callback;
	uiPerformance.shellEffects.generic.statusText	= "Enable/Disable player and weapon shells";
	uiPerformance.shellEffects.minValue			= 0;
	uiPerformance.shellEffects.maxValue			= 1;
	uiPerformance.shellEffects.range			= 1;

	uiPerformance.screenBlends.generic.id		= ID_SCREENBLENDS;
	uiPerformance.screenBlends.generic.type		= QMTYPE_SPINCONTROL;
	uiPerformance.screenBlends.generic.flags	= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiPerformance.screenBlends.generic.x		= 580;
	uiPerformance.screenBlends.generic.y		= 480;
	uiPerformance.screenBlends.generic.width	= 198;
	uiPerformance.screenBlends.generic.height	= 30;
	uiPerformance.screenBlends.generic.callback	= UI_Performance_Callback;
	uiPerformance.screenBlends.generic.statusText	= "Enable/Disable screen blends";
	uiPerformance.screenBlends.minValue			= 0;
	uiPerformance.screenBlends.maxValue			= 2;
	uiPerformance.screenBlends.range			= 1;

	uiPerformance.blood.generic.id				= ID_BLOOD;
	uiPerformance.blood.generic.type			= QMTYPE_SPINCONTROL;
	uiPerformance.blood.generic.flags			= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiPerformance.blood.generic.x				= 580;
	uiPerformance.blood.generic.y				= 512;
	uiPerformance.blood.generic.width			= 198;
	uiPerformance.blood.generic.height			= 30;
	uiPerformance.blood.generic.callback		= UI_Performance_Callback;
	uiPerformance.blood.generic.statusText		= "Enable/Disable blood effects";
	uiPerformance.blood.minValue				= 0;
	uiPerformance.blood.maxValue				= 1;
	uiPerformance.blood.range					= 1;

	UI_Performance_GetConfig();

	UI_AddItem(&uiPerformance.menu, (void *)&uiPerformance.background);
	UI_AddItem(&uiPerformance.menu, (void *)&uiPerformance.banner);
	UI_AddItem(&uiPerformance.menu, (void *)&uiPerformance.textShadow1);
	UI_AddItem(&uiPerformance.menu, (void *)&uiPerformance.textShadow2);
	UI_AddItem(&uiPerformance.menu, (void *)&uiPerformance.text1);
	UI_AddItem(&uiPerformance.menu, (void *)&uiPerformance.text2);
	UI_AddItem(&uiPerformance.menu, (void *)&uiPerformance.back);
	UI_AddItem(&uiPerformance.menu, (void *)&uiPerformance.screenSize);
	UI_AddItem(&uiPerformance.menu, (void *)&uiPerformance.syncEveryFrame);
	UI_AddItem(&uiPerformance.menu, (void *)&uiPerformance.inGameVideo);
	UI_AddItem(&uiPerformance.menu, (void *)&uiPerformance.dynamicLights);
	UI_AddItem(&uiPerformance.menu, (void *)&uiPerformance.shadows);
	UI_AddItem(&uiPerformance.menu, (void *)&uiPerformance.caustics);
	UI_AddItem(&uiPerformance.menu, (void *)&uiPerformance.particles);
	UI_AddItem(&uiPerformance.menu, (void *)&uiPerformance.decals);
	UI_AddItem(&uiPerformance.menu, (void *)&uiPerformance.ejectingBrass);
	UI_AddItem(&uiPerformance.menu, (void *)&uiPerformance.shellEffects);
	UI_AddItem(&uiPerformance.menu, (void *)&uiPerformance.screenBlends);
	UI_AddItem(&uiPerformance.menu, (void *)&uiPerformance.blood);
}

/*
 ==================
 UI_Performance_Precache
 ==================
*/
void UI_Performance_Precache (){

	R_RegisterMaterialNoMip(ART_BACKGROUND);
	R_RegisterMaterialNoMip(ART_BANNER);
	R_RegisterMaterialNoMip(ART_TEXT1);
	R_RegisterMaterialNoMip(ART_TEXT2);
}

/*
 ==================
 UI_Performance_Menu
 ==================
*/
void UI_Performance_Menu (){

	UI_Performance_Precache();
	UI_Performance_Init();

	UI_Performance_UpdateConfig();

	UI_PushMenu(&uiPerformance.menu);
}