#include "sign_func.h"
#include "CEntity.h"
#include "CDetour/detours.h"
#include "CAI_NPC.h"
#include "ai_namespaces.h"
#include "CAI_schedule.h"
#include "CAI_senses.h"
#include "ragdoll_shared.h"
class CBasePlayer;
#include "soundenvelope.h"
#include "stringregistry.h"
#include "GameSystem.h"
#include "stringpool.h"
#include "gamemovement.h"
#include "CPlayer.h"
#include "ammodef.h"
#include "eventqueue.h"
#include "CAI_Hint.h"
#include "ai_waypoint.h"
#include "CAI_NetworkManager.h"
#include "callqueue.h"
#include "CPathTrack.h"

#ifdef PLATFORM_WINDOWS
#define ALWAYSINLINE __forceinline
#else
#define ALWAYSINLINE inline __attribute__((always_inline))
#endif

class CAI_SensedObjectsManager;
struct TemplateEntityData_t;
class CGameStringPool;
class CMemoryPool;
class CCallQueue;
class CCheckClient;
class CFlexSceneFileManager;
class CGlobalState;
class CDefaultResponseSystem;
class CGameMovement;
class CPhysicsHook;
class CCollisionEvent;


HelperFunction g_helpfunc;



CGameMovement *g_CGameMovement = NULL;
Vector	*g_vecAttackDir = NULL;
CSoundEnvelopeController *g_SoundController = NULL;
IPredictionSystem *IPredictionSystem::g_pPredictionSystems = NULL;
CAI_SensedObjectsManager *g_AI_SensedObjectsManager = NULL;
CCallQueue *g_PostSimulationQueue = NULL;
INotify *g_pNotify = NULL;
IPhysicsObject *g_PhysWorldObject = NULL;
CViewVectors *g_ViewVectors = NULL;
CViewVectors *g_DefaultViewVectors = NULL;
IMoveHelper **sm_pSingleton = NULL;
int *g_interactionHitByPlayerThrownPhysObj = NULL;

extern CMultiDamage *my_g_MultiDamage;
extern CUtlVector<TemplateEntityData_t *> *g_Templates;
extern CEMemoryPool *g_EntityListPool;
extern ConVar *ammo_hegrenade_max;
extern ConVar *sk_autoaim_mode;
extern trace_t *g_TouchTrace;
extern INetworkStringTable *g_pStringTableParticleEffectNames;
extern CUtlVector<IValveGameSystem*> *s_GameSystems;
extern CCheckClient *g_CheckClient;
extern CFlexSceneFileManager *g_FlexSceneFileManager;
extern CGlobalState *gGlobalState;
extern CUtlMap< int,  CAIHintVector > *my_gm_TypedHints;
extern CAI_SquadManager *g_AI_SquadManager;
extern CPhysicsHook *g_PhysicsHook;
extern CCollisionEvent *g_Collisions;

extern void InitDefaultAIRelationships();

extern bool SetResponseSystem();

SH_DECL_MANUALHOOK0(GameRules_FAllowNPCsHook, 0, 0, 0, bool);
SH_DECL_MANUALHOOK2(GameRules_ShouldCollideHook, 0, 0, 0, bool, int, int);
SH_DECL_MANUALHOOK2(GameRules_IsSpawnPointValid, 0, 0, 0, bool, CBaseEntity*, CBaseEntity*);
SH_DECL_MANUALHOOK1(OnLadderHook, 0, 0, 0, bool, trace_t &);

class HelperSystem : public CBaseGameSystem
{
public:
	HelperSystem(const char *name) : CBaseGameSystem(name)
	{
	}
	void LevelInitPreEntity()
	{
		g_helpfunc.LevelInitPreEntity();
	}
	void LevelInitPostEntity()
	{
		g_helpfunc.LevelInitPostEntity();
	}
	void LevelShutdownPreEntity()
	{
		g_helpfunc.LevelShutdownPreEntity();
	}
	void LevelShutdownPostEntity()
	{
		g_helpfunc.LevelShutdownPostEntity();
	}
	void SDKShutdown()
	{
		g_helpfunc.Shutdown();
	}
};


static HelperSystem g_helpersystem("HelperSystem");


template <typename ReturnType, typename... Args>
static ALWAYSINLINE ReturnType call_virtual(void* instance, int index, Args... args)
{
	using Fn = ReturnType(THISCALLCONV *)(void*, Args...);

	auto function = (*reinterpret_cast<Fn**>(instance))[index];
	return function(instance, args...);
}

string_t AllocPooledString( const char * pszValue )
{
	return g_helpfunc.AllocPooledString(pszValue);
}

string_t FindPooledString( const char *pszValue )
{
	return g_helpfunc.FindPooledString(pszValue);
}

HelperFunction::HelperFunction()
{
	my_g_pGameRules = NULL;
	g_SoundEmitterSystem = NULL;
}

template<typename T>
static bool GetPointerViaGameConf(const char *name, T& ptr)
{
	static_assert(std::is_pointer<T>::value, "Expected a pointer");
	char* addr = nullptr;

	META_CONPRINTF("[%s] Getting %s - ", g_Monster.GetLogTag(), name);
	if (!g_pGameConf->GetMemSig(name, reinterpret_cast<void**>(&addr)) || addr == nullptr) {
		META_CONPRINT("Fail\n");
		g_pSM->LogError(myself, "Unable to get %s", name);
		return false;
	}

#if defined(PLATFORM_WINDOWS)
	int offset;
	if (!g_pGameConf->GetOffset(name, &offset)) {
		META_CONPRINT("Fail\n");
		g_pSM->LogError(myself, "Unable to get offset for %s", name);
		return false;
	}

	void* finalAddr = reinterpret_cast<void*>(addr + offset);
	//memcpy(&ptr, finalAddr, sizeof(T*));
	ptr = (T)finalAddr;
#else
	//memcpy(&ptr, addr, sizeof(T*));
	ptr = (T)addr;
#endif

	META_CONPRINT("Success\n");
	return true;
}

static void* GetVariableViaGameConf(const char *name)
{
	char* addr = nullptr;

	META_CONPRINTF("[%s] Getting %s - ", g_Monster.GetLogTag(), name);
	if (!g_pGameConf->GetMemSig(name, reinterpret_cast<void**>(&addr)) || addr == nullptr) {
		META_CONPRINT("Fail\n");
		g_pSM->LogError(myself, "Unable to get %s", name);
		return nullptr;
	}

#if defined(PLATFORM_WINDOWS)
	int offset;
	if (!g_pGameConf->GetOffset(name, &offset)) {
		META_CONPRINT("Fail\n");
		g_pSM->LogError(myself, "Unable to get offset for %s", name);
		return false;
	}

	void* finalAddr = reinterpret_cast<void*>(addr + offset);
	//memcpy(ptr, finalAddr, sizeof(T));
#else
	void* finalAddr = addr;
#endif

	META_CONPRINT("Success\n");
	return finalAddr;
}


void HelperFunction::LevelInitPreEntity()
{
	g_pStringTableParticleEffectNames = netstringtables->FindTable("ParticleEffectNames");

	g_AI_SchedulesManager.CreateStringRegistries();

	InitDefaultAIRelationships();

	HookGameRules();

	g_ViewVectors = GameRules_GetViewVectors();

	META_CONPRINTF("[%s] Server may crash at this time!\n",g_Monster.GetLogTag());

	my_g_EntityCollisionHash = *(IPhysicsObjectPairHash**)(GetVariableViaGameConf("g_EntityCollisionHash"));

	CBaseEntity *g_WorldEntity = *(CBaseEntity**)GetVariableViaGameConf("g_WorldEntity");
	my_g_WorldEntity = CEntity::Instance(g_WorldEntity);

	CAI_NPC::m_pActivitySR = *(CStringRegistry**) GetVariableViaGameConf("m_pActivitySR");
	CAI_NPC::m_pEventSR = *(CStringRegistry**) GetVariableViaGameConf("m_pEventSR");

	CAI_NPC::m_iNumActivities = (int*) GetVariableViaGameConf("m_iNumActivities");
	CAI_NPC::m_iNumEvents = (int*) GetVariableViaGameConf("m_iNumEvents");

	sm_pSingleton = (IMoveHelper**) GetVariableViaGameConf("sm_pSingleton");

	//GET_VARIABLE(g_PhysWorldObject, IPhysicsObject *);
	g_PhysWorldObject = *(IPhysicsObject**)GetVariableViaGameConf("g_PhysWorldObject");

	g_AI_SchedulesManager.LoadAllSchedules();

	if(g_ViewVectors && g_DefaultViewVectors == NULL)
	{
		g_DefaultViewVectors = new CViewVectors();
		memcpy(g_DefaultViewVectors, g_ViewVectors, sizeof(CViewVectors));
	}
}

void HelperFunction::LevelInitPostEntity()
{
	CEAI_NetworkManager::InitializeAINetworks();
}

void HelperFunction::LevelShutdownPreEntity()
{
	UnHookGameRules();
}

void HelperFunction::LevelShutdownPostEntity()
{
	g_AI_SchedulesManager.DeleteAllSchedules();
	g_AI_SchedulesManager.DestroyStringRegistries();
}


void HelperFunction::Shutdown()
{
	LevelShutdownPreEntity();
	CCombatCharacter::Shutdown();

	SH_REMOVE_MANUALHOOK_MEMFUNC(OnLadderHook, g_CGameMovement, &g_helpfunc, &HelperFunction::OnLadder, false);

	delete g_DefaultViewVectors;
}

bool HelperFunction::FindAllValveGameSystem()
{
	FindValveGameSystem(g_CheckClient, CCheckClient *, "CCheckClient");
	
	FindValveGameSystem(g_PropDataSystem, CPropData *, "CPropData");

	FindValveGameSystem(g_SoundEmitterSystem, CValveBaseGameSystem *, "CSoundEmitterSystem");

	FindValveGameSystem(g_FlexSceneFileManager, CFlexSceneFileManager *, "CFlexSceneFileManager");

	FindValveGameSystem(gGlobalState, CGlobalState *, "CGlobalState");

	FindValveGameSystem(g_PhysicsHook, CPhysicsHook *, "CPhysicsHook");

	if(!SetResponseSystem())
		return false;

	return true;
}

