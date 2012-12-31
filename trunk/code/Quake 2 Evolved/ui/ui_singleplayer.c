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
// ui_singleplayer.c - Single player menu
//


#include "ui_local.h"


#define ART_BACKGROUND				"ui/assets/ui/ui_sub_singleplayer"
#define ART_BANNER					"ui/assets/banners/singleplayer_t"
#define ART_LEVELSHOTBLUR			"ui/assets/segments/sp_mapshot"
#define ART_LISTBLUR				"ui/assets/segments/sp_mapload"
#define ART_EASY					"ui/assets/buttons/sp_easy"
#define ART_MEDIUM					"ui/assets/buttons/sp_medium"
#define ART_HARD					"ui/assets/buttons/sp_hard"
#define ART_HARDPLUS				"ui/assets/buttons/sp_hard+"

#define MAX_MISSIONS				32
#define MAX_LEVELSHOTS				16

typedef enum {
	ID_BACKGROUND,
	ID_BANNER,

	ID_BACK,
	ID_START,

	ID_LEVELSHOT,
	ID_MISSIONLIST,

	ID_SKILL,
	ID_LOADGAME,
	ID_SAVEGAME
} uiSinglePlayerId_t;

typedef struct {
	char					mission[MAX_PATH_LENGTH];
	char					name[80];
	char					objectives[1024];
	char					levelShots[MAX_LEVELSHOTS][MAX_PATH_LENGTH];
	int						numLevelShots;
} missionInfo_t;

typedef struct {
	missionInfo_t			missions[MAX_MISSIONS];
	int						numMissions;
	char					*missionsPtr[MAX_MISSIONS];

	int						currentLevelShot;
	int						fadeTime;

	int						currentSkill;

	menuFramework_t			menu;

	menuBitmap_t			background;
	menuBitmap_t			banner;

	menuBitmap_t			back;
	menuBitmap_t			start;

	menuBitmap_t			levelShot;
	menuScrollList_t		missionList;

	menuBitmap_t			skill;
	menuBitmap_t			loadGame;
	menuBitmap_t			saveGame;
} uiSinglePlayer_t;

static uiSinglePlayer_t		uiSinglePlayer;


/*
 ==================
 UI_SinglePlayer_GetMissionList

 TODO: i think this can be a bit cleaner
 ==================
*/
static void UI_SinglePlayer_GetMissionList (){

	script_t	*script;
	token_t		token;
	char		name[MAX_PATH_LENGTH];
	char		*levelshot;
	int			i;

	// Load the script file
	Str_SPrintf(name, sizeof(name), "scripts/ui/mission_%s.txt", CVar_GetVariableString("fs_game"));

	script = PS_LoadScriptFile(name);
	if (!script){
		for (i = 0; i < MAX_MISSIONS; i++)
			uiSinglePlayer.missionsPtr[i] = NULL;

		uiSinglePlayer.missionList.itemNames = uiSinglePlayer.missionsPtr;
		return;
	}

	PS_SetScriptFlags(script, SF_NOWARNINGS | SF_NOERRORS | SF_ALLOWPATHNAMES);

	// Parse missions
	while (1){
		// Parse the name
		if (!PS_ReadToken(script, &token))
			break;		// End of script

		Str_Copy(uiSinglePlayer.missions[uiSinglePlayer.numMissions].mission, uiSinglePlayer.missions[uiSinglePlayer.numMissions].mission, sizeof(uiSinglePlayer.missions[uiSinglePlayer.numMissions].mission));

		PS_ReadToken(script, &token);
		if (Str_ICompare(token.string, "{"))
			break;

		while (1){
			if (!PS_ReadToken(script, &token))
				break;	// End of script

			if (!Str_ICompare(token.string, "name")){
				PS_ReadToken(script, &token);
				Str_Copy(uiSinglePlayer.missions[uiSinglePlayer.numMissions].name, token.string, sizeof(uiSinglePlayer.missions[uiSinglePlayer.numMissions].name));
			}
			else if (!Str_ICompare(token.string, "objective")){
				PS_ReadToken(script, &token);
				Str_Copy(uiSinglePlayer.missions[uiSinglePlayer.numMissions].objectives, token.string, sizeof(uiSinglePlayer.missions[uiSinglePlayer.numMissions].objectives));
			}
			else if (!Str_ICompare(token.string, "levelshots")){
				for (i = 0; i < MAX_LEVELSHOTS; i++){
					if (!PS_ReadToken(script, &token))
						break;

					levelshot = uiSinglePlayer.missions[uiSinglePlayer.numMissions].levelShots[uiSinglePlayer.missions[uiSinglePlayer.numMissions].numLevelShots];
					Str_Copy(levelshot, token.string, MAX_PATH_LENGTH);
					uiSinglePlayer.missions[uiSinglePlayer.numMissions].numLevelShots++;
				}
			}
			else if (!Str_ICompare(token.string, "}")){
				uiSinglePlayer.numMissions++;
				break;
			}
			else
				break;
		}
	}

	PS_FreeScript(script);

	for (i = 0; i < uiSinglePlayer.numMissions; i++)
		uiSinglePlayer.missionsPtr[i] = uiSinglePlayer.missions[i].name;
	for ( ; i < MAX_MISSIONS; i++)
		uiSinglePlayer.missionsPtr[i] = NULL;

	uiSinglePlayer.missionList.itemNames = uiSinglePlayer.missionsPtr;
}

