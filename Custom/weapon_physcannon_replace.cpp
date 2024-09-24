
#include "weapon_physcannon_replace.h"
#include "player_pickup.h"
#include "pickup.h"
#include "CBeam.h"
#include "CSprite.h"
#include "CE_recipientfilter.h"
#include "effect_dispatch_data.h"
#include "prop_combine_ball.h"
#include "pickup.h"


CE_LINK_ENTITY_TO_CLASS( weapon_c4, CWeaponPhysCannon );


extern CSoundEnvelopeController *g_SoundController;

bool CWeaponPhysCannon::IS_REPLACE_SPAWN = false;

ConVar physcannon_minforce( "physcannon_minforce", "700" );
ConVar physcannon_maxforce( "physcannon_maxforce", "1500" );
ConVar physcannon_maxmass( "physcannon_maxmass", "250" );
ConVar physcannon_tracelength( "physcannon_tracelength", "250" );
ConVar physcannon_mega_tracelength( "physcannon_mega_tracelength", "850" );
ConVar physcannon_chargetime("physcannon_chargetime", "2" );
ConVar physcannon_pullforce( "physcannon_pullforce", "4000" );
ConVar physcannon_mega_pullforce( "physcannon_mega_pullforce", "8000" );
ConVar physcannon_cone( "physcannon_cone", "0.97" );
ConVar physcannon_ball_cone( "physcannon_ball_cone", "0.997" );
ConVar physcannon_punt_cone( "physcannon_punt_cone", "0.997" );
ConVar player_throwforce( "player_throwforce", "1000" );
ConVar physcannon_dmg_glass( "physcannon_dmg_glass", "15" );
ConVar physcannon_right_turrets( "physcannon_right_turrets", "0" );


#define PHYSCANNON_BEAM_SPRITE "sprites/orangelight1.vmt"
#define PHYSCANNON_BEAM_SPRITE_NOZ "sprites/orangelight1_noz.vmt"
#define PHYSCANNON_GLOW_SPRITE "sprites/glow04_noz"
#define PHYSCANNON_ENDCAP_SPRITE "sprites/orangeflare1"
#define PHYSCANNON_CENTER_GLOW "sprites/orangecore1"
#define PHYSCANNON_BLAST_SPRITE "sprites/orangecore2"

int g_iModelPhysBeam;

BEGIN_DATADESC( CWeaponPhysCannon )
					DEFINE_KEYFIELD( m_bIsPhysCannon,		FIELD_BOOLEAN,	"mm_physcannon_replace" ),
END_DATADESC()


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
// this will hit skip the pass entity, but not anything it owns 
// (lets player grab own grenades)
class CTraceFilterNoOwnerTest : public CE_CTraceFilterSimple
{
public:
	DECLARE_CLASS( CTraceFilterNoOwnerTest, CE_CTraceFilterSimple );

	CTraceFilterNoOwnerTest( const IHandleEntity *passentity, int collisionGroup )
			: CE_CTraceFilterSimple( NULL, collisionGroup ), m_pPassNotOwner(passentity)
	{
	}

	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		if ( pHandleEntity != m_pPassNotOwner )
			return BaseClass::ShouldHitEntity( pHandleEntity, contentsMask );

		return false;
	}

protected:
	const IHandleEntity *m_pPassNotOwner;
};


CWeaponPhysCannon::CWeaponPhysCannon()
{
	m_bOpen					= false;
	m_nChangeState			= ELEMENT_STATE_NONE;
	m_flCheckSuppressTime	= 0.0f;
	m_EffectState			= (int)EFFECT_NONE;
	m_flLastDenySoundPlayed	= false;
}

void CWeaponPhysCannon::Precache()
{
	g_iModelPhysBeam = PrecacheModel("sprites/physbeam.vmt");
	//PrecacheModel( PHYSCANNON_BEAM_SPRITE );
	//PrecacheModel( PHYSCANNON_BEAM_SPRITE_NOZ );

	PrecacheScriptSound( "Weapon_PhysCannon.HoldSound" );

	PrecacheScriptSound("Weapon_PhysCannon.Launch");
	PrecacheScriptSound("Weapon_MegaPhysCannon.Launch");
	PrecacheScriptSound("Weapon_PhysCannon.Charge");
	PrecacheScriptSound("Weapon_MegaPhysCannon.Charge");
	PrecacheScriptSound("Weapon_PhysCannon.DryFire");
	PrecacheScriptSound("Weapon_MegaPhysCannon.DryFire");
	PrecacheScriptSound("Weapon_PhysCannon.Pickup");
	PrecacheScriptSound("Weapon_MegaPhysCannon.Pickup");
	PrecacheScriptSound("Weapon_PhysCannon.OpenClaws");
	PrecacheScriptSound("Weapon_PhysCannon.CloseClaws");
	PrecacheScriptSound("Weapon_PhysCannon.Drop");
	PrecacheScriptSound("Weapon_MegaPhysCannon.Drop");
	PrecacheScriptSound("Weapon_PhysCannon.HoldSound");
	PrecacheScriptSound("Weapon_MegaPhysCannon.HoldSound");
	PrecacheScriptSound("Weapon_MegaPhysCannon.ChargeZap");
	PrecacheScriptSound("Weapon_PhysCannon.TooHeavy");
	PrecacheScriptSound("Weapon_Physgun.On");
	PrecacheScriptSound("Weapon_Physgun.Off");
	PrecacheScriptSound("Weapon_Physgun.Special1");
	PrecacheScriptSound("Weapon_Physgun.LockedOn");
	PrecacheScriptSound("Weapon_Physgun.Scanning");
	PrecacheScriptSound("Weapon_Physgun.LightObject");
	PrecacheScriptSound("Weapon_Physgun.HeavyObject");


	m_iWeaponWorldModel = PrecacheModel("models/weapons/w_physics.mdl");
	m_iWeaponViewModel = PrecacheModel("models/weapons/v_physcannon.mdl");

	BaseClass::Precache();
}

