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
// ui_multiplayer.c - Multiplayer menu
//

// TODO!!!


#include "ui_local.h"


#define ART_BACKGROUND				"ui/assets/ui/ui_sub_multiplayer"
#define ART_BANNER					"ui/assets/banners/multiplayer_t"

#define ID_BACKGROUND 0
#define ID_BANNER 1
#define ID_BACK 2

typedef enum {
//	ID_BACKGROUND,
//	ID_BANNER,

//	ID_BACK,

	ID_ADDRESSBOOK,
	ID_REFRESH,
	ID_SERVERS,

	ID_FALLINGDAMAGE,
	ID_WEAPONSSTAY,
	ID_INSTANTPOWERUPS,
	ID_ALLOWPOWERUPS,
	ID_ALLOWHEALTH,
	ID_ALLOWARMOR,
	ID_SPAWNFARTHEST,
	ID_SAMEMAP,
	ID_FORCERESPAWN,
	ID_TEAMPLAY,
	ID_ALLOWEXIT,
	ID_INFINITEAMMO,
	ID_FIXEDFOV,
	ID_QUADDROP,
	ID_FRIENDLYFIRE
} uiDMOptionsId_t;

typedef enum {
//	ID_BACKGROUND,
//	ID_BANNER,

//	ID_BACK,

	ID_MAPLIST,
	ID_RULES,
	ID_TIMELIMIT,
	ID_FRAGLIMIT,
	ID_MAXCLIENTS,
	ID_HOSTNAME,
	ID_DMOPTIONS,
	ID_BEGIN
} uiStartServerId_t;

typedef enum {
//	ID_BACKGROUND,
//	ID_BANNER,

//	ID_BACK,

	ID_JOINSERVER,
	ID_STARTSERVER
} uiMultiPlayerId_t;

static const char *			uiDMOptionsYesNo[] = {
	"False",
	"True"
};

typedef struct {
	menuFramework_t			menu;

	menuBitmap_t			background;
	menuBitmap_t			banner;

	menuBitmap_t			back;

	menuField_t				servers[16];
} uiAddressBook_t;

static uiAddressBook_t		uiAddressBook;

typedef struct {
	menuFramework_t			menu;

	menuBitmap_t			background;
	menuBitmap_t			banner;

	menuBitmap_t			back;

	menuAction_t			addressBook;
	menuAction_t			refresh;
	menuAction_t			servers[10];
} uiJoinServer_t;

static uiJoinServer_t		uiJoinServer;

typedef struct {
	menuFramework_t			menu;

	menuBitmap_t			background;
	menuBitmap_t			banner;

	menuBitmap_t			back;

	menuSpinControl_t		fallingDamage;
	menuSpinControl_t		weaponsStay;
	menuSpinControl_t		instantPowerups;
	menuSpinControl_t		allowPowerups;
	menuSpinControl_t		allowHealth;
	menuSpinControl_t		allowArmor;
	menuSpinControl_t		spawnFarthest;
	menuSpinControl_t		sameMap;
	menuSpinControl_t		forceRespawn;
	menuSpinControl_t		teamPlay;
	menuSpinControl_t		allowExit;
	menuSpinControl_t		infiniteAmmo;
	menuSpinControl_t		fixedFOV;
	menuSpinControl_t		quadDrop;
	menuSpinControl_t		friendlyFire;
} uiDMOptions_t;

static uiDMOptions_t		uiDMOptions;

typedef struct {
	char					names[256][80];
	char					maps[256][80];
	int						numMaps;

	menuFramework_t			menu;

	menuBitmap_t			background;
	menuBitmap_t			banner;

	menuBitmap_t			back;

	menuSpinControl_t		mapList;
	menuSpinControl_t		rules;
	menuField_t				timeLimit;
	menuField_t				fragLimit;
	menuField_t				maxClients;
	menuField_t				hostName;
	menuAction_t			dmOptions;
	menuAction_t			begin;
} uiStartServer_t;

static uiStartServer_t		uiStartServer;

typedef struct {
	menuFramework_t			menu;

	menuBitmap_t			back;

	menuBitmap_t			background;
	menuBitmap_t			banner;

	menuAction_t			joinServer;
	menuAction_t			startServer;
} uiMultiPlayer_t;

static uiMultiPlayer_t		uiMultiPlayer;


/*
 ==================
 
 ==================
*/
static void UI_AddressBook_SaveServers (){

	char	name[32];
	int		i;

	for (i = 0; i < 16; i++){
		Str_SPrintf(name, sizeof(name), "server%i", i+1);
		CVar_SetVariableString(name, uiAddressBook.servers[i].buffer, false);
	}
}

/*
 ==================
 
 ==================
*/
static const char *UI_AddressBook_KeyFunc (int key){

	if (key == K_ESCAPE || key == K_MOUSE2)
		UI_AddressBook_SaveServers();

	return UI_DefaultKey(&uiAddressBook.menu, key);
}

/*
 ==================
 
 ==================
*/
static void UI_AddressBook_Callback (void *self, int event){

	menuCommon_t	*item = (menuCommon_t *)self;

	if (event != QM_ACTIVATED)
		return;

	switch (item->id){
	case ID_BACK:
		UI_AddressBook_SaveServers();
		UI_PopMenu();
		break;
	}
}

/*
 ==================
 
 ==================
*/
static void UI_AddressBook_Ownerdraw (void *self){

	menuCommon_t	*item = (menuCommon_t *)self;

	if (uiAddressBook.menu.items[uiAddressBook.menu.cursor] == self)
		UI_DrawPicOLD(item->x, item->y, item->width, item->height, colorWhite, UI_MOVEBOXFOCUS);
	else
		UI_DrawPicOLD(item->x, item->y, item->width, item->height, colorWhite, UI_MOVEBOX);

	UI_DrawPicOLD(item->x, item->y, item->width, item->height, colorWhite, ((menuBitmap_t *)self)->pic);
}

/*
 ==================
 
 ==================
*/
static void UI_AddressBook_Init (){

	int		y;
	int		i;

	Mem_Fill(&uiAddressBook, 0, sizeof(uiAddressBook_t));

	uiAddressBook.menu.keyFunc = UI_AddressBook_KeyFunc;

	uiAddressBook.background.generic.id			= ID_BACKGROUND;
	uiAddressBook.background.generic.type		= QMTYPE_BITMAP;
	uiAddressBook.background.generic.flags		= QMF_INACTIVE;
	uiAddressBook.background.generic.x			= 0;
	uiAddressBook.background.generic.y			= 0;
	uiAddressBook.background.generic.width		= 1024;
	uiAddressBook.background.generic.height		= 768;
	uiAddressBook.background.pic				= ART_BACKGROUND;

	uiAddressBook.banner.generic.id				= ID_BANNER;
	uiAddressBook.banner.generic.type			= QMTYPE_BITMAP;
	uiAddressBook.banner.generic.flags			= QMF_INACTIVE;
	uiAddressBook.banner.generic.x				= 0;
	uiAddressBook.banner.generic.y				= 66;
	uiAddressBook.banner.generic.width			= 1024;
	uiAddressBook.banner.generic.height			= 46;
	uiAddressBook.banner.pic					= ART_BANNER;

	uiAddressBook.back.generic.id				= ID_BACK;
	uiAddressBook.back.generic.type				= QMTYPE_BITMAP;
	uiAddressBook.back.generic.x				= 413;
	uiAddressBook.back.generic.y				= 656;
	uiAddressBook.back.generic.width			= 198;
	uiAddressBook.back.generic.height			= 38;
	uiAddressBook.back.generic.callback			= UI_AddressBook_Callback;
	uiAddressBook.back.generic.ownerdraw		= UI_AddressBook_Ownerdraw;
	uiAddressBook.back.pic						= UI_BACKBUTTON;

	for (i = 0, y = 128; i < 16; i++, y += 32){
		uiAddressBook.servers[i].generic.id			= ID_SERVERS+i;
		uiAddressBook.servers[i].generic.type		= QMTYPE_FIELD;
		uiAddressBook.servers[i].generic.flags		= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
		uiAddressBook.servers[i].generic.x			= 413;
		uiAddressBook.servers[i].generic.y			= y;
		uiAddressBook.servers[i].generic.width		= 198;
		uiAddressBook.servers[i].generic.height		= 30;
		Str_Copy(uiAddressBook.servers[i].buffer, CVar_GetVariableString(Str_VarArgs("server%i", i+1)), sizeof(uiAddressBook.servers[i].buffer));
	}

	UI_AddItem(&uiAddressBook.menu, (void *)&uiAddressBook.background);
	UI_AddItem(&uiAddressBook.menu, (void *)&uiAddressBook.banner);
	UI_AddItem(&uiAddressBook.menu, (void *)&uiAddressBook.back);

	for (i = 0; i < 16; i++)
		UI_AddItem(&uiAddressBook.menu, (void *)&uiAddressBook.servers[i]);

	UI_PushMenu(&uiAddressBook.menu);
}

