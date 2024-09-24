
#include "CRagdollProp.h"
#include "CPlayer.h"
#include "physics_shared.h"


CE_LINK_ENTITY_TO_CLASS(CRagdollProp, CE_CRagdollProp);
CE_LINK_ENTITY_TO_CLASS(CRagdollPropAttached, CE_CRagdollPropAttached);

const char *s_pFadeOutContext = "RagdollFadeOutContext";



// Sendprop
DEFINE_PROP(m_flBlendWeight, CE_CRagdollProp);
DEFINE_PROP(m_nOverlaySequence, CE_CRagdollProp);


// Datamaps
DEFINE_PROP(m_ragdoll, CE_CRagdollProp);
DEFINE_PROP(m_hDamageEntity, CE_CRagdollProp);
DEFINE_PROP(m_flDefaultFadeScale, CE_CRagdollProp);
DEFINE_PROP(m_flFadeTime, CE_CRagdollProp);
DEFINE_PROP(m_flFadeOutStartTime, CE_CRagdollProp);





DEFINE_PROP(m_bShouldDetach, CE_CRagdollPropAttached);



#define	SF_RAGDOLLPROP_DEBRIS		0x0004
#define SF_RAGDOLLPROP_USE_LRU_RETIREMENT	0x1000
#define	SF_RAGDOLLPROP_ALLOW_DISSOLVE		0x2000	// Allow this prop to be dissolved
#define	SF_RAGDOLLPROP_MOTIONDISABLED		0x4000
#define	SF_RAGDOLLPROP_ALLOW_STRETCH		0x8000
#define	SF_RAGDOLLPROP_STARTASLEEP			0x10000




CE_CRagdollProp::CE_CRagdollProp()
{
}

void CE_CRagdollProp::SetDamageEntity( CEntity *pEntity )
{
	// Damage passing
	m_hDamageEntity.ptr->Set((pEntity)?pEntity->BaseEntity():NULL);

	// Set our takedamage to match it
	if ( pEntity )
	{
		*(m_takedamage) = pEntity->m_takedamage;
	}
	else
	{
		m_takedamage = DAMAGE_EVENTS_ONLY;
	}
}

void CE_CRagdollProp::SetOverlaySequence( Activity activity )
{
	int seq = SelectWeightedSequence( activity );
	if ( seq < 0 )
	{
		m_nOverlaySequence = -1;
	}
	else
	{
		m_nOverlaySequence = seq;
	}
}

void CE_CRagdollProp::DisableAutoFade()
{
	m_flFadeScale = 0;
	m_flDefaultFadeScale = 0;
}

void CE_CRagdollProp::RecheckCollisionFilter()
{
	for ( int i = 0; i < m_ragdoll->listCount; i++ )
	{
		m_ragdoll->list[i].pObject->RecheckCollisionFilter();
	}
}

#define FADE_OUT_LENGTH 0.5f

void CE_CRagdollProp::FadeOut( float flDelay, float fadeTime)
{
	if ( IsFading() )
		return;

	m_flFadeTime = ( fadeTime == -1 ) ? FADE_OUT_LENGTH : fadeTime;

	m_flFadeOutStartTime = gpGlobals->curtime + flDelay;
	m_flFadeScale = 0;
	SetContextThink( &CE_CRagdollProp::FadeOutThink, gpGlobals->curtime + flDelay + 0.01f, s_pFadeOutContext );

}

bool CE_CRagdollProp::IsFading()
{
	return ( GetNextThink( s_pFadeOutContext ) >= gpGlobals->curtime );
}

void CE_CRagdollProp::FadeOutThink()
{
	float dt = gpGlobals->curtime - m_flFadeOutStartTime;
	if ( dt < 0 )
	{
		SetContextThink( &CE_CRagdollProp::FadeOutThink, gpGlobals->curtime + 0.1, s_pFadeOutContext );
	}
	else if ( dt < m_flFadeTime )
	{
		float alpha = 1.0f - dt / m_flFadeTime;
		int nFade = (int)(alpha * 255.0f);
		m_nRenderMode = kRenderTransTexture;
		SetRenderColorA( nFade );
		NetworkStateChanged();
		SetContextThink( &CE_CRagdollProp::FadeOutThink, gpGlobals->curtime + TICK_INTERVAL, s_pFadeOutContext );
	}
	else
	{
		// Necessary to cause it to do the appropriate death cleanup
		// Yeah, the player may have nothing to do with it, but
		// passing NULL to TakeDamage causes bad things to happen
		CPlayer *pPlayer = nullptr;
		for(int i=1;i<=gpGlobals->maxClients;i++)
		{
			pPlayer = UTIL_PlayerByIndex(i);
			if(!pPlayer)
				continue;
		}

		CBaseEntity *inflictor = pPlayer ? pPlayer->BaseEntity() : NULL;
		if (!inflictor)
		{
			g_pSM->LogError(myself, "No player found! \"bad things\" will happen");
		}

		CTakeDamageInfo info( inflictor, inflictor, 10000.0, DMG_GENERIC );
		TakeDamage( info );
		UTIL_Remove( this );
	}
}






CE_CRagdollPropAttached::CE_CRagdollPropAttached()
{
}

void CE_CRagdollPropAttached::DetachOnNextUpdate()
{
	m_bShouldDetach = true;
}

void DetachAttachedRagdoll( CEntity *pRagdollIn )
{
	CE_CRagdollPropAttached *pRagdoll = dynamic_cast<CE_CRagdollPropAttached *>(pRagdollIn);

	if ( pRagdoll )
	{
		pRagdoll->DetachOnNextUpdate();
	}
}
