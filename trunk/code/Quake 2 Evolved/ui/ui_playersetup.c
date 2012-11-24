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
// ui_playersetup.c - Player set up menu
//

// TODO:
// - finish railgun options
// - skin list crashes, some kind of memory error (pcx texture loading?)


#include "ui_local.h"


#define ART_BACKGROUND				"ui/assets/ui/ui_sub_options"
#define ART_BANNER					"ui/assets/banners/playersetup_t"
#define ART_TEXT1					"ui/assets/text/playersetup_text_p1"
#define ART_TEXT2					"ui/assets/text/playersetup_text_p2"
#define ART_PLAYERVIEW				"ui/assets/segments/player_view"
#define ART_VIEWRAILCORE			"ui/assets/ui/rail_type1_1"
#define ART_VIEWRAILSPIRAL			"ui/assets/ui/rail_type1_2"

#define MAX_PLAYER_MODELS			256
#define MAX_PLAYER_SKINS			2048

typedef enum {
	ID_BACKGROUND,
	ID_BANNER,

	ID_TEXT1,
	ID_TEXT2,
	ID_TEXTSHADOW1,
	ID_TEXTSHADOW2,

	ID_BACK,

	ID_VIEW,
	ID_NAME,
	ID_MODEL,
	ID_SKIN,
	ID_RAILTRAIL,
	ID_VIEWRAILCORE,
	ID_VIEWRAILSPIRAL,
	ID_RAILCORECOLOR,
	ID_RAILSPIRALCOLOR
} uiPlayerSetupId_t;

typedef struct {
	char					playerModels[MAX_PLAYER_MODELS][MAX_QPATH];
	int						numPlayerModels;
	char					playerSkins[MAX_PLAYER_SKINS][MAX_QPATH];
	int						numPlayerSkins;
	char					currentModel[MAX_QPATH];
	char					currentSkin[MAX_QPATH];
	
	menuFramework_t			menu;

	menuBitmap_t			background;
	menuBitmap_t			banner;

	menuBitmap_t			textShadow1;
	menuBitmap_t			textShadow2;
	menuBitmap_t			text1;
	menuBitmap_t			text2;

	menuBitmap_t			back;

	menuBitmap_t			view;

	menuField_t				name;
	menuSpinControl_t		model;
	menuSpinControl_t		skin;
	menuSpinControl_t		railTrail;
	menuBitmap_t			viewRailCore;
	menuBitmap_t			viewRailSpiral;
	menuSpinControl_t		railCoreColor;
	menuSpinControl_t		railSpiralColor;
} uiPlayerSetup_t;

static uiPlayerSetup_t		uiPlayerSetup;


/*
 ==================
 UI_PlayerSetup_CalcFov

 TODO: rewrite?

 I need this here...
 ==================
*/
float UI_PlayerSetup_CalcFov (float fovX, float width, float height){

	float	x, y;

	if (fovX < 1.0f)
		fovX = 1.0f;
	else if (fovX > 179.0f)
		fovX = 179.0f;

	x = width / tan(fovX / 360.0f * M_PI);
	y = atan(height / x) * 360.0f / M_PI;

	return y;
}

/*
 ==================
 UI_PlayerSetup_FindModels
 ==================
*/
static void UI_PlayerSetup_FindModels (){

	const char	**fileList;
	int			numFiles;
	char		pathMD2[MAX_OSPATH], pathMD3[MAX_OSPATH];
	char		list[MAX_PLAYER_MODELS][MAX_OSPATH];
	int			count = 0;
	int			i;

	uiPlayerSetup.numPlayerModels = 0;

	// Load the models into a list
	fileList = FS_ListFiles("players", "/", true, &numFiles);

	for (i = 0; i < numFiles; i++){
		if (Str_FindChar(fileList[i], '.'))
			continue;

		if (count == MAX_PLAYER_MODELS)
			break;

		Str_Copy(list[count], fileList[i], sizeof(list[count]));
		count++;
	}

	FS_FreeFileList(fileList);

	// Build the model list
	for (i = 0; i < count; i++){
		// Check if the model exists
		Str_SPrintf(pathMD2, sizeof(pathMD2), "players/%s/tris.md2", list[i]);
		Str_SPrintf(pathMD3, sizeof(pathMD3), "players/%s/tris.md3", list[i]);

		if (!FS_FileExists(pathMD2) && !FS_FileExists(pathMD3))
			continue;

		Str_Copy(uiPlayerSetup.playerModels[uiPlayerSetup.numPlayerModels], list[i], sizeof(uiPlayerSetup.playerModels[uiPlayerSetup.numPlayerModels]));
		uiPlayerSetup.numPlayerModels++;
	}
}

