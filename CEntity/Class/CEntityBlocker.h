#ifndef INCLUDE_CENTITYBLOCKER_H_
#define INCLUDE_CENTITYBLOCKER_H_

#include "CEntity.h"


class CE_CEntityBlocker : public CEntity
{
	DECLARE_CLASS( CE_CEntityBlocker, CEntity );

public:
	static CE_CEntityBlocker *Create( const Vector &origin, const Vector &mins, const Vector &maxs, CBaseEntity *pOwner = NULL, bool bBlockPhysics = false );
};


#endif //INCLUDE_CENTITYBLOCKER_H_
