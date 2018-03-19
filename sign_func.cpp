#include "sign_func.h"
#include "CEntity.h"
#include "CDetour/detours.h"
#include "CAI_NPC.h"
#include "ai_namespaces.h"
#include "CAI_schedule.h"
#include "CAI_senses.h"
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

class CAI_SensedObjectsManager;
struct TemplateEntityData_t;
class CGameStringPool;
class CMemoryPool;
class CCallQueue;
class CCheckClient;
class CFlexSceneFileManager;
class CDefaultResponseSystem;

HelperFunction g_helpfunc;



CGameMovement *g_CGameMovement = NULL;
Vector	*g_vecAttackDir = NULL;
CSoundEnvelopeController *g_SoundController = NULL;
IPredictionSystem *IPredictionSystem::g_pPredictionSystems = NULL;
CAI_SensedObjectsManager *g_AI_SensedObjectsManager = NULL;
CCallQueue *g_PostSimulationQueue = NULL;
IPhysicsObject *g_PhysWorldObject = NULL;

extern CMultiDamage *my_g_MultiDamage;
extern CUtlVector<TemplateEntityData_t *> *g_Templates;
extern CMemoryPool *g_EntityListPool;
extern ConVar *ammo_hegrenade_max;
extern ConVar *sk_autoaim_mode;
extern trace_t *g_TouchTrace;
extern INetworkStringTable *g_pStringTableParticleEffectNames;
extern CUtlVector<IValveGameSystem*> *s_GameSystems;
extern CCheckClient *g_CheckClient;
extern CFlexSceneFileManager *g_FlexSceneFileManager;

void InitDefaultAIRelationships();

SH_DECL_MANUALHOOK0(GameRules_FAllowNPCsHook, 0, 0, 0, bool);
SH_DECL_MANUALHOOK2(GameRules_ShouldCollide, 0, 0, 0, bool, int, int);
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

void HelperFunction::LevelInitPreEntity()
{
	g_pStringTableParticleEffectNames = netstringtables->FindTable("ParticleEffectNames");

	g_AI_SchedulesManager.CreateStringRegistries();

	InitDefaultAIRelationships();
	HookGameRules();

	META_CONPRINTF("[%s] Server may crash at this time!\n",g_Monster.GetLogTag());

	IPhysicsObjectPairHash *g_EntityCollisionHash;
	GET_VARIABLE_POINTER_NORET(g_EntityCollisionHash,  IPhysicsObjectPairHash *);
	my_g_EntityCollisionHash = g_EntityCollisionHash;

	CBaseEntity *g_WorldEntity = NULL;
	GET_VARIABLE_POINTER_NORET(g_WorldEntity,  CBaseEntity *);
	my_g_WorldEntity = CEntity::Instance(g_WorldEntity);
	my_g_WorldEntity_cbase = g_WorldEntity;


	// "ACT_RESET"
	CStringRegistry *m_pActivitySR = NULL;
	GET_VARIABLE_POINTER_NORET(m_pActivitySR, CStringRegistry *);
	CAI_NPC::m_pActivitySR = m_pActivitySR;


	// above 2
	int *m_iNumActivities = NULL;
	GET_VARIABLE_NORET(m_iNumActivities, int *);
	CAI_NPC::m_iNumActivities = m_iNumActivities;


	// "ERROR:  Mistake in default schedule def" above 2
	CStringRegistry *m_pEventSR = NULL;
	GET_VARIABLE_POINTER_NORET(m_pEventSR, CStringRegistry *);
	CAI_NPC::m_pEventSR = m_pEventSR;

	int m_iNumEvents_offset;
	if(!g_pGameConf->GetOffset("m_iNumEvents", &m_iNumEvents_offset))
	{
		g_pSM->LogError(myself,"Unable getting %s","m_iNumEvents");
		return;
	}

	char *m_iNumEvents_final_addr = reinterpret_cast<char *>(CAI_NPC::m_iNumActivities + m_iNumEvents_offset);
	CAI_NPC::m_iNumEvents = (int *)m_iNumEvents_final_addr;

	g_AI_SchedulesManager.LoadAllSchedules();
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
}


extern bool SetResponseSystem();

