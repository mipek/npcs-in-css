
#include "CPropVehicle.h"
#include "vehicles.h"
#include "fourwheelvehiclephysics.h"
#include "vehicle_base.h"
#include "CPlayer.h"
#include "CSprite.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define SF_PROP_VEHICLE_ALWAYSTHINK		0x00000001

CE_LINK_ENTITY_TO_CLASS(CPropVehicle, CE_CPropVehicle);
CE_LINK_ENTITY_TO_CLASS(CPropVehicleDriveable, CE_CPropVehicleDriveable);

// CE_CPropVehicle
DEFINE_PROP(m_nVehicleType, CE_CPropVehicle);
DEFINE_PROP(m_VehiclePhysics, CE_CPropVehicle);





// CE_CPropVehicleDriveable
DEFINE_PROP(m_nSpeed, CE_CPropVehicleDriveable);
DEFINE_PROP(m_nRPM, CE_CPropVehicleDriveable);
DEFINE_PROP(m_flThrottle, CE_CPropVehicleDriveable);
DEFINE_PROP(m_nBoostTimeLeft, CE_CPropVehicleDriveable);
DEFINE_PROP(m_nHasBoost, CE_CPropVehicleDriveable);
DEFINE_PROP(m_vecEyeExitEndpoint, CE_CPropVehicleDriveable);
DEFINE_PROP(m_vecGunCrosshair, CE_CPropVehicleDriveable);
DEFINE_PROP(m_bUnableToFire, CE_CPropVehicleDriveable);
DEFINE_PROP(m_bHasGun, CE_CPropVehicleDriveable);
DEFINE_PROP(m_nScannerDisabledWeapons, CE_CPropVehicleDriveable);
DEFINE_PROP(m_nScannerDisabledVehicle, CE_CPropVehicleDriveable);
DEFINE_PROP(m_bEnterAnimOn, CE_CPropVehicleDriveable);
DEFINE_PROP(m_bExitAnimOn, CE_CPropVehicleDriveable);
DEFINE_PROP(m_hPlayer, CE_CPropVehicleDriveable);

DEFINE_PROP(m_pServerVehicle, CE_CPropVehicleDriveable);
DEFINE_PROP(m_bEngineLocked, CE_CPropVehicleDriveable);
DEFINE_PROP(m_bLocked, CE_CPropVehicleDriveable);
DEFINE_PROP(m_flMinimumSpeedToEnterExit, CE_CPropVehicleDriveable);
DEFINE_PROP(m_flTurnOffKeepUpright, CE_CPropVehicleDriveable);
DEFINE_PROP(m_hNPCDriver, CE_CPropVehicleDriveable);

DEFINE_PROP(m_pressedAttack, CE_CPropVehicleDriveable);
DEFINE_PROP(m_pressedAttack2, CE_CPropVehicleDriveable);
DEFINE_PROP(m_attackaxis, CE_CPropVehicleDriveable);
//DEFINE_PROP(m_attackaxis2, CE_CPropVehicleDriveable);


// CE_CPropVehicleDriveable
SH_DECL_MANUALHOOK2_void(ProcessMovement, 0, 0, 0, CBaseEntity *, CMoveData *);
DECLARE_DEFAULTHANDLER_SUBCLASS_void(CE_CPropVehicleDriveable, IDrivableVehicle, ProcessMovement, (CBaseEntity *pPlayer, CMoveData *pMoveData), (pPlayer, pMoveData));
DECLARE_HOOK_SUBCLASS(ProcessMovement, CE_CPropVehicleDriveable, IDrivableVehicle);

SH_DECL_MANUALHOOK4_void(SetupMove, 0, 0, 0, CBaseEntity *, CUserCmd *, IMoveHelper *, CMoveData *);
DECLARE_DEFAULTHANDLER_SUBCLASS_void(CE_CPropVehicleDriveable, IDrivableVehicle, SetupMove, (CBaseEntity *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move), (player, ucmd, pHelper, move));
DECLARE_HOOK_SUBCLASS(SetupMove, CE_CPropVehicleDriveable, IDrivableVehicle);

SH_DECL_MANUALHOOK2(AllowBlockedExit, 0, 0, 0, bool, CBaseEntity *, int);
DECLARE_DEFAULTHANDLER_SUBCLASS(CE_CPropVehicleDriveable, IDrivableVehicle, AllowBlockedExit, bool, (CBaseEntity *pPlayer, int nRole), (pPlayer, nRole));
DECLARE_HOOK_SUBCLASS(AllowBlockedExit, CE_CPropVehicleDriveable, IDrivableVehicle);

