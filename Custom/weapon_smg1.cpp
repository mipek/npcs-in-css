#include "CNPCBaseWeapon.h"
#include "CE_recipientfilter.h"
#include "CAI_NPC.h"
#include "weapon_smg1.h"


static CEntityFactory_CE<CNPCWeapon_SMG1> WEAPON_SMG1_REPLACE(WEAPON_SMG1_REPLACE_NAME);

acttable_t CNPCWeapon_SMG1::m_acttable[] =
		{
				{ ACT_RANGE_ATTACK1,			ACT_RANGE_ATTACK_SMG1,			true },
				{ ACT_RELOAD,					ACT_RELOAD_SMG1,				true },
				{ ACT_IDLE,						ACT_IDLE_SMG1,					true },
				{ ACT_IDLE_ANGRY,				ACT_IDLE_ANGRY_SMG1,			true },

				{ ACT_WALK,						ACT_WALK_RIFLE,					true },
				{ ACT_WALK_AIM,					ACT_WALK_AIM_RIFLE,				true  },

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
				{ ACT_GESTURE_RANGE_ATTACK1,	ACT_GESTURE_RANGE_ATTACK_SMG1,	true },
				{ ACT_RANGE_ATTACK1_LOW,		ACT_RANGE_ATTACK_SMG1_LOW,		true },
				{ ACT_COVER_LOW,				ACT_COVER_SMG1_LOW,				false },
				{ ACT_RANGE_AIM_LOW,			ACT_RANGE_AIM_SMG1_LOW,			false },
				{ ACT_RELOAD_LOW,				ACT_RELOAD_SMG1_LOW,			false },
				{ ACT_GESTURE_RELOAD,			ACT_GESTURE_RELOAD_SMG1,		true },
		};


void CNPCWeapon_SMG1::Spawn()
{
	m_iWeaponModel = PrecacheModel("models/weapons/w_smg1.mdl");
	BaseClass::Spawn();
}

void CNPCWeapon_SMG1::Precache()
{
	PrecacheScriptSound("Weapon_SMG1.Reload");
	PrecacheScriptSound("Weapon_SMG1.NPC_Reload");
	PrecacheScriptSound("Weapon_SMG1.Empty");
	PrecacheScriptSound("Weapon_SMG1.Single");
	PrecacheScriptSound("Weapon_SMG1.NPC_Single");
	PrecacheScriptSound("Weapon_SMG1.Special1");
	PrecacheScriptSound("Weapon_SMG1.Special2");
	PrecacheScriptSound("Weapon_SMG1.Double");
	PrecacheScriptSound("Weapon_SMG1.Burst");

	BaseClass::Precache();
}

const char *CNPCWeapon_SMG1::NPCWeaponGetWorldModel() const
{
	return "models/weapons/w_smg1.mdl";
}

acttable_t*	CNPCWeapon_SMG1::NPCWeaponActivityList()
{
	return m_acttable;
}

int	CNPCWeapon_SMG1::NPCWeaponActivityListCount()
{
	return ARRAYSIZE(m_acttable);
}

void CNPCWeapon_SMG1::OnNPCEquip(CCombatCharacter *owner)
{
	if( owner->Classify() == CLASS_PLAYER_ALLY )
	{
		m_fMaxRange1 = 3000;
	} else {
		m_fMaxRange1 = 1400;
	}
}

void CNPCWeapon_SMG1::FireNPCPrimaryAttack( CCombatCharacter *pOperator, Vector &vecShootOrigin, Vector &vecShootDir )
{
	CustomWeaponSound(pOperator->entindex(), vecShootOrigin, "Weapon_SMG1.NPC_Single");

	g_helpfunc.CSoundEnt_InsertSound( SOUND_COMBAT|SOUND_CONTEXT_GUNFIRE, pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2f, pOperator->BaseEntity(), SOUNDENT_CHANNEL_WEAPON, pOperator->CB_GetEnemy() );

	pOperator->FireBullets( 1, vecShootOrigin, vecShootDir, VECTOR_CONE_PRECALCULATED,
							MAX_TRACE_LENGTH, m_iPrimaryAmmoType, 1, entindex(), 0, 5.0f );

	pOperator->DoMuzzleFlash();

	m_iClip1 = *(m_iClip1) - 1;
}

void CNPCWeapon_SMG1::NPCWeaponOperator_HandleAnimEvent( animevent_t *pEvent, CBaseEntity *pOperator )
{
	switch( pEvent->event )
	{
		case EVENT_WEAPON_SMG1:
		{
			CCombatCharacter *owner = (CCombatCharacter *)CEntity::Instance(pOperator);

			Vector vecShootOrigin, vecShootDir;
			QAngle angDiscard;
			// Support old style attachment point firing
			if ((pEvent->options == NULL) || (pEvent->options[0] == '\0') || (!owner->GetAttachment(pEvent->options, vecShootOrigin, angDiscard)))
			{
				vecShootOrigin = owner->Weapon_ShootPosition();
			}

			CAI_NPC *npc = owner->MyNPCPointer();
			Assert( npc != NULL );
			vecShootDir = npc->GetActualShootTrajectory( vecShootOrigin );
			FireNPCPrimaryAttack( owner, vecShootOrigin, vecShootDir );
			break;
		}
		default:
			CCombatWeapon::Operator_HandleAnimEvent(pEvent, pOperator);
			break;
	}
}

void CNPCWeapon_SMG1::NPCWeaponOperator_ForceNPCFire( CBaseEntity  *pOperator, bool bSecondary )
{
	CCombatCharacter *owner = (CCombatCharacter *)CEntity::Instance(pOperator);

	*(m_iClip1) += 1;

	Vector vecShootOrigin, vecShootDir;
	QAngle	angShootDir;
	GetAttachment( LookupAttachment( "muzzle" ), vecShootOrigin, angShootDir );
	AngleVectors( angShootDir, &vecShootDir );
	FireNPCPrimaryAttack( owner, vecShootOrigin, vecShootDir );
}

const WeaponProficiencyInfo_t *CNPCWeapon_SMG1::NPCWeaponGetProficiencyValues()
{
	static WeaponProficiencyInfo_t proficiencyTable[] =
			{
					{ 7.0,		0.75	},
					{ 5.00,		0.75	},
					{ 10.0/3.0, 0.75	},
					{ 5.0/3.0,	0.75	},
					{ 1.00,		1.0		},
			};
	COMPILE_TIME_ASSERT( ARRAYSIZE(proficiencyTable) == WEAPON_PROFICIENCY_PERFECT + 1);

	return proficiencyTable;
}
