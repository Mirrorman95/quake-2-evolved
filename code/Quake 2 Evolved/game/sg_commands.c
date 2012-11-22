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
// sg_commands.c - Commands executed by the client
//


#include "g_local.h"
#include "m_player.h"


static void SG_SelectNextItem (edict_t *entity, int itflags);


/*
 ==================
 SG_PlayerSort
 ==================
*/
static int SG_PlayerSort (void const *a, void const *b){

	int		anum, bnum;

	anum = *(int *)a;
	bnum = *(int *)b;

	anum = game.clients[anum].ps.stats[STAT_FRAGS];
	bnum = game.clients[bnum].ps.stats[STAT_FRAGS];

	if (anum < bnum)
		return -1;
	if (anum > bnum)
		return 1;

	return 0;
}

/*
 ==================
 SG_ClientTeam
 ==================
*/
const char *SG_ClientTeam (edict_t *entity){

	static char	value[512];
	char		*p;

	value[0] = 0;

	if (!entity->client)
		return value;

	strcpy(value, Info_ValueForKey(entity->client->pers.userinfo, "skin"));

	p = strchr(value, '/');
	if (!p)
		return value;

	if ((int)(dmflags->value) & DF_MODELTEAMS){
		*p = 0;
		return value;
	}

	return ++p;
}

/*
 ==================
 SG_ValidateSelectedItem
 ==================
*/
void SG_ValidateSelectedItem (edict_t *entity){

	gclient_t	*client;

	client = entity->client;

	if (client->pers.inventory[client->pers.selected_item])
		return;		// Valid

	SG_SelectNextItem(entity, -1);
}

/*
 ==================
 SG_SelectNextItem
 ==================
*/
static void SG_SelectNextItem (edict_t *entity, int itflags){

	gclient_t	*client;
	int			i, index;
	gItem_t		*it;

	client = entity->client;

	if (client->chaseTarget){
		SG_ChaseNext(entity);
		return;
	}

	// Scan for the next valid one
	for (i = 1; i <= MAX_ITEMS; i++){
		index = (client->pers.selected_item + i) % MAX_ITEMS;

		if (!client->pers.inventory[index])
			continue;

		it = &sg_itemList[index];

		if (!it->use)
			continue;
		if (!(it->flags & itflags))
			continue;

		client->pers.selected_item = index;
		return;
	}

	client->pers.selected_item = -1;
}

/*
 ==================
 SG_SelectPrevItem
 ==================
*/
static void SG_SelectPrevItem (edict_t *entity, int itflags){

	gclient_t	*client;
	int			i, index;
	gItem_t		*it;

	client = entity->client;

	if (client->chaseTarget){
		SG_ChasePrev(entity);
		return;
	}

	// Scan  for the next valid one
	for (i = 1; i <= MAX_ITEMS; i++){
		index = (client->pers.selected_item + MAX_ITEMS - i) % MAX_ITEMS;

		if (!client->pers.inventory[index])
			continue;

		it = &sg_itemList[index];

		if (!it->use)
			continue;
		if (!(it->flags & itflags))
			continue;

		client->pers.selected_item = index;
		return;
	}

	client->pers.selected_item = -1;
}


/*
 ==============================================================================

 COMMAND EXECUTION

 ==============================================================================
*/


/*
 ==================
 SG_Players_f
 ==================
*/
static void SG_Players_f (edict_t *entity){

	int		count;
	int		i;
	int		index[256];
	char	large[1280];
	char	small[64];

	count = 0;

	for (i = 0; i < maxclients->value; i++){
		if (game.clients[i].pers.connected){
			index[count] = i;
			count++;
		}
	}

	// Sort by frags
	qsort(index, count, sizeof(index[0]), SG_PlayerSort);

	// Print information
	large[0] = 0;

	for (i = 0 ; i < count ; i++){
		Com_sprintf(small, sizeof(small), "%3i %s\n", game.clients[index[i]].ps.stats[STAT_FRAGS], game.clients[index[i]].pers.netname);

		// Can't print all of them in one packet
		if (strlen(small) + strlen(large) > sizeof(large) - 100){
			strcat(large, "...\n");
			break;
		}

		strcat(large, small);
	}

	gi.cprintf(entity, PRINT_HIGH, "%s\n%i players\n", large, count);
}

