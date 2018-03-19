
#include "CAI_NPC.h"
#include "eventlist.h"
#include "sign_func.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



Animevent EventList_RegisterPrivateEvent( const char *pszEventName )
{
	return g_helpfunc.EventList_RegisterPrivateEvent(pszEventName);
}

