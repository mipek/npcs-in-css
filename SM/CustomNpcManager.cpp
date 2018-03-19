
#include "CustomNpcManager.h"
#include "npc_custom.h"
#include <sm_trie_tpl.h>
#include <sh_vector.h>

using namespace SourceHook;


CustomNPCManager g_CustomNPCManager;

KTrie<SM_CustomNPC *> m_customNPC_by_name;
KTrie<SM_CustomNPC *> m_customNPC_by_sp;

List<CE_NPC_Custom *> m_customNPC_list;


class SM_CustomNPCEntityFactory : public IEntityFactoryReal
{
public:
	void AddToList() { }

	void AddToList(char *pClassName)
	{
		assert(EntityFactoryDictionary_CE);
		EntityFactoryDictionary_CE()->InstallFactory((IEntityFactory_CE *)this, pClassName );
	}

	IServerNetworkable *Create( const char *pClassName )
	{
		IEntityFactoryReal *pFactory = (IEntityFactoryReal *)EntityFactoryDictionary_CE()->FindFactory("cycler");
		assert(pFactory);

		return pFactory->Create("cycler");
	}

	void Destroy( IServerNetworkable *pNetworkable )
	{
		IEntityFactoryReal *pFactory = (IEntityFactoryReal *)EntityFactoryDictionary_CE()->FindFactory("cycler");
		assert(pFactory);
		return pFactory->Destroy(pNetworkable);
	}

	virtual size_t GetEntitySize()
	{
		IEntityFactoryReal *pFactory = (IEntityFactoryReal *)EntityFactoryDictionary_CE()->FindFactory("cycler");
		assert(pFactory);
		return pFactory->GetEntitySize();
	}
};


static SM_CustomNPCEntityFactory g_SM_CustomNPCEntityFactory;

CustomNPCManager::CustomNPCManager()
{
	npc_id = 0;
}


CustomNPCManager::~CustomNPCManager()
{


}

CEntity *CustomNPCManager::Create(edict_t *pEdict, CBaseEntity *pEnt)
{
	if (!pEnt)
	{
		return NULL;
	}

	CE_NPC_Custom* pOurEnt = new CE_NPC_Custom();
	pOurEnt->Init(pEdict, pEnt);
	m_customNPC_list.push_back(pOurEnt);

	return pOurEnt;
}

void CustomNPCManager::CustomNpcRemove(CE_NPC_Custom *npc)
{
	m_customNPC_list.remove(npc);
}

void CustomNPCManager::LinkCustomNPC(char *pClassName)
{
	GetEntityManager()->LinkEntityToClass(this, pClassName);
}





SM_CustomNPC *CustomNPCManager::FindCustomNPC(const char *name)
{
	SM_CustomNPC **pLookup = m_customNPC_by_name.retrieve(name);
	return (pLookup != NULL) ? *pLookup : NULL;
}

SM_CustomNPC *CustomNPCManager::FindCustomNPC(IPluginRuntime *the_sp)
{
	char v[20];
	_snprintf(v, sizeof(v), "%x", (unsigned int) the_sp);	
	SM_CustomNPC **pLookup = m_customNPC_by_sp.retrieve(v);
	return (pLookup != NULL) ? *pLookup : NULL;
}

void CustomNPCManager::AddCustomNPC(SM_CustomNPC *npc)
{
	char v[20];
	_snprintf(v, sizeof(v), "%x", (unsigned int) npc->sp_runtime);
	m_customNPC_by_name.insert(npc->npc_name, npc);
	m_customNPC_by_sp.insert(v, npc);
}


cell_t CustomNPCManager::CallBaseFunction(IPluginContext *pContext, const cell_t *params)
{
	IPluginRuntime *the_sp = pContext->GetRuntime();
	SM_CustomNPC *sp_npc = FindCustomNPC(the_sp);
	if(sp_npc == NULL)
		return 0;

	CE_NPC_Custom *npc = sp_npc->current_pNPC;
	if(npc == NULL)
		return 0;

	Entity_Function func_type = (Entity_Function)params[1];

	switch(func_type)
	{
		case EF_Spawn:
			npc->BaseClass::Spawn();
			break;
		case EF_Teleport:

			break;
		case EF_UpdateOnRemove:
			npc->BaseClass::UpdateOnRemove();
			break;

	}
	return 1;
}