bool HelperFunction::FindAllValveGameSystem()
{
	FindValveGameSystem(g_CheckClient, CCheckClient *, "CCheckClient");
	
	FindValveGameSystem(g_PropDataSystem, CPropData *, "CPropData");

	FindValveGameSystem(g_SoundEmitterSystem, CValveBaseGameSystem *, "CSoundEmitterSystem");

	FindValveGameSystem(g_FlexSceneFileManager, CFlexSceneFileManager *, "CFlexSceneFileManager");

	if(!SetResponseSystem())
		return false;


	return true;
}

static CSoundEnvelopeController *GetSoundController()
{
	// "Invalid starting duration value in env" bellow 2, first function
	META_CONPRINTF("[%s] Getting Soundcontroller - ", g_Monster.GetLogTag());
	DWORD pGetSoundCtrl_Call;
	if(!g_pGameConf->GetMemSig("g_SoundController", (void**)&pGetSoundCtrl_Call))
	{
		META_CONPRINT("Failed\n");
		META_CONPRINTF("[%s] Couldn't find sig: g_SoundController", g_Monster.GetLogTag());
		return false;
	}
	int pGetSoundCtrl_Offs;
	if(!g_pGameConf->GetOffset("g_SoundController", &pGetSoundCtrl_Offs))
	{
		META_CONPRINT("Failed\n");
		META_CONPRINTF("[%s] Couldn't find offs: g_SoundController", g_Monster.GetLogTag());
		return false;
	}
	META_CONPRINTF("Success\n");

	typedef CSoundEnvelopeController* (*GetSoundCtrler_t)();
	pGetSoundCtrl_Call += pGetSoundCtrl_Offs; // point to call offset E8 >?? ?? ?? ??<
	DWORD dwJmpOffs = *(DWORD*)pGetSoundCtrl_Call;
	GetSoundCtrler_t getSoundController = (GetSoundCtrler_t)(pGetSoundCtrl_Call + 4 + dwJmpOffs);
	return getSoundController();
}

bool HelperFunction::Initialize()
{
	char *addr = NULL;
	int offset = 0;

	RegisterHook("GameRules_FAllowNPCs",GameRules_FAllowNPCsHook);
	RegisterHook("GameRules_ShouldCollide",GameRules_ShouldCollide);
	RegisterHook("CGameMovement_OnLadder",OnLadderHook);

	if(!g_pGameConf->GetMemSig("CreateGameRulesObject", (void **)&addr) || !addr)
		return false;

	if(!g_pGameConf->GetOffset("g_pGameRules", &offset) || !offset)
		return false;

	my_g_pGameRules = *reinterpret_cast<void ***>(addr + offset);
	
	// this is a little tricky, we sigscan references to a func that returns the soundcontroller
	g_SoundController = GetSoundController();

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
	Relationship_t ***m_DefaultRelationship;
	GET_VARIABLE(m_DefaultRelationship, Relationship_t ** *);
	CCombatCharacter::m_DefaultRelationship = m_DefaultRelationship;

	// "CNavArea::IncrementPlayerCount: Underfl"  retn    8 bellow 4
	int *m_lastInteraction = NULL;
	GET_VARIABLE(m_lastInteraction, int *);
	CCombatCharacter::m_lastInteraction = m_lastInteraction;

	// "Can't find decal %s\n"
	GET_VARIABLE_POINTER(decalsystem, IDecalEmitterSystem *);
	
	IPredictionSystem *g_pPredictionSystems = NULL;
	GET_VARIABLE_POINTER(g_pPredictionSystems, IPredictionSystem *);
	IPredictionSystem::g_pPredictionSystems = g_pPredictionSystems;

	//GET_VARIABLE_POINTER(te, ITempEntsSystem *);
	te = servertools->GetTempEntsSystem();

	GET_VARIABLE(my_g_MultiDamage, CMultiDamage *);

	GET_VARIABLE(g_Templates, CUtlVector<TemplateEntityData_t *> *);

	GET_VARIABLE(g_EntityListPool, CMemoryPool *);

	GET_VARIABLE(g_CEventQueue, CEventQueue *);
	
	GET_VARIABLE(g_TouchTrace, trace_t *);

	GET_VARIABLE(g_PostSimulationQueue, CCallQueue *);

	GET_VARIABLE(g_PhysWorldObject, IPhysicsObject *);

	CMemoryPool *EventQueuePrioritizedEvent_t_s_Allocator;
	GET_VARIABLE(EventQueuePrioritizedEvent_t_s_Allocator, CMemoryPool *);
	EventQueuePrioritizedEvent_t::s_Allocator = EventQueuePrioritizedEvent_t_s_Allocator;
	
	CAIHintVector *gm_AllHints = NULL;
	GET_VARIABLE(gm_AllHints, CAIHintVector *);
	CAI_HintManager::gm_AllHints = gm_AllHints;

	GET_VARIABLE(s_GameSystems, CUtlVector<IValveGameSystem*> *);

	CMemoryPool *AI_Waypoint_t_s_Allocator;
	GET_VARIABLE(AI_Waypoint_t_s_Allocator, CMemoryPool *);
	AI_Waypoint_t::s_Allocator = AI_Waypoint_t_s_Allocator;

	GET_VARIABLE(g_AIFriendliesTalkSemaphore, CAI_TimedSemaphore *);
	GET_VARIABLE(g_AIFoesTalkSemaphore, CAI_TimedSemaphore *);

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

	if(!FindAllValveGameSystem())
		return false;

	IGameSystem::HookValveSystem();

	return true;
}


