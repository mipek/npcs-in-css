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

#include "CEntity.h"
#include "shareddefs.h"
#include "CEntityManager.h"
#include "CTakeDamageInfo.h"
#include "CAI_NPC.h"
#include "CPlayer.h"
#include "model_types.h"
#include "isaverestore.h"
#include "CAI_Criteria.h"
#include "CE_recipientfilter.h"
#include "groundlink.h"
#include "CSkyCamera.h"
#include "soundchars.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



IHookTracker *IHookTracker::m_Head = NULL;
IPropTracker *IPropTracker::m_Head = NULL;
IDetourTracker *IDetourTracker::m_Head = NULL;
ISigOffsetTracker *ISigOffsetTracker::m_Head = NULL;

ISaveRestoreOps *eventFuncs = NULL;

BEGIN_SIMPLE_DATADESC( ResponseContext_t )

	DEFINE_FIELD( m_iszName,			FIELD_STRING ),
	DEFINE_FIELD( m_iszValue,			FIELD_STRING ),
	DEFINE_FIELD( m_fExpirationTime,	FIELD_TIME ),

END_DATADESC()


SH_DECL_MANUALHOOK3_void(Teleport, 0, 0, 0, const Vector *, const QAngle *, const Vector *);
SH_DECL_MANUALHOOK0_void(UpdateOnRemove, 0, 0, 0);
SH_DECL_MANUALHOOK0_void(Spawn, 0, 0, 0);
SH_DECL_MANUALHOOK1(OnTakeDamage, 0, 0, 0, int, const CTakeDamageInfo &);
SH_DECL_MANUALHOOK0_void(Think, 0, 0, 0);
SH_DECL_MANUALHOOK5(AcceptInput, 0, 0, 0, bool, const char *, CBaseEntity *, CBaseEntity *, variant_t, int);
SH_DECL_MANUALHOOK0(GetDataDescMap_Real, 0, 0, 0, datamap_t *);
SH_DECL_MANUALHOOK1_void(StartTouch, 0, 0, 0, CBaseEntity *);
SH_DECL_MANUALHOOK1_void(Touch, 0, 0, 0, CBaseEntity *);
SH_DECL_MANUALHOOK1_void(EndTouch, 0, 0, 0, CBaseEntity *);
SH_DECL_MANUALHOOK0(GetSoundEmissionOrigin, 0, 0, 0, Vector);
SH_DECL_MANUALHOOK1(VPhysicsTakeDamage, 0, 0, 0, int, const CTakeDamageInfo &);
SH_DECL_MANUALHOOK2(VPhysicsGetObjectList, 0, 0, 0, int, IPhysicsObject **, int);
SH_DECL_MANUALHOOK0(GetServerClass, 0, 0, 0, ServerClass *);
SH_DECL_MANUALHOOK0(Classify, 0, 0, 0, Class_T);
SH_DECL_MANUALHOOK2(ShouldCollide, 0, 0, 0, bool, int, int);
SH_DECL_MANUALHOOK0(ObjectCaps, 0, 0, 0, int);
SH_DECL_MANUALHOOK0(IsNPC, 0, 0, 0, bool);
SH_DECL_MANUALHOOK2_void(SetParent, 0, 0, 0, CBaseEntity *, int);
SH_DECL_MANUALHOOK2_void(DecalTrace, 0, 0, 0, trace_t *, char const *);
SH_DECL_MANUALHOOK1_void(Event_Killed, 0, 0, 0, const CTakeDamageInfo &);
SH_DECL_MANUALHOOK4_void(TraceAttack, 0, 0, 0, const CTakeDamageInfo &, const Vector &, trace_t *, CDmgAccumulator *);
SH_DECL_MANUALHOOK2(BodyTarget, 0, 0, 0, Vector, const Vector &, bool);
SH_DECL_MANUALHOOK0(GetServerVehicle, 0, 0, 0, IServerVehicle *);
SH_DECL_MANUALHOOK0(IsAlive, 0, 0, 0, bool);
SH_DECL_MANUALHOOK0(WorldSpaceCenter, 0, 0, 0, const Vector &);
SH_DECL_MANUALHOOK0_void(PhysicsSimulate, 0, 0, 0);
SH_DECL_MANUALHOOK0(BloodColor, 0, 0, 0, int);
SH_DECL_MANUALHOOK0_void(StopLoopingSounds, 0, 0, 0);
SH_DECL_MANUALHOOK1_void(SetOwnerEntity, 0, 0, 0, CBaseEntity *);
SH_DECL_MANUALHOOK0_void(Activate, 0, 0, 0);
SH_DECL_MANUALHOOK1(HeadTarget, 0, 0, 0, Vector, const Vector &);
SH_DECL_MANUALHOOK0(GetAutoAimRadius, 0, 0, 0, float);
SH_DECL_MANUALHOOK0(PhysicsSolidMaskForEntity, 0, 0, 0, unsigned int);
SH_DECL_MANUALHOOK1(CanStandOn, 0, 0, 0, bool, CBaseEntity *);
SH_DECL_MANUALHOOK0(IsMoving, 0, 0, 0, bool);
SH_DECL_MANUALHOOK0(GetSmoothedVelocity, 0, 0, 0, Vector);
SH_DECL_MANUALHOOK4_void(Use, 0, 0, 0, CBaseEntity *, CBaseEntity *, USE_TYPE , float );
SH_DECL_MANUALHOOK3(FVisible_Entity, 0, 0, 0, bool, CBaseEntity *, int, CBaseEntity **);
SH_DECL_MANUALHOOK3(FVisible_Vector, 0, 0, 0, bool, const Vector &, int, CBaseEntity **);
SH_DECL_MANUALHOOK0(EarPosition, 0, 0,0, Vector);
SH_DECL_MANUALHOOK0(GetAutoAimCenter, 0, 0,0, Vector);
SH_DECL_MANUALHOOK0(EyePosition, 0, 0,0, Vector);
SH_DECL_MANUALHOOK0_void(OnRestore, 0, 0, 0);
SH_DECL_MANUALHOOK3_void(ImpactTrace, 0, 0, 0, trace_t *, int , const char *);
SH_DECL_MANUALHOOK3(TestHitboxes, 0, 0, 0, bool, const Ray_t &, unsigned int , trace_t&);
SH_DECL_MANUALHOOK2_void(VPhysicsCollision, 0, 0, 0, int, gamevcollisionevent_t *);
SH_DECL_MANUALHOOK2_void(VPhysicsShadowCollision, 0, 0, 0, int, gamevcollisionevent_t *);
SH_DECL_MANUALHOOK0_void(Splash, 0, 0, 0);
SH_DECL_MANUALHOOK0(ShouldSavePhysics, 0, 0, 0, bool);
SH_DECL_MANUALHOOK0(CreateVPhysics, 0, 0, 0, bool);
SH_DECL_MANUALHOOK0(IsNetClient, 0, 0, 0, bool);
SH_DECL_MANUALHOOK1(HasPhysicsAttacker, 0, 0, 0, CBaseEntity *, float);
SH_DECL_MANUALHOOK0(EyeAngles, 0, 0, 0, const QAngle &);
SH_DECL_MANUALHOOK1(Save, 0, 0, 0, int, ISave &);
SH_DECL_MANUALHOOK1(Restore, 0, 0, 0, int, IRestore &);
SH_DECL_MANUALHOOK1_void(ModifyOrAppendCriteria, 0, 0, 0, AI_CriteriaSet &);
SH_DECL_MANUALHOOK1_void(DeathNotice, 0, 0, 0, CBaseEntity *);
SH_DECL_MANUALHOOK1(PassesDamageFilter, 0, 0, 0, bool, const CTakeDamageInfo &);
SH_DECL_MANUALHOOK0_void(Precache, 0, 0, 0);
SH_DECL_MANUALHOOK2(DispatchKeyValue, 0, 0, 0, bool, const char *, const char *);
SH_DECL_MANUALHOOK2_void(OnEntityEvent, 0, 0, 0, EntityEvent_t , void *);
SH_DECL_MANUALHOOK0_void(VPhysicsDestroyObject, 0, 0, 0);
SH_DECL_MANUALHOOK2(TakeHealth, 0, 0, 0, int, float , int );
SH_DECL_MANUALHOOK1(GetAttackDamageScale, 0, 0, 0, float, CBaseEntity * );
SH_DECL_MANUALHOOK1(OnControls, 0, 0, 0, bool, CBaseEntity * );
SH_DECL_MANUALHOOK1_void(VPhysicsUpdate, 0, 0, 0,IPhysicsObject *);
SH_DECL_MANUALHOOK1_void(VPhysicsShadowUpdate, 0, 0, 0, IPhysicsObject *);
SH_DECL_MANUALHOOK1_void(SetModel, 0, 0, 0, const char *);
SH_DECL_MANUALHOOK1(IsTriggered, 0, 0, 0, bool, CBaseEntity *);
SH_DECL_MANUALHOOK1_void(FireBullets, 0, 0, 0, const FireBulletsInfo_t &);
SH_DECL_MANUALHOOK0(GetTracerType, 0, 0, 0, const char	*);
SH_DECL_MANUALHOOK0(UpdateTransmitState, 0, 0, 0, int);
SH_DECL_MANUALHOOK2_void(SetTransmit, 0, 0, 0, CCheckTransmitInfo *, bool);
SH_DECL_MANUALHOOK1(CanBeSeenBy, 0, 0, 0, bool, CBaseEntity *);
SH_DECL_MANUALHOOK0(IsViewable, 0, 0, 0, bool);
SH_DECL_MANUALHOOK0(GetResponseSystem, 0, 0, 0, IResponseSystem *);



DECLARE_HOOK(Teleport, CEntity);
DECLARE_HOOK(UpdateOnRemove, CEntity);
DECLARE_HOOK(Spawn, CEntity);
DECLARE_HOOK(OnTakeDamage, CEntity);
DECLARE_HOOK(Think, CEntity);
DECLARE_HOOK(AcceptInput, CEntity);
DECLARE_HOOK(GetDataDescMap_Real, CEntity);
DECLARE_HOOK(StartTouch, CEntity);
DECLARE_HOOK(Touch, CEntity);
DECLARE_HOOK(EndTouch, CEntity);
DECLARE_HOOK(GetSoundEmissionOrigin, CEntity);
DECLARE_HOOK(VPhysicsTakeDamage, CEntity);
DECLARE_HOOK(VPhysicsGetObjectList, CEntity);
DECLARE_HOOK(GetServerClass, CEntity);
DECLARE_HOOK(Classify, CEntity);
DECLARE_HOOK(ShouldCollide, CEntity);
DECLARE_HOOK(ObjectCaps, CEntity);
DECLARE_HOOK(IsNPC, CEntity);
DECLARE_HOOK(SetParent, CEntity);
DECLARE_HOOK(DecalTrace, CEntity);
DECLARE_HOOK(Event_Killed, CEntity);
DECLARE_HOOK(TraceAttack, CEntity);
DECLARE_HOOK(BodyTarget, CEntity);
//DECLARE_HOOK(GetServerVehicle, CEntity);
DECLARE_HOOK(IsAlive, CEntity);
DECLARE_HOOK(WorldSpaceCenter, CEntity);
DECLARE_HOOK(PhysicsSimulate, CEntity);
DECLARE_HOOK(BloodColor, CEntity);
DECLARE_HOOK(StopLoopingSounds, CEntity);
DECLARE_HOOK(SetOwnerEntity, CEntity);
DECLARE_HOOK(Activate, CEntity);
DECLARE_HOOK(HeadTarget, CEntity);
DECLARE_HOOK(GetAutoAimRadius, CEntity);
DECLARE_HOOK(PhysicsSolidMaskForEntity, CEntity);
DECLARE_HOOK(CanStandOn, CEntity);
DECLARE_HOOK(IsMoving, CEntity);
DECLARE_HOOK(GetSmoothedVelocity, CEntity);
DECLARE_HOOK(Use, CEntity);
DECLARE_HOOK(FVisible_Entity, CEntity);
DECLARE_HOOK(FVisible_Vector, CEntity);
DECLARE_HOOK(EarPosition, CEntity);
DECLARE_HOOK(GetAutoAimCenter, CEntity);
DECLARE_HOOK(EyePosition, CEntity);
DECLARE_HOOK(OnRestore, CEntity);
DECLARE_HOOK(ImpactTrace, CEntity);
DECLARE_HOOK(TestHitboxes, CEntity);
DECLARE_HOOK(VPhysicsCollision, CEntity);
DECLARE_HOOK(VPhysicsShadowCollision, CEntity);
DECLARE_HOOK(Splash, CEntity);
DECLARE_HOOK(ShouldSavePhysics, CEntity);
DECLARE_HOOK(CreateVPhysics, CEntity);
DECLARE_HOOK(IsNetClient, CEntity);
DECLARE_HOOK(HasPhysicsAttacker, CEntity);
DECLARE_HOOK(EyeAngles, CEntity);
DECLARE_HOOK(Save, CEntity);
DECLARE_HOOK(Restore, CEntity);
DECLARE_HOOK(ModifyOrAppendCriteria, CEntity);
DECLARE_HOOK(DeathNotice, CEntity);
DECLARE_HOOK(PassesDamageFilter, CEntity);
DECLARE_HOOK(Precache, CEntity);

