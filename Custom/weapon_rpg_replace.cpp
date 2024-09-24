
#include "CEntity.h"
#include "weapon_rpg_replace.h"
#include "weapon_rpg.h"
#include "CPlayer.h"
#include "CE_recipientfilter.h"



CE_LINK_ENTITY_TO_CLASS( flashbang_projectile, CE_FlashbangProjectile );


CE_LINK_ENTITY_TO_CLASS( weapon_flashbang, CWeaponRPG );


CWeaponRPG *CE_FlashbangProjectile::CURRENT_RPG = NULL;
CPlayer *CE_FlashbangProjectile::CURRENT_THROWER = NULL;


bool CWeaponRPG::IS_REPLACE_SPAWN = false;


BEGIN_DATADESC( CWeaponRPG )
					DEFINE_KEYFIELD( m_bIsRPG,		FIELD_BOOLEAN,	"mm_rpg_replace" ),
END_DATADESC()


#define	RPG_BEAM_SPRITE		"effects/laser1.vmt"
#define	RPG_BEAM_SPRITE_NOZ	"effects/laser1_noz.vmt"


void CE_FlashbangProjectile::Spawn()
{
	BaseClass::Spawn();

	if(CE_FlashbangProjectile::CURRENT_RPG != NULL && CE_FlashbangProjectile::CURRENT_THROWER != NULL)
	{
		CE_FlashbangProjectile::CURRENT_RPG->CreateRPG(CE_FlashbangProjectile::CURRENT_THROWER);

		UTIL_Remove(this);
	}
}


CWeaponRPG::CWeaponRPG()
{
	m_bDeployed = true;

}

void CWeaponRPG::Spawn()
{
	if(CWeaponRPG::IS_REPLACE_SPAWN)
	{
		SetIsRPG(true);
		CWeaponRPG::IS_REPLACE_SPAWN = false;
	}
	BaseClass::Spawn();
}

bool CWeaponRPG::CreateRPG(CPlayer *player)
{
	Assert(player);

	if ( m_hMissile != NULL )
		return false;

	Vector vecOrigin;
	Vector vecForward;

	m_flNextPrimaryAttack = gpGlobals->curtime + 0.5f;

	Vector	vForward, vRight, vUp;
	player->EyeVectors( &vForward, &vRight, &vUp );
	Vector	muzzlePoint = player->Weapon_ShootPosition() + vForward * 12.0f + vRight * 6.0f + vUp * -3.0f;


	QAngle vecAngles;
	VectorAngles( vForward, vecAngles );

	CMissile *pMissile = CMissile::Create( muzzlePoint, vecAngles, player);
	pMissile->m_hOwner.Set(BaseEntity());


	// If the shot is clear to the player, give the missile a grace period
	trace_t	tr;
	Vector vecEye = player->EyePosition();
	UTIL_TraceLine( vecEye, vecEye + vForward * 128, MASK_SHOT, BaseEntity(), COLLISION_GROUP_NONE, &tr );
	if ( tr.fraction == 1.0 )
	{
		pMissile->SetGracePeriod( 0.3 );
	}

	pMissile->SetDamage( 150.0f );

	m_hMissile.Set(pMissile->BaseEntity());

	CSoundParameters params;
	if(GetParametersForSound("Weapon_RPG.Single", params, NULL))
	{
		CPASAttenuationFilter filter(muzzlePoint, params.soundlevel );
		EmitSound( filter, player->entindex(), "Weapon_RPG.Single", NULL, 0.0f );
	}

	m_bDeployed = false;

	return true;
}

void CWeaponRPG::NotifyRocketDied( void )
{
	m_hMissile.Set(NULL);

	/*if ( GetActivity() == ACT_VM_RELOAD )
		return;

	Reload();*/
}

void CWeaponRPG::Activate()
{
	BaseClass::Activate();

	if(!IsRPGReplace())
		return;

	// Restore the laser pointer after transition
	if ( m_bGuiding )
	{
		CPlayer *pOwner = ToBasePlayer( GetOwner() );

		if ( pOwner == NULL )
			return;

		if ( pOwner->GetActiveWeapon() == this )
		{
			StartGuiding();
		}
	}
}