bool HelperFunction::OnLadder( trace_t &trace )
{
	CBaseEntity *cbase = (CBaseEntity *)g_CGameMovement->player;
	CPlayer *pPlayer = (CPlayer *)CEntity::Instance(cbase);
	if(pPlayer)
	{
		CEntity *ground = pPlayer->GetGroundEntity();
		if(pPlayer->m_bOnLadder && ground == NULL)
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

void HelperFunction::HookGameRules()
{
	if(my_g_pGameRules == NULL)
		return;

	void *rules = NULL;
	memcpy(&rules, reinterpret_cast<void*>(my_g_pGameRules), sizeof(char*));
	if(rules == NULL)
		return;

	SH_ADD_MANUALHOOK_MEMFUNC(GameRules_FAllowNPCsHook, rules, &g_helpfunc, &HelperFunction::GameRules_FAllowNPCs, false);
	SH_ADD_MANUALHOOK_MEMFUNC(GameRules_ShouldCollide, rules, &g_helpfunc, &HelperFunction::GameRules_ShouldCollide_Hook, true);
}


void HelperFunction::UnHookGameRules()
{
	if(my_g_pGameRules == NULL)
		return;

	void *rules = NULL;
	memcpy(&rules, reinterpret_cast<void*>(my_g_pGameRules), sizeof(char*));
	if(rules == NULL)
		return;

	SH_REMOVE_MANUALHOOK_MEMFUNC(GameRules_FAllowNPCsHook, rules, &g_helpfunc, &HelperFunction::GameRules_FAllowNPCs, false);
	SH_REMOVE_MANUALHOOK_MEMFUNC(GameRules_ShouldCollide, rules, &g_helpfunc, &HelperFunction::GameRules_ShouldCollide_Hook, true);
}

bool HelperFunction::CGameRules_ShouldCollide( int collisionGroup0, int collisionGroup1 )
{
	if ( collisionGroup0 > collisionGroup1 )
	{
		// swap so that lowest is always first
		swap(collisionGroup0,collisionGroup1);
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

	unsigned char *rules = NULL;
	memcpy(&rules, reinterpret_cast<void*>(my_g_pGameRules), sizeof(char*));

	void **this_ptr = *reinterpret_cast<void ***>(&rules);
	void **vtable = *reinterpret_cast<void ***>(rules);
	void *vfunc = vtable[offset];

	union
	{
		bool (VEmptyClass::*mfpnew)(int, int);
		void *addr;
	} u;
	u.addr = vfunc;

	return (reinterpret_cast<VEmptyClass *>(this_ptr)->*u.mfpnew)(collisionGroup0, collisionGroup1);
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
	
	unsigned char *rules = NULL;
	memcpy(&rules, reinterpret_cast<void*>(my_g_pGameRules), sizeof(char*));

	void **this_ptr = *reinterpret_cast<void ***>(&rules);
	void **vtable = *reinterpret_cast<void ***>(rules);
	void *vfunc = vtable[offset];

	union
	{
		bool (VEmptyClass::*mfpnew)(int);
		void *addr;
	} u;
	u.addr = vfunc;

	return (reinterpret_cast<VEmptyClass *>(this_ptr)->*u.mfpnew)(iDmgType);
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
	
	unsigned char *rules = NULL;
	memcpy(&rules, reinterpret_cast<void*>(my_g_pGameRules), sizeof(char*));

	void **this_ptr = *reinterpret_cast<void ***>(&rules);
	void **vtable = *reinterpret_cast<void ***>(rules);
	void *vfunc = vtable[offset];

	union
	{
		bool (VEmptyClass::*mfpnew)(int);
		void *addr;
	} u;
	u.addr = vfunc;

	return (reinterpret_cast<VEmptyClass *>(this_ptr)->*u.mfpnew)(iLevel);
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
	
	unsigned char *rules = NULL;
	memcpy(&rules, reinterpret_cast<void*>(my_g_pGameRules), sizeof(char*));

	void **this_ptr = *reinterpret_cast<void ***>(&rules);
	void **vtable = *reinterpret_cast<void ***>(rules);
	void *vfunc = vtable[offset];

	union
	{
		bool (VEmptyClass::*mfpnew)();
		void *addr;
	} u;
	u.addr = vfunc;

	return (reinterpret_cast<VEmptyClass *>(this_ptr)->*u.mfpnew)();
}

void HelperFunction::GameRules_RadiusDamage(const CTakeDamageInfo &info, const Vector &vecSrc, float flRadius, int iClassIgnore, CBaseEntity *pEntityIgnore)
{
	static int offset = NULL;
	if(!offset)
	{
		if(!g_pGameConf->GetOffset("GameRules_RadiusDamage", &offset))
		{
			assert(0);
			return;
		}
	}
	
	unsigned char *rules = NULL;
	memcpy(&rules, reinterpret_cast<void*>(my_g_pGameRules), sizeof(char*));

	void **this_ptr = *reinterpret_cast<void ***>(&rules);
	void **vtable = *reinterpret_cast<void ***>(rules);
	void *vfunc = vtable[offset];

	union
	{
		void (VEmptyClass::*mfpnew)(const CTakeDamageInfo &, const Vector &, float , int , CBaseEntity *);
		void *addr;
	} u;
	u.addr = vfunc;

	(reinterpret_cast<VEmptyClass *>(this_ptr)->*u.mfpnew)(info,vecSrc,flRadius,iClassIgnore,pEntityIgnore);
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
	
	unsigned char *rules = NULL;
	memcpy(&rules, reinterpret_cast<void*>(my_g_pGameRules), sizeof(char*));

	void **this_ptr = *reinterpret_cast<void ***>(&rules);
	void **vtable = *reinterpret_cast<void ***>(rules);
	void *vfunc = vtable[offset];

	union
	{
		CBaseEntity *(VEmptyClass::*mfpnew)(CBaseEntity *, CBaseEntity *);
		void *addr;
	} u;
	u.addr = vfunc;

	CBaseEntity *ret = (reinterpret_cast<VEmptyClass *>(this_ptr)->*u.mfpnew)(pPlayer, pCurrentWeapon);
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
	
	unsigned char *rules = NULL;
	memcpy(&rules, reinterpret_cast<void*>(my_g_pGameRules), sizeof(char*));

	void **this_ptr = *reinterpret_cast<void ***>(&rules);
	void **vtable = *reinterpret_cast<void ***>(rules);
	void *vfunc = vtable[offset];

	union
	{
		bool (VEmptyClass::*mfpnew)(CBaseEntity *, CBaseEntity *, const CTakeDamageInfo &info);
		void *addr;
	} u;
	u.addr = vfunc;

	return (reinterpret_cast<VEmptyClass *>(this_ptr)->*u.mfpnew)(pPlayer, pAttacker, info);
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
	
	unsigned char *rules = NULL;
	memcpy(&rules, reinterpret_cast<void*>(my_g_pGameRules), sizeof(char*));

	void **this_ptr = *reinterpret_cast<void ***>(&rules);
	void **vtable = *reinterpret_cast<void ***>(rules);
	void *vfunc = vtable[offset];

	union
	{
		void (VEmptyClass::*mfpnew)();
		void *addr;
	} u;
	u.addr = vfunc;

	return (reinterpret_cast<VEmptyClass *>(this_ptr)->*u.mfpnew)();
}

int HelperFunction::GameRules_GetAutoAimMode()
{
	// we use the cvar to avoid doing a sigscan..
	return sk_autoaim_mode->GetInt();
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

	typedef HSOUNDSCRIPTHANDLE (__thiscall *_func)(void *, const char *); //TODO: i though that func is static?
    _func thisfunc = (_func)func;
    return thisfunc(g_SoundEmitterSystem, soundname);
}

void HelperFunction::EmitSoundByHandle( IRecipientFilter& filter, int entindex, const EmitSound_t & ep, HSOUNDSCRIPTHANDLE& handle )
{
	g_helpfunc.EmitSound(filter, entindex, ep);
#if 0
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("EmitSoundByHandle", &func))
		{
			assert(0);
			return;
		}
	}

	typedef void (__thiscall *_func)(void *, IRecipientFilter& , int , const EmitSound_t & , HSOUNDSCRIPTHANDLE& );
    _func thisfunc = (_func)func;
    thisfunc(g_SoundEmitterSystem, filter, entindex, ep, handle);
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

void HelperFunction::PhysCallbackRemove(IServerNetworkable *pRemove)
{
	if ( PhysIsInCallback() ) {
		Assert(!"not implemented"); // CE_TODO: impl me
		//g_Collisions.AddRemoveObject(pRemove);
	} else {
		g_helpfunc.UTIL_Remove(pRemove);
	}
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

	typedef void (__thiscall *_func)(void *, float, const char *);
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
#if 0
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("SimThink_EntityChanged", &func))
		{
			assert(0);
			return;
		}
	}

	if(!pEntity)
		return;

	typedef void (__cdecl *_func)(CSimThinkManager *, CBaseEntity *);
    _func thisfunc = (_func)(func);
	(thisfunc)(GetSimThinkMngr(), pEntity);
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

	typedef void (__thiscall *_func)(CBaseEntity *, CBaseEntity *);
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

	typedef void (__thiscall *_func)(CBaseEntity *, const Vector &);
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

	typedef void (__thiscall *_func)(void *, const QAngle &);
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

	void **this_ptr = *reinterpret_cast<void ***>(&pEntity);
	void **vtable = *reinterpret_cast<void ***>(pEntity);
	void *vfunc = vtable[offset];

	union
	{
		IServerVehicle *(VEmptyClass::*mfpnew)();
		void *addr;
	} u;
	u.addr = vfunc;

	return (reinterpret_cast<VEmptyClass *>(this_ptr)->*u.mfpnew)();
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

	void **this_ptr = *reinterpret_cast<void ***>(&pEntity);
	void **vtable = *reinterpret_cast<void ***>(pEntity);
	void *vfunc = vtable[offset];

	union
	{
		IServerVehicle *(VEmptyClass::*mfpnew)();
		void *addr;
	} u;
	u.addr = vfunc;

	return (reinterpret_cast<VEmptyClass *>(this_ptr)->*u.mfpnew)();
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

	typedef void (__thiscall *_func)(void *, const char *, float, float *);
	_func thisfunc = (_func)(func);
	(thisfunc)(pEntity,soundname, soundtime, duration);
}

