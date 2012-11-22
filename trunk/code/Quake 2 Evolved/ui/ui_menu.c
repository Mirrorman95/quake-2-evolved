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
// ui_menu.c - Main menu interface
//

// TODO:
// - most "ownerdraw" functions must be rewritten
// - i do not think uiStatic.menuActive->translucent is used


#include "ui_local.h"


uiStatic_t					uiStatic;

static cvar_t *				ui_precache;
static cvar_t *				ui_sensitivity;
cvar_t *					ui_singlePlayerSkill;

const char *				uiSoundIn		= "ui/menu1.wav";
const char *				uiSoundMove		= "ui/menu2.wav";
const char *				uiSoundOut		= "ui/menu3.wav";
const char *				uiSoundBuzz		= "ui/menu4.wav";
const char *				uiSoundNull		= "";

vec4_t						uiColorLightGray = {0.80f, 0.80f, 0.80f, 1.00f};
vec4_t						uiColorDarkGray = {0.25f, 0.25f, 0.25f, 1.00f};


/*
 =================
 UI_ScaleCoords

 Any parameter can be NULL if you don't want it
 =================
*/
void UI_ScaleCoords (int *x, int *y, int *w, int *h){

	if (x)
		*x *= uiStatic.scaleX;
	if (y)
		*y *= uiStatic.scaleY;
	if (w)
		*w *= uiStatic.scaleX;
	if (h)
		*h *= uiStatic.scaleY;
}

/*
 ==================
 UI_CursorInRect
 ==================
*/
bool UI_CursorInRect (int x, int y, int w, int h){

	if (uiStatic.cursorX < x)
		return false;
	if (uiStatic.cursorX > x+w)
		return false;
	if (uiStatic.cursorY < y)
		return false;
	if (uiStatic.cursorY > y+h)
		return false;

	return true;
}

/*
 ==================
 UI_DrawPic
 ==================
*/
void UI_DrawPic (float x, float y, float w, float h, const vec4_t color, const char *pic){

	material_t	*material;

	if (!pic || !pic[0])
		return;

	material = R_RegisterMaterialNoMip(pic);

	R_SetColor(color);
	R_DrawStretchPic(x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, H_NONE, 1.0f, V_NONE, 1.0f, material);
}

/*
 ==================
 UI_FillRect
 ==================
*/
void UI_FillRect (float x, float y, float w, float h, const vec4_t color){

	material_t	*material;

	material = R_RegisterMaterialNoMip("white");

	R_SetColor(color);
	R_DrawStretchPic(x, y, w, h, 0.0f, 0.0f, 1.0f, 1.0f, H_NONE, 1.0f, V_NONE, 1.0f, material);
}

/*
 ==================
 
 ==================
*/
void UI_DrawString (float x, float y, float w, float h, const char *string, const vec4_t color, bool forceColor, float charW, float charH, int justify, bool shadow){

	material_t		*material;

	if (!string || !string[0])
		return;

	material = R_RegisterMaterialNoMip("charset");

//	R_DrawString(x, y, w, h, string, color, forceColor, xShadow, yShadow, H_NONE, 1.0f, V_NONE, 1.0f, material);
}

// TODO: remove this placeholder
void UI_DrawPicOLD (int x, int y, int w, int h, const vec4_t color, const char *pic){

}

// TODO: remove this placeholder
void UI_DrawStringOLD (int x, int y, int w, int h, const char *string, const color_t color, bool forceColor, int charW, int charH, int justify, bool shadow){

}

