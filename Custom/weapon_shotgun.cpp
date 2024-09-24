#include "CNPCBaseWeapon.h"
#include "CE_recipientfilter.h"
#include "CAI_NPC.h"



class CNPCWeapon_ShotGun : public CNPCBaseWeapon
{
public:
	DECLARE_CLASS( CNPCWeapon_ShotGun, CNPCBaseWeapon );

	void Spawn();
	void Precache();
	int Weapon_CapabilitiesGet( void );

	const char *NPCWeaponGetWorldModel() const;
	acttable_t*	NPCWeaponActivityList();
	int	NPCWeaponActivityListCount();
	void NPCWeaponOperator_HandleAnimEvent( animevent_t *pEvent, CBaseEntity *pOperator );
	void NPCWeaponOperator_ForceNPCFire( CBaseEntity  *pOperator, bool bSecondary );
	const WeaponProficiencyInfo_t *NPCWeaponGetProficiencyValues();

	void OnNPCEquip(CCombatCharacter *owner);


private:
	void FireNPCPrimaryAttack( CCombatCharacter *pOperator, bool bUseWeaponAngles );
	const Vector& GetBulletSpread( void );

private:
	static acttable_t m_acttable[];

};


static CEntityFactory_CE<CNPCWeapon_ShotGun> WEAPON_SHOTGUN_REPLACE(WEAPON_SHOTGUN_REPLACE_NAME);


acttable_t	CNPCWeapon_ShotGun::m_acttable[] =
		{
				{ ACT_IDLE,						ACT_IDLE_SMG1,					true },	// FIXME: hook to shotgun unique

				{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_SHOTGUN,			true },
				{ ACT_RELOAD,					ACT_RELOAD_SHOTGUN,					false },
				{ ACT_WALK,						ACT_WALK_RIFLE,						true },
				{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_SHOTGUN,				true },

// Readiness activities (not aiming)
				{ ACT_IDLE_RELAXED,				ACT_IDLE_SHOTGUN_RELAXED,		false },//never aims
				{ ACT_IDLE_STIMULATED,			ACT_IDLE_SHOTGUN_STIMULATED,	false },
				{ ACT_IDLE_AGITATED,			ACT_IDLE_SHOTGUN_AGITATED,		false },//always aims

				{ ACT_WALK_RELAXED,				ACT_WALK_RIFLE_RELAXED,			false },//never aims
				{ ACT_WALK_STIMULATED,			ACT_WALK_RIFLE_STIMULATED,		false },
				{ ACT_WALK_AGITATED,			ACT_WALK_AIM_RIFLE,				false },//always aims

				{ ACT_RUN_RELAXED,				ACT_RUN_RIFLE_RELAXED,			false },//never aims
				{ ACT_RUN_STIMULATED,			ACT_RUN_RIFLE_STIMULATED,		false },
				{ ACT_RUN_AGITATED,				ACT_RUN_AIM_RIFLE,				false },//always aims

// Readiness activities (aiming)
				{ ACT_IDLE_AIM_RELAXED,			ACT_IDLE_SMG1_RELAXED,			false },//never aims
				{ ACT_IDLE_AIM_STIMULATED,		ACT_IDLE_AIM_RIFLE_STIMULATED,	false },
				{ ACT_IDLE_AIM_AGITATED,		ACT_IDLE_ANGRY_SMG1,			false },//always aims

				{ ACT_WALK_AIM_RELAXED,			ACT_WALK_RIFLE_RELAXED,			false },//never aims
				{ ACT_WALK_AIM_STIMULATED,		ACT_WALK_AIM_RIFLE_STIMULATED,	false },
				{ ACT_WALK_AIM_AGITATED,		ACT_WALK_AIM_RIFLE,				false },//always aims

				{ ACT_RUN_AIM_RELAXED,			ACT_RUN_RIFLE_RELAXED,			false },//never aims
				{ ACT_RUN_AIM_STIMULATED,		ACT_RUN_AIM_RIFLE_STIMULATED,	false },
				{ ACT_RUN_AIM_AGITATED,			ACT_RUN_AIM_RIFLE,				false },//always aims
//End readiness activities

				{ ACT_WALK_AIM,					ACT_WALK_AIM_SHOTGUN,				true },
				{ ACT_WALK_CROUCH,				ACT_WALK_CROUCH_RIFLE,				true },
				{ ACT_WALK_CROUCH_AIM,			ACT_WALK_CROUCH_AIM_RIFLE,			true },
				{ ACT_RUN,						ACT_RUN_RIFLE,						true },
				{ ACT_RUN_AIM,					ACT_RUN_AIM_SHOTGUN,				true },
				{ ACT_RUN_CROUCH,				ACT_RUN_CROUCH_RIFLE,				true },
				{ ACT_RUN_CROUCH_AIM,			ACT_RUN_CROUCH_AIM_RIFLE,			true },
				{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_SHOTGUN,	true },
				{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_SHOTGUN_LOW,		true },
				{ ACT_RELOAD_LOW,				ACT_RELOAD_SHOTGUN_LOW,				false },
				{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_SHOTGUN,			false },
		};


