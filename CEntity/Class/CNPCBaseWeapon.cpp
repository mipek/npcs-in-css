#include "CNPCBaseWeapon.h"
#include "CE_recipientfilter.h"
#include "CAI_NPC.h"
#include "npc_combine.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



/* CNPCBaseWeapon */
static const char *s_pDelayRemoveThinkContext = "DelayRemoveThinkContext";


CNPCBaseWeapon::CNPCBaseWeapon(): m_bIsNPCUsing(false), m_iWeaponModel(0)
{

}

void CNPCBaseWeapon::DelayRemove()
{
	if(m_bIsNPCUsing)
	{
		UTIL_Remove(this);
	}
}

void CNPCBaseWeapon::Drop( const Vector &vecVelocity )
{
	if(m_bIsNPCUsing)
	{
		SetContextThink(&CNPCBaseWeapon::DelayRemove, gpGlobals->curtime + 15.0f, s_pDelayRemoveThinkContext);
	}
	BaseClass::Drop(vecVelocity);
}

void CNPCBaseWeapon::Equip( CBaseEntity *pOwner )
{
	SetContextThink( NULL, -1, s_pDelayRemoveThinkContext );

	m_bIsNPCUsing = false;
	CCombatCharacter *owner = (CCombatCharacter *)CEntity::Instance(pOwner);
	if(owner)
	{
		m_bIsNPCUsing = owner->IsNPC();
	}

	if(m_bIsNPCUsing)
	{
		OnNPCEquip(owner);
	} else {
		RestoreMinMaxRange();
	}

	BaseClass::Equip(pOwner);

	if(m_bIsNPCUsing)
	{
		m_iWorldModelIndex = m_iWeaponModel;
	}
}

const char *CNPCBaseWeapon::GetWorldModel( void ) const
{
	if(m_bIsNPCUsing)
		return NPCWeaponGetWorldModel();
	else
		return BaseClass::GetWorldModel();
}

acttable_t*	CNPCBaseWeapon::ActivityList()
{
	if(m_bIsNPCUsing)
		return NPCWeaponActivityList();
	else
		return BaseClass::ActivityList();
}

int	CNPCBaseWeapon::ActivityListCount()
{
	if(m_bIsNPCUsing)
		return NPCWeaponActivityListCount();
	else
		return BaseClass::ActivityListCount();
}


void CNPCBaseWeapon::Operator_HandleAnimEvent( animevent_t *pEvent, CBaseEntity *pOperator )
{
	if(!m_bIsNPCUsing)
	{
		BaseClass::Operator_HandleAnimEvent(pEvent, pOperator);
		return;
	}

	return NPCWeaponOperator_HandleAnimEvent(pEvent, pOperator);


}

void CNPCBaseWeapon::Operator_ForceNPCFire( CBaseEntity  *pOperator, bool bSecondary )
{
	if(!m_bIsNPCUsing)
	{
		BaseClass::Operator_ForceNPCFire(pOperator, bSecondary);
		return;
	}

	NPCWeaponOperator_ForceNPCFire(pOperator, bSecondary);
}

const WeaponProficiencyInfo_t *CNPCBaseWeapon::GetProficiencyValues()
{
	if(!m_bIsNPCUsing)
	{
		return BaseClass::GetProficiencyValues();
	}

	return NPCWeaponGetProficiencyValues();
}

void CNPCBaseWeapon::Touch( CEntity *pOther )
{
	if(!m_bIsNPCUsing)
	{
		BaseClass::Touch(pOther);
		return;
	}

	if ( pOther->GetServerVehicle() )
	{
		pOther = CEntity::Instance(pOther->GetServerVehicle()->GetPassenger());
		if ( !pOther )
			return;
	}

	if ( !pOther->IsPlayer() )
		return;

	CPlayer *pPlayer = (CPlayer *)pOther;

	if ( ItemCanBeTouchedByPlayer( pPlayer ) == false )
		return;

	m_OnCacheInteraction.FireOutput(pOther, this);

	if ( !pPlayer->IsAllowedToPickupWeapons() )
		return;

	CEntity *ret = BaseClass::MyTouch( pPlayer);
	if(ret != NULL)
	{
		m_OnPlayerTouch.FireOutput(pOther, this);
		if(ret == this)
		{
			UTIL_Remove(this);
		}
	}
}



/* CNPCBaseBludgeonWeapon */
Activity CNPCBaseBludgeonWeapon::GetPrimaryAttackActivity()
{
	if(IsNPCUsing())
		return ACT_VM_HITCENTER;
	else
		return BaseClass::GetPrimaryAttackActivity();
}

