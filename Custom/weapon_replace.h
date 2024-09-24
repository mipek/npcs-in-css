
#ifndef NPCS_IN_CSS_WEAPON_REPLACE_H
#define NPCS_IN_CSS_WEAPON_REPLACE_H

class CCombatWeapon;

#define WEAPON_SMG1_REPLACE			weapon_ak47
#define WEAPON_AR2_REPLACE			weapon_p90
#define WEAPON_SHOTGUN_REPLACE		weapon_m3
#define WEAPON_RPG_REPLACE			weapon_flashbang
#define WEAPON_STUNSTICK_REPLACE	weapon_knife
#define WEAPON_ALYXGUN_REPLACE		weapon_p228
#define WEAPON_PISTOL_REPLACE		weapon_deagle

#define WEAPON_SMG1_REPLACE_NAME		"weapon_ak47"
#define WEAPON_AR2_REPLACE_NAME			"weapon_p90"
#define WEAPON_SHOTGUN_REPLACE_NAME		"weapon_m3"
#define WEAPON_RPG_REPLACE_NAME			"weapon_flashbang"
#define WEAPON_STUNSTICK_REPLACE_NAME	"weapon_knife"
#define WEAPON_ALYXGUN_REPLACE_NAME		"weapon_p228"
#define WEAPON_PISTOL_REPLACE_NAME		"weapon_deagle"

const char *GetWeaponReplaceName(const char *name);
const char *NPC_WeaponReplace(const char *classname);

void PreWeaponReplace(CCombatWeapon *pWeapon);
void PreWeaponReplace(const char *name);

void PostWeaponReplace();

#endif //NPCS_IN_CSS_WEAPON_REPLACE_H
