
#include "CCombatWeapon.h"
#include "ItemRespawnSystem.h"
#include "CPlayer.h"
#include "CE_recipientfilter.h"
#include "ammodef.h"
#include "vphysics/constraints.h"
#include "CAI_NPC.h"
#include "weapon_rpg_replace.h"
#include "weapon_physcannon_replace.h"

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

SH_DECL_MANUALHOOK0_void(PrimaryAttack, 0, 0, 0);
DECLARE_HOOK(PrimaryAttack, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER_void(Template_CCombatWeapon, PrimaryAttack, (), ());

SH_DECL_MANUALHOOK0_void(SecondaryAttack, 0, 0, 0);
DECLARE_HOOK(SecondaryAttack, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER_void(Template_CCombatWeapon, SecondaryAttack, (), ());

SH_DECL_MANUALHOOK1_void(SendWeaponAnim, 0, 0, 0, int);
DECLARE_HOOK(SendWeaponAnim, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER_void(Template_CCombatWeapon, SendWeaponAnim, (int iActivity), (iActivity));

SH_DECL_MANUALHOOK0_void(ItemPostFrame, 0, 0, 0);
DECLARE_HOOK(ItemPostFrame, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER_void(Template_CCombatWeapon, ItemPostFrame, (), ());

SH_DECL_MANUALHOOK2_void(Weapon_SetActivity, 0, 0, 0, Activity, float);
DECLARE_HOOK(Weapon_SetActivity, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER_void(Template_CCombatWeapon, Weapon_SetActivity, (Activity activity, float flDuration), (activity, flDuration));

SH_DECL_MANUALHOOK0(ActivityList, 0, 0, 0, acttable_t *);
DECLARE_HOOK(ActivityList, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER(Template_CCombatWeapon, ActivityList, acttable_t *, (), ());

//SH_DECL_MANUALHOOK0(ActivityListCount, 0, 0, 0, int);
//DECLARE_HOOK(ActivityListCount, Template_CCombatWeapon);
//DECLARE_DEFAULTHANDLER(Template_CCombatWeapon, ActivityListCount, int, (), ());

SH_DECL_MANUALHOOK2(ActivityOverride, 0, 0, 0, Activity, Activity, bool *);
DECLARE_HOOK(ActivityOverride, Template_CCombatWeapon);
//DECLARE_DEFAULTHANDLER(Template_CCombatWeapon, ActivityOverride, Activity, (Activity baseAct, bool *pRequire), (baseAct, pRequire));

// This is our custom implementation of ActivityOverride because ActivityListCount seems to be inlined..
Activity Template_CCombatWeapon::ActivityOverride( Activity baseAct, bool *pRequired )
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

SH_DECL_MANUALHOOK0(GetWorldModel, 0, 0, 0, const char *);
DECLARE_HOOK(GetWorldModel, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER(Template_CCombatWeapon, GetWorldModel, const char *, () const, ());

SH_DECL_MANUALHOOK0(GetProficiencyValues, 0, 0, 0, const WeaponProficiencyInfo_t *);
DECLARE_HOOK(GetProficiencyValues, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER(Template_CCombatWeapon, GetProficiencyValues, const WeaponProficiencyInfo_t *, (), ());

SH_DECL_MANUALHOOK0(GetPrimaryAttackActivity, 0, 0, 0, Activity);
DECLARE_HOOK(GetPrimaryAttackActivity, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER(Template_CCombatWeapon, GetPrimaryAttackActivity, Activity, (), ());

SH_DECL_MANUALHOOK0(GetSecondaryAttackActivity, 0, 0, 0, Activity);
DECLARE_HOOK(GetSecondaryAttackActivity, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER(Template_CCombatWeapon, GetSecondaryAttackActivity, Activity, (), ());

SH_DECL_MANUALHOOK0(HasAnyAmmo, 0, 0, 0, bool);
DECLARE_HOOK(HasAnyAmmo, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER(Template_CCombatWeapon, HasAnyAmmo, bool, (), ());

SH_DECL_MANUALHOOK0_void(ItemPreFrame, 0, 0, 0);
DECLARE_HOOK(ItemPreFrame, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER_void(Template_CCombatWeapon, ItemPreFrame, (), ());

SH_DECL_MANUALHOOK0_void(WeaponIdle, 0, 0, 0);
DECLARE_HOOK(WeaponIdle, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER_void(Template_CCombatWeapon, WeaponIdle, (), ());

SH_DECL_MANUALHOOK1(GetViewModel, 0, 0, 0, const char *, int);
DECLARE_HOOK(GetViewModel, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER(Template_CCombatWeapon, GetViewModel,const char *, (int viewmodelindex) const, (viewmodelindex));

SH_DECL_MANUALHOOK0(GetBulletSpread, 0, 0, 0, const Vector &);
DECLARE_HOOK(GetBulletSpread, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER_REFERENCE(Template_CCombatWeapon, GetBulletSpread,const Vector &, (), ());

SH_DECL_MANUALHOOK0(GetMinBurst, 0, 0, 0, int);
DECLARE_HOOK(GetMinBurst, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER(Template_CCombatWeapon, GetMinBurst, int, (), ());

SH_DECL_MANUALHOOK0(GetMaxBurst, 0, 0, 0, int);
DECLARE_HOOK(GetMaxBurst, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER(Template_CCombatWeapon, GetMaxBurst, int, (), ());

SH_DECL_MANUALHOOK0(GetMinRestTime, 0, 0, 0, float);
DECLARE_HOOK(GetMinRestTime, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER(Template_CCombatWeapon, GetMinRestTime, float, (), ());

SH_DECL_MANUALHOOK0(GetMaxRestTime, 0, 0, 0, float);
DECLARE_HOOK(GetMaxRestTime, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER(Template_CCombatWeapon, GetMaxRestTime, float, (), ());

SH_DECL_MANUALHOOK0(CanBePickedUpByNPCs, 0, 0, 0, bool);
DECLARE_HOOK(CanBePickedUpByNPCs, Template_CCombatWeapon);
DECLARE_DEFAULTHANDLER(Template_CCombatWeapon, CanBePickedUpByNPCs, bool, (), ());

// Sendprops
DEFINE_PROP(m_iPrimaryAmmoType, CCombatWeapon);
DEFINE_PROP(m_iSecondaryAmmoType, CCombatWeapon);
DEFINE_PROP(m_iClip1, CCombatWeapon);
DEFINE_PROP(m_iClip2, CCombatWeapon);
DEFINE_PROP(m_flNextPrimaryAttack, CCombatWeapon);
DEFINE_PROP(m_flNextSecondaryAttack, CCombatWeapon);
DEFINE_PROP(m_flTimeWeaponIdle, CCombatWeapon);
DEFINE_PROP(m_iState, CCombatWeapon);
DEFINE_PROP(m_iWorldModelIndex, CCombatWeapon);
DEFINE_PROP(m_hOwner, CCombatWeapon);
DEFINE_PROP(m_iViewModelIndex, CCombatWeapon);

//Datamaps
DEFINE_PROP(m_bRemoveable, CCombatWeapon);
DEFINE_PROP(m_bFireOnEmpty, CCombatWeapon);
DEFINE_PROP(m_bFiresUnderwater, CCombatWeapon);
//DEFINE_PROP(m_bAltFiresUnderwater, CCombatWeapon);
DEFINE_PROP(m_pConstraint, CCombatWeapon);
DEFINE_PROP(m_iSubType, CCombatWeapon);
DEFINE_PROP(m_IdealActivity, CCombatWeapon);
DEFINE_PROP(m_Activity, CCombatWeapon);
DEFINE_PROP(m_fMinRange1, CCombatWeapon);
DEFINE_PROP(m_fMinRange2, CCombatWeapon);
DEFINE_PROP(m_fMaxRange1, CCombatWeapon);
DEFINE_PROP(m_fMaxRange2, CCombatWeapon);
DEFINE_PROP(m_fFireDuration, CCombatWeapon);
DEFINE_PROP(m_flUnlockTime, CCombatWeapon);
DEFINE_PROP(m_hLocker, CCombatWeapon);

#if 0
BEGIN_DATADESC_CENTITY( CCombatWeapon )
	//CE_DEFINE_INPUT( CItem<Template_CCombatWeapon>::m_bRespawn,		FIELD_BOOLEAN,	"mm_respawn" ),
	DEFINE_FIELD( m_flRaiseTime,		FIELD_TIME ),
	DEFINE_FIELD( m_flHolsterTime,		FIELD_TIME ),
	DEFINE_FIELD( m_iPrimaryAttacks,	FIELD_INTEGER ),
	DEFINE_FIELD( m_iSecondaryAttacks,	FIELD_INTEGER ),
END_DATADESC()
#endif

BEGIN_DATADESC( CCombatWeapon )
	DEFINE_KEYFIELD( CItem<Template_CCombatWeapon>::m_bRespawn,		FIELD_BOOLEAN,	"mm_respawn" ),
END_DATADESC()

class CWeaponList : public CBaseGameSystem
{
public:
	CWeaponList( char const *name ) : CBaseGameSystem( name )
	{
	}
	virtual void LevelShutdownPostEntity()
	{
		m_list.Purge();
	}

	void AddWeapon( CCombatWeapon *pWeapon )
	{
		m_list.AddToTail( pWeapon );
	}

	void RemoveWeapon( CCombatWeapon *pWeapon )
	{
		m_list.FindAndRemove( pWeapon );
	}
	CUtlLinkedList< CCombatWeapon * > m_list;
};

CWeaponList g_WeaponList( "CWeaponList" );

CCombatWeapon::CCombatWeapon():
	m_bRemoveOnDrop(false),
	m_flRaiseTime(0.0f), m_flHolsterTime(0.0f),
	m_iPrimaryAttacks(0), m_iSecondaryAttacks(0)
{
	g_WeaponList.AddWeapon( this );
}

CCombatWeapon::~CCombatWeapon()
{
	g_WeaponList.RemoveWeapon( this );
}

void CCombatWeapon::PostConstructor()
{
	BaseClass::PostConstructor();
	if (m_Activity.offset == 0)
	{
		m_Activity.offset = m_IdealActivity.offset - 8;
		m_Activity.ptr = (Activity *)(((uint8_t *)(BaseEntity())) + m_Activity.offset);
	}
}

void CCombatWeapon::Spawn()
{
	if(m_bRemoveOnDrop)
	{
		m_bRespawn = false;
	}
	BaseClass::Spawn();
	/*if(m_bRespawn)
	{
		UTIL_DropToFloor( this, MASK_SOLID );
	}*/
	SetNextThink(gpGlobals->curtime + 0.1f);
	SaveMinMaxRange();
}

CEntity* CCombatWeapon::MyTouch( CPlayer *pPlayer )
{
	if(pPlayer->HaveThisWeaponType(this))
	{
		// give sound
		int nCount = GetMaxClip1();
		if(nCount == -1)
			nCount = 1;
		int ret = pPlayer->GiveAmmo(nCount, GetPrimaryAmmoType());
		if(ret > 0)
		{
			WeaponReplaceGiveAmmo(pPlayer);

			CPASAttenuationFilter filter( pPlayer, "BaseCombatCharacter.AmmoPickup" );
			EmitSound( filter, pPlayer->entindex(), "BaseCombatCharacter.AmmoPickup" );
			return this;
		}
		return NULL;
	}

	int slot = GetSlot();
	CCombatWeapon *pWeapon;
	if(slot != 3)
	{
		pWeapon = (CCombatWeapon *)CEntity::Instance(pPlayer->Weapon_GetSlot(slot));
		if(pWeapon)
			return NULL;
	}

	PreWeaponReplace(this);
	pWeapon = (CCombatWeapon *)CEntity::Instance(pPlayer->GiveNamedItem(GetClassname()));
	PostWeaponReplace();
	if(pWeapon)
	{
		pWeapon->m_bRemoveOnDrop = true;
		WeaponReplaceCreate(pWeapon);
		return pWeapon;
	}

	return NULL;
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

void CCombatWeapon::CustomWeaponSound(int index, const Vector &origin, const char *sound)
{
	CSoundParameters params;
	if(GetParametersForSound(sound, params, NULL))
	{
		CPASAttenuationFilter filter(origin, params.soundlevel );
		EmitSound( filter, index, sound, NULL, 0.0f );
	}
}

void CCombatWeapon::CustomWeaponSound(const Vector &origin, const char *sound, WeaponSound_t sound_type)
{
	CPlayer *pOwner = ToBasePlayer( GetOwner() );
	if(pOwner)
	{
		CustomWeaponSound(pOwner->entindex(), origin, sound);
		if( sound_type == EMPTY )
		{
			g_helpfunc.CSoundEnt_InsertSound( SOUND_COMBAT, pOwner->GetAbsOrigin(), SOUNDENT_VOLUME_EMPTY, 0.2, pOwner->BaseEntity() );
		}
	} else {
		CustomWeaponSound(entindex(), origin, sound);
	}
}


void CCombatWeapon::Lock( float lockTime, CEntity *pLocker )
{
	m_flUnlockTime = gpGlobals->curtime + lockTime;
	m_hLocker.ptr->Set( (pLocker)?pLocker->BaseEntity():NULL );
}

bool CCombatWeapon::IsLocked( CEntity *pAsker )
{
	CEntity *cent = m_hLocker;
	return ( m_flUnlockTime > gpGlobals->curtime && cent != pAsker );
}

void CCombatWeapon::SaveMinMaxRange()
{
	backup_m_fMinRange1 = m_fMinRange1;
	backup_m_fMinRange2 = m_fMinRange2;
	backup_m_fMaxRange1 = m_fMaxRange1;
	backup_m_fMaxRange2 = m_fMaxRange2;
}

void CCombatWeapon::RestoreMinMaxRange()
{
	m_fMinRange1 = backup_m_fMinRange1;
	m_fMinRange2 = backup_m_fMinRange2;
	m_fMaxRange1 = backup_m_fMaxRange1;
	m_fMaxRange2 = backup_m_fMaxRange2;
}

void CCombatWeapon::WeaponReplaceCreate(CCombatWeapon *pWeapon)
{
	CWeaponRPG *rpg = ToCWeaponRPG(this);
	if(rpg)
	{
		rpg = dynamic_cast<CWeaponRPG *>(pWeapon);
		if(rpg)
		{
			rpg->SetIsRPG(true);
		}
		return;
	}
	CWeaponPhysCannon *physcannon = ToCWeaponPhysCannon(this);
	if(physcannon)
	{
		physcannon = dynamic_cast<CWeaponPhysCannon *>(pWeapon);
		if(physcannon)
		{
			physcannon->SetIsPhysCannon(true);
		}
		return;
	}
}

void CCombatWeapon::WeaponReplaceGiveAmmo(CPlayer *pPlayer)
{
	CWeaponRPG *rpg = ToCWeaponRPG(this);
	if(rpg)
	{
		rpg = (CWeaponRPG*)pPlayer->GetThisWeaponType(this);
		if(rpg)
		{
			rpg->SetIsRPG(true);
		}
		return;
	}
	CWeaponPhysCannon *physcannon = ToCWeaponPhysCannon(this);
	if(physcannon)
	{
		physcannon = (CWeaponPhysCannon*)pPlayer->GetThisWeaponType(this);
		if(physcannon)
		{
			physcannon->SetIsPhysCannon(true);
		}
		return;
	}
}


int CCombatWeapon::GetAvailableWeaponsInBox( CCombatWeapon **pList, int listMax, const Vector &mins, const Vector &maxs )
{
	// linear search all weapons
	int count = 0;
	int index = g_WeaponList.m_list.Head();
	while ( index != g_WeaponList.m_list.InvalidIndex() )
	{
		CCombatWeapon *pWeapon = g_WeaponList.m_list[index];
		// skip any held weapon
		if ( !pWeapon->GetOwner() )
		{
			// restrict to mins/maxs
			if ( IsPointInBox( pWeapon->GetAbsOrigin(), mins, maxs ) )
			{
				if ( count < listMax )
				{
					pList[count] = pWeapon;
					count++;
				}
			}
		}
		index = g_WeaponList.m_list.Next( index );
	}

	return count;
}