void CWeaponPhysCannon::Spawn()
{
	if(CWeaponPhysCannon::IS_REPLACE_SPAWN)
	{
		SetIsPhysCannon(true);
		CWeaponPhysCannon::IS_REPLACE_SPAWN = false;
	}
	BaseClass::Spawn();
}

const char *CWeaponPhysCannon::GetWorldModel( void ) const
{
	if(IsPhysCannonReplace())
		return "models/weapons/w_physics.mdl";
	else
		return BaseClass::GetWorldModel();
}

void CWeaponPhysCannon::Equip( CBaseEntity *pOwner )
{
	BaseClass::Equip(pOwner);
	if(IsPhysCannonReplace())
	{
		m_iWorldModelIndex = m_iWeaponWorldModel;
		m_iViewModelIndex = m_iWeaponViewModel;
	}
}

const char *CWeaponPhysCannon::GetViewModel( int viewmodelindex ) const
{
	if(IsPhysCannonReplace())
	{
		return "models/weapons/v_physcannon.mdl";
	} else {
		return BaseClass::GetViewModel(viewmodelindex);
	}
}

void CWeaponPhysCannon::OnRestore()
{
	BaseClass::OnRestore();
	m_grabController.OnRestore();

	// Tracker 8106:  Physcannon effects disappear through level transition, so
	//  just recreate any effects here
	if ( m_EffectState != EFFECT_NONE )
	{
		DoEffect( m_EffectState, NULL );
	}
}

void CWeaponPhysCannon::UpdateOnRemove()
{
	DestroyEffects( );
	BaseClass::UpdateOnRemove();
}

inline float CWeaponPhysCannon::SpriteScaleFactor()
{
	return 1.0f;
}

bool CWeaponPhysCannon::Deploy()
{
	if(!IsPhysCannonReplace())
	{
		return BaseClass::Deploy();
	}


	CloseElements();
	DoEffect( EFFECT_READY );

	bool bReturn = BaseClass::Deploy();

	*(m_flNextSecondaryAttack) = m_flNextPrimaryAttack = gpGlobals->curtime;

	CPlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner )
	{
		pOwner->SetNextAttack( gpGlobals->curtime );
	}

	return bReturn;
}

void CWeaponPhysCannon::ForceDrop( void )
{
	CloseElements();
	DetachObject();
	StopEffects();
}

bool CWeaponPhysCannon::DropIfEntityHeld( CEntity *pTarget )
{
	if ( pTarget == NULL )
		return false;

	CEntity *pHeld = m_grabController.GetAttached();

	if ( pHeld == NULL )
		return false;

	if ( pHeld == pTarget )
	{
		ForceDrop();
		return true;
	}

	return false;
}

void CWeaponPhysCannon::Drop( const Vector &vecVelocity )
{
	if(!IsPhysCannonReplace())
	{
		BaseClass::Drop(vecVelocity);
		return;
	}

	ForceDrop();

	UTIL_Remove( this );
}

bool CWeaponPhysCannon::CanHolster()
{
	if(!IsPhysCannonReplace())
	{
		return BaseClass::CanHolster();
	}

	//Don't holster this weapon if we're holding onto something
	if ( m_bActive )
		return false;

	return BaseClass::CanHolster();
}

bool CWeaponPhysCannon::Holster( CBaseEntity *pSwitchingTo )
{
	if(!IsPhysCannonReplace())
	{
		return BaseClass::Holster(pSwitchingTo);
	}

	//Don't holster this weapon if we're holding onto something
	if ( m_bActive )
		return false;

	ForceDrop();
	DestroyEffects();

	return BaseClass::Holster( pSwitchingTo );
}

void CWeaponPhysCannon::DryFire( void )
{
	SendWeaponAnim( ACT_VM_PRIMARYATTACK );

	CustomWeaponSound(GetAbsOrigin(),"Weapon_PhysCannon.DryFire", EMPTY);
}

void CWeaponPhysCannon::Think()
{
	BaseClass::Think();
	if(IsPhysCannonReplace())
	{
		m_iWorldModelIndex = m_iWeaponWorldModel;
	}
}

void CWeaponPhysCannon::PrimaryFireEffect( void )
{
	CPlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
		return;

	pOwner->ViewPunch( QAngle(-6, enginerandom->RandomInt(-2,2) ,0) );

	color32 white = { 245, 245, 255, 32 };
	UTIL_ScreenFade( pOwner, white, 0.1f, 0.0f, FFADE_IN );

	CustomWeaponSound(GetAbsOrigin(), "Weapon_PhysCannon.Launch", SINGLE);
}


void CWeaponPhysCannon::PuntNonVPhysics( CEntity *pEntity, const Vector &forward, trace_t &tr )
{
	if ( m_hLastPuntedObject == pEntity && gpGlobals->curtime < m_flRepuntObjectTime )
		return;

	CTakeDamageInfo	info;

	CEntity *pOwner = GetOwner();

	info.SetAttacker( (pOwner)?pOwner->BaseEntity():NULL );
	info.SetInflictor( BaseEntity() );
	info.SetDamage( 1.0f );
	info.SetDamageType( DMG_CRUSH | DMG_PHYSGUN );
	info.SetDamageForce( forward );	// Scale?
	info.SetDamagePosition( tr.endpos );

	m_hLastPuntedObject.Set(pEntity->BaseEntity());
	m_flRepuntObjectTime = gpGlobals->curtime + 0.5f;

	pEntity->DispatchTraceAttack( info, forward, &tr );

	ApplyMultiDamage();

	//Explosion effect
	DoEffect( EFFECT_LAUNCH, &tr.endpos );

	PrimaryFireEffect();
	SendWeaponAnim( ACT_VM_SECONDARYATTACK );

	m_nChangeState = ELEMENT_STATE_CLOSED;
	m_flElementDebounce = gpGlobals->curtime + 0.5f;
	m_flCheckSuppressTime = gpGlobals->curtime + 0.25f;
}

