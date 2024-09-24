
#include "CAI_NPC_AI_Patch.h"
#include "CPlayer.h"
#include "CAI_tacticalservices.h"
#include "CESoundEnt.h"
#include "shot_manipulator.h"
#include "CCombatWeapon.h"
#include "npc_bullseye.h"
#include "eventqueue.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define	MIN_PHYSICS_FLINCH_DAMAGE	5.0f


CE_LINK_ENTITY_TO_CLASS(CAI_BaseNPC, CAI_NPC_AI_Patch);

extern ConVar *ai_use_think_optimizations;
extern ConVar *ai_use_efficiency;
extern ConVar *ai_efficiency_override;
extern ConVar *ai_frametime_limit;
extern ConVar *ai_default_efficient;
extern ConVar *ai_shot_stats;
extern ConVar *ai_shot_stats_term;

#define ShouldUseEfficiency()			( ai_use_think_optimizations->GetBool() && ai_use_efficiency->GetBool() )
#define ShouldDefaultEfficient()		( ai_use_think_optimizations->GetBool() && ai_default_efficient->GetBool() )



void CAI_NPC_AI_Patch::RunTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
		case TASK_FACE_PLAYER:
		{
			// Get edict for one player
			CPlayer *pPlayer = UTIL_GetNearestVisiblePlayer(this);
			if ( pPlayer )
			{
				GetMotor()->SetIdealYawToTargetAndUpdate( pPlayer->GetAbsOrigin(), AI_KEEP_YAW_SPEED );
				SetTurnActivity();
				if ( IsWaitFinished() && GetMotor()->DeltaIdealYaw() < 10 )
				{
					TaskComplete();
				}
			}
			else
			{
				TaskFail(FAIL_NO_PLAYER);
			}
		}
			break;
		case TASK_MOVE_AWAY_PATH:
		{
			QAngle ang = GetLocalAngles();
			ang.y = GetMotor()->GetIdealYaw() + 180;
			Vector move;

			switch ( GetTaskInterrupt() )
			{
				case 0:
				{
					if( IsPlayerAlly() )
					{
						// Look for a move away hint node.
						CE_AI_Hint *pHint;
						CHintCriteria hintCriteria;

						hintCriteria.AddHintType( HINT_PLAYER_ALLY_MOVE_AWAY_DEST );
						hintCriteria.SetFlag( bits_HINT_NODE_NEAREST );
						hintCriteria.AddIncludePosition( GetAbsOrigin(), (20.0f * 12.0f) ); // 20 feet max
						hintCriteria.AddExcludePosition( GetAbsOrigin(), 28.0f ); // don't plant on an hint that you start on

						pHint = CAI_HintManager::FindHint( this, hintCriteria );

						if( pHint )
						{
							CPlayer *pPlayer = UTIL_GetNearestPlayer(GetAbsOrigin());
							Vector vecGoal = pHint->GetAbsOrigin();

							if( pPlayer != NULL && vecGoal.DistToSqr(GetAbsOrigin()) < vecGoal.DistToSqr(pPlayer->GetAbsOrigin()) )
							{
								if( GetNavigator()->SetGoal(vecGoal) )
								{
									pHint->DisableForSeconds( 0.1f ); // Force others to find their own.
									TaskComplete();
									break;
								}
							}
						}
					}

#ifdef HL2_EPISODIC
					// See if we're moving away from a vehicle
					CSound *pBestSound = GetBestSound( SOUND_MOVE_AWAY );
					if ( pBestSound && pBestSound->m_hOwner && pBestSound->m_hOwner->GetServerVehicle() )
					{
						// Move away from the vehicle's center, regardless of our facing
						move = ( GetAbsOrigin() - pBestSound->m_hOwner->WorldSpaceCenter() );
						VectorNormalize( move );
					}
					else
					{
						// Use the first angles
						AngleVectors( ang, &move );
					}
#else
					AngleVectors( ang, &move );
#endif	//HL2_EPISODIC
					if ( GetNavigator()->SetVectorGoal( move, (float)pTask->flTaskData, MIN(36,pTask->flTaskData), true ) && IsValidMoveAwayDest( GetNavigator()->GetGoalPos() ))
					{
						TaskComplete();
					}
					else
					{
						ang.y = GetMotor()->GetIdealYaw() + 91;
						AngleVectors( ang, &move );

						if ( GetNavigator()->SetVectorGoal( move, (float)pTask->flTaskData, MIN(24,pTask->flTaskData), true ) && IsValidMoveAwayDest( GetNavigator()->GetGoalPos() ) )
						{
							TaskComplete();
						}
						else
						{
							TaskInterrupt();
						}
					}
				}
					break;

				case 1:
				{
					ang.y = GetMotor()->GetIdealYaw() + 271;
					AngleVectors( ang, &move );

					if ( GetNavigator()->SetVectorGoal( move, (float)pTask->flTaskData, MIN(24,pTask->flTaskData), true ) && IsValidMoveAwayDest( GetNavigator()->GetGoalPos() ) )
					{
						TaskComplete();
					}
					else
					{
						ang.y = GetMotor()->GetIdealYaw() + 180;
						while (ang.y < 0)
							ang.y += 360;
						while (ang.y >= 360)
							ang.y -= 360;
						if ( ang.y < 45 || ang.y >= 315 )
							ang.y = 0;
						else if ( ang.y < 135 )
							ang.y = 90;
						else if ( ang.y < 225 )
							ang.y = 180;
						else
							ang.y = 270;

						AngleVectors( ang, &move );

						if ( GetNavigator()->SetVectorGoal( move, (float)pTask->flTaskData, MIN(6,pTask->flTaskData), false ) && IsValidMoveAwayDest( GetNavigator()->GetGoalPos() ) )
						{
							TaskComplete();
						}
						else
						{
							TaskInterrupt();
						}
					}
				}
					break;

				case 2:
				{
					ClearTaskInterrupt();
					Vector coverPos;

					if ( GetTacticalServices()->FindCoverPos( GetLocalOrigin(), EyePosition(), 0, CoverRadius(), &coverPos ) && IsValidMoveAwayDest( GetNavigator()->GetGoalPos() ) )
					{
						GetNavigator()->SetGoal( AI_NavGoal_t( coverPos, ACT_RUN ) );
						m_flMoveWaitFinished = gpGlobals->curtime + 2;
					}
					else
					{
						// no coverwhatsoever.
						TaskFail(FAIL_NO_ROUTE);
					}
				}
					break;

			}
		}
			break;
		default:
			BaseClass::RunTask(pTask);
	}
}


