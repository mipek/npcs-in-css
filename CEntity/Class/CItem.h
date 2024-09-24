#ifndef _INCLUDE_CITEM_H
#define _INCLUDE_CITEM_H

#include "CEntity.h"
#include "CPlayer.h"
#include "CSode_Fix.h"
#include "ItemRespawnSystem.h"
#include "vphysics/constraints.h"
#include "player_pickup.h"


// Armor given by a battery
#define MAX_NORMAL_BATTERY	100

// Ammo counts given by ammo items
#define SIZE_AMMO_PISTOL			20
#define SIZE_AMMO_PISTOL_LARGE		100
#define SIZE_AMMO_SMG1				45
#define SIZE_AMMO_SMG1_LARGE		225
#define SIZE_AMMO_AR2				20
#define SIZE_AMMO_AR2_LARGE			100
#define SIZE_AMMO_RPG_ROUND			1
#define SIZE_AMMO_SMG1_GRENADE		1
#define SIZE_AMMO_BUCKSHOT			20
#define SIZE_AMMO_357				6
#define SIZE_AMMO_357_LARGE			20
#define SIZE_AMMO_CROSSBOW			6
#define	SIZE_AMMO_AR2_ALTFIRE		1

#define SF_ITEM_START_CONSTRAINED	0x00000001

#define ITEM_PICKUP_BOX_BLOAT		24


abstract_class ICItem
{
public:
	CEntity *GetOuter() { return pOuter; }
	void SetOuter(CEntity *pEntity) { pOuter = pEntity; }
	virtual void Materialize( void ) =0;
	virtual bool ShouldMaterialize() =0;
	virtual bool GetObjectsOriginalParameters(Vector &vOriginalOrigin, QAngle &vOriginalAngles) =0;

private:
	CEntity *pOuter;
};

template <class BASE_T>
class CItem : public BASE_T, public ICItem, public CDefaultPlayerPickupVPhysics
{
public:
	DECLARE_CLASS_NOFRIEND( CItem, BASE_T );

	CItem();
	virtual ~CItem();

	void Respawn( void );
	virtual void Spawn( void );
	virtual void UpdateOnRemove();
	virtual void Precache();
	virtual void Think(void);
	virtual void Touch( CEntity *pOther );
	virtual void OnEntityEvent( EntityEvent_t event, void *pEventData );

	void Materialize( void );
	bool ShouldMaterialize();
	virtual bool GetObjectsOriginalParameters(Vector &vOriginalOrigin, QAngle &vOriginalAngles);

	virtual CEntity* MyTouch( CPlayer *pPlayer ) { return nullptr; };


	void ActivateWhenAtRest();

	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	bool	CreateItemVPhysicsObject( void );
	bool	ItemCanBeTouchedByPlayer( CPlayer *pPlayer );


	void	FallThink( void );

	Vector	GetOriginalSpawnOrigin( void ) { return m_vOriginalSpawnOrigin;	}
	QAngle	GetOriginalSpawnAngles( void ) { return m_vOriginalSpawnAngles;	}
	void	SetOriginalSpawnOrigin( const Vector& origin ) { m_vOriginalSpawnOrigin = origin; }
	void	SetOriginalSpawnAngles( const QAngle& angles ) { m_vOriginalSpawnAngles = angles; }


private:
	void ComeToRest( void );

protected:
	bool m_bRespawn;
	float m_flNextResetCheckTime;
	float m_fNextTouchTime;
	float m_fRespawnTime;

	COutputEvent m_OnPlayerTouch;
	COutputEvent m_OnCacheInteraction;

protected:
	bool m_bActivateWhenAtRest;
	Vector m_vOriginalSpawnOrigin;
	QAngle m_vOriginalSpawnAngles;
	IPhysicsConstraint *m_pConstraint;

};

extern ConVar sv_css_item_respawn_time;


template <class BASE_T>
CItem<BASE_T>::CItem()
{
	m_bRespawn = false;
	m_pConstraint = NULL;
	m_bActivateWhenAtRest = false;
	m_flNextResetCheckTime = 0.0f;
	m_fRespawnTime = -1;
	m_fNextTouchTime = 0.0f;
	SetOuter(this);
}

template <class BASE_T>
CItem<BASE_T>::~CItem()
{
	if ( m_pConstraint != NULL )
	{
		physenv->DestroyConstraint( m_pConstraint );
		m_pConstraint = NULL;
	}
}

