
#include "CItem.h"
#include "CSode_Fix.h"
#include "CPlayer.h"
#include "ItemRespawnSystem.h"
#include "vphysics/constraints.h"


class CItemBattery : public CPickupItem
{
public:
	CE_DECLARE_CLASS( CItemBattery, CPickupItem );

	void Spawn( void )
	{ 
		Precache();
		SetModel( "models/items/battery.mdl" );
		BaseClass::Spawn( );
	}
	void Precache( void )
	{
		BaseClass::Precache();
		PrecacheModel ("models/items/battery.mdl");
		PrecacheScriptSound( "ItemBattery.Touch" );

	}
	bool MyTouch( CPlayer *pPlayer )
	{
		return pPlayer->ApplyBattery();
	}
};

LINK_ENTITY_TO_CUSTOM_CLASS( item_battery, item_sodacan, CItemBattery );


