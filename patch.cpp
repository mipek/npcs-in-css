#include "CDetour/detours.h"
#include "GameSystem.h"
#include "CCombatCharacter.h"
#include "player_pickup.h"
#include "CPlayer.h"
#include "CTrigger.h"

class PatchSystem : public CBaseGameSystem
{
	struct CachedEntityData
	{
		CachedEntityData(): entity_index(-1), name(NULL_STRING)
		{
		}
		int entity_index;
		string_t name;
		Vector pos;
	};

	CUtlVector<CachedEntityData> m_cachedData;
	//CachedEntityData m_cachedEntityData[ENTITY_ARRAY_SIZE];
public:
	PatchSystem(const char *name) : CBaseGameSystem(name)
	{
	}
	bool SDKInit();
	void SDKShutdown();
	void FixParentedPreCleanup();
	void FixParentedPostCleanup();

	CachedEntityData *GetCachedEntityData(int entityIndex)
	{
		for (int i=0; i<m_cachedData.Size(); ++i)
		{
			if (m_cachedData[i].entity_index == i)
			{
				return &m_cachedData[i];
			}
		}
		return nullptr;
	}
};


static PatchSystem g_patchsystem("PatchSystem");

CDetour *AllocateDefaultRelationships_CDetour = NULL;
CDetour *UTIL_BloodDrips_CDetour = NULL;
CDetour *ShouldRemoveThisRagdoll_CDetour = NULL;
CDetour *FindInList_CDetour = NULL;
CDetour *Pickup_ForcePlayerToDropThisObject_CDetour = NULL;
CDetour *UTIL_GetLocalPlayer_CDetour = NULL;
CDetour *CleanUpMap_CDetour = NULL;

// raydan (https://forums.alliedmods.net/showpost.php?p=2583960&postcount=4):
// it try fix round restart make some entity(info_target) position/angle go wrong
// in cs:s it auto call "round draw/TerminateRound" when first player join game, then some entity position/angle go wrong
// problem happen in hl2dm too, but hl2dm no round restart. you can call it manually
// example:
// the last boss in coop_zelda01_b
// any func_tank, example map tbr_coop_thousand_antlions_v3b
const char BrokenParentingEntities[][32] = {"move_rope",
										  "keyframe_rope",
										  "info_target",
										  "func_brush"};

