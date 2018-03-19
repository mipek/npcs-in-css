
#include "CAI_Network.h"
#include "CCombatCharacter.h"
#include "CAI_Node.h"
#include "CAI_NPC.h"
#include "CAI_utils.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



CAI_Network * 		g_pBigAINet;

extern ConVar *ai_no_node_cache;


abstract_class INodeListFilter
{
public:
	virtual bool	NodeIsValid( CAI_Node &node ) = 0;
	virtual float	NodeDistanceSqr( CAI_Node &node ) = 0;
};


//-------------------------------------
// Purpose: Filters nodes for an NPC
//-------------------------------------

class CNodeFilter : public INodeListFilter
{
public:
	CNodeFilter( CAI_NPC *pNPC, const Vector &pos ) : m_pos(pos), m_pNPC(pNPC) 
	{
		if ( m_pNPC )
			m_capabilities = m_pNPC->CapabilitiesGet();
	}

	CNodeFilter( const Vector &pos ) : m_pos(pos), m_pNPC(NULL)
	{
	}

	virtual bool NodeIsValid( CAI_Node &node )
	{
		if ( node.GetType() == NODE_DELETED )
			return false;

		if ( !m_pNPC )
			return true;

		if ( m_pNPC->GetNavType() == NAV_FLY && node.GetType() != NODE_AIR )
			return false;

		// Check that node is of proper type for this NPC's navigation ability
		if ((node.GetType() == NODE_AIR    && !(m_capabilities & bits_CAP_MOVE_FLY))		||
			(node.GetType() == NODE_GROUND && !(m_capabilities & bits_CAP_MOVE_GROUND))	)
			return false;
			
		CEntity *cent = node.GetHint();
		if ( m_pNPC->IsUnusableNode( node.GetId(), (cent)?cent->BaseEntity():NULL) )
			return false;

		return true;
	}

	virtual float	NodeDistanceSqr( CAI_Node &node )
	{
		// UNDONE: This call to Position() really seems excessive here.  What is the real
		// error % relative to 800 units (MAX_NODE_LINK_DIST) ?
		if ( m_pNPC )
			return (node.GetPosition(m_pNPC->GetHullType()) - m_pos).LengthSqr();
		else
			return (node.GetOrigin() - m_pos).LengthSqr();
	}

	const Vector &m_pos;
	CAI_NPC	*m_pNPC;
	int			m_capabilities;	// cache this
};


#define MAX_NEAR_NODES	10			// Trace to 10 nodes at most




//-----------------------------------------------------------------------------

Vector CAI_Network::GetNodePosition( Hull_t hull, int nodeID )
{
	if ( !m_pAInode )
	{
		Assert( 0 );
		return vec3_origin;
	}
	
	if ( ( nodeID < 0 ) || ( nodeID > m_iNumNodes ) )
	{
		Assert( 0 );
		return vec3_origin;
	}

	return m_pAInode[nodeID]->GetPosition( hull );
}

//-----------------------------------------------------------------------------

Vector CAI_Network::GetNodePosition( CCombatCharacter *pNPC, int nodeID )
{
	if ( pNPC == NULL )
	{
		Assert( 0 );
		return vec3_origin;
	}

	return GetNodePosition( pNPC->GetHullType(), nodeID );
}


//-----------------------------------------------------------------------------
// Purpose: Return ID of node nearest of vecOrigin for pNPC with the given
//			tolerance distance.  If a route is required to get to the node
//			node_route is set.
//-----------------------------------------------------------------------------

