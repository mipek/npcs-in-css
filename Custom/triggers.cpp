
#include "CEntity.h"
#include "CPlayer.h"
#include "CCombatWeapon.h"
#include "CTriggerMultiple.h"
#include "CE_recipientfilter.h"
#include "weapon_physcannon_replace.h"

class CTriggerWeaponStrip : public CETriggerMultiple
{
public:
	CE_DECLARE_CLASS( CTriggerWeaponStrip, CETriggerMultiple );
	DECLARE_DATADESC();
	CE_CUSTOM_ENTITY();

public:
	void StartTouch(CEntity *pOther);
	void EndTouch(CEntity *pOther);

private:
	bool m_bKillWeapons;
};



LINK_ENTITY_TO_CUSTOM_CLASS( trigger_weapon_strip, trigger_multiple, CTriggerWeaponStrip );



BEGIN_DATADESC( CTriggerWeaponStrip )
	DEFINE_KEYFIELD( m_bKillWeapons,	FIELD_BOOLEAN, "KillWeapons" ),
END_DATADESC()


//-----------------------------------------------------------------------------
// Drops all weapons, marks the character as not being able to pick up weapons
//-----------------------------------------------------------------------------
void CTriggerWeaponStrip::StartTouch(CEntity *pOther)
{
	BaseClass::StartTouch( pOther );

	if ( PassesTriggerFilters((pOther)?pOther->BaseEntity():NULL) == false )
		return;

	CCombatCharacter *pCharacter = pOther->MyCombatCharacterPointer();
	
	if ( m_bKillWeapons )
	{
		for ( int i = 0 ; i < pCharacter->WeaponCount(); ++i )
		{
			CCombatWeapon *pWeapon = pCharacter->GetWeapon( i );
			if ( !pWeapon )
				continue;

			pCharacter->Weapon_Drop( pWeapon->BaseEntity() );
			UTIL_Remove( pWeapon );
		}
		return;
	}

	// Strip the player of his weapons
	if ( pCharacter && pCharacter->IsAllowedToPickupWeapons() )
	{
		pCharacter->Weapon_DropAll( true );
		pCharacter->SetPreventWeaponPickup( true );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Called when an entity stops touching us.
// Input  : pOther - The entity that was touching us.
//-----------------------------------------------------------------------------
void CTriggerWeaponStrip::EndTouch(CEntity *pOther)
{
	if ( IsTouching( pOther) )
	{
		CCombatCharacter *pCharacter = pOther->MyCombatCharacterPointer();
		if ( pCharacter )
		{
			pCharacter->SetPreventWeaponPickup( false );
		}
	}

	BaseClass::EndTouch( pOther );
}






class CTriggerPhysicsTrap : public CETriggerMultiple
{
public:
	CE_DECLARE_CLASS( CTriggerPhysicsTrap, CETriggerMultiple );
	DECLARE_DATADESC();
	CE_CUSTOM_ENTITY();

	void Touch( CEntity *pOther );

private:
	void InputEnable( inputdata_t &inputdata );
	void InputDisable( inputdata_t &inputdata );
	void InputToggle( inputdata_t &inputdata );

	int m_nDissolveType;
};



LINK_ENTITY_TO_CUSTOM_CLASS( trigger_physics_trap, trigger_multiple, CTriggerPhysicsTrap );


BEGIN_DATADESC( CTriggerPhysicsTrap )

	DEFINE_KEYFIELD( m_nDissolveType,	FIELD_INTEGER,	"dissolvetype" ),

	DEFINE_INPUTFUNC( FIELD_VOID, "Enable", InputEnable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Disable", InputDisable ),
	DEFINE_INPUTFUNC( FIELD_VOID, "Toggle", InputToggle ),

END_DATADESC()


void CTriggerPhysicsTrap::InputToggle( inputdata_t &inputdata )
{
	if ( m_bDisabled )
	{
		InputEnable( inputdata );
	}
	else
	{
		InputDisable( inputdata );
	}
}

void CTriggerPhysicsTrap::InputEnable( inputdata_t &inputdata )
{
	if ( m_bDisabled )
	{
		Enable();
	}
}

void CTriggerPhysicsTrap::InputDisable( inputdata_t &inputdata )
{
	if ( !m_bDisabled )
	{
		Disable();
	}
}

//-----------------------------------------------------------------------------
// Traps the entities
//-----------------------------------------------------------------------------
#define JOINTS_TO_CONSTRAIN 1

void CTriggerPhysicsTrap::Touch( CEntity *pOther )
{
	if ( !PassesTriggerFilters((pOther)?pOther->BaseEntity():NULL) )
		return;

	CAnimating *pAnim = pOther->GetBaseAnimating();
	if ( !pAnim )
		return;

	pAnim->Dissolve( NULL, gpGlobals->curtime, false, m_nDissolveType );
}

class CTriggerWeaponDissolve : public CETriggerMultiple
{
public:
	CE_DECLARE_CLASS( CTriggerWeaponDissolve, CETriggerMultiple );
	DECLARE_DATADESC();

	~CTriggerWeaponDissolve( void );

	virtual void Spawn( void );
	virtual void Precache( void );
	virtual void Activate( void );
	virtual void StartTouch( CEntity *pOther );

	inline bool HasWeapon( CCombatWeapon *pWeapon );

	Vector	GetConduitPoint( CEntity *pTarget );

	void	InputStopSound( inputdata_t &inputdata );

	void	AddWeapon( CCombatWeapon *pWeapon );
	void	CreateBeam( const Vector &vecSource, CEntity *pDest, float flLifetime );
	void	DissolveThink( void );

private:

	COutputEvent	m_OnDissolveWeapon;
	COutputEvent	m_OnChargingPhyscannon;

	CUtlVector< CEFakeHandle<CCombatWeapon> >	m_pWeapons;
	CUtlVector< CFakeHandle >					m_pConduitPoints;
	string_t									m_strEmitterName;
	int											m_spriteTexture;
};


LINK_ENTITY_TO_CUSTOM_CLASS( trigger_weapon_dissolve, trigger_multiple, CTriggerWeaponDissolve );



BEGIN_DATADESC( CTriggerWeaponDissolve )

					DEFINE_KEYFIELD( m_strEmitterName,	FIELD_STRING, "emittername" ),
					//DEFINE_UTLVECTOR( m_pWeapons,		FIELD_EHANDLE ),
					//DEFINE_UTLVECTOR( m_pConduitPoints, FIELD_EHANDLE ),
					DEFINE_FIELD( m_spriteTexture,		FIELD_MODELINDEX ),

					DEFINE_OUTPUT( m_OnDissolveWeapon, "OnDissolveWeapon" ),
					DEFINE_OUTPUT( m_OnChargingPhyscannon, "OnChargingPhyscannon" ),

					DEFINE_INPUTFUNC( FIELD_VOID, "StopSound", InputStopSound ),

					DEFINE_THINKFUNC( DissolveThink ),

END_DATADESC()


//-----------------------------------------------------------------------------
// Destructor
//-----------------------------------------------------------------------------
CTriggerWeaponDissolve::~CTriggerWeaponDissolve( void )
{
	m_pWeapons.Purge();
	m_pConduitPoints.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: Call precache for our sprite texture
//-----------------------------------------------------------------------------
void CTriggerWeaponDissolve::Spawn( void )
{
	BaseClass::Spawn();
	Precache();
}

//-----------------------------------------------------------------------------
// Purpose: Precache our sprite texture
//-----------------------------------------------------------------------------
void CTriggerWeaponDissolve::Precache( void )
{
	BaseClass::Precache();

	m_spriteTexture = PrecacheModel( "sprites/lgtning.vmt" );

	PrecacheScriptSound( "WeaponDissolve.Dissolve" );
	PrecacheScriptSound( "WeaponDissolve.Charge" );
	PrecacheScriptSound( "WeaponDissolve.Beam" );
}

static const char *s_pDissolveThinkContext = "DissolveThinkContext";

//-----------------------------------------------------------------------------
// Purpose: Collect all our known conduit points
//-----------------------------------------------------------------------------
void CTriggerWeaponDissolve::Activate( void )
{
	BaseClass::Activate();

	CEntity *pEntity = NULL;

	while ( ( pEntity = g_helpfunc.FindEntityByName( pEntity, m_strEmitterName ) ) != NULL )
	{
		CFakeHandle hndl;
		hndl.Set(pEntity->BaseEntity());
		m_pConduitPoints.AddToTail( hndl );
	}

	SetContextThink( &CTriggerWeaponDissolve::DissolveThink, gpGlobals->curtime + 0.1f, s_pDissolveThinkContext );
}

//-----------------------------------------------------------------------------
// Purpose: Checks to see if a weapon is already known
// Input  : *pWeapon - weapon to check for
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CTriggerWeaponDissolve::HasWeapon( CCombatWeapon *pWeapon )
{
	CEFakeHandle<CCombatWeapon> hndl;
	hndl.Set(pWeapon->BaseEntity());
	if ( m_pWeapons.Find( hndl ) == m_pWeapons.InvalidIndex() )
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Adds a weapon to the known weapon list
// Input  : *pWeapon - weapon to add
//-----------------------------------------------------------------------------
void CTriggerWeaponDissolve::AddWeapon( CCombatWeapon *pWeapon )
{
	if ( HasWeapon( pWeapon ) )
		return;

	CEFakeHandle<CCombatWeapon> hndl;
	hndl.Set(pWeapon->BaseEntity());
	m_pWeapons.AddToTail( hndl );
}

//-----------------------------------------------------------------------------
// Purpose: Collect any weapons inside our volume
// Input  : *pOther -
//-----------------------------------------------------------------------------
void CTriggerWeaponDissolve::StartTouch( CEntity *pOther )
{
	BaseClass::StartTouch( pOther );

	if ( PassesTriggerFilters( (pOther)?pOther->BaseEntity():NULL ) == false )
		return;

	CCombatWeapon *pWeapon = dynamic_cast<CCombatWeapon *>(pOther);

	if ( pWeapon == NULL )
		return;

	AddWeapon( pWeapon );
}

//-----------------------------------------------------------------------------
// Purpose: Creates a beam between a conduit point and a weapon
// Input  : &vecSource - conduit point
//			*pDest - weapon
//			flLifetime - amount of time
//-----------------------------------------------------------------------------
void CTriggerWeaponDissolve::CreateBeam( const Vector &vecSource, CEntity *pDest, float flLifetime )
{
	CBroadcastRecipientFilter filter;

	te->BeamEntPoint( filter, 0.0,
					  0,
					  &vecSource,
					  pDest->entindex(),
					  &(pDest->WorldSpaceCenter()),
					  m_spriteTexture,
					  0,				// No halo
					  1,				// Frame
					  30,
					  flLifetime,
					  16.0f,			// Start width
					  4.0f,			// End width
					  0,				// No fade
					  8,				// Amplitude
					  255,
					  255,
					  255,
					  255,
					  16 );			// Speed
}

//-----------------------------------------------------------------------------
// Purpose: Returns the closest conduit point to a weapon
// Input  : *pTarget - weapon to check for
// Output : Vector - position of the conduit
//-----------------------------------------------------------------------------
Vector CTriggerWeaponDissolve::GetConduitPoint( CEntity *pTarget )
{
	float	nearDist = 9999999.0f;
	Vector	bestPoint = vec3_origin;
	float	testDist;

	// Find the nearest conduit to the target
	for ( int i = 0; i < m_pConduitPoints.Count(); i++ )
	{
		testDist = ( m_pConduitPoints[i]->GetAbsOrigin() - pTarget->GetAbsOrigin() ).LengthSqr();

		if ( testDist < nearDist )
		{
			bestPoint = m_pConduitPoints[i]->GetAbsOrigin();
			nearDist = testDist;
		}
	}

	return bestPoint;
}

//-----------------------------------------------------------------------------
// Purpose: Dissolve all weapons within our volume
//-----------------------------------------------------------------------------
void CTriggerWeaponDissolve::DissolveThink( void )
{
	int	numWeapons = m_pWeapons.Count();

	// Dissolve all the items within the volume
	for ( int i = 0; i < numWeapons; i++ )
	{
		CCombatWeapon *pWeapon = m_pWeapons[i];
		Vector vecConduit = GetConduitPoint( pWeapon );

		// The physcannon upgrades when this happens
		if ( ToCWeaponPhysCannon( pWeapon) )
		{
			// This must be the last weapon for us to care
			if ( numWeapons > 1 )
				continue;

			//FIXME: Make them do this on a stagger!

			// All conduits send power to the weapon
			for ( int i = 0; i < m_pConduitPoints.Count(); i++ )
			{
				CreateBeam( m_pConduitPoints[i]->GetAbsOrigin(), pWeapon, 4.0f );
			}

			//PhysCannonBeginUpgrade( pWeapon );
			m_OnChargingPhyscannon.FireOutput( this, this );

			EmitSound( "WeaponDissolve.Beam" );

			// We're done
			m_pWeapons.Purge();
			m_pConduitPoints.Purge();
			SetContextThink( NULL, 0, s_pDissolveThinkContext );
			return;
		}

		// Randomly dissolve them all
		float flLifetime = enginerandom->RandomFloat( 2.5f, 4.0f );
		CreateBeam( vecConduit, pWeapon, flLifetime );
		pWeapon->Dissolve( NULL, gpGlobals->curtime + ( 3.0f - flLifetime ), false );

		m_OnDissolveWeapon.FireOutput( this, this );

		CPASAttenuationFilter filter( pWeapon );
		EmitSound( filter, pWeapon->entindex(), "WeaponDissolve.Dissolve" );

		// Beam looping sound
		EmitSound( "WeaponDissolve.Beam" );

		m_pWeapons.Remove( i );
		SetContextThink( &CTriggerWeaponDissolve::DissolveThink, gpGlobals->curtime + enginerandom->RandomFloat( 0.5f, 1.5f ), s_pDissolveThinkContext );
		return;
	}

	SetContextThink( &CTriggerWeaponDissolve::DissolveThink, gpGlobals->curtime + 0.1f, s_pDissolveThinkContext );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  : &inputdata -
//-----------------------------------------------------------------------------
void CTriggerWeaponDissolve::InputStopSound( inputdata_t &inputdata )
{
	StopSound( "WeaponDissolve.Beam" );
	StopSound( "WeaponDissolve.Charge" );
}
