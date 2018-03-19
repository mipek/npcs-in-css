//========= Copyright ?1996-2005, Valve Corporation, All rights reserved. ====
//
// Purpose: 
//
//=============================================================================

#include "func_tank.h"
#include "CPlayer.h"
#include "CAI_NPC.h"
#include "ammodef.h"
#include "CDynamicProp.h"
#include "CCombatWeapon.h"
#include "CAI_utils.h"
#include "effect_dispatch_data.h"
#include "CSprite.h"
#include "rumble_shared.h"
#include "CE_recipientfilter.h"
#include "grenade_beam.h"
#include "CEnvLaser.h"


extern Vector PointOnLineNearestPoint(const Vector& vStartPos, const Vector& vEndPos, const Vector& vPoint);


BEGIN_DATADESC( CFuncTank )
	DEFINE_KEYFIELD( m_yawRate, FIELD_FLOAT, "yawrate" ),
	DEFINE_KEYFIELD( m_yawRange, FIELD_FLOAT, "yawrange" ),
	DEFINE_KEYFIELD( m_yawTolerance, FIELD_FLOAT, "yawtolerance" ),
	DEFINE_KEYFIELD( m_pitchRate, FIELD_FLOAT, "pitchrate" ),
	DEFINE_KEYFIELD( m_pitchRange, FIELD_FLOAT, "pitchrange" ),
	DEFINE_KEYFIELD( m_pitchTolerance, FIELD_FLOAT, "pitchtolerance" ),
	DEFINE_KEYFIELD( m_fireRate, FIELD_FLOAT, "firerate" ),
	DEFINE_FIELD( m_fireTime, FIELD_TIME ),
	DEFINE_KEYFIELD( m_persist, FIELD_FLOAT, "persistence" ),
	DEFINE_KEYFIELD( m_persist2, FIELD_FLOAT, "persistence2" ),
	DEFINE_KEYFIELD( m_minRange, FIELD_FLOAT, "minRange" ),
	DEFINE_KEYFIELD( m_maxRange, FIELD_FLOAT, "maxRange" ),
	DEFINE_FIELD( m_flMinRange2, FIELD_FLOAT ),
	DEFINE_FIELD( m_flMaxRange2, FIELD_FLOAT ),
	DEFINE_KEYFIELD( m_iAmmoCount, FIELD_INTEGER, "ammo_count" ),
	DEFINE_KEYFIELD( m_spriteScale, FIELD_FLOAT, "spritescale" ),
	DEFINE_KEYFIELD( m_iszSpriteSmoke, FIELD_STRING, "spritesmoke" ),
	DEFINE_KEYFIELD( m_iszSpriteFlash, FIELD_STRING, "spriteflash" ),
	DEFINE_KEYFIELD( m_bulletType, FIELD_INTEGER, "bullet" ),
	DEFINE_FIELD( m_nBulletCount, FIELD_INTEGER ),
	DEFINE_KEYFIELD( m_spread, FIELD_INTEGER, "firespread" ),
	DEFINE_KEYFIELD( m_iBulletDamage, FIELD_INTEGER, "bullet_damage" ),
	DEFINE_KEYFIELD( m_iBulletDamageVsPlayer, FIELD_INTEGER, "bullet_damage_vs_player" ),
	DEFINE_KEYFIELD( m_iszMaster, FIELD_STRING, "master" ),
	
#ifdef HL2_EPISODIC	
	DEFINE_KEYFIELD( m_iszAmmoType, FIELD_STRING, "ammotype" ),
	DEFINE_FIELD( m_iAmmoType, FIELD_INTEGER ),
#else
	DEFINE_FIELD( m_iSmallAmmoType, FIELD_INTEGER ),
	DEFINE_FIELD( m_iMediumAmmoType, FIELD_INTEGER ),
	DEFINE_FIELD( m_iLargeAmmoType, FIELD_INTEGER ),
