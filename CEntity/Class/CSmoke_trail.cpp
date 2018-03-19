
#include "CSmoke_trail.h"


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



CE_LINK_ENTITY_TO_CLASS(env_sporeexplosion, CE_SporeExplosion);

DEFINE_PROP(m_bDontRemove, CE_SporeExplosion );
DEFINE_PROP(m_bEmit, CE_SporeExplosion );
DEFINE_PROP(m_flSpawnRate, CE_SporeExplosion );

DEFINE_PROP(m_bDisabled, CE_SporeExplosion );



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

void CSmokeTrail::SetLifetime(float lifetime)
{
	if(lifetime == -1)
		SetNextThink( TICK_NEVER_THINK );
	else
		SetNextThink( gpGlobals->curtime + lifetime );
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
