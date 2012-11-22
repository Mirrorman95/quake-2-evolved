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
// sg_local.h - Local definitions for the server-game module
//


#include "q_shared.h"


// define GAME_INCLUDE so that game.h does not define the
// short, server-visible gclient_t and edict_t structures,
// because we define the full size ones in this file
#define	GAME_INCLUDE
#include "game.h"

#define GAME_VERSION "0.40 BETA"

// the "gameversion" client command will print this plus compile date
#define	GAMEVERSION	"baseq2"

// protocol bytes that can be directly added to messages
#define	svc_muzzleflash		1
#define	svc_muzzleflash2	2
#define	svc_temp_entity		3
#define	svc_layout			4
#define	svc_inventory		5
#define	svc_stufftext		11

//==================================================================

// View pitching times
#define DAMAGE_TIME						0.5f
#define	FALL_TIME						0.3f


// edict->spawnflags
// these are set with checkboxes on each entity in the map editor
#define	SPAWNFLAG_NOT_EASY			0x00000100
#define	SPAWNFLAG_NOT_MEDIUM		0x00000200
#define	SPAWNFLAG_NOT_HARD			0x00000400
#define	SPAWNFLAG_NOT_DEATHMATCH	0x00000800
#define	SPAWNFLAG_NOT_COOP			0x00001000

// edict->flags
#define	FL_FLY					0x00000001
#define	FL_SWIM					0x00000002	// implied immunity to drowining
#define FL_IMMUNE_LASER			0x00000004
#define	FL_INWATER				0x00000008
#define	FL_GODMODE				0x00000010
#define	FL_NOTARGET				0x00000020
#define FL_IMMUNE_SLIME			0x00000040
#define FL_IMMUNE_LAVA			0x00000080
#define	FL_PARTIALGROUND		0x00000100	// not all corners are valid
#define	FL_WATERJUMP			0x00000200	// player jumping out of water
#define	FL_TEAMSLAVE			0x00000400	// not the first on the team
#define FL_NO_KNOCKBACK			0x00000800
#define FL_POWER_ARMOR			0x00001000	// power armor (if any) is active
#define FL_RESPAWN				0x80000000	// used for item respawning

#define	FRAMETIME						0.1f

// Memory tags to allow dynamic memory to be cleaned up
#define	TAG_GAME						765		// clear when unloading the dll
#define	TAG_LEVEL						766		// clear when loading a new level

#define MELEE_DISTANCE					80.0f

#define BODY_QUEUE_SIZE					8

typedef enum {
	DAMAGE_NO,
	DAMAGE_YES,			// will take damage if hit
	DAMAGE_AIM			// auto targeting recognizes this
} damage_t;

typedef enum {
	WEAPON_READY, 
	WEAPON_ACTIVATING,
	WEAPON_DROPPING,
	WEAPON_FIRING
} weaponState_t;

typedef enum {
	AMMO_BULLETS,
	AMMO_SHELLS,
	AMMO_ROCKETS,
	AMMO_GRENADES,
	AMMO_CELLS,
	AMMO_SLUGS
} ammo_t;

// Dead flag types
typedef enum {
	DEAD_NO,
	DEAD_DYING,
	DEAD_DEAD,
	DEAD_RESPAWNABLE
} deadFlagTypes_t;

// Range types
typedef enum {
	RANGE_MELEE,
	RANGE_NEAR,
	RANGE_MID,
	RANGE_FAR
} rangeTypes_t;

// Gib types
typedef enum {
	GIB_ORGANIC,
	GIB_METALLIC
} gibTypes_t;

// Monster ai flags
#define AI_STAND_GROUND			0x00000001
#define AI_TEMP_STAND_GROUND	0x00000002
#define AI_SOUND_TARGET			0x00000004
#define AI_LOST_SIGHT			0x00000008
#define AI_PURSUIT_LAST_SEEN	0x00000010
#define AI_PURSUE_NEXT			0x00000020
#define AI_PURSUE_TEMP			0x00000040
#define AI_HOLD_FRAME			0x00000080
#define AI_GOOD_GUY				0x00000100
#define AI_BRUTAL				0x00000200
#define AI_NOSTEP				0x00000400
#define AI_DUCKED				0x00000800
#define AI_COMBAT_POINT			0x00001000
#define AI_MEDIC				0x00002000
#define AI_RESURRECTING			0x00004000

// Monster attack state
#define AS_STRAIGHT				1
#define AS_SLIDING				2
#define	AS_MELEE				3
#define	AS_MISSILE				4

// Armor types
#define ARMOR_NONE				0
#define ARMOR_JACKET			1
#define ARMOR_COMBAT			2
#define ARMOR_BODY				3
#define ARMOR_SHARD				4

// Power armor types
#define POWER_ARMOR_NONE		0
#define POWER_ARMOR_SCREEN		1
#define POWER_ARMOR_SHIELD		2