/*
 ==================
 UI_SinglePlayer_StartGame
 ==================
*/
static void UI_SinglePlayer_StartGame (){

	const char	*game = CVar_GetVariableString("fs_game");
	char		text[256];

	CVar_SetVariableInteger("skill", uiSinglePlayer.currentSkill, false);
	CVar_SetVariableInteger("deathmatch", 0, false);
	CVar_SetVariableInteger("coop", 0, false);

	if (uiSinglePlayer.numMissions)
		Str_SPrintf(text, sizeof(text), "killserver ; wait ; exec %s.cfg\n", uiSinglePlayer.missions[uiSinglePlayer.missionList.curItem].mission);
	else {
		if (!Str_ICompare(game, "baseq2"))
			Str_SPrintf(text, sizeof(text), "killserver ; wait ; gamemap *ntro.cin+base1\n");
		else if (!Str_ICompare(game, "xatrix"))
			Str_SPrintf(text, sizeof(text), "killserver ; wait ; gamemap *xin.cin+xswamp\n");
		else if (!Str_ICompare(game, "rogue"))
			Str_SPrintf(text, sizeof(text), "killserver ; wait ; gamemap *rintro.cin+rmine1\n");
		else
			Str_SPrintf(text, sizeof(text), "killserver ; wait ; newgame\n");
	}

	Cmd_ExecuteText(CMD_EXEC_APPEND, text);
}

/*
 ==================
 UI_SinglePlayer_Callback
 ==================
*/
static void UI_SinglePlayer_Callback (void *self, int event){

	menuCommon_t	*item = (menuCommon_t *)self;

	if (event == QM_CHANGED){
		// Restart levelshot animation
		uiSinglePlayer.currentLevelShot = 0;
		uiSinglePlayer.fadeTime = uiStatic.realTime;
		return;
	}

	if (event != QM_ACTIVATED)
		return;

	switch (item->id){
	case ID_BACK:
		UI_PopMenu();
		break;
	case ID_START:
		UI_SinglePlayer_StartGame();
		break;
	case ID_SKILL:
		uiSinglePlayer.currentSkill++;
		if (uiSinglePlayer.currentSkill > 3)
			uiSinglePlayer.currentSkill = 0;

		CVar_SetVariableInteger("ui_singlePlayerSkill", uiSinglePlayer.currentSkill, false);

		switch (uiSinglePlayer.currentSkill){
		case 0:
			uiSinglePlayer.skill.pic = ART_EASY;
			break;
		case 1:
			uiSinglePlayer.skill.pic = ART_MEDIUM;
			break;
		case 2:
			uiSinglePlayer.skill.pic = ART_HARD;
			break;
		case 3:
			uiSinglePlayer.skill.pic = ART_HARDPLUS;
			break;
		}
		break;
	case ID_LOADGAME:
		UI_LoadGame_Menu();
		break;
	case ID_SAVEGAME:
		UI_SaveGame_Menu();
		break;
	}
}

