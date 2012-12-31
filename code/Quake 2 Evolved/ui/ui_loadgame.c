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
// ui_loadgame.c - Load game menu
//

// TODO:
// - uiLoadGameGameId_t


#include "ui_local.h"


#define ART_BACKGROUND				"ui/assets/ui/ui_sub_singleplayer"
#define ART_BANNER					"ui/assets/banners/loadgame_t"
#define ART_LISTBACK				"ui/assets/segments/files_box"
#define ART_LEVELSHOTBLUR			"ui/assets/segments/sp_mapshot"

typedef enum {
	ID_BACKGROUND,
	ID_BANNER,

	ID_BACK,
	ID_LOAD,

	ID_LISTBACK,
	ID_GAMETITLE,
	ID_LISTGAMES,
	ID_LEVELSHOT,

	ID_NEWGAME,
	ID_SAVEGAME,
	ID_DELETEGAME
} uiLoadGameGameId_t;

#if 0
#define ID_BACKGROUND		0
#define ID_BANNER			1

#define ID_BACK				2
#define ID_LOAD				3

#define ID_LISTBACK			4
#define ID_GAMETITLE		5
#define ID_LISTGAMES		6
#define ID_LEVELSHOT		20

#define ID_NEWGAME			21
#define ID_SAVEGAME			22
#define ID_DELETEGAME		23
#endif

typedef struct {
	char					map[80];
	char					time[8];
	char					date[8];
	char					name[32];
	bool					valid;
} uiLoadGameGame_t;

static color_t				uiLoadGameColor = {0, 76, 127, 255};

typedef struct {
	uiLoadGameGame_t		games[14];
	int						currentGame;

	int						currentLevelShot;
	int						fadeTime;

	menuFramework_t			menu;

	menuBitmap_t			background;
	menuBitmap_t			banner;

	menuBitmap_t			back;
	menuBitmap_t			load;

	menuBitmap_t			listBack;
	menuAction_t			gameTitle;
	menuAction_t			listGames[14];	
	menuBitmap_t			levelShot;

	menuBitmap_t			newGame;
	menuBitmap_t			saveGame;
	menuBitmap_t			deleteGame;
} uiLoadGame_t;

static uiLoadGame_t			uiLoadGame;


/*
 ==================
 UI_LoadGame_GetGameList
 ==================
*/
static void UI_LoadGame_GetGameList (){

	char	name[MAX_PATH_LENGTH];
	char	*buffer;
	int		i;

	for (i = 0; i < 14; i++){
		Str_SPrintf(name, sizeof(name), "save/save%i/server.ssv", i);
		FS_ReadFile(name, (void **)&buffer);
		if (!buffer){
			Str_Copy(uiLoadGame.games[i].map, "", sizeof(uiLoadGame.games[i].map));
			Str_Copy(uiLoadGame.games[i].time, "", sizeof(uiLoadGame.games[i].time));
			Str_Copy(uiLoadGame.games[i].date, "", sizeof(uiLoadGame.games[i].date));
			Str_Copy(uiLoadGame.games[i].name, "<EMPTY>", sizeof(uiLoadGame.games[i].name));
			uiLoadGame.games[i].valid = false;

			continue;
		}

		if (Str_FindText(buffer, "ENTERING", false)){
			Mem_Copy(uiLoadGame.games[i].map, buffer+32, 80-32);
			Str_Copy(uiLoadGame.games[i].time, "", sizeof(uiLoadGame.games[i].time));
			Str_Copy(uiLoadGame.games[i].date, "", sizeof(uiLoadGame.games[i].date));
			Mem_Copy(uiLoadGame.games[i].name, buffer, 32);
		}
		else {
			Mem_Copy(uiLoadGame.games[i].map, buffer+32, 80-32);
			Mem_Copy(uiLoadGame.games[i].time, buffer, 5);
			Mem_Copy(uiLoadGame.games[i].date, buffer+6, 5);
			Mem_Copy(uiLoadGame.games[i].name, buffer+13, 32-13);
		}
		uiLoadGame.games[i].valid = true;

		FS_FreeFile(buffer);
	}

	// Select first valid slot
	for (i = 0; i < 14; i++){
		if (uiLoadGame.games[i].valid){
			uiLoadGame.listGames[i].generic.color = uiLoadGameColor;
			uiLoadGame.currentGame = i;
			break;
		}
	}

	// Couldn't find a valid slot, so gray load button
	if (i == 14)
		uiLoadGame.load.generic.flags |= QMF_GRAYED;
}

