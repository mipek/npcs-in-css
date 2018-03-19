
#include "CSprite.h"



// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



CE_LINK_ENTITY_TO_CLASS(CSprite, CE_CSprite);


DEFINE_PROP(m_hAttachedToEntity, CE_CSprite);
DEFINE_PROP(m_nAttachment, CE_CSprite);
DEFINE_PROP(m_nBrightness, CE_CSprite);
DEFINE_PROP(m_flBrightnessTime, CE_CSprite);
DEFINE_PROP(m_flScaleTime, CE_CSprite);
DEFINE_PROP(m_flSpriteScale, CE_CSprite);
DEFINE_PROP(m_flSpriteFramerate, CE_CSprite);
DEFINE_PROP(m_flFrame, CE_CSprite);
DEFINE_PROP(m_flGlowProxySize, CE_CSprite);



DEFINE_PROP(m_flDieTime, CE_CSprite);
DEFINE_PROP(m_flMaxFrame, CE_CSprite);
DEFINE_PROP(m_flLastTime, CE_CSprite);



CE_CSprite *CE_CSprite::SpriteCreate( const char *pSpriteName, const Vector &origin, bool animate )
{
	CE_CSprite *pSprite = (CE_CSprite *)CreateEntityByName("env_sprite");
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

void CE_CSprite::SpriteInit( const char *pSpriteName, const Vector &origin )
{
	SetModelName( MAKE_STRING(pSpriteName) );
	SetLocalOrigin( origin );
	Spawn();
}

void CE_CSprite::SetBrightness( int brightness, float time )
{
	m_nBrightness			= brightness;	//Take our current position as our starting position
	m_flBrightnessTime		= time;
}

void CE_CSprite::SetScale( float scale, float time )
{
	m_flScaleTime		= time;
	SetSpriteScale( scale );
	// The surrounding box is based on sprite scale... it changes, box is dirty
}

void CE_CSprite::SetSpriteScale( float scale )
{
	if ( scale != m_flSpriteScale )
	{
		m_flSpriteScale		= scale;	//Take our current position as our new starting position
		// The surrounding box is based on sprite scale... it changes, box is dirty
		CollisionProp()->MarkSurroundingBoundsDirty();

	}
}

void CE_CSprite::AnimateUntilDead( void )
{
	if ( gpGlobals->curtime > m_flDieTime )
	{
		Remove( );
	}
	else
	{
		AnimateThink();
		SetNextThink( gpGlobals->curtime );
	}
}

void CE_CSprite::AnimateThink( void )
{
	Animate( m_flSpriteFramerate * (gpGlobals->curtime - m_flLastTime) );

	SetNextThink( gpGlobals->curtime );
	m_flLastTime			= gpGlobals->curtime;
}

void CE_CSprite::Animate( float frames )
{ 
	m_flFrame += frames;
	if ( m_flFrame.ptr > m_flMaxFrame.ptr )
	{
		if ( m_spawnflags & SF_SPRITE_ONCE )
		{
			TurnOff();
		}
		else
		{
			if ( m_flMaxFrame > 0 )
				m_flFrame = fmod( m_flFrame, m_flMaxFrame );
		}
	}
}

void CE_CSprite::TurnOff( void )
{
	AddEffects( EF_NODRAW );
	SetNextThink( TICK_NEVER_THINK );
}

void CE_CSprite::TurnOn( void )
{
	RemoveEffects( EF_NODRAW );
	if ( (m_flSpriteFramerate && m_flMaxFrame > 1.0)
#if !defined( CLIENT_DLL )
		|| (m_spawnflags & SF_SPRITE_ONCE) 
#endif
		)
	{
		SetThink( &CE_CSprite::AnimateThink );
		SetNextThink( gpGlobals->curtime );
		m_flLastTime = gpGlobals->curtime;
	}
	m_flFrame = 0;
}