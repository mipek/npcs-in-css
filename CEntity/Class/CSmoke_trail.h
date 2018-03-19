
#ifndef _INCLUDE_CSMOKE_TRAIL_H_
#define _INCLUDE_CSMOKE_TRAIL_H_

#include "CEntity.h"



class CSmokeTrail : public CEntity
{
public:
	CE_DECLARE_CLASS(CSmokeTrail, CEntity);

public:
	static	CSmokeTrail*		CreateSmokeTrail();
	
	void			SetLifetime(float lifetime);


public:
	DECLARE_SENDPROP( Vector, m_StartColor );
	DECLARE_SENDPROP( Vector, m_EndColor );
	DECLARE_SENDPROP( float, m_Opacity );
	DECLARE_SENDPROP( float, m_SpawnRate );
	DECLARE_SENDPROP( float, m_ParticleLifetime );
	DECLARE_SENDPROP( float, m_StopEmitTime );
	DECLARE_SENDPROP( float, m_MinSpeed );
	DECLARE_SENDPROP( float, m_MaxSpeed );
	DECLARE_SENDPROP( float, m_StartSize );
	DECLARE_SENDPROP( float, m_EndSize );	
	DECLARE_SENDPROP( float, m_SpawnRadius );
	DECLARE_SENDPROP( float, m_MinDirectedSpeed );
	DECLARE_SENDPROP( float, m_MaxDirectedSpeed );
	DECLARE_SENDPROP( bool, m_bEmit );
	DECLARE_SENDPROP( int, m_nAttachment );
};

class CE_SporeExplosion : public CEntity
{
public:
	CE_DECLARE_CLASS(CE_SporeExplosion, CEntity);

	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );

public:
	DECLARE_SENDPROP( bool, m_bDontRemove );
	DECLARE_SENDPROP( bool, m_bEmit );
	DECLARE_SENDPROP( float, m_flSpawnRate );

public:
	DECLARE_DATAMAP(bool, m_bDisabled);

};



#endif