void HelperFunction::EmitSound(IRecipientFilter& filter, int iEntIndex, const EmitSound_t & params)
{
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

	typedef void (__thiscall *_func)(void *);
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

	typedef void (__thiscall *_func)(void *, CBaseEntity *, CBaseEntity *, USE_TYPE , float );
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

	typedef bool (__thiscall *_func)(void *, CBaseEntity *, int , CBaseEntity **);
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

	typedef void (__thiscall *_func)(void *);
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

	typedef void (__thiscall *_func)(void *, CBaseEntity *, trace_t &);
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

	typedef void *(__thiscall *_func)(void *, int);
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

	typedef void (__thiscall *_func)(void *, MoveType_t, MoveCollide_t);
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

	typedef void (__thiscall *_func)(void *);
	_func thisfunc = (_func)(func);
	(thisfunc)(pEntity);
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

void HelperFunction::PhysDisableEntityCollisions( IPhysicsObject *pObject0, IPhysicsObject *pObject1 )
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("PhysDisableEntityCollisions_VPhysics", &func))
		{
			assert(0);
			return;
		}
	}

	typedef ragdoll_t *(*_func)(IPhysicsObject*, IPhysicsObject*);
	_func thisfunc = (_func)(func);
	(thisfunc)(pObject0, pObject1);
}

