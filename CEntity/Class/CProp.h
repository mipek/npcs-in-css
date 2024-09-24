
#ifndef _INCLUDE_CPROP_H_
#define _INCLUDE_CPROP_H_

#include "CEntity.h"
#include "CAnimating.h"

class CE_Prop : public CAnimating
{
public:
	CE_DECLARE_CLASS( CE_Prop, CAnimating );
public:
	virtual bool OverridePropdata();

public:
	void Think() override;

public:
	DECLARE_DEFAULTHEADER(OverridePropdata, bool, ());
};

#endif