void CAI_NPC_AI_Patch::UpdateEfficiency( bool bInPVS )
{
	// Sleeping NPCs always dormant
	if ( GetSleepState() != AISS_AWAKE )
	{
		SetEfficiency( AIE_DORMANT );
		return;
	}

	m_bInChoreo = ( GetState() == NPC_STATE_SCRIPT || IsCurSchedule( SCHED_SCENE_GENERIC, false ) );

	if ( !ShouldUseEfficiency() )
	{
		SetEfficiency( AIE_NORMAL );
		SetMoveEfficiency( AIME_NORMAL );
		return;
	}

	//---------------------------------

	CPlayer *pPlayer = UTIL_GetNearestPlayer(GetAbsOrigin());
	static Vector vPlayerEyePosition;
	static Vector vPlayerForward;
	static int iPrevFrame = -1;
	if ( gpGlobals->framecount != iPrevFrame )
	{
		iPrevFrame = gpGlobals->framecount;
		if ( pPlayer )
		{
			pPlayer->EyePositionAndVectors( &vPlayerEyePosition, &vPlayerForward, NULL, NULL );
		}
	}

	Vector	vToNPC		= GetAbsOrigin() - vPlayerEyePosition;
	float	playerDist	= VectorNormalize( vToNPC );
	bool	bPlayerFacing;

	bool	bClientPVSExpanded = UTIL_ClientPVSIsExpanded();

	if ( pPlayer )
	{
		bPlayerFacing = ( bClientPVSExpanded || ( bInPVS && vPlayerForward.Dot( vToNPC ) > 0 ) );
	}
	else
	{
		playerDist = 0;
		bPlayerFacing = true;
	}

	//---------------------------------

	bool bInVisibilityPVS = ( bClientPVSExpanded && UTIL_FindClientInVisibilityPVS( edict() ) != NULL );

	//---------------------------------

	if ( ( bInPVS && ( bPlayerFacing || playerDist < 25*12 ) ) || bClientPVSExpanded )
	{
		SetMoveEfficiency( AIME_NORMAL );
	}
	else
	{
		SetMoveEfficiency( AIME_EFFICIENT );
	}

	//---------------------------------

	if ( !IsRetail() && ai_efficiency_override->GetInt() > AIE_NORMAL && ai_efficiency_override->GetInt() <= AIE_DORMANT )
	{
		SetEfficiency( (AI_Efficiency_t)ai_efficiency_override->GetInt() );
		return;
	}

	//---------------------------------

	// Some conditions will always force normal
	if ( gpGlobals->curtime - GetLastAttackTime() < .15 )
	{
		SetEfficiency( AIE_NORMAL );
		return;
	}

	bool bFramerateOk = ( gpGlobals->frametime < ai_frametime_limit->GetFloat() );

	if ( m_bForceConditionsGather ||
		 gpGlobals->curtime - GetLastAttackTime() < .2 ||
		 gpGlobals->curtime - m_flLastDamageTime < .2 ||
		 ( GetState() < NPC_STATE_IDLE || GetState() > NPC_STATE_SCRIPT ) ||
		 ( ( bInPVS || bInVisibilityPVS ) &&
		   ( ( GetTask() && !TaskIsRunning() ) ||
			 GetTaskInterrupt() > 0 ||
			 m_bInChoreo ) ) )
	{
		SetEfficiency( ( bFramerateOk ) ? AIE_NORMAL : AIE_EFFICIENT );
		return;
	}

	AI_Efficiency_t minEfficiency;

	if ( !ShouldDefaultEfficient() )
	{
		minEfficiency = ( bFramerateOk ) ? AIE_NORMAL : AIE_EFFICIENT;
	}
	else
	{
		minEfficiency = ( bFramerateOk ) ? AIE_EFFICIENT : AIE_VERY_EFFICIENT;
	}

	// Stay normal if there's any chance of a relevant danger sound
	bool bPotentialDanger = false;

	if ( GetSoundInterests() & SOUND_DANGER )
	{
		int	iSound = CE_CSoundEnt::ActiveList();
		while ( iSound >= 0 && iSound < MAX_WORLD_SOUNDS_MP )
		{
			CSound *pCurrentSound = CE_CSoundEnt::SoundPointerForIndex( iSound );

			float hearingSensitivity = HearingSensitivity();
			Vector vEarPosition = EarPosition();

			if ( pCurrentSound && (SOUND_DANGER & pCurrentSound->SoundType()) )
			{
				float flHearDistanceSq = pCurrentSound->Volume() * hearingSensitivity;
				flHearDistanceSq *= flHearDistanceSq;
				if ( pCurrentSound->GetSoundOrigin().DistToSqr( vEarPosition ) <= flHearDistanceSq )
				{
					bPotentialDanger = true;
					break;
				}
			}
			iSound = pCurrentSound ? pCurrentSound->NextSound() : (int) SOUNDLIST_EMPTY;
		}
	}

	if ( bPotentialDanger )
	{
		SetEfficiency( minEfficiency );
		return;
	}

	//---------------------------------

	if ( !pPlayer )
	{
		// No heuristic currently for dedicated servers
		SetEfficiency( minEfficiency );
		return;
	}

	enum
	{
		DIST_NEAR,
		DIST_MID,
		DIST_FAR
	};

	int	range;
	if ( bInPVS )
	{
		if ( playerDist < 15*12 )
		{
			SetEfficiency( minEfficiency );
			return;
		}

		range = ( playerDist < 50*12 ) ? DIST_NEAR :
				( playerDist < 200*12 ) ? DIST_MID : DIST_FAR;
	}
	else
	{
		range = ( playerDist < 25*12 ) ? DIST_NEAR :
				( playerDist < 100*12 ) ? DIST_MID : DIST_FAR;
	}

	// Efficiency mappings
	int state = GetState();
	if (state == NPC_STATE_SCRIPT ) // Treat script as alert. Already confirmed not in PVS
		state = NPC_STATE_ALERT;

	static AI_Efficiency_t mappings[] =
			{
					// Idle
					// In PVS
					// Facing
					AIE_NORMAL,
					AIE_EFFICIENT,
					AIE_EFFICIENT,
					// Not facing
					AIE_EFFICIENT,
					AIE_EFFICIENT,
					AIE_VERY_EFFICIENT,
					// Not in PVS
					AIE_VERY_EFFICIENT,
					AIE_SUPER_EFFICIENT,
					AIE_SUPER_EFFICIENT,
					// Alert
					// In PVS
					// Facing
					AIE_NORMAL,
					AIE_EFFICIENT,
					AIE_EFFICIENT,
					// Not facing
					AIE_NORMAL,
					AIE_EFFICIENT,
					AIE_EFFICIENT,
					// Not in PVS
					AIE_EFFICIENT,
					AIE_VERY_EFFICIENT,
					AIE_SUPER_EFFICIENT,
					// Combat
					// In PVS
					// Facing
					AIE_NORMAL,
					AIE_NORMAL,
					AIE_EFFICIENT,
					// Not facing
					AIE_NORMAL,
					AIE_EFFICIENT,
					AIE_EFFICIENT,
					// Not in PVS
					AIE_NORMAL,
					AIE_EFFICIENT,
					AIE_VERY_EFFICIENT,
			};

	static const int stateBase[] = { 0, 9, 18 };
	const int NOT_FACING_OFFSET = 3;
	const int NO_PVS_OFFSET = 6;

	int iStateOffset = stateBase[state - NPC_STATE_IDLE] ;
	int iFacingOffset = (!bInPVS || bPlayerFacing) ? 0 : NOT_FACING_OFFSET;
	int iPVSOffset = (bInPVS) ? 0 : NO_PVS_OFFSET;
	int iMapping = iStateOffset + iPVSOffset + iFacingOffset + range;

	Assert( iMapping < ARRAYSIZE( mappings ) );

	AI_Efficiency_t efficiency = mappings[iMapping];

	//---------------------------------

	AI_Efficiency_t maxEfficiency = AIE_SUPER_EFFICIENT;
	if ( bInVisibilityPVS && state >= NPC_STATE_ALERT )
	{
		maxEfficiency = AIE_EFFICIENT;
	}
	else if ( bInVisibilityPVS || HasCondition( COND_SEE_PLAYER ) )
	{
		maxEfficiency = AIE_VERY_EFFICIENT;
	}

	//---------------------------------

	SetEfficiency( clamp( efficiency, minEfficiency, maxEfficiency ) );
}

