//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "CEntity.h"
#include "CCombatCharacter.h"
#include "CPlayer.h"
#include "CPropVehicle.h"
#include "soundent.h"
#include "physics_saverestore.h"
#include "vphysics/constraints.h"

#include "tier0/memdbgon.h"

#define	VEHICLE_HITBOX_DRIVER		1


//
// Anim events.
//
enum
{
	AE_POD_OPEN = 1,	// The pod is now open and can be entered or exited.
	AE_POD_CLOSE = 2,	// The pod is now closed and cannot be entered or exited.
};

class CPropVehiclePrisonerPod;
class CPrisonerPodServerVehicle : public CBaseServerVehicle
{
	typedef CBaseServerVehicle BaseClass;

// IServerVehicle
public:
	void GetVehicleViewPosition( int nRole, Vector *pAbsOrigin, QAngle *pAbsAngles, float *pFOV = NULL );
	virtual void ItemPostFrame( CBaseEntity *pPlayer );

	virtual bool	IsPassengerEntering( void ) { return false; }	// NOTE: This mimics the scenario HL2 would have seen
	virtual bool	IsPassengerExiting( void ) { return false; }

protected:

	CPropVehiclePrisonerPod *GetPod( void );
};

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CPropVehiclePrisonerPod : public CE_Prop, public IDrivableVehicle
{
public:
	CE_DECLARE_CLASS( CPropVehiclePrisonerPod, CE_Prop );
	DECLARE_DATADESC();
	//DECLARE_SERVERCLASS();

	CPropVehiclePrisonerPod( void ):
		m_bLocked(false), m_bForcedExit(false), m_bEnterAnimOn(false), m_bExitAnimOn(false)
	{
	}

	~CPropVehiclePrisonerPod( void )
	{
	}

	// CBaseEntity
	void	Precache( void ) override;
	void	Spawn( void ) override;
	void	Think( void ) override;

	IServerVehicle *GetServerVehicle() override
	{
		return &m_ServerVehicle;
	}

	/*bool CreateVPhysics( void ) override
	{
		VPhysicsInitNormal( SOLID_BBOX, 0, false );
	}*/

	int		ObjectCaps( void ) override { return BaseClass::ObjectCaps() | FCAP_IMPULSE_USE; };

	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value ) override;

	Vector	BodyTarget( const Vector &posSrc, bool bNoisy ) override;
	void 	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator ) override;
	int		OnTakeDamage( const CTakeDamageInfo &info ) override;

	// CBaseAnimating
	void 	HandleAnimEvent( animevent_t *pEvent ) override;

	// IDriveableVehicle
	void	ProcessMovement( CBaseEntity *pPlayer, CMoveData *pMoveData ) override
	{
	}
	void	SetupMove( CBaseEntity *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move ) override
	{
	}

	bool	CanEnterVehicle( CBaseEntity *pEntity ) override;
	bool	CanExitVehicle( CBaseEntity *pEntity ) override;
	void	SetVehicleEntryAnim( bool bOn );
	void	SetVehicleExitAnim( bool bOn, Vector vecEyeExitEndpoint ) { m_bExitAnimOn = bOn; }
	void	EnterVehicle( CBaseEntity *pPassenger ) override;
	void	ExitVehicle( int nRole ) override;

	//
	bool PassengerShouldReceiveDamage( CTakeDamageInfo &info ) override
	{
		if ( info.GetDamageType() & DMG_VEHICLE )
			return true;

		return (info.GetDamageType() & (DMG_RADIATION|DMG_BLAST) ) == 0;
	}
	CBaseEntity	*GetDriver( void ) override
	{
		if (m_hPlayer)
			return m_hPlayer->BaseEntity();
		return nullptr;
	}
	void FinishMove( CBaseEntity *player, CUserCmd *ucmd, CMoveData *move ) {}
	bool AllowBlockedExit( CBaseEntity *pPassenger, int nRole ) { return true; }
	bool AllowMidairExit( CBaseEntity *pPassenger, int nRole ) { return true; }
	void PreExitVehicle( CBaseEntity *pPassenger, int nRole ) override {}
	void ItemPostFrame( CBaseEntity *pPlayer ) override {}
	string_t GetVehicleScriptName() override { return m_vehicleScript; }

	// Inputs
	void InputEnterVehicleImmediate( inputdata_t &inputdata );
	void InputEnterVehicle( inputdata_t &inputdata );
	void InputExitVehicle( inputdata_t &inputdata );
	void InputLock( inputdata_t &inputdata );
	void InputUnlock( inputdata_t &inputdata );
	void InputOpen( inputdata_t &inputdata );
	void InputClose( inputdata_t &inputdata );

	// stuff

	void ResetUseKey(CPlayer *pPlayer);

	bool ShouldForceExit() { return m_bForcedExit; }
	void ClearForcedExit() { m_bForcedExit = false; }

