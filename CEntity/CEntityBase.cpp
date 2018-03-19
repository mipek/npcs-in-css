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

#include "CEntityBase.h"
#include "CEntity.h"

CEntity *pEntityData[ENTITY_ARRAY_SIZE];


CEntity *CEntityLookup::Instance(CBaseEntity *pEnt)
{
	if (!pEnt)
	{
		return NULL;
	}
	
	return Instance(pEnt->GetRefEHandle());
}

CEntity *CEntityLookup::Instance(int iEnt)
{
	if(iEnt < 0 || iEnt >= ENTITY_ARRAY_SIZE)
	{
		return NULL;
	}
#ifdef _DEBUG
	CEntityManager::count++;
#endif
	return pEntityData[iEnt];
}

CEntity *CEntityLookup::Instance(const edict_t *pEnt)
{
	return Instance((edict_t *)pEnt);
}

CEntity *CEntityLookup::Instance(const CBaseHandle &hEnt)
{
	IHandleEntity *handle = hEnt.Get();
	if(handle == NULL)
	{
		return NULL;
	}
	return Instance(hEnt.GetEntryIndex());
}

CEntity *CEntityLookup::Instance(edict_t *pEnt)
{
	return Instance(engine->IndexOfEdict(pEnt));
}

