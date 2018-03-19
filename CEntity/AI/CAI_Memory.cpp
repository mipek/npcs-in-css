
#include "CAI_Memory.h"
#include "CAI_NPC.h"



// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



#define	EMEMORY_POOL_SIZE		  64
#define AI_FREE_KNOWLEDGE_DURATION 1.75


CEAI_Enemies::CEAI_Enemies(void)
{
	m_flFreeKnowledgeDuration = AI_FREE_KNOWLEDGE_DURATION;
	m_flEnemyDiscardTime = AI_DEF_ENEMY_DISCARD_TIME;
	m_vecDefaultLKP = vec3_invalid;
	m_vecDefaultLSP = vec3_invalid;
	m_serial = 0;
	SetDefLessFunc( m_Map );
}


CEAI_Enemies::~CEAI_Enemies()
{
	for ( CMemMap::IndexType_t i = m_Map.FirstInorder(); i != m_Map.InvalidIndex(); i = m_Map.NextInorder( i ) )
	{
		g_pMemAlloc->Free(m_Map[i]);
	}
}

void CEAI_Enemies::SetFreeKnowledgeDuration( float flDuration )
{ 
	m_flFreeKnowledgeDuration = flDuration;	

	if ( m_flFreeKnowledgeDuration >= m_flEnemyDiscardTime )
	{
		// If your free knowledge time is greater than your discard time,
		// you'll forget about secondhand enemies passed to you by squadmates
		// as soon as you're given them.
		//Assert( m_flFreeKnowledgeDuration < m_flEnemyDiscardTime );

		m_flFreeKnowledgeDuration = m_flEnemyDiscardTime - .1;
	}
}



//-----------------------------------------------------------------------------
// Purpose: Returns last known posiiton of given enemy
//-----------------------------------------------------------------------------
const Vector &CEAI_Enemies::LastKnownPosition( CBaseEntity *pEnemy )
{
	AI_EnemyInfo_t *pMemory = Find( pEnemy, true );
	if ( pMemory )
	{
		m_vecDefaultLKP = pMemory->vLastKnownLocation;
	}
	else
	{
		DevWarning( 2,"Asking LastKnownPosition for enemy that's not in my memory!!\n");
	}
	return m_vecDefaultLKP;
}


//-----------------------------------------------------------------------------

AI_EnemyInfo_t *CEAI_Enemies::Find( CBaseEntity *pEntity, bool bTryDangerMemory )
{
	if ( pEntity == AI_UNKNOWN_ENEMY )
		pEntity = NULL;

	CMemMap::IndexType_t i = m_Map.Find( pEntity );
	if ( i == m_Map.InvalidIndex() )
	{
		if ( !bTryDangerMemory || ( i = m_Map.Find( NULL ) ) == m_Map.InvalidIndex() )
			return NULL;
		Assert(m_Map[i]->bDangerMemory == true);
	}
	return m_Map[i];
}


void CEAI_Enemies::SetEnemyDiscardTime( float flTime )
{ 
	m_flEnemyDiscardTime = flTime;			

	if ( m_flFreeKnowledgeDuration >= m_flEnemyDiscardTime )
	{
		// If your free knowledge time is greater than your discard time,
		// you'll forget about secondhand enemies passed to you by squadmates
		// as soon as you're given them.
		Assert( m_flFreeKnowledgeDuration < m_flEnemyDiscardTime );

		m_flFreeKnowledgeDuration = m_flEnemyDiscardTime - .1;
	}
}

//-----------------------------------------------------------------------------
// Purpose: Notes that the given enemy has eluded me
//-----------------------------------------------------------------------------
void CEAI_Enemies::MarkAsEluded( CBaseEntity *pEnemy )
{
	AI_EnemyInfo_t *pMemory = Find( pEnemy );
	if ( pMemory )
	{
		pMemory->bEludedMe = true;
	}
}

float CEAI_Enemies::LastTimeSeen( CBaseEntity *pEnemy, bool bCheckDangerMemory /*= true*/ )
{
	// I've never seen something that doesn't exist
	if (!pEnemy)
		return 0;

	AI_EnemyInfo_t *pMemory = Find( pEnemy, bCheckDangerMemory );
	if ( pMemory )
		return pMemory->timeLastSeen;

	if ( pEnemy != AI_UNKNOWN_ENEMY )
		DevWarning( 2,"Asking LastTimeSeen for enemy that's not in my memory!!\n");
	return AI_INVALID_TIME;
}


//-----------------------------------------------------------------------------
// Purpose:	Clear information about our enemy
//-----------------------------------------------------------------------------
void CEAI_Enemies::ClearMemory(CBaseEntity *pEnemy)
{
	CMemMap::IndexType_t i = m_Map.Find( pEnemy );
	if ( i != m_Map.InvalidIndex() )
	{
		g_pMemAlloc->Free( m_Map[i] );
		m_Map.RemoveAt( i );
	}
}

AI_EnemyInfo_t *CEAI_Enemies::GetNext( AIEnemiesIter_t *pIter )
{
	CMemMap::IndexType_t i = (CMemMap::IndexType_t)((unsigned)(*pIter));

	if ( i == m_Map.InvalidIndex() )
		return NULL;

	i = m_Map.NextInorder( i );
	*pIter = (AIEnemiesIter_t)(unsigned)i;
	if ( i == m_Map.InvalidIndex() )
		return NULL;

	if ( m_Map[i]->hEnemy == NULL )
		return GetNext( pIter );

	return m_Map[i];
}


