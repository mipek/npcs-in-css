
#ifndef _INCLUDE_WEAPON_RPG_REPLACE_H_
#define _INCLUDE_WEAPON_RPG_REPLACE_H_

#include "CEntity.h"
#include "weapon_rpg.h"
#include "CGrenade.h"
#include "CCombatWeapon.h"
#include "CCombatCharacter.h"
#include "CNPCBaseWeapon.h"
#include "CGrenadeWeapon.h"



class CWeaponRPG;

class CE_FlashbangProjectile : public CE_Grenade
{
public:
	CE_DECLARE_CLASS( CE_FlashbangProjectile, CE_Grenade );

	void Spawn();

	static CWeaponRPG *CURRENT_RPG;
	static CPlayer *CURRENT_THROWER;

};




class CWeaponRPG : public CGrenadeWeapon
{
public:
	CE_DECLARE_CLASS( CWeaponRPG, CGrenadeWeapon );
	DECLARE_DATADESC();

	CWeaponRPG();

	void	Spawn();
	void	Precache();
	void	Activate();
	void	ItemPostFrame();

	void	StartGuiding( void );
	void	StopGuiding( void );
	void	ToggleGuiding( void );
	bool	IsGuiding( void );
	void	NotifyRocketDied();

	bool	HasAnyAmmo( void );

	void Think();

	void EmitGrenade(Vector vecSrc, QAngle vecAngles, Vector vecVel, AngularImpulse angImpulse, CBaseEntity *pPlayer);

	void Drop( const Vector &vecVelocity );
	const char *GetWorldModel( void ) const;
	void	Equip( CBaseEntity *pOwner );
	bool	Holster( CBaseEntity *pSwitchingTo );
	bool	CanHolster( void );
	bool	Deploy( void );
	void	SuppressGuiding( bool state = true );
	void	CreateLaserPointer( void );
	void	UpdateLaserPosition( Vector vecMuzzlePos = vec3_origin, Vector vecEndPos = vec3_origin );
	Vector	GetLaserPosition( void );

	CEntity *GetMissile( void ) { return m_hMissile; }

	inline	bool IsRPGReplace() const { return m_bIsRPG; }
	inline	void SetIsRPG(bool is_rpg) { m_bIsRPG = is_rpg; }
	inline	void SetDeloyed(bool deloyed) { m_bDeployed = deloyed; }

	bool	CreateRPG(CPlayer *player);


	void	UpdateNPCLaserPosition( const Vector &vecTarget );
	void	SetNPCLaserPosition( const Vector &vecTarget );
	const Vector &GetNPCLaserPosition( void );

public:
	static bool IS_REPLACE_SPAWN;

private:
	CFakeHandle		m_hMissile;
	CEFakeHandle<CLaserDot>	m_hLaserDot;
	bool			m_bGuiding;
	bool			m_bHideGuiding;
	bool			m_bInitialStateUpdate;
	Vector			m_vecNPCLaserDot;

	int				m_iWeaponModel;
	bool			m_bDeployed;
	bool			m_bIsRPG;

};

CWeaponRPG *ToCWeaponRPG(CEntity *cent);
CWeaponRPG *ToCWeaponRPG(CBaseEntity *cbase);


#endif
