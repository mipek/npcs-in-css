#ifndef _INCLUDE_CEAI_SCRIPTEDSEQUENCE_H_
#define _INCLUDE_CEAI_SCRIPTEDSEQUENCE_H_

#include "CEntity.h"
#include "CAI_NPC.h"


//
// The number of unique outputs that a script can fire from animation events.
// These are fired via SCRIPT_EVENT_FIREEVENT in CAI_BaseNPC::HandleAnimEvent.
//
#define MAX_SCRIPT_EVENTS				8


#define SF_SCRIPT_WAITTILLSEEN			1
#define SF_SCRIPT_EXITAGITATED			2
#define SF_SCRIPT_REPEATABLE			4		// Whether the script can be played more than once.
#define SF_SCRIPT_LEAVECORPSE			8
#define SF_SCRIPT_START_ON_SPAWN		16
#define SF_SCRIPT_NOINTERRUPT			32
#define SF_SCRIPT_OVERRIDESTATE			64
#define SF_SCRIPT_DONT_TELEPORT_AT_END	128		// Don't fixup end position with a teleport when the SS is finished
#define SF_SCRIPT_LOOP_IN_POST_IDLE		256		// Loop in the post idle animation after playing the action animation.
#define SF_SCRIPT_HIGH_PRIORITY			512		// If set, we don't allow other scripts to steal our spot in the queue.
#define SF_SCRIPT_SEARCH_CYCLICALLY		1024	// Start search from last entity found.
#define SF_SCRIPT_NO_COMPLAINTS			2048	// doesn't bitch if it can't find anything
#define SF_SCRIPT_ALLOW_DEATH			4096	// the actor using this scripted sequence may die without interrupting the scene (used for scripted deaths)


enum script_moveto_t
{
	CINE_MOVETO_WAIT = 0,
	CINE_MOVETO_WALK = 1,
	CINE_MOVETO_RUN = 2,
	CINE_MOVETO_CUSTOM = 3,
	CINE_MOVETO_TELEPORT = 4,
	CINE_MOVETO_WAIT_FACING = 5,
};

enum SCRIPT_PLAYER_DEATH
{
	SCRIPT_DO_NOTHING = 0,
	SCRIPT_CANCEL = 1,
};


//
// Interrupt levels for grabbing NPCs to act out scripted events. These indicate
// how important it is to get a specific NPC, and can affect how they respond.
//
enum SS_INTERRUPT
{
	SS_INTERRUPT_BY_CLASS = 0,		// Indicates that we are asking for this NPC by class
	SS_INTERRUPT_BY_NAME,			// Indicates that we are asking for this NPC by name
};


// when a NPC finishes an AI scripted sequence, we can choose
// a schedule to place them in. These defines are the aliases to
// resolve worldcraft input to real schedules (sjb)
#define SCRIPT_FINISHSCHED_DEFAULT	0
#define SCRIPT_FINISHSCHED_AMBUSH	1



class CEAI_ScriptedSequence : public CEntity
{
public:
	CE_DECLARE_CLASS( CEAI_ScriptedSequence, CEntity );

public:
	void CancelScript( void );
	void FixScriptNPCSchedule( CAI_NPC *pNPC, int iSavedCineFlags );
	void FixFlyFlag( CAI_NPC *pNPC, int iSavedCineFlags );

	void SetTarget( CBaseEntity *pTarget ) { m_hTargetEnt.ptr->Set(pTarget); };
	CEntity *GetTarget( void ) { return m_hTargetEnt; };

	bool IsPlayingAction( void ) { return ( (*(m_sequenceStarted)) && !(*(m_bIsPlayingEntry)) ); }
	void StopActionLoop( bool bStopSynchronizedScenes );

public:
	static void ScriptEntityCancel( CEntity *pentCine, bool bPretendSuccess = false );

public:
	DECLARE_DATAMAP(string_t, m_iszCustomMove);
	DECLARE_DATAMAP(string_t, m_iszPlay);
	DECLARE_DATAMAP(bool, m_bDontCancelOtherSequences);
	DECLARE_DATAMAP(float, m_startTime);
	DECLARE_DATAMAP(CFakeHandle, m_hTargetEnt);
	DECLARE_DATAMAP(int, m_savedFlags);
	DECLARE_DATAMAP(int, m_saved_effects);
	DECLARE_DATAMAP(int, m_iDelay);
	DECLARE_DATAMAP(COutputEvent, m_OnEndSequence);
	DECLARE_DATAMAP(COutputEvent, m_OnPostIdleEndSequence);
	DECLARE_DATAMAP(COutputEvent, m_OnCancelSequence);
	DECLARE_DATAMAP(COutputEvent, m_OnCancelFailedSequence);
	DECLARE_DATAMAP(bool, m_sequenceStarted);
	DECLARE_DATAMAP(bool, m_bIsPlayingEntry);
	DECLARE_DATAMAP(bool, m_bLoopActionSequence);

	

};

#endif