void CWeaponPhysCannon::Physgun_OnPhysGunPickup( CEntity *pEntity, CPlayer *pOwner, PhysGunPickup_t reason )
{
	// If the target is debris, convert it to non-debris
	if ( pEntity->GetCollisionGroup() == COLLISION_GROUP_DEBRIS )
	{
		// Interactive debris converts back to debris when it comes to rest
		pEntity->SetCollisionGroup( COLLISION_GROUP_INTERACTIVE_DEBRIS );
	}

	Pickup_OnPhysGunPickup( pEntity, pOwner, reason );
}

void CWeaponPhysCannon::PuntVPhysics( CEntity *pEntity, const Vector &vecForward, trace_t &tr )
{
	CPlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( m_hLastPuntedObject == pEntity && gpGlobals->curtime < m_flRepuntObjectTime )
		return;

	m_hLastPuntedObject.Set(pEntity->BaseEntity());
	m_flRepuntObjectTime = gpGlobals->curtime + 0.5f;

	CTakeDamageInfo	info;

	Vector forward = vecForward;

	info.SetAttacker( (pOwner)?pOwner->BaseEntity():NULL );
	info.SetInflictor( BaseEntity() );
	info.SetDamage( 0.0f );
	info.SetDamageType( DMG_PHYSGUN );
	pEntity->DispatchTraceAttack( info, forward, &tr );
	ApplyMultiDamage();


	if ( Pickup_OnAttemptPhysGunPickup( pEntity, pOwner, PUNTED_BY_CANNON ) )
	{
		IPhysicsObject *pList[VPHYSICS_MAX_OBJECT_LIST_COUNT];
		int listCount = pEntity->VPhysicsGetObjectList( pList, ARRAYSIZE(pList) );
		if ( !listCount )
		{
			//FIXME: Do we want to do this if there's no physics object?
			Physgun_OnPhysGunPickup( pEntity, pOwner, PUNTED_BY_CANNON );
			DryFire();
			return;
		}

		if( forward.z < 0 )
		{
			//reflect, but flatten the trajectory out a bit so it's easier to hit standing targets
			forward.z *= -0.65f;
		}

		// NOTE: Do this first to enable motion (if disabled) - so forces will work
		// Tell the object it's been punted
		Physgun_OnPhysGunPickup( pEntity, pOwner, PUNTED_BY_CANNON );

		// don't push vehicles that are attached to the world via fixed constraints
		// they will just wiggle...
		if ( (pList[0]->GetGameFlags() & FVPHYSICS_CONSTRAINT_STATIC) && pEntity->GetServerVehicle() )
		{
			forward.Init();
		}

		if ( !Pickup_ShouldPuntUseLaunchForces( pEntity, PHYSGUN_FORCE_PUNTED ) )
		{
			int i;

			// limit mass to avoid punting REALLY huge things
			float totalMass = 0;
			for ( i = 0; i < listCount; i++ )
			{
				totalMass += pList[i]->GetMass();
			}
			float maxMass = 250;
			IServerVehicle *pVehicle = pEntity->GetServerVehicle();
			if ( pVehicle )
			{
				maxMass *= 2.5;	// 625 for vehicles
			}
			float mass = MIN(totalMass, maxMass); // max 250kg of additional force

			// Put some spin on the object
			for ( i = 0; i < listCount; i++ )
			{
				const float hitObjectFactor = 0.5f;
				const float otherObjectFactor = 1.0f - hitObjectFactor;
				// Must be light enough
				float ratio = pList[i]->GetMass() / totalMass;
				if ( pList[i] == pEntity->VPhysicsGetObject() )
				{
					ratio += hitObjectFactor;
					ratio = MIN(ratio,1.0f);
				}
				else
				{
					ratio *= otherObjectFactor;
				}
				pList[i]->ApplyForceCenter( forward * 15000.0f * ratio );
				pList[i]->ApplyForceOffset( forward * mass * 600.0f * ratio, tr.endpos );
			}
		}
		else
		{
			ApplyVelocityBasedForce( pEntity, vecForward );
		}
	}


	// Add recoil
	QAngle	recoil = QAngle( enginerandom->RandomFloat( 1.0f, 2.0f ), enginerandom->RandomFloat( -1.0f, 1.0f ), 0 );
	pOwner->ViewPunch( recoil );

	//Explosion effect
	DoEffect( EFFECT_LAUNCH, &tr.endpos );

	PrimaryFireEffect();
	SendWeaponAnim( ACT_VM_SECONDARYATTACK );

	m_nChangeState = ELEMENT_STATE_CLOSED;
	m_flElementDebounce = gpGlobals->curtime + 0.5f;
	m_flCheckSuppressTime = gpGlobals->curtime + 0.25f;

	// Don't allow the gun to regrab a thrown object!!
	*(m_flNextSecondaryAttack) = m_flNextPrimaryAttack = gpGlobals->curtime + 0.5f;
}


void CWeaponPhysCannon::ApplyVelocityBasedForce( CEntity *pEntity, const Vector &forward )
{
	IPhysicsObject *pPhysicsObject = pEntity->VPhysicsGetObject();
	Assert(pPhysicsObject); // Shouldn't ever get here with a non-vphysics object.
	if (!pPhysicsObject)
		return;

	float flForceMax = physcannon_maxforce.GetFloat();
	float flForce = flForceMax;

	float mass = pPhysicsObject->GetMass();
	if (mass > 100)
	{
		mass = MIN(mass, 1000);
		float flForceMin = physcannon_minforce.GetFloat();
		flForce = SimpleSplineRemapVal(mass, 100, 600, flForceMax, flForceMin);
	}

	Vector vVel = forward * flForce;
	// FIXME: Josh needs to put a real value in for PHYSGUN_FORCE_PUNTED
	AngularImpulse aVel = Pickup_PhysGunLaunchAngularImpulse( pEntity, PHYSGUN_FORCE_PUNTED );

	pPhysicsObject->AddVelocity( &vVel, &aVel );

}

