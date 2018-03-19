/**
* =============================================================================
* CEntity Entity Handling Framework
* Copyright (C) 2011 Matt Woodrow.  All rights reserved.
* =============================================================================
*
* This program is free software; you can redistribute it and/or modify it under
* the terms of the GNU General Public License, version 3.0, as published by the
* Free Software Foundation.
* 
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
* FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
* details.
*
* You should have received a copy of the GNU General Public License along with
* this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#include "CEntityManager.h"
#include "shareddefs.h"
#include "sourcehook.h"
#include "IEntityFactory.h"
#include "ehandle.h"
class CBaseEntity;
typedef CHandle<CBaseEntity> EHANDLE;
#include "takedamageinfo.h"
#include "server_class.h"
#include "CEntity.h"
#include "usercmd.h"
#include "isaverestore.h"

//#ifdef WIN32
#include "rtti.h"
//#endif
#include "../CDetour/detours.h"


SH_DECL_HOOK1(IEntityFactoryDictionary_CE, Create, SH_NOATTRIB, 0, IServerNetworkable *, const char *);
SH_DECL_HOOK1_void(IVEngineServer, RemoveEdict, SH_NOATTRIB, 0, edict_t *);

IEntityFactoryReal *IEntityFactoryReal::m_Head;

//EntityFactoryDictionaryCall EntityFactoryDictionary_CE = NULL;

CDetour *RemoveEntity_CDetour = NULL;
CDetour *PostConstructor_CDetour = NULL;
const char *szCurrentReplacedClassname = NULL;


#ifdef _DEBUG
unsigned long long CEntityManager::count = 0;
#endif

DETOUR_DECL_MEMBER1(RemoveEntity, void, CBaseHandle, handle)
{
	CEntity *cent = CEntity::Instance(handle);
	if(cent)
	{
		cent->Destroy();
	}
	DETOUR_MEMBER_CALL(RemoveEntity)(handle);
}

DETOUR_DECL_MEMBER1(PostConstructor, void, const char *, szClassname)
{
	CBaseEntity *pEntity = (CBaseEntity*)this;
	const char *szName = (szCurrentReplacedClassname)?szCurrentReplacedClassname:szClassname;

	DETOUR_MEMBER_CALL(PostConstructor)(szName);
	CEntity *cent = GetEntityManager()->CBaseEntityPostConstructor(pEntity, szName);
	szCurrentReplacedClassname = NULL;

	if(cent)
	{
		cent->PostConstructor();
	}
}

CEntityManager *GetEntityManager()
{
	static CEntityManager *entityManager = new CEntityManager();
	return entityManager;
}

CEntityManager::CEntityManager()
{
	memset(pEntityData, 0, sizeof(pEntityData));
	m_bEnabled = false;
}

bool CEntityManager::Init(IGameConfig *pConfig)
{
	void *addr;
#if 0
	/* Find the IEntityFactoryDictionary_CE* */
	if (!pConfig->GetMemSig("EntityFactory", &addr) || addr == NULL)
	{
		g_pSM->LogError(myself, "[CENTITY] Couldn't find sig: %s.", "EntityFactory");
		return false;
	}
	
	EntityFactoryDictionary_CE = (EntityFactoryDictionaryCall)addr;
	pDict = EntityFactoryDictionary_CE();
