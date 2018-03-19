#ifndef _INCLUDE_ITEM_HEALTHKIT_H
#define _INCLUDE_ITEM_HEALTHKIT_H

#include "CEntityManager.h"
#include "CEntity.h"
#include "CItem.h"
#include "CSode_Fix.h"

typedef CPickupItem CHealthItem;

class CHealthKit : public CHealthItem
{
public:
	CE_DECLARE_CLASS( CHealthKit, CHealthItem );

	void Spawn( void );
	void Precache( void );
	bool MyTouch( CPlayer *pPlayer );

	int	ObjectCaps() { return BaseClass::ObjectCaps() | FCAP_IMPULSE_USE; };

};


class CHealthVial : public CHealthItem
{
public:
	CE_DECLARE_CLASS( CHealthVial, CHealthItem );

	void Spawn( void );
	void Precache( void );
	bool MyTouch( CPlayer *pPlayer );

	int	ObjectCaps() { return BaseClass::ObjectCaps() | FCAP_IMPULSE_USE; };

};

#endif