void HelperFunction::PhysEnableEntityCollisions( CBaseEntity *pObject0, CBaseEntity *pObject1 )
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("PhysEnableEntityCollisions_BaseEnt", &func))
		{
			assert(0);
			return;
		}
	}

	typedef ragdoll_t *(*_func)(CBaseEntity*, CBaseEntity*);
	_func thisfunc = (_func)(func);
	(thisfunc)(pObject0, pObject1);
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

	typedef void (__thiscall *_func)(CBaseEntity*);
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

void HelperFunction::CGib_Spawn( CBaseEntity *gib, const char *mdl )
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

	typedef void (__thiscall *_func)(CBaseEntity *, const char *);
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

	typedef void (__thiscall *_func)(void *, int);
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

	typedef void (__thiscall *_func)(void *, int);
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

	typedef void (__thiscall *_func)(void *);
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

	typedef void (__thiscall *_func)(void *, CBaseEntity *, unsigned int, unsigned int);
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
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("FindEntityByClassname", &func))
		{
			assert(0);
			return NULL;
		}
	}

	typedef CBaseEntity *(__thiscall *_func)(void *, CBaseEntity *, const char *);
	_func thisfunc = (_func)(func);
	CBaseEntity *cbase = (thisfunc)(g_pEntityList,pStartEntity, szName);
	return CEntity::Instance(cbase);
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
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("FindEntityByName", &func))
		{
			assert(0);
			return NULL;
		}
	}

	typedef CBaseEntity *(__thiscall *_func)(void *, CBaseEntity *, const char *, CBaseEntity *, CBaseEntity *, CBaseEntity *, IEntityFindFilter *);
	_func thisfunc = (_func)(func);
	return CEntity::Instance((thisfunc)(g_pEntityList, pStartEntity, szName, pSearchingEntity, pActivator, pCaller, pFilter));
}

