
#include "extension.h"
#include "CAI_NetworkManager.h"
#include "GameSystem.h"
#include "tier3.h"
#include "datacache/imdlcache.h"
#include "scenefilecache/ISceneFileCache.h"
#include "item_ammo.h"

Monster g_Monster;

bool g_bUseNetworkVars = false;

SMEXT_LINK(&g_Monster);

IServerPluginCallbacks *vsp_callbacks = NULL;
CGlobalVars *gpGlobals = NULL;
INetworkStringTableContainer *netstringtables = NULL;
INetworkStringTable *g_pStringTableParticleEffectNames = NULL;
IEngineSound *engsound = NULL;
IEngineTrace *enginetrace = NULL;
IServerGameClients *gameclients = NULL;
ICvar *icvar = NULL;
IServerPluginHelpers *helpers = NULL;
IUniformRandomStream *enginerandom = NULL;
IStaticPropMgrServer *staticpropmgr = NULL;
IVModelInfo *modelinfo = NULL;
IPhysicsObjectPairHash *my_g_EntityCollisionHash = NULL;
ISpatialPartition *partition = NULL;
IPhysicsSurfaceProps *physprops = NULL;
IPhysicsCollision *physcollision = NULL;
IPhysicsEnvironment *physenv;
IPhysics *iphysics = NULL;
ISoundEmitterSystemBase *soundemitterbase = NULL;
IFileSystem *filesystem = NULL;
IEffects *g_pEffects = NULL;
IDecalEmitterSystem *decalsystem = NULL;
IEngineSound *enginesound = NULL;
ITempEntsSystem *te = NULL;
CSharedEdictChangeInfo *g_pSharedChangeInfo = NULL;
IGameMovement *g_pGameMovement = NULL;
IGameConfig *g_pGameConf = NULL;
IServerTools *servertools = NULL;
ISceneFileCache *scenefilecache = NULL;

CBaseEntityList *g_pEntityList = NULL;

CEntity *my_g_WorldEntity = NULL;
CBaseEntity *my_g_WorldEntity_cbase = NULL;

int gCmdIndex;
int gMaxClients;

int g_sModelIndexSmoke;
short g_sModelIndexBubbles;

bool CommandInitialize();

//extern sp_nativeinfo_t g_MonsterNatives[];

SH_DECL_HOOK3_void(IServerGameDLL, ServerActivate, SH_NOATTRIB, 0, edict_t *, int, int);
SH_DECL_HOOK1_void(IServerGameClients, SetCommandClient, SH_NOATTRIB, 0, int);


#define	TEST_SIGNATURE		1

#if TEST_SIGNATURE
static size_t UTIL_DecodeHexString(unsigned char *buffer, size_t maxlength, const char *hexstr)
{
	size_t written = 0;
	size_t length = strlen(hexstr);

	for (size_t i = 0; i < length; i++)
	{
		if (written >= maxlength)
			break;
		buffer[written++] = hexstr[i];
		if (hexstr[i] == '\\' && hexstr[i + 1] == 'x')
		{
			if (i + 3 >= length)
				continue;
			/* Get the hex part. */
			char s_byte[3];
			int r_byte;
			s_byte[0] = hexstr[i + 2];
			s_byte[1] = hexstr[i + 3];
			s_byte[2] = '\0';
			/* Read it as an integer */
			sscanf(s_byte, "%x", &r_byte);
			/* Save the value */
			buffer[written - 1] = r_byte;
			/* Adjust index */
			i += 3;
		}
	}

	return written;
}

class Test_Signature : public ITextListener_SMC
{
	virtual void ReadSMC_ParseStart()
	{
		has_error = false;
		addrInBase = (void *)g_SMAPI->GetServerFactory(false);
		ignore = true;
		META_CONPRINTF("[%s DEBUG] SIGNATURE TEST BEGIN\n", g_Monster.GetLogTag());
		//g_pSM->LogError(myself, "SIGNATURE TEST");
	}

