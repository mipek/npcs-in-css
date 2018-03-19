//========= Copyright ?1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "CEntity.h"
#include "CAI_utils.h"
#include "CAI_memory.h"
#include "CAI_NPC.h"
#include "CAI_moveprobe.h"
#include "vphysics/object_hash.h"



// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"




BEGIN_SIMPLE_DATADESC( CAI_MoveMonitor )
	DEFINE_FIELD( m_vMark, FIELD_POSITION_VECTOR ), 
	DEFINE_FIELD( m_flMarkTolerance, FIELD_FLOAT )
END_DATADESC()


//-----------------------------------------------------------------------------

BEGIN_SIMPLE_DATADESC( CAI_ShotRegulator )
	DEFINE_FIELD( m_flNextShotTime, FIELD_TIME ),
	DEFINE_FIELD( m_bInRestInterval, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_nBurstShotsRemaining, FIELD_SHORT ),
	DEFINE_FIELD( m_nMinBurstShots, FIELD_SHORT ),
	DEFINE_FIELD( m_nMaxBurstShots, FIELD_SHORT ),
	DEFINE_FIELD( m_flMinRestInterval, FIELD_FLOAT ),
	DEFINE_FIELD( m_flMaxRestInterval, FIELD_FLOAT ),
	DEFINE_FIELD( m_flMinBurstInterval, FIELD_FLOAT ),
	DEFINE_FIELD( m_flMaxBurstInterval, FIELD_FLOAT ),
	DEFINE_FIELD( m_bDisabled, FIELD_BOOLEAN ),
END_DATADESC()


bool CAI_ShotRegulator::IsInRestInterval() const
{
	return (m_bInRestInterval && !ShouldShoot()); 
}


//-----------------------------------------------------------------------------
// Should we shoot?
//-----------------------------------------------------------------------------
bool CAI_ShotRegulator::ShouldShoot() const
{ 
	return ( !m_bDisabled && (m_flNextShotTime <= gpGlobals->curtime) ); 
}


//-----------------------------------------------------------------------------
// How much time should I wait in between shots in a single burst?
//-----------------------------------------------------------------------------
void CAI_ShotRegulator::SetBurstInterval( float flMinBurstInterval, float flMaxBurstInterval )
{
	m_flMinBurstInterval = flMinBurstInterval;
	m_flMaxBurstInterval = flMaxBurstInterval;
}


//-----------------------------------------------------------------------------
// How much time should I rest between bursts?
//-----------------------------------------------------------------------------
void CAI_ShotRegulator::SetRestInterval( float flMinRestInterval, float flMaxRestInterval )
{
	m_flMinRestInterval = flMinRestInterval;
	m_flMaxRestInterval = flMaxRestInterval;
}


//-----------------------------------------------------------------------------
// Sets the number of shots to shoot in a single burst
//-----------------------------------------------------------------------------
void CAI_ShotRegulator::SetBurstShotCountRange( int minShotsPerBurst, int maxShotsPerBurst )
{
	m_nMinBurstShots = minShotsPerBurst;
	m_nMaxBurstShots = maxShotsPerBurst;
}

void CAI_ShotRegulator::GetBurstShotCountRange( int *pMinShotsPerBurst, int *pMaxShotsPerBurst ) const
{
	*pMinShotsPerBurst = m_nMinBurstShots;
	*pMaxShotsPerBurst = m_nMaxBurstShots;
}

void CAI_ShotRegulator::GetRestInterval( float *pMinRestInterval, float *pMaxRestInterval ) const
{
	*pMinRestInterval = m_flMinRestInterval;
	*pMaxRestInterval = m_flMaxRestInterval;
}

void CAI_ShotRegulator::GetBurstInterval( float *pMinBurstInterval, float *pMaxBurstInterval ) const
{
	*pMinBurstInterval = m_flMinBurstInterval;
	*pMaxBurstInterval = m_flMaxBurstInterval;
}

int CAI_ShotRegulator::GetBurstShotsRemaining() const				
{ 
	return m_nBurstShotsRemaining; 
}

void CAI_ShotRegulator::SetBurstShotsRemaining( int shots )	
{
	m_nBurstShotsRemaining = shots;
}




//-----------------------------------------------------------------------------
CTraceFilterNav::CTraceFilterNav( CAI_NPC *pProber, bool bIgnoreTransientEntities, const IServerEntity *passedict, int collisionGroup, bool bAllowPlayerAvoid ) : 
	CE_CTraceFilterSimple( passedict, collisionGroup ),
	m_pProber(pProber),
	m_bIgnoreTransientEntities(bIgnoreTransientEntities),
	m_bAllowPlayerAvoid(bAllowPlayerAvoid)
{
	m_bCheckCollisionTable = my_g_EntityCollisionHash->IsObjectInHash( pProber->BaseEntity() );
}