DECLARE_HOOK(DispatchKeyValue, CEntity);
DECLARE_HOOK(OnEntityEvent, CEntity);
DECLARE_HOOK(VPhysicsDestroyObject, CEntity);
DECLARE_HOOK(TakeHealth, CEntity);
DECLARE_HOOK(GetAttackDamageScale, CEntity);
DECLARE_HOOK(OnControls, CEntity);
DECLARE_HOOK(VPhysicsUpdate, CEntity);
DECLARE_HOOK(VPhysicsShadowUpdate, CEntity);
DECLARE_HOOK(SetModel, CEntity);
DECLARE_HOOK(IsTriggered, CEntity);
DECLARE_HOOK(FireBullets, CEntity);
DECLARE_HOOK(GetTracerType, CEntity);
DECLARE_HOOK(UpdateTransmitState, CEntity);
DECLARE_HOOK(SetTransmit, CEntity);
DECLARE_HOOK(CanBeSeenBy, CEntity);
DECLARE_HOOK(IsViewable, CEntity);
DECLARE_HOOK(GetResponseSystem, CEntity);

DECLARE_DEFAULTHANDLER_void(CEntity, Teleport, (const Vector *origin, const QAngle* angles, const Vector *velocity), (origin, angles, velocity));
DECLARE_DEFAULTHANDLER_void(CEntity, Spawn, (), ());
DECLARE_DEFAULTHANDLER(CEntity, OnTakeDamage, int, (const CTakeDamageInfo &info), (info));
DECLARE_DEFAULTHANDLER(CEntity, VPhysicsTakeDamage, int, (const CTakeDamageInfo &inputInfo), (inputInfo));
DECLARE_DEFAULTHANDLER(CEntity, VPhysicsGetObjectList, int, (IPhysicsObject **pList, int listMax), (pList, listMax));
DECLARE_DEFAULTHANDLER(CEntity, GetServerClass, ServerClass *, (), ());
DECLARE_DEFAULTHANDLER(CEntity,Classify,Class_T,(),());
DECLARE_DEFAULTHANDLER(CEntity,ShouldCollide,bool,(int collisionGroup, int contentsMask),(collisionGroup, contentsMask));
DECLARE_DEFAULTHANDLER(CEntity,ObjectCaps,int,(),());
DECLARE_DEFAULTHANDLER(CEntity,IsNPC,bool,(),());
DECLARE_DEFAULTHANDLER_void(CEntity,SetParent,(CBaseEntity *pParentEntity, int iParentAttachment),(pParentEntity, iParentAttachment));
DECLARE_DEFAULTHANDLER_void(CEntity,DecalTrace, (trace_t *pTrace, char const *decalName) ,(pTrace, decalName));
DECLARE_DEFAULTHANDLER_void(CEntity,Event_Killed, (const CTakeDamageInfo &info), (info));
DECLARE_DEFAULTHANDLER_void(CEntity,TraceAttack, (const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator), (info, vecDir, ptr, pAccumulator));
DECLARE_DEFAULTHANDLER_SPECIAL(CEntity,BodyTarget, Vector, (const Vector &posSrc, bool bNoisy),(posSrc, bNoisy), vec3_origin);
DECLARE_DEFAULTHANDLER(CEntity,IsAlive, bool, (),());
DECLARE_DEFAULTHANDLER_REFERENCE(CEntity,WorldSpaceCenter, const Vector &, () const,());
DECLARE_DEFAULTHANDLER_void(CEntity,PhysicsSimulate, (),());
DECLARE_DEFAULTHANDLER(CEntity, BloodColor, int, (), ());
DECLARE_DEFAULTHANDLER_void(CEntity, StopLoopingSounds, (), ());
DECLARE_DEFAULTHANDLER_void(CEntity, SetOwnerEntity, (CBaseEntity* pOwner), (pOwner));
DECLARE_DEFAULTHANDLER_void(CEntity, Activate, (), ());
DECLARE_DEFAULTHANDLER_SPECIAL(CEntity, HeadTarget, Vector, (const Vector &posSrc), (posSrc), vec3_origin);
DECLARE_DEFAULTHANDLER(CEntity, GetAutoAimRadius, float, (), ());
DECLARE_DEFAULTHANDLER(CEntity, PhysicsSolidMaskForEntity, unsigned int, () const, ());
DECLARE_DEFAULTHANDLER(CEntity, CanStandOn, bool, (CBaseEntity *pSurface) const, (pSurface));
DECLARE_DEFAULTHANDLER(CEntity, IsMoving, bool, (), ());
DECLARE_DEFAULTHANDLER_SPECIAL(CEntity, GetSmoothedVelocity, Vector, (), (), vec3_origin);
DECLARE_DEFAULTHANDLER(CEntity, FVisible_Entity, bool, (CBaseEntity *pEntity, int traceMask, CBaseEntity **ppBlocker), (pEntity,  traceMask, ppBlocker));
DECLARE_DEFAULTHANDLER(CEntity, FVisible_Vector, bool, (const Vector &vecTarget, int traceMask, CBaseEntity **ppBlocker), (vecTarget,  traceMask, ppBlocker));
DECLARE_DEFAULTHANDLER(CEntity, EarPosition, Vector, () const, ());
DECLARE_DEFAULTHANDLER_SPECIAL(CEntity, GetAutoAimCenter, Vector, (), (), vec3_origin);
DECLARE_DEFAULTHANDLER(CEntity, EyePosition, Vector, (), ());
DECLARE_DEFAULTHANDLER_void(CEntity, OnRestore, (), ());
DECLARE_DEFAULTHANDLER_void(CEntity, ImpactTrace, (trace_t *pTrace, int iDamageType, const char *pCustomImpactName), (pTrace, iDamageType, pCustomImpactName));
DECLARE_DEFAULTHANDLER(CEntity, TestHitboxes, bool, (const Ray_t &ray, unsigned int fContentsMask, trace_t& tr), (ray, fContentsMask, tr));
DECLARE_DEFAULTHANDLER_void(CEntity, VPhysicsCollision, (int index, gamevcollisionevent_t *pEvent), (index, pEvent));
DECLARE_DEFAULTHANDLER_void(CEntity, VPhysicsShadowCollision, (int index, gamevcollisionevent_t *pEvent), (index, pEvent));
DECLARE_DEFAULTHANDLER_void(CEntity, Splash, (), ());
DECLARE_DEFAULTHANDLER(CEntity, ShouldSavePhysics, bool, (), ());
DECLARE_DEFAULTHANDLER(CEntity, CreateVPhysics, bool, (), ());
DECLARE_DEFAULTHANDLER(CEntity, IsNetClient, bool, () const, ());
DECLARE_DEFAULTHANDLER(CEntity, HasPhysicsAttacker, CBaseEntity *, (float dt), (dt));
//fuck this!
DECLARE_DEFAULTHANDLER_REFERENCE(CEntity, EyeAngles, const QAngle &, (), ());
DECLARE_DEFAULTHANDLER(CEntity, Save, int, (ISave &save), (save));
DECLARE_DEFAULTHANDLER(CEntity, Restore, int, (IRestore &restore), (restore));
DECLARE_DEFAULTHANDLER_void(CEntity, ModifyOrAppendCriteria, (AI_CriteriaSet& set), (set));
DECLARE_DEFAULTHANDLER_void(CEntity, DeathNotice, (CBaseEntity *pVictim), (pVictim));
DECLARE_DEFAULTHANDLER(CEntity, PassesDamageFilter, bool, (const CTakeDamageInfo &info), (info));
DECLARE_DEFAULTHANDLER_void(CEntity, Precache, (), ());
DECLARE_DEFAULTHANDLER_void(CEntity, OnEntityEvent, (EntityEvent_t event, void *pEventData), (event, pEventData));
DECLARE_DEFAULTHANDLER_void(CEntity, VPhysicsDestroyObject, (), ());
DECLARE_DEFAULTHANDLER(CEntity, TakeHealth, int, (float flHealth, int bitsDamageType), (flHealth, bitsDamageType));
DECLARE_DEFAULTHANDLER(CEntity, GetAttackDamageScale, float, (CBaseEntity *pVictim), (pVictim));
DECLARE_DEFAULTHANDLER(CEntity, OnControls, bool, (CBaseEntity *pControls), (pControls));
DECLARE_DEFAULTHANDLER_void(CEntity, VPhysicsUpdate, (IPhysicsObject *pPhysics), (pPhysics));
DECLARE_DEFAULTHANDLER_void(CEntity, VPhysicsShadowUpdate, (IPhysicsObject *pPhysics), (pPhysics));
DECLARE_DEFAULTHANDLER_void(CEntity,SetModel,(const char *model),(model));
DECLARE_DEFAULTHANDLER(CEntity,IsTriggered, bool, (CBaseEntity *pActivator), (pActivator));
DECLARE_DEFAULTHANDLER_void(CEntity,FireBullets, ( const FireBulletsInfo_t &info ), (info));
DECLARE_DEFAULTHANDLER(CEntity,GetTracerType, const char *, (), ());
DECLARE_DEFAULTHANDLER(CEntity,UpdateTransmitState, int, (), ());
DECLARE_DEFAULTHANDLER_void(CEntity,SetTransmit, (CCheckTransmitInfo *pInfo, bool bAlways), (pInfo, bAlways));
DECLARE_DEFAULTHANDLER(CEntity,CanBeSeenBy, bool, (CBaseEntity *pNPC), (pNPC));
DECLARE_DEFAULTHANDLER(CEntity,IsViewable, bool, (), ());
DECLARE_DEFAULTHANDLER(CEntity,GetResponseSystem, IResponseSystem *, (), ());



//Sendprops
DEFINE_PROP(m_iTeamNum, CEntity);
DEFINE_PROP(m_vecOrigin, CEntity);
DEFINE_PROP(m_CollisionGroup, CEntity);
DEFINE_PROP(m_hOwnerEntity, CEntity);
DEFINE_PROP(m_vecVelocity, CEntity);
DEFINE_PROP(m_usSolidFlags, CEntity);
DEFINE_PROP(m_clrRender, CEntity);
DEFINE_PROP(m_nRenderMode, CEntity);
DEFINE_PROP(m_nSolidType, CEntity);
DEFINE_PROP(m_nModelIndex, CEntity);
DEFINE_PROP(m_angRotation, CEntity);
DEFINE_PROP(m_vecMins, CEntity);
DEFINE_PROP(m_vecMaxs, CEntity);
DEFINE_PROP(m_nRenderFX, CEntity);
DEFINE_PROP(m_nSurroundType, CEntity);
DEFINE_PROP(m_vecSpecifiedSurroundingMins, CEntity);
DEFINE_PROP(m_vecSpecifiedSurroundingMaxs, CEntity);
DEFINE_PROP(m_iParentAttachment, CEntity);



//Datamaps
DEFINE_PROP(m_vecAbsOrigin, CEntity);
DEFINE_PROP(m_vecAbsVelocity, CEntity);
DEFINE_PROP(m_nNextThinkTick, CEntity);
DEFINE_PROP(m_rgflCoordinateFrame, CEntity);
DEFINE_PROP(m_vecAngVelocity, CEntity);
DEFINE_PROP(m_vecBaseVelocity, CEntity);
DEFINE_PROP(m_hMoveParent, CEntity);
DEFINE_PROP(m_hMoveChild, CEntity);
DEFINE_PROP(m_hMovePeer, CEntity);
DEFINE_PROP(m_iEFlags, CEntity);
DEFINE_PROP(m_pPhysicsObject, CEntity);
DEFINE_PROP(m_pParent, CEntity);
DEFINE_PROP(m_MoveType, CEntity);
DEFINE_PROP(m_MoveCollide, CEntity);
DEFINE_PROP(m_iName, CEntity);