/*
 ==================
 
 TODO: aspect ratio?
 ==================
*/
void UI_DrawMouseCursor (){

	material_t		*material = NULL;
	menuCommon_t	*item;
	int				w = UI_CURSOR_SIZE, h = UI_CURSOR_SIZE;
	int				i;

	for (i = 0; i < uiStatic.menuActive->numItems; i++){
		item = (menuCommon_t *)uiStatic.menuActive->items[i];

		if (item->flags & (QMF_INACTIVE | QMF_HIDDEN))
			continue;

		if (!UI_CursorInRect(item->x, item->y, item->width, item->height))
			continue;

		if (item->flags & QMF_GRAYED)
			material = R_RegisterMaterialNoMip(UI_CURSOR_DISABLED);
		else {
			if (item->type == QMTYPE_FIELD)
				material = R_RegisterMaterialNoMip(UI_CURSOR_TYPING);
		}

		break;
	}

	if (!material)
		material = R_RegisterMaterialNoMip(UI_CURSOR_NORMAL);

	R_SetColor1(1.0f);
	R_DrawStretchPic(uiStatic.cursorX, uiStatic.cursorY, w, h, 0.0f, 0.0f, 1.0f, 1.0f, H_NONE, 1.0f, V_NONE, 1.0f, material);
}

/*
 ==================
 UI_StartSound
 ==================
*/
void UI_StartSound (const char *name){

	sound_t	*sound;

	sound = S_RegisterSound(name, 0);
	S_PlayLocalSound(sound);
}


// ============================================================================


/*
 ==================
 UI_AddItem
 ==================
*/
void UI_AddItem (menuFramework_t *menu, void *item){

	menuCommon_t	*generic = (menuCommon_t *)item;

	if (menu->numItems >= UI_MAX_MENUITEMS)
		Com_Error(ERR_FATAL, "UI_AddItem: UI_MAX_MENUITEMS hit");

	menu->items[menu->numItems] = item;
	((menuCommon_t *)menu->items[menu->numItems])->parent = menu;
	((menuCommon_t *)menu->items[menu->numItems])->flags &= ~QMF_HASMOUSEFOCUS;
	menu->numItems++;

	switch (generic->type){
	case QMTYPE_SCROLLLIST:
		UI_ScrollList_Init((menuScrollList_t *)item);
		break;
	case QMTYPE_SPINCONTROL:
		UI_SpinControl_Init((menuSpinControl_t *)item);
		break;
	case QMTYPE_FIELD:
		UI_Field_Init((menuField_t *)item);
		break;
	case QMTYPE_ACTION:
		UI_Action_Init((menuAction_t *)item);
		break;
	case QMTYPE_BITMAP:
		UI_Bitmap_Init((menuBitmap_t *)item);
		break;
	default:
		Com_Error(ERR_FATAL, "UI_AddItem: unknown item type (%i)\n", generic->type);
	}
}

/*
 ==================
 UI_CursorMoved
 ==================
*/
void UI_CursorMoved (menuFramework_t *menu){

	void (*callback) (void *self, int event);

	if (menu->cursor == menu->cursorPrev)
		return;

	if (menu->cursorPrev >= 0 && menu->cursorPrev < menu->numItems){
		callback = ((menuCommon_t *)menu->items[menu->cursorPrev])->callback;
		if (callback)
			callback(menu->items[menu->cursorPrev], QM_LOSTFOCUS);
	}

	if (menu->cursor >= 0 && menu->cursor < menu->numItems){
		callback = ((menuCommon_t *)menu->items[menu->cursor])->callback;
		if (callback)
			callback(menu->items[menu->cursor], QM_GOTFOCUS);
	}
}

/*
 ==================
 UI_SetCursor
 ==================
*/
void UI_SetCursor (menuFramework_t *menu, int cursor){

	if (((menuCommon_t *)(menu->items[cursor]))->flags & (QMF_GRAYED | QMF_INACTIVE | QMF_HIDDEN))
		return;

	menu->cursorPrev = menu->cursor;
	menu->cursor = cursor;

	UI_CursorMoved(menu);
}

/*
 ==================
 UI_SetCursorToItem
 ==================
*/
void UI_SetCursorToItem (menuFramework_t *menu, void *item){

	int		i;

	for (i = 0; i < menu->numItems; i++){
		if (menu->items[i] == item){
			UI_SetCursor(menu, i);
			return;
		}
	}
}

/*
 ==================
 UI_ItemAtCursor
 ==================
*/
void *UI_ItemAtCursor (menuFramework_t *menu){

	if (menu->cursor < 0 || menu->cursor >= menu->numItems)
		return 0;

	return menu->items[menu->cursor];
}

