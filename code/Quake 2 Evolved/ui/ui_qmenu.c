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
// ui_qmenu.c - Quake back-end menu framework
//


#include "ui_local.h"


/*
 ==============================================================================

 SCROLL LIST

 ==============================================================================
*/


/*
 ==================
 UI_ScrollList_Init
 ==================
*/
void UI_ScrollList_Init (menuScrollList_t *sl){

	if (!sl->generic.name)
		sl->generic.name = "";

	if (sl->generic.flags & QMF_BIGFONT){
		sl->generic.charWidth = UI_BIG_CHAR_WIDTH;
		sl->generic.charHeight = UI_BIG_CHAR_HEIGHT;
	}
	else if (sl->generic.flags & QMF_SMALLFONT){
		sl->generic.charWidth = UI_SMALL_CHAR_WIDTH;
		sl->generic.charHeight = UI_SMALL_CHAR_HEIGHT;
	}
	else {
		if (sl->generic.charWidth < 1)
			sl->generic.charWidth = UI_SMALL_CHAR_WIDTH;
		if (sl->generic.charHeight < 1)
			sl->generic.charHeight = UI_SMALL_CHAR_HEIGHT;
	}

	UI_ScaleCoords(NULL, NULL, &sl->generic.charWidth, &sl->generic.charHeight);

	if (!(sl->generic.flags & (QMF_LEFT_JUSTIFY | QMF_CENTER_JUSTIFY | QMF_RIGHT_JUSTIFY)))
		sl->generic.flags |= QMF_LEFT_JUSTIFY;

	if (!sl->generic.color)
		sl->generic.color = colorWhite;
	if (!sl->generic.focusColor)
		sl->generic.focusColor = uiColorLightGray;

	if (!sl->background)
		sl->background = UI_BACKGROUNDLISTBOX;

	if (!sl->upArrow)
		sl->upArrow = UI_UPARROW;
	if (!sl->upArrowFocus)
		sl->upArrowFocus = UI_UPARROWFOCUS;
	if (!sl->downArrow)
		sl->downArrow = UI_DOWNARROW;
	if (!sl->downArrowFocus)
		sl->downArrowFocus = UI_DOWNARROWFOCUS;

	sl->curItem = 0;
	sl->topItem = 0;

	// Count number of items
	sl->numItems = 0;
	while (sl->itemNames[sl->numItems])
		sl->numItems++;

	// Scale the center box
	sl->generic.x2 = sl->generic.x;
	sl->generic.y2 = sl->generic.y;
	sl->generic.width2 = sl->generic.width;
	sl->generic.height2 = sl->generic.height;
	UI_ScaleCoords(&sl->generic.x2, &sl->generic.y2, &sl->generic.width2, &sl->generic.height2);

	// Calculate number of visible rows
	sl->numRows = (sl->generic.height2 / sl->generic.charHeight) - 2;
	if (sl->numRows > sl->numItems)
		sl->numRows = sl->numItems;

	// Extend the height so it has room for the arrows
	sl->generic.height += (sl->generic.width / 4);

	// Calculate new Y for the control
	sl->generic.y -= (sl->generic.width / 8);

	UI_ScaleCoords(&sl->generic.x, &sl->generic.y, &sl->generic.width, &sl->generic.height);
}

/*
 ==================
 
 ==================
*/
const char *UI_ScrollList_Key (menuScrollList_t *sl, int key){

	return 0;
}