DEFINE_PROP(m_Collision, CEntity);
DEFINE_PROP(m_vecViewOffset, CEntity);
DEFINE_PROP(m_spawnflags, CEntity);
DEFINE_PROP(m_iHealth, CEntity);
DEFINE_PROP(m_fFlags, CEntity);
DEFINE_PROP(m_takedamage, CEntity);
DEFINE_PROP(m_Network, CEntity);
DEFINE_PROP(m_flAnimTime, CEntity);
DEFINE_PROP(m_flPrevAnimTime, CEntity);
DEFINE_PROP(m_lifeState, CEntity);
DEFINE_PROP(m_nWaterLevel, CEntity);
DEFINE_PROP(m_hGroundEntity, CEntity);
DEFINE_PROP(m_angAbsRotation, CEntity);
DEFINE_PROP(m_flGravity, CEntity);
DEFINE_PROP(m_iMaxHealth, CEntity);
DEFINE_PROP(m_fEffects, CEntity);
DEFINE_PROP(m_ModelName, CEntity);
DEFINE_PROP(m_flGroundChangeTime, CEntity);
DEFINE_PROP(m_nLastThinkTick, CEntity);
DEFINE_PROP(m_flNavIgnoreUntilTime, CEntity);
DEFINE_PROP(m_flSpeed, CEntity);
DEFINE_PROP(m_target, CEntity);
DEFINE_PROP(m_nWaterType, CEntity);
DEFINE_PROP(m_vecSurroundingMins, CEntity);
DEFINE_PROP(m_vecSurroundingMaxs, CEntity);
DEFINE_PROP(m_flRadius, CEntity);
DEFINE_PROP(m_pfnThink, CEntity);
DEFINE_PROP(m_triggerBloat, CEntity);
DEFINE_PROP(m_flFriction, CEntity);
DEFINE_PROP(m_flMoveDoneTime, CEntity);
DEFINE_PROP(m_flLocalTime, CEntity);
DEFINE_PROP(m_aThinkFunctions, CEntity);
DEFINE_PROP(m_pLink, CEntity);
DEFINE_PROP(m_ResponseContexts, CEntity);
DEFINE_PROP(m_bSimulatedEveryTick, CEntity);


/* MUST BE HERE */
DEFINE_PROP(m_iClassname, CEntity);
/* MUST BE HERE */


DECLARE_DETOUR(SetLocalOrigin, CEntity);
DECLARE_DEFAULTHANDLER_DETOUR_void(CEntity, SetLocalOrigin, (const Vector& origin ), (origin));

DECLARE_DETOUR(PhysicsTouchTriggers, CEntity);
DECLARE_DEFAULTHANDLER_DETOUR_void(CEntity, PhysicsTouchTriggers, (const Vector *pPrevAbsOrigin ), (pPrevAbsOrigin));

DECLARE_DETOUR(TakeDamage, CEntity);
DECLARE_DEFAULTHANDLER_DETOUR_void(CEntity, TakeDamage, (const CTakeDamageInfo &inputInfo), (inputInfo));

DECLARE_DETOUR(SetAbsOrigin, CEntity);
DECLARE_DEFAULTHANDLER_DETOUR_void(CEntity, SetAbsOrigin, (const Vector& absOrigin), (absOrigin));

DECLARE_DETOUR(SetLocalAngles, CEntity);
DECLARE_DEFAULTHANDLER_DETOUR_void(CEntity, SetLocalAngles, (const QAngle& angles), (angles));

DECLARE_DETOUR(AddStepDiscontinuity, CEntity);
DECLARE_DEFAULTHANDLER_DETOUR(CEntity, AddStepDiscontinuity, bool, (float flTime, const Vector &vecOrigin, const QAngle &vecAngle), (flTime, vecOrigin, vecAngle));

DECLARE_DETOUR(VPhysicsInitShadow, CEntity);
DECLARE_DEFAULTHANDLER_DETOUR(CEntity, VPhysicsInitShadow, IPhysicsObject *, ( bool allowPhysicsMovement, bool allowPhysicsRotation, solid_t *pSolid), (allowPhysicsMovement, allowPhysicsRotation, pSolid));

DECLARE_DETOUR(VPhysicsInitNormal, CEntity);
DECLARE_DEFAULTHANDLER_DETOUR(CEntity, VPhysicsInitNormal, IPhysicsObject *, (SolidType_t solidType, int nSolidFlags, bool createAsleep, solid_t *pSolid) , (solidType,  nSolidFlags, createAsleep, pSolid));

DECLARE_DETOUR(VPhysicsInitStatic, CEntity);
DECLARE_DEFAULTHANDLER_DETOUR(CEntity, VPhysicsInitStatic, IPhysicsObject *, (), ());

DECLARE_DEFAULTHANDLER(CEntity, GetDataDescMap_Real, datamap_t *, (), ());

/* Hacked Datamap declaration to fallback to the corresponding real entities one */
datamap_t CEntity::m_DataMap = { 0, 0, "CEntity", NULL };


datamap_t *CEntity::GetBaseMap()
{
	return NULL;
}
/*BEGIN_DATADESC_GUTS(CEntity)
END_DATADESC()
*/

datamap_t* CEntity::GetDataDescMap()
{
	return &m_DataMap;
}

PhysIsInCallbackFuncType PhysIsInCallback;

IServerVehicle *CEntity::GetServerVehicle()
{
	return g_helpfunc.GetServerVehicle(BaseEntity());
}

bool CEntity::DispatchKeyValue(const char *szKeyName, const char *szValue)
{
	if (!m_bInDispatchKeyValue)
		return SH_MCALL(BaseEntity(), DispatchKeyValue)(szKeyName, szValue);

	SET_META_RESULT(MRES_IGNORED);
	SH_GLOB_SHPTR->DoRecall();
	SourceHook::EmptyClass *thisptr = reinterpret_cast<SourceHook::EmptyClass*>(SH_GLOB_SHPTR->GetIfacePtr());
	if(thisptr != (void *)BaseEntity()) {
		RETURN_META_VALUE(MRES_SUPERCEDE, SH_MCALL(BaseEntity(), DispatchKeyValue)(szKeyName, szValue));
	}
	bool ret = (thisptr->*(__SoureceHook_FHM_GetRecallMFPDispatchKeyValue(thisptr)))(szKeyName, szValue);
	RETURN_META_VALUE(MRES_SUPERCEDE, ret);
}

bool ParseKeyvalue( void *pObject, typedescription_t *pFields, int iNumFields, const char *szKeyName, const char *szValue )
{
	int i;
	typedescription_t 	*pField;

	for ( i = 0; i < iNumFields; i++ )
	{
		pField = &pFields[i];

		int fieldOffset = pField->fieldOffset[ TD_OFFSET_NORMAL ];

		// Check the nested classes, but only if they aren't in array form.
		if ((pField->fieldType == FIELD_EMBEDDED) && (pField->fieldSize == 1))
		{
			for ( datamap_t *dmap = pField->td; dmap != NULL; dmap = dmap->baseMap )
			{
				void *pEmbeddedObject = (void*)((char*)pObject + fieldOffset);
				if ( ParseKeyvalue( pEmbeddedObject, dmap->dataDesc, dmap->dataNumFields, szKeyName, szValue) )
					return true;
			}
		}

		if ( (pField->flags & FTYPEDESC_KEY) && !stricmp(pField->externalName, szKeyName) )
		{
			switch( pField->fieldType )
			{
			case FIELD_MODELNAME:
			case FIELD_SOUNDNAME:
			case FIELD_STRING:
				(*(string_t *)((char *)pObject + fieldOffset)) = AllocPooledString( szValue );
				return true;

			case FIELD_TIME:
			case FIELD_FLOAT:
				(*(float *)((char *)pObject + fieldOffset)) = atof( szValue );
				return true;

			case FIELD_BOOLEAN:
				(*(bool *)((char *)pObject + fieldOffset)) = (bool)(atoi( szValue ) != 0);
				return true;

			case FIELD_CHARACTER:
				(*(char *)((char *)pObject + fieldOffset)) = (char)atoi( szValue );
				return true;

			case FIELD_SHORT:
				(*(short *)((char *)pObject + fieldOffset)) = (short)atoi( szValue );
				return true;

			case FIELD_INTEGER:
			case FIELD_TICK:
				(*(int *)((char *)pObject + fieldOffset)) = atoi( szValue );
				return true;

			case FIELD_POSITION_VECTOR:
			case FIELD_VECTOR:
				UTIL_StringToVector( (float *)((char *)pObject + fieldOffset), szValue );
				return true;

			case FIELD_VMATRIX:
			case FIELD_VMATRIX_WORLDSPACE:
				UTIL_StringToFloatArray( (float *)((char *)pObject + fieldOffset), 16, szValue );
				return true;

			case FIELD_MATRIX3X4_WORLDSPACE:
				UTIL_StringToFloatArray( (float *)((char *)pObject + fieldOffset), 12, szValue );
				return true;

			case FIELD_COLOR32:
				UTIL_StringToColor32( (color32 *) ((char *)pObject + fieldOffset), szValue );
				return true;

			case FIELD_CUSTOM:
			{
				SaveRestoreFieldInfo_t fieldInfo =
				{
					(char *)pObject + fieldOffset,
					pObject,
					pField
				};
				pField->pSaveRestoreOps->Parse( fieldInfo, szValue );
				return true;
			}

			default:
			case FIELD_INTERVAL: // Fixme, could write this if needed
			case FIELD_CLASSPTR:
			case FIELD_MODELINDEX:
			case FIELD_MATERIALINDEX:
			case FIELD_EDICT:
				Warning( "Bad field in entity!!\n" );
				Assert(0);
				break;
			}
		}
	}

	return false;
}

bool CEntity::CustomDispatchKeyValue(const char *szKeyName, const char *szValue)
{
	datamap_t *dmap = NULL;
	for ( dmap = GetDataDescMap(); dmap != NULL; dmap = dmap->baseMap )
	{
		if(ParseKeyvalue(this, dmap->dataDesc, dmap->dataNumFields, szKeyName, szValue))
		{
			return true;
		}
	}
	return DispatchKeyValue(szKeyName, szValue);
}

bool CEntity::InternalDispatchKeyValue(const char *szKeyName, const char *szValue)
{
	SET_META_RESULT(MRES_SUPERCEDE);
	CEntity *pEnt = (CEntity *)CEntity::Instance(META_IFACEPTR(CBaseEntity));
	if (!pEnt)
		RETURN_META_VALUE(MRES_IGNORED, (bool)0);
	int index = pEnt->entindex_non_network();
	pEnt->m_bInDispatchKeyValue = true;

	//if(pEnt->IsCustomEntity())
	{
		bool ret = false;
		datamap_t *dmap = NULL;
		for ( dmap = pEnt->GetDataDescMap(); dmap != NULL; dmap = dmap->baseMap )
		{
			ret = ParseKeyvalue(pEnt, dmap->dataDesc, dmap->dataNumFields, szKeyName, szValue);
			if(ret)
			{
				pEnt->m_bInDispatchKeyValue = false;
				return true;
			}
		}
	}

	bool retvalue = pEnt->DispatchKeyValue(szKeyName, szValue);
	if (pEnt == CEntity::Instance(index))
		pEnt->m_bInDispatchKeyValue = false;
	return retvalue;
}


CE_LINK_ENTITY_TO_CLASS(baseentity, CEntity);

variant_t g_Variant;


CEntity::~CEntity()
{

}

void CEntity::CE_Init(edict_t *pEdict, CBaseEntity *pBaseEntity)
{
	m_pEntity = pBaseEntity;
	m_pEdict = pEdict;

	int index = entindex_non_network();

	assert(!pEntityData[entindex_non_network()]);

	pEntityData[entindex_non_network()] = this;
	if(!m_pEntity /* || !m_pEdict*/)
		return;

	ce_m_pfnThink = NULL;
	ce_m_pfnTouch = NULL;
	ce_m_pfnUse = NULL;

	col_ptr = new CECollisionProperty(this);
}

void CEntity::Destroy()
{
	int index = entindex_non_network();
	ClearAllFlags();
	delete col_ptr;
	col_ptr = NULL;
	delete this;
	pEntityData[index] = NULL;
}

