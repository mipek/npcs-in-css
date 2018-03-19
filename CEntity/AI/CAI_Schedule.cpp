#include "extension.h"
#include "CAI_NPC.h"
#include "CAI_schedule.h"
#include "ai_squadslot.h"
#include "stringregistry.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CAI_SchedulesManager g_AI_SchedulesManager;

//-----------------------------------------------------------------------------
// Purpose: Read data on schedules
//			As I'm parsing a human generated file, give a lot of error output
// Output:  true  - if data successfully read
//			false - if data load fails
//-----------------------------------------------------------------------------
bool CAI_SchedulesManager::LoadSchedulesFromBuffer( const char *prefix, char *pStartFile, CAI_ClassScheduleIdSpace *pIdSpace )
{
	char token[1024];
	char save_token[1024];
	const char *pfile = engine->ParseFile(pStartFile, token, sizeof( token ) );

	while (!stricmp("Schedule",token))
	{
		pfile = engine->ParseFile(pfile, token, sizeof( token ) );

		// -----------------------------
		// Check for duplicate schedule
		// -----------------------------
		if (GetScheduleByName(token))
		{
			DevMsg("ERROR: file contains a schedule (%s) that has already been defined!\n",token);
			DevMsg("       Aborting schedule load.\n");
			Assert(0);
			return false;
		}

		int scheduleID = CAI_NPC::GetScheduleID(token);
		if (scheduleID == -1)
		{
			DevMsg( "ERROR: LoadSchd (%s): Unknown schedule type (%s)\n", prefix, token);
			// FIXME: .sch's not being in code/perforce makes it hard to share branches between developers
			// for now, just stop processing this entities schedules if one is found that isn't in the schedule registry
			break;
			// return false;
		}

		CAI_Schedule *new_schedule = CreateSchedule(token,scheduleID);

		pfile = engine->ParseFile(pfile, token, sizeof( token ) );
		if (stricmp(token,"Tasks"))
		{
			DevMsg( "ERROR: LoadSchd (%s): (%s) Malformed AI Schedule.  Expecting 'Tasks' keyword.\n",prefix,new_schedule->GetName());
			Assert(0);
			return false;
		}

		// ==========================
		// Now read in the tasks
		// ==========================
		// Store in temp array until number of tasks is known
		Task_t tempTask[50];
		int	   taskNum = 0;

		pfile = engine->ParseFile(pfile, token, sizeof( token ) );
		while ((token[0] != '\0') && (stricmp("Interrupts", token)))
		{
			// Convert generic ID to sub-class specific enum
			int taskID = CAI_NPC::GetTaskID(token);
			tempTask[taskNum].iTask = (pIdSpace) ? pIdSpace->TaskGlobalToLocal(taskID) : AI_RemapFromGlobal( taskID );
			
			// If not a valid condition, send a warning message
			if (tempTask[taskNum].iTask == -1)
			{
				DevMsg( "ERROR: LoadSchd (%s): (%s) Unknown task %s!\n", prefix,new_schedule->GetName(), token);
				Assert(0);
				return false;
			}

			Assert( AI_IdIsLocal( tempTask[taskNum].iTask ) );

			// Read in the task argument
			pfile = engine->ParseFile(pfile, token, sizeof( token ) );

			if (!stricmp("Activity",token))
			{
				// Skip the ";", but make sure it's present
				pfile = engine->ParseFile(pfile, token, sizeof( token ) );
				if (stricmp(token,":"))
				{
					DevMsg( "ERROR: LoadSchd (%s): (%s) Malformed AI Schedule.  Expecting ':' after type 'ACTIVITY.\n",prefix,new_schedule->GetName());
					Assert(0);
					return false;
				}

				// Load the activity and make sure its valid
				pfile = engine->ParseFile(pfile, token, sizeof( token ) );
				tempTask[taskNum].flTaskData = CAI_NPC::GetActivityID(token);
				if (tempTask[taskNum].flTaskData == -1)
				{
					DevMsg( "ERROR: LoadSchd (%s): (%s) Unknown activity %s!\n", prefix,new_schedule->GetName(), token);
					Assert(0);
					return false;
				}
			}
			else if (!stricmp("Task",token))
			{
				// Skip the ";", but make sure it's present
				pfile = engine->ParseFile(pfile, token, sizeof( token ) );
				if (stricmp(token,":"))
				{
					DevMsg( "ERROR: LoadSchd (%s): (%s) Malformed AI Schedule.  Expecting ':' after type 'ACTIVITY.\n",prefix,new_schedule->GetName());
					Assert(0);
					return false;
				}

				// Load the activity and make sure its valid
				pfile = engine->ParseFile(pfile, token, sizeof( token ) );

				// Convert generic ID to sub-class specific enum
				int taskID = CAI_NPC::GetTaskID(token);
				tempTask[taskNum].flTaskData = (pIdSpace) ? pIdSpace->TaskGlobalToLocal(taskID) : AI_RemapFromGlobal( taskID );

				if (tempTask[taskNum].flTaskData == -1)
				{
					DevMsg( "ERROR: LoadSchd (%s): (%s) Unknown task %s!\n", prefix,new_schedule->GetName(), token);
					Assert(0);
					return false;
				}
			}
			else if (!stricmp("Schedule",token))
			{
				// Skip the ";", but make sure it's present
				pfile = engine->ParseFile(pfile, token, sizeof( token ) );
				if (stricmp(token,":"))
				{
					DevMsg( "ERROR: LoadSchd (%s): (%s) Malformed AI Schedule.  Expecting ':' after type 'ACTIVITY.\n",prefix,new_schedule->GetName());
					Assert(0);
					return false;
				}

				// Load the schedule and make sure its valid
				pfile = engine->ParseFile(pfile, token, sizeof( token ) );

				// Convert generic ID to sub-class specific enum
				int schedID = CAI_NPC::GetScheduleID(token);
				tempTask[taskNum].flTaskData = (pIdSpace) ? pIdSpace->ScheduleGlobalToLocal(schedID) : AI_RemapFromGlobal( schedID );

				if (tempTask[taskNum].flTaskData == -1)
				{
					DevMsg( "ERROR: LoadSchd %d (%s): (%s) Unknown shedule %s!\n", __LINE__, prefix,new_schedule->GetName(), token);
					Assert(0);
					return false;
				}
			}
			else if (!stricmp("State",token))
			{
				// Skip the ";", but make sure it's present
				pfile = engine->ParseFile(pfile, token, sizeof( token ) );
				if (stricmp(token,":"))
				{
					DevMsg( "ERROR: LoadSchd (%s): (%s) Malformed AI Schedule.  Expecting ':' after type 'STATE.\n",prefix,new_schedule->GetName());
					Assert(0);
					return false;
				}

				// Load the activity and make sure its valid
				pfile = engine->ParseFile(pfile, token, sizeof( token ) );
				tempTask[taskNum].flTaskData = CAI_SchedulesManager::GetStateID(token);
				if (tempTask[taskNum].flTaskData == -1)
				{
					DevMsg( "ERROR: LoadSchd %d (%s): (%s) Unknown shedule %s!\n", __LINE__, prefix,new_schedule->GetName(), token);
					Assert(0);
					return false;
				}
			}
			else if (!stricmp("Memory",token))
			{

				// Skip the ";", but make sure it's present
				pfile = engine->ParseFile(pfile, token, sizeof( token ) );
				if (stricmp(token,":"))
				{
					DevMsg( "ERROR: LoadSchd (%s): (%s) Malformed AI Schedule.  Expecting ':' after type 'STATE.\n",prefix,new_schedule->GetName());
					Assert(0);
					return false;
				}

				// Load the activity and make sure its valid
				pfile = engine->ParseFile(pfile, token, sizeof( token ) );
				tempTask[taskNum].flTaskData = CAI_SchedulesManager::GetMemoryID(token);
				if (tempTask[taskNum].flTaskData == -1)
				{
					DevMsg( "ERROR: LoadSchd %d (%s): (%s) Unknown shedule %s!\n", __LINE__, prefix,new_schedule->GetName(), token);
					Assert(0);
					return false;
				}
			}
			else if (!stricmp("Path",token))
			{
				// Skip the ";", but make sure it's present
				pfile = engine->ParseFile(pfile, token, sizeof( token ) );
				if (stricmp(token,":"))
				{
					DevMsg( "ERROR: LoadSchd (%s): (%s) Malformed AI Schedule.  Expecting ':' after type 'PATH.\n",prefix,new_schedule->GetName());
					Assert(0);
					return false;
				}

				// Load the activity and make sure its valid
				pfile = engine->ParseFile(pfile, token, sizeof( token ) );
				tempTask[taskNum].flTaskData = CAI_SchedulesManager::GetPathID( token );
				if (tempTask[taskNum].flTaskData == -1)
				{
					DevMsg( "ERROR: LoadSchd (%s): (%s) Unknown path type %s!\n", prefix,new_schedule->GetName(), token);
					Assert(0);
					return false;
				}
			}
			else if (!stricmp("Goal",token))
			{
				// Skip the ";", but make sure it's present
				pfile = engine->ParseFile(pfile, token, sizeof( token ) );
				if (stricmp(token,":"))
				{
					DevMsg( "ERROR: LoadSchd (%s): (%s) Malformed AI Schedule.  Expecting ':' after type 'GOAL.\n",prefix,new_schedule->GetName());
					Assert(0);
					return false;
				}

				// Load the activity and make sure its valid
				pfile = engine->ParseFile(pfile, token, sizeof( token ) );
				tempTask[taskNum].flTaskData = CAI_SchedulesManager::GetGoalID( token );
				if (tempTask[taskNum].flTaskData == -1)
				{
					DevMsg( "ERROR: LoadSchd (%s): (%s) Unknown goal type  %s!\n", prefix,new_schedule->GetName(), token);
					Assert(0);
					return false;
				}
			}
			else if ( !stricmp( "HintFlags",token ) )
			{
				// Skip the ":", but make sure it's present
				pfile = engine->ParseFile(pfile, token, sizeof( token ) );
				if (stricmp(token,":"))
				{
					DevMsg( "ERROR: LoadSchd (%s): (%s) Malformed AI Schedule.  Expecting ':' after type 'HINTFLAG'\n",prefix,new_schedule->GetName());
					Assert(0);
					return false;
				}

				// Load the flags and make sure they are valid
				pfile = engine->ParseFile( pfile, token, sizeof( token ) );
				tempTask[taskNum].flTaskData = CAI_HintManager::GetFlags( token );
				if (tempTask[taskNum].flTaskData == -1)
				{
					DevMsg( "ERROR: LoadSchd (%s): (%s) Unknown hint flag type  %s!\n", prefix,new_schedule->GetName(), token);
					Assert(0);
					return false;
				}
			}
			else if (!stricmp("Interrupts",token) || !strnicmp("TASK_",token,5) )
			{
				// a parse error.  Interrupts is the next section, TASK_ is probably the next task, missing task argument?
				Warning( "ERROR: LoadSchd (%s): (%s) Bad syntax at task #%d (wasn't expecting %s)\n", prefix, new_schedule->GetName(), taskNum, token);
				Assert(0);
				return false;
			}
			else
			{
				tempTask[taskNum].flTaskData = atof(token);
			}
			taskNum++;

			// Read the next token
			Q_strncpy(save_token,token,sizeof(save_token));
			pfile = engine->ParseFile(pfile, token, sizeof( token ) );

			// Check for malformed task argument type
			if (!stricmp(token,":"))
			{
				DevMsg( "ERROR: LoadSchd (%s): Schedule (%s),\n        Task (%s), has a malformed AI Task Argument = (%s)\n",
						prefix,new_schedule->GetName(),taskID,save_token);
				Assert(0);
				return false;
			}
		}

		// Now copy the tasks into the new schedule
		new_schedule->m_iNumTasks = taskNum;
		new_schedule->m_pTaskList = new Task_t[taskNum];
		for (int i=0;i<taskNum;i++)
		{
			new_schedule->m_pTaskList[i].iTask		= tempTask[i].iTask;
			new_schedule->m_pTaskList[i].flTaskData = tempTask[i].flTaskData;

			Assert( AI_IdIsLocal( new_schedule->m_pTaskList[i].iTask ) );
		}

		// ==========================
		// Now read in the interrupts
		// ==========================
		pfile = engine->ParseFile(pfile, token, sizeof( token ) );
		while ((token[0] != '\0') && (stricmp("Schedule", token)))
		{
			// Convert generic ID to sub-class specific enum
			int condID = CAI_NPC::GetConditionID(token);

			// If not a valid condition, send a warning message
			if (condID == -1)
			{
				DevMsg( "ERROR: LoadSchd (%s): Schedule (%s), Unknown condition %s!\n", prefix,new_schedule->GetName(),token);
				Assert(0);
			}

			// Otherwise, add to this schedules list of conditions
			else
			{
				int interrupt = AI_RemapFromGlobal(condID);
				Assert( AI_IdIsGlobal( condID ) && interrupt >= 0 && interrupt < MAX_CONDITIONS );
				new_schedule->m_InterruptMask.Set(interrupt);
			}

			// Read the next token
			pfile = engine->ParseFile(pfile, token, sizeof( token ) );
		}
	}
	return true;
}



//-----------------------------------------------------------------------------
// Purpose: Creates and returns schedule of the given name
//			This should eventually be replaced when we convert to
//			non-hard coded schedules
// Input  :
// Output :
//-----------------------------------------------------------------------------
CAI_Schedule *CAI_SchedulesManager::CreateSchedule(char *name, int schedule_id)
{
	// Allocate schedule
	CAI_Schedule *pSched = new CAI_Schedule(name,schedule_id,CAI_SchedulesManager::allSchedules);
	CAI_SchedulesManager::allSchedules = pSched;

	// Return schedule
	return pSched;
}




//-----------------------------------------------------------------------------
// Purpose:
// Input  : *token -
// Output : int
//-----------------------------------------------------------------------------
int CAI_SchedulesManager::GetGoalID( const char *token )
{
	if		( !stricmp( token, "ENEMY" ) )			{	return GOAL_ENEMY;			}
	else if ( !stricmp( token, "ENEMY_LKP" ) )		{	return GOAL_ENEMY_LKP;		}
	else if ( !stricmp( token, "TARGET" ) )			{	return GOAL_TARGET;			}
	else if ( !stricmp( token, "SAVED_POSITION" ) )	{	return GOAL_SAVED_POSITION;	}

	return -1;
}


//-----------------------------------------------------------------------------
// Purpose:
// Input  : *token -
// Output : int
//-----------------------------------------------------------------------------
int CAI_SchedulesManager::GetPathID( const char *token )
{
	if		( !stricmp( token, "TRAVEL" ) )	{	return PATH_TRAVEL;		}
	else if ( !stricmp( token, "LOS" ) )		{	return PATH_LOS;		}
//	else if ( !stricmp( token, "FLANK" ) )		{	return PATH_FLANK;		}
//	else if ( !stricmp( token, "FLANK_LOS" ) )	{	return PATH_FLANK_LOS;	}
	else if ( !stricmp( token, "COVER" ) )		{	return PATH_COVER;		}
//	else if ( !stricmp( token, "COVER_LOS" ) )	{	return PATH_COVER_LOS;	}

	return -1;
}


//-----------------------------------------------------------------------------
// Purpose: Given text name of a memory bit returns its ID number
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CAI_SchedulesManager::GetMemoryID(const char *state_name)
{
	if		(!stricmp(state_name,"PROVOKED"))		{	return bits_MEMORY_PROVOKED;		}
	else if (!stricmp(state_name,"INCOVER"))		{	return bits_MEMORY_INCOVER;			}
	else if (!stricmp(state_name,"SUSPICIOUS"))		{	return bits_MEMORY_SUSPICIOUS;		}
	else if (!stricmp(state_name,"PATH_FAILED"))	{	return bits_MEMORY_PATH_FAILED;		}
	else if (!stricmp(state_name,"FLINCHED"))		{	return bits_MEMORY_FLINCHED;		}
	else if (!stricmp(state_name,"TOURGUIDE"))		{	return bits_MEMORY_TOURGUIDE;		}
	else if (!stricmp(state_name,"LOCKED_HINT"))	{	return bits_MEMORY_LOCKED_HINT;		}
	else if (!stricmp(state_name,"TURNING"))		{	return bits_MEMORY_TURNING;			}
	else if (!stricmp(state_name,"TURNHACK"))		{	return bits_MEMORY_TURNHACK;		}
	else if (!stricmp(state_name,"CUSTOM4"))		{	return bits_MEMORY_CUSTOM4;			}
	else if (!stricmp(state_name,"CUSTOM3"))		{	return bits_MEMORY_CUSTOM3;			}
	else if (!stricmp(state_name,"CUSTOM2"))		{	return bits_MEMORY_CUSTOM2;			}
	else if (!stricmp(state_name,"CUSTOM1"))		{	return bits_MEMORY_CUSTOM1;			}
	else												return -1;
}


//-----------------------------------------------------------------------------
// Purpose: Given text name of a NPC state returns its ID number
// Input  :
// Output :
//-----------------------------------------------------------------------------
int CAI_SchedulesManager::GetStateID(const char *state_name)
{
	if		(!stricmp(state_name,"NONE"))		{	return NPC_STATE_NONE;		}
	else if (!stricmp(state_name,"IDLE"))		{	return NPC_STATE_IDLE;		}
	else if (!stricmp(state_name,"COMBAT"))		{	return NPC_STATE_COMBAT;	}
	else if (!stricmp(state_name,"PRONE"))		{	return NPC_STATE_PRONE;		}
	else if (!stricmp(state_name,"ALERT"))		{	return NPC_STATE_ALERT;		}
	else if (!stricmp(state_name,"SCRIPT"))		{	return NPC_STATE_SCRIPT;	}
	else if (!stricmp(state_name,"PLAYDEAD"))	{	return NPC_STATE_PLAYDEAD;	}
	else if (!stricmp(state_name,"DEAD"))		{	return NPC_STATE_DEAD;		}
	else											return -1;
}

//-----------------------------------------------------------------------------
// Purpose: Given a schedule name, returns a schedule of the given type
//-----------------------------------------------------------------------------
CAI_Schedule *CAI_SchedulesManager::GetScheduleByName( const char *name )
{
	for ( CAI_Schedule *schedule = CAI_SchedulesManager::allSchedules; schedule != NULL; schedule = schedule->nextSchedule )
	{
		if (FStrEq(schedule->GetName(),name))
			return schedule;
	}

	return NULL;
}