/*
 ==================
 UI_AdjustCursor

 This functiont takes the given menu, the direction, and attempts to
 adjust the menu's cursor so that it's at the next available slot
 ==================
*/
void UI_AdjustCursor (menuFramework_t *menu, int dir){

	menuCommon_t	*item;
	bool			wrapped = false;

wrap:
	while (menu->cursor >= 0 && menu->cursor < menu->numItems){
		item = (menuCommon_t *)menu->items[menu->cursor];
		if (item->flags & (QMF_GRAYED | QMF_INACTIVE | QMF_HIDDEN | QMF_MOUSEONLY))
			menu->cursor += dir;
		else
			break;
	}

	if (dir == 1){
		if (menu->cursor >= menu->numItems){
			if (wrapped){
				menu->cursor = menu->cursorPrev;
				return;
			}
			menu->cursor = 0;
			wrapped = true;
			goto wrap;
		}
	}
	else if (dir == -1){
		if (menu->cursor < 0){
			if (wrapped){
				menu->cursor = menu->cursorPrev;
				return;
			}
			menu->cursor = menu->numItems - 1;
			wrapped = true;
			goto wrap;
		}
	}
}

/*
 ==================
 
 ==================
*/
void UI_DrawMenu (menuFramework_t *menu){

	static int			statusFadeTime;
	static menuCommon_t	*lastItem;
	color_t				color = {255, 255, 255, 255};
	int					i;
	menuCommon_t		*item;

	// Draw contents
	for (i = 0; i < menu->numItems; i++){
		item = (menuCommon_t *)menu->items[i];

		if (item->flags & QMF_HIDDEN)
			continue;

		if (item->ownerdraw){
			// Total subclassing, owner draws everything
			item->ownerdraw(item);
			continue;
		}

		switch (item->type){
		case QMTYPE_SCROLLLIST:
			UI_ScrollList_Draw((menuScrollList_t *)item);
			break;
		case QMTYPE_SPINCONTROL:
			UI_SpinControl_Draw((menuSpinControl_t *)item);
			break;
		case QMTYPE_FIELD:
			UI_Field_Draw((menuField_t *)item);
			break;
		case QMTYPE_ACTION:
			UI_Action_Draw((menuAction_t *)item);
			break;
		case QMTYPE_BITMAP:
			UI_Bitmap_Draw((menuBitmap_t *)item);
			break;
		}
	}

	// Draw status bar
	item = (menuCommon_t *)UI_ItemAtCursor(menu);
	if (item != lastItem){
		statusFadeTime = uiStatic.realTime;
		lastItem = item;
	}

	if (item && (item->flags & QMF_HASMOUSEFOCUS) && (item->statusText != NULL)){
		// Fade it in, but wait a second
		color[3] = Clamp((float)((uiStatic.realTime - statusFadeTime) - 1000) / 1000, 0.0f, 1.0f) * 255;

		UI_DrawStringOLD(0, 720*uiStatic.scaleY, 1024*uiStatic.scaleX, 28*uiStatic.scaleY, item->statusText, color, true, UI_SMALL_CHAR_WIDTH*uiStatic.scaleX, UI_SMALL_CHAR_HEIGHT*uiStatic.scaleY, 1, true);
	}
	else
		statusFadeTime = uiStatic.realTime;
}

