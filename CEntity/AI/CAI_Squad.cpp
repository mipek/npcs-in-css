
#include "CAI_NPC.h"
#include "CAI_Squad.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



//-----------------------------------------------------------------------------
// Purpose: Try to get one of a contiguous range of slots
// Input  : slotIDStart - start of slot range
//			slotIDEnd - end of slot range
//			hEnemy - enemy this slot is for
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAI_Squad::OccupyStrategySlotRange( CEntity *pEnemy, int slotIDStart, int slotIDEnd, int *pSlot )
{
#ifndef PER_ENEMY_SQUADSLOTS
	// FIXME: combat slots need to be per enemy, not per squad.  
	// As it is, once a squad is occupied it stops making even simple attacks to other things nearby.
	// This code may make soldiers too aggressive
	if (GetLeader() && pEnemy != GetLeader()->GetEnemy())
	{
		*pSlot = SQUAD_SLOT_NONE;
		return true;
	}
#endif

	// If I'm already occupying this slot
	if ( *pSlot >= slotIDStart && *pSlot <= slotIDEnd)
		return true;

	for ( int i = slotIDStart; i <= slotIDEnd; i++ )
	{
		// Check enemy to see if slot already occupied
		if (!IsSlotOccupied(pEnemy, i))
		{
			// Clear any previous spot;
			if (*pSlot != SQUAD_SLOT_NONE)
			{
				// As a debug measure check to see if slot was filled
				if (!IsSlotOccupied(pEnemy, *pSlot))
				{
					DevMsg( "ERROR! Vacating an empty slot!\n");
				}

				// Free the slot
				VacateSlot(pEnemy, *pSlot);
			}

			// Fill the slot
			OccupySlot(pEnemy, i);
			*pSlot = i;
			return true;
		}
	}
	return false;
}


//------------------------------------------------------------------------------

bool CAI_Squad::IsSlotOccupied( CEntity *pEnemy, int i ) const	
{ 
	const AISquadEnemyInfo_t *pInfo = FindEnemyInfo( pEnemy );
	return pInfo->slots.IsBitSet(i);
}


//------------------------------------------------------------------------------

void CAI_Squad::VacateSlot( CEntity *pEnemy, int i )			
{ 
	AISquadEnemyInfo_t *pInfo = FindEnemyInfo( pEnemy );
	pInfo->slots.Clear(i);
}

//------------------------------------------------------------------------------

void CAI_Squad::OccupySlot( CEntity *pEnemy, int i )			
{ 
	AISquadEnemyInfo_t *pInfo = FindEnemyInfo( pEnemy );
	pInfo->slots.Set(i);

}

AISquadEnemyInfo_t *CAI_Squad::FindEnemyInfo( CEntity *pEnemy )
{
	int i;
	if ( gpGlobals->curtime > m_flEnemyInfoCleanupTime )
	{
		if ( m_EnemyInfos.Count() )
		{
			m_pLastFoundEnemyInfo = NULL;
			CUtlRBTree<CBaseEntity *> activeEnemies;
			SetDefLessFunc( activeEnemies );

			// Gather up the set of active enemies

			for ( i = 0; i < m_SquadMembers.Count(); i++ )
			{
				CAI_NPC *npc = CEntity::Instance(m_SquadMembers[i])->MyNPCPointer();
				CEntity *pMemberEnemy = npc->GetEnemy();
				if ( pMemberEnemy && activeEnemies.Find( pMemberEnemy->BaseEntity() ) == activeEnemies.InvalidIndex() )
				{
					activeEnemies.Insert( pMemberEnemy->BaseEntity() );
				}
			}
			
			// Remove the records for deleted or unused enemies
			for ( i = m_EnemyInfos.Count() - 1; i >= 0; --i )
			{
				if ( m_EnemyInfos[i].hEnemy == NULL || activeEnemies.Find( m_EnemyInfos[i].hEnemy ) == activeEnemies.InvalidIndex() )
				{
					m_EnemyInfos.FastRemove( i );
				}
			}
		}
		
		m_flEnemyInfoCleanupTime = gpGlobals->curtime + 30;
	}

	if ( m_pLastFoundEnemyInfo && m_pLastFoundEnemyInfo->hEnemy == pEnemy->BaseEntity())
		return m_pLastFoundEnemyInfo;

	for ( i = 0; i < m_EnemyInfos.Count(); i++ )
	{
		if ( m_EnemyInfos[i].hEnemy == pEnemy->BaseEntity() )
		{
			m_pLastFoundEnemyInfo = &m_EnemyInfos[i];
			return &m_EnemyInfos[i];
		}
	}

	m_pLastFoundEnemyInfo = NULL;
	i = m_EnemyInfos.AddToTail();
	m_EnemyInfos[i].hEnemy = pEnemy->BaseEntity();

	m_pLastFoundEnemyInfo = &m_EnemyInfos[i];
	return &m_EnemyInfos[i];
}

