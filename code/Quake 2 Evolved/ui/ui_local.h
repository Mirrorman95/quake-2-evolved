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
// ui_local.h - Local header file to all UI files
//


#ifndef __UI_LOCAL_H
#define __UI_LOCAL_H__


#include "../client/client.h"


#define UI_CURSOR_NORMAL			"ui/assets/misc/mouse_cursor"
#define UI_CURSOR_DISABLED			"ui/assets/misc/mouse_cursor_denied"
#define UI_CURSOR_TYPING			"ui/assets/misc/mouse_cursor_type"
#define UI_LEFTARROW				"ui/assets/arrows/arrow_left_small"
#define UI_LEFTARROWFOCUS			"ui/assets/arrows/arrow_left_small_s"
#define UI_RIGHTARROW				"ui/assets/arrows/arrow_right_small"
#define UI_RIGHTARROWFOCUS			"ui/assets/arrows/arrow_right_small_s"
#define UI_UPARROW					"ui/assets/arrows/arrow_up_big"
#define UI_UPARROWFOCUS				"ui/assets/arrows/arrow_up_big_s"
#define UI_DOWNARROW				"ui/assets/arrows/arrow_down_big"
#define UI_DOWNARROWFOCUS			"ui/assets/arrows/arrow_down_big_s"
#define UI_BACKGROUNDLISTBOX		"ui/assets/segments/list_mid"
#define UI_SELECTIONBOX				"ui/assets/ui/list_sel"
#define UI_BACKGROUNDBOX			"ui/assets/buttons/options2_b"
#define UI_MOVEBOX					"ui/assets/buttons/move_box"
#define UI_MOVEBOXFOCUS				"ui/assets/buttons/move_box_s"
#define UI_BACKBUTTON				"ui/assets/buttons/back_b"
#define UI_LOADBUTTON				"ui/assets/buttons/load_b"
#define UI_SAVEBUTTON				"ui/assets/buttons/save_b"
#define UI_DELETEBUTTON				"ui/assets/buttons/delete_b"
#define UI_CANCELBUTTON				"ui/assets/buttons/cancel_b"
#define UI_APPLYBUTTON				"ui/assets/buttons/apply_b"
#define UI_ACCEPTBUTTON				"ui/assets/buttons/accept_b"
#define UI_PLAYBUTTON				"ui/assets/buttons/play_b"
#define UI_STARTBUTTON				"ui/assets/buttons/fight_b"
#define UI_NEWGAMEBUTTON			"ui/assets/buttons/newgame_b"

#define UI_MAX_MENUDEPTH			8
#define UI_MAX_MENUITEMS			64

#define UI_CURSOR_SIZE				40

#define UI_PULSE_DIVISOR			75
#define UI_BLINK_TIME				250
#define UI_BLINK_MASK				499

#define UI_SMALL_CHAR_WIDTH			10
#define UI_SMALL_CHAR_HEIGHT		20
#define UI_BIG_CHAR_WIDTH			20
#define UI_BIG_CHAR_HEIGHT			40

#define UI_MAX_FIELD_LINE			256

#define UI_MAX_SERVERS				10

// Generic types
typedef enum {
	QMTYPE_SCROLLLIST,
	QMTYPE_SPINCONTROL,
	QMTYPE_FIELD,
	QMTYPE_ACTION,
	QMTYPE_BITMAP
} menuType_t;

// Generic flags
typedef enum {
	QMF_LEFT_JUSTIFY				= BIT(0),
	QMF_CENTER_JUSTIFY				= BIT(1),
	QMF_RIGHT_JUSTIFY				= BIT(2),
	QMF_GRAYED						= BIT(3),	// Grays and disables
	QMF_INACTIVE					= BIT(4),	// Disables any input
	QMF_HIDDEN						= BIT(5),	// Doesn't draw
	QMF_NUMBERSONLY					= BIT(6),	// Edit field is only numbers
	QMF_LOWERCASE					= BIT(7),	// Edit field is all lower case
	QMF_UPPERCASE					= BIT(8),	// Edit field is all upper case
	QMF_BLINKIFFOCUS				= BIT(9),
	QMF_PULSEIFFOCUS				= BIT(10),
	QMF_HIGHLIGHTIFFOCUS			= BIT(11),
	QMF_SMALLFONT					= BIT(12),
	QMF_BIGFONT						= BIT(13),
	QMF_DROPSHADOW					= BIT(14),
	QMF_SILENT						= BIT(15),	// Don't play sounds
	QMF_HASMOUSEFOCUS				= BIT(16),
	QMF_MOUSEONLY					= BIT(17),	// Only mouse input allowed
	QMF_FOCUSBEHIND					= BIT(18)	// Focus draws behind normal item
} menuFlags_t;

