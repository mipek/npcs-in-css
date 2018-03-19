#include "CEntity.h"
#include "physics.h"
#include "physics_impact_damage.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



float CalculateDefaultPhysicsDamage( int index, gamevcollisionevent_t *pEvent, float energyScale, bool allowStaticDamage, int &damageType, string_t iszDamageTableName, bool bDamageFromHeldObjects )
{
	return g_helpfunc.CalculateDefaultPhysicsDamage(index, pEvent, energyScale, allowStaticDamage, damageType, iszDamageTableName, bDamageFromHeldObjects);
}

void PhysCallbackDamage( CBaseEntity *pEntity, const CTakeDamageInfo &info, gamevcollisionevent_t &event, int hurtIndex )
{
	g_helpfunc.PhysCallbackDamage(pEntity, info, event, hurtIndex);
}