IHandleEntity *CEntity::GetIHandle() const
{
	return reinterpret_cast<IHandleEntity *>(m_pEntity);
}

/* Expanded handler for readability and since this one actually does something */
void CEntity::UpdateOnRemove()
{
	if (!m_bInUpdateOnRemove)
	{
		SH_MCALL(BaseEntity(), UpdateOnRemove);
		return;
	}

	SET_META_RESULT(MRES_IGNORED);
	SH_GLOB_SHPTR->DoRecall();
	SourceHook::EmptyClass *thisptr = reinterpret_cast<SourceHook::EmptyClass*>(SH_GLOB_SHPTR->GetIfacePtr());
	if(thisptr != (void *)BaseEntity()) {
		SH_MCALL(BaseEntity(), UpdateOnRemove);
		RETURN_META(MRES_SUPERCEDE);
	}
	(thisptr->*(__SoureceHook_FHM_GetRecallMFPUpdateOnRemove(thisptr)))();
	RETURN_META(MRES_SUPERCEDE);
}

void CEntity::InternalUpdateOnRemove()
{
	SET_META_RESULT(MRES_SUPERCEDE);

	CEntity *pEnt = CEntity::Instance(META_IFACEPTR(CBaseEntity));
	if (!pEnt)
	{
		RETURN_META(MRES_IGNORED);
	}

	int index = pEnt->entindex_non_network();
	pEnt->m_bInUpdateOnRemove = true;
	pEnt->UpdateOnRemove();
	if (pEnt == CEntity::Instance(index))
	{
		pEnt->m_bInUpdateOnRemove = false;
		//pEnt->Destroy();
	}
}


void CEntity::StartTouch(CEntity *pOther)
{
	if (!m_bInStartTouch)
	{
		SH_MCALL(BaseEntity(), StartTouch)(*pOther);
		return;
	}

	SET_META_RESULT(MRES_IGNORED);
	SH_GLOB_SHPTR->DoRecall();
	SourceHook::EmptyClass *thisptr = reinterpret_cast<SourceHook::EmptyClass*>(SH_GLOB_SHPTR->GetIfacePtr());
	if(thisptr != (void *)BaseEntity()) {
		SH_MCALL(BaseEntity(), StartTouch)(*pOther);
		RETURN_META(MRES_SUPERCEDE);
	}
	(thisptr->*(__SoureceHook_FHM_GetRecallMFPStartTouch(thisptr)))(*pOther);
	RETURN_META(MRES_SUPERCEDE);
}

void CEntity::InternalStartTouch(CBaseEntity *pOther)
{
	SET_META_RESULT(MRES_SUPERCEDE);

	CEntity *pEnt = *META_IFACEPTR(CBaseEntity);
	CEntity *pEntOther = *pOther;
	if (!pEnt || !pEntOther)
	{
		RETURN_META(MRES_IGNORED);
	}

	int index = pEnt->entindex_non_network();
	pEnt->m_bInStartTouch = true;
	pEnt->StartTouch(pEntOther);
	if (pEnt == CEntity::Instance(index))
		pEnt->m_bInStartTouch = false;
}

void CEntity::EndTouch(CEntity *pOther)
{
	if (!m_bInEndTouch)
	{
		SH_MCALL(BaseEntity(), EndTouch)(*pOther);
		return;
	}

	SET_META_RESULT(MRES_IGNORED);
	SH_GLOB_SHPTR->DoRecall();
	SourceHook::EmptyClass *thisptr = reinterpret_cast<SourceHook::EmptyClass*>(SH_GLOB_SHPTR->GetIfacePtr());
	if(thisptr != (void *)BaseEntity()) {
		SH_MCALL(BaseEntity(), EndTouch)(*pOther);
		RETURN_META(MRES_SUPERCEDE);
	}
	(thisptr->*(__SoureceHook_FHM_GetRecallMFPEndTouch(thisptr)))(*pOther);
	RETURN_META(MRES_SUPERCEDE);
}

void CEntity::InternalEndTouch(CBaseEntity *pOther)
{
	SET_META_RESULT(MRES_SUPERCEDE);

	CEntity *pEnt = *META_IFACEPTR(CBaseEntity);
	CEntity *pEntOther = *pOther;
	if (!pEnt || !pEntOther)
	{
		RETURN_META(MRES_IGNORED);
	}

	int index = pEnt->entindex_non_network();
	pEnt->m_bInEndTouch = true;
	pEnt->EndTouch(pEntOther);
	if (pEnt == CEntity::Instance(index))
		pEnt->m_bInEndTouch = false;
}

void CEntity::Touch(CEntity *pOther)
{
	if ( ce_m_pfnTouch ) 
	{
		SET_META_RESULT(MRES_SUPERCEDE);
		(this->*ce_m_pfnTouch)(pOther);
		return;
	}

	//if (m_pParent)
	//	m_pParent->Touch(pOther);

	if (!m_bInTouch)
	{
		SH_MCALL(BaseEntity(), Touch)(*pOther);
		return;
	}

	SET_META_RESULT(MRES_IGNORED);
	SH_GLOB_SHPTR->DoRecall();
	SourceHook::EmptyClass *thisptr = reinterpret_cast<SourceHook::EmptyClass*>(SH_GLOB_SHPTR->GetIfacePtr());
	if(thisptr != (void *)BaseEntity()) {
		SH_MCALL(BaseEntity(), Touch)(*pOther);
		RETURN_META(MRES_SUPERCEDE);
	}
	(thisptr->*(__SoureceHook_FHM_GetRecallMFPTouch(thisptr)))(*pOther);
	RETURN_META(MRES_SUPERCEDE);
}

void CEntity::InternalTouch(CBaseEntity *pOther)
{
	SET_META_RESULT(MRES_SUPERCEDE);

	CEntity *pEnt = *META_IFACEPTR(CBaseEntity);
	CEntity *pEntOther = *pOther;
	if (!pEnt || !pEntOther)
	{
		RETURN_META(MRES_IGNORED);
	}

	int index = pEnt->entindex_non_network();
	pEnt->m_bInTouch = true;
	pEnt->Touch(pEntOther);
	if (pEnt == CEntity::Instance(index))
		pEnt->m_bInTouch = false;
}

void CEntity::Use(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	if ( ce_m_pfnUse ) 
	{
		SET_META_RESULT(MRES_SUPERCEDE);
		(this->*ce_m_pfnUse)(pActivator, pCaller, useType, value);
		return;
	}

	if (!m_bInUse)
	{
		SH_MCALL(BaseEntity(), Use)(pActivator, pCaller, useType, value);
		return;
	}

	SET_META_RESULT(MRES_IGNORED);
	SH_GLOB_SHPTR->DoRecall();
	SourceHook::EmptyClass *thisptr = reinterpret_cast<SourceHook::EmptyClass*>(SH_GLOB_SHPTR->GetIfacePtr());
	if(thisptr != (void *)BaseEntity()) {
		SH_MCALL(BaseEntity(), Use)(pActivator, pCaller, useType, value);
		RETURN_META(MRES_SUPERCEDE);
	}
	(thisptr->*(__SoureceHook_FHM_GetRecallMFPUse(thisptr)))(pActivator, pCaller, useType, value);
	RETURN_META(MRES_SUPERCEDE);
}

void CEntity::InternalUse(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value)
{
	SET_META_RESULT(MRES_SUPERCEDE);

	CEntity *pEnt = *META_IFACEPTR(CBaseEntity);

	int index = pEnt->entindex_non_network();
	pEnt->m_bInUse = true;
	pEnt->Use(pActivator, pCaller, useType, value);
	if (pEnt == CEntity::Instance(index))
		pEnt->m_bInUse = false;
}


Vector CEntity::GetSoundEmissionOrigin()
{
if (!m_bInGetSoundEmissionOrigin)
	{
		Vector ret = SH_MCALL(BaseEntity(), GetSoundEmissionOrigin)();
		return ret;
	}

	SET_META_RESULT(MRES_IGNORED);
	SH_GLOB_SHPTR->DoRecall();
	SourceHook::EmptyClass *thisptr = reinterpret_cast<SourceHook::EmptyClass*>(SH_GLOB_SHPTR->GetIfacePtr());
	if(thisptr != (void *)BaseEntity()) {
		RETURN_META_VALUE(MRES_SUPERCEDE, SH_MCALL(BaseEntity(), GetSoundEmissionOrigin)());
	}
	RETURN_META_VALUE(MRES_SUPERCEDE, (thisptr->*(__SoureceHook_FHM_GetRecallMFPGetSoundEmissionOrigin(thisptr)))());
}

Vector CEntity::InternalGetSoundEmissionOrigin()
{
	SET_META_RESULT(MRES_SUPERCEDE);

	CEntity *pEnt = *META_IFACEPTR(CBaseEntity);
	if (!pEnt)
	{
		RETURN_META_VALUE(MRES_IGNORED, NULL);
	}

	int index = pEnt->entindex_non_network();
	pEnt->m_bInGetSoundEmissionOrigin = true;
	Vector ret = pEnt->GetSoundEmissionOrigin();
	if (pEnt == CEntity::Instance(index))
		pEnt->m_bInGetSoundEmissionOrigin = false;

	return ret;
}

void CEntity::Think()
{
	if (ce_m_pfnThink)
	{
		SET_META_RESULT(MRES_SUPERCEDE);
		(this->*ce_m_pfnThink)();
		return;
	}

	if (!m_bInThink)
	{
		SH_MCALL(BaseEntity(), Think)();
		return;
	}

	SET_META_RESULT(MRES_IGNORED);
	SH_GLOB_SHPTR->DoRecall();
	SourceHook::EmptyClass *thisptr = reinterpret_cast<SourceHook::EmptyClass*>(SH_GLOB_SHPTR->GetIfacePtr());
	if(thisptr != (void *)BaseEntity()) {
		SH_MCALL(BaseEntity(), Think)();
		RETURN_META(MRES_SUPERCEDE);
	}
	(thisptr->*(__SoureceHook_FHM_GetRecallMFPThink(thisptr)))();
	RETURN_META(MRES_SUPERCEDE);
}

void CEntity::InternalThink()
{
	SET_META_RESULT(MRES_SUPERCEDE);

	CEntity *pEnt = CEntity::Instance(META_IFACEPTR(CBaseEntity));
	if (!pEnt)
	{
		RETURN_META(MRES_IGNORED);
	}

	int index = pEnt->entindex_non_network();
	pEnt->m_bInThink = true;
	pEnt->Think();
	if (pEnt == CEntity::Instance(index))
		pEnt->m_bInThink = false;
}

void CEntity::CBaseEntityThink()
{
	CEntity *cent = CEntity::Instance(reinterpret_cast<CBaseEntity *>(this));
	if(cent)
	{
		cent->CEntityThink();
	}
}

void CEntity::CEntityThink()
{
	if(ce_m_pfnThink)
	{
		(this->*ce_m_pfnThink)();
	}
}

int	CEntity::GetIndexForThinkContext( const char *pszContext )
{
	for ( int i = 0; i < m_aThinkFunctions->Size(); i++ )
	{
		if ( !Q_strncmp( STRING( m_aThinkFunctions->Element(i).m_iszContext ), pszContext, MAX_CONTEXT_LENGTH ) )
			return i;
	}

	return NO_THINK_CONTEXT;
}

BASEPTR	CEntity::ThinkSet(BASEPTR func, float thinkTime, const char *szContext)
{
	//m_pfnThink = NULL;
	if ( !szContext )
	{
		if(func == (BASEPTR)&CAI_NPC::CallNPCThink)
		{
			*(m_pfnThink) = CAI_NPC::func_CallNPCThink;
			ce_m_pfnThink = NULL;
		} else {
			*(m_pfnThink) = reinterpret_cast<VALVE_BASEPTR>(&CEntity::CBaseEntityThink);
			ce_m_pfnThink = func;
		}
		return ce_m_pfnThink;
	}

	int iIndex = GetIndexForThinkContext( szContext );
	if ( iIndex == NO_THINK_CONTEXT )
	{
		iIndex = RegisterThinkContext( szContext );
	}

	m_aThinkFunctions->Element(iIndex).m_pfnThink = func;

	if ( thinkTime != 0 )
	{
		int thinkTick = ( thinkTime == TICK_NEVER_THINK ) ? TICK_NEVER_THINK : TIME_TO_TICKS( thinkTime );
		m_aThinkFunctions->Element(iIndex).m_nNextThinkTick = thinkTick;
		CheckHasThinkFunction( thinkTick == TICK_NEVER_THINK ? false : true );
	}
	return func;
}