/*
 ==================
 SG_Say_f
 ==================
*/
static void SG_Say_f (edict_t *ent, qboolean team, qboolean arg0){

	char		text[2048];
	char		*p;
	gclient_t	*client;
	int			i, j;
	edict_t		*other;

	if (gi.argc () < 2 && !arg0)
		return;

	if (!((int)(dmflags->value) & (DF_MODELTEAMS | DF_SKINTEAMS)))
		team = false;

	if (team)
		Com_sprintf(text, sizeof(text), "(%s): ", ent->client->pers.netname);
	else
		Com_sprintf(text, sizeof(text), "%s: ", ent->client->pers.netname);

	if (arg0){
		strcat(text, gi.argv(0));
		strcat(text, " ");
		strcat(text, gi.args());
	}
	else {
		p = gi.args();

		if (*p == '"'){
			p++;
			p[strlen(p) - 1] = 0;
		}

		strcat(text, p);
	}

	// Don't let text be too long for malicious reasons
	if (strlen(text) > 150)
		text[150] = 0;

	strcat(text, "\n");

	if (flood_msgs->value){
		client = ent->client;

		if (level.time < client->flood_locktill){
			gi.cprintf(ent, PRINT_HIGH, "You can't talk for %d more seconds\n", (int)(client->flood_locktill - level.time));
			return;
		}

		i = client->flood_whenhead - flood_msgs->value + 1;
		if (i < 0)
			i = (sizeof(client->flood_when)/sizeof(client->flood_when[0])) + i;

		if (client->flood_when[i] && level.time - client->flood_when[i] < flood_persecond->value){
			client->flood_locktill = level.time + flood_waitdelay->value;
			gi.cprintf(ent, PRINT_CHAT, "Flood protection:  You can't talk for %d seconds.\n", (int)flood_waitdelay->value);
			return;
		}

		client->flood_whenhead = (client->flood_whenhead + 1) % (sizeof(client->flood_when)/sizeof(client->flood_when[0]));
		client->flood_when[client->flood_whenhead] = level.time;
	}

	if (dedicated->value)
		gi.cprintf(NULL, PRINT_CHAT, "%s", text);

	for (j = 1; j <= game.maxclients; j++){
		other = &g_edicts[j];

		if (!other->inuse)
			continue;
		if (!other->client)
			continue;

		if (team){
			if (!SG_IsOnSameTeam(ent, other))
				continue;
		}

		gi.cprintf(other, PRINT_CHAT, "%s", text);
	}
}

/*
 ==================
 SG_Use_f

 Use an inventory item
 ==================
*/
static void SG_Use_f (edict_t *entity){

	char		*s;
	gItem_t		*it;
	int			index;

	s = gi.args();

	it = SG_FindItem(s);

	if (!it){
		gi.cprintf(entity, PRINT_HIGH, "Unknown item: %s\n", s);
		return;
	}

	if (!it->use){
		gi.cprintf(entity, PRINT_HIGH, "Item is not usable.\n");
		return;
	}

	index = ITEM_INDEX(it);

	if (!entity->client->pers.inventory[index]){
		gi.cprintf(entity, PRINT_HIGH, "Out of item: %s\n", s);
		return;
	}

	it->use(entity, it);
}

/*
 ==================
 SG_Drop_f

 Drop an inventory item
 ==================
*/
static void SG_Drop_f (edict_t *entity){

	char	*s;
	gItem_t	*it;
	int		index;

	s = gi.args();

	it = SG_FindItem(s);

	if (!it){
		gi.cprintf(entity, PRINT_HIGH, "Unknown item: %s\n", s);
		return;
	}

	if (!it->drop){
		gi.cprintf(entity, PRINT_HIGH, "Item is not dropable.\n");
		return;
	}

	index = ITEM_INDEX(it);

	if (!entity->client->pers.inventory[index]){
		gi.cprintf(entity, PRINT_HIGH, "Out of item: %s\n", s);
		return;
	}

	it->drop(entity, it);
}

