
#include "CEnvExplosion.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



CE_LINK_ENTITY_TO_CLASS(CEnvExplosion, CE_CEnvExplosion);

// Datamaps
DEFINE_PROP(m_hInflictor, CE_CEnvExplosion);
DEFINE_PROP(m_iCustomDamageType, CE_CEnvExplosion);
DEFINE_PROP(m_hEntityIgnore, CE_CEnvExplosion);
DEFINE_PROP(m_iClassIgnore, CE_CEnvExplosion);


void ExplosionCreate( const Vector &center, const QAngle &angles, 
	CBaseEntity *pOwner, int magnitude, int radius, int nSpawnFlags, float flExplosionForce, CBaseEntity *pInflictor, int iCustomDamageType,
	const EHANDLE *ignoredEntity , Class_T ignoredClass )
{
	char			buf[128];

	CE_CEnvExplosion *pExplosion = (CE_CEnvExplosion*)CEntity::Create( "env_explosion", center, angles, pOwner );
	Q_snprintf( buf,sizeof(buf), "%3d", magnitude );
	const char *szKeyName = "iMagnitude";
	char *szValue = buf;
	pExplosion->CustomDispatchKeyValue( szKeyName, szValue );

	pExplosion->AddSpawnFlags( nSpawnFlags );

	if ( radius )
	{
		Q_snprintf( buf,sizeof(buf), "%d", radius );
		pExplosion->CustomDispatchKeyValue( "iRadiusOverride", buf );
	}

	if ( flExplosionForce != 0.0f )
	{
		Q_snprintf( buf,sizeof(buf), "%.3f", flExplosionForce );
		pExplosion->CustomDispatchKeyValue( "DamageForce", buf );
	}

	variant_t emptyVariant;
	pExplosion->m_nRenderMode = kRenderTransAdd;
	pExplosion->SetOwnerEntity( pOwner );
	pExplosion->Spawn();
	pExplosion->m_hInflictor.ptr->Set(pInflictor);
	pExplosion->SetCustomDamageType( iCustomDamageType );
	if (ignoredEntity)
	{
		pExplosion->m_hEntityIgnore.ptr->Set(*ignoredEntity);
	}
	pExplosion->m_iClassIgnore = ignoredClass;

	pExplosion->CustomAcceptInput( "Explode", NULL, NULL, emptyVariant, 0 );
}



void ExplosionCreate( const Vector &center, const QAngle &angles, 
	CBaseEntity *pOwner, int magnitude, int radius, bool doDamage, float flExplosionForce, bool bSurfaceOnly, bool bSilent, int iCustomDamageType )
{
	// For E3, no sparks
	int nFlags = SF_ENVEXPLOSION_NOSPARKS | SF_ENVEXPLOSION_NODLIGHTS | SF_ENVEXPLOSION_NOSMOKE;
	if ( !doDamage )
	{
		nFlags |= SF_ENVEXPLOSION_NODAMAGE;
	}

	if( bSurfaceOnly )
	{
		nFlags |= SF_ENVEXPLOSION_SURFACEONLY;
	}

	if( bSilent )
	{
		nFlags |= SF_ENVEXPLOSION_NOSOUND;
	}

	ExplosionCreate( center, angles, pOwner, magnitude, radius, nFlags, flExplosionForce, NULL, iCustomDamageType );
}

// this version lets you specify classes or entities to be ignored
void ExplosionCreate( const Vector &center, const QAngle &angles, 
					 CBaseEntity *pOwner, int magnitude, int radius, bool doDamage, 
					 const EHANDLE *ignoredEntity, Class_T ignoredClass,
					 float flExplosionForce , bool bSurfaceOnly , bool bSilent , int iCustomDamageType )
{
	// For E3, no sparks
	int nFlags = SF_ENVEXPLOSION_NOSPARKS | SF_ENVEXPLOSION_NODLIGHTS | SF_ENVEXPLOSION_NOSMOKE;
	if ( !doDamage )
	{
		nFlags |= SF_ENVEXPLOSION_NODAMAGE;
	}

	if( bSurfaceOnly )
	{
		nFlags |= SF_ENVEXPLOSION_SURFACEONLY;
	}

	if( bSilent )
	{
		nFlags |= SF_ENVEXPLOSION_NOSOUND;
	}

	ExplosionCreate( center, angles, pOwner, magnitude, radius, nFlags, flExplosionForce, NULL, iCustomDamageType, ignoredEntity, ignoredClass );
}

