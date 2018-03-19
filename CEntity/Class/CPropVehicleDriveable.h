#ifndef _INCLUDE_CPROPVEHICLEDRIVEABLE_H_
#define _INCLUDE_CPROPVEHICLEDRIVEABLE_H_

#include "CEntityManager.h"
#include "CEntity.h"

class CE_CPropVehicleDriveable : public CEntity
{
public:
	CE_DECLARE_CLASS(CE_CPropVehicleDriveable, CEntity);
	
public:
	CEntity *GetDriver();

protected:
	DECLARE_SENDPROP(CFakeHandle, m_hPlayer);

public:
	DECLARE_SENDPROP(int, m_nRPM);

protected:
	DECLARE_DATAMAP(CFakeHandle, m_hNPCDriver);

};

#endif

