

#ifndef _INCLUDE_CWORLD_H_
#define _INCLUDE_CWORLD_H_

#include "CEntity.h"

class CE_CWorld : public CEntity
{
public:
	CE_DECLARE_CLASS( CE_CWorld, CEntity );

	void Precache();
};


#endif
