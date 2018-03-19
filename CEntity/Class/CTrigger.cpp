
#include "CTrigger.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"




CE_LINK_ENTITY_TO_CLASS(CBaseTrigger, CTrigger);


SH_DECL_MANUALHOOK1(PassesTriggerFilters, 0, 0, 0, bool, CBaseEntity *);
DECLARE_HOOK(PassesTriggerFilters, CTrigger);
DECLARE_DEFAULTHANDLER(CTrigger, PassesTriggerFilters, bool, (CBaseEntity *pOther), (pOther));



//Datamaps
DEFINE_PROP(m_hTouchingEntities, CTrigger);
DEFINE_PROP(m_bDisabled, CTrigger);



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