static CSoundEnvelopeController *GetSoundController()
{
	// this is a little tricky, on windows we sigscan references to a func that returns the soundcontroller
#if defined(PLATFORM_WINDOWS)
	// "Invalid starting duration value in env" bellow 2, first function
	META_CONPRINTF("[%s] Getting Soundcontroller - ", g_Monster.GetLogTag());
	DWORD pGetSoundCtrl_Call;
	if(!g_pGameConf->GetMemSig("g_SoundController", (void**)&pGetSoundCtrl_Call))
	{
		META_CONPRINT("Failed\n");
		META_CONPRINTF("[%s] Couldn't find sig: g_SoundController", g_Monster.GetLogTag());
		return nullptr;
	}
	int pGetSoundCtrl_Offs;
	if(!g_pGameConf->GetOffset("g_SoundController", &pGetSoundCtrl_Offs))
	{
		META_CONPRINT("Failed\n");
		META_CONPRINTF("[%s] Couldn't find offs: g_SoundController", g_Monster.GetLogTag());
		return nullptr;
	}
	META_CONPRINTF("Success\n");

	typedef CSoundEnvelopeController* (*GetSoundCtrler_t)();
	pGetSoundCtrl_Call += pGetSoundCtrl_Offs; // point to call offset E8 >?? ?? ?? ??<
	DWORD dwJmpOffs = *(DWORD*)pGetSoundCtrl_Call;
	GetSoundCtrler_t getSoundController = (GetSoundCtrler_t)(pGetSoundCtrl_Call + 4 + dwJmpOffs);
	return getSoundController();
#else
	CSoundEnvelopeController* controller;
	if(!g_pGameConf->GetMemSig("g_SoundController", (void**)&controller))
	{
		META_CONPRINTF("[%s] Couldn't find sig: g_SoundController", g_Monster.GetLogTag());
		return nullptr;
	}
	return controller;
#endif
}

