
#include "CPhysicsProp.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


CE_LINK_ENTITY_TO_CLASS(CPhysicsProp, CE_CPhysicsProp);

DEFINE_PROP(m_OnPhysGunPickup, CE_CPhysicsProp);
DEFINE_PROP(m_OnPhysGunOnlyPickup, CE_CPhysicsProp);
DEFINE_PROP(m_OnPhysGunPunt, CE_CPhysicsProp);
DEFINE_PROP(m_OnPhysGunDrop, CE_CPhysicsProp);


bool CE_CPhysicsProp::GetPropDataAngles( const char *pKeyName, QAngle &vecAngles )
{
	KeyValues *modelKeyValues = new KeyValues("");
	if ( modelKeyValues->LoadFromBuffer( modelinfo->GetModelName( GetModel() ), modelinfo->GetModelKeyValueText( GetModel() ) ) )
	{
		KeyValues *pkvPropData = modelKeyValues->FindKey( "physgun_interactions" );
		if ( pkvPropData )
		{
			char const *pszBase = pkvPropData->GetString( pKeyName );
			if ( pszBase && pszBase[0] )
			{
				UTIL_StringToVector( vecAngles.Base(), pszBase );
				modelKeyValues->deleteThis();
				return true;
			}
		}
	}

	modelKeyValues->deleteThis();
	return false;
}

/*void CE_CPhysicsProp::OnPhysGunPickup( CBaseEntity *pPhysGunUser, PhysGunPickup_t reason )
{
	BaseClass::OnPhysGunPickup(pPhysGunUser, reason);

	m_OnPhysGunPickup->FireOutput( pPhysGunUser, this );

	if (reason == PICKED_UP_BY_CANNON)
	{
		m_OnPhysGunOnlyPickup->FireOutput( pPhysGunUser, this );
	}

	if ( reason == PUNTED_BY_CANNON )
	{
		m_OnPhysGunPunt->FireOutput( pPhysGunUser, this );
	}
}

void CE_CPhysicsProp::OnPhysGunDrop( CBaseEntity *pPhysGunUser, PhysGunDrop_t reason )
{
	BaseClass::OnPhysGunDrop(pPhysGunUser, reason);

	m_OnPhysGunDrop->FireOutput( pPhysGunUser, this );
}*/