#ifndef _INCLUDE_GRENADE_FRAG_H
#define _INCLUDE_GRENADE_FRAG_H
#pragma once

#include <CEntity.h>

class CE_Grenade;
struct edict_t;

CE_Grenade *Fraggrenade_Create( const Vector &position, const QAngle &angles, const Vector &velocity, const AngularImpulse &angVelocity, CBaseEntity *pOwner, float timer, bool combineSpawned );
bool	Fraggrenade_WasPunted( const CBaseEntity *pEntity );
bool	Fraggrenade_WasCreatedByCombine( const CBaseEntity *pEntity );

#endif // _INCLUDE_GRENADE_FRAG_H