CBaseEntity *CAI_NPC_AI_Patch::FindNamedEntity( const char *name, IEntityFindFilter *pFilter )
{
	if ( !stricmp( name, "!player" ))
	{
		CPlayer *player = UTIL_GetNearestPlayer(GetAbsOrigin());
		return (player)?player->BaseEntity():NULL;
	} else if ( !stricmp( name, "!nearestfriend" ) || !stricmp( name, "!friend" ) ) {
		CPlayer *player = UTIL_GetNearestPlayer(GetAbsOrigin());
		return (player)?player->BaseEntity():NULL;
	} else if ( !stricmp( name, "Player" )) {
		static int playerwarningcount = 0;
		if ( ++playerwarningcount < 5 )
		{
			DevMsg( "ERROR: \"player\" is no longer used, use \"!player\" in vcd instead!\n" );
		}

		CPlayer *player = UTIL_GetNearestPlayer(GetAbsOrigin());
		return (player)?player->BaseEntity():NULL;
	}

	return BaseClass::FindNamedEntity( name, pFilter );
}

bool CAI_NPC_AI_Patch::IsPlayerAlly( CBaseEntity *pPlayer )
{
	if ( pPlayer == NULL )
	{
		CPlayer *player = UTIL_GetNearestPlayer(GetAbsOrigin());
		pPlayer = (player)?player->BaseEntity():NULL;
	}

	return ( !pPlayer || IRelationType( pPlayer ) == D_LI );

}


