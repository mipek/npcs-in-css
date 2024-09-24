
#include "weapon_stunstick.h"
#include "CE_recipientfilter.h"
#include "CAI_NPC.h"
#include "npc_combine.h"
#include "npc_metropolice.h"


static CEntityFactory_CE<CNPCWeapon_StunStick> WEAPON_STUNSTICK_REPLACE(WEAPON_STUNSTICK_REPLACE_NAME);


ConVar sk_crowbar_lead_time( "sk_crowbar_lead_time", "0.9" );

ConVar    sk_plr_dmg_stunstick	( "sk_plr_dmg_stunstick","0");
ConVar    sk_npc_dmg_stunstick	( "sk_npc_dmg_stunstick","0");

extern ConVar metropolice_move_and_melee;

acttable_t CNPCWeapon_StunStick::m_acttable[] =
		{
				{ ACT_MELEE_ATTACK1,	ACT_MELEE_ATTACK_SWING, true },
				{ ACT_IDLE,				ACT_IDLE_ANGRY_MELEE,	false },
				{ ACT_IDLE_ANGRY,		ACT_IDLE_ANGRY_MELEE,	false },
		};

void CNPCWeapon_StunStick::Spawn()
{
	m_iWeaponModel = PrecacheModel("models/weapons/w_stunbaton.mdl");
	BaseClass::Spawn();
}

void CNPCWeapon_StunStick::Precache()
{
	PrecacheScriptSound("Weapon_StunStick.Swing");
	PrecacheScriptSound("Weapon_StunStick.Melee_Miss");
	PrecacheScriptSound("Weapon_StunStick.Melee_Hit");
	PrecacheScriptSound("Weapon_StunStick.Melee_HitWorld");

	BaseClass::Precache();
}

void CNPCWeapon_StunStick::Drop( const Vector &vecVelocity )
{
	if(IsNPCUsing())
	{
		UTIL_Remove(this);
	} else {
		BaseClass::Drop(vecVelocity);
	}
}

const char *CNPCWeapon_StunStick::NPCWeaponGetWorldModel() const
{
	return "models/weapons/w_stunbaton.mdl";
}

acttable_t*	CNPCWeapon_StunStick::NPCWeaponActivityList()
{
	return m_acttable;
}

int	CNPCWeapon_StunStick::NPCWeaponActivityListCount()
{
	return ARRAYSIZE(m_acttable);
}

float CNPCWeapon_StunStick::GetDamageForActivity( Activity hitActivity )
{
	if ( ( GetOwner() != NULL ) && ( GetOwner()->IsPlayer() ) )
		return sk_plr_dmg_stunstick.GetFloat();

	return sk_npc_dmg_stunstick.GetFloat();
}

