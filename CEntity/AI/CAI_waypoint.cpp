
#include "CEntity.h"
#include "ai_navtype.h"
#include "CAI_node.h"
#include "ai_waypoint.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



#define	WAYPOINT_POOL_SIZE 512


CMemoryPool *AI_Waypoint_t::s_Allocator = NULL;


AI_Waypoint_t::AI_Waypoint_t()
{
	memset( this, 0, sizeof(*this) );
	vecLocation	= vec3_invalid;
	iNodeID		= NO_NODE;
	flPathDistGoal = -1;
}

//-------------------------------------

AI_Waypoint_t::AI_Waypoint_t( const Vector &initPosition, float initYaw, Navigation_t initNavType, int initWaypointFlags, int initNodeID )
{
	memset( this, 0, sizeof(*this) );

	// A Route of length one to the endpoint
	vecLocation	= initPosition;
	flYaw		= initYaw;
	m_iWPType	= initNavType;
	m_fWaypointFlags = initWaypointFlags;
	iNodeID		= initNodeID;

	flPathDistGoal = -1;
}

//-----------------------------------------------------------------------------

void CAI_WaypointList::RemoveAll()
{
	DeleteAll( &m_pFirstWaypoint );
	Assert( m_pFirstWaypoint == NULL );
}


//-----------------------------------------------------------------------------
// Purpose: Deletes a waypoint linked list
//-----------------------------------------------------------------------------
void DeleteAll( AI_Waypoint_t *pWaypointList )
{
	while ( pWaypointList )
	{
		AI_Waypoint_t *pPrevWaypoint = pWaypointList;
		pWaypointList = pWaypointList->GetNext();
		delete pPrevWaypoint;
	}
}

//-------------------------------------

AI_Waypoint_t *CAI_WaypointList::GetLast()
{
	AI_Waypoint_t *p = GetFirst();
	if (!p)
		return NULL;
	while ( p->GetNext() )
		p = p->GetNext();

	return p;
}

void CAI_WaypointList::Set(AI_Waypoint_t* route)
{
	m_pFirstWaypoint = route;
}


//-------------------------------------

void CAI_WaypointList::PrependWaypoints( AI_Waypoint_t *pWaypoints )
{
	AddWaypointLists( pWaypoints, GetFirst() );
	Set( pWaypoints );
}


//-----------------------------------------------------------------------------
// Purpose: Adds addRoute to the end of oldRoute
//-----------------------------------------------------------------------------
void AddWaypointLists(AI_Waypoint_t *oldRoute, AI_Waypoint_t *addRoute)
{
	// Add to the end of the route
	AI_Waypoint_t *waypoint = oldRoute;

	while (waypoint->GetNext()) 
	{
		waypoint = waypoint->GetNext();
	}

	waypoint->ModifyFlags( bits_WP_TO_GOAL, false );

	// Check for duplication, but copy the type
	if (waypoint->iNodeID != NO_NODE			&&
		waypoint->iNodeID == addRoute->iNodeID	)
	{
//		waypoint->iWPType = addRoute->iWPType; <<TODO>> found case where this was bad
		AI_Waypoint_t *pNext = addRoute->GetNext();
		delete addRoute;
		waypoint->SetNext(pNext);
	}
	else
	{
		waypoint->SetNext(addRoute);
	}

	while (waypoint->GetNext()) 
	{
		waypoint = waypoint->GetNext();
	}

	waypoint->ModifyFlags( bits_WP_TO_GOAL, true );

}

void CAI_WaypointList::PrependWaypoint( const Vector &newPoint, Navigation_t navType, unsigned waypointFlags, float flYaw )
{
	PrependWaypoints( new AI_Waypoint_t( newPoint, flYaw, navType, waypointFlags, NO_NODE ) );
}



//-----------------------------------------------------------------------------

