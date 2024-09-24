
#ifndef _INCLUDE_CBREAKABLEPROP_H_
#define _INCLUDE_CBREAKABLEPROP_H_

#include "CEntity.h"
#include "CProp.h"
#include "pickup.h"
#include "player_pickup.h"


class CE_CBreakableProp : public CE_Prop, public IPlayerPickupVPhysics
{
public:
	CE_DECLARE_CLASS( CE_CBreakableProp, CE_Prop );

	void SetInteraction( propdata_interactions_t Interaction ) { *(m_iInteractions) |= (1 << Interaction); }

	//void Break( CBaseEntity *pBreaker, const CTakeDamageInfo &info );

public:
	virtual bool			OnAttemptPhysGunPickup( CBaseEntity *pPhysGunUser, PhysGunPickup_t reason = PICKED_UP_BY_CANNON );
	virtual CBaseEntity		*OnFailedPhysGunPickup( Vector vPhysgunPos );
	virtual void			OnPhysGunPickup( CBaseEntity *pPhysGunUser, PhysGunPickup_t reason = PICKED_UP_BY_CANNON );
	virtual void			OnPhysGunDrop( CBaseEntity *pPhysGunUser, PhysGunDrop_t Reason );
	virtual bool			HasPreferredCarryAnglesForPlayer( CBaseEntity *pPlayer = NULL );
	virtual QAngle			PreferredCarryAngles( void );
	virtual bool			ForcePhysgunOpen( CBaseEntity *pPlayer );
	virtual AngularImpulse	PhysGunLaunchAngularImpulse();
	virtual bool			ShouldPuntUseLaunchForces( PhysGunForce_t reason );
	virtual Vector			PhysGunLaunchVelocity( const Vector &vecForward, float flMass );


protected: //Datamaps
	DECLARE_DATAMAP(int, m_iInteractions);


};



#endif