protected:
	CPrisonerPodServerVehicle m_ServerVehicle;

	bool				m_bLocked;
	bool				m_bForcedExit;

	string_t 			m_vehicleScript;
	CEFakeHandle<CPlayer> 			m_hPlayer;
	bool m_bEnterAnimOn;
	bool m_bExitAnimOn;

	COutputEvent		m_playerOn;
	COutputEvent		m_playerOff;
	COutputEvent		m_OnOpen;
	COutputEvent		m_OnClose;
};

// CE TODO: should be prop_vehicle ?
LINK_ENTITY_TO_CUSTOM_CLASS( prop_vehicle_prisoner_pod, prop_physics, CPropVehiclePrisonerPod );


BEGIN_DATADESC( CPropVehiclePrisonerPod )

					// Inputs
					DEFINE_INPUTFUNC( FIELD_VOID, "Lock",	InputLock ),
					DEFINE_INPUTFUNC( FIELD_VOID, "Unlock",	InputUnlock ),
					DEFINE_INPUTFUNC( FIELD_VOID, "EnterVehicle", InputEnterVehicle ),
					DEFINE_INPUTFUNC( FIELD_VOID, "EnterVehicleImmediate", InputEnterVehicleImmediate ),
					DEFINE_INPUTFUNC( FIELD_VOID, "ExitVehicle", InputExitVehicle ),
					DEFINE_INPUTFUNC( FIELD_VOID, "Open", InputOpen ),
					DEFINE_INPUTFUNC( FIELD_VOID, "Close", InputClose ),

					// Keys
					//DEFINE_EMBEDDED( m_ServerVehicle ),

					///DEFINE_FIELD( m_hPlayer, FIELD_EHANDLE ),
					DEFINE_FIELD( m_bEnterAnimOn, FIELD_BOOLEAN ),
					DEFINE_FIELD( m_bExitAnimOn, FIELD_BOOLEAN ),
					DEFINE_FIELD( m_bForcedExit, FIELD_BOOLEAN ),
					//DEFINE_FIELD( m_vecEyeExitEndpoint, FIELD_POSITION_VECTOR ),

					DEFINE_KEYFIELD( m_bLocked, FIELD_BOOLEAN, "vehiclelocked" ),

					DEFINE_OUTPUT( m_playerOn, "PlayerOn" ),
					DEFINE_OUTPUT( m_playerOff, "PlayerOff" ),
					DEFINE_OUTPUT( m_OnOpen, "OnOpen" ),
					DEFINE_OUTPUT( m_OnClose, "OnClose" ),

END_DATADESC()

//------------------------------------------------
// Precache
//------------------------------------------------
void CPropVehiclePrisonerPod::Precache( void )
{
	BaseClass::Precache();

	PrecacheScriptSound( "d3_citadel.pod_open" );
	PrecacheScriptSound( "d3_citadel.pod_close" );

	m_ServerVehicle.CE_SetVehicle( this );
	m_ServerVehicle.Initialize( "scripts/vehicles/prisoner_pod.txt" );
}

