
#ifndef _INCLUDE_CTRIGGER_H_
#define _INCLUDE_CTRIGGER_H_

#include "CEntity.h"
#include "CToggle.h"

class CTrigger : public CToggle
{
public:
	CE_DECLARE_CLASS(CTrigger, CToggle);

	bool IsTouching( CEntity *pOther );
	void Enable( void );
	void Disable( void );

public:
	virtual bool PassesTriggerFilters(CBaseEntity *pOther);

public:
	DECLARE_DEFAULTHEADER(PassesTriggerFilters, bool, (CBaseEntity *pOther));


protected:
	DECLARE_DATAMAP(CUtlVector< EHANDLE >,m_hTouchingEntities);
	DECLARE_DATAMAP(bool ,m_bDisabled);



};



#endif