SH_DECL_MANUALHOOK1(CanEnterVehicle, 0, 0, 0, bool, CBaseEntity *);
DECLARE_DEFAULTHANDLER_SUBCLASS(CE_CPropVehicleDriveable, IDrivableVehicle, CanEnterVehicle, bool, (CBaseEntity *pEntity), (pEntity));
DECLARE_HOOK_SUBCLASS(CanEnterVehicle, CE_CPropVehicleDriveable, IDrivableVehicle);

SH_DECL_MANUALHOOK1(CanExitVehicle, 0, 0, 0, bool, CBaseEntity *);
DECLARE_DEFAULTHANDLER_SUBCLASS(CE_CPropVehicleDriveable, IDrivableVehicle, CanExitVehicle, bool, (CBaseEntity *pEntity), (pEntity));
DECLARE_HOOK_SUBCLASS(CanExitVehicle, CE_CPropVehicleDriveable, IDrivableVehicle);

SH_DECL_MANUALHOOK1_void(EnterVehicle, 0, 0, 0, CBaseEntity *);
DECLARE_DEFAULTHANDLER_SUBCLASS_void(CE_CPropVehicleDriveable, IDrivableVehicle, EnterVehicle, (CBaseEntity *pPassenger), (pPassenger));
DECLARE_HOOK_SUBCLASS(EnterVehicle, CE_CPropVehicleDriveable, IDrivableVehicle);

SH_DECL_MANUALHOOK1_void(ExitVehicle, 0, 0, 0, int );
DECLARE_DEFAULTHANDLER_SUBCLASS_void(CE_CPropVehicleDriveable, IDrivableVehicle, ExitVehicle, (int nRole), (nRole));
DECLARE_HOOK_SUBCLASS(ExitVehicle, CE_CPropVehicleDriveable, IDrivableVehicle);

SH_DECL_MANUALHOOK1(PassengerShouldReceiveDamage, 0, 0, 0, bool, CTakeDamageInfo & );
DECLARE_DEFAULTHANDLER_SUBCLASS(CE_CPropVehicleDriveable, IDrivableVehicle, PassengerShouldReceiveDamage, bool,(CTakeDamageInfo &info), (info));
DECLARE_HOOK_SUBCLASS(PassengerShouldReceiveDamage, CE_CPropVehicleDriveable, IDrivableVehicle);

//SH_DECL_MANUALHOOK0(GetDriver, 0, 0, 0, CBaseEntity * );
//DECLARE_DEFAULTHANDLER_SUBCLASS(CE_CPropVehicleDriveable, IDrivableVehicle, GetDriver, CBaseEntity *, (), ());
//DECLARE_HOOK_SUBCLASS(GetDriver, CE_CPropVehicleDriveable, IDrivableVehicle);

SH_DECL_MANUALHOOK0(GetDriver, 0, 0, 0, CBaseEntity*);
DECLARE_HOOK(GetDriver, CE_CPropVehicleDriveable);
DECLARE_DEFAULTHANDLER(CE_CPropVehicleDriveable, GetDriver, CBaseEntity*, (), ());







SH_DECL_MANUALHOOK4_void(DriveVehicle, 0, 0, 0, float , CUserCmd *, int, int );
DECLARE_HOOK(DriveVehicle, CE_CPropVehicleDriveable);
DECLARE_DEFAULTHANDLER_void(CE_CPropVehicleDriveable, DriveVehicle, (float flFrameTime, CUserCmd *ucmd, int iButtonsDown, int iButtonsReleased), (flFrameTime, ucmd, iButtonsDown, iButtonsReleased));

SH_DECL_MANUALHOOK2_void(DampenEyePosition, 0, 0, 0, Vector &, QAngle &);
DECLARE_HOOK(DampenEyePosition, CE_CPropVehicleDriveable);
DECLARE_DEFAULTHANDLER_void(CE_CPropVehicleDriveable, DampenEyePosition, (Vector &vecVehicleEyePos, QAngle &vecVehicleEyeAngles), (vecVehicleEyePos, vecVehicleEyeAngles));

SH_DECL_MANUALHOOK0(IsVehicleBodyInWater, 0, 0, 0, bool);
DECLARE_HOOK(IsVehicleBodyInWater, CE_CPropVehicleDriveable);
DECLARE_DEFAULTHANDLER(CE_CPropVehicleDriveable, IsVehicleBodyInWater, bool, (), ());

