
#include "CEntity.h"
#include "CInfoTarget_Fix.h"
#include "CE_recipientfilter.h"
#include "tempentity.h"
#include "explode.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


class CConcussiveBlast : public CE_InfoTarget_Fix
{
public:
	CE_DECLARE_CLASS( CConcussiveBlast, CE_InfoTarget_Fix );
	DECLARE_DATADESC();

	int		m_spriteTexture;

	CConcussiveBlast( void ) {}

	void Spawn( void )
	{
		Precache();
		BaseClass::Spawn();
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	// Output :
	//-----------------------------------------------------------------------------
	void Precache( void )
	{
		m_spriteTexture = PrecacheModel( "sprites/lgtning.vmt" );

		BaseClass::Precache();
	}

	//-----------------------------------------------------------------------------
	// Purpose: 
	// Output :
	//-----------------------------------------------------------------------------

	void Explode( float magnitude )
	{
		//Create a concussive explosion
		CPASFilter filter( GetAbsOrigin() );

		Vector vecForward;
		AngleVectors( GetAbsAngles(), &vecForward );

		te->Explosion(filter, 0.0, &GetAbsOrigin(), g_sModelIndexFireball , 1.0f, enginerandom->RandomInt( 8, 15 ), TE_EXPLFLAG_NOPARTICLES, (int)(256*magnitude), (int)(175*magnitude), &vecForward);

		int	colorRamp = enginerandom->RandomInt( 128, 255 );

		//Shockring
		CBroadcastRecipientFilter filter2;
		te->BeamRingPoint( filter2, 0,
						   GetAbsOrigin(),	//origin
						   16,			//start radius
						   300*magnitude,		//end radius
						   m_spriteTexture, //texture
						   0,			//halo index
						   0,			//start frame
						   2,			//framerate
						   0.3f,		//life
						   128,		//width
						   16,			//spread
						   0,			//amplitude
						   colorRamp,	//r
						   colorRamp,	//g
						   255,		//g
						   24,			//a
						   128			//speed
		);

		//Do the radius damage
		CEntity *owner = GetOwnerEntity();
		RadiusDamage( CTakeDamageInfo( BaseEntity(), (owner)?owner->BaseEntity():NULL, 200, DMG_BLAST|DMG_DISSOLVE ), GetAbsOrigin(), 256, CLASS_NONE, NULL );

		UTIL_Remove( this );
	}
};

LINK_ENTITY_TO_CUSTOM_CLASS( concussiveblast, info_target, CConcussiveBlast );

BEGIN_DATADESC( CConcussiveBlast )

//	DEFINE_FIELD( m_spriteTexture,	FIELD_INTEGER ),

END_DATADESC()


void CreateConcussiveBlast( const Vector &origin, const Vector &surfaceNormal, CBaseEntity *pOwner, float magnitude )
{
	QAngle angles;
	VectorAngles( surfaceNormal, angles );
	CConcussiveBlast *pBlast = (CConcussiveBlast *) CEntity::Create( "concussiveblast", origin, angles, pOwner );

	if ( pBlast )
	{
		pBlast->Explode( magnitude );
	}
}