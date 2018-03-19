//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Dynamic light at the end of a spotlight
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#include "CSpotlightEnd.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CE_LINK_ENTITY_TO_CLASS(CSpotlightEnd, CE_CSpotlightEnd);

DEFINE_PROP(m_flLightScale, CE_CSpotlightEnd);
DEFINE_PROP(m_Radius, CE_CSpotlightEnd);

#if 0
IMPLEMENT_SERVERCLASS_ST(CSpotlightEnd, DT_SpotlightEnd)
	SendPropFloat(SENDINFO(m_flLightScale), 0, SPROP_NOSCALE),
	SendPropFloat(SENDINFO(m_Radius), 0, SPROP_NOSCALE),
//	SendPropVector(SENDINFO(m_vSpotlightDir), -1, SPROP_NORMAL),
//	SendPropVector(SENDINFO(m_vSpotlightOrg), -1, SPROP_COORD),
END_SEND_TABLE()
#endif


//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CE_CSpotlightEnd )

	DEFINE_FIELD( m_flLightScale, FIELD_FLOAT ),
	DEFINE_FIELD( m_Radius, FIELD_FLOAT ),
	DEFINE_FIELD( m_vSpotlightDir, FIELD_VECTOR ),
	DEFINE_FIELD( m_vSpotlightOrg, FIELD_POSITION_VECTOR ),

END_DATADESC()


//------------------------------------------------------------------------------
// Purpose :
// Input   :
// Output  :
//------------------------------------------------------------------------------
void CE_CSpotlightEnd::Spawn( void )
{
	Precache();
	m_flLightScale  = 100.f;
	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_FLY );
	UTIL_SetSize( BaseEntity(), vec3_origin, vec3_origin );
	AddEFlags( EFL_FORCE_CHECK_TRANSMIT );
}