//-----------------------------------------------------------------------------
bool CTraceFilterNav::ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
{
	IServerEntity *pServerEntity = (IServerEntity*)pHandleEntity;
	CBaseEntity *cbase = (CBaseEntity *)pServerEntity;
	CEntity *pEntity = CEntity::Instance(cbase);

	if(pEntity == NULL)
	{
		edict_t *pEdict = servergameents->BaseEntityToEdict(cbase);
		int index = engine->IndexOfEdict(pEdict);
		META_CONPRINTF("%d %s\n", index, pEdict->GetClassNameA());
	}
	if ( m_pProber == pEntity )
		return false;

	if ( m_pProber->GetMoveProbe()->ShouldBrushBeIgnored( cbase ) == true )
		return false;

	if ( m_bIgnoreTransientEntities && (pEntity->IsPlayer() || pEntity->IsNPC() ) )
		return false;

	//Adrian - If I'm flagged as using the new collision method, then ignore the player when trying
	//to check if I can get somewhere.
	if ( m_bAllowPlayerAvoid && m_pProber->ShouldPlayerAvoid() && pEntity->IsPlayer() )
		return false;

	if ( pEntity->IsNavIgnored() )
		return false;

	if ( m_bCheckCollisionTable )
	{
		if ( my_g_EntityCollisionHash->IsObjectPairInHash( m_pProber->BaseEntity(), pEntity ) )
			return false;
	}

	if ( m_pProber->ShouldProbeCollideAgainstEntity( cbase ) == false )
		return false;

	return CE_CTraceFilterSimple::ShouldHitEntity( pHandleEntity, contentsMask );
}

extern ConVar *ai_LOS_mode;

//-----------------------------------------------------------------------------
// Purpose: Use this to perform AI tracelines that are trying to determine LOS between points.
//			LOS checks between entities should use FVisible.
//-----------------------------------------------------------------------------
void AI_TraceLOS( const Vector& vecAbsStart, const Vector& vecAbsEnd, CBaseEntity *pLooker, trace_t *ptr, ITraceFilter *pFilter )
{
	if ( ai_LOS_mode->GetBool() )
	{
		// Don't use LOS tracefilter
		UTIL_TraceLine( vecAbsStart, vecAbsEnd, MASK_BLOCKLOS, pLooker, COLLISION_GROUP_NONE, ptr );
		return;
	}

	// Use the custom LOS trace filter
	CTraceFilterLOS traceFilter( pLooker, COLLISION_GROUP_NONE );
	if ( !pFilter )
		pFilter = &traceFilter;
	UTIL_TraceLine( vecAbsStart, vecAbsEnd, MASK_BLOCKLOS_AND_NPCS, pFilter, ptr );
}


//-----------------------------------------------------------------------------
// Purpose: Custom trace filter used for NPC LOS traces
//-----------------------------------------------------------------------------
CTraceFilterLOS::CTraceFilterLOS( IHandleEntity *pHandleEntity, int collisionGroup, IHandleEntity *pHandleEntity2 ) :
		CTraceFilterSkipTwoEntities( pHandleEntity, pHandleEntity2, collisionGroup )
{
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CTraceFilterLOS::ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
{
	CEntity *pEntity = CE_EntityFromEntityHandle( pHandleEntity );

	if ( !pEntity->BlocksLOS() )
		return false;

	return CE_CTraceFilterSimple::ShouldHitEntity( pHandleEntity, contentsMask );
}

//-----------------------------------------------------------------------------
// Trace filter that skips two entities
//-----------------------------------------------------------------------------
CTraceFilterSkipTwoEntities::CTraceFilterSkipTwoEntities( const IHandleEntity *passentity, const IHandleEntity *passentity2, int collisionGroup ) :
	BaseClass( passentity, collisionGroup ), m_pPassEnt2(passentity2)
{
}

bool CTraceFilterSkipTwoEntities::ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
{
	Assert( pHandleEntity );
	if ( !PassServerEntityFilter( pHandleEntity, m_pPassEnt2 ) )
		return false;

	return BaseClass::ShouldHitEntity( pHandleEntity, contentsMask );
}




bool FBoxVisible( CEntity *pLooker, CEntity *pTarget, Vector &vecTargetOrigin, float flSize )
{
	// don't look through water
	if ((pLooker->GetWaterLevel() != 3 && pTarget->GetWaterLevel() == 3) 
		|| (pLooker->GetWaterLevel() == 3 && pTarget->GetWaterLevel() == 0))
		return FALSE;

	trace_t tr;
	Vector	vecLookerOrigin = pLooker->EyePosition();//look through the NPC's 'eyes'
	for (int i = 0; i < 5; i++)
	{
		Vector vecTarget = pTarget->GetAbsOrigin();
		vecTarget.x += enginerandom->RandomFloat( pTarget->WorldAlignMins().x + flSize, pTarget->WorldAlignMaxs().x - flSize);
		vecTarget.y += enginerandom->RandomFloat( pTarget->WorldAlignMins().y + flSize, pTarget->WorldAlignMaxs().y - flSize);
		vecTarget.z += enginerandom->RandomFloat( pTarget->WorldAlignMins().z + flSize, pTarget->WorldAlignMaxs().z - flSize);

		UTIL_TraceLine(vecLookerOrigin, vecTarget, MASK_BLOCKLOS, pLooker->BaseEntity(), COLLISION_GROUP_NONE, &tr);
		
		if (tr.fraction == 1.0)
		{
			vecTargetOrigin = vecTarget;
			return TRUE;// line of sight is valid.
		}
	}
	return FALSE;// Line of sight is not established
}



void CAI_ShotRegulator::Reset( bool bStartShooting )
{
	m_bDisabled = false;
	m_nBurstShotsRemaining = enginerandom->RandomInt( m_nMinBurstShots, m_nMaxBurstShots );
	if ( bStartShooting )
	{
		m_flNextShotTime = gpGlobals->curtime;
		m_bInRestInterval = false;
	}
	else
	{
		m_flNextShotTime = gpGlobals->curtime + enginerandom->RandomFloat( m_flMinRestInterval, m_flMaxRestInterval );
		m_bInRestInterval = true;
	}
}

