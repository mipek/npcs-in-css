//========= Copyright ?1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
//=============================================================================//

#ifndef AI_BEHAVIOR_ASSAULT_H
#define AI_BEHAVIOR_ASSAULT_H
#ifdef _WIN32
#pragma once
#endif

#include "simtimer.h"
#include "CAI_Behavior.h"
#include "CAI_goalentity.h"
#include "CAI_Moveshoot.h"
#include "CAI_utils.h"

#define CUE_POINT_TOLERANCE (3.0*12.0)

enum RallySelectMethod_t
{
	RALLY_POINT_SELECT_DEFAULT = 0,
	RALLY_POINT_SELECT_RANDOM,
};

enum AssaultCue_t
{
	CUE_NO_ASSAULT = 0,	// used to indicate that no assault is being conducted presently

	CUE_ENTITY_INPUT = 1,
	CUE_PLAYER_GUNFIRE,
	CUE_DONT_WAIT,
	CUE_COMMANDER,
	CUE_NONE,
};

enum
{
	ASSAULT_SENTENCE_HIT_RALLY_POINT = SENTENCE_BASE_BEHAVIOR_INDEX,
	ASSAULT_SENTENCE_HIT_ASSAULT_POINT,
	ASSAULT_SENTENCE_SQUAD_ADVANCE_TO_RALLY,
	ASSAULT_SENTENCE_SQUAD_ADVANCE_TO_ASSAULT,
	ASSAULT_SENTENCE_COVER_NO_AMMO,
	ASSAULT_SENTENCE_UNDER_ATTACK,
};

// Allow diversion from the assault up to this amount of time after last having an enemy
#define ASSAULT_DIVERSION_TIME		4

#define SF_ASSAULTPOINT_CLEARONARRIVAL	0x00000001

//=============================================================================
//=============================================================================
class CE_CRallyPoint : public CEntity 
{
public:
	CE_DECLARE_CLASS( CE_CRallyPoint, CEntity );

	bool Lock( CEntity *pLocker )
	{
		if( IsLocked() )
		{
			// Already locked.
			return false;
		}

		m_hLockedBy.ptr->Set( (pLocker)?pLocker->BaseEntity():NULL );
		return true;
	}

	bool Unlock( CEntity *pUnlocker )
	{
		if( IsLocked() )
		{
			if( (CEntity *)(m_hLockedBy) != pUnlocker )
			{
				// Refuse! Only the locker may unlock.
				return false;
			}
		}

		m_hLockedBy.ptr->Set( NULL );
		return true;
	}


	bool IsLocked( void ) { return ( m_hLockedBy != NULL); }
	
	bool IsExclusive();

	enum
	{
		RALLY_EXCLUSIVE_NOT_EVALUATED = -1,
		RALLY_EXCLUSIVE_NO,
		RALLY_EXCLUSIVE_YES,
	};

public:
	DECLARE_DATAMAP(float, m_flAssaultDelay);
	DECLARE_DATAMAP(COutputEvent, m_OnArrival);
	DECLARE_DATAMAP(string_t, m_RallySequenceName);
	DECLARE_DATAMAP(CFakeHandle, m_hLockedBy);
	DECLARE_DATAMAP(int, m_iPriority);
	DECLARE_DATAMAP(bool, m_bForceCrouch);
	DECLARE_DATAMAP(bool, m_bIsUrgent);
	DECLARE_DATAMAP(string_t, m_AssaultPointName);
	DECLARE_DATAMAP(short, m_sExclusivity);


};

//=============================================================================
//=============================================================================
class CE_CAssaultPoint : public CEntity 
{
	CE_DECLARE_CLASS( CE_CAssaultPoint, CEntity );

public:
	DECLARE_DATAMAP(float, m_flTimeLastUsed);
	DECLARE_DATAMAP(string_t, m_NextAssaultPointName);
	DECLARE_DATAMAP(string_t, m_AssaultHintGroup);
	DECLARE_DATAMAP(COutputEvent, m_OnAssaultClear);
	DECLARE_DATAMAP(COutputEvent, m_OnArrival);
	DECLARE_DATAMAP(bool, m_bNeverTimeout);
	DECLARE_DATAMAP(float, m_flAssaultTimeout);
	DECLARE_DATAMAP(bool, m_bInputForcedClear);
	DECLARE_DATAMAP(bool, m_bClearOnContact);
	DECLARE_DATAMAP(float, m_flAssaultPointTolerance);
	DECLARE_DATAMAP(int, m_iStrictness);
	DECLARE_DATAMAP(bool, m_bForceCrouch);
	DECLARE_DATAMAP(bool, m_bIsUrgent);
	DECLARE_DATAMAP(bool, m_bAllowDiversion);
	DECLARE_DATAMAP(float, m_flAllowDiversionRadius);


};



//=============================================================================
//=============================================================================
class CAI_AssaultBehavior : public CAI_SimpleBehavior
{
	DECLARE_CLASS( CAI_AssaultBehavior, CAI_SimpleBehavior );

public:
	CAI_AssaultBehavior();
	
