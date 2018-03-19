
#include "CAI_NPC.h"
#include "activitylist.h"
#include "sign_func.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



Activity ActivityList_RegisterPrivateActivity( const char *pszActivityName )
{
	return g_helpfunc.ActivityList_RegisterPrivateActivity(pszActivityName);
}

const char *ActivityList_NameForIndex( int activityIndex )
{
	return g_helpfunc.ActivityList_NameForIndex(activityIndex);
}

