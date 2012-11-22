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
// ui_credits.c - Credits menu
//


#include "ui_local.h"


#define ART_BACKGROUND				"ui/assets/ui/ui_sub_quit"

static const char *			uiCreditsId[] = {
	"+QUAKE II EVOLVED BY TEAM BLUR",
	"www.team-blur-games.com",
	"",
	"+PROGRAMMING",
	"Nicolas \"^4BE^3RSE^4RK^7\" Flekenstein",
	"Kevin \"Annatar\" Jonasson",
	"",
	"+ART",
	"Gavin \"o'dium\" Stevens",
	"Main menu background by \"snak9\"",
	"Various glow skins by KMan",
	"",
	"+MODELS",
	"Gavin \"o'dium\" Stevens",
	"Karmacop",
	"",
	"+BETA TESTING",
	"BLooDMaN, Federelli, Karmacop, Quaked Out",
	"",
	"",
	"",
	"+SPECIAL THANKS",
	"id Software for making Quake II and",
	"releasing its source code",
	"",
	"Thanks to the id Legends area of the",
	"Quake 3 World forums for their support",
	"during the production of Quake II Evolved",
	"www.quake3world.com",
	"",
	"The \"Team Blur forums\" moderators",
	"Bitterman, Federelli, Karmacop, Quaked Out",
	"",
	"All the people contributing to the",
	"retexturing and remodeling projects",
	"",
	"+CODE",
	"Vic, Tr3B, mSparks, MrG, Psychospaz, Jitspoe,",
	"Knightmare, LordHavoc, Heffo, Discoloda,",
	"Stephen Taylor, Martin Kraus, Tim Ferguson",
	"",
	"Portions of Quake II Evolved based in part",
	"on the work of the Independent JPEG Group",
	"",
	"Contains ZLib software",
	"",
	"OggVorbis software by the Xiph.org Foundation",
	"",
	"",
	"Quake II Evolved is a Team Blur",
	"production. Distributed under the terms",
	"of the GNU General Public License.",
	"Quake II (C)1997-2004 Id Software, Inc.",
	"",
	"",
	"",
	"",
	"",
	"",
	"+QUAKE II BY ID SOFTWARE",
	"",
	"+PROGRAMMING",
	"John Carmack",
	"John Cash",
	"Brian Hook",
	"",
	"+ART",
	"Adrian Carmack",
	"Kevin Cloud",
	"Paul Steed",
	"",
	"+LEVEL DESIGN",
	"Tim Willits",
	"American McGee",
	"Christian Antkow",
	"Paul Jaquays",
	"Brandon James",
	"",
	"+BIZ",
	"Todd Hollenshead",
	"Barrett (Bear) Alexander",
	"Donna Jackson",
	"",
	"",
	"+SPECIAL THANKS",
	"Ben Donges for beta testing",
	"",
	"",
	"",
	"",
	"",
	"",
	"+ADDITIONAL SUPPORT",
	"",
	"+LINUX PORT AND CTF",
	"Dave \"Zoid\" Kirsch",
	"",
	"+CINEMATIC SEQUENCES",
	"Ending Cinematic by Blur Studio - ",
	"Venice, CA",
	"",
	"Environment models for Introduction",
	"Cinematic by Karl Dolgener",
	"",
	"Assistance with environment design",
	"by Cliff Iwai",
	"",
	"+SOUND EFFECTS AND MUSIC",
	"Sound Design by Soundelux Media Labs.",
	"Music Composed and Produced by",
	"Soundelux Media Labs.  Special thanks",
	"to Bill Brown, Tom Ozanich, Brian",
	"Celano, Jeff Eisner, and The Soundelux",
	"Players.",
	"",
	"\"Level Music\" by Sonic Mayhem",
	"www.sonicmayhem.com",
	"",
	"\"Quake II Theme Song\"",
	"(C) 1997 Rob Zombie. All Rights",
	"Reserved.",
	"",
	"Track 10 (\"Climb\") by Jer Sypult",
	"",
	"Voice of computers by",
	"Carly Staehlin-Taylor",
	"",
	"+THANKS TO ACTIVISION",
	"+IN PARTICULAR:",
	"",
	"John Tam",
	"Steve Rosenthal",
	"Marty Stratton",
	"Henk Hartong",
	"",
	"Quake II(tm) (C)1997 Id Software, Inc.",
	"All Rights Reserved.  Distributed by",
	"Activision, Inc. under license.",
	"Quake II(tm), the Id Software name,",
	"the \"Q II\"(tm) logo and id(tm)",
	"logo are trademarks of Id Software,",
	"Inc. Activision(R) is a registered",
	"trademark of Activision, Inc. All",
	"other trademarks and trade names are",
	"properties of their respective owners.",
	0
};

