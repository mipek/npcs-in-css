
#include "CPropDoor.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



CE_LINK_ENTITY_TO_CLASS(CBasePropDoor, CPropDoor);



SH_DECL_MANUALHOOK0(GetOpenInterval, 0, 0, 0, float);
DECLARE_HOOK(GetOpenInterval, CPropDoor);
DECLARE_DEFAULTHANDLER(CPropDoor, GetOpenInterval, float,() , ());


// Datamaps
DEFINE_PROP(m_eDoorState, CPropDoor);



//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pNPC - 
//-----------------------------------------------------------------------------
bool CPropDoor::NPCOpenDoor( CAI_NPC *pNPC )
{
	// dvs: TODO: use activator filter here
	// dvs: TODO: outboard entity containing rules for whether door is operable?
	
	if ( IsDoorClosed() )
	{
		// Use the door
		Use( pNPC->BaseEntity(), pNPC->BaseEntity(), USE_ON, 0 );
	}

	return true;
}

bool CPropDoor::IsDoorClosed()
{
	return m_eDoorState == DOOR_STATE_CLOSED;
}