int CEntity::RegisterThinkContext( const char *szContext )
{
	int iIndex = GetIndexForThinkContext( szContext );
	if ( iIndex != NO_THINK_CONTEXT )
		return iIndex;

	// Make a new think func
	thinkfunc_t sNewFunc;
	Q_memset( &sNewFunc, 0, sizeof( sNewFunc ) );
	sNewFunc.m_pfnThink = NULL;
	sNewFunc.m_nNextThinkTick = 0;
	sNewFunc.m_iszContext = AllocPooledString(szContext);

	// Insert it into our list
	return m_aThinkFunctions->AddToTail( sNewFunc );
}

float CEntity::GetNextThink( const char *szContext)
{
	/*if ( m_nNextThinkTick == TICK_NEVER_THINK )
		return TICK_NEVER_THINK;

	// Old system
	return TICK_INTERVAL * (m_nNextThinkTick );*/

	int iIndex = 0;
	if ( !szContext )
	{
		if ( m_nNextThinkTick == TICK_NEVER_THINK )
			return TICK_NEVER_THINK;

		// Old system
		return TICK_INTERVAL * (m_nNextThinkTick );
	}
	else
	{
		// Find the think function in our list
		iIndex = GetIndexForThinkContext( szContext );
	}

	if ( iIndex == m_aThinkFunctions->InvalidIndex() )
		return TICK_NEVER_THINK;

	if ( m_aThinkFunctions->Element(iIndex).m_nNextThinkTick == TICK_NEVER_THINK )
	{
		return TICK_NEVER_THINK;
	}
	return TICK_INTERVAL * (m_aThinkFunctions->Element(iIndex).m_nNextThinkTick );
}

int	CEntity::GetNextThinkTick( const char *szContext)
{
	// Are we currently in a think function with a context?
	int iIndex = 0;
	if ( !szContext )
	{
		if ( m_nNextThinkTick == TICK_NEVER_THINK )
			return TICK_NEVER_THINK;

		// Old system
		return m_nNextThinkTick;
	}
	else
	{
		// Find the think function in our list
		iIndex = GetIndexForThinkContext( szContext );

		// Looking up an invalid think context!
		Assert( iIndex != -1 );
	}

	if ( ( iIndex == -1 ) || ( m_aThinkFunctions->Element(iIndex).m_nNextThinkTick == TICK_NEVER_THINK ) )
	{
		return TICK_NEVER_THINK;
	}

	return m_aThinkFunctions->Element(iIndex).m_nNextThinkTick;
}


VALVE_BASEPTR CEntity::GetCurrentThinkPointer()
{
	return m_pfnThink;
}

void CEntity::SetNextThink(float thinkTime, const char *szContext)
{
	g_helpfunc.SetNextThink(m_pEntity, thinkTime, szContext);
}

void CEntity::AddEFlags(int nEFlagMask)
{
	m_iEFlags |= nEFlagMask;

	if ( nEFlagMask & ( EFL_FORCE_CHECK_TRANSMIT | EFL_IN_SKYBOX ) )
	{
		DispatchUpdateTransmitState();
	}
}

void CEntity::RemoveEFlags(int nEFlagMask)
{
	m_iEFlags &= ~nEFlagMask;
	if ( nEFlagMask & ( EFL_FORCE_CHECK_TRANSMIT | EFL_IN_SKYBOX ) )
		DispatchUpdateTransmitState();
}

bool CEntity::IsEFlagSet(int nEFlagMask) const
{
	return (m_iEFlags & nEFlagMask) != 0;
}

void CEntity::CheckHasThinkFunction(bool isThinking)
{
	if ( IsEFlagSet( EFL_NO_THINK_FUNCTION ) && isThinking )
	{
		RemoveEFlags( EFL_NO_THINK_FUNCTION );
	}
	else if ( !isThinking && !IsEFlagSet( EFL_NO_THINK_FUNCTION ) && !WillThink() )
	{
		AddEFlags( EFL_NO_THINK_FUNCTION );
	}

	g_helpfunc.SimThink_EntityChanged(m_pEntity);
}

bool CEntity::WillThink()
{
	if (m_nNextThinkTick > 0)
		return true;

	return false;
}

const char* CEntity::GetClassname()
{
	return STRING(*(m_iClassname));
}

void CEntity::SetClassname(const char *pClassName)
{
	m_iClassname = AllocPooledString(pClassName);
}

const char* CEntity::GetEntityName()
{
	return STRING(*(m_iName));
}

string_t CEntity::GetEntityName_String()
{
	return *(m_iName);
}

void CEntity::SetName(const char *pTargetName)
{
	m_iName = AllocPooledString(pTargetName);
}

void CEntity::ChangeTeam(int iTeamNum)
{
	m_iTeamNum = iTeamNum;
}

int CEntity::GetTeamNumber(void) const
{
	return m_iTeamNum;
}

bool CEntity::InSameTeam(CEntity *pEntity) const
{
	if (!pEntity)
		return false;

	return (pEntity->GetTeamNumber() == GetTeamNumber());
}

const Vector& CEntity::GetLocalOrigin(void) const
{
	return m_vecOrigin;
}

const Vector& CEntity::GetAbsOrigin(void) const
{
	if (IsEFlagSet(EFL_DIRTY_ABSTRANSFORM))
	{
		const_cast<CEntity*>(this)->CalcAbsolutePosition();
	}
	return m_vecAbsOrigin;
}

void CEntity::CalcAbsoluteVelocity()
{
	if (!IsEFlagSet( EFL_DIRTY_ABSVELOCITY ))
		return;

	RemoveEFlags( EFL_DIRTY_ABSVELOCITY );

	CEntity *pMoveParent = GetMoveParent();
	if ( !pMoveParent )
	{
		*(m_vecAbsVelocity) = *(m_vecVelocity);
		return;
	}

	// This transforms the local velocity into world space
	VectorRotate( m_vecVelocity, pMoveParent->EntityToWorldTransform(), m_vecAbsVelocity );

	// Now add in the parent abs velocity
	m_vecAbsVelocity = *(m_vecAbsVelocity) + pMoveParent->GetAbsVelocity();
}

const Vector &CEntity::GetAbsVelocity() const
{
	if (IsEFlagSet(EFL_DIRTY_ABSVELOCITY))
	{
		const_cast<CEntity*>(this)->CalcAbsoluteVelocity();
	}
	return m_vecAbsVelocity;
}

const Vector & CEntity::GetVelocity() const
{
	return m_vecVelocity;
}

CEntity *CEntity::GetMoveParent(void)
{
	return Instance(m_hMoveParent); 
}

CEntity *CEntity::GetRootMoveParent()
{
	CEntity *pEntity = this;
	CEntity *pParent = m_hMoveParent;
	while ( pParent )
	{
		pEntity = pParent;
		pParent = pEntity->GetMoveParent();
	}

	return pEntity;
}

int CEntity::entindex_non_network()
{
	return BaseEntity()->GetRefEHandle().GetEntryIndex();

	/*cell_t ref = gamehelpers->EntityToReference(BaseEntity());
	return gamehelpers->ReferenceToIndex(ref);*/
}

int CEntity::entindex()
{
	return engine->IndexOfEdict(m_pEdict);	
}

bool CEntity::IsPlayer()
{
	return false;
}

int CEntity::GetTeam()
{
	return m_iTeamNum;
}

#define ACCEPTINPUT_FAIL		0
#define ACCEPTINPUT_FOUND		1
#define ACCEPTINPUT_NOTFOUND	2

static int CE_AcceptInput(CEntity *pEntity, const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t Value, int outputID)
{
	for ( datamap_t *dmap = pEntity->GetDataDescMap(); dmap != NULL; dmap = dmap->baseMap )
	{
		// search through all the actions in the data description, looking for a match
		for ( int i = 0; i < dmap->dataNumFields; i++ )
		{
			if ( dmap->dataDesc[i].flags & FTYPEDESC_INPUT )
			{
				if ( !Q_stricmp(dmap->dataDesc[i].externalName, szInputName) )
				{
					// convert the value if necessary
					if ( Value.FieldType() != dmap->dataDesc[i].fieldType )
					{
						if ( !(Value.FieldType() == FIELD_VOID && dmap->dataDesc[i].fieldType == FIELD_STRING) ) // allow empty strings
						{
							if ( !Value.Convert( (fieldtype_t)dmap->dataDesc[i].fieldType ) )
							{
								pEntity->m_bInAcceptInput = false;
								return ACCEPTINPUT_FAIL;
							}
						}
					}
					// call the input handler, or if there is none just set the value
					inputfunc_t pfnInput = dmap->dataDesc[i].inputFunc;
					if ( pfnInput )
					{ 
						// Package the data into a struct for passing to the input handler.
						inputdata_t data;
						data.pActivator = pActivator;
						data.pCaller = pCaller;
						data.value = Value;
						data.nOutputID = outputID;

						(pEntity->*pfnInput)( data );
					}
					else if ( dmap->dataDesc[i].flags & FTYPEDESC_KEY )
					{
						// set the value directly
						Value.SetOther( ((char*)pEntity) + dmap->dataDesc[i].fieldOffset[ TD_OFFSET_NORMAL ] );
					
						// TODO: if this becomes evil and causes too many full entity updates, then we should make
						// a macro like this:
						//
						// define MAKE_INPUTVAR(x) void Note##x##Modified() { x.GetForModify(); }
						//
						// Then the datadesc points at that function and we call it here. The only pain is to add
						// that function for all the DEFINE_INPUT calls.
						
						// CEntity: this is custom input, need this??
						//pEnt->NetworkStateChanged();
					}
					pEntity->m_bInAcceptInput = false;
					return ACCEPTINPUT_FOUND;
				}
			}
		}
	}
	return ACCEPTINPUT_NOTFOUND;
}

bool CEntity::CustomAcceptInput(const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t Value, int outputID)
{
	int local_ret = CE_AcceptInput(this,szInputName, pActivator, pCaller, Value, outputID);
	if(local_ret < ACCEPTINPUT_NOTFOUND)
		return (local_ret != 0);

	return AcceptInput(szInputName, pActivator, pCaller, Value, outputID);
}

bool CEntity::AcceptInput(const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t Value, int outputID)
{
	if (!m_bInAcceptInput)
	{
		return SH_MCALL(BaseEntity(), AcceptInput)(szInputName, pActivator, pCaller, Value, outputID);
	}

	SET_META_RESULT(MRES_IGNORED);
	SH_GLOB_SHPTR->DoRecall();
	SourceHook::EmptyClass *thisptr = reinterpret_cast<SourceHook::EmptyClass*>(SH_GLOB_SHPTR->GetIfacePtr());
	if(thisptr != (void *)BaseEntity()) {
		RETURN_META_VALUE(MRES_SUPERCEDE, SH_MCALL(BaseEntity(), AcceptInput)(szInputName, pActivator, pCaller, Value, outputID));
	}
	bool ret = (thisptr->*(__SoureceHook_FHM_GetRecallMFPAcceptInput(thisptr)))(szInputName, pActivator, pCaller, Value, outputID);
	RETURN_META_VALUE(MRES_SUPERCEDE, ret);
}

bool CEntity::InternalAcceptInput(const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t Value, int outputID)
{
	SET_META_RESULT(MRES_SUPERCEDE);

	CEntity *pEnt = *META_IFACEPTR(CBaseEntity);
	if (!pEnt)
	{
		RETURN_META_VALUE(MRES_IGNORED, false);
	}

	int index = pEnt->entindex_non_network();
	pEnt->m_bInAcceptInput = true;

	//if(pEnt->IsCustomEntity())
	{
		int local_ret = CE_AcceptInput(pEnt,szInputName, pActivator, pCaller, Value, outputID);
		if(local_ret < ACCEPTINPUT_NOTFOUND)
			return (local_ret != 0);	
	}

	bool ret = pEnt->AcceptInput(szInputName, pActivator, pCaller, Value, outputID);

	if (pEnt == CEntity::Instance(index))
		pEnt->m_bInAcceptInput = false;

	return ret;
}

void CEntity::InitHooks()
{
	IHookTracker *pTracker = IHookTracker::m_Head;
	while (pTracker)
	{
		pTracker->AddHook(this);
		pTracker = pTracker->m_Next;
	}
}