Activity CNPCBaseBludgeonWeapon::GetSecondaryAttackActivity()
{
	if(IsNPCUsing())
		return ACT_VM_HITCENTER2;
	else
		return BaseClass::GetSecondaryAttackActivity();
}

int	CNPCBaseBludgeonWeapon::Weapon_CapabilitiesGet()
{
	if(IsNPCUsing())
		return bits_CAP_WEAPON_MELEE_ATTACK1;
	else
		return BaseClass::Weapon_CapabilitiesGet();
}

int	CNPCBaseBludgeonWeapon::WeaponMeleeAttack1Condition( float flDot, float flDist )
{
	if(!IsNPCUsing())
		return BaseClass::WeaponMeleeAttack1Condition(flDot, flDist);

	if (flDist > 64)
	{
		return COND_TOO_FAR_TO_ATTACK;
	}
	else if (flDot < 0.7)
	{
		return COND_NOT_FACING_ATTACK;
	}

	return COND_CAN_MELEE_ATTACK1;

}

float CNPCBaseBludgeonWeapon::GetFireRate( void )
{
	if(IsNPCUsing())
		return 0.2f;
	else
		return BaseClass::GetFireRate();
}



/* CNPCBaseMachineGunWeapon*/
int CNPCBaseMachineGunWeapon::Weapon_CapabilitiesGet()
{
	if(IsNPCUsing())
		return bits_CAP_WEAPON_RANGE_ATTACK1;
	else
		return BaseClass::Weapon_CapabilitiesGet();
}

int	CNPCBaseMachineGunWeapon::WeaponRangeAttack1Condition( float flDot, float flDist )
{
	if(!IsNPCUsing())
		return BaseClass::WeaponRangeAttack1Condition(flDot, flDist);

	if ( m_iClip1 <=0 )
	{
		return COND_NO_PRIMARY_AMMO;
	}
	else if ( flDist < m_fMinRange1 )
	{
		return COND_TOO_CLOSE_TO_ATTACK;
	}
	else if ( flDist > m_fMaxRange1 )
	{
		return COND_TOO_FAR_TO_ATTACK;
	}
	else if ( flDot < 0.5f )	// UNDONE: Why check this here? Isn't the AI checking this already?
	{
		return COND_NOT_FACING_ATTACK;
	}

	return COND_CAN_RANGE_ATTACK1;
}

const Vector &CNPCBaseMachineGunWeapon::GetBulletSpread( void )
{
	if(!IsNPCUsing())
		return BaseClass::GetBulletSpread();

	static Vector cone = VECTOR_CONE_3DEGREES;
	return cone;
}




/* CNPCBaseSelectFireMachineGunWeapon */
float CNPCBaseSelectFireMachineGunWeapon::GetFireRate()
{
	if(IsNPCUsing())
		return 0.1f;
	else
		return CNPCBaseWeapon::GetFireRate();
}

int	CNPCBaseSelectFireMachineGunWeapon::WeaponRangeAttack1Condition( float flDot, float flDist )
{
	if(!IsNPCUsing())
		return BaseClass::WeaponRangeAttack1Condition(flDot, flDist);

	if (m_iClip1 <=0)
	{
		return COND_NO_PRIMARY_AMMO;
	}
	else if ( flDist < m_fMinRange1)
	{
		return COND_TOO_CLOSE_TO_ATTACK;
	}
	else if (flDist > m_fMaxRange1)
	{
		return COND_TOO_FAR_TO_ATTACK;
	}
	else if (flDot < 0.5)	// UNDONE: Why check this here? Isn't the AI checking this already?
	{
		return COND_NOT_FACING_ATTACK;
	}

	return COND_CAN_RANGE_ATTACK1;
}

int	CNPCBaseSelectFireMachineGunWeapon::WeaponRangeAttack2Condition( float flDot, float flDist )
{
	if(!IsNPCUsing())
		return BaseClass::WeaponRangeAttack2Condition(flDot, flDist);

	return COND_NONE; // FIXME: disabled for now

	// m_iClip2 == -1 when no secondary clip is used
	if ( m_iClip2 == 0 && UsesSecondaryAmmo() )
	{
		return COND_NO_SECONDARY_AMMO;
	}
	else if ( flDist < m_fMinRange2 )
	{
		// Don't return	COND_TOO_CLOSE_TO_ATTACK only for primary attack
		return COND_NONE;
	}
	else if (flDist > m_fMaxRange2 )
	{
		// Don't return COND_TOO_FAR_TO_ATTACK only for primary attack
		return COND_NONE;
	}
	else if ( flDot < 0.5 ) // UNDONE: Why check this here? Isn't the AI checking this already?
	{
		return COND_NOT_FACING_ATTACK;
	}

	return COND_CAN_RANGE_ATTACK2;
}


