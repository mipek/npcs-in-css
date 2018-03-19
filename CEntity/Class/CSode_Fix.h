#ifndef _INCLUDE_CSODA_FIX_H
#define _INCLUDE_CSODA_FIX_H

#include "CEntityManager.h"
#include "CEntity.h"
#include "CAnimating.h"


abstract_class CSode_Fix : public CAnimating
{
public:
	CE_DECLARE_CLASS(CSode_Fix, CAnimating);
	CE_CUSTOM_ENTITY();

public:
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual void Think(void);


};

#endif