void CNPCWeapon_ShotGun::Spawn()
{
	m_iWeaponModel = PrecacheModel("models/weapons/w_shotgun.mdl");

	BaseClass::Spawn();
}

void CNPCWeapon_ShotGun::Precache()
{
	PrecacheScriptSound("Weapon_Shotgun.Empty");
	PrecacheScriptSound("Weapon_Shotgun.Reload");
	PrecacheScriptSound("Weapon_Shotgun.Special1");
	PrecacheScriptSound("Weapon_Shotgun.Single");
	PrecacheScriptSound("Weapon_Shotgun.Double");
	PrecacheScriptSound("Weapon_Shotgun.NPC_Reload");
	PrecacheScriptSound("Weapon_Shotgun.NPC_Single");

	BaseClass::Precache();
}

int CNPCWeapon_ShotGun::Weapon_CapabilitiesGet()
{
	if(IsNPCUsing())
		return bits_CAP_WEAPON_RANGE_ATTACK1;
	else
		return BaseClass::Weapon_CapabilitiesGet();
}

const char *CNPCWeapon_ShotGun::NPCWeaponGetWorldModel() const
{
	return "models/weapons/w_shotgun.mdl";
}

acttable_t*	CNPCWeapon_ShotGun::NPCWeaponActivityList()
{
	return m_acttable;
}

int	CNPCWeapon_ShotGun::NPCWeaponActivityListCount()
{
	return ARRAYSIZE(m_acttable);
}

void CNPCWeapon_ShotGun::OnNPCEquip(CCombatCharacter *owner)
{
	m_fMinRange1 = 0.0;
	m_fMaxRange1 = 500;
	m_fMinRange2 = 0.0;
	m_fMaxRange2 = 200;
}

const Vector& CNPCWeapon_ShotGun::GetBulletSpread( void )
{
	if(!IsNPCUsing())
		return BaseClass::GetBulletSpread();

	static Vector vitalAllyCone = VECTOR_CONE_3DEGREES;
	static Vector cone = VECTOR_CONE_10DEGREES;

	CCombatCharacter *owner = m_hOwner->Get();
	if(owner&& (owner->Classify() == CLASS_PLAYER_ALLY_VITAL) )
	{
		return vitalAllyCone;
	}
	return cone;
}

void CNPCWeapon_ShotGun::FireNPCPrimaryAttack( CCombatCharacter *pOperator, bool bUseWeaponAngles )
{
	Vector vecShootOrigin, vecShootDir;
	CAI_NPC *npc = pOperator->MyNPCPointer();
	Assert( npc != NULL );

	pOperator->DoMuzzleFlash();
	m_iClip1 = *(m_iClip1) - 1;

	if ( bUseWeaponAngles )
	{
		QAngle	angShootDir;
		GetAttachment( LookupAttachment( "muzzle" ), vecShootOrigin, angShootDir );
		AngleVectors( angShootDir, &vecShootDir );
	}
	else
	{
		vecShootOrigin = pOperator->Weapon_ShootPosition();
		vecShootDir = npc->GetActualShootTrajectory( vecShootOrigin );
	}


	CustomWeaponSound(pOperator->entindex(), vecShootOrigin, "Weapon_Shotgun.NPC_Single");

	pOperator->FireBullets( 8, vecShootOrigin, vecShootDir, GetBulletSpread(), MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 1, entindex(), 0, 9.0f);
}


void CNPCWeapon_ShotGun::NPCWeaponOperator_HandleAnimEvent( animevent_t *pEvent, CBaseEntity *pOperator )
{
	switch( pEvent->event )
	{
		case EVENT_WEAPON_SHOTGUN_FIRE:
		{
			FireNPCPrimaryAttack( (CCombatCharacter *)CEntity::Instance(pOperator), false );
			break;
		}
		default:
			CCombatWeapon::Operator_HandleAnimEvent(pEvent, pOperator);
			break;
	}
}

void CNPCWeapon_ShotGun::NPCWeaponOperator_ForceNPCFire( CBaseEntity  *pOperator, bool bSecondary )
{
	*(m_iClip1) += 1;

	FireNPCPrimaryAttack( (CCombatCharacter *)CEntity::Instance(pOperator), true );
}

const WeaponProficiencyInfo_t *CNPCWeapon_ShotGun::NPCWeaponGetProficiencyValues()
{
	return CCombatWeapon::GetProficiencyValues();
}
