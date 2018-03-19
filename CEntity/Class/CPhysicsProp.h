
#ifndef _INCLUDE_CPHYSICSPROP_H_
#define _INCLUDE_CPHYSICSPROP_H_

#include "CEntity.h"
#include "CProp.h"

class CE_CPhysicsProp : public CE_Prop
{
public:
	CE_DECLARE_CLASS(CE_CPhysicsProp, CE_Prop);

public:
	bool GetPropDataAngles( const char *pKeyName, QAngle &vecAngles );

};


#endif