// Handedness values
#define RIGHT_HANDED			0
#define LEFT_HANDED				1
#define CENTER_HANDED			2


// game.serverflags values
#define SFL_CROSS_TRIGGER_1		0x00000001
#define SFL_CROSS_TRIGGER_2		0x00000002
#define SFL_CROSS_TRIGGER_3		0x00000004
#define SFL_CROSS_TRIGGER_4		0x00000008
#define SFL_CROSS_TRIGGER_5		0x00000010
#define SFL_CROSS_TRIGGER_6		0x00000020
#define SFL_CROSS_TRIGGER_7		0x00000040
#define SFL_CROSS_TRIGGER_8		0x00000080
#define SFL_CROSS_TRIGGER_MASK	0x000000ff


// noise types for PlayerNoise
#define PNOISE_SELF				0
#define PNOISE_WEAPON			1
#define PNOISE_IMPACT			2


// edict->movetype values
typedef enum {
	MOVETYPE_NONE,			// never moves
	MOVETYPE_NOCLIP,		// origin and angles change with no interaction
	MOVETYPE_PUSH,			// no clip to world, push on box contact
	MOVETYPE_STOP,			// no clip to world, stops on box contact

	MOVETYPE_WALK,			// gravity
	MOVETYPE_STEP,			// gravity, special edge handling
	MOVETYPE_FLY,
	MOVETYPE_TOSS,			// gravity
	MOVETYPE_FLYMISSILE,	// extra size to monsters
	MOVETYPE_BOUNCE
} movetype_t;

// This structure is left intact through an entire game
// it should be initialized at dll load time, and read/written to
// the server.ssv file for savegames
typedef struct {
	char				helpmessage1[512];
	char				helpmessage2[512];
	int					helpchanged;			// flash F1 icon if non 0, play sound
												// and increment only if 1, 2, or 3

	gclient_t *			clients;				// [maxclients]

	// Can't store spawnpoint in level, because
	// it would get overwritten by the savegame restore
	char				spawnpoint[512];		// needed for coop respawns

	// Store latched cvars here that we want to get at often
	int					maxclients;
	int					maxentities;

	// Cross level triggers
	int					serverflags;

	// Items
	int					num_items;

	qboolean			autosaved;

	// No notarget check for monsters
	qboolean			noTarget;
} gameLocals_t;

// This structure is cleared as each map is entered
// it is read/written to the level.sav file for savegames
typedef struct {
	int					framenum;
	float				time;

	char				level_name[MAX_QPATH];	// the descriptive name (Outer Base, etc)
	char				mapname[MAX_QPATH];		// the server name (base1, etc)
	char				nextmap[MAX_QPATH];		// go here when fraglimit is hit

	// Intermission state
	float				intermissiontime;		// time the intermission was started
	char *				changemap;
	int					exitintermission;
	vec3_t				intermission_origin;
	vec3_t				intermission_angle;

	edict_t *			sight_client;			// changed once each frame for coop games

	edict_t *			sight_entity;
	int					sight_entity_framenum;
	edict_t *			sound_entity;
	int					sound_entity_framenum;
	edict_t *			sound2_entity;
	int					sound2_entity_framenum;

	int					pic_health;

	int					total_secrets;
	int					found_secrets;

	int					total_goals;
	int					found_goals;

	int					total_monsters;
	int					killed_monsters;

	edict_t *			current_entity;			// entity running from SG_Framework
	int					body_que;				// dead bodies

	int					power_cubes;			// ugly necessity for coop

	// NoTarget check for monsters
	qboolean			noTarget;
} levelLocals_t;

// spawn_temp_t is only used to hold entity field values that
// can be set from the editor, but aren't actualy present
// in edict_t during gameplay
typedef struct {
	// World vars
	char *				sky;
	float				skyrotate;
	vec3_t				skyaxis;
	char *				nextmap;

	int					lip;
	int					distance;
	int					height;
	char *				noise;
	float				pausetime;
	char *				item;
	char *				gravity;

	float				minyaw;
	float				maxyaw;
	float				minpitch;
	float				maxpitch;
} spawn_temp_t;

typedef struct {
	void				(*aifunc)(edict_t *self, float dist);
	float				dist;
	void				(*thinkfunc)(edict_t *self);
} mframe_t;

typedef struct {
	int					firstframe;
	int					lastframe;
	mframe_t *			frame;
	void				(*endfunc)(edict_t *self);
} mmove_t;


extern int				sm_meat_index;
extern int				snd_fry;


