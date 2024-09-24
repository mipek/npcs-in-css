
#include "CGrenadeWeapon.h"


SH_DECL_MANUALHOOK5_void(EmitGrenade, 0, 0, 0, Vector , QAngle , Vector , AngularImpulse , CBaseEntity *);
DECLARE_HOOK(EmitGrenade, CGrenadeWeapon);
DECLARE_DEFAULTHANDLER_void(CGrenadeWeapon, EmitGrenade, (Vector vecSrc, QAngle vecAngles, Vector vecVel, AngularImpulse angImpulse, CBaseEntity *pPlayer), (vecSrc, vecAngles, vecVel, angImpulse, pPlayer));

