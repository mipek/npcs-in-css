#ifndef _INCLUDE_CPLAYER_H_
#define _INCLUDE_CPLAYER_H_

#include "CEntity.h"
#include "CTakeDamageInfo.h"
#include "CCombatCharacter.h"
#include "PlayerState.h"
#include "in_buttons.h"

#define	PLAYER_USE_RADIUS	80.f
#define CONE_45_DEGREES		0.707f
#define CONE_15_DEGREES		0.9659258f
#define CONE_90_DEGREES		0

#define TRAIN_ACTIVE	0x80 
#define TRAIN_NEW		0xc0
#define TRAIN_OFF		0x00
#define TRAIN_NEUTRAL	0x01
#define TRAIN_SLOW		0x02
#define TRAIN_MEDIUM	0x03
#define TRAIN_FAST		0x04 
#define TRAIN_BACK		0x05

enum PlayerPhysFlag_e
{
	PFLAG_DIROVERRIDE	= ( 1<<0 ),		// override the player's directional control (trains, physics gun, etc.)
	PFLAG_DUCKING		= ( 1<<1 ),		// In the process of ducking, but totally squatted yet
	PFLAG_USING			= ( 1<<2 ),		// Using a continuous entity
	PFLAG_OBSERVER		= ( 1<<3 ),		// player is locked in stationary cam mode. Spectators can move, observers can't.
	PFLAG_VPHYSICS_MOTIONCONTROLLER = ( 1<<4 ),	// player is physically attached to a motion controller
	PFLAG_GAMEPHYSICS_ROTPUSH = (1<<5), // game physics did a rotating push that we may want to override with vphysics

	// If you add another flag here check that you aren't 
	// overwriting phys flags in the HL2 of TF2 player classes
};

class CViewModel;

abstract_class Hooked_CPlayer : public CCombatCharacter
{
public:
	DECLARE_CLASS(Hooked_CPlayer, CCombatCharacter);

	virtual CBaseEntity	*GiveNamedItem( const char *szName, int iSubType = 0 );
	virtual void PreThink();
	virtual void PostThink();
	virtual CBaseEntity *FindUseEntity();
	virtual bool IsUseableEntity( CBaseEntity *pEntity, unsigned int requiredCaps );
	virtual bool BumpWeapon( CBaseEntity *pWeapon );
	virtual void PickupObject( CBaseEntity *pObject, bool bLimitMassAndSize = true );
	virtual Vector GetAutoaimVector_Float( float flScale );
	virtual void ForceDropOfCarriedPhysObjects( CBaseEntity *pOnlyIfHoldingThis);
	virtual void ModifyOrAppendPlayerCriteria( AI_CriteriaSet& set );
	virtual void SetAnimation( PLAYER_ANIM playerAnim );

public:
	DECLARE_DEFAULTHEADER(GiveNamedItem, CBaseEntity *, (const char *szName, int iSubType));
	DECLARE_DEFAULTHEADER(PreThink, void, ());
	DECLARE_DEFAULTHEADER(PostThink, void, ());
	DECLARE_DEFAULTHEADER(FindUseEntity, CBaseEntity *, ());
	DECLARE_DEFAULTHEADER(IsUseableEntity, bool, (CBaseEntity *pEntity, unsigned int requiredCaps));
	DECLARE_DEFAULTHEADER(BumpWeapon, bool, (CBaseEntity *pWeapon));
	DECLARE_DEFAULTHEADER(PickupObject, void, (CBaseEntity *pObject, bool bLimitMassAndSize));
	DECLARE_DEFAULTHEADER(GetAutoaimVector_Float, Vector, ( float flScale));
	DECLARE_DEFAULTHEADER(ForceDropOfCarriedPhysObjects, void, (CBaseEntity *pOnlyIfHoldingThis));
	DECLARE_DEFAULTHEADER(ModifyOrAppendPlayerCriteria, void, (AI_CriteriaSet& set));
	DECLARE_DEFAULTHEADER(SetAnimation, void, (PLAYER_ANIM playerAnim));
};

class CPlayer : public Hooked_CPlayer
{
public:
	CE_DECLARE_CLASS(CPlayer, Hooked_CPlayer);
	DECLARE_DATADESC();

public:
	void	Spawn();
	void	PreThink();
	void	PostThink();

