#ifndef _INCLUDE_ITEMRESPAWNSYSTEM_H
#define _INCLUDE_ITEMRESPAWNSYSTEM_H


#include "CEntity.h"
#include "GameSystem.h"

class CCombatWeapon;
class ICItem;
class CAmmo;

class ItemRespawnSystem : public CBaseGameSystem
{
public:
	ItemRespawnSystem(const char *name);
	void LevelInitPreEntity();
	void LevelShutdownPreEntity();
	void Think();

	void AddItem( ICItem *pItem );
	void RemoveItem( ICItem *pItem );
	bool FindItem( ICItem *pItem );

private:
	void ManageObjectRelocation();
	
private:
	CUtlVector<ICItem *> m_Item;
};

extern ItemRespawnSystem g_ItemRespawnSystem;

#endif
