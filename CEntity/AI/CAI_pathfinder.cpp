
#include "CEntity.h"
#include "CAI_pathfinder.h"
#include "CAI_Network.h"
#include "CAI_Node.h"
#include "CAI_NPC.h"
#include "CAI_Link.h"
#include "CDynamicLink.h"
#include "CAI_Hint.h"
#include "ai_waypoint.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



//-----------------------------------------------------------------------------
// Purpose: Build a path between two nodes
//-----------------------------------------------------------------------------

AI_Waypoint_t *CAI_Pathfinder::FindBestPath(int startID, int endID) 
{
	if ( !GetNetwork()->NumNodes() )
		return NULL;

	int nNodes = GetNetwork()->NumNodes();
	CAI_Node **pAInode = GetNetwork()->AccessNodes();

	CVarBitVec	openBS(nNodes);
	CVarBitVec	closeBS(nNodes);

	// ------------- INITIALIZE ------------------------
	float* nodeG = (float *)stackalloc( nNodes * sizeof(float) );
	float* nodeH = (float *)stackalloc( nNodes * sizeof(float) );
	float* nodeF = (float *)stackalloc( nNodes * sizeof(float) );
	int*   nodeP = (int *)stackalloc( nNodes * sizeof(int) );		// Node parent 

	for (int node=0;node<nNodes;node++)
	{
		nodeG[node] = FLT_MAX;
		nodeP[node] = -1;
	}

	nodeG[startID] = 0;

	nodeH[startID] = 0.1*(pAInode[startID]->GetPosition(GetHullType())-pAInode[endID]->GetPosition(GetHullType())).Length(); // Don't want to over estimate
	nodeF[startID] = nodeG[startID] + nodeH[startID];

	openBS.Set(startID);
	closeBS.Set( startID );

	// --------------- FIND BEST PATH ------------------
	while (!openBS.IsAllClear()) 
	{
		int smallestID = CAI_Network::FindBSSmallest(&openBS,nodeF,nNodes);
	
		openBS.Clear(smallestID);

		CAI_Node *pSmallestNode = pAInode[smallestID];
		
		if (GetOuter()->IsUnusableNode(smallestID, pSmallestNode->m_pHint))
			continue;

		if (smallestID == endID) 
		{
			AI_Waypoint_t* route = MakeRouteFromParents(&nodeP[0], endID);
			return route;
		}

		// Check this if the node is immediately in the path after the startNode 
		// that it isn't blocked
		for (int link=0; link < pSmallestNode->NumLinks();link++) 
		{
			CAI_Link *nodeLink = pSmallestNode->GetLinkByIndex(link);
			
			if (!IsLinkUsable(nodeLink,smallestID))
				continue;

			// FIXME: the cost function should take into account Node costs (danger, flanking, etc).
			int moveType = nodeLink->m_iAcceptedMoveTypes[GetHullType()] & CapabilitiesGet();
			int testID	 = nodeLink->DestNodeID(smallestID);

			Vector r1 = pSmallestNode->GetPosition(GetHullType());
			Vector r2 = pAInode[testID]->GetPosition(GetHullType());
			float dist   = GetOuter()->GetNavigator()->MovementCost( moveType, r1, r2 ); // MovementCost takes ref parameters!!

			if ( dist == FLT_MAX )
				continue;

			float new_g  = nodeG[smallestID] + dist;

			if ( !closeBS.IsBitSet(testID) || (new_g < nodeG[testID]) ) 
			{
				nodeP[testID] = smallestID;
				nodeG[testID] = new_g;
				nodeH[testID] = (pAInode[testID]->GetPosition(GetHullType())-pAInode[endID]->GetPosition(GetHullType())).Length();
				nodeF[testID] = nodeG[testID] + nodeH[testID];

				closeBS.Set( testID );
				openBS.Set( testID );
			}
		}
	}

	return NULL;   
}

int CAI_Pathfinder::NearestNodeToPoint( const Vector &vecOrigin )
{
	return GetNetwork()->NearestNodeToPoint( GetOuter(), vecOrigin );
}