/*
 ==================
 
 ==================
*/
const char *UI_DefaultKey (menuFramework_t *menu, int key){

	const char		*sound = 0;
	menuCommon_t	*item;
	int				cursorPrev;

	// Menu system key
	if (key == K_ESCAPE || key == K_MOUSE2){
		UI_PopMenu();
		return uiSoundOut;
	}

	if (!menu || !menu->numItems)
		return 0;

	item = (menuCommon_t *)UI_ItemAtCursor(menu);
	if (item && !(item->flags & (QMF_GRAYED | QMF_INACTIVE | QMF_HIDDEN))){
		switch (item->type){
		case QMTYPE_SCROLLLIST:
			sound = UI_ScrollList_Key((menuScrollList_t *)item, key);
			break;
		case QMTYPE_SPINCONTROL:
			sound = UI_SpinControl_Key((menuSpinControl_t *)item, key);
			break;
		case QMTYPE_FIELD:
			sound = UI_Field_Key((menuField_t *)item, key);
			break;
		case QMTYPE_ACTION:
			sound = UI_Action_Key((menuAction_t *)item, key);
			break;
		case QMTYPE_BITMAP:
			sound = UI_Bitmap_Key((menuBitmap_t *)item, key);
			break;
		}

		if (sound)
			return sound;	// Key was handled
	}

	// Default handling
	switch (key){
	case K_UPARROW:
	case K_KP_UPARROW:
	case K_LEFTARROW:
	case K_KP_LEFTARROW:
		cursorPrev = menu->cursor;
		menu->cursorPrev = menu->cursor;
		menu->cursor--;

		UI_AdjustCursor(menu, -1);
		if (cursorPrev != menu->cursor){
			UI_CursorMoved(menu);
			if (!(((menuCommon_t *)menu->items[menu->cursor])->flags & QMF_SILENT))
				sound = uiSoundMove;
		}

		break;
	case K_DOWNARROW:
	case K_KP_DOWNARROW:
	case K_RIGHTARROW:
	case K_KP_RIGHTARROW:
	case K_TAB:
		cursorPrev = menu->cursor;
		menu->cursorPrev = menu->cursor;
		menu->cursor++;

		UI_AdjustCursor(menu, 1);
		if (cursorPrev != menu->cursor){
			UI_CursorMoved(menu);
			if (!(((menuCommon_t *)menu->items[menu->cursor])->flags & QMF_SILENT))
				sound = uiSoundMove;
		}

		break;
	case K_MOUSE1:
		if (item){
			if ((item->flags & QMF_HASMOUSEFOCUS) && !(item->flags & (QMF_GRAYED | QMF_INACTIVE | QMF_HIDDEN)))
				return UI_ActivateItem(menu, item);
		}

		break;
	case K_ENTER:
	case K_KP_ENTER:
		if (item){
			if (!(item->flags & (QMF_GRAYED | QMF_INACTIVE | QMF_HIDDEN | QMF_MOUSEONLY)))
				return UI_ActivateItem(menu, item);
		}

		break;
	}

	return sound;
}		

/*
 ==================
 
 ==================
*/
const char *UI_ActivateItem (menuFramework_t *menu, menuCommon_t *item){

	if (item->callback){
		item->callback(item, QM_ACTIVATED);

		if (!(item->flags & QMF_SILENT))
			return uiSoundMove;
	}

	return 0;
}

/*
 ==================
 
 ==================
*/
void UI_RefreshServerList (void){

	int		i;

	for (i = 0; i < UI_MAX_SERVERS; i++){
		Mem_Fill(&uiStatic.serverAddresses[i], 0, sizeof(netAdr_t));
		Str_Copy(uiStatic.serverNames[i], "<no server>", sizeof(uiStatic.serverNames[i]));
	}

	uiStatic.numServers = 0;

	Cmd_ExecuteText(CMD_EXEC_APPEND, "localservers\npingservers\n");
}


// =====================================================================


/*
 ==================
 
 ==================
*/
void UI_CloseMenu (){

	uiStatic.menuActive = NULL;
	uiStatic.menuDepth = 0;
	uiStatic.visible = false;

	Key_ClearStates();
	IN_ClearStates();

	Key_SetKeyDest(KEY_GAME);

	CVar_ForceSet("paused", "0");
}