int	CAI_Network::NearestNodeToPoint( CAI_NPC *pNPC, const Vector &vecOrigin, bool bCheckVisibility, INearestNodeFilter *pFilter )
{	
	// --------------------------------
	//  Check if network has no nodes
	// --------------------------------
	if (m_iNumNodes == 0)
		return NO_NODE;

	// ----------------------------------------------------------------
	//  First check cached nearest node positions
	// ----------------------------------------------------------------
	int cachePos;
	int cachedNode = GetCachedNearestNode( vecOrigin, pNPC, &cachePos );
	if ( cachedNode != NO_NODE )
	{
		if ( bCheckVisibility )
		{
			trace_t tr;

			Vector vTestLoc = ( pNPC ) ? 
								m_pAInode[cachedNode]->GetPosition(pNPC->GetHullType()) + pNPC->GetViewOffset() : 
								m_pAInode[cachedNode]->GetOrigin();

			CBaseEntity *cbase = (pNPC) ? pNPC->BaseEntity() : NULL;
			CTraceFilterNav traceFilter( pNPC, true, cbase, COLLISION_GROUP_NONE );
			UTIL_TraceLine ( vecOrigin, vTestLoc, MASK_NPCSOLID_BRUSHONLY, &traceFilter, &tr );

			if ( tr.fraction != 1.0 )
				cachedNode = NO_NODE;
		}

		if ( cachedNode != NO_NODE && ( !pFilter || pFilter->IsValid( m_pAInode[cachedNode] ) ) )
		{
			m_NearestCache[cachePos].expiration	= gpGlobals->curtime + NEARNODE_CACHE_LIFE;
			return cachedNode;
		}
	}

	// ---------------------------------------------------------------
	// First get nodes distances and eliminate those that are beyond 
	// the maximum allowed distance for local movements
	// ---------------------------------------------------------------
	CNodeFilter filter( pNPC, vecOrigin );

	AI_NearNode_t *pBuffer = (AI_NearNode_t *)stackalloc( sizeof(AI_NearNode_t) * MAX_NEAR_NODES );
	CNodeList list( pBuffer, MAX_NEAR_NODES );

	// OPTIMIZE: If not flying, this box should be smaller in Z (2 * height?)
	Vector ext(MAX_NODE_LINK_DIST, MAX_NODE_LINK_DIST, MAX_NODE_LINK_DIST);
	// If the NPC can fly, check further
	if ( pNPC && ( pNPC->CapabilitiesGet() & bits_CAP_MOVE_FLY ) )
	{
		ext.Init( MAX_AIR_NODE_LINK_DIST, MAX_AIR_NODE_LINK_DIST, MAX_AIR_NODE_LINK_DIST );
	}

	ListNodesInBox( list, MAX_NEAR_NODES, vecOrigin - ext, vecOrigin + ext, &filter );

	// --------------------------------------------------------------
	//  Now find a reachable node searching the close nodes first
	// --------------------------------------------------------------
	//int smallestVisibleID = NO_NODE;

	for( ;list.Count(); list.RemoveAtHead() )
	{
		int smallest = list.ElementAtHead().nodeIndex;

		// Check not already rejected above
		if ( smallest == cachedNode )
			continue;

		// Check that this node is usable by the current hull size
		if ( pNPC && !pNPC->GetNavigator()->CanFitAtNode(smallest))
			continue;

		if ( bCheckVisibility )
		{
			trace_t tr;

			Vector vTestLoc = ( pNPC ) ? 
								m_pAInode[smallest]->GetPosition(pNPC->GetHullType()) + pNPC->GetNodeViewOffset() : 
								m_pAInode[smallest]->GetOrigin();

			Vector vecVisOrigin = vecOrigin + Vector(0,0,1);

			CBaseEntity *cbase = (pNPC) ? pNPC->BaseEntity() : NULL;
			CTraceFilterNav traceFilter( pNPC, true, cbase, COLLISION_GROUP_NONE );
			UTIL_TraceLine ( vecVisOrigin, vTestLoc, MASK_NPCSOLID_BRUSHONLY, &traceFilter, &tr );

			if ( tr.fraction != 1.0 )
				continue;
		}

		if ( pFilter )
		{
			if ( !pFilter->IsValid( m_pAInode[smallest] ) )
			{
				if ( !pFilter->ShouldContinue() )
					break;
				continue;
			}
		}

		SetCachedNearestNode( vecOrigin, smallest, (pNPC) ? pNPC->GetHullType() : HULL_NONE );

		return smallest;
	}

	// Store inability to reach in cache for later use
	SetCachedNearestNode( vecOrigin, NO_NODE, (pNPC) ? pNPC->GetHullType() : HULL_NONE );

	return NO_NODE;
}


//-----------------------------------------------------------------------------
// Purpose: Build a list of nearby nodes sorted by distance
// Input  : &list - 
//			maxListCount - 
//			*pFilter - 
// Output : int - count of list
//-----------------------------------------------------------------------------

