
#include "CCombatWeapon.h"
#include "ItemRespawnSystem.h"
#include "CPlayer.h"
#include "CE_recipientfilter.h"
#include "ammodef.h"
#include "vphysics/constraints.h"
#include "CAI_NPC.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CE_LINK_ENTITY_TO_CLASS(CBaseCombatWeapon, CCombatWeapon);

ConVar sv_css_weapon_respawn_time( "sv_css_weapon_respawn_time", "20", FCVAR_GAMEDLL | FCVAR_NOTIFY );

SH_DECL_MANUALHOOK1_void(Drop, 0, 0, 0, const Vector &);
DECLARE_HOOK(Drop, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER_void(Template_CCombatWeapon, Drop, (const Vector &vecVelocity), (vecVelocity));

SH_DECL_MANUALHOOK0(GetMaxClip1, 0, 0, 0, int);
DECLARE_HOOK(GetMaxClip1, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER(Template_CCombatWeapon, GetMaxClip1, int, () const, ());

SH_DECL_MANUALHOOK0(GetSlot, 0, 0, 0, int);
DECLARE_HOOK(GetSlot, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER(Template_CCombatWeapon, GetSlot, int, () const, ());

SH_DECL_MANUALHOOK0(HasPrimaryAmmo, 0, 0, 0, bool);
DECLARE_HOOK(HasPrimaryAmmo, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER(Template_CCombatWeapon, HasPrimaryAmmo, bool, (), ());

SH_DECL_MANUALHOOK0(UsesClipsForAmmo1, 0, 0, 0, bool);
DECLARE_HOOK(UsesClipsForAmmo1, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER(Template_CCombatWeapon, UsesClipsForAmmo1, bool, () const, ());

SH_DECL_MANUALHOOK0(HasSecondaryAmmo, 0, 0, 0, bool);
DECLARE_HOOK(HasSecondaryAmmo, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER(Template_CCombatWeapon, HasSecondaryAmmo, bool, (), ());

SH_DECL_MANUALHOOK0(UsesClipsForAmmo2, 0, 0, 0, bool);
DECLARE_HOOK(UsesClipsForAmmo2, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER(Template_CCombatWeapon, UsesClipsForAmmo2, bool, () const, ());

SH_DECL_MANUALHOOK1(Holster, 0, 0, 0, bool, CBaseEntity *);
DECLARE_HOOK(Holster, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER(Template_CCombatWeapon, Holster, bool, (CBaseEntity *pSwitchingTo), (pSwitchingTo));

SH_DECL_MANUALHOOK0(Deploy, 0, 0, 0, bool);
DECLARE_HOOK(Deploy, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER(Template_CCombatWeapon, Deploy, bool, (), ());

SH_DECL_MANUALHOOK0(CanHolster, 0, 0, 0, bool);
DECLARE_HOOK(CanHolster, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER(Template_CCombatWeapon, CanHolster, bool, (), ());

SH_DECL_MANUALHOOK0(Reload, 0, 0, 0, bool);
DECLARE_HOOK(Reload, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER(Template_CCombatWeapon, Reload, bool, (), ());

SH_DECL_MANUALHOOK0(GetRandomBurst, 0, 0, 0, int);
DECLARE_HOOK(GetRandomBurst, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER(Template_CCombatWeapon, GetRandomBurst, int, (), ());

SH_DECL_MANUALHOOK0(GetFireRate, 0, 0, 0, float);
DECLARE_HOOK(GetFireRate, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER(Template_CCombatWeapon, GetFireRate, float, (), ());

SH_DECL_MANUALHOOK0(Weapon_CapabilitiesGet, 0, 0, 0, int);
DECLARE_HOOK(Weapon_CapabilitiesGet, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER(Template_CCombatWeapon, Weapon_CapabilitiesGet, int, (), ());

SH_DECL_MANUALHOOK2_void(Operator_HandleAnimEvent, 0, 0, 0, animevent_t *, CBaseEntity *);
DECLARE_HOOK(Operator_HandleAnimEvent, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER_void(Template_CCombatWeapon, Operator_HandleAnimEvent, (animevent_t *pEvent, CBaseEntity *pOperator), (pEvent, pOperator));

SH_DECL_MANUALHOOK2_void(WeaponSound, 0, 0, 0, WeaponSound_t , float );
DECLARE_HOOK(WeaponSound, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER_void(Template_CCombatWeapon, WeaponSound, (WeaponSound_t sound_type, float soundtime), (sound_type, soundtime));

SH_DECL_MANUALHOOK0(GetMaxClip2, 0, 0, 0, int);
DECLARE_HOOK(GetMaxClip2, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER(Template_CCombatWeapon, GetMaxClip2, int, () const, ());

SH_DECL_MANUALHOOK2_void(Operator_ForceNPCFire, 0, 0, 0, CBaseEntity *, bool );
DECLARE_HOOK(Operator_ForceNPCFire, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER_void(Template_CCombatWeapon, Operator_ForceNPCFire, (CBaseEntity  *pOperator, bool bSecondary), (pOperator, bSecondary));

SH_DECL_MANUALHOOK1_void(Equip, 0, 0, 0, CBaseEntity *);
DECLARE_HOOK(Equip, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER_void(Template_CCombatWeapon, Equip, (CBaseEntity *pOwner), (pOwner));

SH_DECL_MANUALHOOK2(WeaponRangeAttack1Condition, 0, 0, 0, int, float, float);
DECLARE_HOOK(WeaponRangeAttack1Condition, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER(Template_CCombatWeapon, WeaponRangeAttack1Condition, int, (float flDot, float flDist), (flDot, flDist));

SH_DECL_MANUALHOOK2(WeaponRangeAttack2Condition, 0, 0, 0, int, float, float);
DECLARE_HOOK(WeaponRangeAttack2Condition, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER(Template_CCombatWeapon, WeaponRangeAttack2Condition, int, (float flDot, float flDist), (flDot, flDist));

SH_DECL_MANUALHOOK2(WeaponMeleeAttack1Condition, 0, 0, 0, int, float, float);
DECLARE_HOOK(WeaponMeleeAttack1Condition, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER(Template_CCombatWeapon, WeaponMeleeAttack1Condition, int, (float flDot, float flDist), (flDot, flDist));

SH_DECL_MANUALHOOK2(WeaponMeleeAttack2Condition, 0, 0, 0, int, float, float);
DECLARE_HOOK(WeaponMeleeAttack2Condition, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER(Template_CCombatWeapon, WeaponMeleeAttack2Condition, int, (float flDot, float flDist), (flDot, flDist));

SH_DECL_MANUALHOOK1_void(SendWeaponAnim, 0, 0, 0, int);
DECLARE_HOOK(SendWeaponAnim, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER_void(Template_CCombatWeapon, SendWeaponAnim, (int iActivity), (iActivity));

SH_DECL_MANUALHOOK0_void(ItemPostFrame, 0, 0, 0);
DECLARE_HOOK(ItemPostFrame, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER_void(Template_CCombatWeapon, ItemPostFrame, (), ());

SH_DECL_MANUALHOOK2_void(Weapon_SetActivity, 0, 0, 0, Activity, float);
DECLARE_HOOK(Weapon_SetActivity, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER_void(Template_CCombatWeapon, Weapon_SetActivity, (Activity activity, float flDuration), (activity, flDuration));

SH_DECL_MANUALHOOK2(ActivityOverride, 0, 0, 0, Activity, Activity, bool*);
DECLARE_HOOK(ActivityOverride, Template_CCombatWeapon);
//DECLARE_DEFAULTHANDLER(Template_CCombatWeapon, ActivityOverride, Activity, (Activity activity, bool *pRequired), (activity, pRequired));
Activity Template_CCombatWeapon::ActivityOverride( Activity baseAct, bool *pRequired ) // This is our custom implementation of ActivityOverride because ActivityListCount seems to be inlined..
{
	acttable_t *pTable = ActivityList();
	int actCount = ActivityListCount();

	for ( int i = 0; i < actCount; i++, pTable++ )
	{
		if ( baseAct == pTable->baseAct )
		{
			if (pRequired)
			{
				*pRequired = pTable->required;
			}
			return (Activity)pTable->weaponAct;
		}
	}
	return baseAct;
}
Activity Template_CCombatWeapon::InternalActivityOverride(Activity activity, bool *pRequired)
{
	SET_META_RESULT(MRES_SUPERCEDE);
	Template_CCombatWeapon *pEnt = (Template_CCombatWeapon *)CEntity::Instance(META_IFACEPTR(CBaseEntity));
	if (!pEnt)
		RETURN_META_VALUE(MRES_IGNORED, ACT_INVALID);
	pEnt->m_bInActivityOverride = true;
	Activity retvalue = pEnt->ActivityOverride(activity, pRequired);
	pEnt->m_bInActivityOverride = false; // No need to check if pEnt is still valid because CE_ActivityOverride won't do any harm to it
	return retvalue;
}

// Sendprops
DEFINE_PROP(m_iPrimaryAmmoType, CCombatWeapon);
DEFINE_PROP(m_iSecondaryAmmoType, CCombatWeapon);
DEFINE_PROP(m_iClip1, CCombatWeapon);
DEFINE_PROP(m_iClip2, CCombatWeapon);
DEFINE_PROP(m_flNextPrimaryAttack, CCombatWeapon);
DEFINE_PROP(m_flNextSecondaryAttack, CCombatWeapon);
DEFINE_PROP(m_flTimeWeaponIdle, CCombatWeapon);
DEFINE_PROP(m_iState, CCombatWeapon);

//Datamaps
DEFINE_PROP(m_bRemoveable, CCombatWeapon);
DEFINE_PROP(m_bFireOnEmpty, CCombatWeapon);
DEFINE_PROP(m_bFiresUnderwater, CCombatWeapon);
//DEFINE_PROP(m_bAltFiresUnderwater, CCombatWeapon);
DEFINE_PROP(m_pConstraint, CCombatWeapon);
DEFINE_PROP(m_iSubType, CCombatWeapon);
DEFINE_PROP(m_fMinRange1, CCombatWeapon);
DEFINE_PROP(m_fMinRange2, CCombatWeapon);
DEFINE_PROP(m_fMaxRange1, CCombatWeapon);
DEFINE_PROP(m_fMaxRange2, CCombatWeapon);
DEFINE_PROP(m_fFireDuration, CCombatWeapon);

BEGIN_DATADESC_CENTITY( CCombatWeapon )
	//CE_DEFINE_INPUT( CItem<Template_CCombatWeapon>::m_bRespawn,		FIELD_BOOLEAN,	"mm_respawn" ),
	DEFINE_FIELD( m_flRaiseTime,		FIELD_TIME ),
	DEFINE_FIELD( m_flHolsterTime,		FIELD_TIME ),
	DEFINE_FIELD( m_iPrimaryAttacks,	FIELD_INTEGER ),
	DEFINE_FIELD( m_iSecondaryAttacks,	FIELD_INTEGER ),
END_DATADESC()

CCombatWeapon::CCombatWeapon():
	m_bRemoveOnDrop(false),
	m_flRaiseTime(0.0f), m_flHolsterTime(0.0f),
	m_iPrimaryAttacks(0), m_iSecondaryAttacks(0)
{
}

void CCombatWeapon::Spawn()
{
	if(m_bRemoveOnDrop)
	{
		m_bRespawn = false;
	}
	BaseClass::Spawn();
	if(m_bRespawn)
	{
		UTIL_DropToFloor( this, MASK_SOLID );
	}
	SetNextThink(gpGlobals->curtime + 0.1f);
}

bool CCombatWeapon::MyTouch( CPlayer *pPlayer )
{
	if ( !pPlayer->IsAllowedToPickupWeapons() )
		return false;

	if(pPlayer->HaveThisWeaponType(this))
	{
		// give sound
		int nCount = GetMaxClip1();
		if(nCount == -1)
			nCount = 1;
		bool ret = pPlayer->GiveAmmo(nCount, GetPrimaryAmmoType());
		if(ret)
		{
			CPASAttenuationFilter filter( pPlayer, "BaseCombatCharacter.AmmoPickup" );
			EmitSound( filter, pPlayer->entindex(), "BaseCombatCharacter.AmmoPickup" );
		}
		return ret;
	}

	CCombatWeapon *pWeapon = (CCombatWeapon *)CEntity::Instance(pPlayer->Weapon_GetSlot(GetSlot()));
	if(pWeapon)
		return false;
	
	//give sound
	pWeapon = (CCombatWeapon *)CEntity::Instance(pPlayer->GiveNamedItem(GetClassname()));
	if(pWeapon)
	{
		pWeapon->m_bRemoveOnDrop = true;
	}
	return true;
}


void CCombatWeapon::OnWeaponDrop(CPlayer *pOwner)
{
	if(m_bRemoveOnDrop)
	{
		UTIL_Remove(this);
	}
}

void CCombatWeapon::OnWeaponEquip( CPlayer *pOwner)
{
}

bool CCombatWeapon::GetObjectsOriginalParameters(Vector &vOriginalOrigin, QAngle &vOriginalAngles)
{
	if ( m_flNextResetCheckTime > gpGlobals->curtime )
		return false;

	vOriginalOrigin = GetOriginalSpawnOrigin();
	vOriginalAngles = GetOriginalSpawnAngles();

	m_flNextResetCheckTime = gpGlobals->curtime + sv_css_weapon_respawn_time.GetFloat();
	return true;
}

Activity CCombatWeapon::GetPrimaryAttackActivity( void )
{
	return ACT_VM_PRIMARYATTACK;
}

Activity CCombatWeapon::GetSecondaryAttackActivity( void )
{
	return ACT_VM_SECONDARYATTACK;
}

void CCombatWeapon::SetWeaponIdleTime( float time )
{
	m_flTimeWeaponIdle = time;
}

float CCombatWeapon::GetWeaponIdleTime( void )
{
	return m_flTimeWeaponIdle;
}

const WeaponProficiencyInfo_t *CCombatWeapon::GetDefaultProficiencyValues()
{
	// Weapon proficiency table. Keep this in sync with WeaponProficiency_t enum in the header!!
	static WeaponProficiencyInfo_t g_BaseWeaponProficiencyTable[] =
	{
		{ 2.50, 1.0	},
		{ 2.00, 1.0	},
		{ 1.50, 1.0	},
		{ 1.25, 1.0 },
		{ 1.00, 1.0	},
	};

	COMPILE_TIME_ASSERT( ARRAYSIZE(g_BaseWeaponProficiencyTable) == WEAPON_PROFICIENCY_PERFECT + 1);

	return g_BaseWeaponProficiencyTable;
}