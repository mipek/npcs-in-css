//========= Copyright ?1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "CEntity.h"
#include "CAI_utils.h"
#include "CAI_Memory.h"
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

void CAI_ShotRegulator::SetParameters( int minShotsPerBurst, int maxShotsPerBurst, float minRestTime, float maxRestTime )
{
	SetBurstShotCountRange( minShotsPerBurst, maxShotsPerBurst );
	SetRestInterval( minRestTime, maxRestTime );
	Reset( false );
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
		IServerNetworkable *networkable = cbase->GetNetworkable();
		int index = networkable ? engine->IndexOfEdict(networkable->GetEdict()) : -1;
		META_CONPRINTF("ShouldHitEntity %d %s\n", index, networkable ? networkable->GetClassName() : "<null>");
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

void CAI_ShotRegulator::FireNoEarlierThan( float flTime )
{
	if ( flTime > m_flNextShotTime )
	{
		m_flNextShotTime = flTime;
	}
}

// CAI_FreePass

BEGIN_SIMPLE_DATADESC( AI_FreePassParams_t )

					DEFINE_KEYFIELD( timeToTrigger,			FIELD_FLOAT, "freepass_timetotrigger"),
					DEFINE_KEYFIELD( duration,				FIELD_FLOAT, "freepass_duration"),
					DEFINE_KEYFIELD( moveTolerance,			FIELD_FLOAT, "freepass_movetolerance"),
					DEFINE_KEYFIELD( refillRate,			FIELD_FLOAT, "freepass_refillrate"),
					DEFINE_FIELD(	 coverDist,				FIELD_FLOAT),
					DEFINE_KEYFIELD( peekTime,				FIELD_FLOAT, "freepass_peektime"),
					DEFINE_FIELD(	 peekTimeAfterDamage,	FIELD_FLOAT),
					DEFINE_FIELD(	 peekEyeDist,			FIELD_FLOAT),
					DEFINE_FIELD(	 peekEyeDistZ,			FIELD_FLOAT),

END_DATADESC()

BEGIN_SIMPLE_DATADESC( CAI_FreePass )

					DEFINE_FIELD( m_hTarget, FIELD_EHANDLE ),
					DEFINE_FIELD( m_FreePassTimeRemaining,	FIELD_FLOAT ),
					DEFINE_EMBEDDED( m_FreePassMoveMonitor ),
					DEFINE_EMBEDDED( m_Params ),

END_DATADESC()

void CAI_FreePass::Update( )
{
	CEntity *pTarget = GetPassTarget();
	if ( !pTarget || m_Params.duration < 0.1 )
		return;

	//---------------------------------
	//
	// Free pass logic
	//
	AI_EnemyInfo_t *pTargetInfo = GetOuter()->GetEnemies()->Find( pTarget->BaseEntity() );

	// This works with old data because need to do before base class so as to not choose as enemy
	if ( !HasPass() )
	{
		float timePlayerLastSeen = (pTargetInfo) ? pTargetInfo->timeLastSeen : AI_INVALID_TIME;
		float lastTimeDamagedBy = (pTargetInfo) ? pTargetInfo->timeLastReceivedDamageFrom : AI_INVALID_TIME;

		if ( timePlayerLastSeen == AI_INVALID_TIME || gpGlobals->curtime - timePlayerLastSeen > .15 ) // If didn't see the player last think
		{
			trace_t tr;
			UTIL_TraceLine( pTarget->EyePosition(), GetOuter()->EyePosition(), MASK_BLOCKLOS, GetOuter()->BaseEntity(), COLLISION_GROUP_NONE, &tr );
			if ( tr.fraction != 1.0 && tr.m_pEnt != pTarget->BaseEntity() )
			{
				float dist = (tr.endpos - tr.startpos).Length() * tr.fraction;

				if ( dist < m_Params.coverDist )
				{
					if ( ( timePlayerLastSeen == AI_INVALID_TIME || gpGlobals->curtime - timePlayerLastSeen > m_Params.timeToTrigger ) &&
						 ( lastTimeDamagedBy == AI_INVALID_TIME || gpGlobals->curtime - lastTimeDamagedBy > m_Params.timeToTrigger ) )
					{
						m_FreePassTimeRemaining = m_Params.duration;
						m_FreePassMoveMonitor.SetMark( pTarget, m_Params.moveTolerance );
					}
				}
			}
		}
	}
	else
	{
		float temp = m_FreePassTimeRemaining;
		m_FreePassTimeRemaining = 0;
		CAI_Senses *pSenses = GetOuter()->GetSenses();
		bool bCanSee = ( pSenses && pSenses->ShouldSeeEntity( pTarget->BaseEntity() ) && pSenses->CanSeeEntity( pTarget->BaseEntity() ) );
		m_FreePassTimeRemaining = temp;

		if ( bCanSee )
		{
			if ( !m_FreePassMoveMonitor.TargetMoved( pTarget ) )
				m_FreePassTimeRemaining -= 0.1;
			else
				Revoke( true );
		}
		else
		{
			m_FreePassTimeRemaining += 0.1 * m_Params.refillRate;
			if ( m_FreePassTimeRemaining > m_Params.duration )
				m_FreePassTimeRemaining = m_Params.duration;
			m_FreePassMoveMonitor.SetMark( pTarget, m_Params.moveTolerance );
		}
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CAI_FreePass::HasPass()
{
	return ( m_FreePassTimeRemaining > 0 );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CAI_FreePass::Revoke( bool bUpdateMemory )
{
	m_FreePassTimeRemaining = 0;
	if ( bUpdateMemory && GetPassTarget() )
	{
		GetOuter()->UpdateEnemyMemory( GetPassTarget()->BaseEntity(), GetPassTarget()->GetAbsOrigin() );
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
bool CAI_FreePass::ShouldAllowFVisible(bool bBaseResult )
{
	CEntity *	pTarget 	= GetPassTarget();
	AI_EnemyInfo_t *pTargetInfo = GetOuter()->GetEnemies()->Find( (pTarget)?pTarget->BaseEntity():NULL );

	if ( !bBaseResult || HasPass() )
		return false;

	bool bIsVisible = true;

	// Peek logic
	if ( m_Params.peekTime > 0.1 )
	{
		float lastTimeSeen = (pTargetInfo) ? pTargetInfo->timeLastSeen : AI_INVALID_TIME;
		float lastTimeDamagedBy = (pTargetInfo) ? pTargetInfo->timeLastReceivedDamageFrom : AI_INVALID_TIME;

		if ( ( lastTimeSeen == AI_INVALID_TIME || gpGlobals->curtime - lastTimeSeen > m_Params.peekTime ) &&
			 ( lastTimeDamagedBy == AI_INVALID_TIME || gpGlobals->curtime - lastTimeDamagedBy > m_Params.peekTimeAfterDamage ) )
		{
			Vector vToTarget;

			VectorSubtract( pTarget->EyePosition(), GetOuter()->EyePosition(), vToTarget );
			vToTarget.z = 0.0f;
			VectorNormalize( vToTarget );

			Vector vecRight( -vToTarget.y, vToTarget.x, 0.0f );
			trace_t	tr;

			UTIL_TraceLine( GetOuter()->EyePosition(), pTarget->EyePosition() + (vecRight * m_Params.peekEyeDist - Vector( 0, 0, m_Params.peekEyeDistZ )), MASK_BLOCKLOS, GetOuter()->BaseEntity(), COLLISION_GROUP_NONE, &tr );
			if ( tr.fraction != 1.0 && tr.m_pEnt != pTarget->BaseEntity() )
			{
				bIsVisible = false;
			}

			if ( bIsVisible )
			{
				UTIL_TraceLine( GetOuter()->EyePosition(), pTarget->EyePosition() + (-vecRight * m_Params.peekEyeDist - Vector( 0, 0, m_Params.peekEyeDistZ )), MASK_BLOCKLOS, GetOuter()->BaseEntity(), COLLISION_GROUP_NONE, &tr );
				if ( tr.fraction != 1.0 && tr.m_pEnt != pTarget->BaseEntity() )
				{
					bIsVisible = false;
				}
			}
		}
	}

	return bIsVisible;
}

void CAI_FreePass::Reset( float passTime, float moveTolerance )
{
	CEntity *pTarget = GetPassTarget();

	if ( !pTarget || m_Params.duration < 0.1 )
		return;

	if ( passTime == -1 )
	{
		m_FreePassTimeRemaining = m_Params.duration;
	}
	else
	{
		m_FreePassTimeRemaining = passTime;
	}

	if ( moveTolerance == -1  )
	{
		m_FreePassMoveMonitor.SetMark( pTarget, m_Params.moveTolerance );
	}
	else
	{
		m_FreePassMoveMonitor.SetMark( pTarget, moveTolerance );
	}
}