CBaseEntity *CAI_Squad::GetFirstMember( AISquadIter_t *pIter, bool bIgnoreSilentMembers )
{
	int i = 0;
	if ( bIgnoreSilentMembers )
	{
		for ( ; i < m_SquadMembers.Count(); i++ )
		{
			if ( !IsSilentMember( m_SquadMembers[i] ) )
				break;
		}
	}

	if ( pIter )
		*pIter = (AISquadIter_t)i;
	if ( i >= m_SquadMembers.Count() )
		return NULL;

	return m_SquadMembers[i];
}

bool CAI_Squad::IsSilentMember( const CBaseEntity *pNPC )
{
	CAI_NPC *npc = (CAI_NPC *)CEntity::Instance(pNPC);
	if ( !npc || ( npc->GetMoveType() == MOVETYPE_NONE && npc->GetSolid() == SOLID_NONE ) ) // a.k.a., enemy finder
		return true;
	return npc->IsSilentSquadMember();
}

bool CAI_Squad::IsSilentMember( const CAI_NPC *pNPC )
{
	if ( !pNPC || ( pNPC->GetMoveType() == MOVETYPE_NONE && pNPC->GetSolid() == SOLID_NONE ) ) // a.k.a., enemy finder
		return true;
	return pNPC->IsSilentSquadMember();
}


CBaseEntity *CAI_Squad::GetNextMember( AISquadIter_t *pIter, bool bIgnoreSilentMembers )
{
	int &i = (int &)*pIter;
	i++;
	if ( bIgnoreSilentMembers )
	{
		for ( ; i < m_SquadMembers.Count(); i++ )
		{
			if ( !IsSilentMember( m_SquadMembers[i] ) )
				break;
		}
	}

	if ( i >= m_SquadMembers.Count() )
		return NULL;

	return m_SquadMembers[i];
}

bool CAI_Squad::IsStrategySlotRangeOccupied( CEntity *pEnemy, int slotIDStart, int slotIDEnd )
{
	for ( int i = slotIDStart; i <= slotIDEnd; i++ )
	{
		if (!IsSlotOccupied(pEnemy, i))
			return false;
	}
	return true;
}

//-------------------------------------
// Purpose: Broadcast a message to all squad members
// Input:	messageID - generic message handle
//			data - generic data handle
//			sender - who sent the message (NULL by default, if not, will not resend to the sender)
//-------------------------------------

int	CAI_Squad::BroadcastInteraction( int interactionType, void *data, CCombatCharacter *sender )
{
	//Must have a squad
	if ( m_SquadMembers.Count() == 0 )
		return false;

	//Broadcast to all members of the squad
	for ( int i = 0; i < m_SquadMembers.Count(); i++ )
	{
		CAI_NPC *pMember = CEntity::Instance(m_SquadMembers[i])->MyNPCPointer();		
		//Validate and don't send again to the sender
		if ( ( pMember != NULL) && ( pMember != sender ) )
		{
			//Send it
			pMember->DispatchInteraction( interactionType, data, sender->BaseEntity() );
		}
	}

	return true;
}

void CAI_Squad::VacateStrategySlot( CEntity *pEnemy, int slot)
{
	// If I wasn't taking up a squad slot I'm done
	if (slot == SQUAD_SLOT_NONE)
		return;

	// As a debug measure check to see if slot was filled
	if (!IsSlotOccupied(pEnemy, slot))
	{
		DevMsg( "ERROR! Vacating an empty slot!\n");
	}

	// Free the slot
	VacateSlot(pEnemy, slot);
}

//-------------------------------------
// Purpose: Alert everyone in the squad to the presence of a new enmey
//-------------------------------------

int	CAI_Squad::NumMembers( bool bIgnoreSilentMembers )
{
	int nSilentMembers = 0;
	if ( bIgnoreSilentMembers )
	{
		for ( int i = 0; i < m_SquadMembers.Count(); i++ )
		{
			if ( IsSilentMember( m_SquadMembers[i] ) )
				nSilentMembers++;
		}
	}
	return ( m_SquadMembers.Count() - nSilentMembers );
}


