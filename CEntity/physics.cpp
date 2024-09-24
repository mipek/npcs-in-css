
#include "CEntity.h"
#include "physics.h"
#include "vphysics/friction.h"
#include "solidsetdefaults.h"
#include "vphysics/object_hash.h"
#include "fmtstr.h"
#include "GameSystem.h"
#include "vehicle_sounds.h"
#include "callqueue.h"
#include "physics_shared.h"
#include "physics_collisionevent.h"



// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

extern CCallQueue *g_PostSimulationQueue;



const objectparams_t g_PhysDefaultObjectParams =
		{
				NULL,
				1.0, //mass
				1.0, // inertia
				0.1f, // damping
				0.1f, // rotdamping
				0.05f, // rotIntertiaLimit
				"DEFAULT",
				NULL,// game data
				0.f, // volume (leave 0 if you don't have one or call physcollision->CollideVolume() to compute it)
				1.0f, // drag coefficient
				true,// enable collisions?
		};


void CSolidSetDefaults::ParseKeyValue( void *pData, const char *pKey, const char *pValue )
{
	if ( !Q_stricmp( pKey, "contents" ) )
	{
		m_contentsMask = atoi( pValue );
	}
}

void CSolidSetDefaults::SetDefaults( void *pData )
{
	solid_t *pSolid = (solid_t *)pData;
	pSolid->params = g_PhysDefaultObjectParams;
	m_contentsMask = CONTENTS_SOLID;
}

CSolidSetDefaults g_SolidSetup;

float PhysGetEntityMass( CEntity *pEntity )
{
	IPhysicsObject *pList[VPHYSICS_MAX_OBJECT_LIST_COUNT];
	int physCount = pEntity->VPhysicsGetObjectList( pList, ARRAYSIZE(pList) );
	float otherMass = 0;
	for ( int i = 0; i < physCount; i++ )
	{
		otherMass += pList[i]->GetMass();
	}

	return otherMass;
}


void PhysForceClearVelocity( IPhysicsObject *pPhys )
{
	IPhysicsFrictionSnapshot *pSnapshot = pPhys->CreateFrictionSnapshot();
	// clear the velocity of the rigid body
	Vector vel;
	AngularImpulse angVel;
	vel.Init();
	angVel.Init();
	pPhys->SetVelocity( &vel, &angVel );
	// now clear the "strain" stored in the contact points
	while ( pSnapshot->IsValid() )
	{
		pSnapshot->ClearFrictionForce();
		pSnapshot->RecomputeFriction();
		pSnapshot->NextFrictionData();
	}
	pPhys->DestroyFrictionSnapshot( pSnapshot );
}


void PhysComputeSlideDirection( IPhysicsObject *pPhysics, const Vector &inputVelocity, const AngularImpulse &inputAngularVelocity,
								Vector *pOutputVelocity, Vector *pOutputAngularVelocity, float minMass )
{
	Vector velocity = inputVelocity;
	AngularImpulse angVel = inputAngularVelocity;
	Vector pos;

	IPhysicsFrictionSnapshot *pSnapshot = pPhysics->CreateFrictionSnapshot();
	while ( pSnapshot->IsValid() )
	{
		IPhysicsObject *pOther = pSnapshot->GetObject( 1 );
		if ( !pOther->IsMoveable() || pOther->GetMass() > minMass )
		{
			Vector normal;
			pSnapshot->GetSurfaceNormal( normal );

			// BUGBUG: Figure out the correct rotation clipping equation
			if ( pOutputAngularVelocity )
			{
				angVel = normal * DotProduct( angVel, normal );
#if 0
				pSnapshot->GetContactPoint( point );
				Vector point, dummy;
				AngularImpulse angularClip, clip2;

				pPhysics->CalculateVelocityOffset( normal, point, dummy, angularClip );
				VectorNormalize( angularClip );
				float proj = DotProduct( angVel, angularClip );
				if ( proj > 0 )
				{
					angVel -= angularClip * proj;
				}
				CrossProduct( angularClip, normal, clip2 );
				proj = DotProduct( angVel, clip2 );
				if ( proj > 0 )
				{
					angVel -= clip2 * proj;
				}
				//NDebugOverlay::Line( point, point - normal * 20, 255, 0, 0, true, 0.1 );
#endif
			}

			// Determine how far along plane to slide based on incoming direction.
			// NOTE: Normal points away from this object
			float proj = DotProduct( velocity, normal );
			if ( proj > 0.0f )
			{
				velocity -= normal * proj;
			}
		}
		pSnapshot->NextFrictionData();
	}
	pPhysics->DestroyFrictionSnapshot( pSnapshot );

	//NDebugOverlay::Line( pos, pos + unitVel * 20, 0, 0, 255, true, 0.1 );

	if ( pOutputVelocity )
	{
		*pOutputVelocity = velocity;
	}
	if ( pOutputAngularVelocity )
	{
		*pOutputAngularVelocity = angVel;
	}
}



