
#include "CEntity.h"
#include "CEAI_ScriptedSequence.h"



// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



CE_LINK_ENTITY_TO_CLASS(CAI_ScriptedSequence, CEAI_ScriptedSequence);


//Datamaps
DEFINE_PROP(m_iszCustomMove, CEAI_ScriptedSequence);
DEFINE_PROP(m_iszPlay, CEAI_ScriptedSequence);
DEFINE_PROP(m_bDontCancelOtherSequences, CEAI_ScriptedSequence);
DEFINE_PROP(m_startTime, CEAI_ScriptedSequence);
DEFINE_PROP(m_hTargetEnt, CEAI_ScriptedSequence);
DEFINE_PROP(m_savedFlags, CEAI_ScriptedSequence);
DEFINE_PROP(m_saved_effects, CEAI_ScriptedSequence);
DEFINE_PROP(m_iDelay, CEAI_ScriptedSequence);
DEFINE_PROP(m_OnEndSequence, CEAI_ScriptedSequence);
DEFINE_PROP(m_OnPostIdleEndSequence, CEAI_ScriptedSequence);
DEFINE_PROP(m_OnCancelFailedSequence, CEAI_ScriptedSequence);
DEFINE_PROP(m_OnCancelSequence, CEAI_ScriptedSequence);
DEFINE_PROP(m_sequenceStarted, CEAI_ScriptedSequence);
DEFINE_PROP(m_bIsPlayingEntry, CEAI_ScriptedSequence);
DEFINE_PROP(m_bLoopActionSequence, CEAI_ScriptedSequence);



#define CLASSNAME "scripted_sequence"