	virtual void ReadSMC_ParseEnd(bool halted, bool failed)
	{
		META_CONPRINTF("[%s DEBUG] SIGNATURE TEST DONE\n", g_Monster.GetLogTag());
	}

	virtual SMCResult ReadSMC_NewSection(const SMCStates *states, const char *name)
	{
		if(strcmp(name,"Signatures") == 0)
		{
			ignore = false;
		}

		strncpy(current_name,name, strlen(name));
		current_name[strlen(name)] = '\0';

		return SMCResult_Continue;
	}

	virtual SMCResult ReadSMC_KeyValue(const SMCStates *states, const char *key, const char *value)
	{
		if(!ignore && strcmp(key,"windows") == 0)
		{
			unsigned char real_sig[511];
			size_t real_bytes;
			//size_t length;

			real_bytes = 0;
			//length = strlen(value);

			real_bytes = UTIL_DecodeHexString(real_sig, sizeof(real_sig), value);
			if (real_bytes >= 1)
			{
				void *addr = memutils->FindPattern(addrInBase,(char *)real_sig,real_bytes);
				if(addr == NULL)
				{
					has_error = true;
					META_CONPRINTF("[%s DEBUG] %s - FAIL\n",g_Monster.GetLogTag(), current_name);
					g_pSM->LogError(myself, "Failed to find \"%s\"", current_name);
				}
			}
		}
		return SMCResult_Continue;
	}

public:
	bool HasError() { return has_error; }

private:
	bool ignore;
	void *addrInBase;
	char current_name[128];
	bool has_error;
} g_Test_Signature;
#endif

bool Monster::SDK_OnLoad(char *error, size_t maxlength, bool late)
{
	const char *game_foler = g_pSM->GetGameFolderName();

	/*if(stricmp(game_foler,"hl2mp") == 0 || 
	   stricmp(game_foler,"garrysmod") == 0 ||
	   stricmp(game_foler,"obsidian") == 0
	)*/

	if(stricmp(game_foler,"cstrike") != 0)
	{
		g_pSM->Format(error, maxlength, "Unsupported Game: %s", game_foler);
		return false;
	}

	char conf_error[255] = "";
	char config_path[255];
	snprintf(config_path, sizeof(config_path),"monster/monster.%s.games",game_foler);
	if (!gameconfs->LoadGameConfigFile(config_path, &g_pGameConf, conf_error, sizeof(conf_error)))
	{
		if (conf_error[0])
		{
			g_pSM->Format(error, maxlength, "Could not read monster.%s.games: %s", game_foler, conf_error);
		}
		return false;
	}

#if TEST_SIGNATURE
	char path[512];
	g_pSM->BuildPath(Path_SM,path, sizeof(path),"gamedata/monster/monster.%s.games.txt",game_foler);
	SMCStates state = {0, 0};
	textparsers->ParseFile_SMC(path, &g_Test_Signature, &state);

	if(g_Test_Signature.HasError())
	{
		g_pSM->Format(error, maxlength, "Not all Signatures found.");
		return false;
	}
#endif

	if (!GetEntityManager()->Init(g_pGameConf))
	{
		g_pSM->Format(error, maxlength, "CEntity failed to initalize.");
		//g_pSM->LogError(myself, "CEntity failed to Initialize.");
		return false;
	}

	g_pSharedChangeInfo = engine->GetSharedEdictChangeInfo();

	g_pEntityList = (CBaseEntityList *)gamehelpers->GetGlobalEntityList();
	
	if(!CommandInitialize())
	{
		g_pSM->Format(error, maxlength, "Commands failed to initalize.");
		g_pSM->LogError(myself, "Command failed to Initialize. Server may Crash!");
		return false;
	}

	if(!g_helpfunc.Initialize())
	{
		g_pSM->Format(error, maxlength, "Helper failed to initalize.");
		g_pSM->LogError(myself, "Helper failed to Initialize. Server may Crash!");
		return false;
	}

	/* add a few hl2 ammotypes. */
	RegisterHL2AmmoTypes();

	//sharesys->AddNatives(myself, g_MonsterNatives);

 	return true;
}



