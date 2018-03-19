
#include "CEntity.h"
#include "CAI_NPC.h"
#include "CAI_speech.h"
#include "sceneentity.h"
#include "CE_recipientfilter.h"
#include "CCombatWeapon.h"



// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"




CAI_TimedSemaphore *g_AIFriendliesTalkSemaphore = NULL;
CAI_TimedSemaphore *g_AIFoesTalkSemaphore = NULL;




BEGIN_SIMPLE_DATADESC( CAI_Expresser )
	//									m_pSink		(reconnected on load)
//	DEFINE_FIELD( m_pOuter, CHandle < CBaseFlex > ),
//	DEFINE_CUSTOM_FIELD( m_ConceptHistories,	&g_ConceptHistoriesSaveDataOps ),
	DEFINE_FIELD(		m_flStopTalkTime,		FIELD_TIME		),
	DEFINE_FIELD(		m_flStopTalkTimeWithoutDelay, FIELD_TIME		),
	DEFINE_FIELD(		m_flBlockedTalkTime, 	FIELD_TIME		),
	DEFINE_FIELD(		m_voicePitch,			FIELD_INTEGER	),
	DEFINE_FIELD(		m_flLastTimeAcceptedSpeak, 	FIELD_TIME		),
END_DATADESC()

CAI_Expresser::CAI_Expresser( CBaseEntity *pOuter )
 :	m_pSink( NULL ),
	m_flStopTalkTime( 0 ),
	m_flStopTalkTimeWithoutDelay( 0 ),
	m_flBlockedTalkTime( 0 ),
	m_voicePitch( 100 ),
	m_flLastTimeAcceptedSpeak(0.0f),
	m_pOuter( pOuter )
{
}

CAI_Expresser::~CAI_Expresser()
{
	m_ConceptHistories.Purge();

	CAI_TimedSemaphore *pSemaphore = GetMySpeechSemaphore( GetOuter() );
	if ( pSemaphore )
	{
		if ( pSemaphore->GetOwner() == GetOuter()->BaseEntity() )
			pSemaphore->Release();
	}
}

bool CAI_Expresser::IsValidResponse( ResponseType_t type, const char *pszValue )
{
	if ( type == RESPONSE_SCENE )
	{
		char szInstanceFilename[256];
		GetOuter()->GenderExpandString( pszValue, szInstanceFilename, sizeof( szInstanceFilename ) );
		return ( GetSceneDuration( szInstanceFilename ) > 0 );
	}
	return true;
}

int CAI_Expresser::SpeakRawSentence( const char *pszSentence, float delay, float volume, soundlevel_t soundlevel, CBaseEntity *pListener )
{
	int sentenceIndex = -1;

	if ( !pszSentence )
		return sentenceIndex;

	if ( pszSentence[0] == AI_SP_SPECIFIC_SENTENCE )
	{
		sentenceIndex = SENTENCEG_Lookup( pszSentence );

		if( sentenceIndex == -1 )
		{
			// sentence not found
			return -1;
		}

		CPASAttenuationFilter filter( GetOuter(), soundlevel );
		CEntity::EmitSentenceByIndex( filter, GetOuter()->entindex(), CHAN_VOICE, sentenceIndex, volume, soundlevel, 0, GetVoicePitch());
	}
	else
	{
		sentenceIndex = SENTENCEG_PlayRndSz( GetOuter()->NetworkProp()->edict(), pszSentence, volume, soundlevel, 0, GetVoicePitch() );
	}

	NoteSpeaking( engine->SentenceLength( sentenceIndex ), delay );

	return sentenceIndex;
}




bool CAI_Expresser::IsSpeaking( void )
{
	if ( m_flLastTimeAcceptedSpeak == gpGlobals->curtime ) // only one speak accepted per think
		return true;

	return ( m_flStopTalkTime > gpGlobals->curtime );
}

void CAI_Expresser::NoteSpeaking( float duration, float delay )
{
	duration += delay;
	
	GetSink()->OnStartSpeaking();

	if ( duration <= 0 )
	{
		// no duration :( 
		m_flStopTalkTime = gpGlobals->curtime + 3;
		duration = 0;
	}
	else
	{
		m_flStopTalkTime = gpGlobals->curtime + duration;
	}

	m_flStopTalkTimeWithoutDelay = m_flStopTalkTime - delay;

	if ( GetSink()->UseSemaphore() )
	{
		CAI_TimedSemaphore *pSemaphore = GetMySpeechSemaphore( GetOuter() );
		if ( pSemaphore )
		{
			pSemaphore->Acquire( duration, GetOuter()->BaseEntity() );
		}
	}
}

