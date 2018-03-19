
#ifndef _INCLUDE_CDYNAMICLINK_H_
#define _INCLUDE_CDYNAMICLINK_H_

#include "CEntity.h"


class CDynamicLink : public CEntity
{
public:
	CE_DECLARE_CLASS(CDynamicLink, CEntity);

public:
	DECLARE_DATAMAP(string_t, m_strAllowUse);
	DECLARE_DATAMAP(bool, m_bInvertAllow);



	
};





#endif