/*
 ==================
 
 ==================
*/
static void UI_JoinServer_Callback (void *self, int event){

	menuCommon_t	*item = (menuCommon_t *)self;

	if (event != QM_ACTIVATED)
		return;

	switch (item->id){
	case ID_BACK:
		UI_PopMenu();
		break;
	case ID_ADDRESSBOOK:
		UI_AddressBook_Init();
		break;
	case ID_REFRESH:
		UI_RefreshServerList();
		break;
	default:
		if (Q_stricmp(uiStatic.serverNames[item->id - ID_SERVERS], "<no server>")){
			char	text[128];

			Str_SPrintf(text, sizeof(text), "connect %s\n", NET_AddressToString(uiStatic.serverAddresses[item->id - ID_SERVERS]));
			Cmd_ExecuteText(CMD_EXEC_APPEND, text);
		}
		break;
	}
}

/*
 ==================
 
 ==================
*/
static void UI_JoinServer_Ownerdraw (void *self){

	menuCommon_t	*item = (menuCommon_t *)self;

	if (uiJoinServer.menu.items[uiJoinServer.menu.cursor] == self)
		UI_DrawPicOLD(item->x, item->y, item->width, item->height, colorWhite, UI_MOVEBOXFOCUS);
	else
		UI_DrawPicOLD(item->x, item->y, item->width, item->height, colorWhite, UI_MOVEBOX);

	UI_DrawPicOLD(item->x, item->y, item->width, item->height, colorWhite, ((menuBitmap_t *)self)->pic);
}

/*
 ==================
 
 ==================
*/
static void UI_JoinServer_Init (){

	int		i, y;

	Mem_Fill(&uiJoinServer, 0, sizeof(uiJoinServer_t));

	uiJoinServer.background.generic.id			= ID_BACKGROUND;
	uiJoinServer.background.generic.type		= QMTYPE_BITMAP;
	uiJoinServer.background.generic.flags		= QMF_INACTIVE;
	uiJoinServer.background.generic.x			= 0;
	uiJoinServer.background.generic.y			= 0;
	uiJoinServer.background.generic.width		= 1024;
	uiJoinServer.background.generic.height		= 768;
	uiJoinServer.background.pic					= ART_BACKGROUND;

	uiJoinServer.banner.generic.id				= ID_BANNER;
	uiJoinServer.banner.generic.type			= QMTYPE_BITMAP;
	uiJoinServer.banner.generic.flags			= QMF_INACTIVE;
	uiJoinServer.banner.generic.x				= 0;
	uiJoinServer.banner.generic.y				= 66;
	uiJoinServer.banner.generic.width			= 1024;
	uiJoinServer.banner.generic.height			= 46;
	uiJoinServer.banner.pic						= ART_BANNER;

	uiJoinServer.back.generic.id				= ID_BACK;
	uiJoinServer.back.generic.type				= QMTYPE_BITMAP;
	uiJoinServer.back.generic.x					= 413;
	uiJoinServer.back.generic.y					= 656;
	uiJoinServer.back.generic.width				= 198;
	uiJoinServer.back.generic.height			= 38;
	uiJoinServer.back.generic.callback			= UI_JoinServer_Callback;
	uiJoinServer.back.generic.ownerdraw			= UI_JoinServer_Ownerdraw;
	uiJoinServer.back.pic						= UI_BACKBUTTON;

	uiJoinServer.addressBook.generic.id			= ID_ADDRESSBOOK;
	uiJoinServer.addressBook.generic.name		= "Address Book";
	uiJoinServer.addressBook.generic.type		= QMTYPE_ACTION;
	uiJoinServer.addressBook.generic.flags		= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiJoinServer.addressBook.generic.x			= 413;
	uiJoinServer.addressBook.generic.y			= 160;
	uiJoinServer.addressBook.generic.width		= 198;
	uiJoinServer.addressBook.generic.height		= 30;
	uiJoinServer.addressBook.generic.callback	= UI_JoinServer_Callback;
	uiJoinServer.addressBook.background			= "";

	uiJoinServer.refresh.generic.id				= ID_REFRESH;
	uiJoinServer.refresh.generic.name			= "Refresh Server List";
	uiJoinServer.refresh.generic.type			= QMTYPE_ACTION;
	uiJoinServer.refresh.generic.flags			= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiJoinServer.refresh.generic.x				= 413;
	uiJoinServer.refresh.generic.y				= 192;
	uiJoinServer.refresh.generic.width			= 198;
	uiJoinServer.refresh.generic.height			= 30;
	uiJoinServer.refresh.generic.callback		= UI_JoinServer_Callback;
	uiJoinServer.refresh.background				= "";

	for (i = 0, y = 256; i < 10; i++, y += 32){
		uiJoinServer.servers[i].generic.id			= ID_SERVERS+i;
		uiJoinServer.servers[i].generic.name		= uiStatic.serverNames[i];
		uiJoinServer.servers[i].generic.type		= QMTYPE_ACTION;
		uiJoinServer.servers[i].generic.flags		= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
		uiJoinServer.servers[i].generic.x			= 312;
		uiJoinServer.servers[i].generic.y			= y;
		uiJoinServer.servers[i].generic.width		= 400;
		uiJoinServer.servers[i].generic.height		= 30;
		uiJoinServer.servers[i].generic.callback	= UI_JoinServer_Callback;
		uiJoinServer.servers[i].generic.statusText	= "Press ENTER or click to connect";
		uiJoinServer.servers[i].background			= "";
	}

	UI_AddItem(&uiJoinServer.menu, (void *)&uiJoinServer.background);
	UI_AddItem(&uiJoinServer.menu, (void *)&uiJoinServer.banner);
	UI_AddItem(&uiJoinServer.menu, (void *)&uiJoinServer.back);
	UI_AddItem(&uiJoinServer.menu, (void *)&uiJoinServer.addressBook);
	UI_AddItem(&uiJoinServer.menu, (void *)&uiJoinServer.refresh);

	for (i = 0; i < 10; i++)
		UI_AddItem(&uiJoinServer.menu, (void *)&uiJoinServer.servers[i]);

	UI_RefreshServerList();

	UI_PushMenu(&uiJoinServer.menu);
}