//-----------------------------------------------------------------------------
// Purpose:	Purges any dead enemies from memory
//-----------------------------------------------------------------------------

AI_EnemyInfo_t *CEAI_Enemies::GetFirst( AIEnemiesIter_t *pIter )
{
	CMemMap::IndexType_t i = m_Map.FirstInorder();
	*pIter = (AIEnemiesIter_t)(unsigned)i;

	if ( i == m_Map.InvalidIndex() )
		return NULL;

	if ( m_Map[i]->hEnemy == NULL )
		return GetNext( pIter );

	return m_Map[i];
}

//------------------------------------------------------------------------------
// Purpose : Returns true if this enemy is part of my memory
//------------------------------------------------------------------------------
bool CEAI_Enemies::HasMemory( CBaseEntity *pEnemy )
{
	return ( Find( pEnemy ) != NULL );
}

//------------------------------------------------------------------------------
// Purpose : Returns true if this enemy is part of my memory
//------------------------------------------------------------------------------
void CEAI_Enemies::OnTookDamageFrom( CBaseEntity *pEnemy )
{
	AI_EnemyInfo_t *pMemory = Find( pEnemy, true );
	if ( pMemory )
		pMemory->timeLastReceivedDamageFrom = gpGlobals->curtime;
}

void CEAI_Enemies::RefreshMemories(void)
{
	if ( m_flFreeKnowledgeDuration >= m_flEnemyDiscardTime )
	{
		m_flFreeKnowledgeDuration = m_flEnemyDiscardTime - .1;
	}

	// -------------------
	// Check each record
	// -------------------
	
	CMemMap::IndexType_t i = m_Map.FirstInorder();
	while ( i != m_Map.InvalidIndex() )
	{	
		AI_EnemyInfo_t *pMemory = m_Map[i];
		
		CMemMap::IndexType_t iNext = m_Map.NextInorder( i ); // save so can remove
		if ( ShouldDiscardMemory( pMemory ) )
		{
			g_pMemAlloc->Free(pMemory);
			m_Map.RemoveAt(i);
		}
		else if ( pMemory->hEnemy )
		{
			CEntity *enemy = CEntity::Instance(pMemory->hEnemy);

			if ( gpGlobals->curtime <= pMemory->timeLastSeen + m_flFreeKnowledgeDuration )
			{
				// Free knowledge is ignored if the target has notarget on
				if ( !(enemy->GetFlags() & FL_NOTARGET) )
				{
					pMemory->vLastKnownLocation = enemy->GetAbsOrigin();
				}
			}

			if ( gpGlobals->curtime <= pMemory->timeLastSeen )
			{
				pMemory->vLastSeenLocation = enemy->GetAbsOrigin();
			}
		}
		i = iNext;
	}
}

bool CEAI_Enemies::ShouldDiscardMemory( AI_EnemyInfo_t *pMemory )
{
	CEntity *pEnemy = CEntity::Instance(pMemory->hEnemy);

	if ( pEnemy )
	{
		CAI_NPC *pEnemyNPC = pEnemy->MyNPCPointer();
		if ( pEnemyNPC && pEnemyNPC->GetState() == NPC_STATE_DEAD )
			return true;
	}
	else
	{
		if ( !pMemory->bDangerMemory )
			return true;
	}

	if ( !pMemory->bUnforgettable &&
		 gpGlobals->curtime > pMemory->timeLastSeen + m_flEnemyDiscardTime )
	{
		return true;
	}

	return false;
}


bool CEAI_Enemies::HasEludedMe( CBaseEntity *pEnemy )
{
	AI_EnemyInfo_t *pMemory = Find( pEnemy );
	if ( pMemory )
		return pMemory->bEludedMe;
	return false;
}

const Vector &CEAI_Enemies::LastSeenPosition( CBaseEntity *pEnemy )
{
	AI_EnemyInfo_t *pMemory = Find( pEnemy, true );
	if ( pMemory )
	{
		m_vecDefaultLSP = pMemory->vLastSeenLocation;
	}
	else
	{
		DevWarning( 2,"Asking LastSeenPosition for enemy that's not in my memory!!\n");
	}
	return m_vecDefaultLSP;
}

float CEAI_Enemies::TimeAtFirstHand( CBaseEntity *pEnemy )
{
	// I've never seen something that doesn't exist
	if (!pEnemy)
		return 0;

	AI_EnemyInfo_t *pMemory = Find( pEnemy, true );
	if ( pMemory )
		return pMemory->timeAtFirstHand;

	if ( pEnemy != AI_UNKNOWN_ENEMY )
		DevWarning( 2,"Asking TimeAtFirstHand for enemy that's not in my memory!!\n");
	return AI_INVALID_TIME;
}

float CEAI_Enemies::FirstTimeSeen( CBaseEntity *pEnemy)
{
	// I've never seen something that doesn't exist
	if (!pEnemy)
		return 0;

	AI_EnemyInfo_t *pMemory = Find( pEnemy, true );
	if ( pMemory )
		return pMemory->timeFirstSeen;

	if ( pEnemy != AI_UNKNOWN_ENEMY )
		DevWarning( 2,"Asking FirstTimeSeen for enemy that's not in my memory!!\n");
	return AI_INVALID_TIME;
}

AI_EnemyInfo_t *CEAI_Enemies::GetDangerMemory()
{
	CMemMap::IndexType_t i = m_Map.Find( NULL );
	if ( i == m_Map.InvalidIndex() )
		return NULL;
	Assert(m_Map[i]->bDangerMemory == true);
	return m_Map[i];
}

