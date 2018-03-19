
#include "CAI_NPC.h"
#include "CAI_Navigator.h"
#include "CAI_Route.h"
#include "ai_routedist.h"
#include "CAI_Hint.h"
#include "CAI_utils.h"
#include "CPropDoor.h"



// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



extern ConVar *ai_use_clipped_paths;
extern ConVar *ai_navigator_generate_spikes;
extern ConVar *ai_navigator_generate_spikes_strength;

//-----------------------------------------------------------------------------

class CAI_NavInHintGroupFilter : public INearestNodeFilter
{
public:
	CAI_NavInHintGroupFilter( string_t iszGroup = NULL_STRING ) :
	  m_iszGroup( iszGroup )
	  {
	  }

	  bool IsValid( CAI_Node *pNode )
	  {
		  if ( !pNode->GetHint() )
		  {
			  return false;
		  }

		  if ( pNode->GetHint()->GetGroup() != m_iszGroup )
		  {
			  return false;
		  }

		  return true;
	  }

	  bool ShouldContinue()
	  {
		  return true;
	  }

	  string_t m_iszGroup;

};


const Vector AIN_NO_DEST( FLT_MAX, FLT_MAX, FLT_MAX );
#define NavVecToString(v) ((v == AIN_NO_DEST) ? "AIN_NO_DEST" : VecToString(v))

#define FLIER_CUT_CORNER_DIST		16 // 8 means the npc's bounding box is contained without the box of the node in WC

#define NAV_STOP_MOVING_TOLERANCE	6	// Goal tolerance for TASK_STOP_MOVING stopping paths


//-----------------------------------------------------------------------------
bool CAI_Navigator::SetRadialGoal( const Vector &destination, const Vector &center, float radius, float arc, float stepDist, bool bClockwise, bool bAirRoute )
{
	//DbgNavMsg( GetOuter(), "Set radial goal\n" );
	OnNewGoal();
	GetPath()->SetGoalType(GOALTYPE_LOCATION);

	GetPath()->SetWaypoints( GetPathfinder()->BuildRadialRoute( GetLocalOrigin(), center, destination, radius, arc, stepDist, bClockwise, GetPath()->GetGoalTolerance(), bAirRoute ), true);			
	GetPath()->SetGoalTolerance( GetOuter()->GetDefaultNavGoalTolerance() );

	return IsGoalActive();
}


//-----------------------------------------------------------------------------

bool CAI_Navigator::IsGoalActive() const
{
	return ( GetPath() && !( const_cast<CAI_Path *>(GetPath())->IsEmpty() ) );
}


//-----------------------------------------------------------------------------

Activity CAI_Navigator::SetMovementActivity(Activity activity)
{
	return GetPath()->SetMovementActivity( activity );
}


//-----------------------------------------------------------------------------
// Purpose: Sets navigation type, maintaining any necessary invariants
//-----------------------------------------------------------------------------
void CAI_Navigator::SetNavType( Navigation_t navType )
{ 
	m_navType = navType;
}


bool CAI_Navigator::IsGoalSet() const
{
	return ( GetPath()->GoalType() != GOALTYPE_NONE );
}


bool CAI_Navigator::ClearGoal()
{ 
	ClearPath();
	OnNewGoal();
	return true; 
}

//-----------------------------------------------------------------------------

void CAI_Navigator::ClearPath( void )
{
	OnClearPath();

	m_timePathRebuildMax	= 0;					// How long to try rebuilding path before failing task
	m_timePathRebuildFail	= 0;					// Current global time when should fail building path
	m_timePathRebuildNext	= 0;					// Global time to try rebuilding again
	m_timePathRebuildDelay	= 0;					// How long to wait before trying to rebuild again

	Forget( bits_MEMORY_PATH_FAILED );

	AI_Waypoint_t *pWaypoint = GetPath()->GetCurWaypoint();

	if ( pWaypoint )
	{
		SaveStoppingPath();
		m_PreviousMoveActivity = GetMovementActivity();
		m_PreviousArrivalActivity = GetArrivalActivity();

		if( m_pClippedWaypoints && m_pClippedWaypoints->GetFirst() )
		{
			Assert( m_PreviousMoveActivity > ACT_RESET );
		}

		while ( pWaypoint )
		{
			if ( pWaypoint->iNodeID != NO_NODE )
			{
				CAI_Node *pNode = GetNetwork()->GetNode(pWaypoint->iNodeID);
				
				if ( pNode )
				{
					if ( pNode->IsLocked() )
						 pNode->Unlock();
				}
			}
			pWaypoint = pWaypoint->GetNext();
		}
	}

	GetPath()->Clear();
}

//-----------------------------------------------------------------------------

Activity CAI_Navigator::GetArrivalActivity( ) const
{
	return GetPath()->GetArrivalActivity( );
}

//-----------------------------------------------------------------------------

Activity CAI_Navigator::GetMovementActivity() const
{
	return GetPath()->GetMovementActivity();
}