AngularImpulse ComputeRotSpeedToAlignAxes( const Vector &testAxis, const Vector &alignAxis, const AngularImpulse &currentSpeed, float damping, float scale, float maxSpeed )
{
	Vector rotationAxis = CrossProduct( testAxis, alignAxis );

	// atan2() is well defined, so do a Dot & Cross instead of asin(Cross)
	float cosine = DotProduct( testAxis, alignAxis );
	float sine = VectorNormalize( rotationAxis );
	float angle = atan2( sine, cosine );

	angle = RAD2DEG(angle);
	AngularImpulse angular = rotationAxis * scale * angle;
	angular -= rotationAxis * damping * DotProduct( currentSpeed, rotationAxis );

	float len = VectorNormalize( angular );

	if ( len > maxSpeed )
	{
		len = maxSpeed;
	}

	return angular * len;
}




bool PhysModelParseSolid( solid_t &solid, CBaseEntity *pEntity, int modelIndex )
{
	return PhysModelParseSolidByIndex( solid, pEntity, modelIndex, -1 );
}

bool PhysModelParseSolidByIndex( solid_t &solid, CBaseEntity *pEntity, int modelIndex, int solidIndex )
{
	vcollide_t *pCollide = modelinfo->GetVCollide( modelIndex );
	if ( !pCollide )
		return false;

	bool parsed = false;

	memset( &solid, 0, sizeof(solid) );
	solid.params = g_PhysDefaultObjectParams;

	IVPhysicsKeyParser *pParse = physcollision->VPhysicsKeyParserCreate( pCollide->pKeyValues );
	while ( !pParse->Finished() )
	{
		const char *pBlock = pParse->GetCurrentBlockName();
		if ( !V_stricmp( pBlock, "solid" ) )
		{
			solid_t tmpSolid;
			memset( &tmpSolid, 0, sizeof(tmpSolid) );
			tmpSolid.params = g_PhysDefaultObjectParams;

			pParse->ParseSolid( &tmpSolid, &g_SolidSetup );

			if ( solidIndex < 0 || tmpSolid.index == solidIndex )
			{
				parsed = true;
				solid = tmpSolid;
				// just to be sure we aren't ever getting a non-zero solid by accident
				Assert( solidIndex >= 0 || solid.index == 0 );
				break;
			}
		}
		else
		{
			pParse->SkipBlock();
		}
	}
	physcollision->VPhysicsKeyParserDestroy( pParse );

	// collisions are off by default
	solid.params.enableCollisions = true;

	solid.params.pGameData = static_cast<void *>(pEntity);
	solid.params.pName = STRING(pEntity->GetModelName());
	return parsed;
}


void PhysRecheckObjectPair( IPhysicsObject *pObject0, IPhysicsObject *pObject1 )
{
	if ( !pObject0->IsStatic() )
	{
		pObject0->RecheckCollisionFilter();
	}
	if ( !pObject1->IsStatic() )
	{
		pObject1->RecheckCollisionFilter();
	}
}


void PhysEnableEntityCollisions( IPhysicsObject *pObject0, IPhysicsObject *pObject1 )
{
	if ( !pObject0 || !pObject1 )
		return;

	my_g_EntityCollisionHash->RemoveObjectPair( pObject0->GetGameData(), pObject1->GetGameData() );
	PhysRecheckObjectPair( pObject0, pObject1 );
}

void PhysDisableEntityCollisions( IPhysicsObject *pObject0, IPhysicsObject *pObject1 )
{
	if ( !pObject0 || !pObject1 )
		return;

	my_g_EntityCollisionHash->AddObjectPair( pObject0->GetGameData(), pObject1->GetGameData() );
	PhysRecheckObjectPair( pObject0, pObject1 );
}

IPhysicsObject *FindPhysicsObjectByName( const char *pName, CEntity *pErrorEntity )
{
	if ( !pName || !strlen(pName) )
		return NULL;

	CEntity *pEntity = NULL;
	IPhysicsObject *pBestObject = NULL;
	while (1)
	{
		pEntity = g_helpfunc.FindEntityByName( pEntity, pName );
		if ( !pEntity )
			break;
		if ( pEntity->VPhysicsGetObject() )
		{
			if ( pBestObject )
			{
				const char *pErrorName = pErrorEntity ? pErrorEntity->GetClassname() : "Unknown";
				Vector origin = pErrorEntity ? pErrorEntity->GetAbsOrigin() : vec3_origin;
				DevWarning("entity %s at %s has physics attachment to more than one entity with the name %s!!!\n", pErrorName, VecToString(origin), pName );
				while ( ( pEntity = g_helpfunc.FindEntityByName( pEntity, pName ) ) != NULL )
				{
					DevWarning("Found %s\n", pEntity->GetClassname() );
				}
				break;

			}
			pBestObject = pEntity->VPhysicsGetObject();
		}
	}
	return pBestObject;
}



