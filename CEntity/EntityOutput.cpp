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

#include "EntityOutput.h"
#include "isaverestore.h"
#include "GameSystem.h"

#include <mempool.h>

/*
extern ISaveRestoreOps *variantFuncs;

BEGIN_SIMPLE_DATADESC( CEventAction )
	DEFINE_FIELD( m_iTarget, FIELD_STRING ),
	DEFINE_FIELD( m_iTargetInput, FIELD_STRING ),
	DEFINE_FIELD( m_iParameter, FIELD_STRING ),
	DEFINE_FIELD( m_flDelay, FIELD_FLOAT ),
	DEFINE_FIELD( m_nTimesToFire, FIELD_INTEGER ),
	DEFINE_FIELD( m_iIDStamp, FIELD_INTEGER ),
END_DATADESC()
*/

CMemoryPool *g_EntityListPool = NULL;

FireOutputFuncType FireOutputFunc = NULL;

void CBaseEntityOutput::FireOutput(variant_t Value, CBaseEntity *pActivator, CBaseEntity *pCaller, float fDelay)
{
	assert(FireOutputFunc);
	FireOutputFunc(this, Value, pActivator, pCaller, fDelay);
}



void COutputEvent::FireOutput(CBaseEntity *pActivator, CBaseEntity *pCaller, float fDelay)
{
	variant_t Val;
	Val.Set( FIELD_VOID, NULL );
	CBaseEntityOutput::FireOutput(Val, pActivator, pCaller, fDelay);
}

void COutputEvent::FireOutput(CEntity *pActivator, CEntity *pCaller, float fDelay /*= 0*/)
{
	CBaseEntity *pRealActivator;
	CBaseEntity *pRealCaller;

	if (pActivator)
		pRealActivator = *pActivator;
	else
		pRealActivator = NULL;

	if (pCaller)
		pRealCaller = *pCaller;
	else
		pRealCaller = NULL;

	FireOutput(pRealActivator, pRealCaller, fDelay);
}

void COutputEvent::FireOutput(CBaseEntity *pActivator, CEntity *pCaller, float fDelay /*= 0*/)
{
	if (!pCaller)
		FireOutput(pActivator, (CBaseEntity *)NULL, fDelay);
	else
		FireOutput(pActivator, *pCaller, fDelay);
}

void COutputEvent::FireOutput(CEntity *pActivator, CBaseEntity *pCaller, float fDelay /*= 0*/)
{
	if (!pActivator)
		FireOutput((CBaseEntity *)NULL, pCaller, fDelay);
	else
		FireOutput(*pActivator, pCaller, fDelay);
}



//-----------------------------------------------------------------------------
// Purpose: Destructor.
//-----------------------------------------------------------------------------

CBaseEntityOutput::~CBaseEntityOutput()
{
	CEventAction *ev = m_ActionList;
	while (ev != NULL)
	{
		CEventAction *pNext = ev->m_pNext;	
		g_EntityListPool->Free(ev);
		ev = pNext;
	}
}

static CBaseEntityClassList *s_pClassLists = NULL;
CBaseEntityClassList::CBaseEntityClassList()
{
	m_pNextClassList = s_pClassLists;
	s_pClassLists = this;
}
CBaseEntityClassList::~CBaseEntityClassList()
{
}

class CEntityListSystem : public CBaseGameSystem
{
public:
	CEntityListSystem( char const *name ) : CBaseGameSystem( name )
	{
	}
	void LevelShutdownPostEntity()
	{
		CBaseEntityClassList *pClassList = s_pClassLists;
		while ( pClassList )
		{
			pClassList->LevelShutdownPostEntity();
			pClassList = pClassList->m_pNextClassList;
		}
	}
};