void CAI_NPC_AI_Patch::CollectShotStats( const Vector &vecShootOrigin, const Vector &vecShootDir )
{
	if( ai_shot_stats->GetBool() != 0 && GetEnemy()->IsPlayer() )
	{
		int iterations = ai_shot_stats_term->GetInt();
		int iHits = 0;
		Vector testDir = vecShootDir;

		CShotManipulator manipulator( testDir );

		for( int i = 0 ; i < iterations ; i++ )
		{
			// Apply appropriate accuracy.
			manipulator.ApplySpread( GetAttackSpread( GetActiveWeapon()->BaseEntity(), GetEnemy_CBase() ), GetSpreadBias( GetActiveWeapon()->BaseEntity(), GetEnemy_CBase() ) );
			Vector shotDir = manipulator.GetResult();

			Vector vecEnd = vecShootOrigin + shotDir * 8192;

			trace_t tr;
			UTIL_TraceLine( vecShootOrigin, vecEnd, MASK_SHOT, BaseEntity(), COLLISION_GROUP_NONE, &tr);

			if( tr.m_pEnt && tr.m_pEnt == GetEnemy_CBase() )
			{
				iHits++;
			}
			Vector vecProjectedPosition = GetActualShootPosition( vecShootOrigin );
			Vector testDir = vecProjectedPosition - vecShootOrigin;
			VectorNormalize( testDir );
			manipulator.SetShootDir( testDir );
		}
	}
}

