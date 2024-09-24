
#include "weapon_rpg.h"
#include "effect_dispatch_data.h"
#include "CEnvExplosion.h"
#include "CSmoke_trail.h"
#include "weapon_rpg_replace.h"
#include "CSprite.h"
#include "CInfoTarget_Fix.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


LINK_ENTITY_TO_CUSTOM_CLASS( rpg_missile, item_sodacan, CMissile );


#define	RPG_SPEED	1500


const char *g_pLaserDotThink = "LaserThinkContext";

static ConVar sk_apc_missile_damage("sk_apc_missile_damage", "15");
ConVar rpg_missle_use_custom_detonators( "rpg_missle_use_custom_detonators", "1" );

#define APC_MISSILE_DAMAGE	sk_apc_missile_damage.GetInt()



CEntityClassList<CLaserDot> g_LaserDotList;
template <> CLaserDot *CEntityClassList<CLaserDot>::m_pClassList = NULL;
CLaserDot *GetLaserDotList()
{
	return g_LaserDotList.m_pClassList;
}


BEGIN_DATADESC( CMissile )

					DEFINE_FIELD( m_hOwner,					FIELD_EHANDLE ),
					DEFINE_FIELD( m_hRocketTrail,			FIELD_EHANDLE ),
					DEFINE_FIELD( m_flAugerTime,			FIELD_TIME ),
					DEFINE_FIELD( m_flMarkDeadTime,			FIELD_TIME ),
					DEFINE_FIELD( m_flGracePeriodEndsAt,	FIELD_TIME ),
					DEFINE_FIELD( m_flDamage,				FIELD_FLOAT ),

					// Function Pointers
					//DEFINE_FUNCTION( MissileTouch ),
					//DEFINE_FUNCTION( AccelerateThink ),
					//DEFINE_FUNCTION( AugerThink ),
					//DEFINE_FUNCTION( IgniteThink ),
					//DEFINE_FUNCTION( SeekThink ),

END_DATADESC()


CMissile::CMissile()
{
	m_hRocketTrail.Set(NULL);
}

CMissile::~CMissile()
{
}


//-----------------------------------------------------------------------------
// Purpose:
//
//
//-----------------------------------------------------------------------------
void CMissile::Precache( void )
{
	PrecacheModel( "models/weapons/w_missile.mdl" );
	PrecacheModel( "models/weapons/w_missile_launch.mdl" );
	PrecacheModel( "models/weapons/w_missile_closed.mdl" );
}


//-----------------------------------------------------------------------------
// Purpose:
//
//
//-----------------------------------------------------------------------------
void CMissile::Spawn( void )
{
	Precache();

	SetSolid( SOLID_BBOX );
	SetModel("models/weapons/w_missile_launch.mdl");
	UTIL_SetSize( BaseEntity(), -Vector(4,4,4), Vector(4,4,4) );

	SetTouch( &CMissile::MissileTouch );

	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
	SetThink( &CMissile::IgniteThink );

	SetNextThink( gpGlobals->curtime + 0.3f );

	m_takedamage = DAMAGE_YES;
	*(m_iHealth) = m_iMaxHealth = 100;
//	m_bloodColor = DONT_BLEED;
	m_flGracePeriodEndsAt = 0;

	AddFlag( FL_OBJECT );
}


//---------------------------------------------------------
//---------------------------------------------------------
void CMissile::Event_Killed( const CTakeDamageInfo &info )
{
	m_takedamage = DAMAGE_NO;

	ShotDown();
}

unsigned int CMissile::PhysicsSolidMaskForEntity( void ) const
{
	//CE_MODIFY
	return MASK_SOLID|CONTENTS_HITBOX;
	//return BaseClass::PhysicsSolidMaskForEntity() | CONTENTS_HITBOX;
}

int	CMissile::OnTakeDamage( const CTakeDamageInfo &info )
{
	//CE_MODIFY
	int retVal = 0;

	if (!m_takedamage)
		return 0;

	if ( info.GetDamageType() & DMG_SHOCK )
	{
		g_pEffects->Sparks( info.GetDamagePosition(), 2, 2 );
		UTIL_Smoke( info.GetDamagePosition(), enginerandom->RandomInt( 10, 15 ), 10 );
	}

	switch( m_lifeState )
	{
		case LIFE_ALIVE:
			retVal = OnTakeDamage_Alive( info );
			if ( m_iHealth <= 0 )
			{
				IPhysicsObject *pPhysics = VPhysicsGetObject();
				if ( pPhysics )
				{
					pPhysics->EnableCollisions( false );
				}

				Event_Killed( info );
			}
			return retVal;
			break;

		case LIFE_DYING:
			return 1;

		default:
		case LIFE_DEAD:
			retVal = OnTakeDamage_Dead( info );
			return retVal;
	}
}

int CMissile::OnTakeDamage_Dead( const CTakeDamageInfo &info )
{
	// do the damage
	if ( m_takedamage != DAMAGE_EVENTS_ONLY )
	{
		m_iHealth -= info.GetDamage();
	}

	return 1;
}

int CMissile::CBaseCombatCharacter_OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	// grab the vector of the incoming attack. ( pretend that the inflictor is a little lower than it really is, so the body will tend to fly upward a bit).
	Vector vecDir = vec3_origin;
	CEntity *inflictor = CEntity::Instance(info.GetInflictor());
	if (inflictor)
	{
		vecDir = inflictor->WorldSpaceCenter() - Vector ( 0, 0, 10 ) - WorldSpaceCenter();
		VectorNormalize(vecDir);
	}
	*g_vecAttackDir = vecDir;

	//!!!LATER - make armor consideration here!
	// do the damage
	if ( m_takedamage != DAMAGE_EVENTS_ONLY )
	{
		// Separate the fractional amount of damage from the whole
		float flFractionalDamage = info.GetDamage() - floor( info.GetDamage() );
		float flIntegerDamage = info.GetDamage() - flFractionalDamage;

		// Add fractional damage to the accumulator
		m_flDamageAccumulator += flFractionalDamage;

		// If the accumulator is holding a full point of damage, move that point
		// of damage into the damage we're about to inflict.
		if( m_flDamageAccumulator >= 1.0 )
		{
			flIntegerDamage += 1.0;
			m_flDamageAccumulator -= 1.0;
		}

		if ( flIntegerDamage <= 0 )
			return 0;

		m_iHealth -= flIntegerDamage;
	}

	return 1;
}