static const char *			uiCreditsXatrix[] = {
	"+QUAKE II EVOLVED BY TEAM BLUR",
	"www.team-blur-games.com",
	"",
	"+PROGRAMMING",
	"Nicolas \"^4BE^3RSE^4RK^7\" Flekenstein",
	"Kevin \"Annatar\" Jonasson",
	"",
	"+ART",
	"Gavin \"o'dium\" Stevens",
	"Main menu background by \"snak9\"",
	"Various glow skins by KMan",
	"",
	"+MODELS",
	"Gavin \"o'dium\" Stevens",
	"Karmacop",
	"",
	"+BETA TESTING",
	"BLooDMaN, Federelli, Karmacop, Quaked Out",
	"",
	"",
	"",
	"+SPECIAL THANKS",
	"id Software for making Quake II and",
	"releasing its source code",
	"",
	"Thanks to the id Legends area of the",
	"Quake 3 World forums for their support",
	"during the production of Quake II Evolved",
	"www.quake3world.com",
	"",
	"The \"Team Blur forums\" moderators",
	"Bitterman, Federelli, Karmacop, Quaked Out",
	"",
	"All the people contributing to the",
	"retexturing and remodeling projects",
	"",
	"+CODE",
	"Vic, Tr3B, mSparks, MrG, Psychospaz, Jitspoe,",
	"Knightmare, LordHavoc, Heffo, Discoloda,",
	"Stephen Taylor, Martin Kraus, Tim Ferguson",
	"",
	"Portions of Quake II Evolved based in part",
	"on the work of the Independent JPEG Group",
	"",
	"Contains ZLib software",
	"",
	"OggVorbis software by the Xiph.org Foundation",
	"",
	"",
	"Quake II Evolved is a Team Blur",
	"production. Distributed under the terms",
	"of the GNU General Public License.",
	"Quake II (C)1997-2004 Id Software, Inc.",
	"",
	"",
	"",
	"",
	"",
	"",
	"+QUAKE II MISSION PACK: THE RECKONING",
	"+BY",
	"+XATRIX ENTERTAINMENT, INC.",
	"",
	"+DESIGN AND DIRECTION",
	"Drew Markham",
	"",
	"+PRODUCED BY",
	"Greg Goodrich",
	"",
	"+PROGRAMMING",
	"Rafael Paiz",
	"",
	"+LEVEL DESIGN / ADDITIONAL GAME DESIGN",
	"Alex Mayberry",
	"",
	"+LEVEL DESIGN",
	"Mal Blackwell",
	"Dan Koppel",
	"",
	"+ART DIRECTION",
	"Michael \"Maxx\" Kaufman",
	"",
	"+COMPUTER GRAPHICS SUPERVISOR AND",
	"+CHARACTER ANIMATION DIRECTION",
	"Barry Dempsey",
	"",
	"+SENIOR ANIMATOR AND MODELER",
	"Jason Hoover",
	"",
	"+CHARACTER ANIMATION AND",
	"+MOTION CAPTURE SPECIALIST",
	"Amit Doron",
	"",
	"+ART",
	"Claire Praderie-Markham",
	"Viktor Antonov",
	"Corky Lehmkuhl",
	"",
	"+INTRODUCTION ANIMATION",
	"Dominique Drozdz",
	"",
	"+ADDITIONAL LEVEL DESIGN",
	"Aaron Barber",
	"Rhett Baldwin",
	"",
	"+3D CHARACTER ANIMATION TOOLS",
	"Gerry Tyra, SA Technology",
	"",
	"+ADDITIONAL EDITOR TOOL PROGRAMMING",
	"Robert Duffy",
	"",
	"+ADDITIONAL PROGRAMMING",
	"Ryan Feltrin",
	"",
	"+PRODUCTION COORDINATOR",
	"Victoria Sylvester",
	"",
	"+SOUND DESIGN",
	"Gary Bradfield",
	"",
	"+MUSIC BY",
	"Sonic Mayhem",
	"",
	"",
	"",
	"+SPECIAL THANKS",
	"+TO",
	"+OUR FRIENDS AT ID SOFTWARE",
	"",
	"John Carmack",
	"John Cash",
	"Brian Hook",
	"Adrian Carmack",
	"Kevin Cloud",
	"Paul Steed",
	"Tim Willits",
	"Christian Antkow",
	"Paul Jaquays",
	"Brandon James",
	"Todd Hollenshead",
	"Barrett (Bear) Alexander",
	"Dave \"Zoid\" Kirsch",
	"Donna Jackson",
	"",
	"",
	"",
	"+THANKS TO ACTIVISION",
	"+IN PARTICULAR:",
	"",
	"Marty Stratton",
	"Henk \"The Original Ripper\" Hartong",
	"Kevin Kraff",
	"Jamey Gottlieb",
	"Chris Hepburn",
	"",
	"+AND THE GAME TESTERS",
	"",
	"Tim Vanlaw",
	"Doug Jacobs",
	"Steven Rosenthal",
	"David Baker",
	"Chris Campbell",
	"Aaron Casillas",
	"Steve Elwell",
	"Derek Johnstone",
	"Igor Krinitskiy",
	"Samantha Lee",
	"Michael Spann",
	"Chris Toft",
	"Juan Valdes",
	"",
	"+THANKS TO INTERGRAPH COMPUTER SYTEMS",
	"+IN PARTICULAR:",
	"",
	"Michael T. Nicolaou",
	"",
	"",
	"Quake II Mission Pack: The Reckoning",
	"(tm) (C)1998 Id Software, Inc. All",
	"Rights Reserved. Developed by Xatrix",
	"Entertainment, Inc. for Id Software,",
	"Inc. Distributed by Activision Inc.",
	"under license. Quake(R) is a",
	"registered trademark of Id Software,",
	"Inc. Quake II Mission Pack: The",
	"Reckoning(tm), Quake II(tm), the Id",
	"Software name, the \"Q II\"(tm) logo",
	"and id(tm) logo are trademarks of Id",
	"Software, Inc. Activision(R) is a",
	"registered trademark of Activision,",
	"Inc. Xatrix(R) is a registered",
	"trademark of Xatrix Entertainment,",
	"Inc. All other trademarks and trade",
	"names are properties of their",
	"respective owners.",
	0
};