/*
 ==================
 
 ==================
*/
void UI_ScrollList_Draw (menuScrollList_t *sl){

	int		justify;
	bool	shadow;
	int		x, y, w, h;
	int		arrowWidth, arrowHeight, upX, upY, downX, downY;
	bool	upFocus, downFocus;
	int		i;

	if (sl->generic.flags & QMF_LEFT_JUSTIFY)
		justify = 0;
	else if (sl->generic.flags & QMF_CENTER_JUSTIFY)
		justify = 1;
	else if (sl->generic.flags & QMF_RIGHT_JUSTIFY)
		justify = 2;

	shadow = (sl->generic.flags & QMF_DROPSHADOW);

	// Calculate size and position for the arrows
	arrowWidth = sl->generic.width / 4;
	arrowHeight = sl->generic.width / 8;

	upX = sl->generic.x + (arrowWidth * 1.5);
	upY = sl->generic.y;
	downX = sl->generic.x + (arrowWidth * 1.5);
	downY = sl->generic.y + (sl->generic.height - arrowHeight);

	// Draw the arrows
	if (sl->generic.flags & QMF_GRAYED){
		UI_DrawPic(upX, upY, arrowWidth, arrowHeight, colorWhite, sl->upArrow);
		UI_DrawPic(downX, downY, arrowWidth, arrowHeight, colorWhite, sl->downArrow);
	}
	else {
		if ((menuCommon_t *)sl != (menuCommon_t *)UI_ItemAtCursor(sl->generic.parent)){
			UI_DrawPic(upX, upY, arrowWidth, arrowHeight, sl->generic.color, sl->upArrow);
			UI_DrawPic(downX, downY, arrowWidth, arrowHeight, sl->generic.color, sl->downArrow);
		}
		else {
			// See which arrow has the mouse focus
			upFocus = UI_CursorInRect(upX, upY, arrowWidth, arrowHeight);
			downFocus = UI_CursorInRect(downX, downY, arrowWidth, arrowHeight);

			if (!(sl->generic.flags & QMF_FOCUSBEHIND)){
				UI_DrawPic(upX, upY, arrowWidth, arrowHeight, sl->generic.color, sl->upArrow);
				UI_DrawPic(downX, downY, arrowWidth, arrowHeight, sl->generic.color, sl->downArrow);
			}

			if (sl->generic.flags & QMF_HIGHLIGHTIFFOCUS){
				UI_DrawPic(upX, upY, arrowWidth, arrowHeight, (upFocus) ? sl->generic.focusColor : sl->generic.color, (upFocus) ? sl->upArrowFocus : sl->upArrow);
				UI_DrawPic(downX, downY, arrowWidth, arrowHeight, (downFocus) ? sl->generic.focusColor : sl->generic.color, (downFocus) ? sl->downArrowFocus : sl->downArrow);
			}
			else if (sl->generic.flags & QMF_PULSEIFFOCUS){
				vec4_t	color;

				VectorCopy(sl->generic.color, color);
				color[3] = 1.0f * (0.5f + 0.5f * sin(uiStatic.realTime / UI_PULSE_DIVISOR));

				UI_DrawPic(upX, upY, arrowWidth, arrowHeight, (upFocus) ? color : sl->generic.color, (upFocus) ? sl->upArrowFocus : sl->upArrow);
				UI_DrawPic(downX, downY, arrowWidth, arrowHeight, (downFocus) ? color : sl->generic.color, (downFocus) ? sl->downArrowFocus : sl->downArrow);
			}
			else if (sl->generic.flags & QMF_BLINKIFFOCUS){
				if ((uiStatic.realTime & UI_BLINK_MASK) < UI_BLINK_TIME){
					UI_DrawPic(upX, upY, arrowWidth, arrowHeight, (upFocus) ? sl->generic.focusColor : sl->generic.color, (upFocus) ? sl->upArrowFocus : sl->upArrow);
					UI_DrawPic(downX, downY, arrowWidth, arrowHeight, (downFocus) ? sl->generic.focusColor : sl->generic.color, (downFocus) ? sl->downArrowFocus : sl->downArrow);
				}
			}

			if (sl->generic.flags & QMF_FOCUSBEHIND){
				UI_DrawPic(upX, upY, arrowWidth, arrowHeight, sl->generic.color, sl->upArrow);
				UI_DrawPic(downX, downY, arrowWidth, arrowHeight, sl->generic.color, sl->downArrow);
			}
		}
	}

	// Hightlight the selected item
	if (!(sl->generic.flags & QMF_GRAYED)){
		y = sl->generic.y2 + sl->generic.charHeight;
		for (i = sl->topItem; i < sl->topItem + sl->numRows; i++, y += sl->generic.charHeight){
			if (!sl->itemNames[i])
				break;		// Done

			if (i == sl->curItem){
				UI_DrawPic(sl->generic.x, y, sl->generic.width, sl->generic.charHeight, colorWhite, UI_SELECTIONBOX);
				break;
			}
		}
	}

	// Get size and position for the center box
	w = sl->generic.width2;
	h = sl->generic.height2;
	x = sl->generic.x2;
	y = sl->generic.y2;

	UI_DrawPic(x, y, w, h, colorWhite, sl->background);

	// Draw the list
	h = sl->generic.charHeight;
	y = sl->generic.y2 + sl->generic.charHeight;
	for (i = sl->topItem; i < sl->topItem + sl->numRows; i++, y += sl->generic.charHeight){
		if (!sl->itemNames[i])
			break;		// Done

		if (sl->generic.flags & QMF_GRAYED){
			UI_DrawStringOLD(x, y, w, h, sl->itemNames[i], colorWhite, true, sl->generic.charWidth, sl->generic.charHeight, justify, shadow);
			continue;	// Grayed
		}

		if (i != sl->curItem){
			UI_DrawStringOLD(x, y, w, h, sl->itemNames[i], sl->generic.color, false, sl->generic.charWidth, sl->generic.charHeight, justify, shadow);
			continue;	// No focus
		}

		if (!(sl->generic.flags & QMF_FOCUSBEHIND))
			UI_DrawStringOLD(x, y, w, h, sl->itemNames[i], sl->generic.color, false, sl->generic.charWidth, sl->generic.charHeight, justify, shadow);

		if (sl->generic.flags & QMF_HIGHLIGHTIFFOCUS)
			UI_DrawStringOLD(x, y, w, h, sl->itemNames[i], sl->generic.focusColor, false, sl->generic.charWidth, sl->generic.charHeight, justify, shadow);
		else if (sl->generic.flags & QMF_PULSEIFFOCUS){
			vec4_t	color;

			VectorCopy(sl->generic.color, color);
			color[3] = 1.0f * (0.5f + 0.5f * sin(uiStatic.realTime / UI_PULSE_DIVISOR));

			UI_DrawStringOLD(x, y, w, h, sl->itemNames[i], color, false, sl->generic.charWidth, sl->generic.charHeight, justify, shadow);
		}
		else if (sl->generic.flags & QMF_BLINKIFFOCUS){
			if ((uiStatic.realTime & UI_BLINK_MASK) < UI_BLINK_TIME)
				UI_DrawStringOLD(x, y, w, h, sl->itemNames[i], sl->generic.focusColor, false, sl->generic.charWidth, sl->generic.charHeight, justify, shadow);
		}

		if (sl->generic.flags & QMF_FOCUSBEHIND)
			UI_DrawStringOLD(x, y, w, h, sl->itemNames[i], sl->generic.color, false, sl->generic.charWidth, sl->generic.charHeight, justify, shadow);
	}
}


