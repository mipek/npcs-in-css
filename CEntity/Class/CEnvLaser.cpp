
#include "CEnvLaser.h"
#include "CSprite.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



CE_LINK_ENTITY_TO_CLASS(CEnvLaser, CE_CEnvLaser);


DEFINE_PROP(m_pSprite, CE_CEnvLaser);
DEFINE_PROP(m_iszLaserTarget, CE_CEnvLaser);


void CE_CEnvLaser::TurnOn( void )
{
	RemoveEffects( EF_NODRAW );
	CE_CSprite *cent_m_pSprite = (CE_CSprite *)CEntity::Instance(m_pSprite);
	if ( cent_m_pSprite )
		cent_m_pSprite->TurnOn();

	m_flFireTime = gpGlobals->curtime;

	SetThink( &CE_CEnvLaser::StrikeThink );

	//
	// Call StrikeThink here to update the end position, otherwise we will see
	// the beam in the wrong place for one frame since we cleared the nodraw flag.
	//
	StrikeThink();
}

void CE_CEnvLaser::TurnOff( void )
{
	AddEffects( EF_NODRAW );
	CE_CSprite *cent_m_pSprite = (CE_CSprite *)CEntity::Instance(m_pSprite);
	if ( cent_m_pSprite )
		cent_m_pSprite->TurnOff();

	SetNextThink( TICK_NEVER_THINK );
	SetThink( NULL );
}

void CE_CEnvLaser::FireAtPoint( trace_t &tr )
{
	SetAbsEndPos( tr.endpos );
	CE_CSprite *cent_m_pSprite = (CE_CSprite *)CEntity::Instance(m_pSprite);
	if ( cent_m_pSprite )
	{
		UTIL_SetOrigin( cent_m_pSprite, tr.endpos );
	}

	// Apply damage and do sparks every 1/10th of a second.
	if ( gpGlobals->curtime >= m_flFireTime + 0.1 )
	{
		BeamDamage( &tr );
		DoSparks( GetAbsStartPos(), tr.endpos );
	}
}

void CE_CEnvLaser::StrikeThink( void )
{
	CEntity *pEnd = RandomTargetname( STRING( *m_iszLaserTarget ) );

	Vector vecFireAt = GetAbsEndPos();
	if ( pEnd )
	{
		vecFireAt = pEnd->GetAbsOrigin();
	}

	trace_t tr;

	UTIL_TraceLine( GetAbsOrigin(), vecFireAt, MASK_SOLID, NULL, COLLISION_GROUP_NONE, &tr );
	FireAtPoint( tr );
	SetNextThink( gpGlobals->curtime );
}

