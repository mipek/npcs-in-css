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

#ifndef _INCLUDE_IENTITYFACTORY_H_
#define _INCLUDE_IENTITYFACTORY_H_

#include "CEntityManager.h"
#include "CEntity.h"

#define CE_LINK_ENTITY_TO_CLASS(mapClassName,DLLClassName) \
	static CEntityFactory_CE<DLLClassName> mapClassName(#mapClassName);

#define LINK_ENTITY_TO_CUSTOM_CLASS(mapClassName,replaceClassName,DLLClassName); \
	static CCustomEntityFactory<DLLClassName> mapClassName##custom(#mapClassName, #replaceClassName); \
	static CEntityFactory_CE<DLLClassName> mapClassName(#mapClassName, #replaceClassName);


class CEntity;
extern const char *szCurrentReplacedClassname;

class IEntityFactoryDictionary_CE
{
public:
	virtual void InstallFactory( IEntityFactory_CE *pFactory, const char *pClassName ) = 0;
	virtual IServerNetworkable *Create( const char *pClassName ) = 0;
	virtual void Destroy( const char *pClassName, IServerNetworkable *pNetworkable ) = 0;
	virtual IEntityFactory_CE *FindFactory( const char *pClassName ) = 0;
	virtual const char *GetCannonicalName( const char *pClassName ) = 0;
};


class IEntityFactory_CE
{
public:
	virtual CEntity *Create(edict_t *pEdict, CBaseEntity *pEnt) = 0;
};

template <class T>
class CEntityFactory_CE : public IEntityFactory_CE
{
public:
	CEntityFactory_CE(const char *pClassName)
	{
		GetEntityManager()->LinkEntityToClass(this, pClassName);
	}

	CEntityFactory_CE(const char *pClassName, const char *pReplaceName)
	{
		GetEntityManager()->LinkEntityToClass(this, pClassName, pReplaceName);
	}

	CEntity *Create(edict_t *pEdict, CBaseEntity *pEnt)
	{
		if (/*!pEdict || */!pEnt)
		{
			return NULL;
		}

		T* pOurEnt = new T();
		pOurEnt->CE_Init(pEdict, pEnt);

		return pOurEnt;
	}
};

abstract_class IEntityFactoryReal
{
public:
	IEntityFactoryReal()
	{
		m_Next = m_Head;
		m_Head = this;
	}
	virtual IServerNetworkable *Create( const char *pClassName ) = 0;
	virtual void Destroy( IServerNetworkable *pNetworkable ) = 0;
	virtual size_t GetEntitySize() = 0;
	virtual void AddToList() =0;

	static IEntityFactoryReal *m_Head;
	IEntityFactoryReal *m_Next;
};

template <class T>
class CCustomEntityFactory : public IEntityFactoryReal
{
	static IEntityFactoryDictionary_CE *EntityFactoryDictionary_CE()
	{
		return reinterpret_cast<IEntityFactoryDictionary_CE*>(servertools->GetEntityFactoryDictionary());
	}

public:
	CCustomEntityFactory(const char *pClassName, const char *pReplaceName)
	{
		this->pReplaceName = pReplaceName;
		this->pClassName = pClassName;		
	}

	void AddToList()
	{
		//assert(EntityFactoryDictionary_CE);
		EntityFactoryDictionary_CE()->InstallFactory((IEntityFactory_CE *)this, pClassName );
	}

	IServerNetworkable *Create( const char *pClassName )
	{
		szCurrentReplacedClassname = pClassName;
		IEntityFactoryReal *pFactory = (IEntityFactoryReal *)EntityFactoryDictionary_CE()->FindFactory(pReplaceName);
		assert(pFactory);

		return pFactory->Create(pReplaceName);
	}

	void Destroy( IServerNetworkable *pNetworkable )
	{
		IEntityFactoryReal *pFactory = (IEntityFactoryReal *)EntityFactoryDictionary_CE()->FindFactory(pReplaceName);
		assert(pFactory);
		return pFactory->Destroy(pNetworkable);
	}

	virtual size_t GetEntitySize()
	{
		IEntityFactoryReal *pFactory = (IEntityFactoryReal *)EntityFactoryDictionary_CE()->FindFactory(pReplaceName);
		assert(pFactory);
		return pFactory->GetEntitySize();
	}

	const char *pReplaceName;
	const char *pClassName;
};


class IEntityListener
{
public:
	virtual void OnEntityCreated( CBaseEntity *pEntity ) {};
	virtual void OnEntitySpawned( CBaseEntity *pEntity ) {};
	virtual void OnEntityDeleted( CBaseEntity *pEntity ) {};
};



#endif // _INCLUDE_IENTITYFACTORY_H_
