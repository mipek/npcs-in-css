#ifndef WEAPON_SMG1_H
#define WEAPON_SMG1_H
#ifdef _WIN32
#pragma once
#endif

#include "CNPCBaseWeapon.h"
#include "CE_recipientfilter.h"
#include "CAI_NPC.h"


class CNPCWeapon_SMG1 : public CNPCBaseSelectFireMachineGunWeapon
{
public:
	DECLARE_CLASS( CNPCWeapon_SMG1, CNPCBaseSelectFireMachineGunWeapon );

	void Spawn();
	void Precache();
	const char *NPCWeaponGetWorldModel() const;
	acttable_t*	NPCWeaponActivityList();
	int	NPCWeaponActivityListCount();
	void NPCWeaponOperator_HandleAnimEvent( animevent_t *pEvent, CBaseEntity *pOperator );
	void NPCWeaponOperator_ForceNPCFire( CBaseEntity  *pOperator, bool bSecondary );
	const WeaponProficiencyInfo_t *NPCWeaponGetProficiencyValues();

	void OnNPCEquip(CCombatCharacter *owner);

private:
	void FireNPCPrimaryAttack( CCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir );

private:
	static acttable_t m_acttable[];

};


#endif