//    if (!GetPointerViaGameConf(#var, var))
#define GET_VARIABLE(var, type) \
    if ((var = (type)GetVariableViaGameConf(#var)) == nullptr) \
		return false;

void *defaultRelship=nullptr;
bool HelperFunction::Initialize()
{
	char *addr = NULL;
	int offset = 0;

	RegisterHook("GameRules_FAllowNPCs",GameRules_FAllowNPCsHook);
	RegisterHook("GameRules_ShouldCollide",GameRules_ShouldCollideHook);
	RegisterHook("GameRules_IsSpawnPointValid",GameRules_IsSpawnPointValid);
	RegisterHook("CGameMovement_OnLadder",OnLadderHook);

	g_Templates = (CUtlVector<TemplateEntityData_t *> *)GetVariableViaGameConf("g_Templates");

	my_g_pGameRules = (void**)FindGameRules();
	if (!my_g_pGameRules) {
		g_pSM->LogError(myself, "Unable finding GameRules pointer!");
		return false;
	}

	g_SoundController = GetSoundController();

	GET_VARIABLE(s_GameSystems, CUtlVector<IValveGameSystem*> *);

	if(!FindAllValveGameSystem()) {
		return false;
	}

	/*
	Script failed for %s\n
	unnamed
	*/
	GET_VARIABLE(g_AI_SensedObjectsManager, CAI_SensedObjectsManager *);

	/*
	"Player.PlasmaDamage"
	*/
	GET_VARIABLE(g_vecAttackDir, Vector *);


	// "ERROR: Attempting to give unknown ammo " above 1
	defaultRelship = GetVariableViaGameConf("m_DefaultRelationship");
	if (!defaultRelship) return false;
	CCombatCharacter::m_DefaultRelationship = (Relationship_t ***)defaultRelship;

	// "CNavArea::IncrementPlayerCount: Underfl"  retn    8 bellow 4
	int *m_lastInteraction = NULL;
	GET_VARIABLE(m_lastInteraction, int *);
	CCombatCharacter::m_lastInteraction = m_lastInteraction;

	// "Can't find decal %s\n"
	GET_VARIABLE(decalsystem, IDecalEmitterSystem *);
	
	IPredictionSystem **g_pPredictionSystems = nullptr;
	GET_VARIABLE(g_pPredictionSystems, IPredictionSystem **);
	IPredictionSystem::g_pPredictionSystems = *g_pPredictionSystems;

	te = servertools->GetTempEntsSystem();

	GET_VARIABLE(my_g_MultiDamage, CMultiDamage *);

	GET_VARIABLE(g_EntityListPool, CEMemoryPool *);

	GET_VARIABLE(g_CEventQueue, CEventQueue *);
	
	GET_VARIABLE(g_TouchTrace, trace_t *);

	GET_VARIABLE(g_PostSimulationQueue, CCallQueue *);

	GET_VARIABLE(g_pNotify, INotify *);
	g_pNotify = *(INotify**)g_pNotify;

	void *sAllocator = GetVariableViaGameConf("EventQueuePrioritizedEvent_t_s_Allocator");
	if (sAllocator)
	{
		EventQueuePrioritizedEvent_t::s_Allocator = (CUtlMemoryPool*)sAllocator;
	} else {
		return false;
	}
	
	CAIHintVector *gm_AllHints = nullptr;
	GET_VARIABLE(gm_AllHints, CAIHintVector *);
	CAI_HintManager::gm_AllHints = gm_AllHints;

	sAllocator = GetVariableViaGameConf("AI_Waypoint_t_s_Allocator");
	if (sAllocator)
	{
		AI_Waypoint_t::s_Allocator = (CUtlMemoryPool*)sAllocator;
	} else {
		return false;
	}

	GET_VARIABLE(g_AIFriendliesTalkSemaphore, CAI_TimedSemaphore *);
	GET_VARIABLE(g_AIFoesTalkSemaphore, CAI_TimedSemaphore *);

	GET_VARIABLE(g_AI_SquadManager, CAI_SquadManager *);

	typedef CUtlMap<int, CAIHintVector>* HINTS_TYPE;
	HINTS_TYPE gm_TypedHints;
	GET_VARIABLE(gm_TypedHints, HINTS_TYPE);
	my_gm_TypedHints = gm_TypedHints;

	GET_VARIABLE(g_interactionHitByPlayerThrownPhysObj, int *);

	GET_VARIABLE(g_Collisions, CCollisionEvent *);

	CE_CPathTrack::s_nCurrIterVal = (int*) GetVariableViaGameConf("s_nCurrIterVal");
	CE_CPathTrack::s_bIsIterating = (bool*) GetVariableViaGameConf("s_bIsIterating");

	// fix some stuff

	g_CGameMovement = (CGameMovement *)g_pGameMovement;

	SH_ADD_MANUALHOOK_MEMFUNC(OnLadderHook, g_CGameMovement, &g_helpfunc, &HelperFunction::OnLadder, false);

	CAmmoDef *def = GetAmmoDef();
	int grenade = def->Index("AMMO_TYPE_HEGRENADE");
	if(grenade > 0)
	{
		Ammo_t *ammo = def->GetAmmoOfIndex(grenade);
		if(ammo)
		{
			ammo->pMaxCarry = -1;
			ammo->pMaxCarryCVar = ammo_hegrenade_max;
		}
	}

	IGameSystem::HookValveSystem();

	return true;
}


bool HelperFunction::OnLadder( trace_t &trace )
{
	CPlayer *pPlayer = (CPlayer *)CEntity::Instance((CBaseEntity *)g_CGameMovement->player);
	if(pPlayer)
	{
		if(pPlayer->m_bOnLadder && pPlayer->GetGroundEntity() == NULL)
		{
			RETURN_META_VALUE(MRES_SUPERCEDE, true);
		}
	}
	RETURN_META_VALUE(MRES_IGNORED, true);
}

bool HelperFunction::GameRules_FAllowNPCs()
{
	RETURN_META_VALUE(MRES_SUPERCEDE, true);
}

void** HelperFunction::FindGameRules()
{
	if (!my_g_pGameRules)
	{
		if (sdktools)
			my_g_pGameRules = (void **) sdktools->GetGameRules();

		if (!my_g_pGameRules)
		{
#if defined(PLATFORM_WINDOWS)
			void *addr;
			int offset;
			if (!g_pGameConf->GetMemSig("CreateGameRulesObject", (void **) &addr) || !addr)
				return nullptr;

			if (!g_pGameConf->GetOffset("g_pGameRules", &offset) || !offset)
				return nullptr;
			my_g_pGameRules = *reinterpret_cast<void ***>(addr + offset);
#else
			my_g_pGameRules = (void**)GetVariableViaGameConf("g_pGameRules");
#endif

		}
	}

	return my_g_pGameRules;
}

void HelperFunction::HookGameRules()
{
	if (!my_g_pGameRules || !*my_g_pGameRules)
		return;

	SH_ADD_MANUALHOOK_MEMFUNC(GameRules_FAllowNPCsHook, *my_g_pGameRules, &g_helpfunc, &HelperFunction::GameRules_FAllowNPCs, false);
	SH_ADD_MANUALHOOK_MEMFUNC(GameRules_ShouldCollideHook, *my_g_pGameRules, &g_helpfunc, &HelperFunction::GameRules_ShouldCollide_Hook, true);
	SH_ADD_MANUALHOOK_MEMFUNC(GameRules_IsSpawnPointValid, *my_g_pGameRules, &g_helpfunc, &HelperFunction::GameRules_IsSpawnPointValid_Hook, true);
}


void HelperFunction::UnHookGameRules()
{
	if (!my_g_pGameRules || !*my_g_pGameRules)
		return;

	SH_REMOVE_MANUALHOOK_MEMFUNC(GameRules_FAllowNPCsHook, *my_g_pGameRules, &g_helpfunc, &HelperFunction::GameRules_FAllowNPCs, false);
	SH_REMOVE_MANUALHOOK_MEMFUNC(GameRules_ShouldCollideHook, *my_g_pGameRules, &g_helpfunc, &HelperFunction::GameRules_ShouldCollide_Hook, true);
	SH_REMOVE_MANUALHOOK_MEMFUNC(GameRules_IsSpawnPointValid, *my_g_pGameRules, &g_helpfunc, &HelperFunction::GameRules_IsSpawnPointValid_Hook, true);
}

bool HelperFunction::CGameRules_ShouldCollide( int collisionGroup0, int collisionGroup1 )
{
	if ( collisionGroup0 > collisionGroup1 )
	{
		// swap so that lowest is always first
		std::swap(collisionGroup0,collisionGroup1);
	}

	if ( collisionGroup0 == COLLISION_GROUP_DEBRIS && collisionGroup1 == COLLISION_GROUP_PUSHAWAY )
	{
		// let debris and multiplayer objects collide
		return true;
	}
	
	// --------------------------------------------------------------------------
	// NOTE: All of this code assumes the collision groups have been sorted!!!!
	// NOTE: Don't change their order without rewriting this code !!!
	// --------------------------------------------------------------------------

	// Don't bother if either is in a vehicle...
	if (( collisionGroup0 == COLLISION_GROUP_IN_VEHICLE ) || ( collisionGroup1 == COLLISION_GROUP_IN_VEHICLE ))
		return false;

	if ( ( collisionGroup1 == COLLISION_GROUP_DOOR_BLOCKER ) && ( collisionGroup0 != COLLISION_GROUP_NPC ) )
		return false;

	if ( ( collisionGroup0 == COLLISION_GROUP_PLAYER ) && ( collisionGroup1 == COLLISION_GROUP_PASSABLE_DOOR ) )
		return false;

	if ( collisionGroup0 == COLLISION_GROUP_DEBRIS || collisionGroup0 == COLLISION_GROUP_DEBRIS_TRIGGER )
	{
		// put exceptions here, right now this will only collide with COLLISION_GROUP_NONE
		return false;
	}

	// Dissolving guys only collide with COLLISION_GROUP_NONE
	if ( (collisionGroup0 == COLLISION_GROUP_DISSOLVING) || (collisionGroup1 == COLLISION_GROUP_DISSOLVING) )
	{
		if ( collisionGroup0 != COLLISION_GROUP_NONE )
			return false;
	}

	// doesn't collide with other members of this group
	// or debris, but that's handled above
	if ( collisionGroup0 == COLLISION_GROUP_INTERACTIVE_DEBRIS && collisionGroup1 == COLLISION_GROUP_INTERACTIVE_DEBRIS )
		return false;

	if ( collisionGroup0 == COLLISION_GROUP_BREAKABLE_GLASS && collisionGroup1 == COLLISION_GROUP_BREAKABLE_GLASS )
		return false;

	// interactive objects collide with everything except debris & interactive debris
	if ( collisionGroup1 == COLLISION_GROUP_INTERACTIVE && collisionGroup0 != COLLISION_GROUP_NONE )
		return false;

	// Projectiles hit everything but debris, weapons, + other projectiles
	if ( collisionGroup1 == COLLISION_GROUP_PROJECTILE )
	{
		if ( collisionGroup0 == COLLISION_GROUP_DEBRIS || 
			collisionGroup0 == COLLISION_GROUP_WEAPON ||
			collisionGroup0 == COLLISION_GROUP_PROJECTILE )
		{
			return false;
		}
	}

	// Don't let vehicles collide with weapons
	// Don't let players collide with weapons...
	// Don't let NPCs collide with weapons
	// Weapons are triggers, too, so they should still touch because of that
	if ( collisionGroup1 == COLLISION_GROUP_WEAPON )
	{
		if ( collisionGroup0 == COLLISION_GROUP_VEHICLE || 
			collisionGroup0 == COLLISION_GROUP_PLAYER ||
			collisionGroup0 == COLLISION_GROUP_NPC )
		{
			return false;
		}
	}

	// collision with vehicle clip entity??
	if ( collisionGroup0 == COLLISION_GROUP_VEHICLE_CLIP || collisionGroup1 == COLLISION_GROUP_VEHICLE_CLIP )
	{
		// yes then if it's a vehicle, collide, otherwise no collision
		// vehicle sorts lower than vehicle clip, so must be in 0
		if ( collisionGroup0 == COLLISION_GROUP_VEHICLE )
			return true;
		// vehicle clip against non-vehicle, no collision
		return false;
	}

	return true;
}


bool HelperFunction::CHalfLife2_ShouldCollide( int collisionGroup0, int collisionGroup1)
{
	// The smaller number is always first
	if ( collisionGroup0 > collisionGroup1 )
	{
		// swap so that lowest is always first
		int tmp = collisionGroup0;
		collisionGroup0 = collisionGroup1;
		collisionGroup1 = tmp;
	}
	
	// Prevent the player movement from colliding with spit globs (caused the player to jump on top of globs while in water)
	if ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT && collisionGroup1 == HL2COLLISION_GROUP_SPIT )
		return false;

	// HL2 treats movement and tracing against players the same, so just remap here
	if ( collisionGroup0 == COLLISION_GROUP_PLAYER_MOVEMENT )
	{
		collisionGroup0 = COLLISION_GROUP_PLAYER;
	}

	if( collisionGroup1 == COLLISION_GROUP_PLAYER_MOVEMENT )
	{
		collisionGroup1 = COLLISION_GROUP_PLAYER;
	}

	//If collisionGroup0 is not a player then NPC_ACTOR behaves just like an NPC.
	if ( collisionGroup1 == COLLISION_GROUP_NPC_ACTOR && collisionGroup0 != COLLISION_GROUP_PLAYER )
	{
		collisionGroup1 = COLLISION_GROUP_NPC;
	}

	if ( collisionGroup0 == HL2COLLISION_GROUP_COMBINE_BALL )
	{
		if ( collisionGroup1 == HL2COLLISION_GROUP_COMBINE_BALL )
			return false;
	}

	if ( collisionGroup0 == HL2COLLISION_GROUP_COMBINE_BALL && collisionGroup1 == HL2COLLISION_GROUP_COMBINE_BALL_NPC )
		return false;

	if ( ( collisionGroup0 == COLLISION_GROUP_WEAPON ) ||
		( collisionGroup0 == COLLISION_GROUP_PLAYER ) ||
		( collisionGroup0 == COLLISION_GROUP_PROJECTILE ) )
	{
		if ( collisionGroup1 == HL2COLLISION_GROUP_COMBINE_BALL )
			return false;
	}

	if ( collisionGroup0 == COLLISION_GROUP_DEBRIS )
	{
		if ( collisionGroup1 == HL2COLLISION_GROUP_COMBINE_BALL )
			return true;
	}

	if (collisionGroup0 == HL2COLLISION_GROUP_HOUNDEYE && collisionGroup1 == HL2COLLISION_GROUP_HOUNDEYE )
		return false;

	if (collisionGroup0 == HL2COLLISION_GROUP_HOMING_MISSILE && collisionGroup1 == HL2COLLISION_GROUP_HOMING_MISSILE )
		return false;

	if ( collisionGroup1 == HL2COLLISION_GROUP_CROW )
	{
		if ( collisionGroup0 == COLLISION_GROUP_PLAYER || collisionGroup0 == COLLISION_GROUP_NPC ||
			 collisionGroup0 == HL2COLLISION_GROUP_CROW )
			return false;
	}

	if ( ( collisionGroup0 == HL2COLLISION_GROUP_HEADCRAB ) && ( collisionGroup1 == HL2COLLISION_GROUP_HEADCRAB ) )
		return false;

	// striders don't collide with other striders
	if ( collisionGroup0 == HL2COLLISION_GROUP_STRIDER && collisionGroup1 == HL2COLLISION_GROUP_STRIDER )
		return false;

	// gunships don't collide with other gunships
	if ( collisionGroup0 == HL2COLLISION_GROUP_GUNSHIP && collisionGroup1 == HL2COLLISION_GROUP_GUNSHIP )
		return false;

	// weapons and NPCs don't collide
	if ( collisionGroup0 == COLLISION_GROUP_WEAPON && (collisionGroup1 >= HL2COLLISION_GROUP_FIRST_NPC && collisionGroup1 <= HL2COLLISION_GROUP_LAST_NPC ) )
		return false;

	//players don't collide against NPC Actors.
	//I could've done this up where I check if collisionGroup0 is NOT a player but I decided to just
	//do what the other checks are doing in this function for consistency sake.
	if ( collisionGroup1 == COLLISION_GROUP_NPC_ACTOR && collisionGroup0 == COLLISION_GROUP_PLAYER )
		return false;
		
	// In cases where NPCs are playing a script which causes them to interpenetrate while riding on another entity,
	// such as a train or elevator, you need to disable collisions between the actors so the mover can move them.
	if ( collisionGroup0 == COLLISION_GROUP_NPC_SCRIPTED && collisionGroup1 == COLLISION_GROUP_NPC_SCRIPTED )
		return false;

	// Spit doesn't touch other spit
	if ( collisionGroup0 == HL2COLLISION_GROUP_SPIT && collisionGroup1 == HL2COLLISION_GROUP_SPIT )
		return false;

	return CGameRules_ShouldCollide(collisionGroup0,collisionGroup1);
}

bool HelperFunction::GameRules_ShouldCollide_Hook(int collisionGroup0, int collisionGroup1)
{
	bool css_ret = META_RESULT_ORIG_RET(bool);
	if(collisionGroup0 >= LAST_SHARED_COLLISION_GROUP || collisionGroup1 >= LAST_SHARED_COLLISION_GROUP)
	{
		bool hl2_ret = CHalfLife2_ShouldCollide(collisionGroup0,collisionGroup1);
		RETURN_META_VALUE(MRES_SUPERCEDE, hl2_ret);
	} else {
		RETURN_META_VALUE(MRES_IGNORED, true);
	}
}

// replicates HL2/HL2DM spawn point validation
bool HelperFunction::GameRules_IsSpawnPointValid_Hook(CBaseEntity *point, CBaseEntity *player)
{
	CEntity *cent_point = CEntity::Instance(point);
	CEntity *cent_player = (CPlayer*)CEntity::Instance(player);

	if ( !cent_point->IsTriggered(player) )
	{
		RETURN_META_VALUE(MRES_SUPERCEDE, false);
	}

	CEntity *ent = nullptr;
	for ( CEntitySphereQuery sphere( cent_point->GetAbsOrigin(), 128 );
	      (ent = sphere.GetCurrentEntity()) != nullptr;
		  sphere.NextEntity() )
	{
		// if ent is a client, don't spawn on 'em
		if ( ent->IsPlayer() && ent != cent_player )
		{
			RETURN_META_VALUE(MRES_SUPERCEDE, false);
		}
	}

	RETURN_META_VALUE(MRES_SUPERCEDE, true);
}

/*
 * template <typename ReturnType, typename... Args>
static ALWAYSINLINE ReturnType call_virtual(void* instance, int index, Args... args)
{
	using Fn = ReturnType(THISCALLCONV *)(void*, Args...);

	auto function = (*reinterpret_cast<Fn**>(instance))[index];
	return function(instance, args...);
}
 */

bool HelperFunction::GameRules_ShouldCollide(int collisionGroup0, int collisionGroup1)
{
	static int offset = NULL;
	if(!offset)
	{
		if(!g_pGameConf->GetOffset("GameRules_ShouldCollide", &offset))
		{
			assert(0);
			return false;
		}
	}

	return call_virtual<bool>(*my_g_pGameRules, offset, collisionGroup0, collisionGroup1);
}

bool HelperFunction::GameRules_Damage_NoPhysicsForce(int iDmgType)
{
	static int offset = NULL;
	if(!offset)
	{
		if(!g_pGameConf->GetOffset("GameRules_Damage_NoPhysicsForce", &offset))
		{
			assert(0);
			return false;
		}
	}

	return call_virtual<bool>(*my_g_pGameRules, offset, iDmgType);
}

bool HelperFunction::GameRules_IsSkillLevel(int iLevel)
{
	static int offset = NULL;
	if(!offset)
	{
		if(!g_pGameConf->GetOffset("GameRules_IsSkillLevel", &offset))
		{
			assert(0);
			return false;
		}
	}

	return call_virtual<bool>(*my_g_pGameRules, offset, iLevel);
}

bool HelperFunction::GameRules_IsTeamplay()
{
	static int offset = NULL;
	if(!offset)
	{
		if(!g_pGameConf->GetOffset("GameRules_IsTeamplay", &offset))
		{
			assert(0);
			return false;
		}
	}

	return call_virtual<bool>(*my_g_pGameRules, offset);
}

void HelperFunction::GameRules_RadiusDamage(const CTakeDamageInfo &info, const Vector &vecSrc, float flRadius, int iClassIgnore, CBaseEntity *pEntityIgnore)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("CGameRules::RadiusDamage", &func))
		{
			assert(0);
			return;
		}
	}
	typedef Activity (THISCALLCONV *_func)(void*, const CTakeDamageInfo&, const Vector&, float, int, CBaseEntity*);
	_func thisfunc = (_func)func;
	thisfunc(*my_g_pGameRules, info, vecSrc, flRadius, iClassIgnore, pEntityIgnore);
}

