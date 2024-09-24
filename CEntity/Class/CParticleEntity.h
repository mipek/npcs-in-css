
#ifndef _INCLUDE_CPARTICLE_ENTITY_H_
#define _INCLUDE_CPARTICLE_ENTITY_H_

#include "CEntity.h"


class CParticleEntity : public CEntity
{
public:
	CE_DECLARE_CLASS(CParticleEntity, CEntity);

public:
	void	SetLifetime(float lifetime);

	void	FollowEntity( CEntity *pEntity);

public:


};



#endif
