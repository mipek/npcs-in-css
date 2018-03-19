
#ifndef _INCLUDE_CRAGDOLLPROP_H_
#define _INCLUDE_CRAGDOLLPROP_H_

#include "CEntity.h"
#include "CAnimating.h"

class CBaseEntity;

class CE_CRagdollProp : public CAnimating
{
public:
	CE_DECLARE_CLASS( CE_CRagdollProp, CAnimating );

	void SetDamageEntity ( CBaseEntity *pent ); 

public:
	//virtual void SetDamageEntity ( CBaseEntity *pent ); 
	//DECLARE_DEFAULTHEADER(SetDamageEntity, void, ( CBaseEntity *pent )); 
};


#endif