SH_DECL_MANUALHOOK0_void(CreateServerVehicle, 0, 0, 0);
DECLARE_HOOK(CreateServerVehicle, CE_CPropVehicleDriveable);
DECLARE_DEFAULTHANDLER_void(CE_CPropVehicleDriveable, CreateServerVehicle, (), ());

SH_DECL_MANUALHOOK0(IsOverturned, 0, 0, 0, bool);
DECLARE_HOOK(IsOverturned, CE_CPropVehicleDriveable);
DECLARE_DEFAULTHANDLER(CE_CPropVehicleDriveable, IsOverturned, bool, (), ());

void CE_CPropVehicle::Spawn( )
{
	/*CFourWheelServerVehicle *pServerVehicle = dynamic_cast<CFourWheelServerVehicle*>(GetServerVehicle());
	m_VehiclePhysics->SetOuter( this, pServerVehicle );

	// NOTE: The model has to be set before we can spawn vehicle physics
	CEntity::Spawn();
	SetCollisionGroup( COLLISION_GROUP_VEHICLE );

	m_VehiclePhysics->Spawn();
	if (!m_VehiclePhysics->Initialize( STRING(m_vehicleScript), m_nVehicleType ))
		return;
	SetNextThink( gpGlobals->curtime );

	m_vecSmoothedVelocity.Init();*/
	BaseClass::Spawn();

	// CEntity: Fix invalid vehicle scripts
	// this could be improved but it's good enough for now (tm)

	IPhysicsVehicleController *vehicle = m_VehiclePhysics->GetVehicle();

	auto *operatingParams = const_cast<vehicle_operatingparams_t*>(&vehicle->GetOperatingParams());
	auto &params = vehicle->GetVehicleParamsForChange();

	if (stricmp(GetClassname(), "prop_vehicle_jeep") == 0 &&
		operatingParams->steeringAngle == 0.0f && params.steering.degreesSlow == 0.0f)
	{
		// overwrite via PhysFindOrAddVehicleScript ?
		META_CONPRINTF("[MONSTER] Invalid vehicle script data in '%s'\n", GetDebugName());

		//operatingParams->steeringAngle = 1.0f;

		params.steering.degreesSlow = 60.0f;
		params.steering.degreesFast = 40.0f;
		params.steering.degreesBoost = 15.0f;
		//params.steering.degreesFast = 18.0f;
		//params.steering.degreesBoost = 11.0f;

		params.steering.speedSlow = 8.0f;
		params.steering.speedFast = 10.0f;
		//params.steering.speedFast = 15.0f;
		///params.steering.turnThrottleReduceSlow = 0.01f;
		///params.steering.turnThrottleReduceFast = 2.0f;
		params.steering.powerSlideAccel = 250.0f;
		params.steering.boostSteeringRateFactor = 1.7f;
		params.steering.boostSteeringRestRateFactor = 1.7f;
		params.steering.steeringExponent = 1.4f;
		params.steering.isSkidAllowed = true;
	}
}

void CE_CPropVehicle::Think()
{
	// nope:
	//	BaseClass::Think();

	GetPhysics()->Think();

	// Derived classes of CPropVehicle have their own code to determine how frequently to think.
	// But the prop_vehicle entity native to this class will only think one time, so this flag
	// was added to allow prop_vehicle to always think without affecting the derived classes.
	if( HasSpawnFlags(SF_PROP_VEHICLE_ALWAYSTHINK) )
	{
		SetNextThink(gpGlobals->curtime);
	}
}

IServerVehicle *CE_CPropVehicleDriveable::GetServerVehicle()
{
	//return g_helpfunc.GetServerVehicle(BaseEntity());
	return m_pServerVehicle;
}

void CE_CPropVehicleDriveable::StartEngine( void )
{
	if ( m_bEngineLocked )
	{
		m_VehiclePhysics->SetHandbrake( true );
		return;
	}

	m_VehiclePhysics->TurnOn();
}

void CE_CPropVehicleDriveable::StopEngine( void )
{
	m_VehiclePhysics->TurnOff();
}

bool CE_CPropVehicleDriveable::IsEngineOn( void )
{
	return m_VehiclePhysics->IsOn();
}