int	CAI_Pathfinder::NearestNodeToNPC()
{
	return GetNetwork()->NearestNodeToPoint( GetOuter(), GetAbsOrigin() );
}


//------------------------------------------------------------------------------
// Purpose : Returns true is link us usable by the given NPC from the
//			 startID node.
//------------------------------------------------------------------------------

bool CAI_Pathfinder::IsLinkUsable(CAI_Link *pLink, int startID)
{
	// --------------------------------------------------------------------------
	// Skip if link turned off
	// --------------------------------------------------------------------------
	if (pLink->m_LinkInfo & bits_LINK_OFF)
	{
		CDynamicLink *pDynamicLink = dynamic_cast<CDynamicLink *>(CEntity::Instance(pLink->m_pDynamicLink));

		if ( !pDynamicLink || *(pDynamicLink->m_strAllowUse.ptr) == NULL_STRING )
			return false;

		const char *pszAllowUse = STRING( *(pDynamicLink->m_strAllowUse) );
		if ( *(pDynamicLink->m_bInvertAllow) )
		{
			// Exlude only the specified entity name or classname
			if ( GetOuter()->NameMatches(pszAllowUse) || GetOuter()->ClassMatches( pszAllowUse ) )
				return false;
		}
		else
		{
			// Exclude everything but the allowed entity name or classname
			if ( !GetOuter()->NameMatches( pszAllowUse) && !GetOuter()->ClassMatches( pszAllowUse ) )
				return false;
		}
	}

	// --------------------------------------------------------------------------			
	//  Get the destination nodeID
	// --------------------------------------------------------------------------			
	int endID = pLink->DestNodeID(startID);

	// --------------------------------------------------------------------------
	// Make sure I have the ability to do the type of movement specified by the link
	// --------------------------------------------------------------------------
	int linkMoveTypes = pLink->m_iAcceptedMoveTypes[GetHullType()];
	int moveType = ( linkMoveTypes & CapabilitiesGet() );

	CAI_Node *pStartNode,*pEndNode;

	pStartNode = GetNetwork()->GetNode(startID);
	pEndNode = GetNetwork()->GetNode(endID);

	if ( (linkMoveTypes & bits_CAP_MOVE_JUMP) && !moveType )
	{
		CE_AI_Hint *pStartHint = dynamic_cast<CE_AI_Hint *>(pStartNode->GetHint());
		CE_AI_Hint *pEndHint = dynamic_cast<CE_AI_Hint *>(pEndNode->GetHint());
		if ( pStartHint && pEndHint )
		{
			if ( pStartHint->HintType() == HINT_JUMP_OVERRIDE && 
				 pEndHint->HintType() == HINT_JUMP_OVERRIDE &&
				 ( ( ( pStartHint->GetSpawnFlags() | pEndHint->GetSpawnFlags() ) & SF_ALLOW_JUMP_UP ) || pStartHint->GetAbsOrigin().z > pEndHint->GetAbsOrigin().z ) )
			{
				if ( !pStartNode->IsLocked() )
				{
					if ( pStartHint->GetTargetNode() == -1 || pStartHint->GetTargetNode() == endID )
						moveType = bits_CAP_MOVE_JUMP;
				}
			}
		}
	}

	if (!moveType)
	{
		return false;
	}

	// --------------------------------------------------------------------------
	// Check if NPC has a reason not to use the desintion node
	// --------------------------------------------------------------------------
	CEntity *cent = pEndNode->GetHint();

	if (GetOuter()->IsUnusableNode(endID, ((cent) ? cent->BaseEntity() : NULL)))
	{
		return false;
	}	

	// --------------------------------------------------------------------------
	// If a jump make sure the jump is within NPC's legal parameters for jumping
	// --------------------------------------------------------------------------
	if (moveType == bits_CAP_MOVE_JUMP)
	{	
		if (!GetOuter()->IsJumpLegal(pStartNode->GetPosition(GetHullType()), 
									 pEndNode->GetPosition(GetHullType()),
									 pEndNode->GetPosition(GetHullType())))
		{
			return false;
		}
	}

	// --------------------------------------------------------------------------
	// If an NPC suggested that this link is stale and I haven't checked it yet
	// I should make sure the link is still valid before proceeding
	// --------------------------------------------------------------------------
	if (pLink->m_LinkInfo & bits_LINK_STALE_SUGGESTED)
	{
		if (IsLinkStillStale(moveType, pLink))
		{
			return false;
		}
	}
	return true;
}


