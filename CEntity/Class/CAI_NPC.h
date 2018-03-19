
#ifndef _INCLUDE_CAI_NPC_H_
#define _INCLUDE_CAI_NPC_H_

#include "CEntity.h"
#include "CCombatCharacter.h"
#include "CAI_Memory.h"
#include "CAI_Hint.h"
#include "CEAI_ScriptedSequence.h"
#include "CAI_Navigator.h"
#include "CAI_senses.h"


#include "ai_npcstate.h"
#include "ai_activity.h"
#include "ai_default.h"
#include "ai_task.h"
#include "ai_condition.h"
#include "ai_namespaces.h"
#include "CAI_schedule.h"
#include "npcevent.h"
#include "ai_movetypes.h"
#include "CAI_Moveshoot.h"
#include "CAI_motor.h"
#include "ai_activity.h"
#include "CAI_moveprobe.h"
#include "CAI_Squad.h"
#include "CAI_Hint.h"
#include "ai_squadslot.h"
#include "activitylist.h"
#include "eventlist.h"
#include "CAI_speech.h"
#include "CAI_Interactions.h"


#define PLAYER_SQUADNAME "player_squad"

class CAI_BehaviorBase;
class CEAI_ScriptedSequence;
class CAI_TacticalServices;
class CPropDoor;
class CE_AI_Hint;
class CE_AI_GoalEntity;
class CAI_ShotRegulator;

//=============================================================================
//
// Constants & enumerations
//
//=============================================================================
#define TURRET_CLOSE_RANGE	200
#define TURRET_MEDIUM_RANGE 500

#define COMMAND_GOAL_TOLERANCE	48	// 48 inches.
#define TIME_CARE_ABOUT_DAMAGE	3.0

#define ITEM_PICKUP_TOLERANCE	48.0f

// Max's of the box used to search for a weapon to pick up. 45x45x~8 ft.
#define WEAPON_SEARCH_DELTA	Vector( 540, 540, 100 )

enum Interruptability_t
{
	GENERAL_INTERRUPTABILITY,
	DAMAGEORDEATH_INTERRUPTABILITY,
	DEATH_INTERRUPTABILITY
};

//-------------------------------------
// Memory
//-------------------------------------

#define MEMORY_CLEAR					0
#define bits_MEMORY_PROVOKED			( 1 << 0 )// right now only used for houndeyes.
#define bits_MEMORY_INCOVER				( 1 << 1 )// npc knows it is in a covered position.
#define bits_MEMORY_SUSPICIOUS			( 1 << 2 )// Ally is suspicious of the player, and will move to provoked more easily
#define	bits_MEMORY_TASK_EXPENSIVE		( 1 << 3 )// NPC has completed a task which is considered costly, so don't do another task this frame
//#define	bits_MEMORY_				( 1 << 4 )
#define bits_MEMORY_PATH_FAILED			( 1 << 5 )// Failed to find a path
#define bits_MEMORY_FLINCHED			( 1 << 6 )// Has already flinched
//#define bits_MEMORY_ 					( 1 << 7 )
#define bits_MEMORY_TOURGUIDE			( 1 << 8 )// I have been acting as a tourguide.
//#define bits_MEMORY_					( 1 << 9 )// 
#define bits_MEMORY_LOCKED_HINT			( 1 << 10 )// 
//#define bits_MEMORY_					( 1 << 12 )

#define bits_MEMORY_TURNING				( 1 << 13 )// Turning, don't interrupt me.
#define bits_MEMORY_TURNHACK			( 1 << 14 )

#define bits_MEMORY_HAD_ENEMY			( 1 << 15 )// Had an enemy
#define bits_MEMORY_HAD_PLAYER			( 1 << 16 )// Had player
#define bits_MEMORY_HAD_LOS				( 1 << 17 )// Had LOS to enemy

#define bits_MEMORY_MOVED_FROM_SPAWN	( 1 << 18 )// Has moved since spawning.

#define bits_MEMORY_CUSTOM4				( 1 << 28 )	// NPC-specific memory
#define bits_MEMORY_CUSTOM3				( 1 << 29 )	// NPC-specific memory
#define bits_MEMORY_CUSTOM2				( 1 << 30 )	// NPC-specific memory
#define bits_MEMORY_CUSTOM1				( 1 << 31 )	// NPC-specific memory

//-------------------------------------
// Spawn flags
//-------------------------------------
#define SF_NPC_WAIT_TILL_SEEN			( 1 << 0  )	// spawnflag that makes npcs wait until player can see them before attacking.
#define SF_NPC_GAG						( 1 << 1  )	// no idle noises from this npc
#define SF_NPC_FALL_TO_GROUND			( 1 << 2  )	// used my NPC_Maker
#define SF_NPC_DROP_HEALTHKIT			( 1 << 3  )	// Drop a healthkit upon death
#define SF_NPC_START_EFFICIENT			( 1 << 4  ) // Set into efficiency mode from spawn
//										( 1 << 5  )
//										( 1 << 6  )
#define SF_NPC_WAIT_FOR_SCRIPT			( 1 << 7  )	// spawnflag that makes npcs wait to check for attacking until the script is done or they've been attacked
#define SF_NPC_LONG_RANGE				( 1 << 8  )	// makes npcs look far and relaxes weapon range limit 
#define SF_NPC_FADE_CORPSE				( 1 << 9  )	// Fade out corpse after death
#define SF_NPC_ALWAYSTHINK				( 1 << 10 )	// Simulate even when player isn't in PVS.
#define SF_NPC_TEMPLATE					( 1 << 11 )	// This NPC will be used as a template by an npc_maker -- do not spawn.
#define SF_NPC_ALTCOLLISION				( 1 << 12 )
#define SF_NPC_NO_WEAPON_DROP			( 1 << 13 )	// This NPC will not actually drop a weapon that can be picked up
#define SF_NPC_NO_PLAYER_PUSHAWAY		( 1 << 14 )	
//										( 1 << 15 )	
// !! Flags above ( 1 << 15 )	 are reserved for NPC sub-classes

//-------------------------------------
//
// Return codes from CanPlaySequence.
//
//-------------------------------------

enum CanPlaySequence_t
{
	CANNOT_PLAY = 0,		// Can't play for any number of reasons.
	CAN_PLAY_NOW,			// Can play the script immediately.
	CAN_PLAY_ENQUEUED,		// Can play the script after I finish playing my current script.
};

//-------------------------------------
// Weapon holstering
//-------------------------------------
enum DesiredWeaponState_t
{
	DESIREDWEAPONSTATE_IGNORE = 0,
	DESIREDWEAPONSTATE_HOLSTERED,
	DESIREDWEAPONSTATE_HOLSTERED_DESTROYED, // Put the weapon away, then destroy it.
	DESIREDWEAPONSTATE_UNHOLSTERED,
	DESIREDWEAPONSTATE_CHANGING,
	DESIREDWEAPONSTATE_CHANGING_DESTROY,	// Destroy the weapon when this change is complete.
};

//-------------------------------------
//
// Efficiency modes
//
//-------------------------------------

enum AI_Efficiency_t
{
	// Run at full tilt
	AIE_NORMAL,

	// Run decision process less often
	AIE_EFFICIENT,

	// Run decision process even less often, ignore other NPCs
	AIE_VERY_EFFICIENT,

	// Run decision process even less often, ignore other NPCs
	AIE_SUPER_EFFICIENT,

	// Don't run at all
	AIE_DORMANT,
};

enum AI_MoveEfficiency_t
{
	AIME_NORMAL,
	AIME_EFFICIENT,
};

//-------------------------------------
//
// Sleep state
//
//-------------------------------------

enum AI_SleepState_t
{
	AISS_AWAKE,
	AISS_WAITING_FOR_THREAT,
	AISS_WAITING_FOR_PVS,
	AISS_WAITING_FOR_INPUT,
	AISS_AUTO_PVS,
	AISS_AUTO_PVS_AFTER_PVS, // Same as AUTO_PVS, except doesn't activate until/unless the NPC is IN the player's PVS. 
};

#define AI_SLEEP_FLAGS_NONE					0x00000000
#define AI_SLEEP_FLAG_AUTO_PVS				0x00000001
#define AI_SLEEP_FLAG_AUTO_PVS_AFTER_PVS	0x00000002


//-------------------------------------
//
// Debug bits
//
//-------------------------------------

enum DebugBaseNPCBits_e
{
	bits_debugDisableAI = 0x00000001,		// disable AI
	bits_debugStepAI	= 0x00000002,		// step AI

};

//-------------------------------------
//
// Base Sentence index for behaviors
//
//-------------------------------------
enum SentenceIndex_t
{
	SENTENCE_BASE_BEHAVIOR_INDEX = 1000,
};

#ifdef AI_MONITOR_FOR_OSCILLATION
struct AIScheduleChoice_t 
{
	float			m_flTimeSelected;
	CAI_Schedule	*m_pScheduleSelected;
};
#endif//AI_MONITOR_FOR_OSCILLATION

#define MARK_TASK_EXPENSIVE()	\
	if ( GetOuter() ) \
	{ \
		GetOuter()->Remember( bits_MEMORY_TASK_EXPENSIVE ); \
	}

//=============================================================================
//
// Types used by CAI_BaseNPC
//
//=============================================================================

struct AIScheduleState_t
{
	int					 iCurTask;
	TaskStatus_e		 fTaskStatus;
	float				 timeStarted;
	float				 timeCurTaskStarted;
	AI_TaskFailureCode_t taskFailureCode;
	int					 iTaskInterrupt;
	bool 				 bTaskRanAutomovement;
	bool 				 bTaskUpdatedYaw;
	bool				 bScheduleWasInterrupted;

	DECLARE_SIMPLE_DATADESC();
};

// -----------------------------------------
//	An entity that this NPC can't reach
// -----------------------------------------

struct UnreachableEnt_t
{
	EHANDLE	hUnreachableEnt;	// Entity that's unreachable
	float	fExpireTime;		// Time to forget this information
	Vector	vLocationWhenUnreachable;
	
	DECLARE_SIMPLE_DATADESC();
};

//=============================================================================
// SCRIPTED NPC INTERACTIONS
//=============================================================================
// -----------------------------------------
//	Scripted NPC interaction flags
// -----------------------------------------
#define SCNPC_FLAG_TEST_OTHER_ANGLES			( 1 << 1 )
#define SCNPC_FLAG_TEST_OTHER_VELOCITY			( 1 << 2 )
#define SCNPC_FLAG_LOOP_IN_ACTION				( 1 << 3 )
#define SCNPC_FLAG_NEEDS_WEAPON_ME				( 1 << 4 )
#define SCNPC_FLAG_NEEDS_WEAPON_THEM			( 1 << 5 )
#define SCNPC_FLAG_DONT_TELEPORT_AT_END_ME		( 1 << 6 )
#define SCNPC_FLAG_DONT_TELEPORT_AT_END_THEM	( 1 << 7 )

// -----------------------------------------
//	Scripted NPC interaction trigger methods
// -----------------------------------------
enum
{
	SNPCINT_CODE = 0,
	SNPCINT_AUTOMATIC_IN_COMBAT = 1,
};

// -----------------------------------------
//	Scripted NPC interaction loop breaking trigger methods
// -----------------------------------------
#define SNPCINT_LOOPBREAK_ON_DAMAGE				( 1 << 1 )
#define SNPCINT_LOOPBREAK_ON_FLASHLIGHT_ILLUM	( 1 << 2 )

// -----------------------------------------
//	Scripted NPC interaction anim phases
// -----------------------------------------
enum
{
	SNPCINT_ENTRY = 0,
	SNPCINT_SEQUENCE,
	SNPCINT_EXIT,

	SNPCINT_NUM_PHASES
};

struct ScriptedNPCInteraction_Phases_t
{
	string_t	iszSequence;
	int			iActivity;

	DECLARE_SIMPLE_DATADESC();
};

// Allowable delta from the desired dynamic scripted sequence point
#define DSS_MAX_DIST			6
#define DSS_MAX_ANGLE_DIFF		4

// Interaction Logic States
enum
{
	NPCINT_NOT_RUNNING = 0,
	NPCINT_RUNNING_ACTIVE,		// I'm in an interaction that I initiated
	NPCINT_RUNNING_PARTNER,		// I'm in an interaction that was initiated by the other NPC
	NPCINT_MOVING_TO_MARK,		// I'm moving to a position to do an interaction
};

#define NPCINT_NONE				-1

#define MAXTACLAT_IGNORE		-1