/*
 ==================
 
 ==================
*/
static void UI_DMOptions_Update (){

	uiDMOptions.fallingDamage.generic.name = uiDMOptionsYesNo[(int)uiDMOptions.fallingDamage.curValue];
	
	uiDMOptions.weaponsStay.generic.name = uiDMOptionsYesNo[(int)uiDMOptions.weaponsStay.curValue];

	uiDMOptions.instantPowerups.generic.name = uiDMOptionsYesNo[(int)uiDMOptions.instantPowerups.curValue];

	uiDMOptions.allowPowerups.generic.name = uiDMOptionsYesNo[(int)uiDMOptions.allowPowerups.curValue];

	uiDMOptions.allowHealth.generic.name = uiDMOptionsYesNo[(int)uiDMOptions.allowHealth.curValue];

	uiDMOptions.allowArmor.generic.name = uiDMOptionsYesNo[(int)uiDMOptions.allowArmor.curValue];

	uiDMOptions.spawnFarthest.generic.name = uiDMOptionsYesNo[(int)uiDMOptions.spawnFarthest.curValue];

	uiDMOptions.sameMap.generic.name = uiDMOptionsYesNo[(int)uiDMOptions.sameMap.curValue];

	uiDMOptions.forceRespawn.generic.name = uiDMOptionsYesNo[(int)uiDMOptions.forceRespawn.curValue];

	if (uiDMOptions.teamPlay.curValue == 0)
		uiDMOptions.teamPlay.generic.name = "Disabled";
	else if (uiDMOptions.teamPlay.curValue == 1)
		uiDMOptions.teamPlay.generic.name = "By Skin";
	else
		uiDMOptions.teamPlay.generic.name = "By Model";

	uiDMOptions.allowExit.generic.name = uiDMOptionsYesNo[(int)uiDMOptions.allowExit.curValue];

	uiDMOptions.infiniteAmmo.generic.name = uiDMOptionsYesNo[(int)uiDMOptions.infiniteAmmo.curValue];

	uiDMOptions.fixedFOV.generic.name = uiDMOptionsYesNo[(int)uiDMOptions.fixedFOV.curValue];

	uiDMOptions.quadDrop.generic.name = uiDMOptionsYesNo[(int)uiDMOptions.quadDrop.curValue];

	uiDMOptions.friendlyFire.generic.name = uiDMOptionsYesNo[(int)uiDMOptions.friendlyFire.curValue];
}

/*
 ==================
 
 ==================
*/
static void UI_DMOptions_Callback (void *self, int event){

	menuCommon_t		*item = (menuCommon_t *)self;
	menuSpinControl_t	*sc = (menuSpinControl_t *)self;
	int					flags = CVar_GetVariableInteger("dmflags");
	int					bit = 0;

	if (event == QM_ACTIVATED){
		if (item->id == ID_BACK){
			UI_PopMenu();
			return;
		}
	}

	if (item->id == ID_FRIENDLYFIRE){
		if (sc->curValue)
			flags &= ~DF_NO_FRIENDLY_FIRE;
		else
			flags |= DF_NO_FRIENDLY_FIRE;
		
		goto setvalue;
	}
	else if ( item->id == ID_FALLINGDAMAGE){
		if (sc->curValue)
			flags &= ~DF_NO_FALLING;
		else
			flags |= DF_NO_FALLING;
		
		goto setvalue;
	}
	else if (item->id == ID_WEAPONSSTAY)
		bit = DF_WEAPONS_STAY;
	else if (item->id == ID_INSTANTPOWERUPS)
		bit = DF_INSTANT_ITEMS;
	else if (item->id == ID_ALLOWEXIT)
		bit = DF_ALLOW_EXIT;
	else if (item->id == ID_ALLOWPOWERUPS){
		if (sc->curValue)
			flags &= ~DF_NO_ITEMS;
		else
			flags |= DF_NO_ITEMS;
		
		goto setvalue;
	}
	else if (item->id == ID_ALLOWHEALTH){
		if (sc->curValue)
			flags &= ~DF_NO_HEALTH;
		else
			flags |= DF_NO_HEALTH;
		
		goto setvalue;
	}
	else if (item->id == ID_SPAWNFARTHEST)
		bit = DF_SPAWN_FARTHEST;
	else if (item->id == ID_TEAMPLAY){
		if (sc->curValue == 1){
			flags |=  DF_SKINTEAMS;
			flags &= ~DF_MODELTEAMS;
		}
		else if (sc->curValue == 2){
			flags |=  DF_MODELTEAMS;
			flags &= ~DF_SKINTEAMS;
		}
		else
			flags &= ~(DF_MODELTEAMS | DF_SKINTEAMS);

		goto setvalue;
	}
	else if (item->id == ID_SAMEMAP)
		bit = DF_SAME_LEVEL;
	else if (item->id == ID_FORCERESPAWN)
		bit = DF_FORCE_RESPAWN;
	else if (item->id == ID_ALLOWARMOR){
		if (sc->curValue)
			flags &= ~DF_NO_ARMOR;
		else
			flags |= DF_NO_ARMOR;
		
		goto setvalue;
	}
	else if (item->id == ID_INFINITEAMMO)
		bit = DF_INFINITE_AMMO;
	else if (item->id == ID_FIXEDFOV)
		bit = DF_FIXED_FOV;
	else if (item->id == ID_QUADDROP)
		bit = DF_QUAD_DROP;

	if (sc->curValue == 0)
		flags &= ~bit;
	else
		flags |= bit;
	
setvalue:
	CVar_SetVariableInteger("dmflags", flags, false);

	UI_DMOptions_Update();
}

/*
 ==================
 
 ==================
*/
static void UI_DMOptions_Ownerdraw (void *self){

	menuCommon_t	*item = (menuCommon_t *)self;
	int				x, y, w, h, cw, ch, gap;

	if (item->id == ID_BACKGROUND){
		UI_DrawPicOLD(item->x, item->y, item->width, item->height, colorWhite, ART_BACKGROUND);

		x = 246;
		y = 160;
		w = 198;
		h = 30;
		UI_ScaleCoords(&x, &y, &w, &h);

		cw = UI_SMALL_CHAR_WIDTH;
		ch = UI_SMALL_CHAR_HEIGHT;
		UI_ScaleCoords(NULL, NULL, &cw, &ch);

		gap = 32;
		UI_ScaleCoords(NULL, &gap, NULL, NULL);

		UI_DrawStringOLD(x, y+(gap*0), w, h, "Falling Damage", colorWhite, true, cw, ch, 2, true);
		UI_DrawStringOLD(x, y+(gap*1), w, h, "Weapons Stay", colorWhite, true, cw, ch, 2, true);
		UI_DrawStringOLD(x, y+(gap*2), w, h, "Instant Powerups", colorWhite, true, cw, ch, 2, true);
		UI_DrawStringOLD(x, y+(gap*3), w, h, "Allow Powerups", colorWhite, true, cw, ch, 2, true);
		UI_DrawStringOLD(x, y+(gap*4), w, h, "Allow Health", colorWhite, true, cw, ch, 2, true);
		UI_DrawStringOLD(x, y+(gap*5), w, h, "Allow Armor", colorWhite, true, cw, ch, 2, true);
		UI_DrawStringOLD(x, y+(gap*6), w, h, "Spawn Farthest", colorWhite, true, cw, ch, 2, true);
		UI_DrawStringOLD(x, y+(gap*7), w, h, "Same Map", colorWhite, true, cw, ch, 2, true);
		UI_DrawStringOLD(x, y+(gap*8), w, h, "Force Respawn", colorWhite, true, cw, ch, 2, true);
		UI_DrawStringOLD(x, y+(gap*9), w, h, "Team Play", colorWhite, true, cw, ch, 2, true);
		UI_DrawStringOLD(x, y+(gap*10), w, h, "Allow Exit", colorWhite, true, cw, ch, 2, true);
		UI_DrawStringOLD(x, y+(gap*11), w, h, "Infinite Ammo", colorWhite, true, cw, ch, 2, true);
		UI_DrawStringOLD(x, y+(gap*12), w, h, "Fixed FOV", colorWhite, true, cw, ch, 2, true);
		UI_DrawStringOLD(x, y+(gap*13), w, h, "Quad Drop", colorWhite, true, cw, ch, 2, true);
		UI_DrawStringOLD(x, y+(gap*14), w, h, "Friendly Fire", colorWhite, true, cw, ch, 2, true);
	}
	else {
		if (uiDMOptions.menu.items[uiDMOptions.menu.cursor] == self)
			UI_DrawPicOLD(item->x, item->y, item->width, item->height, colorWhite, UI_MOVEBOXFOCUS);
		else
			UI_DrawPicOLD(item->x, item->y, item->width, item->height, colorWhite, UI_MOVEBOX);

		UI_DrawPicOLD(item->x, item->y, item->width, item->height, colorWhite, ((menuBitmap_t *)self)->pic);
	}
}