int CAI_Expresser::GetVoicePitch() const
{
	return m_voicePitch + enginerandom->RandomInt(0,3);
}

CAI_TimedSemaphore *CAI_Expresser::GetMySpeechSemaphore( CEntity *pNpc ) 
{
	if ( !pNpc->MyNPCPointer() )
		return false;

	return (pNpc->MyNPCPointer()->IsPlayerAlly() ? g_AIFriendliesTalkSemaphore : g_AIFoesTalkSemaphore );
}

bool CAI_Expresser::Speak( AIConcept_t concept, const char *modifiers /*= NULL*/, char *pszOutResponseChosen /* = NULL*/, size_t bufsize /* = 0 */, IRecipientFilter *filter /* = NULL */ )
{
	AI_Response *result = SpeakFindResponse( concept, modifiers );
	if ( !result )
	{
		return false;
	}

	bool spoke = SpeakDispatchResponse( concept, result, filter );
	if ( pszOutResponseChosen )
	{
		result->GetResponse( pszOutResponseChosen, bufsize );
	}
	
	return spoke;
}

bool CAI_Expresser::SpeakDispatchResponse( AIConcept_t concept, AI_Response *result, IRecipientFilter *filter /* = NULL */ )
{
	char response[ 256 ];
	result->GetResponse( response, sizeof( response ) );

	float delay = result->GetDelay();
	
	bool spoke = false;

	soundlevel_t soundlevel = result->GetSoundLevel();

	if ( IsSpeaking() && concept[0] != 0 )
	{
		DevMsg( "SpeakDispatchResponse:  Entity ( %i/%s ) already speaking, forcing '%s'\n", GetOuter()->entindex(), GetOuter()->GetEntityName(), concept );

		// Tracker 15911:  Can break the game if we stop an imported map placed lcs here, so only
		//  cancel actor out of instanced scripted scenes.  ywb
		RemoveActorFromScriptedScenes( GetOuter(), true /*instanced scenes only*/ );
		GetOuter()->SentenceStop();

		if ( IsRunningScriptedScene( GetOuter() ) )
		{
			DevMsg( "SpeakDispatchResponse:  Entity ( %i/%s ) refusing to speak due to scene entity, tossing '%s'\n", GetOuter()->entindex(), GetOuter()->GetEntityName(), concept );
			delete result;
			return false;
		}
	}

	switch ( result->GetType() )
	{
	default:
	case RESPONSE_NONE:
		break;

	case RESPONSE_SPEAK:
		{
			if ( !result->ShouldntUseScene() )
			{
				// This generates a fake CChoreoScene wrapping the sound.txt name
				spoke = SpeakAutoGeneratedScene( response, delay );
			}
			else
			{
				float speakTime = GetResponseDuration( result );
				GetOuter()->EmitSound( response );

				DevMsg( "SpeakDispatchResponse:  Entity ( %i/%s ) playing sound '%s'\n", GetOuter()->entindex(), GetOuter()->GetEntityName(), response );
				NoteSpeaking( speakTime, delay );
				spoke = true;
			}
		}
		break;

	case RESPONSE_SENTENCE:
		{
			spoke = ( -1 != SpeakRawSentence( response, delay, VOL_NORM, soundlevel ) ) ? true : false;
		}
		break;

	case RESPONSE_SCENE:
		{
			spoke = SpeakRawScene( response, delay, result, filter );
		}
		break;

	case RESPONSE_RESPONSE:
		{
			// This should have been recursively resolved already
			Assert( 0 );
		}
		break;
	case RESPONSE_PRINT:
		{

		}
		break;
	}

	if ( spoke )
	{
		m_flLastTimeAcceptedSpeak = gpGlobals->curtime;

		if ( result->IsApplyContextToWorld() )
		{
			CEntity *pEntity = CEntity::Instance( engine->PEntityOfEntIndex( 0 ) );
			if ( pEntity )
			{
				pEntity->AddContext( result->GetContext() );
			}
		}
		else
		{
			GetOuter()->AddContext( result->GetContext() );
		}
		SetSpokeConcept( concept, result );
	}
	else
	{
		delete result;
	}

	return spoke;
}

