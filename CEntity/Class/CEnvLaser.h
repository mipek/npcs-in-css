
#ifndef _INCLUDE_CENVLASER_H_
#define _INCLUDE_CENVLASER_H_

#include "CBeam.h"


class CE_CEnvLaser : public CE_CBeam
{
public:
	CE_DECLARE_CLASS(CE_CEnvLaser, CE_CBeam);

public:
	void	TurnOn( void );
	void	TurnOff( void );
	void	FireAtPoint( trace_t &tr );
	
	void	StrikeThink( void );

protected:
	DECLARE_DATAMAP(CBaseEntity *, m_pSprite);
	DECLARE_DATAMAP(string_t, m_iszLaserTarget);

};


#endif