void Monster::SDK_OnUnload()
{
	if(g_pGameConf != NULL)
	{
		gameconfs->CloseGameConfigFile(g_pGameConf);
		g_pGameConf = NULL;
	}
}

void Monster::SDK_OnAllLoaded()
{
}

bool Monster::QueryRunning(char *error, size_t maxlength)
{
	return true;
}

void Monster::OnVSPListening(IServerPluginCallbacks *iface)
{
	vsp_callbacks = iface;
}

bool Monster::SDK_OnMetamodLoad(ISmmAPI *ismm, char *error, size_t maxlen, bool late)
{
	CreateInterfaceFn appSystemFactory = ismm->GetEngineFactory();
	ConnectTier1Libraries(&appSystemFactory, 1);
	ConnectTier3Libraries(&appSystemFactory, 1);

	MathLib_Init(2.2f, 2.2f, 0.0f, 2);

	gpGlobals = ismm->GetCGlobals();
	if ((vsp_callbacks = ismm->GetVSPInfo(NULL)) == NULL)
	{
		ismm->AddListener(this, this);
		ismm->EnableVSPListener();
	}

	GET_V_IFACE_CURRENT(GetEngineFactory, icvar, ICvar, CVAR_INTERFACE_VERSION);

	GET_V_IFACE_CURRENT(GetEngineFactory, helpers, IServerPluginHelpers, INTERFACEVERSION_ISERVERPLUGINHELPERS);
	GET_V_IFACE_CURRENT(GetEngineFactory, netstringtables, INetworkStringTableContainer, INTERFACENAME_NETWORKSTRINGTABLESERVER);
	GET_V_IFACE_CURRENT(GetEngineFactory, engsound, IEngineSound, IENGINESOUND_SERVER_INTERFACE_VERSION);
    GET_V_IFACE_CURRENT(GetEngineFactory, enginetrace, IEngineTrace, INTERFACEVERSION_ENGINETRACE_SERVER);
	GET_V_IFACE_CURRENT(GetServerFactory, gameclients, IServerGameClients, INTERFACEVERSION_SERVERGAMECLIENTS);
	GET_V_IFACE_CURRENT(GetEngineFactory, enginerandom, IUniformRandomStream, VENGINE_SERVER_RANDOM_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, staticpropmgr, IStaticPropMgrServer, INTERFACEVERSION_STATICPROPMGR_SERVER);
	GET_V_IFACE_CURRENT(GetEngineFactory, modelinfo, IVModelInfo, VMODELINFO_SERVER_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, partition, ISpatialPartition, INTERFACEVERSION_SPATIALPARTITION);
	GET_V_IFACE_CURRENT(GetPhysicsFactory, physprops, IPhysicsSurfaceProps,  VPHYSICS_SURFACEPROPS_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, soundemitterbase, ISoundEmitterSystemBase,  SOUNDEMITTERSYSTEM_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetPhysicsFactory, physcollision, IPhysicsCollision,  VPHYSICS_COLLISION_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetFileSystemFactory, filesystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetServerFactory, g_pEffects, IEffects, IEFFECTS_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetPhysicsFactory, iphysics, IPhysics,  VPHYSICS_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, enginesound, IEngineSound,  IENGINESOUND_SERVER_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, mdlcache, IMDLCache,  MDLCACHE_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetServerFactory, g_pGameMovement, IGameMovement, INTERFACENAME_GAMEMOVEMENT);
	GET_V_IFACE_ANY(GetServerFactory, servertools, IServerTools, VSERVERTOOLS_INTERFACE_VERSION);
	GET_V_IFACE_ANY(GetEngineFactory, scenefilecache, ISceneFileCache, SCENE_FILE_CACHE_INTERFACE_VERSION);

	g_pCVar = icvar;
	
	ConVar_Register(0, this);

	SH_ADD_HOOK(IServerGameDLL, ServerActivate, gamedll, SH_MEMBER(this, &Monster::ServerActivate), true);
	SH_ADD_HOOK(IServerGameClients, SetCommandClient, serverclients, SH_MEMBER(this, &Monster::SetCommandClient), true);

	IGameSystem::InitAllSystems();

	return true;
}