void CEntity::InitProps()
{
	IPropTracker *pTracker = IPropTracker::m_Head;
	while (pTracker)
	{
		pTracker->InitProp(this);
		pTracker = pTracker->m_Next;
	}
}

void CEntity::ClearAllFlags()
{
	IHookTracker *pTracker = IHookTracker::m_Head;
	while (pTracker)
	{
		pTracker->ClearFlag(this);
		pTracker = pTracker->m_Next;
	}
}

const char *CEntity::GetDebugName(void)
{
	if ( this == NULL )
		return "<<null>>";

	if ( *(m_iName) != NULL_STRING ) 
	{
		return STRING(*(m_iName));
	}
	else
	{
		return STRING(*(m_iClassname));
	}
}


CEntity *CEntity::GetOwnerEntity()
{
	return m_hOwnerEntity;
}

void CEntity::SetOwner(CEntity *pOwnerEntity)
{
	(*m_hOwnerEntity.ptr).Set((pOwnerEntity)? pOwnerEntity->edict()->GetIServerEntity() : NULL);
}

void CEntity::SetCollisionGroup(int collisionGroup)
{
	if ((int)m_CollisionGroup != collisionGroup)
	{
		m_CollisionGroup = collisionGroup;
		CollisionRulesChanged();
	}
}

#define VPHYSICS_MAX_OBJECT_LIST_COUNT	1024
void CEntity::CollisionRulesChanged()
{
	// ivp maintains state based on recent return values from the collision filter, so anything
	// that can change the state that a collision filter will return (like m_Solid) needs to call RecheckCollisionFilter.
	if (VPhysicsGetObject())
	{
		if (PhysIsInCallback())
		{
			Warning("Changing collision rules within a callback is likely to cause crashes!\n");
			Assert(0);
		}
		IPhysicsObject *pList[VPHYSICS_MAX_OBJECT_LIST_COUNT];
		int count = VPhysicsGetObjectList(pList, ARRAYSIZE(pList));
		for (int i = 0; i < count; i++)
		{
			if (pList[i] != NULL) //this really shouldn't happen, but it does >_<
				pList[i]->RecheckCollisionFilter();
		}
	}
}

MoveType_t CEntity::GetMoveType() const
{
	return (MoveType_t)(unsigned char)m_MoveType;
}

void CEntity::SetMoveType( MoveType_t val, MoveCollide_t moveCollide )
{
	g_helpfunc.SetMoveType(BaseEntity(), val, moveCollide);
	//m_MoveType = MoveType;
}

MoveCollide_t CEntity::GetMoveCollide() const
{
	return (MoveCollide_t)(unsigned char)m_MoveCollide;
}

void CEntity::SetMoveCollide( MoveCollide_t MoveCollide )
{
	m_MoveCollide = MoveCollide;
}

SolidType_t CEntity::GetSolid() const
{
	return (SolidType_t)(unsigned char)m_nSolidType;
}

void CEntity::SetSolid(SolidType_t val)
{
	CollisionProp()->SetSolid(val);
}

void CEntity::SetSolidFlags(int flags)
{
	CollisionProp()->SetSolidFlags(flags);
}

void CEntity::AddSolidFlags(int nFlags)
{
	CollisionProp()->AddSolidFlags(nFlags);
}

void CEntity::RemoveSolidFlags(int nFlags)
{
	CollisionProp()->RemoveSolidFlags(nFlags);
}

bool CEntity::IsSolid() const
{
	return CollisionProp()->IsSolid( );
}

bool CEntity::IsSolidFlagSet( int flagMask ) const
{
	return CollisionProp()->IsSolidFlagSet( flagMask );
}


void CEntity::SetViewOffset(const Vector& v) 
{ 
	m_vecViewOffset = v; 
}

void CEntity::AddEffects(int nEffects)
{
	m_fEffects |= nEffects;

	if(nEffects & EF_NODRAW)
	{
		DispatchUpdateTransmitState();
	}
}

void CEntity::SetEffects( int nEffects )
{
	if ( nEffects != m_fEffects )
	{
		m_fEffects = nEffects;

		DispatchUpdateTransmitState();
	}
}

void CEntity::RemoveEffects(int nEffects)
{
	m_fEffects &= ~nEffects;
	if(nEffects & EF_NODRAW)
	{
		NetworkProp()->MarkPVSInformationDirty();
		DispatchUpdateTransmitState();
	}
}

void CEntity::ClearEffects()
{
	m_fEffects = 0;
	DispatchUpdateTransmitState();
}

#define CHANGE_FLAGS(flags,newFlags) { unsigned int old = flags; flags = (newFlags); g_helpfunc.ReportEntityFlagsChanged( m_pEntity, old, flags ); }

void CEntity::AddFlag(int flags)
{
	CHANGE_FLAGS( m_fFlags, m_fFlags | flags );
}

void CEntity::RemoveFlag(int flagsToRemove)
{
	CHANGE_FLAGS( m_fFlags, m_fFlags & ~flagsToRemove );
}

void CEntity::ToggleFlag(int flagToToggle)
{
	CHANGE_FLAGS( m_fFlags, m_fFlags ^ flagToToggle );
}

void CEntity::ClearFlags()
{
	CHANGE_FLAGS( m_fFlags, 0 );
}

int CEntity::DispatchUpdateTransmitState()
{
	return g_helpfunc.DispatchUpdateTransmitState(m_pEntity);
}

CServerNetworkProperty *CEntity::NetworkProp() const
{
	return m_Network;
}

model_t	*CEntity::GetModel()
{
	return (model_t *)modelinfo->GetModel( GetModelIndex() );
}

void CEntity::SetGroundEntity(CBaseEntity *ground)
{
	g_helpfunc.SetGroundEntity(m_pEntity, ground);
}

void CEntity::SetAbsVelocity(const Vector &vecAbsVelocity)
{
	g_helpfunc.SetAbsVelocity(m_pEntity, vecAbsVelocity);
}

CAI_NPC	*CEntity::MyNPCPointer()
{
	if(!IsNPC()) 
		return NULL;

	CAI_NPC *ce = dynamic_cast<CAI_NPC *>(this);
	return ce;
}

CEntity *CEntity::GetGroundEntity() const
{
	return m_hGroundEntity;
}

CEntity *CEntity::GetGroundEntity()
{
	return m_hGroundEntity;
}

const QAngle& CEntity::GetAbsAngles() const
{
	if (IsEFlagSet(EFL_DIRTY_ABSTRANSFORM))
	{
		const_cast<CEntity*>(this)->CalcAbsolutePosition();
	}
	return m_angAbsRotation;
}

matrix3x4_t &CEntity::EntityToWorldTransform() 
{ 
	if (IsEFlagSet(EFL_DIRTY_ABSTRANSFORM))
	{
		CalcAbsolutePosition();
	}
	return m_rgflCoordinateFrame; 
}

const matrix3x4_t &CEntity::EntityToWorldTransform() const
{ 
	if (IsEFlagSet(EFL_DIRTY_ABSTRANSFORM))
	{
		const_cast<CEntity*>(this)->CalcAbsolutePosition();
	}
	return m_rgflCoordinateFrame; 
}

void CEntity::CalcAbsolutePosition()
{
	g_helpfunc.CalcAbsolutePosition(BaseEntity());
}

void CEntity::SetAbsAngles(const QAngle& absAngles)
{
	g_helpfunc.SetAbsAngles(BaseEntity(), absAngles);
}

void CEntity::GetVectors(Vector* pForward, Vector* pRight, Vector* pUp) const
{
	const matrix3x4_t &entityToWorld = EntityToWorldTransform();

	if (pForward != NULL)
	{
		MatrixGetColumn( entityToWorld, 0, *pForward ); 
	}

	if (pRight != NULL)
	{
		MatrixGetColumn( entityToWorld, 1, *pRight ); 
		*pRight *= -1.0f;
	}

	if (pUp != NULL)
	{
		MatrixGetColumn( entityToWorld, 2, *pUp ); 
	}
}

void CEntity::ApplyAbsVelocityImpulse( const Vector &vecImpulse )
{
	if (vecImpulse != vec3_origin )
	{
		if ( GetMoveType() == MOVETYPE_VPHYSICS )
		{
			VPhysicsGetObject()->AddVelocity( &vecImpulse, NULL );
		}
		else
		{
			// NOTE: Have to use GetAbsVelocity here to ensure it's the correct value
			Vector vecResult;
			VectorAdd( GetAbsVelocity(), vecImpulse, vecResult );
			SetAbsVelocity( vecResult );
		}
	}
}

void CEntity::ApplyLocalAngularVelocityImpulse( const AngularImpulse &angImpulse )
{
	if (angImpulse != vec3_origin )
	{
		// Safety check against receive a huge impulse, which can explode physics
		if ( !IsEntityAngularVelocityReasonable( angImpulse ) )
		{
			//Warning( "Bad ApplyLocalAngularVelocityImpulse(%f,%f,%f) on %s\n", angImpulse.x, angImpulse.y, angImpulse.z, GetDebugName() );
			Assert( false );
			return;
		}

		if ( GetMoveType() == MOVETYPE_VPHYSICS )
		{
			VPhysicsGetObject()->AddVelocity( NULL, &angImpulse );
		}
		else
		{
			QAngle vecResult;
			AngularImpulseToQAngle( angImpulse, vecResult );
			VectorAdd( GetLocalAngularVelocity(), vecResult, vecResult );
			SetLocalAngularVelocity( vecResult );
		}
	}
}

CEntity *CEntity::Create( const char *szName, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner )
{
	CEntity *pEntity = CreateNoSpawn( szName, vecOrigin, vecAngles, pOwner );

	DispatchSpawn( (pEntity)?pEntity->BaseEntity():NULL );
	return pEntity;
}

CEntity *CEntity::CreateNoSpawn( const char *szName, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner)
{
	return g_helpfunc.CreateNoSpawn(szName, vecOrigin, vecAngles, pOwner);
}

HSOUNDSCRIPTHANDLE CEntity::PrecacheScriptSound( const char *soundname )
{
	return g_helpfunc.PrecacheScriptSound(soundname);
}

void CEntity::EmitSound( const char *soundname, float soundtime, float *duration)
{
	return g_helpfunc.EmitSound(BaseEntity(), soundname, soundtime, duration);
}

void CEntity::EmitSound( const char *soundname, HSOUNDSCRIPTHANDLE& handle, float soundtime , float *duration )
{
	CPASAttenuationFilter filter( this, soundname, handle );

	EmitSound_t params;
	params.m_pSoundName = soundname;
	params.m_flSoundTime = soundtime;
	params.m_pflSoundDuration = duration;
	params.m_bWarnOnDirectWaveReference = true;

	g_helpfunc.EmitSoundByHandle(filter, entindex(), params, handle);
}

void CEntity::EmitSound( IRecipientFilter& filter, int iEntIndex, const EmitSound_t & params )
{
	g_helpfunc.EmitSound(filter, iEntIndex, params);
}

void CEntity::EmitSound( IRecipientFilter& filter, int iEntIndex, const char *soundname, const Vector *pOrigin /*= NULL*/, float soundtime /*= 0.0f*/, float *duration /*=NULL*/ )
{
	g_helpfunc.EmitSound(filter, iEntIndex, soundname, pOrigin, soundtime, duration);
}

void CEntity::RemoveDeferred( void )
{
	g_helpfunc.RemoveDeferred(BaseEntity());
}

bool CEntity::IsBSPModel() const
{
	if ( GetSolid() == SOLID_BSP )
		return true;
	
	const model_t *model = modelinfo->GetModel( GetModelIndex() );

	if ( GetSolid() == SOLID_VPHYSICS && modelinfo->GetModelType( model ) == mod_brush )
		return true;

	return false;
}

