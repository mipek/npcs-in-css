
#include "CTrigger.h"
#include "touchlink.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

#define TRIGGER_HURT_FORGIVE_TIME	3.0f	// time in seconds

CE_LINK_ENTITY_TO_CLASS(CBaseTrigger, CTrigger);
CE_LINK_ENTITY_TO_CLASS(trigger_hurt, CTriggerHurt);


SH_DECL_MANUALHOOK1(PassesTriggerFilters, 0, 0, 0, bool, CBaseEntity *);
DECLARE_HOOK(PassesTriggerFilters, CTrigger);
DECLARE_DEFAULTHANDLER(CTrigger, PassesTriggerFilters, bool, (CBaseEntity *pOther), (pOther));



//Datamaps
DEFINE_PROP(m_hTouchingEntities, CTrigger);
DEFINE_PROP(m_bDisabled, CTrigger);

//Datamaps
DEFINE_PROP(m_flLastDmgTime, CTriggerHurt);
DEFINE_PROP(m_hurtEntities, CTriggerHurt);
DEFINE_PROP(m_damageModel, CTriggerHurt);
DEFINE_PROP(m_flDmgResetTime, CTriggerHurt);
DEFINE_PROP(m_flDamage, CTriggerHurt);
DEFINE_PROP(m_flOriginalDamage, CTriggerHurt);
DEFINE_PROP(m_flDamageCap, CTriggerHurt);
DEFINE_PROP(m_bitsDamageInflict, CTriggerHurt);
DEFINE_PROP(m_bNoDmgForce, CTriggerHurt);
DEFINE_PROP(m_OnHurtPlayer, CTriggerHurt);
DEFINE_PROP(m_OnHurt, CTriggerHurt);


bool CTrigger::IsTouching( CEntity *pOther )
{
	CFakeHandle hOther;
	hOther.Set((pOther)?pOther->BaseEntity():NULL);
	return ( m_hTouchingEntities->Find( hOther ) != m_hTouchingEntities->InvalidIndex() );
}

void CTrigger::Enable( void )
{
	m_bDisabled = false;

	if ( VPhysicsGetObject())
	{
		VPhysicsGetObject()->EnableCollisions( true );
	}

	if (!IsSolidFlagSet( FSOLID_TRIGGER ))
	{
		AddSolidFlags( FSOLID_TRIGGER ); 
		PhysicsTouchTriggers();
	}
}

void CTrigger::Disable( void )
{ 
	m_bDisabled = true;

	if ( VPhysicsGetObject())
	{
		VPhysicsGetObject()->EnableCollisions( false );
	}

	if (IsSolidFlagSet(FSOLID_TRIGGER))
	{
		RemoveSolidFlags( FSOLID_TRIGGER ); 
		PhysicsTouchTriggers();
	}
}

int CTriggerHurt::HurtAllTouchers( float dt )
{
	int hurtCount = 0;
	// half second worth of damage
	float fldmg = m_flDamage * dt;
	m_flLastDmgTime = gpGlobals->curtime;

	m_hurtEntities->RemoveAll();

	touchlink_t *root = ( touchlink_t * )GetDataObject( TOUCHLINK );
	if ( root )
	{
		for ( touchlink_t *link = root->nextLink; link != root; link = link->nextLink )
		{
			if(!link)
				break;

			CEntity *pTouch = CEntity::Instance(link->entityTouched);
			if ( pTouch )
			{
				if ( HurtEntity( pTouch, fldmg ) )
				{
					hurtCount++;
				}
			}
		}
	}

	if( m_damageModel == DAMAGEMODEL_DOUBLE_FORGIVENESS )
	{
		if( hurtCount == 0 )
		{
			if( gpGlobals->curtime > m_flDmgResetTime  )
			{
				// Didn't hurt anyone. Reset the damage if it's time. (hence, the forgiveness)
				m_flDamage = *(m_flOriginalDamage);
			}
		}
		else
		{
			// Hurt someone! double the damage
			m_flDamage *= 2.0f;

			if( *(m_flDamage) > *(m_flDamageCap) )
			{
				// Clamp
				m_flDamage = *(m_flDamageCap);
			}

			// Now, put the damage reset time into the future. The forgive time is how long the trigger
			// must go without harming anyone in order that its accumulated damage be reset to the amount
			// set by the level designer. This is a stop-gap for an exploit where players could hop through
			// slime and barely take any damage because the trigger would reset damage anytime there was no
			// one in the trigger when this function was called. (sjb)
			m_flDmgResetTime = gpGlobals->curtime + TRIGGER_HURT_FORGIVE_TIME;
		}
	}

	return hurtCount;
}

bool CTriggerHurt::HurtEntity( CEntity *pOther, float damage )
{
	if ( !pOther->m_takedamage || !PassesTriggerFilters(pOther->BaseEntity()) )
		return false;

	if ( damage < 0 )
	{
		pOther->TakeHealth( -damage, m_bitsDamageInflict );
	}
	else
	{
		// The damage position is the nearest point on the damaged entity
		// to the trigger's center. Not perfect, but better than nothing.
		Vector vecCenter = CollisionProp_Actual()->WorldSpaceCenter();

		Vector vecDamagePos;
		pOther->CollisionProp()->CalcNearestPoint( vecCenter, &vecDamagePos );

		CTakeDamageInfo info( BaseEntity(), BaseEntity(), damage, m_bitsDamageInflict );
		info.SetDamagePosition( vecDamagePos );
		if ( !m_bNoDmgForce )
		{
			GuessDamageForce( &info, ( vecDamagePos - vecCenter ), vecDamagePos );
		}
		else
		{
			info.SetDamageForce( vec3_origin );
		}

		pOther->TakeDamage( info );
	}

	if (pOther->IsPlayer())
	{
		m_OnHurtPlayer->FireOutput(pOther, this);
	}
	else
	{
		m_OnHurt->FireOutput(pOther, this);
	}
	m_hurtEntities->AddToTail( EHANDLE(pOther->BaseEntity()) );
	return true;
}