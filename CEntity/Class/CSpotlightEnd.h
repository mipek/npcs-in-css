//========= Copyright Valve Corporation, All rights reserved. ============//
//
// Purpose:		Dynamic light at the end of a spotlight 
//
// $Workfile:     $
// $Date:         $
// $NoKeywords: $
//=============================================================================//

#ifndef	SPOTLIGHTEND_H
#define	SPOTLIGHTEND_H

#ifdef _WIN32
#pragma once
#endif


#include "CEntity.h"

class CE_CSpotlightEnd : public CEntity
{
	DECLARE_DATADESC();
public:
	CE_DECLARE_CLASS( CE_CSpotlightEnd, CEntity );

	void				Spawn( void );

	int					ObjectCaps( void )
	{
		// Don't save and don't go across transitions
		return (BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION) | FCAP_DONT_SAVE; 
	}

public:
	DECLARE_SENDPROP( float, m_flLightScale );
	DECLARE_SENDPROP( float, m_Radius );
//	CNetworkVector( m_vSpotlightDir );
//	CNetworkVector( m_vSpotlightOrg );
	Vector			m_vSpotlightDir;
	Vector			m_vSpotlightOrg;
};

#endif	//SPOTLIGHTEND_H