/*
 ==================
 UI_LoadGame_Callback
 ==================
*/
static void UI_LoadGame_Callback (void *self, int event){

	menuCommon_t	*item = (menuCommon_t *)self;

	if (event != QM_ACTIVATED)
		return;

	if (item->type == QMTYPE_ACTION){
		// Reset color, get current game, set color
		uiLoadGame.listGames[uiLoadGame.currentGame].generic.color = colorWhite;
		uiLoadGame.currentGame = item->id - ID_LISTGAMES;
		uiLoadGame.listGames[uiLoadGame.currentGame].generic.color = uiLoadGameColor;

		// Restart levelshot animation
		uiLoadGame.currentLevelShot = 0;
		uiLoadGame.fadeTime = uiStatic.realTime;
		return;
	}
	
	switch (item->id){
	case ID_BACK:
		if (cls.state == CA_ACTIVE)
			UI_InGame_Menu();
		else
			UI_Main_Menu();

		break;
	case ID_LOAD:
		if (uiLoadGame.games[uiLoadGame.currentGame].valid)
			Cmd_ExecuteText(CMD_EXEC_APPEND, Str_VarArgs("loadGame save%i\n", uiLoadGame.currentGame));

		break;
	case ID_NEWGAME:
		UI_SinglePlayer_Menu();
		break;
	case ID_SAVEGAME:
		UI_SaveGame_Menu();
		break;
	case ID_DELETEGAME:
		Cmd_ExecuteText(CMD_EXEC_NOW, Str_VarArgs("deleteGame save%i\n", uiLoadGame.currentGame));
		UI_LoadGame_GetGameList();
		break;
	}
}

