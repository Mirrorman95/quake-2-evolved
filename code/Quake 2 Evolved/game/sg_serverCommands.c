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
// sg_svcmds.c - Server interaction commands
//


#include "g_local.h"


/*
 ==============================================================================

 IP FILTERING

 You can add or remove addresses from the filter list with:

 addIP <ip>
 removeIP <ip>

 The IP address is specified in dot format, and you can use a filter to match
 any value so you can specify an entire class C network with, for example,
 "addIP 192.246.40.*".

 The removeIP command will only remove an IP address specified exactly the same
 way. You cannot addIP a subnet, then removeIP a single host.

 The listIP command prints the current list of filters.

 The writeIP command Dumps "addip <ip>" commands to listip.cfg so it can be execed at a later date,
 the filter lists are not saved and restored by default, because I beleive it would cause too much confusion.

 If sg_filterBan is enabled then IP addresses matching the current list will be
 prohibited from entering the game.

 ==============================================================================
*/

#define	MAX_IP_FILTERS	1024

typedef struct {
	unsigned			mask;
	unsigned			compare;
} ipFilter_t;

ipFilter_t				sg_ipFilters[MAX_IP_FILTERS];
int						sg_numIpFilters;


/*
 ==================
 SG_StringToFilter
 ==================
*/
static qboolean SG_StringToFilter (const char *string, ipFilter_t *filter){

	int		i, j;
	byte	b[4];
	byte	m[4];
	char	num[128];

	for (i = 0; i < 4; i++){
		b[i] = 0;
		m[i] = 0;
	}

	for (i = 0; i < 4; i++){
		if (*string < '0' || *string > '9'){
			gi.cprintf(NULL, PRINT_HIGH, "Bad filter address: %s\n", string);
			return false;
		}

		j = 0;

		while (*string >= '0' && *string <= '9')
			num[j++] = *string++;

		num[j] = 0;

		b[i] = atoi(num);

		if (b[i] != 0)
			m[i] = 255;

		if (!*string)
			break;

		string++;
	}

	filter->mask = *(unsigned *)m;
	filter->compare = *(unsigned *)b;

	return true;
}

/*
 ==================
 SG_FilterPacket
 ==================
*/
qboolean SG_FilterPacket (const char *from){

	int			i;
	const char	*p;
	unsigned	in;
	byte		m[4];

	i = 0;
	p = from;

	while (*p && i < 4){
		m[i] = 0;

		while (*p >= '0' && *p <= '9'){
			m[i] = m[i] * 10 + (*p - '0');
			p++;
		}

		if (!*p || *p == ':')
			break;

		i++, p++;
	}

	in = *(unsigned *)m;

	for (i = 0; i < sg_numIpFilters; i++){
		if ((in & sg_ipFilters[i].mask) == sg_ipFilters[i].compare)
			return (int)filterban->value;
	}

	return (int)!filterban->value;
}

/*
 ==================
 SG_Test_f
 ==================
*/
static void SG_Test_f (){

	gi.cprintf(NULL, PRINT_HIGH, "SG_Test_f()\n");
}

/*
 ==================
 SG_AddIP_f
 ==================
*/
static void SG_AddIP_f (){

	int		i;

	if (gi.argc() < 3){
		gi.cprintf(NULL, PRINT_HIGH, "Usage: addIP <ip>\n");
		return;
	}

	for (i = 0; i < sg_numIpFilters; i++){
		if (sg_ipFilters[i].compare == 0xffffffff)
			break;	// Free spot
	}

	if (i == sg_numIpFilters){
		if (sg_numIpFilters == MAX_IP_FILTERS){
			gi.cprintf(NULL, PRINT_HIGH, "IP filter list is full!\n");
			return;
		}

		sg_numIpFilters++;
	}

	if (!SG_StringToFilter(gi.argv(2), &sg_ipFilters[i]))
		sg_ipFilters[i].compare = 0xFFFFFFFF;
}

/*
 ==================
 SG_RemoveIP_f
 ==================
*/
static void SG_RemoveIP_f (){

	ipFilter_t	f;
	int			i, j;

	if (gi.argc() < 3){
		gi.cprintf(NULL, PRINT_HIGH, "Usage: removeIP <ip>\n");
		return;
	}

	if (!SG_StringToFilter(gi.argv(2), &f))
		return;

	for (i = 0; i < sg_numIpFilters; i++){
		if (sg_ipFilters[i].mask == f.mask && sg_ipFilters[i].compare == f.compare){
			for (j = i + 1; j < sg_numIpFilters; j++)
				sg_ipFilters[j-1] = sg_ipFilters[j];

			sg_numIpFilters--;

			gi.cprintf(NULL, PRINT_HIGH, "Removed.\n");
			return;
		}
	}

	gi.cprintf(NULL, PRINT_HIGH, "Didn't find %s.\n", gi.argv(2));
}

/*
 ==================
 SG_ListIP_f
 ==================
*/
static void SG_ListIP_f (){

	byte	b[4];
	int		i;

	gi.cprintf(NULL, PRINT_HIGH, "Filter list:\n");

	for (i = 0; i < sg_numIpFilters; i++){
		*(unsigned *)b = sg_ipFilters[i].compare;
		gi.cprintf(NULL, PRINT_HIGH, "%3i.%3i.%3i.%3i\n", b[0], b[1], b[2], b[3]);
	}
}

/*
 ==================
 SG_WriteIP_f
 ==================
*/
static void SG_WriteIP_f (){

	FILE	*f;
	char	name[MAX_OSPATH];
	byte	b[4];
	int		i;
	cvar_t	*game;

	game = gi.cvar("game", "", 0);

	if (!*game->string)
		sprintf(name, "%s/listIp.cfg", GAMEVERSION);
	else
		sprintf(name, "%s/listIp.cfg", game->string);

	gi.cprintf(NULL, PRINT_HIGH, "Writing %s.\n", name);

	f = fopen (name, "wb");
	if (!f){
		gi.cprintf(NULL, PRINT_HIGH, "Couldn't open %s\n", name);
		return;
	}
	
	fprintf(f, "set filterban %d\n", (int)filterban->value);

	for (i = 0; i < sg_numIpFilters; i++){
		*(unsigned *)b = sg_ipFilters[i].compare;
		fprintf(f, "sv addip %i.%i.%i.%i\n", b[0], b[1], b[2], b[3]);
	}
	
	fclose(f);
}

/*
 ==================
 SG_ServerCommand

 ServerCommand will be called when an "sv" command is issued.
 The game can issue gi.argc() / gi.argv() commands to get the rest
 of the parameters
 ==================
*/
void SG_ServerCommand (){

	char	*cmd;

	cmd = gi.argv(1);

	if (!Q_stricmp(cmd, "test"))
		SG_Test_f();
	else if (!Q_stricmp(cmd, "addip"))
		SG_AddIP_f();
	else if (!Q_stricmp(cmd, "removeip"))
		SG_RemoveIP_f();
	else if (!Q_stricmp(cmd, "listip"))
		SG_ListIP_f();
	else if (!Q_stricmp(cmd, "writeip"))
		SG_WriteIP_f();
	else
		gi.cprintf(NULL, PRINT_HIGH, "Unknown server command \"%s\"\n", cmd);
}