// -----------------------------------------
//	A scripted interaction between NPCs
// -----------------------------------------
struct ScriptedNPCInteraction_t
{
	ScriptedNPCInteraction_t()
	{
		iszInteractionName = NULL_STRING;
		iFlags = 0;
		iTriggerMethod = SNPCINT_CODE;
		iLoopBreakTriggerMethod = 0;
		vecRelativeOrigin = vec3_origin;
		bValidOnCurrentEnemy = false;
		flDelay = 5.0;
		flDistSqr = (DSS_MAX_DIST * DSS_MAX_DIST);
		flNextAttemptTime = 0;
		iszMyWeapon = NULL_STRING;
		iszTheirWeapon = NULL_STRING;

		for ( int i = 0; i < SNPCINT_NUM_PHASES; i++)
		{
			sPhases[i].iszSequence = NULL_STRING;
			sPhases[i].iActivity = ACT_INVALID;
		}
	}

	// Fill out these when passing to AddScriptedNPCInteraction
	string_t	iszInteractionName;
	int			iFlags;
	int			iTriggerMethod;
	int			iLoopBreakTriggerMethod;
	Vector		vecRelativeOrigin;			// (forward, right, up)
	QAngle		angRelativeAngles;				
	Vector		vecRelativeVelocity;		// Desired relative velocity of the other NPC
	float		flDelay;					// Delay before interaction can be used again
	float		flDistSqr;					// Max distance sqr from the relative origin the NPC is allowed to be to trigger
	string_t	iszMyWeapon;				// Classname of the weapon I'm holding, if any
	string_t	iszTheirWeapon;				// Classname of the weapon my interaction partner is holding, if any
	ScriptedNPCInteraction_Phases_t sPhases[SNPCINT_NUM_PHASES];

	// These will be filled out for you in AddScriptedNPCInteraction
	VMatrix		matDesiredLocalToWorld;		// Desired relative position / angles of the other NPC
	bool		bValidOnCurrentEnemy;

	float		flNextAttemptTime;

	DECLARE_SIMPLE_DATADESC();
};

float DeltaV( float v0, float v1, float d );
Vector VecCheckToss ( CEntity *pEdict, Vector vecSpot1, Vector vecSpot2, float flHeightMaxRatio, float flGravityAdj, bool bRandomize, Vector *vecMins = NULL, Vector *vecMaxs = NULL );
Vector VecCheckToss ( CEntity *pEntity, ITraceFilter *pFilter, Vector vecSpot1, Vector vecSpot2, float flHeightMaxRatio, float flGravityAdj, bool bRandomize, Vector *vecMins = NULL, Vector *vecMaxs = NULL );
Vector VecCheckThrow( CEntity *pEdict, const Vector &vecSpot1, Vector vecSpot2, float flSpeed, float flGravityAdj = 1.0f, Vector *vecMins = NULL, Vector *vecMaxs = NULL );

extern bool AIStrongOpt( void );


bool FBoxVisible ( CEntity *pLooker, CEntity *pTarget );
bool FBoxVisible ( CEntity *pLooker, CEntity *pTarget, Vector &vecTargetOrigin, float flSize = 0.0 );


/*
extern CAI_SchedulesManager *my_g_AI_SchedulesManager;
extern CAI_GlobalScheduleNamespace *my_gm_SchedulingSymbols;
extern CAI_ClassScheduleIdSpace *my_gm_ClassScheduleIdSpace;
extern CAI_LocalIdSpace    *my_gm_SquadSlotIdSpace;
extern CAI_GlobalNamespace *my_gm_SquadSlotNamespace;*/

class CBaseCombatCharacter;


class CAI_NPC : public CCombatCharacter
{
public:
	CE_DECLARE_CLASS(CAI_NPC, CCombatCharacter);
	
	CAI_NPC();
	virtual ~CAI_NPC();
	virtual void PostConstructor();

	void		SetHullSizeNormal(bool force=false);

	void		CapabilitiesClear();
	int			CapabilitiesAdd(int capability);
	int			CapabilitiesRemove(int capability);

	void		SetCondition(int iCondition);
	bool		HasCondition( int iCondition );
	bool		HasCondition( int iCondition, bool bUseIgnoreConditions );
	void		ClearCondition( int iCondition );
	void		ClearConditions( int *pConditions, int nConditions );
	bool		HasInterruptCondition( int iCondition );

	const Vector &		GetEnemyLKP() const;
	float				GetEnemyLastTimeSeen() const;

	CEntity		*GetEnemy()									{ return m_hEnemy; }
	CEntity		*GetEnemy()	const							{ return m_hEnemy; }
	CBaseEntity	*GetEnemy_CBase()							{ return (m_hEnemy != NULL) ? m_hEnemy->BaseEntity() : NULL; }


	void		SetEnemyOccluder(CEntity *pBlocker);
	CEntity		*GetEnemyOccluder()							{ return m_hEnemyOccluder; }
	CEntity		*GetEnemyOccluder() const					{ return m_hEnemyOccluder; }
	CBaseEntity	*GetEnemyOccluder_CBase()					{ return (m_hEnemyOccluder != NULL) ? m_hEnemyOccluder->BaseEntity() : NULL; }


	bool		AutoMovement(CEntity *pTarget = NULL, AIMoveTrace_t *pTraceResult = NULL);
	bool		AutoMovement( float flInterval, CEntity *pTarget = NULL, AIMoveTrace_t *pTraceResult = NULL );
	
	void		TaskComplete(  bool fIgnoreSetFailedCondition = false);
	void 		SetTaskStatus( TaskStatus_e status )	{ m_ScheduleState->fTaskStatus = status; 	}

	CAI_MoveProbe *		GetMoveProbe() 				{ return m_pMoveProbe; }
	const CAI_MoveProbe *GetMoveProbe() const		{ return m_pMoveProbe; }

	CAI_Motor *			GetMotor() 					{ return m_pMotor; }
	const CAI_Motor *	GetMotor() const			{ return m_pMotor; }

	CAI_Pathfinder *	GetPathfinder() 			{ return m_pPathfinder; }
	const CAI_Pathfinder *GetPathfinder() const 	{ return m_pPathfinder; }

	CE_AI_Hint				*GetHintNode()				{ return (CE_AI_Hint *)(m_pHintNode.ptr->Get()); }
	const CE_AI_Hint		*GetHintNode() const		{ return (CE_AI_Hint *)(m_pHintNode.ptr->Get()); }

	const Vector &		GetHullMins() const		{ return NAI_Hull::Mins(GetHullType()); }
	const Vector &		GetHullMaxs() const		{ return NAI_Hull::Maxs(GetHullType()); }
	float				GetHullWidth()	const	{ return NAI_Hull::Width(GetHullType()); }
	float				GetHullHeight() const	{ return NAI_Hull::Height(GetHullType()); }

	Activity			GetActivity( void ) { return m_Activity; }

	string_t			GetHintGroup( void )			{ return m_strHintGroup; }
	void				ClearHintGroup( void )			{ SetHintGroup( NULL_STRING );	}
	void				SetHintGroup( string_t name, bool bHintGroupNavLimiting = false );

	NPC_STATE			GetState( void )				{ return m_NPCState; }

	AI_SleepState_t		GetSleepState() const			{ return m_SleepState; }

	void SetIdealActivity( Activity NewActivity );
	bool IsCurSchedule( int schedId, bool fIdeal = true );

	inline bool SetSchedule(int localScheduleID) { return SetSchedule_Int(localScheduleID); }
	
	bool HaveSequenceForActivity( Activity activity );

	Activity GetIdealActivity() { return m_IdealActivity; }
	
	void ResetActivity(void) { m_Activity = ACT_RESET; }

	void SetHintNode( CBaseEntity *pHintNode );
	void ClearHintNode( float reuseDelay = 0.0 );

	CAI_Navigator *		GetNavigator() 				{ return m_pNavigator; }
	const CAI_Navigator *GetNavigator() const 		{ return m_pNavigator; }
	
	CAI_LocalNavigator *GetLocalNavigator()			{ return m_pLocalNavigator; }
	const CAI_LocalNavigator *GetLocalNavigator() const { return m_pLocalNavigator; }

	CAI_Squad *			GetSquad() 				{ return m_pSquad; }
	const CAI_Squad		*GetSquad() const 		{ return m_pSquad; }

	bool				IsInSquad() const				{ return GetSquad() != NULL; }

	CAI_Senses *		GetSenses()				{ return m_pSenses; }
	const CAI_Senses *	GetSenses() const		{ return m_pSenses; }

	bool	FacingIdeal();

	int 				GetScheduleCurTaskIndex() const			{ return m_ScheduleState.ptr->iCurTask;	}
	CAI_Schedule *		GetCurSchedule()						{ return *(m_pSchedule.ptr); }

	const Task_t*		GetTask( void );


	static CAI_GlobalScheduleNamespace *GetSchedulingSymbols()		{ return &gm_SchedulingSymbols; }
	static CAI_ClassScheduleIdSpace &AccessClassScheduleIdSpaceDirect() { return gm_ClassScheduleIdSpace; }

	
	bool OccupyStrategySlotRange( int slotIDStart, int slotIDEnd );
	
	Navigation_t		GetNavType() const;
	void				SetNavType( Navigation_t navType );


	inline void			Remember( int iMemory ) 		{ m_afMemory |= iMemory; }
	inline void			Forget( int iMemory ) 			{ m_afMemory &= ~iMemory; }
	inline bool			HasMemory( int iMemory ) 		{ if ( m_afMemory & iMemory ) return TRUE; return FALSE; }
	inline bool			HasAllMemories( int iMemory ) 	{ if ( (m_afMemory & iMemory) == iMemory ) return TRUE; return FALSE; }

	inline CEAI_ScriptedSequence *Get_m_hCine()			{ return (CEAI_ScriptedSequence *)(m_hCine.ptr->Get()); }

	CEntity*		GetGoalEnt()							{ return m_hGoalEnt.ptr->Get();	}
	void			SetGoalEnt( CBaseEntity *pGoalEnt )		{ m_hGoalEnt.ptr->Set( pGoalEnt ); }


	void				SetCustomInterruptCondition( int nCondition );
	bool				IsCustomInterruptConditionSet( int nCondition );
	void				ClearCustomInterruptCondition( int nCondition );
	void				ClearCustomInterruptConditions( void );

	inline void			SetIdealState( NPC_STATE eIdealState );	
	inline NPC_STATE	GetIdealState();

	bool				IsMoving( void );

	void				ClearSchedule( const char *szReason );

	void				ResetScheduleCurTaskIndex();

	bool				IsWaitFinished();
	float				SetWait( float minWait, float maxWait = 0.0 );

	bool				DispatchInteraction( int interactionType, void *data, CBaseEntity* sourceEnt )	{ return ( interactionType > 0 ) ? HandleInteraction( interactionType, data, sourceEnt ) : false; }
	
	CAI_NPC *			GetInteractionPartner( void );

	string_t			GetSquadName()	{ return m_SquadName; }
	void				SetSquadName( string_t name )	{ m_SquadName = name; 	}

	void				SetDefaultEyeOffset ( void );
	const Vector &		GetDefaultEyeOffset( void )			{ return m_vDefaultEyeOffset;	}

	bool				ConditionInterruptsCurSchedule( int iCondition );

	void				ResetIdealActivity( Activity newIdealActivity );

	inline int			LookupPoseMoveYaw()		{ return m_poseMove_Yaw; }

	void				SetUpdatedYaw()	{ m_ScheduleState->bTaskUpdatedYaw = true; }

	bool				IsFlaggedEfficient() const					{ return HasSpawnFlags( SF_NPC_START_EFFICIENT ); }

	float				GetStepDownMultiplier() const;

	bool				UpdateTurnGesture( void );

	Activity			GetStoppedActivity( void );
	int					GetScriptCustomMoveSequence( void );

	CEntity *			GetNavTargetEntity(void);

	void				SetTarget( CBaseEntity *pTarget );
	CEntity*			GetTarget()								{ return m_hTargetEnt; }

	CAI_TacticalServices *GetTacticalServices()			{ return m_pTacticalServices; }
	const CAI_TacticalServices *GetTacticalServices() const { return m_pTacticalServices; }

	bool				IsJumpLegal( const Vector &startPos, const Vector &apex, const Vector &endPos, float maxUp, float maxDown, float maxDist ) const;

	void	OpenPropDoorBegin( CPropDoor *pDoor );
	void	OpenPropDoorNow( CPropDoor *pDoor );