struct impactsound_t
{
	void			*pGameData;
	int				entityIndex;
	int				soundChannel;
	float			volume;
	float			impactSpeed;
	unsigned short	surfaceProps;
	unsigned short	surfacePropsHit;
	Vector			origin;
};

// UNDONE: Use a sorted container and sort by volume/distance?
struct soundlist_t
{
	CUtlVector<impactsound_t>	elements;
	impactsound_t	&GetElement(int index) { return elements[index]; }
	impactsound_t	&AddElement() { return elements[elements.AddToTail()]; }
	int Count() { return elements.Count(); }
	void RemoveAll() { elements.RemoveAll(); }
};

struct breaksound_t
{
	Vector			origin;
	int				surfacePropsBreak;
};

struct vehiclescript_t
{
	string_t scriptName;
	vehicleparams_t params;
	vehiclesounds_t sounds;
};

class CPhysicsHook : public CValveBaseGameSystemPerFrame
{
public:
	bool FindOrAddVehicleScript( const char *pScriptName, vehicleparams_t *pVehicle, vehiclesounds_t *pSounds );

	soundlist_t m_impactSounds;
	CUtlVector<breaksound_t> m_breakSounds;

	CUtlVector<masscenteroverride_t>	m_massCenterOverrides;
	CUtlVector<vehiclescript_t>			m_vehicleScripts;

	float		m_impactSoundTime;
	bool		m_bPaused;
	bool		m_isFinalTick;
};


CPhysicsHook *g_PhysicsHook;

bool CPhysicsHook::FindOrAddVehicleScript( const char *pScriptName, vehicleparams_t *pVehicle, vehiclesounds_t *pSounds )
{
	bool bLoadedSounds = false;
	int index = -1;
	for ( int i = 0; i < m_vehicleScripts.Count(); i++ )
	{
		if ( !Q_stricmp(m_vehicleScripts[i].scriptName.ToCStr(), pScriptName) )
		{
			index = i;
			bLoadedSounds = true;
			break;
		}
	}

	if ( index < 0 )
	{
		byte *pFile = UTIL_LoadFileForMe( pScriptName, NULL );
		if ( pFile )
		{
			// new script, parse it and write to the table
			index = m_vehicleScripts.AddToTail();
			m_vehicleScripts[index].scriptName = AllocPooledString(pScriptName);
			m_vehicleScripts[index].sounds.Init();

			IVPhysicsKeyParser *pParse = physcollision->VPhysicsKeyParserCreate( (char *)pFile );
			while ( !pParse->Finished() )
			{
				const char *pBlock = pParse->GetCurrentBlockName();
				if ( !Q_stricmp( pBlock, "vehicle" ) )
				{
					pParse->ParseVehicle( &m_vehicleScripts[index].params, NULL );
				}
				else if ( !Q_stricmp( pBlock, "vehicle_sounds" ) )
				{
					// CE_TODO vehicle
					bLoadedSounds = true;
					CVehicleSoundsParser soundParser;
					pParse->ParseCustom( &m_vehicleScripts[index].sounds, &soundParser );
				}
				else
				{
					pParse->SkipBlock();
				}
			}
			physcollision->VPhysicsKeyParserDestroy( pParse );
			UTIL_FreeFile( pFile );
		}
	}

	if ( index >= 0 )
	{
		if ( pVehicle )
		{
			*pVehicle = m_vehicleScripts[index].params;
		}
		if ( pSounds )
		{
			// We must pass back valid data here!
			if ( bLoadedSounds == false )
				return false;

			*pSounds = m_vehicleScripts[index].sounds;
		}
		return true;
	}

	return false;
}


void AddImpactSound( soundlist_t &list, void *pGameData, int entityIndex, int soundChannel, IPhysicsObject *pObject, int surfaceProps, int surfacePropsHit, float volume, float impactSpeed )
{
	impactSpeed += 1e-4;
	for ( int i = list.Count()-1; i >= 0; --i )
	{
		impactsound_t &sound = list.GetElement(i);
		// UNDONE: Compare entity or channel somehow?
		// UNDONE: Doing one slot per entity is too noisy.  So now we use one slot per material
		// heuristic - after 4 impacts sounds in one frame, start merging everything
		if ( surfaceProps == sound.surfaceProps || list.Count() > 4 )
		{
			// UNDONE: Store instance volume separate from aggregate volume and compare that?
			if ( volume > sound.volume )
			{
				pObject->GetPosition( &sound.origin, NULL );
				sound.pGameData = pGameData;
				sound.entityIndex = entityIndex;
				sound.soundChannel = soundChannel;
				sound.surfacePropsHit = surfacePropsHit;
			}
			sound.volume += volume;
			sound.impactSpeed = MAX(impactSpeed,sound.impactSpeed);
			return;
		}
	}
	impactsound_t &sound = list.AddElement();
	sound.pGameData = pGameData;
	sound.entityIndex = entityIndex;
	sound.soundChannel = soundChannel;
	pObject->GetPosition( &sound.origin, NULL );
	sound.surfaceProps = surfaceProps;
	sound.surfacePropsHit = surfacePropsHit;
	sound.volume = volume;
	sound.impactSpeed = impactSpeed;
}


