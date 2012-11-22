//
// m_makron_teleport.c - Makron monster teleporting in Boss1
//


#include "g_local.h"
#include "m_boss32.h"



/*
 ==============================================================================

 TELEPORT EFFECT

 ==============================================================================
*/


/*
 ==================
 Makron_Trigger
 ==================
*/
static void Makron_Trigger (edict_t *entity, edict_t *other, edict_t *activator){

	gi.WriteByte(svc_temp_entity);
	gi.WriteByte(TE_BOSSTPORT);
	gi.WritePosition(entity->s.origin);
	gi.multicast(entity->s.origin, MULTICAST_PVS);

	SG_FreeEntity(entity);
}

/*
 ==================
 Makron_Stand_Think
 ==================
*/
static void Makron_Stand_Think (edict_t *entity){

	if (entity->s.frame == FRAME_stand260)
		entity->s.frame = FRAME_stand201;
	else
		entity->s.frame++;

	entity->nextthink = level.time + FRAMETIME;
}


/*
 ==============================================================================

 MONSTER_BOSS3_STAND

 ==============================================================================
*/


/*
 ==================
 SG_MonsterBoss3Stand_Spawn
 ==================
*/
void SG_MonsterBoss3Stand_Spawn (edict_t *self){

	if (deathmatch->value){
		SG_FreeEntity(self);
		return;
	}

	gi.soundindex("misc/bigtele.wav");

	self->model = "models/monsters/boss3/rider/tris.md2";
	self->s.modelindex = gi.modelindex(self->model);
	self->s.frame = FRAME_stand201;

	VectorSet(self->mins, -32, -32, 0);
	VectorSet(self->maxs, 32, 32, 90);
	self->solid = SOLID_BBOX;

	self->movetype = MOVETYPE_STEP;

	self->use = Makron_Trigger;

	self->think = Makron_Stand_Think;
	self->nextthink = level.time + FRAMETIME;

	gi.linkentity(self);
}