// Callback notifications
typedef enum {
	QM_GOTFOCUS						= 1,
	QM_LOSTFOCUS					= 2,
	QM_ACTIVATED					= 3,
	QM_CHANGED						= 4
} menuCallbacks_t;

typedef struct {
	bool					translucent;

	int						cursor;
	int						cursorPrev;

	void *					items[UI_MAX_MENUITEMS];
	int						numItems;

	void					(*drawFunc) ();
	const char				*(*keyFunc) (int key);
} menuFramework_t;

typedef struct {
	menuType_t				type;
	const char *			name;
	int						id;

	int						flags;

	int						x;
	int						y;
	int						width;
	int						height;

	int						x2;
	int						y2;
	int						width2;
	int						height2;

	float *					color;
	float *					focusColor;

	int						charWidth;
	int						charHeight;

	const char *			statusText;

	menuFramework_t *		parent;

	void					(*callback) (void *self, int event);
	void					(*ownerdraw) (void *self);
} menuCommon_t;

typedef struct {
	menuCommon_t			generic;

	const char *			background;
	const char *			upArrow;
	const char *			upArrowFocus;
	const char *			downArrow;
	const char *			downArrowFocus;
	const char **			itemNames;
	int						numItems;
	int						curItem;
	int						topItem;
	int						numRows;
} menuScrollList_t;

typedef struct {
	menuCommon_t			generic;

	const char *			background;
	const char *			leftArrow;
	const char *			rightArrow;
	const char *			leftArrowFocus;
	const char *			rightArrowFocus;
	float					minValue;
	float					maxValue;
	float					curValue;
	float					range;
} menuSpinControl_t;

typedef struct {
	menuCommon_t			generic;

	const char *			background;
	int						maxLenght;
	int						visibleLength;
	char					buffer[UI_MAX_FIELD_LINE];
	int						length;
	int						cursor;
} menuField_t;

typedef struct {
	menuCommon_t			generic;

	const char *			background;
} menuAction_t;

typedef struct {
	menuCommon_t			generic;

	const char *			pic;
	const char *			focusPic;
} menuBitmap_t;

void			UI_ScrollList_Init (menuScrollList_t *sl);
const char *	UI_ScrollList_Key (menuScrollList_t *sl, int key);
void			UI_ScrollList_Draw (menuScrollList_t *sl);

void			UI_SpinControl_Init (menuSpinControl_t *sc);
const char *	UI_SpinControl_Key (menuSpinControl_t *sc, int key);
void			UI_SpinControl_Draw (menuSpinControl_t *sc);

void			UI_Field_Init (menuField_t *f);
const char *	UI_Field_Key (menuField_t *f, int key);
void			UI_Field_Draw (menuField_t *f);

void			UI_Action_Init (menuAction_t *t);
const char *	UI_Action_Key (menuAction_t *t, int key);
void			UI_Action_Draw (menuAction_t *t);

void			UI_Bitmap_Init (menuBitmap_t *b);
const char *	UI_Bitmap_Key (menuBitmap_t *b, int key);
void			UI_Bitmap_Draw (menuBitmap_t *b);

/*
 ==============================================================================

 GLOBALS

 ==============================================================================
*/

typedef struct {
	char					name[MAX_QPATH];
	cinHandle_t				handle;
} uiCin_t;

typedef struct {
	menuFramework_t *		menuActive;
	menuFramework_t	*		menuStack[UI_MAX_MENUDEPTH];
	int						menuDepth;

	netAdr_t				serverAddresses[UI_MAX_SERVERS];
	char					serverNames[UI_MAX_SERVERS][80];
	int						numServers;

	bool					playingCinematic;

	glConfig_t				glConfig;
	alConfig_t				alConfig;

	float					scaleX;
	float					scaleY;

	float					cursorX;
	float					cursorY;
	int						realTime;
	bool					firstDraw;
	bool					enterSound;
	bool					visible;

	bool					initialized;
} uiStatic_t;