	float	VecToYaw( const Vector &vecDir );

	int		WalkMove( const Vector& vecPosition, unsigned int mask );
	int		FlyMove( const Vector& pfPosition, unsigned int mask );

	void	RememberUnreachable( CEntity* pEntity, float duration = -1 );

	void	ForceChooseNewEnemy()	{ m_EnemiesSerialNumber = -1; }
	bool	HasStrategySlotRange( int slotIDStart, int slotIDEnd );

	void	TaskMovementComplete( void );
	TaskStatus_e 		GetTaskStatus() const					{ return m_ScheduleState.ptr->fTaskStatus; 	}

	bool				OccupyStrategySlot( int squadSlotID );
	bool				IsStrategySlotRangeOccupied( int slotIDStart, int slotIDEnd );	// Returns true if all in the range are occupied

	inline bool			IsInAScript( void ) { return m_bInAScript; }
	inline void			SetInAScript( bool bScript ) { m_bInAScript = bScript; }

	void				VacateStrategySlot( void );

	CCombatCharacter	*GetEnemyCombatCharacterPointer();
	int					TaskIsRunning( void );

	void				ClearEnemyMemory();

	void				TestPlayerPushing( CEntity *pPlayer );
	void				CascadePlayerPush( const Vector &push, const Vector &pushOrigin );

	CSound *			GetLoudestSoundOfType( int iType );
	
	inline CAI_ShotRegulator* GetShotRegulator()		{ return m_ShotRegulator; }

	void				TaskInterrupt()					{ m_ScheduleState.ptr->iTaskInterrupt++; }
	void				ClearTaskInterrupt()			{ m_ScheduleState.ptr->iTaskInterrupt = 0; }
	int					GetTaskInterrupt() const		{ return m_ScheduleState.ptr->iTaskInterrupt; }

	bool IsActiveDynamicInteraction( void ) { return (m_iInteractionState == NPCINT_RUNNING_ACTIVE && (m_hCine != NULL)); }

	AI_Efficiency_t		GetEfficiency() const						{ return m_Efficiency; }
	void				SetEfficiency( AI_Efficiency_t efficiency )	{ m_Efficiency = efficiency; }

	AI_MoveEfficiency_t GetMoveEfficiency() const					{ return m_MoveEfficiency; }
	void				SetMoveEfficiency( AI_MoveEfficiency_t efficiency )	{ m_MoveEfficiency = efficiency; }

	bool				IsWeaponStateChanging( void );

	void				SetIgnoreConditions( int *pConditions, int nConditions );
	void				ClearIgnoreConditions( int *pConditions, int nConditions );

	void				SetDistLook( float flDistLook );

	bool				ConditionsGathered() const		{ return m_bConditionsGathered; }

	float				EnemyDistance( CEntity *pEnemy );

	ScriptedNPCInteraction_t *GetRunningDynamicInteraction( void ) { return &(m_ScriptedInteractions->Element(m_iInteractionPlaying)); }
	
	bool				HasInteractionCantDie( void );
	
	bool				IsInPlayerSquad() const;

	string_t			GetPlayerSquadName() const	{ Assert( gm_iszPlayerSquad != NULL_STRING ); return gm_iszPlayerSquad; }

	void				ForceGatherConditions()	{ m_bForceConditionsGather = true; SetEfficiency( AIE_NORMAL ); }	// Force an NPC out of PVS to call GatherConditions on next think

	bool				IsSquadmateInSpread( const Vector &sourcePos, const Vector &targetPos, float flSpread, float maxDistOffCenter );
	bool				PointInSpread( CCombatCharacter *pCheckEntity, const Vector &sourcePos, const Vector &targetPos, const Vector &testPoint, float flSpread, float maxDistOffCenter );

	bool				CheckPVSCondition();

	void				AddSceneLock( float flDuration = 0.2f ) { m_flSceneTime = MAX( gpGlobals->curtime + flDuration, m_flSceneTime ); };
	void				ClearSceneLock( float flDuration = 0.2f ) { m_flSceneTime = gpGlobals->curtime + flDuration; };
	bool				IsInLockedScene( void ) { return m_flSceneTime > gpGlobals->curtime; };

	bool				ExitScriptedSequence();

	bool				TaskRanAutomovement( void ) { return m_ScheduleState->bTaskRanAutomovement; }

	inline void			DesireStand() {	m_bCrouchDesired = false; }

	int					SelectFlinchSchedule( void );
	
	bool				IsLimitingHintGroups( void )	{ return m_bHintGroupNavLimiting; }

	bool				CrouchIsDesired( void ) const;

	float				GetTimeScheduleStarted() const				{ return m_ScheduleState.ptr->timeStarted; }

	void				SetLastAttackTime( float time)	{ m_flLastAttackTime = time; }

	float				GetLastAttackTime() const { return m_flLastAttackTime; }
	float				GetLastDamageTime() const { return m_flLastDamageTime; }
	float				GetLastPlayerDamageTime() const { return m_flLastPlayerDamageTime; }
	float				GetLastEnemyTime() const { return m_flLastEnemyTime; }
	
	void				ForceDecisionThink()  { m_flNextDecisionTime = 0; SetEfficiency( AIE_NORMAL ); }
	bool				DidChooseEnemy() const	{ return !m_bSkippedChooseEnemy; }
	float				GetTimeEnemyAcquired()	{ return m_flTimeEnemyAcquired; }
	
	inline void	ForceCrouch( void );
	inline void	ClearForceCrouch( void );
	
	bool				HasConditionsToInterruptSchedule( int nLocalScheduleID );

	Activity TranslateActivity( Activity idealActivity, Activity *pIdealWeaponActivity = NULL );

protected:
	void				ChainStartTask( int task, float taskData = 0 )	{ Task_t tempTask = { task, taskData }; StartTask( (const Task_t *)&tempTask ); }
	void				ChainRunTask( int task, float taskData = 0 )	{ Task_t tempTask = { task, taskData }; RunTask( (const Task_t *)	&tempTask );	}
	
	bool				CouldShootIfCrouching( CEntity *pTarget );
		
public:
	template <class BEHAVIOR_TYPE>
	bool GetBehavior( BEHAVIOR_TYPE **ppBehavior )
	{
		CAI_BehaviorBase **ppBehaviors = AccessBehaviors();
		
		*ppBehavior = NULL;
		for ( int i = 0; i < NumBehaviors(); i++ )
		{
			*ppBehavior = dynamic_cast<BEHAVIOR_TYPE *>(ppBehaviors[i]);
			if ( *ppBehavior )
				return true;
		}
		return false;
	}