CEntity *HelperFunction::FindEntityInSphere( CEntity *pStartEntity, const Vector &vecCenter, float flRadius )
{
	return FindEntityInSphere((pStartEntity)?pStartEntity->BaseEntity():(CBaseEntity *)NULL, vecCenter, flRadius);
}

CEntity *HelperFunction::FindEntityInSphere( CBaseEntity *pStartEntity, const Vector &vecCenter, float flRadius )
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("FindEntityInSphere", &func))
		{
			assert(0);
			return NULL;
		}
	}

	typedef CBaseEntity *(__thiscall *_func)(void *, CBaseEntity *,  const Vector &, float);
	_func thisfunc = (_func)(func);
	return CEntity::Instance((thisfunc)(g_pEntityList, pStartEntity, vecCenter, flRadius));
}

CEntity *HelperFunction::FindEntityGeneric( CBaseEntity *pStartEntity, const char *szName, CBaseEntity *pSearchingEntity, CBaseEntity *pActivator, CBaseEntity *pCaller )
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("FindEntityGeneric", &func))
		{
			assert(0);
			return NULL;
		}
	}

	typedef CBaseEntity *(__thiscall *_func)(void *, const char *, CBaseEntity *, CBaseEntity *, CBaseEntity *);
	_func thisfunc = (_func)(func);
	return CEntity::Instance((thisfunc)(g_pEntityList, szName, pSearchingEntity, pActivator, pCaller));
}