CCombatWeapon *HelperFunction::GameRules_GetNextBestWeapon(CBaseEntity *pPlayer, CBaseEntity *pCurrentWeapon)
{
	static int offset = NULL;
	if(!offset)
	{
		if(!g_pGameConf->GetOffset("GameRules_GetNextBestWeapon", &offset))
		{
			assert(0);
			return NULL;
		}
	}

	CBaseEntity *ret = call_virtual<CBaseEntity*>(*my_g_pGameRules, offset, pPlayer, pCurrentWeapon);
	return (CCombatWeapon*)CEntity::Instance(ret);
}

bool HelperFunction::GameRules_FPlayerCanTakeDamage(CBaseEntity *pPlayer, CBaseEntity *pAttacker, const CTakeDamageInfo &info)
{
	static int offset = NULL;
	if(!offset)
	{
		if(!g_pGameConf->GetOffset("GameRules_FPlayerCanTakeDamage", &offset))
		{
			assert(0);
			return NULL;
		}
	}

	return call_virtual<bool>(*my_g_pGameRules, offset, pPlayer, pAttacker, info);
}

void HelperFunction::GameRules_EndMultiplayerGame()
{
	static int offset = NULL;
	if(!offset)
	{
		if(!g_pGameConf->GetOffset("GameRules_EndMultiplayerGame", &offset))
		{
			assert(0);
			return;
		}
	}

	call_virtual<void>(*my_g_pGameRules, offset);
}

int HelperFunction::GameRules_GetAutoAimMode()
{
	// we use the cvar to avoid doing a sigscan..
	return sk_autoaim_mode->GetInt();
}

CViewVectors *HelperFunction::GameRules_GetViewVectors()
{
	static int offset = NULL;
	if(!offset)
	{
		if(!g_pGameConf->GetOffset("GameRules_GetViewVectors", &offset))
		{
			assert(0);
			return nullptr;
		}
	}

	return call_virtual<CViewVectors*>(*my_g_pGameRules, offset);
}

Activity HelperFunction::ActivityList_RegisterPrivateActivity( const char *pszActivityName )
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("ActivityList_RegisterPrivateActivity", &func))
		{
			assert(0);
			return ACT_INVALID;
		}
	}

	typedef Activity (*_func)(char const *);
	_func thisfunc = (_func)func;
    return thisfunc(pszActivityName);
}

Animevent HelperFunction::EventList_RegisterPrivateEvent( const char *pszEventName )
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("EventList_RegisterPrivateEvent", &func))
		{
			assert(0);
			return AE_INVALID;
		}
	}

	typedef Animevent (*_func)(char const *);
    _func thisfunc = (_func)func;
    return thisfunc(pszEventName);
}

CBaseEntity *HelperFunction::NPCPhysics_CreateSolver(CBaseEntity *pNPC, CBaseEntity *c, bool disableCollisions, float separationDuration)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("NPCPhysics_CreateSolver", &func))
		{
			assert(0);
			return NULL;
		}
	}

	CBaseEntity *pEntity = NULL;
	typedef CBaseEntity* (*_func)(CBaseEntity *, CBaseEntity *, bool, float);
    _func thisfunc = (_func)func;
    pEntity = (CBaseEntity*)thisfunc(pNPC, pNPC, disableCollisions, separationDuration);
	return pEntity;
}

HSOUNDSCRIPTHANDLE HelperFunction::PrecacheScriptSound(const char *soundname)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("PrecacheScriptSound", &func))
		{
			assert(0);
			return SOUNDEMITTER_INVALID_HANDLE;
		}
	}
#ifdef PLATFORM_WINDOWS
	typedef HSOUNDSCRIPTHANDLE (__thiscall *_func)(void *, const char *);
	_func thisfunc = (_func)func;
    return thisfunc(g_SoundEmitterSystem, soundname);
#else
	typedef HSOUNDSCRIPTHANDLE (*_func)(const char *);
	_func thisfunc = (_func)func;
	return thisfunc(soundname);
#endif
}

void HelperFunction::EmitSoundByHandle( IRecipientFilter& filter, int entindex, const EmitSound_t & ep, HSOUNDSCRIPTHANDLE& handle )
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("EmitSoundByHandle", &func))
		{
			assert(0);
			return;
		}
	}
#ifdef PLATFORM_WINDOWS
	typedef void (THISCALLCONV *_func)(void *, IRecipientFilter& , int , const EmitSound_t & , HSOUNDSCRIPTHANDLE& );
    _func thisfunc = (_func)func;
    thisfunc(g_SoundEmitterSystem, filter, entindex, ep, handle);
#else
	typedef void (*_func)(IRecipientFilter& , int , const EmitSound_t & , HSOUNDSCRIPTHANDLE& );
	_func thisfunc = (_func)func;
	thisfunc(filter, entindex, ep, handle);
#endif
}

void HelperFunction::PhysicsImpactSound(CBaseEntity *pEntity, IPhysicsObject *pPhysObject, int channel, int surfaceProps, int surfacePropsHit, float volume, float impactSpeed)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("PhysicsImpactSound", &func))
		{
			assert(0);
			return;
		}
	}

	typedef void (*_func)(CBaseEntity *, IPhysicsObject *, int , int , int , float, float);
    _func thisfunc = (_func)func;
    thisfunc(pEntity, pPhysObject, channel, surfaceProps, surfacePropsHit, volume, impactSpeed);
}

CEntity *HelperFunction::CreateRagGib( const char *szModel, const Vector &vecOrigin, const QAngle &vecAngles, const Vector &vecForce, float flFadeTime, bool bShouldIgnite )
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("CreateRagGib", &func))
		{
			assert(0);
			return NULL;
		}
	}

	typedef CBaseEntity* (*_func)(const char *, const Vector &, const QAngle &, const Vector &, float , bool );
    _func thisfunc = (_func)func;
    CBaseEntity *cbase = thisfunc(szModel, vecOrigin,  vecAngles, vecForce, flFadeTime, bShouldIgnite);

	return CEntity::Instance(cbase);
}

void HelperFunction::VerifySequenceIndex(void *ptr)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("VerifySequenceIndex", &func))
		{
			assert(0);
			return;
		}
	}

	typedef void (*_func)(void *);
    _func thisfunc = (_func)func;
    thisfunc(ptr);
}

void HelperFunction::DispatchEffect( const char *pName, const CEffectData &data )
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("DispatchEffect", &func))
		{
			assert(0);
			return;
		}
	}

	typedef void (*_func)(const char *, const CEffectData &);
    _func thisfunc = (_func)func;
    thisfunc(pName, data);
}

CAmmoDef *HelperFunction::GetAmmoDef()
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("GetAmmoDef", &func))
		{
			assert(0);
			return NULL;
		}
	}

	typedef CAmmoDef *(*_func)();
    _func thisfunc = (_func)func;
    return thisfunc();
}

CEntity *HelperFunction::CreateNoSpawn( const char *szName, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("CreateNoSpawn", &func))
		{
			assert(0);
			return NULL;
		}
	}

	typedef CBaseEntity *(*_func)(const char *, const Vector &, const QAngle &, CBaseEntity *);
    _func thisfunc = (_func)func;
    CBaseEntity *cbase = thisfunc(szName, vecOrigin, vecAngles, pOwner);
	return CEntity::Instance(cbase);
}

void HelperFunction::SetMinMaxSize(CBaseEntity *pEnt, const Vector &vecMin, const Vector &vecMax)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("SetMinMaxSize", &func))
		{
			assert(0);
			return;
		}
	}

	typedef void (*_func)(CBaseEntity *pEnt, const Vector &vecMin, const Vector &vecMax);
    _func thisfunc = (_func)func;
    thisfunc(pEnt, vecMin, vecMax);
}

