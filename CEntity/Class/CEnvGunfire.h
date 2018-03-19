
#ifndef _INCLUDE_CENVGUNFIRE_H_
#define _INCLUDE_CENVGUNFIRE_H_

#include "CEntity.h"


class CE_CEnvGunfire : public CEntity
{
public:
	CE_DECLARE_CLASS(CE_CEnvGunfire, CEntity);

public:
	virtual void Spawn(void);

protected:
	DECLARE_DATAMAP(Vector, m_vecTargetPosition);
	DECLARE_DATAMAP(string_t, m_target);
	DECLARE_DATAMAP(CFakeHandle, m_hTarget);
	DECLARE_DATAMAP(string_t, m_iszTracerType);
};


#endif
