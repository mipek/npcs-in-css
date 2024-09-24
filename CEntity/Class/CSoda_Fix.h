#ifndef _INCLUDE_CSODA_FIX_H
#define _INCLUDE_CSODA_FIX_H

#include "CEntity.h"
#include "CAnimating.h"


abstract_class CSoda_Fix : public CAnimating
{
public:
	CE_DECLARE_CLASS(CSoda_Fix, CAnimating);
	CE_CUSTOM_ENTITY();

public:
	void Spawn();
	void Think();

};

#endif
