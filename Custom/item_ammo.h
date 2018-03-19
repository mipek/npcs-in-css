#ifndef _include_item_ammo_
#define _include_item_ammo_

class CPlayer;
bool ITEM_GiveAmmo( CPlayer *pPlayer, float flCount, const char *pszAmmoName);

void RegisterHL2AmmoTypes();

#endif //_include_item_ammo_