/*
 ==================
 UI_SinglePlayer_Ownerdraw
 ==================
*/
static void UI_SinglePlayer_Ownerdraw (void *self){

	menuCommon_t	*item = (menuCommon_t *)self;
	int				x, y, w, h, cw, ch;
	char			str[2048];
	int				prev;
	color_t			color = {255, 255, 255, 255};

	if (item->id == ID_LEVELSHOT){	
		// Draw the levelshot
		x = 66;
		y = 134;
		w = 410;
		h = 202;
		
		UI_ScaleCoords(&x, &y, &w, &h);

		if (uiSinglePlayer.numMissions && uiSinglePlayer.missions[uiSinglePlayer.missionList.curItem].numLevelShots){
			if (uiStatic.realTime - uiSinglePlayer.fadeTime >= 3000){
				uiSinglePlayer.fadeTime = uiStatic.realTime;

				uiSinglePlayer.currentLevelShot++;
				if (uiSinglePlayer.currentLevelShot == uiSinglePlayer.missions[uiSinglePlayer.missionList.curItem].numLevelShots)
					uiSinglePlayer.currentLevelShot = 0;
			}

			prev = uiSinglePlayer.currentLevelShot - 1;
			if (prev < 0)
				prev = uiSinglePlayer.missions[uiSinglePlayer.missionList.curItem].numLevelShots - 1;

			color[3] = Clamp((float)(uiStatic.realTime - uiSinglePlayer.fadeTime) / 1000, 0.0f, 1.0f) * 255;

			UI_DrawPicOLD(x, y, w, h, colorWhite, Str_VarArgs("ui/menu_levelshots/%s", uiSinglePlayer.missions[uiSinglePlayer.missionList.curItem].levelShots[prev]));
			UI_DrawPicOLD(x, y, w, h, color, Str_VarArgs("ui/menu_levelshots/%s", uiSinglePlayer.missions[uiSinglePlayer.missionList.curItem].levelShots[uiSinglePlayer.currentLevelShot]));
		}
		else
			UI_DrawPicOLD(x, y, w, h, colorWhite, "ui/menu_levelshots/unknownmap");

		// Draw the blurred frame
		UI_DrawPicOLD(item->x, item->y, item->width, item->height, colorWhite, ((menuBitmap_t *)self)->pic);

		// Draw the info text
		x = 512;
		y = 134;
		w = 448;
		h = 202;

		cw = 10;
		ch = 20;

		UI_ScaleCoords(&x, &y, &w, &h);
		UI_ScaleCoords(NULL, NULL, &cw, &ch);

		MakeRGBA(color, 0, 76, 127, 255);

		if (uiSinglePlayer.numMissions){
			Str_SPrintf(str, sizeof(str), "MISSION: ^7%s\n", uiSinglePlayer.missions[uiSinglePlayer.missionList.curItem].name);
			UI_DrawStringOLD(x, y, w, h, str, color, false, cw, ch, 0, true);
		
			y += ch;
			Str_SPrintf(str, sizeof(str), "OBJECTIVES:\n");
			UI_DrawStringOLD(x, y, w, h, str, color, false, cw, ch, 0, true);

			y += ch;
			Str_SPrintf(str, sizeof(str), "%s\n", uiSinglePlayer.missions[uiSinglePlayer.missionList.curItem].objectives);
			UI_DrawStringOLD(x, y, w, h, str, colorWhite, true, cw, ch, 0, true);
		}
		else
			UI_DrawStringOLD(x, y, w, h, "NO MISSION DATA AVAILABLE", color, true, cw, ch, 1, true);
	}
	else {
		if (uiSinglePlayer.menu.items[uiSinglePlayer.menu.cursor] == self)
			UI_DrawPicOLD(item->x, item->y, item->width, item->height, colorWhite, UI_MOVEBOXFOCUS);
		else
			UI_DrawPicOLD(item->x, item->y, item->width, item->height, colorWhite, UI_MOVEBOX);

		UI_DrawPicOLD(item->x, item->y, item->width, item->height, colorWhite, ((menuBitmap_t *)self)->pic);
	}
}