template <class BASE_T>
void CItem<BASE_T>::Spawn( void )
{
	if(!m_bRespawn)
	{
		BaseClass::Spawn();
		return;
	}

	BaseClass::Spawn();

	this->PrecacheScriptSound("AlyxEmp.Charge");

	this->SetMoveType( MOVETYPE_FLYGRAVITY );
	this->SetSolid( SOLID_BBOX );
	this->SetBlocksLOS( false );
	this->AddEFlags( EFL_NO_ROTORWASH_PUSH );
	
	// This will make them not collide with the player, but will collide
	// against other items + weapons
	this->SetCollisionGroup( COLLISION_GROUP_WEAPON );
	this->CollisionProp()->UseTriggerBounds( true, ITEM_PICKUP_BOX_BLOAT );
	m_fNextTouchTime = gpGlobals->curtime;

	if ( CreateItemVPhysicsObject() == false )
		return;

	this->m_takedamage = DAMAGE_EVENTS_ONLY;

	// Constrained start?
	if ( this->HasSpawnFlags( SF_ITEM_START_CONSTRAINED ) )
	{
		//Constrain the weapon in place
		IPhysicsObject *pReferenceObject, *pAttachedObject;

		pReferenceObject = my_g_WorldEntity->VPhysicsGetObject();
		pAttachedObject = this->VPhysicsGetObject();

		if ( pReferenceObject && pAttachedObject )
		{
			constraint_fixedparams_t fixed;
			fixed.Defaults();
			fixed.InitWithCurrentObjectState( pReferenceObject, pAttachedObject );

			fixed.constraint.forceLimit	= lbs2kg( 10000 );
			fixed.constraint.torqueLimit = lbs2kg( 10000 );

			m_pConstraint = physenv->CreateFixedConstraint( pReferenceObject, pAttachedObject, nullptr, fixed );

			m_pConstraint->SetGameData( (void *) this->BaseEntity() );
		}
	}

	this->SetThink( &CItem::FallThink );
	this->SetNextThink( gpGlobals->curtime + 0.1f );
}

template <class BASE_T>
void CItem<BASE_T>::Think(void)
{
	if(!m_bRespawn)
	{
		BaseClass::Think();
		return;
	}

	VALVE_BASEPTR original_think = this->m_pfnThink;
	if(original_think != NULL)
	{
		(this->BaseEntity()->*original_think)();
		return;
	}
}


template <class BASE_T>
void CItem<BASE_T>::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if(!m_bRespawn)
	{
		BaseClass::Use(pActivator, pCaller, useType, value);
		return;
	}

	CPlayer *pPlayer = ToBasePlayer( CEntity::Instance(pActivator) );

	if ( pPlayer )
	{
		//pPlayer->PickupObject( this );

		// Revan: This is a custom addition so players can pickup guns just like in CS:GO..
		//if(!this->GetOwnerEntity())
		//{
		//	MyTouch(pPlayer);
		//}
	}
}

template <class BASE_T>
void CItem<BASE_T>::ActivateWhenAtRest()
{
	this->RemoveSolidFlags( FSOLID_TRIGGER );
	m_bActivateWhenAtRest = true;
	this->SetThink( &CItem::ComeToRest );
	this->SetNextThink( gpGlobals->curtime + 0.5f );
}

template <class BASE_T>
void CItem<BASE_T>::ComeToRest( void )
{
	if ( m_bActivateWhenAtRest )
	{
		m_bActivateWhenAtRest = false;
		this->AddSolidFlags( FSOLID_TRIGGER );
		this->SetThink( NULL );
	}
}

template <class BASE_T>
void CItem<BASE_T>::OnEntityEvent( EntityEvent_t event, void *pEventData )
{
	BaseClass::OnEntityEvent( event, pEventData );

	if(!m_bRespawn)
		return;

	switch( event )
	{
	case ENTITY_EVENT_WATER_TOUCH:
		{
			// Delay rest for a sec, to avoid changing collision 
			// properties inside a collision callback.
			this->SetThink( &CItem::ComeToRest );
			this->SetNextThink( gpGlobals->curtime + 0.1f );
		}
		break;
	default:
		break;
	}
}


template <class BASE_T>
void CItem<BASE_T>::FallThink ( void )
{
	this->SetNextThink( gpGlobals->curtime + 0.1f );

	bool shouldMaterialize = false;
	IPhysicsObject *pPhysics = this->VPhysicsGetObject();
	if ( pPhysics )
	{
		shouldMaterialize = pPhysics->IsAsleep();
	}
	else
	{
		shouldMaterialize = (this->GetFlags() & FL_ONGROUND);
	}

	if ( shouldMaterialize )
	{
		this->SetThink ( NULL );

		m_vOriginalSpawnOrigin = this->GetAbsOrigin();
		m_vOriginalSpawnAngles = this->GetAbsAngles();

		g_ItemRespawnSystem.AddItem(this);
	}
}

template <class BASE_T>
bool CItem<BASE_T>::ItemCanBeTouchedByPlayer( CPlayer *pPlayer )
{
	return UTIL_ItemCanBeTouchedByPlayer( this, pPlayer );
}