//-----------------------------------------------------------------------------
// Purpose: Given a schedule ID, returns a schedule of the given type
//-----------------------------------------------------------------------------
CAI_Schedule *CAI_SchedulesManager::GetScheduleFromID( int schedID )
{
	for ( CAI_Schedule *schedule = CAI_SchedulesManager::allSchedules; schedule != NULL; schedule = schedule->nextSchedule )
	{
		if (schedule->m_iScheduleID == schedID)
			return schedule;
	}

//	DevMsg( "Couldn't find schedule (%s)\n", CAI_BaseNPC::GetSchedulingSymbols()->ScheduleIdToSymbol(schedID) );

	return NULL;
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------

CAI_Schedule::CAI_Schedule(char *name, int schedule_id, CAI_Schedule *pNext)
{
	m_iScheduleID = schedule_id;

	int len = strlen(name);
	m_pName = new char[len+1];
	Q_strncpy(m_pName,name,len+1);

	m_pTaskList = NULL;
	m_iNumTasks = 0;

	// ---------------------------------
	//  Add to linked list of schedules
	// ---------------------------------
	nextSchedule = pNext;
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
CAI_Schedule::~CAI_Schedule( void )
{
	delete[] m_pName;
	delete[] m_pTaskList;
}



// CAI_NPC

void CAI_NPC::InitSchedulingTables()
{
	CAI_NPC::gm_ClassScheduleIdSpace.Init( "CAI_BaseNPC", CAI_NPC::GetSchedulingSymbols() );
	CAI_NPC::InitDefaultScheduleSR();
	CAI_NPC::InitDefaultConditionSR();
	CAI_NPC::InitDefaultTaskSR();
	CAI_NPC::InitDefaultActivitySR();
	CAI_NPC::InitDefaultSquadSlotSR();
}

void CAI_NPC::InitDefaultScheduleSR(void)
{
	#define ADD_DEF_SCHEDULE( name, localId ) idSpace.AddSchedule(name, localId, "CAI_BaseNPC" )

	CAI_ClassScheduleIdSpace &idSpace = CAI_NPC::AccessClassScheduleIdSpaceDirect();

	ADD_DEF_SCHEDULE( "SCHED_NONE",							SCHED_NONE);
	ADD_DEF_SCHEDULE( "SCHED_IDLE_STAND",					SCHED_IDLE_STAND);
	ADD_DEF_SCHEDULE( "SCHED_IDLE_WALK",					SCHED_IDLE_WALK);
	ADD_DEF_SCHEDULE( "SCHED_IDLE_WANDER",					SCHED_IDLE_WANDER);
	ADD_DEF_SCHEDULE( "SCHED_WAKE_ANGRY",					SCHED_WAKE_ANGRY);
	ADD_DEF_SCHEDULE( "SCHED_ALERT_FACE",					SCHED_ALERT_FACE);
	ADD_DEF_SCHEDULE( "SCHED_ALERT_FACE_BESTSOUND",			SCHED_ALERT_FACE_BESTSOUND);
	ADD_DEF_SCHEDULE( "SCHED_ALERT_REACT_TO_COMBAT_SOUND",	SCHED_ALERT_REACT_TO_COMBAT_SOUND);
	ADD_DEF_SCHEDULE( "SCHED_ALERT_SCAN",					SCHED_ALERT_SCAN);
	ADD_DEF_SCHEDULE( "SCHED_ALERT_STAND",					SCHED_ALERT_STAND);
	ADD_DEF_SCHEDULE( "SCHED_ALERT_WALK",					SCHED_ALERT_WALK);
	ADD_DEF_SCHEDULE( "SCHED_INVESTIGATE_SOUND",			SCHED_INVESTIGATE_SOUND);
	ADD_DEF_SCHEDULE( "SCHED_COMBAT_FACE",					SCHED_COMBAT_FACE);
	ADD_DEF_SCHEDULE( "SCHED_COMBAT_SWEEP",					SCHED_COMBAT_SWEEP);
	ADD_DEF_SCHEDULE( "SCHED_FEAR_FACE",					SCHED_FEAR_FACE);
	ADD_DEF_SCHEDULE( "SCHED_COMBAT_STAND",					SCHED_COMBAT_STAND);
	ADD_DEF_SCHEDULE( "SCHED_COMBAT_WALK",					SCHED_COMBAT_WALK);
	ADD_DEF_SCHEDULE( "SCHED_CHASE_ENEMY",					SCHED_CHASE_ENEMY);
	ADD_DEF_SCHEDULE( "SCHED_CHASE_ENEMY_FAILED",			SCHED_CHASE_ENEMY_FAILED);
	ADD_DEF_SCHEDULE( "SCHED_VICTORY_DANCE",				SCHED_VICTORY_DANCE);
	ADD_DEF_SCHEDULE( "SCHED_TARGET_FACE",					SCHED_TARGET_FACE);
	ADD_DEF_SCHEDULE( "SCHED_TARGET_CHASE",					SCHED_TARGET_CHASE);
	ADD_DEF_SCHEDULE( "SCHED_SMALL_FLINCH",					SCHED_SMALL_FLINCH);	
	ADD_DEF_SCHEDULE( "SCHED_BIG_FLINCH",					SCHED_BIG_FLINCH);	
	ADD_DEF_SCHEDULE( "SCHED_BACK_AWAY_FROM_ENEMY",			SCHED_BACK_AWAY_FROM_ENEMY);
	ADD_DEF_SCHEDULE( "SCHED_MOVE_AWAY_FROM_ENEMY",			SCHED_MOVE_AWAY_FROM_ENEMY);
	ADD_DEF_SCHEDULE( "SCHED_BACK_AWAY_FROM_SAVE_POSITION",	SCHED_BACK_AWAY_FROM_SAVE_POSITION);
	ADD_DEF_SCHEDULE( "SCHED_TAKE_COVER_FROM_ENEMY",		SCHED_TAKE_COVER_FROM_ENEMY);
	ADD_DEF_SCHEDULE( "SCHED_TAKE_COVER_FROM_BEST_SOUND",	SCHED_TAKE_COVER_FROM_BEST_SOUND);
	ADD_DEF_SCHEDULE( "SCHED_FLEE_FROM_BEST_SOUND",			SCHED_FLEE_FROM_BEST_SOUND);
	ADD_DEF_SCHEDULE( "SCHED_TAKE_COVER_FROM_ORIGIN",		SCHED_TAKE_COVER_FROM_ORIGIN);
	ADD_DEF_SCHEDULE( "SCHED_FAIL_TAKE_COVER",				SCHED_FAIL_TAKE_COVER);
	ADD_DEF_SCHEDULE( "SCHED_RUN_FROM_ENEMY",				SCHED_RUN_FROM_ENEMY);
	ADD_DEF_SCHEDULE( "SCHED_RUN_FROM_ENEMY_FALLBACK",		SCHED_RUN_FROM_ENEMY_FALLBACK);
	ADD_DEF_SCHEDULE( "SCHED_MOVE_TO_WEAPON_RANGE",			SCHED_MOVE_TO_WEAPON_RANGE);
	ADD_DEF_SCHEDULE( "SCHED_ESTABLISH_LINE_OF_FIRE",		SCHED_ESTABLISH_LINE_OF_FIRE);
	ADD_DEF_SCHEDULE( "SCHED_SHOOT_ENEMY_COVER",			SCHED_SHOOT_ENEMY_COVER);
	ADD_DEF_SCHEDULE( "SCHED_ESTABLISH_LINE_OF_FIRE_FALLBACK",		SCHED_ESTABLISH_LINE_OF_FIRE_FALLBACK);
	ADD_DEF_SCHEDULE( "SCHED_PRE_FAIL_ESTABLISH_LINE_OF_FIRE", SCHED_PRE_FAIL_ESTABLISH_LINE_OF_FIRE);
	ADD_DEF_SCHEDULE( "SCHED_FAIL_ESTABLISH_LINE_OF_FIRE",	SCHED_FAIL_ESTABLISH_LINE_OF_FIRE);
	ADD_DEF_SCHEDULE( "SCHED_COWER",						SCHED_COWER);
	ADD_DEF_SCHEDULE( "SCHED_MELEE_ATTACK1",				SCHED_MELEE_ATTACK1);
	ADD_DEF_SCHEDULE( "SCHED_MELEE_ATTACK2",				SCHED_MELEE_ATTACK2);
	ADD_DEF_SCHEDULE( "SCHED_RANGE_ATTACK1",				SCHED_RANGE_ATTACK1);
	ADD_DEF_SCHEDULE( "SCHED_RANGE_ATTACK2",				SCHED_RANGE_ATTACK2);
	ADD_DEF_SCHEDULE( "SCHED_SPECIAL_ATTACK1",				SCHED_SPECIAL_ATTACK1);
	ADD_DEF_SCHEDULE( "SCHED_SPECIAL_ATTACK2",				SCHED_SPECIAL_ATTACK2);
	ADD_DEF_SCHEDULE( "SCHED_STANDOFF",						SCHED_STANDOFF);
	ADD_DEF_SCHEDULE( "SCHED_ARM_WEAPON",					SCHED_ARM_WEAPON);
	ADD_DEF_SCHEDULE( "SCHED_DISARM_WEAPON",				SCHED_DISARM_WEAPON);
	ADD_DEF_SCHEDULE( "SCHED_HIDE_AND_RELOAD",				SCHED_HIDE_AND_RELOAD);
	ADD_DEF_SCHEDULE( "SCHED_RELOAD",						SCHED_RELOAD);
	ADD_DEF_SCHEDULE( "SCHED_AMBUSH",						SCHED_AMBUSH);
	ADD_DEF_SCHEDULE( "SCHED_DIE",							SCHED_DIE);
	ADD_DEF_SCHEDULE( "SCHED_DIE_RAGDOLL",					SCHED_DIE_RAGDOLL);
	ADD_DEF_SCHEDULE( "SCHED_WAIT_FOR_SCRIPT",				SCHED_WAIT_FOR_SCRIPT);
	ADD_DEF_SCHEDULE( "SCHED_AISCRIPT",						SCHED_AISCRIPT);
	ADD_DEF_SCHEDULE( "SCHED_SCRIPTED_WALK",				SCHED_SCRIPTED_WALK);
	ADD_DEF_SCHEDULE( "SCHED_SCRIPTED_RUN",					SCHED_SCRIPTED_RUN);
	ADD_DEF_SCHEDULE( "SCHED_SCRIPTED_CUSTOM_MOVE",			SCHED_SCRIPTED_CUSTOM_MOVE);
	ADD_DEF_SCHEDULE( "SCHED_SCRIPTED_WAIT",				SCHED_SCRIPTED_WAIT);
	ADD_DEF_SCHEDULE( "SCHED_SCRIPTED_FACE",				SCHED_SCRIPTED_FACE);
	ADD_DEF_SCHEDULE( "SCHED_SCENE_GENERIC",				SCHED_SCENE_GENERIC);
	ADD_DEF_SCHEDULE( "SCHED_NEW_WEAPON",					SCHED_NEW_WEAPON);
	ADD_DEF_SCHEDULE( "SCHED_NEW_WEAPON_CHEAT",				SCHED_NEW_WEAPON_CHEAT);
	ADD_DEF_SCHEDULE( "SCHED_SWITCH_TO_PENDING_WEAPON",		SCHED_SWITCH_TO_PENDING_WEAPON );
	ADD_DEF_SCHEDULE( "SCHED_GET_HEALTHKIT",				SCHED_GET_HEALTHKIT);
	ADD_DEF_SCHEDULE( "SCHED_MOVE_AWAY",					SCHED_MOVE_AWAY);
	ADD_DEF_SCHEDULE( "SCHED_MOVE_AWAY_FAIL",				SCHED_MOVE_AWAY_FAIL);
	ADD_DEF_SCHEDULE( "SCHED_MOVE_AWAY_END",				SCHED_MOVE_AWAY_END);
	ADD_DEF_SCHEDULE( "SCHED_WAIT_FOR_SPEAK_FINISH",		SCHED_WAIT_FOR_SPEAK_FINISH);
	ADD_DEF_SCHEDULE( "SCHED_FORCED_GO",					SCHED_FORCED_GO);
	ADD_DEF_SCHEDULE( "SCHED_FORCED_GO_RUN",				SCHED_FORCED_GO_RUN);
	ADD_DEF_SCHEDULE( "SCHED_PATROL_WALK",					SCHED_PATROL_WALK);
	ADD_DEF_SCHEDULE( "SCHED_COMBAT_PATROL",				SCHED_COMBAT_PATROL);
	ADD_DEF_SCHEDULE( "SCHED_PATROL_RUN",					SCHED_PATROL_RUN);
	ADD_DEF_SCHEDULE( "SCHED_RUN_RANDOM",					SCHED_RUN_RANDOM);
	ADD_DEF_SCHEDULE( "SCHED_FAIL",							SCHED_FAIL);
	ADD_DEF_SCHEDULE( "SCHED_FAIL_NOSTOP",					SCHED_FAIL_NOSTOP);
	ADD_DEF_SCHEDULE( "SCHED_FALL_TO_GROUND",				SCHED_FALL_TO_GROUND);
	ADD_DEF_SCHEDULE( "SCHED_DROPSHIP_DUSTOFF",				SCHED_DROPSHIP_DUSTOFF);
	ADD_DEF_SCHEDULE( "SCHED_NPC_FREEZE",					SCHED_NPC_FREEZE);

	ADD_DEF_SCHEDULE( "SCHED_FLINCH_PHYSICS",			SCHED_FLINCH_PHYSICS);

	ADD_DEF_SCHEDULE( "SCHED_RUN_FROM_ENEMY_MOB",		SCHED_RUN_FROM_ENEMY_MOB );

	ADD_DEF_SCHEDULE( "SCHED_DUCK_DODGE",				SCHED_DUCK_DODGE );

	ADD_DEF_SCHEDULE( "SCHED_INTERACTION_MOVE_TO_PARTNER",				SCHED_INTERACTION_MOVE_TO_PARTNER );
	ADD_DEF_SCHEDULE( "SCHED_INTERACTION_WAIT_FOR_PARTNER",				SCHED_INTERACTION_WAIT_FOR_PARTNER );

	ADD_DEF_SCHEDULE( "SCHED_SLEEP",					SCHED_SLEEP );
}


//-----------------------------------------------------------------------------
// Purpose: Register the default conditions
// Input  :
// Output :
//-----------------------------------------------------------------------------

#define ADD_CONDITION_TO_SR( _n ) idSpace.AddCondition( #_n, _n, "CAI_BaseNPC" )

void	CAI_NPC::InitDefaultConditionSR(void)
{
	CAI_ClassScheduleIdSpace &idSpace = CAI_NPC::AccessClassScheduleIdSpaceDirect();

	ADD_CONDITION_TO_SR( COND_NONE );
	ADD_CONDITION_TO_SR( COND_IN_PVS );
	ADD_CONDITION_TO_SR( COND_IDLE_INTERRUPT );
	ADD_CONDITION_TO_SR( COND_LOW_PRIMARY_AMMO );
	ADD_CONDITION_TO_SR( COND_NO_PRIMARY_AMMO );
	ADD_CONDITION_TO_SR( COND_NO_SECONDARY_AMMO );
	ADD_CONDITION_TO_SR( COND_NO_WEAPON );
	ADD_CONDITION_TO_SR( COND_SEE_HATE );
	ADD_CONDITION_TO_SR( COND_SEE_FEAR );
	ADD_CONDITION_TO_SR( COND_SEE_DISLIKE );
	ADD_CONDITION_TO_SR( COND_SEE_ENEMY );
	ADD_CONDITION_TO_SR( COND_LOST_ENEMY );
	ADD_CONDITION_TO_SR( COND_ENEMY_WENT_NULL );
	ADD_CONDITION_TO_SR( COND_HAVE_ENEMY_LOS );
	ADD_CONDITION_TO_SR( COND_HAVE_TARGET_LOS );
	ADD_CONDITION_TO_SR( COND_ENEMY_OCCLUDED );
	ADD_CONDITION_TO_SR( COND_TARGET_OCCLUDED );
	ADD_CONDITION_TO_SR( COND_ENEMY_TOO_FAR );
	ADD_CONDITION_TO_SR( COND_LIGHT_DAMAGE );
	ADD_CONDITION_TO_SR( COND_HEAVY_DAMAGE );
	ADD_CONDITION_TO_SR( COND_PHYSICS_DAMAGE );
	ADD_CONDITION_TO_SR( COND_REPEATED_DAMAGE );
	ADD_CONDITION_TO_SR( COND_CAN_RANGE_ATTACK1 );
	ADD_CONDITION_TO_SR( COND_CAN_RANGE_ATTACK2 );
	ADD_CONDITION_TO_SR( COND_CAN_MELEE_ATTACK1 );
	ADD_CONDITION_TO_SR( COND_CAN_MELEE_ATTACK2 );
	ADD_CONDITION_TO_SR( COND_PROVOKED );
	ADD_CONDITION_TO_SR( COND_NEW_ENEMY );
	ADD_CONDITION_TO_SR( COND_ENEMY_FACING_ME );
	ADD_CONDITION_TO_SR( COND_BEHIND_ENEMY );
	ADD_CONDITION_TO_SR( COND_ENEMY_DEAD );
	ADD_CONDITION_TO_SR( COND_ENEMY_UNREACHABLE );
	ADD_CONDITION_TO_SR( COND_SEE_PLAYER );
	ADD_CONDITION_TO_SR( COND_LOST_PLAYER );
	ADD_CONDITION_TO_SR( COND_SEE_NEMESIS );
	ADD_CONDITION_TO_SR( COND_TASK_FAILED );
	ADD_CONDITION_TO_SR( COND_SCHEDULE_DONE );
	ADD_CONDITION_TO_SR( COND_SMELL );
	ADD_CONDITION_TO_SR( COND_TOO_CLOSE_TO_ATTACK );
	ADD_CONDITION_TO_SR( COND_TOO_FAR_TO_ATTACK );
	ADD_CONDITION_TO_SR( COND_NOT_FACING_ATTACK );
	ADD_CONDITION_TO_SR( COND_WEAPON_HAS_LOS );
	ADD_CONDITION_TO_SR( COND_WEAPON_BLOCKED_BY_FRIEND );			// Friend between gun and target
	ADD_CONDITION_TO_SR( COND_WEAPON_PLAYER_IN_SPREAD );		// Player in shooting direction
	ADD_CONDITION_TO_SR( COND_WEAPON_PLAYER_NEAR_TARGET );	// Player near shooting position
	ADD_CONDITION_TO_SR( COND_WEAPON_SIGHT_OCCLUDED );
	ADD_CONDITION_TO_SR( COND_BETTER_WEAPON_AVAILABLE );
	ADD_CONDITION_TO_SR( COND_HEALTH_ITEM_AVAILABLE );
	ADD_CONDITION_TO_SR( COND_FLOATING_OFF_GROUND );
	ADD_CONDITION_TO_SR( COND_MOBBED_BY_ENEMIES );
	ADD_CONDITION_TO_SR( COND_GIVE_WAY );
	ADD_CONDITION_TO_SR( COND_WAY_CLEAR );
	ADD_CONDITION_TO_SR( COND_HEAR_DANGER );
	ADD_CONDITION_TO_SR( COND_HEAR_THUMPER );
	ADD_CONDITION_TO_SR( COND_HEAR_COMBAT );
	ADD_CONDITION_TO_SR( COND_HEAR_WORLD );
	ADD_CONDITION_TO_SR( COND_HEAR_PLAYER );
	ADD_CONDITION_TO_SR( COND_HEAR_BULLET_IMPACT );
	ADD_CONDITION_TO_SR( COND_HEAR_BUGBAIT );
	ADD_CONDITION_TO_SR( COND_HEAR_PHYSICS_DANGER );
	ADD_CONDITION_TO_SR( COND_HEAR_MOVE_AWAY );
	ADD_CONDITION_TO_SR( COND_NO_HEAR_DANGER );
	ADD_CONDITION_TO_SR( COND_PLAYER_PUSHING );
	ADD_CONDITION_TO_SR( COND_RECEIVED_ORDERS );
	ADD_CONDITION_TO_SR( COND_PLAYER_ADDED_TO_SQUAD );
	ADD_CONDITION_TO_SR( COND_PLAYER_REMOVED_FROM_SQUAD );
	ADD_CONDITION_TO_SR( COND_NPC_FREEZE );
	ADD_CONDITION_TO_SR( COND_NPC_UNFREEZE );
	ADD_CONDITION_TO_SR( COND_TALKER_RESPOND_TO_QUESTION );
	ADD_CONDITION_TO_SR( COND_NO_CUSTOM_INTERRUPTS );
}


//-----------------------------------------------------------------------------
// Purpose: Initialize the task name string registry
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_NPC::InitDefaultTaskSR(void)
{
	#define ADD_DEF_TASK( name ) idSpace.AddTask(#name, name, "CAI_BaseNPC" )

	CAI_ClassScheduleIdSpace &idSpace = CAI_NPC::AccessClassScheduleIdSpaceDirect();

	ADD_DEF_TASK( TASK_INVALID );
	ADD_DEF_TASK( TASK_ANNOUNCE_ATTACK );
	ADD_DEF_TASK( TASK_RESET_ACTIVITY );
	ADD_DEF_TASK( TASK_WAIT );
	ADD_DEF_TASK( TASK_WAIT_FACE_ENEMY );
	ADD_DEF_TASK( TASK_WAIT_FACE_ENEMY_RANDOM );
	ADD_DEF_TASK( TASK_WAIT_PVS );
	ADD_DEF_TASK( TASK_SUGGEST_STATE );
	ADD_DEF_TASK( TASK_TARGET_PLAYER );
	ADD_DEF_TASK( TASK_SCRIPT_WALK_TO_TARGET );
	ADD_DEF_TASK( TASK_SCRIPT_RUN_TO_TARGET );
	ADD_DEF_TASK( TASK_SCRIPT_CUSTOM_MOVE_TO_TARGET );
	ADD_DEF_TASK( TASK_MOVE_TO_TARGET_RANGE );
	ADD_DEF_TASK( TASK_MOVE_TO_GOAL_RANGE );
	ADD_DEF_TASK( TASK_MOVE_AWAY_PATH );
	ADD_DEF_TASK( TASK_GET_PATH_AWAY_FROM_BEST_SOUND );
	ADD_DEF_TASK( TASK_SET_GOAL );
	ADD_DEF_TASK( TASK_GET_PATH_TO_GOAL );
	ADD_DEF_TASK( TASK_GET_PATH_TO_ENEMY );
	ADD_DEF_TASK( TASK_GET_PATH_TO_ENEMY_LKP );
	ADD_DEF_TASK( TASK_GET_CHASE_PATH_TO_ENEMY );
	ADD_DEF_TASK( TASK_GET_PATH_TO_ENEMY_LKP_LOS );
	ADD_DEF_TASK( TASK_GET_PATH_TO_RANGE_ENEMY_LKP_LOS );
	ADD_DEF_TASK( TASK_GET_PATH_TO_ENEMY_CORPSE );
	ADD_DEF_TASK( TASK_GET_PATH_TO_PLAYER );
	ADD_DEF_TASK( TASK_GET_PATH_TO_ENEMY_LOS );
	ADD_DEF_TASK( TASK_GET_FLANK_ARC_PATH_TO_ENEMY_LOS );
	ADD_DEF_TASK( TASK_GET_FLANK_RADIUS_PATH_TO_ENEMY_LOS );
	ADD_DEF_TASK( TASK_GET_PATH_TO_TARGET );
	ADD_DEF_TASK( TASK_GET_PATH_TO_TARGET_WEAPON );
	ADD_DEF_TASK( TASK_CREATE_PENDING_WEAPON );
	ADD_DEF_TASK( TASK_GET_PATH_TO_HINTNODE );
	ADD_DEF_TASK( TASK_STORE_LASTPOSITION );
	ADD_DEF_TASK( TASK_CLEAR_LASTPOSITION );
	ADD_DEF_TASK( TASK_STORE_POSITION_IN_SAVEPOSITION );
	ADD_DEF_TASK( TASK_STORE_BESTSOUND_IN_SAVEPOSITION );
	ADD_DEF_TASK( TASK_STORE_BESTSOUND_REACTORIGIN_IN_SAVEPOSITION );
	ADD_DEF_TASK( TASK_REACT_TO_COMBAT_SOUND );
	ADD_DEF_TASK( TASK_STORE_ENEMY_POSITION_IN_SAVEPOSITION );
	ADD_DEF_TASK( TASK_GET_PATH_TO_COMMAND_GOAL );
	ADD_DEF_TASK( TASK_MARK_COMMAND_GOAL_POS );
	ADD_DEF_TASK( TASK_CLEAR_COMMAND_GOAL );
	ADD_DEF_TASK( TASK_GET_PATH_TO_LASTPOSITION );
	ADD_DEF_TASK( TASK_GET_PATH_TO_SAVEPOSITION );
	ADD_DEF_TASK( TASK_GET_PATH_TO_SAVEPOSITION_LOS );
	ADD_DEF_TASK( TASK_GET_PATH_TO_BESTSOUND );
	ADD_DEF_TASK( TASK_GET_PATH_TO_BESTSCENT );
	ADD_DEF_TASK( TASK_GET_PATH_TO_RANDOM_NODE );
	ADD_DEF_TASK( TASK_RUN_PATH );
	ADD_DEF_TASK( TASK_WALK_PATH );
	ADD_DEF_TASK( TASK_WALK_PATH_TIMED );
	ADD_DEF_TASK( TASK_WALK_PATH_WITHIN_DIST );
	ADD_DEF_TASK( TASK_RUN_PATH_WITHIN_DIST );
	ADD_DEF_TASK( TASK_WALK_PATH_FOR_UNITS );
	ADD_DEF_TASK( TASK_RUN_PATH_FOR_UNITS );
	ADD_DEF_TASK( TASK_RUN_PATH_FLEE );
	ADD_DEF_TASK( TASK_RUN_PATH_TIMED );
	ADD_DEF_TASK( TASK_STRAFE_PATH );
	ADD_DEF_TASK( TASK_CLEAR_MOVE_WAIT );
	ADD_DEF_TASK( TASK_SMALL_FLINCH );
	ADD_DEF_TASK( TASK_BIG_FLINCH );
	ADD_DEF_TASK( TASK_DEFER_DODGE );
	ADD_DEF_TASK( TASK_FACE_IDEAL );
	ADD_DEF_TASK( TASK_FACE_REASONABLE );
	ADD_DEF_TASK( TASK_FACE_PATH );
	ADD_DEF_TASK( TASK_FACE_PLAYER );
	ADD_DEF_TASK( TASK_FACE_ENEMY );
	ADD_DEF_TASK( TASK_FACE_HINTNODE );
	ADD_DEF_TASK( TASK_PLAY_HINT_ACTIVITY );
	ADD_DEF_TASK( TASK_FACE_TARGET );
	ADD_DEF_TASK( TASK_FACE_LASTPOSITION );
	ADD_DEF_TASK( TASK_FACE_SAVEPOSITION );
	ADD_DEF_TASK( TASK_FACE_AWAY_FROM_SAVEPOSITION );
	ADD_DEF_TASK( TASK_SET_IDEAL_YAW_TO_CURRENT );
	ADD_DEF_TASK( TASK_RANGE_ATTACK1 );
	ADD_DEF_TASK( TASK_RANGE_ATTACK2 );
	ADD_DEF_TASK( TASK_MELEE_ATTACK1 );
	ADD_DEF_TASK( TASK_MELEE_ATTACK2 );
	ADD_DEF_TASK( TASK_RELOAD );
	ADD_DEF_TASK( TASK_SPECIAL_ATTACK1 );
	ADD_DEF_TASK( TASK_SPECIAL_ATTACK2 );
	ADD_DEF_TASK( TASK_FIND_HINTNODE );
	ADD_DEF_TASK( TASK_CLEAR_HINTNODE );
	ADD_DEF_TASK( TASK_FIND_LOCK_HINTNODE );
	ADD_DEF_TASK( TASK_LOCK_HINTNODE );
	ADD_DEF_TASK( TASK_SOUND_ANGRY );
	ADD_DEF_TASK( TASK_SOUND_DEATH );
	ADD_DEF_TASK( TASK_SOUND_IDLE );
	ADD_DEF_TASK( TASK_SOUND_WAKE );
	ADD_DEF_TASK( TASK_SOUND_PAIN );
	ADD_DEF_TASK( TASK_SOUND_DIE );
	ADD_DEF_TASK( TASK_SPEAK_SENTENCE );
	ADD_DEF_TASK( TASK_WAIT_FOR_SPEAK_FINISH );
	ADD_DEF_TASK( TASK_SET_ACTIVITY );
	ADD_DEF_TASK( TASK_RANDOMIZE_FRAMERATE );
	ADD_DEF_TASK( TASK_SET_SCHEDULE );
	ADD_DEF_TASK( TASK_SET_FAIL_SCHEDULE );
	ADD_DEF_TASK( TASK_SET_TOLERANCE_DISTANCE );
	ADD_DEF_TASK( TASK_SET_ROUTE_SEARCH_TIME );
	ADD_DEF_TASK( TASK_CLEAR_FAIL_SCHEDULE );
	ADD_DEF_TASK( TASK_PLAY_SEQUENCE );
	ADD_DEF_TASK( TASK_PLAY_PRIVATE_SEQUENCE );
	ADD_DEF_TASK( TASK_PLAY_PRIVATE_SEQUENCE_FACE_ENEMY );
	ADD_DEF_TASK( TASK_PLAY_SEQUENCE_FACE_ENEMY );
	ADD_DEF_TASK( TASK_PLAY_SEQUENCE_FACE_TARGET );
	ADD_DEF_TASK( TASK_FIND_COVER_FROM_BEST_SOUND );
	ADD_DEF_TASK( TASK_FIND_COVER_FROM_ENEMY );
	ADD_DEF_TASK( TASK_FIND_LATERAL_COVER_FROM_ENEMY );
	ADD_DEF_TASK( TASK_FIND_BACKAWAY_FROM_SAVEPOSITION );
	ADD_DEF_TASK( TASK_FIND_NODE_COVER_FROM_ENEMY );
	ADD_DEF_TASK( TASK_FIND_NEAR_NODE_COVER_FROM_ENEMY );
	ADD_DEF_TASK( TASK_FIND_FAR_NODE_COVER_FROM_ENEMY );
	ADD_DEF_TASK( TASK_FIND_COVER_FROM_ORIGIN );
	ADD_DEF_TASK( TASK_DIE );
	ADD_DEF_TASK( TASK_WAIT_FOR_SCRIPT );
	ADD_DEF_TASK( TASK_PUSH_SCRIPT_ARRIVAL_ACTIVITY );
	ADD_DEF_TASK( TASK_PLAY_SCRIPT );
	ADD_DEF_TASK( TASK_PLAY_SCRIPT_POST_IDLE );
	ADD_DEF_TASK( TASK_ENABLE_SCRIPT );
	ADD_DEF_TASK( TASK_PLANT_ON_SCRIPT );
	ADD_DEF_TASK( TASK_FACE_SCRIPT );
	ADD_DEF_TASK( TASK_PLAY_SCENE );
	ADD_DEF_TASK( TASK_WAIT_RANDOM );
	ADD_DEF_TASK( TASK_WAIT_INDEFINITE );
	ADD_DEF_TASK( TASK_STOP_MOVING );
	ADD_DEF_TASK( TASK_TURN_LEFT );
	ADD_DEF_TASK( TASK_TURN_RIGHT );
	ADD_DEF_TASK( TASK_REMEMBER );
	ADD_DEF_TASK( TASK_FORGET );
	ADD_DEF_TASK( TASK_WAIT_FOR_MOVEMENT );
	ADD_DEF_TASK( TASK_WAIT_FOR_MOVEMENT_STEP );
	ADD_DEF_TASK( TASK_WAIT_UNTIL_NO_DANGER_SOUND );
	ADD_DEF_TASK( TASK_WEAPON_FIND );
	ADD_DEF_TASK( TASK_WEAPON_PICKUP );
	ADD_DEF_TASK( TASK_WEAPON_RUN_PATH );
	ADD_DEF_TASK( TASK_WEAPON_CREATE );
	ADD_DEF_TASK( TASK_ITEM_RUN_PATH );
	ADD_DEF_TASK( TASK_ITEM_PICKUP );
	ADD_DEF_TASK( TASK_USE_SMALL_HULL );
	ADD_DEF_TASK( TASK_FALL_TO_GROUND );
	ADD_DEF_TASK( TASK_WANDER );
	ADD_DEF_TASK( TASK_FREEZE );
	ADD_DEF_TASK( TASK_GATHER_CONDITIONS );
	ADD_DEF_TASK( TASK_IGNORE_OLD_ENEMIES );
	ADD_DEF_TASK( TASK_DEBUG_BREAK );
	ADD_DEF_TASK( TASK_ADD_HEALTH );
	ADD_DEF_TASK( TASK_GET_PATH_TO_INTERACTION_PARTNER );
	ADD_DEF_TASK( TASK_PRE_SCRIPT );
}


#define ADD_ACTIVITY_TO_SR(activityname) AddActivityToSR(#activityname,activityname)

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CAI_NPC::InitDefaultActivitySR(void) 
{
	ADD_ACTIVITY_TO_SR( ACT_INVALID );

	ADD_ACTIVITY_TO_SR( ACT_RESET );
	ADD_ACTIVITY_TO_SR( ACT_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_TRANSITION );
	ADD_ACTIVITY_TO_SR( ACT_COVER );
	ADD_ACTIVITY_TO_SR( ACT_COVER_MED );
	ADD_ACTIVITY_TO_SR( ACT_COVER_LOW );
	ADD_ACTIVITY_TO_SR( ACT_WALK );
	ADD_ACTIVITY_TO_SR( ACT_WALK_AIM );
	ADD_ACTIVITY_TO_SR( ACT_WALK_CROUCH );
	ADD_ACTIVITY_TO_SR( ACT_WALK_CROUCH_AIM );
	ADD_ACTIVITY_TO_SR( ACT_RUN );
	ADD_ACTIVITY_TO_SR( ACT_RUN_AIM );
	ADD_ACTIVITY_TO_SR( ACT_RUN_CROUCH );
	ADD_ACTIVITY_TO_SR( ACT_RUN_CROUCH_AIM );
	ADD_ACTIVITY_TO_SR( ACT_RUN_PROTECTED );
	ADD_ACTIVITY_TO_SR( ACT_SCRIPT_CUSTOM_MOVE );
	ADD_ACTIVITY_TO_SR( ACT_RANGE_ATTACK1 );
	ADD_ACTIVITY_TO_SR( ACT_RANGE_ATTACK2 );
	ADD_ACTIVITY_TO_SR( ACT_RANGE_ATTACK1_LOW );
	ADD_ACTIVITY_TO_SR( ACT_RANGE_ATTACK2_LOW );
	ADD_ACTIVITY_TO_SR( ACT_DIESIMPLE );
	ADD_ACTIVITY_TO_SR( ACT_DIEBACKWARD );
	ADD_ACTIVITY_TO_SR( ACT_DIEFORWARD );
	ADD_ACTIVITY_TO_SR( ACT_DIEVIOLENT );
	ADD_ACTIVITY_TO_SR( ACT_DIERAGDOLL );
	ADD_ACTIVITY_TO_SR( ACT_FLY );
	ADD_ACTIVITY_TO_SR( ACT_HOVER );
	ADD_ACTIVITY_TO_SR( ACT_GLIDE );
	ADD_ACTIVITY_TO_SR( ACT_SWIM );
	ADD_ACTIVITY_TO_SR( ACT_JUMP );
	ADD_ACTIVITY_TO_SR( ACT_HOP );
	ADD_ACTIVITY_TO_SR( ACT_LEAP );
	ADD_ACTIVITY_TO_SR( ACT_LAND );
	ADD_ACTIVITY_TO_SR( ACT_CLIMB_UP );
	ADD_ACTIVITY_TO_SR( ACT_CLIMB_DOWN );
	ADD_ACTIVITY_TO_SR( ACT_CLIMB_DISMOUNT );
	ADD_ACTIVITY_TO_SR( ACT_SHIPLADDER_UP );
	ADD_ACTIVITY_TO_SR( ACT_SHIPLADDER_DOWN );
	ADD_ACTIVITY_TO_SR( ACT_STRAFE_LEFT );
	ADD_ACTIVITY_TO_SR( ACT_STRAFE_RIGHT );
	ADD_ACTIVITY_TO_SR( ACT_ROLL_LEFT );
	ADD_ACTIVITY_TO_SR( ACT_ROLL_RIGHT );
	ADD_ACTIVITY_TO_SR( ACT_TURN_LEFT );
	ADD_ACTIVITY_TO_SR( ACT_TURN_RIGHT );
	ADD_ACTIVITY_TO_SR( ACT_CROUCH );
	ADD_ACTIVITY_TO_SR( ACT_CROUCHIDLE );
	ADD_ACTIVITY_TO_SR( ACT_STAND );
	ADD_ACTIVITY_TO_SR( ACT_USE );
	ADD_ACTIVITY_TO_SR( ACT_SIGNAL1 );
	ADD_ACTIVITY_TO_SR( ACT_SIGNAL2 );
	ADD_ACTIVITY_TO_SR( ACT_SIGNAL3 );
	ADD_ACTIVITY_TO_SR( ACT_SIGNAL_ADVANCE );
	ADD_ACTIVITY_TO_SR( ACT_SIGNAL_FORWARD );
	ADD_ACTIVITY_TO_SR( ACT_SIGNAL_GROUP );
	ADD_ACTIVITY_TO_SR( ACT_SIGNAL_HALT );
	ADD_ACTIVITY_TO_SR( ACT_SIGNAL_LEFT );
	ADD_ACTIVITY_TO_SR( ACT_SIGNAL_RIGHT );
	ADD_ACTIVITY_TO_SR( ACT_SIGNAL_TAKECOVER );
	ADD_ACTIVITY_TO_SR( ACT_LOOKBACK_RIGHT );
	ADD_ACTIVITY_TO_SR( ACT_LOOKBACK_LEFT );
	ADD_ACTIVITY_TO_SR( ACT_COWER );
	ADD_ACTIVITY_TO_SR( ACT_SMALL_FLINCH );
	ADD_ACTIVITY_TO_SR( ACT_BIG_FLINCH );
	ADD_ACTIVITY_TO_SR( ACT_MELEE_ATTACK1 );
	ADD_ACTIVITY_TO_SR( ACT_MELEE_ATTACK2 );
	ADD_ACTIVITY_TO_SR( ACT_RELOAD );
	ADD_ACTIVITY_TO_SR( ACT_RELOAD_START );
	ADD_ACTIVITY_TO_SR( ACT_RELOAD_FINISH );
	ADD_ACTIVITY_TO_SR( ACT_RELOAD_LOW );
	ADD_ACTIVITY_TO_SR( ACT_ARM );
	ADD_ACTIVITY_TO_SR( ACT_DISARM );
	ADD_ACTIVITY_TO_SR( ACT_DROP_WEAPON );
	ADD_ACTIVITY_TO_SR( ACT_DROP_WEAPON_SHOTGUN );
	ADD_ACTIVITY_TO_SR( ACT_PICKUP_GROUND );
	ADD_ACTIVITY_TO_SR( ACT_PICKUP_RACK );
	ADD_ACTIVITY_TO_SR( ACT_IDLE_ANGRY );
	ADD_ACTIVITY_TO_SR( ACT_IDLE_RELAXED );
	ADD_ACTIVITY_TO_SR( ACT_IDLE_STIMULATED );
	ADD_ACTIVITY_TO_SR( ACT_IDLE_AGITATED );
	ADD_ACTIVITY_TO_SR( ACT_IDLE_STEALTH );
	ADD_ACTIVITY_TO_SR( ACT_IDLE_HURT );
	ADD_ACTIVITY_TO_SR( ACT_WALK_RELAXED );
	ADD_ACTIVITY_TO_SR( ACT_WALK_STIMULATED );
	ADD_ACTIVITY_TO_SR( ACT_WALK_AGITATED );
	ADD_ACTIVITY_TO_SR( ACT_WALK_STEALTH );
	ADD_ACTIVITY_TO_SR( ACT_RUN_RELAXED );
	ADD_ACTIVITY_TO_SR( ACT_RUN_STIMULATED );
	ADD_ACTIVITY_TO_SR( ACT_RUN_AGITATED );
	ADD_ACTIVITY_TO_SR( ACT_RUN_STEALTH );
	ADD_ACTIVITY_TO_SR( ACT_IDLE_AIM_RELAXED );
	ADD_ACTIVITY_TO_SR( ACT_IDLE_AIM_STIMULATED );
	ADD_ACTIVITY_TO_SR( ACT_IDLE_AIM_AGITATED );
	ADD_ACTIVITY_TO_SR( ACT_IDLE_AIM_STEALTH );
	ADD_ACTIVITY_TO_SR( ACT_WALK_AIM_RELAXED );
	ADD_ACTIVITY_TO_SR( ACT_WALK_AIM_STIMULATED );
	ADD_ACTIVITY_TO_SR( ACT_WALK_AIM_AGITATED );
	ADD_ACTIVITY_TO_SR( ACT_WALK_AIM_STEALTH );
	ADD_ACTIVITY_TO_SR( ACT_RUN_AIM_RELAXED );
	ADD_ACTIVITY_TO_SR( ACT_RUN_AIM_STIMULATED );
	ADD_ACTIVITY_TO_SR( ACT_RUN_AIM_AGITATED );
	ADD_ACTIVITY_TO_SR( ACT_RUN_AIM_STEALTH );
	ADD_ACTIVITY_TO_SR( ACT_CROUCHIDLE_STIMULATED );
	ADD_ACTIVITY_TO_SR( ACT_CROUCHIDLE_AIM_STIMULATED );
	ADD_ACTIVITY_TO_SR( ACT_CROUCHIDLE_AGITATED );
	ADD_ACTIVITY_TO_SR( ACT_WALK_HURT );
	ADD_ACTIVITY_TO_SR( ACT_RUN_HURT );
	ADD_ACTIVITY_TO_SR( ACT_SPECIAL_ATTACK1 );
	ADD_ACTIVITY_TO_SR( ACT_SPECIAL_ATTACK2 );
	ADD_ACTIVITY_TO_SR( ACT_COMBAT_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_WALK_SCARED );
	ADD_ACTIVITY_TO_SR( ACT_RUN_SCARED );
	ADD_ACTIVITY_TO_SR( ACT_VICTORY_DANCE );
	ADD_ACTIVITY_TO_SR( ACT_DIE_HEADSHOT );
	ADD_ACTIVITY_TO_SR( ACT_DIE_CHESTSHOT );
	ADD_ACTIVITY_TO_SR( ACT_DIE_GUTSHOT );
	ADD_ACTIVITY_TO_SR( ACT_DIE_BACKSHOT );
	ADD_ACTIVITY_TO_SR( ACT_FLINCH_HEAD );
	ADD_ACTIVITY_TO_SR( ACT_FLINCH_CHEST );
	ADD_ACTIVITY_TO_SR( ACT_FLINCH_STOMACH );
	ADD_ACTIVITY_TO_SR( ACT_FLINCH_LEFTARM );
	ADD_ACTIVITY_TO_SR( ACT_FLINCH_RIGHTARM );
	ADD_ACTIVITY_TO_SR( ACT_FLINCH_LEFTLEG );
	ADD_ACTIVITY_TO_SR( ACT_FLINCH_RIGHTLEG );
	ADD_ACTIVITY_TO_SR( ACT_FLINCH_PHYSICS );
	ADD_ACTIVITY_TO_SR( ACT_IDLE_ON_FIRE );
	ADD_ACTIVITY_TO_SR( ACT_WALK_ON_FIRE );
	ADD_ACTIVITY_TO_SR( ACT_RUN_ON_FIRE );
	ADD_ACTIVITY_TO_SR( ACT_RAPPEL_LOOP );
	ADD_ACTIVITY_TO_SR( ACT_180_LEFT );
	ADD_ACTIVITY_TO_SR( ACT_180_RIGHT );
	ADD_ACTIVITY_TO_SR( ACT_90_LEFT );
	ADD_ACTIVITY_TO_SR( ACT_90_RIGHT );
	ADD_ACTIVITY_TO_SR( ACT_STEP_LEFT );
	ADD_ACTIVITY_TO_SR( ACT_STEP_RIGHT );
	ADD_ACTIVITY_TO_SR( ACT_STEP_BACK );
	ADD_ACTIVITY_TO_SR( ACT_STEP_FORE );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_RANGE_ATTACK1 );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_RANGE_ATTACK2 );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_MELEE_ATTACK1 );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_MELEE_ATTACK2 );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_RANGE_ATTACK1_LOW );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_RANGE_ATTACK2_LOW );
	ADD_ACTIVITY_TO_SR( ACT_MELEE_ATTACK_SWING_GESTURE );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_SMALL_FLINCH );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_BIG_FLINCH );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_FLINCH_BLAST );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_FLINCH_BLAST_SHOTGUN );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_FLINCH_BLAST_DAMAGED );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_FLINCH_BLAST_DAMAGED_SHOTGUN );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_FLINCH_HEAD );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_FLINCH_CHEST );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_FLINCH_STOMACH );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_FLINCH_LEFTARM );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_FLINCH_RIGHTARM );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_FLINCH_LEFTLEG );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_FLINCH_RIGHTLEG );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_TURN_LEFT );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_TURN_RIGHT );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_TURN_LEFT45 );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_TURN_RIGHT45 );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_TURN_LEFT90 );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_TURN_RIGHT90 );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_TURN_LEFT45_FLAT );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_TURN_RIGHT45_FLAT );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_TURN_LEFT90_FLAT );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_TURN_RIGHT90_FLAT );
	ADD_ACTIVITY_TO_SR( ACT_BARNACLE_HIT );
	ADD_ACTIVITY_TO_SR( ACT_BARNACLE_PULL );
	ADD_ACTIVITY_TO_SR( ACT_BARNACLE_CHOMP );
	ADD_ACTIVITY_TO_SR( ACT_BARNACLE_CHEW );
	ADD_ACTIVITY_TO_SR( ACT_DO_NOT_DISTURB );
	ADD_ACTIVITY_TO_SR( ACT_VM_DRAW );
	ADD_ACTIVITY_TO_SR( ACT_VM_HOLSTER );
	ADD_ACTIVITY_TO_SR( ACT_VM_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_VM_FIDGET );
	ADD_ACTIVITY_TO_SR( ACT_VM_PULLBACK );
	ADD_ACTIVITY_TO_SR( ACT_VM_PULLBACK_HIGH );
	ADD_ACTIVITY_TO_SR( ACT_VM_PULLBACK_LOW );
	ADD_ACTIVITY_TO_SR( ACT_VM_THROW );
	ADD_ACTIVITY_TO_SR( ACT_VM_PULLPIN );
	ADD_ACTIVITY_TO_SR( ACT_VM_PRIMARYATTACK );
	ADD_ACTIVITY_TO_SR( ACT_VM_SECONDARYATTACK );
	ADD_ACTIVITY_TO_SR( ACT_VM_RELOAD );
	ADD_ACTIVITY_TO_SR( ACT_VM_RELOAD_START );
	ADD_ACTIVITY_TO_SR( ACT_VM_RELOAD_FINISH );
	ADD_ACTIVITY_TO_SR( ACT_VM_DRYFIRE );
	ADD_ACTIVITY_TO_SR( ACT_VM_HITLEFT );
	ADD_ACTIVITY_TO_SR( ACT_VM_HITLEFT2 );
	ADD_ACTIVITY_TO_SR( ACT_VM_HITRIGHT );
	ADD_ACTIVITY_TO_SR( ACT_VM_HITRIGHT2 );
	ADD_ACTIVITY_TO_SR( ACT_VM_HITCENTER );
	ADD_ACTIVITY_TO_SR( ACT_VM_HITCENTER2 );
	ADD_ACTIVITY_TO_SR( ACT_VM_MISSLEFT );
	ADD_ACTIVITY_TO_SR( ACT_VM_MISSLEFT2 );
	ADD_ACTIVITY_TO_SR( ACT_VM_MISSRIGHT );
	ADD_ACTIVITY_TO_SR( ACT_VM_MISSRIGHT2 );
	ADD_ACTIVITY_TO_SR( ACT_VM_MISSCENTER );
	ADD_ACTIVITY_TO_SR( ACT_VM_MISSCENTER2 );
	ADD_ACTIVITY_TO_SR( ACT_VM_HAULBACK );
	ADD_ACTIVITY_TO_SR( ACT_VM_SWINGHARD );
	ADD_ACTIVITY_TO_SR( ACT_VM_SWINGMISS );
	ADD_ACTIVITY_TO_SR( ACT_VM_SWINGHIT );
	ADD_ACTIVITY_TO_SR( ACT_VM_IDLE_TO_LOWERED );
	ADD_ACTIVITY_TO_SR( ACT_VM_IDLE_LOWERED );
	ADD_ACTIVITY_TO_SR( ACT_VM_LOWERED_TO_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_VM_RECOIL1 );
	ADD_ACTIVITY_TO_SR( ACT_VM_RECOIL2 );
	ADD_ACTIVITY_TO_SR( ACT_VM_RECOIL3 );
	ADD_ACTIVITY_TO_SR( ACT_VM_PICKUP );
	ADD_ACTIVITY_TO_SR( ACT_VM_RELEASE );
	ADD_ACTIVITY_TO_SR( ACT_VM_ATTACH_SILENCER );
	ADD_ACTIVITY_TO_SR( ACT_VM_DETACH_SILENCER );
	ADD_ACTIVITY_TO_SR( ACT_VM_DRAW_SPECIAL );
	ADD_ACTIVITY_TO_SR( ACT_VM_HOLSTER_SPECIAL );
	ADD_ACTIVITY_TO_SR( ACT_VM_IDLE_SPECIAL );
	ADD_ACTIVITY_TO_SR( ACT_VM_PULLBACK_SPECIAL );
	ADD_ACTIVITY_TO_SR( ACT_VM_PRIMARYATTACK_SPECIAL );
	ADD_ACTIVITY_TO_SR( ACT_VM_SECONDARYATTACK_SPECIAL );
	ADD_ACTIVITY_TO_SR( ACT_VM_HITCENTER_SPECIAL );
	ADD_ACTIVITY_TO_SR( ACT_VM_SWINGHARD_SPECIAL );
	ADD_ACTIVITY_TO_SR( ACT_VM_IDLE_TO_LOWERED_SPECIAL );
	ADD_ACTIVITY_TO_SR( ACT_VM_IDLE_LOWERED_SPECIAL );
	ADD_ACTIVITY_TO_SR( ACT_VM_LOWERED_TO_IDLE_SPECIAL );
	ADD_ACTIVITY_TO_SR( ACT_FISTS_VM_HITLEFT );
	ADD_ACTIVITY_TO_SR( ACT_FISTS_VM_HITRIGHT );
	ADD_ACTIVITY_TO_SR( ACT_FISTS_VM_SWINGHARD );
	ADD_ACTIVITY_TO_SR( ACT_FISTS_VM_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_FISTS_VM_DRAW );
	ADD_ACTIVITY_TO_SR( ACT_SLAM_STICKWALL_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_SLAM_STICKWALL_ND_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_SLAM_STICKWALL_ATTACH );
	ADD_ACTIVITY_TO_SR( ACT_SLAM_STICKWALL_ATTACH2 );
	ADD_ACTIVITY_TO_SR( ACT_SLAM_STICKWALL_ND_ATTACH );
	ADD_ACTIVITY_TO_SR( ACT_SLAM_STICKWALL_ND_ATTACH2 );
	ADD_ACTIVITY_TO_SR( ACT_SLAM_STICKWALL_DETONATE );
	ADD_ACTIVITY_TO_SR( ACT_SLAM_STICKWALL_DETONATOR_HOLSTER );
	ADD_ACTIVITY_TO_SR( ACT_SLAM_STICKWALL_DRAW );
	ADD_ACTIVITY_TO_SR( ACT_SLAM_STICKWALL_ND_DRAW );
	ADD_ACTIVITY_TO_SR( ACT_SLAM_STICKWALL_TO_THROW );
	ADD_ACTIVITY_TO_SR( ACT_SLAM_STICKWALL_TO_THROW_ND );
	ADD_ACTIVITY_TO_SR( ACT_SLAM_STICKWALL_TO_TRIPMINE_ND );
	ADD_ACTIVITY_TO_SR( ACT_SLAM_THROW_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_SLAM_THROW_ND_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_SLAM_THROW_THROW );
	ADD_ACTIVITY_TO_SR( ACT_SLAM_THROW_THROW2 );
	ADD_ACTIVITY_TO_SR( ACT_SLAM_THROW_THROW_ND );
	ADD_ACTIVITY_TO_SR( ACT_SLAM_THROW_THROW_ND2 );
	ADD_ACTIVITY_TO_SR( ACT_SLAM_THROW_DRAW );
	ADD_ACTIVITY_TO_SR( ACT_SLAM_THROW_ND_DRAW );
	ADD_ACTIVITY_TO_SR( ACT_SLAM_THROW_TO_STICKWALL );
	ADD_ACTIVITY_TO_SR( ACT_SLAM_THROW_TO_STICKWALL_ND );
	ADD_ACTIVITY_TO_SR( ACT_SLAM_THROW_DETONATE );
	ADD_ACTIVITY_TO_SR( ACT_SLAM_THROW_DETONATOR_HOLSTER );
	ADD_ACTIVITY_TO_SR( ACT_SLAM_THROW_TO_TRIPMINE_ND );
	ADD_ACTIVITY_TO_SR( ACT_SLAM_TRIPMINE_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_SLAM_TRIPMINE_DRAW );
	ADD_ACTIVITY_TO_SR( ACT_SLAM_TRIPMINE_ATTACH );
	ADD_ACTIVITY_TO_SR( ACT_SLAM_TRIPMINE_ATTACH2 );
	ADD_ACTIVITY_TO_SR( ACT_SLAM_TRIPMINE_TO_STICKWALL_ND );
	ADD_ACTIVITY_TO_SR( ACT_SLAM_TRIPMINE_TO_THROW_ND );
	ADD_ACTIVITY_TO_SR( ACT_SLAM_DETONATOR_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_SLAM_DETONATOR_DRAW );
	ADD_ACTIVITY_TO_SR( ACT_SLAM_DETONATOR_DETONATE );
	ADD_ACTIVITY_TO_SR( ACT_SLAM_DETONATOR_HOLSTER );
	ADD_ACTIVITY_TO_SR( ACT_SLAM_DETONATOR_STICKWALL_DRAW );
	ADD_ACTIVITY_TO_SR( ACT_SLAM_DETONATOR_THROW_DRAW );
	ADD_ACTIVITY_TO_SR( ACT_SHOTGUN_RELOAD_START );
	ADD_ACTIVITY_TO_SR( ACT_SHOTGUN_RELOAD_FINISH );
	ADD_ACTIVITY_TO_SR( ACT_SHOTGUN_PUMP );
	ADD_ACTIVITY_TO_SR( ACT_SMG2_IDLE2 );
	ADD_ACTIVITY_TO_SR( ACT_SMG2_FIRE2 );
	ADD_ACTIVITY_TO_SR( ACT_SMG2_DRAW2 );
	ADD_ACTIVITY_TO_SR( ACT_SMG2_RELOAD2 );
	ADD_ACTIVITY_TO_SR( ACT_SMG2_DRYFIRE2 );
	ADD_ACTIVITY_TO_SR( ACT_SMG2_TOAUTO );
	ADD_ACTIVITY_TO_SR( ACT_SMG2_TOBURST );
	ADD_ACTIVITY_TO_SR( ACT_PHYSCANNON_UPGRADE );
	ADD_ACTIVITY_TO_SR( ACT_RANGE_ATTACK_AR1 );
	ADD_ACTIVITY_TO_SR( ACT_RANGE_ATTACK_AR2 );
	ADD_ACTIVITY_TO_SR( ACT_RANGE_ATTACK_AR2_LOW );
	ADD_ACTIVITY_TO_SR( ACT_RANGE_ATTACK_AR2_GRENADE );
	ADD_ACTIVITY_TO_SR( ACT_RANGE_ATTACK_HMG1 );
	ADD_ACTIVITY_TO_SR( ACT_RANGE_ATTACK_ML );
	ADD_ACTIVITY_TO_SR( ACT_RANGE_ATTACK_SMG1 );
	ADD_ACTIVITY_TO_SR( ACT_RANGE_ATTACK_SMG1_LOW );
	ADD_ACTIVITY_TO_SR( ACT_RANGE_ATTACK_SMG2 );
	ADD_ACTIVITY_TO_SR( ACT_RANGE_ATTACK_SHOTGUN );
	ADD_ACTIVITY_TO_SR( ACT_RANGE_ATTACK_SHOTGUN_LOW );
	ADD_ACTIVITY_TO_SR( ACT_RANGE_ATTACK_PISTOL );
	ADD_ACTIVITY_TO_SR( ACT_RANGE_ATTACK_PISTOL_LOW );
	ADD_ACTIVITY_TO_SR( ACT_RANGE_ATTACK_SLAM );
	ADD_ACTIVITY_TO_SR( ACT_RANGE_ATTACK_TRIPWIRE );
	ADD_ACTIVITY_TO_SR( ACT_RANGE_ATTACK_THROW );
	ADD_ACTIVITY_TO_SR( ACT_RANGE_ATTACK_SNIPER_RIFLE );
	ADD_ACTIVITY_TO_SR( ACT_RANGE_ATTACK_RPG );
	ADD_ACTIVITY_TO_SR( ACT_MELEE_ATTACK_SWING );
	ADD_ACTIVITY_TO_SR( ACT_RANGE_AIM_LOW );
	ADD_ACTIVITY_TO_SR( ACT_RANGE_AIM_SMG1_LOW );
	ADD_ACTIVITY_TO_SR( ACT_RANGE_AIM_PISTOL_LOW );
	ADD_ACTIVITY_TO_SR( ACT_RANGE_AIM_AR2_LOW );
	ADD_ACTIVITY_TO_SR( ACT_COVER_PISTOL_LOW );
	ADD_ACTIVITY_TO_SR( ACT_COVER_SMG1_LOW );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_RANGE_ATTACK_AR1 );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_RANGE_ATTACK_AR2 );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_RANGE_ATTACK_AR2_GRENADE );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_RANGE_ATTACK_HMG1 );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_RANGE_ATTACK_ML );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_RANGE_ATTACK_SMG1 );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_RANGE_ATTACK_SMG1_LOW );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_RANGE_ATTACK_SMG2 );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_RANGE_ATTACK_SHOTGUN );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_RANGE_ATTACK_PISTOL );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_RANGE_ATTACK_PISTOL_LOW );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_RANGE_ATTACK_SLAM );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_RANGE_ATTACK_TRIPWIRE );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_RANGE_ATTACK_THROW );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_RANGE_ATTACK_SNIPER_RIFLE );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_MELEE_ATTACK_SWING );
	ADD_ACTIVITY_TO_SR( ACT_IDLE_RIFLE );
	ADD_ACTIVITY_TO_SR( ACT_IDLE_SMG1 );
	ADD_ACTIVITY_TO_SR( ACT_IDLE_ANGRY_SMG1 );
	ADD_ACTIVITY_TO_SR( ACT_IDLE_PISTOL );
	ADD_ACTIVITY_TO_SR( ACT_IDLE_ANGRY_PISTOL );
	ADD_ACTIVITY_TO_SR( ACT_IDLE_ANGRY_SHOTGUN );
	ADD_ACTIVITY_TO_SR( ACT_IDLE_STEALTH_PISTOL );
	ADD_ACTIVITY_TO_SR( ACT_IDLE_PACKAGE );
	ADD_ACTIVITY_TO_SR( ACT_WALK_PACKAGE );
	ADD_ACTIVITY_TO_SR( ACT_IDLE_SUITCASE );
	ADD_ACTIVITY_TO_SR( ACT_WALK_SUITCASE );
	ADD_ACTIVITY_TO_SR( ACT_IDLE_SMG1_RELAXED );
	ADD_ACTIVITY_TO_SR( ACT_IDLE_SMG1_STIMULATED );
	ADD_ACTIVITY_TO_SR( ACT_WALK_RIFLE_RELAXED );
	ADD_ACTIVITY_TO_SR( ACT_RUN_RIFLE_RELAXED );
	ADD_ACTIVITY_TO_SR( ACT_WALK_RIFLE_STIMULATED );
	ADD_ACTIVITY_TO_SR( ACT_RUN_RIFLE_STIMULATED );
	ADD_ACTIVITY_TO_SR( ACT_IDLE_AIM_RIFLE_STIMULATED );
	ADD_ACTIVITY_TO_SR( ACT_WALK_AIM_RIFLE_STIMULATED );
	ADD_ACTIVITY_TO_SR( ACT_RUN_AIM_RIFLE_STIMULATED );
	ADD_ACTIVITY_TO_SR( ACT_IDLE_SHOTGUN_RELAXED );
	ADD_ACTIVITY_TO_SR( ACT_IDLE_SHOTGUN_STIMULATED );
	ADD_ACTIVITY_TO_SR( ACT_IDLE_SHOTGUN_AGITATED );
	ADD_ACTIVITY_TO_SR( ACT_WALK_ANGRY );
	ADD_ACTIVITY_TO_SR( ACT_POLICE_HARASS1 );
	ADD_ACTIVITY_TO_SR( ACT_POLICE_HARASS2 );
	ADD_ACTIVITY_TO_SR( ACT_IDLE_MANNEDGUN );
	ADD_ACTIVITY_TO_SR( ACT_IDLE_MELEE );
	ADD_ACTIVITY_TO_SR( ACT_IDLE_ANGRY_MELEE );
	ADD_ACTIVITY_TO_SR( ACT_IDLE_RPG_RELAXED );
	ADD_ACTIVITY_TO_SR( ACT_IDLE_RPG );
	ADD_ACTIVITY_TO_SR( ACT_IDLE_ANGRY_RPG );
	ADD_ACTIVITY_TO_SR( ACT_COVER_LOW_RPG );
	ADD_ACTIVITY_TO_SR( ACT_WALK_RPG );
	ADD_ACTIVITY_TO_SR( ACT_RUN_RPG );
	ADD_ACTIVITY_TO_SR( ACT_WALK_CROUCH_RPG );
	ADD_ACTIVITY_TO_SR( ACT_RUN_CROUCH_RPG );
	ADD_ACTIVITY_TO_SR( ACT_WALK_RPG_RELAXED );
	ADD_ACTIVITY_TO_SR( ACT_RUN_RPG_RELAXED );
	ADD_ACTIVITY_TO_SR( ACT_WALK_RIFLE );
	ADD_ACTIVITY_TO_SR( ACT_WALK_AIM_RIFLE );
	ADD_ACTIVITY_TO_SR( ACT_WALK_CROUCH_RIFLE );
	ADD_ACTIVITY_TO_SR( ACT_WALK_CROUCH_AIM_RIFLE );
	ADD_ACTIVITY_TO_SR( ACT_RUN_RIFLE );
	ADD_ACTIVITY_TO_SR( ACT_RUN_AIM_RIFLE );
	ADD_ACTIVITY_TO_SR( ACT_RUN_CROUCH_RIFLE );
	ADD_ACTIVITY_TO_SR( ACT_RUN_CROUCH_AIM_RIFLE );
	ADD_ACTIVITY_TO_SR( ACT_RUN_STEALTH_PISTOL );
	ADD_ACTIVITY_TO_SR( ACT_WALK_AIM_SHOTGUN );
	ADD_ACTIVITY_TO_SR( ACT_RUN_AIM_SHOTGUN );
	ADD_ACTIVITY_TO_SR( ACT_WALK_PISTOL );
	ADD_ACTIVITY_TO_SR( ACT_RUN_PISTOL );
	ADD_ACTIVITY_TO_SR( ACT_WALK_AIM_PISTOL );
	ADD_ACTIVITY_TO_SR( ACT_RUN_AIM_PISTOL );
	ADD_ACTIVITY_TO_SR( ACT_WALK_STEALTH_PISTOL );
	ADD_ACTIVITY_TO_SR( ACT_WALK_AIM_STEALTH_PISTOL );
	ADD_ACTIVITY_TO_SR( ACT_RUN_AIM_STEALTH_PISTOL );
	ADD_ACTIVITY_TO_SR( ACT_RELOAD_PISTOL );
	ADD_ACTIVITY_TO_SR( ACT_RELOAD_PISTOL_LOW );
	ADD_ACTIVITY_TO_SR( ACT_RELOAD_SMG1 );
	ADD_ACTIVITY_TO_SR( ACT_RELOAD_SMG1_LOW );
	ADD_ACTIVITY_TO_SR( ACT_RELOAD_SHOTGUN );
	ADD_ACTIVITY_TO_SR( ACT_RELOAD_SHOTGUN_LOW );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_RELOAD );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_RELOAD_PISTOL );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_RELOAD_SMG1 );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_RELOAD_SHOTGUN );
	ADD_ACTIVITY_TO_SR( ACT_BUSY_LEAN_LEFT );
	ADD_ACTIVITY_TO_SR( ACT_BUSY_LEAN_LEFT_ENTRY );
	ADD_ACTIVITY_TO_SR( ACT_BUSY_LEAN_LEFT_EXIT );
	ADD_ACTIVITY_TO_SR( ACT_BUSY_LEAN_BACK );
	ADD_ACTIVITY_TO_SR( ACT_BUSY_LEAN_BACK_ENTRY );
	ADD_ACTIVITY_TO_SR( ACT_BUSY_LEAN_BACK_EXIT );
	ADD_ACTIVITY_TO_SR( ACT_BUSY_SIT_GROUND );
	ADD_ACTIVITY_TO_SR( ACT_BUSY_SIT_GROUND_ENTRY );
	ADD_ACTIVITY_TO_SR( ACT_BUSY_SIT_GROUND_EXIT );
	ADD_ACTIVITY_TO_SR( ACT_BUSY_SIT_CHAIR );
	ADD_ACTIVITY_TO_SR( ACT_BUSY_SIT_CHAIR_ENTRY );
	ADD_ACTIVITY_TO_SR( ACT_BUSY_SIT_CHAIR_EXIT );
	ADD_ACTIVITY_TO_SR( ACT_BUSY_STAND );
	ADD_ACTIVITY_TO_SR( ACT_BUSY_QUEUE );
	ADD_ACTIVITY_TO_SR( ACT_DUCK_DODGE );
	ADD_ACTIVITY_TO_SR( ACT_DIE_BARNACLE_SWALLOW );
	ADD_ACTIVITY_TO_SR( ACT_GESTURE_BARNACLE_STRANGLE );
	ADD_ACTIVITY_TO_SR( ACT_PHYSCANNON_DETACH );
	ADD_ACTIVITY_TO_SR( ACT_PHYSCANNON_ANIMATE );
	ADD_ACTIVITY_TO_SR( ACT_PHYSCANNON_ANIMATE_PRE );
	ADD_ACTIVITY_TO_SR( ACT_PHYSCANNON_ANIMATE_POST );
	ADD_ACTIVITY_TO_SR( ACT_DIE_FRONTSIDE );
	ADD_ACTIVITY_TO_SR( ACT_DIE_RIGHTSIDE );
	ADD_ACTIVITY_TO_SR( ACT_DIE_BACKSIDE );
	ADD_ACTIVITY_TO_SR( ACT_DIE_LEFTSIDE );
	ADD_ACTIVITY_TO_SR( ACT_OPEN_DOOR );
	ADD_ACTIVITY_TO_SR( ACT_DI_ALYX_ZOMBIE_MELEE );
	ADD_ACTIVITY_TO_SR( ACT_DI_ALYX_ZOMBIE_TORSO_MELEE );
	ADD_ACTIVITY_TO_SR( ACT_DI_ALYX_HEADCRAB_MELEE );
	ADD_ACTIVITY_TO_SR( ACT_DI_ALYX_ANTLION );
	ADD_ACTIVITY_TO_SR( ACT_DI_ALYX_ZOMBIE_SHOTGUN64 );
	ADD_ACTIVITY_TO_SR( ACT_DI_ALYX_ZOMBIE_SHOTGUN26 );
	ADD_ACTIVITY_TO_SR( ACT_READINESS_RELAXED_TO_STIMULATED );
	ADD_ACTIVITY_TO_SR( ACT_READINESS_RELAXED_TO_STIMULATED_WALK );
	ADD_ACTIVITY_TO_SR( ACT_READINESS_AGITATED_TO_STIMULATED );
	ADD_ACTIVITY_TO_SR( ACT_READINESS_STIMULATED_TO_RELAXED );
	ADD_ACTIVITY_TO_SR( ACT_READINESS_PISTOL_RELAXED_TO_STIMULATED );
	ADD_ACTIVITY_TO_SR( ACT_READINESS_PISTOL_RELAXED_TO_STIMULATED_WALK );
	ADD_ACTIVITY_TO_SR( ACT_READINESS_PISTOL_AGITATED_TO_STIMULATED );
	ADD_ACTIVITY_TO_SR( ACT_READINESS_PISTOL_STIMULATED_TO_RELAXED );
	ADD_ACTIVITY_TO_SR( ACT_IDLE_CARRY );
	ADD_ACTIVITY_TO_SR( ACT_WALK_CARRY );
	ADD_ACTIVITY_TO_SR( ACT_STARTDYING );
	ADD_ACTIVITY_TO_SR( ACT_DYINGLOOP );
	ADD_ACTIVITY_TO_SR( ACT_DYINGTODEAD );
	ADD_ACTIVITY_TO_SR( ACT_RIDE_MANNED_GUN );
	ADD_ACTIVITY_TO_SR( ACT_VM_SPRINT_ENTER );
	ADD_ACTIVITY_TO_SR( ACT_VM_SPRINT_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_VM_SPRINT_LEAVE );
	ADD_ACTIVITY_TO_SR( ACT_FIRE_START );
	ADD_ACTIVITY_TO_SR( ACT_FIRE_LOOP );
	ADD_ACTIVITY_TO_SR( ACT_FIRE_END );
	ADD_ACTIVITY_TO_SR( ACT_CROUCHING_GRENADEIDLE );
	ADD_ACTIVITY_TO_SR( ACT_CROUCHING_GRENADEREADY );
	ADD_ACTIVITY_TO_SR( ACT_CROUCHING_PRIMARYATTACK );
	ADD_ACTIVITY_TO_SR( ACT_OVERLAY_GRENADEIDLE );
	ADD_ACTIVITY_TO_SR( ACT_OVERLAY_GRENADEREADY );
	ADD_ACTIVITY_TO_SR( ACT_OVERLAY_PRIMARYATTACK );
	ADD_ACTIVITY_TO_SR( ACT_OVERLAY_SHIELD_UP );
	ADD_ACTIVITY_TO_SR( ACT_OVERLAY_SHIELD_DOWN );
	ADD_ACTIVITY_TO_SR( ACT_OVERLAY_SHIELD_UP_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_OVERLAY_SHIELD_ATTACK );
	ADD_ACTIVITY_TO_SR( ACT_OVERLAY_SHIELD_KNOCKBACK );
	ADD_ACTIVITY_TO_SR( ACT_SHIELD_UP );
	ADD_ACTIVITY_TO_SR( ACT_SHIELD_DOWN );
	ADD_ACTIVITY_TO_SR( ACT_SHIELD_UP_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_SHIELD_ATTACK );
	ADD_ACTIVITY_TO_SR( ACT_SHIELD_KNOCKBACK );
	ADD_ACTIVITY_TO_SR( ACT_CROUCHING_SHIELD_UP );
	ADD_ACTIVITY_TO_SR( ACT_CROUCHING_SHIELD_DOWN );
	ADD_ACTIVITY_TO_SR( ACT_CROUCHING_SHIELD_UP_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_CROUCHING_SHIELD_ATTACK );
	ADD_ACTIVITY_TO_SR( ACT_CROUCHING_SHIELD_KNOCKBACK );
	ADD_ACTIVITY_TO_SR( ACT_TURNRIGHT45 );
	ADD_ACTIVITY_TO_SR( ACT_TURNLEFT45 );
	ADD_ACTIVITY_TO_SR( ACT_TURN );
	ADD_ACTIVITY_TO_SR( ACT_OBJ_ASSEMBLING );
	ADD_ACTIVITY_TO_SR( ACT_OBJ_DISMANTLING );
	ADD_ACTIVITY_TO_SR( ACT_OBJ_STARTUP );
	ADD_ACTIVITY_TO_SR( ACT_OBJ_RUNNING );
	ADD_ACTIVITY_TO_SR( ACT_OBJ_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_OBJ_PLACING );
	ADD_ACTIVITY_TO_SR( ACT_OBJ_DETERIORATING );
	ADD_ACTIVITY_TO_SR( ACT_OBJ_UPGRADING );
	ADD_ACTIVITY_TO_SR( ACT_DEPLOY );
	ADD_ACTIVITY_TO_SR( ACT_DEPLOY_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_UNDEPLOY );
	ADD_ACTIVITY_TO_SR( ACT_GRENADE_ROLL );
	ADD_ACTIVITY_TO_SR( ACT_GRENADE_TOSS );
	ADD_ACTIVITY_TO_SR( ACT_HANDGRENADE_THROW1 );
	ADD_ACTIVITY_TO_SR( ACT_HANDGRENADE_THROW2 );
	ADD_ACTIVITY_TO_SR( ACT_HANDGRENADE_THROW3 );
	ADD_ACTIVITY_TO_SR( ACT_SHOTGUN_IDLE_DEEP );
	ADD_ACTIVITY_TO_SR( ACT_SHOTGUN_IDLE4 );
	ADD_ACTIVITY_TO_SR( ACT_GLOCK_SHOOTEMPTY );
	ADD_ACTIVITY_TO_SR( ACT_GLOCK_SHOOT_RELOAD );
	ADD_ACTIVITY_TO_SR( ACT_RPG_DRAW_UNLOADED );
	ADD_ACTIVITY_TO_SR( ACT_RPG_HOLSTER_UNLOADED );
	ADD_ACTIVITY_TO_SR( ACT_RPG_IDLE_UNLOADED );
	ADD_ACTIVITY_TO_SR( ACT_RPG_FIDGET_UNLOADED );
	ADD_ACTIVITY_TO_SR( ACT_CROSSBOW_DRAW_UNLOADED );
	ADD_ACTIVITY_TO_SR( ACT_CROSSBOW_IDLE_UNLOADED );
	ADD_ACTIVITY_TO_SR( ACT_CROSSBOW_FIDGET_UNLOADED );
	ADD_ACTIVITY_TO_SR( ACT_GAUSS_SPINUP );
	ADD_ACTIVITY_TO_SR( ACT_GAUSS_SPINCYCLE );
	ADD_ACTIVITY_TO_SR( ACT_TRIPMINE_GROUND );
	ADD_ACTIVITY_TO_SR( ACT_TRIPMINE_WORLD );
	ADD_ACTIVITY_TO_SR( ACT_VM_PRIMARYATTACK_SILENCED );
	ADD_ACTIVITY_TO_SR( ACT_VM_RELOAD_SILENCED );
	ADD_ACTIVITY_TO_SR( ACT_VM_DRYFIRE_SILENCED );
	ADD_ACTIVITY_TO_SR( ACT_VM_IDLE_SILENCED );
	ADD_ACTIVITY_TO_SR( ACT_VM_DRAW_SILENCED );
	ADD_ACTIVITY_TO_SR( ACT_VM_IDLE_EMPTY_LEFT );
	ADD_ACTIVITY_TO_SR( ACT_VM_DRYFIRE_LEFT );
	ADD_ACTIVITY_TO_SR( ACT_PLAYER_IDLE_FIRE );
	ADD_ACTIVITY_TO_SR( ACT_PLAYER_CROUCH_FIRE );
	ADD_ACTIVITY_TO_SR( ACT_PLAYER_CROUCH_WALK_FIRE );
	ADD_ACTIVITY_TO_SR( ACT_PLAYER_WALK_FIRE );
	ADD_ACTIVITY_TO_SR( ACT_PLAYER_RUN_FIRE );
	ADD_ACTIVITY_TO_SR( ACT_IDLETORUN );
	ADD_ACTIVITY_TO_SR( ACT_RUNTOIDLE );
	ADD_ACTIVITY_TO_SR( ACT_SPRINT );
	ADD_ACTIVITY_TO_SR( ACT_GET_DOWN_STAND );
	ADD_ACTIVITY_TO_SR( ACT_GET_UP_STAND );
	ADD_ACTIVITY_TO_SR( ACT_GET_DOWN_CROUCH );
	ADD_ACTIVITY_TO_SR( ACT_GET_UP_CROUCH );
	ADD_ACTIVITY_TO_SR( ACT_PRONE_FORWARD );
	ADD_ACTIVITY_TO_SR( ACT_PRONE_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_DEEPIDLE1 );
	ADD_ACTIVITY_TO_SR( ACT_DEEPIDLE2 );
	ADD_ACTIVITY_TO_SR( ACT_DEEPIDLE3 );
	ADD_ACTIVITY_TO_SR( ACT_DEEPIDLE4 );
	ADD_ACTIVITY_TO_SR( ACT_VM_RELOAD_DEPLOYED );
	ADD_ACTIVITY_TO_SR( ACT_VM_RELOAD_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_VM_DRAW_DEPLOYED );
	ADD_ACTIVITY_TO_SR( ACT_VM_DRAW_EMPTY );
	ADD_ACTIVITY_TO_SR( ACT_VM_PRIMARYATTACK_EMPTY );
	ADD_ACTIVITY_TO_SR( ACT_VM_RELOAD_EMPTY );
	ADD_ACTIVITY_TO_SR( ACT_VM_IDLE_EMPTY );
	ADD_ACTIVITY_TO_SR( ACT_VM_IDLE_DEPLOYED_EMPTY );
	ADD_ACTIVITY_TO_SR( ACT_VM_IDLE_8 );
	ADD_ACTIVITY_TO_SR( ACT_VM_IDLE_7 );
	ADD_ACTIVITY_TO_SR( ACT_VM_IDLE_6 );
	ADD_ACTIVITY_TO_SR( ACT_VM_IDLE_5 );
	ADD_ACTIVITY_TO_SR( ACT_VM_IDLE_4 );
	ADD_ACTIVITY_TO_SR( ACT_VM_IDLE_3 );
	ADD_ACTIVITY_TO_SR( ACT_VM_IDLE_2 );
	ADD_ACTIVITY_TO_SR( ACT_VM_IDLE_1 );
	ADD_ACTIVITY_TO_SR( ACT_VM_IDLE_DEPLOYED );
	ADD_ACTIVITY_TO_SR( ACT_VM_IDLE_DEPLOYED_8 );
	ADD_ACTIVITY_TO_SR( ACT_VM_IDLE_DEPLOYED_7 );
	ADD_ACTIVITY_TO_SR( ACT_VM_IDLE_DEPLOYED_6 );
	ADD_ACTIVITY_TO_SR( ACT_VM_IDLE_DEPLOYED_5 );
	ADD_ACTIVITY_TO_SR( ACT_VM_IDLE_DEPLOYED_4 );
	ADD_ACTIVITY_TO_SR( ACT_VM_IDLE_DEPLOYED_3 );
	ADD_ACTIVITY_TO_SR( ACT_VM_IDLE_DEPLOYED_2 );
	ADD_ACTIVITY_TO_SR( ACT_VM_IDLE_DEPLOYED_1 );
	ADD_ACTIVITY_TO_SR( ACT_VM_UNDEPLOY );
	ADD_ACTIVITY_TO_SR( ACT_VM_UNDEPLOY_8 );
	ADD_ACTIVITY_TO_SR( ACT_VM_UNDEPLOY_7 );
	ADD_ACTIVITY_TO_SR( ACT_VM_UNDEPLOY_6 );
	ADD_ACTIVITY_TO_SR( ACT_VM_UNDEPLOY_5 );
	ADD_ACTIVITY_TO_SR( ACT_VM_UNDEPLOY_4 );
	ADD_ACTIVITY_TO_SR( ACT_VM_UNDEPLOY_3 );
	ADD_ACTIVITY_TO_SR( ACT_VM_UNDEPLOY_2 );
	ADD_ACTIVITY_TO_SR( ACT_VM_UNDEPLOY_1 );
	ADD_ACTIVITY_TO_SR( ACT_VM_UNDEPLOY_EMPTY );
	ADD_ACTIVITY_TO_SR( ACT_VM_DEPLOY );
	ADD_ACTIVITY_TO_SR( ACT_VM_DEPLOY_8 );
	ADD_ACTIVITY_TO_SR( ACT_VM_DEPLOY_7 );
	ADD_ACTIVITY_TO_SR( ACT_VM_DEPLOY_6 );
	ADD_ACTIVITY_TO_SR( ACT_VM_DEPLOY_5 );
	ADD_ACTIVITY_TO_SR( ACT_VM_DEPLOY_4 );
	ADD_ACTIVITY_TO_SR( ACT_VM_DEPLOY_3 );
	ADD_ACTIVITY_TO_SR( ACT_VM_DEPLOY_2 );
	ADD_ACTIVITY_TO_SR( ACT_VM_DEPLOY_1 );
	ADD_ACTIVITY_TO_SR( ACT_VM_DEPLOY_EMPTY );
	ADD_ACTIVITY_TO_SR( ACT_VM_PRIMARYATTACK_8 );
	ADD_ACTIVITY_TO_SR( ACT_VM_PRIMARYATTACK_7 );
	ADD_ACTIVITY_TO_SR( ACT_VM_PRIMARYATTACK_6 );
	ADD_ACTIVITY_TO_SR( ACT_VM_PRIMARYATTACK_5 );
	ADD_ACTIVITY_TO_SR( ACT_VM_PRIMARYATTACK_4 );
	ADD_ACTIVITY_TO_SR( ACT_VM_PRIMARYATTACK_3 );
	ADD_ACTIVITY_TO_SR( ACT_VM_PRIMARYATTACK_2 );
	ADD_ACTIVITY_TO_SR( ACT_VM_PRIMARYATTACK_1 );
	ADD_ACTIVITY_TO_SR( ACT_VM_PRIMARYATTACK_DEPLOYED );
	ADD_ACTIVITY_TO_SR( ACT_VM_PRIMARYATTACK_DEPLOYED_8 );
	ADD_ACTIVITY_TO_SR( ACT_VM_PRIMARYATTACK_DEPLOYED_7 );
	ADD_ACTIVITY_TO_SR( ACT_VM_PRIMARYATTACK_DEPLOYED_6 );
	ADD_ACTIVITY_TO_SR( ACT_VM_PRIMARYATTACK_DEPLOYED_5 );
	ADD_ACTIVITY_TO_SR( ACT_VM_PRIMARYATTACK_DEPLOYED_4 );
	ADD_ACTIVITY_TO_SR( ACT_VM_PRIMARYATTACK_DEPLOYED_3 );
	ADD_ACTIVITY_TO_SR( ACT_VM_PRIMARYATTACK_DEPLOYED_2 );
	ADD_ACTIVITY_TO_SR( ACT_VM_PRIMARYATTACK_DEPLOYED_1 );
	ADD_ACTIVITY_TO_SR( ACT_VM_PRIMARYATTACK_DEPLOYED_EMPTY );
	ADD_ACTIVITY_TO_SR( ACT_DOD_DEPLOYED );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONE_DEPLOYED );
	ADD_ACTIVITY_TO_SR( ACT_DOD_IDLE_ZOOMED );
	ADD_ACTIVITY_TO_SR( ACT_DOD_WALK_ZOOMED );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCH_ZOOMED );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCHWALK_ZOOMED );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONE_ZOOMED );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONE_FORWARD_ZOOMED );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_DEPLOYED );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_PRONE_DEPLOYED );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_DEPLOYED );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_PRONE_DEPLOYED );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_PRONE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_SECONDARYATTACK_PRONE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_CROUCH );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_PRONE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_STAND_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_STAND_AIM );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCH_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCH_AIM );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCHWALK_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCHWALK_AIM );
	ADD_ACTIVITY_TO_SR( ACT_DOD_WALK_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_WALK_AIM );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RUN_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RUN_AIM );
	ADD_ACTIVITY_TO_SR( ACT_DOD_STAND_AIM_PISTOL );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCH_AIM_PISTOL );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCHWALK_AIM_PISTOL );
	ADD_ACTIVITY_TO_SR( ACT_DOD_WALK_AIM_PISTOL );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RUN_AIM_PISTOL );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONE_AIM_PISTOL );
	ADD_ACTIVITY_TO_SR( ACT_DOD_STAND_IDLE_PISTOL );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCH_IDLE_PISTOL );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCHWALK_IDLE_PISTOL );
	ADD_ACTIVITY_TO_SR( ACT_DOD_WALK_IDLE_PISTOL );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RUN_IDLE_PISTOL );
	ADD_ACTIVITY_TO_SR( ACT_DOD_SPRINT_IDLE_PISTOL );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONEWALK_IDLE_PISTOL );
	ADD_ACTIVITY_TO_SR( ACT_DOD_STAND_AIM_C96 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCH_AIM_C96 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCHWALK_AIM_C96 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_WALK_AIM_C96 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RUN_AIM_C96 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONE_AIM_C96 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_STAND_IDLE_C96 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCH_IDLE_C96 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCHWALK_IDLE_C96 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_WALK_IDLE_C96 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RUN_IDLE_C96 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_SPRINT_IDLE_C96 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONEWALK_IDLE_C96 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_STAND_AIM_RIFLE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCH_AIM_RIFLE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCHWALK_AIM_RIFLE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_WALK_AIM_RIFLE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RUN_AIM_RIFLE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONE_AIM_RIFLE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_STAND_IDLE_RIFLE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCH_IDLE_RIFLE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCHWALK_IDLE_RIFLE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_WALK_IDLE_RIFLE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RUN_IDLE_RIFLE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_SPRINT_IDLE_RIFLE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONEWALK_IDLE_RIFLE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_STAND_AIM_BOLT );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCH_AIM_BOLT );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCHWALK_AIM_BOLT );
	ADD_ACTIVITY_TO_SR( ACT_DOD_WALK_AIM_BOLT );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RUN_AIM_BOLT );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONE_AIM_BOLT );
	ADD_ACTIVITY_TO_SR( ACT_DOD_STAND_IDLE_BOLT );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCH_IDLE_BOLT );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCHWALK_IDLE_BOLT );
	ADD_ACTIVITY_TO_SR( ACT_DOD_WALK_IDLE_BOLT );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RUN_IDLE_BOLT );
	ADD_ACTIVITY_TO_SR( ACT_DOD_SPRINT_IDLE_BOLT );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONEWALK_IDLE_BOLT );
	ADD_ACTIVITY_TO_SR( ACT_DOD_STAND_AIM_TOMMY );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCH_AIM_TOMMY );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCHWALK_AIM_TOMMY );
	ADD_ACTIVITY_TO_SR( ACT_DOD_WALK_AIM_TOMMY );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RUN_AIM_TOMMY );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONE_AIM_TOMMY );
	ADD_ACTIVITY_TO_SR( ACT_DOD_STAND_IDLE_TOMMY );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCH_IDLE_TOMMY );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCHWALK_IDLE_TOMMY );
	ADD_ACTIVITY_TO_SR( ACT_DOD_WALK_IDLE_TOMMY );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RUN_IDLE_TOMMY );
	ADD_ACTIVITY_TO_SR( ACT_DOD_SPRINT_IDLE_TOMMY );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONEWALK_IDLE_TOMMY );
	ADD_ACTIVITY_TO_SR( ACT_DOD_STAND_AIM_MP40 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCH_AIM_MP40 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCHWALK_AIM_MP40 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_WALK_AIM_MP40 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RUN_AIM_MP40 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONE_AIM_MP40 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_STAND_IDLE_MP40 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCH_IDLE_MP40 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCHWALK_IDLE_MP40 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_WALK_IDLE_MP40 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RUN_IDLE_MP40 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_SPRINT_IDLE_MP40 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONEWALK_IDLE_MP40 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_STAND_AIM_MP44 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCH_AIM_MP44 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCHWALK_AIM_MP44 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_WALK_AIM_MP44 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RUN_AIM_MP44 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONE_AIM_MP44 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_STAND_IDLE_MP44 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCH_IDLE_MP44 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCHWALK_IDLE_MP44 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_WALK_IDLE_MP44 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RUN_IDLE_MP44 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_SPRINT_IDLE_MP44 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONEWALK_IDLE_MP44 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_STAND_AIM_GREASE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCH_AIM_GREASE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCHWALK_AIM_GREASE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_WALK_AIM_GREASE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RUN_AIM_GREASE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONE_AIM_GREASE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_STAND_IDLE_GREASE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCH_IDLE_GREASE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCHWALK_IDLE_GREASE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_WALK_IDLE_GREASE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RUN_IDLE_GREASE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_SPRINT_IDLE_GREASE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONEWALK_IDLE_GREASE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_STAND_AIM_MG );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCH_AIM_MG );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCHWALK_AIM_MG );
	ADD_ACTIVITY_TO_SR( ACT_DOD_WALK_AIM_MG );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RUN_AIM_MG );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONE_AIM_MG );
	ADD_ACTIVITY_TO_SR( ACT_DOD_STAND_IDLE_MG );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCH_IDLE_MG );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCHWALK_IDLE_MG );
	ADD_ACTIVITY_TO_SR( ACT_DOD_WALK_IDLE_MG );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RUN_IDLE_MG );
	ADD_ACTIVITY_TO_SR( ACT_DOD_SPRINT_IDLE_MG );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONEWALK_IDLE_MG );
	ADD_ACTIVITY_TO_SR( ACT_DOD_STAND_AIM_30CAL );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCH_AIM_30CAL );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCHWALK_AIM_30CAL );
	ADD_ACTIVITY_TO_SR( ACT_DOD_WALK_AIM_30CAL );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RUN_AIM_30CAL );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONE_AIM_30CAL );
	ADD_ACTIVITY_TO_SR( ACT_DOD_STAND_IDLE_30CAL );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCH_IDLE_30CAL );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCHWALK_IDLE_30CAL );
	ADD_ACTIVITY_TO_SR( ACT_DOD_WALK_IDLE_30CAL );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RUN_IDLE_30CAL );
	ADD_ACTIVITY_TO_SR( ACT_DOD_SPRINT_IDLE_30CAL );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONEWALK_IDLE_30CAL );
	ADD_ACTIVITY_TO_SR( ACT_DOD_STAND_AIM_GREN_FRAG );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCH_AIM_GREN_FRAG );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCHWALK_AIM_GREN_FRAG );
	ADD_ACTIVITY_TO_SR( ACT_DOD_WALK_AIM_GREN_FRAG );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RUN_AIM_GREN_FRAG );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONE_AIM_GREN_FRAG );
	ADD_ACTIVITY_TO_SR( ACT_DOD_SPRINT_AIM_GREN_FRAG );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONEWALK_AIM_GREN_FRAG );
	ADD_ACTIVITY_TO_SR( ACT_DOD_STAND_AIM_GREN_STICK );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCH_AIM_GREN_STICK );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCHWALK_AIM_GREN_STICK );
	ADD_ACTIVITY_TO_SR( ACT_DOD_WALK_AIM_GREN_STICK );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RUN_AIM_GREN_STICK );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONE_AIM_GREN_STICK );
	ADD_ACTIVITY_TO_SR( ACT_DOD_SPRINT_AIM_GREN_STICK );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONEWALK_AIM_GREN_STICK );
	ADD_ACTIVITY_TO_SR( ACT_DOD_STAND_AIM_KNIFE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCH_AIM_KNIFE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCHWALK_AIM_KNIFE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_WALK_AIM_KNIFE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RUN_AIM_KNIFE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONE_AIM_KNIFE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_SPRINT_AIM_KNIFE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONEWALK_AIM_KNIFE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_STAND_AIM_SPADE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCH_AIM_SPADE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCHWALK_AIM_SPADE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_WALK_AIM_SPADE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RUN_AIM_SPADE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONE_AIM_SPADE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_SPRINT_AIM_SPADE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONEWALK_AIM_SPADE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_STAND_AIM_BAZOOKA );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCH_AIM_BAZOOKA );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCHWALK_AIM_BAZOOKA );
	ADD_ACTIVITY_TO_SR( ACT_DOD_WALK_AIM_BAZOOKA );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RUN_AIM_BAZOOKA );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONE_AIM_BAZOOKA );
	ADD_ACTIVITY_TO_SR( ACT_DOD_STAND_IDLE_BAZOOKA );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCH_IDLE_BAZOOKA );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCHWALK_IDLE_BAZOOKA );
	ADD_ACTIVITY_TO_SR( ACT_DOD_WALK_IDLE_BAZOOKA );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RUN_IDLE_BAZOOKA );
	ADD_ACTIVITY_TO_SR( ACT_DOD_SPRINT_IDLE_BAZOOKA );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONEWALK_IDLE_BAZOOKA );
	ADD_ACTIVITY_TO_SR( ACT_DOD_STAND_AIM_PSCHRECK );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCH_AIM_PSCHRECK );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCHWALK_AIM_PSCHRECK );
	ADD_ACTIVITY_TO_SR( ACT_DOD_WALK_AIM_PSCHRECK );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RUN_AIM_PSCHRECK );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONE_AIM_PSCHRECK );
	ADD_ACTIVITY_TO_SR( ACT_DOD_STAND_IDLE_PSCHRECK );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCH_IDLE_PSCHRECK );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCHWALK_IDLE_PSCHRECK );
	ADD_ACTIVITY_TO_SR( ACT_DOD_WALK_IDLE_PSCHRECK );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RUN_IDLE_PSCHRECK );
	ADD_ACTIVITY_TO_SR( ACT_DOD_SPRINT_IDLE_PSCHRECK );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONEWALK_IDLE_PSCHRECK );
	ADD_ACTIVITY_TO_SR( ACT_DOD_STAND_AIM_BAR );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCH_AIM_BAR );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCHWALK_AIM_BAR );
	ADD_ACTIVITY_TO_SR( ACT_DOD_WALK_AIM_BAR );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RUN_AIM_BAR );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONE_AIM_BAR );
	ADD_ACTIVITY_TO_SR( ACT_DOD_STAND_IDLE_BAR );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCH_IDLE_BAR );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCHWALK_IDLE_BAR );
	ADD_ACTIVITY_TO_SR( ACT_DOD_WALK_IDLE_BAR );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RUN_IDLE_BAR );
	ADD_ACTIVITY_TO_SR( ACT_DOD_SPRINT_IDLE_BAR );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONEWALK_IDLE_BAR );
	ADD_ACTIVITY_TO_SR( ACT_DOD_STAND_ZOOM_RIFLE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCH_ZOOM_RIFLE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCHWALK_ZOOM_RIFLE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_WALK_ZOOM_RIFLE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RUN_ZOOM_RIFLE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONE_ZOOM_RIFLE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_STAND_ZOOM_BOLT );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCH_ZOOM_BOLT );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCHWALK_ZOOM_BOLT );
	ADD_ACTIVITY_TO_SR( ACT_DOD_WALK_ZOOM_BOLT );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RUN_ZOOM_BOLT );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONE_ZOOM_BOLT );
	ADD_ACTIVITY_TO_SR( ACT_DOD_STAND_ZOOM_BAZOOKA );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCH_ZOOM_BAZOOKA );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCHWALK_ZOOM_BAZOOKA );
	ADD_ACTIVITY_TO_SR( ACT_DOD_WALK_ZOOM_BAZOOKA );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RUN_ZOOM_BAZOOKA );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONE_ZOOM_BAZOOKA );
	ADD_ACTIVITY_TO_SR( ACT_DOD_STAND_ZOOM_PSCHRECK );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCH_ZOOM_PSCHRECK );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCHWALK_ZOOM_PSCHRECK );
	ADD_ACTIVITY_TO_SR( ACT_DOD_WALK_ZOOM_PSCHRECK );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RUN_ZOOM_PSCHRECK );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONE_ZOOM_PSCHRECK );
	ADD_ACTIVITY_TO_SR( ACT_DOD_DEPLOY_RIFLE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_DEPLOY_TOMMY );
	ADD_ACTIVITY_TO_SR( ACT_DOD_DEPLOY_MG );
	ADD_ACTIVITY_TO_SR( ACT_DOD_DEPLOY_30CAL );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONE_DEPLOY_RIFLE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONE_DEPLOY_TOMMY );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONE_DEPLOY_MG );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONE_DEPLOY_30CAL );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_RIFLE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_SECONDARYATTACK_RIFLE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_PRONE_RIFLE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_SECONDARYATTACK_PRONE_RIFLE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_PRONE_DEPLOYED_RIFLE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_DEPLOYED_RIFLE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_BOLT );
	ADD_ACTIVITY_TO_SR( ACT_DOD_SECONDARYATTACK_BOLT );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_PRONE_BOLT );
	ADD_ACTIVITY_TO_SR( ACT_DOD_SECONDARYATTACK_PRONE_BOLT );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_TOMMY );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_PRONE_TOMMY );
	ADD_ACTIVITY_TO_SR( ACT_DOD_SECONDARYATTACK_TOMMY );
	ADD_ACTIVITY_TO_SR( ACT_DOD_SECONDARYATTACK_PRONE_TOMMY );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_MP40 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_PRONE_MP40 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_SECONDARYATTACK_MP40 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_SECONDARYATTACK_PRONE_MP40 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_MP44 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_PRONE_MP44 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_GREASE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_PRONE_GREASE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_PISTOL );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_PRONE_PISTOL );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_C96 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_PRONE_C96 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_MG );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_PRONE_MG );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_PRONE_DEPLOYED_MG );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_DEPLOYED_MG );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_30CAL );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_PRONE_30CAL );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_DEPLOYED_30CAL );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_PRONE_DEPLOYED_30CAL );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_GREN_FRAG );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_PRONE_GREN_FRAG );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_GREN_STICK );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_PRONE_GREN_STICK );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_KNIFE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_PRONE_KNIFE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_SPADE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_PRONE_SPADE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_BAZOOKA );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_PRONE_BAZOOKA );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_PSCHRECK );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_PRONE_PSCHRECK );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_BAR );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_PRONE_BAR );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_GARAND );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_K43 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_BAR );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_MP40 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_MP44 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_BOLT );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_M1CARBINE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_TOMMY );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_GREASEGUN );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_PISTOL );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_FG42 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_RIFLE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_RIFLEGRENADE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_C96 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_CROUCH_BAR );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_CROUCH_RIFLE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_CROUCH_RIFLEGRENADE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_CROUCH_BOLT );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_CROUCH_MP44 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_CROUCH_MP40 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_CROUCH_TOMMY );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_CROUCH_BAZOOKA );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_CROUCH_PSCHRECK );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_CROUCH_PISTOL );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_CROUCH_M1CARBINE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_CROUCH_C96 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_BAZOOKA );
	ADD_ACTIVITY_TO_SR( ACT_DOD_ZOOMLOAD_BAZOOKA );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_PSCHRECK );
	ADD_ACTIVITY_TO_SR( ACT_DOD_ZOOMLOAD_PSCHRECK );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_DEPLOYED_FG42 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_DEPLOYED_30CAL );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_DEPLOYED_MG );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_DEPLOYED_MG34 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_DEPLOYED_BAR );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_PRONE_PISTOL );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_PRONE_GARAND );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_PRONE_M1CARBINE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_PRONE_BOLT );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_PRONE_K43 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_PRONE_MP40 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_PRONE_MP44 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_PRONE_BAR );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_PRONE_GREASEGUN );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_PRONE_TOMMY );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_PRONE_FG42 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_PRONE_RIFLE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_PRONE_RIFLEGRENADE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_PRONE_C96 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_PRONE_BAZOOKA );
	ADD_ACTIVITY_TO_SR( ACT_DOD_ZOOMLOAD_PRONE_BAZOOKA );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_PRONE_PSCHRECK );
	ADD_ACTIVITY_TO_SR( ACT_DOD_ZOOMLOAD_PRONE_PSCHRECK );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_PRONE_DEPLOYED_BAR );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_PRONE_DEPLOYED_FG42 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_PRONE_DEPLOYED_30CAL );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_PRONE_DEPLOYED_MG );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RELOAD_PRONE_DEPLOYED_MG34 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONE_ZOOM_FORWARD_RIFLE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONE_ZOOM_FORWARD_BOLT );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONE_ZOOM_FORWARD_BAZOOKA );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONE_ZOOM_FORWARD_PSCHRECK );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_CROUCH );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_CROUCH_SPADE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_CROUCH_KNIFE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_CROUCH_GREN_FRAG );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRIMARYATTACK_CROUCH_GREN_STICK );
	ADD_ACTIVITY_TO_SR( ACT_DOD_SECONDARYATTACK_CROUCH );
	ADD_ACTIVITY_TO_SR( ACT_DOD_SECONDARYATTACK_CROUCH_TOMMY );
	ADD_ACTIVITY_TO_SR( ACT_DOD_SECONDARYATTACK_CROUCH_MP40 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_HS_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_HS_CROUCH );
	ADD_ACTIVITY_TO_SR( ACT_DOD_HS_IDLE_30CAL );
	ADD_ACTIVITY_TO_SR( ACT_DOD_HS_IDLE_BAZOOKA );
	ADD_ACTIVITY_TO_SR( ACT_DOD_HS_IDLE_PSCHRECK );
	ADD_ACTIVITY_TO_SR( ACT_DOD_HS_IDLE_KNIFE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_HS_IDLE_MG42 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_HS_IDLE_PISTOL );
	ADD_ACTIVITY_TO_SR( ACT_DOD_HS_IDLE_STICKGRENADE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_HS_IDLE_TOMMY );
	ADD_ACTIVITY_TO_SR( ACT_DOD_HS_IDLE_MP44 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_HS_IDLE_K98 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_HS_CROUCH_30CAL );
	ADD_ACTIVITY_TO_SR( ACT_DOD_HS_CROUCH_BAZOOKA );
	ADD_ACTIVITY_TO_SR( ACT_DOD_HS_CROUCH_PSCHRECK );
	ADD_ACTIVITY_TO_SR( ACT_DOD_HS_CROUCH_KNIFE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_HS_CROUCH_MG42 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_HS_CROUCH_PISTOL );
	ADD_ACTIVITY_TO_SR( ACT_DOD_HS_CROUCH_STICKGRENADE );
	ADD_ACTIVITY_TO_SR( ACT_DOD_HS_CROUCH_TOMMY );
	ADD_ACTIVITY_TO_SR( ACT_DOD_HS_CROUCH_MP44 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_HS_CROUCH_K98 );
	ADD_ACTIVITY_TO_SR( ACT_DOD_STAND_IDLE_TNT );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCH_IDLE_TNT );
	ADD_ACTIVITY_TO_SR( ACT_DOD_CROUCHWALK_IDLE_TNT );
	ADD_ACTIVITY_TO_SR( ACT_DOD_WALK_IDLE_TNT );
	ADD_ACTIVITY_TO_SR( ACT_DOD_RUN_IDLE_TNT );
	ADD_ACTIVITY_TO_SR( ACT_DOD_SPRINT_IDLE_TNT );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PRONEWALK_IDLE_TNT );
	ADD_ACTIVITY_TO_SR( ACT_DOD_PLANT_TNT );
	ADD_ACTIVITY_TO_SR( ACT_DOD_DEFUSE_TNT );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_RUN );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_IDLE_CROUCH );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_WALK_CROUCH );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_GESTURE_RANGE_ATTACK );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_GESTURE_RELOAD );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_JUMP );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_IDLE_PISTOL );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_RUN_PISTOL );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_IDLE_CROUCH_PISTOL );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_WALK_CROUCH_PISTOL );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_GESTURE_RANGE_ATTACK_PISTOL );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_GESTURE_RELOAD_PISTOL );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_JUMP_PISTOL );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_IDLE_SMG1 );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_RUN_SMG1 );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_IDLE_CROUCH_SMG1 );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_WALK_CROUCH_SMG1 );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_GESTURE_RANGE_ATTACK_SMG1 );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_GESTURE_RELOAD_SMG1 );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_JUMP_SMG1 );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_IDLE_AR2 );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_RUN_AR2 );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_IDLE_CROUCH_AR2 );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_WALK_CROUCH_AR2 );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_GESTURE_RANGE_ATTACK_AR2 );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_GESTURE_RELOAD_AR2 );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_JUMP_AR2 );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_IDLE_SHOTGUN );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_RUN_SHOTGUN );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_IDLE_CROUCH_SHOTGUN );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_WALK_CROUCH_SHOTGUN );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_GESTURE_RANGE_ATTACK_SHOTGUN );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_GESTURE_RELOAD_SHOTGUN );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_JUMP_SHOTGUN );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_IDLE_RPG );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_RUN_RPG );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_IDLE_CROUCH_RPG );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_WALK_CROUCH_RPG );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_GESTURE_RANGE_ATTACK_RPG );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_GESTURE_RELOAD_RPG );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_JUMP_RPG );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_IDLE_GRENADE );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_RUN_GRENADE );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_IDLE_CROUCH_GRENADE );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_WALK_CROUCH_GRENADE );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_GESTURE_RANGE_ATTACK_GRENADE );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_GESTURE_RELOAD_GRENADE );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_JUMP_GRENADE );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_IDLE_PHYSGUN );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_RUN_PHYSGUN );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_IDLE_CROUCH_PHYSGUN );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_WALK_CROUCH_PHYSGUN );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_GESTURE_RANGE_ATTACK_PHYSGUN );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_GESTURE_RELOAD_PHYSGUN );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_JUMP_PHYSGUN );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_IDLE_CROSSBOW );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_RUN_CROSSBOW );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_IDLE_CROUCH_CROSSBOW );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_WALK_CROUCH_CROSSBOW );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_GESTURE_RANGE_ATTACK_CROSSBOW );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_GESTURE_RELOAD_CROSSBOW );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_JUMP_CROSSBOW );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_IDLE_MELEE );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_RUN_MELEE );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_IDLE_CROUCH_MELEE );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_WALK_CROUCH_MELEE );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_GESTURE_RANGE_ATTACK_MELEE );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_GESTURE_RELOAD_MELEE );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_JUMP_MELEE );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_IDLE_SLAM );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_RUN_SLAM );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_IDLE_CROUCH_SLAM );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_WALK_CROUCH_SLAM );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_GESTURE_RANGE_ATTACK_SLAM );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_GESTURE_RELOAD_SLAM );
	ADD_ACTIVITY_TO_SR( ACT_HL2MP_JUMP_SLAM );
	ADD_ACTIVITY_TO_SR( ACT_VM_FIZZLE );
	ADD_ACTIVITY_TO_SR( ACT_MP_STAND_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_MP_CROUCH_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_MP_CROUCH_DEPLOYED_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_MP_CROUCH_DEPLOYED );
	ADD_ACTIVITY_TO_SR( ACT_MP_CROUCHWALK_DEPLOYED );
	ADD_ACTIVITY_TO_SR( ACT_MP_DEPLOYED_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_MP_RUN );
	ADD_ACTIVITY_TO_SR( ACT_MP_WALK );
	ADD_ACTIVITY_TO_SR( ACT_MP_AIRWALK );
	ADD_ACTIVITY_TO_SR( ACT_MP_CROUCHWALK );
	ADD_ACTIVITY_TO_SR( ACT_MP_SPRINT );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_START );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_FLOAT );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_LAND );
	ADD_ACTIVITY_TO_SR( ACT_MP_DOUBLEJUMP );
	ADD_ACTIVITY_TO_SR( ACT_MP_SWIM );
	ADD_ACTIVITY_TO_SR( ACT_MP_DEPLOYED );
	ADD_ACTIVITY_TO_SR( ACT_MP_SWIM_DEPLOYED );
	ADD_ACTIVITY_TO_SR( ACT_MP_VCD );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_STAND_PRIMARYFIRE );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_STAND_PRIMARYFIRE_DEPLOYED );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_STAND_SECONDARYFIRE );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_STAND_GRENADE );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_CROUCH_PRIMARYFIRE );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_CROUCH_PRIMARYFIRE_DEPLOYED );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_CROUCH_SECONDARYFIRE );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_CROUCH_GRENADE );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_SWIM_PRIMARYFIRE );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_SWIM_SECONDARYFIRE );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_SWIM_GRENADE );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_AIRWALK_PRIMARYFIRE );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_AIRWALK_SECONDARYFIRE );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_AIRWALK_GRENADE );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_STAND );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_STAND_LOOP );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_STAND_END );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_CROUCH );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_CROUCH_LOOP );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_CROUCH_END );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_SWIM );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_SWIM_LOOP );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_SWIM_END );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_AIRWALK );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_AIRWALK_LOOP );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_AIRWALK_END );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_STAND_PREFIRE );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_STAND_POSTFIRE );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_STAND_STARTFIRE );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_CROUCH_PREFIRE );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_CROUCH_POSTFIRE );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_SWIM_PREFIRE );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_SWIM_POSTFIRE );
	ADD_ACTIVITY_TO_SR( ACT_MP_STAND_PRIMARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_CROUCH_PRIMARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_RUN_PRIMARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_WALK_PRIMARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_AIRWALK_PRIMARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_CROUCHWALK_PRIMARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_PRIMARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_START_PRIMARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_FLOAT_PRIMARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_LAND_PRIMARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_SWIM_PRIMARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_DEPLOYED_PRIMARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_SWIM_DEPLOYED_PRIMARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_CROUCHWALK_DEPLOYED_PRIMARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_CROUCH_DEPLOYED_IDLE_PRIMARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_STAND_PRIMARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_STAND_PRIMARY_DEPLOYED );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_CROUCH_PRIMARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_CROUCH_PRIMARY_DEPLOYED );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_SWIM_PRIMARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_AIRWALK_PRIMARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_STAND_PRIMARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_STAND_PRIMARY_LOOP );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_STAND_PRIMARY_END );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_CROUCH_PRIMARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_CROUCH_PRIMARY_LOOP );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_CROUCH_PRIMARY_END );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_SWIM_PRIMARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_SWIM_PRIMARY_LOOP );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_SWIM_PRIMARY_END );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_AIRWALK_PRIMARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_AIRWALK_PRIMARY_LOOP );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_AIRWALK_PRIMARY_END );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_STAND_PRIMARY_2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_STAND_PRIMARY_LOOP_2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_STAND_PRIMARY_END_2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_CROUCH_PRIMARY_2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_CROUCH_PRIMARY_LOOP_2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_CROUCH_PRIMARY_END_2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_SWIM_PRIMARY_2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_SWIM_PRIMARY_LOOP_2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_SWIM_PRIMARY_END_2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_AIRWALK_PRIMARY_2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_AIRWALK_PRIMARY_LOOP_2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_AIRWALK_PRIMARY_END_2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_STAND_PRIMARY_ALT );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_CROUCH_PRIMARY_ALT );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_SWIM_PRIMARY_ALT );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_STAND_PRIMARY_ALT );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_CROUCH_PRIMARY_ALT );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_AIRWALK_PRIMARY_ALT );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_STAND_PRIMARY_LOOP_ALT );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_CROUCH_PRIMARY_LOOP_ALT );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_AIRWALK_PRIMARY_LOOP_ALT );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_STAND_PRIMARY_END_ALT );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_CROUCH_PRIMARY_END_ALT );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_AIRWALK_PRIMARY_END_ALT );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_SWIM_PRIMARY_ALT );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_STAND_PRIMARY_SUPER );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_CROUCH_PRIMARY_SUPER );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_SWIM_PRIMARY_SUPER );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_STAND_GRENADE_PRIMARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_CROUCH_GRENADE_PRIMARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_SWIM_GRENADE_PRIMARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_AIRWALK_GRENADE_PRIMARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_STAND_SECONDARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_CROUCH_SECONDARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_RUN_SECONDARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_WALK_SECONDARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_AIRWALK_SECONDARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_CROUCHWALK_SECONDARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_SECONDARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_START_SECONDARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_FLOAT_SECONDARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_LAND_SECONDARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_SWIM_SECONDARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_STAND_SECONDARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_CROUCH_SECONDARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_SWIM_SECONDARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_AIRWALK_SECONDARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_STAND_SECONDARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_STAND_SECONDARY_LOOP );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_STAND_SECONDARY_END );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_CROUCH_SECONDARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_CROUCH_SECONDARY_LOOP );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_CROUCH_SECONDARY_END );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_SWIM_SECONDARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_SWIM_SECONDARY_LOOP );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_SWIM_SECONDARY_END );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_AIRWALK_SECONDARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_AIRWALK_SECONDARY_LOOP );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_AIRWALK_SECONDARY_END );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_STAND_SECONDARY_2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_CROUCH_SECONDARY_2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_SWIM_SECONDARY_2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_AIRWALK_SECONDARY_2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_STAND_GRENADE_SECONDARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_CROUCH_GRENADE_SECONDARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_SWIM_GRENADE_SECONDARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_AIRWALK_GRENADE_SECONDARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_STAND_SECONDARY2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_CROUCH_SECONDARY2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_RUN_SECONDARY2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_WALK_SECONDARY2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_AIRWALK_SECONDARY2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_CROUCHWALK_SECONDARY2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_SECONDARY2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_START_SECONDARY2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_FLOAT_SECONDARY2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_LAND_SECONDARY2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_SWIM_SECONDARY2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_STAND_SECONDARY2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_CROUCH_SECONDARY2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_SWIM_SECONDARY2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_AIRWALK_SECONDARY2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_STAND_SECONDARY2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_STAND_SECONDARY2_LOOP );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_STAND_SECONDARY2_END );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_CROUCH_SECONDARY2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_CROUCH_SECONDARY2_LOOP );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_CROUCH_SECONDARY2_END );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_SWIM_SECONDARY2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_SWIM_SECONDARY2_LOOP );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_SWIM_SECONDARY2_END );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_AIRWALK_SECONDARY2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_AIRWALK_SECONDARY2_LOOP );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_AIRWALK_SECONDARY2_END );
	ADD_ACTIVITY_TO_SR( ACT_MP_STAND_MELEE );
	ADD_ACTIVITY_TO_SR( ACT_MP_CROUCH_MELEE );
	ADD_ACTIVITY_TO_SR( ACT_MP_RUN_MELEE );
	ADD_ACTIVITY_TO_SR( ACT_MP_WALK_MELEE );
	ADD_ACTIVITY_TO_SR( ACT_MP_AIRWALK_MELEE );
	ADD_ACTIVITY_TO_SR( ACT_MP_CROUCHWALK_MELEE );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_MELEE );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_START_MELEE );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_FLOAT_MELEE );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_LAND_MELEE );
	ADD_ACTIVITY_TO_SR( ACT_MP_SWIM_MELEE );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_STAND_MELEE );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_STAND_MELEE_SECONDARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_CROUCH_MELEE );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_CROUCH_MELEE_SECONDARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_SWIM_MELEE );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_AIRWALK_MELEE );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_STAND_GRENADE_MELEE );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_CROUCH_GRENADE_MELEE );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_SWIM_GRENADE_MELEE );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_AIRWALK_GRENADE_MELEE );
	ADD_ACTIVITY_TO_SR( ACT_MP_STAND_ITEM1 );
	ADD_ACTIVITY_TO_SR( ACT_MP_CROUCH_ITEM1 );
	ADD_ACTIVITY_TO_SR( ACT_MP_RUN_ITEM1 );
	ADD_ACTIVITY_TO_SR( ACT_MP_WALK_ITEM1 );
	ADD_ACTIVITY_TO_SR( ACT_MP_AIRWALK_ITEM1 );
	ADD_ACTIVITY_TO_SR( ACT_MP_CROUCHWALK_ITEM1 );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_ITEM1 );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_START_ITEM1 );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_FLOAT_ITEM1 );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_LAND_ITEM1 );
	ADD_ACTIVITY_TO_SR( ACT_MP_SWIM_ITEM1 );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_STAND_ITEM1 );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_STAND_ITEM1_SECONDARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_CROUCH_ITEM1 );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_CROUCH_ITEM1_SECONDARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_SWIM_ITEM1 );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_AIRWALK_ITEM1 );
	ADD_ACTIVITY_TO_SR( ACT_MP_DEPLOYED_ITEM1 );
	ADD_ACTIVITY_TO_SR( ACT_MP_DEPLOYED_IDLE_ITEM1 );
	ADD_ACTIVITY_TO_SR( ACT_MP_CROUCHWALK_DEPLOYED_ITEM1 );
	ADD_ACTIVITY_TO_SR( ACT_MP_CROUCH_DEPLOYED_IDLE_ITEM1 );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_STAND_PRIMARY_DEPLOYED_ITEM1 );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_CROUCH_PRIMARY_DEPLOYED_ITEM1 );
	ADD_ACTIVITY_TO_SR( ACT_MP_STAND_ITEM2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_CROUCH_ITEM2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_RUN_ITEM2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_WALK_ITEM2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_AIRWALK_ITEM2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_CROUCHWALK_ITEM2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_ITEM2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_START_ITEM2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_FLOAT_ITEM2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_LAND_ITEM2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_SWIM_ITEM2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_STAND_ITEM2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_STAND_ITEM2_SECONDARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_CROUCH_ITEM2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_CROUCH_ITEM2_SECONDARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_SWIM_ITEM2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_AIRWALK_ITEM2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_STAND_HARD_ITEM2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_CROUCH_HARD_ITEM2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_SWIM_HARD_ITEM2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_DEPLOYED_ITEM2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_DEPLOYED_IDLE_ITEM2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_CROUCHWALK_DEPLOYED_ITEM2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_CROUCH_DEPLOYED_IDLE_ITEM2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_STAND_PRIMARY_DEPLOYED_ITEM2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_CROUCH_PRIMARY_DEPLOYED_ITEM2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_STAND_ITEM2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_STAND_ITEM2_LOOP );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_STAND_ITEM2_END );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_CROUCH_ITEM2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_CROUCH_ITEM2_LOOP );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_CROUCH_ITEM2_END );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_SWIM_ITEM2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_SWIM_ITEM2_LOOP );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_SWIM_ITEM2_END );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_AIRWALK_ITEM2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_AIRWALK_ITEM2_LOOP );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_AIRWALK_ITEM2_END );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_NO_AMMO_ITEM2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_STAND_GRENADE_ITEM2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_CROUCH_GRENADE_ITEM2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_SWIM_GRENADE_ITEM2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_AIRWALK_GRENADE_ITEM2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_FLINCH );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_FLINCH_PRIMARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_FLINCH_SECONDARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_FLINCH_MELEE );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_FLINCH_ITEM1 );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_FLINCH_ITEM2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_FLINCH_HEAD );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_FLINCH_CHEST );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_FLINCH_STOMACH );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_FLINCH_LEFTARM );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_FLINCH_RIGHTARM );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_FLINCH_LEFTLEG );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_FLINCH_RIGHTLEG );
	ADD_ACTIVITY_TO_SR( ACT_MP_GRENADE1_DRAW );
	ADD_ACTIVITY_TO_SR( ACT_MP_GRENADE1_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_MP_GRENADE1_ATTACK );
	ADD_ACTIVITY_TO_SR( ACT_MP_GRENADE2_DRAW );
	ADD_ACTIVITY_TO_SR( ACT_MP_GRENADE2_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_MP_GRENADE2_ATTACK );
	ADD_ACTIVITY_TO_SR( ACT_MP_PRIMARY_GRENADE1_DRAW );
	ADD_ACTIVITY_TO_SR( ACT_MP_PRIMARY_GRENADE1_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_MP_PRIMARY_GRENADE1_ATTACK );
	ADD_ACTIVITY_TO_SR( ACT_MP_PRIMARY_GRENADE2_DRAW );
	ADD_ACTIVITY_TO_SR( ACT_MP_PRIMARY_GRENADE2_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_MP_PRIMARY_GRENADE2_ATTACK );
	ADD_ACTIVITY_TO_SR( ACT_MP_SECONDARY_GRENADE1_DRAW );
	ADD_ACTIVITY_TO_SR( ACT_MP_SECONDARY_GRENADE1_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_MP_SECONDARY_GRENADE1_ATTACK );
	ADD_ACTIVITY_TO_SR( ACT_MP_SECONDARY_GRENADE2_DRAW );
	ADD_ACTIVITY_TO_SR( ACT_MP_SECONDARY_GRENADE2_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_MP_SECONDARY_GRENADE2_ATTACK );
	ADD_ACTIVITY_TO_SR( ACT_MP_MELEE_GRENADE1_DRAW );
	ADD_ACTIVITY_TO_SR( ACT_MP_MELEE_GRENADE1_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_MP_MELEE_GRENADE1_ATTACK );
	ADD_ACTIVITY_TO_SR( ACT_MP_MELEE_GRENADE2_DRAW );
	ADD_ACTIVITY_TO_SR( ACT_MP_MELEE_GRENADE2_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_MP_MELEE_GRENADE2_ATTACK );
	ADD_ACTIVITY_TO_SR( ACT_MP_ITEM1_GRENADE1_DRAW );
	ADD_ACTIVITY_TO_SR( ACT_MP_ITEM1_GRENADE1_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_MP_ITEM1_GRENADE1_ATTACK );
	ADD_ACTIVITY_TO_SR( ACT_MP_ITEM1_GRENADE2_DRAW );
	ADD_ACTIVITY_TO_SR( ACT_MP_ITEM1_GRENADE2_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_MP_ITEM1_GRENADE2_ATTACK );
	ADD_ACTIVITY_TO_SR( ACT_MP_ITEM2_GRENADE1_DRAW );
	ADD_ACTIVITY_TO_SR( ACT_MP_ITEM2_GRENADE1_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_MP_ITEM2_GRENADE1_ATTACK );
	ADD_ACTIVITY_TO_SR( ACT_MP_ITEM2_GRENADE2_DRAW );
	ADD_ACTIVITY_TO_SR( ACT_MP_ITEM2_GRENADE2_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_MP_ITEM2_GRENADE2_ATTACK );
	ADD_ACTIVITY_TO_SR( ACT_MP_STAND_BUILDING );
	ADD_ACTIVITY_TO_SR( ACT_MP_CROUCH_BUILDING );
	ADD_ACTIVITY_TO_SR( ACT_MP_RUN_BUILDING );
	ADD_ACTIVITY_TO_SR( ACT_MP_WALK_BUILDING );
	ADD_ACTIVITY_TO_SR( ACT_MP_AIRWALK_BUILDING );
	ADD_ACTIVITY_TO_SR( ACT_MP_CROUCHWALK_BUILDING );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_BUILDING );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_START_BUILDING );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_FLOAT_BUILDING );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_LAND_BUILDING );
	ADD_ACTIVITY_TO_SR( ACT_MP_SWIM_BUILDING );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_STAND_BUILDING );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_CROUCH_BUILDING );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_SWIM_BUILDING );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_AIRWALK_BUILDING );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_STAND_GRENADE_BUILDING );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_CROUCH_GRENADE_BUILDING );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_SWIM_GRENADE_BUILDING );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_AIRWALK_GRENADE_BUILDING );
	ADD_ACTIVITY_TO_SR( ACT_MP_STAND_BUILDING_DEPLOYED );
	ADD_ACTIVITY_TO_SR( ACT_MP_CROUCH_BUILDING_DEPLOYED );
	ADD_ACTIVITY_TO_SR( ACT_MP_RUN_BUILDING_DEPLOYED );
	ADD_ACTIVITY_TO_SR( ACT_MP_WALK_BUILDING_DEPLOYED );
	ADD_ACTIVITY_TO_SR( ACT_MP_AIRWALK_BUILDING_DEPLOYED );
	ADD_ACTIVITY_TO_SR( ACT_MP_CROUCHWALK_BUILDING_DEPLOYED );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_BUILDING_DEPLOYED );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_START_BUILDING_DEPLOYED );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_FLOAT_BUILDING_DEPLOYED );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_LAND_BUILDING_DEPLOYED );
	ADD_ACTIVITY_TO_SR( ACT_MP_SWIM_BUILDING_DEPLOYED );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_STAND_BUILDING_DEPLOYED );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_CROUCH_BUILDING_DEPLOYED );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_SWIM_BUILDING_DEPLOYED );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_AIRWALK_BUILDING_DEPLOYED );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_STAND_GRENADE_BUILDING_DEPLOYED );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_CROUCH_GRENADE_BUILDING_DEPLOYED );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_SWIM_GRENADE_BUILDING_DEPLOYED );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_AIRWALK_GRENADE_BUILDING_DEPLOYED );
	ADD_ACTIVITY_TO_SR( ACT_MP_STAND_PDA );
	ADD_ACTIVITY_TO_SR( ACT_MP_CROUCH_PDA );
	ADD_ACTIVITY_TO_SR( ACT_MP_RUN_PDA );
	ADD_ACTIVITY_TO_SR( ACT_MP_WALK_PDA );
	ADD_ACTIVITY_TO_SR( ACT_MP_AIRWALK_PDA );
	ADD_ACTIVITY_TO_SR( ACT_MP_CROUCHWALK_PDA );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_PDA );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_START_PDA );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_FLOAT_PDA );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_LAND_PDA );
	ADD_ACTIVITY_TO_SR( ACT_MP_SWIM_PDA );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_STAND_PDA );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_SWIM_PDA );
	ADD_ACTIVITY_TO_SR( ACT_MP_STAND_LOSERSTATE );
	ADD_ACTIVITY_TO_SR( ACT_MP_CROUCH_LOSERSTATE );
	ADD_ACTIVITY_TO_SR( ACT_MP_RUN_LOSERSTATE );
	ADD_ACTIVITY_TO_SR( ACT_MP_WALK_LOSERSTATE );
	ADD_ACTIVITY_TO_SR( ACT_MP_AIRWALK_LOSERSTATE );
	ADD_ACTIVITY_TO_SR( ACT_MP_CROUCHWALK_LOSERSTATE );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_LOSERSTATE );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_START_LOSERSTATE );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_FLOAT_LOSERSTATE );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_LAND_LOSERSTATE );
	ADD_ACTIVITY_TO_SR( ACT_MP_SWIM_LOSERSTATE );
	ADD_ACTIVITY_TO_SR( ACT_MP_DOUBLEJUMP_LOSERSTATE );
	ADD_ACTIVITY_TO_SR( ACT_MP_DOUBLEJUMP_CROUCH );
	ADD_ACTIVITY_TO_SR( ACT_MP_DOUBLEJUMP_CROUCH_PRIMARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_DOUBLEJUMP_CROUCH_SECONDARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_DOUBLEJUMP_CROUCH_MELEE );
	ADD_ACTIVITY_TO_SR( ACT_MP_DOUBLEJUMP_CROUCH_ITEM1 );
	ADD_ACTIVITY_TO_SR( ACT_MP_DOUBLEJUMP_CROUCH_ITEM2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_DOUBLEJUMP_CROUCH_LOSERSTATE );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_HANDMOUTH );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_FINGERPOINT );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_FISTPUMP );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_THUMBSUP );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_NODYES );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_NODNO );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_HANDMOUTH_PRIMARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_FINGERPOINT_PRIMARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_FISTPUMP_PRIMARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_THUMBSUP_PRIMARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_NODYES_PRIMARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_NODNO_PRIMARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_HANDMOUTH_SECONDARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_FINGERPOINT_SECONDARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_FISTPUMP_SECONDARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_THUMBSUP_SECONDARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_NODYES_SECONDARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_NODNO_SECONDARY );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_HANDMOUTH_MELEE );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_FINGERPOINT_MELEE );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_FISTPUMP_MELEE );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_THUMBSUP_MELEE );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_NODYES_MELEE );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_NODNO_MELEE );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_HANDMOUTH_ITEM1 );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_FINGERPOINT_ITEM1 );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_FISTPUMP_ITEM1 );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_THUMBSUP_ITEM1 );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_NODYES_ITEM1 );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_NODNO_ITEM1 );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_HANDMOUTH_ITEM2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_FINGERPOINT_ITEM2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_FISTPUMP_ITEM2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_THUMBSUP_ITEM2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_NODYES_ITEM2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_NODNO_ITEM2 );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_HANDMOUTH_BUILDING );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_FINGERPOINT_BUILDING );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_FISTPUMP_BUILDING );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_THUMBSUP_BUILDING );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_NODYES_BUILDING );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_NODNO_BUILDING );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_HANDMOUTH_PDA );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_FINGERPOINT_PDA );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_FISTPUMP_PDA );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_THUMBSUP_PDA );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_NODYES_PDA );
	ADD_ACTIVITY_TO_SR( ACT_MP_GESTURE_VC_NODNO_PDA );
	ADD_ACTIVITY_TO_SR( ACT_MP_STUN_BEGIN );
	ADD_ACTIVITY_TO_SR( ACT_MP_STUN_MIDDLE );
	ADD_ACTIVITY_TO_SR( ACT_MP_STUN_END );
	ADD_ACTIVITY_TO_SR( ACT_VM_UNUSABLE );
	ADD_ACTIVITY_TO_SR( ACT_VM_UNUSABLE_TO_USABLE );
	ADD_ACTIVITY_TO_SR( ACT_VM_USABLE_TO_UNUSABLE );
	ADD_ACTIVITY_TO_SR( ACT_PRIMARY_VM_DRAW );
	ADD_ACTIVITY_TO_SR( ACT_PRIMARY_VM_HOLSTER );
	ADD_ACTIVITY_TO_SR( ACT_PRIMARY_VM_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_PRIMARY_VM_PULLBACK );
	ADD_ACTIVITY_TO_SR( ACT_PRIMARY_VM_PRIMARYATTACK );
	ADD_ACTIVITY_TO_SR( ACT_PRIMARY_VM_SECONDARYATTACK );
	ADD_ACTIVITY_TO_SR( ACT_PRIMARY_VM_RELOAD );
	ADD_ACTIVITY_TO_SR( ACT_PRIMARY_RELOAD_START );
	ADD_ACTIVITY_TO_SR( ACT_PRIMARY_RELOAD_FINISH );
	ADD_ACTIVITY_TO_SR( ACT_PRIMARY_VM_DRYFIRE );
	ADD_ACTIVITY_TO_SR( ACT_PRIMARY_VM_IDLE_TO_LOWERED );
	ADD_ACTIVITY_TO_SR( ACT_PRIMARY_VM_IDLE_LOWERED );
	ADD_ACTIVITY_TO_SR( ACT_PRIMARY_VM_LOWERED_TO_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_PRIMARY_VM_RELOAD_2 );
	ADD_ACTIVITY_TO_SR( ACT_PRIMARY_RELOAD_START_2 );
	ADD_ACTIVITY_TO_SR( ACT_PRIMARY_RELOAD_FINISH_2 );
	ADD_ACTIVITY_TO_SR( ACT_PRIMARY_VM_RELOAD_3 );
	ADD_ACTIVITY_TO_SR( ACT_PRIMARY_RELOAD_START_3 );
	ADD_ACTIVITY_TO_SR( ACT_PRIMARY_RELOAD_FINISH_3 );
	ADD_ACTIVITY_TO_SR( ACT_PRIMARY_VM_PRIMARYATTACK_3 );
	ADD_ACTIVITY_TO_SR( ACT_SECONDARY_VM_DRAW );
	ADD_ACTIVITY_TO_SR( ACT_SECONDARY_VM_HOLSTER );
	ADD_ACTIVITY_TO_SR( ACT_SECONDARY_VM_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_SECONDARY_VM_PULLBACK );
	ADD_ACTIVITY_TO_SR( ACT_SECONDARY_VM_PRIMARYATTACK );
	ADD_ACTIVITY_TO_SR( ACT_SECONDARY_VM_SECONDARYATTACK );
	ADD_ACTIVITY_TO_SR( ACT_SECONDARY_VM_RELOAD );
	ADD_ACTIVITY_TO_SR( ACT_SECONDARY_RELOAD_START );
	ADD_ACTIVITY_TO_SR( ACT_SECONDARY_RELOAD_FINISH );
	ADD_ACTIVITY_TO_SR( ACT_SECONDARY_VM_RELOAD2 );
	ADD_ACTIVITY_TO_SR( ACT_SECONDARY_VM_DRYFIRE );
	ADD_ACTIVITY_TO_SR( ACT_SECONDARY_VM_IDLE_TO_LOWERED );
	ADD_ACTIVITY_TO_SR( ACT_SECONDARY_VM_IDLE_LOWERED );
	ADD_ACTIVITY_TO_SR( ACT_SECONDARY_VM_LOWERED_TO_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_SECONDARY_VM_DRAW_2 );
	ADD_ACTIVITY_TO_SR( ACT_SECONDARY_VM_IDLE_2 );
	ADD_ACTIVITY_TO_SR( ACT_SECONDARY_VM_PRIMARYATTACK_2 );
	ADD_ACTIVITY_TO_SR( ACT_SECONDARY_VM_RELOAD_2 );
	ADD_ACTIVITY_TO_SR( ACT_MELEE_VM_DRAW );
	ADD_ACTIVITY_TO_SR( ACT_MELEE_VM_HOLSTER );
	ADD_ACTIVITY_TO_SR( ACT_MELEE_VM_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_MELEE_VM_PULLBACK );
	ADD_ACTIVITY_TO_SR( ACT_MELEE_VM_PRIMARYATTACK );
	ADD_ACTIVITY_TO_SR( ACT_MELEE_VM_SECONDARYATTACK );
	ADD_ACTIVITY_TO_SR( ACT_MELEE_VM_RELOAD );
	ADD_ACTIVITY_TO_SR( ACT_MELEE_VM_DRYFIRE );
	ADD_ACTIVITY_TO_SR( ACT_MELEE_VM_IDLE_TO_LOWERED );
	ADD_ACTIVITY_TO_SR( ACT_MELEE_VM_IDLE_LOWERED );
	ADD_ACTIVITY_TO_SR( ACT_MELEE_VM_LOWERED_TO_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_MELEE_VM_STUN );
	ADD_ACTIVITY_TO_SR( ACT_MELEE_VM_HITCENTER );
	ADD_ACTIVITY_TO_SR( ACT_MELEE_VM_SWINGHARD );
	ADD_ACTIVITY_TO_SR( ACT_PDA_VM_DRAW );
	ADD_ACTIVITY_TO_SR( ACT_PDA_VM_HOLSTER );
	ADD_ACTIVITY_TO_SR( ACT_PDA_VM_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_PDA_VM_PULLBACK );
	ADD_ACTIVITY_TO_SR( ACT_PDA_VM_PRIMARYATTACK );
	ADD_ACTIVITY_TO_SR( ACT_PDA_VM_SECONDARYATTACK );
	ADD_ACTIVITY_TO_SR( ACT_PDA_VM_RELOAD );
	ADD_ACTIVITY_TO_SR( ACT_PDA_VM_DRYFIRE );
	ADD_ACTIVITY_TO_SR( ACT_PDA_VM_IDLE_TO_LOWERED );
	ADD_ACTIVITY_TO_SR( ACT_PDA_VM_IDLE_LOWERED );
	ADD_ACTIVITY_TO_SR( ACT_PDA_VM_LOWERED_TO_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_ENGINEER_PDA1_VM_DRAW );
	ADD_ACTIVITY_TO_SR( ACT_ENGINEER_PDA2_VM_DRAW );
	ADD_ACTIVITY_TO_SR( ACT_ENGINEER_BLD_VM_DRAW );
	ADD_ACTIVITY_TO_SR( ACT_ENGINEER_PDA1_VM_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_ENGINEER_PDA2_VM_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_ENGINEER_BLD_VM_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_ITEM1_VM_DRAW );
	ADD_ACTIVITY_TO_SR( ACT_ITEM1_VM_HOLSTER );
	ADD_ACTIVITY_TO_SR( ACT_ITEM1_VM_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_ITEM1_VM_IDLE_2 );
	ADD_ACTIVITY_TO_SR( ACT_ITEM1_VM_PULLBACK );
	ADD_ACTIVITY_TO_SR( ACT_ITEM1_VM_PRIMARYATTACK );
	ADD_ACTIVITY_TO_SR( ACT_ITEM1_VM_SECONDARYATTACK );
	ADD_ACTIVITY_TO_SR( ACT_ITEM1_VM_RELOAD );
	ADD_ACTIVITY_TO_SR( ACT_ITEM1_VM_DRYFIRE );
	ADD_ACTIVITY_TO_SR( ACT_ITEM1_VM_IDLE_TO_LOWERED );
	ADD_ACTIVITY_TO_SR( ACT_ITEM1_VM_IDLE_LOWERED );
	ADD_ACTIVITY_TO_SR( ACT_ITEM1_VM_LOWERED_TO_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_ITEM1_RELOAD_START );
	ADD_ACTIVITY_TO_SR( ACT_ITEM1_RELOAD_FINISH );
	ADD_ACTIVITY_TO_SR( ACT_ITEM1_VM_HITCENTER );
	ADD_ACTIVITY_TO_SR( ACT_ITEM1_VM_SWINGHARD );
	ADD_ACTIVITY_TO_SR( ACT_ITEM1_BACKSTAB_VM_UP );
	ADD_ACTIVITY_TO_SR( ACT_ITEM1_BACKSTAB_VM_DOWN );
	ADD_ACTIVITY_TO_SR( ACT_ITEM1_BACKSTAB_VM_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_MELEE_VM_ITEM1_STUN );
	ADD_ACTIVITY_TO_SR( ACT_ITEM2_VM_DRAW );
	ADD_ACTIVITY_TO_SR( ACT_ITEM2_VM_HOLSTER );
	ADD_ACTIVITY_TO_SR( ACT_ITEM2_VM_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_ITEM2_VM_PULLBACK );
	ADD_ACTIVITY_TO_SR( ACT_ITEM2_VM_PRIMARYATTACK );
	ADD_ACTIVITY_TO_SR( ACT_ITEM2_VM_SECONDARYATTACK );
	ADD_ACTIVITY_TO_SR( ACT_ITEM2_VM_RELOAD );
	ADD_ACTIVITY_TO_SR( ACT_ITEM2_VM_DRYFIRE );
	ADD_ACTIVITY_TO_SR( ACT_ITEM2_VM_IDLE_TO_LOWERED );
	ADD_ACTIVITY_TO_SR( ACT_ITEM2_VM_IDLE_LOWERED );
	ADD_ACTIVITY_TO_SR( ACT_ITEM2_VM_LOWERED_TO_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_ITEM2_VM_CHARGE );
	ADD_ACTIVITY_TO_SR( ACT_ITEM2_VM_IDLE_2 );
	ADD_ACTIVITY_TO_SR( ACT_ITEM2_VM_IDLE_3 );
	ADD_ACTIVITY_TO_SR( ACT_ITEM2_VM_CHARGE_IDLE_3 );
	ADD_ACTIVITY_TO_SR( ACT_ITEM2_VM_HITCENTER );
	ADD_ACTIVITY_TO_SR( ACT_ITEM2_VM_SWINGHARD );
	ADD_ACTIVITY_TO_SR( ACT_ITEM2_BACKSTAB_VM_UP );
	ADD_ACTIVITY_TO_SR( ACT_ITEM2_BACKSTAB_VM_DOWN );
	ADD_ACTIVITY_TO_SR( ACT_ITEM2_BACKSTAB_VM_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_MELEE_VM_ITEM2_STUN );
	ADD_ACTIVITY_TO_SR( ACT_ITEM3_VM_DRAW );
	ADD_ACTIVITY_TO_SR( ACT_ITEM3_VM_HOLSTER );
	ADD_ACTIVITY_TO_SR( ACT_ITEM3_VM_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_ITEM3_VM_PULLBACK );
	ADD_ACTIVITY_TO_SR( ACT_ITEM3_VM_PRIMARYATTACK );
	ADD_ACTIVITY_TO_SR( ACT_ITEM3_VM_SECONDARYATTACK );
	ADD_ACTIVITY_TO_SR( ACT_ITEM3_VM_RELOAD );
	ADD_ACTIVITY_TO_SR( ACT_ITEM3_VM_DRYFIRE );
	ADD_ACTIVITY_TO_SR( ACT_ITEM3_VM_IDLE_TO_LOWERED );
	ADD_ACTIVITY_TO_SR( ACT_ITEM3_VM_IDLE_LOWERED );
	ADD_ACTIVITY_TO_SR( ACT_ITEM3_VM_LOWERED_TO_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_ITEM3_VM_CHARGE );
	ADD_ACTIVITY_TO_SR( ACT_ITEM3_VM_IDLE_2 );
	ADD_ACTIVITY_TO_SR( ACT_ITEM3_VM_IDLE_3 );
	ADD_ACTIVITY_TO_SR( ACT_ITEM3_VM_CHARGE_IDLE_3 );
	ADD_ACTIVITY_TO_SR( ACT_ITEM3_VM_HITCENTER );
	ADD_ACTIVITY_TO_SR( ACT_ITEM3_VM_SWINGHARD );
	ADD_ACTIVITY_TO_SR( ACT_SECONDARY2_VM_DRAW );
	ADD_ACTIVITY_TO_SR( ACT_SECONDARY2_VM_HOLSTER );
	ADD_ACTIVITY_TO_SR( ACT_SECONDARY2_VM_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_SECONDARY2_VM_PULLBACK );
	ADD_ACTIVITY_TO_SR( ACT_SECONDARY2_VM_PRIMARYATTACK );
	ADD_ACTIVITY_TO_SR( ACT_SECONDARY2_VM_SECONDARY2ATTACK );
	ADD_ACTIVITY_TO_SR( ACT_SECONDARY2_VM_RELOAD );
	ADD_ACTIVITY_TO_SR( ACT_SECONDARY2_RELOAD_START );
	ADD_ACTIVITY_TO_SR( ACT_SECONDARY2_RELOAD_FINISH );
	ADD_ACTIVITY_TO_SR( ACT_SECONDARY2_VM_RELOAD2 );
	ADD_ACTIVITY_TO_SR( ACT_SECONDARY2_VM_DRYFIRE );
	ADD_ACTIVITY_TO_SR( ACT_SECONDARY2_VM_IDLE_TO_LOWERED );
	ADD_ACTIVITY_TO_SR( ACT_SECONDARY2_VM_IDLE_LOWERED );
	ADD_ACTIVITY_TO_SR( ACT_SECONDARY2_VM_LOWERED_TO_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_BACKSTAB_VM_UP );
	ADD_ACTIVITY_TO_SR( ACT_BACKSTAB_VM_DOWN );
	ADD_ACTIVITY_TO_SR( ACT_BACKSTAB_VM_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_PRIMARY_ATTACK_STAND_PREFIRE );
	ADD_ACTIVITY_TO_SR( ACT_PRIMARY_ATTACK_STAND_POSTFIRE );
	ADD_ACTIVITY_TO_SR( ACT_PRIMARY_ATTACK_STAND_STARTFIRE );
	ADD_ACTIVITY_TO_SR( ACT_PRIMARY_ATTACK_CROUCH_PREFIRE );
	ADD_ACTIVITY_TO_SR( ACT_PRIMARY_ATTACK_CROUCH_POSTFIRE );
	ADD_ACTIVITY_TO_SR( ACT_PRIMARY_ATTACK_SWIM_PREFIRE );
	ADD_ACTIVITY_TO_SR( ACT_PRIMARY_ATTACK_SWIM_POSTFIRE );
	ADD_ACTIVITY_TO_SR( ACT_SECONDARY_ATTACK_STAND_PREFIRE );
	ADD_ACTIVITY_TO_SR( ACT_SECONDARY_ATTACK_STAND_POSTFIRE );
	ADD_ACTIVITY_TO_SR( ACT_SECONDARY_ATTACK_STAND_STARTFIRE );
	ADD_ACTIVITY_TO_SR( ACT_SECONDARY_ATTACK_CROUCH_PREFIRE );
	ADD_ACTIVITY_TO_SR( ACT_SECONDARY_ATTACK_CROUCH_POSTFIRE );
	ADD_ACTIVITY_TO_SR( ACT_SECONDARY_ATTACK_SWIM_PREFIRE );
	ADD_ACTIVITY_TO_SR( ACT_SECONDARY_ATTACK_SWIM_POSTFIRE );
	ADD_ACTIVITY_TO_SR( ACT_MELEE_ATTACK_STAND_PREFIRE );
	ADD_ACTIVITY_TO_SR( ACT_MELEE_ATTACK_STAND_POSTFIRE );
	ADD_ACTIVITY_TO_SR( ACT_MELEE_ATTACK_STAND_STARTFIRE );
	ADD_ACTIVITY_TO_SR( ACT_MELEE_ATTACK_CROUCH_PREFIRE );
	ADD_ACTIVITY_TO_SR( ACT_MELEE_ATTACK_CROUCH_POSTFIRE );
	ADD_ACTIVITY_TO_SR( ACT_MELEE_ATTACK_SWIM_PREFIRE );
	ADD_ACTIVITY_TO_SR( ACT_MELEE_ATTACK_SWIM_POSTFIRE );
	ADD_ACTIVITY_TO_SR( ACT_ITEM1_ATTACK_STAND_PREFIRE );
	ADD_ACTIVITY_TO_SR( ACT_ITEM1_ATTACK_STAND_POSTFIRE );
	ADD_ACTIVITY_TO_SR( ACT_ITEM1_ATTACK_STAND_STARTFIRE );
	ADD_ACTIVITY_TO_SR( ACT_ITEM1_ATTACK_CROUCH_PREFIRE );
	ADD_ACTIVITY_TO_SR( ACT_ITEM1_ATTACK_CROUCH_POSTFIRE );
	ADD_ACTIVITY_TO_SR( ACT_ITEM1_ATTACK_SWIM_PREFIRE );
	ADD_ACTIVITY_TO_SR( ACT_ITEM1_ATTACK_SWIM_POSTFIRE );
	ADD_ACTIVITY_TO_SR( ACT_ITEM2_ATTACK_STAND_PREFIRE );
	ADD_ACTIVITY_TO_SR( ACT_ITEM2_ATTACK_STAND_POSTFIRE );
	ADD_ACTIVITY_TO_SR( ACT_ITEM2_ATTACK_STAND_STARTFIRE );
	ADD_ACTIVITY_TO_SR( ACT_ITEM2_ATTACK_CROUCH_PREFIRE );
	ADD_ACTIVITY_TO_SR( ACT_ITEM2_ATTACK_CROUCH_POSTFIRE );
	ADD_ACTIVITY_TO_SR( ACT_ITEM2_ATTACK_SWIM_PREFIRE );
	ADD_ACTIVITY_TO_SR( ACT_ITEM2_ATTACK_SWIM_POSTFIRE );
	ADD_ACTIVITY_TO_SR( ACT_MP_STAND_MELEE_ALLCLASS );
	ADD_ACTIVITY_TO_SR( ACT_MP_CROUCH_MELEE_ALLCLASS );
	ADD_ACTIVITY_TO_SR( ACT_MP_RUN_MELEE_ALLCLASS );
	ADD_ACTIVITY_TO_SR( ACT_MP_WALK_MELEE_ALLCLASS );
	ADD_ACTIVITY_TO_SR( ACT_MP_AIRWALK_MELEE_ALLCLASS );
	ADD_ACTIVITY_TO_SR( ACT_MP_CROUCHWALK_MELEE_ALLCLASS );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_MELEE_ALLCLASS );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_START_MELEE_ALLCLASS );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_FLOAT_MELEE_ALLCLASS );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_LAND_MELEE_ALLCLASS );
	ADD_ACTIVITY_TO_SR( ACT_MP_SWIM_MELEE_ALLCLASS );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_STAND_MELEE_ALLCLASS );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_STAND_MELEE_SECONDARY_ALLCLASS );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_CROUCH_MELEE_ALLCLASS );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_CROUCH_MELEE_SECONDARY_ALLCLASS );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_SWIM_MELEE_ALLCLASS );
	ADD_ACTIVITY_TO_SR( ACT_MP_ATTACK_AIRWALK_MELEE_ALLCLASS );
	ADD_ACTIVITY_TO_SR( ACT_MELEE_ALLCLASS_VM_DRAW );
	ADD_ACTIVITY_TO_SR( ACT_MELEE_ALLCLASS_VM_HOLSTER );
	ADD_ACTIVITY_TO_SR( ACT_MELEE_ALLCLASS_VM_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_MELEE_ALLCLASS_VM_PULLBACK );
	ADD_ACTIVITY_TO_SR( ACT_MELEE_ALLCLASS_VM_PRIMARYATTACK );
	ADD_ACTIVITY_TO_SR( ACT_MELEE_ALLCLASS_VM_SECONDARYATTACK );
	ADD_ACTIVITY_TO_SR( ACT_MELEE_ALLCLASS_VM_RELOAD );
	ADD_ACTIVITY_TO_SR( ACT_MELEE_ALLCLASS_VM_DRYFIRE );
	ADD_ACTIVITY_TO_SR( ACT_MELEE_ALLCLASS_VM_IDLE_TO_LOWERED );
	ADD_ACTIVITY_TO_SR( ACT_MELEE_ALLCLASS_VM_IDLE_LOWERED );
	ADD_ACTIVITY_TO_SR( ACT_MELEE_ALLCLASS_VM_LOWERED_TO_IDLE );
	ADD_ACTIVITY_TO_SR( ACT_MELEE_ALLCLASS_VM_STUN );
	ADD_ACTIVITY_TO_SR( ACT_MELEE_ALLCLASS_VM_HITCENTER );
	ADD_ACTIVITY_TO_SR( ACT_MELEE_ALLCLASS_VM_SWINGHARD );
	ADD_ACTIVITY_TO_SR( ACT_MP_STAND_BOMB );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_START_BOMB );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_FLOAT_BOMB );
	ADD_ACTIVITY_TO_SR( ACT_MP_JUMP_LAND_BOMB );
	ADD_ACTIVITY_TO_SR( ACT_MP_RUN_BOMB );
	ADD_ACTIVITY_TO_SR( ACT_MP_SWIM_BOMB );
	ADD_ACTIVITY_TO_SR( ACT_VM_DRAW_QRL );
	ADD_ACTIVITY_TO_SR( ACT_VM_IDLE_QRL );
	ADD_ACTIVITY_TO_SR( ACT_VM_PULLBACK_QRL );
	ADD_ACTIVITY_TO_SR( ACT_VM_PRIMARYATTACK_QRL );
	ADD_ACTIVITY_TO_SR( ACT_VM_RELOAD_QRL );
	ADD_ACTIVITY_TO_SR( ACT_VM_RELOAD_START_QRL );
	ADD_ACTIVITY_TO_SR( ACT_VM_RELOAD_FINISH_QRL );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_STAND_PRIMARY3 );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_CROUCH_PRIMARY3 );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_AIRWALK_PRIMARY3 );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_STAND_PRIMARY3_LOOP );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_CROUCH_PRIMARY3_LOOP );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_AIRWALK_PRIMARY3_LOOP );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_STAND_PRIMARY3_END );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_CROUCH_PRIMARY3_END );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_AIRWALK_PRIMARY3_END );
	ADD_ACTIVITY_TO_SR( ACT_MP_RELOAD_SWIM_PRIMARY3 );

}