/*
 ==================
 UI_LoadGame_Ownerdraw
 ==================
*/
static void UI_LoadGame_Ownerdraw (void *self){

	menuCommon_t	*item = (menuCommon_t *)self;

	if (item->type == QMTYPE_ACTION){
		color_t		color;
		char		*time, *date, *name;
		bool		centered;

		if (item->id == ID_GAMETITLE){
			time = "Time";
			date = "Date";
			name = "Map Name";
			centered = false;
		}
		else {
			time = uiLoadGame.games[item->id - ID_LISTGAMES].time;
			date = uiLoadGame.games[item->id - ID_LISTGAMES].date;
			name = uiLoadGame.games[item->id - ID_LISTGAMES].name;
			centered = !uiLoadGame.games[item->id - ID_LISTGAMES].valid;

			if (Str_FindText(uiLoadGame.games[item->id - ID_LISTGAMES].name, "ENTERING", false))
				centered = true;
		}

		if (!centered){
			UI_DrawStringOLD(item->x, item->y, 82*uiStatic.scaleX, item->height, time, item->color, true, item->charWidth, item->charHeight, 1, true);
			UI_DrawStringOLD(item->x + 83*uiStatic.scaleX, item->y, 82*uiStatic.scaleX, item->height, date, item->color, true, item->charWidth, item->charHeight, 1, true);
			UI_DrawStringOLD(item->x + 83*uiStatic.scaleX + 83*uiStatic.scaleX, item->y, 296*uiStatic.scaleX, item->height, name, item->color, true, item->charWidth, item->charHeight, 1, true);
		}
		else
			UI_DrawStringOLD(item->x, item->y, item->width, item->height, name, item->color, true, item->charWidth, item->charHeight, 1, true);

		if (self != UI_ItemAtCursor(item->parent))
			return;

		*(unsigned *)color = *(unsigned *)item->color;
		color[3] = 255 * (0.5 + 0.5 * sin(uiStatic.realTime / UI_PULSE_DIVISOR));

		if (!centered){
			UI_DrawStringOLD(item->x, item->y, 82*uiStatic.scaleX, item->height, time, color, true, item->charWidth, item->charHeight, 1, true);
			UI_DrawStringOLD(item->x + 83*uiStatic.scaleX, item->y, 82*uiStatic.scaleX, item->height, date, color, true, item->charWidth, item->charHeight, 1, true);
			UI_DrawStringOLD(item->x + 83*uiStatic.scaleX + 83*uiStatic.scaleX, item->y, 296*uiStatic.scaleX, item->height, name, color, true, item->charWidth, item->charHeight, 1, true);
		}
		else
			UI_DrawStringOLD(item->x, item->y, item->width, item->height, name, color, true, item->charWidth, item->charHeight, 1, true);
	}
	else {
		if (item->id == ID_LEVELSHOT){
			int			x, y, w, h;
			int			prev;
			color_t		color = {255, 255, 255, 255};

			// Draw the levelshot
			x = 570;
			y = 210;
			w = 410;
			h = 202;
		
			UI_ScaleCoords(&x, &y, &w, &h);

			if (uiLoadGame.games[uiLoadGame.currentGame].map[0]){
				char	pathTGA[MAX_PATH_LENGTH];

				if (uiStatic.realTime - uiLoadGame.fadeTime >= 3000){
					uiLoadGame.fadeTime = uiStatic.realTime;

					uiLoadGame.currentLevelShot++;
					if (uiLoadGame.currentLevelShot == 3)
						uiLoadGame.currentLevelShot = 0;
				}

				prev = uiLoadGame.currentLevelShot - 1;
				if (prev < 0)
					prev = 2;

				color[3] = Clamp((float)(uiStatic.realTime - uiLoadGame.fadeTime) / 1000, 0.0f, 1.0f) * 255;

				Str_SPrintf(pathTGA, sizeof(pathTGA), "ui/menu_levelshots/%s_1.tga", uiLoadGame.games[uiLoadGame.currentGame].map);

				if (!FS_FileExists(pathTGA))
					UI_DrawPicOLD(x, y, w, h, colorWhite, "ui/menu_levelshots/unknownmap");
				else {
					UI_DrawPicOLD(x, y, w, h, colorWhite, Str_VarArgs("ui/menu_levelshots/%s_%i", uiLoadGame.games[uiLoadGame.currentGame].map, prev+1));
					UI_DrawPicOLD(x, y, w, h, color, Str_VarArgs("ui/menu_levelshots/%s_%i", uiLoadGame.games[uiLoadGame.currentGame].map, uiLoadGame.currentLevelShot+1));
				}
			}
			else
				UI_DrawPicOLD(x, y, w, h, colorWhite, "ui/menu_levelshots/unknownmap");

			// Draw the blurred frame
			UI_DrawPicOLD(item->x, item->y, item->width, item->height, colorWhite, ((menuBitmap_t *)self)->pic);
		}
		else {
			if (uiLoadGame.menu.items[uiLoadGame.menu.cursor] == self)
				UI_DrawPicOLD(item->x, item->y, item->width, item->height, colorWhite, UI_MOVEBOXFOCUS);
			else
				UI_DrawPicOLD(item->x, item->y, item->width, item->height, colorWhite, UI_MOVEBOX);

			UI_DrawPicOLD(item->x, item->y, item->width, item->height, colorWhite, ((menuBitmap_t *)self)->pic);
		}
	}
}

