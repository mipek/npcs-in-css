
#include "CEntityFlame.h"
#include "CAI_NPC.h"

CE_LINK_ENTITY_TO_CLASS( CEntityFlame, CE_CEntityFlame );


DEFINE_PROP(m_flSize, CE_CEntityFlame);
DEFINE_PROP(m_hEntAttached, CE_CEntityFlame);
DEFINE_PROP(m_flLifetime, CE_CEntityFlame);
DEFINE_PROP(m_bUseHitboxes, CE_CEntityFlame);


VALVE_BASEPTR CE_CEntityFlame::CEntityFlameFlameThink = NULL;


void CE_CEntityFlame::PostConstructor()
{
	BaseClass::PostConstructor();
	m_bPlayingSound.offset = m_flLifetime.offset + 4;
	m_bPlayingSound.ptr = (bool *)(((uint8_t *)(BaseEntity())) + m_bPlayingSound.offset);

	if(CEntityFlameFlameThink == NULL)
	{
		void *ptr = UTIL_FunctionFromName( GetDataDescMap_Real(), "CEntityFlameFlameThink");
		if(ptr)
		{
			memcpy(&CEntityFlameFlameThink, &ptr, sizeof(void *));
		}
	}
	Assert(CEntityFlameFlameThink);
}


CE_CEntityFlame	*CE_CEntityFlame::Create( CEntity *pTarget, bool useHitboxes)
{
	CE_CEntityFlame *pFlame = (CE_CEntityFlame *) CreateEntityByName( "entityflame" );

	if ( pFlame == NULL )
		return NULL;

	float xSize = pTarget->CollisionProp_Actual()->OBBMaxs().x - pTarget->CollisionProp_Actual()->OBBMins().x;
	float ySize = pTarget->CollisionProp_Actual()->OBBMaxs().y - pTarget->CollisionProp_Actual()->OBBMins().y;

	float size = ( xSize + ySize ) * 0.5f;

	if ( size < 16.0f )
	{
		size = 16.0f;
	}

	UTIL_SetOrigin( pFlame, pTarget->GetAbsOrigin() );

	pFlame->m_flSize = size;
	pFlame->SetThink( &CE_CEntityFlame::FlameThink );
	pFlame->SetNextThink( gpGlobals->curtime + 0.1f );

	pFlame->AttachToEntity( pTarget );
	pFlame->SetLifetime( 2.0f );

	//Send to the client even though we don't have a model
	pFlame->AddEFlags( EFL_FORCE_CHECK_TRANSMIT );

	pFlame->SetUseHitboxes( useHitboxes );

	return pFlame;

}

void CE_CEntityFlame::SetUseHitboxes( bool use )
{
	m_bUseHitboxes = use;
}

void CE_CEntityFlame::SetLifetime( float lifetime )
{
	m_flLifetime = gpGlobals->curtime + lifetime;
}


void CE_CEntityFlame::AttachToEntity( CEntity *pTarget )
{
	// For networking to the client.
	m_hEntAttached.ptr->Set(pTarget->BaseEntity());

	if( pTarget->IsNPC() )
	{
		EmitSound( "General.BurningFlesh" );
	}
	else
	{
		EmitSound( "General.BurningObject" );
	}

	m_bPlayingSound = true;

	// So our heat emitter follows the entity around on the server.
	SetParent( pTarget->BaseEntity() );
}


void CE_CEntityFlame::FlameThink( void )
{
	(BaseEntity()->*CEntityFlameFlameThink)();
}


