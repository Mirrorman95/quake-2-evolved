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
// ui_demos.c - Demos menu
//


#include "ui_local.h"


#define ART_BACKGROUND				"ui/assets/ui/ui_sub_demos"
#define ART_BANNER					"ui/assets/banners/demos_t"

#define MAX_DEMOS					64

typedef enum {
	ID_BACKGROUND,
	ID_BANNER,

	ID_BACK,
	ID_LOAD,

	ID_DEMOLIST
} uiDemosId_t;

typedef struct {
	char					demos[MAX_DEMOS][MAX_QPATH];
	char					*demosPtr[MAX_DEMOS];

	menuFramework_t			menu;

	menuBitmap_t			background;
	menuBitmap_t			banner;

	menuBitmap_t			back;
	menuBitmap_t			load;

	menuScrollList_t		demoList;
} uiDemos_t;

static uiDemos_t			uiDemos;


/*
 ==================
 UI_Demos_GetDemoList
 ==================
*/
static void UI_Demos_GetDemoList (){

	const char	**fileList;
	int			numFiles;
	int			i, count = 0;

	fileList = FS_ListFiles("demos", ".dm2", true, &numFiles);

	if (numFiles > MAX_DEMOS)
		numFiles = MAX_DEMOS;

	for (i = 0; i < numFiles; i++){
		Str_Copy(uiDemos.demos[count], fileList[i], sizeof(uiDemos.demos[count]));
		*strchr(uiDemos.demos[count], '.') = 0;
		count++;
	}

	FS_FreeFileList(fileList);

	for (i = 0; i < count; i++)
		uiDemos.demosPtr[i] = uiDemos.demos[i];
	for ( ; i < MAX_DEMOS; i++)
		uiDemos.demosPtr[i] = NULL;

	uiDemos.demoList.itemNames = uiDemos.demosPtr;
}

/*
 ==================
 UI_Demos_Callback
 ==================
*/
static void UI_Demos_Callback (void *self, int event){

	menuCommon_t	*item = (menuCommon_t *)self;

	if (event != QM_ACTIVATED)
		return;

	switch (item->id){
	case ID_BACK:
		UI_PopMenu();
		break;
	case ID_LOAD:
		Cmd_ExecuteText(CMD_EXEC_APPEND, Str_VarArgs("demo %s\n", uiDemos.demos[uiDemos.demoList.curItem]));
		break;
	}
}

/*
 ==================
 UI_Demos_Ownerdraw
 ==================
*/
static void UI_Demos_Ownerdraw (void *self){

	menuCommon_t	*item = (menuCommon_t *)self;

	if (uiDemos.menu.items[uiDemos.menu.cursor] == self)
		UI_DrawPic(item->x, item->y, item->width, item->height, colorWhite, UI_MOVEBOXFOCUS);
	else
		UI_DrawPic(item->x, item->y, item->width, item->height, colorWhite, UI_MOVEBOX);

	UI_DrawPic(item->x, item->y, item->width, item->height, colorWhite, ((menuBitmap_t *)self)->pic);
}

/*
 ==================
 UI_Demos_Init
 ==================
*/
static void UI_Demos_Init (){

	Mem_Fill(&uiDemos, 0, sizeof(uiDemos_t));

	uiDemos.background.generic.id				= ID_BACKGROUND;
	uiDemos.background.generic.type				= QMTYPE_BITMAP;
	uiDemos.background.generic.flags			= QMF_INACTIVE;
	uiDemos.background.generic.x				= 0;
	uiDemos.background.generic.y				= 0;
	uiDemos.background.generic.width			= 1024;
	uiDemos.background.generic.height			= 768;
	uiDemos.background.pic						= ART_BACKGROUND;

	uiDemos.banner.generic.id					= ID_BANNER;
	uiDemos.banner.generic.type					= QMTYPE_BITMAP;
	uiDemos.banner.generic.flags				= QMF_INACTIVE;
	uiDemos.banner.generic.x					= 0;
	uiDemos.banner.generic.y					= 66;
	uiDemos.banner.generic.width				= 1024;
	uiDemos.banner.generic.height				= 46;
	uiDemos.banner.pic							= ART_BANNER;

	uiDemos.back.generic.id						= ID_BACK;
	uiDemos.back.generic.type					= QMTYPE_BITMAP;
	uiDemos.back.generic.x						= 310;
	uiDemos.back.generic.y						= 656;
	uiDemos.back.generic.width					= 198;
	uiDemos.back.generic.height					= 38;
	uiDemos.back.generic.callback				= UI_Demos_Callback;
	uiDemos.back.generic.ownerdraw				= UI_Demos_Ownerdraw;
	uiDemos.back.pic							= UI_BACKBUTTON;

	uiDemos.load.generic.id						= ID_LOAD;
	uiDemos.load.generic.type					= QMTYPE_BITMAP;
	uiDemos.load.generic.x						= 516;
	uiDemos.load.generic.y						= 656;
	uiDemos.load.generic.width					= 198;
	uiDemos.load.generic.height					= 38;
	uiDemos.load.generic.callback				= UI_Demos_Callback;
	uiDemos.load.generic.ownerdraw				= UI_Demos_Ownerdraw;
	uiDemos.load.pic							= UI_LOADBUTTON;

	uiDemos.demoList.generic.id					= ID_DEMOLIST;
	uiDemos.demoList.generic.type				= QMTYPE_SCROLLLIST;
	uiDemos.demoList.generic.flags				= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiDemos.demoList.generic.x					= 256;
	uiDemos.demoList.generic.y					= 208;
	uiDemos.demoList.generic.width				= 512;
	uiDemos.demoList.generic.height				= 352;

	UI_Demos_GetDemoList();

	UI_AddItem(&uiDemos.menu, (void *)&uiDemos.background);
	UI_AddItem(&uiDemos.menu, (void *)&uiDemos.banner);
	UI_AddItem(&uiDemos.menu, (void *)&uiDemos.back);
	UI_AddItem(&uiDemos.menu, (void *)&uiDemos.load);
	UI_AddItem(&uiDemos.menu, (void *)&uiDemos.demoList);
}

/*
 ==================
 UI_Demos_Precache
 ==================
*/
void UI_Demos_Precache (){

	R_RegisterMaterialNoMip(ART_BACKGROUND);
	R_RegisterMaterialNoMip(ART_BANNER);
}

/*
 ==================
 UI_Demos_Menu
 ==================
*/
void UI_Demos_Menu (){

	UI_Demos_Precache();
	UI_Demos_Init();

	UI_PushMenu(&uiDemos.menu);
}