//------------------------------------------------------------------------------
// Purpose : Test if stale link is no longer stale
//------------------------------------------------------------------------------

bool CAI_Pathfinder::IsLinkStillStale(int moveType, CAI_Link *nodeLink)
{
	if ( m_bIgnoreStaleLinks )
		return false;

	if ( !(nodeLink->m_LinkInfo & bits_LINK_STALE_SUGGESTED ) )
		return false;

	if ( gpGlobals->curtime < nodeLink->m_timeStaleExpires )
		return true;

	// NPC should only check one stale link per think
	if (gpGlobals->curtime == m_flLastStaleLinkCheckTime)
	{
		return true;
	}
	else
	{
		m_flLastStaleLinkCheckTime = gpGlobals->curtime;
	}
	
	// Test movement, if suceeds, clear the stale bit
	if (CheckStaleRoute(GetNetwork()->GetNode(nodeLink->m_iSrcID)->GetPosition(GetHullType()),
		GetNetwork()->GetNode(nodeLink->m_iDestID)->GetPosition(GetHullType()), moveType))
	{
		nodeLink->m_LinkInfo &= ~bits_LINK_STALE_SUGGESTED;
		return false;
	}

	nodeLink->m_timeStaleExpires = gpGlobals->curtime + 1.0;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Checks if a local route (not using nodes) between vStart
//			and vEnd exists using the moveType
// Input  :
// Output : Returns a route if sucessful or NULL if no local route was possible
//-----------------------------------------------------------------------------
bool CAI_Pathfinder::CheckStaleRoute(const Vector &vStart, const Vector &vEnd, int moveTypes)
{
	// -------------------------------------------------------------------
	// First try to go there directly
	// -------------------------------------------------------------------
	if (moveTypes & bits_CAP_MOVE_GROUND) 
	{
		if (CheckStaleNavTypeRoute( NAV_GROUND, vStart, vEnd ))
			return true;
	}

	// -------------------------------------------------------------------
	// First try to go there directly
	// -------------------------------------------------------------------
	if (moveTypes & bits_CAP_MOVE_FLY) 
	{
		if (CheckStaleNavTypeRoute( NAV_FLY, vStart, vEnd ))
			return true;
	}

	// --------------------------------------------------------------
	//  Try to jump if we can jump to a node
	// --------------------------------------------------------------
	if (moveTypes & bits_CAP_MOVE_JUMP)
	{
		AIMoveTrace_t moveTrace;
		GetOuter()->GetMoveProbe()->MoveLimit( NAV_JUMP, vStart, vEnd, MASK_NPCSOLID, NULL, &moveTrace);
		if (!IsMoveBlocked(moveTrace))
		{
			return true;
		}
		else
		{
			// Can't tell jump up from jump down at this point
			GetOuter()->GetMoveProbe()->MoveLimit( NAV_JUMP, vEnd, vStart, MASK_NPCSOLID, NULL, &moveTrace);
			if (!IsMoveBlocked(moveTrace))
				return true;
		}
	}

	// --------------------------------------------------------------
	//  Try to climb if we can climb to a node
	// --------------------------------------------------------------
	if (moveTypes & bits_CAP_MOVE_CLIMB)
	{
		AIMoveTrace_t moveTrace;
		GetOuter()->GetMoveProbe()->MoveLimit( NAV_CLIMB, vStart, vEnd, MASK_NPCSOLID, NULL, &moveTrace);
		if (!IsMoveBlocked(moveTrace))
		{	
			return true;
		}
	}

	// Man do we suck! Couldn't get there by any route
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Given an array of parentID's and endID, contruct a linked 
//			list of waypoints through those parents
//-----------------------------------------------------------------------------
AI_Waypoint_t* CAI_Pathfinder::MakeRouteFromParents( int *parentArray, int endID ) 
{
	AI_Waypoint_t *pOldWaypoint = NULL;
	AI_Waypoint_t *pNewWaypoint = NULL;
	int	currentID = endID;

	CAI_Node **pAInode = GetNetwork()->AccessNodes();

	while (currentID != NO_NODE) 
	{
		// Try to link it to the previous waypoint
		int prevID = parentArray[currentID];

		int destID; 
		if (prevID != NO_NODE)
		{
			destID = prevID;
		}
		else
		{		   
			// If we have no previous node, then use the next node
			if ( !pOldWaypoint )
				return NULL;
			destID = pOldWaypoint->iNodeID;
		}

		Navigation_t waypointType = ComputeWaypointType( pAInode, currentID, destID );

		// BRJ 10/1/02
		// FIXME: It appears potentially possible for us to compute waypoints 
		// here which the NPC is not capable of traversing (because 
		// pNPC->CapabilitiesGet() in ComputeWaypointType() above filters it out). 
		// It's also possible if none of the lines have an appropriate DestNodeID.
		// Um, shouldn't such a waypoint not be allowed?!?!?
		Assert( waypointType != NAV_NONE );

		pNewWaypoint = new AI_Waypoint_t( pAInode[currentID]->GetPosition(GetHullType()),
			pAInode[currentID]->GetYaw(), waypointType, bits_WP_TO_NODE, currentID );

		// Link it up...
		pNewWaypoint->SetNext( pOldWaypoint );
		pOldWaypoint = pNewWaypoint;

		currentID = prevID;
	}

	return pOldWaypoint;
}


//-----------------------------------------------------------------------------
// Checks a stale navtype route 
//-----------------------------------------------------------------------------
bool CAI_Pathfinder::CheckStaleNavTypeRoute( Navigation_t navType, const Vector &vStart, const Vector &vEnd )
{
	AIMoveTrace_t moveTrace;
	GetOuter()->GetMoveProbe()->MoveLimit( navType, vStart, vEnd, MASK_NPCSOLID, NULL, 100, AIMLF_IGNORE_TRANSIENTS, &moveTrace);

	// Is the direct route clear?
	if (!IsMoveBlocked(moveTrace))
	{
		return true;
	}

	// Next try to triangulate
	// FIXME: Since blocked dist is an unreliable number, this computation is bogus
	Vector vecDelta;
	VectorSubtract( vEnd, vStart, vecDelta );
	float flTotalDist = vecDelta.Length();

	Vector vApex;
	if (Triangulate( navType, vStart, vEnd, flTotalDist - moveTrace.flDistObstructed, NULL, &vApex ))
	{
		return true;
	}

	CEntity *cent = CEntity::Instance(moveTrace.pObstruction);
	// Try a giveway request, if I can get there ignoring NPCs
	if ( cent && cent->MyNPCPointer() )
	{
		GetOuter()->GetMoveProbe()->MoveLimit( navType, vStart, vEnd, MASK_NPCSOLID_BRUSHONLY, NULL, &moveTrace);

		if (!IsMoveBlocked(moveTrace))
		{
			return true;
		}
	}

	return false;
}



//-----------------------------------------------------------------------------
// Compute move type bits to nav type
//-----------------------------------------------------------------------------
Navigation_t MoveBitsToNavType( int fBits )
{
	switch (fBits)
	{
	case bits_CAP_MOVE_GROUND:
		return NAV_GROUND;

	case bits_CAP_MOVE_FLY:
		return NAV_FLY;

	case bits_CAP_MOVE_CLIMB:
		return NAV_CLIMB;

	case bits_CAP_MOVE_JUMP:
		return NAV_JUMP;

	default:
		// This will only happen if more than one bit is set
		Assert(0);
		return NAV_NONE;
	}
}



//-----------------------------------------------------------------------------
// Computes the link type
//-----------------------------------------------------------------------------
Navigation_t CAI_Pathfinder::ComputeWaypointType( CAI_Node **ppNodes, int parentID, int destID )
{
	Navigation_t navType = NAV_NONE;

	CAI_Node *pNode = ppNodes[parentID];
	for (int link=0; link < pNode->NumLinks();link++) 
	{
		if (pNode->GetLinkByIndex(link)->DestNodeID(parentID) == destID)
		{
			// BRJ 10/1/02
			// FIXME: pNPC->CapabilitiesGet() is actually the mechanism by which fliers
			// filter out the bitfields in the waypoint type (most importantly, bits_MOVE_CAP_GROUND)
			// that would cause the waypoint distance to be computed in a 2D, as opposed to 3D fashion
			// This is a super-scary weak link if you ask me.
			int linkMoveTypeBits = pNode->GetLinkByIndex(link)->m_iAcceptedMoveTypes[GetHullType()];
			int moveTypeBits = ( linkMoveTypeBits & CapabilitiesGet());
			if ( !moveTypeBits && linkMoveTypeBits == bits_CAP_MOVE_JUMP )
			{
				Assert( pNode->GetHint() && pNode->GetHint()->HintType() == HINT_JUMP_OVERRIDE );
				ppNodes[destID]->Lock(0.3);
				moveTypeBits = linkMoveTypeBits;
			}
			Navigation_t linkType = MoveBitsToNavType( moveTypeBits );

			// This will only trigger if the links disagree about their nav type
			Assert( (navType == NAV_NONE) || (navType == linkType) );
			navType = linkType; 
			break;
		}
	}

	// @TODO (toml 10-15-02): one would not expect to come out of the above logic
	// with NAV_NONE. However, if a graph is newly built, it can contain malformed
	// links that are referred to by the destination node, not the source node.
	// This has to be fixed
	if ( navType == NAV_NONE )
	{
		pNode = ppNodes[destID];
		for (int link=0; link < pNode->NumLinks();link++) 
		{
			if (pNode->GetLinkByIndex(link)->DestNodeID(parentID) == destID)
			{
				int npcMoveBits = CapabilitiesGet();
				int nodeMoveBits = pNode->GetLinkByIndex(link)->m_iAcceptedMoveTypes[GetHullType()];
				int moveTypeBits = ( npcMoveBits & nodeMoveBits );
				Navigation_t linkType = MoveBitsToNavType( moveTypeBits );

				Assert( (navType == NAV_NONE) || (navType == linkType) );
				navType = linkType; 

				DevMsg( "Note: Strange link found between nodes in AI node graph\n" );
				break;
			}
		}
	}

	AssertMsg( navType != NAV_NONE, "Pathfinder appears to have output a path with consecutive nodes thate are not actually connected\n" );

	return navType;
}


//-----------------------------------------------------------------------------
// Purpose: tries to overcome local obstacles by triangulating a path around them.
// Input  : flDist is is how far the obstruction that we are trying
//			to triangulate around is from the npc
// Output :
//-----------------------------------------------------------------------------

// FIXME: this has no concept that the blocker may not be exactly along the vecStart, vecEnd vector.
// FIXME: it should take a position (and size?) to avoid
// FIXME: this does the position checks in the same order as GiveWay() so they tend to fight each other when both are active
#define MAX_TRIAGULATION_DIST (32*12)
bool CAI_Pathfinder::Triangulate( Navigation_t navType, const Vector &vecStart, const Vector &vecEndIn, 
	float flDistToBlocker, const CEntity *pTargetEnt, Vector *pApex )
{
	if ( GetOuter()->IsFlaggedEfficient() )
		return false;

	Assert( pApex );

	Vector vecForward, vecUp, vecPerpendicular;
	VectorSubtract( vecEndIn, vecStart, vecForward );
	float flTotalDist = VectorNormalize( vecForward );

	Vector vecEnd;

	// If we're walking, then don't try to triangulate over large distances
	if ( navType != NAV_FLY && flTotalDist > MAX_TRIAGULATION_DIST)
	{
		vecEnd = vecForward * MAX_TRIAGULATION_DIST;
		flTotalDist = MAX_TRIAGULATION_DIST;
		if ( !GetOuter()->GetMoveProbe()->MoveLimit(navType, vecEnd, vecEndIn, MASK_NPCSOLID, pTargetEnt) )
		{
			return false;
		}

	}
	else
		vecEnd = vecEndIn;

	// Compute a direction vector perpendicular to the desired motion direction
	if ( 1.0f - fabs(vecForward.z) > 1e-3 )
	{
		vecUp.Init( 0, 0, 1 );
		CrossProduct( vecForward, vecUp, vecPerpendicular );	// Orthogonal to facing
	}
	else
	{
		vecUp.Init( 0, 1, 0 );
		vecPerpendicular.Init( 1, 0, 0 ); 
	}

	// Grab the size of the navigation bounding box
	float sizeX = 0.5f * NAI_Hull::Length(GetHullType());
	float sizeZ = 0.5f * NAI_Hull::Height(GetHullType());

	// start checking right about where the object is, picking two equidistant
	// starting points, one on the left, one on the right. As we progress 
	// through the loop, we'll push these away from the obstacle, hoping to 
	// find a way around on either side. m_vecSize.x is added to the ApexDist 
	// in order to help select an apex point that insures that the NPC is 
	// sufficiently past the obstacle before trying to turn back onto its original course.


	float flApexDist = flDistToBlocker + sizeX;
	if (flApexDist > flTotalDist) 
	{
		flApexDist = flTotalDist;
	}

	// Compute triangulation apex points (NAV_FLY attempts vertical triangulation too)
	Vector vecDelta[2];
	Vector vecApex[4];
	float pApexDist[4];

	Vector vecCenter;
	int nNumToTest = 2;
	VectorMultiply( vecPerpendicular, sizeX, vecDelta[0] );

	VectorMA( vecStart, flApexDist, vecForward, vecCenter );
	VectorSubtract( vecCenter, vecDelta[0], vecApex[0] );
	VectorAdd( vecCenter, vecDelta[0], vecApex[1] );
 	vecDelta[0] *= 2.0f;
	pApexDist[0] = pApexDist[1] = flApexDist;

	if (navType == NAV_FLY)
	{
		VectorMultiply( vecUp, 3.0f * sizeZ, vecDelta[1] );
		VectorSubtract( vecCenter, vecDelta[1], vecApex[2] );
		VectorAdd( vecCenter, vecDelta[1], vecApex[3] );
		pApexDist[2] = pApexDist[3] = flApexDist;
		nNumToTest = 4;
	}

	AIMoveTrace_t moveTrace;
	for (int i = 0; i < 2; ++i )
	{
		// NOTE: Do reverse order so fliers try to move over the top first 
		for (int j = nNumToTest; --j >= 0; )
		{
			if (TestTriangulationRoute(navType, vecStart, vecApex[j], vecEnd, pTargetEnt, &moveTrace))
			{
				*pApex  = vecApex[j];
				return true;
			}

			// Here, the starting half of the triangle was blocked. Lets
			// pull back the apex toward the start...
			if (IsMoveBlocked(moveTrace))
			{
				Vector vecStartToObstruction;
				VectorSubtract( moveTrace.vEndPosition, vecStart, vecStartToObstruction );
				float flDistToObstruction = DotProduct( vecStartToObstruction, vecForward );

				float flNewApexDist = pApexDist[j];
				if (pApexDist[j] > flDistToObstruction)
					flNewApexDist = flDistToObstruction;

				VectorMA( vecApex[j], flNewApexDist - pApexDist[j], vecForward, vecApex[j] );
				pApexDist[j] = flNewApexDist;
			}

			// NOTE: This has to occur *after* the code above because
			// the above code uses vecApex for some distance computations
			if (j & 0x1)
				vecApex[j] += vecDelta[j >> 1];
			else
				vecApex[j] -= vecDelta[j >> 1];
		}
	}

	return false;
}



//-----------------------------------------------------------------------------
// Test the triangulation route...
//-----------------------------------------------------------------------------
#ifdef _WIN32
#pragma warning (disable:4701)
#endif

bool CAI_Pathfinder::TestTriangulationRoute( Navigation_t navType, const Vector& vecStart, 
	const Vector &vecApex, const Vector &vecEnd, const CEntity *pTargetEnt, AIMoveTrace_t *pStartTrace )
{
	AIMoveTrace_t endTrace;
	endTrace.fStatus = AIMR_OK;	// just to make the debug overlay code easy

	// Check the triangulation
	CAI_MoveProbe *pMoveProbe = GetOuter()->GetMoveProbe();

	bool bPathClear = false;

	// See if we can get from the start point to the triangle apex
	if ( pMoveProbe->MoveLimit(navType, vecStart, vecApex, MASK_NPCSOLID, pTargetEnt, pStartTrace ) )
	{
		// Ok, we got from the start to the triangle apex, now try
		// the triangle apex to the end
		if ( pMoveProbe->MoveLimit(navType, vecApex, vecEnd, MASK_NPCSOLID, pTargetEnt, &endTrace ) )
		{
			bPathClear = true;
		}
	}

	return bPathClear;
}

#ifdef _WIN32
#pragma warning (default:4701)
#endif


//-----------------------------------------------------------------------------
// Purpose: Find a short random path of at least pathLength distance.  If
//			vDirection is given random path will expand in the given direction,
//			and then attempt to go generally straight
//-----------------------------------------------------------------------------

AI_Waypoint_t* CAI_Pathfinder::FindShortRandomPath(int startID, float minPathLength, const Vector &directionIn) 
{
	int				pNeighbor[AI_MAX_NODE_LINKS];
	int				pStaleNeighbor[AI_MAX_NODE_LINKS];
	int				numNeighbors		= 1;	// The start node
	int				numStaleNeighbors	= 0;
	int				neighborID			= NO_NODE;

	int nNodes = GetNetwork()->NumNodes();
	CAI_Node **pAInode = GetNetwork()->AccessNodes();

	if ( !nNodes )
		return NULL;
	
	MARK_TASK_EXPENSIVE();

	int *nodeParent	= (int *)stackalloc( sizeof(int) * nNodes );
	CVarBitVec closeBS(nNodes);
	Vector vDirection = directionIn;

	// ------------------------------------------
	// Bail immediately if node has no neighbors
	// ------------------------------------------
	if (pAInode[startID]->NumLinks() == 0)
	{
		return NULL;
	}

	// ------------- INITIALIZE ------------------------
	nodeParent[startID] = NO_NODE;
	pNeighbor[0]		= startID;

	// --------------- FIND PATH ---------------------------------------------------------------
	// Quit when path is long enough, and I've run out of neighbors unless I'm on a climb node
	// in which case I'm not allowed to stop
	// -----------------------------------------------------------------------------------------
	float	pathLength	 = 0;
	int		nSearchCount = 0;
	while ( (pathLength < minPathLength) || 
			(neighborID != NO_NODE && pAInode[neighborID]->GetType() == NODE_CLIMB))
	{
		nSearchCount++;

		// If no neighbors try circling back to last node
		if (neighborID			!= NO_NODE	&&
			numNeighbors		== 0		&& 
			numStaleNeighbors	== 0		)
		{
			// If we dead ended on a climb node we've failed as we
			// aren't allowed to stop on a climb node
			if (pAInode[neighborID]->GetType() == NODE_CLIMB)
			{
				// If no neighbors exist we've failed.
				return NULL;
			}
			// Otherwise accept this path to a dead end
			else
			{
				AI_Waypoint_t* route = MakeRouteFromParents(&nodeParent[0], neighborID);
				return route;
			}
		}

		// ----------------------
		//  Pick a neighbor
		// ----------------------
		int lastID = neighborID;

		// If vDirection is non-zero attempt to expand close to current direction
		if (vDirection != vec3_origin)
		{
			float	bestDot		= -1;
			Vector	vLastPos;

			if (lastID == NO_NODE)
			{
				vLastPos = GetLocalOrigin();
			}
			else
			{
				vLastPos = pAInode[lastID]->GetOrigin();
			}

			// If no neighbors, try using a stale one
			if (numNeighbors == 0)
			{
				neighborID = pStaleNeighbor[enginerandom->RandomInt(0,numStaleNeighbors-1)];
			}
			else
			{
				for (int i=0;i<numNeighbors;i++)
				{
					Vector nodeDir = vLastPos - pAInode[pNeighbor[i]]->GetOrigin();
					VectorNormalize(nodeDir);
					float fDotPr = DotProduct(vDirection,nodeDir);
					if (fDotPr > bestDot)
					{
						bestDot = fDotPr;
						neighborID = pNeighbor[i];
					}
				}
			}

			if (neighborID != NO_NODE)
			{
				vDirection = vLastPos - pAInode[neighborID]->GetOrigin();
				VectorNormalize(vDirection);
			}

		}
		// Pick random neighbor 
		else if (numNeighbors != 0)
		{
			neighborID = pNeighbor[enginerandom->RandomInt(0,numNeighbors-1)];
		}
		// If no neighbors, try using a stale one
		else
		{
			neighborID = pStaleNeighbor[enginerandom->RandomInt(0,numStaleNeighbors-1)];
		}

		// BUGBUG: This routine is totally hosed!
		if ( neighborID < 0 )
			return NULL;

		// Set previous nodes parent
		nodeParent[neighborID] = lastID;
		closeBS.Set(neighborID);

		// Add the new length
		if (lastID != NO_NODE)
		{
			pathLength += (pAInode[lastID]->GetOrigin() - pAInode[neighborID]->GetOrigin()).Length();
		}

		// If path is long enough or we've hit a maximum number of search nodes,
		// we're done unless we've ended on a climb node
		if ((pathLength >= minPathLength || nSearchCount > 20) &&
			pAInode[neighborID]->GetType() != NODE_CLIMB)
		{
			return MakeRouteFromParents(&nodeParent[0], neighborID);
		}

		// Clear neighbors
		numNeighbors		= 0;
		numStaleNeighbors	= 0;

		// Now add in new neighbors, pick links in different order ever time
		pAInode[neighborID]->ShuffleLinks();
		for (int link=0; link < pAInode[neighborID]->NumLinks();link++) 
		{
			if ( numStaleNeighbors == ARRAYSIZE(pStaleNeighbor) )
			{
				AssertMsg( 0, "Array overflow" );
				return NULL;
			}
			if ( numNeighbors == ARRAYSIZE(pStaleNeighbor) )
			{
				AssertMsg( 0, "Array overflow" );
				return NULL;
			}

			CAI_Link*	nodeLink = pAInode[neighborID]->GetShuffeledLink(link);
			int			testID	 = nodeLink->DestNodeID(neighborID);

			// --------------------------------------------------------------------------
			//  Don't loop
			// --------------------------------------------------------------------------
			if (closeBS.IsBitSet(testID))
			{
				continue;
			}

			// --------------------------------------------------------------------------
			// Don't go back to the node I just visited
			// --------------------------------------------------------------------------
			if (testID == lastID)
			{
				continue; 
			}

			// --------------------------------------------------------------------------
			// Make sure link is valid
			// --------------------------------------------------------------------------
			if (!IsLinkUsable(nodeLink,neighborID))
			{
				continue;
			}

			// --------------------------------------------------------------------------
			// If its a stale node add to stale list
			// --------------------------------------------------------------------------
			if (pAInode[testID]->IsLocked())
			{
				pStaleNeighbor[numStaleNeighbors]=testID;
				numStaleNeighbors++;
			}

			// --------------------------------------
			//  Add to list of non-stale neighbors
			// --------------------------------------
			else
			{
				pNeighbor[numNeighbors]=testID;
				numNeighbors++;
			}
		}
	}
	// Failed to get a path of full length, but return what we have
	return MakeRouteFromParents(&nodeParent[0], neighborID);
}
