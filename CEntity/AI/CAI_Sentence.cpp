
#include "CAI_NPC.h"
#include "CAI_Sentence.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



BEGIN_SIMPLE_DATADESC(CAI_SentenceBase)
	DEFINE_FIELD( m_voicePitch, FIELD_INTEGER ),
	DEFINE_FIELD( m_nQueuedSentenceIndex, FIELD_INTEGER ),
	DEFINE_FIELD( m_flQueueTimeout, FIELD_TIME ),
	DEFINE_FIELD( m_nQueueSoundPriority, FIELD_INTEGER ),
END_DATADESC();



CAI_SentenceBase::CAI_SentenceBase()
{
	m_voicePitch = 0;
	m_nQueuedSentenceIndex = 0;
	m_flQueueTimeout = 0.0f;
	m_nQueueSoundPriority = 0;
	ClearQueue();
}


int CAI_SentenceBase::Speak( const char *pSentence, SentencePriority_t nSoundPriority, SentenceCriteria_t nCriteria )
{
	if ( !MatchesCriteria(nCriteria) )
		return -1;

	// Speaking clears the queue
	ClearQueue();

	if ( nSoundPriority == SENTENCE_PRIORITY_INVALID )
	{
		return PlaySentence( pSentence );
	}

	int nSentenceIndex = -1;
	if ( GetOuter()->FOkToMakeSound( nSoundPriority ) )
	{
		nSentenceIndex = PlaySentence( pSentence );

		// Make sure sentence length utility works
//		float flSentenceTime = enginesound->GetSoundDuration( nSentenceIndex );
		GetOuter()->JustMadeSound( nSoundPriority, 2.0f /*flSentenceTime*/ );
	}
	else
	{
		SentenceMsg( "CULL", pSentence );
	}

	return nSentenceIndex;
}


void CAI_SentenceBase::UpdateSentenceQueue()
{
	if ( m_nQueuedSentenceIndex == -1 )
		return;

	// Check for timeout
	if ( m_flQueueTimeout < gpGlobals->curtime )
	{
		ClearQueue();
		return;
	}

	if ( GetOuter()->FOkToMakeSound( m_nQueueSoundPriority ) )
	{
		SENTENCEG_PlaySentenceIndex( GetOuter()->edict(), m_nQueuedSentenceIndex, GetVolume(), GetSoundLevel(), 0, GetVoicePitch() );

		const char *pSentenceName = engine->SentenceNameFromIndex( m_nQueuedSentenceIndex ); 
		SentenceMsg( "Speaking [from QUEUE]", pSentenceName );

		GetOuter()->JustMadeSound( m_nQueueSoundPriority );
		ClearQueue();
	}
}


void CAI_SentenceBase::ClearQueue()
{
	m_nQueuedSentenceIndex = -1;
}

extern ConVar *npc_sentences;

void CAI_SentenceBase::SentenceMsg( const char *pStatus, const char *pSentence )
{
}

int CAI_SentenceBase::PlaySentence( const char *pSentence )
{
	int nSentenceIndex = SENTENCEG_PlayRndSz( GetOuter()->edict(), pSentence, GetVolume(), GetSoundLevel(), 0, GetVoicePitch());
	if ( nSentenceIndex < 0 )
	{
		SentenceMsg( "BOGUS", pSentence );
		return -1;
	}

	const char *pSentenceName = engine->SentenceNameFromIndex( nSentenceIndex ); 
	SentenceMsg( "Speaking", pSentenceName );
	return nSentenceIndex;
}

bool CAI_SentenceBase::MatchesCriteria( SentenceCriteria_t nCriteria )
{
	switch(	nCriteria )
	{
	case SENTENCE_CRITERIA_ALWAYS:
		return true;

	case SENTENCE_CRITERIA_NORMAL:
		return (GetOuter()->GetState() == NPC_STATE_COMBAT) || (GetOuter()->HasSpawnFlags( SF_NPC_GAG ) == 0);

	case SENTENCE_CRITERIA_IN_SQUAD:
		if ( (GetOuter()->GetState() != NPC_STATE_COMBAT) && GetOuter()->HasSpawnFlags( SF_NPC_GAG ) )
			return false;
		return GetOuter()->GetSquad() && (GetOuter()->GetSquad()->NumMembers() > 1);

	case SENTENCE_CRITERIA_SQUAD_LEADER:
		{
			if ( (GetOuter()->GetState() != NPC_STATE_COMBAT) && GetOuter()->HasSpawnFlags( SF_NPC_GAG ) )
				return false;

			CAI_Squad *pSquad = GetOuter()->GetSquad();
			return pSquad && (pSquad->NumMembers() > 1) && pSquad->IsLeader( GetOuter() );
		}
	}

	return true;
}