float CWeaponPhysCannon::TraceLength()
{
	return physcannon_tracelength.GetFloat();
}





void CWeaponPhysCannon::PrimaryAttack()
{
	if(!IsPhysCannonReplace())
	{
		BaseClass::PrimaryAttack();
		return;
	}


	if( m_flNextPrimaryAttack > gpGlobals->curtime )
		return;

	CPlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
		return;

	if( m_bActive )
	{
		// Punch the object being held!!
		Vector forward;
		pOwner->EyeVectors( &forward );

		// Validate the item is within punt range
		CEntity *pHeld = m_grabController.GetAttached();
		Assert( pHeld != NULL );

		if ( pHeld != NULL )
		{
			float heldDist = ( pHeld->WorldSpaceCenter() - pOwner->WorldSpaceCenter() ).Length();

			if ( heldDist > physcannon_tracelength.GetFloat() )
			{
				// We can't punt this yet
				DryFire();
				return;
			}
		}

		LaunchObject( forward, physcannon_maxforce.GetFloat() );

		PrimaryFireEffect();
		SendWeaponAnim( ACT_VM_SECONDARYATTACK );
		return;
	}

	// If not active, just issue a physics punch in the world.
	m_flNextPrimaryAttack = gpGlobals->curtime + 0.5f;

	Vector forward;
	pOwner->EyeVectors( &forward );

	// NOTE: Notice we're *not* using the mega tracelength here
	// when you have the mega cannon. Punting has shorter range.
	Vector start, end;
	start = pOwner->Weapon_ShootPosition();
	float flPuntDistance = physcannon_tracelength.GetFloat();
	VectorMA( start, flPuntDistance, forward, end );

	CTraceFilterNoOwnerTest filter( pOwner->BaseEntity(), COLLISION_GROUP_NONE );
	trace_t tr;
	UTIL_TraceHull( start, end, -Vector(8,8,8), Vector(8,8,8), MASK_SHOT|CONTENTS_GRATE, &filter, &tr );
	bool bValid = true;
	CEntity *pEntity = CEntity::Instance(tr.m_pEnt);
	if ( tr.fraction == 1 || !pEntity || pEntity->IsEFlagSet( EFL_NO_PHYSCANNON_INTERACTION ) )
	{
		bValid = false;
	}
	else if ( (pEntity->GetMoveType() != MOVETYPE_VPHYSICS) && ( pEntity->m_takedamage == DAMAGE_NO ) )
	{
		bValid = false;
	}

	// If the entity we've hit is invalid, try a traceline instead
	if ( !bValid )
	{
		UTIL_TraceLine( start, end, MASK_SHOT|CONTENTS_GRATE, &filter, &tr );
		pEntity = CEntity::Instance(tr.m_pEnt);
		if ( tr.fraction == 1 || !pEntity || pEntity->IsEFlagSet( EFL_NO_PHYSCANNON_INTERACTION ) )
		{
			// Play dry-fire sequence
			DryFire();
			return;
		}

		pEntity = CEntity::Instance(tr.m_pEnt);
	}

	// See if we hit something
	if ( pEntity->GetMoveType() != MOVETYPE_VPHYSICS )
	{
		if ( pEntity->m_takedamage == DAMAGE_NO )
		{
			DryFire();
			return;
		}

		if( GetOwner()->IsPlayer() )
		{
			// Don't let the player zap any NPC's except regular antlions and headcrabs.
			if( pEntity->IsPlayer() )
			{
				DryFire();
				return;
			}
		}

		PuntNonVPhysics( pEntity, forward, tr );
	}
	else
	{
		if ( pEntity->VPhysicsIsFlesh( ) )
		{
			DryFire();
			return;
		}
		PuntVPhysics( pEntity, forward, tr );
	}
}

void CWeaponPhysCannon::SecondaryAttack()
{
	if(!IsPhysCannonReplace())
	{
		BaseClass::SecondaryAttack();
		return;
	}

	if ( m_flNextSecondaryAttack > gpGlobals->curtime )
		return;

	CPlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
		return;

	// See if we should drop a held item
	if ( ( m_bActive ) && ( pOwner->m_afButtonPressed & IN_ATTACK2 ) )
	{
		// Drop the held object
		m_flNextPrimaryAttack = gpGlobals->curtime + 0.5;
		m_flNextSecondaryAttack = gpGlobals->curtime + 0.5;

		DetachObject();

		DoEffect( EFFECT_READY );

		SendWeaponAnim( ACT_VM_PRIMARYATTACK );
	}
	else
	{
		// Otherwise pick it up
		FindObjectResult_t result = FindObject();
		switch ( result )
		{
			case OBJECT_FOUND:
				CustomWeaponSound(GetAbsOrigin(), "Weapon_PhysCannon.Pickup", SPECIAL1);
				SendWeaponAnim( ACT_VM_PRIMARYATTACK );
				m_flNextSecondaryAttack = gpGlobals->curtime + 0.5f;

				// We found an object. Debounce the button
				m_nAttack2Debounce |= pOwner->m_nButtons;
				break;

			case OBJECT_NOT_FOUND:
				m_flNextSecondaryAttack = gpGlobals->curtime + 0.1f;
				CloseElements();
				break;

			case OBJECT_BEING_DETACHED:
				m_flNextSecondaryAttack = gpGlobals->curtime + 0.01f;
				break;
		}

		DoEffect( EFFECT_HOLDING );
	}
}




