#include "CAI_Hint.h"
#include "CPlayer.h"
#include "CAI_Network.h"
#include "CAI_node.h"



// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



//==================================================
// CHintCriteria
//==================================================

CHintCriteria::CHintCriteria( void )
{
	m_iFirstHintType = HINT_NONE;
	m_iLastHintType = HINT_NONE;
	m_strGroup		= NULL_STRING;
	m_iFlags		= 0;
	m_HintTypes.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CHintCriteria::~CHintCriteria( void )
{
	m_zoneInclude.Purge();
	m_zoneExclude.Purge();
	m_HintTypes.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: Sets the hint type for this search criteria
// Input  : nHintType - the hint type for this search criteria
//-----------------------------------------------------------------------------
void CHintCriteria::SetHintType( int nHintType )
{
	m_iFirstHintType = nHintType;
	m_iLastHintType = HINT_NONE;
	m_HintTypes.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: Add another type of hint that matches the search criteria
//-----------------------------------------------------------------------------
void CHintCriteria::AddHintType( int hintType )
{
	m_HintTypes.AddToTail( hintType );
}

int CHintCriteria::NumHintTypes() const
{
	return m_HintTypes.Count();
}

int	CHintCriteria::GetHintType( int idx ) const
{
	return m_HintTypes[ idx ];
}

bool CHintCriteria::MatchesSingleHintType() const
{ 
	if ( m_HintTypes.Count() != 0 )
	{
		return false;
	}

	if ( m_iFirstHintType != HINT_ANY && 
		    m_iLastHintType == HINT_NONE )
	{
		return true;
	}
	
	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CHintCriteria::MatchesHintType( int hintType ) const
{
	int c = m_HintTypes.Count();
 	for ( int i = 0; i < c; ++i )
	{
		if ( m_HintTypes[i] == hintType )
			return true;
	}

	// See if we're trying to filter the nodes
	if ( GetFirstHintType() != HINT_ANY )
	{
		if( GetLastHintType() == HINT_NONE )
		{
			// Searching for a single type of hint.
			if( GetFirstHintType() != hintType )
				return false;
		}
		else
		{
			// This search is for a range of hint types.
			if( hintType < GetFirstHintType() || hintType > GetLastHintType() )
				return false;
		}

		return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Allows us to search for nodes within a range of consecutive types.
//-----------------------------------------------------------------------------
void CHintCriteria::SetHintTypeRange( int firstType, int lastType )
{
	if( lastType < firstType )
	{
		DevMsg( 2, "Hint Type Range is backwards - Fixing up.\n" );

		int temp;

		temp = firstType;
		firstType = lastType;
		lastType = temp;
	}

	m_iFirstHintType = firstType;
	m_iLastHintType = lastType;
	m_HintTypes.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bitmask - 
//-----------------------------------------------------------------------------
void CHintCriteria::SetFlag( int bitmask )
{
	m_iFlags |= bitmask;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bitmask - 
//-----------------------------------------------------------------------------
void CHintCriteria::ClearFlag( int bitmask )
{
	m_iFlags &= ~bitmask;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : group - 
//-----------------------------------------------------------------------------
void CHintCriteria::SetGroup( string_t group )
{
	m_strGroup = group;
}

//-----------------------------------------------------------------------------
// Purpose: Adds a zone to a zone list
// Input  : list - the list of zones to add the new zone to
//			&position - the origin point of the zone
//			radius - the radius of the zone
//-----------------------------------------------------------------------------
void CHintCriteria::AddZone( zoneList_t &list, const Vector &position, float radius )
{
	int id = list.AddToTail();
	list[id].position	= position;
	list[id].radiussqr	= radius*radius;
}

//-----------------------------------------------------------------------------
// Purpose: Adds an include zone to the search criteria
// Input  : &position - the origin point of the zone
//			radius - the radius of the zone
//-----------------------------------------------------------------------------
void CHintCriteria::AddIncludePosition( const Vector &position, float radius )
{
	AddZone( m_zoneInclude, position, radius );
}

//-----------------------------------------------------------------------------
// Purpose: Adds an exclude zone to the search criteria
// Input  : &position - the origin point of the zone
//			radius - the radius of the zone
//-----------------------------------------------------------------------------
void CHintCriteria::AddExcludePosition( const Vector &position, float radius )
{
	AddZone( m_zoneExclude, position, radius );
}

//-----------------------------------------------------------------------------
// Purpose: Test to see if this position falls within any of the zones in the list
// Input  : *zone - list of zones to test against
//			&testPosition - position to test with
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
inline bool CHintCriteria::InZone( const zoneList_t &zone, const Vector &testPosition ) const
{
	int	numZones = zone.Count();

	//Iterate through all zones in the list
	for ( int i = 0; i < numZones; i++ )
	{
		if ( ((zone[i].position) - testPosition).LengthSqr() < (zone[i].radiussqr) )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Determine if a point within our include list
// Input  : &testPosition - position to test with
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHintCriteria::InIncludedZone( const Vector &testPosition ) const
{
	return InZone( m_zoneInclude, testPosition );
}

//-----------------------------------------------------------------------------
// Purpose: Determine if a point within our exclude list
// Input  : &testPosition - position to test with
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CHintCriteria::InExcludedZone( const Vector &testPosition ) const
{
	return InZone( m_zoneExclude, testPosition );
}


CAIHintVector *CAI_HintManager::gm_AllHints;

DEFINE_PROP(m_NodeData, CE_AI_Hint);
DEFINE_PROP(m_flNextUseTime, CE_AI_Hint);
DEFINE_PROP(m_hHintOwner, CE_AI_Hint);
DEFINE_PROP(m_nTargetNodeID, CE_AI_Hint);
DEFINE_PROP(m_nodeFOV, CE_AI_Hint);
DEFINE_PROP(m_vecForward, CE_AI_Hint);


bool CE_AI_Hint::IsLocked( void )
{
	if (m_NodeData.ptr->iDisabled)
	{
		return true;
	}

	if (gpGlobals->curtime < m_flNextUseTime)
	{
		return true;
	}
	
	if (m_hHintOwner.ptr != NULL)
	{
		return true;
	}
	return false;
}

bool CE_AI_Hint::Lock( CBaseEntity* pNPC )
{
	if ( *(m_hHintOwner.ptr) != pNPC && *(m_hHintOwner.ptr) != NULL )
		return false;
	*(m_hHintOwner.ptr) = pNPC;
	return true;
}

void CE_AI_Hint::GetPosition(CCombatCharacter *pBCC, Vector *vPosition)
{
	if ( m_NodeData->nNodeID != NO_NODE )
	{
		*vPosition = g_pBigAINet->GetNodePosition( pBCC, m_NodeData->nNodeID );
	}
	else
	{
		*vPosition = GetAbsOrigin();
	}
}

void CE_AI_Hint::GetPosition( Hull_t hull, Vector *vPosition )
{
	if ( m_NodeData->nNodeID != NO_NODE )
	{
		*vPosition = g_pBigAINet->GetNodePosition( hull, m_NodeData->nNodeID );
	}
	else
	{
		*vPosition = GetAbsOrigin();
	}
}

void CE_AI_Hint::Unlock( float delay )
{
	*(m_hHintOwner.ptr)	= NULL;
	m_flNextUseTime = gpGlobals->curtime + delay;
}

bool CE_AI_Hint::IsLockedBy( CEntity *pNPC )
{
	CBaseEntity *cbase = (pNPC) ? pNPC->BaseEntity() : NULL;
	return (m_hHintOwner == cbase);
};

void CE_AI_Hint::DisableForSeconds( float flSeconds )
{
	Unlock( flSeconds );
}

Vector CE_AI_Hint::GetDirection()
{
	return UTIL_YawToVector( Yaw() );
}

float CE_AI_Hint::Yaw(void)
{
	if (m_NodeData->nNodeID != NO_NODE)
	{
		return g_pBigAINet->GetNodeYaw(m_NodeData->nNodeID );
	}
	else
	{
		return GetLocalAngles().y;
	}
}

bool CE_AI_Hint::HintMatchesCriteria( CAI_NPC *pNPC, const CHintCriteria &hintCriteria, const Vector &position, float *flNearestDistance, bool bIgnoreLock, bool bIgnoreHintType )
{
	// Cannot be locked
	if ( !bIgnoreLock && IsLocked() )
	{
		return false;
	}

	if ( !bIgnoreHintType && !hintCriteria.MatchesHintType( HintType() ) )
	{
		return false;
	}

	if ( GetMinState() > NPC_STATE_IDLE || GetMaxState() < NPC_STATE_COMBAT )
	{
		if ( pNPC && ( pNPC->GetState() < GetMinState() || pNPC->GetState() > GetMaxState() ) )
		{
			return false;
		}
	}

	// See if we're filtering by group name
	if ( hintCriteria.GetGroup() != NULL_STRING )
	{
		//AssertIsValidString( GetGroup() );
		//AssertIsValidString( hintCriteria.GetGroup() );
		if ( GetGroup() == NULL_STRING || GetGroup() != hintCriteria.GetGroup() )
		{
			Assert(GetGroup() == NULL_STRING || strcmp( STRING(GetGroup()), STRING(hintCriteria.GetGroup())) != 0 );
			return false;
		}
	}

	// If we're watching for include zones, test it
	if ( ( hintCriteria.HasIncludeZones() ) && ( hintCriteria.InIncludedZone( GetAbsOrigin() ) == false ) )
	{
		return false;
	}

	// If we're watching for exclude zones, test it
	if ( ( hintCriteria.HasExcludeZones() ) && ( hintCriteria.InExcludedZone( GetAbsOrigin() ) ) )
	{
		return false;
	}

	// See if the class handles this hint type
	if ( ( pNPC != NULL ) && ( pNPC->FValidateHintType( BaseEntity() ) == false ) )
	{
		return false;
	}

	if ( hintCriteria.HasFlag(bits_HINT_NPC_IN_NODE_FOV) )
	{
		if ( pNPC == NULL )
		{
			AssertMsg(0,"Hint node attempted to verify NPC in node FOV without NPC!\n");
		}
		else
		{
			if( !IsInNodeFOV(pNPC) )
			{
				return false;
			}
		}
	}

	if ( hintCriteria.HasFlag( bits_HINT_NODE_IN_AIMCONE ) )
	{
		if ( pNPC == NULL )
		{
			AssertMsg( 0, "Hint node attempted to find node in aimcone without specifying NPC!\n" );
		}
		else
		{
			if( !pNPC->FInAimCone_Vector( GetAbsOrigin() ) )
			{
				return false;
			}
		}
	}

	if ( hintCriteria.HasFlag( bits_HINT_NODE_IN_VIEWCONE ) )
	{
		if ( pNPC == NULL )
		{
			AssertMsg( 0, "Hint node attempted to find node in viewcone without specifying NPC!\n" );
		}
		else
		{
			if( !pNPC->FInViewCone_Entity( BaseEntity() ) )
			{
				return false;
			}
		}
	}

	if ( hintCriteria.HasFlag( bits_HINT_NOT_CLOSE_TO_ENEMY ) )
	{
		if ( pNPC == NULL )
		{
			AssertMsg( 0, "Hint node attempted to find node not close to enemy without specifying NPC!\n" );
		}
		else
		{
			if( pNPC->GetEnemy() )
			{
				float flDistHintToEnemySqr = GetAbsOrigin().DistToSqr( pNPC->GetEnemy()->GetAbsOrigin() ) ;

				if( flDistHintToEnemySqr < Square( 30.0f * 12.0f ) )
				{
					return false;
				}
			}
		}
	}

	{
		// See if we're requesting a visible node
		if ( hintCriteria.HasFlag( bits_HINT_NODE_VISIBLE ) )
		{
			if ( pNPC == NULL )
			{
				//NOTENOTE: If you're hitting this, you've asked for a visible node without specifing an NPC!
				AssertMsg( 0, "Hint node attempted to find visible node without specifying NPC!\n" );
			}
			else
			{
				if( m_NodeData->nNodeID == NO_NODE )
				{
					// This is just an info_hint, not a node.
					if( !pNPC->FVisible_Entity( BaseEntity() ) )
					{
						return false;
					}
				}
				else
				{
					// This hint associated with a node.
					trace_t tr;
					Vector vHintPos;
					GetPosition(pNPC,&vHintPos);
					UTIL_TraceLine ( pNPC->EyePosition(), vHintPos + pNPC->GetViewOffset(), MASK_NPCSOLID_BRUSHONLY, pNPC->BaseEntity(), COLLISION_GROUP_NONE, &tr );
					if ( tr.fraction != 1.0f )
					{
						return false;
					}
				}
			}
		}
	}

	// Check for clear if requested
	if ( hintCriteria.HasFlag( bits_HINT_NODE_CLEAR ) )
	{
		if ( pNPC == NULL )
		{
			//NOTENOTE: If you're hitting this, you've asked for a clear node without specifing an NPC!
			AssertMsg( 0, "Hint node attempted to find clear node without specifying NPC!\n" );
		}
		else
		{
			trace_t tr;
			// Can my bounding box fit there?
			UTIL_TraceHull ( GetAbsOrigin(), GetAbsOrigin(), pNPC->WorldAlignMins(), pNPC->WorldAlignMaxs(), 
				MASK_SOLID, pNPC->BaseEntity(), COLLISION_GROUP_NONE, &tr );

			if ( tr.fraction != 1.0 )
			{
				return false;
			}
		}
	}

	// See if this is our next, closest node
	if ( hintCriteria.HasFlag( bits_HINT_NODE_NEAREST ) )
	{
		Assert( flNearestDistance );

		// Calculate our distance
		float distance = (GetAbsOrigin() - position).Length();

		// Must be closer than the current best
		if ( distance > *flNearestDistance )
		{
			return false;
		}

		// Remember the distance
		*flNearestDistance = distance;
	}

	if ( hintCriteria.HasFlag(bits_HINT_HAS_LOS_TO_PLAYER|bits_HAS_EYEPOSITION_LOS_TO_PLAYER) )
	{
		CPlayer *pPlayer = UTIL_GetNearestVisiblePlayer(this);
		if ( !pPlayer )
			UTIL_GetNearestPlayer(GetAbsOrigin());

		if( pPlayer != NULL )
		{
			Vector vecDest = GetAbsOrigin(); 

			if( hintCriteria.HasFlag(bits_HAS_EYEPOSITION_LOS_TO_PLAYER) )
			{
				vecDest += pNPC->GetDefaultEyeOffset();
			}

			if( !pPlayer->FVisible_Vector(vecDest) )
			{
				return false;
			}
		}
	}

	// Must either be visible or not if requested
	if ( hintCriteria.HasFlag( bits_HINT_NODE_NOT_VISIBLE_TO_PLAYER|bits_HINT_NODE_VISIBLE_TO_PLAYER ) )
	{
		bool bWasSeen = false;
		// Test all potential seers
		for ( int i = 1; i <= gpGlobals->maxClients; i++ )
		{
			CPlayer *pPlayer = UTIL_PlayerByIndex(i);
			
			if ( pPlayer )
			{
				// Only spawn if the player's looking away from me
				Vector vLookDir = pPlayer->EyeDirection3D();
				Vector vTargetDir = GetAbsOrigin() - pPlayer->EyePosition();
				VectorNormalize(vTargetDir);

				float fDotPr = DotProduct(vLookDir,vTargetDir);
				if ( fDotPr > 0 )
				{
					trace_t tr;
					UTIL_TraceLine( pPlayer->EyePosition(), GetAbsOrigin(), MASK_SOLID_BRUSHONLY, pPlayer->BaseEntity(), COLLISION_GROUP_NONE, &tr);
					
					if ( tr.fraction == 1.0 )
					{
						if ( hintCriteria.HasFlag( bits_HINT_NODE_NOT_VISIBLE_TO_PLAYER ) )
						{
							return false;
						}
						bWasSeen = true;
					}
				}
			}
		}

		if ( !bWasSeen && hintCriteria.HasFlag( bits_HINT_NODE_VISIBLE_TO_PLAYER ) )
		{
			return false;
		}
	}

	return true;
}


bool CE_AI_Hint::IsInNodeFOV( CEntity *pOther )
{
	if( m_nodeFOV == 360 )
	{
		return true;
	}

	Vector vecToNPC = pOther->GetAbsOrigin() - GetAbsOrigin();
	VectorNormalize( vecToNPC );
	float flDot = DotProduct( vecToNPC, m_vecForward );

	if( flDot > m_nodeFOV )
	{
		return true;
	}

	return false;
}

CAI_Node *CE_AI_Hint::GetNode( void )
{
	if ( m_NodeData->nNodeID != NO_NODE )
	{
		return g_pBigAINet->GetNode( m_NodeData->nNodeID, false );
	}
	return NULL;
}




int CAI_HintManager::GetFlags( const char *token )
{
	int len = strlen( token );
	if ( len <= 0 )
	{
		return bits_HINT_NODE_NONE;
	}

	char *lowercase = (char *)_alloca( len + 1 );
	Q_strncpy( lowercase, token, len+1 );
	Q_strlower( lowercase );

	if ( strstr( "none", lowercase ) )
	{
		return bits_HINT_NODE_NONE;
	}

	int bits = 0;

	if ( strstr( "visible", lowercase ) )
	{
		bits |= bits_HINT_NODE_VISIBLE;
	}

	if ( strstr( "nearest", lowercase ) )
	{
		bits |= bits_HINT_NODE_NEAREST;
	}

	if ( strstr( "random", lowercase ) )
	{
		bits |= bits_HINT_NODE_RANDOM;
	}

	// Can't be nearest and random, defer to nearest
	if ( ( bits & bits_HINT_NODE_NEAREST ) &&
		 ( bits & bits_HINT_NODE_RANDOM ) )
	{
		// Remove random
		bits &= ~bits_HINT_NODE_RANDOM;

		DevMsg( "HINTFLAGS:%s, inconsistent, the nearest node is never a random hint node, treating as nearest request!\n",
			token );
	}

	return bits;
}


//-----------------------------------------------------------------------------
// Purpose: Searches for a hint node that this NPC cares about. If one is
//			claims that hint node for this NPC so that no other NPCs
//			try to use it.
//
// Input  : nFlags - Search criterea. Can currently be one or more of the following:
//				bits_HINT_NODE_VISIBLE - searches for visible hint nodes.
//				bits_HINT_NODE_RANDOM - calls through the FindHintRandom and builds list of all matching
//				nodes and picks randomly from among them.  Note:  Depending on number of hint nodes, this
//				could be slower, so use with care.
//
// Output : Returns pointer to hint node if available hint node was found that matches the
//			given criterea that this NPC also cares about. Otherwise, returns NULL
//-----------------------------------------------------------------------------
CE_AI_Hint* CAI_HintManager::FindHint( CAI_NPC *pNPC, Hint_e nHintType, int nFlags, float flMaxDist, const Vector *pMaxDistFrom )
{
	assert( pNPC != NULL );
	if ( pNPC == NULL )
		return NULL;

	CHintCriteria	hintCriteria;
	hintCriteria.SetHintType( nHintType );
	hintCriteria.SetFlag( nFlags );

	// Using the NPC's hint group?
	if ( nFlags & bits_HINT_NODE_USE_GROUP )
	{
		hintCriteria.SetGroup( pNPC->GetHintGroup() );
	}

	// Add the search position
	Vector vecPosition = ( pMaxDistFrom != NULL ) ? (*pMaxDistFrom) : pNPC->GetAbsOrigin();
	hintCriteria.AddIncludePosition( vecPosition, flMaxDist );

	// If asking for a random node, use random logic instead
	if ( nFlags & bits_HINT_NODE_RANDOM )
		return FindHintRandom( pNPC, vecPosition, hintCriteria );

	return FindHint( pNPC, vecPosition, hintCriteria );
}



//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *hintCriteria - 
// Output : CAI_Hint
//-----------------------------------------------------------------------------
CE_AI_Hint *CAI_HintManager::FindHint( CAI_NPC *pNPC, const Vector &position, const CHintCriteria &hintCriteria )
{
	return (CE_AI_Hint *)g_helpfunc.CAI_HintManager_FindHint( pNPC->BaseEntity(), position, hintCriteria );
}

CE_AI_Hint *CAI_HintManager::FindHint( CAI_NPC *pNPC, const CHintCriteria &hintCriteria )
{
	assert( pNPC != NULL );
	if ( pNPC == NULL )
		return NULL;

	return FindHint( pNPC, pNPC->GetAbsOrigin(), hintCriteria );
}

CE_AI_Hint *CAI_HintManager::FindHint( const Vector &position, const CHintCriteria &hintCriteria )
{
	return FindHint( NULL, position, hintCriteria );
}


//-----------------------------------------------------------------------------
// Purpose: Finds a random hint within the requested radious of the npc
//  Builds a list of all suitable hints and chooses randomly from amongst them.
// Input  : *pNPC - 
//			nHintType - 
//			nFlags - 
//			flMaxDist - 
// Output : CAI_Hint
//-----------------------------------------------------------------------------
CE_AI_Hint *CAI_HintManager::FindHintRandom( CAI_NPC *pNPC, const Vector &position, const CHintCriteria &hintCriteria )
{
	return (CE_AI_Hint *)g_helpfunc.CAI_HintManager_FindHintRandom( pNPC->BaseEntity(), position, hintCriteria );
}

int CAI_HintManager::FindAllHints( CAI_NPC *pNPC, const Vector &position, const CHintCriteria &hintCriteria, CUtlVector<CE_AI_Hint *> *pResult )
{
	//  If we have no hints, bail
	int c = CAI_HintManager::gm_AllHints->Count();
	if ( !c )
		return 0;

	// Remove the nearest flag. It makes now sense with random.
	bool hadNearest = hintCriteria.HasFlag( bits_HINT_NODE_NEAREST );
	(const_cast<CHintCriteria &>(hintCriteria)).ClearFlag( bits_HINT_NODE_NEAREST );

	//  Now loop till we find a valid hint or return to the start
	CE_AI_Hint *pTestHint;
	for ( int i = 0; i < c; ++i )
	{
		pTestHint = (CE_AI_Hint *)CEntity::Instance(CAI_HintManager::gm_AllHints->Element(i));
		Assert( pTestHint );
		if ( pTestHint->HintMatchesCriteria( pNPC, hintCriteria, position, NULL ) )
			pResult->AddToTail( pTestHint );
	}

	if ( hadNearest )
		(const_cast<CHintCriteria &>(hintCriteria)).SetFlag( bits_HINT_NODE_NEAREST );

	return pResult->Count();
}







CE_LINK_ENTITY_TO_CLASS(CAI_Hint, CE_AI_Hint);