// means of death
#define MOD_UNKNOWN			0
#define MOD_BLASTER			1
#define MOD_SHOTGUN			2
#define MOD_SSHOTGUN		3
#define MOD_MACHINEGUN		4
#define MOD_CHAINGUN		5
#define MOD_GRENADE			6
#define MOD_G_SPLASH		7
#define MOD_ROCKET			8
#define MOD_R_SPLASH		9
#define MOD_HYPERBLASTER	10
#define MOD_RAILGUN			11
#define MOD_BFG_LASER		12
#define MOD_BFG_BLAST		13
#define MOD_BFG_EFFECT		14
#define MOD_HANDGRENADE		15
#define MOD_HG_SPLASH		16
#define MOD_WATER			17
#define MOD_SLIME			18
#define MOD_LAVA			19
#define MOD_CRUSH			20
#define MOD_TELEFRAG		21
#define MOD_FALLING			22
#define MOD_SUICIDE			23
#define MOD_HELD_GRENADE	24
#define MOD_EXPLOSIVE		25
#define MOD_BARREL			26
#define MOD_BOMB			27
#define MOD_EXIT			28
#define MOD_SPLASH			29
#define MOD_TARGET_LASER	30
#define MOD_TRIGGER_HURT	31
#define MOD_HIT				32
#define MOD_TARGET_BLASTER	33
#define MOD_FRIENDLY_FIRE	0x8000000

extern	int	meansOfDeath;


extern	edict_t			*g_edicts;

#define	FOFS(x) (int)&(((edict_t *)0)->x)
#define	STOFS(x) (int)&(((spawn_temp_t *)0)->x)
#define	LLOFS(x) (int)&(((levelLocals_t *)0)->x)
#define	CLOFS(x) (int)&(((gclient_t *)0)->x)

#define random()	((rand () & 0x7fff) / ((float)0x7fff))
#define crandom()	(2.0 * (random() - 0.5))

#define world	(&g_edicts[0])

//
// FIXME!!!
//





/*
 ==============================================================================

 DATA WRITE/READ MANAGER

 ==============================================================================
*/

// Fields are needed for spawning from the entity string
// and saving / loading games
#define FFL_SPAWNTEMP		1
#define FFL_NOSPAWN			2

typedef enum {
	F_INT, 
	F_FLOAT,
	F_LSTRING,									// string on disk, pointer in memory, TAG_LEVEL
	F_GSTRING,									// string on disk, pointer in memory, TAG_GAME
	F_VECTOR,
	F_ANGLEHACK,
	F_EDICT,									// index on disk, pointer in memory
	F_ITEM,										// index on disk, pointer in memory
	F_CLIENT,									// index on disk, pointer in memory
	F_FUNCTION,
	F_MMOVE,
	F_IGNORE
} fieldType_t;

typedef struct {
	char *				name;
	int					ofs;
	fieldType_t			type;
	int					flags;
} field_t;


extern	field_t			fields[];

void			SG_WriteGame (char *name, qboolean autoSave);
void			SG_ReadGame (char *name);

void			SG_WriteLevel (char *name);
void			SG_ReadLevel (char *name);

/*
 ==============================================================================

 ITEM MANAGER

 ==============================================================================
*/

// Item flags
#define	IT_WEAPON						1		// use makes active weapon
#define	IT_AMMO							2
#define IT_ARMOR						4
#define IT_STAY_COOP					8
#define IT_KEY							16
#define IT_POWERUP						32

// gItem_t->weapmodel for weapons indicates model index
#define WEAP_BLASTER					1 
#define WEAP_SHOTGUN					2 
#define WEAP_SUPERSHOTGUN				3 
#define WEAP_MACHINEGUN					4 
#define WEAP_CHAINGUN					5 
#define WEAP_GRENADES					6 
#define WEAP_GRENADELAUNCHER			7 
#define WEAP_ROCKETLAUNCHER				8 
#define WEAP_HYPERBLASTER				9 
#define WEAP_RAILGUN					10
#define WEAP_BFG						11

// Item spawnflags
#define ITEM_TRIGGER_SPAWN				0x00000001
#define ITEM_NO_TOUCH					0x00000002

// 6 bits reserved for editor flags
// 8 bits used as power cube id bits for coop games
#define DROPPED_ITEM					0x00010000
#define	DROPPED_PLAYER_ITEM				0x00020000
#define ITEM_TARGETS_USED				0x00040000

#define	ITEM_INDEX(x)					((x) - sg_itemList)
#define ARMOR_INDEX						ITEM_INDEX(SG_FindItem("Body Armor"))

typedef struct {
	int					baseCount;
	int					maxCount;

	float				normalProtection;
	float				energyProtection;

	int					armor;
} gItemArmor_t;

typedef struct gitem_s {
	char *				className;

	qboolean			(*pickup)(struct edict_s *entity, struct edict_s *other);
	void				(*use)(struct edict_s *entity, struct gitem_s *item);
	void				(*drop)(struct edict_s *entity, struct gitem_s *item);
	void				(*weaponthink)(struct edict_s *entity);

	char *				pickupSound;
	char *				worldModel;
	int					worldWodelFlags;
	char *				viewModel;

	// Client side info
	char *				icon;
	char *				pickupName;				// For printing on pickup
	int					countWidth;				// Number of digits to display by icon

	int					quantity;				// For ammo how much, for weapons how much is used per shot
	char *				ammo;					// For weapons
	int					flags;					// IT_* flags

	int					weaponModel;			// Weapon model index (for weapons)

	void *				info;
	int					tag;

	char *				precaches;				// String of all models, sounds, and images this item will use
} gItem_t;

