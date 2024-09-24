
#include "player_pickup.h"
#include "CPlayer.h"
#include "CPhysicsProp.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

bool Pickup_GetPreferredCarryAngles( CEntity *pObject, CPlayer *pPlayer, matrix3x4_t &localToWorld, QAngle &outputAnglesWorldSpace )
{
	if(!pObject)
		return false;

	IPlayerPickupVPhysics *pPickup = dynamic_cast<IPlayerPickupVPhysics *>(pObject);
	if(!pPickup)
	{
		pPickup = dynamic_cast<IPlayerPickupVPhysics *>(pObject->BaseEntity());
	}

	if ( pPickup )
	{
		if ( pPickup->HasPreferredCarryAnglesForPlayer( (pPlayer)?pPlayer->BaseEntity():NULL ) )
		{
			outputAnglesWorldSpace = TransformAnglesToWorldSpace( pPickup->PreferredCarryAngles(), localToWorld );
			return true;
		}
	}
	return false;
}


void Pickup_ForcePlayerToDropThisObject( CEntity *pTarget )
{
	if ( pTarget == NULL )
		return;

	IPhysicsObject *pPhysics = pTarget->VPhysicsGetObject();
	if ( pPhysics == NULL )
		return;

	if ( pPhysics->GetGameFlags() & FVPHYSICS_PLAYER_HELD )
	{
		for(int i=1;i<=gpGlobals->maxClients;i++)
		{
			CPlayer *pPlayer = UTIL_PlayerByIndex(i);
			if(!pPlayer)
				continue;

			if(pPlayer->GetHoldEntity() == pTarget)
			{
				pPlayer->ForceDropOfCarriedPhysObjects(pTarget);
				return;
			}
		}
	}
}



void Pickup_OnPhysGunDrop( CEntity *pDroppedObject, CPlayer *pPlayer, PhysGunDrop_t Reason )
{
	if(!pDroppedObject)
		return;

	IPlayerPickupVPhysics *pPickup = dynamic_cast<IPlayerPickupVPhysics *>(pDroppedObject);
	if(!pPickup)
	{
		pPickup = dynamic_cast<IPlayerPickupVPhysics *>(pDroppedObject->BaseEntity());
	}
	if ( pPickup )
	{
		pPickup->OnPhysGunDrop( (pPlayer)?pPlayer->BaseEntity():NULL, Reason );
	}
}


void Pickup_OnPhysGunPickup( CEntity *pPickedUpObject, CPlayer *pPlayer, PhysGunPickup_t reason )
{
	if(!pPickedUpObject)
		return;

	IPlayerPickupVPhysics *pPickup = dynamic_cast<IPlayerPickupVPhysics *>(pPickedUpObject);
	if(!pPickup)
	{
		pPickup = dynamic_cast<IPlayerPickupVPhysics *>(pPickedUpObject->BaseEntity());
	}
	if ( pPickup )
	{
		pPickup->OnPhysGunPickup( (pPlayer)?pPlayer->BaseEntity():NULL, reason );
	}
}

extern ConVar physcannon_maxforce;
extern ConVar physcannon_minforce;


Vector Pickup_DefaultPhysGunLaunchVelocity( const Vector &vecForward, float flMass )
{
	// Calculate the velocity based on physcannon rules
	float flForceMax = physcannon_maxforce.GetFloat();
	float flForce = flForceMax;

	float mass = flMass;
	if ( mass > 100 )
	{
		mass = MIN( mass, 1000 );
		float flForceMin = physcannon_minforce.GetFloat();
		flForce = SimpleSplineRemapValClamped( mass, 100, 600, flForceMax, flForceMin );
	}

	return ( vecForward * flForce );
}



AngularImpulse Pickup_PhysGunLaunchAngularImpulse( CEntity *pObject, PhysGunForce_t reason )
{
	if(!pObject)
		return RandomAngularImpulse( -600, 600 );

	IPlayerPickupVPhysics *pPickup = dynamic_cast<IPlayerPickupVPhysics *>(pObject);
	if(!pPickup)
	{
		pPickup = dynamic_cast<IPlayerPickupVPhysics *>(pObject->BaseEntity());
	}
	if ( pPickup != NULL && pPickup->ShouldPuntUseLaunchForces( reason ) )
	{
		return pPickup->PhysGunLaunchAngularImpulse();
	}
	return RandomAngularImpulse( -600, 600 );
}

bool Pickup_ShouldPuntUseLaunchForces( CEntity *pObject, PhysGunForce_t reason )
{
	if(!pObject)
		return false;

	IPlayerPickupVPhysics *pPickup = dynamic_cast<IPlayerPickupVPhysics *>(pObject);
	if(!pPickup)
	{
		pPickup = dynamic_cast<IPlayerPickupVPhysics *>(pObject->BaseEntity());
	}
	if ( pPickup )
	{
		return pPickup->ShouldPuntUseLaunchForces( reason );
	}
	return false;
}


bool Pickup_OnAttemptPhysGunPickup( CEntity *pPickedUpObject, CPlayer *pPlayer, PhysGunPickup_t reason )
{
	if(!pPickedUpObject)
		return true;

	IPlayerPickupVPhysics *pPickup = dynamic_cast<IPlayerPickupVPhysics *>(pPickedUpObject);
	if(!pPickup && pPickedUpObject)
	{
		pPickup = dynamic_cast<IPlayerPickupVPhysics *>(pPickedUpObject->BaseEntity());
	}
	if ( pPickup )
	{
		return pPickup->OnAttemptPhysGunPickup( (pPlayer)?pPlayer->BaseEntity():NULL, reason );
	}
	return true;
}


bool Pickup_ForcePhysGunOpen( CEntity *pObject, CPlayer *pPlayer )
{
	if(!pObject)
		return false;

	IPlayerPickupVPhysics *pPickup = dynamic_cast<IPlayerPickupVPhysics *>(pObject);
	if(!pPickup)
	{
		pPickup = dynamic_cast<IPlayerPickupVPhysics *>(pObject->BaseEntity());
	}
	if ( pPickup )
	{
		return pPickup->ForcePhysgunOpen( (pPlayer)?pPlayer->BaseEntity():NULL );
	}
	return false;
}