/*
 ==================
 
 ==================
*/
static void UI_DMOptions_Init (){

	int		flags = CVar_GetVariableInteger("dmflags");

	Mem_Fill(&uiDMOptions, 0, sizeof(uiDMOptions_t));

	uiDMOptions.background.generic.id			= ID_BACKGROUND;
	uiDMOptions.background.generic.type			= QMTYPE_BITMAP;
	uiDMOptions.background.generic.flags		= QMF_INACTIVE;
	uiDMOptions.background.generic.x			= 0;
	uiDMOptions.background.generic.y			= 0;
	uiDMOptions.background.generic.width		= 1024;
	uiDMOptions.background.generic.height		= 768;
	uiDMOptions.background.generic.ownerdraw	= UI_DMOptions_Ownerdraw;
	uiDMOptions.background.pic					= ART_BACKGROUND;

	uiDMOptions.banner.generic.id				= ID_BANNER;
	uiDMOptions.banner.generic.type				= QMTYPE_BITMAP;
	uiDMOptions.banner.generic.flags			= QMF_INACTIVE;
	uiDMOptions.banner.generic.x				= 0;
	uiDMOptions.banner.generic.y				= 66;
	uiDMOptions.banner.generic.width			= 1024;
	uiDMOptions.banner.generic.height			= 46;
	uiDMOptions.banner.pic						= ART_BANNER;

	uiDMOptions.back.generic.id					= ID_BACK;
	uiDMOptions.back.generic.type				= QMTYPE_BITMAP;
	uiDMOptions.back.generic.x					= 413;
	uiDMOptions.back.generic.y					= 656;
	uiDMOptions.back.generic.width				= 198;
	uiDMOptions.back.generic.height				= 38;
	uiDMOptions.back.generic.callback			= UI_DMOptions_Callback;
	uiDMOptions.back.generic.ownerdraw			= UI_DMOptions_Ownerdraw;
	uiDMOptions.back.pic						= UI_BACKBUTTON;

	uiDMOptions.fallingDamage.generic.id		= ID_FALLINGDAMAGE;
	uiDMOptions.fallingDamage.generic.type		= QMTYPE_SPINCONTROL;
	uiDMOptions.fallingDamage.generic.flags		= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiDMOptions.fallingDamage.generic.x			= 580;
	uiDMOptions.fallingDamage.generic.y			= 160;
	uiDMOptions.fallingDamage.generic.width		= 198;
	uiDMOptions.fallingDamage.generic.height	= 30;
	uiDMOptions.fallingDamage.generic.callback	= UI_DMOptions_Callback;
	uiDMOptions.fallingDamage.minValue			= 0;
	uiDMOptions.fallingDamage.maxValue			= 1;
	uiDMOptions.fallingDamage.range				= 1;
	uiDMOptions.fallingDamage.curValue			= (flags & DF_NO_FALLING) == 0;

	uiDMOptions.weaponsStay.generic.id			= ID_WEAPONSSTAY;
	uiDMOptions.weaponsStay.generic.type		= QMTYPE_SPINCONTROL;
	uiDMOptions.weaponsStay.generic.flags		= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiDMOptions.weaponsStay.generic.x			= 580;
	uiDMOptions.weaponsStay.generic.y			= 192;
	uiDMOptions.weaponsStay.generic.width		= 198;
	uiDMOptions.weaponsStay.generic.height		= 30;
	uiDMOptions.weaponsStay.generic.callback	= UI_DMOptions_Callback;
	uiDMOptions.weaponsStay.minValue			= 0;
	uiDMOptions.weaponsStay.maxValue			= 1;
	uiDMOptions.weaponsStay.range				= 1;
	uiDMOptions.weaponsStay.curValue			= (flags & DF_WEAPONS_STAY) != 0;

	uiDMOptions.instantPowerups.generic.id		= ID_INSTANTPOWERUPS;
	uiDMOptions.instantPowerups.generic.type	= QMTYPE_SPINCONTROL;
	uiDMOptions.instantPowerups.generic.flags	= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiDMOptions.instantPowerups.generic.x		= 580;
	uiDMOptions.instantPowerups.generic.y		= 224;
	uiDMOptions.instantPowerups.generic.width	= 198;
	uiDMOptions.instantPowerups.generic.height	= 30;
	uiDMOptions.instantPowerups.generic.callback	= UI_DMOptions_Callback;
	uiDMOptions.instantPowerups.minValue		= 0;
	uiDMOptions.instantPowerups.maxValue		= 1;
	uiDMOptions.instantPowerups.range			= 1;
	uiDMOptions.instantPowerups.curValue		= (flags & DF_INSTANT_ITEMS) != 0;

	uiDMOptions.allowPowerups.generic.id		= ID_ALLOWPOWERUPS;
	uiDMOptions.allowPowerups.generic.type		= QMTYPE_SPINCONTROL;
	uiDMOptions.allowPowerups.generic.flags		= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiDMOptions.allowPowerups.generic.x			= 580;
	uiDMOptions.allowPowerups.generic.y			= 256;
	uiDMOptions.allowPowerups.generic.width		= 198;
	uiDMOptions.allowPowerups.generic.height	= 30;
	uiDMOptions.allowPowerups.generic.callback	= UI_DMOptions_Callback;
	uiDMOptions.allowPowerups.minValue			= 0;
	uiDMOptions.allowPowerups.maxValue			= 1;
	uiDMOptions.allowPowerups.range				= 1;
	uiDMOptions.allowPowerups.curValue			= (flags & DF_NO_ITEMS) == 0;

	uiDMOptions.allowHealth.generic.id			= ID_ALLOWHEALTH;
	uiDMOptions.allowHealth.generic.type		= QMTYPE_SPINCONTROL;
	uiDMOptions.allowHealth.generic.flags		= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiDMOptions.allowHealth.generic.x			= 580;
	uiDMOptions.allowHealth.generic.y			= 288;
	uiDMOptions.allowHealth.generic.width		= 198;
	uiDMOptions.allowHealth.generic.height		= 30;
	uiDMOptions.allowHealth.generic.callback	= UI_DMOptions_Callback;
	uiDMOptions.allowHealth.minValue			= 0;
	uiDMOptions.allowHealth.maxValue			= 1;
	uiDMOptions.allowHealth.range				= 1;
	uiDMOptions.allowHealth.curValue			= (flags & DF_NO_HEALTH) == 0;

	uiDMOptions.allowArmor.generic.id			= ID_ALLOWARMOR;
	uiDMOptions.allowArmor.generic.type			= QMTYPE_SPINCONTROL;
	uiDMOptions.allowArmor.generic.flags		= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiDMOptions.allowArmor.generic.x			= 580;
	uiDMOptions.allowArmor.generic.y			= 320;
	uiDMOptions.allowArmor.generic.width		= 198;
	uiDMOptions.allowArmor.generic.height		= 30;
	uiDMOptions.allowArmor.generic.callback		= UI_DMOptions_Callback;
	uiDMOptions.allowArmor.minValue				= 0;
	uiDMOptions.allowArmor.maxValue				= 1;
	uiDMOptions.allowArmor.range				= 1;
	uiDMOptions.allowArmor.curValue				= (flags & DF_NO_ARMOR) == 0;

	uiDMOptions.spawnFarthest.generic.id		= ID_SPAWNFARTHEST;
	uiDMOptions.spawnFarthest.generic.type		= QMTYPE_SPINCONTROL;
	uiDMOptions.spawnFarthest.generic.flags		= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiDMOptions.spawnFarthest.generic.x			= 580;
	uiDMOptions.spawnFarthest.generic.y			= 352;
	uiDMOptions.spawnFarthest.generic.width		= 198;
	uiDMOptions.spawnFarthest.generic.height	= 30;
	uiDMOptions.spawnFarthest.generic.callback	= UI_DMOptions_Callback;
	uiDMOptions.spawnFarthest.minValue			= 0;
	uiDMOptions.spawnFarthest.maxValue			= 1;
	uiDMOptions.spawnFarthest.range				= 1;
	uiDMOptions.spawnFarthest.curValue			= (flags & DF_SPAWN_FARTHEST) != 0;

	uiDMOptions.sameMap.generic.id				= ID_SAMEMAP;
	uiDMOptions.sameMap.generic.type			= QMTYPE_SPINCONTROL;
	uiDMOptions.sameMap.generic.flags			= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiDMOptions.sameMap.generic.x				= 580;
	uiDMOptions.sameMap.generic.y				= 384;
	uiDMOptions.sameMap.generic.width			= 198;
	uiDMOptions.sameMap.generic.height			= 30;
	uiDMOptions.sameMap.generic.callback		= UI_DMOptions_Callback;
	uiDMOptions.sameMap.minValue				= 0;
	uiDMOptions.sameMap.maxValue				= 1;
	uiDMOptions.sameMap.range					= 1;
	uiDMOptions.sameMap.curValue				= (flags & DF_SAME_LEVEL) != 0;

	uiDMOptions.forceRespawn.generic.id			= ID_FORCERESPAWN;
	uiDMOptions.forceRespawn.generic.type		= QMTYPE_SPINCONTROL;
	uiDMOptions.forceRespawn.generic.flags		= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiDMOptions.forceRespawn.generic.x			= 580;
	uiDMOptions.forceRespawn.generic.y			= 416;
	uiDMOptions.forceRespawn.generic.width		= 198;
	uiDMOptions.forceRespawn.generic.height		= 30;
	uiDMOptions.forceRespawn.generic.callback	= UI_DMOptions_Callback;
	uiDMOptions.forceRespawn.minValue			= 0;
	uiDMOptions.forceRespawn.maxValue			= 1;
	uiDMOptions.forceRespawn.range				= 1;
	uiDMOptions.forceRespawn.curValue			= (flags & DF_FORCE_RESPAWN) != 0;

	uiDMOptions.teamPlay.generic.id				= ID_TEAMPLAY;
	uiDMOptions.teamPlay.generic.type			= QMTYPE_SPINCONTROL;
	uiDMOptions.teamPlay.generic.flags			= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiDMOptions.teamPlay.generic.x				= 580;
	uiDMOptions.teamPlay.generic.y				= 448;
	uiDMOptions.teamPlay.generic.width			= 198;
	uiDMOptions.teamPlay.generic.height			= 30;
	uiDMOptions.teamPlay.generic.callback		= UI_DMOptions_Callback;
	uiDMOptions.teamPlay.minValue				= 0;
	uiDMOptions.teamPlay.maxValue				= 2;
	uiDMOptions.teamPlay.range					= 1;

	uiDMOptions.allowExit.generic.id			= ID_ALLOWEXIT;
	uiDMOptions.allowExit.generic.type			= QMTYPE_SPINCONTROL;
	uiDMOptions.allowExit.generic.flags			= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiDMOptions.allowExit.generic.x				= 580;
	uiDMOptions.allowExit.generic.y				= 480;
	uiDMOptions.allowExit.generic.width			= 198;
	uiDMOptions.allowExit.generic.height		= 30;
	uiDMOptions.allowExit.generic.callback		= UI_DMOptions_Callback;
	uiDMOptions.allowExit.minValue				= 0;
	uiDMOptions.allowExit.maxValue				= 1;
	uiDMOptions.allowExit.range					= 1;
	uiDMOptions.allowExit.curValue				= (flags & DF_ALLOW_EXIT) != 0;

	uiDMOptions.infiniteAmmo.generic.id			= ID_INFINITEAMMO;
	uiDMOptions.infiniteAmmo.generic.type		= QMTYPE_SPINCONTROL;
	uiDMOptions.infiniteAmmo.generic.flags		= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiDMOptions.infiniteAmmo.generic.x			= 580;
	uiDMOptions.infiniteAmmo.generic.y			= 512;
	uiDMOptions.infiniteAmmo.generic.width		= 198;
	uiDMOptions.infiniteAmmo.generic.height		= 30;
	uiDMOptions.infiniteAmmo.generic.callback	= UI_DMOptions_Callback;
	uiDMOptions.infiniteAmmo.minValue			= 0;
	uiDMOptions.infiniteAmmo.maxValue			= 1;
	uiDMOptions.infiniteAmmo.range				= 1;
	uiDMOptions.infiniteAmmo.curValue			= (flags & DF_INFINITE_AMMO) != 0;

	uiDMOptions.fixedFOV.generic.id				= ID_FIXEDFOV;
	uiDMOptions.fixedFOV.generic.type			= QMTYPE_SPINCONTROL;
	uiDMOptions.fixedFOV.generic.flags			= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiDMOptions.fixedFOV.generic.x				= 580;
	uiDMOptions.fixedFOV.generic.y				= 544;
	uiDMOptions.fixedFOV.generic.width			= 198;
	uiDMOptions.fixedFOV.generic.height			= 30;
	uiDMOptions.fixedFOV.generic.callback		= UI_DMOptions_Callback;
	uiDMOptions.fixedFOV.minValue				= 0;
	uiDMOptions.fixedFOV.maxValue				= 1;
	uiDMOptions.fixedFOV.range					= 1;
	uiDMOptions.fixedFOV.curValue				= (flags & DF_FIXED_FOV) != 0;

	uiDMOptions.quadDrop.generic.id				= ID_QUADDROP;
	uiDMOptions.quadDrop.generic.type			= QMTYPE_SPINCONTROL;
	uiDMOptions.quadDrop.generic.flags			= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiDMOptions.quadDrop.generic.x				= 580;
	uiDMOptions.quadDrop.generic.y				= 576;
	uiDMOptions.quadDrop.generic.width			= 198;
	uiDMOptions.quadDrop.generic.height			= 30;
	uiDMOptions.quadDrop.generic.callback		= UI_DMOptions_Callback;
	uiDMOptions.quadDrop.minValue				= 0;
	uiDMOptions.quadDrop.maxValue				= 1;
	uiDMOptions.quadDrop.range					= 1;
	uiDMOptions.quadDrop.curValue				= (flags & DF_QUAD_DROP) != 0;

	uiDMOptions.friendlyFire.generic.id			= ID_FRIENDLYFIRE;
	uiDMOptions.friendlyFire.generic.type		= QMTYPE_SPINCONTROL;
	uiDMOptions.friendlyFire.generic.flags		= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiDMOptions.friendlyFire.generic.x			= 580;
	uiDMOptions.friendlyFire.generic.y			= 608;
	uiDMOptions.friendlyFire.generic.width		= 198;
	uiDMOptions.friendlyFire.generic.height		= 30;
	uiDMOptions.friendlyFire.generic.callback	= UI_DMOptions_Callback;
	uiDMOptions.friendlyFire.minValue			= 0;
	uiDMOptions.friendlyFire.maxValue			= 1;
	uiDMOptions.friendlyFire.range				= 1;
	uiDMOptions.friendlyFire.curValue			= (flags & DF_NO_FRIENDLY_FIRE) == 0;

	UI_AddItem(&uiDMOptions.menu, (void *)&uiDMOptions.background);
	UI_AddItem(&uiDMOptions.menu, (void *)&uiDMOptions.banner);
	UI_AddItem(&uiDMOptions.menu, (void *)&uiDMOptions.back);
	UI_AddItem(&uiDMOptions.menu, (void *)&uiDMOptions.fallingDamage);
	UI_AddItem(&uiDMOptions.menu, (void *)&uiDMOptions.weaponsStay);
	UI_AddItem(&uiDMOptions.menu, (void *)&uiDMOptions.instantPowerups);
	UI_AddItem(&uiDMOptions.menu, (void *)&uiDMOptions.allowPowerups);
	UI_AddItem(&uiDMOptions.menu, (void *)&uiDMOptions.allowHealth);
	UI_AddItem(&uiDMOptions.menu, (void *)&uiDMOptions.allowArmor);
	UI_AddItem(&uiDMOptions.menu, (void *)&uiDMOptions.spawnFarthest);
	UI_AddItem(&uiDMOptions.menu, (void *)&uiDMOptions.sameMap);
	UI_AddItem(&uiDMOptions.menu, (void *)&uiDMOptions.forceRespawn);
	UI_AddItem(&uiDMOptions.menu, (void *)&uiDMOptions.teamPlay);
	UI_AddItem(&uiDMOptions.menu, (void *)&uiDMOptions.allowExit);
	UI_AddItem(&uiDMOptions.menu, (void *)&uiDMOptions.infiniteAmmo);
	UI_AddItem(&uiDMOptions.menu, (void *)&uiDMOptions.fixedFOV);
	UI_AddItem(&uiDMOptions.menu, (void *)&uiDMOptions.quadDrop);
	UI_AddItem(&uiDMOptions.menu, (void *)&uiDMOptions.friendlyFire);

	UI_DMOptions_Update();

	UI_PushMenu(&uiDMOptions.menu);
}