void CAI_Expresser::SetSpokeConcept( AIConcept_t concept, AI_Response *response, bool bCallback )
{
	int idx = m_ConceptHistories.Find( concept );
	if ( idx == m_ConceptHistories.InvalidIndex() )
	{
		ConceptHistory_t h;
		h.timeSpoken = gpGlobals->curtime;
		idx = m_ConceptHistories.Insert( concept, h );
	}

	ConceptHistory_t *slot = &m_ConceptHistories[ idx ];

	slot->timeSpoken = gpGlobals->curtime;
	// Update response info
	if ( response )
	{
		AI_Response *r = slot->response;
		if ( r )
		{
			delete r;
		}

		// FIXME:  Are we leaking AI_Responses?
		slot->response = response;
	}

	if ( bCallback )
		GetSink()->OnSpokeConcept( concept, response );
}


float CAI_Expresser::GetResponseDuration( AI_Response *result )
{
	Assert( result );
	char response[ 256 ];
	result->GetResponse( response, sizeof( response ) );

	switch ( result->GetType() )
	{
	default:
	case RESPONSE_NONE:
		break;
	case RESPONSE_SPEAK:
		{
			return GetOuter()->GetSoundDuration( response, STRING( GetOuter()->GetModelName() ) );
		}
		break;
	case RESPONSE_SENTENCE:
		{
			Assert( 0 );
			return 999.0f;
		}
		break;
	case RESPONSE_SCENE:
		{
			return GetSceneDuration( response );
		}
		break;
	case RESPONSE_RESPONSE:
		{
			// This should have been recursively resolved already
			Assert( 0 );
		}
		break;
	case RESPONSE_PRINT:
		{
			return 1.0;
		}
		break;
	}

	return 0.0f;
}


bool CAI_Expresser::SpeakAutoGeneratedScene( char const *soundname, float delay )
{
	float speakTime = GetOuter()->PlayAutoGeneratedSoundScene( soundname );
	if ( speakTime > 0 )
	{
		NoteSpeaking( speakTime, delay );
		return true;
	}
	return false;
}


bool CAI_Expresser::SpeakRawScene( const char *pszScene, float delay, AI_Response *response, IRecipientFilter *filter /* = NULL */ )
{
	float sceneLength = GetOuter()->PlayScene( pszScene, delay, response, filter );
	if ( sceneLength > 0 )
	{
		NoteSpeaking( sceneLength, delay );
		return true;
	}
	return false;
}


AI_Response *CAI_Expresser::SpeakFindResponse( AIConcept_t concept, const char *modifiers /*= NULL*/ )
{
	IResponseSystem *rs = GetOuter()->GetResponseSystem();
	if ( !rs )
	{
		Assert( !"No response system installed for CAI_Expresser::GetOuter()!!!" );
		return NULL;
	}

	AI_CriteriaSet set;
	// Always include the concept name
	set.AppendCriteria( "concept", concept, CONCEPT_WEIGHT );

	// Always include any optional modifiers
	if ( modifiers != NULL )
	{
		char copy_modifiers[ 255 ];
		const char *pCopy;
		char key[ 128 ] = { 0 };
		char value[ 128 ] = { 0 };

		Q_strncpy( copy_modifiers, modifiers, sizeof( copy_modifiers ) );
		pCopy = copy_modifiers;

		while( pCopy )
		{
			pCopy = SplitContext( pCopy, key, sizeof( key ), value, sizeof( value ), NULL );

			if( *key && *value )
			{
				set.AppendCriteria( key, value, CONCEPT_WEIGHT );
			}
		}
	}

	// Let our outer fill in most match criteria
	GetOuter()->ModifyOrAppendCriteria( set );

	// Append local player criteria to set, but not if this is a player doing the talking
	if ( !GetOuter()->IsPlayer() )
	{
		CPlayer *pPlayer = UTIL_PlayerByIndex( 1 );
		if( pPlayer )
			pPlayer->ModifyOrAppendPlayerCriteria( set );
	}

	// Now that we have a criteria set, ask for a suitable response
	AI_Response *result = new AI_Response;
	Assert( result && "new AI_Response: Returned a NULL AI_Response!" );
	bool found = rs->FindBestResponse( set, *result, this );


	if ( !found )
	{
		//Assert( !"rs->FindBestResponse: Returned a NULL AI_Response!" );
		delete result;
		return NULL;
	}

	char response[ 256 ];
	result->GetResponse( response, sizeof( response ) );

	if ( !response[0] )
	{
		delete result;
		return NULL;
	}

	if ( result->GetOdds() < 100 && enginerandom->RandomInt( 1, 100 ) <= result->GetOdds() )
	{
		delete result;
		return NULL;
	}

	return result;
}


