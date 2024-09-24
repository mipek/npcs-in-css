//========= Copyright ?1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: This is the abstraction layer for the physics simulation system
// Any calls to the external physics library (ipion) should be made through this
// layer.  Eventually, the physics system will probably become a DLL and made 
// accessible to the client & server side code.
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#ifndef PHYSICS_H
#define PHYSICS_H

#ifdef _WIN32
#pragma once
#endif


const float VPHYSICS_LARGE_OBJECT_MASS = 500.0f;

struct gamevcollisionevent_t : public vcollisionevent_t
{
	Vector			preVelocity[2];
	Vector			postVelocity[2];
	AngularImpulse	preAngularVelocity[2];
	CBaseEntity		*pEntities[2];

	void Init( vcollisionevent_t *pEvent )
	{
		*((vcollisionevent_t *)this) = *pEvent;
		pEntities[0] = NULL;
		pEntities[1] = NULL;
	}
};


struct masscenteroverride_t
{
	enum align_type
	{
		ALIGN_POINT = 0,
		ALIGN_AXIS = 1,
	};

	void Defaults()
	{
		entityName = NULL_STRING;
	}

	void SnapToPoint( string_t name, const Vector &pointWS )
	{
		entityName = name;
		center = pointWS;
		axis.Init();
		alignType = ALIGN_POINT;
	}

	void SnapToAxis( string_t name, const Vector &axisStartWS, const Vector &unitAxisDirWS )
	{
		entityName = name;
		center = axisStartWS;
		axis = unitAxisDirWS;
		alignType = ALIGN_AXIS;
	}

	Vector		center;
	Vector		axis;
	int			alignType;
	string_t	entityName;
};



void PhysCallbackDamage( CBaseEntity *pEntity, const CTakeDamageInfo &info, gamevcollisionevent_t &event, int hurtIndex );

void PhysicsImpactSound( CBaseEntity *pEntity, IPhysicsObject *pPhysObject, int channel, int surfaceProps, int surfacePropsHit, float volume, float impactSpeed );

void PhysCallbackSetVelocity( IPhysicsObject *pPhysicsObject, const Vector &vecVelocity );

void PhysCallbackRemove(IServerNetworkable *pRemove);

struct breakablepropparams_t
{
	breakablepropparams_t( const Vector &_origin, const QAngle &_angles, const Vector &_velocity, const AngularImpulse &_angularVelocity )
			: origin(_origin), angles(_angles), velocity(_velocity), angularVelocity(_angularVelocity)
	{
		impactEnergyScale = 0;
		defBurstScale = 0;
		defCollisionGroup = COLLISION_GROUP_NONE;
	}

	const Vector &origin;
	const QAngle &angles;
	const Vector &velocity;
	const AngularImpulse &angularVelocity;
	float impactEnergyScale;
	float defBurstScale;
	int defCollisionGroup;
};


struct triggerevent_t
{
	CBaseEntity		*pTriggerEntity;
	IPhysicsObject	*pTriggerPhysics;
	CBaseEntity		*pEntity;
	IPhysicsObject	*pObject;
	bool			bStart;

	inline void Init( CBaseEntity *triggerEntity, IPhysicsObject *triggerPhysics, CBaseEntity *entity, IPhysicsObject *object, bool startTouch )
	{
		pTriggerEntity = triggerEntity;
		pTriggerPhysics= triggerPhysics;
		pEntity = entity;
		pObject = object;
		bStart = startTouch;
	}
	inline void Clear()
	{
		memset( this, 0, sizeof(*this) );
	}
};



float PhysGetEntityMass( CEntity *pEntity );

AngularImpulse ComputeRotSpeedToAlignAxes( const Vector &testAxis, const Vector &alignAxis, const AngularImpulse &currentSpeed,
										   float damping, float scale, float maxSpeed );


bool PhysModelParseSolid( solid_t &solid, CBaseEntity *pEntity, int modelIndex );

bool PhysModelParseSolidByIndex( solid_t &solid, CBaseEntity *pEntity, int modelIndex, int solidIndex );

IPhysicsObject *FindPhysicsObjectByName( const char *pName, CEntity *pErrorEntity );

bool PhysFindOrAddVehicleScript( const char *pScriptName, struct vehicleparams_t *pParams, struct vehiclesounds_t *pSounds );

void PhysEnableEntityCollisions( IPhysicsObject *pObject0, IPhysicsObject *pObject1 );
void PhysDisableEntityCollisions( IPhysicsObject *pObject0, IPhysicsObject *pObject1 );

extern const objectparams_t g_PhysDefaultObjectParams;

#endif