/*
 ==================
 
 ==================
*/
static void UI_StartServer_GetMapList (){

	FILE	*f;
	int		size;
	char	*buffer, *buf, *tok;

	f = fopen(Str_VarArgs("%s/%s/maps.lst", CVar_GetVariableString("fs_savePath"), CVar_GetVariableString("fs_game")), "rb");
	if (!f){
		if (FS_ReadFile("maps.lst", (void **)&buffer) == -1)
			Com_Error(ERR_FATAL, "Couldn't find maps.lst");
	}
	else {
		fseek(f, 0, SEEK_END);
		size = ftell(f);
		fseek(f, 0, SEEK_SET);

		buffer = (char *)Mem_Alloc(size+1, TAG_TEMPORARY);
		fread(buffer, 1, size, f);
		fclose(f);
	}

	buf = buffer;
	while (1){
		tok = Com_ParseExt(&buf, true);
		if (!tok[0])
			break;		// End of data

		Str_Copy(uiStartServer.maps[uiStartServer.numMaps], tok, sizeof(uiStartServer.maps[uiStartServer.numMaps]));

		tok = Com_ParseExt(&buf, false);
		if (!tok[0])
			continue;	// Invalid

		Str_Copy(uiStartServer.names[uiStartServer.numMaps], tok, sizeof(uiStartServer.names[uiStartServer.numMaps]));
		
		uiStartServer.numMaps++;
	}

	if (!f)
		FS_FreeFile(buffer);
	else {
		Mem_Free(buffer);
	}

	if (!uiStartServer.numMaps){
		uiStartServer.mapList.generic.flags |= QMF_GRAYED;
		uiStartServer.begin.generic.flags |= QMF_GRAYED;
	}
	else
		uiStartServer.mapList.maxValue = uiStartServer.numMaps - 1;
}