int CAI_Network::ListNodesInBox( CNodeList &list, int maxListCount, const Vector &mins, const Vector &maxs, INodeListFilter *pFilter )
{
	CNodeList result;
	
	result.SetLessFunc( CNodeList::RevIsLowerPriority );
	
	// NOTE: maxListCount must be > 0 or this will crash
	bool full = false;
	float flClosest = 1000000.0 * 1000000;
	int closest = 0;

// UNDONE: Store the nodes in a tree and query the tree instead of the entire list!!!
	for ( int node = 0; node < m_iNumNodes; node++ )
	{
		CAI_Node *pNode = m_pAInode[node];
		const Vector &origin = pNode->GetOrigin();
		// in box?
		if ( origin.x < mins.x || origin.x > maxs.x ||
			 origin.y < mins.y || origin.y > maxs.y ||
			 origin.z < mins.z || origin.z > maxs.z )
			continue;

		if ( !pFilter->NodeIsValid(*pNode) )
			continue;

		float flDist = pFilter->NodeDistanceSqr(*pNode);

		if ( flDist < flClosest )
		{
			closest = node;
			flClosest = flDist;
		}

		if ( !full || (flDist < result.ElementAtHead().dist) )
		{
			if ( full )
				result.RemoveAtHead();

			result.Insert( AI_NearNode_t(node, flDist) );
	
			full = (result.Count() == maxListCount);
		}
	}
	
	list.RemoveAll();
	while ( result.Count() )
	{
		list.Insert( result.ElementAtHead() );
		result.RemoveAtHead();
	}

	return list.Count();
}

//-----------------------------------------------------------------------------
// Purpose: Update nearest node cache with new data
//			if nHull == HULL_NONE, reachability of this node wasn't checked
//-----------------------------------------------------------------------------

void CAI_Network::SetCachedNearestNode(const Vector &checkPos, int nodeID, Hull_t nHull)
{
	if ( ai_no_node_cache->GetBool() )
		return;

	m_NearestCache[m_iNearestCacheNext].vTestPosition	= checkPos;
	m_NearestCache[m_iNearestCacheNext].node			= nodeID;
	m_NearestCache[m_iNearestCacheNext].hull			= nHull;
	m_NearestCache[m_iNearestCacheNext].expiration		= gpGlobals->curtime + NEARNODE_CACHE_LIFE;

	m_iNearestCacheNext--;
	if ( m_iNearestCacheNext < 0 )
	{
		m_iNearestCacheNext = NEARNODE_CACHE_SIZE - 1;
	}
}



//-----------------------------------------------------------------------------

int	CAI_Network::GetCachedNearestNode(const Vector &checkPos, CAI_NPC *pNPC, int *pCachePos )
{
	if ( pNPC )
	{
		CNodeFilter filter( pNPC, checkPos );

		int nodeID = GetCachedNode( checkPos, pNPC->GetHullType(), pCachePos );
		if ( nodeID >= 0 )
		{
			if ( filter.NodeIsValid( *m_pAInode[nodeID] ) && pNPC->GetNavigator()->CanFitAtNode(nodeID) )
				return nodeID;
		}
	}
	return NO_NODE;
}



//-----------------------------------------------------------------------------
// Purpose: Check nearest node cache for checkPos and return cached nearest
//			node if it exists in the cache.  Doesn't care about reachability,
//			only if the node is visible
//-----------------------------------------------------------------------------
int	CAI_Network::GetCachedNode(const Vector &checkPos, Hull_t nHull, int *pCachePos )
{
	if ( ai_no_node_cache->GetBool() )
		return NOT_CACHED;

	// Walk from newest to oldest.
	int iNewest = m_iNearestCacheNext + 1;
	for ( int i = 0; i < NEARNODE_CACHE_SIZE; i++ )
	{
		int iCurrent = ( iNewest + i ) % NEARNODE_CACHE_SIZE;
		if ( m_NearestCache[iCurrent].hull == nHull && m_NearestCache[iCurrent].expiration > gpGlobals->curtime )
		{
			if ( (m_NearestCache[iCurrent].vTestPosition - checkPos).LengthSqr() < Square(24.0) )
			{
				if ( pCachePos )
					*pCachePos = iCurrent;
				return m_NearestCache[iCurrent].node;
			}
		}
	}


	if ( pCachePos )
		*pCachePos = -1;
	return NOT_CACHED;
}


//-----------------------------------------------------------------------------
// Purpose: Given an bitString and float array of size array_size, return the 
//			index of the smallest number in the array whose it is set
//-----------------------------------------------------------------------------

int	CAI_Network::FindBSSmallest(CVarBitVec *bitString, float *float_array, int array_size) 
{
	int	  winIndex = -1;
	float winSize  = FLT_MAX;
	for (int i=0;i<array_size;i++) 
	{
		if (bitString->IsBitSet(i) && (float_array[i]<winSize)) 
		{
			winIndex = i;
			winSize  = float_array[i];
		}
	}
	return winIndex;
}

float CAI_Network::GetNodeYaw( int nodeID )
{
	if ( !m_pAInode )
	{
		Assert( 0 );
		return 0.0f;
	}
	
	if ( ( nodeID < 0 ) || ( nodeID > m_iNumNodes ) )
	{
		Assert( 0 );
		return 0.0f;
	}

	return m_pAInode[nodeID]->GetYaw();
}