#endif
	pDict = reinterpret_cast<IEntityFactoryDictionary_CE*>(servertools->GetEntityFactoryDictionary());

	IEntityFactoryReal *pList = IEntityFactoryReal::m_Head;
	while (pList)
	{
		pList->AddToList();
		pList = pList->m_Next;
	}

	if (!pConfig->GetMemSig("FireOutput", &addr) || addr == NULL)
	{
		g_pSM->LogError(myself, "[CENTITY] Couldn't find sig: %s.", "FireOutput");
		return false;
	}

	FireOutputFunc = (FireOutputFuncType)addr;

	if (!pConfig->GetMemSig("PhysIsInCallback", &addr) || addr == NULL)
	{
		g_pSM->LogError(myself, "[CENTITY] Couldn't find sig: %s.", "PhysIsInCallback");
		return false;
	}

	PhysIsInCallback = (PhysIsInCallbackFuncType)addr;


	/* Reconfigure all the hooks */
	IHookTracker *pTracker = IHookTracker::m_Head;
	while (pTracker)
	{
		pTracker->ReconfigureHook(pConfig);
		pTracker = pTracker->m_Next;
	}

	CDetourManager::Init(g_pSM->GetScriptingEngine(), pConfig);

	RemoveEntity_CDetour = DETOUR_CREATE_MEMBER(RemoveEntity, "CBaseEntityList::RemoveEntity");
	RemoveEntity_CDetour->EnableDetour();

	PostConstructor_CDetour = DETOUR_CREATE_MEMBER(PostConstructor, "CBaseEntity::PostConstructor");
	PostConstructor_CDetour->EnableDetour();

	IDetourTracker *pDetourTracker = IDetourTracker::m_Head;
	while (pDetourTracker)
	{
		pDetourTracker->AddHook();
		pDetourTracker = pDetourTracker->m_Next;
	}

	ISigOffsetTracker *pSigOffsetTracker = ISigOffsetTracker::m_Head;
	while (pSigOffsetTracker)
	{
		pSigOffsetTracker->FindSig(pConfig);
		pSigOffsetTracker = pSigOffsetTracker->m_Next;
	}

	/* Start the creation hooks! */
	SH_ADD_HOOK(IVEngineServer, RemoveEdict, engine, SH_MEMBER(this, &CEntityManager::RemoveEdict), true);

	srand(time(NULL));

	m_bEnabled = true;
	return true;
}

void CEntityManager::Shutdown()
{
	SH_REMOVE_HOOK(IVEngineServer, RemoveEdict, engine, SH_MEMBER(this, &CEntityManager::RemoveEdict), true);

	IDetourTracker *pDetourTracker = IDetourTracker::m_Head;
	while (pDetourTracker)
	{
		pDetourTracker->RemoveHook();
		pDetourTracker = pDetourTracker->m_Next;
	}
	
	if(RemoveEntity_CDetour)
		RemoveEntity_CDetour->DisableDetour();
	RemoveEntity_CDetour = NULL;

	if(PostConstructor_CDetour)
		PostConstructor_CDetour->DisableDetour();
	PostConstructor_CDetour = NULL;

	pFactoryTrie.clear();
	pSwapTrie.clear();
	pHookedTrie.clear();
	pCacheTrie.clear();
}

void CEntityManager::LinkEntityToClass(IEntityFactory_CE *pFactory, const char *className)
{
	assert(pFactory);
	pFactoryTrie.insert(className, pFactory);
}

void CEntityManager::LinkEntityToClass(IEntityFactory_CE *pFactory, const char *className, const char *replaceName)
{
	LinkEntityToClass(pFactory, className);
	pSwapTrie.insert(className, replaceName);
}

CEntity *CEntityManager::CBaseEntityPostConstructor(CBaseEntity *pEntity, const char * szClassname )
{
	IServerNetworkable *pNetworkable = pEntity->GetNetworkable();
	Assert(pNetworkable);

	edict_t *pEdict = pNetworkable->GetEdict();

	if(strcmp(szClassname,"player") == 0 && engine->IndexOfEdict(pEdict) == 0)
	{
		return NULL;
	}

	IEntityFactory_CE **value = NULL;
	bool m_bShouldAddToCache = false;
	value = pCacheTrie.retrieve(szClassname);

	if(!value)
	{
		m_bShouldAddToCache = true;
		value = pFactoryTrie.retrieve(szClassname);
	}

	if (!value)
	{
		/* Attempt to do an RTTI lookup for C++ class links */
		IType *pType = GetType(pEntity);
		IBaseType *pBase = pType->GetBaseType();

		do 
		{
			const char *classname = GetTypeName(pBase->GetTypeInfo());
			value = pFactoryTrie.retrieve(classname);

			if (value)
			{
				break;
			}

		} while (pBase->GetNumBaseClasses() && (pBase = pBase->GetBaseClass(0)));

		pType->Destroy();
	}

	if (!value)
	{
		/* No specific handler for this entity */
		value = pFactoryTrie.retrieve("baseentity");
		assert(value);
	}

	IEntityFactory_CE *pFactory = *value;
	assert(pFactory);

	if(m_bShouldAddToCache)
	{
		pCacheTrie.insert(szClassname, pFactory);
	}

	CEntity *cent = pFactory->Create(pEdict, pEntity);

	char vtable[20];
	_snprintf(vtable, sizeof(vtable), "%x", (unsigned int) *(void **)pEntity);

	cent->ClearAllFlags();
	cent->InitProps();
	
	if (!pHookedTrie.retrieve(vtable))
	{
		cent->InitHooks();
		pHookedTrie.insert(vtable, true);
	}
	
	cent->InitDataMap();

	return cent;
}