bool CWeaponPhysCannon::AttachObject( CEntity *pObject, const Vector &vPosition )
{

	if ( m_bActive )
		return false;

	if ( CanPickupObject( pObject ) == false )
		return false;

	m_grabController.SetIgnorePitch( false );
	m_grabController.SetAngleAlignment( 0 );

	IPhysicsObject *pPhysics = pObject->VPhysicsGetObject();

	// Must be valid
	if ( !pPhysics )
		return false;

	CPlayer *pOwner = (CPlayer *)ToBasePlayer( GetOwner() );

	m_bActive = true;
	if( pOwner )
	{
		// NOTE: This can change the mass; so it must be done before max speed setting
		Physgun_OnPhysGunPickup( pObject, pOwner, PICKED_UP_BY_CANNON );
	}

	// NOTE :This must happen after OnPhysGunPickup because that can change the mass
	m_grabController.AttachEntity( pOwner, pObject, pPhysics, false, vPosition, false );
	m_hAttachedObject.Set(pObject->BaseEntity());
	m_attachedPositionObjectSpace = m_grabController.m_attachedPositionObjectSpace;
	m_attachedAnglesPlayerSpace = m_grabController.m_attachedAnglesPlayerSpace;

	m_bResetOwnerEntity = false;

	if ( m_hAttachedObject->GetOwnerEntity() == NULL )
	{
		m_hAttachedObject->SetOwnerEntity( (pOwner)?pOwner->BaseEntity():NULL );
		m_bResetOwnerEntity = true;
	}

/*	if( pOwner )
	{
		pOwner->EnableSprint( false );

		float	loadWeight = ( 1.0f - GetLoadPercentage() );
		float	maxSpeed = hl2_walkspeed.GetFloat() + ( ( hl2_normspeed.GetFloat() - hl2_walkspeed.GetFloat() ) * loadWeight );

		//Msg( "Load perc: %f -- Movement speed: %f/%f\n", loadWeight, maxSpeed, hl2_normspeed.GetFloat() );
		pOwner->SetMaxSpeed( maxSpeed );
	}*/

	// Don't drop again for a slight delay, in case they were pulling objects near them
	m_flNextSecondaryAttack = gpGlobals->curtime + 0.4f;

	DoEffect( EFFECT_HOLDING );
	OpenElements();

	if ( GetMotorSound() )
	{
		g_SoundController->Play( GetMotorSound(), 0.0f, 50 );
		g_SoundController->SoundChangePitch( GetMotorSound(), 100, 0.5f );
		g_SoundController->SoundChangeVolume( GetMotorSound(), 0.8f, 0.5f );
	}
	return true;
}


CWeaponPhysCannon::FindObjectResult_t CWeaponPhysCannon::FindObject( void )
{
	CPlayer *pPlayer = ToBasePlayer( GetOwner() );

	Assert( pPlayer );
	if ( pPlayer == NULL )
		return OBJECT_NOT_FOUND;

	Vector forward;
	pPlayer->EyeVectors( &forward );

	// Setup our positions
	Vector	start = pPlayer->Weapon_ShootPosition();
	float	testLength = TraceLength() * 4.0f;
	Vector	end = start + forward * testLength;

	// Try to find an object by looking straight ahead
	trace_t tr;
	CTraceFilterNoOwnerTest filter( pPlayer->BaseEntity(), COLLISION_GROUP_NONE );
	UTIL_TraceLine( start, end, MASK_SHOT|CONTENTS_GRATE, &filter, &tr );

	// Try again with a hull trace
	CEntity *cent = CEntity::Instance(tr.m_pEnt);
	if ( ( tr.fraction == 1.0 ) || ( cent == NULL ) || ( cent->IsWorld() ) )
	{
		UTIL_TraceHull( start, end, -Vector(4,4,4), Vector(4,4,4), MASK_SHOT|CONTENTS_GRATE, &filter, &tr );
	}

	cent = CEntity::Instance(tr.m_pEnt);
	CEntity *pEntity = (cent) ? cent->GetRootMoveParent() : NULL;

	bool	bAttach = false;
	bool	bPull = false;

	// If we hit something, pick it up or pull it
	if ( ( tr.fraction != 1.0f ) && ( cent ) && ( cent->IsWorld() == false ) )
	{
		// Attempt to attach if within range
		if ( tr.fraction <= 0.25f )
		{
			bAttach = true;
		}
		else if ( tr.fraction > 0.25f )
		{
			bPull = true;
		}
	}

	// Find anything within a general cone in front
	CEntity *pConeEntity = NULL;

	if (!bAttach && !bPull)
	{
		pConeEntity = FindObjectInCone( start, forward, physcannon_cone.GetFloat() );
	}

	if ( pConeEntity )
	{
		pEntity = pConeEntity;

		// If the object is near, grab it. Else, pull it a bit.
		if ( pEntity->WorldSpaceCenter().DistToSqr( start ) <= (testLength * testLength) )
		{
			bAttach = true;
		}
		else
		{
			bPull = true;
		}
	}

	if ( CanPickupObject( pEntity ) == false )
	{
		// Make a noise to signify we can't pick this up
		if ( !m_flLastDenySoundPlayed )
		{
			m_flLastDenySoundPlayed = true;

			CustomWeaponSound(GetAbsOrigin(), "Weapon_PhysCannon.TooHeavy", SPECIAL3);
		}

		return OBJECT_NOT_FOUND;
	}

	// Check to see if the object is constrained + needs to be ripped off...
	CPlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( !Pickup_OnAttemptPhysGunPickup( pEntity, pOwner, PICKED_UP_BY_CANNON ) )
		return OBJECT_BEING_DETACHED;

	if ( bAttach )
	{
		return AttachObject( pEntity, tr.endpos ) ? OBJECT_FOUND : OBJECT_NOT_FOUND;
	}

	if ( !bPull )
		return OBJECT_NOT_FOUND;

	// FIXME: This needs to be run through the CanPickupObject logic
	IPhysicsObject *pObj = pEntity->VPhysicsGetObject();
	if ( !pObj )
		return OBJECT_NOT_FOUND;

	// If we're too far, simply start to pull the object towards us
	Vector	pullDir = start - pEntity->WorldSpaceCenter();
	VectorNormalize( pullDir );
	pullDir *= physcannon_pullforce.GetFloat();

	float mass = PhysGetEntityMass( pEntity );
	if ( mass < 50.0f )
	{
		pullDir *= (mass + 0.5) * (1/50.0f);
	}

	// Nudge it towards us
	pObj->ApplyForceCenter( pullDir );
	return OBJECT_NOT_FOUND;
}

