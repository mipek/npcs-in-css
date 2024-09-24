
#include "CEntity.h"
#include "globalstate.h"
#include "GameSystem.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

struct globalentity_t
{
	DECLARE_SIMPLE_DATADESC();

	CUtlSymbol	name;
	CUtlSymbol	levelName;
	GLOBALESTATE	state;
	int				counter;
};


class CGlobalState : public CValveAutoGameSystem
{
public:
	int GetIndex( const char *pGlobalname )
	{
		CUtlSymbol symName = m_nameList.Find( pGlobalname );

		if ( symName.IsValid() )
		{
			for ( int i = m_list.Count() - 1; i >= 0; --i )
			{
				if ( m_list[i].name == symName )
					return i;
			}
		}

		return -1;
	}

	GLOBALESTATE GetState( int globalIndex )
	{
		if ( !m_list.IsValidIndex(globalIndex) )
			return GLOBAL_OFF;
		return m_list[globalIndex].state;
	}

	int AddEntity( const char *pGlobalname, const char *pMapName, GLOBALESTATE state )
	{
		globalentity_t entity;
		entity.name = m_nameList.AddString( pGlobalname );
		entity.levelName = m_nameList.AddString( pMapName );
		entity.state = state;
		entity.counter = 0;

		int index = GetIndex( m_nameList.String( entity.name ) );
		if ( index >= 0 )
			return index;
		return m_list.AddToTail( entity );
	}

	void SetCounter( int globalIndex, int counter )
	{
		if ( m_disableStateUpdates || !m_list.IsValidIndex(globalIndex) )
			return;
		m_list[globalIndex].counter = counter;
	}

	int AddToCounter( int globalIndex, int delta )
	{
		if ( m_disableStateUpdates || !m_list.IsValidIndex(globalIndex) )
			return 0;
		return ( m_list[globalIndex].counter += delta );
	}

	int GetCounter( int globalIndex )
	{
		if ( !m_list.IsValidIndex(globalIndex) )
			return 0;
		return m_list[globalIndex].counter;
	}


public:
	CUtlSymbolTable	m_nameList;
private:
	bool			m_disableStateUpdates;
	CUtlVector<globalentity_t> m_list;
};


CGlobalState *gGlobalState = NULL;



int GlobalEntity_GetIndex( const char *pGlobalname )
{
	return gGlobalState->GetIndex( pGlobalname );
}

GLOBALESTATE GlobalEntity_GetState( int globalIndex )
{
	return gGlobalState->GetState( globalIndex );
}

int GlobalEntity_Add( const char *pGlobalname, const char *pMapName, GLOBALESTATE state )
{
	return gGlobalState->AddEntity( pGlobalname, pMapName, state );
}

void GlobalEntity_SetCounter( int globalIndex, int counter )
{
	gGlobalState->SetCounter( globalIndex, counter );
}

int GlobalEntity_AddToCounter( int globalIndex, int delta )
{
	return gGlobalState->AddToCounter( globalIndex, delta );
}

int GlobalEntity_GetCounter( int globalIndex )
{
	return gGlobalState->GetCounter( globalIndex );
}