	enum
	{
		NEXT_SCHEDULE 	= LAST_SHARED_SCHEDULE,
		NEXT_TASK		= LAST_SHARED_TASK,
		NEXT_CONDITION 	= LAST_SHARED_CONDITION,
	};

public:
	void Weapon_SetActivity( Activity newActivity, float duration );

public:
	virtual CEAI_Enemies *GetEnemies();
	virtual void NPCInit();
	virtual void SetState(NPC_STATE State);
	virtual void SetActivity(Activity NewActivity);
	virtual bool SetSchedule_Int(int localScheduleID);
	virtual void RunTask( const Task_t *pTask );
	virtual void OnChangeActivity(Activity eNewActivity);
	virtual float MaxYawSpeed();
	virtual CAI_ClassScheduleIdSpace *GetClassScheduleIdSpace();
	virtual void NPCThink();
	virtual bool IsActivityFinished();
	virtual float CalcIdealYaw( const Vector &vecTarget );
	virtual void GatherConditions();
	virtual void PrescheduleThink();
	virtual void IdleSound();
	virtual void AlertSound();
	virtual void PainSound(const CTakeDamageInfo &info);
	virtual void DeathSound(const CTakeDamageInfo &info);
	virtual bool FInAimCone_Vector(const Vector &vecSpot);
	virtual CBaseEntity *BestEnemy();
	void TaskFail(AI_TaskFailureCode_t code);
	void TaskFail(const char *pszGeneralFailText) { TaskFail( MakeFailCode( pszGeneralFailText ) ); }
	virtual void StartTask( const Task_t *pTask );
	virtual bool Event_Gibbed( const CTakeDamageInfo &info );
	virtual int	TranslateSchedule( int scheduleType );
	virtual int	SelectSchedule();
	virtual int	SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode );
	//virtual Activity Weapon_TranslateActivity( Activity baseAct, bool *pRequired = NULL );
	virtual Activity NPC_TranslateActivity( Activity eNewActivity );
	virtual bool HandleInteraction(int interactionType, void *data, CBaseEntity* sourceEnt);
	virtual CAI_Schedule *GetSchedule(int schedule);
	virtual void PlayerHasIlluminatedNPC(CBaseEntity *pPlayer, float flDot );
	virtual bool QuerySeeEntity( CBaseEntity *pEntity, bool bOnlyHateOrFearIfNPC = false );
	virtual int	GetSoundInterests();
	virtual void BuildScheduleTestBits( void );
	virtual bool UpdateEnemyMemory( CBaseEntity *pEnemy, const Vector &position, CBaseEntity *pInformer = NULL);
	virtual bool OverrideMove( float flInterval );
	virtual	bool OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval );
	virtual float GetHitgroupDamageMultiplier( int iHitGroup, const CTakeDamageInfo &info );
	virtual bool OnBehaviorChangeStatus(  CAI_BehaviorBase *pBehavior, bool fCanFinishSchedule );
	virtual bool IsInterruptable();
	virtual void MakeAIFootstepSound( float volume, float duration = 0.5f );
	virtual void OnScheduleChange( void );
	virtual void TranslateNavGoal( CBaseEntity *pEnemy, Vector &chasePosition);
	virtual int	MeleeAttack1Conditions( float flDot, float flDist );
	virtual bool IsUnreachable( CBaseEntity* pEntity );	
	virtual bool OnObstructingDoor(AILocalMoveGoal_t *pMoveGoal, CBaseEntity *pDoor, float distClear, AIMoveResult_t *pResult);
	virtual bool IsHeavyDamage( const CTakeDamageInfo &info );
	virtual float GetTimeToNavGoal();
	virtual float StepHeight() const;
	virtual float GetJumpGravity() const;
	virtual bool ShouldPlayerAvoid( void );
	virtual bool ShouldProbeCollideAgainstEntity( CBaseEntity *pEntity );
	virtual int	CapabilitiesGet( void ) const;
	virtual bool IsCrouching( void );
	virtual void OnChangeHintGroup( string_t oldGroup, string_t newGroup );
	virtual bool IsJumpLegal( const Vector &startPos, const Vector &apex, const Vector &endPos ) const;
	virtual bool ShouldFailNav( bool bMovementFailed );
	virtual bool IsNavigationUrgent();
	virtual float GetDefaultNavGoalTolerance();
	virtual void OnMovementFailed();
	virtual bool ShouldBruteForceFailedNav();
	virtual bool IsUnusableNode(int iNodeID, CBaseEntity *pHint);
	virtual	bool MovementCost( int moveType, const Vector &vecStart, const Vector &vecEnd, float *pCost );
	virtual Vector GetNodeViewOffset();
	virtual void PostNPCInit();
	virtual float InnateRange1MaxRange( void );
	virtual float InnateRange1MinRange (void );
	virtual int	RangeAttack1Conditions( float flDot, float flDist );
	virtual int	RangeAttack2Conditions( float flDot, float flDist );
	virtual	void OnStateChange( NPC_STATE OldState, NPC_STATE NewState );
	virtual bool ShouldPlayIdleSound( void );
	virtual bool IsLightDamage( const CTakeDamageInfo &info );
	virtual	bool CanBeAnEnemyOf( CBaseEntity *pEnemy );
	virtual	bool AllowedToIgnite( void );
	virtual void GatherEnemyConditions( CBaseEntity *pEnemy );
	virtual bool IsSilentSquadMember() const;
	virtual void OnMovementComplete();
	virtual void AddFacingTarget_E_V_F_F_F( CBaseEntity *pTarget, const Vector &vecPosition, float flImportance, float flDuration, float flRamp = 0.0 );
	virtual bool QueryHearSound( CSound *pSound );
	virtual bool IsPlayerAlly( CBaseEntity *pPlayer = NULL );
	virtual CAI_BehaviorBase *GetRunningBehavior();
	virtual void OnUpdateShotRegulator();
	virtual void CleanupOnDeath( CBaseEntity *pCulprit, bool bFireDeathOutput );
	virtual void OnStartSchedule( int scheduleType );
	virtual void AimGun();
	virtual const char *GetSchedulingErrorName();
	virtual bool IsCurTaskContinuousMove();
	virtual	bool IsValidEnemy( CBaseEntity *pEnemy );
	virtual	bool IsValidCover( const Vector &vLocation, CBaseEntity const *pHint );
	virtual	bool IsValidShootPosition( const Vector &vLocation, CAI_Node *pNode, CBaseEntity const *pHint );
	virtual float GetMaxTacticalLateralMovement();
	virtual bool ShouldIgnoreSound( CSound *pSound );
	virtual void OnSeeEntity( CBaseEntity *pEntity );
	virtual float GetReasonableFacingDist( void );
	virtual bool CanFlinch();
	virtual bool IsCrouchedActivity( Activity activity );
	virtual bool CanRunAScriptedNPCInteraction( bool bForced = false );
	virtual Activity GetFlinchActivity( bool bHeavyDamage, bool bGesture );
	virtual bool OnCalcBaseMove( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult );
	virtual bool ShouldAlwaysThink();
	virtual bool ScheduledMoveToGoalEntity( int scheduleType, CBaseEntity *pGoalEntity, Activity movementActivity );
	virtual bool ScheduledFollowPath( int scheduleType, CBaseEntity *pPathStart, Activity movementActivity );
	virtual const char *TaskName(int taskID);
	virtual CAI_Schedule *GetNewSchedule();
	virtual CAI_Schedule *GetFailSchedule();
	virtual CAI_BehaviorBase **AccessBehaviors();
	virtual int	NumBehaviors();
	virtual CAI_Expresser *GetExpresser();
	virtual bool ValidateNavGoal();
	virtual void NotifyDeadFriend(CBaseEntity *pFriend);
	virtual void SetAim(const Vector &aimDir);
	virtual	void RelaxAim( void );
	virtual Activity GetHintActivity( short sHintType, Activity HintsActivity );
	virtual bool FValidateHintType( CBaseEntity *pHint );
	virtual float GetHintDelay( short sHintType );
	virtual bool InnateWeaponLOSCondition( const Vector &ownerPos, const Vector &targetPos, bool bSetConditions );	
	virtual bool OnObstructionPreSteer( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult );
	virtual float CoverRadius() ;
	virtual void SetTurnActivity();
	virtual bool FCanCheckAttacks();
	virtual int	GetGlobalScheduleId( int localScheduleID );
	virtual void AddFacingTarget_V_F_F_F( const Vector &vecPosition, float flImportance, float flDuration, float flRamp = 0.0 );
	virtual void SetSquad( CAI_Squad *pSquad );
	virtual void ClearCommandGoal();
	virtual bool FindCoverPosInRadius( CBaseEntity *pEntity, const Vector &goalPos, float coverRadius, Vector *pResult );
	virtual void CheckAmmo( void );
	virtual void OnRangeAttack1();
	virtual Vector GetShootEnemyDir( const Vector &shootOrigin, bool bNoisy = true );
	virtual	void SetScriptedScheduleIgnoreConditions( Interruptability_t interrupt );
	virtual void OnStartScene( );
	virtual NPC_STATE SelectIdealState( void );
	virtual void RunAI( void );
	virtual	bool ShouldMoveAndShoot( void );
	virtual bool CanBeUsedAsAFriend( void );
	virtual void OnClearGoal( CAI_BehaviorBase *pBehavior, CE_AI_GoalEntity *pGoal );
	virtual bool ShouldAcceptGoal( CAI_BehaviorBase *pBehavior, CE_AI_GoalEntity *pGoal );
	virtual	void Wake( bool bFireOutput = true );
	virtual float GetReactionDelay( CBaseEntity *pEnemy );
	virtual const char*	SquadSlotName(int slotID);
	virtual	bool PlayerInSpread( const Vector &sourcePos, const Vector &targetPos, float flSpread, float maxDistOffCenter, bool ignoreHatedPlayers = true );
	virtual Vector EyeOffset( Activity nActivity );
	virtual void CollectShotStats( const Vector &vecShootOrigin, const Vector &vecShootDir );
	virtual void OnLooked( int iDistance );
	virtual bool ShouldNotDistanceCull();
	virtual int	HolsterWeapon( void );
	virtual int	UnholsterWeapon( void );
	virtual	CBaseEntity *FindNamedEntity( const char *pszName, IEntityFindFilter *pFilter = NULL );
	virtual	Vector FacingPosition();
	virtual void AddFacingTarget_E_F_F_F( CBaseEntity *pTarget, float flImportance, float flDuration, float flRamp = 0.0 );
	virtual bool ValidEyeTarget(const Vector &lookTargetPos);
	virtual void SetHeadDirection( const Vector &vTargetPos, float flInterval );
	virtual	void MaintainTurnActivity( void );
	virtual void AddLookTarget_E( CBaseEntity *pTarget, float flImportance, float flDuration, float flRamp = 0.0 );
	virtual void AddLookTarget_V( const Vector &vecPosition, float flImportance, float flDuration, float flRamp = 0.0 );
	virtual void MaintainLookTargets( float flInterval );
	virtual bool Stand( void );
	virtual bool Crouch( void );
	virtual CSound *GetBestSound( int validTypes = ALL_SOUNDS );
	virtual bool FOkToMakeSound( int soundPriority = 0 );
	virtual void JustMadeSound( int soundPriority = 0, float flSoundLength = 0.0f );
	virtual void FearSound( void );
	virtual bool IsWaitingToRappel();
	virtual void BeginRappel();
	virtual void DesireCrouch( void );
	virtual bool WeaponLOSCondition(const Vector &ownerPos, const Vector &targetPos, bool bSetConditions);
	virtual bool OnBeginMoveAndShoot( void );
	virtual const char *GetSquadSlotDebugName( int iSquadSlot );
	virtual void OnEndMoveAndShoot( void );
	virtual bool TestShootPosition(const Vector &vecShootPos, const Vector &targetPos );
	virtual void ClearAttackConditions( void );
	virtual void OnListened();
	virtual void SpeakSentence( int sentenceType );
	virtual Activity GetCoverActivity( CBaseEntity* pHint );
	virtual bool IsCoverPosition( const Vector &vecThreat, const Vector &vecPosition );
	virtual bool OnMoveBlocked( AIMoveResult_t *pResult );
	virtual bool CreateComponents();
	virtual CAI_Senses *CreateSenses();
	virtual CAI_MoveProbe *CreateMoveProbe();
	virtual CAI_Motor *CreateMotor();
	virtual CAI_LocalNavigator *CreateLocalNavigator();
	virtual CAI_Navigator *CreateNavigator();
	virtual CAI_Pathfinder *CreatePathfinder();
	virtual CAI_TacticalServices *CreateTacticalServices();

public: // sign
	void CallNPCThink();
	void SetEnemy(CBaseEntity *pEnemy, bool bSetCondNewEnemy = true);
	void SetupVPhysicsHull();
	bool CineCleanup();
	void TestPlayerPushing( CBaseEntity *pPlayer );

public: // Custom
	virtual Vector GetActualShootPosition( const Vector &shootOrigin );
	virtual Vector GetActualShootTrajectory( const Vector &shootOrigin );

	virtual	float GetSpreadBias( CBaseEntity *pWeapon, CBaseEntity *pTarget );

	virtual CEntity *DropItem(const char *pszItemName, Vector vecPos, QAngle vecAng);

protected:
	static bool			LoadSchedules(void);
	virtual bool		LoadedSchedules(void);

public:
	// Scripted sequence Info
	enum SCRIPTSTATE
	{
		SCRIPT_PLAYING = 0,				// Playing the action animation.
		SCRIPT_WAIT,						// Waiting on everyone in the script to be ready. Plays the pre idle animation if there is one.
		SCRIPT_POST_IDLE,					// Playing the post idle animation after playing the action animation.
		SCRIPT_CLEANUP,					// Cancelling the script / cleaning up.
		SCRIPT_WALK_TO_MARK,				// Walking to the scripted sequence position.
		SCRIPT_RUN_TO_MARK,				// Running to the scripted sequence position.
		SCRIPT_CUSTOM_MOVE_TO_MARK,	// Moving to the scripted sequence position while playing a custom movement animation.
	};


public:
	static void			AddActivityToSR(const char *actName, int conID);
	
	static void			AddEventToSR(const char *eventName, int conID);
	static const char*	GetEventName	(int actID);
	static int			GetEventID	(const char* actName);
	
	static int			GetScheduleID	(const char* schedName);
	static int			GetActivityID	(const char* actName);
	static int			GetConditionID	(const char* condName);
	static int			GetTaskID		(const char* taskName);
	static int			GetSquadSlotID	(const char* slotName);

	static bool			FindSpotForNPCInRadius( Vector *pResult, const Vector &vStartPos, CAI_NPC *pNPC, float radius, bool bOutOfPlayerViewcone = false );

	static string_t gm_iszPlayerSquad;

	static const char*	GetActivityName	(int actID);

public:
	static void InitSchedulingTables();

	/*static CAI_GlobalNamespace gm_SquadSlotNamespace; // use the pointer
	static CAI_LocalIdSpace    gm_SquadSlotIdSpace; // use the pointer
	static CAI_ClassScheduleIdSpace		gm_ClassScheduleIdSpace;*/

	static CAI_GlobalScheduleNamespace	gm_SchedulingSymbols; // need acturl pointer?
	static CAI_ClassScheduleIdSpace		gm_ClassScheduleIdSpace;

	static VALVE_BASEPTR		func_CallNPCThink;

protected:
	static CAI_GlobalNamespace gm_SquadSlotNamespace;
	static CAI_LocalIdSpace    gm_SquadSlotIdSpace;

private:
	static bool			LoadDefaultSchedules(void);

	static void			InitDefaultScheduleSR(void);
	static void			InitDefaultTaskSR(void);
	static void			InitDefaultConditionSR(void);
	static void			InitDefaultActivitySR(void);
	static void			InitDefaultSquadSlotSR(void);

	friend HelperFunction;
	static CStringRegistry*		m_pActivitySR;
	static int					*m_iNumActivities;

	static CStringRegistry*		m_pEventSR;
	static int					*m_iNumEvents;

