#include "CEntity.h"
#include "effects.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


class CE_CRotorWashShooter : public CEntity, public IRotorWashShooter
{
public:
	CE_DECLARE_CLASS( CE_CRotorWashShooter, CEntity );

public:
	virtual CBaseEntity *DoWashPush( float flWashStartTime, const Vector &vecForce );


public:
	DECLARE_DEFAULTHEADER(DoWashPush, CBaseEntity *, (float flWashStartTime, const Vector &vecForce ));

};


CE_LINK_ENTITY_TO_CLASS(CRotorWashShooter, CE_CRotorWashShooter);


SH_DECL_MANUALHOOK2(DoWashPush, 0, 0, 0, CBaseEntity *, float, const Vector &);
DECLARE_DEFAULTHANDLER_SUBCLASS(CE_CRotorWashShooter, IRotorWashShooter, DoWashPush, CBaseEntity *, (float flWashStartTime, const Vector &vecForce), (flWashStartTime, vecForce));
DECLARE_HOOK_SUBCLASS(DoWashPush, CE_CRotorWashShooter, IRotorWashShooter);



IRotorWashShooter *GetRotorWashShooter( CEntity *pEntity )
{
	CE_CRotorWashShooter *pShooter = dynamic_cast<CE_CRotorWashShooter*>(pEntity);
	return pShooter;
}