void CWeaponRPG::ItemPostFrame()
{
	if(!IsRPGReplace()) {
		BaseClass::ItemPostFrame();
		return;
	}

	if(m_hMissile == NULL)
	{
		BaseClass::ItemPostFrame();
	}

	CPlayer *pPlayer = ToBasePlayer( GetOwner() );

	if ( pPlayer == NULL )
		return;

	//If we're pulling the weapon out for the first time, wait to draw the laser
	if ( ( m_bInitialStateUpdate ))// && ( GetActivity() != ACT_VM_DRAW ) )
	{
		StartGuiding();
		m_bInitialStateUpdate = false;
	}

	// Supress our guiding effects if we're lowered
	if ( GetIdealActivity() == ACT_VM_IDLE_LOWERED )
	{
		SuppressGuiding();
	}
	else
	{
		SuppressGuiding( false );
	}

	//Move the laser
	UpdateLaserPosition();

	if(m_hMissile == NULL)
	{
		int ammo = pPlayer->GetAmmoCount(m_iPrimaryAmmoType);
		if(ammo <= 0)
		{
			StopGuiding();
		} else  {
			CWeaponRPG *weapon = (CWeaponRPG*)pPlayer->GetRPGWeapon();
			if(weapon)
			{
				CCombatWeapon *current_weapon = pPlayer->GetActiveWeapon();
				if(current_weapon != weapon)
				{
					weapon->m_bDeployed = true;
					pPlayer->Weapon_Switch(weapon->BaseEntity());
				} else if(current_weapon && !m_bDeployed) {
					m_bDeployed = true;
					current_weapon->Deploy();
				}
			}
		}
	}
}

void CWeaponRPG::StartGuiding( void )
{
	// Don't start back up if we're overriding this
	if ( m_bHideGuiding )
		return;

	m_bGuiding = true;

	CSoundParameters params;
	if(GetParametersForSound("Weapon_RPG.LaserOn", params, NULL))
	{
		CPASAttenuationFilter filter(GetAbsOrigin(), params.soundlevel );
		EmitSound( filter, entindex(), "Weapon_RPG.LaserOn", NULL, 0.0f );
	}

	CreateLaserPointer();
}

void CWeaponRPG::CreateLaserPointer( void )
{
	if ( m_hLaserDot != NULL )
		return;

	CCombatCharacter *pOwner = GetOwner();

	if ( pOwner == NULL )
		return;

	if ( pOwner->GetAmmoCount(m_iPrimaryAmmoType) <= 0 )
		return;


	CLaserDot *dot = CLaserDot::Create( GetAbsOrigin(), pOwner->BaseEntity() );
	m_hLaserDot.Set(dot->BaseEntity());
	dot->TurnOff();

	UpdateLaserPosition();
}

void CWeaponRPG::UpdateLaserPosition( Vector vecMuzzlePos, Vector vecEndPos )
{
	if ( vecMuzzlePos == vec3_origin || vecEndPos == vec3_origin )
	{
		CPlayer *pPlayer = ToBasePlayer( GetOwner() );
		if ( !pPlayer )
			return;

		vecMuzzlePos = pPlayer->Weapon_ShootPosition();
		Vector	forward;
		pPlayer->EyeVectors( &forward );
		vecEndPos = vecMuzzlePos + ( forward * MAX_TRACE_LENGTH );
	}

	//Move the laser dot, if active
	trace_t	tr;

	CEntity *owner = GetOwner();
	// Trace out for the endpoint
	UTIL_TraceLine( vecMuzzlePos, vecEndPos, (MASK_SHOT & ~CONTENTS_WINDOW), (owner)?owner->BaseEntity():NULL, COLLISION_GROUP_NONE, &tr );

	// Move the laser sprite
	if ( m_hLaserDot != NULL )
	{
		Vector	laserPos = tr.endpos;
		m_hLaserDot->SetLaserPosition( laserPos, tr.plane.normal );

		if ( tr.DidHitNonWorldEntity() )
		{
			CEntity *pHit = CEntity::Instance(tr.m_pEnt);

			if ( ( pHit != NULL ) && ( pHit->m_takedamage ) )
			{
				m_hLaserDot->SetTargetEntity( pHit->BaseEntity() );
			}
			else
			{
				m_hLaserDot->SetTargetEntity( NULL );
			}
		}
		else
		{
			m_hLaserDot->SetTargetEntity( NULL );
		}
	}
}



bool CWeaponRPG::IsGuiding( void )
{
	return m_bGuiding;
}

void CWeaponRPG::Drop( const Vector &vecVelocity )
{
	if(IsRPGReplace())
	{
		StopGuiding();
	}

	BaseClass::Drop( vecVelocity );

	if(IsRPGReplace())
	{
		m_iWorldModelIndex = m_iWeaponModel;
	}
}

bool CWeaponRPG::Deploy( void )
{
	m_bInitialStateUpdate = true;

	return BaseClass::Deploy();
}

bool CWeaponRPG::CanHolster( void )
{
	if(IsRPGReplace())
	{
		if ( m_hMissile != NULL )
			return false;
	}

	return BaseClass::CanHolster();
}

void CWeaponRPG::SuppressGuiding( bool state )
{
	m_bHideGuiding = state;

	if ( m_hLaserDot == NULL )
	{
		StartGuiding();

		//STILL!?
		if ( m_hLaserDot == NULL )
			return;
	}

	if ( state )
	{
		m_hLaserDot->TurnOff();
	}
	else
	{
		m_hLaserDot->TurnOn();
	}
}