public:
	DECLARE_DEFAULTHEADER_DETOUR(SetHullSizeNormal, void, (bool force));
	DECLARE_DEFAULTHEADER(GetEnemies, CEAI_Enemies *, ());
	DECLARE_DEFAULTHEADER(NPCInit, void, ());
	DECLARE_DEFAULTHEADER_DETOUR(SetState, void, (NPC_STATE State));
	DECLARE_DEFAULTHEADER(SetActivity, void, (Activity NewActivity));
	DECLARE_DEFAULTHEADER(RunTask, void, (const Task_t *pTask));
	DECLARE_DEFAULTHEADER(OnChangeActivity, void, (Activity eNewActivity));
	DECLARE_DEFAULTHEADER(MaxYawSpeed, float, ());
	DECLARE_DEFAULTHEADER(GetClassScheduleIdSpace, CAI_ClassScheduleIdSpace *, ());
	DECLARE_DEFAULTHEADER(NPCThink, void, ());
	DECLARE_DEFAULTHEADER(IsActivityFinished, bool, ());
	DECLARE_DEFAULTHEADER(CalcIdealYaw, float, (const Vector &vecTarget));
	DECLARE_DEFAULTHEADER(GatherConditions, void, ());
	DECLARE_DEFAULTHEADER(PrescheduleThink, void, ());
	DECLARE_DEFAULTHEADER(IdleSound, void, ());
	DECLARE_DEFAULTHEADER(AlertSound, void, ());
	DECLARE_DEFAULTHEADER(PainSound, void, (const CTakeDamageInfo &info));
	DECLARE_DEFAULTHEADER(DeathSound, void, (const CTakeDamageInfo &info));
	DECLARE_DEFAULTHEADER(FInAimCone_Vector, bool, (const Vector &vecSpot));
	DECLARE_DEFAULTHEADER(BestEnemy, CBaseEntity *, ());
	DECLARE_DEFAULTHEADER(TaskFail, void, (AI_TaskFailureCode_t code));
	DECLARE_DEFAULTHEADER(StartTask, void, (const Task_t *pTask));
	DECLARE_DEFAULTHEADER(Event_Gibbed, bool, (const CTakeDamageInfo &info));
	DECLARE_DEFAULTHEADER(TranslateSchedule, int, (int scheduleType));
	DECLARE_DEFAULTHEADER(SelectSchedule, int, ());
	DECLARE_DEFAULTHEADER(SelectFailSchedule, int, (int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode));
	//DECLARE_DEFAULTHEADER(Weapon_TranslateActivity, Activity, (Activity baseAct, bool *pRequired));
	DECLARE_DEFAULTHEADER(NPC_TranslateActivity, Activity, (Activity eNewActivity));
	DECLARE_DEFAULTHEADER(GetSchedule, CAI_Schedule *, (int schedule));
	DECLARE_DEFAULTHEADER(PlayerHasIlluminatedNPC, void, (CBaseEntity *pPlayer, float flDot));
	DECLARE_DEFAULTHEADER(QuerySeeEntity, bool, (CBaseEntity *pEntity, bool bOnlyHateOrFearIfNPC));
	DECLARE_DEFAULTHEADER(GetSoundInterests, int, ());
	DECLARE_DEFAULTHEADER(BuildScheduleTestBits, void, ());
	DECLARE_DEFAULTHEADER(UpdateEnemyMemory, bool, (CBaseEntity *pEnemy, const Vector &position, CBaseEntity *pInformer));
	DECLARE_DEFAULTHEADER(OverrideMove, bool, ( float flInterval ));
	DECLARE_DEFAULTHEADER(OverrideMoveFacing, bool, (const AILocalMoveGoal_t &move, float flInterval));
	DECLARE_DEFAULTHEADER(GetHitgroupDamageMultiplier , float, (int iHitGroup, const CTakeDamageInfo &info));
	DECLARE_DEFAULTHEADER(OnBehaviorChangeStatus, bool, (CAI_BehaviorBase *pBehavior, bool fCanFinishSchedule));
	DECLARE_DEFAULTHEADER(IsInterruptable, bool, ());
	DECLARE_DEFAULTHEADER(MakeAIFootstepSound, void, (float volume, float duration));
	DECLARE_DEFAULTHEADER(OnScheduleChange, void, ());
	DECLARE_DEFAULTHEADER(TranslateNavGoal, void, (CBaseEntity *pEnemy, Vector &chasePosition));
	DECLARE_DEFAULTHEADER(MeleeAttack1Conditions, int, (float flDot, float flDist ));
	DECLARE_DEFAULTHEADER(IsUnreachable, bool, (CBaseEntity* pEntity));
	DECLARE_DEFAULTHEADER(OnObstructingDoor, bool, (AILocalMoveGoal_t *pMoveGoal, CBaseEntity *pDoor, float distClear, AIMoveResult_t *pResult));
	DECLARE_DEFAULTHEADER(IsHeavyDamage, bool, (const CTakeDamageInfo &info));
	DECLARE_DEFAULTHEADER(GetTimeToNavGoal, float, ());
	DECLARE_DEFAULTHEADER(StepHeight, float, () const);
	DECLARE_DEFAULTHEADER(GetJumpGravity, float, () const);
	DECLARE_DEFAULTHEADER(ShouldPlayerAvoid, bool, ());
	DECLARE_DEFAULTHEADER(ShouldProbeCollideAgainstEntity, bool, (CBaseEntity *pEntity));
	DECLARE_DEFAULTHEADER(CapabilitiesGet,int, () const);
	DECLARE_DEFAULTHEADER(IsCrouching, bool,  ());
	DECLARE_DEFAULTHEADER(OnChangeHintGroup, void,  (string_t oldGroup, string_t newGroup));
	DECLARE_DEFAULTHEADER(IsJumpLegal, bool, (const Vector &startPos, const Vector &apex, const Vector &endPos ) const);
	DECLARE_DEFAULTHEADER(ShouldFailNav, bool, (bool bMovementFailed));
	DECLARE_DEFAULTHEADER(IsNavigationUrgent, bool, ());
	DECLARE_DEFAULTHEADER(GetDefaultNavGoalTolerance, float, ());
	DECLARE_DEFAULTHEADER(OnMovementFailed, void, ());
	DECLARE_DEFAULTHEADER(ShouldBruteForceFailedNav, bool, ());
	DECLARE_DEFAULTHEADER(IsUnusableNode, bool, (int iNodeID, CBaseEntity *pHint));
	DECLARE_DEFAULTHEADER(MovementCost, bool, (int moveType, const Vector &vecStart, const Vector &vecEnd, float *pCost));
	DECLARE_DEFAULTHEADER(GetNodeViewOffset, Vector, ());
	DECLARE_DEFAULTHEADER(PostNPCInit, void, ());
	DECLARE_DEFAULTHEADER(InnateRange1MaxRange, float, ());
	DECLARE_DEFAULTHEADER(InnateRange1MinRange, float, ());
	DECLARE_DEFAULTHEADER(RangeAttack1Conditions, int, ( float flDot, float flDist));
	DECLARE_DEFAULTHEADER(RangeAttack2Conditions, int, ( float flDot, float flDist));
	DECLARE_DEFAULTHEADER(OnStateChange, void, (NPC_STATE OldState, NPC_STATE NewState));
	DECLARE_DEFAULTHEADER(ShouldPlayIdleSound, bool, ());
	DECLARE_DEFAULTHEADER(IsLightDamage, bool, (const CTakeDamageInfo &info));
	DECLARE_DEFAULTHEADER(CanBeAnEnemyOf, bool, ( CBaseEntity *pEnemy ));
	DECLARE_DEFAULTHEADER(AllowedToIgnite, bool, ());
	DECLARE_DEFAULTHEADER(GatherEnemyConditions, void, ( CBaseEntity *pEnemy ));
	DECLARE_DEFAULTHEADER(IsSilentSquadMember, bool, () const);
	DECLARE_DEFAULTHEADER(OnMovementComplete, void, ());
	DECLARE_DEFAULTHEADER(AddFacingTarget_E_V_F_F_F, void, ( CBaseEntity *pTarget, const Vector &vecPosition, float flImportance, float flDuration, float flRamp));
	DECLARE_DEFAULTHEADER(QueryHearSound, bool, (CSound *pSound));
	DECLARE_DEFAULTHEADER(IsPlayerAlly, bool, (CBaseEntity *IsPlayerAlly));
	DECLARE_DEFAULTHEADER(GetRunningBehavior, CAI_BehaviorBase *, ());
	DECLARE_DEFAULTHEADER(OnUpdateShotRegulator, void, ());
	DECLARE_DEFAULTHEADER(CleanupOnDeath, void,( CBaseEntity *pCulprit, bool bFireDeathOutput ));
	DECLARE_DEFAULTHEADER(OnStartSchedule, void,( int scheduleType ));
	DECLARE_DEFAULTHEADER(AimGun, void,());
	DECLARE_DEFAULTHEADER(GetSchedulingErrorName, const char *,());
	DECLARE_DEFAULTHEADER(IsCurTaskContinuousMove, bool,());
	DECLARE_DEFAULTHEADER(IsValidEnemy, bool,( CBaseEntity *pEnemy ));
	DECLARE_DEFAULTHEADER(IsValidCover, bool,( const Vector &vLocation, CBaseEntity const *pHint ));
	DECLARE_DEFAULTHEADER(IsValidShootPosition, bool,( const Vector &vLocation, CAI_Node *pNode, CBaseEntity const *pHint ));
	DECLARE_DEFAULTHEADER(GetMaxTacticalLateralMovement, float,());
	DECLARE_DEFAULTHEADER(ShouldIgnoreSound, bool,( CSound *pSound ));
	DECLARE_DEFAULTHEADER(OnSeeEntity, void,( CBaseEntity *pEntity ));
	DECLARE_DEFAULTHEADER(GetReasonableFacingDist, float,());
	DECLARE_DEFAULTHEADER(CanFlinch, bool,());
	DECLARE_DEFAULTHEADER(IsCrouchedActivity, bool,( Activity activity ));
	DECLARE_DEFAULTHEADER(CanRunAScriptedNPCInteraction, bool,( bool bForced ));
	DECLARE_DEFAULTHEADER(GetFlinchActivity, Activity, ( bool bHeavyDamage, bool bGesture ));
	DECLARE_DEFAULTHEADER(OnCalcBaseMove, bool,( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult ));
	DECLARE_DEFAULTHEADER(ShouldAlwaysThink, bool,());
	DECLARE_DEFAULTHEADER(ScheduledMoveToGoalEntity, bool,( int scheduleType, CBaseEntity *pGoalEntity, Activity movementActivity ));
	DECLARE_DEFAULTHEADER(ScheduledFollowPath, bool,( int scheduleType, CBaseEntity *pPathStart, Activity movementActivity ));
	DECLARE_DEFAULTHEADER(TaskName, const char *,(int taskID));
	DECLARE_DEFAULTHEADER(GetNewSchedule, CAI_Schedule *,());
	DECLARE_DEFAULTHEADER(GetFailSchedule, CAI_Schedule *,());
	DECLARE_DEFAULTHEADER(AccessBehaviors,CAI_BehaviorBase **,());
	DECLARE_DEFAULTHEADER(NumBehaviors, int, ());
	DECLARE_DEFAULTHEADER(GetExpresser, CAI_Expresser *, ());
	DECLARE_DEFAULTHEADER(ValidateNavGoal, bool, ());
	DECLARE_DEFAULTHEADER(NotifyDeadFriend, void, (CBaseEntity *pFriend));
	DECLARE_DEFAULTHEADER(SetAim, void, (const Vector &aimDir));
	DECLARE_DEFAULTHEADER(RelaxAim, void, ());
	DECLARE_DEFAULTHEADER(GetHintActivity, Activity, ( short sHintType, Activity HintsActivity ));
	DECLARE_DEFAULTHEADER(FValidateHintType, bool, ( CBaseEntity *pHint ));
	DECLARE_DEFAULTHEADER(GetHintDelay, float,( short sHintType ));
	DECLARE_DEFAULTHEADER(InnateWeaponLOSCondition, bool, ( const Vector &ownerPos, const Vector &targetPos, bool bSetConditions ));	
	DECLARE_DEFAULTHEADER(OnObstructionPreSteer, bool, ( AILocalMoveGoal_t *pMoveGoal, float distClear, AIMoveResult_t *pResult ));
	DECLARE_DEFAULTHEADER(CoverRadius, float, ());
	DECLARE_DEFAULTHEADER(SetTurnActivity, void, ());
	DECLARE_DEFAULTHEADER(FCanCheckAttacks, bool, ());
	DECLARE_DEFAULTHEADER(GetGlobalScheduleId, int, ( int localScheduleID ));
	DECLARE_DEFAULTHEADER(AddFacingTarget_V_F_F_F, void, ( const Vector &vecPosition, float flImportance, float flDuration, float flRamp ));
	DECLARE_DEFAULTHEADER(SetSquad, void, ( CAI_Squad *pSquad ));
	DECLARE_DEFAULTHEADER(ClearCommandGoal, void, ());
	DECLARE_DEFAULTHEADER(FindCoverPosInRadius, bool, ( CBaseEntity *pEntity, const Vector &goalPos, float coverRadius, Vector *pResult ));
	DECLARE_DEFAULTHEADER(CheckAmmo, void, ());
	DECLARE_DEFAULTHEADER(OnRangeAttack1, void, ());
	DECLARE_DEFAULTHEADER(GetShootEnemyDir, Vector,( const Vector &shootOrigin, bool bNoisy));
	DECLARE_DEFAULTHEADER(SetScriptedScheduleIgnoreConditions, void, ( Interruptability_t interrupt ));
	DECLARE_DEFAULTHEADER(OnStartScene, void, ());
	DECLARE_DEFAULTHEADER(SelectIdealState, NPC_STATE, ());
	DECLARE_DEFAULTHEADER(RunAI, void, ());
	DECLARE_DEFAULTHEADER(ShouldMoveAndShoot, bool, ());
	DECLARE_DEFAULTHEADER(CanBeUsedAsAFriend, bool, ());
	DECLARE_DEFAULTHEADER(OnClearGoal, void, ( CAI_BehaviorBase *pBehavior, CE_AI_GoalEntity *pGoal ));
	DECLARE_DEFAULTHEADER(ShouldAcceptGoal, bool, ( CAI_BehaviorBase *pBehavior, CE_AI_GoalEntity *pGoal ));
	DECLARE_DEFAULTHEADER(Wake, void, ( bool bFireOutput ));
	DECLARE_DEFAULTHEADER(GetReactionDelay, float, ( CBaseEntity *pEnemy ));
	DECLARE_DEFAULTHEADER(SquadSlotName, const char*, (int slotID));
	DECLARE_DEFAULTHEADER(PlayerInSpread, bool, ( const Vector &sourcePos, const Vector &targetPos, float flSpread, float maxDistOffCenter, bool ignoreHatedPlayers));
	DECLARE_DEFAULTHEADER(EyeOffset, Vector, ( Activity nActivity ));
	DECLARE_DEFAULTHEADER(CollectShotStats, void, ( const Vector &vecShootOrigin, const Vector &vecShootDir ));
	DECLARE_DEFAULTHEADER(OnLooked, void, ( int iDistance ));
	DECLARE_DEFAULTHEADER(ShouldNotDistanceCull, bool, ());
	DECLARE_DEFAULTHEADER(HolsterWeapon, int, ( void ));
	DECLARE_DEFAULTHEADER(UnholsterWeapon, int,( void ));
	DECLARE_DEFAULTHEADER(FindNamedEntity, CBaseEntity *, ( const char *pszName, IEntityFindFilter *pFilter ));
	DECLARE_DEFAULTHEADER(FacingPosition, Vector, ());
	DECLARE_DEFAULTHEADER(AddFacingTarget_E_F_F_F, void, ( CBaseEntity *pTarget, float flImportance, float flDuration, float flRamp ));
	DECLARE_DEFAULTHEADER(ValidEyeTarget, bool, (const Vector &lookTargetPos));
	DECLARE_DEFAULTHEADER(SetHeadDirection, void, ( const Vector &vTargetPos, float flInterval ));
	DECLARE_DEFAULTHEADER(MaintainTurnActivity, void, ( void ));
	DECLARE_DEFAULTHEADER(AddLookTarget_E, void, ( CBaseEntity *pTarget, float flImportance, float flDuration, float flRamp ));
	DECLARE_DEFAULTHEADER(AddLookTarget_V, void, ( const Vector &vecPosition, float flImportance, float flDuration, float flRamp ));
	DECLARE_DEFAULTHEADER(MaintainLookTargets, void, ( float flInterval ));
	DECLARE_DEFAULTHEADER(Stand, bool, ( void ));
	DECLARE_DEFAULTHEADER(Crouch, bool, ( void ));
	DECLARE_DEFAULTHEADER(GetBestSound, CSound *, ( int validTypes ));
	DECLARE_DEFAULTHEADER(FOkToMakeSound, bool, ( int soundPriority ));
	DECLARE_DEFAULTHEADER(JustMadeSound, void, ( int soundPriority, float flSoundLength ));
	DECLARE_DEFAULTHEADER(FearSound, void, ( void ));
	DECLARE_DEFAULTHEADER(IsWaitingToRappel, bool, ());
	DECLARE_DEFAULTHEADER(BeginRappel, void,());
	DECLARE_DEFAULTHEADER(DesireCrouch, void, ( void ));
	DECLARE_DEFAULTHEADER(WeaponLOSCondition, bool, (const Vector &ownerPos, const Vector &targetPos, bool bSetConditions));
	DECLARE_DEFAULTHEADER(OnBeginMoveAndShoot, bool, ( void ));
	DECLARE_DEFAULTHEADER(GetSquadSlotDebugName, const char*, ( int iSquadSlot ));
	DECLARE_DEFAULTHEADER(OnEndMoveAndShoot, void,( void ));
	DECLARE_DEFAULTHEADER(TestShootPosition, bool, (const Vector &vecShootPos, const Vector &targetPos ));
	DECLARE_DEFAULTHEADER(ClearAttackConditions, void, ( void ));
	DECLARE_DEFAULTHEADER(OnListened, void, ());
	DECLARE_DEFAULTHEADER(SpeakSentence, void, ( int sentenceType ));
	DECLARE_DEFAULTHEADER(GetCoverActivity,Activity,( CBaseEntity* pHint ));
	DECLARE_DEFAULTHEADER(IsCoverPosition, bool, ( const Vector &vecThreat, const Vector &vecPosition ));
	DECLARE_DEFAULTHEADER(OnMoveBlocked, bool, (AIMoveResult_t *pResult));
	DECLARE_DEFAULTHEADER(CreateComponents, bool, ());
	DECLARE_DEFAULTHEADER(CreateSenses, CAI_Senses *, ());
	DECLARE_DEFAULTHEADER(CreateMoveProbe, CAI_MoveProbe *, ());
	DECLARE_DEFAULTHEADER(CreateMotor, CAI_Motor *, ());
	DECLARE_DEFAULTHEADER(CreateLocalNavigator, CAI_LocalNavigator *, ());
	DECLARE_DEFAULTHEADER(CreateNavigator, CAI_Navigator *, ());
	DECLARE_DEFAULTHEADER(CreatePathfinder, CAI_Pathfinder *, ());
	DECLARE_DEFAULTHEADER(CreateTacticalServices, CAI_TacticalServices *, ());

