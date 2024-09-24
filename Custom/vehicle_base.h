#ifndef VEHICLE_BASE_H
#define VEHICLE_BASE_H
#ifdef _WIN32
#pragma once
#endif

#include "vehicles.h"
#include "vehicle_sounds.h"
#include "vehicle_viewblend_shared.h"


class CFourWheelVehiclePhysics;
class CE_CPropVehicleDriveable;
class CSoundPatch;
class CBaseServerVehicle;

// the tires are considered to be skidding if they have sliding velocity of 10 in/s or more
const float DEFAULT_SKID_THRESHOLD = 10.0f;


struct vbs_sound_update_t
{
	float	flFrameTime;
	float	flCurrentSpeedFraction;
	float	flWorldSpaceSpeed;
	bool	bThrottleDown;
	bool	bReverse;
	bool	bTurbo;
	bool	bVehicleInWater;
	bool	bExitVehicle;

	void Defaults()
	{
		flFrameTime = gpGlobals->frametime;
		flCurrentSpeedFraction = 0;
		flWorldSpaceSpeed = 0;
		bThrottleDown = false;
		bReverse = false;
		bTurbo = false;
		bVehicleInWater = false;
		bExitVehicle = false;
	}
};



// -----------------------------------------
//  Seat in a vehicle (attachment and a collection of animations to reach it)
// -----------------------------------------
class CPassengerSeat
{
public:
	CPassengerSeat( void ) : m_nAttachmentID( -1 ) {};
	int GetAttachmentID( void ) const { return m_nAttachmentID; }

private:
	string_t								m_strSeatName;			// Used for save/load fixup
	int										m_nAttachmentID;		// Goal attachment
	CUtlVector<CPassengerSeatTransition>	m_EntryTransitions;		// Entry information
	CUtlVector<CPassengerSeatTransition>	m_ExitTransitions;		// Exit information

	friend class CBaseServerVehicle;
};


// -----------------------------------------
//  Passenger role information
// -----------------------------------------
class CPassengerRole
{
public:
	CPassengerRole( void ) : m_strName( NULL_STRING ) {};
	string_t GetName( void ) const { return m_strName; }

private:
	string_t						m_strName;			// Name of the set
	CUtlVector<CPassengerSeat>		m_PassengerSeats;	// Passenger info

	friend class CBaseServerVehicle;
};



// -----------------------------------------
//  Information about the passenger in the car
// -----------------------------------------
class CPassengerInfo
{
public:
	CPassengerInfo( void ) : m_nRole( -1 ), m_nSeat( -1 ), m_strRoleName( NULL_STRING ), m_strSeatName( NULL_STRING ) {}

	DECLARE_SIMPLE_DATADESC();

	int GetSeat( void ) const { return m_nSeat; }
	int	GetRole( void ) const { return m_nRole; }
	CBaseEntity *GetPassenger( void ) const { return m_hPassenger; }

private:
	int									m_nRole;		// Role (by index)
	int									m_nSeat;		// Seat (by index)
	string_t							m_strRoleName;	// Used in restoration for fix-up
	string_t							m_strSeatName;	// Used in restoration for fix-up
	CHandle<CBaseEntity>				m_hPassenger;	// Actual passenger

	friend class CBaseServerVehicle;
};


// -----------------------------------------
//  Seat transition information (animation and priority)
// -----------------------------------------

class CPassengerSeatTransition
{
public:
	CPassengerSeatTransition( void ) : m_strAnimationName( NULL_STRING ), m_nPriority( -1 ) {};

	string_t GetAnimationName( void ) const { return m_strAnimationName; }
	int		 GetPriority( void ) const { return m_nPriority; }

private:
	string_t	m_strAnimationName;	// Name of animation to play
	int			m_nPriority;		// Priority of the transition

	friend class CBaseServerVehicle;
};



class CBaseServerVehicle : public IServerVehicle
{
public:
	DECLARE_SIMPLE_DATADESC();
	DECLARE_CLASS_NOBASE( CBaseServerVehicle );

	CBaseServerVehicle( void );
	~CBaseServerVehicle( void );

	virtual void			Precache( void );

// IVehicle
public:
	virtual CBaseEntity *GetPassenger( int nRole = VEHICLE_ROLE_DRIVER );