//---------------------------------------------------------
//---------------------------------------------------------
int CMissile::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	if ( ( info.GetDamageType() & (DMG_MISSILEDEFENSE | DMG_AIRBOAT) ) == false )
		return 0;

	bool bIsDamaged;
	if( m_iHealth <= AugerHealth() )
	{
		// This missile is already damaged (i.e., already running AugerThink)
		bIsDamaged = true;
	}
	else
	{
		// This missile isn't damaged enough to wobble in flight yet
		bIsDamaged = false;
	}

	int nRetVal = CBaseCombatCharacter_OnTakeDamage_Alive( info );

	if( !bIsDamaged )
	{
		if ( m_iHealth <= AugerHealth() )
		{
			ShotDown();
		}
	}

	return nRetVal;
}


//-----------------------------------------------------------------------------
// Purpose: Stops any kind of tracking and shoots dumb
//-----------------------------------------------------------------------------
void CMissile::DumbFire( void )
{
	SetThink( NULL );
	SetMoveType( MOVETYPE_FLY );

	SetModel("models/weapons/w_missile.mdl");
	UTIL_SetSize( BaseEntity(), vec3_origin, vec3_origin );

	EmitSound( "Missile.Ignite" );

	// Smoke trail.
	CreateSmokeTrail();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMissile::SetGracePeriod( float flGracePeriod )
{
	m_flGracePeriodEndsAt = gpGlobals->curtime + flGracePeriod;

	// Go non-solid until the grace period ends
	AddSolidFlags( FSOLID_NOT_SOLID );
}

//---------------------------------------------------------
//---------------------------------------------------------
void CMissile::AccelerateThink( void )
{
	Vector vecForward;

	// !!!UNDONE - make this work exactly the same as HL1 RPG, lest we have looping sound bugs again!
	EmitSound( "Missile.Accelerate" );

	// SetEffects( EF_LIGHT );

	AngleVectors( GetLocalAngles(), &vecForward );
	SetAbsVelocity( vecForward * RPG_SPEED );

	SetThink( &CMissile::SeekThink );
	SetNextThink( gpGlobals->curtime + 0.1f );
}

#define AUGER_YDEVIANCE 20.0f
#define AUGER_XDEVIANCEUP 8.0f
#define AUGER_XDEVIANCEDOWN 1.0f

//---------------------------------------------------------
//---------------------------------------------------------
void CMissile::AugerThink( void )
{
	// If we've augered long enough, then just explode
	if ( m_flAugerTime < gpGlobals->curtime )
	{
		Explode();
		return;
	}

	if ( m_flMarkDeadTime < gpGlobals->curtime )
	{
		m_lifeState = LIFE_DYING;
	}

	QAngle angles = GetLocalAngles();

	angles.y += enginerandom->RandomFloat( -AUGER_YDEVIANCE, AUGER_YDEVIANCE );
	angles.x += enginerandom->RandomFloat( -AUGER_XDEVIANCEDOWN, AUGER_XDEVIANCEUP );

	SetLocalAngles( angles );

	Vector vecForward;

	AngleVectors( GetLocalAngles(), &vecForward );

	SetAbsVelocity( vecForward * 1000.0f );

	SetNextThink( gpGlobals->curtime + 0.05f );
}

//-----------------------------------------------------------------------------
// Purpose: Causes the missile to spiral to the ground and explode, due to damage
//-----------------------------------------------------------------------------
void CMissile::ShotDown( void )
{
	CEffectData	data;
	data.m_vOrigin = GetAbsOrigin();

	g_helpfunc.DispatchEffect( "RPGShotDown", data );

	if ( m_hRocketTrail != NULL )
	{
		m_hRocketTrail->m_bDamaged = true;
	}

	SetThink( &CMissile::AugerThink );
	SetNextThink( gpGlobals->curtime );
	m_flAugerTime = gpGlobals->curtime + 1.5f;
	m_flMarkDeadTime = gpGlobals->curtime + 0.75;

	// Let the RPG start reloading immediately
	if ( m_hOwner != NULL )
	{
		m_hOwner->NotifyRocketDied();
		m_hOwner.Set(NULL);
	}
}


//-----------------------------------------------------------------------------
// The actual explosion
//-----------------------------------------------------------------------------
void CMissile::DoExplosion( void )
{
	// Explode
	CEntity *cent = GetOwnerEntity();
	ExplosionCreate( GetAbsOrigin(), GetAbsAngles(), (cent)?cent->BaseEntity():NULL, (int)GetDamage(), (int)(GetDamage() * 2),
					 SF_ENVEXPLOSION_NOSPARKS | SF_ENVEXPLOSION_NODLIGHTS | SF_ENVEXPLOSION_NOSMOKE, 0.0f, BaseEntity());
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMissile::Explode( void )
{
	// Don't explode against the skybox. Just pretend that
	// the missile flies off into the distance.
	Vector forward;

	GetVectors( &forward, NULL, NULL );

	trace_t tr;
	UTIL_TraceLine( GetAbsOrigin(), GetAbsOrigin() + forward * 16, MASK_SHOT, BaseEntity(), COLLISION_GROUP_NONE, &tr );

	m_takedamage = DAMAGE_NO;
	SetSolid( SOLID_NONE );
	if( tr.fraction == 1.0 || !(tr.surface.flags & SURF_SKY) )
	{
		DoExplosion();
	}

	if( m_hRocketTrail )
	{
		m_hRocketTrail->SetLifetime(0.1f);
		m_hRocketTrail.Set(NULL);
	}

	if ( m_hOwner != NULL )
	{
		m_hOwner->NotifyRocketDied();
		m_hOwner.Set(NULL);
	}

	StopSound( "Missile.Ignite" );
	UTIL_Remove( this );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  : *pOther -
//-----------------------------------------------------------------------------
void CMissile::MissileTouch( CEntity *pOther )
{
	Assert( pOther );

	// Don't touch triggers (but DO hit weapons)
	if ( pOther->IsSolidFlagSet(FSOLID_TRIGGER|FSOLID_VOLUME_CONTENTS) && pOther->GetCollisionGroup() != COLLISION_GROUP_WEAPON )
		return;

	Explode();
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMissile::CreateSmokeTrail( void )
{
	if ( m_hRocketTrail )
		return;

	// Smoke trail.
	CRocketTrail *trail = CRocketTrail::CreateRocketTrail();
	if ( trail )
	{
		m_hRocketTrail.Set(trail->BaseEntity());

		trail->m_Opacity = 0.2f;
		trail->m_SpawnRate = 100;
		trail->m_ParticleLifetime = 0.5f;
		trail->m_StartColor->Init( 0.65f, 0.65f , 0.65f );
		trail->m_EndColor->Init( 0.0, 0.0, 0.0 );
		trail->m_StartSize = 8;
		trail->m_EndSize = 32;
		trail->m_SpawnRadius = 4;
		trail->m_MinSpeed = 2;
		trail->m_MaxSpeed = 16;

		trail->SetLifetime( 999 );
		trail->FollowEntity( this, "0" );
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMissile::IgniteThink( void )
{
	SetMoveType( MOVETYPE_FLY );
	SetModel("models/weapons/w_missile.mdl");
	UTIL_SetSize( BaseEntity(), vec3_origin, vec3_origin );
	RemoveSolidFlags( FSOLID_NOT_SOLID );

	//TODO: Play opening sound

	Vector vecForward;

	EmitSound( "Missile.Ignite" );

	AngleVectors( GetLocalAngles(), &vecForward );
	SetAbsVelocity( vecForward * RPG_SPEED );

	SetThink( &CMissile::SeekThink );
	SetNextThink( gpGlobals->curtime );

	CCombatWeapon *owner = m_hOwner;
	if ( owner && owner->GetOwner() )
	{
		CPlayer *pPlayer = ToBasePlayer( owner->GetOwner() );

		color32 white = { 255,225,205,64 };
		UTIL_ScreenFade( pPlayer, white, 0.1f, 0.0f, FFADE_IN );
	}

	CreateSmokeTrail();
}


//-----------------------------------------------------------------------------
// Gets the shooting position
//-----------------------------------------------------------------------------
void CMissile::GetShootPosition( CLaserDot *pLaserDot, Vector *pShootPosition )
{
	if ( pLaserDot->GetOwnerEntity() != NULL )
	{
		//FIXME: Do we care this isn't exactly the muzzle position?
		*pShootPosition = pLaserDot->GetOwnerEntity()->WorldSpaceCenter();
	}
	else
	{
		*pShootPosition = pLaserDot->GetChasePosition();
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
#define	RPG_HOMING_SPEED	0.125f

void CMissile::ComputeActualDotPosition( CLaserDot *pLaserDot, Vector *pActualDotPosition, float *pHomingSpeed )
{
	*pHomingSpeed = RPG_HOMING_SPEED;
	if ( pLaserDot->GetTargetEntity() )
	{
		*pActualDotPosition = pLaserDot->GetChasePosition();
		return;
	}

	Vector vLaserStart;
	GetShootPosition( pLaserDot, &vLaserStart );

	//Get the laser's vector
	Vector vLaserDir;
	VectorSubtract( pLaserDot->GetChasePosition(), vLaserStart, vLaserDir );

	//Find the length of the current laser
	float flLaserLength = VectorNormalize( vLaserDir );

	//Find the length from the missile to the laser's owner
	float flMissileLength = GetAbsOrigin().DistTo( vLaserStart );

	//Find the length from the missile to the laser's position
	Vector vecTargetToMissile;
	VectorSubtract( GetAbsOrigin(), pLaserDot->GetChasePosition(), vecTargetToMissile );
	float flTargetLength = VectorNormalize( vecTargetToMissile );

	// See if we should chase the line segment nearest us
	if ( ( flMissileLength < flLaserLength ) || ( flTargetLength <= 512.0f ) )
	{
		*pActualDotPosition = UTIL_PointOnLineNearestPoint( vLaserStart, pLaserDot->GetChasePosition(), GetAbsOrigin() );
		*pActualDotPosition += ( vLaserDir * 256.0f );
	}
	else
	{
		// Otherwise chase the dot
		*pActualDotPosition = pLaserDot->GetChasePosition();
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CMissile::SeekThink( void )
{
	CEntity	*pBestDot	= NULL;
	float		flBestDist	= MAX_TRACE_LENGTH;
	float		dotDist;

	// If we have a grace period, go solid when it ends
	if ( m_flGracePeriodEndsAt )
	{
		if ( m_flGracePeriodEndsAt < gpGlobals->curtime )
		{
			RemoveSolidFlags( FSOLID_NOT_SOLID );
			m_flGracePeriodEndsAt = 0;
		}
	}

	//Search for all dots relevant to us
	for( CLaserDot *pEnt = GetLaserDotList(); pEnt != NULL; pEnt = pEnt->m_pNext )
	{
		if ( !pEnt->IsOn() )
			continue;

		if ( pEnt->GetOwnerEntity() != GetOwnerEntity() )
			continue;

		dotDist = (GetAbsOrigin() - pEnt->GetAbsOrigin()).Length();

		//Find closest
		if ( dotDist < flBestDist )
		{
			pBestDot	= pEnt;
			flBestDist	= dotDist;
		}
	}

	if( hl2_episodic->GetBool() )
	{
		if( flBestDist <= ( GetAbsVelocity().Length() * 2.5f ) && FVisible_Vector( pBestDot->GetAbsOrigin() ) )
		{
			// Scare targets
			g_helpfunc.CSoundEnt_InsertSound( SOUND_DANGER, pBestDot->GetAbsOrigin(), CMissile::EXPLOSION_RADIUS, 0.2f, pBestDot->BaseEntity(), SOUNDENT_CHANNEL_REPEATED_DANGER, NULL );
		}
	}


	if ( rpg_missle_use_custom_detonators.GetBool() )
	{
		for ( int i = gm_CustomDetonators.Count() - 1; i >=0; --i )
		{
			CustomDetonator_t &detonator = gm_CustomDetonators[i];
			CEntity *cent = detonator.hEntity;
			if ( !cent )
			{
				gm_CustomDetonators.FastRemove( i );
			}
			else
			{
				const Vector &vPos = cent->CollisionProp_Actual()->WorldSpaceCenter();
				if ( detonator.halfHeight > 0 )
				{
					if ( fabsf( vPos.z - GetAbsOrigin().z ) < detonator.halfHeight )
					{
						if ( ( GetAbsOrigin().AsVector2D() - vPos.AsVector2D() ).LengthSqr() < detonator.radiusSq )
						{
							Explode();
							return;
						}
					}
				}
				else
				{
					if ( ( GetAbsOrigin() - vPos ).LengthSqr() < detonator.radiusSq )
					{
						Explode();
						return;
					}
				}
			}
		}
	}

	//If we have a dot target
	if ( pBestDot == NULL )
	{
		//Think as soon as possible
		SetNextThink( gpGlobals->curtime );
		return;
	}

	CLaserDot *pLaserDot = (CLaserDot *)pBestDot;
	Vector	targetPos;

	float flHomingSpeed;
	Vector vecLaserDotPosition;
	ComputeActualDotPosition( pLaserDot, &targetPos, &flHomingSpeed );

	Vector	vTargetDir;
	VectorSubtract( targetPos, GetAbsOrigin(), vTargetDir );
	float flDist = VectorNormalize( vTargetDir );

	Vector	vDir	= GetAbsVelocity();
	float	flSpeed	= VectorNormalize( vDir );
	Vector	vNewVelocity = vDir;
	if ( gpGlobals->frametime > 0.0f )
	{
		if ( flSpeed != 0 )
		{
			vNewVelocity = ( flHomingSpeed * vTargetDir ) + ( ( 1 - flHomingSpeed ) * vDir );

			// This computation may happen to cancel itself out exactly. If so, slam to targetdir.
			if ( VectorNormalize( vNewVelocity ) < 1e-3 )
			{
				vNewVelocity = (flDist != 0) ? vTargetDir : vDir;
			}
		}
		else
		{
			vNewVelocity = vTargetDir;
		}
	}

	QAngle	finalAngles;
	VectorAngles( vNewVelocity, finalAngles );
	SetAbsAngles( finalAngles );

	vNewVelocity *= flSpeed;
	SetAbsVelocity( vNewVelocity );

	if( GetAbsVelocity() == vec3_origin )
	{
		// Strange circumstances have brought this missile to halt. Just blow it up.
		Explode();
		return;
	}

	// Think as soon as possible
	SetNextThink( gpGlobals->curtime );
}


//-----------------------------------------------------------------------------
// Purpose:
//
// Input  : &vecOrigin -
//			&vecAngles -
//			NULL -
//
// Output : CMissile
//-----------------------------------------------------------------------------
CMissile *CMissile::Create( const Vector &vecOrigin, const QAngle &vecAngles, CEntity *pentOwner )
{
	//CMissile *pMissile = (CMissile *)CreateEntityByName("rpg_missile" );
	CMissile *pMissile = (CMissile *) CEntity::Create( "rpg_missile", vecOrigin, vecAngles, (pentOwner)?pentOwner->BaseEntity():NULL );
	pMissile->SetOwnerEntity( (pentOwner)?pentOwner->BaseEntity():NULL );
	pMissile->Spawn();
	pMissile->AddEffects( EF_NOSHADOW );

	Vector vecForward;
	AngleVectors( vecAngles, &vecForward );

	pMissile->SetAbsVelocity( vecForward * 300 + Vector( 0,0, 128 ) );

	return pMissile;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
CUtlVector<CMissile::CustomDetonator_t> CMissile::gm_CustomDetonators;

void CMissile::AddCustomDetonator( CBaseEntity *pEntity, float radius, float height )
{
	int i = gm_CustomDetonators.AddToTail();
	gm_CustomDetonators[i].hEntity.Set(pEntity);
	gm_CustomDetonators[i].radiusSq = Square( radius );
	gm_CustomDetonators[i].halfHeight = height * 0.5f;
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CMissile::RemoveCustomDetonator( CBaseEntity *pEntity )
{
	for ( int i = 0; i < gm_CustomDetonators.Count(); i++ )
	{
		if ( gm_CustomDetonators[i].hEntity == pEntity )
		{
			gm_CustomDetonators.FastRemove( i );
			break;
		}
	}
}





//=============================================================================
// Laser Dot
//=============================================================================

LINK_ENTITY_TO_CUSTOM_CLASS( env_laserdot, env_sprite, CLaserDot );

BEGIN_DATADESC( CLaserDot )
					DEFINE_FIELD( m_vecSurfaceNormal,	FIELD_VECTOR ),
					DEFINE_FIELD( m_hTargetEnt,			FIELD_EHANDLE ),
					DEFINE_FIELD( m_bVisibleLaserDot,	FIELD_BOOLEAN ),
					DEFINE_FIELD( m_bIsOn,				FIELD_BOOLEAN ),

					//DEFINE_FIELD( m_pNext, FIELD_CLASSPTR ),	// don't save - regenerated by constructor
END_DATADESC()


//-----------------------------------------------------------------------------
// Finds missiles in cone
//-----------------------------------------------------------------------------
CEntity *CreateLaserDot( const Vector &origin, CBaseEntity *pOwner, bool bVisibleDot )
{
	return CLaserDot::Create( origin, pOwner, bVisibleDot );
}

void SetLaserDotTarget( CEntity *pLaserDot, CBaseEntity *pTarget )
{
	CLaserDot *pDot = assert_cast< CLaserDot* >(pLaserDot );
	pDot->SetTargetEntity( pTarget );
}

void EnableLaserDot( CEntity *pLaserDot, bool bEnable )
{
	CLaserDot *pDot = assert_cast< CLaserDot* >(pLaserDot );
	if ( bEnable )
	{
		pDot->TurnOn();
	}
	else
	{
		pDot->TurnOff();
	}
}

CLaserDot::CLaserDot( void )
{
	m_hTargetEnt = NULL;
	m_bIsOn = true;
	g_LaserDotList.Insert( this );
}

CLaserDot::~CLaserDot( void )
{
	g_LaserDotList.Remove( this );
}

CLaserDot *CLaserDot::LaserDotCreate( const char *pSpriteName, const Vector &origin, bool animate )
{
	CLaserDot *pSprite = (CLaserDot *)CreateEntityByName("env_laserdot");
	pSprite->SpriteInit( pSpriteName, origin );
	pSprite->SetSolid( SOLID_NONE );
	UTIL_SetSize( pSprite->BaseEntity(), vec3_origin, vec3_origin );
	pSprite->SetMoveType( MOVETYPE_NONE );
	if ( animate )
	{
		pSprite->TurnOn();
	}

	return pSprite;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  : &origin -
// Output : CLaserDot
//-----------------------------------------------------------------------------
CLaserDot *CLaserDot::Create( const Vector &origin, CBaseEntity *pOwner, bool bVisibleDot )
{
	CLaserDot *pLaserDot = CLaserDot::LaserDotCreate("sprites/redglow1.vmt", origin, false );

	if ( pLaserDot == NULL )
		return NULL;

	pLaserDot->SetTransparency( kRenderGlow, 255, 255, 255, 200, kRenderFxNoDissipation );
	pLaserDot->SetScale( 0.2f );
	pLaserDot->SetGlowProxySize( 4.0f );

	pLaserDot->m_bVisibleLaserDot = bVisibleDot;
	pLaserDot->SetMoveType( MOVETYPE_NONE );
	pLaserDot->AddSolidFlags( FSOLID_NOT_SOLID );
	pLaserDot->AddEffects( EF_NOSHADOW );
	UTIL_SetSize( pLaserDot->BaseEntity(), -Vector(4,4,4), Vector(4,4,4) );

	pLaserDot->SetOwnerEntity( pOwner );

	pLaserDot->AddEFlags( EFL_FORCE_CHECK_TRANSMIT );

	if ( !bVisibleDot )
	{
		pLaserDot->MakeInvisible();
	}

	return pLaserDot;
}

void CLaserDot::SetLaserPosition( const Vector &origin, const Vector &normal )
{
	SetAbsOrigin( origin );
	m_vecSurfaceNormal = normal;
}

Vector CLaserDot::GetChasePosition()
{
	return GetAbsOrigin() - m_vecSurfaceNormal * 10;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CLaserDot::TurnOn( void )
{
	m_bIsOn = true;
	if ( m_bVisibleLaserDot )
	{
		//BaseClass::TurnOn();
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CLaserDot::TurnOff( void )
{
	m_bIsOn = false;
	if ( m_bVisibleLaserDot )
	{
		//BaseClass::TurnOff();
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CLaserDot::MakeInvisible( void )
{
}






















//-----------------------------------------------------------------------------
// This entity is used to create little force boxes that the helicopter
// should avoid.
//-----------------------------------------------------------------------------
class CInfoAPCMissileHint : public CE_InfoTarget_Fix
{
public:
	CE_DECLARE_CLASS( CInfoAPCMissileHint, CE_InfoTarget_Fix );
	DECLARE_DATADESC();

	virtual void Spawn( );
	virtual void Activate();
	virtual void UpdateOnRemove();

	static CEntity *FindAimTarget( CEntity *pMissile, const char *pTargetName,
								   const Vector &vecCurrentTargetPos, const Vector &vecCurrentTargetVel );

private:
	CFakeHandle	m_hTarget;

	typedef CEFakeHandle<CInfoAPCMissileHint> APCMissileHintHandle_t;
	static CUtlVector< APCMissileHintHandle_t > s_APCMissileHints;
};


//-----------------------------------------------------------------------------
//
// This entity is used to create little force boxes that the helicopters should avoid.
//
//-----------------------------------------------------------------------------
CUtlVector< CInfoAPCMissileHint::APCMissileHintHandle_t > CInfoAPCMissileHint::s_APCMissileHints;


LINK_ENTITY_TO_CUSTOM_CLASS( info_apc_missile_hint, info_target, CInfoAPCMissileHint );

BEGIN_DATADESC( CInfoAPCMissileHint )
					DEFINE_FIELD( m_hTarget, FIELD_EHANDLE ),
END_DATADESC()



//-----------------------------------------------------------------------------
// Spawn, remove
//-----------------------------------------------------------------------------
void CInfoAPCMissileHint::Spawn( )
{
	SetModel( STRING( GetModelName() ) );
	SetSolid( SOLID_BSP );
	AddSolidFlags( FSOLID_NOT_SOLID );
	AddEffects( EF_NODRAW );
}

void CInfoAPCMissileHint::Activate( )
{
	BaseClass::Activate();

	CEntity *cent = g_helpfunc.FindEntityByName( (CBaseEntity *)NULL, m_target );
	m_hTarget.Set((cent)?cent->BaseEntity():NULL);

	if ( cent == NULL )
	{
		DevWarning( "%s: Could not find target '%s'!\n", GetClassname(),  *(m_target) );
	}
	else
	{
		APCMissileHintHandle_t temp;
		temp.Set(BaseEntity());
		s_APCMissileHints.AddToTail( temp );
	}
}

void CInfoAPCMissileHint::UpdateOnRemove( )
{
	APCMissileHintHandle_t temp;
	temp.Set(BaseEntity());
	s_APCMissileHints.FindAndRemove( temp );
	BaseClass::UpdateOnRemove();
}


//-----------------------------------------------------------------------------
// Where are how should we avoid?
//-----------------------------------------------------------------------------
#define HINT_PREDICTION_TIME 3.0f

CEntity *CInfoAPCMissileHint::FindAimTarget( CEntity *pMissile, const char *pTargetName,
											 const Vector &vecCurrentEnemyPos, const Vector &vecCurrentEnemyVel )
{
	if ( !pTargetName )
		return NULL;

	float flOOSpeed = pMissile->GetAbsVelocity().Length();
	if ( flOOSpeed != 0.0f )
	{
		flOOSpeed = 1.0f / flOOSpeed;
	}

	for ( int i = s_APCMissileHints.Count(); --i >= 0; )
	{
		CInfoAPCMissileHint *pHint = s_APCMissileHints[i];
		if ( !pHint->NameMatches( pTargetName ) )
			continue;

		if ( !pHint->m_hTarget )
			continue;

		Vector vecMissileToHint, vecMissileToEnemy;
		VectorSubtract( pHint->m_hTarget->WorldSpaceCenter(), pMissile->GetAbsOrigin(), vecMissileToHint );
		VectorSubtract( vecCurrentEnemyPos, pMissile->GetAbsOrigin(), vecMissileToEnemy );
		float flDistMissileToHint = VectorNormalize( vecMissileToHint );
		VectorNormalize( vecMissileToEnemy );
		if ( DotProduct( vecMissileToHint, vecMissileToEnemy ) < 0.866f )
			continue;

		// Determine when the target will be inside the volume.
		// Project at most 3 seconds in advance
		Vector vecRayDelta;
		VectorMultiply( vecCurrentEnemyVel, HINT_PREDICTION_TIME, vecRayDelta );

		BoxTraceInfo_t trace;
		if ( !IntersectRayWithOBB( vecCurrentEnemyPos, vecRayDelta, pHint->CollisionProp()->CollisionToWorldTransform(),
								   pHint->CollisionProp_Actual()->OBBMins(), pHint->CollisionProp_Actual()->OBBMaxs(), 0.0f, &trace ))
		{
			continue;
		}

		// Determine the amount of time it would take the missile to reach the target
		// If we can reach the target within the time it takes for the enemy to reach the
		float tSqr = flDistMissileToHint * flOOSpeed / HINT_PREDICTION_TIME;
		if ( (tSqr < (trace.t1 * trace.t1)) || (tSqr > (trace.t2 * trace.t2)) )
			continue;

		return pHint->m_hTarget;
	}

	return NULL;
}


//-----------------------------------------------------------------------------
// a list of missiles to search quickly
//-----------------------------------------------------------------------------
CEntityClassList<CAPCMissile> g_APCMissileList;
template <> CAPCMissile *CEntityClassList<CAPCMissile>::m_pClassList = NULL;
CAPCMissile *GetAPCMissileList()
{
	return g_APCMissileList.m_pClassList;
}

//-----------------------------------------------------------------------------
// Finds apc missiles in cone
//-----------------------------------------------------------------------------
CAPCMissile *FindAPCMissileInCone( const Vector &vecOrigin, const Vector &vecDirection, float flAngle )
{
	float flCosAngle = cos( DEG2RAD( flAngle ) );
	for( CAPCMissile *pEnt = GetAPCMissileList(); pEnt != NULL; pEnt = pEnt->m_pNext )
	{
		if ( !pEnt->IsSolid() )
			continue;

		Vector vecDelta;
		VectorSubtract( pEnt->GetAbsOrigin(), vecOrigin, vecDelta );
		VectorNormalize( vecDelta );
		float flDot = DotProduct( vecDelta, vecDirection );
		if ( flDot > flCosAngle )
			return pEnt;
	}

	return NULL;
}


//-----------------------------------------------------------------------------
//
// Specialized version of the missile
//
//-----------------------------------------------------------------------------
#define MAX_HOMING_DISTANCE 2250.0f
#define MIN_HOMING_DISTANCE 1250.0f
#define MAX_NEAR_HOMING_DISTANCE 1750.0f
#define MIN_NEAR_HOMING_DISTANCE 1000.0f
#define DOWNWARD_BLEND_TIME_START 0.2f
#define MIN_HEIGHT_DIFFERENCE	250.0f
#define MAX_HEIGHT_DIFFERENCE	550.0f
#define CORRECTION_TIME		0.2f
#define	APC_LAUNCH_HOMING_SPEED	0.1f
#define	APC_HOMING_SPEED	0.025f
#define HOMING_SPEED_ACCEL	0.01f

BEGIN_DATADESC( CAPCMissile )

					DEFINE_FIELD( m_flReachedTargetTime,	FIELD_TIME ),
					DEFINE_FIELD( m_flIgnitionTime,			FIELD_TIME ),
					DEFINE_FIELD( m_bGuidingDisabled,		FIELD_BOOLEAN ),
					DEFINE_FIELD( m_hSpecificTarget,		FIELD_EHANDLE ),
					DEFINE_FIELD( m_strHint,				FIELD_STRING ),
					DEFINE_FIELD( m_flLastHomingSpeed,		FIELD_FLOAT ),
//	DEFINE_FIELD( m_pNext,					FIELD_CLASSPTR ),

					DEFINE_THINKFUNC( BeginSeekThink ),
					DEFINE_THINKFUNC( AugerStartThink ),
					DEFINE_THINKFUNC( ExplodeThink ),

					DEFINE_FUNCTION( APCMissileTouch ),

END_DATADESC()

LINK_ENTITY_TO_CUSTOM_CLASS( apc_missile, cycler, CAPCMissile );

CAPCMissile *CAPCMissile::Create( const Vector &vecOrigin, const QAngle &vecAngles, const Vector &vecVelocity, CEntity *pOwner )
{
	CAPCMissile *pMissile = (CAPCMissile *)CEntity::Create( "apc_missile", vecOrigin, vecAngles, (pOwner)?pOwner->BaseEntity():NULL );
	pMissile->SetOwnerEntity( (pOwner)?pOwner->BaseEntity():NULL );
	pMissile->Spawn();
	pMissile->SetAbsVelocity( vecVelocity );
	pMissile->AddFlag( FL_NOTARGET );
	pMissile->AddEffects( EF_NOSHADOW );
	return pMissile;
}


//-----------------------------------------------------------------------------
// Constructor, destructor
//-----------------------------------------------------------------------------
CAPCMissile::CAPCMissile()
{
	g_APCMissileList.Insert( this );
}

CAPCMissile::~CAPCMissile()
{
	g_APCMissileList.Remove( this );
}


//-----------------------------------------------------------------------------
// Shared initialization code
//-----------------------------------------------------------------------------
void CAPCMissile::Init()
{
	SetMoveType( MOVETYPE_FLY );
	SetModel("models/weapons/w_missile.mdl");
	UTIL_SetSize( BaseEntity(), vec3_origin, vec3_origin );
	CreateSmokeTrail();
	SetTouch( &CAPCMissile::APCMissileTouch );
	m_flLastHomingSpeed = APC_HOMING_SPEED;
}


//-----------------------------------------------------------------------------
// For hitting a specific target
//-----------------------------------------------------------------------------
void CAPCMissile::AimAtSpecificTarget( CBaseEntity *pTarget )
{
	m_hSpecificTarget.Set(pTarget);
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  : *pOther -
//-----------------------------------------------------------------------------
void CAPCMissile::APCMissileTouch( CEntity *pOther )
{
	Assert( pOther );
	if ( !pOther->IsSolid() && !pOther->IsSolidFlagSet(FSOLID_VOLUME_CONTENTS) )
		return;

	Explode();
}


//-----------------------------------------------------------------------------
// Specialized version of the missile
//-----------------------------------------------------------------------------
void CAPCMissile::IgniteDelay( void )
{
	m_flIgnitionTime = gpGlobals->curtime + 0.3f;

	SetThink( &CAPCMissile::BeginSeekThink );
	SetNextThink( m_flIgnitionTime );
	Init();
	AddSolidFlags( FSOLID_NOT_SOLID );
}

void CAPCMissile::AugerDelay( float flDelay )
{
	m_flIgnitionTime = gpGlobals->curtime;
	SetThink( &CAPCMissile::AugerStartThink );
	SetNextThink( gpGlobals->curtime + flDelay );
	Init();
	DisableGuiding();
}

void CAPCMissile::AugerStartThink()
{
	if ( m_hRocketTrail != NULL )
	{
		m_hRocketTrail->m_bDamaged = true;
	}
	m_flAugerTime = gpGlobals->curtime + enginerandom->RandomFloat( 1.0f, 2.0f );
	SetThink( &CAPCMissile::AugerThink );
	SetNextThink( gpGlobals->curtime );
}

void CAPCMissile::ExplodeDelay( float flDelay )
{
	m_flIgnitionTime = gpGlobals->curtime;
	SetThink( &CAPCMissile::ExplodeThink );
	SetNextThink( gpGlobals->curtime + flDelay );
	Init();
	DisableGuiding();
}


void CAPCMissile::BeginSeekThink( void )
{
	RemoveSolidFlags( FSOLID_NOT_SOLID );
	SetThink( &CAPCMissile::SeekThink );
	SetNextThink( gpGlobals->curtime );
}

void CAPCMissile::ExplodeThink()
{
	DoExplosion();
}

//-----------------------------------------------------------------------------
// Health lost at which augering starts
//-----------------------------------------------------------------------------
int CAPCMissile::AugerHealth()
{
	return *(m_iMaxHealth) - 25;
}


//-----------------------------------------------------------------------------
// Health lost at which augering starts
//-----------------------------------------------------------------------------
void CAPCMissile::DisableGuiding()
{
	m_bGuidingDisabled = true;
}


//-----------------------------------------------------------------------------
// Guidance hints
//-----------------------------------------------------------------------------
void CAPCMissile::SetGuidanceHint( const char *pHintName )
{
	m_strHint = MAKE_STRING( pHintName );
}


//-----------------------------------------------------------------------------
// The actual explosion
//-----------------------------------------------------------------------------
void CAPCMissile::DoExplosion( void )
{
	if ( GetWaterLevel() != 0 )
	{
		CEffectData data;
		data.m_vOrigin = WorldSpaceCenter();
		data.m_flMagnitude = 128;
		data.m_flScale = 128;
		data.m_fFlags = 0;
		g_helpfunc.DispatchEffect( "WaterSurfaceExplosion", data );
	}
	else
	{
		CEntity *owner = GetOwnerEntity();
		ExplosionCreate( GetAbsOrigin(), GetAbsAngles(), (owner)?owner->BaseEntity():NULL,
						 APC_MISSILE_DAMAGE, 100, true, 20000 );
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CAPCMissile::ComputeLeadingPosition( const Vector &vecShootPosition, CEntity *pTarget, Vector *pLeadPosition )
{
	Vector vecTarget = pTarget->BodyTarget( vecShootPosition, false );
	float flShotSpeed = GetAbsVelocity().Length();
	if ( flShotSpeed == 0 )
	{
		*pLeadPosition = vecTarget;
		return;
	}

	Vector vecVelocity = pTarget->GetSmoothedVelocity();
	vecVelocity.z = 0.0f;
	float flTargetSpeed = VectorNormalize( vecVelocity );
	Vector vecDelta;
	VectorSubtract( vecShootPosition, vecTarget, vecDelta );
	float flTargetToShooter = VectorNormalize( vecDelta );
	float flCosTheta = DotProduct( vecDelta, vecVelocity );

	// Law of cosines... z^2 = x^2 + y^2 - 2xy cos Theta
	// where z = flShooterToPredictedTargetPosition = flShotSpeed * predicted time
	// x = flTargetSpeed * predicted time
	// y = flTargetToShooter
	// solve for predicted time using at^2 + bt + c = 0, t = (-b +/- sqrt( b^2 - 4ac )) / 2a
	float a = flTargetSpeed * flTargetSpeed - flShotSpeed * flShotSpeed;
	float b = -2.0f * flTargetToShooter * flCosTheta * flTargetSpeed;
	float c = flTargetToShooter * flTargetToShooter;

	float flDiscrim = b*b - 4*a*c;
	if (flDiscrim < 0)
	{
		*pLeadPosition = vecTarget;
		return;
	}

	flDiscrim = sqrt(flDiscrim);
	float t = (-b + flDiscrim) / (2.0f * a);
	float t2 = (-b - flDiscrim) / (2.0f * a);
	if ( t < t2 )
	{
		t = t2;
	}

	if ( t <= 0.0f )
	{
		*pLeadPosition = vecTarget;
		return;
	}

	VectorMA( vecTarget, flTargetSpeed * t, vecVelocity, *pLeadPosition );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CAPCMissile::ComputeActualDotPosition( CLaserDot *pLaserDot, Vector *pActualDotPosition, float *pHomingSpeed )
{
	if ( m_bGuidingDisabled )
	{
		*pActualDotPosition = GetAbsOrigin();
		*pHomingSpeed = 0.0f;
		m_flLastHomingSpeed = *pHomingSpeed;
		return;
	}

	if ( ( m_strHint != NULL_STRING ) && (!m_hSpecificTarget) )
	{
		Vector vecOrigin, vecVelocity;
		CEntity *pTarget = pLaserDot->GetTargetEntity();
		if ( pTarget )
		{
			vecOrigin = pTarget->BodyTarget( GetAbsOrigin(), false );
			vecVelocity	= pTarget->GetSmoothedVelocity();
		}
		else
		{
			vecOrigin = pLaserDot->GetChasePosition();
			vecVelocity = vec3_origin;
		}

		CEntity *cent = CInfoAPCMissileHint::FindAimTarget( this, STRING( m_strHint ), vecOrigin, vecVelocity );
		m_hSpecificTarget.Set((cent)?cent->BaseEntity():NULL);

	}

	CEntity *pLaserTarget = (m_hSpecificTarget) ? m_hSpecificTarget : pLaserDot->GetTargetEntity();
	if ( !pLaserTarget )
	{
		BaseClass::ComputeActualDotPosition( pLaserDot, pActualDotPosition, pHomingSpeed );
		m_flLastHomingSpeed = *pHomingSpeed;
		return;
	}

	if ( pLaserTarget->ClassMatches( "npc_bullseye" ) )
	{
		if ( m_flLastHomingSpeed != RPG_HOMING_SPEED )
		{
			if (m_flLastHomingSpeed > RPG_HOMING_SPEED)
			{
				m_flLastHomingSpeed -= HOMING_SPEED_ACCEL * (gpGlobals->interval_per_tick);
				if ( m_flLastHomingSpeed < RPG_HOMING_SPEED )
				{
					m_flLastHomingSpeed = RPG_HOMING_SPEED;
				}
			}
			else
			{
				m_flLastHomingSpeed += HOMING_SPEED_ACCEL * (gpGlobals->interval_per_tick);
				if ( m_flLastHomingSpeed > RPG_HOMING_SPEED )
				{
					m_flLastHomingSpeed = RPG_HOMING_SPEED;
				}
			}
		}
		*pHomingSpeed = m_flLastHomingSpeed;
		*pActualDotPosition = pLaserTarget->WorldSpaceCenter();
		return;
	}

	Vector vLaserStart;
	GetShootPosition( pLaserDot, &vLaserStart );
	*pHomingSpeed = APC_LAUNCH_HOMING_SPEED;

	//Get the laser's vector
	Vector vecTargetPosition = pLaserTarget->BodyTarget( GetAbsOrigin(), false );

	// Compute leading position
	Vector vecLeadPosition;
	ComputeLeadingPosition( GetAbsOrigin(), pLaserTarget, &vecLeadPosition );

	Vector vecTargetToMissile, vecTargetToShooter;
	VectorSubtract( GetAbsOrigin(), vecTargetPosition, vecTargetToMissile );
	VectorSubtract( vLaserStart, vecTargetPosition, vecTargetToShooter );

	*pActualDotPosition = vecLeadPosition;

	float flMinHomingDistance = MIN_HOMING_DISTANCE;
	float flMaxHomingDistance = MAX_HOMING_DISTANCE;
	float flBlendTime = gpGlobals->curtime - m_flIgnitionTime;
	if ( flBlendTime > DOWNWARD_BLEND_TIME_START )
	{
		if ( m_flReachedTargetTime != 0.0f )
		{
			*pHomingSpeed = APC_HOMING_SPEED;
			float flDeltaTime = clamp( gpGlobals->curtime - m_flReachedTargetTime, 0.0f, CORRECTION_TIME );
			*pHomingSpeed = SimpleSplineRemapVal( flDeltaTime, 0.0f, CORRECTION_TIME, 0.2f, *pHomingSpeed );
			flMinHomingDistance = SimpleSplineRemapVal( flDeltaTime, 0.0f, CORRECTION_TIME, MIN_NEAR_HOMING_DISTANCE, flMinHomingDistance );
			flMaxHomingDistance = SimpleSplineRemapVal( flDeltaTime, 0.0f, CORRECTION_TIME, MAX_NEAR_HOMING_DISTANCE, flMaxHomingDistance );
		}
		else
		{
			flMinHomingDistance = MIN_NEAR_HOMING_DISTANCE;
			flMaxHomingDistance = MAX_NEAR_HOMING_DISTANCE;
			Vector vecDelta;
			VectorSubtract( GetAbsOrigin(), *pActualDotPosition, vecDelta );
			if ( vecDelta.z > MIN_HEIGHT_DIFFERENCE )
			{
				float flClampedHeight = clamp( vecDelta.z, MIN_HEIGHT_DIFFERENCE, MAX_HEIGHT_DIFFERENCE );
				float flHeightAdjustFactor = SimpleSplineRemapVal( flClampedHeight, MIN_HEIGHT_DIFFERENCE, MAX_HEIGHT_DIFFERENCE, 0.0f, 1.0f );

				vecDelta.z = 0.0f;
				float flDist = VectorNormalize( vecDelta );

				float flForwardOffset = 2000.0f;
				if ( flDist > flForwardOffset )
				{
					Vector vecNewPosition;
					VectorMA( GetAbsOrigin(), -flForwardOffset, vecDelta, vecNewPosition );
					vecNewPosition.z = pActualDotPosition->z;

					VectorLerp( *pActualDotPosition, vecNewPosition, flHeightAdjustFactor, *pActualDotPosition );
				}
			}
			else
			{
				m_flReachedTargetTime = gpGlobals->curtime;
			}
		}

		// Allows for players right at the edge of rocket range to be threatened
		if ( flBlendTime > 0.6f )
		{
			float flTargetLength = GetAbsOrigin().DistTo( pLaserTarget->WorldSpaceCenter() );
			flTargetLength = clamp( flTargetLength, flMinHomingDistance, flMaxHomingDistance );
			*pHomingSpeed = SimpleSplineRemapVal( flTargetLength, flMaxHomingDistance, flMinHomingDistance, *pHomingSpeed, 0.01f );
		}
	}

	float flDot = DotProduct2D( vecTargetToShooter.AsVector2D(), vecTargetToMissile.AsVector2D() );
	if ( ( flDot < 0 ) || m_bGuidingDisabled )
	{
		*pHomingSpeed = 0.0f;
	}

	m_flLastHomingSpeed = *pHomingSpeed;

}