/*
 ==================
 UI_PlayerSetup_FindSkins
 ==================
*/
static void UI_PlayerSetup_FindSkins (const char *model){

	const char	**fileList;
	int			numFiles;
	char		pathPCX[MAX_OSPATH], pathTGA[MAX_OSPATH];
	char		list[MAX_PLAYER_SKINS][MAX_OSPATH];
	int			count = 0;
	int			i;

	uiPlayerSetup.numPlayerSkins = 0;

	// Load the skin into a list
	fileList = FS_ListFiles(Str_VarArgs("players/%s", model), NULL, true, &numFiles);

	for (i = 0; i < numFiles; i++){
		if (!Str_FindChar(fileList[i], '.'))
			continue;

		if (Str_FindText(fileList[i], "_i.", false))
			continue;

		if (!Str_FindText(fileList[i], ".pcx", false) && !Str_FindText(fileList[i], ".tga", false))
			continue;

		if (count == MAX_PLAYER_SKINS)
			break;

		Str_Copy(list[count], fileList[i], sizeof(list[count]));
		Str_StripFileExtension(list[count]);

		count++;
	}

	FS_FreeFileList(fileList);

	// Build the skin list
	for (i = 0; i < count; i++){
		Str_SPrintf(pathPCX, sizeof(pathPCX), "players/%s/%s_i.pcx", model, list[i]);
		Str_SPrintf(pathTGA, sizeof(pathTGA), "players/%s/%s_i.tga", model, list[i]);

		if (!FS_FileExists(pathPCX) && !FS_FileExists(pathTGA))
			continue;

		Str_Copy(uiPlayerSetup.playerSkins[uiPlayerSetup.numPlayerSkins], list[i], sizeof(uiPlayerSetup.playerSkins[uiPlayerSetup.numPlayerSkins]));
		uiPlayerSetup.numPlayerSkins++;
	}
}

/*
 ==================
 UI_PlayerSetup_GetConfig
 ==================
*/
static void UI_PlayerSetup_GetConfig (){

	char	model[MAX_QPATH], skin[MAX_QPATH];
	char	*ch;
	int		i;

	Str_Copy(uiPlayerSetup.name.buffer, CVar_GetVariableString("name"), sizeof(uiPlayerSetup.name.buffer));

	// Get user set skin
	Str_Copy(model, CVar_GetVariableString("skin"), sizeof(model));
	ch = Str_FindChar(model, '/');
	if (!ch)
		ch = Str_FindChar(model, '\\');
	if (ch){
		*ch++ = 0;
		Str_Copy(skin, ch, sizeof(skin));
	}
	else
		skin[0] = 0;

	// Find models
	UI_PlayerSetup_FindModels();

	// Select current model
	for (i = 0; i < uiPlayerSetup.numPlayerModels; i++){
		if (!Str_ICompare(uiPlayerSetup.playerModels[i], model)){
			uiPlayerSetup.model.curValue = (float)i;
			break;
		}
	}

	Str_Copy(uiPlayerSetup.currentModel, uiPlayerSetup.playerModels[(int)uiPlayerSetup.model.curValue], sizeof(uiPlayerSetup.currentModel));
	uiPlayerSetup.model.maxValue = (float)(uiPlayerSetup.numPlayerModels - 1);

	// Find skins for the selected model
	UI_PlayerSetup_FindSkins(uiPlayerSetup.currentModel);

	// Select current skin
	for (i = 0; i < uiPlayerSetup.numPlayerSkins; i++){
		if (!Str_ICompare(uiPlayerSetup.playerSkins[i], skin)){
			uiPlayerSetup.skin.curValue = (float)i;
			break;
		}
	}

	Str_Copy(uiPlayerSetup.currentSkin, uiPlayerSetup.playerSkins[(int)uiPlayerSetup.skin.curValue], sizeof(uiPlayerSetup.currentSkin));
	uiPlayerSetup.skin.maxValue = (float)(uiPlayerSetup.numPlayerSkins - 1);
}