	virtual int				GetPassengerRole( CBaseEntity *pPassenger );
	virtual void			GetVehicleViewPosition( int nRole, Vector *pOrigin, QAngle *pAngles, float *pFOV = NULL );
	virtual bool			IsPassengerUsingStandardWeapons( int nRole = VEHICLE_ROLE_DRIVER ) { return false; }
	virtual void			SetupMove( CBaseEntity *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move );
	virtual void			ProcessMovement( CBaseEntity *pPlayer, CMoveData *pMoveData );
	virtual void			FinishMove( CBaseEntity *player, CUserCmd *ucmd, CMoveData *move );
	virtual void			ItemPostFrame( CBaseEntity *pPlayer );

// IServerVehicle
public:
	virtual CBaseEntity		*GetVehicleEnt( void ) { return m_pVehicle; }
	virtual void			SetPassenger( int nRole, CBaseEntity *pPassenger );
	virtual bool			IsPassengerVisible( int nRole = VEHICLE_ROLE_DRIVER ) { return false; }
	virtual bool			IsPassengerDamagable( int nRole  = VEHICLE_ROLE_DRIVER ) { return true; }
	virtual bool			PassengerShouldReceiveDamage( CTakeDamageInfo &info );

	virtual bool			IsVehicleUpright( void ) { return true; }
	virtual bool			IsPassengerEntering( void ) { Assert( 0 ); return false; }
	virtual bool			IsPassengerExiting( void ) { Assert( 0 ); return false; }

	virtual void			HandlePassengerEntry( CBaseEntity *pPassenger, bool bAllowEntryOutsideZone = false );
	virtual bool			HandlePassengerExit( CBaseEntity *pPassenger );

	virtual void			GetPassengerSeatPoint( int nRole, Vector *pPoint, QAngle *pAngles );
	virtual bool			GetPassengerExitPoint( int nRole, Vector *pPoint, QAngle *pAngles );
	virtual int			ClassifyPassenger( CBaseEntity *pPassenger, int defaultClassification ) { return defaultClassification; }
	virtual float			PassengerDamageModifier( const CTakeDamageInfo &info ) { return 1.0; }
	virtual const vehicleparams_t	*GetVehicleParams( void ) { return NULL; }
	virtual bool			IsVehicleBodyInWater( void ) { return false; }
	virtual IPhysicsVehicleController *GetVehicleController() { return NULL; }

	// NPC Driving
	virtual bool			NPC_CanDrive( void ) { return true; }
	virtual void			NPC_SetDriver( CBaseEntity *pDriver ) { return; }
	virtual void			NPC_DriveVehicle( void ) { return; }
	virtual void			NPC_ThrottleCenter( void );
	virtual void			NPC_ThrottleReverse( void );
	virtual void			NPC_ThrottleForward( void );
	virtual void			NPC_Brake( void );
	virtual void			NPC_TurnLeft( float flDegrees );
	virtual void			NPC_TurnRight( float flDegrees );
	virtual void			NPC_TurnCenter( void );
	virtual void			NPC_PrimaryFire( void );
	virtual void			NPC_SecondaryFire( void );
	virtual bool			NPC_HasPrimaryWeapon( void ) { return false; }
	virtual bool			NPC_HasSecondaryWeapon( void ) { return false; }
	virtual void			NPC_AimPrimaryWeapon( Vector vecTarget ) { return; }
	virtual void			NPC_AimSecondaryWeapon( Vector vecTarget ) { return; }

	// Weapon handling
	virtual void			Weapon_PrimaryRanges( float *flMinRange, float *flMaxRange );
	virtual void			Weapon_SecondaryRanges( float *flMinRange, float *flMaxRange );
	virtual float			Weapon_PrimaryCanFireAt( void );		// Return the time at which this vehicle's primary weapon can fire again
	virtual float			Weapon_SecondaryCanFireAt( void );		// Return the time at which this vehicle's secondary weapon can fire again

	// ----------------------------------------------------------------------------
	// NPC passenger data

public:

