
#include "CBreakableProp.h"
#include "pickup.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CE_LINK_ENTITY_TO_CLASS(CBreakableProp, CE_CBreakableProp);

//Datamaps
DEFINE_PROP(m_iInteractions,CE_CBreakableProp);


/**void CE_CBreakableProp::Break( CBaseEntity *pBreaker, const CTakeDamageInfo &info)
{
	g_helpfunc.CBreakableProp_Break(BaseEntity(), pBreaker, info);
}*/


bool CE_CBreakableProp::OnAttemptPhysGunPickup( CBaseEntity *pPhysGunUser, PhysGunPickup_t reason)
{
	IPlayerPickupVPhysics *pPickup = dynamic_cast<IPlayerPickupVPhysics *>(BaseEntity());
	Assert(pPickup);
	return pPickup->OnAttemptPhysGunPickup(pPhysGunUser, reason);
}

CBaseEntity *CE_CBreakableProp::OnFailedPhysGunPickup( Vector vPhysgunPos )
{
	IPlayerPickupVPhysics *pPickup = dynamic_cast<IPlayerPickupVPhysics *>(BaseEntity());
	Assert(pPickup);
	return pPickup->OnFailedPhysGunPickup(vPhysgunPos);
}

void CE_CBreakableProp::OnPhysGunPickup( CBaseEntity *pPhysGunUser, PhysGunPickup_t reason)
{
	IPlayerPickupVPhysics *pPickup = dynamic_cast<IPlayerPickupVPhysics *>(BaseEntity());
	Assert(pPickup);
	pPickup->OnPhysGunPickup(pPhysGunUser, reason);

}

void CE_CBreakableProp::OnPhysGunDrop( CBaseEntity *pPhysGunUser, PhysGunDrop_t Reason )
{
	IPlayerPickupVPhysics *pPickup = dynamic_cast<IPlayerPickupVPhysics *>(BaseEntity());
	Assert(pPickup);
	pPickup->OnPhysGunDrop(pPhysGunUser, Reason);
}

bool CE_CBreakableProp::HasPreferredCarryAnglesForPlayer( CBaseEntity *pPlayer )
{
	IPlayerPickupVPhysics *pPickup = dynamic_cast<IPlayerPickupVPhysics *>(BaseEntity());
	Assert(pPickup);
	return pPickup->HasPreferredCarryAnglesForPlayer(pPlayer);
}

QAngle CE_CBreakableProp::PreferredCarryAngles( void )
{
	IPlayerPickupVPhysics *pPickup = dynamic_cast<IPlayerPickupVPhysics *>(BaseEntity());
	Assert(pPickup);
	return pPickup->PreferredCarryAngles();
}

bool CE_CBreakableProp::ForcePhysgunOpen( CBaseEntity *pPlayer )
{
	IPlayerPickupVPhysics *pPickup = dynamic_cast<IPlayerPickupVPhysics *>(BaseEntity());
	Assert(pPickup);
	return pPickup->ForcePhysgunOpen(pPlayer);
}

AngularImpulse CE_CBreakableProp::PhysGunLaunchAngularImpulse()
{
	IPlayerPickupVPhysics *pPickup = dynamic_cast<IPlayerPickupVPhysics *>(BaseEntity());
	Assert(pPickup);
	return pPickup->PhysGunLaunchAngularImpulse();
}

bool CE_CBreakableProp::ShouldPuntUseLaunchForces( PhysGunForce_t reason )
{
	IPlayerPickupVPhysics *pPickup = dynamic_cast<IPlayerPickupVPhysics *>(BaseEntity());
	Assert(pPickup);
	return pPickup->ShouldPuntUseLaunchForces(reason);
}

Vector CE_CBreakableProp::PhysGunLaunchVelocity( const Vector &vecForward, float flMass )
{
	IPlayerPickupVPhysics *pPickup = dynamic_cast<IPlayerPickupVPhysics *>(BaseEntity());
	Assert(pPickup);
	return pPickup->PhysGunLaunchVelocity(vecForward, flMass);
}