/*
 ==================
 
 ==================
*/
static void UI_StartServer_Update (){

	uiStartServer.mapList.generic.name = uiStartServer.names[(int)uiStartServer.mapList.curValue];

	if (uiStartServer.rules.curValue)
		uiStartServer.rules.generic.name = "Coop";
	else
		uiStartServer.rules.generic.name = "Deathmatch";
}

/*
 ==================
 
 ==================
*/
static void UI_StartServer_Start (){

	char	map[80];
	char	*spot;

	if (Com_ServerState())
		Cmd_ExecuteText(CMD_EXEC_APPEND, "disconnect\n");

	CVar_SetVariableInteger("deathmatch", !((int)uiStartServer.rules.curValue), false);
	CVar_SetVariableInteger("coop", ((int)uiStartServer.rules.curValue), false);
	CVar_SetVariableInteger("timelimit", Str_ToInteger(uiStartServer.timeLimit.buffer), false);
	CVar_SetVariableInteger("fraglimit", Str_ToInteger(uiStartServer.fragLimit.buffer), false);
	CVar_SetVariableInteger("maxclients", Str_ToInteger(uiStartServer.maxClients.buffer), false);
	CVar_SetVariableString("sv_hostName", uiStartServer.hostName.buffer, false);

	Str_Copy(map, uiStartServer.maps[(int)uiStartServer.mapList.curValue], sizeof(map));

	if (uiStartServer.rules.curValue){
		if (!Q_stricmp(map, "bunk1"))
			spot = "start";
		else if (!Q_stricmp(map, "mintro"))
			spot = "start";
		else if (!Q_stricmp(map, "fact1"))
			spot = "start";
		else if (!Q_stricmp(map, "power1"))
			spot = "pstart";
		else if (!Q_stricmp(map, "biggun"))
			spot = "bstart";
		else if (!Q_stricmp(map, "hangar1"))
			spot = "unitstart";
		else if (!Q_stricmp(map, "city1"))
			spot = "unitstart";
		else if (!Q_stricmp(map, "boss1"))
			spot = "bosstart";
		else
			spot = NULL;

		if (spot)
			Cmd_ExecuteText(CMD_EXEC_APPEND, Str_VarArgs("gamemap *%s$%s\n", map, spot));
		else
			Cmd_ExecuteText(CMD_EXEC_APPEND, Str_VarArgs("gamemap *%s\n", map));
	}
	else
		Cmd_ExecuteText(CMD_EXEC_APPEND, Str_VarArgs("map %s\n", map));
}

/*
 ==================
 
 ==================
*/
static void UI_StartServer_Callback (void *self, int event){

	menuCommon_t	*item = (menuCommon_t *)self;

	if (event == QM_CHANGED){
		switch (item->id){
		case ID_MAPLIST:
			UI_StartServer_Update();
			break;
		case ID_RULES:
			if (uiStartServer.rules.curValue){
				if (Str_ToInteger(uiStartServer.maxClients.buffer) > 8)
					Str_SPrintf(uiStartServer.maxClients.buffer, sizeof(uiStartServer.maxClients.buffer), "4");
			}
			else {
				if (Str_ToInteger(uiStartServer.maxClients.buffer) > 256)
					Str_SPrintf(uiStartServer.maxClients.buffer, sizeof(uiStartServer.maxClients.buffer), "256");
			}
			UI_StartServer_Update();
			break;
		case ID_HOSTNAME:
			CVar_SetVariableString("sv_hostName", uiStartServer.hostName.buffer, false);
			break;
		}

		return;
	}

	if (event != QM_ACTIVATED)
		return;

	switch (item->id){
	case ID_BACK:
		UI_PopMenu();
		break;
	case ID_DMOPTIONS:
		UI_DMOptions_Init();
		break;
	case ID_BEGIN:
		UI_StartServer_Start();
		break;
	}
}

/*
 ==================
 
 ==================
*/
static void UI_StartServer_Ownerdraw (void *self){

	menuCommon_t	*item = (menuCommon_t *)self;

	if (item->id == ID_BACKGROUND){
		int		x, y, w, h, cw, ch, gap;

		UI_DrawPicOLD(item->x, item->y, item->width, item->height, colorWhite, ART_BACKGROUND);

		x = 246;
		y = 256;
		w = 198;
		h = 30;
		UI_ScaleCoords(&x, &y, &w, &h);

		cw = UI_SMALL_CHAR_WIDTH;
		ch = UI_SMALL_CHAR_HEIGHT;
		UI_ScaleCoords(NULL, NULL, &cw, &ch);

		gap = 32;
		UI_ScaleCoords(NULL, &gap, NULL, NULL);

		UI_DrawStringOLD(x, y+(gap*0), w, h, "Initial Map", colorWhite, true, cw, ch, 2, true);
		UI_DrawStringOLD(x, y+(gap*2), w, h, "Rules", colorWhite, true, cw, ch, 2, true);
		UI_DrawStringOLD(x, y+(gap*3), w, h, "Time Limit", colorWhite, true, cw, ch, 2, true);
		UI_DrawStringOLD(x, y+(gap*4), w, h, "Frag Limit", colorWhite, true, cw, ch, 2, true);
		UI_DrawStringOLD(x, y+(gap*5), w, h, "Max Clients", colorWhite, true, cw, ch, 2, true);
		UI_DrawStringOLD(x, y+(gap*6), w, h, "Host Name", colorWhite, true, cw, ch, 2, true);

		if (uiStartServer.numMaps){
			x = 580;
			y = 288;
			w = 198;
			h = 30;
			UI_ScaleCoords(&x, &y, &w, &h);

			UI_DrawStringOLD(x, y, w, h, uiStartServer.maps[(int)uiStartServer.mapList.curValue], colorWhite, true, cw, ch, 1, true);
		}
	}
	else {
		if (uiStartServer.menu.items[uiStartServer.menu.cursor] == self)
			UI_DrawPicOLD(item->x, item->y, item->width, item->height, colorWhite, UI_MOVEBOXFOCUS);
		else
			UI_DrawPicOLD(item->x, item->y, item->width, item->height, colorWhite, UI_MOVEBOX);

		UI_DrawPicOLD(item->x, item->y, item->width, item->height, colorWhite, ((menuBitmap_t *)self)->pic);
	}
}

