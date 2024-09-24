//========= Copyright ?1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
// $NoKeywords: $
//=============================================================================//

#ifndef EFFECTS_H
#define EFFECTS_H

#ifdef _WIN32
#pragma once
#endif

class CEntity;
class CBaseEntity;
class Vector;


//-----------------------------------------------------------------------------
// The rotor wash shooter. It emits gibs when pushed by a rotor wash
//-----------------------------------------------------------------------------
class IRotorWashShooter
		{
				public:
				virtual CBaseEntity *DoWashPush( float flWashStartTime, const Vector &vecForce ) = 0;
		};


//-----------------------------------------------------------------------------
// Gets at the interface if the entity supports it
//-----------------------------------------------------------------------------
IRotorWashShooter *GetRotorWashShooter( CEntity *pEntity );


#endif // EFFECTS_H