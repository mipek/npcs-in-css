
#include "CGib.h"
#include "CBreakable.h"
#include "CPlayer.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CE_LINK_ENTITY_TO_CLASS(CGib, CE_CGib);

DEFINE_PROP(m_lifeTime, CE_CGib );
DEFINE_PROP(m_bForceRemove, CE_CGib );
DEFINE_PROP(m_material, CE_CGib );
DEFINE_PROP(m_cBloodDecals, CE_CGib );
DEFINE_PROP(m_bloodColor, CE_CGib );

void CE_CGib::InitGib( CEntity *pVictim, float fMinVelocity, float fMaxVelocity )
{
	// ------------------------------------------------------------------------
	// If have a pVictim spawn the gib somewhere in the pVictim's bounding volume
	// ------------------------------------------------------------------------
	if ( pVictim )
	{
		// Find a enginerandom position within the bounding box (add 1 to Z to get it out of the ground)
		Vector vecOrigin;
		pVictim->CollisionProp()->RandomPointInBounds( vec3_origin, Vector( 1, 1, 1 ), &vecOrigin );
		vecOrigin.z += 1.0f;
		SetAbsOrigin( vecOrigin );

		// make the gib fly away from the attack vector
		Vector vecNewVelocity =	 *g_vecAttackDir * -1;

		// mix in some noise
		vecNewVelocity.x += enginerandom->RandomFloat ( -0.25, 0.25 );
		vecNewVelocity.y += enginerandom->RandomFloat ( -0.25, 0.25 );
		vecNewVelocity.z += enginerandom->RandomFloat ( -0.25, 0.25 );

		vecNewVelocity *= enginerandom->RandomFloat ( fMaxVelocity, fMinVelocity );

		QAngle vecNewAngularVelocity = GetLocalAngularVelocity();
		vecNewAngularVelocity.x = enginerandom->RandomFloat ( 100, 200 );
		vecNewAngularVelocity.y = enginerandom->RandomFloat ( 100, 300 );
		SetLocalAngularVelocity( vecNewAngularVelocity );

		// copy owner's blood color
		SetBloodColor( pVictim->BloodColor() );

		AdjustVelocityBasedOnHealth( pVictim->m_iHealth, vecNewVelocity );

		// Attempt to be physical if we can
		if ( VPhysicsInitNormal( SOLID_BBOX, 0, false ) )
		{
			IPhysicsObject *pObj = VPhysicsGetObject();

			if ( pObj != NULL )
			{
				AngularImpulse angImpulse = RandomAngularImpulse( -500, 500 );
				pObj->AddVelocity( &vecNewVelocity, &angImpulse );
			}
		}
		else
		{
			SetSolid( SOLID_BBOX );
			SetCollisionBounds( vec3_origin, vec3_origin );
			SetAbsVelocity( vecNewVelocity );
		}

		SetCollisionGroup( COLLISION_GROUP_DEBRIS );
	}

	LimitVelocity();
}



void CE_CGib::Spawn( const char *szGibModel )
{
	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
	SetFriction(0.55); // deading the bounce a bit

	// sometimes an entity inherits the edict from a former piece of glass,
	// and will spawn using the same render FX or m_nRenderMode! bad!
	SetRenderColorA( 255 );
	m_nRenderMode = kRenderNormal;
	m_nRenderFX = kRenderFxNone;

	// hopefully this will fix the VELOCITY TOO LOW crap
	m_takedamage = DAMAGE_EVENTS_ONLY;
	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetCollisionGroup( COLLISION_GROUP_DEBRIS );

	SetModel( szGibModel );

	SetNextThink( gpGlobals->curtime + 4 );
	m_lifeTime = 25;
	SetTouch ( &CE_CGib::BounceGibTouch );

	m_bForceRemove = false;

	m_material = matNone;
	m_cBloodDecals = 5;
}