CEntity *CWeaponPhysCannon::FindObjectInCone( const Vector &vecOrigin, const Vector &vecDir, float flCone )
{
	// Find the nearest physics-based item in a cone in front of me.
	CEntity *list[256];
	float flNearestDist = TraceLength() + 1.0;
	Vector mins = vecOrigin - Vector( flNearestDist, flNearestDist, flNearestDist );
	Vector maxs = vecOrigin + Vector( flNearestDist, flNearestDist, flNearestDist );

	CEntity *pNearest = NULL;

	int count = UTIL_EntitiesInBox( list, 256, mins, maxs, 0 );
	for( int i = 0 ; i < count ; i++ )
	{
		CEntity *cent = list[ i ];
		if ( !cent->VPhysicsGetObject() )
			continue;

		// Closer than other objects
		Vector los = ( cent->WorldSpaceCenter() - vecOrigin );
		float flDist = VectorNormalize( los );
		if( flDist >= flNearestDist )
			continue;

		// Cull to the cone
		if ( DotProduct( los, vecDir ) <= flCone )
			continue;

		CEntity *owner = GetOwner();
		// Make sure it isn't occluded!
		trace_t tr;
		CTraceFilterNoOwnerTest filter( (owner)?owner->BaseEntity():NULL, COLLISION_GROUP_NONE );
		UTIL_TraceLine( vecOrigin, cent->WorldSpaceCenter(), MASK_SHOT|CONTENTS_GRATE, &filter, &tr );
		if( tr.m_pEnt == cent->BaseEntity() )
		{
			flNearestDist = flDist;
			pNearest = cent;
		}
	}

	return pNearest;
}


void CWeaponPhysCannon::UpdateObject( void )
{
	CPlayer *pPlayer = ToBasePlayer( GetOwner() );
	Assert( pPlayer );

	float flError = 12;
	if ( !m_grabController.UpdateObject( pPlayer, flError ) )
	{
		DetachObject();
		return;
	}
}


void CWeaponPhysCannon::DetachObject( bool playSound, bool wasLaunched )
{
	if ( m_bActive == false )
		return;

	CPlayer *pOwner = ToBasePlayer( GetOwner() );

	CEntity *pObject = m_grabController.GetAttached();

	m_grabController.DetachEntity( wasLaunched );

	if ( pObject != NULL )
	{
		Pickup_OnPhysGunDrop( pObject, pOwner, wasLaunched ? LAUNCHED_BY_CANNON : DROPPED_BY_CANNON );
	}

	// Stop our looping sound
	if ( GetMotorSound() )
	{
		g_SoundController->SoundChangeVolume( GetMotorSound(), 0.0f, 1.0f );
		g_SoundController->SoundChangePitch( GetMotorSound(), 50, 1.0f );
	}

	if ( pObject && m_bResetOwnerEntity == true )
	{
		pObject->SetOwnerEntity( NULL );
	}

	m_bActive = false;
	m_hAttachedObject.Set(NULL);


	if ( playSound )
	{
		//Play the detach sound
		CustomWeaponSound(GetAbsOrigin(), "Weapon_PhysCannon.Drop", MELEE_MISS);
	}
}

void CWeaponPhysCannon::ItemPreFrame()
{
	BaseClass::ItemPreFrame();

	if(!IsPhysCannonReplace())
		return;

	// Update the object if the weapon is switched on.
	if( m_bActive )
	{
		UpdateObject();
	}
}


void CWeaponPhysCannon::CheckForTarget( void )
{
	//See if we're suppressing this
	if ( m_flCheckSuppressTime > gpGlobals->curtime )
		return;

	// holstered
	if ( IsEffectActive( EF_NODRAW ) )
		return;

	CPlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
		return;

	if ( m_bActive )
		return;

	Vector	aimDir;
	pOwner->EyeVectors( &aimDir );

	Vector	startPos	= pOwner->Weapon_ShootPosition();
	Vector	endPos;
	VectorMA( startPos, TraceLength(), aimDir, endPos );

	trace_t	tr;
	UTIL_TraceHull( startPos, endPos, -Vector(4,4,4), Vector(4,4,4), MASK_SHOT|CONTENTS_GRATE, pOwner->BaseEntity(), COLLISION_GROUP_NONE, &tr );

	if ( ( tr.fraction != 1.0f ) && ( tr.m_pEnt != NULL ) )
	{
		CEntity *cent = CEntity::Instance(tr.m_pEnt);
		// FIXME: Try just having the elements always open when pointed at a physics object
		if ( CanPickupObject( cent ) || Pickup_ForcePhysGunOpen( cent, pOwner ) )
			// if ( ( tr.m_pEnt->VPhysicsGetObject() != NULL ) && ( tr.m_pEnt->GetMoveType() == MOVETYPE_VPHYSICS ) )
		{
			m_nChangeState = ELEMENT_STATE_NONE;
			OpenElements();
			return;
		}
	}

	// Close the elements after a delay to prevent overact state switching
	if ( ( m_flElementDebounce < gpGlobals->curtime ) && ( m_nChangeState == ELEMENT_STATE_NONE ) )
	{
		m_nChangeState = ELEMENT_STATE_CLOSED;
		m_flElementDebounce = gpGlobals->curtime + 0.5f;
	}
}

