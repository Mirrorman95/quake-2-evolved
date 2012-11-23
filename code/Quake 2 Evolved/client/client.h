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
// client.c - Primary header for client
//


#ifndef __CLIENT_H__
#define __CLIENT_H__


#include "../common/common.h"
#include "../renderer/refresh.h"
#include "../sound/s_public.h"
#include "input.h"
#include "cinematic.h"
#include "../ui/ui_public.h"


#define DEFAULT_FOV					90.0f

// The cl.parseEntities array must be large enough to hold UPDATE_BACKUP
// frames of entities, so that when a delta compressed message arrives 
// from the server it can be un-deltaed from the original 
#define	MAX_PARSE_ENTITIES			1024

// Allow a lot of command backups for very fast systems
#define	CMD_BACKUP					64
#define CMD_MASK					(CMD_BACKUP - 1)

#define NUM_CROSSHAIRS				20

#define LAG_SAMPLES					256

#define MAX_CLIENTWEAPONMODELS		20

#define FPS_FRAMES					4

typedef struct {
	entity_state_t			baseline;		// Delta from this if not from a previous frame
	entity_state_t			current;
	entity_state_t			prev;			// Will always be valid, but might just be a copy of current

	int						serverFrame;	// If not current, this ent isn't in the frame

	vec3_t					lerpOrigin;		// For trails (variable hz)

	float					flashStartTime;	// Muzzle flash effect start time in seconds
	float					flashRotation;	// Muzzle flash effect rotation

	int						flyStopTime;
} centity_t;

typedef struct {
	bool					valid;			// Cleared if delta parsing was invalid
	int						serverFrame;
	int						serverTime;		// Server time the message is valid for (in msec)
	int						deltaFrame;
	byte					areaBits[MAX_MAP_AREAS/8];	// Portalarea visibility bits
	player_state_t			playerState;
	int						numEntities;
	int						parseEntitiesIndex;	// Non-masked index into cl.parseEntities array
} frame_t;

// The client precaches these files during level load
typedef struct {
	// Sounds
	sound_t *				sfxRichotecs[3];
	sound_t *				sfxSparks[3];
	sound_t *				sfxFootSteps[4];
	sound_t *				sfxLaserHit;
	sound_t *				sfxRailgun;
	sound_t *				sfxRocketExplosion;
	sound_t *				sfxGrenadeExplosion;
	sound_t *				sfxWaterExplosion;
	sound_t *				sfxMachinegunBrass;
	sound_t *				sfxShotgunBrass;
	sound_t *				sfxLightning;
	sound_t *				sfxDisruptorExplosion;

	// Models
	model_t *				parasiteBeamModel;
	model_t *				powerScreenShellModel;
	model_t *				machinegunBrassModel;
	model_t *				shotgunBrassModel;

	// Materials
	material_t *			levelshot;
	material_t *			levelshotDetail;
	material_t *			loadingLogo;
	material_t *			loadingDetail[2];
	material_t *			loadingPercent[20];
	material_t *			lagometerMaterial;
	material_t *			disconnectedMaterial;
	material_t *			backTileMaterial;
	material_t *			pauseMaterial;
	material_t *			crosshairMaterials[NUM_CROSSHAIRS];
	material_t *			hudNumberMaterials[2][11];
	material_t *			bloodBlendMaterial;
	material_t *			fireScreenMaterial;
	material_t *			waterBlurMaterial;
	material_t *			doubleVisionMaterial;
	material_t *			underWaterVisionMaterial;
	material_t *			irGogglesMaterial;
	material_t *			rocketExplosionMaterial;
	material_t *			rocketExplosionWaterMaterial;
	material_t *			grenadeExplosionMaterial;
	material_t *			grenadeExplosionWaterMaterial;
	material_t *			bfgExplosionMaterial;
	material_t *			bfgBallMaterial;
	material_t *			plasmaBallMaterial;
	material_t *			waterPlumeMaterial;
	material_t *			waterSprayMaterial;
	material_t *			waterWakeMaterial;
	material_t *			nukeShockwaveMaterial;
	material_t *			bloodSplatMaterial[2];
	material_t *			bloodCloudMaterial[2];
	material_t *			powerScreenShellMaterial;
	material_t *			invulnerabilityShellMaterial;
	material_t *			quadDamageShellMaterial;
	material_t *			doubleDamageShellMaterial;
	material_t *			halfDamageShellMaterial;
	material_t *			genericShellMaterial;
	material_t *			laserBeamMaterial;
	material_t *			grappleBeamMaterial;
	material_t *			lightningBeamMaterial;
	material_t *			heatBeamMaterial;
	material_t *			energyParticleMaterial;
	material_t *			glowParticleMaterial;
	material_t *			flameParticleMaterial;
	material_t *			smokeParticleMaterial;
	material_t *			liteSmokeParticleMaterial;
	material_t *			bubbleParticleMaterial;
	material_t *			dropletParticleMaterial;
	material_t *			steamParticleMaterial;
	material_t *			sparkParticleMaterial;
	material_t *			impactSparkParticleMaterial;
	material_t *			trackerParticleMaterial;
	material_t *			flyParticleMaterial;

	material_t *			energyMarkMaterial;
	material_t *			bulletMarkMaterial;
	material_t *			burnMarkMaterial;
	material_t *			bloodMarkMaterials[2][6];

	// Files referenced by the server that the client needs
	sound_t *				gameSounds[MAX_SOUNDS];
	model_t *				gameModels[MAX_MODELS];
	struct cmodel_s	*		gameCModels[MAX_MODELS];
	material_t *			gameMaterials[MAX_IMAGES];
} gameMedia_t;

