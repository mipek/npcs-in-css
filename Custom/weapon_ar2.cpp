#include "CNPCBaseWeapon.h"
#include "CE_recipientfilter.h"
#include "CAI_NPC.h"
#include "npc_combine.h"
#include "prop_combine_ball.h"


class CNPCWeapon_AR2 : public CNPCBaseMachineGunWeapon
{
public:
	DECLARE_CLASS( CNPCWeapon_AR2, CNPCBaseMachineGunWeapon );

	void Spawn();
	void Precache();
	const char *NPCWeaponGetWorldModel() const;
	acttable_t*	NPCWeaponActivityList();
	int	NPCWeaponActivityListCount();
	void NPCWeaponOperator_HandleAnimEvent( animevent_t *pEvent, CBaseEntity *pOperator );
	void NPCWeaponOperator_ForceNPCFire( CBaseEntity  *pOperator, bool bSecondary );
	const WeaponProficiencyInfo_t *NPCWeaponGetProficiencyValues();

	void OnNPCEquip(CCombatCharacter *owner);

	float	GetFireRate( void );

private:
	void FireNPCPrimaryAttack( CCombatCharacter *pOperator, bool bUseWeaponAngles );
	void FireNPCSecondaryAttack( CCombatCharacter *pOperator, bool bUseWeaponAngles );


private:
	static acttable_t m_acttable[];

};


static CEntityFactory_CE<CNPCWeapon_AR2> WEAPON_AR2_REPLACE(WEAPON_AR2_REPLACE_NAME);


ConVar sk_weapon_ar2_alt_fire_radius( "sk_weapon_ar2_alt_fire_radius", "10" );
ConVar sk_weapon_ar2_alt_fire_duration( "sk_weapon_ar2_alt_fire_duration", "4" );
ConVar sk_weapon_ar2_alt_fire_mass( "sk_weapon_ar2_alt_fire_mass", "150" );

acttable_t CNPCWeapon_AR2::m_acttable[] =
		{
				{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_AR2,			true },
				{ ACT_RELOAD,					ACT_RELOAD_SMG1,				true },		// FIXME: hook to AR2 unique
				{ ACT_IDLE,						ACT_IDLE_SMG1,					true },		// FIXME: hook to AR2 unique
				{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_SMG1,			true },		// FIXME: hook to AR2 unique

				{ ACT_WALK,						ACT_WALK_RIFLE,					true },

// Readiness activities (not aiming)
				{ ACT_IDLE_RELAXED,				ACT_IDLE_SMG1_RELAXED,			false },//never aims
				{ ACT_IDLE_STIMULATED,			ACT_IDLE_SMG1_STIMULATED,		false },
				{ ACT_IDLE_AGITATED,			ACT_IDLE_ANGRY_SMG1,			false },//always aims

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

				{ ACT_WALK_AIM,					ACT_WALK_AIM_RIFLE,				true },
				{ ACT_WALK_CROUCH,				ACT_WALK_CROUCH_RIFLE,			true },
				{ ACT_WALK_CROUCH_AIM,			ACT_WALK_CROUCH_AIM_RIFLE,		true },
				{ ACT_RUN,						ACT_RUN_RIFLE,					true },
				{ ACT_RUN_AIM,					ACT_RUN_AIM_RIFLE,				true },
				{ ACT_RUN_CROUCH,				ACT_RUN_CROUCH_RIFLE,			true },
				{ ACT_RUN_CROUCH_AIM,			ACT_RUN_CROUCH_AIM_RIFLE,		true },
				{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_AR2,	false },
				{ ACT_COVER_LOW,				ACT_COVER_SMG1_LOW,				false },		// FIXME: hook to AR2 unique
				{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_AR2_LOW,			false },
				{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_SMG1_LOW,		true },		// FIXME: hook to AR2 unique
				{ ACT_RELOAD_LOW,				ACT_RELOAD_SMG1_LOW,			false },
				{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_SMG1,		true },
		};

void CNPCWeapon_AR2::Spawn()
{
	m_iWeaponModel = PrecacheModel("models/weapons/w_irifle.mdl");
	BaseClass::Spawn();
}

void CNPCWeapon_AR2::Precache()
{
	PrecacheScriptSound("Weapon_CombineGuard.Special1");
	PrecacheScriptSound("Weapon_IRifle.Empty");
	PrecacheScriptSound("Weapon_IRifle.Single");
	PrecacheScriptSound("Weapon_AR2.Reload");
	PrecacheScriptSound("Weapon_AR2.Single");
	PrecacheScriptSound("Weapon_AR2.NPC_Single");
	PrecacheScriptSound("Weapon_AR2.NPC_Reload");
	PrecacheScriptSound("Weapon_AR2.NPC_Double");

	BaseClass::Precache();
}

const char *CNPCWeapon_AR2::NPCWeaponGetWorldModel() const
{
	return "models/weapons/w_irifle.mdl";
}

acttable_t*	CNPCWeapon_AR2::NPCWeaponActivityList()
{
	return m_acttable;
}

int	CNPCWeapon_AR2::NPCWeaponActivityListCount()
{
	return ARRAYSIZE(m_acttable);
}

float CNPCWeapon_AR2::GetFireRate()
{
	if(IsNPCUsing())
		return 0.1f;
	else
		return CNPCBaseWeapon::GetFireRate();
}

void CNPCWeapon_AR2::OnNPCEquip(CCombatCharacter *owner)
{
	m_fMinRange1	= 65;
	m_fMaxRange1	= 2048;
	m_fMinRange2	= 256;
	m_fMaxRange2	= 1024;
}