#define GET_DETOUR(name, func)\
	name##_CDetour = func;\
	if(name##_CDetour == NULL)\
	{\
		g_pSM->LogError(myself,"Unable detouring %s",#name);\
		return false;\
	}\
	name##_CDetour->EnableDetour();

#define DestoryDetour(d) \
	if(d != NULL) \
		d->DisableDetour(); \
	d = NULL;

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
DETOUR_DECL_STATIC1(CDetour_ShouldRemoveThisRagdoll, bool, CBaseAnimating *, entity)
{
	return true;
}

// fix env_gunfire target miss on CleanUpMap()
DETOUR_DECL_STATIC2(CDetour_FindInList, bool, const char **,pStrings, const char *,pToFind)
{
	//if(strcmp(pToFind,"info_target") == 0)
	//	return false;

	if(strcmp(pToFind,"info_target") == 0 ||
	   strcmp(pToFind,"func_brush") == 0 ||
	   strcmp(pToFind,"move_rope") == 0 ||
	   strcmp(pToFind,"keyframe_rope") == 0 ||
	   strcmp(pToFind,"env_beam") == 0)
		return true;

	if(strcmp(pToFind, "info_player_start") == 0)
		return true;


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

// fix parent entity
DETOUR_DECL_MEMBER0(CDetour_CleanUpMap, void)
{
	//if(g_MonsterConfig.m_bEnable_CleanUpMap)
	{
		g_patchsystem.FixParentedPreCleanup();
		DETOUR_MEMBER_CALL(CDetour_CleanUpMap)();
		g_patchsystem.FixParentedPostCleanup();
	}
}

// fix crash
DETOUR_DECL_MEMBER1(CDetour_CTriggerHurt_HurtAllTouchers, int, float, dt)
{
	CTriggerHurt *trigger = (CTriggerHurt *)CEntity::Instance((CBaseEntity *)this);
	Assert(trigger);
	return trigger->HurtAllTouchers(dt);
}

bool PatchSystem::SDKInit()
{
	GET_DETOUR(AllocateDefaultRelationships, DETOUR_CREATE_MEMBER(CDetour_AllocateDefaultRelationships, "AllocateDefaultRelationships"));

	GET_DETOUR(UTIL_BloodDrips, DETOUR_CREATE_STATIC(CDetour_UTIL_BloodDrips, "UTIL_BloodDrips"));

	GET_DETOUR(ShouldRemoveThisRagdoll, DETOUR_CREATE_STATIC(CDetour_ShouldRemoveThisRagdoll, "ShouldRemoveThisRagdoll"));

	GET_DETOUR(FindInList, DETOUR_CREATE_STATIC(CDetour_FindInList, "FindInList"));

	GET_DETOUR(Pickup_ForcePlayerToDropThisObject, DETOUR_CREATE_STATIC(CDetour_Pickup_ForcePlayerToDropThisObject, "Pickup_ForcePlayerToDropThisObject"));

	GET_DETOUR(UTIL_GetLocalPlayer, DETOUR_CREATE_STATIC(CDetour_UTIL_GetLocalPlayer, "UTIL_GetLocalPlayer"));

	GET_DETOUR(CleanUpMap, DETOUR_CREATE_MEMBER(CDetour_CleanUpMap, "CleanUpMap"));

	return true;
}
void PatchSystem::SDKShutdown()
{
	DestoryDetour(AllocateDefaultRelationships_CDetour);
	DestoryDetour(UTIL_BloodDrips_CDetour);
	DestoryDetour(ShouldRemoveThisRagdoll_CDetour);
	DestoryDetour(FindInList_CDetour);
	DestoryDetour(Pickup_ForcePlayerToDropThisObject_CDetour);
	DestoryDetour(UTIL_GetLocalPlayer_CDetour);
	DestoryDetour(CleanUpMap_CDetour);
}


void PatchSystem::FixParentedPreCleanup()
{
	for (int i=1;i<=gpGlobals->maxClients;i++)
	{
		CPlayer *pPlayer = UTIL_PlayerByIndex(i);
		if(!pPlayer)
			continue;

		pPlayer->LeaveVehicle();
	}

	for (const char *name : BrokenParentingEntities)
	{
		CEntity *pSearch = nullptr;
		while ( ( pSearch = g_helpfunc.FindEntityByClassname( pSearch, name ) ) != nullptr )
		{
			CEntity *parent = pSearch->GetParent();
			if (parent)
			{
				int index = m_cachedData.AddToTail();
				m_cachedData[index].name = parent->GetEntityName_String();
				pSearch->SetParent(nullptr);
				if (m_cachedData[index].pos == vec3_origin)
				{
					m_cachedData[index].pos = pSearch->GetLocalOrigin();
				}
			}
		}
	}
}

void PatchSystem::FixParentedPostCleanup()
{
	for (const char *name : BrokenParentingEntities)
	{
		CEntity *pSearch = nullptr;
		while ( ( pSearch = g_helpfunc.FindEntityByClassname( pSearch, name ) ) != nullptr )
		{
			CachedEntityData *cachedData = GetCachedEntityData(pSearch->entindex_non_network());

			if (!cachedData || cachedData->name == NULL_STRING)
				continue;

			if (cachedData->pos != vec3_origin)
			{
				pSearch->SetLocalOrigin(cachedData->pos);
			}

			CEntity *parent = g_helpfunc.FindEntityByName( (CBaseEntity*)nullptr, STRING(cachedData->name) );
			if (parent)
			{
				pSearch->SetParent(parent->BaseEntity());
			}
		}
	}
}