typedef struct {
	int						dropped[LAG_SAMPLES];
	int						suppressed[LAG_SAMPLES];
	int						ping[LAG_SAMPLES];
	int						current;
} lagometer_t;

typedef struct {
	bool					valid;
	char					name[MAX_QPATH];
	char					info[MAX_QPATH];
	model_t *				model;
	material_t *			skin;
	material_t *			icon;
	model_t *				weaponModel[MAX_CLIENTWEAPONMODELS];
} clientInfo_t;

typedef struct {
	bool					active;

	renderEntity_t			renderEntity;
} testSprite_t;

typedef struct {
	bool					active;

	renderEntity_t			renderEntity;
} testBeam_t;

typedef struct {
	bool					active;

	bool					isGun;

	char					name[MAX_QPATH];

	int						time;
	int						frames;

	renderEntity_t			renderEntity;
} testModel_t;

// The clientState_t structure is wiped completely at every server map
// change
typedef struct {
	entity_state_t			parseEntities[MAX_PARSE_ENTITIES];
	int						parseEntitiesIndex;	// Index (not anded off) into parseEntities

	centity_t				entities[MAX_EDICTS];

	usercmd_t				cmds[CMD_BACKUP];	// Each mesage will send several old cmds
	int						cmdTime[CMD_BACKUP];	// Time sent, for calculating pings

	short					predictedOrigins[CMD_BACKUP][3];	// For debug comparing against server
	vec3_t					predictedOrigin;	// Generated by CL_PredictMovement
	vec3_t					predictedAngles;
	float					predictedStep;		// For stair up smoothing
	uint					predictedStepTime;
	vec3_t					predictedError;

	frame_t					frame;				// Received from server
	frame_t					frames[UPDATE_BACKUP];

	int						suppressCount;		// Number of messages rate suppressed
	int						timeOutCount;

	// The client maintains its own idea of view angles, which are sent
	// to the server each frame. It is cleared to 0 upon entering each 
	// level.
	// The server sends a delta each frame which is added to the locally
	// tracked view angles to account for standing on rotating objects,
	// and teleport direction changes.
	vec3_t					viewAngles;

	int						time;				// This is the time value that the client is rendering at

	float					lerpFrac;			// Between oldFrame and frame

	player_state_t *		playerState;
	player_state_t *		oldPlayerState;

	// View rendering
	renderView_t			renderView;
	vec3_t					renderViewAngles;

	int						timeDemoStart;
	int						timeDemoFrames;

	gameMedia_t				media;				// Precache

	clientInfo_t			clientInfo[MAX_CLIENTS];
	clientInfo_t			baseClientInfo;

	char					weaponModels[MAX_CLIENTWEAPONMODELS][MAX_QPATH];
	int						numWeaponModels;

	// Development tools
	testSprite_t			testSprite;
	testBeam_t				testBeam;
	testModel_t				testModel;

	lagometer_t				lagometer;

	// View blends
	int						doubleVisionEndTime;
	bool					underwater;
	int						underwaterVisionEndTime;
	int						fireScreenEndTime;

	// Crosshair names
	int						crosshairEntNumber;
	int						crosshairEntTime;

	// Zoom key
	bool					zooming;
	int						zoomTime;
	float					zoomSensitivity;

	// Non-gameserver information
	char					centerPrint[1024];
	int						centerPrintTime;

	// Transient data from server
	char					layout[1024];		// General 2D overlay
	int						inventory[MAX_ITEMS];

	// Server state information
	int						serverCount;		// Server identification for prespawns
	bool					demoPlayback;		// Running a demo, any key will disconnect
	bool					gameMod;
	char					gameDir[MAX_QPATH];
	int						clientNum;
	bool					multiPlayer;

	char					configStrings[MAX_CONFIGSTRINGS][MAX_QPATH];
} clientState_t;