void CWeaponPhysCannon::DoEffectIdle( void )
{

}


void CWeaponPhysCannon::ItemPostFrame()
{
	if(!IsPhysCannonReplace())
	{
		BaseClass::ItemPostFrame();
		return;
	}

	CPlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner == NULL )
	{
		// We found an object. Debounce the button
		m_nAttack2Debounce = 0;
		return;
	}

	//Check for object in pickup range
	if ( m_bActive == false )
	{
		CheckForTarget();

		if ( ( m_flElementDebounce < gpGlobals->curtime ) && ( m_nChangeState != ELEMENT_STATE_NONE ) )
		{
			if ( m_nChangeState == ELEMENT_STATE_OPEN )
			{
				OpenElements();
			}
			else if ( m_nChangeState == ELEMENT_STATE_CLOSED )
			{
				CloseElements();
			}

			m_nChangeState = ELEMENT_STATE_NONE;
		}
	}

	// NOTE: Attack2 will be considered to be pressed until the first item is picked up.
	int nAttack2Mask = pOwner->m_nButtons & (~m_nAttack2Debounce);
	if ( nAttack2Mask & IN_ATTACK2 )
	{
		SecondaryAttack();
	}
	else
	{
		// Reset our debouncer
		m_flLastDenySoundPlayed = false;

		if ( m_bActive == false )
		{
			DoEffect( EFFECT_READY );
		}
	}

	if (( pOwner->m_nButtons & IN_ATTACK2 ) == 0 )
	{
		m_nAttack2Debounce = 0;
	}

	if ( pOwner->m_nButtons & IN_ATTACK )
	{
		PrimaryAttack();
	}
	else
	{
		WeaponIdle();
	}
}


#define PHYSCANNON_DANGER_SOUND_RADIUS 128

void CWeaponPhysCannon::LaunchObject( const Vector &vecDir, float flForce )
{
	CEntity *pObject = m_grabController.GetAttached();

	if ( !(m_hLastPuntedObject == pObject && gpGlobals->curtime < m_flRepuntObjectTime) )
	{
		// FIRE!!!
		if( pObject != NULL )
		{
			DetachObject( false, true );

			m_hLastPuntedObject.Set(pObject->BaseEntity());
			m_flRepuntObjectTime = gpGlobals->curtime + 0.5f;

			// Launch
			ApplyVelocityBasedForce( pObject, vecDir );

			// Don't allow the gun to regrab a thrown object!!
			*(m_flNextSecondaryAttack) = m_flNextPrimaryAttack = gpGlobals->curtime + 0.5;

			Vector	center = pObject->WorldSpaceCenter();

			//Do repulse effect
			DoEffect( EFFECT_LAUNCH, &center );

			m_hAttachedObject.Set(NULL);
			m_bActive = false;
		}
	}

	// Stop our looping sound
	if ( GetMotorSound() )
	{
		g_SoundController->SoundChangeVolume( GetMotorSound(), 0.0f, 1.0f );
		g_SoundController->SoundChangePitch( GetMotorSound(), 50, 1.0f );
	}

	//Close the elements and suppress checking for a bit
	m_nChangeState = ELEMENT_STATE_CLOSED;
	m_flElementDebounce = gpGlobals->curtime + 0.1f;
	m_flCheckSuppressTime = gpGlobals->curtime + 0.25f;
}


bool CWeaponPhysCannon::CanPickupObject( CEntity *pTarget )
{
	if ( pTarget == NULL )
		return false;

	if ( pTarget->GetBaseAnimating() && pTarget->GetBaseAnimating()->IsDissolving() )
		return false;

	if ( pTarget->IsEFlagSet( EFL_NO_PHYSCANNON_INTERACTION ) )
		return false;

	CPlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner && pOwner->GetGroundEntity() == pTarget )
		return false;

	if ( pTarget->VPhysicsIsFlesh( ) )
		return false;

	IPhysicsObject *pObj = pTarget->VPhysicsGetObject();

	if ( pObj && pObj->GetGameFlags() & FVPHYSICS_PLAYER_HELD )
		return false;

	if ( UTIL_IsCombineBall( pTarget ) )
	{
		return CPlayer::CanPickupObject( pTarget, 0, 0 );
	}

	return CPlayer::CanPickupObject( pTarget, physcannon_maxmass.GetFloat(), 0 );

}

void CWeaponPhysCannon::OpenElements( void )
{
	if ( m_bOpen )
		return;

	CustomWeaponSound(GetAbsOrigin(), "Weapon_PhysCannon.OpenClaws", SPECIAL2);

	CPlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
		return;

	SendWeaponAnim( ACT_VM_IDLE );

	m_bOpen = true;

	DoEffect( EFFECT_READY );
}



void CWeaponPhysCannon::CloseElements( void )
{
	if ( m_bOpen == false )
		return;

	CustomWeaponSound(GetAbsOrigin(), "Weapon_PhysCannon.CloseClaws", MELEE_HIT);

	CPlayer *pOwner = ToBasePlayer( GetOwner() );

	if ( pOwner == NULL )
		return;

	SendWeaponAnim( ACT_VM_IDLE );

	m_bOpen = false;

	if ( GetMotorSound() )
	{
		g_SoundController->SoundChangeVolume( GetMotorSound(), 0.0f, 1.0f );
		g_SoundController->SoundChangePitch( GetMotorSound(), 50, 1.0f );
	}

	DoEffect( EFFECT_CLOSED );
}

#define	PHYSCANNON_MAX_MASS		500


float CWeaponPhysCannon::GetLoadPercentage( void )
{
	float loadWeight = m_grabController.GetLoadWeight();
	loadWeight /= physcannon_maxmass.GetFloat();
	loadWeight = clamp( loadWeight, 0.0f, 1.0f );
	return loadWeight;
}

