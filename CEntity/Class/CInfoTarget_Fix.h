#ifndef _INCLUDE_CINFOTARGET_FIX_H_
#define _INCLUDE_CINFOTARGET_FIX_H_

#include "CEntityManager.h"
#include "CEntity.h"

abstract_class CE_InfoTarget_Fix : public CEntity
{
public:
	CE_DECLARE_CLASS(CE_InfoTarget_Fix, CEntity);
	CE_CUSTOM_ENTITY();

public:
	virtual void Spawn(void);
	virtual int	ObjectCaps( void );
	virtual void Think(void);
};


#endif