bool Monster::RegisterConCommandBase(ConCommandBase *pCommand)
{
	META_REGCVAR(pCommand);

	return true;
}

bool Monster::SDK_OnMetamodUnload(char *error, size_t maxlength)
{
	SH_REMOVE_HOOK(IServerGameDLL, ServerActivate, gamedll, SH_MEMBER(this, &Monster::ServerActivate), true);
	SH_REMOVE_HOOK(IServerGameClients, SetCommandClient, serverclients, SH_MEMBER(this, &Monster::SetCommandClient), true);

	IGameSystem::SDKShutdownAllSystems();

	GetEntityManager()->Shutdown();

	DisconnectTier1Libraries();
	DisconnectTier3Libraries();

	return true;
}

void Monster::Precache()
{
	g_sModelIndexSmoke = engine->PrecacheModel("sprites/steam1.vmt",true);
	
	g_sModelIndexBubbles = engine->PrecacheModel("sprites/bubble.vmt",true);

	soundemitterbase->AddSoundOverrides("scripts/sm_monster/game_sounds_BaseNpc.txt", true);
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_headcrab.txt", true);
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_fastheadcrab.txt", true);
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_blackheadcrab.txt", true);
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_zombie.txt", true);
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_fastzombie.txt", true);
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_poisonzombie.txt", true);
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_manhack.txt", true);
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_antlionguard.txt", true);
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_antlionguard_episodic2.txt", true);
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_antlionguard_episodic.txt", true);
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_stalker.txt", true);
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_antlion.txt", true);
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/game_sounds_weapons.txt", true);
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/game_sounds_items.txt", true);
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_vortigaunt.txt", true);
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_rollermine.txt", true);
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_antlion_episodic.txt", true);
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_combine_cannon.txt", true);
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_env_headcrabcanister.txt", true);
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_turret.txt", true);
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_barnacle.txt", false);
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_stalker.txt", false);
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_houndeye.txt", false);
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_scanner.txt", false);
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/npc_sounds_soldier.txt", false);
	soundemitterbase->AddSoundOverrides("scripts/sm_monster/hl2_game_sounds_weapons.txt", false);

}

void Monster::ServerActivate(edict_t *pEdictList, int edictCount, int clientMax)
{
	gMaxClients = clientMax;

	RETURN_META(MRES_IGNORED);
}


void Monster::SetCommandClient( int cmd )
{
	gCmdIndex = cmd + 1;
}

int Monster::GetCommandClient()
{
	return gCmdIndex;
}

int Monster::GetMaxClients()
{
	return gMaxClients;
}

CON_COMMAND(sm_bugreport, "Shows bugreport prompt")
{
	int argc = args.ArgC();
	if (argc < 2)
	{
		META_CONPRINT("Usage: sm_bugreport <userid>\n");
		return;
	}

	int userid = atoi(args.Arg(1));
	if(userid > 0)
	{
		int client_idx = playerhelpers->GetClientOfUserId(userid);
		if(client_idx > 0)
		{
			edict_t *pEdict = gamehelpers->EdictOfIndex(client_idx);
		
			if (!pEdict || pEdict->IsFree())
			{
				return;
			}

			KeyValues *kv = new KeyValues("entry"); 
			kv->SetString("title", "Bug Reporter(Press ESC)"); 
			kv->SetString("msg", "Please input your report - Thanks!\n"); 
			kv->SetString("command", "sm_send_bugreport");
			kv->SetInt("level", 1); 
			kv->SetInt("time", 60); 
			helpers->CreateMessage(pEdict, DIALOG_ENTRY, kv, vsp_callbacks); 
			kv->deleteThis();
		}
	}
}