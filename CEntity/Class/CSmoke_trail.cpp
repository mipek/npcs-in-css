
#include "CSmoke_trail.h"
#include "CAnimating.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CE_LINK_ENTITY_TO_CLASS(env_smoketrail, CSmokeTrail);

DEFINE_PROP(m_StartColor, CSmokeTrail);
DEFINE_PROP(m_EndColor, CSmokeTrail );
DEFINE_PROP(m_Opacity, CSmokeTrail );
DEFINE_PROP(m_SpawnRate, CSmokeTrail );
DEFINE_PROP(m_ParticleLifetime, CSmokeTrail );
DEFINE_PROP(m_StopEmitTime, CSmokeTrail );
DEFINE_PROP(m_MinSpeed, CSmokeTrail );
DEFINE_PROP(m_MaxSpeed, CSmokeTrail );
DEFINE_PROP(m_StartSize, CSmokeTrail );
DEFINE_PROP(m_EndSize, CSmokeTrail );
DEFINE_PROP(m_SpawnRadius , CSmokeTrail);
DEFINE_PROP(m_MinDirectedSpeed, CSmokeTrail );
DEFINE_PROP(m_MaxDirectedSpeed, CSmokeTrail );
DEFINE_PROP(m_bEmit, CSmokeTrail );
DEFINE_PROP(m_nAttachment, CSmokeTrail );



CE_LINK_ENTITY_TO_CLASS(env_fire_trail, CE_CFireTrail);
DEFINE_PROP(m_nAttachment, CE_CFireTrail );



CE_LINK_ENTITY_TO_CLASS(env_sporeexplosion, CE_SporeExplosion);

DEFINE_PROP(m_bDontRemove, CE_SporeExplosion );
DEFINE_PROP(m_bEmit, CE_SporeExplosion );
DEFINE_PROP(m_flSpawnRate, CE_SporeExplosion );

DEFINE_PROP(m_bDisabled, CE_SporeExplosion );






CE_LINK_ENTITY_TO_CLASS(env_rockettrail, CRocketTrail);

DEFINE_PROP(m_bDamaged, CRocketTrail );
DEFINE_PROP(m_Opacity, CRocketTrail );
DEFINE_PROP(m_SpawnRate, CRocketTrail );
DEFINE_PROP(m_ParticleLifetime, CRocketTrail );
DEFINE_PROP(m_StartColor, CRocketTrail );
DEFINE_PROP(m_EndColor, CRocketTrail );
DEFINE_PROP(m_StartSize, CRocketTrail );
DEFINE_PROP(m_EndSize, CRocketTrail );
DEFINE_PROP(m_MinSpeed, CRocketTrail );
DEFINE_PROP(m_MaxSpeed, CRocketTrail );
DEFINE_PROP(m_nAttachment, CRocketTrail );
DEFINE_PROP(m_SpawnRadius, CRocketTrail );
DEFINE_PROP(m_bEmit, CRocketTrail );


CSmokeTrail* CSmokeTrail::CreateSmokeTrail()
{
	CEntity *cent = CreateEntityByName("env_smoketrail");
	if(cent)
	{
		CSmokeTrail *pSmoke = dynamic_cast<CSmokeTrail*>(cent);
		if(pSmoke)
		{
			pSmoke->Activate();
			return pSmoke;
		}
		else
		{
			UTIL_Remove(cent);
		}
	}

	return NULL;
}

void CSmokeTrail::FollowEntity( CEntity *pEntity, const char *pAttachmentName )
{
	// For attachments
	if ( pAttachmentName && pEntity && pEntity->GetBaseAnimating() )
	{
		m_nAttachment = pEntity->GetBaseAnimating()->LookupAttachment( pAttachmentName );
	}
	else
	{
		m_nAttachment = 0;
	}

	BaseClass::FollowEntity(pEntity);
}



CE_CFireTrail *CE_CFireTrail::CreateFireTrail( void )
{
	CEntity *pEnt = CreateEntityByName( "env_fire_trail" );

	if ( pEnt )
	{
		CE_CFireTrail *pTrail = dynamic_cast<CE_CFireTrail*>(pEnt);

		if ( pTrail )
		{
			pTrail->Activate();
			return pTrail;
		}
		else
		{
			UTIL_Remove( pEnt );
		}
	}

	return NULL;
}

void CE_CFireTrail::FollowEntity( CEntity *pEntity, const char *pAttachmentName )
{
	// For attachments
	if ( pAttachmentName && pEntity && pEntity->GetBaseAnimating() )
	{
		m_nAttachment = pEntity->GetBaseAnimating()->LookupAttachment( pAttachmentName );
	}
	else
	{
		m_nAttachment = 0;
	}

	BaseClass::FollowEntity( pEntity );
}


void CE_SporeExplosion::InputEnable( inputdata_t &inputdata )
{
	m_bDontRemove = true;
	m_bDisabled = false;
	m_bEmit = true;
}

void CE_SporeExplosion::InputDisable( inputdata_t &inputdata )
{
	m_bDontRemove = true;
	m_bDisabled = true;
	m_bEmit = false;
}





void CRocketTrail::FollowEntity( CEntity *pEntity, const char *pAttachmentName )
{
	// For attachments
	if ( pAttachmentName && pEntity && pEntity->GetBaseAnimating() )
	{
		m_nAttachment = pEntity->GetBaseAnimating()->LookupAttachment( pAttachmentName );
	}
	else
	{
		m_nAttachment = 0;
	}

	BaseClass::FollowEntity( pEntity );
}

void CRocketTrail::SetEmit(bool bVal)
{
	m_bEmit = bVal;
}

CRocketTrail* CRocketTrail::CreateRocketTrail()
{
	CEntity *pEnt = CreateEntityByName( "env_rockettrail" );

	if( pEnt != NULL )
	{
		CRocketTrail *pTrail = dynamic_cast<CRocketTrail*>(pEnt);
		Assert(pTrail);

		if( pTrail != NULL )
		{
			pTrail->Activate();
			return pTrail;
		}
		else
		{
			UTIL_Remove( pEnt );
		}
	}

	return NULL;
}
