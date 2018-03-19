#include "CDetour/detours.h"
#include "GameSystem.h"
#include "CCombatCharacter.h"
#include "player_pickup.h"
#include "CPlayer.h"


CDetour *AllocateDefaultRelationships_CDetour = NULL;
CDetour *UTIL_BloodDrips_CDetour = NULL;
CDetour *ShouldRemoveThisRagdoll_CDetour = NULL;
CDetour *FindInList_CDetour = NULL;
CDetour *Pickup_ForcePlayerToDropThisObject_CDetour = NULL;
CDetour *UTIL_GetLocalPlayer_CDetour = NULL;

// fix CLASS_
DETOUR_DECL_MEMBER0(CDetour_AllocateDefaultRelationships, void)
{
	CCombatCharacter::AllocateDefaultRelationships();
}

// fix npc blood
DETOUR_DECL_STATIC4(CDetour_UTIL_BloodDrips, void, const Vector &, origin, const Vector &, direction, int, color, int, amount)
{
	IPredictionSystem::SuppressHostEvents(NULL);
	DETOUR_STATIC_CALL(CDetour_UTIL_BloodDrips)(origin, direction, color, amount);
}

// fix crash
DETOUR_DECL_STATIC0(CDetour_ShouldRemoveThisRagdoll, bool)
{
	return true;
}

// fix env_gunfire target miss on CleanUpMap()
DETOUR_DECL_STATIC2(CDetour_FindInList, bool, const char **,pStrings, const char *,pToFind)
{
	if(strcmp(pToFind,"info_target") == 0)
		return false;

	bool ret = DETOUR_STATIC_CALL(CDetour_FindInList)(pStrings,pToFind);
	return ret;
}

// fix UTIL_GetLocalPlayer
DETOUR_DECL_STATIC1(CDetour_Pickup_ForcePlayerToDropThisObject, void, CBaseEntity *, pTarget)
{
	Pickup_ForcePlayerToDropThisObject(CEntity::Instance(pTarget));
}

// fix All UTIL_GetLocalPlayer
DETOUR_DECL_STATIC0(CDetour_UTIL_GetLocalPlayer, CBaseEntity *)
{
	for(int i=1;i<=gpGlobals->maxClients;i++)
	{
		CPlayer *pPlayer = UTIL_PlayerByIndex(i);
		if(!pPlayer)
			continue;
		return pPlayer->BaseEntity();
	}

	g_pSM->LogError(myself, "UTIL_GetLocalPlayer return NULL!");
	return NULL;
}

class PatchSystem : public CBaseGameSystem
{
public:
	PatchSystem(const char *name) : CBaseGameSystem(name)
	{
	}
	bool SDKInit()
	{
		GET_DETOUR(AllocateDefaultRelationships, DETOUR_CREATE_MEMBER(CDetour_AllocateDefaultRelationships, "AllocateDefaultRelationships"));

		GET_DETOUR(UTIL_BloodDrips, DETOUR_CREATE_STATIC(CDetour_UTIL_BloodDrips, "UTIL_BloodDrips"));
		
		GET_DETOUR(ShouldRemoveThisRagdoll, DETOUR_CREATE_STATIC(CDetour_ShouldRemoveThisRagdoll, "ShouldRemoveThisRagdoll"));
	
		GET_DETOUR(FindInList, DETOUR_CREATE_STATIC(CDetour_FindInList, "FindInList"));

		GET_DETOUR(Pickup_ForcePlayerToDropThisObject, DETOUR_CREATE_STATIC(CDetour_Pickup_ForcePlayerToDropThisObject, "Pickup_ForcePlayerToDropThisObject"));

		GET_DETOUR(UTIL_GetLocalPlayer, DETOUR_CREATE_STATIC(CDetour_UTIL_GetLocalPlayer, "UTIL_GetLocalPlayer"));

		return true;
	}
	void SDKShutdown()
	{
		if(AllocateDefaultRelationships_CDetour != NULL)
			AllocateDefaultRelationships_CDetour->DisableDetour();
		AllocateDefaultRelationships_CDetour = NULL;

		if(UTIL_BloodDrips_CDetour != NULL)
			UTIL_BloodDrips_CDetour->DisableDetour();
		UTIL_BloodDrips_CDetour = NULL;

		if(ShouldRemoveThisRagdoll_CDetour != NULL)
			ShouldRemoveThisRagdoll_CDetour->DisableDetour();
		ShouldRemoveThisRagdoll_CDetour = NULL;

		if(FindInList_CDetour != NULL)
			FindInList_CDetour->DisableDetour();
		FindInList_CDetour = NULL;

		if(Pickup_ForcePlayerToDropThisObject_CDetour != NULL)
			Pickup_ForcePlayerToDropThisObject_CDetour->DisableDetour();
		Pickup_ForcePlayerToDropThisObject_CDetour = NULL;

		if(UTIL_GetLocalPlayer_CDetour != NULL)
			UTIL_GetLocalPlayer_CDetour->DisableDetour();
		UTIL_GetLocalPlayer_CDetour = NULL;
	}
};


static PatchSystem g_patchsystem("PatchSystem");