//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
void CAI_NPC::InitDefaultSquadSlotSR(void)
{
	gm_SquadSlotNamespace.AddSymbol( "SQUAD_SLOT_ATTACK1", AI_RemapToGlobal(SQUAD_SLOT_ATTACK1) );
	gm_SquadSlotNamespace.AddSymbol( "SQUAD_SLOT_ATTACK2", AI_RemapToGlobal(SQUAD_SLOT_ATTACK2) );
	gm_SquadSlotNamespace.AddSymbol( "SQUAD_SLOT_INVESTIGATE_SOUND", AI_RemapToGlobal(SQUAD_SLOT_INVESTIGATE_SOUND) );
	gm_SquadSlotNamespace.AddSymbol( "SQUAD_SLOT_EXCLUSIVE_HANDSIGN", AI_RemapToGlobal(SQUAD_SLOT_EXCLUSIVE_HANDSIGN) );
	gm_SquadSlotNamespace.AddSymbol( "SQUAD_SLOT_EXCLUSIVE_RELOAD", AI_RemapToGlobal(SQUAD_SLOT_EXCLUSIVE_RELOAD) );
	gm_SquadSlotNamespace.AddSymbol( "SQUAD_SLOT_PICKUP_WEAPON1", AI_RemapToGlobal(SQUAD_SLOT_PICKUP_WEAPON1) );
	gm_SquadSlotNamespace.AddSymbol( "SQUAD_SLOT_PICKUP_WEAPON2", AI_RemapToGlobal(SQUAD_SLOT_PICKUP_WEAPON2) );
	gm_SquadSlotNamespace.AddSymbol( "SQUAD_SLOT_SPECIAL_ATTACK", AI_RemapToGlobal(SQUAD_SLOT_SPECIAL_ATTACK) );
}

