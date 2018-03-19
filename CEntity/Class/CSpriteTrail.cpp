
#include "CSpriteTrail.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



CE_LINK_ENTITY_TO_CLASS(CSpriteTrail, CE_CSpriteTrail);

DEFINE_PROP(m_flLifeTime, CE_CSpriteTrail);
DEFINE_PROP(m_flStartWidth, CE_CSpriteTrail);
DEFINE_PROP(m_flSkyboxScale, CE_CSpriteTrail);
DEFINE_PROP(m_flEndWidth, CE_CSpriteTrail);
DEFINE_PROP(m_flTextureRes, CE_CSpriteTrail);
DEFINE_PROP(m_flMinFadeLength, CE_CSpriteTrail);
DEFINE_PROP(m_vecSkyboxOrigin, CE_CSpriteTrail);
DEFINE_PROP(m_flStartWidthVariance, CE_CSpriteTrail);



CE_CSpriteTrail *CE_CSpriteTrail::SpriteTrailCreate( const char *pSpriteName, const Vector &origin, bool animate )
{
	CE_CSpriteTrail *pSprite = (CE_CSpriteTrail *)CreateEntityByName("env_spritetrail");

	pSprite->SpriteInit( pSpriteName, origin );
	pSprite->SetSolid( SOLID_NONE );
	pSprite->SetMoveType( MOVETYPE_NOCLIP );
	
	UTIL_SetSize( pSprite->BaseEntity(), vec3_origin, vec3_origin );

	if ( animate )
	{
		pSprite->TurnOn();
	}

	return pSprite;
}

void CE_CSpriteTrail::SetLifeTime( float time ) 
{ 
	m_flLifeTime = time; 
}

void CE_CSpriteTrail::SetStartWidth( float flStartWidth )
{ 
	m_flStartWidth = flStartWidth; 
	m_flStartWidth /= m_flSkyboxScale;
}

void CE_CSpriteTrail::SetStartWidthVariance( float flStartWidthVariance )
{ 
	m_flStartWidthVariance = flStartWidthVariance; 
	m_flStartWidthVariance /= m_flSkyboxScale;
}

void CE_CSpriteTrail::SetEndWidth( float flEndWidth )
{ 
	m_flEndWidth = flEndWidth; 
	m_flEndWidth /= m_flSkyboxScale;
}

void CE_CSpriteTrail::SetTextureResolution( float flTexelsPerInch )
{ 
	m_flTextureRes = flTexelsPerInch; 
	m_flTextureRes *= m_flSkyboxScale;
}

void CE_CSpriteTrail::SetMinFadeLength( float flMinFadeLength )
{
	m_flMinFadeLength = flMinFadeLength;
	m_flMinFadeLength /= m_flSkyboxScale;
}

void CE_CSpriteTrail::SetSkybox( const Vector &vecSkyboxOrigin, float flSkyboxScale )
{
	m_flTextureRes /= m_flSkyboxScale;
	m_flMinFadeLength *= m_flSkyboxScale;
	m_flStartWidth *= m_flSkyboxScale;
	m_flEndWidth *= m_flSkyboxScale;
	m_flStartWidthVariance *= m_flSkyboxScale;

	m_vecSkyboxOrigin = vecSkyboxOrigin;
	m_flSkyboxScale = flSkyboxScale;

	m_flTextureRes *= m_flSkyboxScale;
	m_flMinFadeLength /= m_flSkyboxScale;
	m_flStartWidth /= m_flSkyboxScale;
	m_flEndWidth /= m_flSkyboxScale;
	m_flStartWidthVariance /= m_flSkyboxScale;

	if ( IsInSkybox() )
	{
		AddEFlags( EFL_IN_SKYBOX ); 
	}
	else
	{
		RemoveEFlags( EFL_IN_SKYBOX ); 
	}
}


bool CE_CSpriteTrail::IsInSkybox() const
{
	return (m_flSkyboxScale != 1.0f) || (*(m_vecSkyboxOrigin) != vec3_origin);
}

