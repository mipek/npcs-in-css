
#include "CEntity.h"
#include "CPlayer.h"
#include "CCombatWeapon.h"
#include "CTriggerMultiple.h"


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


