#ifndef _INCLUDE_PLAYER_PICKUP_H_
#define _INCLUDE_PLAYER_PICKUP_H_

#include "CEntity.h"

class CPlayer;


// Reasons behind a pickup
enum PhysGunPickup_t
{
	PICKED_UP_BY_CANNON,
	PUNTED_BY_CANNON,
	PICKED_UP_BY_PLAYER, // Picked up by +USE, not physgun.
};

// Reasons behind a drop
enum PhysGunDrop_t
{
	DROPPED_BY_PLAYER,
	THROWN_BY_PLAYER,
	DROPPED_BY_CANNON,
	LAUNCHED_BY_CANNON,
};

enum PhysGunForce_t
{
	PHYSGUN_FORCE_DROPPED,	// Dropped by +USE
	PHYSGUN_FORCE_THROWN,	// Thrown from +USE
	PHYSGUN_FORCE_PUNTED,	// Punted by cannon
	PHYSGUN_FORCE_LAUNCHED,	// Launched by cannon
};

Vector Pickup_DefaultPhysGunLaunchVelocity( const Vector &vecForward, float flMass );


abstract_class IPlayerPickupVPhysics
{
public:
	// Callbacks for the physgun/cannon picking up an entity
	virtual bool			OnAttemptPhysGunPickup( CBaseEntity *pPhysGunUser, PhysGunPickup_t reason = PICKED_UP_BY_CANNON ) = 0;
	virtual CBaseEntity		*OnFailedPhysGunPickup( Vector vPhysgunPos ) = 0;
	virtual void			OnPhysGunPickup( CBaseEntity *pPhysGunUser, PhysGunPickup_t reason = PICKED_UP_BY_CANNON ) = 0;
	virtual void			OnPhysGunDrop( CBaseEntity *pPhysGunUser, PhysGunDrop_t Reason ) = 0;
	virtual bool			HasPreferredCarryAnglesForPlayer( CBaseEntity *pPlayer = NULL ) = 0;
	virtual QAngle			PreferredCarryAngles( void )  = 0;
	virtual bool			ForcePhysgunOpen( CBaseEntity *pPlayer ) = 0;
	virtual AngularImpulse	PhysGunLaunchAngularImpulse() = 0;
	virtual bool			ShouldPuntUseLaunchForces( PhysGunForce_t reason ) = 0;
	virtual Vector			PhysGunLaunchVelocity( const Vector &vecForward, float flMass ) = 0;
};

class CDefaultPlayerPickupVPhysics : public IPlayerPickupVPhysics
{
public:
	virtual bool			OnAttemptPhysGunPickup( CBaseEntity *pPhysGunUser, PhysGunPickup_t reason = PICKED_UP_BY_CANNON ) { return true; }
	virtual CBaseEntity		*OnFailedPhysGunPickup( Vector vPhysgunPos ) { return NULL; }
	virtual void			OnPhysGunPickup( CBaseEntity *pPhysGunUser, PhysGunPickup_t reason = PICKED_UP_BY_CANNON ) {}
	virtual void			OnPhysGunDrop( CBaseEntity *pPhysGunUser, PhysGunDrop_t reason ) {}
	virtual bool			HasPreferredCarryAnglesForPlayer( CBaseEntity *pPlayer ) { return false; }
	virtual QAngle			PreferredCarryAngles( void ) { return vec3_angle; }
	virtual bool			ForcePhysgunOpen( CBaseEntity *pPlayer ) { return false; }
	virtual AngularImpulse	PhysGunLaunchAngularImpulse() { return RandomAngularImpulse( -600, 600 ); }
	virtual bool			ShouldPuntUseLaunchForces( PhysGunForce_t reason ) { return false; }
	virtual Vector			PhysGunLaunchVelocity( const Vector &vecForward, float flMass )
	{
		return Pickup_DefaultPhysGunLaunchVelocity( vecForward, flMass );
	}
};




bool Pickup_GetPreferredCarryAngles( CEntity *pObject, CPlayer *pPlayer, matrix3x4_t &localToWorld, QAngle &outputAnglesWorldSpace );
void Pickup_ForcePlayerToDropThisObject( CEntity *pTarget );

void Pickup_OnPhysGunDrop( CEntity *pDroppedObject, CPlayer *pPlayer, PhysGunDrop_t reason );
void Pickup_OnPhysGunPickup( CEntity *pPickedUpObject, CPlayer *pPlayer, PhysGunPickup_t reason = PICKED_UP_BY_CANNON );

#endif