typedef enum {
	CA_UNINITIALIZED,	// Only before initialization or for dedicated servers
	CA_DISCONNECTED,	// Not talking to a server
	CA_CONNECTING,		// Sending request packets to the server
	CA_CHALLENGING,		// Sending challenge packets to the server
	CA_CONNECTED,		// netChan_t established, getting game state
	CA_LOADING,			// Only during level load, never during main loop
	CA_PRIMED,			// Got game state, waiting for first frame
	CA_ACTIVE			// Game views should be displayed
} connState_t;

// The client precaches these files during initialization
typedef struct {
	material_t *			whiteMaterial;
	material_t *			consoleMaterial;
	material_t *			charsetMaterial;
} media_t;

typedef struct {
	char					map[MAX_QPATH];
	char					name[MAX_QPATH];

	char					string[128];
	int						percent;
} loadingInfo_t;

// The clientStatic_t structure is persistant through an arbitrary 
// number of server connections
typedef struct {
	connState_t				state;

	int						realTime;			// Always increasing, no clamping, etc...
	float					frameTime;			// Seconds since last frame

	bool					fullyInitialized;	// True if all the subsystems are initialized

	int						fpsIndex;
	int						fpsTimes[FPS_FRAMES];

	// Screen rendering information
	bool					screenDisabled;		// Don't update screen if true

	glConfig_t				glConfig;
	alConfig_t				alConfig;

	media_t					media;				// Precache

	// Loading screen information
	bool					loading;
	loadingInfo_t			loadingInfo;

	// Connection information
	char					serverName[128];	// Name of server from original connect
	netAdr_t				serverAddress;		// Address of server from original connect
	int						serverChallenge;	// From the server to use for connecting
	char					serverMessage[128];	// Connection refused message from server
	int						serverProtocol;		// In case we are doing some kind of version hack
	float					connectTime;		// For connection retransmits
	int						connectCount;		// Connection retransmits count
	netChan_t				netChan;

	// File transfer from server
	fileHandle_t			downloadFile;
	int						downloadStart;
	int						downloadBytes;
	int						downloadPercent;
	char					downloadName[MAX_QPATH];
	char					downloadTempName[MAX_QPATH];

	// Demo recording information
	fileHandle_t			demoFile;
	bool					demoWaiting;
	char					demoName[MAX_QPATH];

	// Cinematic information
	bool					playingCinematic;
} clientStatic_t;

// =====================================================================

// Flags for string drawing functions
#define DSF_FORCECOLOR				1
#define DSF_DROPSHADOW				2
#define DSF_LEFT					4
#define DSF_CENTER					8
#define DSF_RIGHT					16
#define DSF_LOWERCASE				32
#define DSF_UPPERCASE				64

typedef struct {
	int						down[2];			// Key nums holding it down
	uint					downTime;			// Msec timestamp
	uint					msec;				// Msec down this frame
	int						state;
} keyButton_t;

extern clientState_t		cl;
extern clientStatic_t		cls;

extern keyButton_t			in_up;
extern keyButton_t			in_down;
extern keyButton_t			in_left;
extern keyButton_t			in_right;
extern keyButton_t			in_forward;
extern keyButton_t			in_back;
extern keyButton_t			in_lookUp;
extern keyButton_t			in_lookDown;
extern keyButton_t			in_strafe;
extern keyButton_t			in_moveLeft;
extern keyButton_t			in_moveRight;
extern keyButton_t			in_speed;
extern keyButton_t			in_attack;
extern keyButton_t			in_use;
extern keyButton_t			in_kLook;
extern int					in_impulse;