	bool			NPC_AddPassenger( CBaseEntity *pPassenger, string_t strRoleName, int nSeat );
	bool			NPC_RemovePassenger( CBaseEntity *pPassenger );
	virtual bool	NPC_GetPassengerSeatPosition( CBaseEntity *pPassenger, Vector *vecResultPos, QAngle *vecResultAngle );
	virtual bool	NPC_GetPassengerSeatPositionLocal( CBaseEntity *pPassenger, Vector *vecResultPos, QAngle *vecResultAngles );
	virtual int		NPC_GetPassengerSeatAttachment( CBaseEntity *pPassenger );
	virtual int		NPC_GetAvailableSeat( CBaseEntity *pPassenger, string_t strRoleName, VehicleSeatQuery_e nQueryType );
	bool			NPC_HasAvailableSeat( string_t strRoleName );


	virtual const PassengerSeatAnims_t	*NPC_GetPassengerSeatAnims( CBaseEntity *pPassenger, PassengerSeatAnimType_t nType );
	virtual CBaseEntity					*NPC_GetPassengerInSeat( int nRoleID, int nSeatID );

	Vector	GetSavedViewOffset( void ) { return m_savedViewOffset; }

private:

	// Vehicle entering/exiting
	void	ParseNPCRoles( KeyValues *pModelKeyValues );
	void	ParseNPCPassengerSeat( KeyValues *pSetKeyValues, CPassengerSeat *pSeat );
	void	ParseNPCSeatTransition( KeyValues *pTransitionKeyValues, CPassengerSeatTransition *pTransition );

protected:

	int		FindRoleIndexByName( string_t strRoleName );
	int		FindSeatIndexByName( int nRoleIndex, string_t strSeatName );
	int		NPC_GetAvailableSeat_Any( CBaseEntity *pPassenger, int nRoleID );
	int		NPC_GetAvailableSeat_Nearest( CBaseEntity *pPassenger, int nRoleID );

	CPassengerRole *FindOrCreatePassengerRole( string_t strName, int *nIndex = NULL );

	CUtlVector< CPassengerInfo >	m_PassengerInfo;
	CUtlVector< CPassengerRole >	m_PassengerRoles;	// Not save/restored

	// ----------------------------------------------------------------------------
	void	ReloadScript();	// debug/tuning
public:

	void					UseLegacyExitChecks( bool bState ) { m_bUseLegacyExitChecks = bState; }
	void					RestorePassengerInfo( void );

	virtual CBaseEntity		*GetDriver( void );	// Player Driving
	virtual void			ParseEntryExitAnims( void );
	void					ParseExitAnim( KeyValues *pkvExitList, bool bEscapeExit );
	virtual bool			CheckExitPoint( float yaw, int distance, Vector *pEndPoint );
	virtual int				GetEntryAnimForPoint( const Vector &vecPoint );
	virtual int				GetExitAnimToUse( Vector &vecEyeExitEndpoint, bool &bAllPointsBlocked );
	virtual void			HandleEntryExitFinish( bool bExitAnimOn, bool bResetAnim );

	virtual void			SetVehicle( CBaseEntity *pVehicle );
	IDrivableVehicle 		*GetDrivableVehicle( void );

	// Sound handling
	bool					Initialize( const char *pScriptName );
	virtual void			SoundStart();
	virtual void			SoundStartDisabled();
	virtual void			SoundShutdown( float flFadeTime = 0.0 );
	virtual void			SoundUpdate( vbs_sound_update_t &params );
	virtual void			PlaySound( vehiclesound iSound );
	virtual void			StopSound( vehiclesound iSound );
	virtual void 			RecalculateSoundGear( vbs_sound_update_t &params );
	void					SetVehicleVolume( float flVolume ) { m_flVehicleVolume = clamp( flVolume, 0.0f, 1.0f ); }

	// Rumble
	virtual void			StartEngineRumble();
	virtual void			StopEngineRumble();

	// CEntity
	void CE_SetVehicle(CEntity *pVehicle)
	{
		m_pVehicle = pVehicle->BaseEntity();
		m_pDrivableVehicle = dynamic_cast<IDrivableVehicle*>(pVehicle);
		Assert( m_pDrivableVehicle );
	}

public:
	CBaseEntity			*m_pVehicle;
	IDrivableVehicle 	*m_pDrivableVehicle;

	// NPC Driving
	int								m_nNPCButtons;
	int								m_nPrevNPCButtons;
	float							m_flTurnDegrees;

