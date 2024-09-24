
#ifndef NPC_ATTACKCHOPPER_H
#define NPC_ATTACKCHOPPER_H

#ifdef _WIN32
#pragma once
#endif

#include <mathlib/mathlib.h>

class CEntity;

//-----------------------------------------------------------------------------
// Creates an avoidance sphere
//-----------------------------------------------------------------------------
//CBaseEntity *CreateHelicopterAvoidanceSphere( CBaseEntity *pParent, int nAttachment, float flRadius, bool bAvoidBelow = false );

// Chopper gibbage
void Chopper_BecomeChunks( CEntity *pChopper );
void Chopper_CreateChunk( CEntity *pChopper, const Vector &vecChunkPos, const QAngle &vecChunkAngles, const char *pszChunkName, bool bSmall );
void Chopper_PrecacheChunks( CEntity *pChopper );

#endif // NPC_ATTACKCHOPPER_H