void CE_CPropVehicleDriveable::UpdateOnRemove()
{
	UTIL_Remove(m_hPointViewControl);
	m_hPointViewControl.Set(NULL);

	UTIL_Remove(m_hDriverRagdoll);
	m_hDriverRagdoll.Set(NULL);

	CBaseEntity *driver = GetDriver();
	if(driver)
	{
		CPlayer	*pPlayer = ToBasePlayer(CEntity::Instance(driver));
		if(pPlayer)
		{
			pPlayer->LeaveVehicle();
		}
	}
	BaseClass::UpdateOnRemove();
}

void CE_CPropVehicleDriveable::RemoveDriverRagdoll()
{
	UTIL_Remove(m_hDriverRagdoll);
	m_hDriverRagdoll.Set(NULL);

}

void CE_CPropVehicleDriveable::CreateDriverRagdoll(CPlayer *pPlayer)
{
	if(!pPlayer)
		return;

	if(m_hDriverRagdoll)
	{
		Assert(0);
		UTIL_Remove(m_hDriverRagdoll);
		m_hDriverRagdoll.Set(NULL);
	}

	CEntity *body = CreateEntityByName("prop_physics_override");
	if(!body)
		return;

	pPlayer->SetRenderMode(kRenderTransTexture);
	pPlayer->SetRenderColor(0,0,0,0);

	m_hDriverRagdoll.Set(body->BaseEntity());
	body->DispatchKeyValue("model", STRING(pPlayer->GetModelName()));
	body->DispatchKeyValue("skin", "0");

	DispatchSpawn(body->BaseEntity());
	body->Activate();

	body->AddEffects(1|128|512);
	body->SetParent(BaseEntity());
	body->SetParentAttachment("SetParentAttachment","vehicle_driver_eyes", false);

}

void CE_CPropVehicleDriveable::RemovePointViewControl(CPlayer *pPlayer)
{
	if(m_hPointViewControl)
	{
		m_hPointViewControl->SetParent(NULL);
		UTIL_Remove(m_hPointViewControl);
		m_hPointViewControl.Set(NULL);
	}
	m_bInDriverView = false;
}

void CE_CPropVehicleDriveable::PointViewToggle(CPlayer *pPlayer)
{
	if(m_bInDriverView)
	{
		Vector vecAttachPoint,forward;
		QAngle vecAttachAngles;
		if(GetAttachmentLocal(LookupAttachment( "vehicle_3rd" ), vecAttachPoint, vecAttachAngles))
		{
			pPlayer->SetLocalOrigin(vecAttachPoint);
			pPlayer->SetLocalAngles(vecAttachAngles);
			m_bInDriverView = false;
		}
		return;
	}

	if(!m_bInDriverView)
	{
		Vector vecAttachPoint,forward;
		QAngle vecAttachAngles;
		GetAttachmentLocal(LookupAttachment( "vehicle_driver_eyes" ), vecAttachPoint, vecAttachAngles);
		pPlayer->SetLocalOrigin(vecAttachPoint);
		pPlayer->SetLocalAngles(vecAttachAngles);
		m_bInDriverView = true;
		return;
	}
}

void CE_CPropVehicleDriveable::SetPointViewControl(CPlayer *pPlayer)
{
	if(m_hPointViewControl)
	{
		UTIL_Remove(m_hPointViewControl);
		m_hPointViewControl.Set(NULL);
	}

	if(!pPlayer)
		return;

	Vector vec = pPlayer->GetAbsOrigin();
	QAngle angle = pPlayer->GetAbsAngles();

	Vector vecAttachPoint;
	QAngle vecAttachAngles;
	GetAttachmentLocal(LookupAttachment( "vehicle_driver_eyes" ), vecAttachPoint, vecAttachAngles);

	CE_CSprite *pointview = CE_CSprite::SpriteCreate("materials/sprites/dot.vmt", vec, false);
	m_hPointViewControl.Set(pointview->BaseEntity());

	pointview->SetRenderColor(0, 0, 0, 0);
	pointview->SetAbsOrigin(vec);
	pointview->SetAbsAngles(angle);
	DispatchSpawn(pointview->BaseEntity());

	engine->SetView(pPlayer->edict(), pointview->edict());

	pointview->SetParent(pPlayer->BaseEntity());

	//vecAttachPoint.z = 0.0f;
	pPlayer->SetLocalOrigin(vecAttachPoint);
	pPlayer->SetLocalAngles(vecAttachAngles);
	m_bInDriverView = true;
}

class ITimedEvent
{
public:
	/**
	 * @brief Called when a timer is executed.
	 *
	 * @param pTimer		Pointer to the timer instance.
	 * @param pData			Private pointer passed from host.
	 * @return				Pl_Stop to stop timer, Pl_Continue to continue.
	 */
	virtual ResultType OnTimer(ITimer *pTimer, void *pData) =0;