#define	DECLARE_SP_FUNCTION(f_name)\
	sp_npc->sp_function[EF_##f_name] = the_sp->GetFunctionByName("M_"#f_name);\
	if(sp_npc->sp_function[EF_##f_name]) sp_npc->function_count++;


int CustomNPCManager::AddCustomNPC(IPluginContext *pContext, unsigned int data_size)
{
	IPluginRuntime *the_sp = pContext->GetRuntime();

	cell_t *phys_addr_g_NPC_Name;
	cell_t *phys_addr_g_pEntity;
	cell_t *phys_addr_g_NPC_ID;
	cell_t *phys_addr_pNPC;

	cell_t temp;
	uint32_t index;
	int error;

	if((error=the_sp->FindPubvarByName("g_NPC_Name", &index)) != SP_ERROR_NONE ||
		(error = the_sp->GetPubvarAddrs(index, &temp, &phys_addr_g_NPC_Name)) != SP_ERROR_NONE)
	{
		pContext->ThrowNativeError("Could not found g_NPC_Name");
		return 0;
	}

	char *name;
	pContext->LocalToString(temp, &name);

	if(FindCustomNPC(the_sp) || FindCustomNPC(name))
	{
		pContext->ThrowNativeError("Already register this npc - %s",name);
		return 0;
	}

	if((error=the_sp->FindPubvarByName("g_pEntity", &index)) != SP_ERROR_NONE ||
		(error = the_sp->GetPubvarAddrs(index, &temp, &phys_addr_g_pEntity)) != SP_ERROR_NONE)
	{
		pContext->ThrowNativeError("Could not found g_pEntity");
		return 0;
	}
	
	if((error=the_sp->FindPubvarByName("g_NPC_ID", &index)) != SP_ERROR_NONE ||
		(error = the_sp->GetPubvarAddrs(index, &temp, &phys_addr_g_NPC_ID)) != SP_ERROR_NONE)
	{
		pContext->ThrowNativeError("Could not found g_NPC_ID");
		return 0;
	}

	if(data_size > 0)
	{
		if((error=the_sp->FindPubvarByName("pNPC", &index)) != SP_ERROR_NONE ||
		(error = the_sp->GetPubvarAddrs(index, &temp, &phys_addr_pNPC)) != SP_ERROR_NONE)
		{
			pContext->ThrowNativeError("Could not found pNPC");
			return 0;
		}
	}

	npc_id++;
	
	*phys_addr_g_NPC_ID = npc_id;

	SM_CustomNPC *sp_npc = new SM_CustomNPC;
	memset(sp_npc, 0, sizeof(SM_CustomNPC));

	sp_npc->sp_runtime = the_sp;
	sp_npc->g_pEntity = phys_addr_g_pEntity;
	sp_npc->g_pNPC_ID = phys_addr_g_NPC_ID;
	sp_npc->npc_id = npc_id;
	sp_npc->pNPC = phys_addr_pNPC;
	sp_npc->member_data_size = data_size;
	
	strncpy(sp_npc->npc_name, name, strlen(name));

	if(data_size > 0)
	{
		sp_npc->original_member_data = new unsigned char[data_size];
		memcpy(sp_npc->original_member_data, phys_addr_pNPC, data_size);
	}

	sp_npc->current_pNPC = NULL;

	DECLARE_SP_FUNCTION(Init);
	DECLARE_SP_FUNCTION(CE_PostInit);
	DECLARE_SP_FUNCTION(Spawn);
	DECLARE_SP_FUNCTION(Teleport);
	DECLARE_SP_FUNCTION(UpdateOnRemove);
	DECLARE_SP_FUNCTION(UpdateOnRemove);
	DECLARE_SP_FUNCTION(OnTakeDamage);
	

	AddCustomNPC(sp_npc);
	LinkCustomNPC(name);
	g_SM_CustomNPCEntityFactory.AddToList(name);

	return npc_id;
}