CSoundPatch *CWeaponPhysCannon::GetMotorSound( void )
{
	if ( m_sndMotor == NULL )
	{
		CPASAttenuationFilter filter( this );

		m_sndMotor = g_SoundController->SoundCreate( filter, entindex(), CHAN_STATIC, "Weapon_PhysCannon.HoldSound", ATTN_NORM );
	}

	return m_sndMotor;
}

void CWeaponPhysCannon::StopLoopingSounds()
{
	if ( m_sndMotor != NULL )
	{
		g_SoundController->SoundDestroy( m_sndMotor );
		m_sndMotor = NULL;
	}

	BaseClass::StopLoopingSounds();

}

void CWeaponPhysCannon::DestroyEffects( void )
{
	// Stop everything
	StopEffects();
}

void CWeaponPhysCannon::StopEffects( bool stopSound )
{
	// Turn off our effect state
	DoEffect( EFFECT_NONE );

	//Shut off sounds
	if ( stopSound && GetMotorSound() != NULL )
	{
		g_SoundController->SoundFadeOut( GetMotorSound(), 0.1f );
	}
}

void CWeaponPhysCannon::StartEffects( void )
{

}

void CWeaponPhysCannon::DoEffectClosed( void )
{
}

void CWeaponPhysCannon::DoEffectReady( void )
{
}


void CWeaponPhysCannon::DoEffectHolding( void )
{
}

void CWeaponPhysCannon::DoEffectLaunch( Vector *pos )
{
	CPlayer *pOwner = ToBasePlayer( GetOwner() );
	if ( pOwner == NULL )
		return;

	Vector	endPos;
	Vector	shotDir;

	Vector shootPos;
	//shootPos = pOwner->Weapon_ShootPosition();
	pOwner->GetAttachment("muzzle_flash", shootPos);

	// See if we need to predict this position
	if ( pos == NULL )
	{
		// Hit an entity if we're holding one
		if ( m_hAttachedObject )
		{
			endPos = m_hAttachedObject->WorldSpaceCenter();

			shotDir = endPos - shootPos;
			VectorNormalize( shotDir );
		}
		else
		{
			// Otherwise try and find the right spot
			endPos = shootPos;
			pOwner->EyeVectors( &shotDir );

			trace_t	tr;
			UTIL_TraceLine( endPos, endPos + ( shotDir * MAX_TRACE_LENGTH ), MASK_SHOT, pOwner->BaseEntity(), COLLISION_GROUP_NONE, &tr );

			endPos = tr.endpos;
			shotDir = endPos - shootPos;
			VectorNormalize( shotDir );
		}
	}
	else
	{
		// Use what is supplied
		endPos = *pos;
		shotDir = ( endPos - shootPos );
		VectorNormalize( shotDir );
	}

	// End hit
	CPVSFilter filter( endPos );

	//CE_TODO
	// Don't send this to the owning player, they already had it predicted
	/*if ( IsPredicted() )
	{
		filter.UsePredictionRules();
	}*/

	// CE: fake effect because the usermessage is not available.
	filter.SetIgnorePredictionCull(true);
	te->BeamPoints(filter, 0.0f, &shootPos, &endPos, g_iModelPhysBeam, 0, 0, 30, 0.1f,  12.0f, 4.0f, 0.0f, 0.0f, 255, 255, 255, 255, 0);
	//te->GaussExplosion(filter, 0.0f, endPos, shotDir, 0);
	//te->Sparks(filter, 0.0f, &endPos, enginerandom->RandomInt(1, 2), 1, &shotDir);
te->MetalSparks(filter, 0.0f, &endPos, &shotDir);
#if 0
	// Do an impact hit
	CEffectData	data;
	data.m_vOrigin = endPos;
	data.m_nEntIndex = entindex();

	te->DispatchEffect( filter, 0.0, data.m_vOrigin, "PhyscannonImpact", data );
#endif
}


void CWeaponPhysCannon::DoEffectNone( void )
{

}

void CWeaponPhysCannon::DoEffect( int effectType, Vector *pos )
{
	m_EffectState = effectType;

	switch( effectType )
	{
		case EFFECT_CLOSED:
			DoEffectClosed( );
			break;

		case EFFECT_READY:
			DoEffectReady( );
			break;

		case EFFECT_HOLDING:
			DoEffectHolding();
			break;

		case EFFECT_LAUNCH:
			DoEffectLaunch( pos );
			break;

		default:
		case EFFECT_NONE:
			DoEffectNone();
			break;
	}
}

//CE_TODO
/*const char *CWeaponPhysCannon::GetShootSound( int iIndex ) const
{
	return BaseClass::GetShootSound( iIndex );
}
*/

CWeaponPhysCannon *ToCWeaponPhysCannon(CEntity *cent)
{
	CWeaponPhysCannon *physcannon = dynamic_cast<CWeaponPhysCannon *>(cent);
	if(physcannon && physcannon->IsPhysCannonReplace())
		return physcannon;
	return NULL;
}

CWeaponPhysCannon *ToCWeaponPhysCannon(CBaseEntity *cbase)
{
	return ToCWeaponPhysCannon(CEntity::Instance(cbase));
}

CEntity *GetPlayerHeldEntity( CPlayer *pPlayer )
{
	CEntity *pObject = NULL;
	CPlayerPickupController *pPlayerPickupController = (CPlayerPickupController *)(pPlayer->GetUseEntity());

	if ( pPlayerPickupController )
	{
		pObject = pPlayerPickupController->GetGrabController().GetAttached();
	}

	return pObject;
}

CEntity *PhysCannonGetHeldEntity( CCombatWeapon *pActiveWeapon )
{
	CWeaponPhysCannon *pCannon = dynamic_cast<CWeaponPhysCannon *>(pActiveWeapon);
	if ( pCannon )
	{
		CGrabController &grab = pCannon->GetGrabController();
		return grab.GetAttached();
	}

	return NULL;
}