void CEntity::PhysicsStepRecheckGround()
{
	unsigned int mask = PhysicsSolidMaskForEntity();
	// determine if it's on solid ground at all
	Vector	mins, maxs, point;
	int		x, y;
	trace_t trace;
	
	VectorAdd (GetAbsOrigin(), WorldAlignMins(), mins);
	VectorAdd (GetAbsOrigin(), WorldAlignMaxs(), maxs);
	point[2] = mins[2] - 1;
	for	(x=0 ; x<=1 ; x++)
	{
		for	(y=0 ; y<=1 ; y++)
		{
			point[0] = x ? maxs[0] : mins[0];
			point[1] = y ? maxs[1] : mins[1];

			ICollideable *pCollision = GetCollideable();

			if ( pCollision && IsNPC() )
			{
				UTIL_TraceLineFilterEntity( BaseEntity(), point, point, mask, COLLISION_GROUP_NONE, &trace );
			}
			else
			{
				UTIL_TraceLine( point, point, mask, BaseEntity(), COLLISION_GROUP_NONE, &trace );
			}

			if ( trace.startsolid )
			{
				SetGroundEntity( trace.m_pEnt );
				return;
			}
		}
	}
}

soundlevel_t CEntity::LookupSoundLevel( const char *soundname )
{
	return soundemitterbase->LookupSoundLevel( soundname );
}


soundlevel_t CEntity::LookupSoundLevel( const char *soundname, HSOUNDSCRIPTHANDLE& handle )
{
	return soundemitterbase->LookupSoundLevelByHandle( soundname, handle );
}

float CEntity::GetGroundChangeTime( void )
{
	return m_flGroundChangeTime;
}

float CEntity::GetLastThink( const char *szContext )
{
	assert((szContext == NULL));
	return m_nLastThinkTick * TICK_INTERVAL;
}


void CEntity::VelocityPunch( const Vector &vecForce )
{
	CPlayer *pPlayer = ToBasePlayer(this);
	if ( pPlayer )
	{
		pPlayer->CPlayer::VelocityPunch( vecForce );
	}
}

CEntity *CEntity::GetNextTarget( void )
{
	if (!(*(m_target)) )
		return NULL;
	return g_helpfunc.FindEntityByName( (CBaseEntity *)NULL, m_target );
}


FORCEINLINE bool NamesMatch( const char *pszQuery, string_t nameToMatch )
{
	if ( nameToMatch == NULL_STRING )
		return (*pszQuery == 0 || *pszQuery == '*');

	const char *pszNameToMatch = STRING(nameToMatch);

	// If the pointers are identical, we're identical
	if ( pszNameToMatch == pszQuery )
		return true;

	while ( *pszNameToMatch && *pszQuery )
	{
		char cName = *pszNameToMatch;
		char cQuery = *pszQuery;
		if ( cName != cQuery && tolower(cName) != tolower(cQuery) ) // people almost always use lowercase, so assume that first
			break;
		++pszNameToMatch;
		++pszQuery;
	}

	if ( *pszQuery == 0 && *pszNameToMatch == 0 )
		return true;

	// @TODO (toml 03-18-03): Perhaps support real wildcards. Right now, only thing supported is trailing *
	if ( *pszQuery == '*' )
		return true;

	return false;
}


bool CEntity::NameMatchesComplex( const char *pszNameOrWildcard )
{
	if ( !Q_stricmp( "!player", pszNameOrWildcard) )
		return IsPlayer();

	return NamesMatch( pszNameOrWildcard, m_iName );
}

bool CEntity::ClassMatchesComplex( const char *pszClassOrWildcard )
{
	return NamesMatch( pszClassOrWildcard, m_iClassname );
}

void CEntity::SUB_Remove( void )
{
	if (m_iHealth > 0)
	{
		m_iHealth = 0;
	}

	UTIL_Remove( this );
}

void CEntity::SUB_DoNothing( void )
{

}

void CEntity::SUB_TouchNothing(CEntity *pOther)
{

}

void CEntity::Remove( void )
{
	UTIL_Remove( this );
}

int CEntity::GetWaterType() const
{
	int out = 0;
	if ( m_nWaterType & 1 )
		out |= CONTENTS_WATER;
	if ( m_nWaterType & 2 )
		out |= CONTENTS_SLIME;
	return out;
}

bool CEntity::PhysicsCheckWater( void )
{
	if (GetMoveParent())
		return GetWaterLevel() > 1;

	int cont = GetWaterType();

	// If we're not in water + don't have a current, we're done
	if ( ( cont & (MASK_WATER | MASK_CURRENT) ) != (MASK_WATER | MASK_CURRENT) )
		return GetWaterLevel() > 1;

	// Compute current direction
	Vector v( 0, 0, 0 );
	if ( cont & CONTENTS_CURRENT_0 )
	{
		v[0] += 1;
	}
	if ( cont & CONTENTS_CURRENT_90 )
	{
		v[1] += 1;
	}
	if ( cont & CONTENTS_CURRENT_180 )
	{
		v[0] -= 1;
	}
	if ( cont & CONTENTS_CURRENT_270 )
	{
		v[1] -= 1;
	}
	if ( cont & CONTENTS_CURRENT_UP )
	{
		v[2] += 1;
	}
	if ( cont & CONTENTS_CURRENT_DOWN )
	{
		v[2] -= 1;
	}

	// The deeper we are, the stronger the current.
	Vector newBaseVelocity;
	VectorMA (GetBaseVelocity(), 50.0*GetWaterLevel(), v, newBaseVelocity);
	SetBaseVelocity( newBaseVelocity );
	
	return GetWaterLevel() > 1;
}

void CEntity::TraceBleed( float flDamage, const Vector &vecDir, trace_t *ptr, int bitsDamageType )
{
	if ((BloodColor() == DONT_BLEED) || (BloodColor() == BLOOD_COLOR_MECH))
	{
		return;
	}

	if (flDamage == 0)
		return;

	if (! (bitsDamageType & (DMG_CRUSH | DMG_BULLET | DMG_SLASH | DMG_BLAST | DMG_CLUB | DMG_AIRBOAT)))
		return;

	// make blood decal on the wall!
	trace_t Bloodtr;
	Vector vecTraceDir;
	float flNoise;
	int cCount;
	int i;

	if ( !IsAlive() )
	{
		// dealing with a dead npc.
		if ( GetMaxHealth() <= 0 )
		{
			// no blood decal for a npc that has already decalled its limit.
			return;
		}
		else
		{
			m_iMaxHealth -= 1;
		}
	}

	if (flDamage < 10)
	{
		flNoise = 0.1;
		cCount = 1;
	}
	else if (flDamage < 25)
	{
		flNoise = 0.2;
		cCount = 2;
	}
	else
	{
		flNoise = 0.3;
		cCount = 4;
	}

	float flTraceDist = (bitsDamageType & DMG_AIRBOAT) ? 384 : 172;
	for ( i = 0 ; i < cCount ; i++ )
	{
		vecTraceDir = vecDir * -1;// trace in the opposite direction the shot came from (the direction the shot is going)

		vecTraceDir.x += enginerandom->RandomFloat( -flNoise, flNoise );
		vecTraceDir.y += enginerandom->RandomFloat( -flNoise, flNoise );
		vecTraceDir.z += enginerandom->RandomFloat( -flNoise, flNoise );

		// Don't bleed on grates.
		UTIL_TraceLine( ptr->endpos, ptr->endpos + vecTraceDir * -flTraceDist, MASK_SOLID_BRUSHONLY & ~CONTENTS_GRATE, BaseEntity(), COLLISION_GROUP_NONE, &Bloodtr);

		if ( Bloodtr.fraction != 1.0 )
		{
			UTIL_BloodDecalTrace( &Bloodtr, BloodColor() );
		}
	}
}

void CEntity::StopSound( const char *soundname )
{
	StopSound( entindex(), soundname );
}


void CEntity::StopSound( int iEntIndex, const char *soundname )
{
	Sound_StopSound( iEntIndex, soundname );
}

void CEntity::StopSound( int iEntIndex, int iChannel, const char *pSample )
{
	Sound_StopSound(iEntIndex, iChannel, pSample);
}	

bool CEntity::GetParametersForSound( const char *soundname, CSoundParameters &params, const char *actormodel )
{
	gender_t gender = soundemitterbase->GetActorGender( actormodel );
	
	return soundemitterbase->GetParametersForSound( soundname, params, gender );
}

bool CEntity::GetParametersForSound( const char *soundname, HSOUNDSCRIPTHANDLE& handle, CSoundParameters &params, const char *actormodel )
{
	gender_t gender = soundemitterbase->GetActorGender( actormodel );
	
	return soundemitterbase->GetParametersForSoundEx( soundname, handle, params, gender );
}

void CEntity::PhysicsMarkEntitiesAsTouching( CBaseEntity *other, trace_t &trace )
{
	g_helpfunc.PhysicsMarkEntitiesAsTouching(BaseEntity(), other, trace);
}

int	CEntity::PrecacheModel( const char *name )
{
	return g_helpfunc.PrecacheModel(name);
}



void CEntity::PhysicsCheckWaterTransition( void )
{
	int oldcont = GetWaterType();
	UpdateWaterState();
	int cont = GetWaterType();

	// We can exit right out if we're a child... don't bother with this...
	if (GetMoveParent())
		return;

	if ( cont & MASK_WATER )
	{
		if (oldcont == CONTENTS_EMPTY)
		{
			Splash();

			// just crossed into water
			EmitSound( "BaseEntity.EnterWater" );

			if ( !IsEFlagSet( EFL_NO_WATER_VELOCITY_CHANGE ) )
			{
				Vector vecAbsVelocity = GetAbsVelocity();
				vecAbsVelocity[2] *= 0.5;
				SetAbsVelocity( vecAbsVelocity );
			}
		}
	}
	else
	{
		if ( oldcont != CONTENTS_EMPTY )
		{	
			// just crossed out of water
			EmitSound( "BaseEntity.ExitWater" );
		}		
	}
}

void CEntity::UpdateWaterState()
{
	// FIXME: This computation is nonsensical for rigid child attachments
	// Should we just grab the type + level of the parent?
	// Probably for rigid children anyways...

	// Compute the point to check for water state
	Vector	point;
	CollisionProp()->NormalizedToWorldSpace( Vector( 0.5f, 0.5f, 0.0f ), &point );

	SetWaterLevel( 0 );
	SetWaterType( CONTENTS_EMPTY );
	int cont = UTIL_PointContents (point);

	if (( cont & MASK_WATER ) == 0)
		return;

	SetWaterType( cont );
	SetWaterLevel( 1 );

	// point sized entities are always fully submerged
	if ( IsPointSized() )
	{
		SetWaterLevel( 3 );
	}
	else
	{
		// Check the exact center of the box
		point[2] = WorldSpaceCenter().z;

		int midcont = UTIL_PointContents (point);
		if ( midcont & MASK_WATER )
		{
			// Now check where the eyes are...
			SetWaterLevel( 2 );
			point[2] = EyePosition().z;

			int eyecont = UTIL_PointContents (point);
			if ( eyecont & MASK_WATER )
			{
				SetWaterLevel( 3 );
			}
		}
	}
}

void CEntity::SetWaterType( int nType )
{
	m_nWaterType = 0;
	if ( nType & CONTENTS_WATER )
		m_nWaterType |= 1;
	if ( nType & CONTENTS_SLIME )
		m_nWaterType |= 2;
}

void CEntity::FollowEntity( CBaseEntity *pBaseEntity, bool bBoneMerge )
{
	if (pBaseEntity)
	{
		SetParent( pBaseEntity );
		SetMoveType( MOVETYPE_NONE );
		
		if ( bBoneMerge )
			AddEffects( EF_BONEMERGE );

		AddSolidFlags( FSOLID_NOT_SOLID );
		SetLocalOrigin( vec3_origin );
		SetLocalAngles( vec3_angle );
	}
	else
	{
		StopFollowingEntity();
	}
}

void CEntity::StopFollowingEntity( )
{
	if( !IsFollowingEntity() )
	{
		Assert( IsEffectActive( EF_BONEMERGE ) == 0 );
		return;
	}

	SetParent( NULL );
	RemoveEffects( EF_BONEMERGE );
	RemoveSolidFlags( FSOLID_NOT_SOLID );
	SetMoveType( MOVETYPE_NONE );
	CollisionRulesChanged();
}

bool CEntity::IsFollowingEntity()
{
	return IsEffectActive( EF_BONEMERGE ) && (GetMoveType() == MOVETYPE_NONE) && GetMoveParent();
}

void CEntity::ViewPunch( const QAngle &angleOffset )
{
	CPlayer *pPlayer = ToBasePlayer(this);
	if ( pPlayer )
	{
		pPlayer->CPlayer::ViewPunch( angleOffset );
	}
}

void CEntity::SetCollisionBounds( const Vector& mins, const Vector &maxs )
{
	col_ptr->SetCollisionBounds( mins, maxs );
}

