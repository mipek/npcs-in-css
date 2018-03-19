
#include "CAI_NPC.h"
#include "CAI_tacticalservices.h"
#include "CAI_Node.h"
#include "CAI_Network.h"
#include "CAI_utils.h"



// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



#define	COVER_CHECKS	5// how many checks are made
#define COVER_DELTA		48// distance between checks


bool CAI_TacticalServices::FindLos(const Vector &threatPos, const Vector &threatEyePos, float minThreatDist, float maxThreatDist, float blockTime, Vector *pResult)
{
	return FindLos( threatPos, threatEyePos, minThreatDist, maxThreatDist, blockTime, FLANKTYPE_NONE, vec3_origin, 0, pResult );
}

bool CAI_TacticalServices::FindLos(const Vector &threatPos, const Vector &threatEyePos, float minThreatDist, float maxThreatDist, float blockTime, FlankType_t eFlankType, const Vector &vecFlankRefPos, float flFlankParam, Vector *pResult)
{
	MARK_TASK_EXPENSIVE();

	int node = FindLosNode( threatPos, threatEyePos, 
											 minThreatDist, maxThreatDist, 
											 blockTime, eFlankType, vecFlankRefPos, flFlankParam );
	
	if (node == NO_NODE)
		return false;

	*pResult = GetNodePos( node );
	return true;
}

int CAI_TacticalServices::FindLosNode(const Vector &vThreatPos, const Vector &vThreatEyePos, float flMinThreatDist, float flMaxThreatDist, float flBlockTime, FlankType_t eFlankType, const Vector &vecFlankRefPos, float flFlankParam )
{
	return g_helpfunc.CAI_TacticalServices_FindLosNode(this, vThreatPos, vThreatEyePos, flMinThreatDist, flMaxThreatDist, flBlockTime, eFlankType, vecFlankRefPos, flFlankParam);
}

Vector CAI_TacticalServices::GetNodePos( int node )
{
	return GetNetwork()->GetNode((int)node)->GetPosition(GetHullType());
}

bool CAI_TacticalServices::FindCoverPos( const Vector &vThreatPos, const Vector &vThreatEyePos, float flMinDist, float flMaxDist, Vector *pResult )
{
	return FindCoverPos( GetLocalOrigin(), vThreatPos, vThreatEyePos, flMinDist, flMaxDist, pResult );
}

bool CAI_TacticalServices::FindCoverPos( const Vector &vNearPos, const Vector &vThreatPos, const Vector &vThreatEyePos, float flMinDist, float flMaxDist, Vector *pResult )
{
	MARK_TASK_EXPENSIVE();

	int node = FindCoverNode( vNearPos, vThreatPos, vThreatEyePos, flMinDist, flMaxDist );
	
	if (node == NO_NODE)
		return false;

	*pResult = GetNodePos( node );
	return true;
}

int CAI_TacticalServices::FindCoverNode(const Vector &vNearPos, const Vector &vThreatPos, const Vector &vThreatEyePos, float flMinDist, float flMaxDist )
{
	return g_helpfunc.CAI_TacticalServices_FindCoverNode(this, vNearPos, vThreatPos, vThreatEyePos, flMinDist, flMaxDist);
}

extern ConVar *ai_find_lateral_los;

bool CAI_TacticalServices::FindLateralLos( const Vector &vecThreat, Vector *pResult )
{
	if( !m_bAllowFindLateralLos )
	{
		return false;
	}

	MARK_TASK_EXPENSIVE();

	Vector	vecLeftTest;
	Vector	vecRightTest;
	Vector	vecStepRight;
	Vector  vecCheckStart;
	bool	bLookingForEnemy = GetEnemy() && VectorsAreEqual(vecThreat, GetEnemy()->EyePosition(), 0.1f);
	int		i;

	if(  !bLookingForEnemy || GetOuter()->HasCondition(COND_SEE_ENEMY) || GetOuter()->HasCondition(COND_HAVE_ENEMY_LOS) || 
		 GetOuter()->GetTimeScheduleStarted() == gpGlobals->curtime ) // Conditions get nuked before tasks run, assume should try
	{
		// My current position might already be valid.
		if ( TestLateralLos(vecThreat, GetLocalOrigin()) )
		{
			*pResult = GetLocalOrigin();
			return true;
		}
	}

	if( !ai_find_lateral_los->GetBool() )
	{
		// Allows us to turn off lateral LOS at the console. Allow the above code to run 
		// just in case the NPC has line of sight to begin with.
		return false;
	}

	int iChecks = COVER_CHECKS;
	int iDelta = COVER_DELTA;

	// If we're limited in how far we're allowed to move laterally, don't bother checking past it
	int iMaxLateralDelta = (int)GetOuter()->GetMaxTacticalLateralMovement();
	if ( iMaxLateralDelta != MAXTACLAT_IGNORE && iMaxLateralDelta < iDelta )
	{
		iChecks = 1;
		iDelta = iMaxLateralDelta;
	}

	Vector right;
	AngleVectors( GetLocalAngles(), NULL, &right, NULL );
	vecStepRight = right * iDelta;
	vecStepRight.z = 0;

	vecLeftTest = vecRightTest = GetLocalOrigin();
 	vecCheckStart = vecThreat;

	for ( i = 0 ; i < iChecks; i++ )
	{
		vecLeftTest = vecLeftTest - vecStepRight;
		vecRightTest = vecRightTest + vecStepRight;

		if (TestLateralLos( vecCheckStart, vecLeftTest ))
		{
			*pResult = vecLeftTest;
			return true;
		}

		if (TestLateralLos( vecCheckStart, vecRightTest ))
		{
			*pResult = vecRightTest;
			return true;
		}
	}

	return false;
}

