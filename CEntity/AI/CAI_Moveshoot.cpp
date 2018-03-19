
#include "CAI_NPC.h"
#include "CAI_Moveshoot.h"
#include "CAI_utils.h"


void CAI_MoveAndShootOverlay::EndShootWhileMove()
{
	if ( m_bMovingAndShooting )
	{
		// Reset the shot regulator so that we always start the next motion with a new burst
		if ( !GetOuter()->GetShotRegulator()->IsInRestInterval() )
		{
			GetOuter()->GetShotRegulator()->Reset( false );
		}

		m_bMovingAndShooting = false;
		GetOuter()->OnEndMoveAndShoot();
	}
}

void CAI_MoveAndShootOverlay::SetInitialDelay( float delay )
{
	m_initialDelay = delay;
}

void CAI_MoveAndShootOverlay::SuspendMoveAndShoot( float flDuration )
{
	EndShootWhileMove();
	m_flSuspendUntilTime = gpGlobals->curtime + flDuration;
}

