
#ifndef _INCLUDE_CNPCWEAPON_H
#define _INCLUDE_CNPCWEAPON_H

#include "CEntity.h"
#include "CCombatWeapon.h"

abstract_class CNPCBaseWeapon : public CCombatWeapon
{
public:
	DECLARE_CLASS( CNPCBaseWeapon, CCombatWeapon );

	CNPCBaseWeapon();

	bool			IsNPCUsing() { return m_bIsNPCUsing; }

	void			Drop( const Vector &vecVelocity );
	void			Equip( CBaseEntity *pOwner );
	void			Operator_ForceNPCFire( CBaseEntity  *pOperator, bool bSecondary );
	void			Operator_HandleAnimEvent( animevent_t *pEvent, CBaseEntity *pOperator );

	acttable_t*		ActivityList();
	int				ActivityListCount();

	const char *	GetWorldModel( void ) const;
	const			WeaponProficiencyInfo_t *GetProficiencyValues();

	void			Touch( CEntity *pOther );

	void			DelayRemove();

public:
	virtual	void			OnNPCEquip(CCombatCharacter *owner) { };
	virtual const char *	NPCWeaponGetWorldModel() const =0;
	virtual acttable_t*		NPCWeaponActivityList() =0;
	virtual int				NPCWeaponActivityListCount() =0;
	virtual void			NPCWeaponOperator_HandleAnimEvent( animevent_t *pEvent, CBaseEntity *pOperator ) =0;
	virtual const WeaponProficiencyInfo_t *NPCWeaponGetProficiencyValues() =0;
	virtual void			NPCWeaponOperator_ForceNPCFire( CBaseEntity  *pOperator, bool bSecondary ) =0;

protected:
	bool m_bIsNPCUsing;
	int m_iWeaponModel;

private:
	static acttable_t m_acttable[];


};


abstract_class CNPCBaseBludgeonWeapon : public CNPCBaseWeapon
{
public:
	DECLARE_CLASS( CNPCBaseBludgeonWeapon, CNPCBaseWeapon );

	virtual Activity	GetPrimaryAttackActivity();
	virtual Activity	GetSecondaryAttackActivity();
	virtual int			Weapon_CapabilitiesGet( void );
	virtual	int			WeaponMeleeAttack1Condition( float flDot, float flDist );
	virtual	float		GetFireRate( void );

};


abstract_class CNPCBaseMachineGunWeapon : public CNPCBaseWeapon
{
public:
	DECLARE_CLASS( CNPCBaseMachineGunWeapon, CNPCBaseWeapon );

	virtual float	GetFireRate() = 0;
	virtual int		Weapon_CapabilitiesGet();
	virtual int		WeaponRangeAttack1Condition( float flDot, float flDist );
	virtual const Vector &GetBulletSpread();

};



abstract_class CNPCBaseSelectFireMachineGunWeapon : public CNPCBaseMachineGunWeapon
{
public:
	DECLARE_CLASS( CNPCBaseSelectFireMachineGunWeapon, CNPCBaseMachineGunWeapon );

	virtual float	GetFireRate();
	virtual int		WeaponRangeAttack1Condition( float flDot, float flDist );
	virtual int		WeaponRangeAttack2Condition( float flDot, float flDist );
};











#endif