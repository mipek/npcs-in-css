//========== Copyright © 2005, Valve Corporation, All rights reserved. ========
//
// Purpose:
//
//=============================================================================

#ifndef GAMEWEAPONMANAGER_H
#define GAMEWEAPONMANAGER_H

#if defined( _WIN32 )
#pragma once
#endif

class CCombatWeapon;
class CEntity;

void CE_CreateWeaponManager( const char *pWeaponName, int iMaxPieces );

class CBaseCombatWeapon;

void WeaponManager_AmmoMod( CCombatWeapon *pWeapon );

void WeaponManager_AddManaged( CEntity *pWeapon );
void WeaponManager_RemoveManaged( CEntity *pWeapon );

#endif // GAMEWEAPONMANAGER_H