void CNPCWeapon_StunStick::NPCWeaponOperator_HandleAnimEvent( animevent_t *pEvent, CBaseEntity *pOperator )
{
	switch( pEvent->event )
	{
		case EVENT_WEAPON_MELEE_HIT:
		{
			// Trace up or down based on where the enemy is...
			// But only if we're basically facing that direction
			Vector vecDirection;
			AngleVectors( GetAbsAngles(), &vecDirection );

			CCombatCharacter *cent_pOperator = (CCombatCharacter *)CEntity::Instance(pOperator);

			CEntity *pEnemy = cent_pOperator->MyNPCPointer() ? cent_pOperator->MyNPCPointer()->GetEnemy() : NULL;
			if ( pEnemy )
			{
				Vector vecDelta;
				VectorSubtract( pEnemy->WorldSpaceCenter(), cent_pOperator->Weapon_ShootPosition(), vecDelta );
				VectorNormalize( vecDelta );

				Vector2D vecDelta2D = vecDelta.AsVector2D();
				Vector2DNormalize( vecDelta2D );
				if ( DotProduct2D( vecDelta2D, vecDirection.AsVector2D() ) > 0.8f )
				{
					vecDirection = vecDelta;
				}
			}

			Vector vecEnd;
			VectorMA( cent_pOperator->Weapon_ShootPosition(), 32, vecDirection, vecEnd );
			// Stretch the swing box down to catch low level physics objects
			CBaseEntity *pHurt_cbase = cent_pOperator->CheckTraceHullAttack_Vector( cent_pOperator->Weapon_ShootPosition(), vecEnd,
																					Vector(-16,-16,-40), Vector(16,16,16), (int)GetDamageForActivity( GetActivity() ), DMG_CLUB, 0.5f, false );

			Vector vecShootOrigin = cent_pOperator->Weapon_ShootPosition();
			CEntity *pHurt = CEntity::Instance(pHurt_cbase);
			// did I hit someone?
			if ( pHurt )
			{
				// play sound
				CustomWeaponSound(cent_pOperator->entindex(), vecShootOrigin, "Weapon_StunStick.Melee_Hit");
				g_helpfunc.CSoundEnt_InsertSound( SOUND_COMBAT|SOUND_CONTEXT_GUNFIRE, cent_pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2f, cent_pOperator->BaseEntity(), SOUNDENT_CHANNEL_WEAPON, cent_pOperator->CB_GetEnemy() );

				CPlayer *pPlayer = ToBasePlayer( pHurt );

				CNPC_MetroPolice *pCop = dynamic_cast<CNPC_MetroPolice *>(cent_pOperator);
				bool bFlashed = false;

				if ( pCop != NULL && pPlayer != NULL )
				{
					// See if we need to knock out this target
					if ( pCop->ShouldKnockOutTarget( pHurt ) )
					{
						float yawKick = enginerandom->RandomFloat( -48, -24 );

						//Kick the player angles
						pPlayer->ViewPunch( QAngle( -16, yawKick, 2 ) );

						color32 white = {255,255,255,255};
						UTIL_ScreenFade( pPlayer, white, 0.2f, 1.0f, FFADE_OUT|FFADE_PURGE|FFADE_STAYOUT );
						bFlashed = true;

						pCop->KnockOutTarget( pHurt );

						break;
					}
					else
					{
						// Notify that we've stunned a target
						pCop->StunnedTarget( pHurt );
					}
				}

				// Punch angles
				if ( pPlayer != NULL && !(pPlayer->GetFlags() & FL_GODMODE) )
				{
					float yawKick = enginerandom->RandomFloat( -48, -24 );

					//Kick the player angles
					pPlayer->ViewPunch( QAngle( -16, yawKick, 2 ) );

					Vector	dir = pHurt->GetAbsOrigin() - GetAbsOrigin();

					// If the player's on my head, don't knock him up
					if ( pPlayer->GetGroundEntity() == cent_pOperator )
					{
						dir = vecDirection;
						dir.z = 0;
					}

					VectorNormalize(dir);

					dir *= 500.0f;

					//If not on ground, then don't make them fly!
					if ( !(pPlayer->GetFlags() & FL_ONGROUND ) )
						dir.z = 0.0f;

					//Push the target back
					pHurt->ApplyAbsVelocityImpulse( dir );

					if ( !bFlashed )
					{
						color32 red = {128,0,0,128};
						UTIL_ScreenFade( pPlayer, red, 0.5f, 0.1f, FFADE_IN );
					}

					// Force the player to drop anyting they were holding
					pPlayer->ForceDropOfCarriedPhysObjects((CEntity *)NULL);
				}

				// do effect?
			}
			else
			{
				CustomWeaponSound(cent_pOperator->entindex(), vecShootOrigin, "Weapon_StunStick.Melee_Miss");
				g_helpfunc.CSoundEnt_InsertSound( SOUND_COMBAT|SOUND_CONTEXT_GUNFIRE, cent_pOperator->GetAbsOrigin(), SOUNDENT_VOLUME_MACHINEGUN, 0.2f, cent_pOperator->BaseEntity(), SOUNDENT_CHANNEL_WEAPON, cent_pOperator->CB_GetEnemy() );
			}
		}
			break;
		default:
			CCombatWeapon::Operator_HandleAnimEvent(pEvent, pOperator);
			break;
	}
}