/*
 ==================
 
 ==================
*/
void UI_PushMenu (menuFramework_t *menu){

	int				i;
	menuCommon_t	*item;

	// Never pause in multiplayer
	if (!Com_ServerState() || CVar_GetVariableInteger("maxclients") > 1)
		CVar_ForceSet("paused", "0");
	else
		CVar_SetVariableString("paused", "1", false);

	// If this menu is already present, drop back to that level to avoid
	// stacking menus by hotkeys
	for (i = 0; i < uiStatic.menuDepth; i++){
		if (uiStatic.menuStack[i] == menu){
			uiStatic.menuDepth = i;
			break;
		}
	}

	if (i == uiStatic.menuDepth){
		if (uiStatic.menuDepth >= UI_MAX_MENUDEPTH)
			Com_Error(ERR_FATAL, "UI_PushMenu: menu stack overflow");

		uiStatic.menuStack[uiStatic.menuDepth++] = menu;
	}

	uiStatic.menuActive = menu;
	uiStatic.firstDraw = true;
	uiStatic.enterSound = true;
	uiStatic.visible = true;

	Key_SetKeyDest(KEY_MENU);

	menu->cursor = 0;
	menu->cursorPrev = 0;

	// Force first available item to have focus
	for (i = 0; i < menu->numItems; i++){
		item = (menuCommon_t *)menu->items[i];

		if (item->flags & (QMF_GRAYED | QMF_INACTIVE | QMF_HIDDEN | QMF_MOUSEONLY))
			continue;

		menu->cursorPrev = -1;
		UI_SetCursor(menu, i);
		break;
	}
}

/*
 ==================
 UI_PopMenu
 ==================
*/
void UI_PopMenu (){

	UI_StartSound(uiSoundOut);

	uiStatic.menuDepth--;

	if (uiStatic.menuDepth < 0)
		Com_Error(ERR_FATAL, "UI_PopMenu: menu stack underflow");

	if (uiStatic.menuDepth){
		uiStatic.menuActive = uiStatic.menuStack[uiStatic.menuDepth-1];
		uiStatic.firstDraw = true;
	}
	else
		UI_CloseMenu();
}


// =====================================================================


/*
 ==================
 UI_KeyEvent
 ==================
*/
void UI_KeyEvent (int key, bool down){

	const char	*sound;

	if (!uiStatic.initialized)
		return;

	if (!uiStatic.visible)
		return;

	if (!uiStatic.menuActive)
		return;

	if (!down)
		return;

	if (uiStatic.menuActive->keyFunc)
		sound = uiStatic.menuActive->keyFunc(key);
	else
		sound = UI_DefaultKey(uiStatic.menuActive, key);

	if (sound && sound != uiSoundNull)
		UI_StartSound(sound);
}

/*
 ==================
 
 ==================
*/
void UI_CharEvent (int ch){

}

/*
 ==================
 UI_MouseMove
 ==================
*/
void UI_MouseMove (float x, float y){

	menuCommon_t	*item;
	int				i;

	if (!uiStatic.initialized)
		return;

	if (!uiStatic.visible)
		return;

	if (!uiStatic.menuActive)
		return;

	x *= ui_sensitivity->floatValue;
	y *= ui_sensitivity->floatValue;

	// TODO: use ClampFloat
	uiStatic.cursorX += x;
	if (uiStatic.cursorX < 0.0f)
		uiStatic.cursorX = 0.0f;
	else if (uiStatic.cursorX > uiStatic.glConfig.videoWidth)
		uiStatic.cursorX = uiStatic.glConfig.videoWidth;

	uiStatic.cursorY += y;
	if (uiStatic.cursorY < 0.0f)
		uiStatic.cursorY = 0.0f;
	else if (uiStatic.cursorY > uiStatic.glConfig.videoHeight)
		uiStatic.cursorY = uiStatic.glConfig.videoHeight;

	// Region test the active menu items
	for (i = 0; i < uiStatic.menuActive->numItems; i++){
		item = (menuCommon_t *)uiStatic.menuActive->items[i];

		if (item->flags & (QMF_GRAYED | QMF_INACTIVE | QMF_HIDDEN))
			continue;

		if (!UI_CursorInRect(item->x, item->y, item->width, item->height))
			continue;

		// Set focus to item at cursor
		if (uiStatic.menuActive->cursor != i){
			UI_SetCursor(uiStatic.menuActive, i);
			((menuCommon_t *)(uiStatic.menuActive->items[uiStatic.menuActive->cursorPrev]))->flags &= ~QMF_HASMOUSEFOCUS;

			if (!(((menuCommon_t *)(uiStatic.menuActive->items[uiStatic.menuActive->cursor]))->flags & QMF_SILENT))
				UI_StartSound(uiSoundMove);
		}

		((menuCommon_t *)(uiStatic.menuActive->items[uiStatic.menuActive->cursor]))->flags |= QMF_HASMOUSEFOCUS;
		return;
	}

	// Out of any region
	if (uiStatic.menuActive->numItems){
		((menuCommon_t *)(uiStatic.menuActive->items[uiStatic.menuActive->cursor]))->flags &= ~QMF_HASMOUSEFOCUS;

		// A mouse only item restores focus to the previous item
		if (((menuCommon_t *)(uiStatic.menuActive->items[uiStatic.menuActive->cursor]))->flags & QMF_MOUSEONLY){
			if (uiStatic.menuActive->cursorPrev != -1)
				uiStatic.menuActive->cursor = uiStatic.menuActive->cursorPrev;
		}
	}
}

