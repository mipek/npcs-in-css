
#ifndef _INCLUDE_WEAPON_PHYSCANNON_REPLACE_H_
#define _INCLUDE_WEAPON_PHYSCANNON_REPLACE_H_

#include "CEntity.h"
#include "CCombatWeapon.h"
#include "CCombatCharacter.h"
#include "CNPCBaseWeapon.h"
#include "CPlayer.h"
#include "player_pickup.h"
#include "soundenvelope.h"
#include "pickup.h"


enum
{
	ELEMENT_STATE_NONE = -1,
	ELEMENT_STATE_OPEN,
	ELEMENT_STATE_CLOSED,
};

enum
{
	EFFECT_NONE,
	EFFECT_CLOSED,
	EFFECT_READY,
	EFFECT_HOLDING,
	EFFECT_LAUNCH,
};

#define	NUM_BEAMS	4
#define	NUM_SPRITES	6

struct thrown_objects_t
{
	float				fTimeThrown;
	EHANDLE				hEntity;

	DECLARE_SIMPLE_DATADESC();
};

class CE_CSprite;
class CE_CBeam;

class CWeaponPhysCannon : public CCombatWeapon
{
public:
	CE_DECLARE_CLASS( CWeaponPhysCannon, CCombatWeapon );
	DECLARE_DATADESC();

	CWeaponPhysCannon();

	void	Spawn();
	void	Precache();
	void	OnRestore();
	void	UpdateOnRemove();
	bool	Deploy();
	void	Drop( const Vector &vecVelocity );
	bool	Holster( CBaseEntity *pSwitchingTo );
	void	Think();
	void	PrimaryAttack();
	void	SecondaryAttack();
	bool	CanHolster();
	void	ItemPreFrame();
	void	ItemPostFrame();
	void	StopLoopingSounds();
	const char *GetWorldModel( void ) const;
	void Equip( CBaseEntity *pOwner );
	const char *GetViewModel( int viewmodelindex = 0 ) const;

	inline	bool IsPhysCannonReplace() const { return m_bIsPhysCannon; }
	inline	void SetIsPhysCannon(bool is_physcannon) { m_bIsPhysCannon = is_physcannon; }

	void	ForceDrop();
	bool	DropIfEntityHeld( CEntity *pTarget );
	CGrabController &GetGrabController() { return m_grabController; }

	QAngle m_attachedAnglesPlayerSpace;

	Vector m_attachedPositionObjectSpace;

	CFakeHandle m_hAttachedObject;

	CFakeHandle m_hOldAttachedObject;

public:
	static bool	IS_REPLACE_SPAWN;

protected:
	enum FindObjectResult_t
	{
		OBJECT_FOUND = 0,
		OBJECT_NOT_FOUND,
		OBJECT_BEING_DETACHED,
	};

	void	DoEffect( int effectType, Vector *pos = NULL );

	void	OpenElements( void );
	void	CloseElements( void );

	// Pickup and throw objects.
	bool	CanPickupObject( CEntity *pTarget );
	void	CheckForTarget( void );

	bool	AttachObject( CEntity *pObject, const Vector &vPosition );
	FindObjectResult_t		FindObject( void );
	CEntity *FindObjectInCone( const Vector &vecOrigin, const Vector &vecDir, float flCone );

	void	UpdateObject( void );
	void	DetachObject( bool playSound = true, bool wasLaunched = false );
	void	LaunchObject( const Vector &vecDir, float flForce );
	void	StartEffects( void );	// Initialize all sprites and beams
	void	StopEffects( bool stopSound = true );	// Hide all effects temporarily
	void	DestroyEffects( void );	// Destroy all sprites and beams

	// Punt objects - this is pointing at an object in the world and applying a force to it.
	void	PuntNonVPhysics( CEntity *pEntity, const Vector &forward, trace_t &tr );
	void	PuntVPhysics( CEntity *pEntity, const Vector &forward, trace_t &tr );

	// Velocity-based throw common to punt and launch code.
	void	ApplyVelocityBasedForce( CEntity *pEntity, const Vector &forward );

	// Physgun effects
	void	DoEffectClosed( void );
	void	DoEffectReady( void );
	void	DoEffectHolding( void );
	void	DoEffectLaunch( Vector *pos );
	void	DoEffectNone( void );
	void	DoEffectIdle( void );

	// Trace length
	float	TraceLength();

	// Sprite scale factor
	float	SpriteScaleFactor();

	float			GetLoadPercentage();
	CSoundPatch		*GetMotorSound( void );

	void	DryFire( void );
	void	PrimaryFireEffect( void );

	// What happens when the physgun picks up something
	void	Physgun_OnPhysGunPickup( CEntity *pEntity, CPlayer *pOwner, PhysGunPickup_t reason );


	int		m_nChangeState;				// For delayed state change of elements
	float	m_flCheckSuppressTime;		// Amount of time to suppress the checking for targets
	bool	m_flLastDenySoundPlayed;	// Debounce for deny sound
	int		m_nAttack2Debounce;

	bool	m_bActive;
	int		m_EffectState;		// Current state of the effects on the gun
	bool	m_bOpen;

	bool	m_bResetOwnerEntity;

	float	m_flElementDebounce;

	CSoundPatch			*m_sndMotor;		// Whirring sound for the gun

	CGrabController		m_grabController;

	float			m_flRepuntObjectTime;
	CFakeHandle		m_hLastPuntedObject;



private:
	bool m_bIsPhysCannon;
	int	 m_iWeaponWorldModel;
	int	 m_iWeaponViewModel;

};

CWeaponPhysCannon *ToCWeaponPhysCannon(CEntity *cent);
CWeaponPhysCannon *ToCWeaponPhysCannon(CBaseEntity *cbase);


CEntity *GetPlayerHeldEntity( CPlayer *pPlayer );

CEntity *PhysCannonGetHeldEntity( CCombatWeapon *pActiveWeapon );

#endif