void CWeaponRPG::StopGuiding( void )
{
	m_bGuiding = false;

	CSoundParameters params;
	if(GetParametersForSound("Weapon_RPG.LaserOff", params, NULL))
	{
		CPASAttenuationFilter filter(GetAbsOrigin(), params.soundlevel );
		EmitSound( filter, entindex(), "Weapon_RPG.LaserOff", NULL, 0.0f );
	}

	// Kill the dot completely
	if ( m_hLaserDot != NULL )
	{
		m_hLaserDot->TurnOff();
		UTIL_Remove( m_hLaserDot );
		m_hLaserDot.Set(NULL);
	}
}

bool CWeaponRPG::HasAnyAmmo( void )
{
	if(IsRPGReplace())
	{
		if ( m_hMissile != NULL )
			return true;
	}

	return BaseClass::HasAnyAmmo();
}

bool CWeaponRPG::Holster( CBaseEntity *pSwitchingTo )
{
	if(IsRPGReplace())
	{
		StopGuiding();
	}

	return BaseClass::Holster( pSwitchingTo );
}

void CWeaponRPG::Precache( void )
{
	BaseClass::Precache();

	PrecacheScriptSound("Weapon_RPG.Single");
	PrecacheScriptSound("Weapon_RPG.NPC_Single");
	PrecacheScriptSound("Weapon_RPG.LaserOn");
	PrecacheScriptSound("Weapon_RPG.LaserOff");

	PrecacheScriptSound( "Missile.Ignite" );
	PrecacheScriptSound( "Missile.Accelerate" );

	// Laser dot...
	PrecacheModel( "sprites/redglow1.vmt" );
	PrecacheModel( RPG_BEAM_SPRITE );
	PrecacheModel( RPG_BEAM_SPRITE_NOZ );
	m_iWeaponModel = PrecacheModel("models/weapons/w_rocket_launcher.mdl");

	g_helpfunc.UTIL_PrecacheOther( "rpg_missile" );
}

const char *CWeaponRPG::GetWorldModel( void ) const
{
	if(IsRPGReplace())
		return "models/weapons/w_rocket_launcher.mdl";
	else
		return BaseClass::GetWorldModel();
}

void CWeaponRPG::Equip( CBaseEntity *pOwner )
{
	BaseClass::Equip(pOwner);
	if(IsRPGReplace())
	{
		CPlayer *pPlayer = ToBasePlayer(CEntity::Instance(pOwner));
		if(pPlayer)
		{
			pPlayer->m_bHaveRPG = true;
		}
		m_iWorldModelIndex = m_iWeaponModel;
	}
}

void CWeaponRPG::Think()
{
	BaseClass::Think();
	if(IsRPGReplace())
	{
		m_iWorldModelIndex = m_iWeaponModel;
	}
}

void CWeaponRPG::EmitGrenade(Vector vecSrc, QAngle vecAngles, Vector vecVel, AngularImpulse angImpulse, CBaseEntity *pPlayer)
{
	if(IsRPGReplace())
	{
		CE_FlashbangProjectile::CURRENT_RPG = this;
		CE_FlashbangProjectile::CURRENT_THROWER = ToBasePlayer( CEntity::Instance(pPlayer));
	}

	BaseClass::EmitGrenade(vecSrc,vecAngles, vecVel, angImpulse, pPlayer);

	CE_FlashbangProjectile::CURRENT_RPG = NULL;
	CE_FlashbangProjectile::CURRENT_THROWER = NULL;
}

void CWeaponRPG::UpdateNPCLaserPosition( const Vector &vecTarget )
{
	CreateLaserPointer();
	// Turn the laserdot on
	m_bGuiding = true;
	m_hLaserDot->TurnOn();

	Vector muzzlePoint = GetOwner()->Weapon_ShootPosition();
	Vector vecDir = (vecTarget - muzzlePoint);
	VectorNormalize( vecDir );
	vecDir = muzzlePoint + ( vecDir * MAX_TRACE_LENGTH );
	UpdateLaserPosition( muzzlePoint, vecDir );

	SetNPCLaserPosition( vecTarget );
}

void CWeaponRPG::SetNPCLaserPosition( const Vector &vecTarget )
{
	m_vecNPCLaserDot = vecTarget;
}

const Vector &CWeaponRPG::GetNPCLaserPosition( void )
{
	return m_vecNPCLaserDot;
}


CWeaponRPG *ToCWeaponRPG(CEntity *cent)
{
	CWeaponRPG *rpg = dynamic_cast<CWeaponRPG *>(cent);
	if(rpg && rpg->IsRPGReplace())
		return rpg;
	return NULL;
}

CWeaponRPG *ToCWeaponRPG(CBaseEntity *cbase)
{
	return ToCWeaponRPG(CEntity::Instance(cbase));
}