void CAI_NPC_AI_Patch::FireBullets( const FireBulletsInfo_t &info )
{
	// If we're shooting at a bullseye, become perfectly accurate if the bullseye demands it
	if ( GetEnemy() && GetEnemy()->Classify() == CLASS_BULLSEYE )
	{
		CNPC_Bullseye *pBullseye = dynamic_cast<CNPC_Bullseye*>(GetEnemy());
		if ( pBullseye && pBullseye->UsePerfectAccuracy() )
		{
			FireBulletsInfo_t accurateInfo = info;
			accurateInfo.m_vecSpread = vec3_origin;
			BaseClass::FireBullets( accurateInfo );
			return;
		}
	}
	BaseClass::FireBullets( info );
}

extern int g_interactionBarnacleVictimGrab;

bool CAI_NPC_AI_Patch::HandleInteraction( int interactionType, void *data, CBaseEntity* sourceEnt )
{
	if ( interactionType == g_interactionBarnacleVictimGrab )
	{
		// Make the victim stop thinking so they're as good as dead without
		// shocking the system by destroying the entity.
		StopLoopingSounds();
		BarnacleDeathSound();
		SetThink( NULL );

		// Gag the NPC so they won't talk anymore
		AddSpawnFlags( SF_NPC_GAG );

		// Drop any weapon they're holding
		if ( GetActiveWeapon() )
		{
			Weapon_Drop( GetActiveWeapon()->BaseEntity() );
		}

		return true;
	}
	return BaseClass::HandleInteraction(interactionType, data, sourceEnt);
}