template <class BASE_T>
void CItem<BASE_T>::Touch( CEntity *pOther )
{
	if(!m_bRespawn)
	{
		BaseClass::Touch(pOther);
		return;
	}

	if(m_fNextTouchTime > gpGlobals->curtime)
		return;

	// Vehicles can touch items + pick them up
	if ( pOther->GetServerVehicle() )
	{
		pOther = CEntity::Instance(pOther->GetServerVehicle()->GetPassenger());
		if ( !pOther )
			return;
	}

	// if it's not a player, ignore
	if ( !pOther->IsPlayer() )
		return;

	CPlayer *pPlayer = (CPlayer *)pOther;

	// Must be a valid pickup scenario (no blocking). Though this is a more expensive
	// check than some that follow, this has to be first Obecause it's the only one
	// that inhibits firing the output OnCacheInteraction.
	if ( ItemCanBeTouchedByPlayer( pPlayer ) == false )
		return;

	m_OnCacheInteraction.FireOutput(pOther, this);

	// Can I even pick stuff up?
	if ( !pPlayer->IsAllowedToPickupWeapons() )
		return;

	if ( MyTouch( pPlayer ) )
	{
		m_OnPlayerTouch.FireOutput(pOther, this);

		this->SetThink( NULL );

		Respawn();
	}
}

template <class BASE_T>
void CItem<BASE_T>::Respawn( void )
{
	this->SetTouch( nullptr );
	this->AddEffects( EF_NODRAW );

	this->VPhysicsDestroyObject();

	this->SetMoveType( MOVETYPE_NONE );
	this->SetSolid( SOLID_BBOX );
	this->AddSolidFlags( FSOLID_TRIGGER );

	UTIL_SetOrigin( this, this->GetOriginalSpawnOrigin() );// blip to whereever you should respawn.
	this->SetAbsAngles( this->GetOriginalSpawnAngles() );// set the angles.

	UTIL_DropToFloor( this, MASK_SOLID );

	this->RemoveAllDecals(); //remove any decals

	m_fRespawnTime = gpGlobals->curtime + sv_css_item_respawn_time.GetFloat();
	m_fNextTouchTime = m_fRespawnTime + 0.1f;
}

template <class BASE_T>
bool CItem<BASE_T>::ShouldMaterialize()
{
	return (m_fRespawnTime > 0.0f && gpGlobals->curtime > m_fRespawnTime);
}

template <class BASE_T>
void CItem<BASE_T>::Materialize( void )
{
	m_fRespawnTime = -1;

	CreateItemVPhysicsObject();

	if ( this->IsEffectActive( EF_NODRAW ) )
	{
		// changing from invisible state to visible.
		this->EmitSound( "AlyxEmp.Charge" );

		this->RemoveEffects( EF_NODRAW );
		this->DoMuzzleFlash_Animating();
	}

	m_fNextTouchTime = gpGlobals->curtime + 0.5f;
}

template <class BASE_T>
bool CItem<BASE_T>::CreateItemVPhysicsObject( void )
{
	// Create the object in the physics system
	int nSolidFlags = this->GetSolidFlags() | FSOLID_NOT_STANDABLE;
	if ( !m_bActivateWhenAtRest )
	{
		nSolidFlags |= FSOLID_TRIGGER;
	}

	if ( this->VPhysicsInitNormal( SOLID_VPHYSICS, nSolidFlags, false ) == NULL )
	{
		this->SetSolid( SOLID_BBOX );
		this->AddSolidFlags( nSolidFlags );

		// If it's not physical, drop it to the floor
		if (UTIL_DropToFloor(this, MASK_SOLID) == 0)
		{
			Warning( "Item %s fell out of level at %f,%f,%f\n",
					 this->GetClassname(), this->GetAbsOrigin().x, this->GetAbsOrigin().y, this->GetAbsOrigin().z);
			UTIL_Remove( this );
			return false;
		}
	}

	return true;
}

template <class BASE_T>
bool CItem<BASE_T>::GetObjectsOriginalParameters(Vector &vOriginalOrigin, QAngle &vOriginalAngles)
{
	if ( m_flNextResetCheckTime > gpGlobals->curtime )
		return false;

	vOriginalOrigin = m_vOriginalSpawnOrigin;
	vOriginalAngles = m_vOriginalSpawnAngles;

	m_flNextResetCheckTime = gpGlobals->curtime + sv_css_item_respawn_time.GetFloat();
	return true;
}


template <class BASE_T>
void CItem<BASE_T>::UpdateOnRemove()
{
	g_ItemRespawnSystem.RemoveItem(this);
	BaseClass::UpdateOnRemove();
}

template <class BASE_T>
void CItem<BASE_T>::Precache()
{
	BaseClass::Precache();

	this->PrecacheScriptSound( "Item.Materialize" );
}




#endif