void CAI_ExpresserHost_NPC_DoModifyOrAppendCriteria( CAI_NPC *pSpeaker, AI_CriteriaSet& set )
{
	// Append current activity name
	const char *pActivityName = pSpeaker->GetActivityName( pSpeaker->GetActivity() );
	if ( pActivityName )
	{
  		set.AppendCriteria( "activity", pActivityName );
	}

	static const char *pStateNames[] = { "None", "Idle", "Alert", "Combat", "Scripted", "PlayDead", "Dead" };
	if ( (int)pSpeaker->m_NPCState < (int)ARRAYSIZE(pStateNames) )
	{
		set.AppendCriteria( "npcstate", UTIL_VarArgs( "[NPCState::%s]", pStateNames[pSpeaker->m_NPCState] ) );
	}

	if ( pSpeaker->GetEnemy() )
	{
		set.AppendCriteria( "enemy", pSpeaker->GetEnemy()->GetClassname() );
		set.AppendCriteria( "timesincecombat", "-1" );
	}
	else
	{
		if ( pSpeaker->GetLastEnemyTime() == 0.0 )
			set.AppendCriteria( "timesincecombat", "999999.0" );
		else
			set.AppendCriteria( "timesincecombat", UTIL_VarArgs( "%f", gpGlobals->curtime - pSpeaker->GetLastEnemyTime() ) );
	}

	set.AppendCriteria( "speed", UTIL_VarArgs( "%.3f", pSpeaker->GetSmoothedVelocity().Length() ) );

	CCombatWeapon *weapon = pSpeaker->GetActiveWeapon();
	if ( weapon )
	{
		set.AppendCriteria( "weapon", weapon->GetClassname() );
	}
	else
	{
		set.AppendCriteria( "weapon", "none" );
	}

	CPlayer *pPlayer = UTIL_GetNearestPlayer(pSpeaker->GetAbsOrigin());
	if ( pPlayer )
	{
		Vector distance = pPlayer->GetAbsOrigin() - pSpeaker->GetAbsOrigin();

		set.AppendCriteria( "distancetoplayer", UTIL_VarArgs( "%f", distance.Length() ) );

	}
	else
	{
		set.AppendCriteria( "distancetoplayer", UTIL_VarArgs( "%i", MAX_COORD_RANGE ) );
	}

	if ( pSpeaker->HasCondition( COND_SEE_PLAYER ) )
	{
		set.AppendCriteria( "seeplayer", "1" );
	}
	else
	{
		set.AppendCriteria( "seeplayer", "0" );
	}

	if ( pPlayer && pPlayer->FInViewCone_Entity( pSpeaker->BaseEntity() ) && pPlayer->FVisible_Entity( pSpeaker->BaseEntity() ) )
	{
		set.AppendCriteria( "seenbyplayer", "1" );
	}
	else
	{
		set.AppendCriteria( "seenbyplayer", "0" );
	}
}








ConceptHistory_t::~ConceptHistory_t()
{
	if ( response )
	{
		delete response;
	}
	response = NULL;
}


ConceptHistory_t::ConceptHistory_t( const ConceptHistory_t& src )
{
	timeSpoken = src.timeSpoken;
	response = NULL;
	if ( src.response )
	{
		response = new AI_Response( *src.response );
	}
}

ConceptHistory_t& ConceptHistory_t::operator =( const ConceptHistory_t& src )
{
	if ( this == &src )
		return *this;

	timeSpoken = src.timeSpoken;
	response = NULL;
	if ( src.response )
	{
		response = new AI_Response( *src.response );
	}

	return *this;
}