extern char	*				svc_strings[256];

extern cvar_t *				cl_skipRendering;
extern cvar_t *				cl_hand;
extern cvar_t *				cl_zoomFov;
extern cvar_t *				cl_drawGun;
extern cvar_t *				cl_drawShells;
extern cvar_t *				cl_footSteps;
extern cvar_t *				cl_noSkins;
extern cvar_t *				cl_predict;
extern cvar_t *				cl_maxFPS;
extern cvar_t *				cl_freeLook;
extern cvar_t *				cl_lookSpring;
extern cvar_t *				cl_lookStrafe;
extern cvar_t *				cl_upSpeed;
extern cvar_t *				cl_forwardSpeed;
extern cvar_t *				cl_sideSpeed;
extern cvar_t *				cl_yawSpeed;
extern cvar_t *				cl_pitchSpeed;
extern cvar_t *				cl_angleSpeedKey;
extern cvar_t *				cl_run;
extern cvar_t *				cl_noDelta;
extern cvar_t *				cl_showNet;
extern cvar_t *				cl_showMiss;
extern cvar_t *				cl_showMaterial;
extern cvar_t *				cl_timeOut;
extern cvar_t *				cl_visibleWeapons;
extern cvar_t *				cl_thirdPerson;
extern cvar_t *				cl_thirdPersonRange;
extern cvar_t *				cl_thirdPersonAngle;
extern cvar_t *				cl_viewBlend;
extern cvar_t *				cl_particles;
extern cvar_t *				cl_particleLOD;
extern cvar_t *				cl_particleBounce;
extern cvar_t *				cl_particleFriction;
extern cvar_t *				cl_particleVertexLight;
extern cvar_t *				cl_markTime;
extern cvar_t *				cl_brassTime;
extern cvar_t *				cl_blood;
extern cvar_t *				cl_testGunX;
extern cvar_t *				cl_testGunY;
extern cvar_t *				cl_testGunZ;
extern cvar_t *				cl_testModelAnimate;
extern cvar_t *				cl_testModelRotate;
extern cvar_t *				cl_stereoSeparation;
extern cvar_t *				cl_drawCrosshair;
extern cvar_t *				cl_crosshairX;
extern cvar_t *				cl_crosshairY;
extern cvar_t *				cl_crosshairSize;
extern cvar_t *				cl_crosshairColor;
extern cvar_t *				cl_crosshairAlpha;
extern cvar_t *				cl_crosshairHealth;
extern cvar_t *				cl_crosshairNames;
extern cvar_t *				cl_viewSize;
extern cvar_t *				cl_centerTime;
extern cvar_t *				cl_drawGodModeShell;
extern cvar_t *				cl_drawCenterString;
extern cvar_t *				cl_drawPause;
extern cvar_t *				cl_drawFPS;
extern cvar_t *				cl_drawLagometer;
extern cvar_t *				cl_drawDisconnected;
extern cvar_t *				cl_drawRecording;
extern cvar_t *				cl_draw2D;
extern cvar_t *				cl_drawIcons;
extern cvar_t *				cl_drawStatus;
extern cvar_t *				cl_drawInventory;
extern cvar_t *				cl_drawLayout;
extern cvar_t *				cl_newHUD;
extern cvar_t *				cl_allowDownload;
extern cvar_t *				cl_rconPassword;
extern cvar_t *				l_rconAddress;
extern cvar_t *				cl_itemBob;

/*
 ==============================================================================

 CONSOLE

 ==============================================================================
*/

// Clears notify messages
void			Con_ClearNotify ();

// Prints text to the console.
// Handles cursor positioning, line wrapping, colored text, etc.
void			Con_Print (const char *text);

// Toggles the console on/off
void			Con_Toggle ();

// Forces the console off
void			Con_Close ();

// Handles key events
void			Con_KeyEvent (int key, bool down);

// Handles char events
void			Con_CharEvent (int ch);

// Handles key events for chat messages
void			Con_KeyMessageEvent (int key, bool down);

// Handles char events for chat messages
void			Con_CharMessageEvent (int ch);

// Redraws the console, or notify lines if toggled off
void			Con_Redraw ();

// Initializes the console
void			Con_Init ();

// Shuts down the console
void			Con_Shutdown ();