/*
 ==================
 SG_Give_f

 Give items to a client
 ==================
*/
static void SG_Give_f (edict_t *entity){

	char			*name;
	qboolean		giveAll;
	gItemArmor_t	*info;
	gItem_t			*it;
	edict_t			*itEntity;
	int				index;
	int				i;

	if (deathmatch->value && !sv_cheats->value){
		gi.cprintf(entity, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	name = gi.args();

	if (!Q_stricmp(name, "all"))
		giveAll = true;
	else
		giveAll = false;

	if (giveAll || !Q_stricmp(gi.argv(1), "health")){
		if (gi.argc() == 3)
			entity->health = atoi(gi.argv(2));
		else
			entity->health = entity->max_health;

		if (!giveAll)
			return;
	}

	if (giveAll || !Q_stricmp(name, "weapons")){
		for (i = 0; i < game.num_items; i++){
			it = sg_itemList + i;

			if (!it->pickup)
				continue;
			if (!(it->flags & IT_WEAPON))
				continue;

			entity->client->pers.inventory[i] += 1;
		}

		if (!giveAll)
			return;
	}

	if (giveAll || !Q_stricmp(name, "ammo")){
		for (i = 0; i < game.num_items; i++){
			it = sg_itemList + i;

			if (!it->pickup)
				continue;
			if (!(it->flags & IT_AMMO))
				continue;

			SG_Add_Ammo(entity, it, 1000);
		}

		if (!giveAll)
			return;
	}

	if (giveAll || !Q_stricmp(name, "armor")){
		it = SG_FindItem("Jacket Armor");
		entity->client->pers.inventory[ITEM_INDEX(it)] = 0;

		it = SG_FindItem("Combat Armor");
		entity->client->pers.inventory[ITEM_INDEX(it)] = 0;

		it = SG_FindItem("Body Armor");
		info = (gItemArmor_t *)it->info;
		entity->client->pers.inventory[ITEM_INDEX(it)] = info->maxCount;

		if (!giveAll)
			return;
	}

	if (giveAll || !Q_stricmp(name, "Power Shield")){
		it = SG_FindItem("Power Shield");

		itEntity = SG_AllocEntity();
		itEntity->className = it->className;

		SG_SpawnItem(itEntity, it);
		SG_Touch_Item(itEntity, entity, NULL, NULL);

		if (itEntity->inuse)
			SG_FreeEntity(itEntity);

		if (!giveAll)
			return;
	}

	if (giveAll){
		for (i = 0; i < game.num_items; i++){
			it = sg_itemList + i;

			if (!it->pickup)
				continue;
			if (it->flags & (IT_ARMOR | IT_WEAPON | IT_AMMO))
				continue;

			entity->client->pers.inventory[i] = 1;
		}

		return;
	}

	it = SG_FindItem (name);
	if (!it){
		name = gi.argv(1);

		it = SG_FindItem (name);

		if (!it){
			gi.cprintf(entity, PRINT_HIGH, "Unknown item\n");
			return;
		}
	}

	if (!it->pickup){
		gi.cprintf(entity, PRINT_HIGH, "Non-pickup item\n");
		return;
	}

	index = ITEM_INDEX(it);

	if (it->flags & IT_AMMO){
		if (gi.argc() == 3)
			entity->client->pers.inventory[index] = atoi(gi.argv(2));
		else
			entity->client->pers.inventory[index] += it->quantity;
	}
	else {
		itEntity = SG_AllocEntity();
		itEntity->className = it->className;

		SG_SpawnItem(itEntity, it);
		SG_Touch_Item(itEntity, entity, NULL, NULL);

		if (itEntity->inuse)
			SG_FreeEntity(itEntity);
	}
}

/*
 ==================
 SG_God_f

 Sets client to godmode
 ==================
*/
static void SG_God_f (edict_t *entity){

	char	*msg;

	if (deathmatch->value && !sv_cheats->value){
		gi.cprintf(entity, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	entity->flags ^= FL_GODMODE;

	if (!(entity->flags & FL_GODMODE))
		msg = "godMode OFF\n";
	else
		msg = "godMode ON\n";

	gi.cprintf(entity, PRINT_HIGH, msg);
}

/*
 ==================
 SG_NoTarget_f

 Sets client to noTarget
 ==================
*/
static void SG_NoTarget_f (edict_t *entity){

	char	*msg;

	if (deathmatch->value && !sv_cheats->value){
		gi.cprintf(entity, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	entity->flags ^= FL_NOTARGET;

	if (!(entity->flags & FL_NOTARGET)){
		msg = "noTarget OFF\n";
		level.noTarget = false;
	}
	else {
		msg = "noTarget ON\n";
		level.noTarget = true;
	}

	gi.cprintf(entity, PRINT_HIGH, msg);
}

/*
 ==================
 SG_Noclip_f

 Sets client to noclip
 ==================
*/
static void SG_Noclip_f (edict_t *entity){

	char	*msg;

	if (deathmatch->value && !sv_cheats->value){
		gi.cprintf(entity, PRINT_HIGH, "You must run the server with '+set cheats 1' to enable this command.\n");
		return;
	}

	if (entity->movetype == MOVETYPE_NOCLIP){
		entity->movetype = MOVETYPE_WALK;
		msg = "noClip OFF\n";
	}
	else {
		entity->movetype = MOVETYPE_NOCLIP;
		msg = "noClip ON\n";
	}

	gi.cprintf(entity, PRINT_HIGH, msg);
}

/*
 ==================
 SG_Inven_f
 ==================
*/
static void SG_Inven_f (edict_t *entity){

	gclient_t	*client;
	int			i;

	client = entity->client;

	client->showscores = false;
	client->showhelp = false;

	if (client->showinventory){
		client->showinventory = false;
		return;
	}

	client->showinventory = true;

	gi.WriteByte(svc_inventory);

	for (i = 0; i < MAX_ITEMS; i++)
		gi.WriteShort(client->pers.inventory[i]);

	gi.unicast(entity, true);
}

/*
 ==================
 SG_InvNext_f
 ==================
*/
static void SG_InvNext_f (edict_t *entity, int flags){

	SG_SelectNextItem(entity, flags);
}

/*
 ==================
 SG_InvPrev_f
 ==================
*/
static void SG_InvPrev_f (edict_t *entity, int flags){

	SG_SelectPrevItem(entity, flags);
}

/*
 ==================
 SG_InvNextw_f
 ==================
*/
static void SG_InvNextw_f (edict_t *entity, int flags){

	SG_SelectNextItem(entity, flags);
}

/*
 ==================
 SG_InvPrevw_f
 ==================
*/
static void SG_InvPrevw_f (edict_t *entity, int flags){

	SG_SelectPrevItem(entity, flags);
}

/*
 ==================
 SG_InvNextp_f
 ==================
*/
static void SG_InvNextp_f (edict_t *entity, int flags){

	SG_SelectNextItem(entity, flags);
}

/*
 ==================
 SG_InvPrevp_f
 ==================
*/
static void SG_InvPrevp_f (edict_t *entity, int flags){

	SG_SelectPrevItem(entity, flags);
}

/*
 ==================
 SG_InvUse_f
 ==================
*/
static void SG_InvUse_f (edict_t *entity){

	gItem_t	*it;

	SG_ValidateSelectedItem(entity);

	if (entity->client->pers.selected_item == -1){
		gi.cprintf(entity, PRINT_HIGH, "No item to use.\n");
		return;
	}

	it = &sg_itemList[entity->client->pers.selected_item];

	if (!it->use){
		gi.cprintf(entity, PRINT_HIGH, "Item is not usable.\n");
		return;
	}

	it->use(entity, it);
}

/*
 ==================
 SG_InvDrop_f
 ==================
*/
static void SG_InvDrop_f (edict_t *entity){

	gItem_t	*it;

	SG_ValidateSelectedItem(entity);

	if (entity->client->pers.selected_item == -1){
		gi.cprintf(entity, PRINT_HIGH, "No item to drop.\n");
		return;
	}

	it = &sg_itemList[entity->client->pers.selected_item];

	if (!it->drop){
		gi.cprintf(entity, PRINT_HIGH, "Item is not dropable.\n");
		return;
	}

	it->drop(entity, it);
}

/*
 ==================
 SG_WeapPrev_f
 ==================
*/
static void SG_WeapPrev_f (edict_t *entity){

	gclient_t	*client;
	int			selectedWeapon;
	int			i, index;
	gItem_t		*it;

	client = entity->client;

	if (!client->pers.weapon)
		return;

	selectedWeapon = ITEM_INDEX(client->pers.weapon);

	// Scan for the next valid one
	for (i = 1; i <= MAX_ITEMS; i++){
		index = (selectedWeapon + i) % MAX_ITEMS;

		if (!client->pers.inventory[index])
			continue;

		it = &sg_itemList[index];

		if (!it->use)
			continue;
		if (!(it->flags & IT_WEAPON))
			continue;

		it->use(entity, it);

		if (client->pers.weapon == it)
			return;		// Successful
	}
}

/*
 ==================
 SG_WeapNext_f
 ==================
*/
static void SG_WeapNext_f (edict_t *entity){

	gclient_t	*client;
	int			selectedWeapon;
	int			i, index;
	gItem_t		*it;

	client = entity->client;

	if (!client->pers.weapon)
		return;

	selectedWeapon = ITEM_INDEX(client->pers.weapon);

	// Scan for the next valid one
	for (i = 1; i <= MAX_ITEMS; i++){
		index = (selectedWeapon + MAX_ITEMS - i) % MAX_ITEMS;

		if (!client->pers.inventory[index])
			continue;

		it = &sg_itemList[index];

		if (!it->use)
			continue;
		if (!(it->flags & IT_WEAPON))
			continue;

		it->use(entity, it);

		if (client->pers.weapon == it)
			return;		// Successful
	}
}

/*
 ==================
 SG_WeapLast_f
 ==================
*/
static void SG_WeapLast_f (edict_t *entity){

	gclient_t	*client;
	int			index;
	gItem_t		*it;

	client = entity->client;

	if (!client->pers.weapon || !client->pers.lastweapon)
		return;

	index = ITEM_INDEX(client->pers.lastweapon);

	if (!client->pers.inventory[index])
		return;

	it = &sg_itemList[index];

	if (!it->use)
		return;
	if (!(it->flags & IT_WEAPON))
		return;

	it->use(entity, it);
}

/*
 ==================
 SG_Kill_f
 ==================
*/
static void SG_Kill_f (edict_t *entity){

	if ((level.time - entity->client->respawn_time) < 5)
		return;

	entity->flags &= ~FL_GODMODE;
	entity->health = 0;

	meansOfDeath = MOD_SUICIDE;

	SG_Player_Die(entity, entity, entity, 100000, vec3_origin);
}

/*
 ==================
 SG_PutAway_f
 ==================
*/
static void SG_PutAway_f (edict_t *entity){

	entity->client->showscores = false;
	entity->client->showhelp = false;
	entity->client->showinventory = false;
}

/*
 ==================
 SG_Wave_f
 ==================
*/
static void SG_Wave_f (edict_t *entity){

	int		i;

	i = atoi(gi.argv(1));

	// Can't wave when ducked
	if (entity->client->ps.pmove.pm_flags & PMF_DUCKED)
		return;

	if (entity->client->anim_priority > ANIM_WAVE)
		return;

	entity->client->anim_priority = ANIM_WAVE;

	switch (i){
	case 0:
		gi.cprintf(entity, PRINT_HIGH, "flipoff\n");

		entity->s.frame = FRAME_flip01 - 1;
		entity->client->anim_end = FRAME_flip12;
		break;
	case 1:
		gi.cprintf(entity, PRINT_HIGH, "salute\n");

		entity->s.frame = FRAME_salute01 - 1;
		entity->client->anim_end = FRAME_salute11;
		break;
	case 2:
		gi.cprintf(entity, PRINT_HIGH, "taunt\n");

		entity->s.frame = FRAME_taunt01 - 1;
		entity->client->anim_end = FRAME_taunt17;
		break;
	case 3:
		gi.cprintf(entity, PRINT_HIGH, "wave\n");

		entity->s.frame = FRAME_wave01 - 1;
		entity->client->anim_end = FRAME_wave11;
		break;
	case 4:
	default:
		gi.cprintf(entity, PRINT_HIGH, "point\n");

		entity->s.frame = FRAME_point01 - 1;
		entity->client->anim_end = FRAME_point12;
		break;
	}
}

/*
 ==================
 SG_PlayerList_f
 ==================
*/
static void SG_PlayerList_f (edict_t *entity){

	char	text[1400];
	int		i;
	edict_t *e2;
	char	st[80];


	// Connect time, ping, score, name
	*text = 0;

	for (i = 0, e2 = g_edicts + 1; i < maxclients->value; i++, e2++) {
		if (!e2->inuse)
			continue;

		Com_sprintf(st, sizeof(st), "%02d:%02d %4d %3d %s%s\n",
			(level.framenum - e2->client->resp.enterframe) / 600,
			((level.framenum - e2->client->resp.enterframe) % 600) / 10,
			e2->client->ping,
			e2->client->resp.score,
			e2->client->pers.netname,
			e2->client->resp.spectator ? " (spectator)" : "");

		if (strlen(text) + strlen(st) > sizeof(text) - 50){
			sprintf(text + strlen(text), "And more...\n");
			gi.cprintf(entity, PRINT_HIGH, "%s", text);
			return;
		}

		strcat(text, st);
	}

	gi.cprintf(entity, PRINT_HIGH, "%s", text);
}

/*
 ==================
 SG_Spawn_f
 ==================
*/
static void SG_Spawn_f (edict_t *entity){

	char	*cmd;

	if (gi.argc() > 2){
		gi.cprintf(entity, PRINT_HIGH, "Usage: spawn [name]\n");
		return;
	}

	cmd = gi.argv(1);

	SG_SpawnEntity(entity, cmd);
}

/*
 ==================
 SG_ClientCommand
 ==================
*/
void SG_ClientCommand (edict_t *entity){

	char	*cmd;

	if (!entity->client)
		return;		// Not fully in game yet

	cmd = gi.argv(0);

	if (!Q_stricmp(cmd, "players")){
		SG_Players_f(entity);
		return;
	}

	if (!Q_stricmp(cmd, "say")){
		SG_Say_f(entity, false, false);
		return;
	}

	if (!Q_stricmp(cmd, "say_team")){
		SG_Say_f(entity, true, false);
		return;
	}

	if (!Q_stricmp(cmd, "score")){
		SG_Score_f(entity);
		return;
	}

	if (!Q_stricmp(cmd, "help")){
		SG_Help_f(entity);
		return;
	}

	if (level.intermissiontime)
		return;

	if (!Q_stricmp(cmd, "use"))
		SG_Use_f(entity);
	else if (!Q_stricmp(cmd, "drop"))
		SG_Drop_f(entity);
	else if (!Q_stricmp(cmd, "give"))
		SG_Give_f(entity);
	else if (!Q_stricmp(cmd, "god"))
		SG_God_f(entity);
	else if (!Q_stricmp(cmd, "notarget"))
		SG_NoTarget_f(entity);
	else if (!Q_stricmp(cmd, "noclip"))
		SG_Noclip_f(entity);
	else if (!Q_stricmp (cmd, "inven"))
		SG_Inven_f(entity);
	else if (!Q_stricmp(cmd, "invnext"))
		SG_InvNext_f(entity, -1);
	else if (!Q_stricmp(cmd, "invprev"))
		SG_InvPrev_f(entity, -1);
	else if (!Q_stricmp(cmd, "invnextw"))
		SG_InvNextw_f(entity, IT_WEAPON);
	else if (!Q_stricmp(cmd, "invprevw"))
		SG_InvPrevw_f(entity, IT_WEAPON);
	else if (!Q_stricmp(cmd, "invnextp"))
		SG_InvNextp_f(entity, IT_POWERUP);
	else if (!Q_stricmp(cmd, "invprevp"))
		SG_InvPrevp_f(entity, IT_POWERUP);
	else if (!Q_stricmp(cmd, "invuse"))
		SG_InvUse_f(entity);
	else if (!Q_stricmp(cmd, "invdrop"))
		SG_InvDrop_f(entity);
	else if (!Q_stricmp(cmd, "weapprev"))
		SG_WeapPrev_f(entity);
	else if (!Q_stricmp(cmd, "weapnext"))
		SG_WeapNext_f(entity);
	else if (!Q_stricmp(cmd, "weaplast"))
		SG_WeapLast_f(entity);
	else if (!Q_stricmp(cmd, "kill"))
		SG_Kill_f(entity);
	else if (!Q_stricmp(cmd, "putaway"))
		SG_PutAway_f(entity);
	else if (!Q_stricmp (cmd, "wave"))
		SG_Wave_f(entity);
	else if (!Q_stricmp(cmd, "playerlist"))
		SG_PlayerList_f(entity);
	else if (!Q_stricmp(cmd, "spawn"))
		SG_Spawn_f(entity);
	else	// Anything that doesn't match a command will be a chat
		SG_Say_f(entity, false, true);
}