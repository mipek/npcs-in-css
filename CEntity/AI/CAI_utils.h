//========= Copyright ?1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Simple, small, free-standing tools for building AIs
//
//=============================================================================//

#ifndef AI_UTILS_H
#define AI_UTILS_H

#include "simtimer.h"
#include "ai_hull.h"
#include "CAI_component.h"

#if defined( _WIN32 )
#pragma once
#endif



//-----------------------------------------------------------------------------
//
// CAI_ShotRegulator
//
// Purpose: Assists in creating non-constant bursty shooting style
//
//-----------------------------------------------------------------------------
class CAI_ShotRegulator
{
public:
	CAI_ShotRegulator();

	// Sets the various parameters for burst (this one's for backwards compatibility)
	// NOTE: This will modify the next shot time
	void SetParameters( int minShotsPerBurst, int maxShotsPerBurst, float minRestTime, float maxRestTime = 0.0 );

	// NOTE: The next 3 methods will *not* modify the next shot time
	// Sets the number of shots to shoot in a single burst
	void SetBurstShotCountRange( int minShotsPerBurst, int maxShotsPerBurst );

	// How much time should I rest between bursts?
	void SetRestInterval( float flMinRestInterval, float flMaxRestInterval );

	// How much time should I wait in between shots in a single burst?
	void SetBurstInterval( float flMinBurstInterval, float flMaxBurstInterval );
	
	// Poll the current parameters
	void GetBurstShotCountRange( int *pMinShotsPerBurst, int *pMaxShotsPerBurst ) const;
	void GetRestInterval( float *pMinRestInterval, float *pMaxRestInterval ) const;
	void GetBurstInterval( float *pMinBurstInterval, float *pMaxBurstInterval ) const;

	// Reset the state. If true, the next burst time is set to now,
	// otherwise it'll wait one rest interval before shooting 
	void Reset( bool bStartShooting = true );

	// Should we shoot?
	bool ShouldShoot() const;

	// When will I shoot next?
	float NextShotTime() const;

	// Am I in the middle of a rest period?
	bool IsInRestInterval() const;

	// NOTE: These will not modify the next shot time
	int GetBurstShotsRemaining() const;
	void SetBurstShotsRemaining( int shots );

	// Call this when the NPC fired the weapon;
	void OnFiredWeapon();

	// Causes us to potentially delay our shooting time
	void FireNoEarlierThan( float flTime );

	// Prevent/Allow shooting
	void EnableShooting( void );
	void DisableShooting( void );
	
private:
	float	m_flNextShotTime;
	bool	m_bInRestInterval;
	unsigned short	m_nBurstShotsRemaining;
	unsigned short	m_nMinBurstShots, m_nMaxBurstShots;
	float	m_flMinRestInterval, m_flMaxRestInterval;
	float	m_flMinBurstInterval, m_flMaxBurstInterval;
	bool	m_bDisabled;

	DECLARE_SIMPLE_DATADESC();
};



//-----------------------------------------------------------------------------
//
// CAI_MoveMonitor
//
// Purpose: Watch an entity, trigger if moved more than a tolerance
//
//-----------------------------------------------------------------------------

class CAI_MoveMonitor
{
public:
	CAI_MoveMonitor()
	 : m_vMark( 0, 0, 0 ),
	   m_flMarkTolerance( NO_MARK )
	{
	}
	
	void SetMark( CEntity *pEntity, float tolerance )
	{
		if ( pEntity )
		{
			m_vMark = pEntity->GetAbsOrigin();
			m_flMarkTolerance = tolerance;
		}
	}
	
	void ClearMark()
	{
	   m_flMarkTolerance = NO_MARK;
	}

	bool IsMarkSet()
	{
		return ( m_flMarkTolerance != NO_MARK );
	}

	bool TargetMoved( CEntity *pEntity )
	{
		if ( IsMarkSet() && pEntity != NULL )
		{
			float distance = ( m_vMark - pEntity->GetAbsOrigin() ).Length();
			if ( distance > m_flMarkTolerance )
				return true;
		}
		return false;
	}

	bool TargetMoved2D( CEntity *pEntity )
	{
		if ( IsMarkSet() && pEntity != NULL )
		{
			float distance = ( m_vMark.AsVector2D() - pEntity->GetAbsOrigin().AsVector2D() ).Length();
			if ( distance > m_flMarkTolerance )
				return true;
		}
		return false;
	}

	Vector GetMarkPos() { return m_vMark; }
	
private:
	enum
	{
		NO_MARK = -1
	};
	
	Vector			   m_vMark;
	float			   m_flMarkTolerance;

	DECLARE_SIMPLE_DATADESC();
};


void AI_TraceLOS( const Vector& vecAbsStart, const Vector& vecAbsEnd, CBaseEntity *pLooker, trace_t *ptr, ITraceFilter *pFilter = NULL );

//-----------------------------------------------------------------------------

class CTraceFilterNav : public CE_CTraceFilterSimple
{
public:
	CTraceFilterNav( CAI_NPC *pProber, bool bIgnoreTransientEntities, const IServerEntity *passedict, int collisionGroup, bool m_bAllowPlayerAvoid = true );
	bool ShouldHitEntity( IHandleEntity *pServerEntity, int contentsMask );

private:
	CAI_NPC *m_pProber;
	bool m_bIgnoreTransientEntities;
	bool m_bCheckCollisionTable;
	bool m_bAllowPlayerAvoid;
};


extern string_t g_iszFuncBrushClassname;

#endif // AI_UTILS_H