/*
 ==============================================================================

 KEYS

 ==============================================================================
*/

// These are the key numbers that should be passed to key event handlers.
// Normal keys should be passed as lowercased ASCII.
typedef enum {
	K_TAB					= 9,
	K_ENTER					= 13,
	K_ESCAPE				= 27,
	K_SPACE					= 32,
	K_BACKSPACE				= 127,

	K_UPARROW,
	K_DOWNARROW,
	K_LEFTARROW,
	K_RIGHTARROW,

	K_INSERT,
	K_DELETE,
	K_HOME,
	K_END,
	K_PAGEUP,
	K_PAGEDOWN,

	K_LALT,
	K_RALT,
	K_LCTRL,
	K_RCTRL,
	K_LSHIFT,
	K_RSHIFT,

	K_LWIN,
	K_RWIN,
	K_MENU,

	K_F1,
	K_F2,
	K_F3,
	K_F4,
	K_F5,
	K_F6,
	K_F7,
	K_F8,
	K_F9,
	K_F10,
	K_F11,
	K_F12,

	K_PAUSE,
	K_CAPSLOCK,
	K_SCROLLLOCK,
	K_PRINTSCREEN,

	K_KP_HOME,
	K_KP_UPARROW,
	K_KP_PAGEUP,
	K_KP_LEFTARROW,
	K_KP_5,
	K_KP_RIGHTARROW,
	K_KP_END,
	K_KP_DOWNARROW,
	K_KP_PAGEDOWN,
	K_KP_INSERT,
	K_KP_DELETE,
	K_KP_SLASH,
	K_KP_STAR,
	K_KP_MINUS,
	K_KP_PLUS,
	K_KP_ENTER,
	K_KP_NUMLOCK,

	K_MOUSE1,
	K_MOUSE2,
	K_MOUSE3,
	K_MOUSE4,
	K_MOUSE5,
	K_MOUSE6,
	K_MOUSE7,
	K_MOUSE8,

	K_MWHEELUP,
	K_MWHEELDOWN,

	K_CTRL_POV1,
	K_CTRL_POV2,
	K_CTRL_POV3,
	K_CTRL_POV4,
	K_CTRL_POV5,
	K_CTRL_POV6,
	K_CTRL_POV7,
	K_CTRL_POV8,
	K_CTRL_POV9,
	K_CTRL_POV10,
	K_CTRL_POV11,
	K_CTRL_POV12,
	K_CTRL_POV13,
	K_CTRL_POV14,
	K_CTRL_POV15,
	K_CTRL_POV16,

	K_CTRL_BUTTON1,
	K_CTRL_BUTTON2,
	K_CTRL_BUTTON3,
	K_CTRL_BUTTON4,
	K_CTRL_BUTTON5,
	K_CTRL_BUTTON6,
	K_CTRL_BUTTON7,
	K_CTRL_BUTTON8,
	K_CTRL_BUTTON9,
	K_CTRL_BUTTON10,
	K_CTRL_BUTTON11,
	K_CTRL_BUTTON12,
	K_CTRL_BUTTON13,
	K_CTRL_BUTTON14,
	K_CTRL_BUTTON15,
	K_CTRL_BUTTON16,
	K_CTRL_BUTTON17,
	K_CTRL_BUTTON18,
	K_CTRL_BUTTON19,
	K_CTRL_BUTTON20,
	K_CTRL_BUTTON21,
	K_CTRL_BUTTON22,
	K_CTRL_BUTTON23,
	K_CTRL_BUTTON24,
	K_CTRL_BUTTON25,
	K_CTRL_BUTTON26,
	K_CTRL_BUTTON27,
	K_CTRL_BUTTON28,
	K_CTRL_BUTTON29,
	K_CTRL_BUTTON30,
	K_CTRL_BUTTON31,
	K_CTRL_BUTTON32,

	K_X360_DPAD_UP,
	K_X360_DPAD_DOWN,
	K_X360_DPAD_LEFT,
	K_X360_DPAD_RIGHT,
	K_X360_START,
	K_X360_BACK,
	K_X360_LEFT_THUMB,
	K_X360_RIGHT_THUMB,
	K_X360_LEFT_SHOULDER,
	K_X360_RIGHT_SHOULDER,
	K_X360_LEFT_TRIGGER,
	K_X360_RIGHT_TRIGGER,
	K_X360_A,
	K_X360_B,
	K_X360_X,
	K_X360_Y,

	MAX_KEYS				= 256
} keyNum_t;