	// Entry / Exit anims
	struct entryanim_t
	{
		int		iHitboxGroup;
		char	szAnimName[128];
	};
	CUtlVector< entryanim_t >		m_EntryAnimations;

	struct exitanim_t
	{
		bool	bUpright;
		bool	bEscapeExit;
		char	szAnimName[128];
		Vector	vecExitPointLocal;		// Point the animation leaves the player at when finished
		QAngle	vecExitAnglesLocal;
	};

	CUtlVector< exitanim_t >		m_ExitAnimations;
	bool							m_bParsedAnimations;
	bool							m_bUseLegacyExitChecks;	// HACK: Choreo vehicles use non-sensical setups to move the player, we need to poll their attachment point positions
	int								m_iCurrentExitAnim;
	Vector							m_vecCurrentExitEndPoint;
	Vector							m_savedViewOffset;
	CHandle<CBaseEntity>			m_hExitBlocker;				// Entity to prevent other entities blocking the player's exit point during the exit animation

	char							m_chPreviousTextureType;

// sound state
	vehiclesounds_t					m_vehicleSounds;
private:
	float							m_flVehicleVolume;
	int								m_iSoundGear;			// The sound "gear" that we're currently in
	float							m_flSpeedPercentage;

	CSoundPatch						*m_pStateSound;
	CSoundPatch						*m_pStateSoundFade;
	sound_states					m_soundState;
	float							m_soundStateStartTime;
	float							m_lastSpeed;

	void	SoundState_OnNewState( sound_states lastState );
	void	SoundState_Update( vbs_sound_update_t &params );
	sound_states SoundState_ChooseState( vbs_sound_update_t &params );
	void	PlaySound( const char *pSound );
	void	StopLoopingSound( float fadeTime = 0.25f );
	void	PlayLoopingSound( const char *pSoundName );
	bool	PlayCrashSound( float speed );
	bool	CheckCrash( vbs_sound_update_t &params );
	const char *StateSoundName( sound_states state );
	void	InitSoundParams( vbs_sound_update_t &params );
	void	CacheEntryExitPoints( void );
	bool	GetLocalAttachmentAtTime( int nQuerySequence, int nAttachmentIndex, float flCyclePoint, Vector *vecOriginOut, QAngle *vecAnglesOut );
	bool	GetLocalAttachmentAtTime( const char *lpszAnimName, int nAttachmentIndex, float flCyclePoint, Vector *vecOriginOut, QAngle *vecAnglesOut );
};



//-----------------------------------------------------------------------------
// Purpose: Four wheel physics vehicle server vehicle
//-----------------------------------------------------------------------------
class CFourWheelServerVehicle : public CBaseServerVehicle
{
	DECLARE_CLASS( CFourWheelServerVehicle, CBaseServerVehicle );

// IServerVehicle
public:
	virtual ~CFourWheelServerVehicle( void )
	{
	}

	CFourWheelServerVehicle( void );
	virtual bool			IsVehicleUpright( void );
	virtual bool			IsVehicleBodyInWater( void );
	virtual void			GetVehicleViewPosition( int nRole, Vector *pOrigin, QAngle *pAngles, float *pFOV = NULL );
	IPhysicsVehicleController *GetVehicleController();
	const vehicleparams_t	*GetVehicleParams( void );
	const vehicle_controlparams_t *GetVehicleControlParams( void );
	const vehicle_operatingparams_t	*GetVehicleOperatingParams( void );

	// NPC Driving
	void					NPC_SetDriver( CBaseEntity *pDriver );
	void					NPC_DriveVehicle( void );

	CE_CPropVehicleDriveable *GetFourWheelVehicle( void );
	bool					 GetWheelContactPoint( int nWheelIndex, Vector &vecPos );

public:
	virtual void	SetVehicle( CBaseEntity *pVehicle );
	void	InitViewSmoothing( const Vector &vecStartOrigin, const QAngle &vecStartAngles );
	bool	IsPassengerEntering( void );
	bool	IsPassengerExiting( void );

	DECLARE_SIMPLE_DATADESC();

private:
	CFourWheelVehiclePhysics	*GetFourWheelVehiclePhysics( void );

	ViewSmoothingData_t		m_ViewSmoothing;
};


#endif