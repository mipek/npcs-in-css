#include "CGamePlayerEquip.h"
#include "CPlayer.h"

DECLARE_DETOUR(EquipPlayer, Template_CGamePlayerEquip);
DECLARE_DEFAULTHANDLER_DETOUR_void(Template_CGamePlayerEquip, EquipPlayer, (CBaseEntity *pEntity), (pEntity));

DEFINE_PROP(m_weaponNames, Template_CGamePlayerEquip);
DEFINE_PROP(m_weaponCount, Template_CGamePlayerEquip);

class CE_CGamePlayerEquip: public Template_CGamePlayerEquip
{
public:
	CE_DECLARE_CLASS(CE_CGamePlayerEquip, Template_CGamePlayerEquip);
	
public:
	virtual void EquipPlayer(CBaseEntity *pEntity)
	{
		CPlayer *pPlayer = ToBasePlayer(CEntity::Instance(pEntity));

		if ( !pPlayer )
			return;

		string_t *weaponNames = reinterpret_cast<string_t*>(m_weaponNames.ptr);
		int *weaponCount = reinterpret_cast<int*>(m_weaponCount.ptr);
		for ( int i = 0; i < MAX_EQUIP; i++ )
		{
			if ( !weaponNames[i] )
				break;

			for ( int j = 0; j < weaponCount[i]; j++ )
			{
				char szGiveWeapon[64];
				if(ReplaceInvalidItemName(STRING(weaponNames[j]), szGiveWeapon, sizeof(szGiveWeapon))) {
 					pPlayer->GiveNamedItem(szGiveWeapon);
				}
			}
		}
	}

private:
	// Replace invalid weapons with valid ones. For example weapon_pistol is not a valid csgo weapon but weapon_usp is.
	// Returns false if we couldn't find any good counterpart.
	static bool ReplaceInvalidItemName(const char *pszReal, char *out, size_t maxlen)
	{
		// CE_TODO: put this in some config file.
		if(strcmp(pszReal, "weapon_pistol") == 0) {
			strncpy(out, "weapon_usp", maxlen);
			return true;
		} else if(strcmp(pszReal, "weapon_crowbar") == 0) {
			strncpy(out, "weapon_knife", maxlen);
			return true;
		} else if(strcmp(pszReal, "weapon_ar2") == 0) {
			strncpy(out, "weapon_ak47", maxlen);
			return true;
		} else if(strcmp(pszReal, "weapon_crossbow") == 0) {
			strncpy(out, "weapon_famas", maxlen);
			return true;
		} else if(strcmp(pszReal, "weapon_rpg") == 0) {
			strncpy(out, "weapon_awp", maxlen);
			return true;
		} else if(strcmp(pszReal, "weapon_shotgun") == 0) {
			strncpy(out, "weapon_m3", maxlen);
			return true;
		} else if(strcmp(pszReal, "item_ammo_pistol") == 0) {
			strncpy(out, "ammo_45acp", maxlen);
			return true;
		} else if(strcmp(pszReal, "item_smg1") == 0) {
			strncpy(out, "weapon_mp5navy", maxlen);
			return true;
		} else if(strcmp(pszReal, "item_ammo_smg1") == 0) {
			strncpy(out, "ammo_9mm", maxlen);
			return true;
		} else if(strcmp(pszReal, "item_ammo_ar2") == 0) {
			strncpy(out, "ammo_762mm", maxlen);
			return true;
		} else if(strcmp(pszReal, "item_ammo_crossbow") == 0) {
			strncpy(out, "ammo_556mm", maxlen);
			return true;
		} else if(strcmp(pszReal, "item_rpg_round") == 0) {
			strncpy(out, "ammo_338mag", maxlen);
			return true;
		} else if(strcmp(pszReal, "item_box_buckshot") == 0) {
			strncpy(out, "ammo_buckshot", maxlen);
			return true;
		} else if( strcmp(pszReal, "weapon_melee") == 0
				|| strcmp(pszReal, "weapon_medkit") == 0
				|| strcmp(pszReal, "weapon_healer") == 0
				|| strcmp(pszReal, "weapon_physcannon") == 0
				|| strcmp(pszReal, "item_ammo_smg1_grenade") == 0
				|| strcmp(pszReal, "item_ammo_ar2_altfire") == 0) {
			/* no matching counterpart, dont give any item.. */
			return false;
		}

		// name looks good!
		strncpy(out, pszReal, maxlen);
		return true;
	}
};
CE_LINK_ENTITY_TO_CLASS(CGamePlayerEquip, CE_CGamePlayerEquip);