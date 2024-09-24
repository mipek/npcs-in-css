#include "weapon_replace.h"
#include "CCombatWeapon.h"
#include "weapon_rpg_replace.h"
#include "weapon_physcannon_replace.h"
#include "sm_stringhashmap.h"

class WeaponReplaceSystem : public CBaseGameSystem
{
public:
	WeaponReplaceSystem(const char *name): CBaseGameSystem(name)
	{
	}
	const char *GetReplaceName(const char *name)
	{
		auto result = m_pWeaponReplaceData.find(name);
		if (result.found())
			return result->value;
		return nullptr;
	}

	void LevelInitPreEntity()
	{
		AddReplace("weapon_smg1",WEAPON_SMG1_REPLACE_NAME);
		AddReplace("weapon_shotgun",WEAPON_SHOTGUN_REPLACE_NAME);
		AddReplace("weapon_ar2",WEAPON_AR2_REPLACE_NAME);
		AddReplace("weapon_357", WEAPON_PISTOL_REPLACE_NAME);
		AddReplace("weapon_crossbow","weapon_awp");
		AddReplace("weapon_rpg","weapon_flashbang");
		AddReplace("weapon_slam","weapon_hegrenade");
		AddReplace("weapon_frag","weapon_hegrenade");
		AddReplace("weapon_crowbar","weapon_knife");
		AddReplace("weapon_pistol","weapon_usp");
		AddReplace("weapon_physcannon","weapon_c4");
	}

	void LevelShutdownPostEntity()
	{
		m_pWeaponReplaceData.clear();
	}

	void SDKInitPost()
	{
		LevelInitPreEntity();
	}

	void SDKShutdown()
	{
		m_pWeaponReplaceData.clear();
	}

private:
	void AddReplace(const char *name_to_replace, const char *replace_name)
	{
		m_pWeaponReplaceData.replace(name_to_replace, replace_name);
	}

private:
	StringHashMap<const char*> m_pWeaponReplaceData;
};

WeaponReplaceSystem g_WeaponReplaceSystem("WeaponReplaceSystem");

const char *GetWeaponReplaceName(const char *name)
{
	return g_WeaponReplaceSystem.GetReplaceName(name);
}

const char *NPC_WeaponReplace(const char *szValue)
{
	const char *newszValue = szValue;
	if(FStrEq(szValue, "weapon_ar2"))
		newszValue = WEAPON_AR2_REPLACE_NAME;
	else if(FStrEq(szValue, "weapon_shotgun"))
		newszValue = WEAPON_SHOTGUN_REPLACE_NAME;
	else if(FStrEq(szValue, "weapon_smg1"))
		newszValue = WEAPON_SMG1_REPLACE_NAME;
	else if(FStrEq(szValue, "weapon_stunstick"))
		newszValue = WEAPON_STUNSTICK_REPLACE_NAME;
	else if(FStrEq(szValue, "weapon_alyxgun"))
		newszValue = WEAPON_ALYXGUN_REPLACE_NAME;
	else if(FStrEq(szValue, "weapon_pistol"))
		newszValue = WEAPON_PISTOL_REPLACE_NAME;
	else
		META_CONPRINTF("%s: invalid classname to replace '%s'\n", __func__, szValue);
	//newszValue = WEAPON_SMG1_REPLACE_NAME;
	return newszValue;
}

void PreWeaponReplace(CCombatWeapon *pWeapon)
{
	CWeaponRPG *rpg = ToCWeaponRPG(pWeapon);
	if(rpg)
	{
		CWeaponRPG::IS_REPLACE_SPAWN = true;
		return;
	}
	CWeaponPhysCannon *physcannon = ToCWeaponPhysCannon(pWeapon);
	if(physcannon)
	{
		CWeaponPhysCannon::IS_REPLACE_SPAWN = true;
		return;
	}
}

void PreWeaponReplace(const char *name)
{
	if (!Q_stricmp( "weapon_rpg", name) )
	{
		CWeaponRPG::IS_REPLACE_SPAWN = true;
	} else if (!Q_stricmp( "weapon_physcannon", name) ) {
		CWeaponPhysCannon::IS_REPLACE_SPAWN = true;
	}
}

void PostWeaponReplace()
{
	CWeaponRPG::IS_REPLACE_SPAWN = false;
	CWeaponPhysCannon::IS_REPLACE_SPAWN = false;
}