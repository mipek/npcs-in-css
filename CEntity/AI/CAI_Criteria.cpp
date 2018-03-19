
#include "CEntity.h"
#include "soundflags.h"
#include "CAI_Criteria.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



float RandomInterval( const interval_t &interval )
{
	float out = interval.start;
	if ( interval.range != 0 )
	{
		out += RandomFloat( 0, interval.range );
	}

	return out;
}

AI_CriteriaSet::AI_CriteriaSet() : m_Lookup( 0, 0, CritEntry_t::LessFunc )
{
}

AI_CriteriaSet::~AI_CriteriaSet()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *criteria - 
//			"" - 
//			1.0f - 
//-----------------------------------------------------------------------------
void AI_CriteriaSet::AppendCriteria( const char *criteria, const char *value /*= ""*/, float weight /*= 1.0f*/ )
{
	int idx = FindCriterionIndex( criteria );
	if ( idx == -1 )
	{
		CritEntry_t entry;
		entry.criterianame = criteria;
		MEM_ALLOC_CREDIT();
		idx = m_Lookup.Insert( entry );
	}

	CritEntry_t *entry = &m_Lookup[ idx ];

	entry->SetValue( value );
	entry->weight = weight;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *name - 
// Output : int
//-----------------------------------------------------------------------------
int AI_CriteriaSet::FindCriterionIndex( const char *name ) const
{
	CritEntry_t search;
	search.criterianame = name;
	int idx = m_Lookup.Find( search );
	if ( idx == m_Lookup.InvalidIndex() )
		return -1;

	return idx;
}


BEGIN_SIMPLE_DATADESC( AI_ResponseParams )
	DEFINE_FIELD( flags,	FIELD_SHORT ),
	DEFINE_FIELD( odds,	FIELD_SHORT ),	
	DEFINE_FIELD( soundlevel,	FIELD_CHARACTER ),	
	DEFINE_FIELD( delay,	FIELD_INTEGER ),		// These are compressed down to two float16s, so treat as an INT for saverestore
	DEFINE_FIELD( respeakdelay,	FIELD_INTEGER ),	//  
END_DATADESC()

BEGIN_SIMPLE_DATADESC( AI_Response )
	DEFINE_FIELD( m_Type,	FIELD_CHARACTER ),
	DEFINE_ARRAY( m_szResponseName, FIELD_CHARACTER, AI_Response::MAX_RESPONSE_NAME ),	
	DEFINE_ARRAY( m_szMatchingRule, FIELD_CHARACTER, AI_Response::MAX_RULE_NAME ),	
	// DEFINE_FIELD( m_pCriteria, FIELD_??? ), // Don't need to save this probably
	DEFINE_EMBEDDED( m_Params ),
END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
AI_Response::AI_Response()
{
	m_Type = RESPONSE_NONE;
	m_szResponseName[0] = 0;
	m_pCriteria = NULL;
	m_szMatchingRule[0]=0;
	m_szContext = NULL;
	m_bApplyContextToWorld = false;
}

AI_Response::AI_Response( const AI_Response &from )
{
	Assert( (void*)(&m_Type) == (void*)this );
	m_pCriteria = NULL;
	memcpy( this, &from, sizeof(*this) );
	m_pCriteria = NULL;
	m_szContext = NULL;
	SetContext( from.m_szContext );
	m_bApplyContextToWorld = from.m_bApplyContextToWorld;
}

AI_Response::~AI_Response()
{
	if(m_pCriteria)
	{
		m_pCriteria->m_Lookup.Purge();
		g_pMemAlloc->Free(m_pCriteria);
	}
	if(m_szContext)
	{
		g_pMemAlloc->Free(m_szContext);
	}
	delete[] m_szContext;
}

void AI_Response::SetContext( const char *context )
{
	g_pMemAlloc->Free(m_szContext);
	m_szContext = NULL;

	if ( context )
	{
		int len = Q_strlen( context );
		m_szContext = new char[ len + 1 ];
		Q_memcpy( m_szContext, context, len );
		m_szContext[ len ] = 0;
	}
}

bool AI_Response::ShouldBreakOnNonIdle( void ) const
{
	return ( m_Params.flags & AI_ResponseParams::RG_STOP_ON_NONIDLE ) != 0;
}

float AI_Response::GetPreDelay() const
{
	if ( m_Params.flags & AI_ResponseParams::RG_DELAYBEFORESPEAK )
	{
		interval_t temp;
		m_Params.predelay.ToInterval( temp );
		return RandomInterval( temp );
	}
	return 0.0f;
}

void AI_Response::GetResponse( char *buf, size_t buflen ) const
{
	GetName( buf, buflen );
}

void AI_Response::GetName( char *buf, size_t buflen ) const
{
	Q_strncpy( buf, m_szResponseName, buflen );
}

bool AI_Response::ShouldntUseScene( void ) const
{
	return ( m_Params.flags & AI_ResponseParams::RG_DONT_USE_SCENE ) != 0;
}


float AI_Response::GetDelay() const
{
	if ( m_Params.flags & AI_ResponseParams::RG_DELAYAFTERSPEAK )
	{
		interval_t temp;
		m_Params.delay.ToInterval( temp );
		return RandomInterval( temp );
	}
	return 0.0f;
}

int AI_Response::GetOdds( void ) const
{
	if ( m_Params.flags & AI_ResponseParams::RG_ODDS )
	{
		return m_Params.odds;
	}
	return 100;
}

soundlevel_t AI_Response::GetSoundLevel() const
{
	if ( m_Params.flags & AI_ResponseParams::RG_SOUNDLEVEL )
	{
		return (soundlevel_t)m_Params.soundlevel;
	}

	return SNDLVL_TALKING;
}


const char *SplitContext( const char *raw, char *key, int keylen, char *value, int valuelen, float *duration )
{
	char *colon1 = Q_strstr( raw, ":" );
	if ( !colon1 )
	{
		DevMsg( "SplitContext:  warning, ignoring context '%s', missing colon separator!\n", raw );
		*key = *value = 0;
		return NULL;
	}

	int len = colon1 - raw;
	Q_strncpy( key, raw, MIN( len + 1, keylen ) );
	key[ MIN( len, keylen - 1 ) ] = 0;

	bool last = false;
	char *end = Q_strstr( colon1 + 1, "," );
	if ( !end )
	{
		int remaining = Q_strlen( colon1 + 1 );
		end = colon1 + 1 + remaining;
		last = true;
	}

	char *colon2 = Q_strstr( colon1 + 1, ":" );
	if ( colon2 && ( colon2 < end ) )
	{
		if ( duration )
			*duration = atof( colon2 + 1 );

		len = MIN( colon2 - ( colon1 + 1 ), valuelen - 1 );
		Q_strncpy( value, colon1 + 1, len + 1 );
		value[ len ] = 0;
	}
	else
	{
		if ( duration )
			*duration = 0.0;

		len = MIN( end - ( colon1 + 1 ), valuelen - 1 );
		Q_strncpy( value, colon1 + 1, len + 1 );
		value[ len ] = 0;
	}

	return last ? NULL : end + 1;
}




