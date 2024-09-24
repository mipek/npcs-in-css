//========= Copyright ?1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef AI_SPEECHFILTER_H
#define AI_SPEECHFILTER_H
#ifdef _WIN32
#pragma once
#endif

#include "CEntity.h"

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
class CE_AI_SpeechFilter : public CEntity
{
	DECLARE_CLASS( CE_AI_SpeechFilter, CEntity );
public:
	float	GetIdleModifier( void ) { return m_flIdleModifier; }
	bool	NeverSayHello( void ) { return m_bNeverSayHello; }


protected:
	DECLARE_DATAMAP(float, m_flIdleModifier);
	DECLARE_DATAMAP(bool, m_bNeverSayHello);

};

#endif // AI_SPEECHFILTER_H