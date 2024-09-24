
#include "CParticleEntity.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CE_LINK_ENTITY_TO_CLASS(CBaseParticleEntity, CParticleEntity);


void CParticleEntity::SetLifetime(float lifetime)
{
	if(lifetime == -1)
		SetNextThink( TICK_NEVER_THINK );
	else
		SetNextThink( gpGlobals->curtime + lifetime );
}

void CParticleEntity::FollowEntity( CEntity *pEntity)
{
	BaseClass::FollowEntity( (pEntity)?pEntity->BaseEntity():NULL );
	SetLocalOrigin( vec3_origin );
}