float HelperFunction::CalculateDefaultPhysicsDamage( int index, gamevcollisionevent_t *pEvent, float energyScale, bool allowStaticDamage, int &damageType, string_t iszDamageTableName, bool bDamageFromHeldObjects )
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("CalculateDefaultPhysicsDamage", &func))
		{
			assert(0);
			return 0.0f;
		}
	}

	typedef float (*_func)(int , gamevcollisionevent_t *, float , bool , int &, string_t , bool );
    _func thisfunc = (_func)func;
    return thisfunc(index, pEvent, energyScale, allowStaticDamage, damageType, iszDamageTableName, bDamageFromHeldObjects );
}

void HelperFunction::PhysCallbackDamage( CBaseEntity *pEntity, const CTakeDamageInfo &info, gamevcollisionevent_t &event, int hurtIndex )
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("PhysCallbackDamage", &func))
		{
			assert(0);
			return;
		}
	}

	typedef void (*_func)(CBaseEntity *, const CTakeDamageInfo &, gamevcollisionevent_t &, int );
    _func thisfunc = (_func)func;
    thisfunc(pEntity, info, event, hurtIndex);
}

void HelperFunction::PropBreakableCreateAll( int modelindex, IPhysicsObject *pPhysics, const breakablepropparams_t &params, CBaseEntity *pEntity, int iPrecomputedBreakableCount, bool bIgnoreGibLimit, bool defaultLocation )
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("PropBreakableCreateAll", &func))
		{
			assert(0);
			return;
		}
	}

	typedef void (*_func)(int , IPhysicsObject *, const breakablepropparams_t &, CBaseEntity *, int , bool , bool );
	_func thisfunc = (_func)func;
    thisfunc(modelindex, pPhysics, params, pEntity, iPrecomputedBreakableCount, bIgnoreGibLimit, defaultLocation );
}

int HelperFunction::PropBreakablePrecacheAll( string_t modelName )
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("PropBreakablePrecacheAll", &func))
		{
			assert(0);
			return 0;
		}
	}

	typedef int (*_func)(string_t);
	_func thisfunc = (_func)func;
    return thisfunc(modelName);
}

void HelperFunction::UTIL_Remove(IServerNetworkable *oldObj)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("UTIL_Remove", &func))
		{
			return;
		}
	}

	if(!oldObj)
		return;

	typedef void (*_func)(IServerNetworkable *);
    _func thisfunc = (_func)(func);
    (thisfunc)(oldObj);
}


CEntity *HelperFunction::CAI_HintManager_FindHint(CBaseEntity *pNPC, const Vector &position, const CHintCriteria &hintCriteria )
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("CAI_HintManager::FindHint", &func))
		{
			assert(0);
			return NULL;
		}
	}

	// static func (https://github.com/ValveSoftware/source-sdk-2013/blob/master/sp/src/game/server/ai_hint.h#L222)
	typedef CBaseEntity* (*_func)(CBaseEntity *, const Vector &, const CHintCriteria &);
    _func thisfunc = (_func)func;
    CBaseEntity *cbase = thisfunc(pNPC, position, hintCriteria);

	return CEntity::Instance(cbase);
}

CEntity *HelperFunction::CAI_HintManager_FindHintRandom(CBaseEntity *pNPC, const Vector &position, const CHintCriteria &hintCriteria )
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("CAI_HintManager::FindHintRandom", &func))
		{
			assert(0);
			return NULL;
		}
	}

	// static
	typedef CBaseEntity* (*_func)(CBaseEntity *, const Vector &, const CHintCriteria &);
    _func thisfunc = (_func)func;
    CBaseEntity *cbase = thisfunc(pNPC, position, hintCriteria);

	return CEntity::Instance(cbase);
}


int HelperFunction::SelectWeightedSequence(void *pstudiohdr, int activity, int curSequence)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("SelectWeightedSequence", &func))
		{
			assert(0);
			return 0;
		}
	}

	CBaseEntity *pEntity = NULL;
	typedef int (*_func)(void *, int, int);
    _func thisfunc = (_func)func;
    return thisfunc(pstudiohdr, activity, curSequence);	

}

void HelperFunction::SetNextThink(CBaseEntity *pEntity, float thinkTime, const char *szContext)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("SetNextThink", &func))
		{
			assert(0);
			return;
		}
	}

	if(!pEntity)
		return;

	typedef void (THISCALLCONV *_func)(CBaseEntity *, float, const char *);
    _func thisfunc = (_func)(func);

	(thisfunc)(pEntity,thinkTime,szContext);
}

CSimThinkManager *HelperFunction::GetSimThinkMngr()
{
#if defined(PLATFORM_WINDOWS)
	static int offs = -1;
	if(offs == -1)
	{
		if(!g_pGameConf->GetOffset("g_SimThinkManagerOffs", &offs))
		{
			assert(0);
			return NULL;
		}
	}
#endif

	static unsigned char *addr = NULL;
	if(!addr)
	{
		if(!g_pGameConf->GetMemSig("g_SimThinkManager", (void**)&addr))
		{
			assert(0);
			return NULL;
		}
	}

#if defined(PLATFORM_WINDOWS)
	addr += offs;
#endif

	return *reinterpret_cast<CSimThinkManager**>(addr);
}

void HelperFunction::SimThink_EntityChanged(CBaseEntity *pEntity)
{
#if defined(PLATFORM_WINDOWS)
	typedef void (*func_t)(CBaseEntity *pEntity);
	static func_t func = NULL;
	if(!func)
	{
		DWORD call_ptr;
		if(!g_pGameConf->GetMemSig("SimThink_EntityChanged_cdecl", (void**)&call_ptr))
		{
			assert(0);
			return;
		}
		int call_offs = -1;
		if(!g_pGameConf->GetOffset("SimThink_CallOffs", &call_offs))
		{
			assert(0);
			return;
		}

		call_ptr += call_offs;
		DWORD dwJmpOffs = *(DWORD*)call_ptr;
		func = (func_t)(call_ptr + 4 + dwJmpOffs);
	}

	if(!pEntity)
		return;

	func(pEntity);
#else
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("SimThink_EntityChanged_cdecl", &func))
		{
			assert(0);
			return;
		}
	}

	if(!pEntity)
		return;

	typedef void (__cdecl *_func)(CBaseEntity *);
	_func thisfunc = (_func)(func);
	(thisfunc)(pEntity);
#endif
}

void HelperFunction::SetGroundEntity(CBaseEntity *pEntity, CBaseEntity *ground)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("SetGroundEntity", &func))
		{
			assert(0);
			return;
		}
	}

	if(!pEntity)
		return;

	typedef void (THISCALLCONV *_func)(CBaseEntity *, CBaseEntity *);
    _func thisfunc = (_func)(func);

	(thisfunc)(pEntity,ground);
}

void HelperFunction::SetAbsVelocity(CBaseEntity *pEntity, const Vector &vecAbsVelocity)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("SetAbsVelocity", &func))
		{
			assert(0);
			return;
		}
	}

	if(!pEntity)
		return;

	typedef void (THISCALLCONV *_func)(CBaseEntity *, const Vector &);
    _func thisfunc = (_func)(func);

	(thisfunc)(pEntity,vecAbsVelocity);
}

void HelperFunction::SetAbsAngles(CBaseEntity *pEntity, const QAngle& absAngles)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("SetAbsAngles", &func))
		{
			assert(0);
			return;
		}
	}

	if(!pEntity)
		return;

	typedef void (THISCALLCONV *_func)(void *, const QAngle &);
    _func thisfunc = (_func)(func);

	(thisfunc)(pEntity,absAngles);
}

IServerVehicle *HelperFunction::GetServerVehicle(CBaseEntity *pEntity)
{
	static int offset = 0;
	if(!offset)
	{
		if(!g_pGameConf->GetOffset("GetServerVehicle", &offset))
		{
			assert(0);
			return NULL;
		}
	}

	if(!pEntity)
		return NULL;

	return call_virtual<IServerVehicle*>(pEntity, offset);
}

IServerVehicle *HelperFunction::GetVehicle(CBaseEntity *pEntity)
{
	static int offset = 0;
	if(!offset)
	{
		if(!g_pGameConf->GetOffset("GetVehicle", &offset))
		{
			assert(0);
			return NULL;
		}
	}

	if(!pEntity)
		return NULL;

	return call_virtual<IServerVehicle*>(pEntity, offset);
}

void HelperFunction::EmitSound(CBaseEntity *pEntity, const char *soundname, float soundtime, float *duration)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("EmitSound_char_float_pfloat", &func))
		{
			assert(0);
			return;
		}
	}

	if(!pEntity)
		return;

	typedef void (THISCALLCONV *_func)(void *, const char *, float, float *);
	_func thisfunc = (_func)(func);
	(thisfunc)(pEntity,soundname, soundtime, duration);
}

void HelperFunction::EmitSound(IRecipientFilter& filter, int iEntIndex, const EmitSound_t & params)
{
#ifdef OLD_SOUND
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("EmitSound_filter_int_struct", &func))
		{
			assert(0);
			return;
		}
	}
	typedef void (*_func)(IRecipientFilter& , int , const EmitSound_t &);
	_func thisfunc = (_func)(func);
	(thisfunc)(filter, iEntIndex, params);
#else
	int specialDSP = 0;
	Vector *direction = nullptr;
	bool updatePos = true;
	engsound->EmitSound(filter, iEntIndex, params.m_nChannel, params.m_pSoundName,
						params.m_flVolume, params.m_SoundLevel, params.m_nFlags,
						params.m_nPitch, specialDSP, params.m_pOrigin, direction,
						&params.m_UtlVecSoundOrigin, updatePos,
						params.m_flSoundTime, params.m_nSpeakerEntity);
#endif
}

void HelperFunction::EmitSound(IRecipientFilter& filter, int iEntIndex, const char *soundname, const Vector *pOrigin /*= NULL*/, float soundtime /*= 0.0f*/, float *duration /*=NULL*/)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("EmitSound_filter_int_char_vector_float_pfloat", &func))
		{
			assert(0);
			return;
		}
	}

	typedef void (*_func)(IRecipientFilter& , int , const char *, const Vector * , float , float *);
	_func thisfunc = (_func)(func);
	(thisfunc)(filter, iEntIndex, soundname, pOrigin, soundtime, duration);
}

