
#include "CCycler_Fix.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



#define	MIN_PHYSICS_FLINCH_DAMAGE	5.0f

void CE_Cycler_Fix::PostConstructor()
{
	BaseClass::PostConstructor();
	*(m_pfnThink) = NULL;
}

void CE_Cycler_Fix::Spawn(void)
{
	//according CBaseCombatCharacter
	SetBlocksLOS( false );
}

void CE_Cycler_Fix::Precache(void)
{
	g_helpfunc.CAI_BaseNPC_Precache(BaseEntity());

	// CE_TODO: only once?
	gm_iszPlayerSquad = AllocPooledString( PLAYER_SQUADNAME );
}

int CE_Cycler_Fix::ObjectCaps()
{
	int base = BaseClass::ObjectCaps();
	base &= ~FCAP_IMPULSE_USE;
	return base;
}

void CE_Cycler_Fix::Think(void)
{
	VALVE_BASEPTR original_think = m_pfnThink;
	if(original_think != NULL)
	{
		(BaseEntity()->*original_think)();
		return;
	}
}

int CE_Cycler_Fix::OnTakeDamage_Alive(const CTakeDamageInfo& info)
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

	if ( m_iHealth <= 0 && HasInteractionCantDie() && attacker != m_hInteractionPartner )
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

			if( pAttacker && pAttacker->IsAlive() )
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

int CE_Cycler_Fix::OnTakeDamage(const CTakeDamageInfo& info)
{
	return g_helpfunc.CBaseCombatCharacter_OnTakeDamage(BaseEntity(), info);
}

bool CE_Cycler_Fix::IsAlive(void)
{
	return (m_lifeState == LIFE_ALIVE); 
}

Disposition_t CE_Cycler_Fix::IRelationType ( CBaseEntity *pTarget )
{
	Disposition_t ss = BaseClass::IRelationType(pTarget);
	return ss;
}

void CE_Cycler_Fix::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	g_helpfunc.CBaseEntity_Use(BaseEntity(), pActivator, pCaller, useType, value);
}