extern gItem_t			sg_itemList[];

gItem_t	*		SG_GetItemByIndex (int index);
gItem_t	*		SG_FindItemByClassname (char *name);
gItem_t	*		SG_FindItem (char *name);

void			SG_SetRespawnTime (edict_t *entity, float delay);

qboolean		SG_Add_Ammo (edict_t *entity, gItem_t *item, int count);
int				SG_FindArmorByIndex (edict_t *entity);
int				SG_FindPowerArmorByType (edict_t *entity);

void			SG_Touch_Item (edict_t *entity, edict_t *other, cplane_t *plane, csurface_t *surface);
edict_t *		SG_Drop_Item (edict_t *entity, gItem_t *item);

void			SG_PrecacheItem (gItem_t *item);
void			SG_SpawnItem (edict_t *entity, gItem_t *item);

void			SG_SetItemNames ();

void			SG_InitItems ();

/*
 ==============================================================================

 UTILITIES

 ==============================================================================
*/

void			SG_ProjectSource (vec3_t point, vec3_t distance, vec3_t forward, vec3_t right, vec3_t result);

int				SG_EntityRange (edict_t *self, edict_t *other);
float			SG_EntityDistance (edict_t *self, edict_t *other);

qboolean		SG_IsEntityVisible (edict_t *self, edict_t *other);
qboolean		SG_IsEntityInFront (edict_t *self, edict_t *other);
qboolean		SG_IsInFOV (edict_t *self, edict_t *other, int degrees);
qboolean		SG_IsOnSameTeam (edict_t *entity1, edict_t *entity2);

void			SG_SpawnEntity (edict_t *self, char *cmd);
edict_t *		SG_FindEntity (edict_t *from, int offset, char *match);

edict_t *		SG_FindEntityWithinRadius (edict_t *from, vec3_t origin, float radius);

void			SG_InitEntity (edict_t *entity);
edict_t *		SG_AllocEntity ();
void			SG_FreeEntity (edict_t *entity);

edict_t *		SG_PickTarget (char *name);
void			SG_UseTargets (edict_t *entity, edict_t *activator);

void			SG_TouchTriggers (edict_t *entity);
void			SG_TouchSolids (edict_t *entity);

qboolean		SG_KillBox (edict_t *entity);

float *			SG_TempVector (float x, float y, float z);
char *			SG_VectorToString (vec3_t v);
float			SG_VectorToYaw (vec3_t vec);
void			SG_VectorToAngles (vec3_t vec, vec3_t angles);

void			SG_SetMoveDirection (vec3_t angles, vec3_t dir);

char *			SG_CopyString (char *in);

/*
 ==============================================================================

 COMBAT TARGET EVENTS

 ==============================================================================
*/

// Damage flags
#define DAMAGE_RADIUS					0x00000001	// Damage was indirect
#define DAMAGE_NO_ARMOR					0x00000002	// Armour does not protect from this damage
#define DAMAGE_ENERGY					0x00000004	// Damage is from an energy based weapon
#define DAMAGE_NO_KNOCKBACK				0x00000008	// Do not affect velocity, just view angles
#define DAMAGE_BULLET					0x00000010  // Damage is from a bullet (used for ricochets)
#define DAMAGE_NO_PROTECTION			0x00000020  // Armor, shields, invulnerability, and godmode have no effect

#define DEFAULT_BULLET_HSPREAD			300
#define DEFAULT_BULLET_VSPREAD			500
#define DEFAULT_SHOTGUN_HSPREAD			1000
#define DEFAULT_SHOTGUN_VSPREAD			500
#define DEFAULT_DEATHMATCH_SHOTGUN_COUNT	12
#define DEFAULT_SHOTGUN_COUNT			12
#define DEFAULT_SSHOTGUN_COUNT			20

qboolean		SG_InflictorCanDamage (edict_t *targ, edict_t *inflictor);

void			SG_TargetDamage (edict_t *target, edict_t *inflictor, edict_t *attacker, vec3_t dir, vec3_t point, vec3_t normal, int damage, int knockback, int dmgFlags, int mod);
void			SG_RadiusTargetDamage (edict_t *inflictor, edict_t *attacker, float damage, edict_t *ignore, float radius, int mod);

/*
 ==============================================================================

 ENTITY SPAWNING AND PARSING

 ==============================================================================
*/

void			SG_CallSpawn (edict_t *entity);

void			SG_SpawnEntities (char *mapName, char *entities, char *spawnPoint);

/*
 ==============================================================================

 MONSTER BACK-END

 ==============================================================================
*/