void CEntityManager::RemoveEdict(edict_t *e)
{
	CEntity *pEnt = CEntity::Instance(e);
	if (pEnt)
	{
		g_pSM->LogMessage(myself, "Edict Removed, removing CEntity");
		assert(0);
		pEnt->Destroy();
	}
}


void our_trie_iterator(KTrie<IEntityFactory_CE *> *pTrie, const char *name, IEntityFactory_CE *& obj, void *data)
{
	SourceHook::List<CEntity *> *cent_list = (SourceHook::List<CEntity *> *)data;
	int count = 0;
	SourceHook::List<CEntity *>::iterator _iter;
	CEntity *pInfo;
	for(_iter=cent_list->begin(); _iter!=cent_list->end(); _iter++)
	{
		pInfo = (*_iter);

		if(strcmp(name, pInfo->GetClassname()) == 0)
		{
			count++;
			continue;
		}
		IType *pType = GetType(pInfo->BaseEntity());
		IBaseType *pBase = pType->GetBaseType();

		do 
		{
			const char *classname = GetTypeName(pBase->GetTypeInfo());
			if(strcmp(classname, name) == 0)
			{
				count++;
			}
		} while (pBase->GetNumBaseClasses() && (pBase = pBase->GetBaseClass(0)));
		pType->Destroy();
	}

	if(strlen(name) < 7)
		META_CONPRINTF("%s:\t\t\t\t%d\n",name,count);
	else if(strlen(name) < 15)
		META_CONPRINTF("%s:\t\t\t%d\n",name,count);
	else if(strlen(name) < 23)
		META_CONPRINTF("%s:\t\t%d\n",name,count);
	else
		META_CONPRINTF("%s:\t%d\n",name,count);
}

void CEntityManager::PrintDump()
{
	META_CONPRINTF("=====================================\n");
	
	SourceHook::List<CEntity *> cent_list;

	CEntity *cent;
	int networked_count = 0;
	int non_networked_count = 0;
	for(int i=0;i<NUM_ENT_ENTRIES;i++)
	{
		cent = pEntityData[i];
		if(cent != NULL)
		{
			cent_list.push_back(cent);
			if(i < MAX_EDICTS)
				networked_count++;
			else
				non_networked_count++;
		}
	}

#ifdef _DEBUG
	META_CONPRINTF("CEntity Instance:\t\t%lld\n",CEntityManager::count);
#endif
	META_CONPRINTF("CEntity Factory:\t\t%d\n",pFactoryTrie.size());
	META_CONPRINTF("CEntity Swap:\t\t\t%d\n",pSwapTrie.size());
	META_CONPRINTF("CEntity Cache:\t\t\t%d\n",pCacheTrie.size());
	META_CONPRINTF("CEntity Hook:\t\t\t%d\n",pHookedTrie.size());
	META_CONPRINTF("CEntity Networked:\t\t%d\n",networked_count);
	META_CONPRINTF("CEntity Non Networked:\t\t%d\n",non_networked_count);
	META_CONPRINTF("CEntity Total:\t\t\t%d\n",networked_count+non_networked_count);

	char buffer[128];
	pFactoryTrie.bad_iterator(buffer,128, &cent_list,our_trie_iterator);

	META_CONPRINTF("=====================================\n");
}
