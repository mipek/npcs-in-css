//========= Copyright ?1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//
//=============================================================================//
#include "CEntity.h"
#include "CE_recipientfilter.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CRecipientFilter::CRecipientFilter()
{
	Reset();
}

CRecipientFilter::~CRecipientFilter()
{
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : src - 
//-----------------------------------------------------------------------------
void CRecipientFilter::CopyFrom( const CRecipientFilter& src )
{
	m_bReliable = src.IsReliable();
	m_bInitMessage = src.IsInitMessage();

	m_bUsingPredictionRules = src.IsUsingPredictionRules();
	m_bIgnorePredictionCull = src.IgnorePredictionCull();

	int c = src.GetRecipientCount();
	for ( int i = 0; i < c; ++i )
	{
		m_Recipients.AddToTail( src.GetRecipientIndex( i ) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CRecipientFilter::Reset( void )
{
	m_bReliable			= false;
	m_bInitMessage		= false;
	m_Recipients.RemoveAll();
	m_bUsingPredictionRules = false;
	m_bIgnorePredictionCull = false;
}

void CRecipientFilter::MakeReliable( void )
{
	m_bReliable = true;
}

bool CRecipientFilter::IsReliable( void ) const
{
	return m_bReliable;
}

int CRecipientFilter::GetRecipientCount( void ) const
{
	return m_Recipients.Size();
}

int	CRecipientFilter::GetRecipientIndex( int slot ) const
{
	if ( slot < 0 || slot >= GetRecipientCount() )
		return -1;

	return m_Recipients[ slot ];
}

void CRecipientFilter::AddRecipientsByPAS( const Vector& origin )
{
	if ( gpGlobals->maxClients == 1 )
	{
		AddAllPlayers();
	}
	else
	{
		CBitVec< ABSOLUTE_PLAYER_LIMIT > playerbits;
		engine->Message_DetermineMulticastRecipients( true, origin, playerbits );
		AddPlayersFromBitMask( playerbits );
	}
}

bool CRecipientFilter::IsInitMessage( void ) const
{
	return m_bInitMessage;
}

void CRecipientFilter::MakeInitMessage( void )
{
	m_bInitMessage = true;
}

void CRecipientFilter::AddRecipientsByPVS( const Vector& origin )
{
	if ( gpGlobals->maxClients == 1 )
	{
		AddAllPlayers();
	}
	else
	{
		CBitVec< ABSOLUTE_PLAYER_LIMIT > playerbits;
		engine->Message_DetermineMulticastRecipients( false, origin, playerbits );
		AddPlayersFromBitMask( playerbits );
	}
}

void CRecipientFilter::AddPlayersFromBitMask( CBitVec< ABSOLUTE_PLAYER_LIMIT >& playerbits )
{
	int index = playerbits.FindNextSetBit( 0 );

	while ( index > -1 )
	{
		CPlayer *pPlayer = UTIL_PlayerByIndex( index + 1 );
		if ( pPlayer )
		{
			AddRecipient( pPlayer );
		}

		index = playerbits.FindNextSetBit( index + 1 );
	}
}


bool CRecipientFilter::IsUsingPredictionRules( void ) const
{
	return m_bUsingPredictionRules;
}

bool CRecipientFilter::	IgnorePredictionCull( void ) const
{
	return m_bIgnorePredictionCull;
}

void CRecipientFilter::SetIgnorePredictionCull( bool ignore )
{
	m_bIgnorePredictionCull = ignore;
}

void CRecipientFilter::RemoveRecipient( CPlayer *player )
{
	Assert( player );
	if ( player )
	{
		int index = player->entindex();

		// Remove it if it's in the list
		m_Recipients.FindAndRemove( index );
	}
}

void CRecipientFilter::AddRecipient( CPlayer *player )
{
	Assert( player );

	int index = player->entindex();

	// If we're predicting and this is not the first time we've predicted this sound
	//  then don't send it to the local player again.
	/*if ( m_bUsingPredictionRules )
	{
		// Only add local player if this is the first time doing prediction
		if ( g_RecipientFilterPredictionSystem.GetSuppressHost() == player )
		{
			return;
		}
	}*/

	// Already in list
	if ( m_Recipients.Find( index ) != m_Recipients.InvalidIndex() )
		return;

	m_Recipients.AddToTail( index );
}

void CRecipientFilter::AddAllPlayers( void )
{
	m_Recipients.RemoveAll();

	int i;
	for ( i = 1; i <= gpGlobals->maxClients; i++ )
	{
		CPlayer *pPlayer = UTIL_PlayerByIndex( i );
		if ( !pPlayer )
		{
			continue;
		}

		AddRecipient( pPlayer );
	}
}
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : origin - 
//			ATTN_NORM - 
//-----------------------------------------------------------------------------
void CPASAttenuationFilter::Filter( const Vector& origin, float attenuation /*= ATTN_NORM*/ )
{
	// Don't crop for attenuation in single player
	if ( gpGlobals->maxClients == 1 )
		return;

	// CPASFilter adds them by pure PVS in constructor
	if ( attenuation <= 0 )
		return;

	// Now remove recipients that are outside sound radius
	float distance, maxAudible;
	Vector vecRelative;

	int c = GetRecipientCount();
	
	for ( int i = c - 1; i >= 0; i-- )
	{
		int index = GetRecipientIndex( i );

		CEntity *ent = CEntity::Instance( index );
		if ( !ent || !ent->IsPlayer() )
		{
			Assert( 0 );
			continue;
		}

		CPlayer *player = ToBasePlayer( ent );
		if ( !player )
		{
			Assert( 0 );
			continue;
		}

#ifndef _XBOX
		// never remove the HLTV bot
		if ( player->IsHLTV() )
			continue;
#endif

		VectorSubtract( player->EarPosition(), origin, vecRelative );
		distance = VectorLength( vecRelative );
		maxAudible = ( 2 * SOUND_NORMAL_CLIP_DIST ) / attenuation;
		if ( distance <= maxAudible )
			continue;

		RemoveRecipient( player );
	}
}