void			SG_MonsterFireBullet (edict_t *self, vec3_t start, vec3_t dir, int damage, int kick, int hSpread, int vSpread, int flashType);
void			SG_MonsterFireShotgun (edict_t *self, vec3_t start, vec3_t aimDir, int damage, int kick, int hSpread, int vSpread, int count, int flashType);
void			SG_MonsterFireBlaster (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int flashType, int effect);
void			SG_MonsterFireGrenade (edict_t *self, vec3_t start, vec3_t aimDir, int damage, int speed, int flashType);
void			SG_MonsterFireRocket (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int flashType);
void			SG_MonsterFireRailgun (edict_t *self, vec3_t start, vec3_t aimDir, int damage, int kick, int flashType);
void			SG_MonsterFireBFG10K (edict_t *self, vec3_t start, vec3_t aimDir, int damage, int speed, int kick, float damageRadius, int flashType);

qboolean		SG_MonsterCheckAttack (edict_t *self);
qboolean		SG_MonsterCheckAttack2 (edict_t *self);

void			SG_MonsterExplode (edict_t *self);

void			SG_FlyCheck (edict_t *self);

void			SG_CheckGround (edict_t *entity);

void			SG_MonsterUse (edict_t *self, edict_t *other, edict_t *activator);
void			SG_MonsterDeathUse (edict_t *self);

void			SG_GroundMonster (edict_t *self);

void			SG_AirMonster (edict_t *self);

void			SG_WaterMonster (edict_t *self);

void			SG_AttackFinished (edict_t *self, float time);
void			SG_DropToFloor (edict_t *ent);

qboolean		SG_MonsterCheckBottom (edict_t *entity);
qboolean		SG_MonsterMove (edict_t *entity, float yaw, float dist);
void			SG_MonsterChangeYaw (edict_t *entity);
void			SG_MonsterMoveToGoal (edict_t *entity, float dist);

/*
 ==============================================================================

 MISCELLANEOUS FUNCTIONS

 ==============================================================================
*/

void			SG_BecomeExplosion1 (edict_t *self);
void			SG_BecomeExplosion2 (edict_t *self);

void			ThrowGib (edict_t *self, char *gibname, int damage, int type);
void			ThrowHead (edict_t *self, char *gibname, int damage, int type);
void			ThrowClientHead (edict_t *self, int damage);

void			ThrowDebris (edict_t *self, char *modelname, float speed, vec3_t origin);


/*
 ==============================================================================

 AI BEHAVIOUR

 ==============================================================================
*/

void			SG_AIMove (edict_t *self, float dist);
void			SG_AIStand (edict_t *self, float dist);
void			SG_AIWalk (edict_t *self, float dist);
void			SG_AICharge (edict_t *self, float dist);
void			SG_AITurn (edict_t *self, float dist);
void			SG_AIRun (edict_t *self, float dist);

void			SG_AISetSightClient ();
void			SG_AIFoundTarget (edict_t *self);
qboolean		SG_AIFindTarget (edict_t *self);

qboolean		SG_AICheckMonsterAttack (edict_t *self);

/*
 ==============================================================================

 WEAPONS

 ==============================================================================
*/

void			SG_WeaponThinkFramework (edict_t *entity);

qboolean		SG_PickupWeapon (edict_t *entity, edict_t *other);
void			SG_ChangeWeapon (edict_t *entity);
void			SG_UseWeapon (edict_t *entity, gItem_t *inv);
void			SG_DropWeapon (edict_t *entity, gItem_t *inv);

void			SG_GrenadeWeapon (edict_t *entity);
void			SG_GrenadeLauncherWeapon (edict_t *entity);
void			SG_RocketLauncherWeapon (edict_t *entity);
void			SG_BlasterWeapon (edict_t *entity);
void			SG_HyperBlasterWeapon (edict_t *entity);
void			SG_MachinegunWeapon (edict_t *entity);
void			SG_ChaingunWeapon (edict_t *entity);
void			SG_ShotgunWeapon (edict_t *entity);
void			SG_SupershotgunWeapon (edict_t *entity);
void			SG_RailgunWeapon (edict_t *entity);
void			SG_BFGWeapon (edict_t *entity);

/*
 ==============================================================================

 PROJECTILES

 ==============================================================================
*/

qboolean		SG_MeleeHit (edict_t *self, vec3_t aim, int damage, int kick);
void			SG_FireBullet (edict_t *self, vec3_t start, vec3_t aimDir, int damage, int kick, int hSpread, int vSpread, int mod);
void			SG_FirePellet (edict_t *self, vec3_t start, vec3_t aimDir, int damage, int kick, int hSpread, int vSpread, int count, int mod);