void CEntity::DispatchTraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr )
{
	class CHackedDmgAccumulator
	{
	public:
			CHackedDmgAccumulator( void ):
				m_bActive(false)
			{
			}

			virtual void Start( void ) { m_bActive = true; }
			//virtual void AccumulateMultiDamage( const CTakeDamageInfo &info, CBaseEntity *pEntity );
			//virtual void Process( void );

	private:
			CTakeDamageInfo                                        m_updatedInfo;
			CUtlMap< int, CTakeDamageInfo >        m_TargetsDmgInfo;

	private:
			bool                                                        m_bActive;
	};

	// Make sure our damage filter allows the damage.
	if ( !PassesDamageFilter( info ))
	{
		return;
	}

	CHackedDmgAccumulator accumulator;
	TraceAttack( info, vecDir, ptr, (CDmgAccumulator*)&accumulator );
}

void CEntity::SetBlocksLOS( bool bBlocksLOS )
{
	if ( bBlocksLOS )
	{
		RemoveEFlags( EFL_DONTBLOCKLOS );
	}
	else
	{
		AddEFlags( EFL_DONTBLOCKLOS );
	}
}

bool CEntity::BlocksLOS( void ) 
{ 
	return !IsEFlagSet(EFL_DONTBLOCKLOS); 
}

#define BASEENTITY_MSG_REMOVE_DECALS	1

void CEntity::RemoveAllDecals()
{
	bf_write *buffer = engine->EntityMessageBegin( entindex(), GetServerClass(), false );
	if(!buffer)
		return;
	buffer->WriteByte(BASEENTITY_MSG_REMOVE_DECALS);
	engine->MessageEnd();
}

void CEntity::ComputeAbsPosition( const Vector &vecLocalPosition, Vector *pAbsPosition )
{
	CEntity *pMoveParent = GetMoveParent();
	if ( !pMoveParent )
	{
		*pAbsPosition = vecLocalPosition;
	}
	else
	{
		VectorTransform( vecLocalPosition, pMoveParent->EntityToWorldTransform(), *pAbsPosition );
	}
}

void *CEntity::GetDataObject( int type )
{
	return g_helpfunc.GetDataObject(BaseEntity(), type);
}

bool CEntity::HasNPCsOnIt( void )
{
	groundlink_t *link;
	groundlink_t *root = ( groundlink_t * )GetDataObject( GROUNDLINK );
	if ( root )
	{
		for ( link = root->nextLink; link != root; link = link->nextLink )
		{
			CEntity *cent = CEntity::Instance(link->entity);
			if ( cent && cent->MyNPCPointer() )
				return true;
		}
	}

	return false;
}

trace_t *g_TouchTrace;
const trace_t *CEntity::GetTouchTrace( void )
{
	return g_TouchTrace;
}

void CEntity::SetLocalAngularVelocity( const QAngle &vecAngVelocity )
{
	if (*(m_vecAngVelocity) != vecAngVelocity)
	{
//		InvalidatePhysicsRecursive( EFL_DIRTY_ABSANGVELOCITY );
		m_vecAngVelocity = vecAngVelocity;
	}
}

const QAngle &CEntity::GetLocalAngularVelocity( ) const
{
	return *(m_vecAngVelocity);
}

void CEntity::SetMoveDoneTime( float flDelay )
{
	if (flDelay >= 0)
	{
		m_flMoveDoneTime = GetLocalTime() + flDelay;
	}
	else
	{
		m_flMoveDoneTime = -1;
	}
	CheckHasGamePhysicsSimulation();
}


void CEntity::CheckHasGamePhysicsSimulation()
{
	g_helpfunc.CheckHasGamePhysicsSimulation(BaseEntity());
}

int	CEntity::CBaseEntity_ObjectCaps( void )
{
	model_t *pModel = GetModel();
	bool bIsBrush = ( pModel && modelinfo->GetModelType( pModel ) == mod_brush );

	// We inherit our parent's use capabilities so that we can forward use commands
	// to our parent.
	CEntity *pParent = GetParent();
	if ( pParent )
	{
		int caps = pParent->ObjectCaps();

		if ( !bIsBrush )
			caps &= ( FCAP_ACROSS_TRANSITION | FCAP_IMPULSE_USE | FCAP_CONTINUOUS_USE | FCAP_ONOFF_USE | FCAP_DIRECTIONAL_USE );
		else
			caps &= ( FCAP_IMPULSE_USE | FCAP_CONTINUOUS_USE | FCAP_ONOFF_USE | FCAP_DIRECTIONAL_USE );

		if ( pParent->IsPlayer() )
			caps |= FCAP_ACROSS_TRANSITION;

		return caps;
	}
	else if ( !bIsBrush ) 
	{
		return FCAP_ACROSS_TRANSITION;
	}

	return 0;
}

int	CEntity::SetTransmitState( int nFlag)
{
	edict_t *ed = edict();

	if ( !ed )
		return 0;

	// clear current flags = check ShouldTransmit()
	ed->ClearTransmitState();	
	
	int oldFlags = ed->m_fStateFlags;
	ed->m_fStateFlags |= nFlag;
	
	// Tell the engine (used for a network backdoor optimization).
	if ( (oldFlags & FL_EDICT_DONTSEND) != (ed->m_fStateFlags & FL_EDICT_DONTSEND) )
		engine->NotifyEdictFlagsChange( entindex() );

	return ed->m_fStateFlags;
}

bool CEntity::DetectInSkybox()
{
	if ( GetEntitySkybox() != NULL )
	{
		AddEFlags( EFL_IN_SKYBOX );
		return true;
	}

	RemoveEFlags( EFL_IN_SKYBOX );
	return false;
}

CE_CSkyCamera *CEntity::GetEntitySkybox()
{
	int area = engine->GetArea( WorldSpaceCenter() );

	CE_CSkyCamera *pCur = GetSkyCameraList();
	while ( pCur )
	{
		if ( engine->CheckAreasConnected( area, pCur->m_skyboxData->area ) )
			return pCur;

		pCur = pCur->m_pNext;
	}

	return NULL;
}

void CEntity::GenderExpandString( char const *in, char *out, int maxlen )
{
	soundemitterbase->GenderExpandString( STRING(GetModelName()) , in, out, maxlen );
}

void CEntity::EmitSentenceByIndex( IRecipientFilter& filter, int iEntIndex, int iChannel, int iSentenceIndex, 
	float flVolume, soundlevel_t iSoundlevel, int iFlags /*= 0*/, int iPitch /*=PITCH_NORM*/,
	const Vector *pOrigin /*=NULL*/, const Vector *pDirection /*=NULL*/, 
	bool bUpdatePositions /*=true*/, float soundtime /*=0.0f*/ )
{
	CUtlVector< Vector > dummy;
	enginesound->EmitSentenceByIndex( filter, iEntIndex, iChannel, iSentenceIndex, 
		flVolume, iSoundlevel, iFlags, iPitch, 0, pOrigin, pDirection, &dummy, bUpdatePositions, soundtime );
}

extern ConVar *ai_LOS_mode;
bool CEntity::CBaseEntity_FVisible( const Vector &vecTarget, int traceMask, CBaseEntity **ppBlocker )
{
	trace_t tr;
	Vector vecLookerOrigin = EyePosition();// look through the caller's 'eyes'

	if ( ai_LOS_mode->GetBool() )
	{
		UTIL_TraceLine( vecLookerOrigin, vecTarget, traceMask, BaseEntity(), COLLISION_GROUP_NONE, &tr);
	}
	else
	{
		// If we're doing an LOS search, include NPCs.
		if ( traceMask == MASK_BLOCKLOS )
		{
			traceMask = MASK_BLOCKLOS_AND_NPCS;
		}

		// Player sees through nodraw and blocklos
		if ( IsPlayer() )
		{
			traceMask |= CONTENTS_IGNORE_NODRAW_OPAQUE;
			traceMask &= ~CONTENTS_BLOCKLOS;
		}

		// Use the custom LOS trace filter
		CTraceFilterLOS traceFilter( BaseEntity(), COLLISION_GROUP_NONE );
		UTIL_TraceLine( vecLookerOrigin, vecTarget, traceMask, &traceFilter, &tr );
	}

	if (tr.fraction != 1.0)
	{
		if (ppBlocker)
		{
			*ppBlocker = tr.m_pEnt;
		}
		return false;// Line of sight is not established
	}

	return true;// line of sight is valid.
}

static const char *UTIL_TranslateSoundName( const char *soundname, const char *actormodel )
{
	Assert( soundname );

	if ( Q_stristr( soundname, ".wav" ) || Q_stristr( soundname, ".mp3" ) )
	{
		if ( Q_stristr( soundname, ".wav" ) )
		{
			//WaveTrace( soundname, "UTIL_TranslateSoundName" );
		}
		return soundname;
	}

	return soundemitterbase->GetWavFileForSound( soundname, actormodel );
}

float CEntity::GetSoundDuration( const char *soundname, char const *actormodel )
{
	return enginesound->GetSoundDuration( PSkipSoundChars( UTIL_TranslateSoundName( soundname, actormodel ) ) );
}

void CEntity::AddContext( const char *contextName )
{
	char key[ 128 ];
	char value[ 128 ];
	float duration;

	const char *p = contextName;
	while ( p )
	{
		duration = 0.0f;
		p = SplitContext( p, key, sizeof( key ), value, sizeof( value ), &duration );
		if ( duration )
		{
			duration += gpGlobals->curtime;
		}

		int iIndex = FindContextByName( key );
		if ( iIndex != -1 )
		{
			// Set the existing context to the new value
			m_ResponseContexts->Element(iIndex).m_iszValue = AllocPooledString( value );
			m_ResponseContexts->Element(iIndex).m_fExpirationTime = duration;
			continue;
		}

		ResponseContext_t newContext;
		newContext.m_iszName = AllocPooledString( key );
		newContext.m_iszValue = AllocPooledString( value );
		newContext.m_fExpirationTime = duration;

		m_ResponseContexts->AddToTail( newContext );
	}
}


int CEntity::FindContextByName( const char *name ) const
{
	int c = m_ResponseContexts.ptr->Count();
	for ( int i = 0; i < c; i++ )
	{
		if ( FStrEq( name, GetContextName( i ) ) )
			return i;
	}

	return -1;
}

const char *CEntity::GetContextName( int index ) const
{
	if ( index < 0 || index >= m_ResponseContexts.ptr->Count() )
	{
		Assert( 0 );
		return "";
	}

	return  m_ResponseContexts.ptr->Element(index).m_iszName.ToCStr();
}

void CEntity::FireBullets( int cShots, const Vector &vecSrc, 
	const Vector &vecDirShooting, const Vector &vecSpread, float flDistance, 
	int iAmmoType, int iTracerFreq, int firingEntID, int attachmentID,
	float iDamage, CBaseEntity *pAttacker, bool bFirstShotAccurate )
{
	FireBulletsInfo_t info;
	info.m_iShots = cShots;
	info.m_vecSrc = vecSrc;
	info.m_vecDirShooting = vecDirShooting;
	info.m_vecSpread = vecSpread;
	info.m_flDistance = flDistance;
	info.m_iAmmoType = iAmmoType;
	info.m_iTracerFreq = iTracerFreq;
	info.m_iDamage = iDamage;
	info.m_pAttacker = pAttacker;
	info.m_nFlags = bFirstShotAccurate ? FIRE_BULLETS_FIRST_SHOT_ACCURATE : 0;

	FireBullets( info );
}






#include "tier0/memdbgoff.h"

//-----------------------------------------------------------------------------
// CBaseEntity new/delete
// allocates and frees memory for itself from the engine->
// All fields in the object are all initialized to 0.
//-----------------------------------------------------------------------------
void *CEntity::operator new( size_t stAllocateBlock )
{
	// call into engine to get memory
	Assert( stAllocateBlock != 0 );
	return engine->PvAllocEntPrivateData(stAllocateBlock);
};

void *CEntity::operator new( size_t stAllocateBlock, int nBlockUse, const char *pFileName, int nLine )
{
	// call into engine to get memory
	Assert( stAllocateBlock != 0 );
	return engine->PvAllocEntPrivateData(stAllocateBlock);
}

void CEntity::operator delete( void *pMem )
{
	// get the engine to free the memory
	engine->FreeEntPrivateData( pMem );
}

#include "tier0/memdbgon.h"