void CNPCWeapon_AR2::FireNPCPrimaryAttack( CCombatCharacter *pOperator, bool bUseWeaponAngles )
{
	Vector vecShootOrigin, vecShootDir;

	CAI_NPC *npc = pOperator->MyNPCPointer();
	Assert( npc != NULL );

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

	CustomWeaponSound(pOperator->entindex(), vecShootOrigin, "Weapon_AR2.NPC_Single");

	g_helpfunc.CSoundEnt_InsertSound( SOUND_COMBAT|SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2, pOperator->BaseEntity(), SOUNDENT_CHANNEL_WEAPON, pOperator->CB_GetEnemy() );

	pOperator->FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED, MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 1, entindex(), 0, 11.0f );

	// NOTENOTE: This is overriden on the client-side
	// pOperator->DoMuzzleFlash();

	m_iClip1 = *(m_iClip1) - 1;
}


void CNPCWeapon_AR2::FireNPCSecondaryAttack( CCombatCharacter *pOperator, bool bUseWeaponAngles )
{
	CCombatCharacter *owner = m_hOwner->Get();

	if ( !owner )
		return;

	CAI_NPC *pNPC = owner->MyNPCPointer();
	if ( !pNPC )
		return;

	Vector vecShootOrigin;

	if ( bUseWeaponAngles )
	{
		QAngle	angShootDir;
		GetAttachment( LookupAttachment( "muzzle" ), vecShootOrigin, angShootDir );
	}
	else
	{
		vecShootOrigin = pOperator->Weapon_ShootPosition();
	}

	CustomWeaponSound(pOperator->entindex(), vecShootOrigin, "Weapon_AR2.NPC_Double");


	// Fire!
	Vector vecSrc;
	Vector vecAiming;

	if ( bUseWeaponAngles )
	{
		QAngle	angShootDir;
		GetAttachment( LookupAttachment( "muzzle" ), vecSrc, angShootDir );
		AngleVectors( angShootDir, &vecAiming );
	}
	else
	{
		vecSrc = pNPC->Weapon_ShootPosition( );

		Vector vecTarget;

		CNPC_Combine *pSoldier = dynamic_cast<CNPC_Combine *>( pNPC );
		if ( pSoldier )
		{
			// In the distant misty past, elite soldiers tried to use bank shots.
			// Therefore, we must ask them specifically what direction they are shooting.
			vecTarget = pSoldier->GetAltFireTarget();
		}
		else
		{
			// All other users of the AR2 alt-fire shoot directly at their enemy.
			if ( !pNPC->GetEnemy() )
				return;

			vecTarget = pNPC->GetEnemy()->BodyTarget( vecSrc );
		}

		vecAiming = vecTarget - vecSrc;
		VectorNormalize( vecAiming );
	}

	Vector impactPoint = vecSrc + ( vecAiming * MAX_TRACE_LENGTH );

	float flAmmoRatio = 1.0f;
	float flDuration = RemapValClamped( flAmmoRatio, 0.0f, 1.0f, 0.5f, sk_weapon_ar2_alt_fire_duration.GetFloat() );
	float flRadius = RemapValClamped( flAmmoRatio, 0.0f, 1.0f, 4.0f, sk_weapon_ar2_alt_fire_radius.GetFloat() );

	// Fire the bullets
	Vector vecVelocity = vecAiming * 1000.0f;

	// Fire the combine ball
	CreateCombineBall(	vecSrc,
						  vecVelocity,
						  flRadius,
						  sk_weapon_ar2_alt_fire_mass.GetFloat(),
						  flDuration,
						  pNPC );
}

void CNPCWeapon_AR2::NPCWeaponOperator_HandleAnimEvent( animevent_t *pEvent, CBaseEntity *pOperator )
{
	switch( pEvent->event )
	{
		case EVENT_WEAPON_AR2:
		{
			FireNPCPrimaryAttack( (CCombatCharacter *)CEntity::Instance(pOperator), false );
			break;
		}
		case EVENT_WEAPON_AR2_ALTFIRE:
		{
			FireNPCSecondaryAttack( (CCombatCharacter *)CEntity::Instance(pOperator), false );
			break;
		}
		default:
			CCombatWeapon::Operator_HandleAnimEvent(pEvent, pOperator);
			break;
	}
}

void CNPCWeapon_AR2::NPCWeaponOperator_ForceNPCFire( CBaseEntity  *pOperator, bool bSecondary )
{
	if ( bSecondary )
	{
		FireNPCSecondaryAttack( (CCombatCharacter *)CEntity::Instance(pOperator), true );
	} else {
		// Ensure we have enough rounds in the clip
		*(m_iClip1) += 1;
		FireNPCPrimaryAttack( (CCombatCharacter *)CEntity::Instance(pOperator), true );
	}
}

const WeaponProficiencyInfo_t *CNPCWeapon_AR2::NPCWeaponGetProficiencyValues()
{
	static WeaponProficiencyInfo_t proficiencyTable[] =
			{
					{ 7.0,		0.75	},
					{ 5.00,		0.75	},
					{ 3.0,		0.85	},
					{ 5.0/3.0,	0.75	},
					{ 1.00,		1.0		},
			};
	COMPILE_TIME_ASSERT( ARRAYSIZE(proficiencyTable) == WEAPON_PROFICIENCY_PERFECT + 1);

	return proficiencyTable;
}
