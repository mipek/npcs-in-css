
#ifndef _INCLUDE_CFILTERS_H_
#define _INCLUDE_CFILTERS_H_

#include "CEntity.h"

class CE_CBaseFilter : public CEntity
{
public:
	CE_DECLARE_CLASS( CE_CBaseFilter, CEntity );

	bool PassesFilter( CBaseEntity *pCaller, CBaseEntity *pEntity );

public:
	virtual bool PassesFilterImpl( CBaseEntity *pCaller, CBaseEntity *pEntity );

public:
	DECLARE_DEFAULTHEADER(PassesFilterImpl, bool, (CBaseEntity *pCaller, CBaseEntity *pEntity));


protected: //Datamaps
	DECLARE_DATAMAP(bool, m_bNegated);



};


#endif