void PhysicsImpactSound( CBaseEntity *pEntity, IPhysicsObject *pPhysObject, int channel, int surfaceProps, int surfacePropsHit, float volume, float impactSpeed )
{
	AddImpactSound( g_PhysicsHook->m_impactSounds, pEntity, CEntity::Instance(pEntity)->entindex(), channel, pPhysObject, surfaceProps, surfacePropsHit, volume, impactSpeed );
}

void PostSimulation_SetVelocityEvent( IPhysicsObject *pPhysicsObject, const Vector &vecVelocity )
{
	pPhysicsObject->SetVelocity( &vecVelocity, NULL );
}

void PhysCallbackSetVelocity( IPhysicsObject *pPhysicsObject, const Vector &vecVelocity )
{
	Assert( physenv->IsInSimulation() );
	g_PostSimulationQueue->QueueCall( PostSimulation_SetVelocityEvent, pPhysicsObject, RefToVal(vecVelocity) );
}

void PhysDisableObjectCollisions( IPhysicsObject *pObject0, IPhysicsObject *pObject1 )
{
	if ( !pObject0 || !pObject1 )
		return;

	my_g_EntityCollisionHash->AddObjectPair( pObject0, pObject1 );
	PhysRecheckObjectPair( pObject0, pObject1 );
}

CCollisionEvent *g_Collisions = NULL;

void PhysCallbackRemove(IServerNetworkable *pRemove)
{
	if ( PhysIsInCallback() )
	{
		g_Collisions->AddRemoveObject(pRemove);
	}
	else
	{
		servertools->RemoveEntity(pRemove->GetBaseEntity());
	}
}

void CCollisionEvent::AddRemoveObject(IServerNetworkable *pRemove)
{
	if ( pRemove && m_removeObjects.Find(pRemove) == -1 )
	{
		m_removeObjects.AddToTail(pRemove);
	}
}



bool PhysFindOrAddVehicleScript( const char *pScriptName, vehicleparams_t *pParams, vehiclesounds_t *pSounds )
{
	return g_PhysicsHook->FindOrAddVehicleScript(pScriptName, pParams, pSounds);
}



class CPhysicsGameTrace : public IPhysicsGameTrace
{
public:

	void VehicleTraceRay( const Ray_t &ray, void *pVehicle, trace_t *pTrace );
	void VehicleTraceRayWithWater( const Ray_t &ray, void *pVehicle, trace_t *pTrace );
	bool VehiclePointInWater( const Vector &vecPoint );
};

CPhysicsGameTrace g_PhysGameTrace;
IPhysicsGameTrace *physgametrace = &g_PhysGameTrace;


//-----------------------------------------------------------------------------
// Purpose: Game ray-traces in vphysics.
//-----------------------------------------------------------------------------
void CPhysicsGameTrace::VehicleTraceRay( const Ray_t &ray, void *pVehicle, trace_t *pTrace )
{
	CBaseEntity *pBaseEntity = static_cast<CBaseEntity*>( pVehicle );
	UTIL_TraceRay( ray, MASK_SOLID, pBaseEntity, COLLISION_GROUP_NONE, pTrace );
}

//-----------------------------------------------------------------------------
// Purpose: Game ray-traces in vphysics.
//-----------------------------------------------------------------------------
void CPhysicsGameTrace::VehicleTraceRayWithWater( const Ray_t &ray, void *pVehicle, trace_t *pTrace )
{
	CBaseEntity *pBaseEntity = static_cast<CBaseEntity*>( pVehicle );
	UTIL_TraceRay( ray, MASK_SOLID|MASK_WATER, pBaseEntity, COLLISION_GROUP_NONE, pTrace );
}

//-----------------------------------------------------------------------------
// Purpose: Test to see if a vehicle point is in water.
//-----------------------------------------------------------------------------
bool CPhysicsGameTrace::VehiclePointInWater( const Vector &vecPoint )
{
	return ( ( UTIL_PointContents( vecPoint ) & MASK_WATER ) != 0 );
}