/*
 ==================
 
 ==================
*/
static void UI_StartServer_Init (){

	Mem_Fill(&uiStartServer, 0, sizeof(uiStartServer_t));

	uiStartServer.background.generic.id			= ID_BACKGROUND;
	uiStartServer.background.generic.type		= QMTYPE_BITMAP;
	uiStartServer.background.generic.flags		= QMF_INACTIVE;
	uiStartServer.background.generic.x			= 0;
	uiStartServer.background.generic.y			= 0;
	uiStartServer.background.generic.width		= 1024;
	uiStartServer.background.generic.height		= 768;
	uiStartServer.background.generic.ownerdraw	= UI_StartServer_Ownerdraw;

	uiStartServer.banner.generic.id				= ID_BANNER;
	uiStartServer.banner.generic.type			= QMTYPE_BITMAP;
	uiStartServer.banner.generic.flags			= QMF_INACTIVE;
	uiStartServer.banner.generic.x				= 0;
	uiStartServer.banner.generic.y				= 66;
	uiStartServer.banner.generic.width			= 1024;
	uiStartServer.banner.generic.height			= 46;
	uiStartServer.banner.pic					= ART_BANNER;

	uiStartServer.back.generic.id				= ID_BACK;
	uiStartServer.back.generic.type				= QMTYPE_BITMAP;
	uiStartServer.back.generic.x				= 413;
	uiStartServer.back.generic.y				= 656;
	uiStartServer.back.generic.width			= 198;
	uiStartServer.back.generic.height			= 38;
	uiStartServer.back.generic.callback			= UI_StartServer_Callback;
	uiStartServer.back.generic.ownerdraw		= UI_StartServer_Ownerdraw;
	uiStartServer.back.pic						= UI_BACKBUTTON;

	uiStartServer.mapList.generic.id			= ID_MAPLIST;
	uiStartServer.mapList.generic.type			= QMTYPE_SPINCONTROL;
	uiStartServer.mapList.generic.flags			= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiStartServer.mapList.generic.x				= 580;
	uiStartServer.mapList.generic.y				= 256;
	uiStartServer.mapList.generic.width			= 198;
	uiStartServer.mapList.generic.height		= 30;
	uiStartServer.mapList.generic.callback		= UI_StartServer_Callback;
	uiStartServer.mapList.minValue				= 0;
	uiStartServer.mapList.range					= 1;
	
	uiStartServer.rules.generic.id				= ID_RULES;
	uiStartServer.rules.generic.type			= QMTYPE_SPINCONTROL;
	uiStartServer.rules.generic.flags			= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiStartServer.rules.generic.x				= 580;
	uiStartServer.rules.generic.y				= 320;
	uiStartServer.rules.generic.width			= 198;
	uiStartServer.rules.generic.height			= 30;
	uiStartServer.rules.generic.callback		= UI_StartServer_Callback;
	uiStartServer.rules.minValue				= 0;
	uiStartServer.rules.maxValue				= 1;
	uiStartServer.rules.range					= 1;
	if (CVar_GetVariableInteger("coop"))
		uiStartServer.rules.curValue				= 1;

	uiStartServer.timeLimit.generic.id			= ID_TIMELIMIT;
	uiStartServer.timeLimit.generic.type		= QMTYPE_FIELD;
	uiStartServer.timeLimit.generic.flags		= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW | QMF_NUMBERSONLY;
	uiStartServer.timeLimit.generic.x			= 580;
	uiStartServer.timeLimit.generic.y			= 352;
	uiStartServer.timeLimit.generic.width		= 198;
	uiStartServer.timeLimit.generic.height		= 30;
	uiStartServer.timeLimit.generic.statusText	= "0 = No limit";
	uiStartServer.timeLimit.maxLenght			= 3;
	Str_SPrintf(uiStartServer.timeLimit.buffer, sizeof(uiStartServer.timeLimit.buffer), "%i", CVar_GetVariableInteger("timelimit"));

	uiStartServer.fragLimit.generic.id			= ID_FRAGLIMIT;
	uiStartServer.fragLimit.generic.type		= QMTYPE_FIELD;
	uiStartServer.fragLimit.generic.flags		= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW | QMF_NUMBERSONLY;
	uiStartServer.fragLimit.generic.x			= 580;
	uiStartServer.fragLimit.generic.y			= 384;
	uiStartServer.fragLimit.generic.width		= 198;
	uiStartServer.fragLimit.generic.height		= 30;
	uiStartServer.fragLimit.generic.statusText	= "0 = No limit";
	uiStartServer.fragLimit.maxLenght			= 3;
	Str_SPrintf(uiStartServer.fragLimit.buffer, sizeof(uiStartServer.fragLimit.buffer), "%i", CVar_GetVariableInteger("fraglimit"));

	uiStartServer.maxClients.generic.id			= ID_MAXCLIENTS;
	uiStartServer.maxClients.generic.type		= QMTYPE_FIELD;
	uiStartServer.maxClients.generic.flags		= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW | QMF_NUMBERSONLY;
	uiStartServer.maxClients.generic.x			= 580;
	uiStartServer.maxClients.generic.y			= 416;
	uiStartServer.maxClients.generic.width		= 198;
	uiStartServer.maxClients.generic.height		= 30;
	uiStartServer.maxClients.maxLenght			= 3;
	if (CVar_GetVariableInteger("maxclients") <= 1)
		Str_SPrintf(uiStartServer.maxClients.buffer, sizeof(uiStartServer.maxClients.buffer), "8");
	else
		Str_SPrintf(uiStartServer.maxClients.buffer, sizeof(uiStartServer.maxClients.buffer), "%i", CVar_GetVariableInteger("maxclients"));

	uiStartServer.hostName.generic.id			= ID_HOSTNAME;
	uiStartServer.hostName.generic.type			= QMTYPE_FIELD;
	uiStartServer.hostName.generic.flags		= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiStartServer.hostName.generic.x			= 580;
	uiStartServer.hostName.generic.y			= 448;
	uiStartServer.hostName.generic.width		= 198;
	uiStartServer.hostName.generic.height		= 30;
	uiStartServer.hostName.generic.callback		= UI_StartServer_Callback;
	uiStartServer.hostName.maxLenght			= 16;
	Str_Copy(uiStartServer.hostName.buffer, CVar_GetVariableString("sv_hostName"), sizeof(uiStartServer.hostName.buffer));

	uiStartServer.dmOptions.generic.id			= ID_DMOPTIONS;
	uiStartServer.dmOptions.generic.name		= "DM Options";
	uiStartServer.dmOptions.generic.type		= QMTYPE_ACTION;
	uiStartServer.dmOptions.generic.flags		= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiStartServer.dmOptions.generic.x			= 580;
	uiStartServer.dmOptions.generic.y			= 480;
	uiStartServer.dmOptions.generic.width		= 198;
	uiStartServer.dmOptions.generic.height		= 30;
	uiStartServer.dmOptions.generic.callback	= UI_StartServer_Callback;
	uiStartServer.dmOptions.background			= "";

	uiStartServer.begin.generic.id				= ID_BEGIN;
	uiStartServer.begin.generic.name			= "Begin";
	uiStartServer.begin.generic.type			= QMTYPE_ACTION;
	uiStartServer.begin.generic.flags			= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiStartServer.begin.generic.x				= 580;
	uiStartServer.begin.generic.y				= 512;
	uiStartServer.begin.generic.width			= 198;
	uiStartServer.begin.generic.height			= 30;
	uiStartServer.begin.generic.callback		= UI_StartServer_Callback;
	uiStartServer.begin.background				= "";

	UI_StartServer_GetMapList();

	UI_AddItem(&uiStartServer.menu, (void *)&uiStartServer.background);
	UI_AddItem(&uiStartServer.menu, (void *)&uiStartServer.banner);
	UI_AddItem(&uiStartServer.menu, (void *)&uiStartServer.back);
	UI_AddItem(&uiStartServer.menu, (void *)&uiStartServer.mapList);
	UI_AddItem(&uiStartServer.menu, (void *)&uiStartServer.rules);
	UI_AddItem(&uiStartServer.menu, (void *)&uiStartServer.timeLimit);
	UI_AddItem(&uiStartServer.menu, (void *)&uiStartServer.fragLimit);
	UI_AddItem(&uiStartServer.menu, (void *)&uiStartServer.maxClients);
	UI_AddItem(&uiStartServer.menu, (void *)&uiStartServer.hostName);
	UI_AddItem(&uiStartServer.menu, (void *)&uiStartServer.dmOptions);
	UI_AddItem(&uiStartServer.menu, (void *)&uiStartServer.begin);

	UI_StartServer_Update();

	UI_PushMenu(&uiStartServer.menu);
}