void			SG_FireBlasterProjectile (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, int effect, qboolean hyper);
void			SG_FireGrenadeProjectile (edict_t *self, vec3_t start, vec3_t aimDir, int damage, int speed, float timer, float damageRadius);
void			SG_FireGrenadeProjectile2 (edict_t *self, vec3_t start, vec3_t aimDir, int damage, int speed, float timer, float damageRadius, qboolean held);
void			SG_FireRocketProjectile (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, float damageRadius, int radiusDamage);
void			SG_FireRailProjectile (edict_t *self, vec3_t start, vec3_t aimDir, int damage, int kick);
void			SG_FireBFGProjectile (edict_t *self, vec3_t start, vec3_t dir, int damage, int speed, float damageRadius);

/*
 ==============================================================================

 CLIENT TRAIL

 ==============================================================================
*/

void			SG_InitClientTrail ();
void			SG_AddClientTrail (vec3_t spot);

edict_t *		SG_ClientTrail_PickFirst (edict_t *self);
edict_t *		SG_ClientTrail_PickNext (edict_t *self);
edict_t	*		SG_ClientTrail_LastSpot ();

/*
 ==============================================================================

 CLIENT BACK-END

 ==============================================================================
*/

void			SG_PlayerNoise (edict_t *who, vec3_t where, int type);

void			SG_SaveClientData ();

void			SG_Player_Pain (edict_t *self, edict_t *other, float kick, int damage);
void			SG_Player_Die (edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point);

void			SG_InitClientBodyQue ();
void			SG_ClientRespawn (edict_t *self);

void			SG_ClientUserInfoChanged (edict_t *entity, char *userInfo);
qboolean		SG_ClientConnect (edict_t *entity, char *userInfo);
void			SG_ClientDisconnect (edict_t *entity);
void			SG_ClientBegin (edict_t *entity);

void			SG_ClientThinkFramework (edict_t *entity, usercmd_t *userCmd);
void			SG_ClientBeginServerFrame (edict_t *entity);

/*
 ==============================================================================

 SERVER INTERACTION COMMANDS

 ==============================================================================
*/

qboolean		SG_FilterPacket (const char *from);

void			SG_ServerCommand ();

const char *	SG_ClientTeam (edict_t *entity);

void			SG_ValidateSelectedItem (edict_t *entity);

void			SG_ClientCommand (edict_t *entity);

/*
 ==============================================================================

 CLIENT VIEW EFFECTS

 ==============================================================================
*/

void			SG_ClientEndServerFrame (edict_t *entity);

/*
 ==============================================================================

 HUD INTERFACE

 ==============================================================================
*/

void			SG_MoveClientToIntermission (edict_t *client);
void			SG_BeginIntermission (edict_t *target);

void			SG_DeathmatchScoreboardLayout (edict_t *entity, edict_t *killer);

void			SG_Score_f (edict_t *entity);
void			SG_Help_f (edict_t *entity);

void			SG_SetStats (edict_t *entity);
void			SG_CheckChaseStats (edict_t *entity);
void			SG_SetSpectatorStats (edict_t *entity);

//
// g_phys.c
//
void G_RunEntity (edict_t *ent);

/*
 ==============================================================================

 SPECTATOR CHASE CAMERA

 ==============================================================================
*/

typedef enum {
	SPM_FREE,
	SPM_CHASE,
	SPM_EYES
} spectatorMode_t;

void			SG_ChaseCamThinkFramework (usercmd_t *userCmd, gclient_t *client, edict_t *entity);

void			SG_ChaseNext(edict_t *entity);
void			SG_ChasePrev(edict_t *entity);
void			SG_GetChaseTarget(edict_t *entity);

//============================================================================

// client_t->anim_priority
#define	ANIM_BASIC		0		// stand / run
#define	ANIM_WAVE		1
#define	ANIM_JUMP		2
#define	ANIM_PAIN		3
#define	ANIM_ATTACK		4
#define	ANIM_DEATH		5
#define	ANIM_REVERSE	6

// Client data that stays across multiple level loads
typedef struct {
	char		userinfo[MAX_INFO_STRING];
	char		netname[16];
	int			hand;

	qboolean	connected;						// a loadgame will leave valid entities that
												// just don't have a connection yet

	// Values saved and restored from edicts when changing levels
	int			health;
	int			max_health;
	int			savedFlags;

	int			selected_item;
	int			inventory[MAX_ITEMS];

	// Ammo capacities
	int			max_bullets;
	int			max_shells;
	int			max_rockets;
	int			max_grenades;
	int			max_cells;
	int			max_slugs;

	gItem_t		*weapon;
	gItem_t		*lastweapon;

	int			power_cubes;					// used for tracking the cubes in coop games
	int			score;							// for calculating total unit score in coop games

	int			game_helpchanged;
	int			helpchanged;

	qboolean	spectator;						// client is a spectator
} client_persistant_t;

// Client data that stays across deathmatch respawns
typedef struct {
	client_persistant_t	coop_respawn;			// what to set client->pers to on a respawn
	int			enterframe;						// level.framenum the client entered the game
	int			score;							// frags, etc
	vec3_t		cmd_angles;						// angles sent over in the last command

	qboolean	spectator;						// client is a spectator
} client_respawn_t;

