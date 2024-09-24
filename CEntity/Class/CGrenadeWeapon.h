#ifndef _INCLUDE_CGRENADEWEAPON_H_
#define _INCLUDE_CGRENADEWEAPON_H_

#include "CCombatWeapon.h"


class CGrenadeWeapon : public CCombatWeapon
{
public:
	CE_DECLARE_CLASS( CGrenadeWeapon, CCombatWeapon );

	virtual void EmitGrenade(Vector vecSrc, QAngle vecAngles, Vector vecVel, AngularImpulse angImpulse, CBaseEntity *pPlayer);

public:
	DECLARE_DEFAULTHEADER(EmitGrenade, void, (Vector vecSrc, QAngle vecAngles, Vector vecVel, AngularImpulse angImpulse, CBaseEntity *pPlayer));

};

#endif