public:
	DECLARE_DEFAULTHEADER_DETOUR(SetSchedule_Int, bool, (int localScheduleID));
	DECLARE_DEFAULTHEADER_DETOUR(SetEnemy, void, (CBaseEntity *pEnemy, bool bSetCondNewEnemy));
	DECLARE_DEFAULTHEADER_DETOUR(SetupVPhysicsHull, void, ());
	DECLARE_DEFAULTHEADER_DETOUR(CineCleanup, bool, ());
	DECLARE_DEFAULTHEADER_DETOUR(TranslateActivity, Activity, (Activity idealActivity, Activity *pIdealWeaponActivity));

protected: // sign
	void EndTaskOverlay();


protected:
	bool IsRunningDynamicInteraction( void ) { return (m_iInteractionState != NPCINT_NOT_RUNNING && (m_hCine.ptr != NULL)); }

protected: //Sendprops


public: //Datamaps
	DECLARE_DATAMAP(NPC_STATE, m_NPCState);
	DECLARE_DATAMAP(string_t, m_spawnEquipment);
	DECLARE_DATAMAP(CFakeHandle, m_hCine);
	DECLARE_DATAMAP(float, m_flDistTooFar);
	DECLARE_DATAMAP(float, m_flMoveWaitFinished);
	DECLARE_DATAMAP(Vector, m_vInterruptSavePosition);

	DECLARE_DATAMAP(COutputEvent, m_OnRappelTouchdown);
	
protected:
	DECLARE_DATAMAP(int, m_afCapability);
	DECLARE_DATAMAP_OFFSET(CAI_ScheduleBits, m_Conditions);
	DECLARE_DATAMAP(CFakeHandle, m_hEnemy);
	DECLARE_DATAMAP(AIScheduleState_t, m_ScheduleState);
	DECLARE_DATAMAP(CAI_MoveAndShootOverlay, m_MoveAndShootOverlay);
	DECLARE_DATAMAP(CAI_Motor *, m_pMotor);
	DECLARE_DATAMAP(float, m_flNextFlinchTime);
	DECLARE_DATAMAP(CAI_MoveProbe *, m_pMoveProbe);
	DECLARE_DATAMAP(Activity, m_Activity);
	DECLARE_DATAMAP(string_t, m_strHintGroup);
	DECLARE_DATAMAP(CFakeHandle, m_pHintNode);
	DECLARE_DATAMAP(AI_SleepState_t, m_SleepState);
	DECLARE_DATAMAP_OFFSET(CAI_Schedule *, m_pSchedule); // "Schedule cleared: %s\"
	DECLARE_DATAMAP(int, m_IdealSchedule);
	DECLARE_DATAMAP(int, m_iInteractionState);
	DECLARE_DATAMAP(Activity, m_IdealActivity);
	DECLARE_DATAMAP(CAI_Navigator *, m_pNavigator);
	DECLARE_DATAMAP_OFFSET(CAI_Squad *, m_pSquad); // "Found %s that isn't in a squad\n"
	DECLARE_DATAMAP(int, m_iMySquadSlot);
	DECLARE_DATAMAP(int, m_afMemory);
	DECLARE_DATAMAP_OFFSET(CAI_ScheduleBits, m_CustomInterruptConditions);
	DECLARE_DATAMAP(NPC_STATE, m_IdealNPCState);
	DECLARE_DATAMAP_OFFSET(int, m_poseMove_Yaw); // move_yaw
	DECLARE_DATAMAP_OFFSET(CAI_ScheduleBits, m_InverseIgnoreConditions);
	DECLARE_DATAMAP(float, m_flWaitFinished);
	DECLARE_DATAMAP(CFakeHandle, m_hInteractionPartner);
	DECLARE_DATAMAP(string_t, m_SquadName);
	DECLARE_DATAMAP(Vector, m_vDefaultEyeOffset);
	DECLARE_DATAMAP(CAI_LocalNavigator *, m_pLocalNavigator);
	DECLARE_DATAMAP(string_t, m_iszSceneCustomMoveSeq);
	DECLARE_DATAMAP(CFakeHandle, m_hTargetEnt);
	DECLARE_DATAMAP(SCRIPTSTATE, m_scriptState);
	DECLARE_DATAMAP(CFakeHandle, m_hGoalEnt);
	DECLARE_DATAMAP(CAI_TacticalServices *, m_pTacticalServices);
	DECLARE_DATAMAP(bool, m_bHintGroupNavLimiting);
	DECLARE_DATAMAP(CAI_Pathfinder *, m_pPathfinder);
	DECLARE_DATAMAP(float, m_flLastDamageTime);
	DECLARE_DATAMAP(Vector, m_vSavePosition);
	DECLARE_DATAMAP(CUtlVector<UnreachableEnt_t> , m_UnreachableEnts);
	DECLARE_DATAMAP(int, m_EnemiesSerialNumber);
	DECLARE_DATAMAP(CFakeHandle, m_hEnemyOccluder);
	DECLARE_DATAMAP(CAI_Senses *, m_pSenses);
	DECLARE_DATAMAP(COutputEvent, m_OnLostPlayer);
	DECLARE_DATAMAP(COutputEvent, m_OnLostEnemy);
	DECLARE_DATAMAP(bool, m_bInAScript);
	DECLARE_DATAMAP(float, m_flLastAttackTime);
	DECLARE_DATAMAP(CAI_ShotRegulator, m_ShotRegulator);
	DECLARE_DATAMAP(AI_Efficiency_t, m_Efficiency);
	DECLARE_DATAMAP(AI_MoveEfficiency_t, m_MoveEfficiency);
	DECLARE_DATAMAP(DesiredWeaponState_t, m_iDesiredWeaponState);
	DECLARE_DATAMAP(float, m_flEyeIntegRate);
	DECLARE_DATAMAP(bool, m_bConditionsGathered);
	DECLARE_DATAMAP(int, m_iInteractionPlaying);
	DECLARE_DATAMAP(CUtlVector<ScriptedNPCInteraction_t>, m_ScriptedInteractions);
	DECLARE_DATAMAP(bool, m_bCannotDieDuringInteraction);
	DECLARE_DATAMAP(COutputEvent, m_OnDamaged);
	DECLARE_DATAMAP(COutputEvent, m_OnDamagedByPlayer);
	DECLARE_DATAMAP(COutputEvent, m_OnDamagedByPlayerSquad);
	DECLARE_DATAMAP(COutputEvent, m_OnHalfHealth);
	DECLARE_DATAMAP(bool, m_bForceConditionsGather);
	DECLARE_DATAMAP(float, m_flSumDamage);
	DECLARE_DATAMAP(float, m_flLastPlayerDamageTime);
	DECLARE_DATAMAP_OFFSET(int, m_poseAim_Pitch);
	DECLARE_DATAMAP_OFFSET(int, m_poseAim_Yaw);
	DECLARE_DATAMAP(float, m_flSceneTime);
	DECLARE_DATAMAP(bool, m_bCrouchDesired);
	DECLARE_DATAMAP(bool, m_bForceCrouch);
	DECLARE_DATAMAP_OFFSET(CAI_ScheduleBits, m_ConditionsPreIgnore);
	DECLARE_DATAMAP(float, m_flLastEnemyTime);
	DECLARE_DATAMAP(float, m_flNextDecisionTime);
	DECLARE_DATAMAP(bool, m_bSkippedChooseEnemy);
	DECLARE_DATAMAP(float, m_flTimeEnemyAcquired);




	friend class CAI_SchedulesManager;
	friend class CEAI_ScriptedSequence;
};



inline bool	CAI_NPC::HaveSequenceForActivity( Activity activity )				
{
	return g_helpfunc.HaveSequenceForActivity(BaseEntity(), activity);
}