/*
 ==================
 UI_SinglePlayer_Init
 ==================
*/
static void UI_SinglePlayer_Init (){

	Mem_Fill(&uiSinglePlayer, 0, sizeof(uiSinglePlayer_t));

	uiSinglePlayer.fadeTime = uiStatic.realTime;

	uiSinglePlayer.background.generic.id		= ID_BACKGROUND;
	uiSinglePlayer.background.generic.type		= QMTYPE_BITMAP;
	uiSinglePlayer.background.generic.flags		= QMF_INACTIVE;
	uiSinglePlayer.background.generic.x			= 0;
	uiSinglePlayer.background.generic.y			= 0;
	uiSinglePlayer.background.generic.width		= 1024;
	uiSinglePlayer.background.generic.height	= 768;
	uiSinglePlayer.background.pic				= ART_BACKGROUND;

	uiSinglePlayer.banner.generic.id			= ID_BANNER;
	uiSinglePlayer.banner.generic.type			= QMTYPE_BITMAP;
	uiSinglePlayer.banner.generic.flags			= QMF_INACTIVE;
	uiSinglePlayer.banner.generic.x				= 0;
	uiSinglePlayer.banner.generic.y				= 66;
	uiSinglePlayer.banner.generic.width			= 1024;
	uiSinglePlayer.banner.generic.height		= 46;
	uiSinglePlayer.banner.pic					= ART_BANNER;

	uiSinglePlayer.back.generic.id				= ID_BACK;
	uiSinglePlayer.back.generic.type			= QMTYPE_BITMAP;
	uiSinglePlayer.back.generic.x				= 310;
	uiSinglePlayer.back.generic.y				= 656;
	uiSinglePlayer.back.generic.width			= 198;
	uiSinglePlayer.back.generic.height			= 38;
	uiSinglePlayer.back.generic.callback		= UI_SinglePlayer_Callback;
	uiSinglePlayer.back.generic.ownerdraw		= UI_SinglePlayer_Ownerdraw;
	uiSinglePlayer.back.pic						= UI_BACKBUTTON;

	uiSinglePlayer.start.generic.id				= ID_START;
	uiSinglePlayer.start.generic.type			= QMTYPE_BITMAP;
	uiSinglePlayer.start.generic.x				= 516;
	uiSinglePlayer.start.generic.y				= 656;
	uiSinglePlayer.start.generic.width			= 198;
	uiSinglePlayer.start.generic.height			= 38;
	uiSinglePlayer.start.generic.callback		= UI_SinglePlayer_Callback;
	uiSinglePlayer.start.generic.ownerdraw		= UI_SinglePlayer_Ownerdraw;
	uiSinglePlayer.start.pic					= UI_STARTBUTTON;

	uiSinglePlayer.levelShot.generic.id			= ID_LEVELSHOT;
	uiSinglePlayer.levelShot.generic.type		= QMTYPE_BITMAP;
	uiSinglePlayer.levelShot.generic.flags		= QMF_INACTIVE;
	uiSinglePlayer.levelShot.generic.x			= 64;
	uiSinglePlayer.levelShot.generic.y			= 132;
	uiSinglePlayer.levelShot.generic.width		= 414;
	uiSinglePlayer.levelShot.generic.height		= 206;
	uiSinglePlayer.levelShot.generic.ownerdraw	= UI_SinglePlayer_Ownerdraw;
	uiSinglePlayer.levelShot.pic				= ART_LEVELSHOTBLUR;

	uiSinglePlayer.missionList.generic.id		= ID_MISSIONLIST;
	uiSinglePlayer.missionList.generic.type		= QMTYPE_SCROLLLIST;
	uiSinglePlayer.missionList.generic.flags	= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiSinglePlayer.missionList.generic.x		= 64;
	uiSinglePlayer.missionList.generic.y		= 416;
	uiSinglePlayer.missionList.generic.width	= 414;
	uiSinglePlayer.missionList.generic.height	= 140;
	uiSinglePlayer.missionList.generic.callback	= UI_SinglePlayer_Callback;
	uiSinglePlayer.missionList.background		= ART_LISTBLUR;

	uiSinglePlayer.skill.generic.id				= ID_SKILL;
	uiSinglePlayer.skill.generic.type			= QMTYPE_BITMAP;
	uiSinglePlayer.skill.generic.x				= 676;
	uiSinglePlayer.skill.generic.y				= 420;
	uiSinglePlayer.skill.generic.width			= 198;
	uiSinglePlayer.skill.generic.height			= 38;
	uiSinglePlayer.skill.generic.callback		= UI_SinglePlayer_Callback;
	uiSinglePlayer.skill.generic.ownerdraw		= UI_SinglePlayer_Ownerdraw;
	
	uiSinglePlayer.loadGame.generic.id			= ID_LOADGAME;
	uiSinglePlayer.loadGame.generic.type		= QMTYPE_BITMAP;
	uiSinglePlayer.loadGame.generic.x			= 676;
	uiSinglePlayer.loadGame.generic.y			= 468;
	uiSinglePlayer.loadGame.generic.width		= 198;
	uiSinglePlayer.loadGame.generic.height		= 38;
	uiSinglePlayer.loadGame.generic.callback	= UI_SinglePlayer_Callback;
	uiSinglePlayer.loadGame.generic.ownerdraw	= UI_SinglePlayer_Ownerdraw;
	uiSinglePlayer.loadGame.pic					= UI_LOADBUTTON;

	uiSinglePlayer.saveGame.generic.id			= ID_SAVEGAME;
	uiSinglePlayer.saveGame.generic.type		= QMTYPE_BITMAP;
	uiSinglePlayer.saveGame.generic.x			= 676;
	uiSinglePlayer.saveGame.generic.y			= 516;
	uiSinglePlayer.saveGame.generic.width		= 198;
	uiSinglePlayer.saveGame.generic.height		= 38;
	uiSinglePlayer.saveGame.generic.callback	= UI_SinglePlayer_Callback;
	uiSinglePlayer.saveGame.generic.ownerdraw	= UI_SinglePlayer_Ownerdraw;
	uiSinglePlayer.saveGame.pic					= UI_SAVEBUTTON;

	UI_SinglePlayer_GetMissionList();

	uiSinglePlayer.currentSkill = ui_singlePlayerSkill->integerValue;
	if (uiSinglePlayer.currentSkill > 3)
		uiSinglePlayer.currentSkill = 3;
	else if (uiSinglePlayer.currentSkill < 0)
		uiSinglePlayer.currentSkill = 0;

	switch (uiSinglePlayer.currentSkill){
	case 0:
		uiSinglePlayer.skill.pic = ART_EASY;
		break;
	case 1:
		uiSinglePlayer.skill.pic = ART_MEDIUM;
		break;
	case 2:
		uiSinglePlayer.skill.pic = ART_HARD;
		break;
	case 3:
		uiSinglePlayer.skill.pic = ART_HARDPLUS;
		break;
	}

	UI_AddItem(&uiSinglePlayer.menu, (void *)&uiSinglePlayer.background);
	UI_AddItem(&uiSinglePlayer.menu, (void *)&uiSinglePlayer.banner);
	UI_AddItem(&uiSinglePlayer.menu, (void *)&uiSinglePlayer.back);
	UI_AddItem(&uiSinglePlayer.menu, (void *)&uiSinglePlayer.start);
	UI_AddItem(&uiSinglePlayer.menu, (void *)&uiSinglePlayer.levelShot);
	UI_AddItem(&uiSinglePlayer.menu, (void *)&uiSinglePlayer.missionList);
	UI_AddItem(&uiSinglePlayer.menu, (void *)&uiSinglePlayer.skill);
	UI_AddItem(&uiSinglePlayer.menu, (void *)&uiSinglePlayer.loadGame);
	UI_AddItem(&uiSinglePlayer.menu, (void *)&uiSinglePlayer.saveGame);
}

/*
 ==================
 UI_SinglePlayer_Precache
 ==================
*/
void UI_SinglePlayer_Precache (){

	R_RegisterMaterialNoMip(ART_BACKGROUND);
	R_RegisterMaterialNoMip(ART_BANNER);
	R_RegisterMaterialNoMip(ART_LEVELSHOTBLUR);
	R_RegisterMaterialNoMip(ART_LISTBLUR);
	R_RegisterMaterialNoMip(ART_EASY);
	R_RegisterMaterialNoMip(ART_MEDIUM);
	R_RegisterMaterialNoMip(ART_HARD);
	R_RegisterMaterialNoMip(ART_HARDPLUS);
}

/*
 ==================
 UI_SinglePlayer_Menu
 ==================
*/
void UI_SinglePlayer_Menu (){

	UI_SinglePlayer_Precache();
	UI_SinglePlayer_Init();

	UI_PushMenu(&uiSinglePlayer.menu);
}