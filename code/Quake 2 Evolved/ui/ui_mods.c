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
// ui_mods.c - Mods menu
//


#include "ui_local.h"


#define ART_BACKGROUND				"ui/assets/ui/ui_sub_modifications"
#define ART_BANNER					"ui/assets/banners/mods_t"

#define MAX_MODS					128
#define MAX_MODDESC					48

typedef enum {
	ID_BACKGROUND,
	ID_BANNER,

	ID_BACK,
	ID_LOAD,

	ID_MODLIST
} uiModsId_t;

typedef struct {
	char					modsDir[MAX_MODS][MAX_PATH_LENGTH];
	char					modsDescription[MAX_MODS][MAX_MODDESC];
	char *					modsDescriptionPtr[MAX_MODS];

	menuFramework_t			menu;

	menuBitmap_t			background;
	menuBitmap_t			banner;

	menuBitmap_t			back;
	menuBitmap_t			load;

	menuScrollList_t		modList;
} uiMods_t;

static uiMods_t				uiMods;


/*
 ==================
 UI_Mods_GetModList
 ==================
*/
static void UI_Mods_GetModList (){

	modList_t	**modList;
	int			numMods;
	int			i, count = 0;

	// Always start off with baseq2
	Str_Copy(uiMods.modsDir[count], "baseq2", sizeof(uiMods.modsDir[count]));
	Str_Copy(uiMods.modsDescription[count], "Quake II", sizeof(uiMods.modsDescription[count]));
	count++;

	modList = FS_ListMods(true, &numMods);

	if (numMods > MAX_MODS)
		numMods = MAX_MODS;

	for (i = 0; i < numMods; i++){
		Str_Copy(uiMods.modsDir[count], modList[i]->directory, sizeof(uiMods.modsDir[count]));

		// Special check for CTF and mission packs
		if (!Str_ICompare(modList[i]->directory, "ctf"))
			Str_Copy(uiMods.modsDescription[count], "Quake II: Capture The Flag", sizeof(uiMods.modsDescription[count]));
		else if (!Str_ICompare(modList[i]->directory, "xatrix"))
			Str_Copy(uiMods.modsDescription[count], "Quake II: The Reckoning", sizeof(uiMods.modsDescription[count]));
		else if (!Str_ICompare(modList[i]->directory, "rogue"))
			Str_Copy(uiMods.modsDescription[count], "Quake II: Ground Zero", sizeof(uiMods.modsDescription[count]));
		else
			Str_Copy(uiMods.modsDescription[count], modList[i]->description, sizeof(uiMods.modsDescription[count]));

		count++;
	}

	FS_FreeModList(modList);

	for (i = 0; i < count; i++)
		uiMods.modsDescriptionPtr[i] = uiMods.modsDescription[i];
	for ( ; i < MAX_MODS; i++)
		uiMods.modsDescriptionPtr[i] = NULL;

	uiMods.modList.itemNames = uiMods.modsDescriptionPtr;

	// See if the load button should be grayed
	if (!Str_ICompare(CVar_GetVariableString("fs_game"), uiMods.modsDir[0]))
		uiMods.load.generic.flags |= QMF_GRAYED;
}

/*
 ==================
 UI_Mods_Callback
 ==================
*/
static void UI_Mods_Callback (void *self, int event){

	menuCommon_t	*item = (menuCommon_t *)self;

	if (event == QM_CHANGED){
		// See if the load button should be grayed
		if (!Str_ICompare(CVar_GetVariableString("fs_game"), uiMods.modsDir[uiMods.modList.curItem]))
			uiMods.load.generic.flags |= QMF_GRAYED;
		else
			uiMods.load.generic.flags &= ~QMF_GRAYED;

		return;
	}

	if (event != QM_ACTIVATED)
		return;

	switch (item->id){
	case ID_BACK:
		UI_PopMenu();
		break;
	case ID_LOAD:
		if (cls.state == CA_ACTIVE)
			break;		// Don't fuck up the game

		// Update fs_game
		CVar_ForceSet("fs_game", uiMods.modsDir[uiMods.modList.curItem]);

		// Restart file system
		FS_Restart();

		// Flush all data so it will be forced to reload
		Cmd_ExecuteText(CMD_EXEC_APPEND, "restartVideo\n");

		break;
	}
}