/*
 ==============================================================================

 SPIN CONTROL

 ==============================================================================
*/


/*
 ==================
 UI_SpinControl_Init
 ==================
*/
void UI_SpinControl_Init (menuSpinControl_t *sc){

	if (!sc->generic.name)
		sc->generic.name = "";	// This is also the text displayed

	if (sc->generic.flags & QMF_BIGFONT){
		sc->generic.charWidth = UI_BIG_CHAR_WIDTH;
		sc->generic.charHeight = UI_BIG_CHAR_HEIGHT;
	}
	else if (sc->generic.flags & QMF_SMALLFONT){
		sc->generic.charWidth = UI_SMALL_CHAR_WIDTH;
		sc->generic.charHeight = UI_SMALL_CHAR_HEIGHT;
	}
	else {
		if (sc->generic.charWidth < 1)
			sc->generic.charWidth = UI_SMALL_CHAR_WIDTH;
		if (sc->generic.charHeight < 1)
			sc->generic.charHeight = UI_SMALL_CHAR_HEIGHT;
	}

	UI_ScaleCoords(NULL, NULL, &sc->generic.charWidth, &sc->generic.charHeight);

	if (!(sc->generic.flags & (QMF_LEFT_JUSTIFY | QMF_CENTER_JUSTIFY | QMF_RIGHT_JUSTIFY)))
		sc->generic.flags |= QMF_LEFT_JUSTIFY;

	if (!sc->generic.color)
		sc->generic.color = colorWhite;
	if (!sc->generic.focusColor)
		sc->generic.focusColor = uiColorLightGray;

	if (!sc->background)
		sc->background = UI_BACKGROUNDBOX;

	if (!sc->leftArrow)
		sc->leftArrow = UI_LEFTARROW;
	if (!sc->leftArrowFocus)
		sc->leftArrowFocus = UI_LEFTARROWFOCUS;
	if (!sc->rightArrow)
		sc->rightArrow = UI_RIGHTARROW;
	if (!sc->rightArrowFocus)
		sc->rightArrowFocus = UI_RIGHTARROWFOCUS;

	// Scale the center box
	sc->generic.x2 = sc->generic.x;
	sc->generic.y2 = sc->generic.y;
	sc->generic.width2 = sc->generic.width;
	sc->generic.height2 = sc->generic.height;
	UI_ScaleCoords(&sc->generic.x2, &sc->generic.y2, &sc->generic.width2, &sc->generic.height2);

	// Extend the width so it has room for the arrows
	sc->generic.width += (sc->generic.height * 3);

	// Calculate new X for the control
	sc->generic.x -= (sc->generic.height + (sc->generic.height/2));

	UI_ScaleCoords(&sc->generic.x, &sc->generic.y, &sc->generic.width, &sc->generic.height);
}