/*
 ==================
 UI_PlayerSetup_SetConfig
 ==================
*/
static void UI_PlayerSetup_SetConfig (){

	CVar_SetVariableString("name", uiPlayerSetup.name.buffer, false);

	CVar_SetVariableString("skin", Str_VarArgs("%s/%s", uiPlayerSetup.currentModel, uiPlayerSetup.currentSkin), false);
}

/*
 ==================
 UI_PlayerSetup_UpdateConfig
 ==================
*/
static void UI_PlayerSetup_UpdateConfig (){

	const char	*skin;
	char		*ch;
	int			i;

	// See if the model has changed
	if (Str_ICompare(uiPlayerSetup.currentModel, uiPlayerSetup.playerModels[(int)uiPlayerSetup.model.curValue])){
		Str_Copy(uiPlayerSetup.currentModel, uiPlayerSetup.playerModels[(int)uiPlayerSetup.model.curValue], sizeof(uiPlayerSetup.currentModel));

		// Find skins for the selected model
		UI_PlayerSetup_FindSkins(uiPlayerSetup.currentModel);

		// Select current skin
		skin = CVar_GetVariableString("skin");
		ch = Str_FindChar(skin, '/');
		if (!ch)
			ch = Str_FindChar(skin, '\\');
		if (ch)
			skin = ch + 1;

		for (i = 0; i < uiPlayerSetup.numPlayerSkins; i++){
			if (!Str_ICompare(uiPlayerSetup.playerSkins[i], skin)){
				uiPlayerSetup.skin.curValue = (float)i;
				break;
			}
		}

		// Couldn't find, so select first
		if (i == uiPlayerSetup.numPlayerSkins)
			uiPlayerSetup.skin.curValue = 0;

		Str_Copy(uiPlayerSetup.currentSkin, uiPlayerSetup.playerSkins[(int)uiPlayerSetup.skin.curValue], sizeof(uiPlayerSetup.currentSkin));
		uiPlayerSetup.skin.maxValue = (float)(uiPlayerSetup.numPlayerSkins - 1);
	}
	else
		Str_Copy(uiPlayerSetup.currentSkin, uiPlayerSetup.playerSkins[(int)uiPlayerSetup.skin.curValue], sizeof(uiPlayerSetup.currentSkin));

	uiPlayerSetup.model.generic.name = uiPlayerSetup.playerModels[(int)uiPlayerSetup.model.curValue];

	uiPlayerSetup.skin.generic.name = uiPlayerSetup.playerSkins[(int)uiPlayerSetup.skin.curValue];

	uiPlayerSetup.railTrail.generic.name = "Quake 2";
	uiPlayerSetup.railTrail.generic.flags |= QMF_GRAYED;

	uiPlayerSetup.railCoreColor.generic.name = "White";
	uiPlayerSetup.railCoreColor.generic.flags |= QMF_GRAYED;
	
	uiPlayerSetup.railSpiralColor.generic.name = "Cyan";
	uiPlayerSetup.railSpiralColor.generic.flags |= QMF_GRAYED;
}

