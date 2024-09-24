
#ifndef _INCLUDE_CENTITYFLAME_H_
#define _INCLUDE_CENTITYFLAME_H_

#include "CEntity.h"

class CE_CEntityFlame : public CEntity
{
public:
	CE_DECLARE_CLASS(CE_CEntityFlame, CEntity);

	void PostConstructor();


	void	FlameThink( void );

	void	AttachToEntity( CEntity *pTarget );
	void	SetLifetime( float lifetime );
	void	SetUseHitboxes( bool use );

	static CE_CEntityFlame	*Create( CEntity *pTarget, bool useHitboxes = true );

protected:
	DECLARE_SENDPROP(CFakeHandle, m_hEntAttached);

protected:
	DECLARE_DATAMAP(float, m_flSize);
	DECLARE_DATAMAP(float, m_flLifetime);
	DECLARE_DATAMAP(bool, m_bUseHitboxes);
	DECLARE_DATAMAP_OFFSET(bool, m_bPlayingSound);

protected:
	static VALVE_BASEPTR CEntityFlameFlameThink;

};


#endif