/*
 ==================
 
 ==================
*/
const char *UI_SpinControl_Key (menuSpinControl_t *sc, int key){

	const char	*sound = 0;
	int			arrowWidth, arrowHeight, leftX, leftY, rightX, rightY;

	switch (key){
	case K_MOUSE1:
	case K_MOUSE2:
		if (!(sc->generic.flags & QMF_HASMOUSEFOCUS))
			break;

		// Calculate size and position for the arrows
		arrowWidth = sc->generic.height;
		arrowHeight = sc->generic.height;

		leftX = sc->generic.x;
		leftY = sc->generic.y;
		rightX = sc->generic.x + (sc->generic.width - arrowWidth);
		rightY = sc->generic.y;

		// Now see if either left or right arrow has focus
		if (UI_CursorInRect(leftX, leftY, arrowWidth, arrowHeight)){
			if (sc->curValue > sc->minValue){
				sc->curValue -= sc->range;
				if (sc->curValue < sc->minValue)
					sc->curValue = sc->minValue;

				sound = uiSoundMove;
			}
			else
				sound = uiSoundBuzz;
		}
		else if (UI_CursorInRect(rightX, rightY, arrowWidth, arrowHeight)){
			if (sc->curValue < sc->maxValue){
				sc->curValue += sc->range;
				if (sc->curValue > sc->maxValue)
					sc->curValue = sc->maxValue;

				sound = uiSoundMove;
			}
			else
				sound = uiSoundBuzz;
		}

		break;
	case K_LEFTARROW:
	case K_KP_LEFTARROW:
		if (sc->generic.flags & QMF_MOUSEONLY)
			break;

		if (sc->curValue > sc->minValue){
			sc->curValue -= sc->range;
			if (sc->curValue < sc->minValue)
				sc->curValue = sc->minValue;

			sound = uiSoundMove;
		}
		else
			sound = uiSoundBuzz;

		break;
	case K_RIGHTARROW:
	case K_KP_RIGHTARROW:
		if (sc->generic.flags & QMF_MOUSEONLY)
			break;

		if (sc->curValue < sc->maxValue){
			sc->curValue += sc->range;
			if (sc->curValue > sc->maxValue)
				sc->curValue = sc->maxValue;

			sound = uiSoundMove;
		}
		else
			sound = uiSoundBuzz;

		break;
	}

	if (sound && (sc->generic.flags & QMF_SILENT))
		sound = uiSoundNull;

	if (sound && sc->generic.callback){
		if (sound != uiSoundBuzz)
			sc->generic.callback(sc, QM_CHANGED);
	}

	return sound;
}

/*
 ==================
 
 ==================
*/
void UI_SpinControl_Draw (menuSpinControl_t *sc){

	int		justify;
	bool	shadow;
	int		x, y, w, h;
	int		arrowWidth, arrowHeight, leftX, leftY, rightX, rightY;
	bool	leftFocus, rightFocus;
	
	if (sc->generic.flags & QMF_LEFT_JUSTIFY)
		justify = 0;
	else if (sc->generic.flags & QMF_CENTER_JUSTIFY)
		justify = 1;
	else if (sc->generic.flags & QMF_RIGHT_JUSTIFY)
		justify = 2;

	shadow = (sc->generic.flags & QMF_DROPSHADOW);

	// Calculate size and position for the arrows
	arrowWidth = sc->generic.height;
	arrowHeight = sc->generic.height;

	leftX = sc->generic.x;
	leftY = sc->generic.y;
	rightX = sc->generic.x + (sc->generic.width - arrowWidth);
	rightY = sc->generic.y;

	// Get size and position for the center box
	w = sc->generic.width2;
	h = sc->generic.height2;
	x = sc->generic.x2;
	y = sc->generic.y2;

	UI_DrawPicOLD(x, y, w, h, colorWhite, sc->background);

	if (sc->generic.flags & QMF_GRAYED){
		UI_DrawStringOLD(x, y, w, h, sc->generic.name, colorWhite, true, sc->generic.charWidth, sc->generic.charHeight, justify, shadow);
		UI_DrawPic(leftX, leftY, arrowWidth, arrowHeight, colorWhite, sc->leftArrow);
		UI_DrawPic(rightX, rightY, arrowWidth, arrowHeight, colorWhite, sc->rightArrow);
		return;		// Grayed
	}

	if ((menuCommon_t *)sc != (menuCommon_t *)UI_ItemAtCursor(sc->generic.parent)){
		UI_DrawStringOLD(x, y, w, h, sc->generic.name, sc->generic.color, false, sc->generic.charWidth, sc->generic.charHeight, justify, shadow);
		UI_DrawPic(leftX, leftY, arrowWidth, arrowHeight, sc->generic.color, sc->leftArrow);
		UI_DrawPic(rightX, rightY, arrowWidth, arrowHeight, sc->generic.color, sc->rightArrow);
		return;		// No focus
	}

	// See which arrow has the mouse focus
	leftFocus = UI_CursorInRect(leftX, leftY, arrowWidth, arrowHeight);
	rightFocus = UI_CursorInRect(rightX, rightY, arrowWidth, arrowHeight);

	if (!(sc->generic.flags & QMF_FOCUSBEHIND)){
		UI_DrawStringOLD(x, y, w, h, sc->generic.name, sc->generic.color, false, sc->generic.charWidth, sc->generic.charHeight, justify, shadow);
		UI_DrawPic(leftX, leftY, arrowWidth, arrowHeight, sc->generic.color, sc->leftArrow);
		UI_DrawPic(rightX, rightY, arrowWidth, arrowHeight, sc->generic.color, sc->rightArrow);
	}

	if (sc->generic.flags & QMF_HIGHLIGHTIFFOCUS){
		UI_DrawStringOLD(x, y, w, h, sc->generic.name, sc->generic.focusColor, false, sc->generic.charWidth, sc->generic.charHeight, justify, shadow);
		UI_DrawPic(leftX, leftY, arrowWidth, arrowHeight, (leftFocus) ? sc->generic.focusColor : sc->generic.color, (leftFocus) ? sc->leftArrowFocus : sc->leftArrow);
		UI_DrawPic(rightX, rightY, arrowWidth, arrowHeight, (rightFocus) ? sc->generic.focusColor : sc->generic.color, (rightFocus) ? sc->rightArrowFocus : sc->rightArrow);
	}
	else if (sc->generic.flags & QMF_PULSEIFFOCUS){
		vec4_t	color;

		VectorCopy(sc->generic.color, color);
		color[3] = 1.0f * (0.5f + 0.5f * sin(uiStatic.realTime / UI_PULSE_DIVISOR));

		UI_DrawStringOLD(x, y, w, h, sc->generic.name, color, false, sc->generic.charWidth, sc->generic.charHeight, justify, shadow);
		UI_DrawPic(leftX, leftY, arrowWidth, arrowHeight, (leftFocus) ? color : sc->generic.color, (leftFocus) ? sc->leftArrowFocus : sc->leftArrow);
		UI_DrawPic(rightX, rightY, arrowWidth, arrowHeight, (rightFocus) ? color : sc->generic.color, (rightFocus) ? sc->rightArrowFocus : sc->rightArrow);
	}
	else if (sc->generic.flags & QMF_BLINKIFFOCUS){
		if ((uiStatic.realTime & UI_BLINK_MASK) < UI_BLINK_TIME){
			UI_DrawStringOLD(x, y, w, h, sc->generic.name, sc->generic.focusColor, false, sc->generic.charWidth, sc->generic.charHeight, justify, shadow);
			UI_DrawPic(leftX, leftY, arrowWidth, arrowHeight, (leftFocus) ? sc->generic.focusColor : sc->generic.color, (leftFocus) ? sc->leftArrowFocus : sc->leftArrow);
			UI_DrawPic(rightX, rightY, arrowWidth, arrowHeight, (rightFocus) ? sc->generic.focusColor : sc->generic.color, (rightFocus) ? sc->rightArrowFocus : sc->rightArrow);
		}
	}

	if (sc->generic.flags & QMF_FOCUSBEHIND){
		UI_DrawStringOLD(x, y, w, h, sc->generic.name, sc->generic.color, false, sc->generic.charWidth, sc->generic.charHeight, justify, shadow);
		UI_DrawPic(leftX, leftY, arrowWidth, arrowHeight, sc->generic.color, sc->leftArrow);
		UI_DrawPic(rightX, rightY, arrowWidth, arrowHeight, sc->generic.color, sc->rightArrow);
	}
}


