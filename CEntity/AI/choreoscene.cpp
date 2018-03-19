
#include "choreoscene.h"



// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"




//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CChoreoScene::FindStopTime( void )
{
	if ( m_flPrecomputedStopTime != 0.0f )
	{
		return m_flPrecomputedStopTime;
	}

	float lasttime = 0.0f;

	int c = m_Events.Count();
	for ( int i = 0; i < c ; i++ )
	{
		CChoreoEvent *e = m_Events[ i ];
		Assert( e );

		float checktime = e->HasEndTime() ? e->GetEndTime() : e->GetStartTime();
		if ( checktime > lasttime )
		{
			lasttime = checktime;
		}
	}

	return lasttime;
}



//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CChoreoScene::GetTime( void )
{
	return m_flCurrentTime;
}

bool CChoreoScene::IsBackground( )
{
	return m_bIsBackground;
}

char const *CChoreoScene::GetFilename() const
{
	return m_szFileName;
}

void CChoreoScene::SetBackground( bool bIsBackground )
{
	m_bIsBackground = bIsBackground;
}

int CChoreoScene::GetNumActors( void )
{
	return m_Actors.Size();
}

