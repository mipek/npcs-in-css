#include "CEntityBlocker.h"

CE_LINK_ENTITY_TO_CLASS(entity_blocker, CE_CEntityBlocker);

CE_CEntityBlocker *CE_CEntityBlocker::CE_CEntityBlocker::Create( const Vector &origin, const Vector &mins, const Vector &maxs, CBaseEntity *pOwner, bool bBlockPhysics )
{
	CE_CEntityBlocker *pBlocker = (CE_CEntityBlocker *) CEntity::Create( "entity_blocker", origin, vec3_angle, pOwner );

	if ( pBlocker != NULL )
	{
		UTIL_SetSize(pBlocker->BaseEntity(), mins, maxs );
		if ( bBlockPhysics )
		{
			pBlocker->VPhysicsInitStatic();
		}
	}

	return pBlocker;
}