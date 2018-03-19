//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef PROP_COMBINE_BALL_H
#define PROP_COMBINE_BALL_H
#ifdef _WIN32
#pragma once
#endif


//-----------------------------------------------------------------------------
// Includes
//-----------------------------------------------------------------------------
#include "CPlayer.h"
#include "CSode_Fix.h"
#include "CInfoTarget_Fix.h"
#include "CSpriteTrail.h"
#include "player_pickup.h"

//-----------------------------------------------------------------------------
// Forward declarations
//-----------------------------------------------------------------------------
class CFuncCombineBallSpawner;
class CE_CSpriteTrail;
class CSoundPatch;

//-----------------------------------------------------------------------------
// Looks for enemies, bounces a max # of times before it breaks
//-----------------------------------------------------------------------------
class CPropCombineBall : public CSode_Fix, public CDefaultPlayerPickupVPhysics
{
	CE_DECLARE_CLASS( CPropCombineBall, CSode_Fix );
	DECLARE_DATADESC();
	//DECLARE_SERVERCLASS();

public:
	virtual void Precache();
	virtual void Spawn();
	virtual void UpdateOnRemove();
	void StopLoopingSounds();

	virtual void OnPhysGunPickup( CPlayer *pPhysGunUser, PhysGunPickup_t reason );
	virtual void OnPhysGunDrop( CPlayer *pPhysGunUser, PhysGunDrop_t Reason );
	virtual void VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );

	virtual bool OverridePropdata();
	virtual bool CreateVPhysics();

	CFuncCombineBallSpawner *GetSpawner();

	virtual void ExplodeThink( void );

	// Override of IPlayerPickupVPhysics;
	virtual bool ShouldPuntUseLaunchForces( PhysGunForce_t reason ) { return ( reason == PHYSGUN_FORCE_PUNTED ); }

	void SetRadius( float flRadius );
	void SetSpeed( float flSpeed ) { m_flSpeed = flSpeed; }
	float GetSpeed( void ) const { return m_flSpeed; }

	void CaptureBySpawner( );
	bool IsBeingCaptured() const { return m_bCaptureInProgress; }

	void ReplaceInSpawner( float flSpeed );

	// Input
	void InputExplode( inputdata_t &inputdata );
	void InputFadeAndRespawn( inputdata_t &inputdata );
	void InputKill( inputdata_t &inputdata );
	void InputSocketed( inputdata_t &inputdata );

	enum
	{
		STATE_NOT_THROWN = 0,
		STATE_HOLDING,
		STATE_THROWN,
		STATE_LAUNCHED, //by a combine_ball launcher
	};

	void SetState( int state );
	bool IsInField() const;

	void StartWhizSoundThink( void );

	void StartLifetime( float flDuration );
	void ClearLifetime( );
	void SetMass( float mass );

	void SetWeaponLaunched( bool state = true ) { m_bWeaponLaunched = state; m_bLaunched = state; }
	bool WasWeaponLaunched( void ) const { return m_bWeaponLaunched; }

	bool WasFiredByNPC() { return (GetOwnerEntity() && GetOwnerEntity()->IsNPC()); }

	bool ShouldHitPlayer();

	virtual CBaseEntity *HasPhysicsAttacker( float dt );

	void	SetSpawner( CFuncCombineBallSpawner *pSpawner ) { m_hSpawner = pSpawner; }
	void	NotifySpawnerOfRemoval( void );


	float	LastCaptureTime() const;

	unsigned char GetState() const { return m_nState;	}

	int  NumBounces( void ) const { return m_nBounceCount; }

	void SetMaxBounces( int iBounces )
	{
		m_nMaxBounces = iBounces;
	}

	void SetEmitState( bool bEmit )
	{
		m_bEmit = bEmit;
	}

	void SetOriginalOwner( CEntity *pEntity ) { m_hOriginalOwner = pEntity; }
	CEntity *GetOriginalOwner() { return m_hOriginalOwner; }

private:

	void SetPlayerLaunched( CPlayer *pOwner );

	float GetBallHoldDissolveTime();
	float GetBallHoldSoundRampTime();

	// Pow!
	void DoExplosion( );

	void StartAnimating( void );
	void StopAnimating( void );

	void SetBallAsLaunched( void );

	void CollisionEventToTrace( int index, gamevcollisionevent_t *pEvent, trace_t &tr );
	bool DissolveEntity( CEntity *pEntity );
	void OnHitEntity( CEntity *pHitEntity, float flSpeed, int index, gamevcollisionevent_t *pEvent );
	void DoImpactEffect( const Vector &preVelocity, int index, gamevcollisionevent_t *pEvent );

	// Bounce inside the spawner: 
	void BounceInSpawner( float flSpeed, int index, gamevcollisionevent_t *pEvent );

	bool IsAttractiveTarget( CEntity *pEntity );

	// Deflects the ball toward enemies in case of a collision 
	void DeflectTowardEnemy( float flSpeed, int index, gamevcollisionevent_t *pEvent );

	// Is this something we can potentially dissolve? 
	bool IsHittableEntity( CEntity *pHitEntity );

	// Sucky. 
	void WhizSoundThink();
	void DieThink();
	void DissolveThink();
	void DissolveRampSoundThink();
	void AnimThink( void );

	void FadeOut( float flDuration );


	bool OutOfBounces( void ) const
	{
		return ( m_nState == STATE_LAUNCHED && m_nMaxBounces != 0 && m_nBounceCount >= m_nMaxBounces );
	}