// This structure is cleared on each PutClientInServer(),
// except for 'client->pers'
struct gclient_s {
	// known to server
	player_state_t	ps;							// communicated by server to clients
	int				ping;

	// private to game
	client_persistant_t	pers;
	client_respawn_t	resp;
	pmove_state_t		old_pmove;				// for detecting out-of-pmove changes

	qboolean	showscores;						// set layout stat
	qboolean	showinventory;					// set layout stat
	qboolean	showhelp;
	qboolean	showhelpicon;

	int			ammo_index;

	int			buttons;
	int			oldbuttons;
	int			latched_buttons;

	qboolean	weaponThink;

	gItem_t		*newweapon;

	// sum up damage over an entire frame, so
	// shotgun blasts give a single big kick
	int			damage_armor;		// damage absorbed by armor
	int			damage_parmor;		// damage absorbed by power armor
	int			damage_blood;		// damage taken out of health
	int			damage_knockback;	// impact damage
	vec3_t		damage_from;		// origin for vector calculation

	float		killer_yaw;			// when dead, look at killer

	weaponState_t	weaponstate;
	vec3_t		kick_angles;	// weapon kicks
	vec3_t		kick_origin;
	float		v_dmg_roll, v_dmg_pitch, v_dmg_time;	// damage kicks
	float		fall_time, fall_value;		// for view drop on fall
	float		damage_alpha;
	float		bonus_alpha;
	vec3_t		damage_blend;
	vec3_t		v_angle;			// aiming direction
	float		bobtime;			// so off-ground doesn't change it
	vec3_t		oldviewangles;
	vec3_t		oldvelocity;

	float		next_drown_time;
	int			old_waterlevel;
	int			breather_sound;

	int			machinegun_shots;	// for weapon raising

	// animation vars
	int			anim_end;
	int			anim_priority;
	qboolean	anim_duck;
	qboolean	anim_run;

	// Powerup timers
	float		quad_framenum;
	float		invincible_framenum;
	float		breather_framenum;
	float		enviro_framenum;

	qboolean	grenade_blew_up;
	float		grenade_time;
	int			silencer_shots;
	int			weapon_sound;

	float		pickup_msg_time;

	float		flood_locktill;		// locked from talking
	float		flood_when[10];		// when messages were said
	int			flood_whenhead;		// head pointer for when said

	float		respawn_time;		// can respawn when time > this

	// Chase camera
	edict_t *			chaseTarget;
	qboolean			updateChase;
	int					chaseMode;
};

typedef enum {
	WATERLEVEL_NONE,
	WATERLEVEL_FEET,
	WATERLEVEL_WAIST,
	WATERLEVEL_HEAD
} waterLevel_t;

typedef struct {
	// Fixed data
	vec3_t				start_origin;
	vec3_t				start_angles;
	vec3_t				end_origin;
	vec3_t				end_angles;

	int					sound_start;
	int					sound_middle;
	int					sound_end;

	float				accel;
	float				speed;
	float				decel;
	float				distance;

	float				wait;

	// State data
	int					state;
	vec3_t				dir;
	float				current_speed;
	float				move_speed;
	float				next_speed;
	float				remaining_distance;
	float				decel_distance;
	void				(*endfunc)(edict_t *);
} moveInfo_t;

typedef struct {
	mmove_t *			currentmove;
	int					aiflags;
	int					nextframe;
	float				scale;

	void				(*stand)(edict_t *self);
	void				(*idle)(edict_t *self);
	void				(*search)(edict_t *self);
	void				(*walk)(edict_t *self);
	void				(*run)(edict_t *self);
	void				(*dodge)(edict_t *self, edict_t *other, float eta);
	void				(*attack)(edict_t *self);
	void				(*melee)(edict_t *self);
	void				(*sight)(edict_t *self, edict_t *other);
	qboolean			(*checkattack)(edict_t *self);

	float				pausetime;
	float				attack_finished;

	vec3_t				saved_goal;
	float				search_time;
	float				trail_time;
	vec3_t				last_sighting;
	int					attack_state;
	int					lefty;
	float				idle_time;
	int					linkcount;

	int					power_armor_type;
	int					power_armor_power;
} monsterInfo_t;

struct edict_s {
	entity_state_t	s;
	struct gclient_s	*client;	// NULL if not a player
	// the server expects the first part
	// of gclient_s to be a player_state_t
	// but the rest of it is opaque

	qboolean	inuse;
	int			linkcount;

	// FIXME: move these fields to a server private sv_entity_t
	link_t		area;				// linked to a division node or leaf

	int			num_clusters;		// if -1, use headnode instead
	int			clusternums[MAX_ENT_CLUSTERS];
	int			headnode;			// unused if num_clusters != -1
	int			areanum, areanum2;

	//================================

	int			svflags;
	vec3_t		mins, maxs;
	vec3_t		absmin, absmax, size;
	solid_t		solid;
	int			clipmask;
	edict_t		*owner;


