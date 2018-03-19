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

void PhysCallbackDamage( CBaseEntity *pEntity, const CTakeDamageInfo &info, gamevcollisionevent_t &event, int hurtIndex );



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

float PhysGetEntityMass( CEntity *pEntity );

AngularImpulse ComputeRotSpeedToAlignAxes( const Vector &testAxis, const Vector &alignAxis, const AngularImpulse &currentSpeed, 
										  float damping, float scale, float maxSpeed );

#endif