// In order from highest priority to lowest.
// If none of the catchers are active, bound key strings will be executed.
typedef enum {
	KEY_NONE = -1,
	KEY_GAME,
	KEY_CONSOLE,
	KEY_MESSAGE,
	KEY_MENU
} keyDest_t;

// Returns a key number to be used to index a key binding by looking at the
// given string. Single ASCII characters return themselves, while the K_* names
// are matched up.
int				Key_StringToKeyNum (const char *string);

// Returns a string (either a single ASCII char, or a K_* name) for the given
// key number
const char *	Key_KeyNumToString (int keyNum);

// Gets a key binding
const char *	Key_GetBinding (int keyNum);

// Sets a key binding
void			Key_SetBinding (int keyNum, const char *binding);

// Writes lines containing "bind <key> <command>" to the given file for all the
// active key bindings.
// Used for config files.
void			Key_WriteBindings (fileHandle_t f);

// Gets the current key destinations
keyDest_t		Key_GetKeyDest ();

// Sets the current key destinations
void			Key_SetKeyDest (keyDest_t dest);

// Returns true if the given key is down
bool			Key_IsDown (int key);

// Returns true if any key is down
bool			Key_IsAnyDown ();

// Clears key states
void			Key_ClearStates ();

// Handles key events
void			Key_Event (int key, int time, bool down);

// Handles char events
void			Key_CharEvent (int ch);

// Initializes key binding/input
void			Key_Init ();

// Shuts down key binding/input
void			Key_Shutdown ();

/*
 ==============================================================================

 DEMO RECORDING

 ==============================================================================
*/

void			CL_WriteDemoMessage ();

void			CL_Record_f ();
void			CL_StopRecord_f ();

/*
 ==============================================================================

 EFFECTS

 ==============================================================================
*/

void			CL_ClearLightStyles ();
void			CL_RunLightStyles ();
void			CL_AddLightStyles ();
void			CL_SetLightStyle (int style);

void			CL_ClearDynamicLights ();
void			CL_AddDynamicLights ();
void			CL_DynamicLight (vec3_t origin, float intensity, float r, float g, float b, bool fade, int duration);

void			CL_ParsePlayerMuzzleFlash ();
void			CL_ParseMonsterMuzzleFlash ();

/*
 ==============================================================================

 ENTITY FRAME PARSING

 ==============================================================================
*/

void			CL_ParseBaseLine ();
void			CL_ParseFrame ();

void			CL_AddPacketEntities ();
void			CL_AddViewWeapon ();

void			CL_GetEntitySoundSpatialization (int entity, vec3_t origin, vec3_t velocity);

/*
 ==============================================================================

 INPUT BUTTON EVENTS

 ==============================================================================
*/

float			IN_KeyButtonState (keyButton_t *b);

void			CL_CenterView_f ();
void			CL_ZoomDown_f ();
void			CL_ZoomUp_f ();
void			CL_UpDown_f ();
void			CL_UpUp_f ();
void			CL_DownDown_f ();
void			CL_DownUp_f ();
void			CL_LeftDown_f ();
void			CL_LeftUp_f ();
void			CL_RightDown_f ();
void			CL_RightUp_f ();
void			CL_ForwardDown_f ();
void			CL_ForwardUp_f ();
void			CL_BackDown_f ();
void			CL_BackUp_f ();
void			CL_LookUpDown_f ();
void			CL_LookUpUp_f ();
void			CL_LookDownDown_f ();
void			CL_LookDownUp_f ();
void			CL_StrafeDown_f ();
void			CL_StrafeUp_f ();
void			CL_MoveLeftDown_f ();
void			CL_MoveLeftUp_f ();
void			CL_MoveRightDown_f ();
void			CL_MoveRightUp_f ();
void			CL_SpeedDown_f ();
void			CL_SpeedUp_f ();
void			CL_AttackDown_f ();
void			CL_AttackUp_f ();
void			CL_UseDown_f ();
void			CL_UseUp_f ();
void			CL_KLookDown_f ();
void			CL_KLookUp_f ();
void			CL_Impulse_f ();

/*
 ==============================================================================

 USER COMMAND SENT TO THE SERVER

 ==============================================================================
*/