/*
 ==================
 UI_LoadGame_Init
 ==================
*/
static void UI_LoadGame_Init (){

	int		y;
	int		i;

	Mem_Fill(&uiLoadGame, 0, sizeof(uiLoadGame_t));

	uiLoadGame.fadeTime = uiStatic.realTime;

	uiLoadGame.background.generic.id			= ID_BACKGROUND;
	uiLoadGame.background.generic.type			= QMTYPE_BITMAP;
	uiLoadGame.background.generic.flags			= QMF_INACTIVE;
	uiLoadGame.background.generic.x				= 0;
	uiLoadGame.background.generic.y				= 0;
	uiLoadGame.background.generic.width			= 1024;
	uiLoadGame.background.generic.height		= 768;
	uiLoadGame.background.pic					= ART_BACKGROUND;

	uiLoadGame.banner.generic.id				= ID_BANNER;
	uiLoadGame.banner.generic.type				= QMTYPE_BITMAP;
	uiLoadGame.banner.generic.flags				= QMF_INACTIVE;
	uiLoadGame.banner.generic.x					= 0;
	uiLoadGame.banner.generic.y					= 66;
	uiLoadGame.banner.generic.width				= 1024;
	uiLoadGame.banner.generic.height			= 46;
	uiLoadGame.banner.pic						= ART_BANNER;

	uiLoadGame.back.generic.id					= ID_BACK;
	uiLoadGame.back.generic.type				= QMTYPE_BITMAP;
	uiLoadGame.back.generic.x					= 310;
	uiLoadGame.back.generic.y					= 656;
	uiLoadGame.back.generic.width				= 198;
	uiLoadGame.back.generic.height				= 38;
	uiLoadGame.back.generic.callback			= UI_LoadGame_Callback;
	uiLoadGame.back.generic.ownerdraw			= UI_LoadGame_Ownerdraw;
	uiLoadGame.back.pic							= UI_BACKBUTTON;

	uiLoadGame.load.generic.id					= ID_LOAD;
	uiLoadGame.load.generic.type				= QMTYPE_BITMAP;
	uiLoadGame.load.generic.x					= 516;
	uiLoadGame.load.generic.y					= 656;
	uiLoadGame.load.generic.width				= 198;
	uiLoadGame.load.generic.height				= 38;
	uiLoadGame.load.generic.callback			= UI_LoadGame_Callback;
	uiLoadGame.load.generic.ownerdraw			= UI_LoadGame_Ownerdraw;
	uiLoadGame.load.pic							= UI_LOADBUTTON;

	uiLoadGame.listBack.generic.id				= ID_LISTBACK;
	uiLoadGame.listBack.generic.type			= QMTYPE_BITMAP;
	uiLoadGame.listBack.generic.flags			= QMF_INACTIVE;
	uiLoadGame.listBack.generic.x				= 42;
	uiLoadGame.listBack.generic.y				= 146;
	uiLoadGame.listBack.generic.width			= 462;
	uiLoadGame.listBack.generic.height			= 476;
	uiLoadGame.listBack.pic						= ART_LISTBACK;

	uiLoadGame.gameTitle.generic.id				= ID_GAMETITLE;
	uiLoadGame.gameTitle.generic.type			= QMTYPE_ACTION;
	uiLoadGame.gameTitle.generic.flags			= QMF_INACTIVE;
	uiLoadGame.gameTitle.generic.x				= 42;
	uiLoadGame.gameTitle.generic.y				= 146;
	uiLoadGame.gameTitle.generic.width			= 462;
	uiLoadGame.gameTitle.generic.height			= 24;
	uiLoadGame.gameTitle.generic.ownerdraw		= UI_LoadGame_Ownerdraw;

	for (i = 0, y = 182; i < 14; i++, y += 32){
		uiLoadGame.listGames[i].generic.id			= ID_LISTGAMES+i;
		uiLoadGame.listGames[i].generic.type		= QMTYPE_ACTION;
		uiLoadGame.listGames[i].generic.flags		= QMF_SILENT;
		uiLoadGame.listGames[i].generic.x			= 42;
		uiLoadGame.listGames[i].generic.y			= y;
		uiLoadGame.listGames[i].generic.width		= 462;
		uiLoadGame.listGames[i].generic.height		= 24;
		uiLoadGame.listGames[i].generic.callback	= UI_LoadGame_Callback;
		uiLoadGame.listGames[i].generic.ownerdraw	= UI_LoadGame_Ownerdraw;
	}

	uiLoadGame.levelShot.generic.id				= ID_LEVELSHOT;
	uiLoadGame.levelShot.generic.type			= QMTYPE_BITMAP;
	uiLoadGame.levelShot.generic.flags			= QMF_INACTIVE;
	uiLoadGame.levelShot.generic.x				= 568;
	uiLoadGame.levelShot.generic.y				= 208;
	uiLoadGame.levelShot.generic.width			= 414;
	uiLoadGame.levelShot.generic.height			= 206;
	uiLoadGame.levelShot.generic.ownerdraw		= UI_LoadGame_Ownerdraw;
	uiLoadGame.levelShot.pic					= ART_LEVELSHOTBLUR;

	uiLoadGame.newGame.generic.id				= ID_NEWGAME;
	uiLoadGame.newGame.generic.type				= QMTYPE_BITMAP;
	uiLoadGame.newGame.generic.x				= 676;
	uiLoadGame.newGame.generic.y				= 468;
	uiLoadGame.newGame.generic.width			= 198;
	uiLoadGame.newGame.generic.height			= 38;
	uiLoadGame.newGame.generic.callback			= UI_LoadGame_Callback;
	uiLoadGame.newGame.generic.ownerdraw		= UI_LoadGame_Ownerdraw;
	uiLoadGame.newGame.pic						= UI_NEWGAMEBUTTON;

	uiLoadGame.saveGame.generic.id				= ID_SAVEGAME;
	uiLoadGame.saveGame.generic.type			= QMTYPE_BITMAP;
	uiLoadGame.saveGame.generic.x				= 676;
	uiLoadGame.saveGame.generic.y				= 516;
	uiLoadGame.saveGame.generic.width			= 198;
	uiLoadGame.saveGame.generic.height			= 38;
	uiLoadGame.saveGame.generic.callback		= UI_LoadGame_Callback;
	uiLoadGame.saveGame.generic.ownerdraw		= UI_LoadGame_Ownerdraw;
	uiLoadGame.saveGame.pic						= UI_SAVEBUTTON;

	uiLoadGame.deleteGame.generic.id			= ID_DELETEGAME;
	uiLoadGame.deleteGame.generic.type			= QMTYPE_BITMAP;
	uiLoadGame.deleteGame.generic.x				= 676;
	uiLoadGame.deleteGame.generic.y				= 564;
	uiLoadGame.deleteGame.generic.width			= 198;
	uiLoadGame.deleteGame.generic.height		= 38;
	uiLoadGame.deleteGame.generic.callback		= UI_LoadGame_Callback;
	uiLoadGame.deleteGame.generic.ownerdraw		= UI_LoadGame_Ownerdraw;
	uiLoadGame.deleteGame.pic					= UI_DELETEBUTTON;

	UI_LoadGame_GetGameList();

	UI_AddItem(&uiLoadGame.menu, (void *)&uiLoadGame.background);
	UI_AddItem(&uiLoadGame.menu, (void *)&uiLoadGame.banner);
	UI_AddItem(&uiLoadGame.menu, (void *)&uiLoadGame.back);
	UI_AddItem(&uiLoadGame.menu, (void *)&uiLoadGame.load);
	UI_AddItem(&uiLoadGame.menu, (void *)&uiLoadGame.listBack);
	UI_AddItem(&uiLoadGame.menu, (void *)&uiLoadGame.gameTitle);

	for (i = 0; i < 14; i++)
		UI_AddItem(&uiLoadGame.menu, (void *)&uiLoadGame.listGames[i]);

	UI_AddItem(&uiLoadGame.menu, (void *)&uiLoadGame.levelShot);
	UI_AddItem(&uiLoadGame.menu, (void *)&uiLoadGame.newGame);
	UI_AddItem(&uiLoadGame.menu, (void *)&uiLoadGame.saveGame);
	UI_AddItem(&uiLoadGame.menu, (void *)&uiLoadGame.deleteGame);
}

/*
 ==================
 UI_LoadGame_Precache
 ==================
*/
void UI_LoadGame_Precache (){

	R_RegisterMaterialNoMip(ART_BACKGROUND);
	R_RegisterMaterialNoMip(ART_BANNER);
	R_RegisterMaterialNoMip(ART_LISTBACK);
	R_RegisterMaterialNoMip(ART_LEVELSHOTBLUR);
}

/*
 ==================
 UI_LoadGame_Menu
 ==================
*/
void UI_LoadGame_Menu (){

	UI_LoadGame_Precache();
	UI_LoadGame_Init();

	UI_PushMenu(&uiLoadGame.menu);
}