#endif // HL2_EPISODIC

	DEFINE_KEYFIELD( m_soundStartRotate, FIELD_SOUNDNAME, "rotatestartsound" ),
	DEFINE_KEYFIELD( m_soundStopRotate, FIELD_SOUNDNAME, "rotatestopsound" ),
	DEFINE_KEYFIELD( m_soundLoopRotate, FIELD_SOUNDNAME, "rotatesound" ),
	DEFINE_KEYFIELD( m_flPlayerGracePeriod, FIELD_FLOAT, "playergraceperiod" ),
	DEFINE_KEYFIELD( m_flIgnoreGraceUpto, FIELD_FLOAT, "ignoregraceupto" ),
	DEFINE_KEYFIELD( m_flPlayerLockTimeBeforeFire, FIELD_FLOAT, "playerlocktimebeforefire" ),
	DEFINE_FIELD( m_flLastSawNonPlayer, FIELD_TIME ),

	DEFINE_FIELD( m_yawCenter, FIELD_FLOAT ),
	DEFINE_FIELD( m_yawCenterWorld, FIELD_FLOAT ),
	DEFINE_FIELD( m_pitchCenter, FIELD_FLOAT ),
	DEFINE_FIELD( m_pitchCenterWorld, FIELD_FLOAT ),
	DEFINE_FIELD( m_fireLast, FIELD_TIME ),
	DEFINE_FIELD( m_lastSightTime, FIELD_TIME ),
	DEFINE_FIELD( m_barrelPos, FIELD_VECTOR ),
	DEFINE_FIELD( m_sightOrigin, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_hFuncTankTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( m_hController, FIELD_EHANDLE ),
	DEFINE_FIELD( m_vecControllerUsePos, FIELD_VECTOR ),
	DEFINE_FIELD( m_flNextAttack, FIELD_TIME ),
	DEFINE_FIELD( m_targetEntityName, FIELD_STRING ),
	DEFINE_FIELD( m_hTarget, FIELD_EHANDLE ),
	DEFINE_FIELD( m_vTargetPosition, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vecNPCIdleTarget, FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_persist2burst, FIELD_FLOAT),
	//DEFINE_FIELD( m_parentMatrix, FIELD_MATRIX ), // DON'T SAVE
	DEFINE_FIELD( m_hControlVolume, FIELD_EHANDLE ),
	DEFINE_KEYFIELD( m_iszControlVolume, FIELD_STRING, "control_volume" ),
	DEFINE_FIELD( m_flNextControllerSearch, FIELD_TIME ),
	DEFINE_FIELD( m_bShouldFindNPCs, FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bNPCInRoute, FIELD_BOOLEAN ),
	DEFINE_KEYFIELD( m_iszNPCManPoint, FIELD_STRING, "npc_man_point" ),
	DEFINE_FIELD( m_bReadyToFire, FIELD_BOOLEAN ),

	DEFINE_KEYFIELD( m_bPerformLeading, FIELD_BOOLEAN, "LeadTarget" ),
	DEFINE_FIELD( m_flStartLeadFactor, FIELD_FLOAT ),
	DEFINE_FIELD( m_flStartLeadFactorTime, FIELD_TIME ),
	DEFINE_FIELD( m_flNextLeadFactor, FIELD_FLOAT ),
	DEFINE_FIELD( m_flNextLeadFactorTime, FIELD_TIME ),

	// Used for when the gun is attached to another entity
	DEFINE_KEYFIELD( m_iszBaseAttachment, FIELD_STRING, "gun_base_attach" ),
	DEFINE_KEYFIELD( m_iszBarrelAttachment, FIELD_STRING, "gun_barrel_attach" ),
//	DEFINE_FIELD( m_nBarrelAttachment, FIELD_INTEGER ),

	// Used when the gun is actually a part of the parent entity, and pose params aim it
	DEFINE_KEYFIELD( m_iszYawPoseParam, FIELD_STRING, "gun_yaw_pose_param" ),
	DEFINE_KEYFIELD( m_iszPitchPoseParam, FIELD_STRING, "gun_pitch_pose_param" ),
	DEFINE_KEYFIELD( m_flYawPoseCenter, FIELD_FLOAT, "gun_yaw_pose_center" ),
	DEFINE_KEYFIELD( m_flPitchPoseCenter, FIELD_FLOAT, "gun_pitch_pose_center" ),
	DEFINE_FIELD( m_bUsePoseParameters, FIELD_BOOLEAN ),

	DEFINE_KEYFIELD( m_iEffectHandling, FIELD_INTEGER, "effecthandling" ),

	// Inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "Activate", InputActivate ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Deactivate", InputDeactivate ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetFireRate", InputSetFireRate ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetDamage", InputSetDamage ),
	DEFINE_INPUTFUNC( FIELD_VECTOR, "SetTargetPosition", InputSetTargetPosition ),
	DEFINE_INPUTFUNC( FIELD_VECTOR, "SetTargetDir", InputSetTargetDir ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetTargetEntityName", InputSetTargetEntityName ),
	DEFINE_INPUTFUNC( FIELD_EHANDLE, "SetTargetEntity", InputSetTargetEntity ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ClearTargetEntity", InputClearTargetEntity ),
	DEFINE_INPUTFUNC( FIELD_STRING, "FindNPCToManTank", InputFindNPCToManTank ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StopFindingNPCs", InputStopFindingNPCs ),
	DEFINE_INPUTFUNC( FIELD_VOID, "StartFindingNPCs", InputStartFindingNPCs ),
	DEFINE_INPUTFUNC( FIELD_VOID, "ForceNPCOff", InputForceNPCOff ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetMaxRange", InputSetMaxRange ),

	// Outputs
	DEFINE_OUTPUT(m_OnFire,					"OnFire"),
	DEFINE_OUTPUT(m_OnLoseTarget,			"OnLoseTarget"),
	DEFINE_OUTPUT(m_OnAquireTarget,			"OnAquireTarget"),
	DEFINE_OUTPUT(m_OnAmmoDepleted,			"OnAmmoDepleted"),
	DEFINE_OUTPUT(m_OnGotController,		"OnGotController"),
	DEFINE_OUTPUT(m_OnLostController,		"OnLostController"),
	DEFINE_OUTPUT(m_OnGotPlayerController,	"OnGotPlayerController"),
	DEFINE_OUTPUT(m_OnLostPlayerController,	"OnLostPlayerController"),
	DEFINE_OUTPUT(m_OnReadyToFire,			"OnReadyToFire"),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CFuncTank::CFuncTank()
{
	m_nBulletCount = 0;
	m_fireTime = 0.0f;
	m_flMinRange2 = 0.0f;
	m_flMaxRange2 = 0.0f;

	m_fireLast = 0.0f;
	m_flNextAttack = 0.0f;
	m_vecControllerUsePos.Init();
	m_lastSightTime = 0.0f;
	m_persist2burst = 0.0f;

	m_bNPCInRoute = false;
	m_flNextControllerSearch = 0;
	m_bShouldFindNPCs = true;
	m_bUsePoseParameters = false;
	m_bReadyToFire = false;

	m_iszSpriteSmoke = NULL_STRING;
	m_iszSpriteFlash = NULL_STRING;
	m_soundStartRotate = NULL_STRING;
	m_soundStopRotate = NULL_STRING;
	m_soundLoopRotate = NULL_STRING;
	m_iszMaster = NULL_STRING;
	m_targetEntityName = NULL_STRING;
	m_iszBarrelAttachment = NULL_STRING;
	m_iszBaseAttachment = NULL_STRING;
	m_iszYawPoseParam = NULL_STRING;
	m_iszPitchPoseParam = NULL_STRING;
	m_iszNPCManPoint = NULL_STRING;
	m_iszControlVolume = NULL_STRING;

	m_nBarrelAttachment = 0;


	m_vecNPCIdleTarget.Init();
	m_vTargetPosition.Init();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CFuncTank::~CFuncTank( void )
{
	if ( m_soundLoopRotate != NULL_STRING && ( m_spawnflags & SF_TANK_SOUNDON ) )
	{
		StopSound( entindex(), CHAN_STATIC, STRING(m_soundLoopRotate) );
	}
}


//------------------------------------------------------------------------------
// Purpose:
//------------------------------------------------------------------------------
inline bool CFuncTank::CanFire( void )
{ 
	float flTimeDelay = gpGlobals->curtime - m_lastSightTime;

	// Fire when can't see enemy if time is less that persistence time
	if ( flTimeDelay <= m_persist )
		return true;

	// Fire when I'm in a persistence2 burst
	if ( flTimeDelay <= m_persist2burst )
		return true;

	// If less than persistence2, occasionally do another burst
	if ( flTimeDelay <= m_persist2 )
	{
		if ( enginerandom->RandomInt( 0, 30 ) == 0 )
		{
			m_persist2burst = flTimeDelay + 0.5f;
			return true;
		}
	}

	return false;
}

//------------------------------------------------------------------------------
// Purpose: Input handler for activating the tank.
//------------------------------------------------------------------------------
void CFuncTank::InputActivate( inputdata_t &inputdata )
{	
	TankActivate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncTank::TankActivate( void )
{
	m_spawnflags |= SF_TANK_ACTIVE; 
	SetNextThink( gpGlobals->curtime + 0.1f ); 
	m_fireLast = gpGlobals->curtime;
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for deactivating the tank.
//-----------------------------------------------------------------------------
void CFuncTank::InputDeactivate( inputdata_t &inputdata )
{
	TankDeactivate();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncTank::TankDeactivate( void )
{
	m_spawnflags &= ~SF_TANK_ACTIVE; 
	m_fireLast = 0; 
	StopRotSound();
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for changing the name of the tank's target entity.
//-----------------------------------------------------------------------------
void CFuncTank::InputSetTargetEntityName( inputdata_t &inputdata )
{
	m_targetEntityName = inputdata.value.StringID();
	CEntity *target = FindTarget( m_targetEntityName, CEntity::Instance(inputdata.pActivator));
	m_hTarget.Set((target)?target->BaseEntity():NULL);

	// No longer aim at target position if have one
	m_spawnflags &= ~SF_TANK_AIM_AT_POS; 
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for setting a new target entity by ehandle.
//-----------------------------------------------------------------------------
void CFuncTank::InputSetTargetEntity( inputdata_t &inputdata )
{
	CEntity *pEntity = CEntity::Instance(inputdata.value.Entity());
	if ( pEntity != NULL )
	{
		m_targetEntityName = MAKE_STRING(pEntity->GetEntityName());
	}
	else
	{
		m_targetEntityName = NULL_STRING;
	}
	m_hTarget.Set(inputdata.value.Entity());

	// No longer aim at target position if have one
	m_spawnflags &= ~SF_TANK_AIM_AT_POS; 
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for clearing the tank's target entity
//-----------------------------------------------------------------------------
void CFuncTank::InputClearTargetEntity( inputdata_t &inputdata )
{
	m_targetEntityName = NULL_STRING;
	m_hTarget.Set(NULL);

	// No longer aim at target position if have one
	m_spawnflags &= ~SF_TANK_AIM_AT_POS; 
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for setting the rate of fire in shots per second.
//-----------------------------------------------------------------------------
void CFuncTank::InputSetFireRate( inputdata_t &inputdata )
{
	m_fireRate = inputdata.value.Float();
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for setting the damage
//-----------------------------------------------------------------------------
void CFuncTank::InputSetDamage( inputdata_t &inputdata )
{
	m_iBulletDamage = inputdata.value.Int();
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for setting the target as a position.
//-----------------------------------------------------------------------------
void CFuncTank::InputSetTargetPosition( inputdata_t &inputdata )
{
	m_spawnflags |= SF_TANK_AIM_AT_POS; 
	m_hTarget.Set(NULL);

	inputdata.value.Vector3D( m_vTargetPosition );
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for setting the target as a position.
//-----------------------------------------------------------------------------
void CFuncTank::InputSetTargetDir( inputdata_t &inputdata )
{
	m_spawnflags |= SF_TANK_AIM_AT_POS; 
	m_hTarget.Set(NULL);

	Vector vecTargetDir;
	inputdata.value.Vector3D( vecTargetDir );
	m_vTargetPosition = GetAbsOrigin() + m_barrelPos.LengthSqr() * vecTargetDir;
}

//-----------------------------------------------------------------------------
// Purpose: Input handler for telling the func_tank to find an NPC to man it.
//-----------------------------------------------------------------------------
void CFuncTank::InputFindNPCToManTank( inputdata_t &inputdata )
{
	// Verify the func_tank is controllable and available.
	if ( !IsNPCControllable() && !IsNPCSetController() )
		return;

	// If we have a controller already - don't look for one.
	if ( HasController() )
		return;

	// NPC assigned to man the func_tank?
	CEntity *pEntity = g_helpfunc.FindEntityByName((CBaseEntity *) NULL, inputdata.value.StringID() );
	if ( pEntity )
	{
		CAI_NPC *pNPC = pEntity->MyNPCPointer();
		if ( pNPC )
		{
			// Verify the npc has the func_tank controller behavior.
			//CE_TODO
			/*CAI_FuncTankBehavior *pBehavior;
			if ( pNPC->GetBehavior( &pBehavior ) )
			{
				m_hController = pNPC;
				pBehavior->SetFuncTank( this );
				NPC_SetInRoute( true );
				return;
			}*/
		}
	}

	// No controller? Find a nearby NPC who can man this func_tank.
	NPC_FindController();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CFuncTank::InputStopFindingNPCs( inputdata_t &inputdata )
{
	m_bShouldFindNPCs = false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CFuncTank::InputStartFindingNPCs( inputdata_t &inputdata )
{
	m_bShouldFindNPCs = true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CFuncTank::InputForceNPCOff( inputdata_t &inputdata )
{
	// Interrupt any npc in route (ally or not).
	if ( NPC_InRoute() )
	{
		// Interrupt the npc's route.
		NPC_InterruptRoute();
	}

	// If we don't have a controller - then the gun should be free.
	if ( !m_hController )
		return;

	CAI_NPC *pNPC = m_hController->MyNPCPointer();
	if ( !pNPC )
		return;

	//CE_TODO
	/*CAI_FuncTankBehavior *pBehavior;
	if ( pNPC->GetBehavior( &pBehavior ) )
	{
		pBehavior->Dismount();
	}*/

	m_hController.Set(NULL);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CFuncTank::InputSetMaxRange( inputdata_t &inputdata )
{
	m_maxRange = inputdata.value.Float();
	m_flMaxRange2 = m_maxRange * m_maxRange;
}

//-----------------------------------------------------------------------------
// Purpose: Find the closest NPC with the func_tank behavior.
//-----------------------------------------------------------------------------
void CFuncTank::NPC_FindController( void )
{
	// Not NPC controllable or controllable on by specified npc's return.
	if ( !IsNPCControllable() || IsNPCSetController() )
		return;

	// Initialize for finding closest NPC.
	CAI_NPC *pClosestNPC = NULL;
	float flClosestDist2 = ( FUNCTANK_DISTANCE_MAX * FUNCTANK_DISTANCE_MAX );
	float flMinDistToEnemy2 = ( FUNCTANK_DISTANCE_MIN_TO_ENEMY * FUNCTANK_DISTANCE_MIN_TO_ENEMY );
	//CE_TODO
	//CAI_FuncTankBehavior *pClosestBehavior = NULL;

	// Get the mount position.
	Vector vecMountPos;
	NPC_FindManPoint( vecMountPos );

	// Search through the AI list for the closest NPC with the func_tank behavior.
	CAI_NPC **ppAIs = g_AI_Manager.AccessAIs();
	int nAICount = g_AI_Manager.NumAIs();
	for ( int iAI = 0; iAI < nAICount; ++iAI )
	{
		CAI_NPC *pNPC = ppAIs[iAI];
		if ( !pNPC )
			continue;
		
		if ( !pNPC->IsAlive() )
			continue;

		if ( pNPC->IsInAScript() )
			continue;

		//CE_TODO
		/*CAI_FuncTankBehavior *pBehavior;
		if ( pNPC->GetBehavior( &pBehavior ) )
		{
			// Don't mount the func_tank if your "enemy" is within X feet or it or the npc.
			CBaseEntity *pEnemy = pNPC->GetEnemy();

			if ( pEnemy )
			{
				if ( !IsEntityInViewCone(pEnemy) )
				{
					// Don't mount the tank if the tank can't be aimed at the enemy.
					continue;
				}

				float flDist2 = ( pEnemy->GetAbsOrigin() - pNPC->GetAbsOrigin() ).LengthSqr();
				if ( flDist2 < flMinDistToEnemy2 )
					continue;

				flDist2 = ( vecMountPos - pEnemy->GetAbsOrigin() ).LengthSqr();
				if ( flDist2 < flMinDistToEnemy2 )
					continue;

				if ( !pNPC->FVisible( vecMountPos + pNPC->GetViewOffset() ) )
					continue;
			}

			trace_t tr;
			UTIL_TraceEntity( pNPC, vecMountPos, vecMountPos, MASK_NPCSOLID, this, pNPC->GetCollisionGroup(), &tr );
			if( tr.startsolid || tr.fraction < 1.0 )
			{
				// Don't mount the tank if someone/something is located on the control point.
				continue;
			}

			if ( !pBehavior->HasFuncTank() && !pBehavior->IsBusy() )
			{
				float flDist2 = ( vecMountPos - pNPC->GetAbsOrigin() ).LengthSqr();
				if ( flDist2 < flClosestDist2 )
				{
					pClosestNPC = pNPC;
					pClosestBehavior = pBehavior;
					flClosestDist2 = flDist2;
				}
			}
		}*/
	}

	// Set the closest NPC as controller.
	if ( pClosestNPC )
	{
		m_hController.Set(pClosestNPC->BaseEntity());
		//CE_TODO
		//pClosestBehavior->SetFuncTank( this );
		NPC_SetInRoute( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : pAttacker - 
//			flDamage - 
//			vecDir - 
//			ptr - 
//			bitsDamageType - 
//-----------------------------------------------------------------------------
void CFuncTank::TraceAttack( CBaseEntity *pAttacker, float flDamage, const Vector &vecDir, trace_t *ptr, int bitsDamageType)
{
	if (m_spawnflags & SF_TANK_DAMAGE_KICK)
	{
		// Deflect the func_tank
		// Only adjust yaw for now
		CEntity *cent_pAttacker = CEntity::Instance(pAttacker);
		if (cent_pAttacker)
		{
			Vector vFromAttacker = (cent_pAttacker->EyePosition()-GetAbsOrigin());
			vFromAttacker.z = 0;
			VectorNormalize(vFromAttacker);

			Vector vFromAttacker2 = (ptr->endpos-GetAbsOrigin());
			vFromAttacker2.z = 0;
			VectorNormalize(vFromAttacker2);


			Vector vCrossProduct;
			CrossProduct(vFromAttacker,vFromAttacker2, vCrossProduct);

			QAngle angles;
			angles = GetLocalAngles();
			if (vCrossProduct.z > 0)
			{
				angles.y		+= 10;
			}
			else
			{
				angles.y		-= 10;
			}

			// Limit against range in y
			if ( angles.y > m_yawCenter + m_yawRange )
			{
				angles.y = m_yawCenter + m_yawRange;
			}
			else if ( angles.y < (m_yawCenter - m_yawRange) )
			{
				angles.y = (m_yawCenter - m_yawRange);
			}

			SetLocalAngles( angles );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : targetName - 
//			pActivator - 
//-----------------------------------------------------------------------------
CEntity *CFuncTank::FindTarget( string_t targetName, CEntity *pActivator ) 
{
	return g_helpfunc.FindEntityGenericNearest( STRING( targetName ), GetAbsOrigin(), 0, BaseEntity(), (pActivator)?pActivator->BaseEntity():NULL );
}


//-----------------------------------------------------------------------------
// Purpose: Caches entity key values until spawn is called.
// Input  : szKeyName - 
//			szValue - 
// Output : 
//-----------------------------------------------------------------------------
bool CFuncTank::DispatchKeyValue( const char *szKeyName, const char *szValue )
{
	if (FStrEq(szKeyName, "barrel"))
	{
		m_barrelPos.x = atof(szValue);
		return true;
	}
	
	if (FStrEq(szKeyName, "barrely"))
	{
		m_barrelPos.y = atof(szValue);
		return true;
	}
	
	if (FStrEq(szKeyName, "barrelz"))
	{
		m_barrelPos.z = atof(szValue);
		return true;
	}

	return BaseClass::DispatchKeyValue( szKeyName, szValue );
}


static Vector gTankSpread[] =
{
	Vector( 0, 0, 0 ),		// perfect
	Vector( 0.025, 0.025, 0.025 ),	// small cone
	Vector( 0.05, 0.05, 0.05 ),  // medium cone
	Vector( 0.1, 0.1, 0.1 ),	// large cone
	Vector( 0.25, 0.25, 0.25 ),	// extra-large cone
};
#define MAX_FIRING_SPREADS ARRAYSIZE(gTankSpread)


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncTank::Spawn( void )
{
	Precache();

#ifdef HL2_EPISODIC
	m_iAmmoType = GetAmmoDef()->Index( STRING( m_iszAmmoType ) );
#else
	m_iSmallAmmoType	= GetAmmoDef()->Index("BULLET_PLAYER_45ACP");
	m_iMediumAmmoType	= GetAmmoDef()->Index("BULLET_PLAYER_9MM");
	m_iLargeAmmoType	= GetAmmoDef()->Index("BULLET_PLAYER_762MM");
#endif // HL2_EPISODIC

	SetMoveType( MOVETYPE_PUSH );  // so it doesn't get pushed by anything
	SetSolid( SOLID_VPHYSICS );
	SetModel( STRING(GetModelName()));
	AddEFlags( EFL_USE_PARTITION_WHEN_NOT_SOLID );

	if ( HasSpawnFlags(SF_TANK_NOTSOLID) )
	{
		AddSolidFlags( FSOLID_NOT_SOLID );
	}

	m_hControlVolume.Set(NULL);

	if ( GetParent() && GetParent()->GetBaseAnimating() )
	{
		CAnimating *pAnim = GetParent()->GetBaseAnimating();
		if ( m_iszBaseAttachment != NULL_STRING )
		{
			int nAttachment = pAnim->LookupAttachment( STRING( m_iszBaseAttachment ) );
			if ( nAttachment != 0 )
			{
				SetParent( pAnim->BaseEntity(), nAttachment );
				SetLocalOrigin( vec3_origin );
				SetLocalAngles( vec3_angle );
			}
		}

		m_bUsePoseParameters = (m_iszYawPoseParam != NULL_STRING) && (m_iszPitchPoseParam != NULL_STRING);

		if ( m_iszBarrelAttachment != NULL_STRING )
		{
			if ( m_bUsePoseParameters )
			{
				pAnim->SetPoseParameter( STRING( m_iszYawPoseParam ), 0 );
				pAnim->SetPoseParameter( STRING( m_iszPitchPoseParam ), 0 );
				pAnim->InvalidateBoneCache();
			}

			m_nBarrelAttachment = pAnim->LookupAttachment( STRING(m_iszBarrelAttachment) );

			Vector vecWorldBarrelPos;
			QAngle worldBarrelAngle;
			pAnim->GetAttachment( m_nBarrelAttachment, vecWorldBarrelPos, worldBarrelAngle );
			VectorITransform( vecWorldBarrelPos, EntityToWorldTransform( ), m_barrelPos );
		}

		if ( m_bUsePoseParameters )
		{
			// In this case, we're relying on the parent to have the gun model
			AddEffects( EF_NODRAW );
			QAngle localAngles( m_flPitchPoseCenter, m_flYawPoseCenter, 0 );
			SetLocalAngles( localAngles );
			SetSolid( SOLID_NONE );
			SetMoveType( MOVETYPE_NOCLIP );

			// If our parent is a prop_dynamic, make it use hitboxes for renderbox
			CE_CDynamicProp *pProp = dynamic_cast<CE_CDynamicProp*>(GetParent());
			if ( pProp )
			{
				pProp->m_bUseHitboxesForRenderBox = true;
			}
		}
	}

	// For smoothing out leading
	m_flStartLeadFactor = 1.0f;
	m_flNextLeadFactor = 1.0f;
	m_flStartLeadFactorTime = gpGlobals->curtime;
	m_flNextLeadFactorTime = gpGlobals->curtime + 1.0f;

	m_yawCenter			= GetLocalAngles().y;
	m_yawCenterWorld	= GetAbsAngles().y;
	m_pitchCenter		= GetLocalAngles().x;
	m_pitchCenterWorld	= GetAbsAngles().y;
	m_vTargetPosition	= vec3_origin;

	if ( IsActive() || (IsControllable() && !HasController()) )
	{
		// Think to find controllers.
		SetNextThink( gpGlobals->curtime + 1.0f );
		m_flNextControllerSearch = gpGlobals->curtime + 1.0f;
	}

	UpdateMatrix();

	m_sightOrigin = WorldBarrelPosition(); // Point at the end of the barrel

	if ( m_spread > (int)MAX_FIRING_SPREADS )
	{
		m_spread = 0;
	}

	// No longer aim at target position if have one
	m_spawnflags		&= ~SF_TANK_AIM_AT_POS; 

	if (m_spawnflags & SF_TANK_DAMAGE_KICK)
	{
		m_takedamage = DAMAGE_YES;
	}

	// UNDONE: Do this?
	//m_targetEntityName = m_target;
	if ( GetSolid() != SOLID_NONE )
	{
		CreateVPhysics();
	}

	// Setup squared min/max range.
	m_flMinRange2 = m_minRange * m_minRange;
	m_flMaxRange2 = m_maxRange * m_maxRange;
	m_flIgnoreGraceUpto *= m_flIgnoreGraceUpto;

	m_flLastSawNonPlayer = 0;

	if( IsActive() )
	{
		m_OnReadyToFire.FireOutput( this, this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncTank::Activate( void )
{
	BaseClass::Activate();

	// Necessary for save/load
	if ( (m_iszBarrelAttachment != NULL_STRING) && (m_nBarrelAttachment == 0) )
	{
		if ( GetParent() && GetParent()->GetBaseAnimating() )
		{
			CAnimating *pAnim = GetParent()->GetBaseAnimating();
			m_nBarrelAttachment = pAnim->LookupAttachment( STRING(m_iszBarrelAttachment) );
		}
	}
}

bool CFuncTank::CreateVPhysics()
{
	VPhysicsInitShadow( false, false );
	return true;
}


void CFuncTank::Precache( void )
{
	if ( m_iszSpriteSmoke != NULL_STRING )
		PrecacheModel( STRING(m_iszSpriteSmoke) );
	if ( m_iszSpriteFlash != NULL_STRING )
		PrecacheModel( STRING(m_iszSpriteFlash) );

	if ( m_soundStartRotate != NULL_STRING )
		PrecacheScriptSound( STRING(m_soundStartRotate) );
	if ( m_soundStopRotate != NULL_STRING )
		PrecacheScriptSound( STRING(m_soundStopRotate) );
	if ( m_soundLoopRotate != NULL_STRING )
		PrecacheScriptSound( STRING(m_soundLoopRotate) );

	PrecacheScriptSound( "Func_Tank.BeginUse" );
	
	// Precache the combine cannon
	if ( m_iEffectHandling == EH_COMBINE_CANNON )
	{
		PrecacheScriptSound( "NPC_Combine_Cannon.FireBullet" );
	}
}

void CFuncTank::UpdateOnRemove( void )
{
	if ( HasController() )
	{
		StopControl();
	}
	BaseClass::UpdateOnRemove();
}


//-----------------------------------------------------------------------------
// Barrel position
//-----------------------------------------------------------------------------
void CFuncTank::UpdateMatrix( void )
{
	m_parentMatrix.InitFromEntity( GetParent(), GetParentAttachment() );
}

	
//-----------------------------------------------------------------------------
// Barrel position
//-----------------------------------------------------------------------------
Vector CFuncTank::WorldBarrelPosition( void )
{
	if ( (m_nBarrelAttachment == 0) || !GetParent() )
	{
		EntityMatrix tmp;
		tmp.InitFromEntity( this );
		return tmp.LocalToWorld( m_barrelPos );
	}

	Vector vecOrigin;
	QAngle vecAngles;
	CAnimating *pAnim = GetParent()->GetBaseAnimating();
	pAnim->GetAttachment( m_nBarrelAttachment, vecOrigin, vecAngles );
	return vecOrigin;
}


//-----------------------------------------------------------------------------
// Make the parent's pose parameters match the func_tank 
//-----------------------------------------------------------------------------
void CFuncTank::PhysicsSimulate( void )
{
	BaseClass::PhysicsSimulate();

	if ( m_bUsePoseParameters && GetParent() )
	{
		const QAngle &angles = GetLocalAngles();
		CAnimating *pAnim = GetParent()->GetBaseAnimating();
		pAnim->SetPoseParameter( STRING( m_iszYawPoseParam ), angles.y );
		pAnim->SetPoseParameter( STRING( m_iszPitchPoseParam ), angles.x );
		pAnim->StudioFrameAdvance();
	}
}

//=============================================================================
//
// TANK CONTROLLING
//

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CFuncTank::OnControls( CBaseEntity *pTest )
{
	// Is the tank controllable.
	if ( !IsControllable() )
		return false;

	if ( !m_hControlVolume )
	{
		// Find our control volume
		if ( m_iszControlVolume != NULL_STRING )
		{
			CTrigger *trigger = dynamic_cast<CTrigger*>( g_helpfunc.FindEntityByName( (CBaseEntity *)NULL, m_iszControlVolume ) );
			m_hControlVolume.Set((trigger)?trigger->BaseEntity():NULL); 
		}

		if (( !m_hControlVolume ) && IsControllable() )
		{
			Msg( "ERROR: Couldn't find control volume for player-controllable func_tank %s.\n", GetEntityName() );
			return false;
		}
	}

	if ( m_hControlVolume->IsTouching( CEntity::Instance(pTest) ) )
		return true;
	
	return false;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CFuncTank::StartControl( CCombatCharacter *pController )
{
	// Check to see if we have a controller.
	if ( HasController() && GetController() != pController )
		return false;

	// Team only or disabled?
	if ( m_iszMaster != NULL_STRING )
	{
		if ( !UTIL_IsMasterTriggered( m_iszMaster, pController ) )
			return false;
	}

	// Set func_tank as manned by player/npc.
	m_hController.Set((pController)?pController->BaseEntity():NULL);
	if ( pController->IsPlayer() )
	{
		m_spawnflags |= SF_TANK_PLAYER; 

		CPlayer *pPlayer = static_cast<CPlayer*>( m_hController.Get() );
		pPlayer->m_iHideHUD |= HIDEHUD_WEAPONSELECTION;
	}
	else
	{
		m_spawnflags |= SF_TANK_NPC;
		NPC_SetInRoute( false );
	}

	// Holster player/npc weapon
	if ( m_hController->GetActiveWeapon() )
	{
		m_hController->GetActiveWeapon()->Holster();
	}

	// Set the controller's position to be the use position.
	m_vecControllerUsePos = m_hController->GetLocalOrigin();

	EmitSound( "Func_Tank.BeginUse" );
	
	SetNextThink( gpGlobals->curtime + 0.1f );
	
	// Let the map maker know a controller has been found
	if ( m_hController->IsPlayer() )
	{
		m_OnGotPlayerController.FireOutput( this, this );
	}
	else
	{
		m_OnGotController.FireOutput( this, this );
	}

	OnStartControlled();
	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
// TODO: bring back the controllers current weapon
//-----------------------------------------------------------------------------
void CFuncTank::StopControl()
{
	// Do we have a controller?
	if ( !m_hController )
		return;

	OnStopControlled();

	// Arm player/npc weapon.
	if ( m_hController->GetActiveWeapon() )
	{
		m_hController->GetActiveWeapon()->Deploy();
	}

	if ( m_hController->IsPlayer() )
	{
		CPlayer *pPlayer = static_cast<CPlayer*>( m_hController.Get() );
		pPlayer->m_iHideHUD &= ~HIDEHUD_WEAPONSELECTION;
	}

	// Stop thinking.
	SetNextThink( TICK_NEVER_THINK );
	
	// Let the map maker know a controller has been lost.
	if ( m_hController->IsPlayer() )
	{
		m_OnLostPlayerController.FireOutput( this, this );
	}
	else
	{
		m_OnLostController.FireOutput( this, this );
	}

	// Reset the func_tank as unmanned (player/npc).
	if ( m_hController->IsPlayer() )
	{
		m_spawnflags &= ~SF_TANK_PLAYER;
	}
	else
	{		
		m_spawnflags &= ~SF_TANK_NPC;
	}

	m_hController.Set(NULL);

	// Set think, if the func_tank can think on its own.
	if ( IsActive() || (IsControllable() && !HasController()) )
	{
		// Delay the think to find controllers a bit
#ifdef FUNCTANK_AUTOUSE
		m_flNextControllerSearch = gpGlobals->curtime + 1.0f;
#else
		m_flNextControllerSearch = gpGlobals->curtime + 5.0f;
#endif//FUNCTANK_AUTOUSE

		SetNextThink( m_flNextControllerSearch );
	}

	SetLocalAngularVelocity( vec3_angle );
}

//-----------------------------------------------------------------------------
// Purpose:
// Called each frame by the player's ItemPostFrame
//-----------------------------------------------------------------------------
void CFuncTank::ControllerPostFrame( void )
{
	// Make sure we have a contoller.
	Assert( m_hController != NULL );

	// Control the firing rate.
	if ( gpGlobals->curtime < m_flNextAttack )
		return;

	if ( !IsPlayerManned() )
		return;

	CPlayer *pPlayer = static_cast<CPlayer*>( m_hController.Get() );
	if ( ( pPlayer->m_nButtons & IN_ATTACK ) == 0 )
		return;

	Vector forward;
	AngleVectors( GetAbsAngles(), &forward );
	m_fireLast = gpGlobals->curtime - (1/m_fireRate) - 0.01;  // to make sure the gun doesn't fire too many bullets
	
	int bulletCount = (int)((gpGlobals->curtime - m_fireLast) * m_fireRate);
	
	if( HasSpawnFlags( SF_TANK_AIM_ASSISTANCE ) )
	{
		// Trace out a hull and if it hits something, adjust the shot to hit that thing.
		trace_t tr;
		Vector start = WorldBarrelPosition();
		Vector dir = forward;
		
		UTIL_TraceHull( start, start + forward * 8192, -Vector(8,8,8), Vector(8,8,8), MASK_SHOT, BaseEntity(), COLLISION_GROUP_NONE, &tr );
		
		CEntity *pEnt = CEntity::Instance(tr.m_pEnt);
		if( pEnt && pEnt->m_takedamage != DAMAGE_NO && (pEnt->GetFlags() & FL_AIMTARGET) )
		{
			forward = pEnt->WorldSpaceCenter() - start;
			VectorNormalize( forward );
		}
	}
	
	Fire( bulletCount, WorldBarrelPosition(), forward, pPlayer, false );

	// HACKHACK -- make some noise (that the AI can hear)
	g_helpfunc.CSoundEnt_InsertSound( SOUND_COMBAT, WorldSpaceCenter(), FUNCTANK_FIREVOLUME, 0.2 );
	
	if( m_iAmmoCount > -1 )
	{
		if( !(m_iAmmoCount % 10) )
		{
			Msg("Ammo Remaining: %d\n", m_iAmmoCount );
		}
		
		if( --m_iAmmoCount == 0 )
		{
			// Kick the player off the gun, and make myself not usable.
			m_spawnflags &= ~SF_TANK_CANCONTROL;
			StopControl();
			return;				
		}
	}
	SetNextAttack( gpGlobals->curtime + (1/m_fireRate) );
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CFuncTank::HasController( void )
{ 
	return (m_hController != NULL); 
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CBaseCombatCharacter
//-----------------------------------------------------------------------------
CCombatCharacter *CFuncTank::GetController( void ) 
{ 
	return m_hController; 
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CFuncTank::NPC_FindManPoint( Vector &vecPos )
{
	if ( m_iszNPCManPoint != NULL_STRING )
	{	
		CEntity *pEntity = g_helpfunc.FindEntityByName( (CBaseEntity *)NULL, m_iszNPCManPoint );
		if ( pEntity )
		{
			vecPos = pEntity->GetAbsOrigin();
			return true;
		}
	}

	return false; 
}

//-----------------------------------------------------------------------------
// Purpose: The NPC manning this gun just saw a player for the first time since he left cover
//-----------------------------------------------------------------------------
void CFuncTank::NPC_JustSawPlayer( CEntity *pTarget )
{
	SetNextAttack( gpGlobals->curtime + m_flPlayerLockTimeBeforeFire );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CFuncTank::NPC_Fire( void )
{
	// Control the firing rate.
	if ( gpGlobals->curtime < m_flNextAttack )
		return;

	// Check for a valid npc controller.
	if ( !m_hController )
		return;

	CAI_NPC *pNPC = m_hController->MyNPCPointer();
	if ( !pNPC )
		return;

	// Setup for next round of firing.
	if ( m_nBulletCount == 0 )
	{
		m_nBulletCount = GetRandomBurst();
		m_fireTime = 1.0f;
	}

	// m_fireLast looks like it is only needed for Active non-controlled func_tank.
//		m_fireLast = gpGlobals->curtime - (1/m_fireRate) - 0.01;  // to make sure the gun doesn't fire too many bullets		

	Vector vecBarrelEnd = WorldBarrelPosition();		
	Vector vecForward;
	AngleVectors( GetAbsAngles(), &vecForward );

	if ( (pNPC->CapabilitiesGet() & bits_CAP_NO_HIT_SQUADMATES) && pNPC->IsInSquad() )
	{
		// Avoid shooting squadmates.
		if ( pNPC->IsSquadmateInSpread( vecBarrelEnd, vecBarrelEnd + vecForward * 2048, gTankSpread[m_spread].x, 8*12 ) )
		{
			return;
		}
	}

	if ( !HasSpawnFlags( SF_TANK_ALLOW_PLAYER_HITS ) && (pNPC->CapabilitiesGet() & bits_CAP_NO_HIT_PLAYER) )
	{
		// Avoid shooting player.
		if ( pNPC->PlayerInSpread( vecBarrelEnd, vecBarrelEnd + vecForward * 2048, gTankSpread[m_spread].x, 8*12 ) )
		{
			return;
		}
	}

	bool bIgnoreSpread = false;

  	CEntity *pEnemy = pNPC->GetEnemy();
	if ( HasSpawnFlags( SF_TANK_HACKPLAYERHIT ) && pEnemy && pEnemy->IsPlayer() )
	{
		// Every third shot should be fired directly at the player
		if ( m_nBulletCount%2 == 0 )
		{
			Vector vecBodyTarget = pEnemy->BodyTarget( vecBarrelEnd, false );
			vecForward = (vecBodyTarget - vecBarrelEnd);
			VectorNormalize( vecForward );
			bIgnoreSpread = true;
		}
	}

	// Fire the bullet(s).
	Fire( 1, vecBarrelEnd, vecForward, m_hController, bIgnoreSpread );
	--m_nBulletCount;

	// Check ammo counts and dismount when empty.
	if( m_iAmmoCount > -1 )
	{
		if( --m_iAmmoCount == 0 )
		{
			// Disable the func_tank.
			m_spawnflags &= ~SF_TANK_CANCONTROL;

			// Remove the npc.
			StopControl();
			return;				
		}
	}
	
	float flFireTime = GetRandomFireTime();
	if ( m_nBulletCount != 0 )
	{	
		m_fireTime -= flFireTime;
		SetNextAttack( gpGlobals->curtime + flFireTime );
	}
	else
	{
		SetNextAttack( gpGlobals->curtime + m_fireTime );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CFuncTank::NPC_HasEnemy( void )
{
	if ( !IsNPCManned() )
		return false;

	CAI_NPC *pNPC = m_hController->MyNPCPointer();
	Assert( pNPC );

	return ( pNPC->GetEnemy() != NULL );
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CFuncTank::NPC_InterruptRoute( void )
{
	if ( !m_hController )
		return;

	CAI_NPC *pNPC = m_hController->MyNPCPointer();
	if ( !pNPC )
		return;

	//CE_TODO
	/*CAI_FuncTankBehavior *pBehavior;
	if ( pNPC->GetBehavior( &pBehavior ) )
	{
		pBehavior->SetFuncTank( NULL );
	}*/

	// Reset the npc controller.
	m_hController.Set(NULL);

	// No NPC's in route.
	NPC_SetInRoute( false );

	// Delay the think to find controllers a bit
	m_flNextControllerSearch = gpGlobals->curtime + 5.0f;

	if ( !HasController() )
	{
		// Start thinking to find controllers again
		SetNextThink( m_flNextControllerSearch );
	}
}


//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CFuncTank::NPC_InterruptController( void )
{
	// If we don't have a controller - then the gun should be free.
	if ( !m_hController )
		return true;

	CAI_NPC *pNPC = m_hController->MyNPCPointer();
	if ( !pNPC || !pNPC->IsPlayerAlly() )
		return false;

	//CE_TODO
	/*CAI_FuncTankBehavior *pBehavior;
	if ( pNPC->GetBehavior( &pBehavior ) )
	{
		pBehavior->Dismount();
	}*/

	m_hController.Set(NULL);
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CFuncTank::GetRandomFireTime( void )
{
	Assert( m_fireRate != 0 );
	float flOOFireRate = 1.0f / m_fireRate;
	float flOOFireRateBy2 = flOOFireRate * 0.5f;
	float flOOFireRateBy4 = flOOFireRate * 0.25f;
	return enginerandom->RandomFloat( flOOFireRateBy4, flOOFireRateBy2 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CFuncTank::GetRandomBurst( void )
{
	return enginerandom->RandomInt( (int)m_fireRate-2, (int)m_fireRate+2 );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pActivator - 
//			*pCaller - 
//			useType - 
//			value - 
//-----------------------------------------------------------------------------
void CFuncTank::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !IsControllable() )
		return;

	// player controlled turret
	CPlayer *pPlayer = ToBasePlayer( CEntity::Instance(pActivator) );
	if ( !pPlayer )
		return;

	if ( value == 2 && useType == USE_SET )
	{
		ControllerPostFrame();
	}
	else if ( m_hController != pPlayer && useType != USE_OFF )
	{
		// The player must be within the func_tank controls
		if ( !m_hControlVolume )
		{
			// Find our control volume
			if ( m_iszControlVolume != NULL_STRING )
			{
				CTrigger *trigger = dynamic_cast<CTrigger*>( g_helpfunc.FindEntityByName( (CBaseEntity *)NULL, m_iszControlVolume ) );
				m_hControlVolume.Set((trigger)?trigger->BaseEntity():NULL); 
			}

			if (( !m_hControlVolume ) && IsControllable() )
			{
				Msg( "ERROR: Couldn't find control volume for player-controllable func_tank %s.\n", GetEntityName() );
				return;
			}
		}

		if ( !m_hControlVolume->IsTouching( pPlayer ) )
			return;

		// Interrupt any npc in route (ally or not).
		if ( NPC_InRoute() )
		{
			// Interrupt the npc's route.
			NPC_InterruptRoute();
		}

		// Interrupt NPC - if possible (they must be allies).
		if ( IsNPCControllable() && HasController() )
		{
			if ( !NPC_InterruptController() )
				return;
		}

		pPlayer->SetUseEntity( this );
		StartControl( pPlayer );
	}
	else 
	{
		StopControl();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : range - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CFuncTank::InRange( float range )
{
	if ( range < m_minRange )
		return FALSE;
	if ( (m_maxRange > 0) && (range > m_maxRange) )
		return FALSE;

	return TRUE;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CFuncTank::InRange2( float flRange2 )
{
	if ( flRange2 < m_flMinRange2 )
		return false;

	if ( ( m_flMaxRange2 > 0.0f ) && ( flRange2 > m_flMaxRange2 ) )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CFuncTank::Think( void )
{
	FuncTankPreThink();

	m_hFuncTankTarget.Set(NULL);

	// Look for a new controller?
	if ( IsControllable() && !HasController() && (m_flNextControllerSearch <= gpGlobals->curtime) )
	{
		if ( m_bShouldFindNPCs && gpGlobals->curtime > 5.0f )
		{
			// Check for in route and timer.
			if ( !NPC_InRoute() )
			{
				NPC_FindController();
			}
		}

#ifdef FUNCTANK_AUTOUSE
		CBasePlayer *pPlayer = UTIL_PlayerByIndex(1);
		bool bThinkFast = false;

		if( pPlayer )
		{
			if ( !m_hControlVolume )
			{
				// Find our control volume
				if ( m_iszControlVolume != NULL_STRING )
				{
					m_hControlVolume = dynamic_cast<CBaseTrigger*>( gEntList.FindEntityByName( NULL, m_iszControlVolume ) );
				}

				if (( !m_hControlVolume ) && IsControllable() )
				{
					Msg( "ERROR: Couldn't find control volume for player-controllable func_tank %s.\n", STRING(GetEntityName()) );
					return;
				}
			}

			if ( m_hControlVolume )
			{
				if( m_hControlVolume->IsTouching( pPlayer ) && pPlayer->FInViewCone(WorldSpaceCenter()) )
				{
					// If my control volume is touching a player that's facing the mounted gun, automatically use the gun.
					// !!!BUGBUG - this only works in cases where the player can see the gun whilst standing in the control 
					// volume. (This works just fine for all func_tanks mounted on combine walls and small barriers)
					variant_t emptyVariant;
					CustomAcceptInput( "Use", pPlayer, pPlayer, emptyVariant, USE_TOGGLE );
				}
				else
				{
					// If the player is nearby, think faster for snappier response to XBox auto mounting
					float flDistSqr = GetAbsOrigin().DistToSqr( pPlayer->GetAbsOrigin() );

					if( flDistSqr <= Square(360) )
					{
						bThinkFast = true;
					}
				}
			}
		}

		// Keep thinking, in case they turn NPC finding back on
		if ( !HasController() )
		{
			if( bThinkFast )
			{
				SetNextThink( gpGlobals->curtime + 0.1f );
			}
			else
			{
				SetNextThink( gpGlobals->curtime + 2.0f );
			}
		}

		if( bThinkFast )
		{
			m_flNextControllerSearch = gpGlobals->curtime + 0.1f;
		}
		else
		{
			m_flNextControllerSearch = gpGlobals->curtime + 2.0f;
		}
#else
		// Keep thinking, in case they turn NPC finding back on
		if ( !HasController() )
		{
			SetNextThink( gpGlobals->curtime + 2.0f );
		}

		m_flNextControllerSearch = gpGlobals->curtime + 2.0f;
#endif//FUNCTANK_AUTOUSE
	}

	// refresh the matrix
	UpdateMatrix();

	SetLocalAngularVelocity( vec3_angle );
	TrackTarget();

	if ( fabs(GetLocalAngularVelocity().x) > 1 || fabs(GetLocalAngularVelocity().y) > 1 )
	{
		StartRotSound();
	}
	else
	{
		StopRotSound();
	}

	FuncTankPostThink();
}


//-----------------------------------------------------------------------------
// Purpose: Aim the offset barrel at a position in parent space
// Input  : parentTarget - the position of the target in parent space
// Output : Vector - angles in local space
//-----------------------------------------------------------------------------
QAngle CFuncTank::AimBarrelAt( const Vector &parentTarget )
{
	Vector target = parentTarget - GetLocalOrigin();
	float quadTarget = target.LengthSqr();
	float quadTargetXY = target.x*target.x + target.y*target.y;

	// Target is too close!  Can't aim at it
	if ( quadTarget <= m_barrelPos.LengthSqr() )
	{
		return GetLocalAngles();
	}
	else
	{
		// We're trying to aim the offset barrel at an arbitrary point.
		// To calculate this, I think of the target as being on a sphere with 
		// it's center at the origin of the gun.
		// The rotation we need is the opposite of the rotation that moves the target 
		// along the surface of that sphere to intersect with the gun's shooting direction
		// To calculate that rotation, we simply calculate the intersection of the ray 
		// coming out of the barrel with the target sphere (that's the new target position)
		// and use atan2() to get angles

		// angles from target pos to center
		float targetToCenterYaw = atan2( target.y, target.x );
		float centerToGunYaw = atan2( m_barrelPos.y, sqrt( quadTarget - (m_barrelPos.y*m_barrelPos.y) ) );

		float targetToCenterPitch = atan2( target.z, sqrt( quadTargetXY ) );
		float centerToGunPitch = atan2( -m_barrelPos.z, sqrt( quadTarget - (m_barrelPos.z*m_barrelPos.z) ) );
		return QAngle( -RAD2DEG(targetToCenterPitch+centerToGunPitch), RAD2DEG( targetToCenterYaw + centerToGunYaw ), 0 );
	}
}


//-----------------------------------------------------------------------------
// Aim the tank at the player crosshair 
//-----------------------------------------------------------------------------
void CFuncTank::CalcPlayerCrosshairTarget( Vector *pVecTarget )
{
	// Get the player.
	CPlayer *pPlayer = static_cast<CPlayer*>( m_hController.Get() );

	// Tank aims at player's crosshair.
	Vector vecStart, vecDir;
	trace_t	tr;
	
	vecStart = pPlayer->EyePosition();


	//vecDir = pPlayer->EyeDirection3D();

	//META_CONPRINTF("%f %f %f\n",vecDir.x, vecDir.y,vecDir.z);

	vecDir = pPlayer->GetAutoaimVector_Float( AUTOAIM_SCALE_DEFAULT );

	// Make sure to start the trace outside of the player's bbox!
	UTIL_TraceLine( vecStart + vecDir * 24, vecStart + vecDir * 8192, MASK_BLOCKLOS_AND_NPCS, BaseEntity(), COLLISION_GROUP_NONE, &tr );

	*pVecTarget = tr.endpos;
}

//-----------------------------------------------------------------------------
// Aim the tank at the player crosshair 
//-----------------------------------------------------------------------------
void CFuncTank::AimBarrelAtPlayerCrosshair( QAngle *pAngles )
{
	Vector vecTarget;
	CalcPlayerCrosshairTarget( &vecTarget );
	*pAngles = AimBarrelAt( m_parentMatrix.WorldToLocal( vecTarget ) );
}


//-----------------------------------------------------------------------------
// Aim the tank at the NPC's enemy
//-----------------------------------------------------------------------------
void CFuncTank::CalcNPCEnemyTarget( Vector *pVecTarget )
{
	Vector vecTarget;
	CAI_NPC *pNPC = m_hController->MyNPCPointer();

	// Aim the barrel at the npc's enemy, or where the npc is looking.
	CEntity *pEnemy = pNPC->GetEnemy();
	if ( pEnemy )
	{
		// Clear the idle target
		*pVecTarget = pEnemy->BodyTarget( GetAbsOrigin(), false );
		m_vecNPCIdleTarget = *pVecTarget;
	}
	else
	{
		if ( m_vecNPCIdleTarget != vec3_origin )
		{
			*pVecTarget = m_vecNPCIdleTarget;
		}
		else
		{
			Vector vecForward;
			QAngle angCenter( 0, m_yawCenterWorld, 0 );
			AngleVectors( angCenter, &vecForward );
			trace_t tr;
			Vector vecBarrel = GetAbsOrigin() + m_barrelPos;
			UTIL_TraceLine( vecBarrel, vecBarrel + vecForward * 8192, MASK_SHOT, BaseEntity(), COLLISION_GROUP_NONE, &tr );
			*pVecTarget = tr.endpos;
		}
	}
}

	
//-----------------------------------------------------------------------------
// Aim the tank at the NPC's enemy
//-----------------------------------------------------------------------------
void CFuncTank::AimBarrelAtNPCEnemy( QAngle *pAngles )
{
	Vector vecTarget;
	CalcNPCEnemyTarget( &vecTarget );
	*pAngles = AimBarrelAt( m_parentMatrix.WorldToLocal( vecTarget ) );
}

//-----------------------------------------------------------------------------
// Returns true if the desired angles are out of range 
//-----------------------------------------------------------------------------
bool CFuncTank::RotateTankToAngles( const QAngle &angles, float *pDistX, float *pDistY )
{
	bool bClamped = false;

	// Force the angles to be relative to the center position
	float offsetY = UTIL_AngleDistance( angles.y, m_yawCenter );
	float offsetX = UTIL_AngleDistance( angles.x, m_pitchCenter );

	float flActualYaw = m_yawCenter + offsetY;
	float flActualPitch = m_pitchCenter + offsetX;

	if ( ( fabs( offsetY ) > m_yawRange + m_yawTolerance ) ||
		 ( fabs( offsetX ) > m_pitchRange + m_pitchTolerance ) )
	{
		// Limit against range in x
		flActualYaw = clamp( flActualYaw, m_yawCenter - m_yawRange, m_yawCenter + m_yawRange );
		flActualPitch = clamp( flActualPitch, m_pitchCenter - m_pitchRange, m_pitchCenter + m_pitchRange );

		bClamped = true;
	}

	// Get at the angular vel
	QAngle vecAngVel = GetLocalAngularVelocity();

	// Move toward target at rate or less
	float distY = UTIL_AngleDistance( flActualYaw, GetLocalAngles().y );
	vecAngVel.y = distY * 10;
	vecAngVel.y = clamp( vecAngVel.y, -m_yawRate, m_yawRate );

	// Move toward target at rate or less
	float distX = UTIL_AngleDistance( flActualPitch, GetLocalAngles().x );
	vecAngVel.x = distX  * 10;
	vecAngVel.x = clamp( vecAngVel.x, -m_pitchRate, m_pitchRate );

	// How exciting! We're done
	SetLocalAngularVelocity( vecAngVel );

	if ( pDistX && pDistY )
	{
		*pDistX = distX;
		*pDistY = distY;
	}

	return bClamped;
}


//-----------------------------------------------------------------------------
// We lost our target! 
//-----------------------------------------------------------------------------
void CFuncTank::LostTarget( void )
{
	if (m_fireLast != 0)
	{
		m_OnLoseTarget.FireOutput(this, this);
		m_fireLast = 0;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncTank::ComputeLeadingPosition( const Vector &vecShootPosition, CEntity *pTarget, Vector *pLeadPosition )
{
	Vector vecTarget = pTarget->BodyTarget( vecShootPosition, false );
	float flShotSpeed = GetShotSpeed();
	if ( flShotSpeed == 0 )
	{
		*pLeadPosition = vecTarget;
		return;
	}

	Vector vecVelocity = pTarget->GetSmoothedVelocity();
	vecVelocity.z = 0.0f;
	float flTargetSpeed = VectorNormalize( vecVelocity );

	// Guesstimate...
	if ( m_flNextLeadFactorTime < gpGlobals->curtime )
	{
		m_flStartLeadFactor = m_flNextLeadFactor;
		m_flStartLeadFactorTime = gpGlobals->curtime;
		m_flNextLeadFactor = enginerandom->RandomFloat( 0.8f, 1.3f );
		m_flNextLeadFactorTime = gpGlobals->curtime + enginerandom->RandomFloat( 2.0f, 4.0f );
	}

	float flFactor = (gpGlobals->curtime - m_flStartLeadFactorTime) / (m_flNextLeadFactorTime - m_flStartLeadFactorTime);
	float flLeadFactor = SimpleSplineRemapVal( flFactor, 0.0f, 1.0f, m_flStartLeadFactor, m_flNextLeadFactor );
	flTargetSpeed *= flLeadFactor;

	Vector vecDelta;
	VectorSubtract( vecShootPosition, vecTarget, vecDelta );
	float flTargetToShooter = VectorNormalize( vecDelta );
	float flCosTheta = DotProduct( vecDelta, vecVelocity );

	// Law of cosines... z^2 = x^2 + y^2 - 2xy cos Theta
	// where z = flShooterToPredictedTargetPosition = flShotSpeed * predicted time
	// x = flTargetSpeed * predicted time
	// y = flTargetToShooter
	// solve for predicted time using at^2 + bt + c = 0, t = (-b +/- sqrt( b^2 - 4ac )) / 2a
	float a = flTargetSpeed * flTargetSpeed - flShotSpeed * flShotSpeed;
	float b = -2.0f * flTargetToShooter * flCosTheta * flTargetSpeed;
	float c = flTargetToShooter * flTargetToShooter;
	
	float flDiscrim = b*b - 4*a*c;
	if (flDiscrim < 0)
	{
		*pLeadPosition = vecTarget;
		return;
	}

	flDiscrim = sqrt(flDiscrim);
	float t = (-b + flDiscrim) / (2.0f * a);
	float t2 = (-b - flDiscrim) / (2.0f * a);
	if ( t < t2 )
	{
		t = t2;
	}

	if ( t <= 0.0f )
	{
		*pLeadPosition = vecTarget;
		return;
	}

	VectorMA( vecTarget, flTargetSpeed * t, vecVelocity, *pLeadPosition );
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncTank::AimFuncTankAtTarget( void )
{
	// Get world target position
	CEntity *pTarget = NULL;
	trace_t tr;
	QAngle angles;
	bool bUpdateTime = false;

	CEntity *pTargetVehicle = NULL;
	Vector barrelEnd = WorldBarrelPosition();
	Vector worldTargetPosition;
	if (m_spawnflags & SF_TANK_AIM_AT_POS)
	{
		worldTargetPosition = m_vTargetPosition;
	}
	else
	{
		CEntity *pEntity = m_hTarget;
		if ( !pEntity || ( pEntity->GetFlags() & FL_NOTARGET ) )
		{
			if( m_targetEntityName != NULL_STRING )
			{
				CEntity *cent = FindTarget( m_targetEntityName, NULL );
				m_hTarget.Set((cent)?cent->BaseEntity():NULL);
			}
			
			LostTarget();
			return;
		}

		pTarget = pEntity;

		// Calculate angle needed to aim at target
		worldTargetPosition = pEntity->EyePosition();
		if ( pEntity->IsPlayer() )
		{
			CPlayer *pPlayer = assert_cast<CPlayer*>(pEntity);
			pTargetVehicle = CEntity::Instance(pPlayer->GetVehicleEntity());
			if ( pTargetVehicle )
			{
				worldTargetPosition = pTargetVehicle->BodyTarget( GetAbsOrigin(), false );
			}
		}
	}

	float range2 = worldTargetPosition.DistToSqr( barrelEnd );
	if ( !InRange2( range2 ) )
	{
		if ( m_hTarget )
		{
			m_hTarget.Set(NULL);
			LostTarget();
		}
		return;
	}

	Vector vecAimOrigin = m_sightOrigin;
	if (m_spawnflags & SF_TANK_AIM_AT_POS)
	{
		bUpdateTime		= true;
		m_sightOrigin	= m_vTargetPosition;
		vecAimOrigin = m_sightOrigin;
	}
	else
	{
		if ( m_spawnflags & SF_TANK_LINEOFSIGHT )
		{
			AI_TraceLOS( barrelEnd, worldTargetPosition, BaseEntity(), &tr );
		}
		else
		{
			tr.fraction = 1.0f;
			tr.m_pEnt = (pTarget)?pTarget->BaseEntity():NULL;
		}

		CEntity *cent = CEntity::Instance(tr.m_pEnt);

		// No line of sight, don't track
		if ( tr.fraction == 1.0 || cent == pTarget || (pTargetVehicle && (cent == pTargetVehicle)) )
		{
			if ( InRange2( range2 ) && pTarget && pTarget->IsAlive() )
			{
				bUpdateTime = true;

				// Sight position is BodyTarget with no noise (so gun doesn't bob up and down)
				CEntity *pInstance = pTargetVehicle ? pTargetVehicle : pTarget;
				m_hFuncTankTarget.Set((pInstance)?pInstance->BaseEntity():NULL);

				m_sightOrigin = pInstance->BodyTarget( GetAbsOrigin(), false );
				if ( m_bPerformLeading )
				{
					ComputeLeadingPosition( barrelEnd, pInstance, &vecAimOrigin );
				}
				else
				{
					vecAimOrigin = m_sightOrigin;
				}
			}
		}
	}

	// Convert targetPosition to parent
	Vector vecLocalOrigin = m_parentMatrix.WorldToLocal( vecAimOrigin );
	angles = AimBarrelAt( vecLocalOrigin );

	// FIXME: These need to be the clamped angles
	float distX, distY;
	bool bClamped = RotateTankToAngles( angles, &distX, &distY );
	if ( bClamped )
	{
		bUpdateTime = false;
	}

	if ( bUpdateTime )
	{
		if( (gpGlobals->curtime - m_lastSightTime >= 1.0) && (gpGlobals->curtime > m_flNextAttack) )
		{
			// Enemy was hidden for a while, and I COULD fire right now. Instead, tack a delay on.
			m_flNextAttack = gpGlobals->curtime + 0.5;
		}

		m_lastSightTime = gpGlobals->curtime;
		m_persist2burst = 0;
	}

	SetMoveDoneTime( 0.1 );

	if ( CanFire() && ( (fabs(distX) <= m_pitchTolerance) && (fabs(distY) <= m_yawTolerance) || (m_spawnflags & SF_TANK_LINEOFSIGHT) ) )
	{
		bool fire = false;
		Vector forward;
		AngleVectors( GetLocalAngles(), &forward );
		forward = m_parentMatrix.ApplyRotation( forward );

		if ( m_spawnflags & SF_TANK_LINEOFSIGHT )
		{
			UTIL_TraceLine( barrelEnd, pTarget->WorldSpaceCenter(), MASK_SHOT, BaseEntity(), COLLISION_GROUP_NONE, &tr );

			if ( tr.fraction == 1.0f || (tr.m_pEnt && tr.m_pEnt == pTarget->BaseEntity()) )
			{
				fire = true;
			}
		}
		else
		{
			fire = true;
		}

		if ( fire )
		{
			if (m_fireLast == 0)
			{
				m_OnAquireTarget.FireOutput(this, this);
			}
			FiringSequence( barrelEnd, forward, this );
		}
		else 
		{
			LostTarget();
		}
	}
	else 
	{
		LostTarget();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncTank::TrackTarget( void )
{
	QAngle angles;

	if( !m_bReadyToFire && m_flNextAttack <= gpGlobals->curtime )
	{
		m_OnReadyToFire.FireOutput( this, this );
		m_bReadyToFire = true;
	}

	if ( IsPlayerManned() )
	{
		AimBarrelAtPlayerCrosshair( &angles );
		RotateTankToAngles( angles );
		SetNextThink( gpGlobals->curtime + 0.05f );
		SetMoveDoneTime( 0.1 );
		return;
	}

	if ( IsNPCManned() )
	{
		AimBarrelAtNPCEnemy( &angles );
		RotateTankToAngles( angles );
		SetNextThink( gpGlobals->curtime + 0.05f );
		SetMoveDoneTime( 0.1 );
		return;
	}

	if ( !IsActive() )
	{
		// If we're not active, but we're controllable, we need to keep thinking
		if ( IsControllable() && !HasController() )
		{
			// Think to find controllers.
			SetNextThink( m_flNextControllerSearch );
		}
		return;
	}

	// Clean room for unnecessarily complicated old code
	SetNextThink( gpGlobals->curtime + 0.1f );
	AimFuncTankAtTarget();
}


//-----------------------------------------------------------------------------
// Purpose: Start of firing sequence.  By default, just fire now.
// Input  : &barrelEnd - 
//			&forward - 
//			*pAttacker - 
//-----------------------------------------------------------------------------
void CFuncTank::FiringSequence( const Vector &barrelEnd, const Vector &forward, CEntity *pAttacker )
{
	if ( m_fireLast != 0 )
	{
		int bulletCount = (int)((gpGlobals->curtime - m_fireLast) * m_fireRate);
		
		if ( bulletCount > 0 )
		{
			// NOTE: Set m_fireLast first so that Fire can adjust it
			m_fireLast = gpGlobals->curtime;
			Fire( bulletCount, barrelEnd, forward, pAttacker, false );
		}
	}
	else
	{
		m_fireLast = gpGlobals->curtime;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncTank::DoMuzzleFlash( void )
{
	// If we're parented to something, make it play the muzzleflash
	if ( m_bUsePoseParameters && GetParent() )
	{
		CAnimating *pAnim = GetParent()->GetBaseAnimating();
		pAnim->DoMuzzleFlash();

		// Do the AR2 muzzle flash
		if ( m_iEffectHandling == EH_COMBINE_CANNON )
		{
			CEffectData data;
			data.m_nAttachmentIndex = m_nBarrelAttachment;
			data.m_nEntIndex = pAnim->entindex();
			
			// FIXME: Create a custom entry here!
			g_helpfunc.DispatchEffect( "ChopperMuzzleFlash", data );
		}
		else
		{
			CEffectData data;
			data.m_nEntIndex = pAnim->entindex();
			data.m_nAttachmentIndex = m_nBarrelAttachment;
			data.m_flScale = 1.0f;
			data.m_fFlags = MUZZLEFLASH_COMBINE;

			g_helpfunc.DispatchEffect( "MuzzleFlash", data );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const char
//-----------------------------------------------------------------------------
const char *CFuncTank::GetTracerType( void )
{
	switch( m_iEffectHandling )
	{
	case EH_AR2:
		return "Tracer";//"AR2Tracer";

	case EH_COMBINE_CANNON:
		return "Tracer";//"HelicopterTracer";
	}

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose: Fire targets and spawn sprites.
// Input  : bulletCount - 
//			barrelEnd - 
//			forward - 
//			pAttacker - 
//-----------------------------------------------------------------------------
void CFuncTank::Fire( int bulletCount, const Vector &barrelEnd, const Vector &forward, CEntity *pAttacker, bool bIgnoreSpread )
{
	// If we have a specific effect handler, apply it's effects
	if ( m_iEffectHandling == EH_AR2 )
	{
		DoMuzzleFlash();

		// Play the AR2 sound
		EmitSound( "Weapon_functank.Single" );
	}
	else if ( m_iEffectHandling == EH_COMBINE_CANNON )
	{
		DoMuzzleFlash();

		// Play the cannon sound
		EmitSound( "NPC_Combine_Cannon.FireBullet" );
	}
	else
	{
		if ( m_iszSpriteSmoke != NULL_STRING )
		{
			CE_CSprite *pSprite = CE_CSprite::SpriteCreate( STRING(m_iszSpriteSmoke), barrelEnd, TRUE );
			pSprite->AnimateAndDie( enginerandom->RandomFloat( 15.0, 20.0 ) );
			pSprite->SetTransparency( kRenderTransAlpha, m_clrRender->r, m_clrRender->g, m_clrRender->b, 255, kRenderFxNone );

			Vector vecVelocity( 0, 0, enginerandom->RandomFloat(40, 80) ); 
			pSprite->SetAbsVelocity( vecVelocity );
			pSprite->SetScale( m_spriteScale );
		}
		if ( m_iszSpriteFlash != NULL_STRING )
		{
			CE_CSprite *pSprite = CE_CSprite::SpriteCreate( STRING(m_iszSpriteFlash), barrelEnd, TRUE );
			pSprite->AnimateAndDie( 5 );
			pSprite->SetTransparency( kRenderTransAdd, 255, 255, 255, 255, kRenderFxNoDissipation );
			pSprite->SetScale( m_spriteScale );
		}
	}

	if( pAttacker && pAttacker->IsPlayer() )
	{
		g_helpfunc.CSoundEnt_InsertSound( SOUND_MOVE_AWAY, barrelEnd + forward * 32.0f, 32, 0.2f, pAttacker->BaseEntity(), SOUNDENT_CHANNEL_WEAPON );
	}


	m_OnFire.FireOutput(this, this);
	m_bReadyToFire = false;
}


void CFuncTank::TankTrace( const Vector &vecStart, const Vector &vecForward, const Vector &vecSpread, trace_t &tr )
{
	Vector forward, right, up;

	AngleVectors( GetAbsAngles(), &forward, &right, &up );
	// get circular gaussian spread
	float x, y, z;
	do {
		x = enginerandom->RandomFloat(-0.5,0.5) + enginerandom->RandomFloat(-0.5,0.5);
		y = enginerandom->RandomFloat(-0.5,0.5) + enginerandom->RandomFloat(-0.5,0.5);
		z = x*x+y*y;
	} while (z > 1);
	Vector vecDir = vecForward +
		x * vecSpread.x * right +
		y * vecSpread.y * up;
	Vector vecEnd;
	
	vecEnd = vecStart + vecDir * MAX_TRACE_LENGTH;
	UTIL_TraceLine( vecStart, vecEnd, MASK_SOLID, BaseEntity(), COLLISION_GROUP_NONE, &tr );
}

	
void CFuncTank::StartRotSound( void )
{
	if ( m_spawnflags & SF_TANK_SOUNDON )
		return;
	m_spawnflags |= SF_TANK_SOUNDON;
	
	if ( m_soundLoopRotate != NULL_STRING )
	{
		CPASAttenuationFilter filter( this );
		filter.MakeReliable();

		EmitSound_t ep;
		ep.m_nChannel = CHAN_STATIC;
		ep.m_pSoundName = (char*)STRING(m_soundLoopRotate);
		ep.m_flVolume = 0.85;
		ep.m_SoundLevel = SNDLVL_NORM;

		EmitSound( filter, entindex(), ep );
	}
	
	if ( m_soundStartRotate != NULL_STRING )
	{
		CPASAttenuationFilter filter( this );

		EmitSound_t ep;
		ep.m_nChannel = CHAN_BODY;
		ep.m_pSoundName = (char*)STRING(m_soundStartRotate);
		ep.m_flVolume = 1.0f;
		ep.m_SoundLevel = SNDLVL_NORM;

		EmitSound( filter, entindex(), ep );
	}
}


void CFuncTank::StopRotSound( void )
{
	if ( m_spawnflags & SF_TANK_SOUNDON )
	{
		if ( m_soundLoopRotate != NULL_STRING )
		{
			StopSound( entindex(), CHAN_STATIC, (char*)STRING(m_soundLoopRotate) );
		}
		if ( m_soundStopRotate != NULL_STRING )
		{
			CPASAttenuationFilter filter( this );

			EmitSound_t ep;
			ep.m_nChannel = CHAN_BODY;
			ep.m_pSoundName = (char*)STRING(m_soundStopRotate);
			ep.m_flVolume = 1.0f;
			ep.m_SoundLevel = SNDLVL_NORM;

			EmitSound( filter, entindex(), ep );
		}
	}
	m_spawnflags &= ~SF_TANK_SOUNDON;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
bool CFuncTank::IsEntityInViewCone( CEntity *pEntity )
{
	// First check to see if the enemy is in range.
	Vector vecBarrelEnd = WorldBarrelPosition();
	float flRange2 = ( pEntity->GetAbsOrigin() - vecBarrelEnd ).LengthSqr();

	if( !(GetSpawnFlags() & SF_TANK_IGNORE_RANGE_IN_VIEWCONE) )
	{
		if ( !InRange2( flRange2 ) )
			return false;
	}

	// If we're trying to shoot at a player, and we've seen a non-player recently, check the grace period
	if ( m_flPlayerGracePeriod && pEntity->IsPlayer() && (gpGlobals->curtime - m_flLastSawNonPlayer) < m_flPlayerGracePeriod )
	{
		// Grace period is ignored under a certain distance
		if ( flRange2 > m_flIgnoreGraceUpto )
			return false;
	}

	// Check to see if the entity center lies within the yaw and pitch constraints.
	// This isn't horribly accurate, but should do for now.
	QAngle angGun;
	angGun = AimBarrelAt( m_parentMatrix.WorldToLocal( pEntity->GetAbsOrigin() ) );
	
	// Force the angles to be relative to the center position
	float flOffsetY = UTIL_AngleDistance( angGun.y, m_yawCenter );
	float flOffsetX = UTIL_AngleDistance( angGun.x, m_pitchCenter );
	angGun.y = m_yawCenter + flOffsetY;
	angGun.x = m_pitchCenter + flOffsetX;

	if ( ( fabs( flOffsetY ) > m_yawRange + m_yawTolerance ) || ( fabs( flOffsetX ) > m_pitchRange + m_pitchTolerance ) )
		return false;

	// Remember the last time we saw a non-player
	if ( !pEntity->IsPlayer() )
	{
		m_flLastSawNonPlayer = gpGlobals->curtime;
	}

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Return true if this func tank can see the enemy
//-----------------------------------------------------------------------------
bool CFuncTank::HasLOSTo( CEntity *pEntity )
{
	if ( !pEntity )
		return false;

	// Get the barrel position
	Vector vecBarrelEnd = WorldBarrelPosition();
	Vector vecTarget = pEntity->BodyTarget( GetAbsOrigin(), false );
	trace_t tr;

	// Ignore the func_tank and any prop it's parented to
	CEntity *parent = GetParent();
	CTraceFilterSkipTwoEntities traceFilter( BaseEntity(), (parent)?parent->BaseEntity():NULL, COLLISION_GROUP_NONE );

	// UNDONE: Should this hit BLOCKLOS brushes?
	UTIL_TraceLine( vecBarrelEnd, vecTarget, MASK_BLOCKLOS_AND_NPCS, &traceFilter, &tr );
	
	CEntity	*pHitEntity = CEntity::Instance(tr.m_pEnt);
	
	// Is entity in a vehicle? if so, verify vehicle is target and return if so (so npc shoots at vehicle)
	CCombatCharacter *pCCEntity = pEntity->MyCombatCharacterPointer();
	if ( pCCEntity != NULL && pCCEntity->IsInAVehicle() )
	{
		// Ok, player in vehicle, check if vehicle is target we're looking at, fire if it is
		// Also, check to see if the owner of the entity is the vehicle, in which case it's valid too.
		// This catches vehicles that use bone followers.
		CEntity	*pVehicle  = CEntity::Instance(pCCEntity->GetVehicle()->GetVehicleEnt());
		if ( pHitEntity == pVehicle || ( pHitEntity != NULL && pHitEntity->GetOwnerEntity() == pVehicle ) )
			return true;
	}

	return ( tr.fraction == 1.0 || pHitEntity == pEntity );
}


class CBulletsTraceFilter : public CTraceFilterSimpleList
{
public:
	CBulletsTraceFilter( int collisionGroup ) : CTraceFilterSimpleList( collisionGroup ) {}

	bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		if ( m_PassEntities.Count() )
		{
			CEntity *pEntity = CE_EntityFromEntityHandle( pHandleEntity );
			CEntity *pPassEntity = CE_EntityFromEntityHandle( m_PassEntities[0] );
			if ( pEntity && pPassEntity && pEntity->GetOwnerEntity() == pPassEntity && 
				pPassEntity->IsSolidFlagSet(FSOLID_NOT_SOLID) && pPassEntity->IsSolidFlagSet( FSOLID_CUSTOMBOXTEST ) && 
				pPassEntity->IsSolidFlagSet( FSOLID_CUSTOMRAYTEST ) )
			{
				// It's a bone follower of the entity to ignore (toml 8/3/2007)
				return false;
			}
		}
		return CTraceFilterSimpleList::ShouldHitEntity( pHandleEntity, contentsMask );
	}

};

void CFuncTank::ComputeTracerStartPosition( const Vector &vecShotSrc, Vector *pVecTracerStart )
{
	{
		*pVecTracerStart = vecShotSrc;

		CCombatCharacter *pBCC = MyCombatCharacterPointer();
		if ( pBCC != NULL )
		{
			CCombatWeapon *pWeapon = pBCC->GetActiveWeapon();

			if ( pWeapon != NULL )
			{
				Vector vecMuzzle;
				QAngle vecMuzzleAngles;

				if ( pWeapon->GetAttachment( 1, vecMuzzle, vecMuzzleAngles ) )
				{
					*pVecTracerStart = vecMuzzle;
				}
			}
		}
	}
}







// #############################################################################
//   CFuncTankGun
// #############################################################################
class CFuncTankGun : public CFuncTank
{
public:
	CE_DECLARE_CLASS( CFuncTankGun, CFuncTank );

	void Fire( int bulletCount, const Vector &barrelEnd, const Vector &forward, CEntity *pAttacker, bool bIgnoreSpread );
};

LINK_ENTITY_TO_CUSTOM_CLASS(func_tank, info_target, CFuncTankGun);


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CFuncTankGun::Fire( int bulletCount, const Vector &barrelEnd, const Vector &forward, CEntity *pAttacker, bool bIgnoreSpread )
{
	int i;

	FireBulletsInfo_t info;
	info.m_iShots = 1;
	info.m_vecSrc = barrelEnd;
	info.m_vecDirShooting = forward;
	if ( bIgnoreSpread )
	{
		info.m_vecSpread = gTankSpread[0];
	}
	else
	{
		info.m_vecSpread = gTankSpread[m_spread];
	}

	CEntity *parent = GetParent();
	info.m_flDistance = MAX_TRACE_LENGTH;
	info.m_iTracerFreq = 1;
	info.m_iDamage = m_iBulletDamage;
	info.m_iPlayerDamage = m_iBulletDamageVsPlayer;
	info.m_pAttacker = (pAttacker)?pAttacker->BaseEntity():NULL;
	info.m_pAdditionalIgnoreEnt = (parent)?parent->BaseEntity():NULL;

#ifdef HL2_EPISODIC
	if ( m_iAmmoType != -1 )
	{
		for ( i = 0; i < bulletCount; i++ )
		{
			info.m_iAmmoType = m_iAmmoType;
			FireBullets( info );
		}
	}
#else
	for ( i = 0; i < bulletCount; i++ )
	{
		switch( m_bulletType )
		{
		case TANK_BULLET_SMALL:
			info.m_iAmmoType = m_iSmallAmmoType;
			FireBullets( info );
			break;

		case TANK_BULLET_MEDIUM:
			info.m_iAmmoType = m_iMediumAmmoType;
			FireBullets( info );
			break;

		case TANK_BULLET_LARGE:
			info.m_iAmmoType = m_iLargeAmmoType;
			FireBullets( info );
			break;

		default:
		case TANK_BULLET_NONE:
			break;
		}
	}
#endif // HL2_EPISODIC

	CFuncTank::Fire( bulletCount, barrelEnd, forward, pAttacker, bIgnoreSpread );
}

// #############################################################################
//   CFuncTankPulseLaser
// #############################################################################
class CFuncTankPulseLaser : public CFuncTankGun
{
public:
	CE_DECLARE_CLASS( CFuncTankPulseLaser, CFuncTankGun );
	DECLARE_DATADESC();

	CFuncTankPulseLaser()
	{
		// Default setting from hammer
		m_flPulseSpeed = 1000.0f;
		m_flPulseWidth = 20.0f;
		m_flPulseLife = 2.0f;
		m_flPulseLag = 0.05;

		m_sPulseFireSound = NULL_STRING;

		m_flPulseColor.r = 255;
		m_flPulseColor.g = 0;
		m_flPulseColor.b = 0;
		m_flPulseColor.a = 255;

	}

	void Precache();
	void Fire( int bulletCount, const Vector &barrelEnd, const Vector &forward, CEntity *pAttacker, bool bIgnoreSpread );

	float		m_flPulseSpeed;
	float		m_flPulseWidth;
	color32		m_flPulseColor;
	float		m_flPulseLife;
	float		m_flPulseLag;
	string_t	m_sPulseFireSound;
};

LINK_ENTITY_TO_CUSTOM_CLASS( func_tankpulselaser, info_target, CFuncTankPulseLaser );

BEGIN_DATADESC( CFuncTankPulseLaser )

	DEFINE_KEYFIELD( m_flPulseSpeed,	 FIELD_FLOAT,		"PulseSpeed" ),
	DEFINE_KEYFIELD( m_flPulseWidth,	 FIELD_FLOAT,		"PulseWidth" ),
	DEFINE_KEYFIELD( m_flPulseColor,	 FIELD_COLOR32,		"PulseColor" ),
	DEFINE_KEYFIELD( m_flPulseLife,	 FIELD_FLOAT,		"PulseLife" ),
	DEFINE_KEYFIELD( m_flPulseLag,		 FIELD_FLOAT,		"PulseLag" ),
	DEFINE_KEYFIELD( m_sPulseFireSound, FIELD_SOUNDNAME,	"PulseFireSound" ),

END_DATADESC()

//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CFuncTankPulseLaser::Precache(void)
{
	//UTIL_PrecacheOther( "grenade_beam" );

	if ( m_sPulseFireSound != NULL_STRING )
	{
		PrecacheScriptSound( STRING(m_sPulseFireSound) );
	}
	BaseClass::Precache();
}
//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CFuncTankPulseLaser::Fire( int bulletCount, const Vector &barrelEnd, const Vector &vecForward, CEntity *pAttacker, bool bIgnoreSpread )
{
	// --------------------------------------------------
	//  Get direction vectors for spread
	// --------------------------------------------------
	Vector vecUp = Vector(0,0,1);
	Vector vecRight;
	CrossProduct ( vecForward,  vecUp,		vecRight );	
	CrossProduct ( vecForward, -vecRight,   vecUp  );	

	for ( int i = 0; i < bulletCount; i++ )
	{
		// get circular gaussian spread
		float x, y, z;
		do {
			x = enginerandom->RandomFloat(-0.5,0.5) + enginerandom->RandomFloat(-0.5,0.5);
			y = enginerandom->RandomFloat(-0.5,0.5) + enginerandom->RandomFloat(-0.5,0.5);
			z = x*x+y*y;
		} while (z > 1);

		Vector vecDir = vecForward + x * gTankSpread[m_spread].x * vecRight + y * gTankSpread[m_spread].y * vecUp;

		CGrenadeBeam *pPulse =  CGrenadeBeam::Create( pAttacker, barrelEnd);
		pPulse->Format(m_flPulseColor, m_flPulseWidth);
		pPulse->Shoot(vecDir,m_flPulseSpeed,m_flPulseLife,m_flPulseLag,m_iBulletDamage);

		if ( m_sPulseFireSound != NULL_STRING )
		{
			CPASAttenuationFilter filter( this, 0.6f );

			EmitSound_t ep;
			ep.m_nChannel = CHAN_WEAPON;
			ep.m_pSoundName = (char*)STRING(m_sPulseFireSound);
			ep.m_flVolume = 1.0f;
			ep.m_SoundLevel = SNDLVL_85dB;

			EmitSound( filter, entindex(), ep );
		}

	}
	CFuncTank::Fire( bulletCount, barrelEnd, vecForward, pAttacker, bIgnoreSpread );
}



// #############################################################################
//   CFuncTankLaser
// #############################################################################
class CFuncTankLaser : public CFuncTank
{
	CE_DECLARE_CLASS( CFuncTankLaser, CFuncTank );
public:
	CFuncTankLaser()
	{
		m_pLaser = NULL;
		m_laserTime = 0.0f;
		m_iszLaserName = NULL_STRING;
	}
	void	Activate( void );
	void	Fire( int bulletCount, const Vector &barrelEnd, const Vector &forward, CEntity *pAttacker, bool bIgnoreSpread );
	void	Think( void );
	CE_CEnvLaser *GetLaser( void );

	DECLARE_DATADESC();

private:
	CE_CEnvLaser	*m_pLaser;
	float			m_laserTime;
	string_t		m_iszLaserName;
};


LINK_ENTITY_TO_CUSTOM_CLASS( func_tanklaser, info_target, CFuncTankLaser );


BEGIN_DATADESC( CFuncTankLaser )

	DEFINE_KEYFIELD( m_iszLaserName, FIELD_STRING, "laserentity" ),

	DEFINE_FIELD( m_pLaser, FIELD_CLASSPTR ),
	DEFINE_FIELD( m_laserTime, FIELD_TIME ),

END_DATADESC()


void CFuncTankLaser::Activate( void )
{
	BaseClass::Activate();

	if ( !GetLaser() )
	{
		UTIL_Remove(this);
		Warning( "Laser tank with no env_laser!\n" );
	}
	else
	{
		m_pLaser->TurnOff();
	}
}


CE_CEnvLaser *CFuncTankLaser::GetLaser( void )
{
	if ( m_pLaser )
		return m_pLaser;

	CEntity *pLaser = g_helpfunc.FindEntityByName( (CBaseEntity *)NULL, m_iszLaserName );
	while ( pLaser )
	{
		// Found the landmark
		if ( FClassnameIs( pLaser, "env_laser" ) )
		{
			m_pLaser = (CE_CEnvLaser *)pLaser;
			break;
		}
		else
		{
			pLaser = g_helpfunc.FindEntityByName( pLaser, m_iszLaserName );
		}
	}

	return m_pLaser;
}


void CFuncTankLaser::Think( void )
{
	if ( m_pLaser && (gpGlobals->curtime > m_laserTime) )
		m_pLaser->TurnOff();

	CFuncTank::Think();
}


void CFuncTankLaser::Fire( int bulletCount, const Vector &barrelEnd, const Vector &forward, CEntity *pAttacker, bool bIgnoreSpread )
{
	int i;
	trace_t tr;

	if ( GetLaser() )
	{
		for ( i = 0; i < bulletCount; i++ )
		{
			m_pLaser->SetLocalOrigin( barrelEnd );
			TankTrace( barrelEnd, forward, gTankSpread[m_spread], tr );
			
			m_laserTime = gpGlobals->curtime;
			m_pLaser->TurnOn();
			m_pLaser->SetFireTime( gpGlobals->curtime - 1.0 );
			m_pLaser->FireAtPoint( tr );
			m_pLaser->SetNextThink( TICK_NEVER_THINK );
		}
		CFuncTank::Fire( bulletCount, barrelEnd, forward, this, bIgnoreSpread );
	}
}