//-----------------------------------------------------------------------------
// Purpose: Sets the ideal state of this NPC.
//-----------------------------------------------------------------------------
inline void	CAI_NPC::SetIdealState( NPC_STATE eIdealState )
{
	if (eIdealState != m_IdealNPCState)
	{
		m_IdealNPCState = eIdealState;
	}
}

inline bool CAI_NPC::CrouchIsDesired( void ) const
{
	return ( (CapabilitiesGet() & bits_CAP_DUCK) && (m_bCrouchDesired | m_bForceCrouch) );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the current ideal state the NPC will try to achieve.
//-----------------------------------------------------------------------------
inline NPC_STATE CAI_NPC::GetIdealState()
{
	return m_IdealNPCState;
}

//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
inline void CAI_NPC::ResetScheduleCurTaskIndex()
{
	m_ScheduleState->iCurTask = 0;
	m_ScheduleState->iTaskInterrupt = 0;
	m_ScheduleState->bTaskRanAutomovement = false;
	m_ScheduleState->bTaskUpdatedYaw = false;
}

inline void	CAI_NPC::ForceCrouch( void )
{
	m_bForceCrouch = true;
	Crouch();
}

inline void	CAI_NPC::ClearForceCrouch( void )
{
	m_bForceCrouch = false;

	if ( IsCrouching() )
	{
		Stand();
	}
}





inline const QAngle &CAI_Component::GetLocalAngles( void ) const
{
	return GetOuter()->GetLocalAngles();
}

inline const Vector &CAI_Component::GetLocalOrigin() const
{
	return GetOuter()->GetLocalOrigin();
}

inline void CAI_Component::Forget( int iMemory )
{
	GetOuter()->Forget( iMemory );
}

inline int CAI_Component::GetSequence()
{
	return GetOuter()->GetSequence();
}

inline void CAI_Component::TaskComplete( bool fIgnoreSetFailedCondition )
{
	GetOuter()->TaskComplete( fIgnoreSetFailedCondition );
}

inline void CAI_Component::TaskFail( AI_TaskFailureCode_t code )
{
	GetOuter()->TaskFail( code );
}

inline const Task_t *CAI_Component::GetCurTask()
{
	return GetOuter()->GetTask();
}

inline void CAI_Component::TaskFail( const char *pszGeneralFailText )
{
	GetOuter()->TaskFail( pszGeneralFailText );
}

inline void CAI_Component::SetActivity( Activity NewActivity )
{
	GetOuter()->SetActivity( NewActivity );
}

inline const Vector &CAI_Component::GetAbsOrigin() const
{
	return GetOuter()->GetAbsOrigin();
}

inline float CAI_Component::GetHullWidth() const
{
	return NAI_Hull::Width(GetOuter()->GetHullType());
}

inline float CAI_Component::GetLastThink( const char *szContext )
{
	return GetOuter()->GetLastThink( szContext );
}

inline Activity CAI_Component::GetActivity()
{
	return GetOuter()->GetActivity();
}

inline void	 CAI_Component::SetGroundEntity( CBaseEntity *ground )
{
	GetOuter()->SetGroundEntity( ground );
}

inline Vector CAI_Component::WorldSpaceCenter() const
{
	return GetOuter()->WorldSpaceCenter();
}

inline void CAI_Component::SetGravity( float flGravity )
{
	GetOuter()->SetGravity( flGravity );
}

inline void CAI_Component::SetSolid( SolidType_t val )
{
	GetOuter()->SetSolid(val);
}

inline void CAI_Component::SetLocalAngles( const QAngle& angles )
{
	GetOuter()->SetLocalAngles( angles );
}

inline void CAI_Component::SetLocalOrigin(const Vector &origin)
{
	GetOuter()->SetLocalOrigin(origin);
}

inline float CAI_Component::GetHullHeight() const
{
	return NAI_Hull::Height(GetOuter()->GetHullType());
}

inline int CAI_Component::GetCollisionGroup() const
{
	return GetOuter()->GetCollisionGroup();
}

inline const Vector &CAI_Component::WorldAlignMaxs() const
{
	return GetOuter()->WorldAlignMaxs();
}

inline const Vector &CAI_Component::WorldAlignMins() const
{
	return GetOuter()->WorldAlignMins();
}

inline const Vector &CAI_Component::GetHullMins() const
{
	return NAI_Hull::Mins(GetOuter()->GetHullType());
}

inline const Vector &CAI_Component::GetHullMaxs() const
{
	return NAI_Hull::Maxs(GetOuter()->GetHullType());
}

inline void CAI_Component::Remember( int iMemory )
{
	GetOuter()->Remember( iMemory );
}

inline const QAngle &CAI_Component::GetAbsAngles() const
{
	return GetOuter()->GetAbsAngles();
}

inline bool CAI_Component::HasMemory( int iMemory )
{
	return GetOuter()->HasMemory( iMemory );
}

inline const Vector &CAI_Component::GetEnemyLKP() const
{
	return GetOuter()->GetEnemyLKP();
}

inline void CAI_Component::TranslateNavGoal( CEntity *pEnemy, Vector &chasePosition )
{
	GetOuter()->TranslateNavGoal( (pEnemy) ? pEnemy->BaseEntity() : NULL, chasePosition );
}


inline void CAI_Component::SetTarget( CBaseEntity *pTarget )
{
	GetOuter()->SetTarget( pTarget );
}

inline CEntity* CAI_Component::GetGoalEnt()
{
	return GetOuter()->GetGoalEnt();
}

inline void CAI_Component::SetGoalEnt( CBaseEntity *pGoalEnt )
{
	GetOuter()->SetGoalEnt( pGoalEnt );
}

inline int CAI_Component::CapabilitiesGet()
{
	return GetOuter()->CapabilitiesGet();
}

inline Hull_t CAI_Component::GetHullType() const
{
	return GetOuter()->GetHullType();
}

inline CEntity *CAI_Component::GetTarget()
{
	return GetOuter()->GetTarget();
}

inline CEntity *CAI_Component::GetEnemy()
{
	return GetOuter()->GetEnemy();
}







//=============================================================================
//
// class CAI_Manager
//
// Central location for components of the AI to operate across all AIs without
// iterating over the global list of entities.
//
//=============================================================================

class CAI_Manager
{
public:
	CAI_Manager();
	
	CAI_NPC **		AccessAIs();
	int				NumAIs();
	
	void AddAI( CAI_NPC *pAI );
	void RemoveAI( CAI_NPC *pAI );

	bool FindAI( CAI_NPC *pAI )	{ return ( m_AIs.Find( pAI ) != m_AIs.InvalidIndex() ); }
	
private:
	enum
	{
		MAX_AIS = 256
	};
	
	typedef CUtlVector<CAI_NPC *> CAIArray;
	
	CAIArray m_AIs;

};

//-------------------------------------

extern CAI_Manager g_AI_Manager;


//-------------------------------------

struct AI_NamespaceAddInfo_t
{
	AI_NamespaceAddInfo_t( const char *pszName, int localId )
	 :	pszName( pszName ),
		localId( localId )
	{
	}
	
	const char *pszName;
	int			localId;
};


class CAI_NamespaceInfos : public CUtlVector<AI_NamespaceAddInfo_t>
{
public:
	void PushBack(  const char *pszName, int localId )
	{
		AddToTail( AI_NamespaceAddInfo_t( pszName, localId ) );
	}

	void Sort()
	{
		CUtlVector<AI_NamespaceAddInfo_t>::Sort( Compare );
	}
	
private:
	static int __cdecl Compare(const AI_NamespaceAddInfo_t *pLeft, const AI_NamespaceAddInfo_t *pRight )
	{
		return pLeft->localId - pRight->localId;
	}
	
};


//-------------------------------------

// Declares the static variables that hold the string registry offset for the new subclass
// as well as the initialization in schedule load functions

struct AI_SchedLoadStatus_t
{
	bool fValid;
	int  signature;
};


// Load schedules pulled out to support stepping through with debugger
inline bool AI_DoLoadSchedules( bool (*pfnBaseLoad)(), void (*pfnInitCustomSchedules)(),
								AI_SchedLoadStatus_t *pLoadStatus )
{
	(*pfnBaseLoad)();
	
	if (pLoadStatus->signature != g_AI_SchedulesManager.GetScheduleLoadSignature())
	{
		(*pfnInitCustomSchedules)();
		pLoadStatus->fValid	   = true;
		pLoadStatus->signature = g_AI_SchedulesManager.GetScheduleLoadSignature();
	}
	return pLoadStatus->fValid;
}


//-------------------------------------


typedef bool (*AIScheduleLoadFunc_t)();






//-------------------------------------

#define ADD_CUSTOM_SCHEDULE_NAMED(derivedClass,schedName,schedEN)\
	if ( !derivedClass::AccessClassScheduleIdSpaceDirect().AddSchedule( schedName, schedEN, derivedClass::gm_pszErrorClassName ) ) return;

#define ADD_CUSTOM_SCHEDULE(derivedClass,schedEN) ADD_CUSTOM_SCHEDULE_NAMED(derivedClass,#schedEN,schedEN)

#define ADD_CUSTOM_TASK_NAMED(derivedClass,taskName,taskEN)\
	if ( !derivedClass::AccessClassScheduleIdSpaceDirect().AddTask( taskName, taskEN, derivedClass::gm_pszErrorClassName ) ) return;

#define ADD_CUSTOM_TASK(derivedClass,taskEN) ADD_CUSTOM_TASK_NAMED(derivedClass,#taskEN,taskEN)

#define ADD_CUSTOM_CONDITION_NAMED(derivedClass,condName,condEN)\
	if ( !derivedClass::AccessClassScheduleIdSpaceDirect().AddCondition( condName, condEN, derivedClass::gm_pszErrorClassName ) ) return;

#define ADD_CUSTOM_CONDITION(derivedClass,condEN) ADD_CUSTOM_CONDITION_NAMED(derivedClass,#condEN,condEN)

//-------------------------------------

#define INIT_CUSTOM_AI(derivedClass)\
	derivedClass::AccessClassScheduleIdSpaceDirect().Init( #derivedClass, BaseClass::GetSchedulingSymbols(), &BaseClass::AccessClassScheduleIdSpaceDirect() ); \
	derivedClass::gm_SquadSlotIdSpace.Init( &CAI_BaseNPC::gm_SquadSlotNamespace, &BaseClass::gm_SquadSlotIdSpace);

// CEntity version of the above one
#define CE_INIT_CUSTOM_AI(derivedClass)\
	derivedClass::AccessClassScheduleIdSpaceDirect().Init( #derivedClass, BaseClass::GetSchedulingSymbols(), &BaseClass::AccessClassScheduleIdSpaceDirect() ); \
	derivedClass::gm_SquadSlotIdSpace.Init( &CAI_NPC::gm_SquadSlotNamespace, &BaseClass::gm_SquadSlotIdSpace);

#define	ADD_CUSTOM_INTERACTION( interaction )	{ interaction = CCombatCharacter::GetInteractionID(); }

#define ADD_CUSTOM_SQUADSLOT_NAMED(derivedClass,squadSlotName,squadSlotEN)\
	if ( !derivedClass::gm_SquadSlotIdSpace.AddSymbol( squadSlotName, squadSlotEN, "squadslot", derivedClass::gm_pszErrorClassName ) ) return;

#define ADD_CUSTOM_SQUADSLOT(derivedClass,squadSlotEN) ADD_CUSTOM_SQUADSLOT_NAMED(derivedClass,#squadSlotEN,squadSlotEN)

#define ADD_CUSTOM_ACTIVITY_NAMED(derivedClass,activityName,activityEnum)\
	REGISTER_PRIVATE_ACTIVITY(activityEnum);\
	CAI_NPC::AddActivityToSR(activityName,activityEnum);

#define ADD_CUSTOM_ACTIVITY(derivedClass,activityEnum) ADD_CUSTOM_ACTIVITY_NAMED(derivedClass,#activityEnum,activityEnum)


#define ADD_CUSTOM_ANIMEVENT_NAMED(derivedClass,eventName,eventEnum)\
	REGISTER_PRIVATE_ANIMEVENT(eventEnum);\
	CAI_NPC::AddEventToSR(eventName,eventEnum);

#define ADD_CUSTOM_ANIMEVENT(derivedClass,eventEnum) ADD_CUSTOM_ANIMEVENT_NAMED(derivedClass,#eventEnum,eventEnum)




//-------------------------------------

#define DEFINE_CUSTOM_SCHEDULE_PROVIDER\
	static AI_SchedLoadStatus_t 		gm_SchedLoadStatus; \
	static CAI_ClassScheduleIdSpace 	gm_ClassScheduleIdSpace; \
	static const char *					gm_pszErrorClassName;\
	\
	static CAI_ClassScheduleIdSpace &AccessClassScheduleIdSpaceDirect() 	{ return gm_ClassScheduleIdSpace; } \
	virtual CAI_ClassScheduleIdSpace *	GetClassScheduleIdSpace()			{ return &gm_ClassScheduleIdSpace; } \
	virtual const char *				GetSchedulingErrorName()			{ return gm_pszErrorClassName; } \
	\
	static void							InitCustomSchedules(void);\
	\
	static bool							LoadSchedules(void);\
	virtual bool						LoadedSchedules(void); \
	\
	friend class ScheduleLoadHelperImpl;	\
	\
	class CScheduleLoader \
	{ \
	public: \
		CScheduleLoader(); \
	} m_ScheduleLoader; \
	\
	friend class CScheduleLoader;


//-------------------------------------

#define DEFINE_CUSTOM_AI\
	DEFINE_CUSTOM_SCHEDULE_PROVIDER \
	\
	static CAI_LocalIdSpace gm_SquadSlotIdSpace; \
	\
	const char*				SquadSlotName	(int squadSlotID);


//-------------------------------------

#define IMPLEMENT_CUSTOM_SCHEDULE_PROVIDER(derivedClass)\
	AI_SchedLoadStatus_t		derivedClass::gm_SchedLoadStatus = { true, -1 }; \
	CAI_ClassScheduleIdSpace 	derivedClass::gm_ClassScheduleIdSpace; \
	const char *				derivedClass::gm_pszErrorClassName = #derivedClass; \
	\
	derivedClass::CScheduleLoader::CScheduleLoader()\
	{ \
		derivedClass::LoadSchedules(); \
	} \
	\
	/* --------------------------------------------- */ \
	/* Load schedules for this type of NPC           */ \
	/* --------------------------------------------- */ \
	bool derivedClass::LoadSchedules(void)\
	{\
		return AI_DoLoadSchedules( derivedClass::BaseClass::LoadSchedules, \
								   derivedClass::InitCustomSchedules, \
								   &derivedClass::gm_SchedLoadStatus ); \
	}\
	\
	bool derivedClass::LoadedSchedules(void) \
	{ \
		return derivedClass::gm_SchedLoadStatus.fValid;\
	} 


//-------------------------------------

// Initialize offsets and implement methods for loading and getting squad info for the subclass
#define IMPLEMENT_CUSTOM_AI(className, derivedClass)\
	IMPLEMENT_CUSTOM_SCHEDULE_PROVIDER(derivedClass)\
	\
	CAI_LocalIdSpace 	derivedClass::gm_SquadSlotIdSpace; \
	\
	/* -------------------------------------------------- */ \
	/* Given squadSlot enumeration return squadSlot name */ \
	/* -------------------------------------------------- */ \
	const char* derivedClass::SquadSlotName(int slotEN)\
	{\
		return "";\
	}


#define AI_BEGIN_CUSTOM_SCHEDULE_PROVIDER( derivedClass ) \
	IMPLEMENT_CUSTOM_SCHEDULE_PROVIDER(derivedClass ) \
	void derivedClass::InitCustomSchedules( void ) \
	{ \
		typedef derivedClass CNpc; \
		const char *pszClassName = #derivedClass; \
		\
		CUtlVector<char *> schedulesToLoad; \
		CUtlVector<AIScheduleLoadFunc_t> reqiredOthers; \
		CAI_NamespaceInfos scheduleIds; \
		CAI_NamespaceInfos taskIds; \
		CAI_NamespaceInfos conditionIds;
		

//-----------------

#define AI_BEGIN_CUSTOM_NPC( className, derivedClass ) \
	IMPLEMENT_CUSTOM_AI(className, derivedClass ) \
	void derivedClass::InitCustomSchedules( void ) \
	{ \
		typedef derivedClass CNpc; \
		const char *pszClassName = #derivedClass; \
		\
		CUtlVector<char *> schedulesToLoad; \
		CUtlVector<AIScheduleLoadFunc_t> reqiredOthers; \
		CAI_NamespaceInfos scheduleIds; \
		CAI_NamespaceInfos taskIds; \
		CAI_NamespaceInfos conditionIds; \
		CAI_NamespaceInfos squadSlotIds;
		



//-------------------------------------

// IDs are stored and then added in order due to constraints in the namespace implementation
#define AI_END_CUSTOM_NPC() \
		\
		int i; \
		\
		CNpc::AccessClassScheduleIdSpaceDirect().Init( pszClassName, BaseClass::GetSchedulingSymbols(), &BaseClass::AccessClassScheduleIdSpaceDirect() ); \
		CNpc::gm_SquadSlotIdSpace.Init( &BaseClass::gm_SquadSlotNamespace, &BaseClass::gm_SquadSlotIdSpace); \
		\
		scheduleIds.Sort(); \
		taskIds.Sort(); \
		conditionIds.Sort(); \
		squadSlotIds.Sort(); \
		\
		for ( i = 0; i < scheduleIds.Count(); i++ ) \
		{ \
			ADD_CUSTOM_SCHEDULE_NAMED( CNpc, scheduleIds[i].pszName, scheduleIds[i].localId );  \
		} \
		\
		for ( i = 0; i < taskIds.Count(); i++ ) \
		{ \
			ADD_CUSTOM_TASK_NAMED( CNpc, taskIds[i].pszName, taskIds[i].localId );  \
		} \
		\
		for ( i = 0; i < conditionIds.Count(); i++ ) \
		{ \
			if ( ValidateConditionLimits( conditionIds[i].pszName ) ) \
			{ \
				ADD_CUSTOM_CONDITION_NAMED( CNpc, conditionIds[i].pszName, conditionIds[i].localId );  \
			} \
		} \
		\
		for ( i = 0; i < squadSlotIds.Count(); i++ ) \
		{ \
			ADD_CUSTOM_SQUADSLOT_NAMED( CNpc, squadSlotIds[i].pszName, squadSlotIds[i].localId );  \
		} \
		\
		for ( i = 0; i < reqiredOthers.Count(); i++ ) \
		{ \
			(*reqiredOthers[i])();  \
		} \
		\
		for ( i = 0; i < schedulesToLoad.Count(); i++ ) \
		{ \
			if ( CNpc::gm_SchedLoadStatus.fValid ) \
			{ \
				CNpc::gm_SchedLoadStatus.fValid = g_AI_SchedulesManager.LoadSchedulesFromBuffer( pszClassName, schedulesToLoad[i], &AccessClassScheduleIdSpaceDirect() ); \
			} \
			else \
				break; \
		} \
	}

//-------------------------------------


// IDs are stored and then added in order due to constraints in the namespace implementation
#define AI_END_CUSTOM_SCHEDULE_PROVIDER() \
		\
		int i; \
		\
		CNpc::AccessClassScheduleIdSpaceDirect().Init( pszClassName, BaseClass::GetSchedulingSymbols(), &BaseClass::AccessClassScheduleIdSpaceDirect() ); \
		\
		scheduleIds.Sort(); \
		taskIds.Sort(); \
		conditionIds.Sort(); \
		\
		for ( i = 0; i < scheduleIds.Count(); i++ ) \
		{ \
			ADD_CUSTOM_SCHEDULE_NAMED( CNpc, scheduleIds[i].pszName, scheduleIds[i].localId );  \
		} \
		\
		for ( i = 0; i < taskIds.Count(); i++ ) \
		{ \
			ADD_CUSTOM_TASK_NAMED( CNpc, taskIds[i].pszName, taskIds[i].localId );  \
		} \
		\
		for ( i = 0; i < conditionIds.Count(); i++ ) \
		{ \
			if ( ValidateConditionLimits( conditionIds[i].pszName ) ) \
			{ \
				ADD_CUSTOM_CONDITION_NAMED( CNpc, conditionIds[i].pszName, conditionIds[i].localId );  \
			} \
		} \
		\
		for ( i = 0; i < reqiredOthers.Count(); i++ ) \
		{ \
			(*reqiredOthers[i])();  \
		} \
		\
		for ( i = 0; i < schedulesToLoad.Count(); i++ ) \
		{ \
			if ( CNpc::gm_SchedLoadStatus.fValid ) \
			{ \
				CNpc::gm_SchedLoadStatus.fValid = g_AI_SchedulesManager.LoadSchedulesFromBuffer( pszClassName, schedulesToLoad[i], &AccessClassScheduleIdSpaceDirect() ); \
			} \
			else \
				break; \
		} \
	}


inline bool ValidateConditionLimits( const char *pszNewCondition )
{
	int nGlobalConditions = CAI_NPC::GetSchedulingSymbols()->NumConditions();
	if ( nGlobalConditions >= MAX_CONDITIONS )
	{ 
		AssertMsg2( 0, "Exceeded max number of conditions (%d), ignoring condition %s\n", MAX_CONDITIONS, pszNewCondition ); 
		DevWarning( "Exceeded max number of conditions (%d), ignoring condition %s\n", MAX_CONDITIONS, pszNewCondition ); 
		return false;
	}
	return true;
}


		
//-----------------

#define EXTERN_SCHEDULE( id ) \
	scheduleIds.PushBack( #id, id ); \
	extern const char * g_psz##id; \
	schedulesToLoad.AddToTail( (char *)g_psz##id );

//-----------------

#define DEFINE_SCHEDULE( id, text ) \
	scheduleIds.PushBack( #id, id ); \
	const char * g_psz##id = \
			"\n	Schedule" \
			"\n		" #id \
			text \
			"\n"; \
	schedulesToLoad.AddToTail( (char *)g_psz##id );
	
//-----------------

#define DECLARE_CONDITION( id ) \
	conditionIds.PushBack( #id, id );

//-----------------

#define DECLARE_TASK( id ) \
	taskIds.PushBack( #id, id );

//-----------------

#define DECLARE_ACTIVITY( id ) \
	ADD_CUSTOM_ACTIVITY( CNpc, id );

//-----------------

#define DECLARE_SQUADSLOT( id ) \
	squadSlotIds.PushBack( #id, id );

//-----------------

#define DECLARE_INTERACTION( interaction ) \
	ADD_CUSTOM_INTERACTION( interaction );

//-----------------

#define DECLARE_ANIMEVENT( id ) \
	ADD_CUSTOM_ANIMEVENT( CNpc, id );

//-----------------

#define DECLARE_USES_SCHEDULE_PROVIDER( classname )	reqiredOthers.AddToTail( ScheduleLoadHelper(classname) );

//-----------------


// ============================================================================
abstract_class INPCInteractive
{
public:
	virtual bool	CanInteractWith( CAI_NPC *pUser ) = 0;
	virtual	bool	HasBeenInteractedWith()	= 0;
	virtual void	NotifyInteraction( CAI_NPC *pUser ) = 0;

	// Alyx specific interactions
	virtual void	AlyxStartedInteraction( void ) = 0;
	virtual void	AlyxFinishedInteraction( void ) = 0;
};

#define	DEFINE_BASENPCINTERACTABLE_DATADESC() \
	DEFINE_OUTPUT( m_OnAlyxStartedInteraction,				"OnAlyxStartedInteraction" ),	\
	DEFINE_OUTPUT( m_OnAlyxFinishedInteraction,				"OnAlyxFinishedInteraction" ),  \
	DEFINE_INPUTFUNC( FIELD_VOID, "InteractivePowerDown", InputPowerdown )


template <class NPC_CLASS>
class CNPCBaseInteractive : public NPC_CLASS, public INPCInteractive
{
public:
	CE_DECLARE_CLASS( CNPCBaseInteractive, NPC_CLASS );
public:
	virtual bool	CanInteractWith( CAI_NPC *pUser ) { return false; };
	virtual	bool	HasBeenInteractedWith()	{ return false; };
	virtual void	NotifyInteraction( CAI_NPC *pUser ) { return; };

	virtual void	InputPowerdown( inputdata_t &inputdata )
	{

	}

	// Alyx specific interactions
	virtual void	AlyxStartedInteraction( void )
	{
		m_OnAlyxStartedInteraction.FireOutput( this, this );
	}
	virtual void	AlyxFinishedInteraction( void )
	{
		m_OnAlyxFinishedInteraction.FireOutput( this, this );
	}

public:
	// Outputs
	// Alyx specific interactions
	COutputEvent	m_OnAlyxStartedInteraction;
	COutputEvent	m_OnAlyxFinishedInteraction;
};


#endif

