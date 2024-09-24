
#ifndef WEAPON_STUNSTICK_H
#define WEAPON_STUNSTICK_H
#ifdef _WIN32
#pragma once
#endif

#include "CNPCBaseWeapon.h"

class CNPCWeapon_StunStick : public CNPCBaseBludgeonWeapon
{
public:
	DECLARE_CLASS( CNPCWeapon_StunStick, CNPCBaseBludgeonWeapon );

	void Spawn();
	void Precache();
	void Drop( const Vector &vecVelocity );
	const char *NPCWeaponGetWorldModel() const;
	acttable_t*	NPCWeaponActivityList();
	int	NPCWeaponActivityListCount();
	void NPCWeaponOperator_HandleAnimEvent( animevent_t *pEvent, CBaseEntity *pOperator );
	void NPCWeaponOperator_ForceNPCFire( CBaseEntity  *pOperator, bool bSecondary );
	const WeaponProficiencyInfo_t *NPCWeaponGetProficiencyValues();

	float	GetDamageForActivity( Activity hitActivity );
	int		WeaponMeleeAttack1Condition( float flDot, float flDist );

	float	GetFireRate( void )		{ return 0.8f; }
	void	SetStunState( bool state );
	bool	GetStunState( void );

private:
	bool	m_bActive;

private:
	static acttable_t m_acttable[];

};


#endif