	void	ViewPunchReset( float tolerance );
	void	ViewPunch( const QAngle &angleOffset );
	void	VelocityPunch( const Vector &vecForce );
	bool	IsHLTV( void ) const { return pl.ptr->hltv; }
	bool	IsPlayer() { return true; }
	int		ArmorValue() const		{ return m_ArmorValue; }
	void	SetArmorValue(int value) {  m_ArmorValue = value; }
	bool	IsSuitEquipped() const	{ return m_bWearingSuit; }
	void	SetSuitEquipped(bool value) { m_bWearingSuit = value; }

	void	EyeVectors( Vector *pForward, Vector *pRight = NULL, Vector *pUp = NULL );
	void	CacheVehicleView( void );	// Calculate and cache the position of the player in the vehicle
	bool	ApplyBattery( float powerMultiplier = 1.0 );
	void	IncrementArmorValue( int nCount, int nMaxValue = -1 );
	
	bool HasAnyAmmoOfType( int nAmmoIndex );
	bool GiveAmmo(int nCount, int nAmmoIndex);

	CBaseEntity *FindUseEntity();
	CBaseEntity *FindUseEntity_Fix();

	CEntity	*DoubleCheckUseNPC( CEntity *pNPC, const Vector &vecSrc, const Vector &vecDir );
	
	void SetUseEntity( CEntity *pUseEntity );

	CEntity *GetHoldEntity() { return m_hHoldEntity; }
	void	SetHoldEntity(CBaseEntity *pEntity) { m_hHoldEntity.Set(pEntity); }

	float	MuzzleFlashTime() const { return m_flFlashTime; }
	void	SetMuzzleFlashTime( float flTime ) { m_flFlashTime = flTime; }

	void ForceDropOfCarriedPhysObjects( CEntity *pOnlyIfHoldingThis);
	bool ClearUseEntity();

	void ExitLadder();

	QAngle &GetPunchAngle() const
	{
		return m_vecPunchAngle;
	}

	virtual CViewModel *GetViewModel( int index=0 );

public:
	virtual int OnTakeDamage(const CTakeDamageInfo& info);
	virtual void Weapon_Drop( CBaseEntity *pWeapon, const Vector *pvecTarget = NULL, const Vector *pVelocity = NULL );
	virtual void Weapon_Equip( CBaseEntity *pWeapon );
	virtual void PickupObject( CBaseEntity *pObject, bool bLimitMassAndSize = true );
	virtual void ForceDropOfCarriedPhysObjects( CBaseEntity *pOnlyIfHoldingThis);
	virtual void TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator);

public: // input
	void InputForceDropPhysObjects( inputdata_t &data );

public:
	static bool	CanPickupObject( CEntity *pObject, float massLimit, float sizeLimit );

private:
	void	FixHL2Ladder();

	Vector	m_vecVehicleViewOrigin;		// Used to store the calculated view of the player while riding in a vehicle
	QAngle	m_vecVehicleViewAngles;		// Vehicle angles
	float	m_flVehicleViewFOV;			// FOV of the vehicle driver
	int		m_nVehicleViewSavedFrame;	// Used to mark which frame was the last one the view was calculated for
	float	m_flFlashTime;

	CFakeHandle		m_hHoldEntity;

public:
	bool m_bOnLadder;
	bool m_bOnBarnacle;


protected: //Sendprops
	DECLARE_SENDPROP(QAngle, m_vecPunchAngle);
	DECLARE_SENDPROP(QAngle, m_vecPunchAngleVel);
	DECLARE_SENDPROP(int, m_ArmorValue);
	DECLARE_SENDPROP(bool, m_bWearingSuit);
	DECLARE_SENDPROP(CFakeHandle, m_hUseEntity);

public:
	DECLARE_SENDPROP(int, m_iHideHUD);

protected: //Datamaps
	DECLARE_DATAMAP(CPlayerState, pl);
	DECLARE_DATAMAP(float, m_DmgSave);

public:
	DECLARE_DATAMAP(int, m_nButtons);
	DECLARE_DATAMAP(int, m_afButtonPressed);
	DECLARE_DATAMAP(int, m_afButtonReleased);
	DECLARE_DATAMAP(unsigned int, m_afPhysicsFlags);
	DECLARE_DATAMAP(int, m_iTrain);
	DECLARE_DATAMAP(int, m_nOldButtons);

};


inline void CPlayer::SetUseEntity( CEntity *pUseEntity ) 
{ 
	m_hUseEntity.ptr->Set((pUseEntity)?pUseEntity->BaseEntity(): NULL); 
}

inline CPlayer *ToBasePlayer( CEntity *pEntity )
{
	if ( !pEntity || !pEntity->IsPlayer() )
		return NULL;

	return static_cast<CPlayer *>( pEntity );
}


#endif // _INCLUDE_CPLAYER_H_