extern uiStatic_t			uiStatic;

extern cvar_t *				ui_precache;
extern cvar_t *				ui_sensitivity;
extern cvar_t *				ui_singlePlayerSkill;

extern const char *			uiSoundIn;
extern const char *			uiSoundMove;
extern const char *			uiSoundOut;
extern const char *			uiSoundBuzz;
extern const char *			uiSoundNull;

extern vec4_t				uiColorLightGray;
extern vec4_t				uiColorDarkGray;

void			UI_ScaleCoords (int *x, int *y, int *w, int *h);
bool			UI_CursorInRect (int x, int y, int w, int h);
void			UI_DrawPic (float x, float y, float w, float h, const vec4_t color, const char *pic);
void			UI_FillRect (float x, float y, float w, float h, const vec4_t color);
void			UI_DrawString (float x, float y, float w, float h, const char *string, const vec4_t color, bool forceColor, float xShadow, float yShadow, horzAdjust_t horzAdjust, float horzPercent, vertAdjust_t vertAdjust, float vertPercent);
void			UI_DrawPicOLD (int x, int y, int w, int h, const vec4_t color, const char *pic);
void			UI_DrawStringOLD (int x, int y, int w, int h, const char *string, const color_t color, bool forceColor, int charW, int charH, int justify, bool shadow);
void			UI_StartSound (const char *sound);

void			UI_AddItem (menuFramework_t *menu, void *item);
void			UI_CursorMoved (menuFramework_t *menu);
void			UI_SetCursor (menuFramework_t *menu, int cursor);
void			UI_SetCursorToItem (menuFramework_t *menu, void *item);
void *			UI_ItemAtCursor (menuFramework_t *menu);
void			UI_AdjustCursor (menuFramework_t *menu, int dir);
void			UI_DrawMenu (menuFramework_t *menu);
const char *	UI_DefaultKey (menuFramework_t *menu, int key);
const char *	UI_ActivateItem (menuFramework_t *menu, menuCommon_t *item);
void			UI_RefreshServerList ();

void			UI_CloseMenu ();
void			UI_PushMenu (menuFramework_t *menu);
void			UI_PopMenu ();

// Precache
void			UI_Main_Precache ();
void			UI_InGame_Precache ();
	void			UI_SinglePlayer_Precache ();
		void			UI_LoadGame_Precache ();
		void			UI_SaveGame_Precache ();
	void			UI_MultiPlayer_Precache ();
	void			UI_Options_Precache ();
		void			UI_PlayerSetup_Precache ();
		void			UI_Controls_Precache ();
		void			UI_GameOptions_Precache ();
		void			UI_Audio_Precache ();
		void			UI_Video_Precache ();
			void			UI_Advanced_Precache ();
		void			UI_Performance_Precache ();
		void			UI_Network_Precache ();
		void			UI_Defaults_Precache ();
	void			UI_Cinematics_Precache ();
	void			UI_Demos_Precache ();
	void			UI_Mods_Precache ();
	void			UI_Quit_Precache ();
		void			UI_Credits_Precache ();

void				UI_GoToSite_Precache ();

// Menus
void			UI_Main_Menu ();
void			UI_InGame_Menu ();
	void			UI_SinglePlayer_Menu ();
		void			UI_LoadGame_Menu ();
		void			UI_SaveGame_Menu ();
	void			UI_MultiPlayer_Menu ();
	void			UI_Options_Menu ();
		void			UI_PlayerSetup_Menu ();
		void			UI_Controls_Menu ();
		void			UI_GameOptions_Menu ();
		void			UI_Audio_Menu ();
		void			UI_Video_Menu ();
			void			UI_Advanced_Menu ();
		void			UI_Performance_Menu ();
		void			UI_Network_Menu ();
		void			UI_Defaults_Menu ();
	void			UI_Cinematics_Menu ();
	void			UI_Demos_Menu ();
	void			UI_Mods_Menu ();
	void			UI_Quit_Menu ();
		void			UI_Credits_Menu ();

void			UI_GoToSite_Menu (const char *webAddress);


#endif	// __UI_LOCAL_H__