//-----------------------------------------------------------------------------
// Purpose: Cancels the given scripted sequence.
// Input  : pentCine - 
//-----------------------------------------------------------------------------
void CEAI_ScriptedSequence::ScriptEntityCancel( CEntity *pentCine, bool bPretendSuccess )
{
// make sure they are a scripted_sequence
	if ( FClassnameIs( pentCine, CLASSNAME ) )
	{
		CEAI_ScriptedSequence *pCineTarget = (CEAI_ScriptedSequence *)pentCine;

		// make sure they have a NPC in mind for the script
		CEntity *pEntity = pCineTarget->GetTarget();
		CAI_NPC	*pTarget = NULL;
		if ( pEntity )
			pTarget = pEntity->MyNPCPointer();

		if (pTarget)
		{
			// make sure their NPC is actually playing a script
			if ( pTarget->m_NPCState == NPC_STATE_SCRIPT )
			{
				// tell them do die
				pTarget->m_scriptState = CAI_NPC::SCRIPT_CLEANUP;

				CEAI_ScriptedSequence *_m_hCine = pTarget->Get_m_hCine();
				// We have to save off the flags here, because the NPC's m_hCine is cleared in CineCleanup()
				int iSavedFlags = (_m_hCine ? _m_hCine->m_savedFlags : 0);

				// do it now	
				pTarget->CineCleanup();
				pCineTarget->FixScriptNPCSchedule( pTarget, iSavedFlags );
			}
			else
			{
				// Robin HACK: If a script is started and then cancelled before an NPC gets to
				//		 think, we have to manually clear it out of scripted state, or it'll never recover.
				pCineTarget->SetTarget( NULL );
				pTarget->SetEffects( pCineTarget->m_saved_effects );
				pTarget->m_hCine.ptr->Set(NULL);
				pTarget->SetTarget( NULL );
				pTarget->SetGoalEnt( NULL );
				pTarget->SetIdealState( NPC_STATE_IDLE );
			}
		}

		// FIXME: this needs to be done in a cine cleanup function
		pCineTarget->m_iDelay = 0;

		if ( bPretendSuccess )
		{
			// We need to pretend that this sequence actually finished fully
			pCineTarget->m_OnEndSequence->FireOutput((CBaseEntity *)NULL, pCineTarget);
			pCineTarget->m_OnPostIdleEndSequence->FireOutput((CBaseEntity *)NULL, pCineTarget);
		}
		else
		{
			// Fire the cancel
 			pCineTarget->m_OnCancelSequence->FireOutput((CBaseEntity *)NULL, pCineTarget);

			if ( *(pCineTarget->m_startTime.ptr) == 0 )
			{
				// If start time is 0, this sequence never actually ran. Fire the failed output.
				pCineTarget->m_OnCancelFailedSequence->FireOutput((CBaseEntity *)NULL, pCineTarget);
			}
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: Find all the cinematic entities with my targetname and stop them
//			from playing.
//-----------------------------------------------------------------------------
void CEAI_ScriptedSequence::CancelScript( void )
{	
	// Don't cancel matching sequences if we're asked not to, unless we didn't actually
	// succeed in starting, in which case we should always cancel. This fixes
	// dynamic interactions where an NPC was killed the same frame another NPC
	// started a dynamic interaction with him.
	bool bDontCancelOther = ((m_bDontCancelOtherSequences || HasSpawnFlags( SF_SCRIPT_ALLOW_DEATH ) )&& (*(m_startTime.ptr) != 0));
	if ( bDontCancelOther || !GetEntityName() )
	{
		ScriptEntityCancel( this );
		return;
	}

	CEntity *pentCineTarget = g_helpfunc.FindEntityByName( (CBaseEntity *)NULL, GetEntityName() );

	while ( pentCineTarget )
	{
		ScriptEntityCancel( pentCineTarget );
		pentCineTarget = g_helpfunc.FindEntityByName( pentCineTarget->BaseEntity(), GetEntityName() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: When a NPC finishes a scripted sequence, we have to fix up its state
//			and schedule for it to return to a normal AI NPC.
//			Scripted sequences just dirty the Schedule and drop the NPC in Idle State.
// Input  : *pNPC - 
//-----------------------------------------------------------------------------
void CEAI_ScriptedSequence::FixScriptNPCSchedule( CAI_NPC *pNPC, int iSavedCineFlags )
{
	if ( pNPC->GetIdealState() != NPC_STATE_DEAD )
	{
		pNPC->SetIdealState( NPC_STATE_IDLE );
	}

	if ( pNPC == NULL )
		 return;

	FixFlyFlag( pNPC, iSavedCineFlags );

	pNPC->ClearSchedule( "Finished scripted sequence" );
}

void CEAI_ScriptedSequence::FixFlyFlag( CAI_NPC *pNPC, int iSavedCineFlags )
{
	//Adrian: We NEED to clear this or the NPC's FL_FLY flag will never be removed cause of ClearSchedule!
	if ( pNPC->GetTask() && ( pNPC->GetTask()->iTask == TASK_PLAY_SCRIPT || pNPC->GetTask()->iTask == TASK_PLAY_SCRIPT_POST_IDLE ) )
	{
		if ( !(iSavedCineFlags & FL_FLY) )
		{
			if ( pNPC->GetFlags() & FL_FLY )
			{
				 pNPC->RemoveFlag( FL_FLY );
			}
		}
	}
}


void CEAI_ScriptedSequence::StopActionLoop( bool bStopSynchronizedScenes )
{
	// Stop looping our action sequence. Next time the loop finishes,
	// we'll move to the post idle sequence instead.
	m_bLoopActionSequence = false;

	// If we have synchronized scenes, and we're supposed to stop them, do so
	if ( !bStopSynchronizedScenes || *(m_iName) == NULL_STRING )
		return;

	CEntity *pentCine = g_helpfunc.FindEntityByName( (CBaseEntity *)NULL, GetEntityName(), NULL );
	while ( pentCine )
	{
		CEAI_ScriptedSequence *pScene = dynamic_cast<CEAI_ScriptedSequence *>(pentCine);
		if ( pScene && pScene != this )
		{
			pScene->StopActionLoop( false );
		}

		pentCine = g_helpfunc.FindEntityByName( pentCine, GetEntityName(), NULL );
	}
}