/*
 ==================
 
 ==================
*/
static void UI_MultiPlayer_Callback (void *self, int event){

	menuCommon_t	*item = (menuCommon_t *)self;

	if (event != QM_ACTIVATED)
		return;

	switch (item->id){
	case ID_BACK:
		UI_PopMenu();
		break;
	case ID_JOINSERVER:
		UI_JoinServer_Init();
		break;
	case ID_STARTSERVER:
		UI_StartServer_Init();
		break;
	}
}

/*
 ==================
 
 ==================
*/
static void UI_MultiPlayer_Ownerdraw (void *self){

	menuCommon_t	*item = (menuCommon_t *)self;

	if (item->id == ID_BACKGROUND){
		int		x = 0, y = 128, w = 1024, h = 50, cw = 20, ch = 40;

		UI_DrawPicOLD(item->x, item->y, item->width, item->height, colorWhite, ((menuBitmap_t *)self)->pic);

		UI_ScaleCoords(&x, &y, &w, &h);
		UI_ScaleCoords(NULL, NULL, &cw, &ch);
		UI_DrawStringOLD(x, y, w, h, "UNDER CONSTRUCTION!!!", colorWhite, true, cw, ch, 1, true);
	}
	else {
		if (uiMultiPlayer.menu.items[uiMultiPlayer.menu.cursor] == self)
			UI_DrawPicOLD(item->x, item->y, item->width, item->height, colorWhite, UI_MOVEBOXFOCUS);
		else
			UI_DrawPicOLD(item->x, item->y, item->width, item->height, colorWhite, UI_MOVEBOX);

		UI_DrawPicOLD(item->x, item->y, item->width, item->height, colorWhite, ((menuBitmap_t *)self)->pic);
	}
}

/*
 ==================
 
 ==================
*/
static void UI_MultiPlayer_Init (){

	Mem_Fill(&uiMultiPlayer, 0, sizeof(uiMultiPlayer_t));

	uiMultiPlayer.background.generic.id			= ID_BACKGROUND;
	uiMultiPlayer.background.generic.type		= QMTYPE_BITMAP;
	uiMultiPlayer.background.generic.flags		= QMF_INACTIVE;
	uiMultiPlayer.background.generic.x			= 0;
	uiMultiPlayer.background.generic.y			= 0;
	uiMultiPlayer.background.generic.width		= 1024;
	uiMultiPlayer.background.generic.height		= 768;
	uiMultiPlayer.background.generic.ownerdraw	= UI_MultiPlayer_Ownerdraw;
	uiMultiPlayer.background.pic				= ART_BACKGROUND;

	uiMultiPlayer.banner.generic.id				= ID_BANNER;
	uiMultiPlayer.banner.generic.type			= QMTYPE_BITMAP;
	uiMultiPlayer.banner.generic.flags			= QMF_INACTIVE;
	uiMultiPlayer.banner.generic.x				= 0;
	uiMultiPlayer.banner.generic.y				= 66;
	uiMultiPlayer.banner.generic.width			= 1024;
	uiMultiPlayer.banner.generic.height			= 46;
	uiMultiPlayer.banner.pic					= ART_BANNER;

	uiMultiPlayer.back.generic.id				= ID_BACK;
	uiMultiPlayer.back.generic.type				= QMTYPE_BITMAP;
	uiMultiPlayer.back.generic.x				= 413;
	uiMultiPlayer.back.generic.y				= 656;
	uiMultiPlayer.back.generic.width			= 198;
	uiMultiPlayer.back.generic.height			= 38;
	uiMultiPlayer.back.generic.callback			= UI_MultiPlayer_Callback;
	uiMultiPlayer.back.generic.ownerdraw		= UI_MultiPlayer_Ownerdraw;
	uiMultiPlayer.back.pic						= UI_BACKBUTTON;

	uiMultiPlayer.joinServer.generic.id			= ID_JOINSERVER;
	uiMultiPlayer.joinServer.generic.name		= "Join Server";
	uiMultiPlayer.joinServer.generic.type		= QMTYPE_ACTION;
	uiMultiPlayer.joinServer.generic.flags		= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiMultiPlayer.joinServer.generic.x			= 413;
	uiMultiPlayer.joinServer.generic.y			= 354;
	uiMultiPlayer.joinServer.generic.width		= 198;
	uiMultiPlayer.joinServer.generic.height		= 30;
	uiMultiPlayer.joinServer.generic.callback	= UI_MultiPlayer_Callback;
	uiMultiPlayer.joinServer.background			= "";

	uiMultiPlayer.startServer.generic.id		= ID_STARTSERVER;
	uiMultiPlayer.startServer.generic.name		= "Start Server";
	uiMultiPlayer.startServer.generic.type		= QMTYPE_ACTION;
	uiMultiPlayer.startServer.generic.flags		= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiMultiPlayer.startServer.generic.x			= 413;
	uiMultiPlayer.startServer.generic.y			= 384;
	uiMultiPlayer.startServer.generic.width		= 198;
	uiMultiPlayer.startServer.generic.height	= 30;
	uiMultiPlayer.startServer.generic.callback	= UI_MultiPlayer_Callback;
	uiMultiPlayer.startServer.background		= "";

	UI_AddItem(&uiMultiPlayer.menu, (void *)&uiMultiPlayer.background);
	UI_AddItem(&uiMultiPlayer.menu, (void *)&uiMultiPlayer.banner);
	UI_AddItem(&uiMultiPlayer.menu, (void *)&uiMultiPlayer.back);
	UI_AddItem(&uiMultiPlayer.menu, (void *)&uiMultiPlayer.joinServer);
	UI_AddItem(&uiMultiPlayer.menu, (void *)&uiMultiPlayer.startServer);
}

/*
 ==================
 UI_MultiPlayer_Precache
 ==================
*/
void UI_MultiPlayer_Precache (){

	R_RegisterMaterialNoMip(ART_BACKGROUND);
	R_RegisterMaterialNoMip(ART_BANNER);
}

/*
 ==================
 UI_MultiPlayer_Menu
 ==================
*/
void UI_MultiPlayer_Menu (){

	UI_MultiPlayer_Precache();
	UI_MultiPlayer_Init();

	UI_PushMenu(&uiMultiPlayer.menu);
}