	// DO NOT MODIFY ANYTHING ABOVE THIS, THE SERVER
	// EXPECTS THE FIELDS IN THAT ORDER!

	//================================
	int					movetype;
	int					flags;

	char *				model;
	float				freetime;			// sv.time when the object was freed

	// Only used locally in game, not by server
	char *				message;
	char *				className;
	int					spawnflags;

	float				timestamp;

	float				angle;			// set in qe3, -1 = up, -2 = down
	char *				target;
	char *				targetname;
	char *				killtarget;
	char *				team;
	char *				pathtarget;
	char *				deathtarget;
	char *				combattarget;
	edict_t *			target_ent;

	float				speed, accel, decel;
	vec3_t				movedir;
	vec3_t				pos1, pos2;

	vec3_t				velocity;
	vec3_t				avelocity;
	int					mass;
	float				air_finished;
	float				gravity;		// per entity gravity multiplier (1.0 is normal)
	// use for lowgrav artifact, flares

	// AI variables
	edict_t *			goalEntity;
	edict_t *			moveTarget;
	float				yawSpeed;
	float				idealYaw;

	// Callback functions
	float				nextthink;
	void				(*prethink) (edict_t *ent);
	void				(*think)(edict_t *self);
	void				(*blocked)(edict_t *self, edict_t *other);	// FIXME: Move to moveInfo?
	void				(*touch)(edict_t *self, edict_t *other, cplane_t *plane, csurface_t *surf);
	void				(*use)(edict_t *self, edict_t *other, edict_t *activator);
	void				(*pain)(edict_t *self, edict_t *other, float kick, int damage);
	void				(*die)(edict_t *self, edict_t *inflictor, edict_t *attacker, int damage, vec3_t point);

	float				touch_debounce_time;			// are all these legit?  do we need more/less of them?
	float				pain_debounce_time;
	float				damage_debounce_time;
	float				fly_sound_debounce_time;		//move to clientinfo
	float				last_move_time;

	int					health;
	int					max_health;
	int					gib_health;
	int					deadflag;
	qboolean			show_hostile;

	float				powerarmor_time;

	char *				map;					// target_changelevel

	int					viewheight;				// height above origin where eyesight is determined
	int					takedamage;
	int					dmg;
	int					radius_dmg;
	float				dmg_radius;
	int					sounds;					// make this a spawntemp var?
	int					count;

	edict_t *			chain;
	edict_t *			enemy;
	edict_t *			oldenemy;
	edict_t *			activator;
	edict_t *			groundentity;
	int					groundentity_linkcount;
	edict_t *			teamchain;
	edict_t *			teammaster;

	edict_t *			mynoise;				// Can go in client only
	edict_t *			mynoise2;

	int					noiseIndex;
	float				volume;
	float				attenuation;

	// Timing variables
	float				wait;
	float				delay;					// Before firing targets
	float				random;

	float				teleportTime;

	int					waterType;
	waterLevel_t		waterLevel;

	vec3_t				moveOrigin;
	vec3_t				moveAngles;

	// FIXME: Move this to clientinfo?
	int					lightLevel;

	int					style;					// Also used as areaportal number

	gItem_t *			item;					// For bonus items

	// Common data blocks
	moveInfo_t			moveInfo;
	monsterInfo_t		monsterinfo;
};

extern gameLocals_t		game;
extern levelLocals_t	level;
extern game_import_t	gi;
extern game_export_t	globals;
extern spawn_temp_t		st;

extern cvar_t *			maxentities;
extern cvar_t *			deathmatch;
extern cvar_t *			coop;
extern cvar_t *			dmflags;
extern cvar_t *			skill;
extern cvar_t *			fraglimit;
extern cvar_t *			timelimit;
extern cvar_t *			password;
extern cvar_t *			spectator_password;
extern cvar_t *			needpass;
extern cvar_t *			g_select_empty;
extern cvar_t *			dedicated;

extern cvar_t *			filterban;

extern cvar_t *			sv_gravity;
extern cvar_t *			sv_maxvelocity;

extern cvar_t *			gun_x;
extern cvar_t *			gun_y;
extern cvar_t *			gun_z;
extern cvar_t *			sv_rollspeed;
extern cvar_t *			sv_rollangle;

extern cvar_t *			run_pitch;
extern cvar_t *			run_roll;
extern cvar_t *			bob_up;
extern cvar_t *			bob_pitch;
extern cvar_t *			bob_roll;

extern cvar_t *			sv_cheats;
extern cvar_t *			maxclients;
extern cvar_t *			maxspectators;

extern cvar_t *			flood_msgs;
extern cvar_t *			flood_persecond;
extern cvar_t *			flood_waitdelay;

extern cvar_t *			sv_maplist;

extern cvar_t *			sg_allowMonsterSpawn;

void			SG_Framework ();

void			SG_InitGame();
void			SG_ShutdownGame ();