bool CAI_SchedulesManager::LoadAllSchedules(void)
{
	if (!CAI_SchedulesManager::allSchedules)
	{
		// Init defaults
		CAI_NPC::InitSchedulingTables();
		if(!CAI_NPC::LoadDefaultSchedules())
		{
			META_CONPRINT("CAI_NPC::LoadDefaultSchedules Fail\n");
		}
	}

	return true;
}

void CAI_SchedulesManager::CreateStringRegistries( void )
{
	CAI_NPC::GetSchedulingSymbols()->Clear();
	CAI_NPC::gm_SquadSlotNamespace.Clear();
}

void CAI_SchedulesManager::DestroyStringRegistries(void)
{
	CAI_NPC::GetSchedulingSymbols()->Clear();
	CAI_NPC::gm_SquadSlotNamespace.Clear();

}

void CAI_SchedulesManager::DeleteAllSchedules(void)
{
	m_CurLoadSig++;

	if ( m_CurLoadSig < 0 )
		m_CurLoadSig = 0;

	CAI_Schedule *schedule = CAI_SchedulesManager::allSchedules;
	CAI_Schedule *next;

	while (schedule)
	{
		next = schedule->nextSchedule;
		delete schedule;
		schedule = next;
	}
	CAI_SchedulesManager::allSchedules = NULL;
}