/*
 ==================
 UI_PlayerSetup_Callback
 ==================
*/
static void UI_PlayerSetup_Callback (void *self, int event){

	menuCommon_t	*item = (menuCommon_t *)self;

	if (event == QM_CHANGED){
		UI_PlayerSetup_UpdateConfig();
		UI_PlayerSetup_SetConfig();
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
 UI_PlayerSetup_Ownerdraw
 ==================
*/
static void UI_PlayerSetup_Ownerdraw (void *self){

	menuCommon_t	*item = (menuCommon_t *)self;
	renderEntity_t	renderEntity;
	renderLight_t	renderLight;
	renderView_t	renderView;
	vec4_t			iconTrans = {1.0f, 1.0f, 1.0f, 0.5f};
	vec3_t			angles;
	char			path[MAX_QPATH];
	int				x = 630, y = 226, w = 316, h = 316;

	switch (item->id){
	case ID_VIEW:
		// Draw the background
		Str_SPrintf(path, sizeof(path), "players/%s/%s_i", uiPlayerSetup.currentModel, uiPlayerSetup.currentSkin);
		UI_ScaleCoords(&x, &y, &w, &h);

		UI_DrawPic(x, y, w, h, iconTrans, path);
		UI_DrawPic(item->x, item->y, item->width, item->height, colorWhite, uiPlayerSetup.view.pic);

		R_ClearScene();

		// Draw the player model
		Mem_Fill(&renderEntity, 0, sizeof(renderEntity_t));

		renderEntity.type = RE_MODEL;

		VectorSet(renderEntity.origin, 80.0f, -80.0f, 0.0f);
		VectorCopy(renderEntity.origin, renderEntity.oldOrigin);
		VectorSet(angles, 0.0f, (uiStatic.realTime & 4095) * 360.0f / 4096.0f, 0.0f);
		AnglesToMat3(angles, renderEntity.axis);

		Str_SPrintf(path, sizeof(path), "players/%s/tris.md2", uiPlayerSetup.currentModel);
		renderEntity.model = R_RegisterModel(path);

		Str_SPrintf(path, sizeof(path), "players/%s/%s", uiPlayerSetup.currentModel, uiPlayerSetup.currentSkin);
		renderEntity.material = R_RegisterMaterial(path, true);

		renderEntity.materialParms[MATERIALPARM_RED] = 1.0f;
		renderEntity.materialParms[MATERIALPARM_GREEN] = 1.0f;
		renderEntity.materialParms[MATERIALPARM_BLUE] = 1.0f;
		renderEntity.materialParms[MATERIALPARM_ALPHA] = 1.0f;
		renderEntity.materialParms[MATERIALPARM_TIMEOFFSET] = 0.0f;
		renderEntity.materialParms[MATERIALPARM_DIVERSITY] = 0.0f;
		renderEntity.materialParms[MATERIALPARM_MISC] = 0.0f;
		renderEntity.materialParms[MATERIALPARM_MODE] = 1.0f;

		R_AddEntityToScene(&renderEntity);

		// Also draw the default weapon model
		Mem_Fill(&renderEntity, 0, sizeof(renderEntity_t));

		renderEntity.type = RE_MODEL;

		VectorSet(renderEntity.origin, 80.0f, -80.0f, 0.0f);
		VectorCopy(renderEntity.origin, renderEntity.oldOrigin);
		VectorSet(angles, 0.0f, (uiStatic.realTime & 4095) * 360.0f / 4096.0f, 0.0f);
		AnglesToMat3(angles, renderEntity.axis);

		Str_SPrintf(path, sizeof(path), "players/%s/weapon.md2", uiPlayerSetup.currentModel);
		renderEntity.model = R_RegisterModel(path);

		Str_SPrintf(path, sizeof(path), "players/%s/weapon", uiPlayerSetup.currentModel);
		renderEntity.material = R_RegisterMaterial(path, true);

		renderEntity.materialParms[MATERIALPARM_RED] = 1.0f;
		renderEntity.materialParms[MATERIALPARM_GREEN] = 1.0f;
		renderEntity.materialParms[MATERIALPARM_BLUE] = 1.0f;
		renderEntity.materialParms[MATERIALPARM_ALPHA] = 1.0f;
		renderEntity.materialParms[MATERIALPARM_TIMEOFFSET] = 0.0f;
		renderEntity.materialParms[MATERIALPARM_DIVERSITY] = 0.0f;
		renderEntity.materialParms[MATERIALPARM_MISC] = 0.0f;
		renderEntity.materialParms[MATERIALPARM_MODE] = 1.0f;

		R_AddEntityToScene(&renderEntity);

		// Add a light
		Mem_Fill(&renderLight, 0, sizeof(renderLight_t));

		renderLight.type = RL_POINT;
		renderLight.lightNum = 0;

		VectorSet(renderLight.origin, 0.0f, -50.0f, 0.0f);
		VectorClear(renderLight.center);
		Matrix3_Identity(renderLight.axis);

		VectorSet(renderLight.radius, 200.0f, 200.0f, 200.0f);

		renderLight.fogDistance = 500.0f;
		renderLight.fogHeight = 500.0f;

		renderLight.allowInView = VIEW_MAIN;

		renderLight.material = NULL;

		renderLight.materialParms[MATERIALPARM_RED] = 1.0f;
		renderLight.materialParms[MATERIALPARM_GREEN] = 1.0f;
		renderLight.materialParms[MATERIALPARM_BLUE] = 1.0f;
		renderLight.materialParms[MATERIALPARM_ALPHA] = 1.0f;
		renderLight.materialParms[MATERIALPARM_TIMEOFFSET] = 0.0f;
		renderLight.materialParms[MATERIALPARM_DIVERSITY] = 0.0f;
		renderLight.materialParms[MATERIALPARM_MISC] = 0.0f;
		renderLight.materialParms[MATERIALPARM_MODE] = 1.0f;

		R_AddLightToScene(&renderLight);

		// Create and render the scene
		Mem_Fill(&renderView, 0, sizeof(renderView_t));

		renderView.x = item->x + (item->width / 12);
		renderView.y = item->y + (item->height / 12);
		renderView.width = item->width - (item->width / 6);
		renderView.height = item->height - (item->height / 6);

		renderView.horzAdjust = H_NONE;
		renderView.horzPercent = 1.0f;

		renderView.vertAdjust = V_NONE;
		renderView.vertPercent = 1.0f;

		renderView.fovX = DEFAULT_FOV;
		renderView.fovY = UI_PlayerSetup_CalcFov(renderView.fovX, renderView.width, renderView.height);
		renderView.fovScale = renderView.fovX * (1.0f / DEFAULT_FOV);

		Matrix3_Copy(mat3_identity, renderView.axis);

		renderView.time = MS2SEC(uiStatic.realTime);

		R_RenderScene(&renderView, false);

		break;
	default:
		if (uiPlayerSetup.menu.items[uiPlayerSetup.menu.cursor] == self)
			UI_DrawPic(item->x, item->y, item->width, item->height, colorWhite, UI_MOVEBOXFOCUS);
		else
			UI_DrawPic(item->x, item->y, item->width, item->height, colorWhite, UI_MOVEBOX);

		UI_DrawPic(item->x, item->y, item->width, item->height, colorWhite, ((menuBitmap_t *)self)->pic);

		break;
	}
}

/*
 ==================
 UI_PlayerSetup_Init
 ==================
*/
static void UI_PlayerSetup_Init (){

	Mem_Fill(&uiPlayerSetup, 0, sizeof(uiPlayerSetup_t));

	uiPlayerSetup.background.generic.id			= ID_BACKGROUND;
	uiPlayerSetup.background.generic.type		= QMTYPE_BITMAP;
	uiPlayerSetup.background.generic.flags		= QMF_INACTIVE;
	uiPlayerSetup.background.generic.x			= 0;
	uiPlayerSetup.background.generic.y			= 0;
	uiPlayerSetup.background.generic.width		= 1024;
	uiPlayerSetup.background.generic.height		= 768;
	uiPlayerSetup.background.pic				= ART_BACKGROUND;

	uiPlayerSetup.banner.generic.id				= ID_BANNER;
	uiPlayerSetup.banner.generic.type			= QMTYPE_BITMAP;
	uiPlayerSetup.banner.generic.flags			= QMF_INACTIVE;
	uiPlayerSetup.banner.generic.x				= 0;
	uiPlayerSetup.banner.generic.y				= 66;
	uiPlayerSetup.banner.generic.width			= 1024;
	uiPlayerSetup.banner.generic.height			= 46;
	uiPlayerSetup.banner.pic					= ART_BANNER;

	uiPlayerSetup.textShadow1.generic.id		= ID_TEXTSHADOW1;
	uiPlayerSetup.textShadow1.generic.type		= QMTYPE_BITMAP;
	uiPlayerSetup.textShadow1.generic.flags		= QMF_INACTIVE;
	uiPlayerSetup.textShadow1.generic.x			= 98;
	uiPlayerSetup.textShadow1.generic.y			= 230;
	uiPlayerSetup.textShadow1.generic.width		= 128;
	uiPlayerSetup.textShadow1.generic.height	= 256;
	uiPlayerSetup.textShadow1.generic.color		= colorBlack;
	uiPlayerSetup.textShadow1.pic				= ART_TEXT1;

	uiPlayerSetup.textShadow2.generic.id		= ID_TEXTSHADOW2;
	uiPlayerSetup.textShadow2.generic.type		= QMTYPE_BITMAP;
	uiPlayerSetup.textShadow2.generic.flags		= QMF_INACTIVE;
	uiPlayerSetup.textShadow2.generic.x			= 98;
	uiPlayerSetup.textShadow2.generic.y			= 490;
	uiPlayerSetup.textShadow2.generic.width		= 128;
	uiPlayerSetup.textShadow2.generic.height	= 128;
	uiPlayerSetup.textShadow2.generic.color		= colorBlack;
	uiPlayerSetup.textShadow2.pic				= ART_TEXT2;

	uiPlayerSetup.text1.generic.id				= ID_TEXT1;
	uiPlayerSetup.text1.generic.type			= QMTYPE_BITMAP;
	uiPlayerSetup.text1.generic.flags			= QMF_INACTIVE;
	uiPlayerSetup.text1.generic.x				= 96;
	uiPlayerSetup.text1.generic.y				= 228;
	uiPlayerSetup.text1.generic.width			= 128;
	uiPlayerSetup.text1.generic.height			= 256;
	uiPlayerSetup.text1.pic						= ART_TEXT1;

	uiPlayerSetup.text2.generic.id				= ID_TEXT2;
	uiPlayerSetup.text2.generic.type			= QMTYPE_BITMAP;
	uiPlayerSetup.text2.generic.flags			= QMF_INACTIVE;
	uiPlayerSetup.text2.generic.x				= 96;
	uiPlayerSetup.text2.generic.y				= 488;
	uiPlayerSetup.text2.generic.width			= 128;
	uiPlayerSetup.text2.generic.height			= 128;
	uiPlayerSetup.text2.pic						= ART_TEXT2;

	uiPlayerSetup.back.generic.id				= ID_BACK;
	uiPlayerSetup.back.generic.type				= QMTYPE_BITMAP;
	uiPlayerSetup.back.generic.x				= 413;
	uiPlayerSetup.back.generic.y				= 656;
	uiPlayerSetup.back.generic.width			= 198;
	uiPlayerSetup.back.generic.height			= 38;
	uiPlayerSetup.back.generic.callback			= UI_PlayerSetup_Callback;
	uiPlayerSetup.back.generic.ownerdraw		= UI_PlayerSetup_Ownerdraw;
	uiPlayerSetup.back.pic						= UI_BACKBUTTON;

	uiPlayerSetup.view.generic.id				= ID_VIEW;
	uiPlayerSetup.view.generic.type				= QMTYPE_BITMAP;
	uiPlayerSetup.view.generic.flags			= QMF_INACTIVE;
	uiPlayerSetup.view.generic.x				= 628;
	uiPlayerSetup.view.generic.y				= 224;
	uiPlayerSetup.view.generic.width			= 320;
	uiPlayerSetup.view.generic.height			= 320;
	uiPlayerSetup.view.generic.ownerdraw		= UI_PlayerSetup_Ownerdraw;
	uiPlayerSetup.view.pic						= ART_PLAYERVIEW;

	uiPlayerSetup.name.generic.id				= ID_NAME;
	uiPlayerSetup.name.generic.type				= QMTYPE_FIELD;
	uiPlayerSetup.name.generic.flags			= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiPlayerSetup.name.generic.x				= 368;
	uiPlayerSetup.name.generic.y				= 224;
	uiPlayerSetup.name.generic.width			= 198;
	uiPlayerSetup.name.generic.height			= 30;
	uiPlayerSetup.name.generic.callback			= UI_PlayerSetup_Callback;
	uiPlayerSetup.name.generic.statusText		= "Enter your multiplayer display name";
	uiPlayerSetup.name.maxLenght				= 32;

	uiPlayerSetup.model.generic.id				= ID_MODEL;
	uiPlayerSetup.model.generic.type			= QMTYPE_SPINCONTROL;
	uiPlayerSetup.model.generic.flags			= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiPlayerSetup.model.generic.x				= 368;
	uiPlayerSetup.model.generic.y				= 256;
	uiPlayerSetup.model.generic.width			= 198;
	uiPlayerSetup.model.generic.height			= 30;
	uiPlayerSetup.model.generic.callback		= UI_PlayerSetup_Callback;
	uiPlayerSetup.model.generic.statusText		= "Select a model for representation in multiplayer";
	uiPlayerSetup.model.minValue				= 0;
	uiPlayerSetup.model.maxValue				= 1;
	uiPlayerSetup.model.range					= 1;

	uiPlayerSetup.skin.generic.id				= ID_SKIN;
	uiPlayerSetup.skin.generic.type				= QMTYPE_SPINCONTROL;
	uiPlayerSetup.skin.generic.flags			= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiPlayerSetup.skin.generic.x				= 368;
	uiPlayerSetup.skin.generic.y				= 288;
	uiPlayerSetup.skin.generic.width			= 198;
	uiPlayerSetup.skin.generic.height			= 30;
	uiPlayerSetup.skin.generic.callback			= UI_PlayerSetup_Callback;
	uiPlayerSetup.skin.generic.statusText		= "Select a skin for representation in multiplayer";
	uiPlayerSetup.skin.minValue					= 0;
	uiPlayerSetup.skin.maxValue					= 1;
	uiPlayerSetup.skin.range					= 1;

	uiPlayerSetup.railTrail.generic.id			= ID_RAILTRAIL;
	uiPlayerSetup.railTrail.generic.type		= QMTYPE_SPINCONTROL;
	uiPlayerSetup.railTrail.generic.flags		= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiPlayerSetup.railTrail.generic.x			= 368;
	uiPlayerSetup.railTrail.generic.y			= 352;
	uiPlayerSetup.railTrail.generic.width		= 198;
	uiPlayerSetup.railTrail.generic.height		= 30;
	uiPlayerSetup.railTrail.generic.callback	= UI_PlayerSetup_Callback;
	uiPlayerSetup.railTrail.minValue			= 0;
	uiPlayerSetup.railTrail.maxValue			= 1;
	uiPlayerSetup.railTrail.range				= 1;

	uiPlayerSetup.viewRailCore.generic.id		= ID_VIEWRAILCORE;
	uiPlayerSetup.viewRailCore.generic.type		= QMTYPE_BITMAP;
	uiPlayerSetup.viewRailCore.generic.flags	= QMF_INACTIVE;
	uiPlayerSetup.viewRailCore.generic.x		= 368;
	uiPlayerSetup.viewRailCore.generic.y		= 384;
	uiPlayerSetup.viewRailCore.generic.width	= 198;
	uiPlayerSetup.viewRailCore.generic.height	= 96;
	uiPlayerSetup.viewRailCore.generic.color	= colorWhite;
	uiPlayerSetup.viewRailCore.pic				= ART_VIEWRAILCORE;
	
	uiPlayerSetup.viewRailSpiral.generic.id		= ID_VIEWRAILSPIRAL;
	uiPlayerSetup.viewRailSpiral.generic.type	= QMTYPE_BITMAP;
	uiPlayerSetup.viewRailSpiral.generic.flags	= QMF_INACTIVE;
	uiPlayerSetup.viewRailSpiral.generic.x		= 368;
	uiPlayerSetup.viewRailSpiral.generic.y		= 384;
	uiPlayerSetup.viewRailSpiral.generic.width	= 198;
	uiPlayerSetup.viewRailSpiral.generic.height	= 96;
	uiPlayerSetup.viewRailSpiral.generic.color	= colorCyan;
	uiPlayerSetup.viewRailSpiral.pic			= ART_VIEWRAILSPIRAL;

	uiPlayerSetup.railCoreColor.generic.id		= ID_RAILCORECOLOR;
	uiPlayerSetup.railCoreColor.generic.type	= QMTYPE_SPINCONTROL;
	uiPlayerSetup.railCoreColor.generic.flags	= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiPlayerSetup.railCoreColor.generic.x		= 368;
	uiPlayerSetup.railCoreColor.generic.y		= 482;
	uiPlayerSetup.railCoreColor.generic.width	= 198;
	uiPlayerSetup.railCoreColor.generic.height	= 30;
	uiPlayerSetup.railCoreColor.generic.callback	= UI_PlayerSetup_Callback;
	uiPlayerSetup.railCoreColor.minValue		= 0;
	uiPlayerSetup.railCoreColor.maxValue		= 1;
	uiPlayerSetup.railCoreColor.range			= 1;

	uiPlayerSetup.railSpiralColor.generic.id	= ID_RAILSPIRALCOLOR;
	uiPlayerSetup.railSpiralColor.generic.type	= QMTYPE_SPINCONTROL;
	uiPlayerSetup.railSpiralColor.generic.flags	= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiPlayerSetup.railSpiralColor.generic.x		= 368;
	uiPlayerSetup.railSpiralColor.generic.y		= 514;
	uiPlayerSetup.railSpiralColor.generic.width	= 198;
	uiPlayerSetup.railSpiralColor.generic.height	= 30;
	uiPlayerSetup.railSpiralColor.generic.callback	= UI_PlayerSetup_Callback;
	uiPlayerSetup.railSpiralColor.minValue		= 0;
	uiPlayerSetup.railSpiralColor.maxValue		= 1;
	uiPlayerSetup.railSpiralColor.range			= 1;

	UI_PlayerSetup_GetConfig();

	UI_AddItem(&uiPlayerSetup.menu, (void *)&uiPlayerSetup.background);
	UI_AddItem(&uiPlayerSetup.menu, (void *)&uiPlayerSetup.banner);
	UI_AddItem(&uiPlayerSetup.menu, (void *)&uiPlayerSetup.textShadow1);
	UI_AddItem(&uiPlayerSetup.menu, (void *)&uiPlayerSetup.textShadow2);
	UI_AddItem(&uiPlayerSetup.menu, (void *)&uiPlayerSetup.text1);
	UI_AddItem(&uiPlayerSetup.menu, (void *)&uiPlayerSetup.text2);
	UI_AddItem(&uiPlayerSetup.menu, (void *)&uiPlayerSetup.back);
	UI_AddItem(&uiPlayerSetup.menu, (void *)&uiPlayerSetup.view);
	UI_AddItem(&uiPlayerSetup.menu, (void *)&uiPlayerSetup.name);
	UI_AddItem(&uiPlayerSetup.menu, (void *)&uiPlayerSetup.model);
	UI_AddItem(&uiPlayerSetup.menu, (void *)&uiPlayerSetup.skin);
	UI_AddItem(&uiPlayerSetup.menu, (void *)&uiPlayerSetup.railTrail);
	UI_AddItem(&uiPlayerSetup.menu, (void *)&uiPlayerSetup.viewRailCore);
	UI_AddItem(&uiPlayerSetup.menu, (void *)&uiPlayerSetup.viewRailSpiral);
	UI_AddItem(&uiPlayerSetup.menu, (void *)&uiPlayerSetup.railCoreColor);
	UI_AddItem(&uiPlayerSetup.menu, (void *)&uiPlayerSetup.railSpiralColor);
}

/*
 ==================
 UI_PlayerSetup_Precache
 ==================
*/
void UI_PlayerSetup_Precache (){

	R_RegisterMaterialNoMip(ART_BACKGROUND);
	R_RegisterMaterialNoMip(ART_BANNER);
	R_RegisterMaterialNoMip(ART_TEXT1);
	R_RegisterMaterialNoMip(ART_TEXT2);
	R_RegisterMaterialNoMip(ART_PLAYERVIEW);
	R_RegisterMaterialNoMip(ART_VIEWRAILCORE);
	R_RegisterMaterialNoMip(ART_VIEWRAILSPIRAL);
}

/*
 ==================
 UI_PlayerSetup_Menu
 ==================
*/
void UI_PlayerSetup_Menu (){

	UI_PlayerSetup_Precache();
	UI_PlayerSetup_Init();

	UI_PlayerSetup_UpdateConfig();

	UI_PushMenu(&uiPlayerSetup.menu);
}