static const char *			uiCreditsRogue[] = {
	"+QUAKE II EVOLVED BY TEAM BLUR",
	"www.team-blur-games.com",
	"",
	"+PROGRAMMING",
	"Nicolas \"^4BE^3RSE^4RK^7\" Flekenstein",
	"Kevin \"Annatar\" Jonasson",
	"",
	"+ART",
	"Gavin \"o'dium\" Stevens",
	"Main menu background by \"snak9\"",
	"Various glow skins by KMan",
	"",
	"+MODELS",
	"Gavin \"o'dium\" Stevens",
	"Karmacop",
	"",
	"+BETA TESTING",
	"BLooDMaN, Federelli, Karmacop, Quaked Out",
	"",
	"",
	"",
	"+SPECIAL THANKS",
	"id Software for making Quake II and",
	"releasing its source code",
	"",
	"Thanks to the id Legends area of the",
	"Quake 3 World forums for their support",
	"during the production of Quake II Evolved",
	"www.quake3world.com",
	"",
	"The \"Team Blur forums\" moderators",
	"Bitterman, Federelli, Karmacop, Quaked Out",
	"",
	"All the people contributing to the",
	"retexturing and remodeling projects",
	"",
	"+CODE",
	"Vic, Tr3B, mSparks, MrG, Psychospaz, Jitspoe,",
	"Knightmare, LordHavoc, Heffo, Discoloda,",
	"Stephen Taylor, Martin Kraus, Tim Ferguson",
	"",
	"Portions of Quake II Evolved based in part",
	"on the work of the Independent JPEG Group",
	"",
	"Contains ZLib software",
	"",
	"OggVorbis software by the Xiph.org Foundation",
	"",
	"",
	"Quake II Evolved is a Team Blur",
	"production. Distributed under the terms",
	"of the GNU General Public License.",
	"Quake II (C)1997-2004 Id Software, Inc.",
	"",
	"",
	"",
	"",
	"",
	"",
	"+QUAKE II MISSION PACK 2: GROUND ZERO",
	"+BY",
	"+ROGUE ENTERTAINMENT, INC.",
	"",
	"+PRODUCED BY",
	"Jim Molinets",
	"",
	"+PROGRAMMING",
	"Peter Mack",
	"Patrick Magruder",
	"",
	"+LEVEL DESIGN",
	"Jim Molinets",
	"Cameron Lamprecht",
	"Berenger Fish",
	"Robert Selitto",
	"Steve Tietze",
	"Steve Thoms",
	"",
	"+ART DIRECTION",
	"Rich Fleider",
	"",
	"+ART",
	"Rich Fleider",
	"Steve Maines",
	"Won Choi",
	"",
	"+ANIMATION SEQUENCES",
	"Creat Studios",
	"Steve Maines",
	"",
	"+ADDITIONAL LEVEL DESIGN",
	"Rich Fleider",
	"Steve Maines",
	"Peter Mack",
	"",
	"+SOUND",
	"James Grunke",
	"",
	"+GROUND ZERO THEME",
	"+AND",
	"+MUSIC BY",
	"Sonic Mayhem",
	"",
	"+VWEP MODELS",
	"Brent \"Hentai\" Dill",
	"",
	"",
	"",
	"+SPECIAL THANKS",
	"+TO",
	"+OUR FRIENDS AT ID SOFTWARE",
	"",
	"John Carmack",
	"John Cash",
	"Brian Hook",
	"Adrian Carmack",
	"Kevin Cloud",
	"Paul Steed",
	"Tim Willits",
	"Christian Antkow",
	"Paul Jaquays",
	"Brandon James",
	"Todd Hollenshead",
	"Barrett (Bear) Alexander",
	"Katherine Anna Kang",
	"Donna Jackson",
	"Dave \"Zoid\" Kirsch",
	"",
	"",
	"",
	"+THANKS TO ACTIVISION",
	"+IN PARTICULAR:",
	"",
	"Marty Stratton",
	"Henk Hartong",
	"Mitch Lasky",
	"Steve Rosenthal",
	"Steve Elwell",
	"",
	"+AND THE GAME TESTERS",
	"",
	"The Ranger Clan",
	"Dave \"Zoid\" Kirsch",
	"Nihilistic Software",
	"Robert Duffy",
	"",
	"And Countless Others",
	"",
	"",
	"",
	"Quake II Mission Pack 2: Ground Zero",
	"(tm) (C)1998 Id Software, Inc. All",
	"Rights Reserved. Developed by Rogue",
	"Entertainment, Inc. for Id Software,",
	"Inc. Distributed by Activision Inc.",
	"under license. Quake(R) is a",
	"registered trademark of Id Software,",
	"Inc. Quake II Mission Pack 2: Ground",
	"Zero(tm), Quake II(tm), the Id",
	"Software name, the \"Q II\"(tm) logo",
	"and id(tm) logo are trademarks of Id",
	"Software, Inc. Activision(R) is a",
	"registered trademark of Activision,",
	"Inc. Rogue(R) is a registered",
	"trademark of Rogue Entertainment,",
	"Inc. All other trademarks and trade",
	"names are properties of their",
	"respective owners.",
	0
};