/*
 ==============================================================================

 FIELD

 ==============================================================================
*/


/*
 ==================
 UI_Field_Init
 ==================
*/
void UI_Field_Init (menuField_t *f){

	if (!f->generic.name)
		f->generic.name = "";

	if (f->generic.flags & QMF_BIGFONT){
		f->generic.charWidth = UI_BIG_CHAR_WIDTH;
		f->generic.charHeight = UI_BIG_CHAR_HEIGHT;
	}
	else if (f->generic.flags & QMF_SMALLFONT){
		f->generic.charWidth = UI_SMALL_CHAR_WIDTH;
		f->generic.charHeight = UI_SMALL_CHAR_HEIGHT;
	}
	else {
		if (f->generic.charWidth < 1)
			f->generic.charWidth = UI_SMALL_CHAR_WIDTH;
		if (f->generic.charHeight < 1)
			f->generic.charHeight = UI_SMALL_CHAR_HEIGHT;
	}

	UI_ScaleCoords(NULL, NULL, &f->generic.charWidth, &f->generic.charHeight);

	if (!(f->generic.flags & (QMF_LEFT_JUSTIFY | QMF_CENTER_JUSTIFY | QMF_RIGHT_JUSTIFY)))
		f->generic.flags |= QMF_LEFT_JUSTIFY;

	if (!f->generic.color)
		f->generic.color = colorWhite;
	if (!f->generic.focusColor)
		f->generic.focusColor = uiColorLightGray;

	if (!f->background)
		f->background = UI_BACKGROUNDBOX;

	f->maxLenght++;
	if (f->maxLenght <= 1 || f->maxLenght >= UI_MAX_FIELD_LINE)
		f->maxLenght = UI_MAX_FIELD_LINE - 1;

	UI_ScaleCoords(&f->generic.x, &f->generic.y, &f->generic.width, &f->generic.height);

	// Calculate number of visible characters
	f->visibleLength = (f->generic.width / f->generic.charWidth) - 2;

	f->length = Str_Length(f->buffer);
	f->cursor = f->length;
}

