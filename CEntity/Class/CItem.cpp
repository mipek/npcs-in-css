
#include "CEntity.h"
#include "CItem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



ConVar sv_css_item_respawn_time( "sv_css_item_respawn_time", "30", FCVAR_GAMEDLL | FCVAR_NOTIFY );
ConVar sk_battery( "sk_battery","0" );
ConVar sk_battery_max( "sk_battery_max","0" );


BEGIN_DATADESC_CENTITY( CPickupItem )
	DEFINE_INPUT( CPickupItem::m_bRespawn,		FIELD_BOOLEAN,	"mm_respawn" ),
END_DATADESC()

void CPickupItem::Spawn()
{
	m_bRespawn = true;
	BaseClass::Spawn( );
	UTIL_DropToFloor( this, MASK_SOLID );
}