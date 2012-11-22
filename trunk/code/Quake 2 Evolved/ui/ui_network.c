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
// ui_network.c - Network menu
//

// TODO!!!


#include "ui_local.h"


#define ART_BACKGROUND				"ui/assets/ui/ui_sub_options"
#define ART_BANNER					"ui/assets/banners/network_t"
#define ART_TEXT1					"ui/assets/text/network_text_p1"
#define ART_TEXT2					"ui/assets/text/network_text_p2"

typedef enum {
	ID_BACKGROUND,
	ID_BANNER,

	ID_TEXT1,
	ID_TEXT2,
	ID_TEXTSHADOW1,
	ID_TEXTSHADOW2,

	ID_CANCEL,
	ID_APPLY,

	ID_RATE,
	ID_SOCKS,
	ID_SOCKSSERVER,
	ID_SOCKSPORT,
	ID_SOCKSUSERNAME,
	ID_SOCKSPASSWORD
} uiAdvancedId_t;

static const char *			uiNetworkRate[] = {
	"28.8K Modem",
	"33.6K Modem",
	"56K Modem",
	"ISDN",
	"LAN/Cable/xDSL"
};
static const char *			uiNetworkYesNo[] = {
	"False",
	"True"
};

typedef struct {
	float					rate;
	float					socks;
} uiNetworkValues_t;

static uiNetworkValues_t	uiNetworkInitial;

typedef struct {
	menuFramework_t			menu;

	menuBitmap_t			background;
	menuBitmap_t			banner;

	menuBitmap_t			textShadow1;
	menuBitmap_t			textShadow2;
	menuBitmap_t			text1;
	menuBitmap_t			text2;

	menuBitmap_t			cancel;
	menuBitmap_t			apply;

	menuSpinControl_t		rate;
	menuSpinControl_t		socks;
	menuField_t				socksServer;
	menuField_t				socksPort;
	menuField_t				socksUsername;
	menuField_t				socksPassword;
} uiNetwork_t;

static uiNetwork_t			uiNetwork;


/*
 ==================
 
 ==================
*/
static void UI_Network_GetConfig (){

	int	rate;

	rate = CVar_GetVariableInteger("rate");
	if (rate <= 2500)
		uiNetwork.rate.curValue = 0;
	else if (rate <= 3000)
		uiNetwork.rate.curValue = 1;
	else if (rate <= 4000)
		uiNetwork.rate.curValue = 2;
	else if (rate <= 5000)
		uiNetwork.rate.curValue = 3;
	else
		uiNetwork.rate.curValue = 4;

	// Save initial values
	uiNetworkInitial.rate = uiNetwork.rate.curValue;
	uiNetworkInitial.socks = uiNetwork.socks.curValue;
}

/*
 ==================
 
 ==================
*/
static void UI_Network_SetConfig (){

	if (uiNetworkInitial.rate != uiNetwork.rate.curValue){
		uiNetworkInitial.rate = uiNetwork.rate.curValue;

		switch ((int)uiNetwork.rate.curValue){
		case 0:
			CVar_SetVariableInteger("rate", 2500, false);
			break;
		case 1:
			CVar_SetVariableInteger("rate", 3000, false);
			break;
		case 2:
			CVar_SetVariableInteger("rate", 4000, false);
			break;
		case 3:
			CVar_SetVariableInteger("rate", 5000, false);
			break;
		case 4:
			CVar_SetVariableInteger("rate", 25000, false);
			break;
		}
	}

	// Restart network subsystem
	Cmd_ExecuteText(CMD_EXEC_NOW, "restartNetwork\n");
}