/*
 ==================
 
 ==================
*/
const char *UI_Field_Key (menuField_t *f, int key){
	
	return 0;
}

/*
 ==================
 
 ==================
*/
void UI_Field_Draw (menuField_t *f){

	int		justify;
	bool	shadow;
	char	*text = f->buffer;
	int		scroll = 0, width = f->visibleLength - 2;
	int		cursor, x;

	if (f->generic.flags & QMF_LEFT_JUSTIFY)
		justify = 0;
	else if (f->generic.flags & QMF_CENTER_JUSTIFY)
		justify = 1;
	else if (f->generic.flags & QMF_RIGHT_JUSTIFY)
		justify = 2;

	shadow = (f->generic.flags & QMF_DROPSHADOW);

	// Prestep if horizontally scrolling
	while (Q_PrintStrlen(text+scroll) + 2 > f->visibleLength)
		scroll++;
	
	text += scroll;

	// Find cursor position
	if (scroll)
		cursor = width - (Q_PrintStrlen(f->buffer) - f->cursor);
	else
		cursor = f->cursor;

	cursor -= (Str_Length(f->buffer) - Q_PrintStrlen(f->buffer));
	if (cursor > width)
		cursor = width;
	if (cursor < 0)
		cursor = 0;

	if (justify == 0)
		x = f->generic.x;
	else if (justify == 1)
		x = f->generic.x + ((f->generic.width - (Q_PrintStrlen(text) * f->generic.charWidth)) / 2);
	else if (justify == 2)
		x = f->generic.x + (f->generic.width - (Q_PrintStrlen(text) * f->generic.charWidth));

	UI_DrawPicOLD(f->generic.x, f->generic.y, f->generic.width, f->generic.height, colorWhite, f->background);

	if (f->generic.flags & QMF_GRAYED){
		UI_DrawStringOLD(f->generic.x, f->generic.y, f->generic.width, f->generic.height, text, colorWhite, true, f->generic.charWidth, f->generic.charHeight, justify, shadow);
		return;		// Grayed
	}

	if ((menuCommon_t *)f != (menuCommon_t *)UI_ItemAtCursor(f->generic.parent)){
		UI_DrawStringOLD(f->generic.x, f->generic.y, f->generic.width, f->generic.height, text, f->generic.color, false, f->generic.charWidth, f->generic.charHeight, justify, shadow);
		return;		// No focus
	}

	if (!(f->generic.flags & QMF_FOCUSBEHIND)){
		UI_DrawStringOLD(f->generic.x, f->generic.y, f->generic.width, f->generic.height, text, f->generic.color, false, f->generic.charWidth, f->generic.charHeight, justify, shadow);

		if ((uiStatic.realTime & 499) < 250)
			UI_DrawStringOLD(x + (cursor*f->generic.charWidth), f->generic.y, f->generic.charWidth, f->generic.height, "_", f->generic.color, true, f->generic.charWidth, f->generic.charHeight, 0, shadow);
	}

	if (f->generic.flags & QMF_HIGHLIGHTIFFOCUS){
		UI_DrawStringOLD(f->generic.x, f->generic.y, f->generic.width, f->generic.height, text, f->generic.focusColor, false, f->generic.charWidth, f->generic.charHeight, justify, shadow);

		if ((uiStatic.realTime & 499) < 250)
			UI_DrawStringOLD(x + (cursor*f->generic.charWidth), f->generic.y, f->generic.charWidth, f->generic.height, "_", f->generic.focusColor, true, f->generic.charWidth, f->generic.charHeight, 0, shadow);
	}
	else if (f->generic.flags & QMF_PULSEIFFOCUS){
		color_t	color;

		*(unsigned *)color = *(unsigned *)f->generic.color;
		color[3] = 255 * (0.5f + 0.5f * sin(uiStatic.realTime / UI_PULSE_DIVISOR));

		UI_DrawStringOLD(f->generic.x, f->generic.y, f->generic.width, f->generic.height, text, color, false, f->generic.charWidth, f->generic.charHeight, justify, shadow);

		if ((uiStatic.realTime & 499) < 250)
			UI_DrawStringOLD(x + (cursor*f->generic.charWidth), f->generic.y, f->generic.charWidth, f->generic.height, "_", color, true, f->generic.charWidth, f->generic.charHeight, 0, shadow);
	}
	else if (f->generic.flags & QMF_BLINKIFFOCUS){
		if ((uiStatic.realTime & UI_BLINK_MASK) < UI_BLINK_TIME){
			UI_DrawStringOLD(f->generic.x, f->generic.y, f->generic.width, f->generic.height, text, f->generic.focusColor, false, f->generic.charWidth, f->generic.charHeight, justify, shadow);

			if ((uiStatic.realTime & 499) < 250)
				UI_DrawStringOLD(x + (cursor*f->generic.charWidth), f->generic.y, f->generic.charWidth, f->generic.height, "_", f->generic.focusColor, true, f->generic.charWidth, f->generic.charHeight, 0, shadow);
		}
	}

	if (f->generic.flags & QMF_FOCUSBEHIND){
		UI_DrawStringOLD(f->generic.x, f->generic.y, f->generic.width, f->generic.height, text, f->generic.color, false, f->generic.charWidth, f->generic.charHeight, justify, shadow);

		if ((uiStatic.realTime & 499) < 250)
			UI_DrawStringOLD(x + (cursor*f->generic.charWidth), f->generic.y, f->generic.charWidth, f->generic.height, "_", f->generic.color, true, f->generic.charWidth, f->generic.charHeight, 0, shadow);
	}
}