/*
 ==================
 UI_UpdateMenu
 ==================
*/
void UI_UpdateMenu (int realTime){

	if (!uiStatic.initialized)
		return;

	if (!uiStatic.visible)
		return;

	if (!uiStatic.menuActive)
		return;

	uiStatic.realTime = realTime;

	// Draw menu
	if (uiStatic.menuActive->drawFunc)
		uiStatic.menuActive->drawFunc();
	else
		UI_DrawMenu(uiStatic.menuActive);

	if (uiStatic.firstDraw){
		UI_MouseMove(0, 0);
		uiStatic.firstDraw = false;
	}

	// Draw cursor
	UI_DrawMouseCursor();

	// Delay playing the enter sound until after the menu has been
	// drawn, to avoid delay while caching images
	if (uiStatic.enterSound){
		UI_StartSound(uiSoundIn);
		uiStatic.enterSound = false;
	}
}

/*
 ==================
 
 ==================
*/
void UI_SetActiveMenu (uiActiveMenu_t activeMenu){

	if (!uiStatic.initialized)
		return;

	switch (activeMenu){
	case UI_CLOSEMENU:
		UI_CloseMenu();
		break;
	case UI_MAINMENU:
		UI_Main_Menu();
		break;
	case UI_INGAMEMENU:
		UI_InGame_Menu();
		break;
	default:
		Com_Error(ERR_FATAL, "UI_SetActiveMenu: bad activeMenu (%i)", activeMenu);
	}
}

/*
 ==================
 
 ==================
*/
void UI_AddServerToList (netAdr_t address, const char *info){

	int		i;

	if (!uiStatic.initialized)
		return;

	if (uiStatic.numServers == UI_MAX_SERVERS)
		return;		// Full

	while (*info == ' ')
		info++;

	// Ignore if duplicated
	for (i = 0; i < uiStatic.numServers; i++){
		if (!Str_ICompare(uiStatic.serverNames[i], info))
			return;
	}

	// Add it to the list
	uiStatic.serverAddresses[uiStatic.numServers] = address;
	Str_Copy(uiStatic.serverNames[uiStatic.numServers], info, sizeof(uiStatic.serverNames[uiStatic.numServers]));
	uiStatic.numServers++;
}

/*
 ==================
 UI_IsVisible

 Some systems may need to know if it is visible or not
 ==================
*/
bool UI_IsVisible (){

	if (!uiStatic.initialized)
		return false;

	return uiStatic.visible;
}

/*
 ==================
 UI_IsFullscreen
 ==================
*/
bool UI_IsFullscreen (){

	if (!uiStatic.initialized)
		return false;

	if (!uiStatic.visible)
		return false;

	if (!uiStatic.menuActive)
		return false;

	return !uiStatic.menuActive->translucent;
}