void HelperFunction::RemoveDeferred(CBaseEntity *pEntity)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("RemoveDeferred", &func))
		{
			assert(0);
			return;
		}
	}

	if(!pEntity)
		return;

	typedef void (THISCALLCONV *_func)(void *);
	_func thisfunc = (_func)(func);
	(thisfunc)(pEntity);
}

void HelperFunction::CBaseEntity_Use(CBaseEntity *pEntity, CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("CBaseEntity::Use", &func))
		{
			assert(0);
			return;
		}
	}

	if(!pEntity)
		return;

	typedef void (THISCALLCONV *_func)(void *, CBaseEntity *, CBaseEntity *, USE_TYPE , float );
	_func thisfunc = (_func)(func);
	(thisfunc)(pEntity, pActivator, pCaller, useType, value);
}

bool HelperFunction::CBaseEntity_FVisible_Entity(CBaseEntity *the_pEntity, CBaseEntity *pEntity, int traceMask, CBaseEntity **ppBlocker)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("CBaseEntity::FVisible_Entity", &func))
		{
			assert(0);
			return false;
		}
	}

	if(!the_pEntity)
		return false;

	typedef bool (THISCALLCONV *_func)(void *, CBaseEntity *, int , CBaseEntity **);
	_func thisfunc = (_func)(func);
	return (thisfunc)(the_pEntity, pEntity, traceMask, ppBlocker);
}

void HelperFunction::CalcAbsolutePosition(CBaseEntity *pEntity)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("CalcAbsolutePosition", &func))
		{
			assert(0);
			return;
		}
	}

	if(!pEntity)
		return;

	typedef void (THISCALLCONV *_func)(void *);
	_func thisfunc = (_func)(func);
	(thisfunc)(pEntity);
}

void HelperFunction::PhysicsMarkEntitiesAsTouching( CBaseEntity *pEntity, CBaseEntity *other, trace_t &trace )
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("PhysicsMarkEntitiesAsTouching", &func))
		{
			assert(0);
			return;
		}
	}

	if(!pEntity)
		return;

	typedef void (THISCALLCONV *_func)(void *, CBaseEntity *, trace_t &);
	_func thisfunc = (_func)(func);
	(thisfunc)(pEntity, other, trace);
}

void *HelperFunction::GetDataObject( CBaseEntity *pEntity, int type )
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("GetDataObject", &func))
		{
			assert(0);
			return NULL;
		}
	}

	if(!pEntity)
		return NULL;

	typedef void *(THISCALLCONV *_func)(void *, int);
	_func thisfunc = (_func)(func);
	return (thisfunc)(pEntity, type);
}

void HelperFunction::SetMoveType( CBaseEntity *pEntity, MoveType_t val, MoveCollide_t moveCollide )
{
	servertools->SetMoveType(pEntity, val, moveCollide);
#if 0
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("SetMoveType", &func))
		{
			assert(0);
			return;
		}
	}

	if(!pEntity)
		return;

	typedef void (THISCALLCONV *_func)(void *, MoveType_t, MoveCollide_t);
	_func thisfunc = (_func)(func);
	(thisfunc)(pEntity, val,moveCollide);
#endif
}

void HelperFunction::CheckHasGamePhysicsSimulation(CBaseEntity *pEntity)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("CheckHasGamePhysicsSimulation", &func))
		{
			assert(0);
			return;
		}
	}

	if(!pEntity)
		return;

	typedef void (THISCALLCONV *_func)(void *);
	_func thisfunc = (_func)(func);
	(thisfunc)(pEntity);
}

void HelperFunction::InvalidatePhysicsRecursive( CBaseEntity *pEntity, int nChangeFlags )
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("InvalidatePhysicsRecursive", &func))
		{
			assert(0);
			return;
		}
	}

	if(!pEntity)
		return;

	typedef void (THISCALLCONV *_func)(CBaseEntity *,int, int);
	_func thisfunc = (_func)(func);
	(thisfunc)(pEntity,0, nChangeFlags);
}

void HelperFunction::PhysicsPushEntity( CBaseEntity *pEntity, const Vector& push, trace_t *pTrace )
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("PhysicsPushEntity", &func))
		{
			assert(0);
			return;
		}
	}

	if(!pEntity)
		return;

	typedef void (THISCALLCONV *_func)(CBaseEntity *,int,  const Vector&, trace_t *);
	_func thisfunc = (_func)(func);
	(thisfunc)(pEntity,0, push, pTrace);
}

CEntity *HelperFunction::CreateServerRagdoll( CBaseEntity *pAnimating, int forceBone, const CTakeDamageInfo &info, int collisionGroup, bool bUseLRURetirement )
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("CreateServerRagdoll", &func))
		{
			assert(0);
			return NULL;
		}
	}

	if(!pAnimating)
		return NULL;

	typedef CBaseEntity *(*_func)(CBaseEntity *, int , const CTakeDamageInfo &, int , bool );
	_func thisfunc = (_func)(func);
	CBaseEntity *cbase = (thisfunc)(pAnimating, forceBone, info, collisionGroup, bUseLRURetirement);
	return CEntity::Instance(cbase);
}

ragdoll_t *HelperFunction::Ragdoll_GetRagdoll( CBaseEntity *pent )
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("Ragdoll_GetRagdoll", &func))
		{
			assert(0);
			return NULL;
		}
	}

	if(!pent)
		return NULL;

	typedef ragdoll_t *(*_func)(CBaseEntity *);
	_func thisfunc = (_func)(func);
	return (thisfunc)(pent);
}

void HelperFunction::DetachAttachedRagdollsForEntity( CBaseEntity *pent )
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("DetachAttachedRagdollsForEntity", &func))
		{
			assert(0);
			return;
		}
	}

	typedef void (THISCALLCONV *_func)(CBaseEntity*);
	_func thisfunc = (_func)(func);
	(thisfunc)(pent);
}

void HelperFunction::CSoundEnt_InsertSound( int iType, const Vector &vecOrigin, int iVolume, float flDuration, CBaseEntity *pOwner, int soundChannelIndex, CBaseEntity *pSoundTarget )
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("CSoundEnt::InsertSound", &func))
		{
			assert(0);
			return;
		}
	}

	typedef void (*_func)(int , const Vector &, int , float , CBaseEntity *, int , CBaseEntity * );
	_func thisfunc = (_func)(func);
	(thisfunc)(iType, vecOrigin, iVolume, flDuration, pOwner, soundChannelIndex, pSoundTarget);
}

int	HelperFunction::PrecacheModel( const char *name )
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("PrecacheModel", &func))
		{
			assert(0);
			return 0;
		}
	}

	typedef int (*_func)(const char *);
	_func thisfunc = (_func)(func);
	return (thisfunc)(name);
}

const char *HelperFunction::MapEntity_ParseEntity(CEntity *&pEntity, const char *pEntData, IMapEntityFilter *pFilter)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("MapEntity_ParseEntity", &func))
		{
			assert(0);
			return NULL;
		}
	}

	CBaseEntity *cbase = NULL;
	typedef const char *(*_func)(CBaseEntity *&, const char *, IMapEntityFilter *);
	_func thisfunc = (_func)(func);
	const char * ret = (thisfunc)(cbase, pEntData, pFilter);
	pEntity = CEntity::Instance(cbase);
	return ret;
}

void HelperFunction::UTIL_PrecacheOther( const char *szClassname, const char *modelName )
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("UTIL_PrecacheOther", &func))
		{
			assert(0);
			return;
		}
	}

	typedef void (*_func)(const char *, const char *);
	_func thisfunc = (_func)(func);
	(thisfunc)(szClassname, modelName);
}

void HelperFunction::UTIL_RemoveImmediate( CBaseEntity *oldObj )
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("UTIL_RemoveImmediate", &func))
		{
			assert(0);
			return;
		}
	}

	typedef void (*_func)(CBaseEntity *);
	_func thisfunc = (_func)(func);
	(thisfunc)(oldObj);
}

void HelperFunction::SetEventIndexForSequence( mstudioseqdesc_t &seqdesc )
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("SetEventIndexForSequence", &func))
		{
			assert(0);
			return;
		}
	}

	typedef void (*_func)(mstudioseqdesc_t & );
    _func thisfunc = (_func)func;
    thisfunc(seqdesc);
}

void HelperFunction::SetActivityForSequence( CStudioHdr *pstudiohdr, int seq )
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("SetActivityForSequence", &func))
		{
			assert(0);
			return;
		}
	}

	typedef void (*_func)(CStudioHdr *, int );
	_func thisfunc = (_func)func;
	thisfunc(pstudiohdr, seq);
}

int HelperFunction::ActivityList_IndexForName( const char *pszActivityName )
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("ActivityList_IndexForName", &func))
		{
			assert(0);
			return -1;
		}
	}

	typedef int (*_func)(const char *);
	_func thisfunc = (_func)func;
	return thisfunc(pszActivityName);
}

string_t HelperFunction::AllocPooledString( const char * pszValue )
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("AllocPooledString", &func))
		{
			assert(0);
			return NULL_STRING;
		}
	}

	typedef string_t (*_func)(const char * );
    _func thisfunc = (_func)func;
    return thisfunc(pszValue);
}

string_t HelperFunction::FindPooledString( const char * pszValue )
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("FindPooledString", &func))
		{
			assert(0);
			return NULL_STRING;
		}
	}

	typedef string_t (*_func)(const char * );
    _func thisfunc = (_func)func;
    return thisfunc(pszValue);
}

const char *HelperFunction::ActivityList_NameForIndex( int activityIndex )
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("ActivityList_NameForIndex", &func))
		{
			assert(0);
			return NULL;
		}
	}

	typedef const char *(*_func)(int );
    _func thisfunc = (_func)func;
    return thisfunc(activityIndex);
}