void			CL_BuildUserCmd ();

/*
 ==============================================================================

 ASSETS LOADING

 ==============================================================================
*/

void			CL_Loading ();

void			CL_LoadClientInfo (clientInfo_t *ci, const char *string);
void			CL_LoadGameMedia ();

/*
 ==============================================================================

 LOCAL ENTITY EFFECTS

 ==============================================================================
*/

void			CL_Explosion (const vec3_t org, const vec3_t dir, float radius, float rotation, float light, float lightRed, float lightGreen, float lightBlue, material_t *material);
void			CL_WaterSplash (const vec3_t org, const vec3_t dir);
void			CL_ExplosionWaterSplash (const vec3_t org);
void			CL_Sprite (const vec3_t org, float radius, material_t *material);
void			CL_LaserBeam (const vec3_t start, const vec3_t end, int width, int color, byte alpha, int duration, material_t *material);
void			CL_MachinegunEjectBrass (const centity_t *cent, int count, float x, float y, float z);
void			CL_ShotgunEjectBrass (const centity_t *cent, int count, float x, float y, float z);
void			CL_Bleed (const vec3_t org, const vec3_t dir, int count, bool green);
void			CL_BloodTrail (const vec3_t start, const vec3_t end, bool green);
void			CL_NukeShockwave (const vec3_t org);

void			CL_ClearLocalEntities ();
void			CL_AddLocalEntities ();

/*
 ==============================================================================

 PRIMARY CLIENT FILE

 ==============================================================================
*/

void			CL_ClearState ();
void			CL_ClearMemory ();
void			CL_Startup ();
void			CL_Disconnect (bool shuttingDown);
void			CL_PlayBackgroundTrack ();
void			CL_RequestNextDownload ();

/*
 ==============================================================================

 SERVER MESSAGE PARSING

 ==============================================================================
*/

void			CL_ShowNet (int level, const char *fmt, ...);
void			CL_ParseServerMessage (void);

/*
 ==============================================================================

 PARTICLE EFFECTS

 ==============================================================================
*/

void			CL_BlasterTrail (const vec3_t start, const vec3_t end, float r, float g, float b);
void			CL_GrenadeTrail (const vec3_t start, const vec3_t end);
void			CL_RocketTrail (const vec3_t start, const vec3_t end);
void			CL_RailTrail (const vec3_t start, const vec3_t end);
void			CL_BFGTrail (const vec3_t start, const vec3_t end);
void			CL_HeatBeamTrail (const vec3_t start, const vec3_t forward);
void			CL_TrackerTrail (const vec3_t start, const vec3_t end);
void			CL_TagTrail (const vec3_t start, const vec3_t end);
void			CL_BubbleTrail (const vec3_t start, const vec3_t end, float dist, float radius);
void			CL_FlagTrail (const vec3_t start, const vec3_t end, float r, float g, float b);
void			CL_BlasterParticles (const vec3_t origin, const vec3_t dir, float r, float g, float b);
void			CL_BulletParticles (const vec3_t origin, const vec3_t dir);
void			CL_ExplosionParticles (const vec3_t origin);
void			CL_BFGExplosionParticles (const vec3_t origin);
void			CL_TrackerExplosionParticles (const vec3_t origin);
void			CL_SmokePuffParticles (const vec3_t origin, float radius, int count);
void			CL_BubbleParticles (const vec3_t origin, int count, float magnitude);
void			CL_SparkParticles (const vec3_t origin, const vec3_t dir, int count);
void			CL_DamageSparkParticles (const vec3_t origin, const vec3_t dir, int count, int color);
void			CL_LaserSparkParticles (const vec3_t origin, const vec3_t dir, int count, int color);
void			CL_SplashParticles (const vec3_t origin, const vec3_t dir, int count, float magnitude, float spread);
void			CL_LavaSteamParticles (const vec3_t origin, const vec3_t dir, int count);
void			CL_FlyParticles (const vec3_t origin, int count);
void			CL_TeleportParticles (const vec3_t origin);
void			CL_BigTeleportParticles (const vec3_t origin);
void			CL_TeleporterParticles (const vec3_t origin);
void			CL_TrapParticles (const vec3_t origin);
void			CL_LogParticles (const vec3_t origin, float r, float g, float b);
void			CL_ItemRespawnParticles (const vec3_t origin);
void			CL_TrackerShellParticles (const vec3_t origin);
void			CL_NukeSmokeParticles (const vec3_t origin);
void			CL_WeldingSparkParticles (const vec3_t origin, const vec3_t dir, int count, int color);
void			CL_TunnelSparkParticles (const vec3_t origin, const vec3_t dir, int count, int color);
void			CL_ForceWallParticles (const vec3_t start, const vec3_t end, int color);
void			CL_SteamParticles (const vec3_t origin, const vec3_t dir, int count, int color, float magnitude);

