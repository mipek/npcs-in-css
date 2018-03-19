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

#ifndef _INCLUDE_CENTITYBASE_H_
#define _INCLUDE_CENTITYBASE_H_

class CEntity;
class CBaseHandle;
class CBaseEntity;
struct edict_t;

class CEntityLookup
{
public:
	static CEntity *Instance(const CBaseHandle &hEnt);
	static CEntity *Instance(const edict_t *pEnt);
	static CEntity *Instance(edict_t *pEnt);
	static CEntity* Instance(int iEnt);
	static CEntity* Instance(CBaseEntity *pEnt);
};


#endif // _INCLUDE_CENTITYBASE_H_