bool CAI_TacticalServices::TestLateralLos( const Vector &vecCheckStart, const Vector &vecCheckEnd )
{
	trace_t	tr;

	// it's faster to check the SightEnt's visibility to the potential spot than to check the local move, so we do that first.
	AI_TraceLOS(  vecCheckStart, vecCheckEnd + GetOuter()->GetViewOffset(), NULL, &tr );

	if (tr.fraction == 1.0)
	{
		if ( GetOuter()->IsValidShootPosition( vecCheckEnd, NULL, NULL ) )
			{
				if (GetOuter()->TestShootPosition(vecCheckEnd,vecCheckStart))
				{
					AIMoveTrace_t moveTrace;
					GetOuter()->GetMoveProbe()->MoveLimit( NAV_GROUND, GetLocalOrigin(), vecCheckEnd, MASK_NPCSOLID, NULL, &moveTrace );
					if (moveTrace.fStatus == AIMR_OK)
					{
						return true;
					}
				}
		}
	}

	return false;
}

bool CAI_TacticalServices::FindLateralCover( const Vector &vecThreat, float flMinDist, Vector *pResult )
{
	return FindLateralCover( vecThreat, flMinDist, COVER_CHECKS * COVER_DELTA, COVER_CHECKS, pResult );
}

bool CAI_TacticalServices::FindLateralCover( const Vector &vecThreat, float flMinDist, float distToCheck, int numChecksPerDir, Vector *pResult )
{
	return FindLateralCover( GetAbsOrigin(), vecThreat, flMinDist, distToCheck, numChecksPerDir, pResult );
}

extern ConVar *ai_find_lateral_cover;
bool CAI_TacticalServices::FindLateralCover( const Vector &vNearPos, const Vector &vecThreat, float flMinDist, float distToCheck, int numChecksPerDir, Vector *pResult )
{
	MARK_TASK_EXPENSIVE();

	Vector	vecLeftTest;
	Vector	vecRightTest;
	Vector	vecStepRight;
	Vector  vecCheckStart;
	int		i;

	if ( TestLateralCover( vecThreat, vNearPos, flMinDist ) )
	{
		*pResult = GetLocalOrigin();
		return true;
	}

	if( !ai_find_lateral_cover->GetBool() )
	{
		// Force the NPC to use the nodegraph to find cover. NOTE: We let the above code run
		// to detect the case where the NPC may already be standing in cover, but we don't 
		// make any additional lateral checks.
		return false;
	}

	Vector right =  vecThreat - vNearPos;
	float temp;

	right.z = 0;
	VectorNormalize( right );
	temp = right.x;
	right.x = -right.y;
	right.y = temp;

	vecStepRight = right * (distToCheck / (float)numChecksPerDir);
	vecStepRight.z = 0;

	vecLeftTest = vecRightTest = vNearPos;
 	vecCheckStart = vecThreat;

	for ( i = 0 ; i < numChecksPerDir ; i++ )
	{
		vecLeftTest = vecLeftTest - vecStepRight;
		vecRightTest = vecRightTest + vecStepRight;

		if (TestLateralCover( vecCheckStart, vecLeftTest, flMinDist ))
		{
			*pResult = vecLeftTest;
			return true;
		}

		if (TestLateralCover( vecCheckStart, vecRightTest, flMinDist ))
		{
			*pResult = vecRightTest;
			return true;
		}
	}

	return false;
}


bool CAI_TacticalServices::TestLateralCover( const Vector &vecCheckStart, const Vector &vecCheckEnd, float flMinDist )
{
	trace_t	tr;

	if ( (vecCheckStart - vecCheckEnd).LengthSqr() > Square(flMinDist) )
	{
		if (GetOuter()->IsCoverPosition(vecCheckStart, vecCheckEnd + GetOuter()->GetViewOffset()))
		{
			if ( GetOuter()->IsValidCover ( vecCheckEnd, NULL ) )
			{
				AIMoveTrace_t moveTrace;
				GetOuter()->GetMoveProbe()->MoveLimit( NAV_GROUND, GetLocalOrigin(), vecCheckEnd, MASK_NPCSOLID, NULL, &moveTrace );
				if (moveTrace.fStatus == AIMR_OK)
				{
					return true;
				}
			}
		}
	}
	return false;
}


