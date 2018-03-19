
#ifndef _INCLUDE_CSPRITETRAIL_H_
#define _INCLUDE_CSPRITETRAIL_H_

#include "CSprite.h"


class CE_CSpriteTrail : public CE_CSprite
{
public:
	CE_DECLARE_CLASS(CE_CSpriteTrail, CE_CSprite);

public:
	static CE_CSpriteTrail *SpriteTrailCreate( const char *pSpriteName, const Vector &origin, bool animate );

	void SetStartWidth( float flStartWidth );
	void SetEndWidth( float flEndWidth );
	void SetStartWidthVariance( float flStartWidthVariance );
	void SetTextureResolution( float flTexelsPerInch );
	void SetLifeTime( float time );
	void SetMinFadeLength( float flMinFadeLength );
	void SetSkybox( const Vector &vecSkyboxOrigin, float flSkyboxScale );

	bool IsInSkybox() const;

protected: //Sendprops
	DECLARE_SENDPROP(float, m_flLifeTime);
	DECLARE_SENDPROP(float, m_flStartWidth);
	DECLARE_SENDPROP(float, m_flSkyboxScale);
	DECLARE_SENDPROP(float, m_flEndWidth);
	DECLARE_SENDPROP(float, m_flTextureRes);
	DECLARE_SENDPROP(float, m_flMinFadeLength);
	DECLARE_SENDPROP(Vector, m_vecSkyboxOrigin);
	DECLARE_SENDPROP(float, m_flStartWidthVariance);

protected: //Datamaps



};


#endif

