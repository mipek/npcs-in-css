
#ifndef _INCLUDE_CPHYSICSPROP_H_
#define _INCLUDE_CPHYSICSPROP_H_

#include "CEntity.h"
#include "CBreakableProp.h"

class CE_CPhysicsProp : public CE_CBreakableProp
{
public:
	CE_DECLARE_CLASS(CE_CPhysicsProp, CE_CBreakableProp);

public:
	bool GetPropDataAngles( const char *pKeyName, QAngle &vecAngles );

//public:
//	void OnPhysGunPickup( CBaseEntity *pPhysGunUser, PhysGunPickup_t reason = PICKED_UP_BY_CANNON ) override;
//	void OnPhysGunDrop( CBaseEntity *pPhysGunUser, PhysGunDrop_t reason ) override;

private:
	DECLARE_DATAMAP(COutputEvent, m_OnPhysGunPickup);
	DECLARE_DATAMAP(COutputEvent, m_OnPhysGunOnlyPickup);
	DECLARE_DATAMAP(COutputEvent, m_OnPhysGunPunt);
	DECLARE_DATAMAP(COutputEvent, m_OnPhysGunDrop);
};


#endif