void InitDefaultAIRelationships()
{
	int i, j;

	//  Allocate memory for default relationships
	CCombatCharacter::AllocateDefaultRelationships();

	// --------------------------------------------------------------
	// First initialize table so we can report missing relationships
	// --------------------------------------------------------------
	for (i=0;i<NUM_AI_CLASSES;i++)
	{
		for (j=0;j<NUM_AI_CLASSES;j++)
		{
			// By default all relationships are neutral of priority zero
			CCombatCharacter::SetDefaultRelationship( (Class_T)i, (Class_T)j, D_NU, 0 );
		}
	}

	// ------------------------------------------------------------
		//	> CLASS_ANTLION
		// ------------------------------------------------------------
		CCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_NONE,				D_NU, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_PLAYER,			D_HT, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_BARNACLE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_BULLSEYE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_BULLSQUID,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_CITIZEN_PASSIVE,	D_HT, 0);	
		CCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_CITIZEN_REBEL,	D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_COMBINE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_COMBINE_HUNTER,	D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_CONSCRIPT,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_FLARE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_HEADCRAB,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_HOUNDEYE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_MANHACK,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_METROPOLICE,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_MILITARY,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_MISSILE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_SCANNER,			D_HT, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_STALKER,			D_HT, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_VORTIGAUNT,		D_HT, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_ZOMBIE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_PROTOSNIPER,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_ANTLION,			D_LI, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_EARTH_FAUNA,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_PLAYER_ALLY,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_ANTLION,			CLASS_HACKED_ROLLERMINE,D_HT, 0);


		// ------------------------------------------------------------
		//	> CLASS_BARNACLE
		//
		//  In this case, the relationship D_HT indicates which characters
		//  the barnacle will try to eat.
		// ------------------------------------------------------------
		CCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_NONE,				D_NU, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_PLAYER,			D_HT, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_ANTLION,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_BARNACLE,			D_LI, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_BULLSEYE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_BULLSQUID,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_CITIZEN_PASSIVE,	D_HT, 0);	
		CCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_CITIZEN_REBEL,	D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_COMBINE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_COMBINE_HUNTER,	D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_CONSCRIPT,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_FLARE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_HEADCRAB,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_HOUNDEYE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_MANHACK,			D_FR, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_METROPOLICE,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_MILITARY,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_MISSILE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_SCANNER,			D_NU, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_STALKER,			D_HT, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_VORTIGAUNT,		D_HT, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_ZOMBIE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_PROTOSNIPER,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_EARTH_FAUNA,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_PLAYER_ALLY,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BARNACLE,			CLASS_HACKED_ROLLERMINE,D_HT, 0);

		// ------------------------------------------------------------
		//	> CLASS_BULLSEYE
		// ------------------------------------------------------------
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_NONE,				D_NU, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_PLAYER,			D_NU, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_ANTLION,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_BARNACLE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_BULLSEYE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_BULLSQUID,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_CITIZEN_REBEL,	D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_COMBINE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_COMBINE_HUNTER,	D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_CONSCRIPT,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_FLARE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_HEADCRAB,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_HOUNDEYE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_MANHACK,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_METROPOLICE,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_MILITARY,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_MISSILE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_SCANNER,			D_NU, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_STALKER,			D_NU, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_VORTIGAUNT,		D_NU, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_ZOMBIE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_PROTOSNIPER,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_EARTH_FAUNA,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_PLAYER_ALLY,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_PLAYER_ALLY_VITAL,D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSEYE,			CLASS_HACKED_ROLLERMINE,D_NU, 0);

		// ------------------------------------------------------------
		//	> CLASS_BULLSQUID
		// ------------------------------------------------------------
		
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_NONE,				D_NU, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_PLAYER,			D_HT, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_ANTLION,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_BARNACLE,			D_FR, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_BULLSEYE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_BULLSQUID,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_CITIZEN_PASSIVE,	D_HT, 0);	
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_CITIZEN_REBEL,	D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_COMBINE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_COMBINE_HUNTER,	D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_CONSCRIPT,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_FLARE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_HEADCRAB,			D_HT, 1);
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_HOUNDEYE,			D_HT, 1);
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_MANHACK,			D_FR, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_METROPOLICE,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_MILITARY,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_MISSILE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_SCANNER,			D_NU, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_STALKER,			D_HT, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_VORTIGAUNT,		D_HT, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_ZOMBIE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_PROTOSNIPER,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_EARTH_FAUNA,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_PLAYER_ALLY,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_BULLSQUID,			CLASS_HACKED_ROLLERMINE,D_HT, 0);

		
		// ------------------------------------------------------------
		//	> CLASS_CITIZEN_PASSIVE
		// ------------------------------------------------------------
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_NONE,				D_NU, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_PLAYER,			D_NU, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_ANTLION,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_BARNACLE,			D_FR, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_BULLSEYE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_BULLSQUID,		D_FR, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_CITIZEN_REBEL,	D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_COMBINE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_COMBINE_HUNTER,	D_FR, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_CONSCRIPT,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_FLARE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_HEADCRAB,			D_FR, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_HOUNDEYE,			D_FR, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_MANHACK,			D_FR, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_METROPOLICE,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_MILITARY,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_MISSILE,			D_FR, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_SCANNER,			D_NU, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_STALKER,			D_NU, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_VORTIGAUNT,		D_LI, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_ZOMBIE,			D_FR, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_PROTOSNIPER,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_EARTH_FAUNA,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_PLAYER_ALLY,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_PLAYER_ALLY_VITAL,D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_PASSIVE,	CLASS_HACKED_ROLLERMINE,D_NU, 0);

		// ------------------------------------------------------------
		//	> CLASS_CITIZEN_REBEL
		// ------------------------------------------------------------
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_NONE,				D_NU, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_PLAYER,			D_NU, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_ANTLION,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_BARNACLE,			D_FR, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_BULLSEYE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_BULLSQUID,		D_FR, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_CITIZEN_REBEL,	D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_COMBINE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_COMBINE_HUNTER,	D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_CONSCRIPT,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_FLARE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_HEADCRAB,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_HOUNDEYE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_MANHACK,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_METROPOLICE,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_MILITARY,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_MISSILE,			D_FR, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_SCANNER,			D_HT, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_STALKER,			D_HT, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_VORTIGAUNT,		D_LI, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_ZOMBIE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_PROTOSNIPER,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_EARTH_FAUNA,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_PLAYER_ALLY,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_PLAYER_ALLY_VITAL,D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CITIZEN_REBEL,		CLASS_HACKED_ROLLERMINE,D_NU, 0);

		// ------------------------------------------------------------
		//	> CLASS_COMBINE
		// ------------------------------------------------------------
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_NONE,				D_NU, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_PLAYER,			D_HT, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_ANTLION,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_BARNACLE,			D_FR, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_BULLSEYE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_BULLSQUID,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_CITIZEN_REBEL,	D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_COMBINE,			D_LI, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_COMBINE_GUNSHIP,	D_LI, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_COMBINE_HUNTER,	D_LI, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_CONSCRIPT,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_FLARE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_HEADCRAB,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_HOUNDEYE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_MANHACK,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_METROPOLICE,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_MILITARY,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_MISSILE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_SCANNER,			D_NU, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_STALKER,			D_NU, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_VORTIGAUNT,		D_HT, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_ZOMBIE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_PROTOSNIPER,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_EARTH_FAUNA,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_PLAYER_ALLY,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE,			CLASS_HACKED_ROLLERMINE,D_HT, 0);


		// ------------------------------------------------------------
		//	> CLASS_COMBINE_GUNSHIP
		// ------------------------------------------------------------
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_NONE,				D_NU, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_PLAYER,			D_HT, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_ANTLION,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_BARNACLE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_BULLSEYE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_BULLSQUID,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_CITIZEN_REBEL,	D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_COMBINE,			D_LI, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_COMBINE_GUNSHIP,	D_LI, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_COMBINE_HUNTER,	D_LI, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_CONSCRIPT,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_FLARE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_HEADCRAB,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_HOUNDEYE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_MANHACK,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_METROPOLICE,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_MILITARY,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_MISSILE,			D_FR, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_SCANNER,			D_NU, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_STALKER,			D_NU, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_VORTIGAUNT,		D_HT, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_ZOMBIE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_PROTOSNIPER,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_EARTH_FAUNA,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_PLAYER_ALLY,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_GUNSHIP,		CLASS_HACKED_ROLLERMINE,D_HT, 0);

		// ------------------------------------------------------------
		//	> CLASS_COMBINE_HUNTER
		// ------------------------------------------------------------
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_NONE,				D_NU, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_PLAYER,			D_HT, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_ANTLION,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_BARNACLE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_BULLSEYE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,	CLASS_BULLSQUID,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_CITIZEN_PASSIVE,	D_HT, 0);	
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_CITIZEN_REBEL,	D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_COMBINE,			D_LI, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_COMBINE_GUNSHIP,	D_LI, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_COMBINE_HUNTER,	D_LI, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_CONSCRIPT,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_FLARE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_HEADCRAB,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,	CLASS_HOUNDEYE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_MANHACK,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_METROPOLICE,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_MILITARY,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_MISSILE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_SCANNER,			D_NU, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_STALKER,			D_NU, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_VORTIGAUNT,		D_HT, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_ZOMBIE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_PROTOSNIPER,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_EARTH_FAUNA,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_PLAYER_ALLY,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_COMBINE_HUNTER,		CLASS_HACKED_ROLLERMINE,D_HT, 0);

		// ------------------------------------------------------------
		//	> CLASS_CONSCRIPT
		// ------------------------------------------------------------
		CCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_NONE,				D_NU, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_PLAYER,			D_NU, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_ANTLION,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_BARNACLE,			D_FR, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_BULLSEYE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_BULLSQUID,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
		CCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_CITIZEN_REBEL,	D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_COMBINE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_COMBINE_HUNTER,	D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_CONSCRIPT,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_FLARE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_HEADCRAB,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_HOUNDEYE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_MANHACK,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_METROPOLICE,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_MILITARY,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_MISSILE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_SCANNER,			D_HT, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_STALKER,			D_HT, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_VORTIGAUNT,		D_NU, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_ZOMBIE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_PROTOSNIPER,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_EARTH_FAUNA,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_PLAYER_ALLY,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_PLAYER_ALLY_VITAL,D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_CONSCRIPT,			CLASS_HACKED_ROLLERMINE,D_NU, 0);

		// ------------------------------------------------------------
		//	> CLASS_FLARE
		// ------------------------------------------------------------
		CCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_NONE,				D_NU, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_PLAYER,			D_NU, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_ANTLION,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_BARNACLE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_BULLSEYE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_BULLSQUID,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
		CCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_CITIZEN_REBEL,	D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_COMBINE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_COMBINE_HUNTER,	D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_CONSCRIPT,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_FLARE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_HEADCRAB,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_HOUNDEYE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_MANHACK,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_METROPOLICE,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_MILITARY,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_MISSILE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_FLARE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_SCANNER,			D_NU, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_STALKER,			D_NU, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_VORTIGAUNT,		D_NU, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_ZOMBIE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_PROTOSNIPER,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_EARTH_FAUNA,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_PLAYER_ALLY,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_PLAYER_ALLY_VITAL,D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_FLARE,			CLASS_HACKED_ROLLERMINE,D_NU, 0);

		// ------------------------------------------------------------
		//	> CLASS_HEADCRAB
		// ------------------------------------------------------------
		CCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_NONE,				D_NU, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_PLAYER,			D_HT, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_ANTLION,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_BARNACLE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_BULLSEYE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_BULLSQUID,		D_FR, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_CITIZEN_PASSIVE,	D_HT, 0);	
		CCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_CITIZEN_REBEL,	D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_COMBINE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_COMBINE_HUNTER,	D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_CONSCRIPT,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_FLARE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_HEADCRAB,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_HOUNDEYE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_MANHACK,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_METROPOLICE,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_MILITARY,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_MISSILE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_SCANNER,			D_NU, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_STALKER,			D_NU, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_VORTIGAUNT,		D_HT, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_ZOMBIE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_PROTOSNIPER,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_EARTH_FAUNA,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_PLAYER_ALLY,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HEADCRAB,			CLASS_HACKED_ROLLERMINE,D_FR, 0);
	
		// ------------------------------------------------------------
		//	> CLASS_HOUNDEYE
		// ------------------------------------------------------------
		
		CCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_NONE,				D_NU, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_PLAYER,			D_HT, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_ANTLION,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_BARNACLE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_BULLSEYE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_BULLSQUID,		D_FR, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_CITIZEN_PASSIVE,	D_HT, 0);	
		CCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_CITIZEN_REBEL,	D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_COMBINE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_COMBINE_HUNTER,	D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_CONSCRIPT,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_FLARE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_HEADCRAB,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_HOUNDEYE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_MANHACK,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_METROPOLICE,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_MILITARY,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_MISSILE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_SCANNER,			D_NU, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_STALKER,			D_NU, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_VORTIGAUNT,		D_HT, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_ZOMBIE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_PROTOSNIPER,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_EARTH_FAUNA,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_PLAYER_ALLY,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HOUNDEYE,			CLASS_HACKED_ROLLERMINE,D_HT, 0);
		

		// ------------------------------------------------------------
		//	> CLASS_MANHACK
		// ------------------------------------------------------------
		CCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_NONE,				D_NU, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_PLAYER,			D_HT, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_ANTLION,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_BARNACLE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_BULLSEYE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_BULLSQUID,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_CITIZEN_PASSIVE,	D_HT, 0);	
		CCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_CITIZEN_REBEL,	D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_COMBINE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_COMBINE_HUNTER,	D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_CONSCRIPT,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_FLARE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_HEADCRAB,			D_HT,-1);
		CCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_HOUNDEYE,			D_HT,-1);
		CCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_MANHACK,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_METROPOLICE,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_MILITARY,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_MISSILE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_SCANNER,			D_NU, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_STALKER,			D_NU, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_VORTIGAUNT,		D_HT, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_ZOMBIE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_PROTOSNIPER,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_EARTH_FAUNA,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_PLAYER_ALLY,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MANHACK,			CLASS_HACKED_ROLLERMINE,D_HT, 0);

		// ------------------------------------------------------------
		//	> CLASS_METROPOLICE
		// ------------------------------------------------------------
		CCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_NONE,				D_NU, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_PLAYER,			D_HT, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_ANTLION,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_BARNACLE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_BULLSEYE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_BULLSQUID,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
		CCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_CITIZEN_REBEL,	D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_COMBINE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_COMBINE_HUNTER,	D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_CONSCRIPT,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_FLARE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_HEADCRAB,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_HOUNDEYE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_MANHACK,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_METROPOLICE,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_MILITARY,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_MISSILE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_SCANNER,			D_NU, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_STALKER,			D_NU, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_VORTIGAUNT,		D_HT, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_ZOMBIE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_PROTOSNIPER,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_EARTH_FAUNA,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_PLAYER_ALLY,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_METROPOLICE,		CLASS_HACKED_ROLLERMINE,D_HT, 0);

		// ------------------------------------------------------------
		//	> CLASS_MILITARY
		// ------------------------------------------------------------
		CCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_NONE,				D_NU, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_PLAYER,			D_HT, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_ANTLION,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_BARNACLE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_BULLSEYE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_BULLSQUID,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
		CCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_CITIZEN_REBEL,	D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_COMBINE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_COMBINE_HUNTER,	D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_CONSCRIPT,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_FLARE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_HEADCRAB,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_HOUNDEYE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_MANHACK,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_METROPOLICE,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_MILITARY,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_MISSILE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_SCANNER,			D_NU, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_STALKER,			D_NU, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_VORTIGAUNT,		D_HT, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_ZOMBIE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_PROTOSNIPER,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_EARTH_FAUNA,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_PLAYER_ALLY,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MILITARY,			CLASS_HACKED_ROLLERMINE,D_HT, 0);

		// ------------------------------------------------------------
		//	> CLASS_MISSILE
		// ------------------------------------------------------------
		CCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_NONE,				D_NU, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_PLAYER,			D_HT, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_ANTLION,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_BARNACLE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_BULLSEYE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_BULLSQUID,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
		CCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_CITIZEN_REBEL,	D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_COMBINE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_COMBINE_HUNTER,	D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_CONSCRIPT,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_FLARE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_HEADCRAB,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_HOUNDEYE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_MANHACK,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_METROPOLICE,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_MILITARY,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_MISSILE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_SCANNER,			D_NU, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_STALKER,			D_NU, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_VORTIGAUNT,		D_HT, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_ZOMBIE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_PROTOSNIPER,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_EARTH_FAUNA,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_PLAYER_ALLY,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_MISSILE,			CLASS_HACKED_ROLLERMINE,D_HT, 0);


		// ------------------------------------------------------------
		//	> CLASS_NONE
		// ------------------------------------------------------------
		CCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_NONE,				D_NU, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_PLAYER,			D_NU, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_ANTLION,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_BARNACLE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_BULLSEYE,			D_NU, 0);	
		CCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_BULLSQUID,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
		CCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_CITIZEN_REBEL,	D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_COMBINE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_COMBINE_HUNTER,	D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_CONSCRIPT,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_FLARE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_HEADCRAB,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_HOUNDEYE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_MANHACK,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_METROPOLICE,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_MILITARY,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_SCANNER,			D_NU, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_STALKER,			D_NU, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_VORTIGAUNT,		D_NU, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_ZOMBIE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_PROTOSNIPER,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_EARTH_FAUNA,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_PLAYER_ALLY,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_PLAYER_ALLY_VITAL,D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_NONE,				CLASS_HACKED_ROLLERMINE,D_NU, 0);

		// ------------------------------------------------------------
		//	> CLASS_PLAYER
		// ------------------------------------------------------------
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_NONE,				D_NU, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_PLAYER,			D_NU, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_ANTLION,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_BARNACLE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_BULLSEYE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_BULLSQUID,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_CITIZEN_PASSIVE,	D_LI, 0);	
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_CITIZEN_REBEL,	D_LI, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_COMBINE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_COMBINE_GUNSHIP,	D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_COMBINE_HUNTER,	D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_CONSCRIPT,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_FLARE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_HEADCRAB,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_HOUNDEYE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_MANHACK,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_METROPOLICE,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_MILITARY,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_MISSILE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_SCANNER,			D_HT, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_STALKER,			D_HT, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_VORTIGAUNT,		D_LI, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_ZOMBIE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_PROTOSNIPER,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_EARTH_FAUNA,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_PLAYER_ALLY,		D_LI, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_PLAYER_ALLY_VITAL,D_LI, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER,			CLASS_HACKED_ROLLERMINE,D_LI, 0);

		// ------------------------------------------------------------
		//	> CLASS_PLAYER_ALLY
		// ------------------------------------------------------------
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_NONE,				D_NU, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_PLAYER,			D_LI, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_ANTLION,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_BARNACLE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_BULLSEYE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_BULLSQUID,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_CITIZEN_REBEL,	D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_COMBINE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_COMBINE_HUNTER,	D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_CONSCRIPT,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_FLARE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_HEADCRAB,			D_FR, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_HOUNDEYE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_MANHACK,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_METROPOLICE,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_MILITARY,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_MISSILE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_SCANNER,			D_HT, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_STALKER,			D_HT, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_VORTIGAUNT,		D_LI, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_ZOMBIE,			D_FR, 1);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_PROTOSNIPER,		D_FR, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_EARTH_FAUNA,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_PLAYER_ALLY,		D_LI, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_PLAYER_ALLY_VITAL,D_LI, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY,			CLASS_HACKED_ROLLERMINE,D_LI, 0);

		// ------------------------------------------------------------
		//	> CLASS_PLAYER_ALLY_VITAL
		// ------------------------------------------------------------
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_NONE,				D_NU, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_PLAYER,			D_LI, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_ANTLION,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_BARNACLE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_BULLSEYE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_BULLSQUID,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_CITIZEN_REBEL,	D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_COMBINE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_COMBINE_HUNTER,	D_FR, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_CONSCRIPT,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_FLARE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_HEADCRAB,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_HOUNDEYE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_MANHACK,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_METROPOLICE,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_MILITARY,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_MISSILE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_SCANNER,			D_HT, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_STALKER,			D_HT, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_VORTIGAUNT,		D_LI, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_ZOMBIE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_PROTOSNIPER,		D_FR, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_EARTH_FAUNA,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_PLAYER_ALLY,		D_LI, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_PLAYER_ALLY_VITAL,D_LI, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PLAYER_ALLY_VITAL,	CLASS_HACKED_ROLLERMINE,D_LI, 0);

		// ------------------------------------------------------------
		//	> CLASS_SCANNER
		// ------------------------------------------------------------	
		CCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_NONE,				D_NU, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_PLAYER,			D_HT, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_ANTLION,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_BARNACLE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_BULLSEYE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_BULLSQUID,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
		CCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_CITIZEN_REBEL,	D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_COMBINE,			D_LI, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_COMBINE_GUNSHIP,	D_LI, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_COMBINE_HUNTER,	D_LI, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_CONSCRIPT,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_FLARE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_HEADCRAB,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_HOUNDEYE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_MANHACK,			D_LI, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_METROPOLICE,		D_LI, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_MILITARY,			D_LI, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_MISSILE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_SCANNER,			D_LI, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_STALKER,			D_LI, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_VORTIGAUNT,		D_HT, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_ZOMBIE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_PROTOSNIPER,		D_LI, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_EARTH_FAUNA,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_PLAYER_ALLY,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_SCANNER,			CLASS_HACKED_ROLLERMINE,D_HT, 0);

		// ------------------------------------------------------------
		//	> CLASS_STALKER
		// ------------------------------------------------------------	
		CCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_NONE,				D_NU, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_PLAYER,			D_HT, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_ANTLION,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_BARNACLE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_BULLSEYE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_BULLSQUID,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
		CCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_CITIZEN_REBEL,	D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_COMBINE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_COMBINE_HUNTER,	D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_CONSCRIPT,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_FLARE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_HEADCRAB,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_HOUNDEYE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_MANHACK,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_METROPOLICE,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_MILITARY,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_MISSILE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_SCANNER,			D_NU, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_STALKER,			D_NU, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_VORTIGAUNT,		D_HT, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_ZOMBIE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_PROTOSNIPER,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_EARTH_FAUNA,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_PLAYER_ALLY,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_STALKER,			CLASS_HACKED_ROLLERMINE,D_HT, 0);

		// ------------------------------------------------------------
		//	> CLASS_VORTIGAUNT
		// ------------------------------------------------------------	
		CCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_NONE,				D_NU, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_PLAYER,			D_LI, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_ANTLION,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_BARNACLE,			D_FR, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_BULLSEYE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_BULLSQUID,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_CITIZEN_PASSIVE,	D_LI, 0);	
		CCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_CITIZEN_REBEL,	D_LI, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_COMBINE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_COMBINE_HUNTER,	D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_CONSCRIPT,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_FLARE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_HEADCRAB,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_HOUNDEYE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_MANHACK,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_METROPOLICE,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_MILITARY,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_MISSILE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_SCANNER,			D_HT, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_STALKER,			D_HT, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_VORTIGAUNT,		D_NU, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_ZOMBIE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_PROTOSNIPER,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_EARTH_FAUNA,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_PLAYER_ALLY,		D_LI, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_PLAYER_ALLY_VITAL,D_LI, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_VORTIGAUNT,		CLASS_HACKED_ROLLERMINE,D_LI, 0);

		// ------------------------------------------------------------
		//	> CLASS_ZOMBIE
		// ------------------------------------------------------------	
		CCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_NONE,				D_NU, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_PLAYER,			D_HT, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_ANTLION,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_BARNACLE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_BULLSEYE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_BULLSQUID,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_CITIZEN_PASSIVE,	D_HT, 0);	
		CCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_CITIZEN_REBEL,	D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_COMBINE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_COMBINE_HUNTER,	D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_CONSCRIPT,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_FLARE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_HEADCRAB,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_HOUNDEYE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_MANHACK,			D_FR, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_METROPOLICE,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_MILITARY,			D_FR, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_MISSILE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_SCANNER,			D_NU, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_STALKER,			D_NU, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_VORTIGAUNT,		D_HT, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_ZOMBIE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_PROTOSNIPER,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_EARTH_FAUNA,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_PLAYER_ALLY,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_ZOMBIE,			CLASS_HACKED_ROLLERMINE,D_HT, 0);

		// ------------------------------------------------------------
		//	> CLASS_PROTOSNIPER
		// ------------------------------------------------------------	
		CCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_NONE,				D_NU, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_PLAYER,			D_HT, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_ANTLION,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_BARNACLE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_BULLSEYE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_BULLSQUID,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_CITIZEN_PASSIVE,	D_HT, 0);	
		CCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_CITIZEN_REBEL,	D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_COMBINE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_COMBINE_HUNTER,	D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_CONSCRIPT,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_FLARE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_HEADCRAB,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_HOUNDEYE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_MANHACK,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_METROPOLICE,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_MILITARY,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_MISSILE,			D_NU, 5);
		CCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_SCANNER,			D_NU, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_STALKER,			D_NU, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_VORTIGAUNT,		D_HT, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_ZOMBIE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_PROTOSNIPER,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_EARTH_FAUNA,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_PLAYER_ALLY,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_PROTOSNIPER,			CLASS_HACKED_ROLLERMINE,D_HT, 0);

		// ------------------------------------------------------------
		//	> CLASS_EARTH_FAUNA
		//
		// Hates pretty much everything equally except other earth fauna.
		// This will make the critter choose the nearest thing as its enemy.
		// ------------------------------------------------------------	
		CCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_NONE,				D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_PLAYER,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_ANTLION,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_BARNACLE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_BULLSEYE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_BULLSQUID,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_CITIZEN_PASSIVE,	D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_CITIZEN_REBEL,	D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_COMBINE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_COMBINE_GUNSHIP,	D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_COMBINE_HUNTER,	D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_CONSCRIPT,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_FLARE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_HEADCRAB,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_HOUNDEYE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_MANHACK,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_METROPOLICE,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_MILITARY,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_MISSILE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_SCANNER,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_STALKER,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_VORTIGAUNT,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_ZOMBIE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_PROTOSNIPER,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_EARTH_FAUNA,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_PLAYER_ALLY,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_PLAYER_ALLY_VITAL,D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_EARTH_FAUNA,			CLASS_HACKED_ROLLERMINE,D_NU, 0);

		// ------------------------------------------------------------
		//	> CLASS_HACKED_ROLLERMINE
		// ------------------------------------------------------------
		CCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_NONE,				D_NU, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_PLAYER,			D_LI, 0);			
		CCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_ANTLION,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_BARNACLE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_BULLSEYE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_BULLSQUID,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_CITIZEN_PASSIVE,	D_NU, 0);	
		CCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_CITIZEN_REBEL,	D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_COMBINE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_COMBINE_GUNSHIP,	D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_COMBINE_HUNTER,	D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_CONSCRIPT,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_FLARE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_HEADCRAB,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_HOUNDEYE,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_MANHACK,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_METROPOLICE,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_MILITARY,			D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_MISSILE,			D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_SCANNER,			D_NU, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_STALKER,			D_HT, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_VORTIGAUNT,		D_LI, 0);		
		CCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_ZOMBIE,			D_HT, 1);
		CCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_PROTOSNIPER,		D_NU, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_EARTH_FAUNA,		D_HT, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_PLAYER_ALLY,		D_LI, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_PLAYER_ALLY_VITAL,D_LI, 0);
		CCombatCharacter::SetDefaultRelationship(CLASS_HACKED_ROLLERMINE,			CLASS_HACKED_ROLLERMINE,D_LI, 0);

}