CEntity *HelperFunction::FindEntityGenericNearest( const char *szName, const Vector &vecSrc, float flRadius, CBaseEntity *pSearchingEntity, CBaseEntity *pActivator, CBaseEntity *pCaller)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("FindEntityGenericNearest", &func))
		{
			assert(0);
			return NULL;
		}
	}

	typedef CBaseEntity *(__thiscall *_func)(void *, const char *, const Vector &, float , CBaseEntity *, CBaseEntity *, CBaseEntity *);
	_func thisfunc = (_func)(func);
	return CEntity::Instance((thisfunc)(g_pEntityList, szName, vecSrc, flRadius, pSearchingEntity, pActivator, pCaller));
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

	typedef void (__thiscall *_func)(void *, IEntityListener *);
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

	typedef void (__thiscall *_func)(void *, IEntityListener *);
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

	typedef int (__thiscall *_func)(void *);
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

	typedef void (__thiscall *_func)(void *);
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

	typedef bool (__thiscall *_func)(void *, float , CBaseEntity *, AIMoveTrace_t *);
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

	typedef void (__thiscall *_func)(void *);
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

	typedef void (__thiscall *_func)(CBaseEntity *, Activity);
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

	typedef bool (__thiscall *_func)(void *, Activity);
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

	typedef void (__thiscall *_func)(void *, CBaseEntity *);
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

	typedef int (__thiscall *_func)(CBaseEntity *, const CTakeDamageInfo &);
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

	typedef void (__thiscall *_func)(CBaseEntity*, unsigned char, unsigned char, unsigned char);
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

	typedef void (__thiscall *_func)(CBaseEntity*, const QAngle &);
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

	typedef CBaseEntity* (__thiscall *_func)(CBaseEntity*, int, bool);
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
	typedef CBoneCache *(__thiscall *_func)(void *);
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
			return false;
		}
	}
	typedef model_t* (__thiscall *_func)(void *);
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
	typedef void*(__thiscall *_func)(void *);
	_func thisfunc = (_func)(func);
	(thisfunc)(ptr);
}

CStudioHdr *HelperFunction::GetModelPtr(void *ptr)
{
	static int offs = -1;
	if(offs == -1)
	{
		if(!g_pGameConf->GetOffset("CBaseAnimating::CStudioHdr", &offs))
		{
			assert(0);
			return NULL;
		}
	}

#if defined(PLATFORM_WINDOWS)
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
			return;
		}
	}
	typedef CStudioHdr *(__thiscall *_func)(void *);
	_func thisfunc = (_func)(func);
	return (thisfunc)(ptr);
#endif

	return NULL;
}

void HelperFunction::InitBoneControllers(void *ptr)
{
	static int offset = NULL;
	if(!offset)
	{
		if(!g_pGameConf->GetOffset("InitBoneControllers", &offset))
		{
			assert(0);
		}
	}

	void **this_ptr = *reinterpret_cast<void ***>(&ptr);
	void **vtable = *reinterpret_cast<void ***>(ptr);
	void *vfunc = vtable[offset];

	union
	{
		void (VEmptyClass::*mfpnew)();
		void *addr;
	} u;
	u.addr = vfunc;

	(reinterpret_cast<VEmptyClass *>(this_ptr)->*u.mfpnew)();
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

	void **this_ptr = *reinterpret_cast<void ***>(&ptr);
	void **vtable = *reinterpret_cast<void ***>(ptr);
	void *vfunc = vtable[offset];

	union
	{
		void (VEmptyClass::*mfpnew)(matrix3x4_t*, int);
		void *addr;
	} u;
	u.addr = vfunc;

	(reinterpret_cast<VEmptyClass *>(this_ptr)->*u.mfpnew)(pBoneToWorld, boneMask);
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
	typedef void (__thiscall *_func)(CBaseEntity *, IPhysicsObject *);
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
	typedef void (__thiscall *_func)(CBaseEntity *);
	_func thisfunc = (_func)(func);
	(thisfunc)(pEnt);
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
	typedef bool (__thiscall *_func)(void *, CBaseEntity *);
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
	typedef bool (__thiscall *_func)(void *, Navigation_t, const Vector &, const Vector &, unsigned int, const CBaseEntity *, float, unsigned, AIMoveTrace_t*);
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

	typedef int (__thiscall *_func)(void *, const Vector &, const Vector &, float , float , float , FlankType_t , const Vector &, float );
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

	typedef int (__thiscall *_func)(void *, const Vector &, const Vector &, const Vector &, float , float);
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
	typedef bool (__thiscall *_func)(void *, const Vector &);
	_func thisfunc = (_func)(func);
	return (thisfunc)(ptr, goalPos);
}

void PostSimulation_SetVelocityEvent( IPhysicsObject *pPhysicsObject, const Vector &vecVelocity )
{
	pPhysicsObject->SetVelocity( &vecVelocity, NULL );
}

void HelperFunction::PhysCallbackSetVelocity( IPhysicsObject *pPhysicsObject, const Vector &vecVelocity )
{
	Assert( physenv->IsInSimulation() );
	g_PostSimulationQueue->QueueCall( PostSimulation_SetVelocityEvent, pPhysicsObject, RefToVal(vecVelocity) );
}