/*
 ==================
 UI_Precache
 ==================
*/
void UI_Precache (){

	if (!uiStatic.initialized)
		return;

	if (!ui_precache->integerValue)
		return;

	S_RegisterSound(uiSoundIn, 0);
	S_RegisterSound(uiSoundMove, 0);
	S_RegisterSound(uiSoundOut, 0);
	S_RegisterSound(uiSoundBuzz, 0);

	R_RegisterMaterialNoMip(UI_CURSOR_NORMAL);
	R_RegisterMaterialNoMip(UI_CURSOR_DISABLED);
	R_RegisterMaterialNoMip(UI_CURSOR_TYPING);
	R_RegisterMaterialNoMip(UI_LEFTARROW);
	R_RegisterMaterialNoMip(UI_LEFTARROWFOCUS);
	R_RegisterMaterialNoMip(UI_RIGHTARROW);
	R_RegisterMaterialNoMip(UI_RIGHTARROWFOCUS);
	R_RegisterMaterialNoMip(UI_UPARROW);
	R_RegisterMaterialNoMip(UI_UPARROWFOCUS);
	R_RegisterMaterialNoMip(UI_DOWNARROW);
	R_RegisterMaterialNoMip(UI_DOWNARROWFOCUS);
	R_RegisterMaterialNoMip(UI_BACKGROUNDLISTBOX);
	R_RegisterMaterialNoMip(UI_SELECTIONBOX);
	R_RegisterMaterialNoMip(UI_BACKGROUNDBOX);
	R_RegisterMaterialNoMip(UI_MOVEBOX);
	R_RegisterMaterialNoMip(UI_MOVEBOXFOCUS);
	R_RegisterMaterialNoMip(UI_BACKBUTTON);
	R_RegisterMaterialNoMip(UI_LOADBUTTON);
	R_RegisterMaterialNoMip(UI_SAVEBUTTON);
	R_RegisterMaterialNoMip(UI_DELETEBUTTON);
	R_RegisterMaterialNoMip(UI_CANCELBUTTON);
	R_RegisterMaterialNoMip(UI_APPLYBUTTON);
	R_RegisterMaterialNoMip(UI_ACCEPTBUTTON);
	R_RegisterMaterialNoMip(UI_PLAYBUTTON);
	R_RegisterMaterialNoMip(UI_STARTBUTTON);
	R_RegisterMaterialNoMip(UI_NEWGAMEBUTTON);

	if (ui_precache->integerValue == 1)
		return;

	UI_Main_Precache();
	UI_InGame_Precache();
	UI_SinglePlayer_Precache();
	UI_LoadGame_Precache();
	UI_SaveGame_Precache();
	UI_MultiPlayer_Precache();
	UI_Options_Precache();
	UI_PlayerSetup_Precache();
	UI_Controls_Precache();
	UI_GameOptions_Precache();
	UI_Audio_Precache();
	UI_Video_Precache();
	UI_Advanced_Precache();
	UI_Performance_Precache();
	UI_Network_Precache();
	UI_Defaults_Precache();
	UI_Cinematics_Precache();
	UI_Demos_Precache();
	UI_Mods_Precache();
	UI_Quit_Precache();
	UI_Credits_Precache();
	UI_GoToSite_Precache();
}


/*
 ==============================================================================

 INITIALIZATION AND SHUTDOWN

 ==============================================================================
*/