//------------------------------------------------
// Spawn
//------------------------------------------------
void CPropVehiclePrisonerPod::Spawn( void )
{
	Precache();
	SetModel( STRING( GetModelName() ) );

	SetCollisionGroup( COLLISION_GROUP_VEHICLE );

	BaseClass::Spawn();

	m_takedamage = DAMAGE_EVENTS_ONLY;

	SetNextThink( gpGlobals->curtime );

	AddSolidFlags( FSOLID_NOT_STANDABLE );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPropVehiclePrisonerPod::TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	if ( ptr->hitbox == VEHICLE_HITBOX_DRIVER )
	{
		if ( m_hPlayer )
		{
			m_hPlayer->TakeDamage( info );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
int CPropVehiclePrisonerPod::OnTakeDamage( const CTakeDamageInfo &inputInfo )
{
	// Do scaled up physics damage to the pod
	CTakeDamageInfo info = inputInfo;
	info.ScaleDamage( 25 );

	// reset the damage
	info.SetDamage( inputInfo.GetDamage() );

	// Check to do damage to prisoner
	if ( m_hPlayer )
	{
		// Take no damage from physics damages
		if ( info.GetDamageType() & DMG_CRUSH )
			return 0;

		// Take the damage
		m_hPlayer->TakeDamage( info );
	}

	return 0;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
Vector CPropVehiclePrisonerPod::BodyTarget( const Vector &posSrc, bool bNoisy )
{
	Vector	shotPos;
	matrix3x4_t	matrix;

	int eyeAttachmentIndex = LookupAttachment("vehicle_driver_eyes");
	GetAttachment( eyeAttachmentIndex, matrix );
	MatrixGetColumn( matrix, 3, shotPos );

	if ( bNoisy )
	{
		shotPos[0] += enginerandom->RandomFloat( -8.0f, 8.0f );
		shotPos[1] += enginerandom->RandomFloat( -8.0f, 8.0f );
		shotPos[2] += enginerandom->RandomFloat( -8.0f, 8.0f );
	}

	return shotPos;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPropVehiclePrisonerPod::Think(void)
{
	SetNextThink( gpGlobals->curtime + 0.1f );

	if ( GetDriver() )
	{
		BaseClass::Think();

		// If the enter or exit animation has finished, tell the server vehicle
		if ( IsSequenceFinished() && (m_bExitAnimOn || m_bEnterAnimOn) )
		{
			GetServerVehicle()->HandleEntryExitFinish( m_bExitAnimOn, true );
		}
	}

	StudioFrameAdvance();
	DispatchAnimEvents( BaseEntity() );
}

//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CPropVehiclePrisonerPod::InputOpen( inputdata_t &inputdata )
{
	int nSequence = LookupSequence( "open" );

	// Set to the desired anim, or default anim if the desired is not present
	if ( nSequence > ACTIVITY_NOT_AVAILABLE )
	{
		SetCycle( 0 );
		m_flAnimTime = gpGlobals->curtime;
		ResetSequence( nSequence );
		ResetClientsideFrame();
		EmitSound( "d3_citadel.pod_open" );
	}
	else
	{
		// Not available try to get default anim
		Msg( "Prisoner pod %s: missing open sequence\n", GetDebugName() );
		SetSequence( 0 );
	}
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
void CPropVehiclePrisonerPod::InputClose( inputdata_t &inputdata )
{
	// The enter anim closes the pod, so don't do this redundantly!
	if ( m_bLocked || m_bEnterAnimOn )
		return;

	int nSequence = LookupSequence( "close" );

	// Set to the desired anim, or default anim if the desired is not present
	if ( nSequence > ACTIVITY_NOT_AVAILABLE )
	{
		SetCycle( 0 );
		m_flAnimTime = gpGlobals->curtime;
		ResetSequence( nSequence );
		ResetClientsideFrame();
		EmitSound( "d3_citadel.pod_close" );
	}
	else
	{
		// Not available try to get default anim
		Msg( "Prisoner pod %s: missing close sequence\n", GetDebugName() );
		SetSequence( 0 );
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPropVehiclePrisonerPod::HandleAnimEvent( animevent_t *pEvent )
{
	if ( pEvent->event == AE_POD_OPEN )
	{
		m_OnOpen.FireOutput( this, this );
		m_bLocked = false;
	}
	else if ( pEvent->event == AE_POD_CLOSE )
	{
		m_OnClose.FireOutput( this, this );
		m_bLocked = true;
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPropVehiclePrisonerPod::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	CPlayer *pPlayer = ToBasePlayer( CEntity::Instance(pActivator) );
	if ( !pPlayer )
		return;

	GetServerVehicle()->HandlePassengerEntry( pPlayer->BaseEntity(), (value > 0) );
}


//-----------------------------------------------------------------------------
// Purpose: Return true of the player's allowed to enter / exit the vehicle
//-----------------------------------------------------------------------------
bool CPropVehiclePrisonerPod::CanEnterVehicle( CBaseEntity *pEntity )
{
	// Prevent entering if the vehicle's being driven by an NPC
	if ( GetDriver() && GetDriver() != pEntity )
		return false;

	// Prevent entering if the vehicle's locked
	return !m_bLocked;
}


//-----------------------------------------------------------------------------
// Purpose: Return true of the player is allowed to exit the vehicle.
//-----------------------------------------------------------------------------
bool CPropVehiclePrisonerPod::CanExitVehicle( CBaseEntity *pEntity )
{
	// Prevent exiting if the vehicle's locked, rotating, or playing an entry/exit anim.
	return ( !m_bLocked && (GetLocalAngularVelocity() == vec3_angle) && !m_bEnterAnimOn && !m_bExitAnimOn );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPropVehiclePrisonerPod::EnterVehicle( CBaseEntity *pPassenger )
{
	if ( pPassenger == NULL )
		return;

	CPlayer *pPlayer = ToBasePlayer( CEntity::Instance(pPassenger) );
	if ( pPlayer != NULL )
	{
		// Remove any player who may be in the vehicle at the moment
		CPlayer *oldPlayer = m_hPlayer;//(CPlayer *)(m_hPlayer.ptr->Get());
		if ( oldPlayer )
		{
			ExitVehicle( VEHICLE_ROLE_DRIVER );
		}

		m_hPlayer.Set(pPlayer->GetIHandle());
		m_playerOn.FireOutput( pPlayer, this, 0 );

		m_ServerVehicle.SoundStart();
	}
	else
	{
		// NPCs are not supported yet - jdw
		Assert( 0 );
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPropVehiclePrisonerPod::SetVehicleEntryAnim( bool bOn )
{
	m_bEnterAnimOn = bOn;
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPropVehiclePrisonerPod::ExitVehicle( int nRole )
{
	CPlayer *pPlayer = m_hPlayer;//(CPlayer *)(m_hPlayer.ptr->Get());
	if ( !pPlayer )
		return;

	m_hPlayer.Term();
	ResetUseKey( pPlayer );

	m_playerOff.FireOutput( pPlayer, this, 0 );
	m_bEnterAnimOn = false;

	m_ServerVehicle.SoundShutdown( 1.0f );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPropVehiclePrisonerPod::ResetUseKey( CPlayer *pPlayer )
{
	pPlayer->m_afButtonPressed &= ~IN_USE;
}

//-----------------------------------------------------------------------------
// Purpose: Prevent the player from entering / exiting the vehicle
//-----------------------------------------------------------------------------
void CPropVehiclePrisonerPod::InputLock( inputdata_t &inputdata )
{
	m_bLocked = true;
}


//-----------------------------------------------------------------------------
// Purpose: Allow the player to enter / exit the vehicle
//-----------------------------------------------------------------------------
void CPropVehiclePrisonerPod::InputUnlock( inputdata_t &inputdata )
{
	m_bLocked = false;
}


//-----------------------------------------------------------------------------
// Purpose: Force the player to enter the vehicle.
//-----------------------------------------------------------------------------
void CPropVehiclePrisonerPod::InputEnterVehicle( inputdata_t &inputdata )
{
	if ( m_bEnterAnimOn )
		return;

	// Try the activator first & use them if they are a player.
	CCombatCharacter *pPassenger = ToBaseCombatCharacter( CEntity::Instance(inputdata.pActivator) );
	if ( pPassenger == NULL )
	{
		// Activator was not a player, just grab the singleplayer player.
		pPassenger = UTIL_PlayerByIndex( 1 );
		if ( pPassenger == NULL )
			return;
	}

	// FIXME: I hate code like this. I should really add a parameter to HandlePassengerEntry
	//		  to allow entry into locked vehicles
	bool bWasLocked = m_bLocked;
	m_bLocked = false;
	GetServerVehicle()->HandlePassengerEntry( pPassenger->BaseEntity(), true );
	m_bLocked = bWasLocked;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  : &inputdata -
//-----------------------------------------------------------------------------
void CPropVehiclePrisonerPod::InputEnterVehicleImmediate( inputdata_t &inputdata )
{
	if ( m_bEnterAnimOn )
		return;

	// Try the activator first & use them if they are a player.
	CCombatCharacter *pPassenger = ToBaseCombatCharacter( CEntity::Instance(inputdata.pActivator) );
	if ( pPassenger == NULL )
	{
		// Activator was not a player, just grab the singleplayer player.
		pPassenger = UTIL_PlayerByIndex( 1 );
		if ( pPassenger == NULL )
			return;
	}

	CPlayer *pPlayer = ToBasePlayer( pPassenger );
	if ( pPlayer != NULL )
	{
		if ( pPlayer->IsInAVehicle() )
		{
			// Force the player out of whatever vehicle they are in.
			pPlayer->LeaveVehicle();
		}

		pPlayer->GetInVehicle( GetServerVehicle(), VEHICLE_ROLE_DRIVER );
	}
	else
	{
		// NPCs are not currently supported - jdw
		Assert( 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Force the player to exit the vehicle.
//-----------------------------------------------------------------------------
void CPropVehiclePrisonerPod::InputExitVehicle( inputdata_t &inputdata )
{
	m_bForcedExit = true;
}

//========================================================================================================================================
// CRANE VEHICLE SERVER VEHICLE
//========================================================================================================================================
CPropVehiclePrisonerPod *CPrisonerPodServerVehicle::GetPod( void )
{
	return (CPropVehiclePrisonerPod *)GetDrivableVehicle();
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  : pPlayer -
//-----------------------------------------------------------------------------
void CPrisonerPodServerVehicle::ItemPostFrame( CBaseEntity *playerbase )
{
	Assert( playerbase == GetDriver() );

	CPlayer *player = ToBasePlayer(CEntity::Instance(playerbase));
	Assert( player );

	GetDrivableVehicle()->ItemPostFrame( playerbase );

	if (( player->m_afButtonPressed & IN_USE ) || GetPod()->ShouldForceExit() )
	{
		GetPod()->ClearForcedExit();
		if ( GetDrivableVehicle()->CanExitVehicle(playerbase) )
		{
			// Let the vehicle try to play the exit animation
			if ( !HandlePassengerExit( playerbase ) )
			{
				player->PlayUseDenySound();
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CPrisonerPodServerVehicle::GetVehicleViewPosition( int nRole, Vector *pAbsOrigin, QAngle *pAbsAngles, float *pFOV /*= NULL*/ )
{
	// FIXME: This needs to be reconciled with the other versions of this function!
	Assert( nRole == VEHICLE_ROLE_DRIVER );
	CPlayer *pPlayer = ToBasePlayer( CEntity::Instance(GetDrivableVehicle()->GetDriver()) );
	Assert( pPlayer );

	*pAbsAngles = pPlayer->EyeAngles(); // yuck. this is an in/out parameter.

	float flPitchFactor = 1.0;
	matrix3x4_t vehicleEyePosToWorld;
	Vector vehicleEyeOrigin;
	QAngle vehicleEyeAngles;
	GetPod()->GetAttachment( "vehicle_driver_eyes", vehicleEyeOrigin, vehicleEyeAngles );
	AngleMatrix( vehicleEyeAngles, vehicleEyePosToWorld );

	// Compute the relative rotation between the unperterbed eye attachment + the eye angles
	matrix3x4_t cameraToWorld;
	AngleMatrix( *pAbsAngles, cameraToWorld );

	matrix3x4_t worldToEyePos;
	MatrixInvert( vehicleEyePosToWorld, worldToEyePos );

	matrix3x4_t vehicleCameraToEyePos;
	ConcatTransforms( worldToEyePos, cameraToWorld, vehicleCameraToEyePos );

	// Now perterb the attachment point
	vehicleEyeAngles.x = RemapAngleRange( PITCH_CURVE_ZERO * flPitchFactor, PITCH_CURVE_LINEAR, vehicleEyeAngles.x );
	vehicleEyeAngles.z = RemapAngleRange( ROLL_CURVE_ZERO * flPitchFactor, ROLL_CURVE_LINEAR, vehicleEyeAngles.z );
	AngleMatrix( vehicleEyeAngles, vehicleEyeOrigin, vehicleEyePosToWorld );

	// Now treat the relative eye angles as being relative to this new, perterbed view position...
	matrix3x4_t newCameraToWorld;
	ConcatTransforms( vehicleEyePosToWorld, vehicleCameraToEyePos, newCameraToWorld );

	// output new view abs angles
	MatrixAngles( newCameraToWorld, *pAbsAngles );

	// UNDONE: *pOrigin would already be correct in single player if the HandleView() on the server ran after vphysics
	MatrixGetColumn( newCameraToWorld, 3, *pAbsOrigin );
}