//========= Copyright ?1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#include "CEntity.h"
#include "CAI_localnavigator.h"

#include "CAI_NPC.h"
#include "CAI_planesolver.h"
#include "CAI_moveprobe.h"
#include "CAI_motor.h"



// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"




const float TIME_DELAY_FULL_DIRECT_PROBE[2] = { 0.25, 0.35 };


//-------------------------------------

void CAI_LocalNavigator::ResetMoveCalculations()
{
	m_FullDirectTimer.Force();
	m_pPlaneSolver->Reset();
}