void CAI_Navigator::SaveStoppingPath( void )
{
	m_flTimeClipped = -1;

	m_pClippedWaypoints->RemoveAll();
	AI_Waypoint_t *pCurWaypoint = GetPath()->GetCurWaypoint();
	if ( pCurWaypoint )
	{
		if ( ( pCurWaypoint->NavType() == NAV_CLIMB || pCurWaypoint->NavType() == NAV_JUMP ) || ai_use_clipped_paths->GetBool() )
		{	
			if ( GetStoppingPath( m_pClippedWaypoints ) )
				m_flTimeClipped = gpGlobals->curtime;
		}
	}
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::SetVectorGoalFromTarget( const Vector &goalPos, float minDist, bool fShouldDeflect )
{
	Vector vDir = goalPos;
	float dist = ComputePathDirection( GetNavType(), GetLocalOrigin(), goalPos, &vDir );
	return SetVectorGoal( vDir, dist, minDist, fShouldDeflect );
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::SetVectorGoal( const Vector &dir, float targetDist, float minDist, bool fShouldDeflect )
{
	Vector result;

	if ( FindVectorGoal( &result, dir, targetDist, minDist, fShouldDeflect ) )
		return SetGoal( result );
	
	return false;
}


//-----------------------------------------------------------------------------

bool CAI_Navigator::FindVectorGoal( Vector *pResult, const Vector &dir, float targetDist, float minDist, bool fShouldDeflect )
{
	AIMoveTrace_t moveTrace;
	float distAchieved = 0;
	
	MARK_TASK_EXPENSIVE();

	Vector testLoc = GetLocalOrigin() + ( dir * targetDist );
	GetMoveProbe()->MoveLimit( GetNavType(), GetLocalOrigin(), testLoc, MASK_NPCSOLID, NULL, &moveTrace );
	
	if ( moveTrace.fStatus != AIMR_OK )
	{
		distAchieved = targetDist - moveTrace.flDistObstructed;
		if ( fShouldDeflect && moveTrace.vHitNormal != vec3_origin )
		{
			Vector vecDeflect;
			Vector vecNormal = moveTrace.vHitNormal;
			if ( GetNavType() == NAV_GROUND )
				vecNormal.z = 0;

			CalculateDeflection( moveTrace.vEndPosition, dir, vecNormal, &vecDeflect );

			testLoc = moveTrace.vEndPosition + ( vecDeflect * ( targetDist - distAchieved ) );
	
			Vector temp = moveTrace.vEndPosition;
			GetMoveProbe()->MoveLimit( GetNavType(), temp, testLoc, MASK_NPCSOLID, NULL, &moveTrace );

			distAchieved += ( targetDist - distAchieved ) - moveTrace.flDistObstructed;
		}
		
		if ( distAchieved < minDist + 0.01 )
			return false;
	}

	*pResult = moveTrace.vEndPosition;
	return true;
}

//-----------------------------------------------------------------------------

void CAI_Navigator::CalculateDeflection( const Vector &start, const Vector &dir, const Vector &normal, Vector *pResult )
{
	Vector temp;
	CrossProduct( dir, normal, temp );
	CrossProduct( normal, temp, *pResult );
	VectorNormalize( *pResult );
}

//-----------------------------------------------------------------------------
float CAI_Navigator::GetStepDownMultiplier()
{
	if ( m_hBigStepGroundEnt )
	{
		if ( !CEntity::Instance(m_hBigStepGroundEnt)->IsPlayer() )
			return 2.6;
		else
			return 10.0;
	}
	return 1.0;
}

float CAI_Navigator::GetArrivalDistance() const
{
	return GetPath()->GetGoalStoppingDistance();
}


//-----------------------------------------------------------------------------

int CAI_Navigator::GetArrivalSequence( int curSequence )
{
	int sequence = GetPath()->GetArrivalSequence( );
	if (sequence == ACT_INVALID)
	{
		Activity activity = GetOuter()->GetStoppedActivity();

		Assert( activity != ACT_INVALID );
		if (activity == ACT_INVALID)
		{
			activity = ACT_IDLE;
		}

		sequence = GetOuter()->SelectWeightedSequence( GetOuter()->TranslateActivity( activity ), curSequence );

		if ( sequence == ACT_INVALID )
		{
			//DevMsg( GetOuter(), "No appropriate sequence for arrival activity %s (%d)\n", GetOuter()->GetActivityName( GetPath()->GetArrivalActivity() ), GetPath()->GetArrivalActivity() );
			sequence = GetOuter()->SelectWeightedSequence( GetOuter()->TranslateActivity( ACT_IDLE ), curSequence );
		}
		Assert( sequence != ACT_INVALID );
		GetPath()->SetArrivalSequence( sequence );
	}
	return sequence;
}
//-----------------------------------------------------------------------------

int CAI_Navigator::GetMovementSequence( )
{
	int sequence = GetPath()->GetMovementSequence( );
	if (sequence == ACT_INVALID)
	{
		Activity activity = GetPath()->GetMovementActivity();
		Assert( activity != ACT_INVALID );

		sequence = GetOuter()->SelectWeightedSequence( GetOuter()->TranslateActivity( activity ) );
		if ( sequence == ACT_INVALID )
		{
			//DevMsg( GetOuter(), "No appropriate sequence for movement activity %s (%d)\n", GetOuter()->GetActivityName( GetPath()->GetArrivalActivity() ), GetPath()->GetArrivalActivity() );

			if ( activity == ACT_SCRIPT_CUSTOM_MOVE )
			{
				sequence = GetOuter()->GetScriptCustomMoveSequence();
			}
			else
			{
				sequence = GetOuter()->SelectWeightedSequence( GetOuter()->TranslateActivity( ACT_WALK ) );
			}
		}
		// CE_TODO: why does this assertion fail? investgate
		//Assert( sequence != ACT_INVALID );
		GetPath()->SetMovementSequence( sequence );
	}
	return sequence;
}


//-----------------------------------------------------------------------------

float CAI_Navigator::GetArrivalSpeed( void )
{ 
	float flSpeed = GetPath()->GetGoalSpeed( GetAbsOrigin() );

	if (flSpeed >= 0.0)
	{
		return flSpeed;
	}

	int sequence = GetArrivalSequence( ACT_INVALID );

	if (sequence != ACT_INVALID)
	{
		flSpeed = GetOuter()->GetEntryVelocity( sequence );
		SetArrivalSpeed( flSpeed );
	}
	else
	{
		flSpeed = 0.0;
	}

	return flSpeed;
}

void CAI_Navigator::SetArrivalSpeed( float flSpeed )
{
	GetPath()->SetGoalSpeed( flSpeed );
}

Vector CAI_Navigator::GetArrivalDirection( )
{
	return GetPath()->GetGoalDirection( GetAbsOrigin() );
}

GoalType_t CAI_Navigator::GetGoalType() const
{
	return GetPath()->GoalType();
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::RefindPathToGoal( bool fSignalTaskStatus, bool bDontIgnoreBadLinks )
{
	return FindPath( fSignalTaskStatus, bDontIgnoreBadLinks );
}


//-----------------------------------------------------------------------------
// Purpose: Attempts to build a route
// Input  :
// Output : True if successful / false if fail
//-----------------------------------------------------------------------------
bool CAI_Navigator::FindPath( bool fSignalTaskStatus, bool bDontIgnoreBadLinks )
{
	// Test to see if we're resolving spiking problems via threading
	if ( ai_navigator_generate_spikes->GetBool() )
	{
		unsigned int nLargeCount = (INT_MAX>>(ai_navigator_generate_spikes_strength->GetInt()));
		while ( nLargeCount-- ) {}
	}

	bool bRetrying = (HasMemory(bits_MEMORY_PATH_FAILED) && m_timePathRebuildMax != 0 );
	if ( bRetrying )
	{
		// If I've passed by fail time, fail this task
		if (m_timePathRebuildFail < gpGlobals->curtime)
		{
			if ( fSignalTaskStatus )
				OnNavFailed( FAIL_NO_ROUTE );
			else
				OnNavFailed();
			return false;
		}
		else if ( m_timePathRebuildNext > gpGlobals->curtime )
		{
			return false;
		}
	}

	bool bFindResult = DoFindPath();

	if ( !bDontIgnoreBadLinks && !bFindResult && GetOuter()->IsNavigationUrgent() )
	{
		GetPathfinder()->SetIgnoreBadLinks();
		bFindResult = DoFindPath();
	}

	if (bFindResult)
	{	
		Forget(bits_MEMORY_PATH_FAILED);

		if (fSignalTaskStatus) 
		{
			TaskComplete();
		}
		return true;
	}

	if (m_timePathRebuildMax == 0)
	{
		if ( fSignalTaskStatus )
			OnNavFailed( FAIL_NO_ROUTE );
		else
			OnNavFailed();
		return false;
	}
	else
	{
		if ( !bRetrying )
		{
			Remember(bits_MEMORY_PATH_FAILED);
			m_timePathRebuildFail = gpGlobals->curtime + m_timePathRebuildMax;
		}
		m_timePathRebuildNext = gpGlobals->curtime + m_timePathRebuildDelay;
		return false;
	}
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Attemps to find a route
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CAI_Navigator::DoFindPath( void )
{
	GetPath()->ClearWaypoints();

	bool		returnCode;

	returnCode = false;

	switch( GetPath()->GoalType() )
	{
	case GOALTYPE_PATHCORNER:
		{
			returnCode = DoFindPathToPathcorner( GetGoalEnt() );
		}
		break;

	case GOALTYPE_ENEMY:
		{
			// NOTE: This is going to set the goal position, which was *not*
			// set in SetGoal for this movement type
			CEntity *pEnemy = GetPath()->GetTarget();
			if (pEnemy)
			{
				Assert( pEnemy == GetEnemy() );

				Vector newPos = GetEnemyLKP();

				float tolerance = GetPath()->GetGoalTolerance();
				float outerTolerance = GetOuter()->GetDefaultNavGoalTolerance();
				if ( outerTolerance > tolerance )
				{
					GetPath()->SetGoalTolerance( outerTolerance );
					tolerance = outerTolerance;
				}
				
				TranslateNavGoal( pEnemy, newPos );

				// NOTE: Calling reset here because this can get called
				// any time we have to update our goal position
				GetPath()->ResetGoalPosition( newPos );
				GetPath()->SetGoalTolerance( tolerance );

				returnCode = DoFindPathToPos();
			}
		}
		break;

	case GOALTYPE_LOCATION:
	case GOALTYPE_FLANK:
	case GOALTYPE_COVER:
		returnCode = DoFindPathToPos();
		break;

	case GOALTYPE_LOCATION_NEAREST_NODE:
		{
			int myNodeID;
			int destNodeID;

			returnCode = false;
			
			myNodeID = GetPathfinder()->NearestNodeToNPC();
			if (myNodeID != NO_NODE)
			{
				destNodeID = GetPathfinder()->NearestNodeToPoint( GetPath()->ActualGoalPosition() );
				if (destNodeID != NO_NODE)
				{
					AI_Waypoint_t *pRoute = GetPathfinder()->FindBestPath( myNodeID, destNodeID );
					
					if ( pRoute != NULL )
					{
						GetPath()->SetWaypoints( pRoute );
						GetPath()->SetLastNodeAsGoal(true);
						returnCode = true;
					}
				}
			}
		}
		break;

	case GOALTYPE_TARGETENT:
		{
			// NOTE: This is going to set the goal position, which was *not*
			// set in SetGoal for this movement type
			CEntity *pTarget = GetPath()->GetTarget();
			
			if ( pTarget )
			{
				Assert( pTarget == GetTarget() );

				// NOTE: Calling reset here because this can get called
				// any time we have to update our goal position
				
				Vector	initPos = pTarget->GetAbsOrigin();
				TranslateNavGoal( pTarget, initPos );

				GetPath()->ResetGoalPosition( initPos );
				returnCode = DoFindPathToPos();
			}
		}
		break;
	
	default:
		break;
	}

	return returnCode;
}


//-----------------------------------------------------------------------------

void CAI_Navigator::OnNavFailed( bool bMovement )
{
	if ( bMovement )
		GetOuter()->OnMovementFailed();

	ResetCalculations();
	m_fNavComplete = true;
	m_bLastNavFailed = true;
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::DoFindPathToPathcorner( CEntity *pPathCorner )
{
// UNDONE: This is broken
// UNDONE: Remove this and change the pathing code to be able to refresh itself and support
// circular paths, etc.
	bool returnCode = false;
	Assert( GetPath()->GoalType() == GOALTYPE_PATHCORNER );

	// NPC is on a path_corner loop
	if ( pPathCorner != NULL )
	{
		// Find path to first pathcorner
		if ( ( GetGoalFlags() & AIN_NO_PATHCORNER_PATHFINDING ) || m_bNoPathcornerPathfinds )
		{
			// HACKHACK: If the starting path_corner has a speed, copy that to the entity
			if ( *(pPathCorner->m_flSpeed) != 0 )
				SetSpeed( pPathCorner->m_flSpeed );

			GetPath()->ClearWaypoints();

			AI_Waypoint_t *pRoute = new AI_Waypoint_t( pPathCorner->GetLocalOrigin(), 0, GetNavType(), bits_WP_TO_PATHCORNER, NO_NODE );
			pRoute->hPathCorner = pPathCorner->BaseEntity();
			AI_Waypoint_t *pLast = pRoute;
			pPathCorner = GetNextPathcorner(pPathCorner);
			if ( pPathCorner )
			{
				pLast = new AI_Waypoint_t( pPathCorner->GetLocalOrigin(), 0, GetNavType(), bits_WP_TO_PATHCORNER, NO_NODE );
				pLast->hPathCorner = pPathCorner->BaseEntity();
				pRoute->SetNext(pLast);
			}
			pLast->ModifyFlags( bits_WP_TO_GOAL, true );
			GetPath()->SetWaypoints( pRoute, true );
			returnCode = true;
		}
		else
		{
			Vector initPos = pPathCorner->GetLocalOrigin();

			TranslateNavGoal( pPathCorner, initPos );

			GetPath()->ResetGoalPosition( initPos );
			
			float tolerance = GetPath()->GetGoalTolerance();
			float outerTolerance = GetOuter()->GetDefaultNavGoalTolerance();
			if ( outerTolerance > tolerance )
			{
				GetPath()->SetGoalTolerance( outerTolerance );
				tolerance = outerTolerance;
			}

			if ( ( returnCode = DoFindPathToPos() ) != false )
			{
				float speed = pPathCorner->m_flSpeed;
				// HACKHACK: If the starting path_corner has a speed, copy that to the entity
				if (speed != 0 )
				{
					SetSpeed( speed );
				}

				AI_Waypoint_t *lastWaypoint	= GetPath()->GetGoalWaypoint();
				Assert(lastWaypoint);

				lastWaypoint->ModifyFlags( bits_WP_TO_PATHCORNER, true );
				lastWaypoint->hPathCorner = pPathCorner->BaseEntity();

				pPathCorner = GetNextPathcorner(pPathCorner); // first already accounted for in pathfind
				if ( pPathCorner )
				{
					// Place a dummy node in that will be used to signal the next pathfind, also prevents
					// animation system from decellerating when approaching a pathcorner
					lastWaypoint->ModifyFlags( bits_WP_TO_GOAL, false );
					// BRJ 10/4/02
					// FIXME: I'm not certain about the navtype here
					AI_Waypoint_t *curWaypoint = new AI_Waypoint_t( pPathCorner->GetLocalOrigin(), 0, GetNavType(), (bits_WP_TO_PATHCORNER | bits_WP_TO_GOAL), NO_NODE );
					Vector waypointPos = curWaypoint->GetPos();
					TranslateNavGoal( pPathCorner, waypointPos );
					curWaypoint->SetPos( waypointPos );
					GetPath()->SetGoalTolerance( tolerance );
					curWaypoint->hPathCorner = pPathCorner->BaseEntity();
					lastWaypoint->SetNext(curWaypoint);
					GetPath()->ResetGoalPosition( curWaypoint->GetPos() );
				}
			}
		}
	}
	return returnCode;
}

//-----------------------------------------------------------------------------

void CAI_Navigator::ResetCalculations()
{
	m_hPeerWaitingOn = NULL;
	m_PeerWaitMoveTimer.Force();
	m_PeerWaitClearTimer.Force();

	m_hBigStepGroundEnt = NULL;

	m_NextSidestepTimer.Force();

	m_bCalledStartMove = false;

	m_vPosBeginFailedSteer = vec3_invalid;
	m_timeBeginFailedSteer = FLT_MAX;

	m_flLastSuccessfulSimplifyTime = -1;

	GetLocalNavigator()->ResetMoveCalculations();
	GetMotor()->ResetMoveCalculations();
	GetMoveProbe()->ClearBlockingEntity();

	m_nNavFailCounter = 0;
	m_flLastNavFailTime = -1;
}

//-----------------------------------------------------------------------------

void CAI_Navigator::OnNavFailed( AI_TaskFailureCode_t code, bool bMovement )
{
	if ( GetOuter()->ShouldFailNav( bMovement ) )
	{
		OnNavFailed( bMovement );
		SetActivity( GetOuter()->GetStoppedActivity() );
		TaskFail(code);
	}
	else
	{
		m_nNavFailCounter++;
		m_flLastNavFailTime = gpGlobals->curtime;
		if ( GetOuter()->ShouldBruteForceFailedNav() )
		{
			if (bMovement)
			{

				m_timeBeginFailedSteer = FLT_MAX;

				// if failing, turn off collisions with the object
				CEntity *pBlocker = GetBlockingEntity();
				// FIXME: change this to only be for MOVETYPE_VPHYSICS?
				if (pBlocker && !pBlocker->IsWorld() && !pBlocker->IsPlayer() && !FClassnameIs( pBlocker, "func_tracktrain" ))
				{
					//pBlocker->DrawBBoxOverlay( 2.0f );
					if (g_helpfunc.NPCPhysics_CreateSolver( GetOuter()->BaseEntity(), pBlocker->BaseEntity(), true, 10.0f ) != NULL)
					{
						ClearNavFailCounter();
					}
				}

				// if still failing, try jumping forward through the route
				if (GetNavFailCounter() > 0)
				{
					if (TeleportAlongPath())
					{
						ClearNavFailCounter();
					}
				}
			}
			else
			{
				CEntity *pBlocker = GetMoveProbe()->GetBlockingEntity();
				if (pBlocker)
				{
					//pBlocker->DrawBBoxOverlay( 2.0f );
					if (g_helpfunc.NPCPhysics_CreateSolver( GetOuter()->BaseEntity(), pBlocker->BaseEntity(), true, 10.0f ) != NULL)
					{
						ClearNavFailCounter();
					}
				}
			}
		}
	}
}

int	CAI_Navigator::GetNavFailCounter() const
{
	return m_nNavFailCounter;
}

//-----------------------------------------------------------------------------

void CAI_Navigator::ClearNavFailCounter()
{
	m_nNavFailCounter = 0;
}

//-----------------------------------------------------------------------------

int	CAI_Navigator::GetGoalFlags() const
{
	return GetPath()->GoalFlags();
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::TeleportAlongPath()
{
	while (GetPath()->GetCurWaypoint())
	{
		Vector vecStart;
		Vector vTestPoint;

		vecStart = GetPath()->CurWaypointPos();
		AdvancePath();

		GetOuter()->GetMoveProbe()->FloorPoint( vecStart, MASK_NPCSOLID, GetOuter()->StepHeight(), -64, &vTestPoint );

		if ( CanFitAtPosition( vTestPoint, MASK_NPCSOLID, false, false ) )
		{
			if ( GetOuter()->GetMoveProbe()->CheckStandPosition( vTestPoint, MASK_NPCSOLID ) )
			{
				GetOuter()->Teleport( &vTestPoint, NULL, NULL );
				// clear ground entity, let normal fall code reestablish what the npc is now standing on
				GetOuter()->SetGroundEntity( NULL );
				GetOuter()->PhysicsTouchTriggers( &vTestPoint );
				return true;
			}
		}

		if (CurWaypointIsGoal())
			break;
	}
	return false;
}

void CAI_Navigator::SetSpeed( float fl )
{
	GetOuter()->m_flSpeed = fl;
}

//-----------------------------------------------------------------------------

CEntity *CAI_Navigator::GetNextPathcorner( CEntity *pPathCorner )
{
	CEntity *pNextPathCorner = NULL;

	Assert( pPathCorner );
	if ( pPathCorner )
	{
		pNextPathCorner = pPathCorner->GetNextTarget();

		CE_AI_Hint *pHint;
		if ( !pNextPathCorner && ( pHint = dynamic_cast<CE_AI_Hint *>( pPathCorner ) ) != NULL )
		{
			int targetNode = pHint->GetTargetNode();
			if ( targetNode != NO_NODE )
			{
				CAI_Node *pTestNode = GetNetwork()->GetNode(targetNode);
				pNextPathCorner = pTestNode->GetHint();
			}
		}
	}

	return pNextPathCorner;
}

//-----------------------------------------------------------------------------
// Purpose: Returns true if NPC's hull fits in the given spot with the
//			given collision mask
// Input  :
// Output :
//-----------------------------------------------------------------------------
bool CAI_Navigator::CanFitAtPosition( const Vector &vStartPos, unsigned int collisionMask, bool bIgnoreTransients, bool bAllowPlayerAvoid  )
{
	CTraceFilterNav traceFilter( const_cast<CAI_NPC *>(GetOuter()), bIgnoreTransients, GetOuter()->BaseEntity(), COLLISION_GROUP_NONE, bAllowPlayerAvoid );

	Vector vEndPos	= vStartPos;
	vEndPos.z += 0.01;
	trace_t tr;
	UTIL_TraceHull( vStartPos, vEndPos, 
				  GetHullMins(), GetHullMaxs(), 
				  collisionMask, 
				  &traceFilter, 
				  &tr );

	if (tr.startsolid)
	{
		return false;
	}
	return true;
}


//-----------------------------------------------------------------------------
// Purpose: Attempts to advance the route to the next waypoint, triangulating
//			around entities that are in the way
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_Navigator::AdvancePath()
{
	AI_Waypoint_t *pCurWaypoint = GetPath()->GetCurWaypoint();
	bool bPassingPathcorner = ( ( pCurWaypoint->Flags() & bits_WP_TO_PATHCORNER ) != 0 );

	if ( bPassingPathcorner )
	{
		CEntity *pEntity = CEntity::Instance(pCurWaypoint->hPathCorner);
		if ( pEntity )
		{
			variant_t emptyVariant;
			pEntity->CustomAcceptInput( "InPass", GetOuter()->BaseEntity(), pEntity->BaseEntity(), emptyVariant, 0 );
		}
	}

	if ( GetPath()->CurWaypointIsGoal() )
		return;

	if ( pCurWaypoint->Flags() & bits_WP_TO_DOOR )
	{
		CEntity *cent = CEntity::Instance(pCurWaypoint->GetEHandleData());
		CPropDoor *pDoor = (CPropDoor *)cent;
		if (pDoor != NULL)
		{
			GetOuter()->OpenPropDoorBegin(pDoor);
		}
		else
		{
			//DevMsg("%s trying to open a door that has been deleted!\n", GetOuter()->GetDebugName());
		}
	}

	GetPath()->Advance();

	// If we've just passed a path_corner, advance m_pGoalEnt
	if ( bPassingPathcorner )
	{
		pCurWaypoint = GetPath()->GetCurWaypoint();

		if ( pCurWaypoint )
		{
			Assert( (pCurWaypoint->Flags() & (bits_WP_TO_PATHCORNER | bits_WP_TO_GOAL )) == (bits_WP_TO_PATHCORNER | bits_WP_TO_GOAL ));

			SetGoalEnt( pCurWaypoint->hPathCorner );

			CEntity *pEntity = CEntity::Instance(pCurWaypoint->hPathCorner);
			DoFindPathToPathcorner( pEntity );
		}
	}
}


bool CAI_Navigator::CurWaypointIsGoal() const
{
	return GetPath()->CurWaypointIsGoal();
}

CAI_Pathfinder *CAI_Navigator::GetPathfinder()
{ 
	return GetOuter()->GetPathfinder(); 
}

//-----------------------------------------------------------------------------

const CAI_Pathfinder *CAI_Navigator::GetPathfinder() const
{ 
	return GetOuter()->GetPathfinder(); 
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
float CAI_Navigator::MovementCost( int moveType, Vector &vecStart, Vector &vecEnd )
{
	float cost;
	
	cost = (vecStart - vecEnd).Length();

	if ( moveType == bits_CAP_MOVE_JUMP || moveType == bits_CAP_MOVE_CLIMB )
	{
		cost *= 2.0;
	}

	// Allow the NPC to override the movement cost
	GetOuter()->MovementCost( moveType, vecStart, vecEnd, &cost );
	
	return cost;
}

//-----------------------------------------------------------------------------
// Purpose: Will the entities hull fit at the given node
// Input  :
// Output : Returns true if hull fits at node
//-----------------------------------------------------------------------------
bool CAI_Navigator::CanFitAtNode(int nodeNum, unsigned int collisionMask ) 
{
	// Make sure I have a network
	if (!GetNetwork())
	{
		DevMsg("CanFitAtNode() called with no network!\n");
		return false;
	}

	CAI_Node* pTestNode = GetNetwork()->GetNode(nodeNum);
	Vector startPos		= pTestNode->GetPosition(GetHullType());

	// -------------------------------------------------------------------
	// Check ground nodes for standable bottom
	// -------------------------------------------------------------------
	if (pTestNode->GetType() == NODE_GROUND)
	{
		if (!GetMoveProbe()->CheckStandPosition( startPos, collisionMask ))
		{
			return false;
		}
	}


	// -------------------------------------------------------------------
	// Check climb exit nodes for standable bottom
	// -------------------------------------------------------------------
	if ((pTestNode->GetType() == NODE_CLIMB) &&
		(pTestNode->m_eNodeInfo & (bits_NODE_CLIMB_BOTTOM | bits_NODE_CLIMB_EXIT )))
	{
		if (!GetMoveProbe()->CheckStandPosition( startPos, collisionMask ))
		{
			return false;
		}
	}


	// -------------------------------------------------------------------
	// Check that hull fits at position of node
	// -------------------------------------------------------------------
	if (!CanFitAtPosition( startPos, collisionMask ))
	{
		startPos.z += GetOuter()->StepHeight();
		if (!CanFitAtPosition( startPos, collisionMask ))
			return false;
	}

	return true;
}

const Vector &CAI_Navigator::GetCurWaypointPos() const
{
	return GetPath()->CurWaypointPos();
}


const Vector &CAI_Navigator::GetGoalPos() const
{ 
	return GetPath()->BaseGoalPosition(); 
}

//-----------------------------------------------------------------------------

AI_NavPathProgress_t CAI_Navigator::ProgressFlyPath( const AI_ProgressFlyPathParams_t &params )
{
	if ( IsGoalActive() )
	{
		float waypointDist = ( GetCurWaypointPos() - GetLocalOrigin() ).Length();

		if ( CurWaypointIsGoal() )
		{
			float tolerance = max( params.goalTolerance, GetPath()->GetGoalTolerance() );
			if ( waypointDist <= tolerance )
				return AINPP_COMPLETE;
		}
		else
		{
			bool bIsStrictWaypoint = ( (GetPath()->CurWaypointFlags() & (bits_WP_TO_PATHCORNER|bits_WP_DONT_SIMPLIFY) ) != 0 );
			float tolerance = (bIsStrictWaypoint) ? params.strictPointTolerance : params.waypointTolerance;
			if ( waypointDist <= tolerance )
			{
				trace_t tr;
				UTIL_TraceLine( GetAbsOrigin(), GetPath()->GetCurWaypoint()->GetNext()->GetPos(), MASK_NPCSOLID, GetOuter()->BaseEntity(), COLLISION_GROUP_NONE, &tr );
				if ( tr.fraction == 1.0f )
				{
					AdvancePath();	
					return AINPP_ADVANCED;
				}
			}

			if ( SimplifyFlyPath( params ) )
				return AINPP_ADVANCED;
		}
	}

	return AINPP_NO_CHANGE;
}

//-----------------------------------------------------------------------------

void CAI_Navigator::SimplifyFlyPath( unsigned collisionMask, const CBaseEntity *pTarget, 
									 float strictPointTolerance, float blockTolerance,
									 AI_NpcBlockHandling_t blockHandling)
{
	AI_ProgressFlyPathParams_t params( collisionMask, strictPointTolerance, blockTolerance,
									   0, 0, blockHandling );
	params.SetCurrent( pTarget );
	SimplifyFlyPath( params );
}

//-----------------------------------------------------------------------------

#define FLY_ROUTE_SIMPLIFY_TIME_DELAY 0.3
#define FLY_ROUTE_SIMPLIFY_LOOK_DIST (12.0*12.0)

bool CAI_Navigator::SimplifyFlyPath(  const AI_ProgressFlyPathParams_t &params )
{
	if ( !GetPath()->GetCurWaypoint() )
		return false;

	if ( m_flNextSimplifyTime > gpGlobals->curtime)
		return false;

	m_flNextSimplifyTime = gpGlobals->curtime + FLY_ROUTE_SIMPLIFY_TIME_DELAY;

	if ( params.bTrySimplify && SimplifyPathForward( FLY_ROUTE_SIMPLIFY_LOOK_DIST ) )
		return true;

	// don't shorten path_corners
	bool bIsStrictWaypoint = ( !params.bTrySimplify || ( (GetPath()->CurWaypointFlags() & (bits_WP_TO_PATHCORNER|bits_WP_DONT_SIMPLIFY) ) != 0 ) );

	Vector dir = GetCurWaypointPos() - GetLocalOrigin();
	float length = VectorNormalize( dir );
	
	if ( !bIsStrictWaypoint || length < params.strictPointTolerance )
	{
		// FIXME: This seems strange... Why should this condition ever be met?
		// Don't advance your waypoint if you don't have one!
		if (GetPath()->CurWaypointIsGoal())
			return false;

		AIMoveTrace_t moveTrace;
		GetMoveProbe()->MoveLimit( NAV_FLY, GetLocalOrigin(), GetPath()->NextWaypointPos(),
			params.collisionMask, CEntity::Instance(params.pTarget), &moveTrace);
		
		if ( moveTrace.flDistObstructed - params.blockTolerance < 0.01 || 
			 ( ( params.blockHandling == AISF_IGNORE) && ( moveTrace.fStatus == AIMR_BLOCKED_NPC ) ) )
		{
			AdvancePath();
			return true;
		}
		else if ( moveTrace.pObstruction && params.blockHandling == AISF_AVOID )
		{
			PrependLocalAvoidance( params.blockTolerance - moveTrace.flDistObstructed, moveTrace );
		}
	}

	return false;
}

//-------------------------------------

bool CAI_Navigator::SimplifyPathForward( float maxDist )
{
	AI_Waypoint_t *pCurWaypoint = GetPath()->GetCurWaypoint();
	AI_Waypoint_t *pNextWaypoint = pCurWaypoint->GetNext();

	if ( !pNextWaypoint )
		return false;

	static SimplifyForwardScanParams fullScanParams = 
	{
		32 * 12, 	// Distance to move out path
		12 * 12, 	// Radius within which a point must be to be valid
		3 * 12, 	// Increment to move out on
		4, 			// maximum number of point samples
	};

	SimplifyForwardScanParams scanParams = fullScanParams;
	if ( maxDist > fullScanParams.radius )
	{
		float ratio = (maxDist / fullScanParams.radius);

		fullScanParams.radius = maxDist;
		fullScanParams.scanDist *= ratio;
		fullScanParams.increment *= ratio;
	}
	
	if ( SimplifyPathForwardScan( scanParams ) )
		return true;

	if ( ShouldAttemptSimplifyTo( pNextWaypoint->GetPos() ) && 
		 ComputePathDistance( GetNavType(), GetLocalOrigin(), pNextWaypoint->GetPos() ) < scanParams.scanDist && 
		 ShouldSimplifyTo( ( ( pCurWaypoint->Flags() & bits_WP_TO_DETOUR ) != 0 ), pNextWaypoint->GetPos() ) ) // @TODO (toml 04-25-03): need to handle this better. this is here because forward scan may look out so far that a close obvious solution is skipped (due to test limiting)
	{
		delete pCurWaypoint;
		GetPath()->SetWaypoints(pNextWaypoint);
		return true;
	}
	
	return false;
}

//-------------------------------------

bool CAI_Navigator::SimplifyPathForwardScan( const CAI_Navigator::SimplifyForwardScanParams &params, 
											 AI_Waypoint_t *pCurWaypoint, const Vector &curPoint, 
											 float distRemaining, bool skipTest, bool passedDetour, int *pTestCount )
{
	AI_Waypoint_t *pNextWaypoint = pCurWaypoint->GetNext();

	if ( !passedDetour )
		passedDetour = ( ( pCurWaypoint->Flags() & bits_WP_TO_DETOUR) != 0 );

	if ( distRemaining > 0)
	{
		if ( pCurWaypoint->IsReducible() )
		{
			// Walk out to test point, or next waypoint
			AI_Waypoint_t *pRecursionWaypoint;
			Vector nextPoint;
			
			float distToNext = ComputePathDistance( GetNavType(), curPoint, pNextWaypoint->GetPos() );
			if (distToNext < params.increment * 1.10 )
			{
				nextPoint = pNextWaypoint->GetPos();
				distRemaining -= distToNext;
				pRecursionWaypoint = pNextWaypoint;
			}
			else
			{
				Vector offset = pNextWaypoint->GetPos() - pCurWaypoint->GetPos();
				VectorNormalize( offset );
				offset *= params.increment;
				nextPoint = curPoint + offset;
				distRemaining -= params.increment;
				pRecursionWaypoint = pCurWaypoint;
			}
			
			bool skipTestNext = ( ComputePathDistance( GetNavType(), GetLocalOrigin(), nextPoint ) > params.radius + 0.1 );

			if ( SimplifyPathForwardScan( params, pRecursionWaypoint, nextPoint, distRemaining, skipTestNext, passedDetour, pTestCount ) )
				return true;
		}
	}
	
	if ( !skipTest && *pTestCount < params.maxSamples && ShouldAttemptSimplifyTo( curPoint ) )
	{
		(*pTestCount)++;

		if ( ShouldSimplifyTo( passedDetour, curPoint ) )
		{
			SimplifyPathInsertSimplification( pCurWaypoint, curPoint );
			return true;
		}
	}
	
	return false;
}

//-------------------------------------

bool CAI_Navigator::SimplifyPathForwardScan( const CAI_Navigator::SimplifyForwardScanParams &params )
{
	AI_Waypoint_t *pCurWaypoint = GetPath()->GetCurWaypoint();
	float distRemaining = params.scanDist - GetPathDistToCurWaypoint();
	int testCount = 0;

	if ( distRemaining < 0.1 )
		return false;
	
	if ( SimplifyPathForwardScan( params, pCurWaypoint, pCurWaypoint->GetPos(), distRemaining, true, false, &testCount ) )
		return true;
		
	return false;
}


//-------------------------------------


void CAI_Navigator::SimplifyPathInsertSimplification( AI_Waypoint_t *pSegmentStart, const Vector &point )
{
	if ( point != pSegmentStart->GetPos() )
	{
		AI_Waypoint_t *pNextWaypoint = pSegmentStart->GetNext();
		Assert( pNextWaypoint );
		Assert( pSegmentStart->NavType() == pNextWaypoint->NavType() );

		AI_Waypoint_t *pNewWaypoint = new AI_Waypoint_t( point, 0, pSegmentStart->NavType(), 0, NO_NODE );

		while ( GetPath()->GetCurWaypoint() != pNextWaypoint )
		{
			Assert( GetPath()->GetCurWaypoint()->IsReducible() );
			GetPath()->Advance();
		}
		pNewWaypoint->SetNext( pNextWaypoint );
		GetPath()->SetWaypoints( pNewWaypoint );
	}
	else
	{
		while ( GetPath()->GetCurWaypoint() != pSegmentStart )
		{
			Assert( GetPath()->GetCurWaypoint()->IsReducible() );
			GetPath()->Advance();
		}
	}

}

//-------------------------------------


//-------------------------------------

bool CAI_Navigator::ShouldSimplifyTo( bool passedDetour, const Vector &pos )
{
	int flags = AIMLF_QUICK_REJECT;

#ifndef NPCS_BLOCK_SIMPLIFICATION
	if ( !passedDetour )
		flags |= AIMLF_IGNORE_TRANSIENTS;
#endif

	AIMoveTrace_t moveTrace;
	GetMoveProbe()->MoveLimit( GetNavType(), 
		GetLocalOrigin(), pos, MASK_NPCSOLID, 
		GetPath()->GetTarget(), 100, flags, &moveTrace );

	return !IsMoveBlocked(moveTrace);
}


const float MIN_ANGLE_COS_SIMPLIFY = 0.766; // 40 deg left or right

bool CAI_Navigator::ShouldAttemptSimplifyTo( const Vector &pos )
{
	if ( m_bForcedSimplify )
		return true;
		
	Vector vecToPos = ( pos - GetLocalOrigin() );
	vecToPos.z = 0;
	VectorNormalize( vecToPos );

	Vector vecCurrentDirectionOfMovement = ( GetCurWaypointPos() - GetLocalOrigin() );
	vecCurrentDirectionOfMovement.z = 0;
	VectorNormalize( vecCurrentDirectionOfMovement );
	
	float dot = vecCurrentDirectionOfMovement.AsVector2D().Dot( vecToPos.AsVector2D() );
	
	return ( m_bForcedSimplify || dot > MIN_ANGLE_COS_SIMPLIFY );
}


//-----------------------------------------------------------------------------

float CAI_Navigator::GetPathDistToCurWaypoint() const
{
	return ( GetPath()->GetCurWaypoint() ) ? 
				ComputePathDistance( GetNavType(), GetLocalOrigin(), GetPath()->CurWaypointPos() ) :
				0;
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::PrependLocalAvoidance( float distObstacle, const AIMoveTrace_t &directTrace )
{
	if ( AIStrongOpt() )
		return false;

	if ( GetOuter()->IsFlaggedEfficient() )
		return false;

	if ( m_flTimeLastAvoidanceTriangulate >= gpGlobals->curtime )
		return false; // Only triangulate once per think at most

	m_flTimeLastAvoidanceTriangulate = gpGlobals->curtime;

	AI_Waypoint_t *pAvoidanceRoute = NULL;

	Vector vStart = GetLocalOrigin();

	if ( distObstacle < GetHullWidth() * 0.5 )
	{
		AIMoveTrace_t backawayTrace;
		Vector vTestBackaway = GetCurWaypointPos() - GetLocalOrigin();
		VectorNormalize( vTestBackaway );
		vTestBackaway *= -GetHullWidth();
		vTestBackaway += GetLocalOrigin();

		int flags = ( GetNavType() == NAV_GROUND ) ? AIMLF_2D : AIMLF_DEFAULT;

		if ( GetMoveProbe()->MoveLimit( GetNavType(), GetLocalOrigin(), vTestBackaway, 
													 MASK_NPCSOLID, GetNavTargetEntity(), 
													 100.0, 
													 flags, &backawayTrace ) )
		{
			vStart = backawayTrace.vEndPosition;
			pAvoidanceRoute = new AI_Waypoint_t( vStart, 0, GetNavType(), bits_WP_TO_DETOUR, NO_NODE );
		}
	}

	AI_Waypoint_t *pTriangulation = GetPathfinder()->BuildTriangulationRoute(
		vStart, 
		GetCurWaypointPos(), 
		GetNavTargetEntity(), 
		bits_WP_TO_DETOUR, 
		NO_NODE,
		0.0, 
		distObstacle, 
		GetNavType() );
	
	if ( !pTriangulation )
	{
		delete pAvoidanceRoute;
		return false;
	}

	if ( pAvoidanceRoute )
		pAvoidanceRoute->SetNext( pTriangulation );
	else
		pAvoidanceRoute = pTriangulation;

	// @TODO (toml 02-04-04): it would be better to do this on each triangulation test to
	// improve the odds of success. however, difficult in current structure
	float moveThisInterval = GetMotor()->CalcIntervalMove();
	Vector dir = pAvoidanceRoute->GetPos() - GetLocalOrigin();
	float dist = VectorNormalize( dir );
	Vector testPos;
	if ( dist > moveThisInterval )
	{
		dist = moveThisInterval;
		testPos = GetLocalOrigin() + dir * dist;
	}
	else
	{
		testPos = pAvoidanceRoute->GetPos();
	}

	int flags = ( GetNavType() == NAV_GROUND ) ? AIMLF_2D : AIMLF_DEFAULT;

	if ( !GetMoveProbe()->MoveLimit( GetNavType(), GetLocalOrigin(), testPos, 
												 MASK_NPCSOLID, GetNavTargetEntity(), 
												 100.0, 
												 flags ) )
	{
		DeleteAll( pAvoidanceRoute );
		return false;
	}
		
	// FIXME: should the route get simplified? 
	GetPath()->PrependWaypoints( pAvoidanceRoute );
	return true;
}


//-----------------------------------------------------------------------------

CEntity *CAI_Navigator::GetNavTargetEntity()
{
	if ( GetGoalType() == GOALTYPE_ENEMY || GetGoalType() == GOALTYPE_TARGETENT )
		return GetOuter()->GetNavTargetEntity();
	return GetPath()->GetTarget();
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::SetRandomGoal( const Vector &from, float minPathLength, const Vector &dir )
{
	OnNewGoal();
	if ( GetNetwork()->NumNodes() <= 0 )
		return false;

	INearestNodeFilter *pFilter = NULL;
	CAI_NavInHintGroupFilter filter;
	if ( GetOuter()->GetHintGroup() != NULL_STRING )
	{
		filter.m_iszGroup = GetOuter()->GetHintGroup();
		pFilter = &filter;
	}
		
	int fromNodeID = GetNetwork()->NearestNodeToPoint( GetOuter(), from, true, pFilter );
	
	if (fromNodeID == NO_NODE)
		return false;
		
	AI_Waypoint_t* pRoute = GetPathfinder()->FindShortRandomPath( fromNodeID, minPathLength, dir );

	if (!pRoute)
		return false;

	GetPath()->SetGoalType(GOALTYPE_LOCATION);
	GetPath()->SetWaypoints(pRoute);
	GetPath()->SetLastNodeAsGoal();
	GetPath()->SetGoalTolerance( GetOuter()->GetDefaultNavGoalTolerance() );

	SimplifyPath( true );
	
	return true;
}


//-----------------------------------------------------------------------------

bool CAI_Navigator::SetRandomGoal( float minPathLength, const Vector &dir )
{
	return SetRandomGoal( GetLocalOrigin(), minPathLength, dir );
}

//-----------------------------------------------------------------------------

bool CAI_Navigator::UpdateGoalPos( const Vector &goalPos )
{
	return g_helpfunc.CAI_Navigator_UpdateGoalPos(this, goalPos);;	
}

void CAI_Navigator::SetArrivalDirection( const Vector &goalDirection )
{ 
	GetPath()->SetGoalDirection( goalDirection );
}


void CAI_Navigator::SetArrivalDirection( CEntity * pTarget )
{
	GetPath()->SetGoalDirection( pTarget );
}


void CAI_Navigator::StopMoving( bool bImmediate )
{
	if ( IsGoalSet() )
	{
		if ( bImmediate || !SetGoalFromStoppingPath() )
		{
			OnNavComplete();
		}
	}
	else
		ClearGoal();
}

bool CAI_Navigator::SetGoalFromStoppingPath()
{
	if ( m_pClippedWaypoints && m_pClippedWaypoints->IsEmpty() )
		SaveStoppingPath();
	if ( m_pClippedWaypoints && !m_pClippedWaypoints->IsEmpty() )
	{
		if ( m_PreviousMoveActivity <= ACT_RESET && GetMovementActivity() <= ACT_RESET  )
		{
			m_pClippedWaypoints->RemoveAll();
			return false;
		}

		if ( ( m_pClippedWaypoints->GetFirst()->NavType() == NAV_CLIMB || m_pClippedWaypoints->GetFirst()->NavType() == NAV_JUMP ) )
		{
			const Task_t *pCurTask = GetOuter()->GetTask();
			if ( pCurTask && pCurTask->iTask == TASK_STOP_MOVING )
			{
				// Clipped paths are used for 2 reasons: Prepending movement that must be finished in the case of climbing/jumping,
				// and bringing an NPC to a stop. In the second case, we should never be starting climb or jump movement.
				m_pClippedWaypoints->RemoveAll();
				return false;
			}
		}

		if ( !GetPath()->IsEmpty() )
			GetPath()->ClearWaypoints();
		GetPath()->SetWaypoints( m_pClippedWaypoints->GetFirst(), true );
		m_pClippedWaypoints->Set( NULL );
		GetPath()->SetGoalType( GOALTYPE_NONE );
		GetPath()->SetGoalType( GOALTYPE_LOCATION );
		GetPath()->SetGoalTolerance( NAV_STOP_MOVING_TOLERANCE );
		Assert( GetPath()->GetCurWaypoint() );

		Assert( m_PreviousMoveActivity != ACT_INVALID );
		

		if ( m_PreviousMoveActivity != ACT_RESET )
			GetPath()->SetMovementActivity( m_PreviousMoveActivity );
		if ( m_PreviousArrivalActivity != ACT_RESET )
			GetPath()->SetArrivalActivity( m_PreviousArrivalActivity );
		return true;
	}
	return false;
}


void CAI_Navigator::PrependWaypoint( const Vector &newPoint, Navigation_t navType, unsigned waypointFlags )
{
	GetPath()->PrependWaypoint( newPoint, navType, waypointFlags );
}

int CAI_Navigator::GetCurWaypointFlags() const
{
	return GetPath()->CurWaypointFlags();
}

void CAI_Navigator::SetArrivalActivity(Activity activity)
{
	GetPath()->SetArrivalActivity(activity);
}


void CAI_Navigator::SetArrivalSequence( int sequence )
{
	GetPath()->SetArrivalActivity( ACT_INVALID );
	GetPath()->SetArrivalSequence( sequence );
}


bool CAI_Navigator::OnMoveBlocked( AIMoveResult_t *pResult )
{
	if ( *pResult == AIMR_BLOCKED_NPC && 
		 GetPath()->GetCurWaypoint() &&
		 ( GetPath()->GetCurWaypoint()->Flags() & bits_WP_TO_DOOR ) )
	{
		CEntity *cent = CEntity::Instance((CBaseEntity *)GetPath()->GetCurWaypoint()->GetEHandleData());
		CPropDoor *pDoor = (CPropDoor *)cent;
		if (pDoor != NULL)
		{
			GetOuter()->OpenPropDoorBegin( pDoor );
			*pResult = AIMR_OK;
			return true;
		}
	}


	// Allow the NPC to override this behavior
	if ( GetOuter()->OnMoveBlocked( pResult ))
		return true;

	float flWaypointDist;

	if ( !GetPath()->CurWaypointIsGoal() && GetPath()->GetCurWaypoint()->IsReducible() )
	{
		flWaypointDist = ComputePathDistance( GetNavType(), GetLocalOrigin(), GetCurWaypointPos() );
		if ( flWaypointDist < GetHullWidth() )
		{
			AdvancePath();
			*pResult = AIMR_CHANGE_TYPE;
		}
	}

	SetActivity( GetOuter()->GetStoppedActivity() );

	const float EPS = 0.1;
	
	flWaypointDist = ComputePathDistance( GetNavType(), GetLocalOrigin(), GetPath()->ActualGoalPosition() );

	if ( flWaypointDist < GetGoalTolerance() + EPS )
	{
		OnNavComplete();
		*pResult = AIMR_OK;
		return true;
	}

	return false;
}

float CAI_Navigator::GetGoalTolerance() const
{
	return GetPath()->GetGoalTolerance(); 
}

bool CAI_Navigator::GetPointAlongPath( Vector *pResult, float distance, bool fReducibleOnly )
{
	if ( !GetPath()->GetCurWaypoint() )
		return false;

	AI_Waypoint_t *pCurWaypoint	 = GetPath()->GetCurWaypoint();
	AI_Waypoint_t *pEndPoint 	 = pCurWaypoint;
	float 		   distRemaining = distance;
	float 		   distToNext;
	Vector		   vPosPrev		 = GetLocalOrigin();

	while ( pEndPoint->GetNext() )
	{
		distToNext = ComputePathDistance( GetNavType(), vPosPrev, pEndPoint->GetPos() );
		
		if ( distToNext > distRemaining)
			break;
		
		distRemaining -= distToNext;
		vPosPrev = pEndPoint->GetPos();
		if ( fReducibleOnly && !pEndPoint->IsReducible() )
			break;
		pEndPoint = pEndPoint->GetNext();
	}
	
	Vector &result = *pResult;
	float distToEnd = ComputePathDistance( GetNavType(), vPosPrev, pEndPoint->GetPos() ); 
	if ( distToEnd - distRemaining < 0.1 )
	{
		result = pEndPoint->GetPos();
	}
	else
	{
		result = pEndPoint->GetPos() - vPosPrev;
		VectorNormalize( result );
		result *= distRemaining;
		result += vPosPrev;
	}

	return true;
}

float CAI_Navigator::GetPathTimeToGoal()
{
	if ( GetOuter()->m_flGroundSpeed )
		return (GetPathDistanceToGoal() / GetOuter()->m_flGroundSpeed);
	return 0;
}

void CAI_Navigator::SetArrivalDistance( float flDistance )
{
	GetPath()->SetGoalStoppingDistance( flDistance );
}

float CAI_Navigator::GetPathDistanceToGoal()
{
	return GetPath()->GetPathDistanceToGoal(GetAbsOrigin());
}

void CAI_Navigator::SetArrivalDirection( const QAngle &goalAngle ) 
{ 
	Vector goalDirection;

	AngleVectors( goalAngle, &goalDirection );

	GetPath()->SetGoalDirection( goalDirection );
}