CBaseEntity *CAI_NPC_AI_Patch::CreateCustomTarget( const Vector &vecOrigin, float duration)
{
	CNPC_Bullseye *pTarget = (CNPC_Bullseye*)CreateEntityByName( "npc_bullseye" );

	Assert( pTarget != NULL );

	// Build a nonsolid bullseye and place it in the desired location
	// The bullseye must take damage or the SetHealth 0 call will not be able
	pTarget->AddSpawnFlags( SF_BULLSEYE_NONSOLID );
	pTarget->SetAbsOrigin( vecOrigin );
	pTarget->Spawn();

	// Set it up to remove itself, unless told to be infinite (-1)
	if( duration > -1 )
	{
		variant_t value;
		value.SetFloat(0);
		g_CEventQueue->AddEvent( pTarget->BaseEntity(), "SetHealth", value, duration, BaseEntity(), BaseEntity() );
	}

	return pTarget->BaseEntity();
}


int CAI_NPC_AI_Patch::OnTakeDamage_Alive(const CTakeDamageInfo& info)
{
#if 0
	return BaseClass::OnTakeDamage_Alive(info);
#endif

	Forget( bits_MEMORY_INCOVER );

	if ( !CCombatCharacter::FAKE_OnTakeDamage_Alive( info ) )
		return 0;

	if ( GetSleepState() == AISS_WAITING_FOR_THREAT )
		Wake();

	CEntity *attacker = CEntity::Instance(info.GetAttacker());

	// NOTE: This must happen after the base class is called; we need to reduce
	// health before the pain sound, since some NPCs use the final health
	// level as a modifier to determine which pain sound to use.

	// REVISIT: Combine soldiers shoot each other a lot and then talk about it
	// this improves that case a bunch, but it seems kind of harsh.
	if ( !GetSquad() || !GetSquad()->SquadIsMember( attacker ) )
	{
		PainSound( info );// "Ouch!"
	}

	// See if we're running a dynamic interaction that should break when I am damaged.
	if ( IsActiveDynamicInteraction() )
	{
		ScriptedNPCInteraction_t *pInteraction = GetRunningDynamicInteraction();
		if ( pInteraction->iLoopBreakTriggerMethod & SNPCINT_LOOPBREAK_ON_DAMAGE )
		{
			CEAI_ScriptedSequence *_m_hCine = Get_m_hCine();
			// Can only break when we're in the action anim
			if ( _m_hCine->IsPlayingAction() )
			{
				_m_hCine->StopActionLoop( true );
			}
		}
	}

	// If we're not allowed to die, refuse to die
	// Allow my interaction partner to kill me though

	CAI_NPC *npc = m_hInteractionPartner->Get();
	if ( m_iHealth <= 0 && HasInteractionCantDie() && attacker != npc )
	{
		m_iHealth = 1;
	}

	// -----------------------------------
	//  Fire outputs
	// -----------------------------------
	if ( m_flLastDamageTime != gpGlobals->curtime )
	{
		// only fire once per frame
		m_OnDamaged->FireOutput( attacker, this);

		if( attacker && attacker->IsPlayer() )
		{
			m_OnDamagedByPlayer->FireOutput( attacker, this );

			// This also counts as being harmed by player's squad.
			m_OnDamagedByPlayerSquad->FireOutput( attacker, this );
		} else {
			// See if the person that injured me is an NPC.
			CAI_NPC *pAttacker = dynamic_cast<CAI_NPC *>( attacker );

			if( pAttacker && pAttacker->IsAlive() && UTIL_GetNearestPlayer(GetAbsOrigin()))
			{
				if( pAttacker->GetSquad() != NULL && pAttacker->IsInPlayerSquad() )
				{
					m_OnDamagedByPlayerSquad->FireOutput( attacker, this );
				}
			}
		}
	}

	if( (info.GetDamageType() & DMG_CRUSH) && !(info.GetDamageType() & DMG_PHYSGUN) && info.GetDamage() >= MIN_PHYSICS_FLINCH_DAMAGE )
	{
		SetCondition( COND_PHYSICS_DAMAGE );
	}

	if ( m_iHealth <= ( m_iMaxHealth / 2 ) )
	{
		m_OnHalfHealth->FireOutput( attacker, this );
	}

	// react to the damage (get mad)
	if ( ( (GetFlags() & FL_NPC) == 0 ) || !attacker )
		return 1;

	// If the attacker was an NPC or client update my position memory
	if ( attacker->GetFlags() & (FL_NPC | FL_CLIENT) )
	{
		// ------------------------------------------------------------------
		//				DO NOT CHANGE THIS CODE W/O CONSULTING
		// Only update information about my attacker I don't see my attacker
		// ------------------------------------------------------------------
		if ( !FInViewCone_Entity( info.GetAttacker() ) || !FVisible_Entity( info.GetAttacker() ) )
		{
			// -------------------------------------------------------------
			//  If I have an inflictor (enemy / grenade) update memory with
			//  position of inflictor, otherwise update with an position
			//  estimate for where the attack came from
			// ------------------------------------------------------
			Vector vAttackPos;
			CEntity *inflictor = CEntity::Instance(info.GetInflictor());
			if (inflictor)
			{
				vAttackPos = inflictor->GetAbsOrigin();
			}
			else
			{
				vAttackPos = (GetAbsOrigin() + ( *g_vecAttackDir * 64 ));
			}


			// ----------------------------------------------------------------
			//  If I already have an enemy, assume that the attack
			//  came from the enemy and update my enemy's position
			//  unless I already know about the attacker or I can see my enemy
			// ----------------------------------------------------------------
			if ( GetEnemy() != NULL							&&
				 !GetEnemies()->HasMemory( info.GetAttacker() )			&&
				 !HasCondition(COND_SEE_ENEMY)	)
			{
				UpdateEnemyMemory(GetEnemy_CBase(), vAttackPos, GetEnemy_CBase());
			}
				// ----------------------------------------------------------------
				//  If I already know about this enemy, update his position
				// ----------------------------------------------------------------
			else if (GetEnemies()->HasMemory( info.GetAttacker() ))
			{
				UpdateEnemyMemory(info.GetAttacker(), vAttackPos);
			}
				// -----------------------------------------------------------------
				//  Otherwise just note the position, but don't add enemy to my list
				// -----------------------------------------------------------------
			else
			{
				UpdateEnemyMemory(NULL, vAttackPos);
			}
		}

		// add pain to the conditions
		if ( IsLightDamage( info ) )
		{
			SetCondition( COND_LIGHT_DAMAGE );
		}
		if ( IsHeavyDamage( info ) )
		{
			SetCondition( COND_HEAVY_DAMAGE );
		}

		ForceGatherConditions();

		// Keep track of how much consecutive damage I have recieved
		if ((gpGlobals->curtime - m_flLastDamageTime) < 1.0)
		{
			m_flSumDamage += info.GetDamage();
		}
		else
		{
			m_flSumDamage = info.GetDamage();
		}
		m_flLastDamageTime = gpGlobals->curtime;
		if ( attacker && attacker->IsPlayer() )
			m_flLastPlayerDamageTime = gpGlobals->curtime;
		GetEnemies()->OnTookDamageFrom( info.GetAttacker() );

		if (m_flSumDamage > m_iMaxHealth*0.3)
		{
			SetCondition(COND_REPEATED_DAMAGE);
		}

		NotifyFriendsOfDamage( info.GetAttacker() );
	}

	// ---------------------------------------------------------------
	//  Insert a combat sound so that nearby NPCs know I've been hit
	// ---------------------------------------------------------------
	g_helpfunc.CSoundEnt_InsertSound(SOUND_COMBAT, GetAbsOrigin(), 1024, 0.5, BaseEntity(), SOUNDENT_CHANNEL_INJURY );

	return 1;
}