/*
 ==================
 UI_Mods_Ownerdraw
 ==================
*/
static void UI_Mods_Ownerdraw (void *self){

	menuCommon_t	*item = (menuCommon_t *)self;

	if (uiMods.menu.items[uiMods.menu.cursor] == self)
		UI_DrawPic(item->x, item->y, item->width, item->height, colorWhite, UI_MOVEBOXFOCUS);
	else
		UI_DrawPic(item->x, item->y, item->width, item->height, colorWhite, UI_MOVEBOX);

	UI_DrawPic(item->x, item->y, item->width, item->height, colorWhite, ((menuBitmap_t *)self)->pic);
}

/*
 ==================
 UI_Mods_Init
 ==================
*/
static void UI_Mods_Init (){

	Mem_Fill(&uiMods, 0, sizeof(uiMods_t));

	uiMods.background.generic.id				= ID_BACKGROUND;
	uiMods.background.generic.type				= QMTYPE_BITMAP;
	uiMods.background.generic.flags				= QMF_INACTIVE;
	uiMods.background.generic.x					= 0;
	uiMods.background.generic.y					= 0;
	uiMods.background.generic.width				= 1024;
	uiMods.background.generic.height			= 768;
	uiMods.background.pic						= ART_BACKGROUND;

	uiMods.banner.generic.id					= ID_BANNER;
	uiMods.banner.generic.type					= QMTYPE_BITMAP;
	uiMods.banner.generic.flags					= QMF_INACTIVE;
	uiMods.banner.generic.x						= 0;
	uiMods.banner.generic.y						= 66;
	uiMods.banner.generic.width					= 1024;
	uiMods.banner.generic.height				= 46;
	uiMods.banner.pic							= ART_BANNER;

	uiMods.back.generic.id						= ID_BACK;
	uiMods.back.generic.type					= QMTYPE_BITMAP;
	uiMods.back.generic.x						= 310;
	uiMods.back.generic.y						= 656;
	uiMods.back.generic.width					= 198;
	uiMods.back.generic.height					= 38;
	uiMods.back.generic.callback				= UI_Mods_Callback;
	uiMods.back.generic.ownerdraw				= UI_Mods_Ownerdraw;
	uiMods.back.pic								= UI_BACKBUTTON;

	uiMods.load.generic.id						= ID_LOAD;
	uiMods.load.generic.type					= QMTYPE_BITMAP;
	uiMods.load.generic.x						= 516;
	uiMods.load.generic.y						= 656;
	uiMods.load.generic.width					= 198;
	uiMods.load.generic.height					= 38;
	uiMods.load.generic.callback				= UI_Mods_Callback;
	uiMods.load.generic.ownerdraw				= UI_Mods_Ownerdraw;
	uiMods.load.pic								= UI_LOADBUTTON;

	uiMods.modList.generic.id					= ID_MODLIST;
	uiMods.modList.generic.type					= QMTYPE_SCROLLLIST;
	uiMods.modList.generic.flags				= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiMods.modList.generic.x					= 256;
	uiMods.modList.generic.y					= 208;
	uiMods.modList.generic.width				= 512;
	uiMods.modList.generic.height				= 352;
	uiMods.modList.generic.callback				= UI_Mods_Callback;

	UI_Mods_GetModList();

	UI_AddItem(&uiMods.menu, (void *)&uiMods.background);
	UI_AddItem(&uiMods.menu, (void *)&uiMods.banner);
	UI_AddItem(&uiMods.menu, (void *)&uiMods.back);
	UI_AddItem(&uiMods.menu, (void *)&uiMods.load);
	UI_AddItem(&uiMods.menu, (void *)&uiMods.modList);
}

/*
 ==================
 UI_Mods_Precache
 ==================
*/
void UI_Mods_Precache (){

	R_RegisterMaterialNoMip(ART_BACKGROUND);
	R_RegisterMaterialNoMip(ART_BANNER);
}

/*
 ==================
 UI_Mods_Menu
 ==================
*/
void UI_Mods_Menu (){

	UI_Mods_Precache();
	UI_Mods_Init();

	UI_PushMenu(&uiMods.menu);
}