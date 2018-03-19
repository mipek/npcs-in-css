/**
* =============================================================================
* CEntity Entity Handling Framework
* Copyright (C) 2011 Matt Woodrow.  All rights reserved.
* =============================================================================
*
* This program is free software; you can redistribute it and/or modify it under
* the terms of the GNU General Public License, version 3.0, as published by the
* Free Software Foundation.
* 
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
* FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
* details.
*
* You should have received a copy of the GNU General Public License along with
* this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _INCLUDE_CUTIL_H_
#define _INCLUDE_CUTIL_H_

#include "extension.h"
#include "CEntity.h"
#include "networkvar.h"
#include "shake.h"

#define CE_MAX_PLAYERS 65

class VMatrix;


//-----------------------------------------------------------------------------
// These are inlined for backwards compatibility
//-----------------------------------------------------------------------------
inline float UTIL_Approach( float target, float value, float speed )
{
	return Approach( target, value, speed );
}

inline float UTIL_ApproachAngle( float target, float value, float speed )
{
	return ApproachAngle( target, value, speed );
}

inline float UTIL_AngleDistance( float next, float cur )
{
	return AngleDistance( next, cur );
}

inline float UTIL_AngleMod(float a)
{
	return anglemod(a);
}

inline float UTIL_AngleDiff( float destAngle, float srcAngle )
{
	return AngleDiff( destAngle, srcAngle );
}


// make this a fixed size so it just sits on the stack
#define MAX_SPHERE_QUERY	512
class CEntitySphereQuery
{
public:
	// currently this builds the list in the constructor
	// UNDONE: make an iterative query of ISpatialPartition so we could
	// make queries like this optimal
	CEntitySphereQuery( const Vector &center, float radius, int flagMask=0 );
	CEntity *GetCurrentEntity();
	inline void NextEntity() { m_listIndex++; }

private:
	int			m_listIndex;
	int			m_listCount;
	CEntity		*m_pList[MAX_SPHERE_QUERY];
};


//-----------------------------------------------------------------------------
// class CFlaggedEntitiesEnum
//-----------------------------------------------------------------------------
// enumerate entities that match a set of edict flags into a static array
class CFlaggedEntitiesEnum : public IPartitionEnumerator
{
public:
	CFlaggedEntitiesEnum( CEntity **pList, int listMax, int flagMask );

	// This gets called	by the enumeration methods with each element
	// that passes the test.
	virtual IterationRetval_t EnumElement( IHandleEntity *pHandleEntity );
	
	int GetCount() { return m_count; }
	bool AddToList( CEntity *pEntity );
	
private:
	CEntity			**m_pList;
	int				m_listMax;
	int				m_flagMask;
	int				m_count;
};


void UTIL_ClearTrace( trace_t &trace );

// Pass in an array of pointers and an array size, it fills the array and returns the number inserted
int			UTIL_EntitiesInBox( const Vector &mins, const Vector &maxs, CFlaggedEntitiesEnum *pEnum  );
int			UTIL_EntitiesAlongRay( const Ray_t &ray, CFlaggedEntitiesEnum *pEnum  );
int			UTIL_EntitiesInSphere( const Vector &center, float radius, CFlaggedEntitiesEnum *pEnum  );

inline int UTIL_EntitiesInBox( CEntity **pList, int listMax, const Vector &mins, const Vector &maxs, int flagMask )
{
	CFlaggedEntitiesEnum boxEnum( pList, listMax, flagMask );
	return UTIL_EntitiesInBox( mins, maxs, &boxEnum );
}

inline int UTIL_EntitiesAlongRay( CEntity **pList, int listMax, const Ray_t &ray, int flagMask )
{
	CFlaggedEntitiesEnum rayEnum( pList, listMax, flagMask );
	return UTIL_EntitiesAlongRay( ray, &rayEnum );
}

inline int UTIL_EntitiesInSphere( CEntity **pList, int listMax, const Vector &center, float radius, int flagMask )
{
	CFlaggedEntitiesEnum sphereEnum( pList, listMax, flagMask );
	return UTIL_EntitiesInSphere( center, radius, &sphereEnum );
}





class CE_CTraceFilterSimple : public CTraceFilter
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS_NOBASE( CE_CTraceFilterSimple );
	
	CE_CTraceFilterSimple( const IHandleEntity *passentity, int collisionGroup );
	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask );
	virtual void SetPassEntity( const IHandleEntity *pPassEntity ) { m_pPassEnt = pPassEntity; }
	virtual void SetCollisionGroup( int iCollisionGroup ) { m_collisionGroup = iCollisionGroup; }

	const IHandleEntity *GetPassEntity( void ){ return m_pPassEnt;}

private:
	const IHandleEntity *m_pPassEnt;
	int m_collisionGroup;
};


class CTraceFilterOnlyNPCsAndPlayer : public CE_CTraceFilterSimple
{
public:
	CTraceFilterOnlyNPCsAndPlayer( const IHandleEntity *passentity, int collisionGroup )
		: CE_CTraceFilterSimple( passentity, collisionGroup )
	{
	}

	virtual TraceType_t	GetTraceType() const
	{
		return TRACE_ENTITIES_ONLY;
	}

	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask );
};

class CTraceFilterSimpleList : public CE_CTraceFilterSimple
{
public:
	CTraceFilterSimpleList( int collisionGroup );
	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask );

	void	AddEntityToIgnore( IHandleEntity *pEntity );
protected:
	CUtlVector<IHandleEntity*>	m_PassEntities;
};

class CTraceFilterSkipTwoEntities : public CE_CTraceFilterSimple
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS( CTraceFilterSkipTwoEntities, CE_CTraceFilterSimple );
	
	CTraceFilterSkipTwoEntities( const IHandleEntity *passentity, const IHandleEntity *passentity2, int collisionGroup );
	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask );
	virtual void SetPassEntity2( const IHandleEntity *pPassEntity2 ) { m_pPassEnt2 = pPassEntity2; }

private:
	const IHandleEntity *m_pPassEnt2;
};


//-----------------------------------------------------------------------------
// Purpose: Custom trace filter used for NPC LOS traces
//-----------------------------------------------------------------------------
class CTraceFilterLOS : public CTraceFilterSkipTwoEntities
{
public:
	CTraceFilterLOS( IHandleEntity *pHandleEntity, int collisionGroup, IHandleEntity *pHandleEntity2 = NULL );
	bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask );
};

class CTraceFilterSkipClassname : public CE_CTraceFilterSimple
{
public:
	CTraceFilterSkipClassname( const IHandleEntity *passentity, const char *pchClassname, int collisionGroup );
	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask );

private:

	const char *m_pchClassname;
};


class CPlayer;

class CEntityLookup;

bool IsValidEdict(edict_t *pEdict);
bool IsValidEntity(edict_t *pEdict);

inline CEntity *CE_EntityFromEntityHandle( IHandleEntity *pHandleEntity )
{
	if ( staticpropmgr->IsStaticProp( pHandleEntity ) )
		return NULL;

	IServerUnknown *pUnk = (IServerUnknown*)pHandleEntity;
	return CEntityLookup::Instance(pUnk->GetBaseEntity());
}

inline CEntity *CE_EntityFromEntityHandle( const IHandleEntity *pConstHandleEntity )
{
	IHandleEntity *pHandleEntity = const_cast<IHandleEntity*>(pConstHandleEntity);

	if ( staticpropmgr->IsStaticProp( pHandleEntity ) )
		return NULL;

	IServerUnknown *pUnk = (IServerUnknown*)pHandleEntity;
	return CEntityLookup::Instance(pUnk->GetBaseEntity());
}

inline CEntity *CE_EntityFromEntityHandle_NoStatic( IHandleEntity *pConstHandleEntity )
{
	IHandleEntity *pHandleEntity = const_cast<IHandleEntity*>(pConstHandleEntity);

	IServerUnknown *pUnk = (IServerUnknown*)pHandleEntity;
	return CEntityLookup::Instance(pUnk->GetBaseEntity());
}

inline edict_t* INDEXENT( int iEdictNum )		
{ 
	return engine->PEntityOfEntIndex(iEdictNum); 
}



// Derive a class from this if you want to filter entity list searches
abstract_class IEntityFindFilter
{
public:
	virtual bool ShouldFindEntity( CBaseEntity *pEntity ) = 0;
	virtual CBaseEntity *GetFilterResult( void ) = 0;
};


void UTIL_TraceLine( const Vector& vecAbsStart, const Vector& vecAbsEnd, unsigned int mask, 
					 const IHandleEntity *ignore, int collisionGroup, trace_t *ptr );

void UTIL_TraceLine( const Vector& vecAbsStart, const Vector& vecAbsEnd, unsigned int mask, 
					 ITraceFilter *pFilter, trace_t *ptr );

void UTIL_TraceHull( const Vector &vecAbsStart, const Vector &vecAbsEnd, const Vector &hullMin, 
					 const Vector &hullMax,	unsigned int mask, ITraceFilter *pFilter, trace_t *ptr );

void UTIL_TraceHull( const Vector &vecAbsStart, const Vector &vecAbsEnd, const Vector &hullMin, 
					 const Vector &hullMax,	unsigned int mask, const IHandleEntity *ignore, 
					 int collisionGroup, trace_t *ptr );

void UTIL_TraceEntity( CEntity *pEntity, const Vector &vecAbsStart, const Vector &vecAbsEnd, unsigned int mask, trace_t *ptr );

void UTIL_TraceEntity( CEntity *pEntity, const Vector &vecAbsStart, const Vector &vecAbsEnd, 
					  unsigned int mask, const IHandleEntity *pIgnore, int nCollisionGroup, trace_t *ptr );

void UTIL_TraceLineFilterEntity( CBaseEntity *pEntity, const Vector &vecAbsStart, const Vector &vecAbsEnd, 
					   unsigned int mask, int nCollisionGroup, trace_t *ptr );

void UTIL_SetOrigin(CEntity *entity, const Vector &vecOrigin, bool bFireTriggers = false);

bool UTIL_EntityHasMatchingRootParent( CEntity *pRootParent, CEntity *pEntity );

float UTIL_VecToYaw( const Vector &vec );

bool NPC_CheckBrushExclude( CEntity *pEntity, CEntity *pBrush );

CPlayer	*UTIL_PlayerByIndex( int playerIndex );

float GetCurrentGravity();

float UTIL_ScaleForGravity( float desiredGravity );

void UTIL_DecalTrace( trace_t *pTrace, char const *decalName );

void UTIL_ScreenShake( const Vector &center, float amplitude, float frequency, float duration, float radius, ShakeCommand_t eCommand, bool bAirShake=false);

void UTIL_CreateDust(const Vector &pos, const QAngle &angle, float size, float speed);

bool UTIL_ShouldShowBlood( int color );

void UTIL_BloodDrips( const Vector &origin, const Vector &direction, int color, int amount );

void UTIL_Smoke( const Vector &origin, const float scale, const float framerate );

void UTIL_BloodImpact( const Vector &pos, const Vector &dir, int color, int amount );

void UTIL_ImpactTrace( trace_t *pTrace, int iDamageType, const char *pCustomImpactName = NULL );

bool UTIL_IsValidEntity( CEntity *pEnt );

void SpawnBlood(Vector vecSpot, const Vector &vecDir, int bloodColor, float flDamage);

const char *nexttoken(char *token, const char *str, char sep);

void UTIL_BloodSpray( const Vector &pos, const Vector &dir, int color, int amount, int flags );

void UTIL_BloodDecalTrace( trace_t *pTrace, int bloodColor );

bool UTIL_IsLowViolence( void );

Vector UTIL_YawToVector( float yaw );

CEntity *CreateRagGib( const char *szModel, const Vector &vecOrigin, const QAngle &vecAngles, const Vector &vecForce, float flFadeTime, bool bShouldIgnite = false );

void UTIL_SetSize( CBaseEntity *pEnt, const Vector &vecMin, const Vector &vecMax );

void Sound_StopSound( int entindex, const char *soundname );

void Sound_StopSound( int iEntIndex, int iChannel, const char *pSample );

bool UTIL_CheckBottom( CEntity *pEntity, ITraceFilter *pTraceFilter, float flStepSize );

void PropBreakableCreateAll( int modelindex, IPhysicsObject *pPhysics, const breakablepropparams_t &params, CBaseEntity *pEntity, int iPrecomputedBreakableCount, bool bIgnoreGibLImit, bool defaultLocation = true );

void PropBreakableCreateAll( int modelindex, IPhysicsObject *pPhysics, const Vector &origin, const QAngle &angles, const Vector &velocity, const AngularImpulse &angularVelocity, float impactEnergyScale, float burstScale, int collisionGroup, CBaseEntity *pEntity = NULL, bool defaultLocation = true );

int PropBreakablePrecacheAll( string_t modelName );

bool StandardFilterRules( IHandleEntity *pHandleEntity, int fContentsMask );
bool PassServerEntityFilter( const IHandleEntity *pTouch, const IHandleEntity *pPass );

void UTIL_Remove(CEntity *pEntity);
void UTIL_Remove(IServerNetworkable *oldObj);

CPlayer *UTIL_GetNearestPlayer( const Vector &origin );

CPlayer *UTIL_GetNearestVisiblePlayer(CEntity *pLooker, int mask = MASK_SOLID_BRUSHONLY);

float UTIL_VecToPitch(const Vector &vec);

void UTIL_PredictedPosition( CEntity *pTarget, float flTimeDelta, Vector *vecPredictedPosition );

void UTIL_ScreenFade( CEntity *pEntity, const color32 &color, float fadeTime, float fadeHold, int flags );

void UTIL_Bubbles( const Vector& mins, const Vector& maxs, int count );

float UTIL_WaterLevel( const Vector &position, float minz, float maxz );

float UTIL_FindWaterSurface( const Vector &position, float minz, float maxz );

int	UTIL_DropToFloor( CEntity *pEntity, unsigned int mask, CEntity *pIgnore = NULL );

int ENTINDEX( CBaseEntity *pEnt );

void		UTIL_StringToVector( float *pVector, const char *pString );
void		UTIL_StringToIntArray( int *pVector, int count, const char *pString );
void		UTIL_StringToFloatArray( float *pVector, int count, const char *pString );
void		UTIL_StringToColor32( color32 *color, const char *pString );

char *UTIL_VarArgs( const char *format, ... );

bool UTIL_ItemCanBeTouchedByPlayer( CEntity *pItem, CPlayer *pPlayer );

AngularImpulse WorldToLocalRotation( const VMatrix &localToWorld, const Vector &worldAxis, float rotation );

inline int UTIL_PointContents( const Vector &vec )
{
	return enginetrace->GetPointContents( vec );
}

inline bool FStrEq(const char *sz1, const char *sz2)
{
	return ( sz1 == sz2 || stricmp(sz1, sz2) == 0 );
}


//---------------------------------------------------------
//---------------------------------------------------------
inline float UTIL_DistApprox2D( const Vector &vec1, const Vector &vec2 )
{
	float dx;
	float dy;

	dx = vec1.x - vec2.x;
	dy = vec1.y - vec2.y;

	return fabs(dx) + fabs(dy);
}

inline float UTIL_DistApprox( const Vector &vec1, const Vector &vec2 )
{
	float dx;
	float dy;
	float dz;

	dx = vec1.x - vec2.x;
	dy = vec1.y - vec2.y;
	dz = vec1.z - vec2.z;

	return fabs(dx) + fabs(dy) + fabs(dz);
}

class EntityMatrix : public VMatrix
{
public:
	void InitFromEntity( CEntity *pEntity, int iAttachment=0 );
	void InitFromEntityLocal( CEntity *entity );

	inline Vector LocalToWorld( const Vector &vVec ) const
	{
		return VMul4x3( vVec );
	}

	inline Vector WorldToLocal( const Vector &vVec ) const
	{
		return VMul4x3Transpose( vVec );
	}

	inline Vector LocalToWorldRotation( const Vector &vVec ) const
	{
		return VMul3x3( vVec );
	}

	inline Vector WorldToLocalRotation( const Vector &vVec ) const
	{
		return VMul3x3Transpose( vVec );
	}
};

// Dot products for view cone checking
#define VIEW_FIELD_FULL		(float)-1.0 // +-180 degrees
#define	VIEW_FIELD_WIDE		(float)-0.7 // +-135 degrees 0.1 // +-85 degrees, used for full FOV checks 
#define	VIEW_FIELD_NARROW	(float)0.7 // +-45 degrees, more narrow check used to set up ranged attacks
#define	VIEW_FIELD_ULTRA_NARROW	(float)0.9 // +-25 degrees, more narrow check used to set up ranged attacks


CEntity *CreateEntityByName(const char *entityname);
void DispatchSpawn(CBaseEntity *pEntity);

int GetParticleSystemIndex( const char *pParticleSystemName );
void DispatchParticleEffect( const char *pszParticleName, Vector vecOrigin, QAngle vecAngles, CEntity *pEntity = NULL );
void DispatchParticleEffect( int iEffectIndex, Vector vecOrigin, Vector vecStart, QAngle vecAngles, CEntity *pEntity = NULL );

void PrecacheParticleSystem( const char *pParticleSystemName );

bool UTIL_IsMasterTriggered(string_t sMaster, CEntity *pActivator);

edict_t		*UTIL_FindClientInPVS( edict_t *pEdict );
edict_t		*UTIL_FindClientInVisibilityPVS( edict_t *pEdict );

bool		UTIL_ClientPVSIsExpanded();

int SENTENCEG_Lookup(const char *sample);
int SENTENCEG_PlayRndSz(edict_t *entity, const char *szrootname, float volume, soundlevel_t soundlevel, int flags, int pitch);
void SENTENCEG_PlaySentenceIndex( edict_t *entity, int iSentenceIndex, float volume, soundlevel_t soundlevel, int flags, int pitch );

void *UTIL_FunctionFromName( datamap_t *pMap, const char *pName );

bool UTIL_HudHintText( CBaseEntity *pEntity, const char *pMessage );
bool UTIL_HudHintTextAll( const char *pMessage );

bool FASTCALL IsBoxIntersectingRay( const Vector& boxMin, const Vector& boxMax, 
									const Vector& origin, const Vector& vecDelta, float flTolerance );

bool FASTCALL IsBoxIntersectingRay( const Vector& boxMin, const Vector& boxMax, 
 									const Ray_t& ray, float flTolerance = 0.0f );

bool IsBoxIntersectingBox( const Vector& boxMin1, const Vector& boxMax1, 
	const Vector& boxMin2, const Vector& boxMax2 );

void UTIL_ClipPunchAngleOffset( QAngle &in, const QAngle &punch, const QAngle &clip );

// Particle effect tracer
void		UTIL_ParticleTracer( const char *pszTracerEffectName, const Vector &vecStart, const Vector &vecEnd, int iEntIndex = 0, int iAttachment = 0, bool bWhiz = false );

// Old style, non-particle system, tracers
void		UTIL_Tracer( const Vector &vecStart, const Vector &vecEnd, int iEntIndex = 0, int iAttachment = TRACER_DONT_USE_ATTACHMENT, float flVelocity = 0, bool bWhiz = false, const char *pCustomTracerName = NULL, int iParticleID = 0 );

#endif // _INCLUDE_UTIL_H_