/*
 ==============================================================================

 ACTION

 ==============================================================================
*/


/*
 ==================
 UI_Action_Init
 ==================
*/
void UI_Action_Init (menuAction_t *a){

	if (!a->generic.name)	// This is also the text displayed
		a->generic.name = "";

	if (a->generic.flags & QMF_BIGFONT){
		a->generic.charWidth = UI_BIG_CHAR_WIDTH;
		a->generic.charHeight = UI_BIG_CHAR_HEIGHT;
	}
	else if (a->generic.flags & QMF_SMALLFONT){
		a->generic.charWidth = UI_SMALL_CHAR_WIDTH;
		a->generic.charHeight = UI_SMALL_CHAR_HEIGHT;
	}
	else {
		if (a->generic.charWidth < 1)
			a->generic.charWidth = UI_SMALL_CHAR_WIDTH;
		if (a->generic.charHeight < 1)
			a->generic.charHeight = UI_SMALL_CHAR_HEIGHT;
	}

	UI_ScaleCoords(NULL, NULL, &a->generic.charWidth, &a->generic.charHeight);

	if (!(a->generic.flags & (QMF_LEFT_JUSTIFY | QMF_CENTER_JUSTIFY | QMF_RIGHT_JUSTIFY)))
		a->generic.flags |= QMF_LEFT_JUSTIFY;

	if (!a->generic.color)
		a->generic.color = colorWhite;
	if (!a->generic.focusColor)
		a->generic.focusColor = uiColorLightGray;

	if (!a->background)
		a->background = UI_BACKGROUNDBOX;

	UI_ScaleCoords(&a->generic.x,	&a->generic.y, &a->generic.width, &a->generic.height);
}

/*
 ==================
 UI_Action_Key
 ==================
*/
const char *UI_Action_Key (menuAction_t *a, int key){

	const char	*sound = 0;

	switch (key){
	case K_MOUSE1:
		if (!(a->generic.flags & QMF_HASMOUSEFOCUS))
			break;

		sound = uiSoundMove;

		break;
	case K_ENTER:
	case K_KP_ENTER:
		if (a->generic.flags & QMF_MOUSEONLY)
			break;

		sound = uiSoundMove;

		break;
	}

	if (sound && (a->generic.flags & QMF_SILENT))
		sound = uiSoundNull;

	if (sound && a->generic.callback)
		a->generic.callback(a, QM_ACTIVATED);

	return sound;
}

/*
 ==================
 
 ==================
*/
void UI_Action_Draw (menuAction_t *a){

	vec4_t	color;
	int		justify;
	bool	shadow;

	if (a->generic.flags & QMF_LEFT_JUSTIFY)
		justify = 0;
	else if (a->generic.flags & QMF_CENTER_JUSTIFY)
		justify = 1;
	else if (a->generic.flags & QMF_RIGHT_JUSTIFY)
		justify = 2;

	shadow = (a->generic.flags & QMF_DROPSHADOW);

	UI_DrawPic(a->generic.x, a->generic.y, a->generic.width, a->generic.height, colorWhite, a->background);

	if (a->generic.flags & QMF_GRAYED){
		UI_DrawStringOLD(a->generic.x, a->generic.y, a->generic.width, a->generic.height, a->generic.name, colorWhite, true, a->generic.charWidth, a->generic.charHeight, justify, shadow);
		return;		// Grayed
	}

	if ((menuCommon_t *)a != (menuCommon_t *)UI_ItemAtCursor(a->generic.parent)){
		UI_DrawStringOLD(a->generic.x, a->generic.y, a->generic.width, a->generic.height, a->generic.name, a->generic.color, false, a->generic.charWidth, a->generic.charHeight, justify, shadow);
		return;		// No focus
	}

	if (!(a->generic.flags & QMF_FOCUSBEHIND))
		UI_DrawStringOLD(a->generic.x, a->generic.y, a->generic.width, a->generic.height, a->generic.name, a->generic.color, false, a->generic.charWidth, a->generic.charHeight, justify, shadow);

	if (a->generic.flags & QMF_HIGHLIGHTIFFOCUS)
		UI_DrawStringOLD(a->generic.x, a->generic.y, a->generic.width, a->generic.height, a->generic.name, a->generic.focusColor, false, a->generic.charWidth, a->generic.charHeight, justify, shadow);
	else if (a->generic.flags & QMF_PULSEIFFOCUS){
		VectorCopy(a->generic.color, color);
		color[3] = 1.0f * (0.5f + 0.5f * sin(uiStatic.realTime / UI_PULSE_DIVISOR));

		UI_DrawStringOLD(a->generic.x, a->generic.y, a->generic.width, a->generic.height, a->generic.name, color, false, a->generic.charWidth, a->generic.charHeight, justify, shadow);
	}
	else if (a->generic.flags & QMF_BLINKIFFOCUS){
		if ((uiStatic.realTime & UI_BLINK_MASK) < UI_BLINK_TIME)
			UI_DrawStringOLD(a->generic.x, a->generic.y, a->generic.width, a->generic.height, a->generic.name, a->generic.focusColor, false, a->generic.charWidth, a->generic.charHeight, justify, shadow);
	}

	if (a->generic.flags & QMF_FOCUSBEHIND)
		UI_DrawStringOLD(a->generic.x, a->generic.y, a->generic.width, a->generic.height, a->generic.name, a->generic.color, false, a->generic.charWidth, a->generic.charHeight, justify, shadow);
}


