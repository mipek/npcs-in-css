#ifndef _INCLUDE_CGAMEPLAYEREQUIP_H
#define _INCLUDE_CGAMEPLAYEREQUIP_H

#include "CEntityManager.h"
#include "CEntity.h"

#define MAX_EQUIP	32

class Template_CGamePlayerEquip: public CPointEntity
{
public:
	CE_DECLARE_CLASS(Template_CGamePlayerEquip, CPointEntity);
	
public:
	virtual void EquipPlayer(CBaseEntity *pEntity);
	
public:
	DECLARE_DEFAULTHEADER_DETOUR(EquipPlayer, void, (CBaseEntity *pEntity));
	DECLARE_DATAMAP(string_t*, m_weaponNames);
	DECLARE_DATAMAP(int*, m_weaponCount);
};

#endif //_INCLUDE_CGAMEPLAYEREQUIP_H