typedef struct {
	const char **			credits;
	int						startTime;

	menuFramework_t			menu;
} uiCredits_t;

static uiCredits_t			uiCredits;


/*
 ==================
 
 TODO: string rewrite
 ==================
*/
static void UI_Credits_DrawFunc (){

//	vec4_t		color = {0, 76, 127, 255};
	const char	*string;
	int			w = 8, h = 16;
	int			i, y;

	// Draw the background first
	UI_DrawPic(0.0f, 0.0f, 1024.0f*uiStatic.scaleX, 768.0f*uiStatic.scaleY, colorWhite, ART_BACKGROUND);

	// Now draw the credits
	UI_ScaleCoords(NULL, NULL, &w, &h);

	for (i = 0, y = uiStatic.glConfig.videoHeight - ((uiStatic.realTime - uiCredits.startTime) / 40.0); uiCredits.credits[i] != 0 && y < uiStatic.glConfig.videoHeight; i++, y += h){
		if (y <= -h)
			continue;

		string = uiCredits.credits[i];
		if (string[0] == '+')
			UI_DrawString(0, y, 1024*uiStatic.scaleX, h, string+1, colorRed, false, w, h, 1, true);
		else
			UI_DrawString(0, y, 1024*uiStatic.scaleX, h, string, colorWhite, false, w, h, 1, true);
	}

	if (y < 0)
		uiCredits.startTime = uiStatic.realTime;
}

/*
 ==================
 UI_Credits_KeyFunc
 ==================
*/
static const char *UI_Credits_KeyFunc (int key){

	Cmd_ExecuteText(CMD_EXEC_APPEND, "quit\n");

	return uiSoundNull;
}

/*
 ==================
 UI_Credits_Init
 ==================
*/
static void UI_Credits_Init (){

	const char	*game = CVar_GetVariableString("fs_game");

	Mem_Fill(&uiCredits, 0, sizeof(uiCredits_t));

	uiCredits.menu.drawFunc						= UI_Credits_DrawFunc;
	uiCredits.menu.keyFunc						= UI_Credits_KeyFunc;

	if (!Str_ICompare(game, "xatrix"))
		uiCredits.credits = uiCreditsXatrix;
	else if (!Str_ICompare(game, "rogue"))
		uiCredits.credits = uiCreditsRogue;
	else
		uiCredits.credits = uiCreditsId;

	uiCredits.startTime = uiStatic.realTime;
}

/*
 ==================
 UI_Credits_Precache
 ==================
*/
void UI_Credits_Precache (){

	R_RegisterMaterialNoMip(ART_BACKGROUND);
}

/*
 ==================
 UI_Credits_Menu
 ==================
*/
void UI_Credits_Menu (){

	UI_Credits_Precache();
	UI_Credits_Init();

	UI_PushMenu(&uiCredits.menu);
}