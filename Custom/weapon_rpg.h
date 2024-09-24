#ifndef _INCLUDE_WEAPON_RPG_H_
#define _INCLUDE_WEAPON_RPG_H_

#include "CEntity.h"
#include "CGrenade.h"
#include "CSmoke_trail.h"
#include "CCombatWeapon.h"
#include "CCycler_Fix.h"
#include "CSprite.h"
#include "CSoda_Fix.h"

class CLaserDot;
class CWeaponRPG;


class CMissile : public CSode_Fix
{
public:
	CE_DECLARE_CLASS( CMissile, CSode_Fix );
	DECLARE_DATADESC();

	static const int EXPLOSION_RADIUS = 200;

	CMissile();
	~CMissile();

	Class_T Classify( void ) { return CLASS_MISSILE; }

	void	Spawn( void );
	void	Precache( void );
	void	MissileTouch( CEntity *pOther );
	void	Explode( void );
	void	ShotDown( void );
	void	AccelerateThink( void );
	void	AugerThink( void );
	void	IgniteThink( void );
	void	SeekThink( void );
	void	DumbFire( void );
	void	SetGracePeriod( float flGracePeriod );
	void	Activate() { }

	int		OnTakeDamage( const CTakeDamageInfo &info );
	int		OnTakeDamage_Dead( const CTakeDamageInfo &info );
	int		CBaseCombatCharacter_OnTakeDamage_Alive( const CTakeDamageInfo &info );
	int		OnTakeDamage_Alive( const CTakeDamageInfo &info );
	void	Event_Killed( const CTakeDamageInfo &info );

	virtual float	GetDamage() { return m_flDamage; }
	virtual void	SetDamage(float flDamage) { m_flDamage = flDamage; }

	unsigned int PhysicsSolidMaskForEntity( void ) const;

	CEFakeHandle<CWeaponRPG>		m_hOwner;

	static CMissile *Create( const Vector &vecOrigin, const QAngle &vecAngles, CEntity *pentOwner = NULL );

	static void AddCustomDetonator( CBaseEntity *pEntity, float radius, float height = -1 );
	static void RemoveCustomDetonator( CBaseEntity *pEntity );

protected:
	virtual void DoExplosion();
	virtual void ComputeActualDotPosition( CLaserDot *pLaserDot, Vector *pActualDotPosition, float *pHomingSpeed );
	virtual int AugerHealth() { return *(m_iMaxHealth) - 20; }

	// Creates the smoke trail
	void CreateSmokeTrail( void );

	// Gets the shooting position 
	void GetShootPosition( CLaserDot *pLaserDot, Vector *pShootPosition );

	CEFakeHandle<CRocketTrail>	m_hRocketTrail;
	float					m_flAugerTime;		// Amount of time to auger before blowing up anyway
	float					m_flMarkDeadTime;
	float					m_flDamage;

	struct CustomDetonator_t
	{
		CFakeHandle hEntity;
		float radiusSq;
		float halfHeight;
	};

	static CUtlVector<CustomDetonator_t> gm_CustomDetonators;


private:
	float					m_flGracePeriodEndsAt;
	float					m_flDamageAccumulator;

};


class CLaserDot : public CE_CSprite
{
public:
	CE_DECLARE_CLASS( CLaserDot, CE_CSprite );
	DECLARE_DATADESC();

	CLaserDot( void );
	~CLaserDot( void );

	static CLaserDot *Create( const Vector &origin, CBaseEntity *pOwner = NULL, bool bVisibleDot = true );

	void	SetTargetEntity( CBaseEntity *pTarget ) { m_hTargetEnt = pTarget; }
	CEntity *GetTargetEntity( void ) { return CEntity::Instance(m_hTargetEnt); }

	void	SetLaserPosition( const Vector &origin, const Vector &normal );
	Vector	GetChasePosition();
	void	TurnOn( void );
	void	TurnOff( void );
	bool	IsOn() const { return m_bIsOn; }

	void	Toggle( void );

	int		ObjectCaps() { return (BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_DONT_SAVE; }

	void	MakeInvisible( void );

public:
	static CLaserDot *LaserDotCreate( const char *pSpriteName, const Vector &origin, bool animate );

protected:
	Vector				m_vecSurfaceNormal;
	EHANDLE				m_hTargetEnt;
	bool				m_bVisibleLaserDot;
	bool				m_bIsOn;

public:
	CLaserDot			*m_pNext;
};


//-----------------------------------------------------------------------------
// Specialized mizzizzile
//-----------------------------------------------------------------------------
class CAPCMissile : public CMissile
{
public:
	CE_DECLARE_CLASS( CAPCMissile, CMissile );
	DECLARE_DATADESC();

	static CAPCMissile *Create( const Vector &vecOrigin, const QAngle &vecAngles, const Vector &vecVelocity, CEntity *pOwner );

	CAPCMissile();
	~CAPCMissile();
	void	IgniteDelay( void );
	void	AugerDelay( float flDelayTime );
	void	ExplodeDelay( float flDelayTime );
	void	DisableGuiding();

	virtual Class_T Classify ( void ) { return CLASS_COMBINE; }

	void	AimAtSpecificTarget( CBaseEntity *pTarget );
	void	SetGuidanceHint( const char *pHintName );

	void	APCSeekThink( void );

	CAPCMissile			*m_pNext;

protected:
	virtual void DoExplosion();
	virtual void ComputeActualDotPosition( CLaserDot *pLaserDot, Vector *pActualDotPosition, float *pHomingSpeed );
	virtual int AugerHealth();

private:
	void Init();
	void ComputeLeadingPosition( const Vector &vecShootPosition, CEntity *pTarget, Vector *pLeadPosition );
	void BeginSeekThink();
	void AugerStartThink();
	void ExplodeThink();
	void APCMissileTouch( CEntity *pOther );

	float	m_flReachedTargetTime;
	float	m_flIgnitionTime;
	bool	m_bGuidingDisabled;
	float   m_flLastHomingSpeed;
	CFakeHandle m_hSpecificTarget;
	string_t m_strHint;
};


//-----------------------------------------------------------------------------
// Finds apc missiles in cone
//-----------------------------------------------------------------------------
CAPCMissile *FindAPCMissileInCone( const Vector &vecOrigin, const Vector &vecDirection, float flAngle );




CEntity *CreateLaserDot( const Vector &origin, CBaseEntity *pOwner, bool bVisibleDot );
void SetLaserDotTarget( CEntity *pLaserDot, CBaseEntity *pTarget );
void EnableLaserDot( CEntity *pLaserDot, bool bEnable );


#endif