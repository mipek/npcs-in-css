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

#ifndef _INCLUDE_CENTITYMANAGER_H_
#define _INCLUDE_CENTITYMANAGER_H_

#include "sm_trie_tpl.h"
#include "extension.h"

class IEntityFactoryDictionary_CE;
class IEntityFactory_CE;

//typedef IEntityFactoryDictionary_CE *(*EntityFactoryDictionaryCall)();
//extern EntityFactoryDictionaryCall EntityFactoryDictionary_CE;

class CBaseEntityOutput;
#ifndef WIN32
typedef void (* FireOutputFuncType)(CBaseEntityOutput *, variant_t Value, CBaseEntity *pActivator, CBaseEntity *pCaller, float fDelay);
#else
typedef void (__thiscall * FireOutputFuncType)(CBaseEntityOutput *, variant_t Value, CBaseEntity *pActivator, CBaseEntity *pCaller, float fDelay);
#endif
extern FireOutputFuncType FireOutputFunc;

typedef bool (* PhysIsInCallbackFuncType)();
extern PhysIsInCallbackFuncType PhysIsInCallback;

class CEntityManager
{
public:
	CEntityManager();
	bool Init(IGameConfig *pConfig);
	void Shutdown();
	void LinkEntityToClass(IEntityFactory_CE *pFactory, const char *className);
	void LinkEntityToClass(IEntityFactory_CE *pFactory, const char *className, const char *replaceName);

	CEntity *CBaseEntityPostConstructor(CBaseEntity *pEntity, const char * szClassname);
	void RemoveEdict(edict_t *e);

	void PrintDump();

#ifdef _DEBUG
	static unsigned long long count;
#endif

private:
	KTrie<IEntityFactory_CE *> pFactoryTrie;
	KTrie<const char *> pSwapTrie;
	KTrie<bool> pHookedTrie;
	KTrie<IEntityFactory_CE *> pCacheTrie;
	IEntityFactoryDictionary_CE *pDict;
	bool m_bEnabled;

	
};

CEntityManager *GetEntityManager();

#endif // _INCLUDE_CENTITYMANAGER_H_