void			CL_ClearParticles ();
void			CL_AddParticles ();

/*
 ==============================================================================

 PLAYER MOVEMENT PREDICTION

 ==============================================================================
*/

void			CL_BuildSolidList ();
trace_t			CL_Trace (const vec3_t start, const vec3_t mins, const vec3_t maxs, const vec3_t end, int skipNumber, int brushMask, bool brushOnly, int *entNumber);
int				CL_PointContents (const vec3_t point, int skipNumber);

void			CL_CheckPredictionError ();
void			CL_PredictMovement ();

/*
 ==============================================================================

 DRAWING

 ==============================================================================
*/

float *			CL_FadeColor (const vec4_t color, int startTime, int totalTime, int fadeTime);
float *			CL_FadeAlpha (const vec4_t color, int startTime, int totalTime, int fadeTime);
float *			CL_FadeColorAndAlpha (const vec4_t color, int startTime, int totalTime, int fadeTime);
void			CL_FillRect (float x, float y, float w, float h, horzAdjust_t horzAdjust, float horzPercent, vertAdjust_t vertAdjust, float vertPercent);
void			CL_DrawString (float x, float y, float w, float h, float width, const char *string, const color_t color, material_t *fontMaterial, int flags);
void			CL_DrawStringSheared (float x, float y, float w, float h, float shearX, float shearY, float width, const char *string, const color_t color, material_t *fontMaterial, int flags);
void			CL_DrawStringFixed (float x, float y, float w, float h, float width, const char *string, const color_t color, material_t *fontMaterial, int flags);
void			CL_DrawStringShearedFixed (float x, float y, float w, float h, float shearX, float shearY, float width, const char *string, const color_t color, material_t *fontMaterial, int flags);
void			CL_DrawPic (float x, float y, float w, float h, const vec4_t color, material_t *material);
void			CL_DrawPicST (float x, float y, float w, float h, float s1, float t1, float s2, float t2, const vec4_t color, material_t *material);
void			CL_DrawPicSheared (float x, float y, float w, float h, float shearX, float shearY, const color_t color, material_t *material);
void			CL_DrawPicShearedST (float x, float y, float w, float h, float s1, float t1, float s2, float t2, float shearX, float shearY, const color_t color, material_t *material);
void			CL_DrawPicByName (float x, float y, float w, float h, const vec4_t color, const char *pic);
void			CL_DrawPicFixed (float x, float y, material_t *material);
void			CL_DrawPicFixedByName (float x, float y, const char *pic);

void			CL_DrawLoading ();

void			CL_Draw2D ();

/*
 ==============================================================================

 SCREEN DRAWING

 ==============================================================================
*/

void			CL_PlayCinematic (const char *name);
void			CL_StopCinematic ();
void			CL_FinishCinematic ();
void			CL_DrawCinematic ();

void			CL_UpdateScreen ();

/*
 ==============================================================================

 TEMPORARY ENTITY PARSING

 ==============================================================================
*/

void			CL_ParseTempEntity ();
void			CL_ClearTempEntities ();
void			CL_AddTempEntities ();

/*
 ==============================================================================

 VIEW DRAWING

 ==============================================================================
*/

void			CL_TestSprite_f ();
void			CL_TestBeam_f ();
void			CL_TestSound_f ();
void			CL_TestDecal_f ();
void			CL_TestModel_f ();
void			CL_TestGun_f ();
void			CL_TestMaterial_f ();
void			CL_TestMaterialParm_f ();
void			CL_NextFrame_f ();
void			CL_PrevFrame_f ();
void			CL_NextSkin_f ();
void			CL_PrevSkin_f ();
void			CL_Where_f ();
void			CL_SizeUp_f ();
void			CL_SizeDown_f ();

void			CL_RenderActiveFrame ();


#endif	// __CLIENT_H__