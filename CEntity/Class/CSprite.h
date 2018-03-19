
#ifndef _INCLUDE_CSPRITE_H_
#define _INCLUDE_CSPRITE_H_

#include "CEntity.h"

#define SF_SPRITE_STARTON		0x0001
#define SF_SPRITE_ONCE			0x0002
#define SF_SPRITE_TEMPORARY		0x8000



class CE_CSprite : public CEntity
{
public:
	CE_DECLARE_CLASS(CE_CSprite, CEntity);

public:
	static CE_CSprite *SpriteCreate( const char *pSpriteName, const Vector &origin, bool animate );

public:
	void SpriteInit( const char *pSpriteName, const Vector &origin );
	void SetBrightness( int brightness, float duration = 0.0f );
	void SetScale( float scale, float duration = 0.0f );
	void SetSpriteScale( float scale );
	void AnimateUntilDead( void );
	void AnimateThink( void );
	void Animate( float frames );
	void TurnOff( void );
	void TurnOn( void );

	inline void SetAttachment( CBaseEntity *pEntity, int attachment )
	{
		if ( pEntity )
		{
			m_hAttachedToEntity.ptr->Set(pEntity);
			m_nAttachment = attachment;
			FollowEntity( pEntity );
		}
	}

	inline void SetTransparency( int rendermode, int r, int g, int b, int a, int fx )
	{
		SetRenderMode( (RenderMode_t)rendermode );
		SetColor( r, g, b );
		SetBrightness( a );
		m_nRenderFX = fx;
	}

	inline void SetColor( int r, int g, int b ) { SetRenderColor( r, g, b, GetRenderColor().a ); }

	void SetAsTemporary( void ) { AddSpawnFlags( SF_SPRITE_TEMPORARY ); }
	void SetGlowProxySize( float flSize ) { m_flGlowProxySize = flSize; }

	inline void FadeAndDie( float duration ) 
	{ 
		SetBrightness( 0, duration );
		SetThink(&CE_CSprite::AnimateUntilDead); 
		m_flDieTime = gpGlobals->curtime + duration; 
		SetNextThink( gpGlobals->curtime );  
	}

	inline void AnimateAndDie( float framerate ) 
	{ 
		SetThink(&CE_CSprite::AnimateUntilDead); 
		m_flSpriteFramerate = framerate;
		m_flDieTime = gpGlobals->curtime + (m_flMaxFrame / m_flSpriteFramerate); 
		SetNextThink( gpGlobals->curtime ); 
	}

	float GetScale( void ) { return m_flSpriteScale; }
	int	GetBrightness( void ) { return m_nBrightness; }

protected: //Sendprops
	DECLARE_SENDPROP(CFakeHandle, m_hAttachedToEntity);
	DECLARE_SENDPROP(int, m_nAttachment);
	DECLARE_SENDPROP(int, m_nBrightness);
	DECLARE_SENDPROP(float, m_flBrightnessTime);
	DECLARE_SENDPROP(float, m_flScaleTime);
	DECLARE_SENDPROP(float, m_flSpriteScale);
	DECLARE_SENDPROP(float, m_flSpriteFramerate);
	DECLARE_SENDPROP(float, m_flFrame);
	DECLARE_SENDPROP(float, m_flGlowProxySize);


	
protected: //Datamaps
	DECLARE_DATAMAP(float, m_flDieTime);
	DECLARE_DATAMAP(float, m_flMaxFrame);
	DECLARE_DATAMAP(float, m_flLastTime);


};



#endif