/*
 ==================
 
 ==================
*/
static void UI_Network_UpdateConfig (){

	uiNetwork.rate.generic.name = uiNetworkRate[(int)uiNetwork.rate.curValue];

	// TODO!!!
	uiNetwork.socks.generic.name = uiNetworkYesNo[(int)uiNetwork.socks.curValue];
	uiNetwork.socks.generic.flags |= QMF_GRAYED;

	uiNetwork.socksServer.generic.flags |= QMF_GRAYED;
	uiNetwork.socksPort.generic.flags |= QMF_GRAYED;
	uiNetwork.socksUsername.generic.flags |= QMF_GRAYED;
	uiNetwork.socksPassword.generic.flags |= QMF_GRAYED;

	// Some settings can be updated here
	if (uiNetworkInitial.rate != uiNetwork.rate.curValue){
		uiNetworkInitial.rate = uiNetwork.rate.curValue;

		switch ((int)uiNetwork.rate.curValue){
		case 0:
			CVar_SetVariableInteger("rate", 2500, false);
			break;
		case 1:
			CVar_SetVariableInteger("rate", 3000, false);
			break;
		case 2:
			CVar_SetVariableInteger("rate", 4000, false);
			break;
		case 3:
			CVar_SetVariableInteger("rate", 5000, false);
			break;
		case 4:
			CVar_SetVariableInteger("rate", 25000, false);
			break;
		}
	}

	// See if the apply button should be enabled or disabled
	uiNetwork.apply.generic.flags |= QMF_GRAYED;
}

/*
 ==================
 UI_Network_Callback
 ==================
*/
static void UI_Network_Callback (void *self, int event){

	menuCommon_t	*item = (menuCommon_t *)self;

	if (event == QM_CHANGED){
		UI_Network_UpdateConfig();
		return;
	}

	if (event != QM_ACTIVATED)
		return;

	switch (item->id){
	case ID_CANCEL:
		UI_PopMenu();
		break;
	case ID_APPLY:
		UI_Network_SetConfig();
		UI_Network_GetConfig();
		UI_Network_UpdateConfig();
		break;
	}
}

