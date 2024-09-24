
#include "CPathTrack.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CE_LINK_ENTITY_TO_CLASS(CPathTrack, CE_CPathTrack);

DEFINE_PROP(m_paltpath, CE_CPathTrack);
DEFINE_PROP(m_pprevious, CE_CPathTrack);
DEFINE_PROP(m_pnext, CE_CPathTrack);
DEFINE_PROP(m_flRadius, CE_CPathTrack);
DEFINE_PROP(m_nIterVal, CE_CPathTrack);



int *CE_CPathTrack::s_nCurrIterVal = NULL;
bool *CE_CPathTrack::s_bIsIterating = NULL;


CE_CPathTrack *CE_CPathTrack::ValidPath( CE_CPathTrack *ppath, int testFlag )
{
	if ( !ppath )
		return NULL;

	if ( testFlag && FBitSet( ppath->m_spawnflags, SF_PATH_DISABLED ) )
		return NULL;

	return ppath;
}

CE_CPathTrack *CE_CPathTrack::GetNext( void )
{
	CE_CPathTrack *path = m_paltpath->Get();
	if ( path && FBitSet( m_spawnflags, SF_PATH_ALTERNATE ) && !FBitSet( m_spawnflags, SF_PATH_ALTREVERSE ) )
	{
		return path;
	}
	return m_pnext->Get();
}


CE_CPathTrack *CE_CPathTrack::GetPrevious( void )
{
	CE_CPathTrack *path = m_paltpath->Get();
	if ( path && FBitSet( m_spawnflags, SF_PATH_ALTERNATE ) && FBitSet( m_spawnflags, SF_PATH_ALTREVERSE ) )
	{
		return path;
	}
	return m_pprevious->Get();
}


//-----------------------------------------------------------------------------
// Circular path checking
//-----------------------------------------------------------------------------
void CE_CPathTrack::BeginIteration()
{
	Assert( !(*(s_bIsIterating)) );
	++(*(s_nCurrIterVal));
	*(s_bIsIterating) = true;
}

void CE_CPathTrack::EndIteration()
{
	Assert( *(s_bIsIterating) );
	*(s_bIsIterating) = false;
}

void CE_CPathTrack::Visit()
{
	m_nIterVal = *(s_nCurrIterVal);
}

bool CE_CPathTrack::HasBeenVisited() const
{
	return ( m_nIterVal == *(s_nCurrIterVal) );
}