/*
 ==================
 UI_Init
 ==================
*/
void UI_Init (){

	// Register variables
	ui_precache = CVar_Register("ui_precache", "0", CVAR_BOOL, CVAR_ARCHIVE, "Pre-cache UI assets", 0, 0);
	ui_sensitivity = CVar_Register("ui_sensitivity", "1.0", CVAR_FLOAT, CVAR_ARCHIVE, "Mouse sensitivity", 0.0f, 10.0f);
	ui_singlePlayerSkill = CVar_Register("ui_singlePlayerSkill", "1", CVAR_INTEGER, CVAR_ARCHIVE, NULL, 0, 3);

	// Add commands
	Cmd_AddCommand("menu_main", UI_Main_Menu, NULL, NULL);
	Cmd_AddCommand("menu_ingame", UI_InGame_Menu, NULL, NULL);
	Cmd_AddCommand("menu_singleplayer", UI_SinglePlayer_Menu, NULL, NULL);
	Cmd_AddCommand("menu_loadgame", UI_LoadGame_Menu, NULL, NULL);
	Cmd_AddCommand("menu_savegame", UI_SaveGame_Menu, NULL, NULL);
	Cmd_AddCommand("menu_multiplayer", UI_MultiPlayer_Menu, NULL, NULL);
	Cmd_AddCommand("menu_options", UI_Options_Menu, NULL, NULL);
	Cmd_AddCommand("menu_playersetup", UI_PlayerSetup_Menu, NULL, NULL);
	Cmd_AddCommand("menu_controls", UI_Controls_Menu, NULL, NULL);
	Cmd_AddCommand("menu_gameoptions", UI_GameOptions_Menu, NULL, NULL);
	Cmd_AddCommand("menu_audio", UI_Audio_Menu, NULL, NULL);
	Cmd_AddCommand("menu_video", UI_Video_Menu, NULL, NULL);
	Cmd_AddCommand("menu_advanced", UI_Advanced_Menu, NULL, NULL);
	Cmd_AddCommand("menu_performance", UI_Performance_Menu, NULL, NULL);
	Cmd_AddCommand("menu_network", UI_Network_Menu, NULL, NULL);
	Cmd_AddCommand("menu_defaults", UI_Defaults_Menu, NULL, NULL);
	Cmd_AddCommand("menu_cinematics", UI_Cinematics_Menu, NULL, NULL);
	Cmd_AddCommand("menu_demos", UI_Demos_Menu, NULL, NULL);
	Cmd_AddCommand("menu_mods", UI_Mods_Menu, NULL, NULL);
	Cmd_AddCommand("menu_quit", UI_Quit_Menu, NULL, NULL);
	Cmd_AddCommand("menu_credits", UI_Credits_Menu, NULL, NULL);

	// Get information about the OpenGL / OpenAL subsystems
	uiStatic.glConfig = R_GetGLConfig();
	uiStatic.alConfig = S_GetALConfig();

	// Compute the coordinate scales
	uiStatic.scaleX = uiStatic.glConfig.videoWidth / 1024.0f;
	uiStatic.scaleY = uiStatic.glConfig.videoHeight / 768.0f;

	uiStatic.initialized = true;

	Com_Printf("UI System Initialized\n");
}

/*
 ==================
 UI_Shutdown
 ==================
*/
void UI_Shutdown (){

	if (!uiStatic.initialized)
		return;

	// Remove commands
	Cmd_RemoveCommand("menu_main");
	Cmd_RemoveCommand("menu_ingame");
	Cmd_RemoveCommand("menu_singleplayer");
	Cmd_RemoveCommand("menu_loadgame");
	Cmd_RemoveCommand("menu_savegame");
	Cmd_RemoveCommand("menu_multiplayer");
	Cmd_RemoveCommand("menu_options");
	Cmd_RemoveCommand("menu_playersetup");
	Cmd_RemoveCommand("menu_controls");
	Cmd_RemoveCommand("menu_gameoptions");
	Cmd_RemoveCommand("menu_audio");
	Cmd_RemoveCommand("menu_video");
	Cmd_RemoveCommand("menu_advanced");
	Cmd_RemoveCommand("menu_performance");
	Cmd_RemoveCommand("menu_network");
	Cmd_RemoveCommand("menu_defaults");
	Cmd_RemoveCommand("menu_cinematics");
	Cmd_RemoveCommand("menu_demos");
	Cmd_RemoveCommand("menu_mods");
	Cmd_RemoveCommand("menu_quit");
	Cmd_RemoveCommand("menu_credits");

	// Make sure we stop a playing cinematic
	if (uiStatic.playingCinematic)
		CIN_StopCinematic();

	// Free all UI system allocations
	Mem_Fill(&uiStatic, 0, sizeof(uiStatic_t));
}