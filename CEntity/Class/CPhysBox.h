
#ifndef _INCLUDE_CPHYSBOX_H_
#define _INCLUDE_CPHYSBOX_H_

#include "CEntity.h"
#include "CBreakable.h"

class CE_CPhysBox : public CE_CBreakable
{
public:
	CE_DECLARE_CLASS( CE_CPhysBox, CE_CBreakable );

public:
	virtual int	ObjectCaps( void )
	{
		return BaseClass::ObjectCaps() | FCAP_IMPULSE_USE;
	}
};


#endif