/*
 ==============================================================================

 BITMAP

 ==============================================================================
*/


/*
 ==================
 UI_Bitmap_Init
 ==================
*/
void UI_Bitmap_Init (menuBitmap_t *b){

	if (!b->generic.name)
		b->generic.name = "";

	if (!b->focusPic)
		b->focusPic = b->pic;

	if (!b->generic.color)
		b->generic.color = colorWhite;
	if (!b->generic.focusColor)
		b->generic.focusColor = uiColorLightGray;

	UI_ScaleCoords(&b->generic.x, &b->generic.y, &b->generic.width, &b->generic.height);
}

/*
 ==================
 UI_Bitmap_Key
 ==================
*/
const char *UI_Bitmap_Key (menuBitmap_t *b, int key){

	const char	*sound = 0;

	switch (key){
	case K_MOUSE1:
		if (!(b->generic.flags & QMF_HASMOUSEFOCUS))
			break;

		sound = uiSoundMove;

		break;
	case K_ENTER:
	case K_KP_ENTER:
		if (b->generic.flags & QMF_MOUSEONLY)
			break;

		sound = uiSoundMove;

		break;
	}

	if (sound && (b->generic.flags & QMF_SILENT))
		sound = uiSoundNull;

	if (sound && b->generic.callback)
		b->generic.callback(b, QM_ACTIVATED);

	return sound;
}

/*
 ==================
 UI_Bitmap_Draw
 ==================
*/
void UI_Bitmap_Draw (menuBitmap_t *b){

	vec4_t color;

	if (b->generic.flags & QMF_GRAYED){
		UI_DrawPic(b->generic.x, b->generic.y, b->generic.width, b->generic.height, uiColorDarkGray, b->pic);
		return;		// Grayed
	}

	if ((menuCommon_t *)b != (menuCommon_t *)UI_ItemAtCursor(b->generic.parent)){
		UI_DrawPic(b->generic.x, b->generic.y, b->generic.width, b->generic.height, b->generic.color, b->pic);
		return;		// No focus
	}

	if (!(b->generic.flags & QMF_FOCUSBEHIND))
		UI_DrawPic(b->generic.x, b->generic.y, b->generic.width, b->generic.height, b->generic.color, b->pic);

	if (b->generic.flags & QMF_HIGHLIGHTIFFOCUS)
		UI_DrawPic(b->generic.x, b->generic.y, b->generic.width, b->generic.height, b->generic.focusColor, b->focusPic);
	else if (b->generic.flags & QMF_PULSEIFFOCUS){
		VectorCopy(b->generic.color, color);
		color[3] = 1.0f * (0.5f + 0.5f * sin(uiStatic.realTime / UI_PULSE_DIVISOR));

		UI_DrawPic(b->generic.x, b->generic.y, b->generic.width, b->generic.height, color, b->focusPic);
	}
	else if (b->generic.flags & QMF_BLINKIFFOCUS){
		if ((uiStatic.realTime & UI_BLINK_MASK) < UI_BLINK_TIME)
			UI_DrawPic(b->generic.x, b->generic.y, b->generic.width, b->generic.height, b->generic.focusColor, b->focusPic);
	}

	if (b->generic.flags & QMF_FOCUSBEHIND)
		UI_DrawPic(b->generic.x, b->generic.y, b->generic.width, b->generic.height, b->generic.color, b->pic);
}