int CNPCWeapon_StunStick::WeaponMeleeAttack1Condition( float flDot, float flDist )
{
	if(!IsNPCUsing())
		return BaseClass::WeaponMeleeAttack1Condition(flDot, flDist);


	// Attempt to lead the target (needed because citizens can't hit manhacks with the crowbar!)
	CAI_NPC *pNPC	= GetOwner()->MyNPCPointer();
	CEntity *pEnemy = pNPC->GetEnemy();
	if (!pEnemy)
		return COND_NONE;

	Vector vecVelocity;
	AngularImpulse angVelocity;
	pEnemy->GetVelocity( &vecVelocity, &angVelocity );

	// Project where the enemy will be in a little while, add some randomness so he doesn't always hit
	float dt = sk_crowbar_lead_time.GetFloat();
	dt += enginerandom->RandomFloat( -0.3f, 0.2f );
	if ( dt < 0.0f )
		dt = 0.0f;

	Vector vecExtrapolatedPos;
	VectorMA( pEnemy->WorldSpaceCenter(), dt, vecVelocity, vecExtrapolatedPos );

	Vector vecDelta;
	VectorSubtract( vecExtrapolatedPos, pNPC->WorldSpaceCenter(), vecDelta );

	if ( fabs( vecDelta.z ) > 70 )
	{
		return COND_TOO_FAR_TO_ATTACK;
	}

	Vector vecForward = pNPC->BodyDirection2D( );
	vecDelta.z = 0.0f;
	float flExtrapolatedDot = DotProduct2D( vecDelta.AsVector2D(), vecForward.AsVector2D() );
	if ((flDot < 0.7) && (flExtrapolatedDot < 0.7))
	{
		return COND_NOT_FACING_ATTACK;
	}

	float flExtrapolatedDist = Vector2DNormalize( vecDelta.AsVector2D() );

	if( pEnemy->IsPlayer() )
	{
		//Vector vecDir = pEnemy->GetSmoothedVelocity();
		//float flSpeed = VectorNormalize( vecDir );

		// If player will be in front of me in one-half second, clock his arse.
		Vector vecProjectEnemy = pEnemy->GetAbsOrigin() + (pEnemy->GetAbsVelocity() * 0.35);
		Vector vecProjectMe = GetAbsOrigin();

		if( (vecProjectMe - vecProjectEnemy).Length2D() <= 48.0f )
		{
			return COND_CAN_MELEE_ATTACK1;
		}
	}
/*
	if( metropolice_move_and_melee.GetBool() )
	{
		if( pNPC->IsMoving() )
		{
			flTargetDist *= 1.5f;
		}
	}
*/
	float flTargetDist = 48.0f;
	if ((flDist > flTargetDist) && (flExtrapolatedDist > flTargetDist))
	{
		return COND_TOO_FAR_TO_ATTACK;
	}

	return COND_CAN_MELEE_ATTACK1;
}


const WeaponProficiencyInfo_t *CNPCWeapon_StunStick::NPCWeaponGetProficiencyValues()
{
	return CCombatWeapon::GetProficiencyValues();
}

void CNPCWeapon_StunStick::NPCWeaponOperator_ForceNPCFire(CBaseEntity  *pOperator, bool bSecondary)
{
	return CCombatWeapon::Operator_ForceNPCFire(pOperator, bSecondary);
}


void CNPCWeapon_StunStick::SetStunState( bool state )
{
	m_bActive = state;

	if ( m_bActive )
	{
		//FIXME: START - Move to client-side

		Vector vecAttachment;
		QAngle vecAttachmentAngles;

		GetAttachment( 1, vecAttachment, vecAttachmentAngles );
		g_pEffects->Sparks( vecAttachment );

		//FIXME: END - Move to client-side

		EmitSound( "Weapon_StunStick.Activate" );
	}
	else
	{
		EmitSound( "Weapon_StunStick.Deactivate" );
	}
}

bool CNPCWeapon_StunStick::GetStunState( void )
{
	return m_bActive;
}