void HelperFunction::CGib_Spawn( CBaseEntity *gib, const char *mdl ) // UNUSED
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("Spawn_pchar", &func))
		{
			assert(0);
			return;
		}
	}

	typedef void (THISCALLCONV *_func)(CBaseEntity *, const char *);
    _func thisfunc = (_func)func;
    thisfunc(gib, mdl);
}

CBaseEntity *HelperFunction::UTIL_FindClientInPVS_VecVec( const Vector &vecBoxMins, const Vector &vecBoxMaxs )
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("UTIL_FindClientInPVS_VecVec", &func))
		{
			assert(0);
			return NULL;
		}
	}

	typedef CBaseEntity* (*_func)(const Vector &, const Vector &);
    _func fn = (_func)func;
    return fn(vecBoxMins, vecBoxMaxs);
}

CBaseEntity *HelperFunction::CreateServerRagdollAttached( CBaseAnimating *pAnimating, const Vector &vecForce,
	int forceBone, int collisionGroup, IPhysicsObject *pAttached,
	CBaseAnimating *pParentEntity, int boneAttach,
	const Vector &originAttached, int parentBoneAttach, const Vector &boneOrigin )
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("CreateServerRagdollAttached", &func))
		{
			assert(0);
			return NULL;
		}
	}

	typedef CBaseEntity* (*_func)(CBaseAnimating*, const Vector&, int, int, IPhysicsObject*, CBaseAnimating*, int, const Vector&, int, const Vector&);
    _func fn = (_func)func;
    return fn(pAnimating, vecForce, forceBone, collisionGroup, pAttached, pParentEntity, boneAttach, originAttached, parentBoneAttach, boneOrigin);
}

void HelperFunction::PrecacheInstancedScene(const char *pszScene)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("PrecacheInstancedScene", &func))
		{
			assert(0);
			return;
		}
	}

	typedef void (*_func)(const char * );
    _func fn = (_func)func;
    fn(pszScene);
}

void HelperFunction::SetSolid(void *collision_ptr, SolidType_t val)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("SetSolid", &func))
		{
			assert(0);
			return;
		}
	}

	if(!collision_ptr)
		return;

	typedef void (THISCALLCONV *_func)(void *, int);
	_func thisfunc = (_func)(func);
	(thisfunc)(collision_ptr,val);
}


void HelperFunction::SetSolidFlags(void *collision_ptr, int flags)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("SetSolidFlags", &func))
		{
			assert(0);
			return;
		}
	}

	if(!collision_ptr)
		return;

	typedef void (THISCALLCONV *_func)(void *, int);
	_func thisfunc = (_func)(func);
	(thisfunc)(collision_ptr,flags);
}

void HelperFunction::MarkPartitionHandleDirty(void *collision_ptr)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("MarkPartitionHandleDirty", &func))
		{
			assert(0);
			return;
		}
	}

	if(!collision_ptr)
		return;

	typedef void (THISCALLCONV *_func)(void *);
	_func thisfunc = (_func)(func);
	(thisfunc)(collision_ptr);
}

void HelperFunction::ReportEntityFlagsChanged(CBaseEntity *pEntity, unsigned int flagsOld, unsigned int flagsNow)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("ReportEntityFlagsChanged", &func))
		{
			assert(0);
			return;
		}
	}

	if(!pEntity)
		return;

	typedef void (THISCALLCONV *_func)(void *, CBaseEntity *, unsigned int, unsigned int);
	_func thisfunc = (_func)(func);
	(thisfunc)(g_pEntityList, pEntity, flagsOld, flagsNow);
}

CEntity *HelperFunction::FindEntityByClassname(CEntity *pStartEntity, const char *szName)
{
	return FindEntityByClassname((pStartEntity)?pStartEntity->BaseEntity():(CBaseEntity *)NULL, szName);
}

CEntity *HelperFunction::FindEntityByClassname(CBaseEntity *pStartEntity, const char *szName)
{
#if 0
	if(!pStartEntity) {
		pStartEntity = servertools->FirstEntity();
	}

	while(pStartEntity) {
		if(V_strcmp(pStartEntity->GetNetworkable()->GetClassNameA(), szName) == 0)
		{
			return CEntity::Instance(pStartEntity);
		}

		pStartEntity = servertools->NextEntity(pStartEntity);
	}
	return NULL;
#endif

#if 0
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("FindEntityByClassname", &func))
		{
			assert(0);
			return NULL;
		}
	}

	typedef CBaseEntity *(THISCALLCONV *_func)(void *, CBaseEntity *, const char *);
	_func thisfunc = (_func)(func);
	CBaseEntity *cbase = (thisfunc)(g_pEntityList,pStartEntity, szName);
#endif
	CBaseEntity *base = servertools->FindEntityByClassname(pStartEntity, szName);
	return CEntity::Instance(base);
}

CEntity *HelperFunction::FindEntityByName( CEntity *pStartEntity, const char *szName, CBaseEntity *pSearchingEntity, CBaseEntity *pActivator, CBaseEntity *pCaller, IEntityFindFilter *pFilter)
{
	return FindEntityByName((pStartEntity)?pStartEntity->BaseEntity():(CBaseEntity *)NULL, szName, pSearchingEntity, pActivator, pCaller, pFilter );
}

CEntity *HelperFunction::FindEntityByName( CEntity *pStartEntity, string_t szName, CBaseEntity *pSearchingEntity, CBaseEntity *pActivator, CBaseEntity *pCaller, IEntityFindFilter *pFilter)
{
	return FindEntityByName( (pStartEntity)?pStartEntity->BaseEntity():(CBaseEntity *)NULL, STRING(szName), pSearchingEntity, pActivator, pCaller, pFilter);
}


CEntity *HelperFunction::FindEntityByName(CBaseEntity *pStartEntity, const char *szName, CBaseEntity *pSearchingEntity, CBaseEntity *pActivator, CBaseEntity *pCaller, IEntityFindFilter *pFilter )
{
	CBaseEntity *entity = servertools->FindEntityByName(pStartEntity, szName, pSearchingEntity, pActivator);
	return CEntity::Instance(entity);
}

CEntity *HelperFunction::FindEntityInSphere( CEntity *pStartEntity, const Vector &vecCenter, float flRadius )
{
	return FindEntityInSphere((pStartEntity)?pStartEntity->BaseEntity():(CBaseEntity *)NULL, vecCenter, flRadius);
}

CEntity *HelperFunction::FindEntityInSphere( CBaseEntity *pStartEntity, const Vector &vecCenter, float flRadius )
{
	CBaseEntity *entity = servertools->FindEntityInSphere(pStartEntity, vecCenter, flRadius);
	return CEntity::Instance(entity);
}

CEntity *HelperFunction::FindEntityGeneric( CBaseEntity *pStartEntity, const char *szName, CBaseEntity *pSearchingEntity, CBaseEntity *pActivator, CBaseEntity *pCaller )
{
	CBaseEntity *entity = servertools->FindEntityGeneric(pStartEntity, szName, pSearchingEntity, pActivator, pCaller);
	return CEntity::Instance(entity);
}

CEntity *HelperFunction::FindEntityGenericNearest( const char *szName, const Vector &vecSrc, float flRadius, CBaseEntity *pSearchingEntity, CBaseEntity *pActivator, CBaseEntity *pCaller)
{
	CBaseEntity *entity = servertools->FindEntityGenericNearest(szName, vecSrc, flRadius, pSearchingEntity, pActivator, pCaller);
	return CEntity::Instance(entity);
}

CBaseEntity *HelperFunction::NextEnt(CBaseEntity *pCurrentEnt)
{
	return servertools->NextEntity(pCurrentEnt);
}

CBaseEntity *HelperFunction::FirstEnt()
{
	return servertools->FirstEntity();
}

void HelperFunction::AddListenerEntity( IEntityListener *pListener )
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("AddListenerEntity", &func))
		{
			assert(0);
			return;
		}
	}

	typedef void (THISCALLCONV *_func)(void *, IEntityListener *);
	_func thisfunc = (_func)(func);
	return (thisfunc)(g_pEntityList, pListener);
}

void HelperFunction::RemoveListenerEntity( IEntityListener *pListener )
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("RemoveListenerEntity", &func))
		{
			assert(0);
			return;
		}
	}

	typedef void (THISCALLCONV *_func)(void *, IEntityListener *);
	_func thisfunc = (_func)(func);
	return (thisfunc)(g_pEntityList, pListener);
}

int HelperFunction::DispatchUpdateTransmitState(CBaseEntity *pEntity)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("DispatchUpdateTransmitState", &func))
		{
			assert(0);
			return 0;
		}
	}

	if(!pEntity)
		return 0;

	typedef int (THISCALLCONV *_func)(void *);
	_func thisfunc = (_func)(func);
	return (thisfunc)(pEntity);
}


void HelperFunction::CAI_BaseNPC_Precache(CBaseEntity *pEntity)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("CAI_BaseNPC::Precache", &func))
		{
			assert(0);
			return;
		}
	}

	if(!pEntity)
		return;

	typedef void (THISCALLCONV *_func)(void *);
	_func thisfunc = (_func)(func);
	(thisfunc)(pEntity);
}


bool HelperFunction::AutoMovement(CBaseEntity *pEntity, float flInterval, CBaseEntity *pTarget, AIMoveTrace_t *pTraceResult)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("AutoMovement", &func))
		{
			assert(0);
			return false;
		}
	}

	if(!pEntity)
		return false;

	typedef bool (THISCALLCONV *_func)(void *, float , CBaseEntity *, AIMoveTrace_t *);
	_func thisfunc = (_func)(func);
	return (thisfunc)(pEntity, flInterval, pTarget, pTraceResult);
}


void HelperFunction::EndTaskOverlay(CBaseEntity *pEntity)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("EndTaskOverlay", &func))
		{
			assert(0);
			return;
		}
	}

	if(!pEntity)
		return;

	typedef void (THISCALLCONV *_func)(void *);
	_func thisfunc = (_func)(func);
	(thisfunc)(pEntity);
}

void HelperFunction::SetIdealActivity(CBaseEntity *pEntity, Activity NewActivity)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("SetIdealActivity", &func))
		{
			assert(0);
			return;
		}
	}

	if(!pEntity)
		return;

	typedef void (THISCALLCONV *_func)(CBaseEntity *, Activity);
	_func thisfunc = (_func)(func);
	(thisfunc)(pEntity, NewActivity);
}