/*
 ==================
 UI_Network_Ownerdraw
 ==================
*/
static void UI_Network_Ownerdraw (void *self){

	menuCommon_t	*item = (menuCommon_t *)self;

	if (uiNetwork.menu.items[uiNetwork.menu.cursor] == self)
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
static void UI_Network_Init (){

	Mem_Fill(&uiNetwork, 0, sizeof(uiNetwork_t));

	uiNetwork.background.generic.id				= ID_BACKGROUND;
	uiNetwork.background.generic.type			= QMTYPE_BITMAP;
	uiNetwork.background.generic.flags			= QMF_INACTIVE;
	uiNetwork.background.generic.x				= 0;
	uiNetwork.background.generic.y				= 0;
	uiNetwork.background.generic.width			= 1024;
	uiNetwork.background.generic.height			= 768;
	uiNetwork.background.pic					= ART_BACKGROUND;

	uiNetwork.banner.generic.id					= ID_BANNER;
	uiNetwork.banner.generic.type				= QMTYPE_BITMAP;
	uiNetwork.banner.generic.flags				= QMF_INACTIVE;
	uiNetwork.banner.generic.x					= 0;
	uiNetwork.banner.generic.y					= 66;
	uiNetwork.banner.generic.width				= 1024;
	uiNetwork.banner.generic.height				= 46;
	uiNetwork.banner.pic						= ART_BANNER;

	uiNetwork.textShadow1.generic.id			= ID_TEXTSHADOW1;
	uiNetwork.textShadow1.generic.type			= QMTYPE_BITMAP;
	uiNetwork.textShadow1.generic.flags			= QMF_INACTIVE;
	uiNetwork.textShadow1.generic.x				= 182;
	uiNetwork.textShadow1.generic.y				= 170;
	uiNetwork.textShadow1.generic.width			= 256;
	uiNetwork.textShadow1.generic.height		= 256;
	uiNetwork.textShadow1.generic.color			= colorBlack;
	uiNetwork.textShadow1.pic					= ART_TEXT1;

	uiNetwork.textShadow2.generic.id			= ID_TEXTSHADOW2;
	uiNetwork.textShadow2.generic.type			= QMTYPE_BITMAP;
	uiNetwork.textShadow2.generic.flags			= QMF_INACTIVE;
	uiNetwork.textShadow2.generic.x				= 182;
	uiNetwork.textShadow2.generic.y				= 426;
	uiNetwork.textShadow2.generic.width			= 256;
	uiNetwork.textShadow2.generic.height		= 256;
	uiNetwork.textShadow2.generic.color			= colorBlack;
	uiNetwork.textShadow2.pic					= ART_TEXT2;

	uiNetwork.text1.generic.id					= ID_TEXT1;
	uiNetwork.text1.generic.type				= QMTYPE_BITMAP;
	uiNetwork.text1.generic.flags				= QMF_INACTIVE;
	uiNetwork.text1.generic.x					= 180;
	uiNetwork.text1.generic.y					= 168;
	uiNetwork.text1.generic.width				= 256;
	uiNetwork.text1.generic.height				= 256;
	uiNetwork.text1.pic							= ART_TEXT1;

	uiNetwork.text2.generic.id					= ID_TEXT2;
	uiNetwork.text2.generic.type				= QMTYPE_BITMAP;
	uiNetwork.text2.generic.flags				= QMF_INACTIVE;
	uiNetwork.text2.generic.x					= 180;
	uiNetwork.text2.generic.y					= 424;
	uiNetwork.text2.generic.width				= 256;
	uiNetwork.text2.generic.height				= 256;
	uiNetwork.text2.pic							= ART_TEXT2;

	uiNetwork.cancel.generic.id					= ID_CANCEL;
	uiNetwork.cancel.generic.type				= QMTYPE_BITMAP;
	uiNetwork.cancel.generic.x					= 310;
	uiNetwork.cancel.generic.y					= 656;
	uiNetwork.cancel.generic.width				= 198;
	uiNetwork.cancel.generic.height				= 38;
	uiNetwork.cancel.generic.callback			= UI_Network_Callback;
	uiNetwork.cancel.generic.ownerdraw			= UI_Network_Ownerdraw;
	uiNetwork.cancel.pic						= UI_CANCELBUTTON;

	uiNetwork.apply.generic.id					= ID_APPLY;
	uiNetwork.apply.generic.type				= QMTYPE_BITMAP;
	uiNetwork.apply.generic.x					= 516;
	uiNetwork.apply.generic.y					= 656;
	uiNetwork.apply.generic.width				= 198;
	uiNetwork.apply.generic.height				= 38;
	uiNetwork.apply.generic.callback			= UI_Network_Callback;
	uiNetwork.apply.generic.ownerdraw			= UI_Network_Ownerdraw;
	uiNetwork.apply.pic							= UI_APPLYBUTTON;

	uiNetwork.rate.generic.id					= ID_RATE;
	uiNetwork.rate.generic.type					= QMTYPE_SPINCONTROL;
	uiNetwork.rate.generic.flags				= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiNetwork.rate.generic.x					= 580;
	uiNetwork.rate.generic.y					= 160;
	uiNetwork.rate.generic.width				= 198;
	uiNetwork.rate.generic.height				= 30;
	uiNetwork.rate.generic.callback				= UI_Network_Callback;
	uiNetwork.rate.generic.statusText			= "Set your internet connection's speed";
	uiNetwork.rate.minValue						= 0;
	uiNetwork.rate.maxValue						= 4;
	uiNetwork.rate.range						= 1;

	uiNetwork.socks.generic.id					= ID_SOCKS;
	uiNetwork.socks.generic.type				= QMTYPE_SPINCONTROL;
	uiNetwork.socks.generic.flags				= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiNetwork.socks.generic.x					= 580;
	uiNetwork.socks.generic.y					= 224;
	uiNetwork.socks.generic.width				= 198;
	uiNetwork.socks.generic.height				= 30;
	uiNetwork.socks.generic.callback			= UI_Network_Callback;
	uiNetwork.socks.minValue					= 0;
	uiNetwork.socks.maxValue					= 1;
	uiNetwork.socks.range						= 1;

	uiNetwork.socksServer.generic.id			= ID_SOCKSSERVER;
	uiNetwork.socksServer.generic.type			= QMTYPE_FIELD;
	uiNetwork.socksServer.generic.flags			= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiNetwork.socksServer.generic.x				= 580;
	uiNetwork.socksServer.generic.y				= 256;
	uiNetwork.socksServer.generic.width			= 198;
	uiNetwork.socksServer.generic.height		= 30;

	uiNetwork.socksPort.generic.id				= ID_SOCKSPORT;
	uiNetwork.socksPort.generic.type			= QMTYPE_FIELD;
	uiNetwork.socksPort.generic.flags			= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiNetwork.socksPort.generic.x				= 580;
	uiNetwork.socksPort.generic.y				= 288;
	uiNetwork.socksPort.generic.width			= 198;
	uiNetwork.socksPort.generic.height			= 30;

	uiNetwork.socksUsername.generic.id			= ID_SOCKSUSERNAME;
	uiNetwork.socksUsername.generic.type		= QMTYPE_FIELD;
	uiNetwork.socksUsername.generic.flags		= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiNetwork.socksUsername.generic.x			= 580;
	uiNetwork.socksUsername.generic.y			= 320;
	uiNetwork.socksUsername.generic.width		= 198;
	uiNetwork.socksUsername.generic.height		= 30;

	uiNetwork.socksPassword.generic.id			= ID_SOCKSPASSWORD;
	uiNetwork.socksPassword.generic.type		= QMTYPE_FIELD;
	uiNetwork.socksPassword.generic.flags		= QMF_CENTER_JUSTIFY | QMF_PULSEIFFOCUS | QMF_DROPSHADOW;
	uiNetwork.socksPassword.generic.x			= 580;
	uiNetwork.socksPassword.generic.y			= 352;
	uiNetwork.socksPassword.generic.width		= 198;
	uiNetwork.socksPassword.generic.height		= 30;

	UI_Network_GetConfig();

	UI_AddItem(&uiNetwork.menu, (void *)&uiNetwork.background);
	UI_AddItem(&uiNetwork.menu, (void *)&uiNetwork.banner);
	UI_AddItem(&uiNetwork.menu, (void *)&uiNetwork.textShadow1);
	UI_AddItem(&uiNetwork.menu, (void *)&uiNetwork.textShadow2);
	UI_AddItem(&uiNetwork.menu, (void *)&uiNetwork.text1);
	UI_AddItem(&uiNetwork.menu, (void *)&uiNetwork.text2);
	UI_AddItem(&uiNetwork.menu, (void *)&uiNetwork.cancel);
	UI_AddItem(&uiNetwork.menu, (void *)&uiNetwork.apply);
	UI_AddItem(&uiNetwork.menu, (void *)&uiNetwork.rate);
	UI_AddItem(&uiNetwork.menu, (void *)&uiNetwork.socks);
	UI_AddItem(&uiNetwork.menu, (void *)&uiNetwork.socksServer);
	UI_AddItem(&uiNetwork.menu, (void *)&uiNetwork.socksPort);
	UI_AddItem(&uiNetwork.menu, (void *)&uiNetwork.socksUsername);
	UI_AddItem(&uiNetwork.menu, (void *)&uiNetwork.socksPassword);
}

/*
 ==================
 UI_Network_Precache
 ==================
*/
void UI_Network_Precache (){

	R_RegisterMaterialNoMip(ART_BACKGROUND);
	R_RegisterMaterialNoMip(ART_BANNER);
	R_RegisterMaterialNoMip(ART_TEXT1);
	R_RegisterMaterialNoMip(ART_TEXT2);
}

/*
 ==================
 UI_Network_Menu
 ==================
*/
void UI_Network_Menu (){

	UI_Network_Precache();
	UI_Network_Init();

	UI_Network_UpdateConfig();

	UI_PushMenu(&uiNetwork.menu);
}