void CE_CGib::Spawn( const char *szGibModel, float flLifetime )
{
	Spawn( szGibModel );
	m_lifeTime = flLifetime;
	SetThink ( &CE_CGib::SUB_FadeOut );
	SetNextThink( gpGlobals->curtime + m_lifeTime );
}

void CE_CGib::BounceGibTouch ( CEntity *pOther )
{
	Vector	vecSpot;
	trace_t	tr;

	IPhysicsObject *pPhysics = VPhysicsGetObject();

	if ( pPhysics )
		return;

	//if ( enginerandom->RandomInt(0,1) )
	//	return;// don't bleed everytime
	if (GetFlags() & FL_ONGROUND)
	{
		SetAbsVelocity( GetAbsVelocity() * 0.9 );
		QAngle angles = GetLocalAngles();
		angles.x = 0;
		angles.z = 0;
		SetLocalAngles( angles );

		QAngle angVel = GetLocalAngularVelocity();
		angVel.x = 0;
		angVel.z = 0;
		SetLocalAngularVelocity( vec3_angle );
	}
	else
	{
		if ( m_cBloodDecals > 0 && m_bloodColor != DONT_BLEED )
		{
			vecSpot = GetAbsOrigin() + Vector ( 0 , 0 , 8 );//move up a bit, and trace down.
			UTIL_TraceLine ( vecSpot, vecSpot + Vector ( 0, 0, -24 ),  MASK_SOLID_BRUSHONLY, BaseEntity(), COLLISION_GROUP_NONE, &tr);

			UTIL_BloodDecalTrace( &tr, m_bloodColor );

			m_cBloodDecals--;
		}

		if ( m_material != matNone && enginerandom->RandomInt(0,2) == 0 )
		{
			float volume;
			float zvel = fabs(GetAbsVelocity().z);

			volume = 0.8f * MIN(1.0, ((float)zvel) / 450.0f);

			int m = m_material;
			CE_CBreakable::MaterialSoundRandom( entindex(), (Materials)m, volume );
		}
	}
}

void CE_CGib::SetBloodColor( int nBloodColor )
{
	m_bloodColor = nBloodColor;
}

void CE_CGib::LimitVelocity( void )
{
	Vector vecNewVelocity = GetAbsVelocity();
	float length = VectorNormalize( vecNewVelocity );

	// ceiling at 1500.  The gib velocity equation is not bounded properly.  Rather than tune it
	// in 3 separate places again, I'll just limit it here.
	if ( length > 1500.0 )
	{
		vecNewVelocity *= 1500;		// This should really be sv_maxvelocity * 0.75 or something
		SetAbsVelocity( vecNewVelocity );
	}
}

void CE_CGib::AdjustVelocityBasedOnHealth( int nHealth, Vector &vecVelocity )
{
	if ( nHealth > -50)
	{
		vecVelocity *= 0.7;
	}
	else if ( nHealth > -200)
	{
		vecVelocity *= 2;
	}
	else
	{
		vecVelocity *= 4;
	}
}


void CE_CGib::SpawnSpecificGibs(CBaseEntity*	pVictim,
								int				nNumGibs,
								float			vMinVelocity,
								float			vMaxVelocity,
								const char*		cModelName,
								float			flLifetime)
{
	CEntity *cent = CEntity::Instance(pVictim);
	for (int i=0;i<nNumGibs;i++)
	{
		CE_CGib *pGib = (CE_CGib *)CreateEntityByName("gib" );
		pGib->Spawn( cModelName, flLifetime );
		pGib->m_nBody = i;
		pGib->InitGib( cent, vMinVelocity, vMaxVelocity );
		pGib->m_lifeTime = flLifetime;

		if ( pVictim != NULL )
		{
			pGib->SetOwnerEntity( pVictim );
		}

		if ( cent != NULL, cent->GetFlags() & FL_ONFIRE )
		{
			pGib->Ignite( ( flLifetime - 1 ), false );
		}
	}
}