	virtual const char *GetName() {	return "Assault"; }

	virtual void OnRestore();

	bool CanRunAScriptedNPCInteraction( bool bForced );

	virtual bool 	CanSelectSchedule();
	virtual void	BeginScheduleSelection();
	virtual void	EndScheduleSelection();
	
	bool HasHitRallyPoint() { return m_bHitRallyPoint; }
	bool HasHitAssaultPoint() { return m_bHitAssaultPoint; }

	void ClearAssaultPoint( void );
	void OnHitAssaultPoint( void );
	bool PollAssaultCue( void );
	void ReceiveAssaultCue( AssaultCue_t cue );
	bool HasAssaultCue( void ) { return m_AssaultCue != CUE_NO_ASSAULT; }
	bool AssaultHasBegun();

	CE_CAssaultPoint *FindAssaultPoint( string_t iszAssaultPointName );
	void SetAssaultPoint( CE_CAssaultPoint *pAssaultPoint );

	void GatherConditions( void );
	void StartTask( const Task_t *pTask );
	void RunTask( const Task_t *pTask );
	void BuildScheduleTestBits();
	int TranslateSchedule( int scheduleType );
	void OnStartSchedule( int scheduleType );
	void ClearSchedule( const char *szReason );

	void InitializeBehavior();
	void SetParameters( string_t rallypointname, AssaultCue_t assaultcue, int rallySelectMethod );
	void SetParameters( CBaseEntity *pRallyEnt, AssaultCue_t assaultcue );

	bool IsAllowedToDivert( void );
	bool IsValidShootPosition( const Vector &vLocation, CAI_Node *pNode, CBaseEntity const *pHint );
	float GetMaxTacticalLateralMovement( void );

	void UpdateOnRemove();

	bool OnStrictAssault( void	);
	bool UpdateForceCrouch( void );
	bool IsForcingCrouch( void );
	bool IsUrgent( void );

	CE_CRallyPoint *FindBestRallyPointInRadius( const Vector &vecCenter, float flRadius );;

	void Disable( void ) { m_AssaultCue = CUE_NO_ASSAULT; m_bHitRallyPoint = false; m_bHitAssaultPoint = false; }

	enum
	{
		SCHED_MOVE_TO_RALLY_POINT = BaseClass::NEXT_SCHEDULE,		// Try to get out of the player's way
		SCHED_ASSAULT_FAILED_TO_MOVE,
		SCHED_FAIL_MOVE_TO_RALLY_POINT,
		SCHED_MOVE_TO_ASSAULT_POINT,
		SCHED_AT_ASSAULT_POINT,
		SCHED_HOLD_RALLY_POINT,
		SCHED_HOLD_ASSAULT_POINT,
		SCHED_WAIT_AND_CLEAR,
		SCHED_ASSAULT_MOVE_AWAY,
		SCHED_CLEAR_ASSAULT_POINT,
		NEXT_SCHEDULE,

		TASK_GET_PATH_TO_RALLY_POINT = BaseClass::NEXT_TASK,
		TASK_FACE_RALLY_POINT,
		TASK_GET_PATH_TO_ASSAULT_POINT,
		TASK_FACE_ASSAULT_POINT,
		TASK_HIT_ASSAULT_POINT,
		TASK_HIT_RALLY_POINT,
		TASK_AWAIT_CUE,
		TASK_AWAIT_ASSAULT_TIMEOUT,
		TASK_ANNOUNCE_CLEAR,
		TASK_WAIT_ASSAULT_DELAY,
		TASK_ASSAULT_MOVE_AWAY_PATH,
		TASK_ASSAULT_DEFER_SCHEDULE_SELECTION,
		NEXT_TASK,

/*
		COND_PUT_CONDITIONS_HERE = BaseClass::NEXT_CONDITION,
		NEXT_CONDITION,
*/
	};

	DEFINE_CUSTOM_SCHEDULE_PROVIDER;

public:
	CEFakeHandle<CE_CAssaultPoint> m_hAssaultPoint;
	CEFakeHandle<CE_CRallyPoint> m_hRallyPoint;

public:
	void			UnlockRallyPoint( void );

private:
	void			OnScheduleChange();
	virtual int		SelectSchedule();

	AssaultCue_t	m_AssaultCue;			// the cue we're waiting for to begin the assault
	AssaultCue_t	m_ReceivedAssaultCue;	// the last assault cue we received from someone/thing external.

	bool			m_bHitRallyPoint;
	bool			m_bHitAssaultPoint;

	// Diversion
	bool			m_bDiverting;
	float			m_flLastSawAnEnemyAt;

	float			m_flTimeDeferScheduleSelection;

	string_t		m_AssaultPointName;

	//---------------------------------
	
	DECLARE_DATADESC();
};

#endif // AI_BEHAVIOR_ASSAULT_H