//-------------------------------------
// Purpose: Addes the given NPC to the squad
//-------------------------------------
void CAI_Squad::AddToSquad(CAI_NPC *pNPC)
{
	if ( !pNPC || !pNPC->IsAlive() )
	{
		Assert(0);
		return;
	}

	if ( pNPC->GetSquad() == this )
		return;

	if ( pNPC->GetSquad() )
	{
		pNPC->GetSquad()->RemoveFromSquad(pNPC);
	}

	if (m_SquadMembers.Count() == MAX_SQUAD_MEMBERS)
	{
		DevMsg("Error!! Squad %s is too big!!! Replacing last member\n", STRING(this->m_Name));
		m_SquadMembers.Remove(m_SquadMembers.Count()-1);
	}
	m_SquadMembers.AddToTail(pNPC->BaseEntity());

	pNPC->SetSquad( this );
	pNPC->SetSquadName( m_Name );

	if ( m_SquadMembers.Count() > 1 )
	{
		CAI_NPC *pCopyFrom = (CAI_NPC *)CEntity::Instance(m_SquadMembers[0]);
		CEAI_Enemies *pEnemies = pCopyFrom->GetEnemies();
		AIEnemiesIter_t iter;
		AI_EnemyInfo_t *pInfo = pEnemies->GetFirst( &iter );
		while ( pInfo )
		{
			pNPC->UpdateEnemyMemory( pInfo->hEnemy, pInfo->vLastKnownLocation, pCopyFrom->BaseEntity() );
			pInfo = pEnemies->GetNext( &iter );
		}
	}
}

//-------------------------------------
// Purpose: Removes an NPC from a squad
//-------------------------------------

void CAI_Squad::RemoveFromSquad( CAI_NPC *pNPC, bool bDeath )
{
	if ( !pNPC )
		return;

	// Find the index of this squad member
	int member;
	int myIndex = m_SquadMembers.Find(pNPC->BaseEntity());
	if (myIndex == -1)
	{
		DevMsg("ERROR: Attempting to remove non-existing squad membmer!\n");
		return;
	}
	m_SquadMembers.Remove(myIndex);

	// Notify squad members of death 
	if ( bDeath )
	{
		for (member = 0; member < m_SquadMembers.Count(); member++)
		{
			CAI_NPC* pSquadMem = (CAI_NPC *)CEntity::Instance(m_SquadMembers[member]);
			if (pSquadMem)
			{
				pSquadMem->NotifyDeadFriend(pNPC->BaseEntity());
			}
		}
	}

	pNPC->SetSquad(NULL);
	pNPC->SetSquadName( NULL_STRING );
}

bool CAI_Squad::SquadIsMember( CEntity *pMember )
{
	if(!pMember)
		return false;

	CAI_NPC *pNPC = pMember->MyNPCPointer();
	if ( pNPC && pNPC->GetSquad() == this )
		return true;

	return false;
}

void CAI_Squad::SquadRemember( int iMemory )
{
	for (int i = 0; i < m_SquadMembers.Count(); i++)
	{
		if (m_SquadMembers[i] != NULL )
		{
			CAI_NPC* pSquadMem = (CAI_NPC *)CEntity::Instance(m_SquadMembers[i]);
			pSquadMem->Remember( iMemory );
		}
	}
}

CAI_NPC *CAI_Squad::GetLeader( void )
{
	CAI_NPC *pLeader = NULL;
	int nSilentMembers = 0;
	for ( int i = 0; i < m_SquadMembers.Count(); i++ )
	{
		if ( !IsSilentMember( m_SquadMembers[i] ) )
		{
			if ( !pLeader )
				pLeader = (CAI_NPC *)CEntity::Instance(m_SquadMembers[i]);
		}
		else
		{
			nSilentMembers++;
		}
	}
	return ( m_SquadMembers.Count() - nSilentMembers > 1) ? pLeader : NULL;
}

bool CAI_Squad::IsLeader( CAI_NPC *pNPC )
{
	if ( IsSilentMember( pNPC ) )
		return false;

	if ( !pNPC )
		return false;

	if ( GetLeader() == pNPC )
		return true;

	return false;
}

CAI_NPC *CAI_Squad::SquadMemberInRange( const Vector &vecLocation, float flDist )
{
	for (int i = 0; i < m_SquadMembers.Count(); i++)
	{
		CAI_NPC *npc = (CAI_NPC *)CEntity::Instance(m_SquadMembers[i]);
		if (npc != NULL && (vecLocation - npc->GetAbsOrigin() ).Length2D() <= flDist)
			return npc;
	}
	return false;
}

CAI_NPC *CAI_Squad::GetSquadMemberNearestTo( const Vector &vecLocation )
{
	CAI_NPC *pNearest = NULL;
	float		flNearest = FLT_MAX;

	for ( int i = 0; i < m_SquadMembers.Count(); i++ )
	{
		float flDist;
		CAI_NPC *npc = (CAI_NPC *)CEntity::Instance(m_SquadMembers[i]);

		flDist = npc->GetAbsOrigin().DistToSqr( vecLocation );

		if( flDist < flNearest )
		{
			flNearest = flDist;
			pNearest = npc;
		}
	}

	Assert( pNearest != NULL );
	return pNearest;
}