private:

	int		m_nBounceCount;
	int		m_nMaxBounces;
	bool	m_bBounceDie;

	float	m_flLastBounceTime;

	bool	m_bFiredGrabbedOutput;
	bool	m_bStruckEntity;		// Has hit an entity already (control accuracy)
	bool	m_bWeaponLaunched;		// Means this was fired from the AR2
	bool	m_bForward;				// Movement direction in ball spawner

	unsigned char m_nState;
	bool	m_bCaptureInProgress;

	float	m_flSpeed;

	CE_CSpriteTrail *m_pGlowTrail;
	CSoundPatch *m_pHoldingSound;

	float	m_flNextDamageTime;
	float	m_flLastCaptureTime;

	CHandle < CFuncCombineBallSpawner > m_hSpawner;

	CHandle < CEntity > m_hOriginalOwner;

	bool m_bEmit;
	bool m_bHeld;
	bool m_bLaunched;
	float m_flRadius;
	//CNetworkVar( bool, m_bEmit );
	//CNetworkVar( bool, m_bHeld );
	//CNetworkVar( bool, m_bLaunched );
	//CNetworkVar( float, m_flRadius );
};

class CFuncCombineBallSpawner : public CE_InfoTarget_Fix
{
	CE_DECLARE_CLASS( CFuncCombineBallSpawner, CE_InfoTarget_Fix );
	DECLARE_DATADESC();

public:
	CFuncCombineBallSpawner();

	virtual void Spawn();
	virtual void Precache();

	// Balls call this to figure out where to bounce to
	void GetTargetEndpoint( bool bForward, Vector *pVecEndpoint );

	// Balls call this when they've been removed from the spawner
	void RespawnBall( float flRespawnTime );
	void RespawnBallPostExplosion( void );

	// Fire ball grabbed output
	void BallGrabbed( CEntity *pEntity );

	// Get speed of ball to place into the field
	float GetBallSpeed( ) const;

	// Register that a reflection occurred
	void RegisterReflection( CPropCombineBall *pBall, bool bForward );

	// Spawn a ball
	virtual void SpawnBall();

private:

	// Choose a random point inside the cylinder
	void ChoosePointInCylinder( Vector *pVecPoint );

	// Choose a random point inside the box
	void ChoosePointInBox( Vector *pVecPoint );

	// Used to determine when to respawn balls
	void BallThink();

	// Input
	void	InputEnable( inputdata_t &inputdata );
	void	InputDisable( inputdata_t &inputdata );

	// Fire ball grabbed output
	void	GrabBallTouch( CEntity *pOther );

public:
	bool m_bShooter;
	float m_flBallRadius;
	float m_flBallRespawnTime;
	float m_flMinSpeed;
	float m_flMaxSpeed;

private:
	CUtlVector< float > m_BallRespawnTime;
	int m_nBallCount;
	int m_nBallsRemainingInField;
	float m_flRadius;
	float m_flDisableTime;
	bool m_bEnabled;

	COutputEvent m_OnBallGrabbed;
	COutputEvent m_OnBallReinserted;
	COutputEvent m_OnBallHitTopSide;
	COutputEvent m_OnBallHitBottomSide;
	COutputEvent m_OnLastBallGrabbed;
	COutputEvent m_OnFirstBallReinserted;
};

#if 0
class CPointCombineBallLauncher : public CFuncCombineBallSpawner
{
	CE_DECLARE_CLASS( CPointCombineBallLauncher, CFuncCombineBallSpawner );

	DECLARE_DATADESC();

public:

	virtual void Spawn( void );
	virtual void SpawnBall( void );
	void InputLaunchBall ( inputdata_t &inputdata );

	CPointCombineBallLauncher();

private:

	int			m_iBounces;
	float		m_flConeDegrees;
	string_t	m_iszBullseyeName;
};
#endif

// Creates a combine ball
CPropCombineBall *CreateCombineBall( const Vector &origin, const Vector &velocity, float radius, float mass, float lifetime, CEntity *pOwner );

// Query function to find out if a physics object is a combine ball (used for collision checks)
bool UTIL_IsCombineBall( CEntity *pEntity );
bool UTIL_IsCombineBallDefinite( CEntity *pEntity );
bool UTIL_IsAR2CombineBall( CEntity *pEntity );

#endif // PROP_COMBINE_BALL_H