	/**
	 * @brief Called when the timer has been killed.
	 *
	 * @param pTimer		Pointer to the timer instance.
	 * @param pData			Private data pointer passed from host.
	 */
	virtual void OnTimerEnd(ITimer *pTimer, void *pData) =0;
};

class CPropVehicleDriveable_Fix_Timer: public SourceMod::ITimedEvent
{
public:
	ResultType OnTimer(ITimer *pTimer, void *pData) override
	{
		CBaseHandle myRef((unsigned long)pData);
		CEntity *cent = CEntityLookup::Instance(myRef);
		if (cent)
		{
			auto *vehicle = (CE_CPropVehicleDriveable_Fix*)cent;
			vehicle->m_hPlayer.ptr->Set(nullptr);
			vehicle->m_bInVehicleCrashHotfix = false;
		} else {
			g_pSM->LogError(myself, "CPropVehicleDriveable_Fix_Timer: Couldn't resolve my handle!");
		}
		return Pl_Stop;
	}
	void OnTimerEnd(ITimer *pTimer, void *pData) override
	{
	}
	CEFakeHandle<CE_CPropVehicleDriveable_Fix> handle;
} sFixTimer;

bool CE_CPropVehicleDriveable_Fix::CanEnterVehicle( CBaseEntity *pEntity )
{
	// while our hotfix is running, prevent anyone from entering the vehicle.
	if (m_bInVehicleCrashHotfix)
		return false;
	return BaseClass::CanEnterVehicle(pEntity);
}

bool CE_CPropVehicleDriveable_Fix::CanExitVehicle( CBaseEntity *pEntity )
{
	if (gpGlobals->curtime - m_flLastVehicleEnterTime < 1.1f)
		return false;
	return BaseClass::CanExitVehicle(pEntity);
}

void CE_CPropVehicleDriveable_Fix::EnterVehicle( CBaseEntity *pPassenger )
{
	m_flLastVehicleEnterTime = gpGlobals->curtime;

	BaseClass::EnterVehicle(pPassenger);
}

void CE_CPropVehicleDriveable_Fix::ExitVehicle( int nRole )
{
	CPlayer *cent_m_hPlayer = (CPlayer *)(m_hPlayer.ptr->Get());

	BaseClass::ExitVehicle(nRole);

	// the fix is to temporarily keep the player until the client finally gets that the player is no longer in the car!
	m_bInVehicleCrashHotfix = true;
	m_hPlayer.ptr->Set(cent_m_hPlayer->GetIHandle());

	int myRef = GetIHandle()->GetRefEHandle().ToInt();
	timersys->CreateTimer(&sFixTimer, 1.0f, (void*) myRef, TIMER_FLAG_NO_MAPCHANGE);
}

void CE_CPropVehicleDriveable_Fix::DriveVehicle( float flFrameTime, CUserCmd *ucmd, int iButtonsDown, int iButtonsReleased )
{
	int iButtons = ucmd->buttons;

	m_VehiclePhysics->UpdateDriverControls( ucmd, flFrameTime );

	m_nSpeed = m_VehiclePhysics->GetSpeed();	//send speed to client
	m_nRPM = clamp( m_VehiclePhysics->GetRPM(), 0, 4095 );
	m_nBoostTimeLeft = m_VehiclePhysics->BoostTimeLeft();
	m_nHasBoost = m_VehiclePhysics->HasBoost();
	m_flThrottle = m_VehiclePhysics->GetThrottle();

	m_nScannerDisabledWeapons = false;		// off for now, change once we have scanners
	m_nScannerDisabledVehicle = false;		// off for now, change once we have scanners

	//
	// Fire the appropriate outputs based on button pressed events.
	//
	// BUGBUG: m_afButtonPressed is broken - check the player.cpp code!!!
	float attack = 0, attack2 = 0;

	if ( iButtonsDown & IN_ATTACK )
	{
		m_pressedAttack->FireOutput( this, this, 0 );
	}
	if ( iButtonsDown & IN_ATTACK2 )
	{
		m_pressedAttack2->FireOutput( this, this, 0 );
	}

	if ( iButtons & IN_ATTACK )
	{
		attack = 1;
	}
	if ( iButtons & IN_ATTACK2 )
	{
		attack2 = 1;
	}

	//m_attackaxis->Set( attack, this, this );
	//m_attack2axis->Set( attack2, this, this );
}