bool HelperFunction::HaveSequenceForActivity(CBaseEntity *pEntity, Activity activity)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("HaveSequenceForActivity", &func))
		{
			assert(0);
			return false;
		}
	}

	if(!pEntity)
		return false;

	typedef bool (THISCALLCONV *_func)(void *, Activity);
	_func thisfunc = (_func)(func);
	return (thisfunc)(pEntity, activity);
}

void HelperFunction::TestPlayerPushing(CBaseEntity *pEntity, CBaseEntity *pPlayer)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("TestPlayerPushing", &func))
		{
			assert(0);
			return;
		}
	}

	if(!pEntity)
		return;

	typedef void (THISCALLCONV *_func)(void *, CBaseEntity *);
	_func thisfunc = (_func)(func);
	return (thisfunc)(pEntity, pPlayer);
}

int HelperFunction::CBaseCombatCharacter_OnTakeDamage(CBaseEntity *pEntity, const CTakeDamageInfo &info)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("CBaseCombatCharacter::OnTakeDamage", &func))
		{
			assert(0);
			return 0;
		}
	}

	if(!pEntity)
		return 0;

	typedef int (THISCALLCONV *_func)(CBaseEntity *, const CTakeDamageInfo &);
	_func thisfunc = (_func)(func);
	return (thisfunc)(pEntity, info);
}

void HelperFunction::CBasePlayer_RumbleEffect(CBaseEntity *pEntity, unsigned char index, unsigned char rumbleData, unsigned char rumbleFlags)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("CBasePlayer::RumbleEffect", &func))
		{
			assert(0);
			return;
		}
	}

	typedef void (THISCALLCONV *_func)(CBaseEntity*, unsigned char, unsigned char, unsigned char);
	_func thisfunc = (_func)(func);
	(thisfunc)(pEntity, index, rumbleData, rumbleFlags);
}

void HelperFunction::CBasePlayer_SnapEyeAngles(CBaseEntity *pEntity, const QAngle &viewAngles)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("CBasePlayer::SnapEyeAngles", &func))
		{
			assert(0);
			return;
		}
	}

	typedef void (THISCALLCONV *_func)(CBaseEntity*, const QAngle &);
	_func thisfunc = (_func)(func);
	(thisfunc)(pEntity, viewAngles);
}

CBaseEntity *HelperFunction::CBasePlayer_GetViewModel(CBaseEntity *pEntity, int index, bool bObserverOK)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("CBasePlayer::GetViewModel", &func))
		{
			assert(0);
			return NULL;
		}
	}

	if(!pEntity)
		return NULL;

	typedef CBaseEntity* (THISCALLCONV *_func)(CBaseEntity*, int, bool);
	_func thisfunc = (_func)(func);
	return (thisfunc)(pEntity, index, bObserverOK);
}

CBoneCache *HelperFunction::GetBoneCache(void *ptr)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("GetBoneCache", &func))
		{
			assert(0);
			return NULL;
		}
	}
	typedef CBoneCache *(THISCALLCONV *_func)(void *);
	_func thisfunc = (_func)(func);
	return (thisfunc)(ptr);
}

model_t *HelperFunction::GetModel(void *ptr)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("CBaseEntity::GetModel", &func))
		{
			assert(0);
			return nullptr;
		}
	}
	typedef model_t* (THISCALLCONV *_func)(void *);
	_func thisfunc = (_func)(func);
	return (thisfunc)(ptr);
}

void HelperFunction::LockStudioHdr(void *ptr)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("LockStudioHdr", &func))
		{
			assert(0);
			return;
		}
	}
	typedef void*(THISCALLCONV *_func)(void *);
	_func thisfunc = (_func)(func);
	(thisfunc)(ptr);
}

CStudioHdr *HelperFunction::GetModelPtr(void *ptr)
{
#if defined(PLATFORM_WINDOWS)
	static int offs = -1;
	if(offs == -1)
	{
		if(!g_pGameConf->GetOffset("CBaseAnimating::CStudioHdr", &offs))
		{
			assert(0);
			return NULL;
		}
	}

	unsigned char *pent = reinterpret_cast<unsigned char*>(ptr);
	CStudioHdr *pStudioHdr = *reinterpret_cast<CStudioHdr**>(pent + offs);
	if(!pStudioHdr && GetModel(ptr))
	{
		g_helpfunc.LockStudioHdr(ptr);
	}
	return ( pStudioHdr && pStudioHdr->IsValid() ) ? pStudioHdr : NULL;
#else
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("GetModelPtr", &func))
		{
			assert(0);
			return nullptr;
		}
	}
	typedef CStudioHdr *(THISCALLCONV *_func)(void *);
	_func thisfunc = (_func)(func);
	return (thisfunc)(ptr);
#endif
}

void HelperFunction::SetupBones( void *ptr, matrix3x4_t *pBoneToWorld, int boneMask )
{
	static int offset = NULL;
	if(!offset)
	{
		if(!g_pGameConf->GetOffset("InitBoneControllers", &offset))
		{
			assert(0);
		}
	}

	call_virtual<void>(ptr, offset, pBoneToWorld, boneMask);
}

void HelperFunction::VPhysicsSetObject(CBaseEntity *pEnt, IPhysicsObject *pPhysObj)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("CBaseEntity::VPhysicsSetObject", &func))
		{
			assert(0);
			return;
		}
	}
	typedef void (THISCALLCONV *_func)(CBaseEntity *, IPhysicsObject *);
	_func thisfunc = (_func)(func);
	(thisfunc)(pEnt, pPhysObj);
}

void HelperFunction::ResetClientsideFrame(CBaseEntity *pEnt)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("CBaseEntity::ResetClientsideFrame", &func))
		{
			assert(0);
			return;
		}
	}
	typedef void (THISCALLCONV *_func)(CBaseEntity *);
	_func thisfunc = (_func)(func);
	(thisfunc)(pEnt);
}

void HelperFunction::SUB_StartFadeOut(CBaseEntity *pEnt, float delay, bool notSolid)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("CBaseEntity::SUB_StartFadeOut_fb", &func))
		{
			assert(0);
			return;
		}
	}

	typedef void (THISCALLCONV *_func)(CBaseEntity *, float, bool);
	_func thisfunc = (_func)(func);
	return (thisfunc)(pEnt, delay, notSolid);
}

bool HelperFunction::ShouldBrushBeIgnored(void *ptr, CBaseEntity *pEntity)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("ShouldBrushBeIgnored", &func))
		{
			assert(0);
			return false;
		}
	}
	typedef bool (THISCALLCONV *_func)(void *, CBaseEntity *);
	_func thisfunc = (_func)(func);
	return (thisfunc)(ptr, pEntity);
}

bool HelperFunction::MoveLimit(void *ptr, Navigation_t navType, const Vector &vecStart, 
		const Vector &vecEnd, unsigned int collisionMask, const CBaseEntity *pTarget, 
		float pctToCheckStandPositions, unsigned flags, AIMoveTrace_t* pTrace)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("MoveLimit", &func))
		{
			assert(0);
			return false;
		}
	}
	typedef bool (THISCALLCONV *_func)(void *, Navigation_t, const Vector &, const Vector &, unsigned int, const CBaseEntity *, float, unsigned, AIMoveTrace_t*);
	_func thisfunc = (_func)(func);
	return (thisfunc)(ptr, navType, vecStart, vecEnd, collisionMask, pTarget, pctToCheckStandPositions, flags, pTrace);
}

int HelperFunction::CAI_TacticalServices_FindLosNode( void *ptr, const Vector &vThreatPos, const Vector &vThreatEyePos, float flMinThreatDist, float flMaxThreatDist, float flBlockTime, FlankType_t eFlankType, const Vector &vThreatFacing, float flFlankParam )
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("CAI_TacticalServices::FindLosNode", &func))
		{
			assert(0);
			return 0;
		}
	}

	typedef int (THISCALLCONV *_func)(void *, const Vector &, const Vector &, float , float , float , FlankType_t , const Vector &, float );
	_func thisfunc = (_func)(func);
	return (thisfunc)(ptr, vThreatPos, vThreatEyePos,  flMinThreatDist,  flMaxThreatDist,  flBlockTime,  eFlankType, vThreatFacing, flFlankParam);
}

int HelperFunction::CAI_TacticalServices_FindCoverNode( void *ptr, const Vector &vNearPos, const Vector &vThreatPos, const Vector &vThreatEyePos, float flMinDist, float flMaxDist )
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("CAI_TacticalServices::FindCoverNode", &func))
		{
			assert(0);
			return 0;
		}
	}

	typedef int (THISCALLCONV *_func)(void *, const Vector &, const Vector &, const Vector &, float , float);
	_func thisfunc = (_func)(func);
	return (thisfunc)(ptr, vNearPos, vThreatPos, vThreatEyePos, flMinDist, flMaxDist);
}

bool HelperFunction::CAI_Navigator_UpdateGoalPos(void *ptr, const Vector &goalPos)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("CAI_Navigator::UpdateGoalPos", &func))
		{
			assert(0);
			return false;
		}
	}
	typedef bool (THISCALLCONV *_func)(void *, const Vector &);
	_func thisfunc = (_func)(func);
	return (thisfunc)(ptr, goalPos);
}

bool HelperFunction::CAI_Navigator_SetGoal(void *ptr, const AI_NavGoal_t &goal, unsigned flags)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("CAI_Navigator::SetGoal", &func))
		{
			assert(0);
			return false;
		}
	}
	typedef bool (__fastcall *_func)(void *,int, const AI_NavGoal_t &, unsigned);
	_func thisfunc = (_func)(func);
	return (thisfunc)(ptr,0, goal, flags);
}

/*void PostSimulation_SetVelocityEvent( IPhysicsObject *pPhysicsObject, const Vector &vecVelocity )
{
	pPhysicsObject->SetVelocity( &vecVelocity, NULL );
}

void HelperFunction::PhysCallbackSetVelocity( IPhysicsObject *pPhysicsObject, const Vector &vecVelocity )
{
	Assert( physenv->IsInSimulation() );
	g_PostSimulationQueue->QueueCall( PostSimulation_SetVelocityEvent, pPhysicsObject, RefToVal(vecVelocity) );
}*/