
#include "choreoevent.h"
#include "choreoscene.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CChoreoEvent::GetEndTime( )
{
	return m_flEndTime;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CChoreoEvent::GetStartTime( )
{
	return m_flStartTime;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CChoreoEvent::HasEndTime( void )
{
	return m_flEndTime != -1.0f ? true : false;
}

float CChoreoEvent::GetIntensity( float scenetime )
{
	float global_intensity = 1.0f;
	if ( m_pScene )
	{
		global_intensity = m_pScene->GetSceneRampIntensity( scenetime );
	}
	else
	{
		Assert( 0 );
	}

	float event_intensity = _GetIntensity( scenetime );

	return global_intensity * event_intensity;
}





float CCurveData::GetIntensity( ICurveDataAccessor *data, float time )
{
	float zeroValue = 0.0f;

	// find samples that span the time
	if ( !data->CurveHasEndTime() )
	{
		return zeroValue;
	}

	int rampCount = GetCount();
	if ( rampCount < 1 )
	{
		// Full intensity
		return 1.0f;
	}

	CExpressionSample *esStart = NULL;
	CExpressionSample *esEnd = NULL;

	// do binary search for sample in time period
	int j = MAX( rampCount / 2, 1 );
	int i = j;
	while ( i > -2 && i < rampCount + 1 )
	{
		bool dummy;
		esStart = GetBoundedSample( data, i, dummy );
		esEnd = GetBoundedSample( data, i + 1, dummy  );

		j = MAX( j / 2, 1 );
		if ( time < esStart->time)
		{
			i -= j;
		}
		else if ( time > esEnd->time)
		{
			i += j;
		}
		else
		{
			break;
		}
	}

	if (!esStart)
	{
		return 1.0f;
	}

	int prev = i - 1;
	int next = i + 2;

	prev = MAX( -1, prev );
	next = MIN( next, rampCount );

	bool clamp[ 2 ];
	CExpressionSample *esPre = GetBoundedSample( data, prev, clamp[ 0 ] );
	CExpressionSample *esNext = GetBoundedSample( data, next, clamp[ 1 ] );

	float dt = esEnd->time - esStart->time;

	Vector vPre( esPre->time, esPre->value, 0 );
	Vector vStart( esStart->time, esStart->value, 0 );
	Vector vEnd( esEnd->time, esEnd->value, 0 );
	Vector vNext( esNext->time, esNext->value, 0 );

	if ( clamp[ 0 ] )
	{
		vPre.x = vStart.x;
	}

	if ( clamp[ 1 ] )
	{
		vNext.x = vEnd.x;
	}

	float f2 = 0.0f;
	if ( dt > 0.0f )
	{
		f2 = ( time - esStart->time ) / ( dt );
	}
	f2 = clamp( f2, 0.0f, 1.0f );

	Vector vOut;
	int dummy;
	int earlypart, laterpart;

	int startCurve	= esStart->GetCurveType();
	int endCurve	= esEnd->GetCurveType();

	if ( startCurve == CURVE_DEFAULT )
	{
		startCurve = data->GetDefaultCurveType();
	}
	if ( endCurve == CURVE_DEFAULT )
	{
		endCurve = data->GetDefaultCurveType();
	}

	// Not holding out value of previous curve...
	Interpolator_CurveInterpolatorsForType( startCurve, dummy, earlypart );
	Interpolator_CurveInterpolatorsForType( endCurve, laterpart, dummy );

	if ( earlypart == INTERPOLATE_HOLD )
	{
		// Hold "out" of previous sample (can cause a discontinuity)
		VectorLerp( vStart, vEnd, f2, vOut );
		vOut.y = vStart.y;
	}
	else if ( laterpart == INTERPOLATE_HOLD )
	{
		// Hold "out" of previous sample (can cause a discontinuity)
		VectorLerp( vStart, vEnd, f2, vOut );
		vOut.y = vEnd.y;
	}
	else
	{
		bool sameCurveType = earlypart == laterpart ? true : false;
		if ( sameCurveType )
		{
			Interpolator_CurveInterpolate( laterpart, vPre, vStart, vEnd, vNext, f2, vOut );
		}
		else // curves differ, sigh
		{
			Vector vOut1, vOut2;

			Interpolator_CurveInterpolate( earlypart, vPre, vStart, vEnd, vNext, f2, vOut1 );
			Interpolator_CurveInterpolate( laterpart, vPre, vStart, vEnd, vNext, f2, vOut2 );

			VectorLerp( vOut1, vOut2, f2, vOut );
		}
	}

	float retval = clamp( vOut.y, 0.0f, 1.0f );
	return retval;
}


CExpressionSample *CCurveData::GetBoundedSample( ICurveDataAccessor *data, int number, bool& bClamped )
{
	// Search for two samples which span time f
	if ( number < 0 )
	{
		static CExpressionSample nullstart;
		nullstart.time = 0.0f;
		nullstart.value = GetEdgeZeroValue( true );
		nullstart.SetCurveType( GetEdgeCurveType( true ) );
		bClamped = true;
		return &nullstart;
	}
	else if ( number >= GetCount() )
	{
		static CExpressionSample nullend;
		nullend.time = data->GetDuration();
		nullend.value = GetEdgeZeroValue( false );
		nullend.SetCurveType( GetEdgeCurveType( false ) );
		bClamped = true;
		return &nullend;
	}
	
	bClamped = false;
	return Get( number );
}

CExpressionSample *CCurveData::Get( int index )
{
	if ( index < 0 || index >= GetCount() )
		return NULL;

	return &m_Ramp[ index ];
}

EdgeInfo_t *CCurveData::GetEdgeInfo( int idx )
{
	return &m_RampEdgeInfo[ idx ];
}

int	 CCurveData::GetCount( void )
{
	return m_Ramp.Count();
}

float CChoreoEvent::_GetIntensity( float scenetime )
{
	// Convert to event local time
	float time = scenetime - GetStartTime();
	return m_Ramp.GetIntensity( this, time );
}


int CCurveData::GetEdgeCurveType( bool leftEdge ) const
{
	if ( !IsEdgeActive( leftEdge ) )
	{
		return CURVE_DEFAULT;
	}

	int idx = leftEdge ? 0 : 1;
	return m_RampEdgeInfo[ idx ].m_CurveType;
}


float CCurveData::GetEdgeZeroValue( bool leftEdge ) const
{
	if ( !IsEdgeActive( leftEdge ) )
	{
		return 0.0f;
	}

	int idx = leftEdge ? 0 : 1;
	return m_RampEdgeInfo[ idx ].m_flZeroPos;
}

bool CCurveData::IsEdgeActive( bool leftEdge ) const
{
	int idx = leftEdge ? 0 : 1;
	return m_RampEdgeInfo[ idx ].m_bActive;
}

const char *CChoreoEvent::GetParameters2( void )
{
	return m_Parameters2.Get();
}

CChoreoEvent::EVENTTYPE CChoreoEvent::GetType( void )
{
	return (EVENTTYPE)m_fType;
}

const char *CChoreoEvent::GetParameters( void )
{
	return m_Parameters.Get();
}

bool CChoreoEvent::IsLockBodyFacing( void )
{
	return m_bLockBodyFacing;
}



