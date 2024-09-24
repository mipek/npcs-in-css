#ifndef _INCLUDE_ITEM_HEALTHKIT_H
#define _INCLUDE_ITEM_HEALTHKIT_H

#include "CEntityManager.h"
#include "CEntity.h"
#include "CItem.h"
#include "CSode_Fix.h"

class CHealthKit : public CItem<CSode_Fix>
{
public:
	CE_DECLARE_CLASS( CHealthKit, CItem<CSode_Fix> );

	void Spawn( void );
	void Precache( void );
	CEntity* MyTouch( CPlayer *pPlayer );

	int	ObjectCaps() { return BaseClass::ObjectCaps() | FCAP_IMPULSE_USE; };

};


class CHealthVial : public CItem<CSode_Fix>
{
public:
	CE_DECLARE_CLASS( CHealthVial, CItem<CSode_Fix> );

	void Spawn( void );
	void Precache( void );
	CEntity* MyTouch( CPlayer *pPlayer );

	int	ObjectCaps() { return BaseClass::ObjectCaps() | FCAP_IMPULSE_USE; };

};

#endif
