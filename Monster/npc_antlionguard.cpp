
#include "CAI_NPC.h"
#include "npc_antlion.h"
#include "CCycler_Fix.h"
#include "CSprite.h"
#include "CAI_Blended_Movement.h"
#include "CSoundent.h"
#include "physics_impact_damage.h"
#include "CAI_senses.h"
#include "CAI_Network.h"
#include "CAI_Node.h"
#include "soundenvelope.h"
#include "CE_recipientfilter.h"
#include "effect_dispatch_data.h"
#include "eventqueue.h"
#include "player_pickup.h"


inline void TraceHull_SkipPhysics( const Vector &vecAbsStart, const Vector &vecAbsEnd, const Vector &hullMin, 
					 const Vector &hullMax,	unsigned int mask, const CBaseEntity *ignore, 
					 int collisionGroup, trace_t *ptr, float minMass );



extern CSoundEnvelopeController *g_SoundController;

ConVar	sk_antlionguard_dmg_charge( "sk_antlionguard_dmg_charge", "0" );
ConVar	sk_antlionguard_dmg_shove( "sk_antlionguard_dmg_shove", "0" );
ConVar	sk_antlionguard_health( "sk_antlionguard_health", "0" );


// Spawnflags 
#define	SF_ANTLIONGUARD_SERVERSIDE_RAGDOLL	( 1 << 16 )
#define SF_ANTLIONGUARD_INSIDE_FOOTSTEPS	( 1 << 17 )

#define	ANTLIONGUARD_MODEL		"models/antlion_guard.mdl"
#define	MIN_BLAST_DAMAGE		25.0f
#define MIN_CRUSH_DAMAGE		20.0f

//==================================================
//
// Antlion Guard
//
//==================================================

#define ANTLIONGUARD_MAX_OBJECTS				128
#define	ANTLIONGUARD_MIN_OBJECT_MASS			8
#define	ANTLIONGUARD_MAX_OBJECT_MASS			750
#define	ANTLIONGUARD_FARTHEST_PHYSICS_OBJECT	350
#define ANTLIONGUARD_OBJECTFINDING_FOV			DOT_45DEGREE // 1/sqrt(2)

//Melee definitions
#define	ANTLIONGUARD_MELEE1_RANGE		156.0f
#define	ANTLIONGUARD_MELEE1_CONE		0.7f

// Antlion summoning
#define ANTLIONGUARD_SUMMON_COUNT		3

// Sight
#define	ANTLIONGUARD_FOV_NORMAL			-0.4f

// cavern guard's poisoning behavior
#if HL2_EPISODIC
#define ANTLIONGUARD_POISON_TO			12 // we only poison Gordon down to twelve to give him a chance to regen up to 20 by the next charge
#endif

#define	ANTLIONGUARD_CHARGE_MIN			256
#define	ANTLIONGUARD_CHARGE_MAX			2048


int	g_interactionAntlionGuardFoundPhysicsObject = 0;	// We're moving to a physics object to shove it, don't all choose the same object
int	g_interactionAntlionGuardShovedPhysicsObject = 0;	// We've punted an object, it is now clear to be chosen by others

//==================================================
// AntlionGuardSchedules
//==================================================

enum
{
	SCHED_ANTLIONGUARD_CHARGE = LAST_SHARED_SCHEDULE,
	SCHED_ANTLIONGUARD_CHARGE_CRASH,
	SCHED_ANTLIONGUARD_CHARGE_CANCEL,
	SCHED_ANTLIONGUARD_PHYSICS_ATTACK,
	SCHED_ANTLIONGUARD_PHYSICS_DAMAGE_HEAVY,
	SCHED_ANTLIONGUARD_UNBURROW,
	SCHED_ANTLIONGUARD_CHARGE_TARGET,
	SCHED_ANTLIONGUARD_FIND_CHARGE_POSITION,
	SCHED_ANTLIONGUARD_MELEE_ATTACK1,
	SCHED_ANTLIONGUARD_SUMMON,
	SCHED_ANTLIONGUARD_PATROL_RUN,
	SCHED_ANTLIONGUARD_ROAR,
	SCHED_ANTLIONGUARD_CHASE_ENEMY_TOLERANCE,
	SCHED_FORCE_ANTLIONGUARD_PHYSICS_ATTACK,
	SCHED_ANTLIONGUARD_CANT_ATTACK,
	SCHED_ANTLIONGUARD_TAKE_COVER_FROM_ENEMY,
	SCHED_ANTLIONGUARD_CHASE_ENEMY
};


//==================================================
// AntlionGuardTasks
//==================================================

enum
{
	TASK_ANTLIONGUARD_CHARGE = LAST_SHARED_TASK,
	TASK_ANTLIONGUARD_GET_PATH_TO_PHYSOBJECT,
	TASK_ANTLIONGUARD_SHOVE_PHYSOBJECT,
	TASK_ANTLIONGUARD_SUMMON,
	TASK_ANTLIONGUARD_SET_FLINCH_ACTIVITY,
	TASK_ANTLIONGUARD_GET_PATH_TO_CHARGE_POSITION,
	TASK_ANTLIONGUARD_GET_PATH_TO_NEAREST_NODE,
	TASK_ANTLIONGUARD_GET_CHASE_PATH_ENEMY_TOLERANCE,
	TASK_ANTLIONGUARD_OPPORTUNITY_THROW,
	TASK_ANTLIONGUARD_FIND_PHYSOBJECT,
};

//==================================================
// AntlionGuardConditions
//==================================================

enum
{
	COND_ANTLIONGUARD_PHYSICS_TARGET = LAST_SHARED_CONDITION,
	COND_ANTLIONGUARD_PHYSICS_TARGET_INVALID,
	COND_ANTLIONGUARD_HAS_CHARGE_TARGET,
	COND_ANTLIONGUARD_CAN_SUMMON,
	COND_ANTLIONGUARD_CAN_CHARGE
};

enum
{	
	SQUAD_SLOT_ANTLIONGUARD_CHARGE = LAST_SHARED_SQUADSLOT,
};

//==================================================
// AntlionGuard Activities
//==================================================

Activity ACT_ANTLIONGUARD_SEARCH;
Activity ACT_ANTLIONGUARD_PEEK_FLINCH;
Activity ACT_ANTLIONGUARD_PEEK_ENTER;
Activity ACT_ANTLIONGUARD_PEEK_EXIT;
Activity ACT_ANTLIONGUARD_PEEK1;
Activity ACT_ANTLIONGUARD_BARK;
Activity ACT_ANTLIONGUARD_PEEK_SIGHTED;
Activity ACT_ANTLIONGUARD_SHOVE_PHYSOBJECT;
Activity ACT_ANTLIONGUARD_FLINCH_LIGHT;
Activity ACT_ANTLIONGUARD_UNBURROW;
Activity ACT_ANTLIONGUARD_ROAR;
Activity ACT_ANTLIONGUARD_RUN_HURT;

// Flinches
Activity ACT_ANTLIONGUARD_PHYSHIT_FR;
Activity ACT_ANTLIONGUARD_PHYSHIT_FL;
Activity ACT_ANTLIONGUARD_PHYSHIT_RR;
Activity ACT_ANTLIONGUARD_PHYSHIT_RL;

// Charge
Activity ACT_ANTLIONGUARD_CHARGE_START;
Activity ACT_ANTLIONGUARD_CHARGE_CANCEL;
Activity ACT_ANTLIONGUARD_CHARGE_RUN;
Activity ACT_ANTLIONGUARD_CHARGE_CRASH;
Activity ACT_ANTLIONGUARD_CHARGE_STOP;
Activity ACT_ANTLIONGUARD_CHARGE_HIT;
Activity ACT_ANTLIONGUARD_CHARGE_ANTICIPATION;

// Anim events
int AE_ANTLIONGUARD_CHARGE_HIT;
int AE_ANTLIONGUARD_SHOVE_PHYSOBJECT;
int AE_ANTLIONGUARD_SHOVE;
int AE_ANTLIONGUARD_FOOTSTEP_LIGHT;
int AE_ANTLIONGUARD_FOOTSTEP_HEAVY;
int AE_ANTLIONGUARD_CHARGE_EARLYOUT;
int AE_ANTLIONGUARD_VOICE_GROWL;
int AE_ANTLIONGUARD_VOICE_BARK;
int AE_ANTLIONGUARD_VOICE_PAIN;
int AE_ANTLIONGUARD_VOICE_SQUEEZE;
int AE_ANTLIONGUARD_VOICE_SCRATCH;
int AE_ANTLIONGUARD_VOICE_GRUNT;
int AE_ANTLIONGUARD_VOICE_ROAR;
int AE_ANTLIONGUARD_BURROW_OUT;

struct PhysicsObjectCriteria_t
{
	CEntity *pTarget;
	Vector	vecCenter;		// Center point to look around
	float	flRadius;		// Radius to search within
	float	flTargetCone;
	bool	bPreferObjectsAlongTargetVector;	// Prefer objects that we can strike easily as we move towards our target
	float	flNearRadius;						// If we won't hit the player with the object, but get this close, throw anyway
};

#define MAX_FAILED_PHYSOBJECTS 8

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
class CNPC_AntlionGuard : public CAI_BaseAntlionBase
{
public:
	CE_DECLARE_CLASS( CNPC_AntlionGuard, CAI_BaseAntlionBase );

	//DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	CNPC_AntlionGuard( void );

	Class_T	Classify( void ) { return CLASS_ANTLION; }
	virtual int		GetSoundInterests( void ) { return (SOUND_WORLD|SOUND_COMBAT|SOUND_PLAYER|SOUND_DANGER); }
	virtual bool	QueryHearSound( CSound *pSound );

	const impactdamagetable_t &GetPhysicsImpactDamageTable( void );

	virtual int		MeleeAttack1Conditions( float flDot, float flDist );
	virtual int		SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode );

	virtual int		TranslateSchedule( int scheduleType );
	virtual int		OnTakeDamage_Alive( const CTakeDamageInfo &info );
	virtual void	DeathSound( const CTakeDamageInfo &info );
	virtual void	Event_Killed( const CTakeDamageInfo &info );
	virtual int		SelectSchedule( void );

	virtual float GetAutoAimRadius() { return 36.0f; }
	
	virtual void	Precache( void );
	virtual void	Spawn( void );
	virtual void	Activate( void );
	virtual void	HandleAnimEvent( animevent_t *pEvent );
	virtual void	UpdateEfficiency( bool bInPVS )	{ SetEfficiency( ( GetSleepState() != AISS_AWAKE ) ? AIE_DORMANT : AIE_NORMAL ); SetMoveEfficiency( AIME_NORMAL ); }
	virtual void	PrescheduleThink( void );
	virtual void	GatherConditions( void );
	virtual void	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator );
	virtual void	StartTask( const Task_t *pTask );
	virtual void	RunTask( const Task_t *pTask );
	virtual void	StopLoopingSounds();
	virtual bool	HandleInteraction( int interactionType, void *data, CBaseEntity *sender );
	
	// Input handlers.
	void	InputSetShoveTarget( inputdata_t &inputdata );
	void	InputSetChargeTarget( inputdata_t &inputdata );
	void	InputClearChargeTarget( inputdata_t &inputdata );
	void	InputUnburrow( inputdata_t &inputdata );
	void	InputRagdoll( inputdata_t &inputdata );
	void	InputEnableBark( inputdata_t &inputdata );
	void	InputDisableBark( inputdata_t &inputdata );
	void	InputSummonedAntlionDied( inputdata_t &inputdata );
	void	InputEnablePreferPhysicsAttack( inputdata_t &inputdata );
	void	InputDisablePreferPhysicsAttack( inputdata_t &inputdata );

	virtual bool	IsLightDamage( const CTakeDamageInfo &info );
	virtual bool	IsHeavyDamage( const CTakeDamageInfo &info );
	virtual bool	OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval );
	virtual bool	BecomeRagdollOnClient( const Vector &force );
	virtual void	UpdateOnRemove( void );
	virtual bool	IsUnreachable( CBaseEntity* pEntity );			// Is entity is unreachable?

	virtual float	MaxYawSpeed( void );
	virtual bool	OverrideMove( float flInterval );
	virtual bool	CanBecomeRagdoll( void );

	virtual bool	ShouldProbeCollideAgainstEntity( CBaseEntity *pEntity );

	virtual Activity	NPC_TranslateActivity( Activity baseAct );

#if HL2_EPISODIC
	//---------------------------------
	// Navigation & Movement -- prevent stopping paths for the guard
	//---------------------------------
	class CNavigator : public CAI_ComponentWithOuter<CNPC_AntlionGuard, CAI_Navigator>
	{
		typedef CAI_ComponentWithOuter<CNPC_AntlionGuard, CAI_Navigator> BaseClass;
	public:
		CNavigator( CNPC_AntlionGuard *pOuter )
			:	BaseClass( pOuter )
		{
		}

		bool GetStoppingPath( CAI_WaypointList *pClippedWaypoints );
	};
	CAI_Navigator *	CreateNavigator()	{ return new CNavigator( this );	}
#endif

	DEFINE_CUSTOM_AI;

private:

	inline bool CanStandAtPoint( const Vector &vecPos, Vector *pOut );
	bool	RememberFailedPhysicsTarget( CEntity *pTarget );
	void	GetPhysicsShoveDir( CEntity *pObject, float flMass, Vector *pOut );
	void	CreateGlow( CE_CSprite **pSprite, const char *pAttachName );
	void	DestroyGlows( void );
	void	Footstep( bool bHeavy );
	int		SelectCombatSchedule( void );
	int		SelectUnreachableSchedule( void );
	bool	CanSummon( bool bIgnoreTime );
	void	SummonAntlions( void );
					
	void	ChargeLookAhead( void );
	bool	EnemyIsRightInFrontOfMe( CEntity **pEntity );
	bool	HandleChargeImpact( Vector vecImpact, CEntity *pEntity );
	bool	ShouldCharge( const Vector &startPos, const Vector &endPos, bool useTime, bool bCheckForCancel );
	bool	ShouldWatchEnemy( void );
					
	void	ImpactShock( const Vector &origin, float radius, float magnitude, CEntity *pIgnored = NULL );
	void	BuildScheduleTestBits( void );
	void	Shove( void );
	void	FoundEnemy( void );
	void	LostEnemy( void );
	void	UpdateHead( void );
	void	UpdatePhysicsTarget( bool bPreferObjectsAlongTargetVector, float flRadius = ANTLIONGUARD_FARTHEST_PHYSICS_OBJECT );
	void	MaintainPhysicsTarget( void );
	void	ChargeDamage( CEntity *pTarget );
	void	StartSounds( void );
	void	SetHeavyDamageAnim( const Vector &vecSource );
	float	ChargeSteer( void );
	CEntity *FindPhysicsObjectTarget( const PhysicsObjectCriteria_t &criteria );
	Vector	GetPhysicsHitPosition( CEntity *pObject, CEntity *pTarget, Vector *vecTrajectory, float *flClearDistance );
	bool	CanStandAtShoveTarget( CEntity *pShoveObject, CEntity *pTarget, Vector *pOut );
	CEntity *GetNextShoveTarget( CEntity *pLastEntity, AISightIter_t &iter );

	int				m_nFlinchActivity;

	bool			m_bStopped;
	bool			m_bIsBurrowed;
	bool			m_bBarkEnabled;
	float			m_flNextSummonTime;
	int				m_iNumLiveAntlions;
							
	float			m_flSearchNoiseTime;
	float			m_flAngerNoiseTime;
	float			m_flBreathTime;
	float			m_flChargeTime;
	float			m_flPhysicsCheckTime;
	float			m_flNextHeavyFlinchTime;
	float			m_flNextRoarTime;
	int				m_iChargeMisses;
	bool			m_bDecidedNotToStop;
	bool			m_bPreferPhysicsAttack;

	bool			m_bCavernBreed;	// If this guard is meant to be a cavern dweller (uses different assets)
	bool			m_bInCavern;		// Behavioral hint telling the guard to change his behavior

	Vector			m_vecPhysicsTargetStartPos;
	Vector			m_vecPhysicsHitPosition;
					
	CFakeHandle		m_hShoveTarget;
	CFakeHandle		m_hChargeTarget;
	CFakeHandle		m_hChargeTargetPosition;
	CFakeHandle		m_hOldTarget;
	CFakeHandle		m_hPhysicsTarget;
					
	CUtlVectorFixed<CEntity *, MAX_FAILED_PHYSOBJECTS>		m_FailedPhysicsTargets;

	COutputEvent	m_OnSummon;

	CSoundPatch		*m_pGrowlHighSound;
	CSoundPatch		*m_pGrowlLowSound;
	CSoundPatch		*m_pGrowlIdleSound;
	CSoundPatch		*m_pBreathSound;
	CSoundPatch		*m_pConfusedSound;

	string_t		m_iszPhysicsPropClass;
	string_t		m_strShoveTargets;

	CE_CSprite		*m_hCaveGlow[2];

protected:

	int m_poseThrow;
	int m_poseHead_Yaw, m_poseHead_Pitch;
	virtual void	PopulatePoseParameters( void );


// inline accessors
public:	
	inline bool IsCavernBreed( void ) const { return m_bCavernBreed; }
	inline bool IsInCavern( void ) const { return m_bInCavern; }
};



LINK_ENTITY_TO_CUSTOM_CLASS( npc_antlionguard, cycler, CNPC_AntlionGuard );



BEGIN_DATADESC( CNPC_AntlionGuard )

	DEFINE_FIELD( m_nFlinchActivity,	FIELD_INTEGER ),
	DEFINE_FIELD( m_bStopped,			FIELD_BOOLEAN ), 	
	DEFINE_KEYFIELD( m_bIsBurrowed,		FIELD_BOOLEAN, "startburrowed" ),
	DEFINE_KEYFIELD( m_bBarkEnabled,	FIELD_BOOLEAN, "allowbark" ),
	DEFINE_FIELD( m_flNextSummonTime,	FIELD_TIME ),
	DEFINE_FIELD( m_iNumLiveAntlions,	FIELD_INTEGER ),

	DEFINE_FIELD( m_flSearchNoiseTime,	FIELD_TIME ), 
	DEFINE_FIELD( m_flAngerNoiseTime,	FIELD_TIME ), 
	DEFINE_FIELD( m_flBreathTime,		FIELD_TIME ), 
	DEFINE_FIELD( m_flChargeTime,		FIELD_TIME ),

	DEFINE_FIELD( m_hShoveTarget,				FIELD_EHANDLE ), 
	DEFINE_FIELD( m_hChargeTarget,				FIELD_EHANDLE ), 
	DEFINE_FIELD( m_hChargeTargetPosition,		FIELD_EHANDLE ), 
	DEFINE_FIELD( m_hOldTarget,					FIELD_EHANDLE ),
	// m_FailedPhysicsTargets	// We do not save/load these

	DEFINE_FIELD( m_hPhysicsTarget,				FIELD_EHANDLE ),
	DEFINE_FIELD( m_vecPhysicsTargetStartPos,	FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_vecPhysicsHitPosition,		FIELD_POSITION_VECTOR ),
	DEFINE_FIELD( m_flPhysicsCheckTime,			FIELD_TIME ),
	DEFINE_FIELD( m_flNextHeavyFlinchTime,		FIELD_TIME ),
	DEFINE_FIELD( m_flNextRoarTime,				FIELD_TIME ),
	DEFINE_FIELD( m_iChargeMisses,				FIELD_INTEGER ),
	DEFINE_FIELD( m_bDecidedNotToStop,			FIELD_BOOLEAN ),
	DEFINE_FIELD( m_bPreferPhysicsAttack,		FIELD_BOOLEAN ),


	DEFINE_KEYFIELD( m_bCavernBreed,FIELD_BOOLEAN, "cavernbreed" ),
	DEFINE_KEYFIELD( m_bInCavern,	FIELD_BOOLEAN, "incavern" ),
	DEFINE_KEYFIELD( m_strShoveTargets,	FIELD_STRING, "shovetargets" ),

	DEFINE_AUTO_ARRAY( m_hCaveGlow, FIELD_CLASSPTR ),

	//DEFINE_OUTPUT( m_OnSummon,			"OnSummon" ),

	/*DEFINE_SOUNDPATCH( m_pGrowlHighSound ),
	DEFINE_SOUNDPATCH( m_pGrowlLowSound ),
	DEFINE_SOUNDPATCH( m_pGrowlIdleSound ),
	DEFINE_SOUNDPATCH( m_pBreathSound ),
	DEFINE_SOUNDPATCH( m_pConfusedSound ),*/

	DEFINE_INPUTFUNC( FIELD_STRING,	"SetShoveTarget", InputSetShoveTarget ),
	DEFINE_INPUTFUNC( FIELD_STRING,	"SetChargeTarget", InputSetChargeTarget ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"ClearChargeTarget", InputClearChargeTarget ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"Unburrow", InputUnburrow ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"Ragdoll", InputRagdoll ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"EnableBark", InputEnableBark ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"DisableBark", InputDisableBark ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"SummonedAntlionDied", InputSummonedAntlionDied ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"EnablePreferPhysicsAttack", InputEnablePreferPhysicsAttack ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"DisablePreferPhysicsAttack", InputDisablePreferPhysicsAttack ),

END_DATADESC()


//Fast Growl (Growl High)
envelopePoint_t envAntlionGuardFastGrowl[] =
{
	{	1.0f, 1.0f,
		0.2f, 0.4f,
	},
	{	0.1f, 0.1f,
		0.8f, 1.0f,
	},
	{	0.0f, 0.0f,
		0.4f, 0.8f,
	},
};


//Bark 1 (Growl High)
envelopePoint_t envAntlionGuardBark1[] =
{
	{	1.0f, 1.0f,
		0.1f, 0.2f,
	},
	{	0.0f, 0.0f,
		0.4f, 0.6f,
	},
};

//Bark 2 (Confused)
envelopePoint_t envAntlionGuardBark2[] =
{
	{	1.0f, 1.0f,
		0.1f, 0.2f,
	},
	{	0.2f, 0.3f,
		0.1f, 0.2f,
	},
	{	0.0f, 0.0f,
		0.4f, 0.6f,
	},
};

//Pain
envelopePoint_t envAntlionGuardPain1[] =
{
	{	1.0f, 1.0f,
		0.1f, 0.2f,
	},
	{	-1.0f, -1.0f,
		0.5f, 0.8f,
	},
	{	0.1f, 0.2f,
		0.1f, 0.2f,
	},
	{	0.0f, 0.0f,
		0.5f, 0.75f,
	},
};

//Squeeze (High Growl)
envelopePoint_t envAntlionGuardSqueeze[] =
{
	{	1.0f, 1.0f,
		0.1f, 0.2f,
	},
	{	0.0f, 0.0f,
		1.0f, 1.5f,
	},
};

//Scratch (Low Growl)
envelopePoint_t envAntlionGuardScratch[] =
{
	{	1.0f, 1.0f,
		0.4f, 0.6f,
	},
	{	0.5f, 0.5f,
		0.4f, 0.6f,
	},
	{	0.0f, 0.0f,
		1.0f, 1.5f,
	},
};

//Grunt
envelopePoint_t envAntlionGuardGrunt[] =
{
	{	0.6f, 1.0f,
		0.1f, 0.2f,
	},
	{	0.0f, 0.0f,
		0.1f, 0.2f,
	},
};

envelopePoint_t envAntlionGuardGrunt2[] =
{
	{	0.2f, 0.4f,
		0.1f, 0.2f,
	},
	{	0.0f, 0.0f,
		0.4f, 0.6f,
	},
};

//==============================================================================================
// ANTLION GUARD PHYSICS DAMAGE TABLE
//==============================================================================================
static impactentry_t antlionGuardLinearTable[] =
{
	{ 100*100,	10 },
	{ 250*250,	25 },
	{ 350*350,	50 },
	{ 500*500,	75 },
	{ 1000*1000,100 },
};

static impactentry_t antlionGuardAngularTable[] =
{
	{  50* 50, 10 },
	{ 100*100, 25 },
	{ 150*150, 50 },
	{ 200*200, 75 },
};

impactdamagetable_t gAntlionGuardImpactDamageTable =
{
	antlionGuardLinearTable,
	antlionGuardAngularTable,
	
	ARRAYSIZE(antlionGuardLinearTable),
	ARRAYSIZE(antlionGuardAngularTable),

	200*200,// minimum linear speed squared
	180*180,// minimum angular speed squared (360 deg/s to cause spin/slice damage)
	15,		// can't take damage from anything under 15kg

	10,		// anything less than 10kg is "small"
	5,		// never take more than 1 pt of damage from anything under 15kg
	128*128,// <15kg objects must go faster than 36 in/s to do damage

	45,		// large mass in kg 
	2,		// large mass scale (anything over 500kg does 4X as much energy to read from damage table)
	1,		// large mass falling scale
	0,		// my min velocity
};

//-----------------------------------------------------------------------------
// Purpose: 
// Output : const impactdamagetable_t
//-----------------------------------------------------------------------------
const impactdamagetable_t &CNPC_AntlionGuard::GetPhysicsImpactDamageTable( void )
{
	return gAntlionGuardImpactDamageTable;
}

//==================================================
// CNPC_AntlionGuard
//==================================================

CNPC_AntlionGuard::CNPC_AntlionGuard( void )
{
	m_bCavernBreed = false;
	m_bInCavern = false;

	m_iszPhysicsPropClass = AllocPooledString("prop_physics");
}


void CNPC_AntlionGuard::UpdateOnRemove( void )
{
	DestroyGlows();

	// Chain to the base class
	BaseClass::UpdateOnRemove();
}

void CNPC_AntlionGuard::Precache( void )
{
	PrecacheModel( ANTLIONGUARD_MODEL );

	PrecacheScriptSound( "NPC_AntlionGuard.Shove" );
	PrecacheScriptSound( "NPC_AntlionGuard.HitHard" );

	if ( HasSpawnFlags(SF_ANTLIONGUARD_INSIDE_FOOTSTEPS) )
	{
		PrecacheScriptSound( "NPC_AntlionGuard.Inside.StepLight" );
		PrecacheScriptSound( "NPC_AntlionGuard.Inside.StepHeavy" );
	}
	else
	{
		PrecacheScriptSound( "NPC_AntlionGuard.StepLight" );
		PrecacheScriptSound( "NPC_AntlionGuard.StepHeavy" );
	}

	PrecacheScriptSound( "NPC_AntlionGuard.NearStepLight" );
	PrecacheScriptSound( "NPC_AntlionGuard.NearStepHeavy" );
	PrecacheScriptSound( "NPC_AntlionGuard.FarStepLight" );
	PrecacheScriptSound( "NPC_AntlionGuard.FarStepHeavy" );
	PrecacheScriptSound( "NPC_AntlionGuard.BreatheLoop" );
	PrecacheScriptSound( "NPC_AntlionGuard.ShellCrack" );
	PrecacheScriptSound( "NPC_AntlionGuard.Pain_Roar" );

	PrecacheScriptSound( "NPC_AntlionGuard.Anger" );
	PrecacheScriptSound( "NPC_AntlionGuard.Roar" );
	PrecacheScriptSound( "NPC_AntlionGuard.Die" );

	PrecacheScriptSound( "NPC_AntlionGuard.GrowlHigh" );
	PrecacheScriptSound( "NPC_AntlionGuard.GrowlIdle" );
	PrecacheScriptSound( "NPC_AntlionGuard.BreathSound" );
	PrecacheScriptSound( "NPC_AntlionGuard.Confused" );
	PrecacheScriptSound( "NPC_AntlionGuard.Fallover" );

	PrecacheScriptSound( "NPC_AntlionGuard.FrustratedRoar" );

	PrecacheParticleSystem( "blood_antlionguard_injured_light" );
	PrecacheParticleSystem( "blood_antlionguard_injured_heavy" );

	BaseClass::Precache();
}

void CNPC_AntlionGuard::DestroyGlows( void )
{
	if ( m_hCaveGlow[0] )
	{
		UTIL_Remove( m_hCaveGlow[0] );

		// reset it to NULL in case there is a double death cleanup for some reason.
		m_hCaveGlow[0] = NULL;
	}

	if ( m_hCaveGlow[1] )
	{
		UTIL_Remove( m_hCaveGlow[1] );

		// reset it to NULL in case there is a double death cleanup for some reason.
		m_hCaveGlow[1] = NULL;
	}
}

void CNPC_AntlionGuard::CreateGlow( CE_CSprite **pSprite, const char *pAttachName )
{
	if ( pSprite == NULL )
		return;

	// Create the glow sprite
	*pSprite = CE_CSprite::SpriteCreate( "sprites/grubflare1.vmt", GetLocalOrigin(), false );
	Assert( *pSprite );
	if ( *pSprite == NULL )
		return;

	(*pSprite)->TurnOn();
	(*pSprite)->SetTransparency( kRenderWorldGlow, 156, 169, 121, 164, kRenderFxNoDissipation );
	(*pSprite)->SetScale( 1.0f );
	(*pSprite)->SetGlowProxySize( 16.0f );
	int nAttachment = LookupAttachment( pAttachName );
	(*pSprite)->SetParent( BaseEntity(), nAttachment );
	(*pSprite)->SetLocalOrigin( vec3_origin );

	// Don't uselessly animate, we're a static sprite!
	(*pSprite)->SetThink( NULL );
	(*pSprite)->SetNextThink( TICK_NEVER_THINK );
}

void CNPC_AntlionGuard::Spawn( void )
{
	Precache();

	SetModel( ANTLIONGUARD_MODEL );

	// Switch our skin (for now), if we're the cavern guard
	if ( m_bCavernBreed )
	{
		m_nSkin = 1;
		
		// Add glows
		CreateGlow( &(m_hCaveGlow[0]), "attach_glow1" );
		CreateGlow( &(m_hCaveGlow[1]), "attach_glow2" );
	}
	else
	{
		m_nSkin = 0; //CE
		m_hCaveGlow[0] = NULL;
		m_hCaveGlow[1] = NULL;
	}

	SetHullType( HULL_LARGE );
	SetHullSizeNormal();
	SetDefaultEyeOffset();
	
	SetSolid( SOLID_BBOX );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	SetMoveType( MOVETYPE_STEP );

	SetNavType( NAV_GROUND );
	SetBloodColor( BLOOD_COLOR_YELLOW );

	m_iHealth = sk_antlionguard_health.GetInt();
	*(m_iMaxHealth) = m_iHealth;
	m_flFieldOfView	= ANTLIONGUARD_FOV_NORMAL;
	
	m_flPhysicsCheckTime = 0;
	m_flChargeTime = 0;
	m_flNextRoarTime = 0;
	m_flNextSummonTime = 0;
	m_iNumLiveAntlions = 0;
	m_iChargeMisses = 0;
	m_flNextHeavyFlinchTime = 0;
	m_bDecidedNotToStop = false;

	ClearHintGroup();
	
	m_bStopped = false;
	
	m_HackedGunPos->Init(10,0,30);

	CapabilitiesClear();
	CapabilitiesAdd( bits_CAP_MOVE_GROUND | bits_CAP_INNATE_MELEE_ATTACK1 | bits_CAP_SQUAD );
	CapabilitiesAdd( bits_CAP_SKIP_NAV_GROUND_CHECK );

	NPCInit();

	BaseClass::Spawn();

	//See if we're supposed to start burrowed
	if ( m_bIsBurrowed )
	{
		AddEffects( EF_NODRAW );
		AddFlag( FL_NOTARGET );

		m_spawnflags |= SF_NPC_GAG;
		
		AddSolidFlags( FSOLID_NOT_SOLID );
		
		m_takedamage = DAMAGE_NO;

		if ( m_hCaveGlow[0] )
			m_hCaveGlow[0]->TurnOff();
		
		if ( m_hCaveGlow[1] )
			m_hCaveGlow[1]->TurnOff();
	}

	// Do not dissolve
	AddEFlags( EFL_NO_DISSOLVE );

	// We get a minute of free knowledge about the target
	GetEnemies()->SetEnemyDiscardTime( 120.0f );
	GetEnemies()->SetFreeKnowledgeDuration( 60.0f );

	// We need to bloat the absbox to encompass all the hitboxes
	Vector absMin = -Vector(100,100,0);
	Vector absMax = Vector(100,100,128);

	CollisionProp()->SetSurroundingBoundsType( USE_SPECIFIED_BOUNDS, &absMin, &absMax );
}

void CNPC_AntlionGuard::Activate( void )
{
	BaseClass::Activate();

	// Find all nearby physics objects and add them to the list of objects we will sense
	CEntity *pObject = NULL;
	while ( ( pObject = g_helpfunc.FindEntityInSphere( pObject, WorldSpaceCenter(), 2500 ) ) != NULL )
	{
		// Can't throw around debris
		if ( pObject->GetCollisionGroup() == COLLISION_GROUP_DEBRIS )
			continue;

		// We can only throw a few types of things
		if(strstr(pObject->GetClassname(), "prop_physics") == NULL && !FClassnameIs( pObject, "func_physbox" ))
		//if ( !FClassnameIs( pObject, "prop_physics" ) && !FClassnameIs( pObject, "func_physbox" ) )
			continue;

		// Ensure it's mass is within range
		IPhysicsObject *pPhysObj = pObject->VPhysicsGetObject();
		if( ( pPhysObj == NULL ) || ( pPhysObj->GetMass() > ANTLIONGUARD_MAX_OBJECT_MASS ) || ( pPhysObj->GetMass() < ANTLIONGUARD_MIN_OBJECT_MASS ) )
			continue;

		// Tell the AI sensing list that we want to consider this
		g_AI_SensedObjectsManager->AddEntity( pObject->BaseEntity() );
	}
}

bool CNPC_AntlionGuard::CanSummon( bool bIgnoreTime )
{
	if ( !m_bBarkEnabled )
		return false;

	if ( !bIgnoreTime && m_flNextSummonTime > gpGlobals->curtime )
		return false;

	// Hit the max number of them allowed? Only summon when we're 2 down.
	if ( m_iNumLiveAntlions >= std::max(1, ANTLIONGUARD_SUMMON_COUNT-1) )
		return false;

	return true;
}

int CNPC_AntlionGuard::SelectUnreachableSchedule( void )
{
	// If we're in the cavern setting, we opt out of this
	if ( m_bInCavern )
		return SCHED_ANTLIONGUARD_CHASE_ENEMY_TOLERANCE;
	// Summon antlions if we can
	if ( HasCondition( COND_ANTLIONGUARD_CAN_SUMMON ) )
		return SCHED_ANTLIONGUARD_SUMMON;

	// First, look for objects near ourselves
	PhysicsObjectCriteria_t	criteria;
	criteria.bPreferObjectsAlongTargetVector = false;
	criteria.flNearRadius = (40*12);
	criteria.flRadius = (250*12);
	criteria.flTargetCone = -1.0f;
	criteria.pTarget = GetEnemy();
	criteria.vecCenter = GetAbsOrigin();

	CEntity *pTarget = FindPhysicsObjectTarget( criteria );
	if ( pTarget == NULL && GetEnemy() )
	{
		// Use the same criteria, but search near the target instead of us
		criteria.vecCenter = GetEnemy()->GetAbsOrigin();
		pTarget = FindPhysicsObjectTarget( criteria );
	}

	// If we found one, we'll want to attack it
	if ( pTarget )
	{
		m_hPhysicsTarget.Set(pTarget->BaseEntity());
		SetCondition( COND_ANTLIONGUARD_PHYSICS_TARGET );
		m_vecPhysicsTargetStartPos = m_hPhysicsTarget->WorldSpaceCenter();
		
		// Tell any squadmates I'm going for this item so they don't as well
		if ( GetSquad() != NULL )
		{
			GetSquad()->BroadcastInteraction( g_interactionAntlionGuardFoundPhysicsObject, (void *)((CBaseEntity *)m_hPhysicsTarget->BaseEntity()), this );
		}

		return SCHED_ANTLIONGUARD_PHYSICS_ATTACK;
	}

	// Deal with a visible player
	if ( HasCondition( COND_SEE_ENEMY ) )
	{
		// Roar at the player as show of frustration
		if ( m_flNextRoarTime < gpGlobals->curtime )
		{
			m_flNextRoarTime = gpGlobals->curtime + RandomFloat( 20,40 );
			return SCHED_ANTLIONGUARD_ROAR;
		}

		// If we're under attack, then let's leave for a bit
		if ( GetEnemy() && HasCondition( COND_HEAVY_DAMAGE ) )
		{
			Vector dir = GetEnemy()->GetAbsOrigin() - GetAbsOrigin();
			VectorNormalize(dir);

			GetMotor()->SetIdealYaw( -dir );
			
			return SCHED_ANTLIONGUARD_TAKE_COVER_FROM_ENEMY;
		}
	}

	// If we're too far away, try to close distance to our target first
	float flDistToEnemySqr = ( GetEnemy()->GetAbsOrigin() - GetAbsOrigin() ).LengthSqr();
	if ( flDistToEnemySqr > Square( 100*12 ) )
		return SCHED_ANTLIONGUARD_CHASE_ENEMY_TOLERANCE;

	// Fire that we're unable to reach our target!
	if ( GetEnemy() && GetEnemy()->IsPlayer() )
	{
		m_OnLostPlayer->FireOutput( this, this );
	}

	m_OnLostEnemy->FireOutput( this, this );
	GetEnemies()->MarkAsEluded( GetEnemy_CBase() );

	// Move randomly for the moment
	return SCHED_ANTLIONGUARD_CANT_ATTACK;
}

int CNPC_AntlionGuard::SelectCombatSchedule( void )
{
	ClearHintGroup();

	/*
	bool bCanCharge = false;
	if ( HasCondition( COND_SEE_ENEMY ) )
	{
		bCanCharge = ShouldCharge( GetAbsOrigin(), GetEnemy()->GetAbsOrigin(), true, false );
	}
	*/

	// Attack if we can
	if ( HasCondition(COND_CAN_MELEE_ATTACK1) )
		return SCHED_MELEE_ATTACK1;

	// Otherwise, summon antlions
	if ( HasCondition(COND_ANTLIONGUARD_CAN_SUMMON) )
	{
		// If I can charge, and have antlions, charge instead
		if ( HasCondition( COND_ANTLIONGUARD_CAN_CHARGE ) && m_iNumLiveAntlions )
			return SCHED_ANTLIONGUARD_CHARGE;

		return SCHED_ANTLIONGUARD_SUMMON;
	}

	// See if we can bark
	if ( HasCondition( COND_ENEMY_UNREACHABLE ) )
		return SelectUnreachableSchedule();

	//Physics target
	if ( HasCondition( COND_ANTLIONGUARD_PHYSICS_TARGET ) && !m_bInCavern )
		return SCHED_ANTLIONGUARD_PHYSICS_ATTACK;

	// If we've missed a couple of times, and we can summon, make it harder
	if ( m_iChargeMisses >= 2 && CanSummon(true) )
	{
		m_iChargeMisses--;
		return SCHED_ANTLIONGUARD_SUMMON;
	}

	// Charging
	if ( HasCondition( COND_ANTLIONGUARD_CAN_CHARGE ) )
	{
		// Don't let other squad members charge while we're doing it
		OccupyStrategySlot( SQUAD_SLOT_ANTLIONGUARD_CHARGE );
		return SCHED_ANTLIONGUARD_CHARGE;
	}

	return BaseClass::SelectSchedule();
}


bool CNPC_AntlionGuard::ShouldCharge( const Vector &startPos, const Vector &endPos, bool useTime, bool bCheckForCancel )
{
	// Don't charge in tight spaces unless forced to
	if ( hl2_episodic->GetBool() && m_bInCavern && !(m_hChargeTarget && m_hChargeTarget->IsAlive()) )
		return false;

	// Must have a target
	if ( !GetEnemy() )
		return false;

	// No one else in the squad can be charging already
	if ( IsStrategySlotRangeOccupied( SQUAD_SLOT_ANTLIONGUARD_CHARGE, SQUAD_SLOT_ANTLIONGUARD_CHARGE ) )
		return false;

	// Don't check the distance once we start charging
	if ( !bCheckForCancel )
	{
		// Don't allow use to charge again if it's been too soon
		if ( useTime && ( m_flChargeTime > gpGlobals->curtime ) )
			return false;

		float distance = UTIL_DistApprox2D( startPos, endPos );

		// Must be within our tolerance range
		if ( ( distance < ANTLIONGUARD_CHARGE_MIN ) || ( distance > ANTLIONGUARD_CHARGE_MAX ) )
			return false;
	}

	if ( GetSquad() )
	{
		// If someone in our squad is closer to the enemy, then don't charge (we end up hitting them more often than not!)
		float flOurDistToEnemySqr = ( GetAbsOrigin() - GetEnemy()->GetAbsOrigin() ).LengthSqr();
		AISquadIter_t iter;
		for ( CBaseEntity *pSquadMember = GetSquad()->GetFirstMember( &iter ); pSquadMember; pSquadMember = GetSquad()->GetNextMember( &iter ) )
		{
			CEntity *cent_pSquadMember = CEntity::Instance(pSquadMember);
			if ( cent_pSquadMember->IsAlive() == false || cent_pSquadMember == this )
				continue;

			if ( ( cent_pSquadMember->GetAbsOrigin() - GetEnemy()->GetAbsOrigin() ).LengthSqr() < flOurDistToEnemySqr )
				return false;
		}
	}

	//FIXME: We'd like to exclude small physics objects from this check!

	// We only need to hit the endpos with the edge of our bounding box
	Vector vecDir = endPos - startPos;
	VectorNormalize( vecDir );
	float flWidth = WorldAlignSize().x * 0.5;
	Vector vecTargetPos = endPos - (vecDir * flWidth);

	// See if we can directly move there
	AIMoveTrace_t moveTrace;
	GetMoveProbe()->MoveLimit( NAV_GROUND, startPos, vecTargetPos, MASK_NPCSOLID_BRUSHONLY, GetEnemy(), &moveTrace );
	
	// If we're not blocked, charge
	if ( IsMoveBlocked( moveTrace ) )
	{
		// Don't allow it if it's too close to us
		if ( UTIL_DistApprox( WorldSpaceCenter(), moveTrace.vEndPosition ) < ANTLIONGUARD_CHARGE_MIN )
			return false;

		// Allow some special cases to not block us
		if ( moveTrace.pObstruction != NULL )
		{
			// If we've hit the world, see if it's a cliff
			if ( moveTrace.pObstruction == GetContainingEntity( INDEXENT(0) ) )
			{	
				// Can't be too far above/below the target
				if ( fabs( moveTrace.vEndPosition.z - vecTargetPos.z ) > StepHeight() )
					return false;

				// Allow it if we got pretty close
				if ( UTIL_DistApprox( moveTrace.vEndPosition, vecTargetPos ) < 64 )
					return true;
			}
			
			CEntity *cent = CEntity::Instance(moveTrace.pObstruction);
			// Hit things that will take damage
			if ( cent->m_takedamage != DAMAGE_NO )
				return true;

			// Hit things that will move
			if ( cent->GetMoveType() == MOVETYPE_VPHYSICS )
				return true;
		}

		return false;
	}

	// Only update this if we've requested it
	if ( useTime )
	{
		m_flChargeTime = gpGlobals->curtime + 4.0f;
	}

	return true;
}

int CNPC_AntlionGuard::SelectSchedule( void )
{
	// Don't do anything if we're burrowed
	if ( m_bIsBurrowed )
		return SCHED_IDLE_STAND;

	// Flinch on heavy damage, but not if we've flinched too recently.
	// This is done to prevent stun-locks from grenades.
	if ( !m_bInCavern && HasCondition( COND_HEAVY_DAMAGE ) && m_flNextHeavyFlinchTime < gpGlobals->curtime )
	{
		m_flNextHeavyFlinchTime = gpGlobals->curtime + 8.0f;
		return SCHED_ANTLIONGUARD_PHYSICS_DAMAGE_HEAVY;
	}

	// Prefer to use physics, in this case
	if ( m_bPreferPhysicsAttack )
	{
		// If we have a target, try to go for it
		if ( HasCondition( COND_ANTLIONGUARD_PHYSICS_TARGET ) )
			return SCHED_ANTLIONGUARD_PHYSICS_ATTACK;
	}

	// Charge after a target if it's set
	if ( m_hChargeTarget && m_hChargeTargetPosition )
	{
		ClearCondition( COND_ANTLIONGUARD_HAS_CHARGE_TARGET );
		ClearHintGroup();
		
		if ( m_hChargeTarget->IsAlive() == false )
		{
			m_hChargeTarget.Set(NULL);
			m_hChargeTargetPosition.Set(NULL);
			SetEnemy( (m_hOldTarget) ?  m_hOldTarget->BaseEntity() : NULL);

			if ( m_hOldTarget == NULL )
			{
				m_NPCState = NPC_STATE_ALERT;
			}
		}
		else
		{
			m_hOldTarget.Set(GetEnemy_CBase());
			SetEnemy( m_hChargeTarget->BaseEntity() );
			UpdateEnemyMemory( m_hChargeTarget->BaseEntity() , m_hChargeTarget->GetAbsOrigin() );

			//If we can't see the target, run to somewhere we can
			if ( ShouldCharge( GetAbsOrigin(), GetEnemy()->GetAbsOrigin(), false, false ) == false )
				return SCHED_ANTLIONGUARD_FIND_CHARGE_POSITION;

			return SCHED_ANTLIONGUARD_CHARGE_TARGET;
		}
	}

	// See if we need to clear a path to our enemy
	if ( HasCondition( COND_ENEMY_OCCLUDED ) || HasCondition( COND_ENEMY_UNREACHABLE ) )
	{
		CEntity *pBlocker = GetEnemyOccluder();

		if ( ( pBlocker != NULL ) && strstr(pBlocker->GetClassname(), "prop_physics") != NULL && !m_bInCavern )
		//if ( ( pBlocker != NULL ) && FClassnameIs( pBlocker, "prop_physics" ) && !m_bInCavern )
		{
			m_hPhysicsTarget.Set(pBlocker->BaseEntity());
			return SCHED_ANTLIONGUARD_PHYSICS_ATTACK;
		}
	}

	//Only do these in combat states
	if ( m_NPCState == NPC_STATE_COMBAT && GetEnemy() )
		return SelectCombatSchedule();

	return BaseClass::SelectSchedule();
}


int CNPC_AntlionGuard::MeleeAttack1Conditions( float flDot, float flDist )
{
	// Don't attack again too soon
	if ( GetNextAttack() > gpGlobals->curtime )
		return 0;

	// While charging, we can't melee attack
	if ( IsCurSchedule( SCHED_ANTLIONGUARD_CHARGE ) )
		return 0;

	if ( hl2_episodic->GetBool() && m_bInCavern )
	{
		// Predict where they'll be and see if THAT is within range
		Vector vecPredPos;
		UTIL_PredictedPosition( GetEnemy(), 0.25f, &vecPredPos );
		if ( ( GetAbsOrigin() - vecPredPos ).Length() > ANTLIONGUARD_MELEE1_RANGE )
			return COND_TOO_FAR_TO_ATTACK;
	}
	else
	{
		// Must be close enough
		if ( flDist > ANTLIONGUARD_MELEE1_RANGE )
			return COND_TOO_FAR_TO_ATTACK;
	}

	// Must be within a viable cone
	if ( flDot < ANTLIONGUARD_MELEE1_CONE )
		return COND_NOT_FACING_ATTACK;

	// If the enemy is on top of me, I'm allowed to hit the sucker
	if ( GetEnemy()->GetGroundEntity() == this )
		return COND_CAN_MELEE_ATTACK1;

	trace_t	tr;
	TraceHull_SkipPhysics( WorldSpaceCenter(), GetEnemy()->WorldSpaceCenter(), Vector(-10,-10,-10), Vector(10,10,10), MASK_SHOT_HULL, BaseEntity(), COLLISION_GROUP_NONE, &tr, VPhysicsGetObject()->GetMass() * 0.5 );

	// If we hit anything, go for it
	if ( tr.fraction < 1.0f )
		return COND_CAN_MELEE_ATTACK1;

	return 0;
}

float CNPC_AntlionGuard::MaxYawSpeed( void )
{
	Activity eActivity = GetActivity();

	// Stay still
	if (( eActivity == ACT_ANTLIONGUARD_SEARCH ) || 
		( eActivity == ACT_ANTLIONGUARD_PEEK_ENTER ) || 
		( eActivity == ACT_ANTLIONGUARD_PEEK_EXIT ) || 
		( eActivity == ACT_ANTLIONGUARD_PEEK1 ) || 
		( eActivity == ACT_ANTLIONGUARD_BARK ) || 
		( eActivity == ACT_ANTLIONGUARD_PEEK_SIGHTED ) || 
		( eActivity == ACT_MELEE_ATTACK1 ) )
		return 0.0f;

	CEntity *pEnemy = GetEnemy();

	if ( pEnemy != NULL && pEnemy->IsPlayer() == false )
		return 16.0f;

	// Turn slowly when you're charging
	if ( eActivity == ACT_ANTLIONGUARD_CHARGE_START )
		return 4.0f;

	if ( hl2_episodic->GetBool() && m_bInCavern )
	{
		// Allow a better turning rate when moving quickly but not charging the player
		if ( ( eActivity == ACT_ANTLIONGUARD_CHARGE_RUN ) && IsCurSchedule( SCHED_ANTLIONGUARD_CHARGE ) == false )
			return 16.0f;
	}

	// Turn more slowly as we close in on our target
	if ( eActivity == ACT_ANTLIONGUARD_CHARGE_RUN )
	{
		if ( pEnemy == NULL )
			return 2.0f;

		float dist = UTIL_DistApprox2D( GetEnemy()->WorldSpaceCenter(), WorldSpaceCenter() );

		if ( dist > 512 )
			return 16.0f;

		//FIXME: Alter by skill level
		float yawSpeed = RemapVal( dist, 0, 512, 1.0f, 2.0f );
		yawSpeed = clamp( yawSpeed, 1.0f, 2.0f );

		return yawSpeed;
	}

	if ( eActivity == ACT_ANTLIONGUARD_CHARGE_STOP )
		return 8.0f;

	switch( eActivity )
	{
	case ACT_TURN_LEFT:
	case ACT_TURN_RIGHT:
		return 40.0f;
		break;
	
	case ACT_RUN:
	default:
		return 20.0f;
		break;
	}
}

bool CNPC_AntlionGuard::OverrideMove( float flInterval )
{
	// If the guard's charging, we're handling the movement
	if ( IsCurSchedule( SCHED_ANTLIONGUARD_CHARGE ) )
		return true;

	// TODO: This code increases the guard's ability to successfully hit a target, but adds a new dimension of navigation
	//		 trouble to do with him not being able to "close the distance" between himself and the object he wants to hit.
	//		 Fixing this will require some thought on how he picks the correct distances to his targets and when he's "close enough". -- jdw

	/*
	if ( m_hPhysicsTarget != NULL )
	{
		float flWidth = m_hPhysicsTarget->CollisionProp()->BoundingRadius2D();
		GetLocalNavigator()->AddObstacle( m_hPhysicsTarget->WorldSpaceCenter(), flWidth * 0.75f, AIMST_AVOID_OBJECT );
		//NDebugOverlay::Sphere( m_hPhysicsTarget->WorldSpaceCenter(), vec3_angle, flWidth, 255, 255, 255, 0, true, 0.5f );
	}
	*/

	return BaseClass::OverrideMove( flInterval );
}

void CNPC_AntlionGuard::Shove( void )
{
	if ( GetNextAttack() > gpGlobals->curtime )
		return;

	CEntity *pHurt = NULL;
	CEntity *pTarget;

	pTarget = ( m_hShoveTarget == NULL ) ? GetEnemy() : m_hShoveTarget;

	if ( pTarget == NULL )
	{
		m_hShoveTarget.Set(NULL);
		return;
	}

	//Always damage bullseyes if we're scripted to hit them
	if ( pTarget->Classify() == CLASS_BULLSEYE )
	{
		pTarget->TakeDamage( CTakeDamageInfo( BaseEntity(), BaseEntity(), 1.0f, DMG_CRUSH ) );
		m_hShoveTarget.Set(NULL);
		return;
	}

	float damage = ( pTarget->IsPlayer() ) ? sk_antlionguard_dmg_shove.GetFloat() : 250;

	// If the target's still inside the shove cone, ensure we hit him	
	Vector vecForward, vecEnd;
	AngleVectors( GetAbsAngles(), &vecForward );
  	float flDistSqr = ( pTarget->WorldSpaceCenter() - WorldSpaceCenter() ).LengthSqr();
  	Vector2D v2LOS = ( pTarget->WorldSpaceCenter() - WorldSpaceCenter() ).AsVector2D();
  	Vector2DNormalize(v2LOS);
  	float flDot	= DotProduct2D (v2LOS, vecForward.AsVector2D() );
	if ( flDistSqr < (ANTLIONGUARD_MELEE1_RANGE*ANTLIONGUARD_MELEE1_RANGE) && flDot >= ANTLIONGUARD_MELEE1_CONE )
	{
		vecEnd = pTarget->WorldSpaceCenter();
	}
	else
	{
		vecEnd = WorldSpaceCenter() + ( BodyDirection3D() * ANTLIONGUARD_MELEE1_RANGE );
	}

	// Use the melee trace to ensure we hit everything there
	trace_t tr;
	CTakeDamageInfo	dmgInfo( BaseEntity(), BaseEntity(), damage, DMG_SLASH );
	CTraceFilterMelee traceFilter( BaseEntity(), COLLISION_GROUP_NONE, &dmgInfo, 1.0, true );
	Ray_t ray;
	ray.Init( WorldSpaceCenter(), vecEnd, Vector(-16,-16,-16), Vector(16,16,16) );
	enginetrace->TraceRay( ray, MASK_SHOT_HULL, &traceFilter, &tr );
	pHurt = CEntity::Instance(tr.m_pEnt);

	// Knock things around
	ImpactShock( tr.endpos, 100.0f, 250.0f );

	if ( pHurt )
	{
		Vector	traceDir = ( tr.endpos - tr.startpos );
		VectorNormalize( traceDir );

		// Generate enough force to make a 75kg guy move away at 600 in/sec
		Vector vecForce = traceDir * ImpulseScale( 75, 600 );
		CTakeDamageInfo info( BaseEntity(), BaseEntity(), vecForce, tr.endpos, damage, DMG_CLUB );
		pHurt->TakeDamage( info );

		m_hShoveTarget.Set(NULL);

		EmitSound( "NPC_AntlionGuard.Shove" );

		// If the player, throw him around
		if ( pHurt->IsPlayer() )
		{
			//Punch the view
			pHurt->ViewPunch( QAngle(20,0,-20) );
			
			//Shake the screen
			UTIL_ScreenShake( pHurt->GetAbsOrigin(), 100.0, 1.5, 1.0, 2, SHAKE_START );

			//Red damage indicator
			color32 red = {128,0,0,128};
			UTIL_ScreenFade( pHurt, red, 1.0f, 0.1f, FFADE_IN );

			Vector forward, up;
			AngleVectors( GetLocalAngles(), &forward, NULL, &up );
			pHurt->ApplyAbsVelocityImpulse( forward * 400 + up * 150);

			// in the episodes, the cavern guard poisons the player
#if HL2_EPISODIC
			// If I am a cavern guard attacking the player, and he still lives, then poison him too.
			if ( m_bInCavern && pHurt->IsPlayer() && pHurt->IsAlive() && pHurt->m_iHealth > ANTLIONGUARD_POISON_TO)
			{
				// That didn't finish them. Take them down to one point with poison damage. It'll heal.
				pHurt->TakeDamage( CTakeDamageInfo( this, this, pHurt->m_iHealth - ANTLIONGUARD_POISON_TO, DMG_POISON ) );
			}
#endif

		}	
		else
		{
			m_hShoveTarget.Set(NULL);
		}
	}
}

class CTraceFilterCharge : public CTraceFilterEntitiesOnly
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS_NOBASE( CTraceFilterCharge );
	
	CTraceFilterCharge( const IHandleEntity *passentity, int collisionGroup, CNPC_AntlionGuard *pAttacker )
		: m_pPassEnt(passentity), m_collisionGroup(collisionGroup), m_pAttacker(pAttacker)
	{
	}
	
	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		if ( !StandardFilterRules( pHandleEntity, contentsMask ) )
			return false;

		if ( !PassServerEntityFilter( pHandleEntity, m_pPassEnt ) )
			return false;

		// Don't test if the game code tells us we should ignore this collision...
		CEntity *pEntity = CE_EntityFromEntityHandle( pHandleEntity );
		
		if ( pEntity )
		{
			if ( !pEntity->ShouldCollide( m_collisionGroup, contentsMask ) )
				return false;
			
			if ( !g_helpfunc.GameRules_ShouldCollide( m_collisionGroup, pEntity->GetCollisionGroup() ) )
				return false;

			if ( pEntity->m_takedamage == DAMAGE_NO )
				return false;

			// Translate the vehicle into its driver for damage
			if ( pEntity->GetServerVehicle() != NULL )
			{
				CEntity *pDriver = CEntity::Instance(pEntity->GetServerVehicle()->GetPassenger());

				if ( pDriver != NULL )
				{
					pEntity = pDriver;
				}
			}
	
			Vector	attackDir = pEntity->WorldSpaceCenter() - m_pAttacker->WorldSpaceCenter();
			VectorNormalize( attackDir );

			float	flDamage = ( pEntity->IsPlayer() ) ? sk_antlionguard_dmg_shove.GetFloat(): 250;;

			CTakeDamageInfo info( m_pAttacker->BaseEntity(), m_pAttacker->BaseEntity(), flDamage, DMG_CRUSH );
			CalculateMeleeDamageForce( &info, attackDir, m_pAttacker->WorldSpaceCenter(), 4.0f );

			CCombatCharacter *pVictimBCC = pEntity->MyCombatCharacterPointer();

			// Only do these comparisons between NPCs
			if ( pVictimBCC )
			{
				// Can only damage other NPCs that we hate
				if ( m_pAttacker->IRelationType( pEntity->BaseEntity() ) == D_HT )
				{
					pEntity->TakeDamage( info );
					return true;
				}
			}
			else
			{
				// Otherwise just damage passive objects in our way
				pEntity->TakeDamage( info );
				Pickup_ForcePlayerToDropThisObject( pEntity );
			}
		}

		return false;
	}

public:
	const IHandleEntity *m_pPassEnt;
	int					m_collisionGroup;
	CNPC_AntlionGuard	*m_pAttacker;
};


#define	MIN_FOOTSTEP_NEAR_DIST	Square( 80*12.0f )// ft

void CNPC_AntlionGuard::Footstep( bool bHeavy )
{
	CPlayer *pPlayer = UTIL_GetNearestPlayer(GetAbsOrigin());
	Assert( pPlayer != NULL );
	if ( pPlayer == NULL )
		return;

	float flDistanceToPlayerSqr = ( pPlayer->GetAbsOrigin() - GetAbsOrigin() ).LengthSqr();
	float flNearVolume = RemapValClamped( flDistanceToPlayerSqr, Square(10*12.0f), MIN_FOOTSTEP_NEAR_DIST, VOL_NORM, 0.0f );

	EmitSound_t soundParams;
	CPASAttenuationFilter filter( this );

	if ( bHeavy )
	{
		if ( flNearVolume > 0.0f )
		{
			soundParams.m_pSoundName = "NPC_AntlionGuard.NearStepHeavy";
			soundParams.m_flVolume = flNearVolume;
			soundParams.m_nFlags = SND_CHANGE_VOL;
			EmitSound( filter, entindex(), soundParams );
		}

		EmitSound( "NPC_AntlionGuard.FarStepHeavy" );
	}
	else
	{
		if ( flNearVolume > 0.0f )
		{
			soundParams.m_pSoundName = "NPC_AntlionGuard.NearStepLight";
			soundParams.m_flVolume = flNearVolume;
			soundParams.m_nFlags = SND_CHANGE_VOL;
			EmitSound( filter, entindex(), soundParams );
		}

		EmitSound( "NPC_AntlionGuard.FarStepLight" );
	}
}

void CNPC_AntlionGuard::GetPhysicsShoveDir( CEntity *pObject, float flMass, Vector *pOut )
{
	const Vector vecStart = pObject->WorldSpaceCenter();
	const Vector vecTarget = GetEnemy()->WorldSpaceCenter();
	
	const float flBaseSpeed = 800.0f;
	float flSpeed = RemapValClamped( flMass, 0.0f, 150.0f, flBaseSpeed * 2.0f, flBaseSpeed );

	// Try the most direct route
	Vector vecToss = VecCheckThrow( this, vecStart, vecTarget, flSpeed, 1.0f );

	// If this failed then try a little faster (flattens the arc)
	if ( vecToss == vec3_origin )
	{
		vecToss = VecCheckThrow( this, vecStart, vecTarget, flSpeed * 1.25f, 1.0f );
		if ( vecToss == vec3_origin )
		{
			const float flGravity = sv_gravity->GetFloat();

			vecToss = (vecTarget - vecStart);

			// throw at a constant time
			float time = vecToss.Length( ) / flSpeed;
			vecToss = vecToss * (1.0f / time);

			// adjust upward toss to compensate for gravity loss
			vecToss.z += flGravity * time * 0.5f;
		}
	}

	// Save out the result
	if ( pOut )
	{
		*pOut = vecToss;
	}
}

void CNPC_AntlionGuard::HandleAnimEvent( animevent_t *pEvent )
{
	if ( pEvent->event == AE_ANTLIONGUARD_CHARGE_EARLYOUT )
	{
		// Robin: Removed this because it usually made him look less intelligent, not more.
		//		  This code left here so we don't get warnings in the console.

		/*
		// Cancel the charge if it's no longer valid
		if ( ShouldCharge( GetAbsOrigin(), GetEnemy()->GetAbsOrigin(), false, true ) == false )
		{
			SetSchedule( SCHED_ANTLIONGUARD_CHARGE_CANCEL );
		}
		*/

		return;
	}

	// Don't handle anim events after death
	if ( m_NPCState == NPC_STATE_DEAD )
	{
		BaseClass::HandleAnimEvent( pEvent );
		return;
	}

	if ( pEvent->event == AE_ANTLIONGUARD_SHOVE_PHYSOBJECT )
	{
		if ( m_hPhysicsTarget == NULL )
		{
			// Disrupt other objects near us anyway
			ImpactShock( WorldSpaceCenter(), 150, 250.0f );
			return;
		}

		// If we have no enemy, we don't really have a direction to throw the object
		// in. But, we still want to clobber it so the animevent doesn't happen fruitlessly,
		// and we want to clear the condition flags and other state regarding the m_hPhysicsTarget.
		// So, skip the code relevant to computing a launch vector specific to the object I'm
		// striking, but use the ImpactShock call further below to punch everything in the neighborhood.
		CEntity *pEnemy = GetEnemy();
		if ( pEnemy != NULL )
		{
			//Setup the throw velocity
			IPhysicsObject *physObj = m_hPhysicsTarget->VPhysicsGetObject();
			Vector vecShoveVel = ( pEnemy->GetAbsOrigin() - m_hPhysicsTarget->WorldSpaceCenter() );
			float flTargetDist = VectorNormalize( vecShoveVel );

			// Must still be close enough to our target
			Vector shoveDir = m_hPhysicsTarget->WorldSpaceCenter() - WorldSpaceCenter();
			float shoveDist = VectorNormalize( shoveDir );
			if ( shoveDist > 300.0f )
			{
				// Pick a new target next time (this one foiled us!)
				RememberFailedPhysicsTarget( m_hPhysicsTarget );
				m_hPhysicsTarget.Set(NULL);
				return;
			}

			// Toss this if we're episodic
			if ( true/*hl2_episodic.GetBool() */)
			{
				Vector vecTargetDir = vecShoveVel;

				// Get our shove direction
				GetPhysicsShoveDir( m_hPhysicsTarget, physObj->GetMass(), &vecShoveVel );

				// If the player isn't looking at me, and isn't reachable, be more forgiving about hitting them
				if ( HasCondition( COND_ENEMY_UNREACHABLE ) && HasCondition( COND_ENEMY_FACING_ME ) == false )
				{
					// Build an arc around the top of the target that we'll offset our aim by
					Vector vecOffset;
					float flSin, flCos;
					float flRad = enginerandom->RandomFloat( 0, M_PI / 6.0f ); // +/- 30 deg
					if ( enginerandom->RandomInt( 0, 1 ) )
						flRad *= -1.0f;

					SinCos( flRad, &flSin, &flCos );

					// Rotate the 2-d circle to be "facing" along our shot direction
					Vector vecArc;
					QAngle vecAngles;
					VectorAngles( vecTargetDir, vecAngles );
					VectorRotate( Vector( 0.0f, flCos, flSin ), vecAngles, vecArc );

					// Find the radius by which to avoid the player
					float flOffsetRadius = ( m_hPhysicsTarget->CollisionProp()->BoundingRadius() + GetEnemy()->CollisionProp()->BoundingRadius() ) * 1.5f;

					// Add this to our velocity to offset it
					vecShoveVel += ( vecArc * flOffsetRadius );
				}

				// Factor out mass
				vecShoveVel *= physObj->GetMass();
				
				// FIXME: We need to restore this on the object at some point if we do this!
				float flDragCoef = 0.0f;
				physObj->SetDragCoefficient( &flDragCoef, &flDragCoef );
			}
			else
			{
				if ( flTargetDist < 512 )
					flTargetDist = 512;

				if ( flTargetDist > 1024 )
					flTargetDist = 1024;

				vecShoveVel *= flTargetDist * 3 * physObj->GetMass();	//FIXME: Scale by skill
				vecShoveVel[2] += physObj->GetMass() * 350.0f;
			}

			//Send it flying
			AngularImpulse angVel( enginerandom->RandomFloat(-180, 180), 100, enginerandom->RandomFloat(-360, 360) );
			physObj->ApplyForceCenter( vecShoveVel );
			physObj->AddVelocity( NULL, &angVel );
		}
						
		//Display dust
		Vector vecRandom = RandomVector( -1, 1);
		VectorNormalize( vecRandom );
		g_pEffects->Dust( m_hPhysicsTarget->WorldSpaceCenter(), vecRandom, 64.0f, 32 );

		// If it's being held by the player, break that bond
		Pickup_ForcePlayerToDropThisObject( m_hPhysicsTarget );

		EmitSound( "NPC_AntlionGuard.HitHard" );

		//Clear the state information, we're done
		ClearCondition( COND_ANTLIONGUARD_PHYSICS_TARGET );
		ClearCondition( COND_ANTLIONGUARD_PHYSICS_TARGET_INVALID );

		// Disrupt other objects near it, including the m_hPhysicsTarget if we had no valid enemy
		ImpactShock( m_hPhysicsTarget->WorldSpaceCenter(), 150, 250.0f, pEnemy != NULL ? m_hPhysicsTarget.Get() : NULL );

		// Notify any squad members that we're no longer monopolizing this object
		if ( GetSquad() != NULL )
		{
			GetSquad()->BroadcastInteraction( g_interactionAntlionGuardShovedPhysicsObject, (void *)((CBaseEntity *)m_hPhysicsTarget->BaseEntity()), this );
		}

		m_hPhysicsTarget.Set(NULL);
		m_FailedPhysicsTargets.RemoveAll();

		return;
	}
	
	if ( pEvent->event == AE_ANTLIONGUARD_CHARGE_HIT )
	{
		UTIL_ScreenShake( GetAbsOrigin(), 32.0f, 4.0f, 1.0f, 512, SHAKE_START );
		EmitSound( "NPC_AntlionGuard.HitHard" );

		Vector	startPos = GetAbsOrigin();
		float	checkSize = ( CollisionProp()->BoundingRadius() + 8.0f );
		Vector	endPos = startPos + ( BodyDirection3D() * checkSize );

		CTraceFilterCharge traceFilter( BaseEntity(), COLLISION_GROUP_NONE, this );

		Ray_t ray;
		ray.Init( startPos, endPos, GetHullMins(), GetHullMaxs() );

		trace_t tr;
		enginetrace->TraceRay( ray, MASK_SHOT, &traceFilter, &tr );

		/*if ( g_debug_antlionguard.GetInt() == 1 )
		{
			Vector hullMaxs = GetHullMaxs();
			hullMaxs.x += checkSize;

			NDebugOverlay::BoxDirection( startPos, GetHullMins(), hullMaxs, BodyDirection2D(), 100, 255, 255, 20, 1.0f );
		}*/

		//NDebugOverlay::Box3D( startPos, endPos, BodyDirection2D(), 
		if ( m_hChargeTarget && m_hChargeTarget->IsAlive() == false )
		{
			m_hChargeTarget.Set(NULL);
			m_hChargeTargetPosition.Set(NULL);
		}

		// Cause a shock wave from this point which will distrupt nearby physics objects
		ImpactShock( tr.endpos, 200, 500 );
		return;
	}

	if ( pEvent->event == AE_ANTLIONGUARD_SHOVE )
	{
		EmitSound("NPC_AntlionGuard.StepLight", pEvent->eventtime );
		Shove();
		return;
	}

	if ( pEvent->event == AE_ANTLIONGUARD_FOOTSTEP_LIGHT )
	{
		if ( HasSpawnFlags(SF_ANTLIONGUARD_INSIDE_FOOTSTEPS) )
		{
#if HL2_EPISODIC
			Footstep( false );
#else 
			EmitSound("NPC_AntlionGuard.Inside.StepLight", pEvent->eventtime );
#endif // HL2_EPISODIC
		}
		else
		{
#if HL2_EPISODIC
			Footstep( false );
#else 
			EmitSound("NPC_AntlionGuard.StepLight", pEvent->eventtime );
#endif // HL2_EPISODIC
		}
		return;
	}

	if ( pEvent->event == AE_ANTLIONGUARD_FOOTSTEP_HEAVY )
	{
		if ( HasSpawnFlags(SF_ANTLIONGUARD_INSIDE_FOOTSTEPS) )
		{
#if HL2_EPISODIC
			Footstep( true );
#else 
			EmitSound( "NPC_AntlionGuard.Inside.StepHeavy", pEvent->eventtime );
#endif // HL2_EPISODIC
		}
		else
		{
#if HL2_EPISODIC
			Footstep( true );
#else 
			EmitSound( "NPC_AntlionGuard.StepHeavy", pEvent->eventtime );
#endif // HL2_EPISODIC
		}
		return;
	}
	
	if ( pEvent->event == AE_ANTLIONGUARD_VOICE_GROWL )
	{
		StartSounds();

		float duration = 0.0f;

		if ( enginerandom->RandomInt( 0, 10 ) < 6 )
		{
			duration = g_SoundController->SoundPlayEnvelope( m_pGrowlHighSound, SOUNDCTRL_CHANGE_VOLUME, envAntlionGuardFastGrowl, ARRAYSIZE(envAntlionGuardFastGrowl) );
		}
		else
		{
			duration = 1.0f;
			EmitSound( "NPC_AntlionGuard.FrustratedRoar" );
			g_SoundController->SoundFadeOut( m_pGrowlHighSound, 0.5f, false );
		}
		
		m_flAngerNoiseTime = gpGlobals->curtime + duration + enginerandom->RandomFloat( 2.0f, 4.0f );

		g_SoundController->SoundChangeVolume( m_pBreathSound, 0.0f, 0.1f );
		g_SoundController->SoundChangeVolume( m_pGrowlIdleSound, 0.0f, 0.1f );

		m_flBreathTime = gpGlobals->curtime + duration - 0.2f;

		EmitSound( "NPC_AntlionGuard.Anger" );
		return;
	}
		

	if ( pEvent->event == AE_ANTLIONGUARD_VOICE_BARK )
	{
		StartSounds();

		float duration = g_SoundController->SoundPlayEnvelope( m_pGrowlHighSound, SOUNDCTRL_CHANGE_VOLUME, envAntlionGuardBark1, ARRAYSIZE(envAntlionGuardBark1) );
		g_SoundController->SoundPlayEnvelope( m_pConfusedSound, SOUNDCTRL_CHANGE_VOLUME, envAntlionGuardBark2, ARRAYSIZE(envAntlionGuardBark2) );
		
		m_flAngerNoiseTime = gpGlobals->curtime + duration + enginerandom->RandomFloat( 2.0f, 4.0f );

		g_SoundController->SoundChangeVolume( m_pBreathSound, 0.0f, 0.1f );
		g_SoundController->SoundChangeVolume( m_pGrowlIdleSound, 0.0f, 0.1f );
		
		m_flBreathTime = gpGlobals->curtime + duration - 0.2f;
		return;
	}
	
	if ( pEvent->event == AE_ANTLIONGUARD_VOICE_ROAR )
	{
		StartSounds();

		float duration = g_SoundController->SoundPlayEnvelope( m_pGrowlHighSound, SOUNDCTRL_CHANGE_VOLUME, envAntlionGuardFastGrowl, ARRAYSIZE(envAntlionGuardFastGrowl) );
		
		m_flAngerNoiseTime = gpGlobals->curtime + duration + enginerandom->RandomFloat( 2.0f, 4.0f );

		g_SoundController->SoundChangeVolume( m_pBreathSound, 0.0f, 0.1f );
		g_SoundController->SoundChangeVolume( m_pGrowlIdleSound, 0.0f, 0.1f );

		m_flBreathTime = gpGlobals->curtime + duration - 0.2f;

		EmitSound( "NPC_AntlionGuard.Roar" );
		return;
	}

	if ( pEvent->event == AE_ANTLIONGUARD_VOICE_PAIN )
	{
		StartSounds();

		float duration = g_SoundController->SoundPlayEnvelope( m_pConfusedSound, SOUNDCTRL_CHANGE_VOLUME, envAntlionGuardPain1, ARRAYSIZE(envAntlionGuardPain1) );
		g_SoundController->SoundPlayEnvelope( m_pGrowlHighSound, SOUNDCTRL_CHANGE_VOLUME, envAntlionGuardBark2, ARRAYSIZE(envAntlionGuardBark2) );
		
		g_SoundController->SoundChangeVolume( m_pBreathSound, 0.0f, 0.1f );
		g_SoundController->SoundChangeVolume( m_pGrowlIdleSound, 0.0f, 0.1f );
		
		m_flBreathTime = gpGlobals->curtime + duration - 0.2f;
		return;
	}

	if ( pEvent->event == AE_ANTLIONGUARD_VOICE_SQUEEZE )
	{	
		StartSounds();

		float duration = g_SoundController->SoundPlayEnvelope( m_pGrowlHighSound, SOUNDCTRL_CHANGE_VOLUME, envAntlionGuardSqueeze, ARRAYSIZE(envAntlionGuardSqueeze) );
		
		g_SoundController->SoundChangeVolume( m_pBreathSound, 0.6f, enginerandom->RandomFloat( 2.0f, 4.0f ) );
		g_SoundController->SoundChangePitch( m_pBreathSound, enginerandom->RandomInt( 60, 80 ), enginerandom->RandomFloat( 2.0f, 4.0f ) );

		g_SoundController->SoundChangeVolume( m_pGrowlIdleSound, 0.0f, 0.1f );

		m_flBreathTime = gpGlobals->curtime + ( duration * 0.5f );

		EmitSound( "NPC_AntlionGuard.Anger" );
		return;
	}

	if ( pEvent->event == AE_ANTLIONGUARD_VOICE_SCRATCH )
	{	
		StartSounds();

		float duration = enginerandom->RandomFloat( 2.0f, 4.0f );

		g_SoundController->SoundChangeVolume( m_pBreathSound, 0.6f, duration );
		g_SoundController->SoundChangePitch( m_pBreathSound, enginerandom->RandomInt( 60, 80 ), duration );

		g_SoundController->SoundChangeVolume( m_pGrowlIdleSound, 0.0f, 0.1f );

		m_flBreathTime = gpGlobals->curtime + duration;

		EmitSound( "NPC_AntlionGuard.Anger" );
		return;
	}
		
	if ( pEvent->event == AE_ANTLIONGUARD_VOICE_GRUNT )
	{	
		StartSounds();

		float duration = g_SoundController->SoundPlayEnvelope( m_pConfusedSound, SOUNDCTRL_CHANGE_VOLUME, envAntlionGuardGrunt, ARRAYSIZE(envAntlionGuardGrunt) );
		g_SoundController->SoundPlayEnvelope( m_pGrowlHighSound, SOUNDCTRL_CHANGE_VOLUME, envAntlionGuardGrunt2, ARRAYSIZE(envAntlionGuardGrunt2) );
		
		g_SoundController->SoundChangeVolume( m_pBreathSound, 0.0f, 0.1f );
		g_SoundController->SoundChangeVolume( m_pGrowlIdleSound, 0.0f, 0.1f );

		m_flBreathTime = gpGlobals->curtime + duration;
		return;
	}

	if ( pEvent->event == AE_ANTLIONGUARD_BURROW_OUT )
	{
		EmitSound( "NPC_Antlion.BurrowOut" );

		//Shake the screen
		UTIL_ScreenShake( GetAbsOrigin(), 0.5f, 80.0f, 1.0f, 256.0f, SHAKE_START );

		//Throw dust up
		UTIL_CreateDust(GetAbsOrigin() + Vector(0,0,24), GetLocalAngles(), 6.0f, 10.0f);

		RemoveEffects( EF_NODRAW );
		RemoveFlag( FL_NOTARGET );

		if ( m_bCavernBreed )
		{
			if ( m_hCaveGlow[0] )
				m_hCaveGlow[0]->TurnOn();

			if ( m_hCaveGlow[1] )
				m_hCaveGlow[1]->TurnOn();
		}

		return;
	}

	BaseClass::HandleAnimEvent( pEvent );
}

void CNPC_AntlionGuard::SetHeavyDamageAnim( const Vector &vecSource )
{
	if ( !m_bInCavern )
	{
		Vector vFacing = BodyDirection2D();
		Vector vDamageDir = ( vecSource - WorldSpaceCenter() );

		vDamageDir.z = 0.0f;

		VectorNormalize( vDamageDir );

		Vector vDamageRight, vDamageUp;
		VectorVectors( vDamageDir, vDamageRight, vDamageUp );

		float damageDot = DotProduct( vFacing, vDamageDir );
		float damageRightDot = DotProduct( vFacing, vDamageRight );

		// See if it's in front
		if ( damageDot > 0.0f )
		{
			// See if it's right
			if ( damageRightDot > 0.0f )
			{
				m_nFlinchActivity = ACT_ANTLIONGUARD_PHYSHIT_FR;
			}
			else
			{
				m_nFlinchActivity = ACT_ANTLIONGUARD_PHYSHIT_FL;
			}
		}
		else
		{
			// Otherwise it's from behind
			if ( damageRightDot < 0.0f )
			{
				m_nFlinchActivity = ACT_ANTLIONGUARD_PHYSHIT_RR;
			}
			else
			{
				m_nFlinchActivity = ACT_ANTLIONGUARD_PHYSHIT_RL;
			}
		}
	}
}

int CNPC_AntlionGuard::OnTakeDamage_Alive( const CTakeDamageInfo &info )
{
	CTakeDamageInfo dInfo = info;
	
	CEntity *attacker = CEntity::Instance(dInfo.GetAttacker());
	// Don't take damage from another antlion guard!
	if (attacker && attacker != this && FClassnameIs( attacker, "npc_antlionguard" ) )
		return 0;

	if ( ( dInfo.GetDamageType() & DMG_CRUSH ) && !( dInfo.GetDamageType() & DMG_VEHICLE ) )
	{
		// Don't take damage from physics objects that weren't thrown by the player.
		CEntity *pInflictor = CEntity::Instance(dInfo.GetInflictor());
		
		if(pInflictor)
		{
			IPhysicsObject *pObj = pInflictor->VPhysicsGetObject();
			if ( !pObj || !( pObj->GetGameFlags() & FVPHYSICS_WAS_THROWN ) )
			{
				return 0;
			}
		}
	}

	// Hack to make antlion guard harder in HARD
	if ( g_helpfunc.GameRules_IsSkillLevel(SKILL_HARD) && !(info.GetDamageType() & DMG_CRUSH) )
	{
		dInfo.SetDamage( dInfo.GetDamage() * 0.75 );
	}

	// Cap damage taken by crushing (otherwise we can get crushed oddly)
	if ( ( dInfo.GetDamageType() & DMG_CRUSH ) && dInfo.GetDamage() > 100 )
	{
		dInfo.SetDamage( 100 );
	}

	// Only take damage from what we classify as heavy damages (explosions, refrigerators, etc)
	if ( IsHeavyDamage( dInfo ) )
	{
		UTIL_ScreenShake( GetAbsOrigin(), 32.0f, 8.0f, 0.5f, 512, SHAKE_START );

		// Set our response animation
		SetHeavyDamageAnim( dInfo.GetDamagePosition() );

		// Explosive barrels don't carry through their attacker, so this 
		// condition isn't set, and we still want to flinch. So we set it.
		SetCondition( COND_HEAVY_DAMAGE );

		// TODO: Make this its own effect!
		CEffectData data;
		data.m_vOrigin = dInfo.GetDamagePosition();
		data.m_vNormal = -dInfo.GetDamageForce();
		VectorNormalize( data.m_vNormal );
		g_helpfunc.DispatchEffect( "HunterDamage", data );

		// Play a sound for a physics impact
		if ( dInfo.GetDamageType() & DMG_CRUSH )
		{
			EmitSound("NPC_AntlionGuard.ShellCrack");
		}

		// Roar in pain
		EmitSound( "NPC_AntlionGuard.Pain_Roar" );

		// TODO: This will require a more complete solution; for now let's shelve it!
		/*
		if ( dInfo.GetDamageType() & DMG_BLAST )
		{
			// Create the dust effect in place
			CParticleSystem *pParticle = (CParticleSystem *) CreateEntityByName( "info_particle_system" );
			if ( pParticle != NULL )
			{
				// Setup our basic parameters
				pParticle->KeyValue( "start_active", "1" );
				pParticle->KeyValue( "effect_name", "fire_medium_02_nosmoke" );
				pParticle->SetParent( this );
				pParticle->SetParentAttachment( "SetParentAttachment", "attach_glow2", true );
				pParticle->SetLocalOrigin( Vector( -16, 24, 0 ) );
				DispatchSpawn( pParticle );
				if ( gpGlobals->curtime > 0.5f )
					pParticle->Activate();
				
				pParticle->SetThink( &CBaseEntity::SUB_Remove );
				pParticle->SetNextThink( gpGlobals->curtime + enginerandom->RandomFloat( 2.0f, 3.0f ) );
			}
		}
		*/
	}
	
	int nPreHealth = GetHealth();
	int nDamageTaken = BaseClass::OnTakeDamage_Alive( dInfo );

	// See if we've crossed a measured phase in our health and flinch from that to show we do take damage
	if ( !m_bInCavern && HasCondition( COND_HEAVY_DAMAGE ) == false && ( info.GetDamageType() & DMG_BULLET ) )
	{
		bool bTakeHeavyDamage = false;

		// Do an early flinch so that players understand the guard can be hurt by 
		if ( ( (float) GetHealth() / (float) GetMaxHealth() ) > 0.9f )
		{
			float flPrePerc = (float) nPreHealth / (float) GetMaxHealth();
			float flPostPerc = (float) GetHealth() / (float) GetMaxHealth();
			if ( flPrePerc >= 0.95f && flPostPerc < 0.95f )
			{
				bTakeHeavyDamage = true;
			}
		}

		// Otherwise see if we've passed a measured point in our health
		if ( bTakeHeavyDamage == false )
		{
			const float flNumDamagePhases =	5.0f;

			float flDenom = ( (float) GetMaxHealth() / flNumDamagePhases );
			int nPreDamagePhase = (int)ceil(nPreHealth / flDenom);
			int nPostDamagePhase = (int)ceil(GetHealth() / flDenom);
			if ( nPreDamagePhase != nPostDamagePhase )
			{
				bTakeHeavyDamage = true;
			}
		}

		// Flinch if we should
		if ( bTakeHeavyDamage )
		{
			// Set our response animation
			SetHeavyDamageAnim( dInfo.GetDamagePosition() );

			// Roar in pain
			EmitSound( "NPC_AntlionGuard.Pain_Roar" );

			// Flinch!
			SetCondition( COND_HEAVY_DAMAGE );
		}
	}

	return nDamageTaken;
}

void CNPC_AntlionGuard::TraceAttack( const CTakeDamageInfo &inputInfo, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator )
{
	CTakeDamageInfo info = inputInfo;

	// Bullets are weak against us, buckshot less so
	if ( info.GetDamageType() & DMG_BUCKSHOT )
	{
		info.ScaleDamage( 0.5f );
	}
	else if ( info.GetDamageType() & DMG_BULLET )
	{
		info.ScaleDamage( 0.25f );
	}

	// Make sure we haven't rounded down to a minimal amount
	if ( info.GetDamage() < 1.0f )
	{
		info.SetDamage( 1.0f );
	}

	BaseClass::TraceAttack( info, vecDir, ptr, pAccumulator );
}


void CNPC_AntlionGuard::StartTask( const Task_t *pTask )
{
	switch ( pTask->iTask )
	{
	case TASK_ANTLIONGUARD_SET_FLINCH_ACTIVITY:
		SetIdealActivity( (Activity) m_nFlinchActivity );
		break;

	case TASK_ANTLIONGUARD_SUMMON:
		SummonAntlions();
		m_OnSummon.FireOutput( this, this, 0 );
		TaskComplete();
		break;

	case TASK_ANTLIONGUARD_SHOVE_PHYSOBJECT:
		{
			if ( ( m_hPhysicsTarget == NULL ) || ( GetEnemy() == NULL ) )
			{
				TaskFail( "Tried to shove a NULL physics target!\n" );
				break;
			}
			
			//Get the direction and distance to our thrown object
			Vector	dirToTarget = ( m_hPhysicsTarget->WorldSpaceCenter() - WorldSpaceCenter() );
			float	distToTarget = VectorNormalize( dirToTarget );
			dirToTarget.z = 0;

			//Validate it's still close enough to shove
			//FIXME: Real numbers
			if ( distToTarget > 256.0f )
			{
				RememberFailedPhysicsTarget( m_hPhysicsTarget );
				m_hPhysicsTarget.Set(NULL);

				TaskFail( "Shove target moved\n" );
				break;
			}

			//Validate its offset from our facing
			float targetYaw = UTIL_VecToYaw( dirToTarget );
			float offset = UTIL_AngleDiff( targetYaw, UTIL_AngleMod( GetLocalAngles().y ) );

			if ( fabs( offset ) > 55 )
			{
				RememberFailedPhysicsTarget( m_hPhysicsTarget );
				m_hPhysicsTarget.Set(NULL);

				TaskFail( "Shove target off-center\n" );
				break;
			}

			//Blend properly
			SetPoseParameter( m_poseThrow, offset );

			//Start playing the animation
			SetActivity( ACT_ANTLIONGUARD_SHOVE_PHYSOBJECT );
		}
		break;

	case TASK_ANTLIONGUARD_FIND_PHYSOBJECT:
		{
			if ( m_bInCavern && !m_bPreferPhysicsAttack ) 
			{
				TaskFail( "Cavern guard is not allowed to use physics attacks." );
			}

			// Force the antlion guard to find a physobject
			m_flPhysicsCheckTime = 0;
			UpdatePhysicsTarget( false, (100*12) );
			if ( m_hPhysicsTarget )
			{
				TaskComplete();
			}
			else
			{
				TaskFail( "Failed to find a physobject.\n" );
			}
		}
		break;

	case TASK_ANTLIONGUARD_GET_PATH_TO_PHYSOBJECT:
		{
			if ( ( m_hPhysicsTarget == NULL ) || ( GetEnemy() == NULL ) )
			{
				TaskFail( "Tried to find a path to NULL physics target!\n" );
				break;
			}
			
			Vector vecGoalPos = m_vecPhysicsHitPosition; 
			AI_NavGoal_t goal( GOALTYPE_LOCATION, vecGoalPos, ACT_RUN );

			if ( GetNavigator()->SetGoal( goal ) )
			{
				/**if ( g_debug_antlionguard.GetInt() == 1 )
				{
					NDebugOverlay::Cross3D( vecGoalPos, Vector(8,8,8), -Vector(8,8,8), 0, 255, 0, true, 2.0f );
					NDebugOverlay::Line( vecGoalPos, m_hPhysicsTarget->WorldSpaceCenter(), 0, 255, 0, true, 2.0f );
					NDebugOverlay::Line( m_hPhysicsTarget->WorldSpaceCenter(), GetEnemy()->WorldSpaceCenter(), 0, 255, 0, true, 2.0f );
				}*/

				// Face the enemy
				GetNavigator()->SetArrivalDirection( GetEnemy() );
				TaskComplete();
			}
			else
			{
				/*if ( g_debug_antlionguard.GetInt() == 1 )
				{
					NDebugOverlay::Cross3D( vecGoalPos, Vector(8,8,8), -Vector(8,8,8), 255, 0, 0, true, 2.0f );
					NDebugOverlay::Line( vecGoalPos, m_hPhysicsTarget->WorldSpaceCenter(), 255, 0, 0, true, 2.0f );
					NDebugOverlay::Line( m_hPhysicsTarget->WorldSpaceCenter(), GetEnemy()->WorldSpaceCenter(), 255, 0, 0, true, 2.0f );
				}*/

				RememberFailedPhysicsTarget( m_hPhysicsTarget );
				m_hPhysicsTarget.Set(NULL);
				TaskFail( "Unable to navigate to physics attack target!\n" );
				break;
			}

			//!!!HACKHACK - this is a hack that covers a bug in antlion guard physics target selection! (Tracker #77601)
			// This piece of code (combined with some code in GatherConditions) COVERS THE BUG by escaping the schedule
			// if 30 seconds have passed (it should never take this long for the guard to get to an object and hit it).
			// It's too scary to figure out why this particular antlion guard can't get to its object, but we're shipping
			// like, tomorrow. (sjb) 8/22/2007
			m_flWaitFinished = gpGlobals->curtime + 30.0f;
		}
		break;

	case TASK_ANTLIONGUARD_CHARGE:
		{
			// HACK: Because the guard stops running his normal blended movement 
			//		 here, he also needs to remove his blended movement layers!
			GetMotor()->MoveStop();

			SetActivity( ACT_ANTLIONGUARD_CHARGE_START );
			m_bDecidedNotToStop = false;
		}
		break;


	case TASK_ANTLIONGUARD_GET_PATH_TO_CHARGE_POSITION:
		{
			if ( !m_hChargeTargetPosition )
			{
				TaskFail( "Tried to find a charge position without one specified.\n" );
				break;
			}

			// Move to the charge position
			AI_NavGoal_t goal( GOALTYPE_LOCATION, m_hChargeTargetPosition->GetAbsOrigin(), ACT_RUN );
			if ( GetNavigator()->SetGoal( goal ) )
			{
				// We want to face towards the charge target
				Vector vecDir = m_hChargeTarget->GetAbsOrigin() - m_hChargeTargetPosition->GetAbsOrigin();
				VectorNormalize( vecDir );
				vecDir.z = 0;
				GetNavigator()->SetArrivalDirection( vecDir );
				TaskComplete();
			}
			else
			{
				m_hChargeTarget.Set(NULL);
				m_hChargeTargetPosition.Set(NULL);
				TaskFail( FAIL_NO_ROUTE );
			}
		}
		break;

	case TASK_ANTLIONGUARD_GET_PATH_TO_NEAREST_NODE:
		{
			if ( !GetEnemy() )
			{
				TaskFail( FAIL_NO_ENEMY );
				break;
			}

			// Find the nearest node to the enemy
			int node = GetNavigator()->GetNetwork()->NearestNodeToPoint( this, GetEnemy()->GetAbsOrigin(), false );
			CAI_Node *pNode = GetNavigator()->GetNetwork()->GetNode( node );
			if( pNode == NULL )
			{
				TaskFail( FAIL_NO_ROUTE );
				break;
			}

			Vector vecNodePos = pNode->GetPosition( GetHullType() );
			AI_NavGoal_t goal( GOALTYPE_LOCATION, vecNodePos, ACT_RUN );
			if ( GetNavigator()->SetGoal( goal ) )
			{
				GetNavigator()->SetArrivalDirection( GetEnemy() );
				TaskComplete();
				break;
			}


			TaskFail( FAIL_NO_ROUTE );
			break;
		}
		break;

	case TASK_ANTLIONGUARD_GET_CHASE_PATH_ENEMY_TOLERANCE:
		{
			// Chase the enemy, but allow local navigation to succeed if it gets within the goal tolerance
			GetNavigator()->SetLocalSucceedOnWithinTolerance( true );

			if ( GetNavigator()->SetGoal( GOALTYPE_ENEMY ) )
			{
				TaskComplete();
			}
			else
			{
				RememberUnreachable(GetEnemy());
				TaskFail(FAIL_NO_ROUTE);
			}

			GetNavigator()->SetLocalSucceedOnWithinTolerance( false );
		}
		break;

	case TASK_ANTLIONGUARD_OPPORTUNITY_THROW:
		{
			// If we've got some live antlions, look for a physics object to throw
			if ( m_iNumLiveAntlions >= 2 && RandomFloat(0,1) > 0.5 )
			{
				m_FailedPhysicsTargets.RemoveAll();
				UpdatePhysicsTarget( false, m_bPreferPhysicsAttack ? (100*12) : ANTLIONGUARD_FARTHEST_PHYSICS_OBJECT  );
				if ( HasCondition( COND_ANTLIONGUARD_PHYSICS_TARGET ) && !m_bInCavern )
				{
					SetSchedule( SCHED_ANTLIONGUARD_PHYSICS_ATTACK );
				}
			}

			TaskComplete();
		}
		break;

	default:
		BaseClass::StartTask(pTask);
		break;
	}
}


void ApplyChargeDamage( CEntity *pAntlionGuard, CEntity *pTarget, float flDamage )
{
	Vector attackDir = ( pTarget->WorldSpaceCenter() - pAntlionGuard->WorldSpaceCenter() );
	VectorNormalize( attackDir );
	Vector offset = RandomVector( -32, 32 ) + pTarget->WorldSpaceCenter();

	// Generate enough force to make a 75kg guy move away at 700 in/sec
	Vector vecForce = attackDir * ImpulseScale( 75, 700 );

	// Deal the damage
	CTakeDamageInfo	info( pAntlionGuard->BaseEntity(), pAntlionGuard->BaseEntity(), vecForce, offset, flDamage, DMG_CLUB );
	pTarget->TakeDamage( info );

#if HL2_EPISODIC
	// If I am a cavern guard attacking the player, and he still lives, then poison him too.
	Assert( dynamic_cast<CNPC_AntlionGuard *>(pAntlionGuard) );

	if ( static_cast<CNPC_AntlionGuard *>(pAntlionGuard)->IsInCavern() && pTarget->IsPlayer() && pTarget->IsAlive() && pTarget->m_iHealth > ANTLIONGUARD_POISON_TO)
	{
		// That didn't finish them. Take them down to one point with poison damage. It'll heal.
		pTarget->TakeDamage( CTakeDamageInfo( pAntlionGuard, pAntlionGuard, pTarget->m_iHealth - ANTLIONGUARD_POISON_TO, DMG_POISON ) );
	}
#endif

}


class CTraceFilterSkipPhysics : public CTraceFilter
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS_NOBASE( CTraceFilterSkipPhysics );
	
	CTraceFilterSkipPhysics( const IHandleEntity *passentity, int collisionGroup, float minMass )
		: m_pPassEnt(passentity), m_collisionGroup(collisionGroup), m_minMass(minMass)
	{
	}
	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		if ( !StandardFilterRules( pHandleEntity, contentsMask ) )
			return false;

		if ( !PassServerEntityFilter( pHandleEntity, m_pPassEnt ) )
			return false;

		// Don't test if the game code tells us we should ignore this collision...
		CEntity *pEntity = CE_EntityFromEntityHandle( pHandleEntity );
		if ( pEntity )
		{
			if ( !pEntity->ShouldCollide( m_collisionGroup, contentsMask ) )
				return false;
			
			if ( !g_helpfunc.GameRules_ShouldCollide( m_collisionGroup, pEntity->GetCollisionGroup() ) )
				return false;

			// don't test small moveable physics objects (unless it's an NPC)
			if ( !pEntity->IsNPC() && pEntity->GetMoveType() == MOVETYPE_VPHYSICS )
			{
				IPhysicsObject *pPhysics = pEntity->VPhysicsGetObject();
				Assert(pPhysics);
				if ( pPhysics->IsMoveable() && pPhysics->GetMass() < m_minMass )
					return false;
			}

			// If we hit an antlion, don't stop, but kill it
			if ( pEntity->Classify() == CLASS_ANTLION )
			{
				CEntity *pGuard = (CEntity*)CE_EntityFromEntityHandle( m_pPassEnt );
				ApplyChargeDamage( pGuard, pEntity, pEntity->GetHealth() );
				return false;
			}
		}

		return true;
	}

private:
	const IHandleEntity *m_pPassEnt;
	int m_collisionGroup;
	float m_minMass;
};


inline void TraceHull_SkipPhysics( const Vector &vecAbsStart, const Vector &vecAbsEnd, const Vector &hullMin, 
					 const Vector &hullMax,	unsigned int mask, const CBaseEntity *ignore, 
					 int collisionGroup, trace_t *ptr, float minMass )
{
	Ray_t ray;
	ray.Init( vecAbsStart, vecAbsEnd, hullMin, hullMax );
	CTraceFilterSkipPhysics traceFilter( ignore, collisionGroup, minMass );
	enginetrace->TraceRay( ray, mask, &traceFilter, ptr );
}

bool CNPC_AntlionGuard::EnemyIsRightInFrontOfMe( CEntity **pEntity )
{
	if ( !GetEnemy() )
		return false;

	if ( (GetEnemy()->WorldSpaceCenter() - WorldSpaceCenter()).LengthSqr() < (156*156) )
	{
		Vector vecLOS = ( GetEnemy()->GetAbsOrigin() - GetAbsOrigin() );
		vecLOS.z = 0;
		VectorNormalize( vecLOS );
		Vector vBodyDir = BodyDirection2D();
		if ( DotProduct( vecLOS, vBodyDir ) > 0.8 )
		{
			// He's in front of me, and close. Make sure he's not behind a wall.
			trace_t tr;
			UTIL_TraceLine( WorldSpaceCenter(), GetEnemy()->EyePosition(), MASK_SOLID, BaseEntity(), COLLISION_GROUP_NONE, &tr );
			if ( tr.m_pEnt == GetEnemy_CBase() )
			{
				*pEntity = CEntity::Instance(tr.m_pEnt);
				return true;
			}
		}
	}

	return false;
}

void CNPC_AntlionGuard::ChargeLookAhead( void )
{
	trace_t	tr;
	Vector vecForward;
	GetVectors( &vecForward, NULL, NULL );
	Vector vecTestPos = GetAbsOrigin() + ( vecForward * m_flGroundSpeed * 0.75 );
	Vector testHullMins = GetHullMins();
	testHullMins.z += (StepHeight() * 2);
	TraceHull_SkipPhysics( GetAbsOrigin(), vecTestPos, testHullMins, GetHullMaxs(), MASK_SHOT_HULL, BaseEntity(), COLLISION_GROUP_NONE, &tr, VPhysicsGetObject()->GetMass() * 0.5 );

	//NDebugOverlay::Box( tr.startpos, testHullMins, GetHullMaxs(), 0, 255, 0, true, 0.1f );
	//NDebugOverlay::Box( vecTestPos, testHullMins, GetHullMaxs(), 255, 0, 0, true, 0.1f );

	if ( tr.fraction != 1.0 )
	{
		// Start playing the hit animation
		AddGesture( ACT_ANTLIONGUARD_CHARGE_ANTICIPATION );
	}
}

bool CNPC_AntlionGuard::HandleChargeImpact( Vector vecImpact, CEntity *pEntity )
{
	// Cause a shock wave from this point which will disrupt nearby physics objects
	ImpactShock( vecImpact, 128, 350 );

	// Did we hit anything interesting?
	if ( !pEntity || pEntity->IsWorld() )
	{
		// Robin: Due to some of the finicky details in the motor, the guard will hit
		//		  the world when it is blocked by our enemy when trying to step up 
		//		  during a moveprobe. To get around this, we see if the enemy's within
		//		  a volume in front of the guard when we hit the world, and if he is,
		//		  we hit him anyway.
		EnemyIsRightInFrontOfMe( &pEntity );

		// Did we manage to find him? If not, increment our charge miss count and abort.
		if ( pEntity->IsWorld() )
		{
			m_iChargeMisses++;
			return true;
		}
	}
	
	CBaseEntity *cbase = (pEntity) ? pEntity->BaseEntity() : NULL;

	// Hit anything we don't like
	if ( IRelationType( cbase ) == D_HT && ( GetNextAttack() < gpGlobals->curtime ) )
	{
		EmitSound( "NPC_AntlionGuard.Shove" );

		if ( !IsPlayingGesture( ACT_ANTLIONGUARD_CHARGE_HIT ) )
		{
			RestartGesture( ACT_ANTLIONGUARD_CHARGE_HIT );
		}
		
		ChargeDamage( pEntity );
		
		pEntity->ApplyAbsVelocityImpulse( ( BodyDirection2D() * 400 ) + Vector( 0, 0, 200 ) );

		if ( !pEntity->IsAlive() && GetEnemy() == pEntity )
		{
			SetEnemy( NULL );
		}

		SetNextAttack( gpGlobals->curtime + 2.0f );
		SetActivity( ACT_ANTLIONGUARD_CHARGE_STOP );

		// We've hit something, so clear our miss count
		m_iChargeMisses = 0;
		return false;
	}

	// Hit something we don't hate. If it's not moveable, crash into it.
	if ( pEntity->GetMoveType() == MOVETYPE_NONE || pEntity->GetMoveType() == MOVETYPE_PUSH )
		return true;

	// If it's a vphysics object that's too heavy, crash into it too.
	if ( pEntity->GetMoveType() == MOVETYPE_VPHYSICS )
	{
		IPhysicsObject *pPhysics = pEntity->VPhysicsGetObject();
		if ( pPhysics )
		{
			// If the object is being held by the player, knock it out of his hands
			if ( pPhysics->GetGameFlags() & FVPHYSICS_PLAYER_HELD )
			{
				Pickup_ForcePlayerToDropThisObject( pEntity );
				return false;
			}

			if ( (!pPhysics->IsMoveable() || pPhysics->GetMass() > VPhysicsGetObject()->GetMass() * 0.5f ) )
				return true;
		}
	}

	return false;

	/*
	
	ROBIN: Wrote & then removed this. If we want to have large rocks that the guard
		   should smack around, then we should enable it.

	else
	{
		// If we hit a physics prop, smack the crap out of it. (large rocks)
		// Factor the object mass into it, because we want to move it no matter how heavy it is.
		if ( pEntity->GetMoveType() == MOVETYPE_VPHYSICS )
		{
			CTakeDamageInfo info( this, this, 250, DMG_BLAST );
			info.SetDamagePosition( vecImpact );
			float flForce = ImpulseScale( pEntity->VPhysicsGetObject()->GetMass(), 250 );
			flForce *= enginerandom->RandomFloat( 0.85, 1.15 );

			// Calculate the vector and stuff it into the takedamageinfo
			Vector vecForce = BodyDirection3D();
			VectorNormalize( vecForce );
			vecForce *= flForce;
			vecForce *= phys_pushscale.GetFloat();
			info.SetDamageForce( vecForce );

			pEntity->VPhysicsTakeDamage( info );
		}
	}
	*/
}


float CNPC_AntlionGuard::ChargeSteer( void )
{
	trace_t	tr;
	Vector	testPos, steer, forward, right;
	QAngle	angles;
	const float	testLength = m_flGroundSpeed * 0.15f;

	//Get our facing
	GetVectors( &forward, &right, NULL );

	steer = forward;

	const float faceYaw	= UTIL_VecToYaw( forward );

	//Offset right
	VectorAngles( forward, angles );
	angles[YAW] += 45.0f;
	AngleVectors( angles, &forward );

	//Probe out
	testPos = GetAbsOrigin() + ( forward * testLength );

	//Offset by step height
	Vector testHullMins = GetHullMins();
	testHullMins.z += (StepHeight() * 2);

	//Probe
	TraceHull_SkipPhysics( GetAbsOrigin(), testPos, testHullMins, GetHullMaxs(), MASK_SOLID_BRUSHONLY, BaseEntity(), COLLISION_GROUP_NONE, &tr, VPhysicsGetObject()->GetMass() * 0.5f );

	//Debug info
	/*if ( g_debug_antlionguard.GetInt() == 1 )
	{
		if ( tr.fraction == 1.0f )
		{
  			NDebugOverlay::BoxDirection( GetAbsOrigin(), testHullMins, GetHullMaxs() + Vector(testLength,0,0), forward, 0, 255, 0, 8, 0.1f );
   		}
   		else
   		{
  			NDebugOverlay::BoxDirection( GetAbsOrigin(), testHullMins, GetHullMaxs() + Vector(testLength,0,0), forward, 255, 0, 0, 8, 0.1f );
		}
	}*/

	//Add in this component
	steer += ( right * 0.5f ) * ( 1.0f - tr.fraction );

	//Offset left
	angles[YAW] -= 90.0f;
	AngleVectors( angles, &forward );

	//Probe out
	testPos = GetAbsOrigin() + ( forward * testLength );

	// Probe
	TraceHull_SkipPhysics( GetAbsOrigin(), testPos, testHullMins, GetHullMaxs(), MASK_SOLID_BRUSHONLY, BaseEntity(), COLLISION_GROUP_NONE, &tr, VPhysicsGetObject()->GetMass() * 0.5f );

	//Debug
	/*if ( g_debug_antlionguard.GetInt() == 1 )
	{
		if ( tr.fraction == 1.0f )
		{
			NDebugOverlay::BoxDirection( GetAbsOrigin(), testHullMins, GetHullMaxs() + Vector(testLength,0,0), forward, 0, 255, 0, 8, 0.1f );
		}
		else
		{
			NDebugOverlay::BoxDirection( GetAbsOrigin(), testHullMins, GetHullMaxs() + Vector(testLength,0,0), forward, 255, 0, 0, 8, 0.1f );
		}
	}*/

	//Add in this component
	steer -= ( right * 0.5f ) * ( 1.0f - tr.fraction );

	//Debug
	/*if ( g_debug_antlionguard.GetInt() == 1 )
	{
		NDebugOverlay::Line( GetAbsOrigin(), GetAbsOrigin() + ( steer * 512.0f ), 255, 255, 0, true, 0.1f );
		NDebugOverlay::Cross3D( GetAbsOrigin() + ( steer * 512.0f ), Vector(2,2,2), -Vector(2,2,2), 255, 255, 0, true, 0.1f );

		NDebugOverlay::Line( GetAbsOrigin(), GetAbsOrigin() + ( BodyDirection3D() * 256.0f ), 255, 0, 255, true, 0.1f );
		NDebugOverlay::Cross3D( GetAbsOrigin() + ( BodyDirection3D() * 256.0f ), Vector(2,2,2), -Vector(2,2,2), 255, 0, 255, true, 0.1f );
	}*/

	return UTIL_AngleDiff( UTIL_VecToYaw( steer ), faceYaw );
}

void CNPC_AntlionGuard::RunTask( const Task_t *pTask )
{
	switch (pTask->iTask)
	{
	case TASK_ANTLIONGUARD_SET_FLINCH_ACTIVITY:
		
		AutoMovement( );
		
		if ( IsActivityFinished() )
		{
			TaskComplete();
		}
		break;

	case TASK_ANTLIONGUARD_SHOVE_PHYSOBJECT:
	
		if ( IsActivityFinished() )
		{
			TaskComplete();
		}

		break;

		/*
	case TASK_RUN_PATH:
		{


		}
		break;
		*/

	case TASK_ANTLIONGUARD_CHARGE:
		{
			Activity eActivity = GetActivity();

			// See if we're trying to stop after hitting/missing our target
			if ( eActivity == ACT_ANTLIONGUARD_CHARGE_STOP || eActivity == ACT_ANTLIONGUARD_CHARGE_CRASH ) 
			{
				if ( IsActivityFinished() )
				{
					TaskComplete();
					return;
				}

				// Still in the process of slowing down. Run movement until it's done.
				AutoMovement();
				return;
			}

			// Check for manual transition
			if ( ( eActivity == ACT_ANTLIONGUARD_CHARGE_START ) && ( IsActivityFinished() ) )
			{
				SetActivity( ACT_ANTLIONGUARD_CHARGE_RUN );
			}

			// See if we're still running
			if ( eActivity == ACT_ANTLIONGUARD_CHARGE_RUN || eActivity == ACT_ANTLIONGUARD_CHARGE_START ) 
			{
				if ( HasCondition( COND_NEW_ENEMY ) || HasCondition( COND_LOST_ENEMY ) || HasCondition( COND_ENEMY_DEAD ) )
				{
					SetActivity( ACT_ANTLIONGUARD_CHARGE_STOP );
					return;
				}
				else 
				{
					if ( GetEnemy() != NULL )
					{
						Vector	goalDir = ( GetEnemy()->GetAbsOrigin() - GetAbsOrigin() );
						VectorNormalize( goalDir );

						if ( DotProduct( BodyDirection2D(), goalDir ) < 0.25f )
						{
							if ( !m_bDecidedNotToStop )
							{
								// We've missed the target. Randomly decide not to stop, which will cause
								// the guard to just try and swing around for another pass.
								m_bDecidedNotToStop = true;
								if ( RandomFloat(0,1) > 0.3 )
								{
									m_iChargeMisses++;
									SetActivity( ACT_ANTLIONGUARD_CHARGE_STOP );
								}
							}
						}
						else
						{
							m_bDecidedNotToStop = false;
						}
					}
				}
			}

			// Steer towards our target
			float idealYaw;
			if ( GetEnemy() == NULL )
			{
				idealYaw = GetMotor()->GetIdealYaw();
			}
			else
			{
				idealYaw = CalcIdealYaw( GetEnemy()->GetAbsOrigin() );
			}

			// Add in our steering offset
			idealYaw += ChargeSteer();
			
			// Turn to face
			GetMotor()->SetIdealYawAndUpdate( idealYaw );

			// See if we're going to run into anything soon
			ChargeLookAhead();

			// Let our animations simply move us forward. Keep the result
			// of the movement so we know whether we've hit our target.
			AIMoveTrace_t moveTrace;
			if ( AutoMovement( GetEnemy(), &moveTrace ) == false )
			{
				CEntity *cent_pObstruction = CEntity::Instance(moveTrace.pObstruction);
				// Only stop if we hit the world
				if ( HandleChargeImpact( moveTrace.vEndPosition, cent_pObstruction ) )
				{
					// If we're starting up, this is an error
					if ( eActivity == ACT_ANTLIONGUARD_CHARGE_START )
					{
						TaskFail( "Unable to make initial movement of charge\n" );
						return;
					}

					// Crash unless we're trying to stop already
					if ( eActivity != ACT_ANTLIONGUARD_CHARGE_STOP )
					{
						if ( moveTrace.fStatus == AIMR_BLOCKED_WORLD && moveTrace.vHitNormal == vec3_origin )
						{
							SetActivity( ACT_ANTLIONGUARD_CHARGE_STOP );
						}
						else
						{
							SetActivity( ACT_ANTLIONGUARD_CHARGE_CRASH );
						}
					}
				}
				else if ( cent_pObstruction )
				{
					// If we hit an antlion, don't stop, but kill it
					if ( cent_pObstruction->Classify() == CLASS_ANTLION )
					{
						if ( FClassnameIs( cent_pObstruction, "npc_antlionguard" ) )
						{
							// Crash unless we're trying to stop already
							if ( eActivity != ACT_ANTLIONGUARD_CHARGE_STOP )
							{
								SetActivity( ACT_ANTLIONGUARD_CHARGE_STOP );
							}
						}
						else
						{
							ApplyChargeDamage( this, cent_pObstruction, cent_pObstruction->GetHealth() );
						}
					}
				}
			}
		}
		break;

	case TASK_WAIT_FOR_MOVEMENT:
	{
		// the cavern antlion can clothesline gordon
		if ( m_bInCavern )
		{

			// See if we're going to run into anything soon
			ChargeLookAhead();

			if ( HasCondition(COND_CAN_MELEE_ATTACK1) )
			{


				CEntity *pEntity = GetEnemy();

				if (pEntity && pEntity->IsPlayer())
				{
					EmitSound( "NPC_AntlionGuard.Shove" );

					if ( !IsPlayingGesture( ACT_ANTLIONGUARD_CHARGE_HIT ) )
					{
						RestartGesture( ACT_ANTLIONGUARD_CHARGE_HIT );
					}
					
					ChargeDamage( pEntity );
					
					pEntity->ApplyAbsVelocityImpulse( ( BodyDirection2D() * 400 ) + Vector( 0, 0, 200 ) );

					if ( !pEntity->IsAlive() && GetEnemy() == pEntity )
					{
						SetEnemy( NULL );
					}

					SetNextAttack( gpGlobals->curtime + 2.0f );
					SetActivity( ACT_ANTLIONGUARD_CHARGE_STOP );

					// We've hit something, so clear our miss count
					m_iChargeMisses = 0;

					AutoMovement();
					TaskComplete();
					return;
				}
			}
		}

		BaseClass::RunTask( pTask );

	}
	break;

	default:
		BaseClass::RunTask(pTask);
		break;
	}
}

void CNPC_AntlionGuard::SummonAntlions( void )
{
	// We want to spawn them around the guard
	Vector vecForward, vecRight;
	AngleVectors( QAngle(0,GetAbsAngles().y,0), &vecForward, &vecRight, NULL );

	// Spawn positions
	struct spawnpos_t
	{
		float flForward;
		float flRight;
	};

	spawnpos_t sAntlionSpawnPositions[] =
	{
		{ 0, 200 },
		{ 0, -200 },
		{ 128, 128 },
		{ 128, -128 },
		{ -128, 128 },
		{ -128, -128 },
		{ 200, 0 },
		{ -200, 0 },
	};

	// Only spawn up to our max count
	int iSpawnPoint = 0;
	for ( int i = 0; (m_iNumLiveAntlions < ANTLIONGUARD_SUMMON_COUNT) && (iSpawnPoint < (int)ARRAYSIZE(sAntlionSpawnPositions)); i++ )
	{
		// Determine spawn position for the antlion
		Vector vecSpawn = GetAbsOrigin() + ( sAntlionSpawnPositions[iSpawnPoint].flForward * vecForward ) + ( sAntlionSpawnPositions[iSpawnPoint].flRight * vecRight );
		iSpawnPoint++;
		// Randomise it a little
		vecSpawn.x += RandomFloat( -64, 64 );
		vecSpawn.y += RandomFloat( -64, 64 );
		vecSpawn.z += 64;

		// Make sure it's clear, and make sure we hit something
		trace_t	tr;
		UTIL_TraceHull( vecSpawn, vecSpawn - Vector(0,0,128), NAI_Hull::Mins( HULL_MEDIUM ), NAI_Hull::Maxs( HULL_MEDIUM ), MASK_NPCSOLID, NULL, COLLISION_GROUP_NONE, &tr );
		if ( tr.startsolid || tr.allsolid || tr.fraction == 1.0 )
		{
			continue;
		}

		// Ensure it's dirt or sand
		const surfacedata_t *pdata = physprops->GetSurfaceData( tr.surface.surfaceProps );
		if ( ( pdata->game.material != CHAR_TEX_DIRT ) && ( pdata->game.material != CHAR_TEX_SAND ) )
		{
			continue;
		}

		// Make sure the guard can see it
		trace_t	tr_vis;
		UTIL_TraceLine( WorldSpaceCenter(), tr.endpos, MASK_NPCSOLID, BaseEntity(), COLLISION_GROUP_NONE, &tr_vis );
		if ( tr_vis.fraction != 1.0 )
		{
			continue;
		}

		CNPC_Antlion *pAntlion = assert_cast<CNPC_Antlion*>(CreateEntityByName( "npc_antlion" ));
		if ( !pAntlion )
			break;

		CBaseEntity *cbase = pAntlion->BaseEntity();

		vecSpawn = tr.endpos;
		pAntlion->SetAbsOrigin( vecSpawn );

		// Start facing our enemy if we have one, otherwise just match the guard.
		Vector vecFacing = vecForward;
		if ( GetEnemy() )
		{
			vecFacing = GetEnemy()->GetAbsOrigin() - GetAbsOrigin();
			VectorNormalize( vecFacing );
		}
		QAngle vecAngles;
		VectorAngles( vecFacing, vecAngles );
		pAntlion->SetAbsAngles( vecAngles );

		pAntlion->AddSpawnFlags( SF_NPC_FALL_TO_GROUND );
		pAntlion->AddSpawnFlags( SF_NPC_FADE_CORPSE );

		// Make the antlion fire my input when he dies
		pAntlion->CustomDispatchKeyValue( "OnDeath", UTIL_VarArgs("%s,SummonedAntlionDied,,0,-1", GetEntityName()) );

		// Start the antlion burrowed, and tell him to come up
		pAntlion->m_bStartBurrowed = true;
		DispatchSpawn( cbase );
		pAntlion->Activate();

		g_CEventQueue->AddEvent( cbase, "Unburrow", RandomFloat(0.1, 1.0), BaseEntity(), BaseEntity() );

		// Add it to our squad
		if ( GetSquad() != NULL )
		{
			GetSquad()->AddToSquad( pAntlion );
		}

		CEntity *pEnemy = GetEnemy();
		// Set the antlion's enemy to our enemy
		if ( pEnemy )
		{
			pAntlion->SetEnemy( pEnemy->BaseEntity() );
			pAntlion->SetState( NPC_STATE_COMBAT );
			pAntlion->UpdateEnemyMemory( pEnemy->BaseEntity(), pEnemy->GetAbsOrigin() );
		}

		m_iNumLiveAntlions++;
	}

	if ( m_iNumLiveAntlions > 2 )
	{
		m_flNextSummonTime = gpGlobals->curtime + RandomFloat( 15,20 );
	}
	else
	{
		m_flNextSummonTime = gpGlobals->curtime + RandomFloat( 10,15 );
	}
}

void CNPC_AntlionGuard::FoundEnemy( void )
{
	m_flAngerNoiseTime = gpGlobals->curtime + 2.0f;
	SetState( NPC_STATE_COMBAT );
}

void CNPC_AntlionGuard::LostEnemy( void )
{
	m_flSearchNoiseTime = gpGlobals->curtime + 2.0f;
	SetState( NPC_STATE_ALERT );

	m_OnLostPlayer->FireOutput( this, this );
}

void CNPC_AntlionGuard::InputSetShoveTarget( inputdata_t &inputdata )
{
	if ( IsAlive() == false )
		return;

	CEntity *pTarget = g_helpfunc.FindEntityByName( (CBaseEntity *)NULL, inputdata.value.String(), NULL, inputdata.pActivator, inputdata.pCaller );

	if ( pTarget == NULL )
	{
		Warning( "**Guard %s cannot find shove target %s\n", GetClassname(), inputdata.value.String() );
		m_hShoveTarget.Set(NULL);
		return;
	}

	m_hShoveTarget.Set(pTarget->BaseEntity());
}

void CNPC_AntlionGuard::InputSetChargeTarget( inputdata_t &inputdata )
{
	if ( !IsAlive() )
		return;

	// Pull the target & position out of the string
	char parseString[255];
	Q_strncpy(parseString, inputdata.value.String(), sizeof(parseString));

	// Get charge target name
	char *pszParam = strtok(parseString," ");
	CEntity *pTarget = g_helpfunc.FindEntityByName( (CBaseEntity *)NULL, pszParam, NULL, inputdata.pActivator, inputdata.pCaller );
	if ( !pTarget )
	{
		Warning( "ERROR: Guard %s cannot find charge target '%s'\n", GetEntityName(), pszParam );
		return;
	}

	// Get the charge position name
	pszParam = strtok(NULL," ");
	CEntity *pPosition = g_helpfunc.FindEntityByName( (CBaseEntity *)NULL, pszParam, NULL, inputdata.pActivator, inputdata.pCaller );
	if ( !pPosition )
	{
		Warning( "ERROR: Guard %s cannot find charge position '%s'\nMake sure you've specified the parameters as [target start]!\n", GetEntityName(), pszParam );
		return;
	}

	// Make sure we don't stack charge targets
	if ( m_hChargeTarget )
	{
		if ( GetEnemy() == m_hChargeTarget )
		{
			SetEnemy( NULL );
		}
	}

	SetCondition( COND_ANTLIONGUARD_HAS_CHARGE_TARGET );
	m_hChargeTarget.Set(pTarget->BaseEntity());
	m_hChargeTargetPosition.Set(pPosition->BaseEntity());
}

void CNPC_AntlionGuard::InputClearChargeTarget( inputdata_t &inputdata )
{
	m_hChargeTarget.Set(NULL);
}

Activity CNPC_AntlionGuard::NPC_TranslateActivity( Activity baseAct )
{
	//See which run to use
	if ( ( baseAct == ACT_RUN ) && IsCurSchedule( SCHED_ANTLIONGUARD_CHARGE ) )
		return (Activity) ACT_ANTLIONGUARD_CHARGE_RUN;

	// Do extra code if we're trying to close on an enemy in a confined space (unless scripted)
	if ( hl2_episodic->GetBool() && m_bInCavern && baseAct == ACT_RUN && IsInAScript() == false )
		return (Activity) ACT_ANTLIONGUARD_CHARGE_RUN;

	if ( ( baseAct == ACT_RUN ) && ( m_iHealth <= (m_iMaxHealth/4) ) )
		return (Activity) ACT_ANTLIONGUARD_RUN_HURT;

	return baseAct;
}

bool CNPC_AntlionGuard::ShouldWatchEnemy( void )
{
	Activity nActivity = GetActivity();

	if ( ( nActivity == ACT_ANTLIONGUARD_SEARCH ) || 
		 ( nActivity == ACT_ANTLIONGUARD_PEEK_ENTER ) || 
		 ( nActivity == ACT_ANTLIONGUARD_PEEK_EXIT ) || 
		 ( nActivity == ACT_ANTLIONGUARD_PEEK1 ) || 
		 ( nActivity == ACT_ANTLIONGUARD_PEEK_SIGHTED ) || 
		 ( nActivity == ACT_ANTLIONGUARD_SHOVE_PHYSOBJECT ) || 
		 ( nActivity == ACT_ANTLIONGUARD_PHYSHIT_FR ) || 
		 ( nActivity == ACT_ANTLIONGUARD_PHYSHIT_FL ) || 
		 ( nActivity == ACT_ANTLIONGUARD_PHYSHIT_RR ) || 
		 ( nActivity == ACT_ANTLIONGUARD_PHYSHIT_RL ) || 
		 ( nActivity == ACT_ANTLIONGUARD_CHARGE_CRASH ) || 
		 ( nActivity == ACT_ANTLIONGUARD_CHARGE_HIT ) ||
		 ( nActivity == ACT_ANTLIONGUARD_CHARGE_ANTICIPATION ) )
	{
		return false;
	}

	return true;
}

void CNPC_AntlionGuard::UpdateHead( void )
{
	float yaw = GetPoseParameter( m_poseHead_Yaw );
	float pitch = GetPoseParameter( m_poseHead_Pitch );

	// If we should be watching our enemy, turn our head
	if ( ShouldWatchEnemy() && ( GetEnemy() != NULL ) )
	{
		Vector	enemyDir = GetEnemy()->WorldSpaceCenter() - WorldSpaceCenter();
		VectorNormalize( enemyDir );
		
		float angle = VecToYaw( BodyDirection3D() );
		float angleDiff = VecToYaw( enemyDir );
		angleDiff = UTIL_AngleDiff( angleDiff, angle + yaw );

		SetPoseParameter( m_poseHead_Yaw, UTIL_Approach( yaw + angleDiff, yaw, 50 ) );

		angle = UTIL_VecToPitch( BodyDirection3D() );
		angleDiff = UTIL_VecToPitch( enemyDir );
		angleDiff = UTIL_AngleDiff( angleDiff, angle + pitch );

		SetPoseParameter( m_poseHead_Pitch, UTIL_Approach( pitch + angleDiff, pitch, 50 ) );
	}
	else
	{
		// Otherwise turn the head back to its normal position
		SetPoseParameter( m_poseHead_Yaw,	UTIL_Approach( 0, yaw, 10 ) );
		SetPoseParameter( m_poseHead_Pitch, UTIL_Approach( 0, pitch, 10 ) );
	}
}

void CNPC_AntlionGuard::MaintainPhysicsTarget( void )
{
	if ( m_hPhysicsTarget == NULL || GetEnemy() == NULL )
		return;

	// Update our current target to make sure it's still valid
	float flTargetDistSqr = ( m_hPhysicsTarget->WorldSpaceCenter() - m_vecPhysicsTargetStartPos ).LengthSqr();
	bool bTargetMoved = ( flTargetDistSqr > Square(30*12.0f) );
	bool bEnemyCloser = ( ( GetEnemy()->GetAbsOrigin() - GetAbsOrigin() ).LengthSqr() <= flTargetDistSqr );

	// Make sure this hasn't moved too far or that the player is now closer
	if ( bTargetMoved || bEnemyCloser )
	{
		ClearCondition( COND_ANTLIONGUARD_PHYSICS_TARGET );
		SetCondition( COND_ANTLIONGUARD_PHYSICS_TARGET_INVALID );
		m_hPhysicsTarget.Set(NULL);
		return;
	}
	else
	{
		SetCondition( COND_ANTLIONGUARD_PHYSICS_TARGET );
		ClearCondition( COND_ANTLIONGUARD_PHYSICS_TARGET_INVALID );
	}
}

void CNPC_AntlionGuard::UpdatePhysicsTarget( bool bPreferObjectsAlongTargetVector, float flRadius )
{
	if ( GetEnemy() == NULL )
		return;

	// Already have a target, don't bother looking
	if ( m_hPhysicsTarget != NULL )
		return;

	// Too soon to check again
	if ( m_flPhysicsCheckTime > gpGlobals->curtime )
		return;

	// Attempt to find a valid shove target
	PhysicsObjectCriteria_t criteria;
	criteria.pTarget = GetEnemy();
	criteria.vecCenter = GetEnemy()->GetAbsOrigin();
	criteria.flRadius = flRadius;
	criteria.flTargetCone = ANTLIONGUARD_OBJECTFINDING_FOV;
	criteria.bPreferObjectsAlongTargetVector = bPreferObjectsAlongTargetVector;
	criteria.flNearRadius = (20*12); // TODO: It may preferable to disable this for legacy products as well -- jdw

	CEntity *cent = FindPhysicsObjectTarget( criteria );
	m_hPhysicsTarget.Set((cent)?cent->BaseEntity():NULL);
		
	// Found one, so interrupt us if we care
	if ( m_hPhysicsTarget != NULL )
	{
		SetCondition( COND_ANTLIONGUARD_PHYSICS_TARGET );
		m_vecPhysicsTargetStartPos = m_hPhysicsTarget->WorldSpaceCenter();
	}

	// Don't search again for another second
	m_flPhysicsCheckTime = gpGlobals->curtime + 1.0f;
}

bool CNPC_AntlionGuard::ShouldProbeCollideAgainstEntity( CBaseEntity *pEntity )
{
	CEntity *cent_pEntity = CEntity::Instance(pEntity);
	if(strstr(cent_pEntity->GetClassname(), STRING(m_iszPhysicsPropClass)) == NULL)
		return BaseClass::ShouldProbeCollideAgainstEntity( pEntity );

	if ( m_hPhysicsTarget == cent_pEntity )
		return false;

	if ( cent_pEntity->GetMoveType() == MOVETYPE_VPHYSICS )
	{
		IPhysicsObject *pPhysObj = cent_pEntity->VPhysicsGetObject();

		if( pPhysObj && pPhysObj->GetMass() <= ANTLIONGUARD_MAX_OBJECT_MASS )
		{
			return false;
		}
	}

	return BaseClass::ShouldProbeCollideAgainstEntity( pEntity );
}

void CNPC_AntlionGuard::PrescheduleThink( void )
{
	BaseClass::PrescheduleThink();

	// Don't do anything after death
	if ( m_NPCState == NPC_STATE_DEAD )
		return;

	// If we're burrowed, then don't do any of this
	if ( m_bIsBurrowed )
		return;

	// Automatically update our physics target when chasing enemies
	if ( IsCurSchedule( SCHED_ANTLIONGUARD_CHASE_ENEMY ) || 
		 IsCurSchedule( SCHED_ANTLIONGUARD_PATROL_RUN ) ||
		 IsCurSchedule( SCHED_ANTLIONGUARD_CANT_ATTACK ) ||
		 IsCurSchedule( SCHED_ANTLIONGUARD_CHASE_ENEMY_TOLERANCE ) )
	{
		bool bCheckAlongLine = ( IsCurSchedule( SCHED_ANTLIONGUARD_CHASE_ENEMY ) || IsCurSchedule( SCHED_ANTLIONGUARD_CHASE_ENEMY_TOLERANCE ) );
		UpdatePhysicsTarget( bCheckAlongLine );
	}
	else if ( !IsCurSchedule( SCHED_ANTLIONGUARD_PHYSICS_ATTACK ) )
	{
		ClearCondition( COND_ANTLIONGUARD_PHYSICS_TARGET );
		m_hPhysicsTarget.Set(NULL);
	}

	UpdateHead();

	if ( ( m_flGroundSpeed <= 0.0f ) )
	{
		if ( m_bStopped == false )
		{
			StartSounds();

			float duration = enginerandom->RandomFloat( 2.0f, 8.0f );

			g_SoundController->SoundChangeVolume( m_pBreathSound, 0.0f, duration );
			g_SoundController->SoundChangePitch( m_pBreathSound, enginerandom->RandomInt( 40, 60 ), duration );
			
			g_SoundController->SoundChangeVolume( m_pGrowlIdleSound, 0.0f, duration );
			g_SoundController->SoundChangePitch( m_pGrowlIdleSound, enginerandom->RandomInt( 120, 140 ), duration );

			m_flBreathTime = gpGlobals->curtime + duration - (duration*0.75f);
		}
		
		m_bStopped = true;

		if ( m_flBreathTime < gpGlobals->curtime )
		{
			StartSounds();

			g_SoundController->SoundChangeVolume( m_pGrowlIdleSound, enginerandom->RandomFloat( 0.2f, 0.3f ), enginerandom->RandomFloat( 0.5f, 1.0f ) );
			g_SoundController->SoundChangePitch( m_pGrowlIdleSound, enginerandom->RandomInt( 80, 120 ), enginerandom->RandomFloat( 0.5f, 1.0f ) );

			m_flBreathTime = gpGlobals->curtime + enginerandom->RandomFloat( 1.0f, 8.0f );
		}
	}
	else
	{
		if ( m_bStopped ) 
		{
			StartSounds();

			g_SoundController->SoundChangeVolume( m_pBreathSound, 0.6f, enginerandom->RandomFloat( 2.0f, 4.0f ) );
			g_SoundController->SoundChangePitch( m_pBreathSound, enginerandom->RandomInt( 140, 160 ), enginerandom->RandomFloat( 2.0f, 4.0f ) );

			g_SoundController->SoundChangeVolume( m_pGrowlIdleSound, 0.0f, 1.0f );
			g_SoundController->SoundChangePitch( m_pGrowlIdleSound, enginerandom->RandomInt( 90, 110 ), 0.2f );
		}


		m_bStopped = false;
	}

	// Put danger sounds out in front of us
	for ( int i = 0; i < 3; i++ )
	{
		g_helpfunc.CSoundEnt_InsertSound(SOUND_DANGER, WorldSpaceCenter() + ( BodyDirection3D() * 128 * (i+1) ), 128, 0.1f, BaseEntity());
	}
}

void CNPC_AntlionGuard::GatherConditions( void )
{
	BaseClass::GatherConditions();

	if ( CanSummon(false) )
	{
		SetCondition( COND_ANTLIONGUARD_CAN_SUMMON );
	}
	else
	{
		ClearCondition( COND_ANTLIONGUARD_CAN_SUMMON );
	}

	// Make sure our physics target is still valid
	MaintainPhysicsTarget();

	if( IsCurSchedule(SCHED_ANTLIONGUARD_PHYSICS_ATTACK) )
	{
		if( gpGlobals->curtime > m_flWaitFinished )
		{
			ClearCondition( COND_ANTLIONGUARD_PHYSICS_TARGET );
			SetCondition( COND_ANTLIONGUARD_PHYSICS_TARGET_INVALID );
			m_hPhysicsTarget.Set(NULL);
		}
	}

	// See if we can charge the target
	if ( GetEnemy() )
	{
		if ( ShouldCharge( GetAbsOrigin(), GetEnemy()->GetAbsOrigin(), true, false ) )
		{
			SetCondition( COND_ANTLIONGUARD_CAN_CHARGE );
		}
		else
		{
			ClearCondition( COND_ANTLIONGUARD_CAN_CHARGE );
		}
	}
}

void CNPC_AntlionGuard::StopLoopingSounds()
{
	//Stop all sounds
	g_SoundController->SoundDestroy( m_pGrowlHighSound );
	g_SoundController->SoundDestroy( m_pGrowlIdleSound );
	g_SoundController->SoundDestroy( m_pBreathSound );
	g_SoundController->SoundDestroy( m_pConfusedSound );
	
	
	m_pGrowlHighSound	= NULL;
	m_pGrowlIdleSound	= NULL;
	m_pBreathSound		= NULL;
	m_pConfusedSound	= NULL;

	BaseClass::StopLoopingSounds();
}

void CNPC_AntlionGuard::InputUnburrow( inputdata_t &inputdata )
{
	if ( IsAlive() == false )
		return;

	if ( m_bIsBurrowed == false )
		return;

	m_spawnflags &= ~SF_NPC_GAG;
	
	RemoveSolidFlags( FSOLID_NOT_SOLID );
	AddSolidFlags( FSOLID_NOT_STANDABLE );
	
	m_takedamage = DAMAGE_YES;

	SetSchedule( SCHED_ANTLIONGUARD_UNBURROW );

	m_bIsBurrowed = false;
}

bool CNPC_AntlionGuard::IsUnreachable(CBaseEntity *pEntity)
{
	CEntity *cent_pEntity = CEntity::Instance(pEntity);

	float UNREACHABLE_DIST_TOLERANCE_SQ = (240 * 240);

	// Note that it's ok to remove elements while I'm iterating
	// as long as I iterate backwards and remove them using FastRemove

	CUtlVector<UnreachableEnt_t> *data = m_UnreachableEnts;
	for (int i=data->Size()-1;i>=0;i--)
	{
		// Remove any dead elements
		if (data->Element(i).hUnreachableEnt == NULL)
		{
			data->FastRemove(i);
		}
		else if (pEntity == data->Element(i).hUnreachableEnt)
		{
			// Test for reachability on any elements that have timed out
			if ( gpGlobals->curtime > data->Element(i).fExpireTime ||
				cent_pEntity->GetAbsOrigin().DistToSqr(data->Element(i).vLocationWhenUnreachable) > UNREACHABLE_DIST_TOLERANCE_SQ)
			{
				data->FastRemove(i);
				return false;
			}
			return true;
		}
	}
	return false;
}

Vector CNPC_AntlionGuard::GetPhysicsHitPosition( CEntity *pObject, CEntity *pTarget, Vector *vecTrajectory, float *flClearDistance )
{
	// Get the trajectory we want to knock the object along
	Vector vecToTarget = pTarget->WorldSpaceCenter() - pObject->WorldSpaceCenter();
	VectorNormalize( vecToTarget );
	vecToTarget.z = 0;

	// Get the distance we want to be from the object when we hit it
	IPhysicsObject *pPhys = pObject->VPhysicsGetObject();
	Vector extent = physcollision->CollideGetExtent( pPhys->GetCollide(), pObject->GetAbsOrigin(), pObject->GetAbsAngles(), -vecToTarget );
	float flDist = ( extent - pObject->WorldSpaceCenter() ).Length() + CollisionProp()->BoundingRadius() + 32.0f;
	
	if ( vecTrajectory != NULL )
	{
		*vecTrajectory = vecToTarget;
	}

	if ( flClearDistance != NULL )
	{
		*flClearDistance = flDist;
	}

	// Position at which we'd like to be
	return (pObject->WorldSpaceCenter() + ( vecToTarget * -flDist ));
}

inline bool CNPC_AntlionGuard::CanStandAtPoint( const Vector &vecPos, Vector *pOut )
{
	Vector vecStart = vecPos + Vector( 0, 0, (4*12) );
	Vector vecEnd = vecPos - Vector( 0, 0, (4*12) );
	trace_t	tr;
	bool bTraceCleared = false;

	// Start high and try to go lower, looking for the ground between here and there
	// We do this first because it's more likely to succeed in the typical guard arenas (with open terrain)
	UTIL_TraceHull( vecStart, vecEnd, GetHullMins(), GetHullMaxs(), MASK_NPCSOLID, BaseEntity(), COLLISION_GROUP_NONE, &tr );
	if ( tr.startsolid && !tr.allsolid )
	{
		// We started in solid but didn't end up there, see if we can stand where we ended up
		UTIL_TraceHull( tr.endpos, tr.endpos, GetHullMins(), GetHullMaxs(), MASK_NPCSOLID, BaseEntity(), COLLISION_GROUP_NONE, &tr );
		
		// Must not be in solid
		bTraceCleared = ( !tr.allsolid && !tr.startsolid );
	}
	else
	{
		// Must not be in solid and must have found a floor (otherwise we're potentially hanging over a ledge)
		bTraceCleared = ( tr.allsolid == false && tr.fraction < 1.0f );
	}

	// Either we're clear or we're still unlucky
	if ( bTraceCleared == false )
	{
		return false;
	}

	if ( pOut )
	{
		*pOut = tr.endpos;
	}

	return true;
}

bool CNPC_AntlionGuard::CanStandAtShoveTarget( CEntity *pShoveObject, CEntity *pTarget, Vector *pOut )
{
	// Get the position we want to be at to swing at the object
	float flClearDistance;
	Vector vecTrajectory;
	Vector vecHitPosition = GetPhysicsHitPosition( pShoveObject, pTarget, &vecTrajectory, &flClearDistance );
	Vector vecStandPosition;

	// If we failed, try around the sides
	if ( CanStandAtPoint( vecHitPosition, &vecStandPosition ) == false )
	{
		// Get the angle (in reverse) to the target
		float flRad = atan2( -vecTrajectory.y, -vecTrajectory.x );
		float flRadOffset = DEG2RAD( 45.0f );

		// Build an offset vector, rotating around the base
		Vector vecSkewTrajectory;
		SinCos( flRad + flRadOffset, &vecSkewTrajectory.y, &vecSkewTrajectory.x );
		vecSkewTrajectory.z = 0.0f;
		
		// Try to the side
		if ( CanStandAtPoint( ( pShoveObject->WorldSpaceCenter() + ( vecSkewTrajectory * flClearDistance ) ), &vecStandPosition ) == false )
		{
			// Try the other side
			SinCos( flRad - flRadOffset, &vecSkewTrajectory.y, &vecSkewTrajectory.x );
			vecSkewTrajectory.z = 0.0f;			
			if ( CanStandAtPoint( ( pShoveObject->WorldSpaceCenter() + ( vecSkewTrajectory * flClearDistance ) ), &vecStandPosition ) == false )
				return false;
		}
	}

	// Found it, report it
	if ( pOut != NULL )
	{
		*pOut = vecStandPosition;
	}

	return true;
}

CEntity *CNPC_AntlionGuard::GetNextShoveTarget( CEntity *pLastEntity, AISightIter_t &iter )
{
	// Try to find scripted items first
	if ( m_strShoveTargets != NULL_STRING )
	{
		CEntity *pFound = g_helpfunc.FindEntityByName( (pLastEntity) ? pLastEntity->BaseEntity() : NULL, m_strShoveTargets );
		if ( pFound )
			return pFound;
	}

	// Failing that, use our senses
	if ( iter != (AISightIter_t)(-1) )
		return GetSenses()->GetNextSeenEntity( &iter );

    return GetSenses()->GetFirstSeenEntity( &iter, SEEN_MISC );
}

CEntity *CNPC_AntlionGuard::FindPhysicsObjectTarget( const PhysicsObjectCriteria_t &criteria )
{
	// Must have a valid target entity
 	if ( criteria.pTarget == NULL )
		return NULL;

	// Get the vector to our target, from ourself
	Vector vecDirToTarget = criteria.pTarget->GetAbsOrigin() - GetAbsOrigin();
	VectorNormalize( vecDirToTarget );
	vecDirToTarget.z = 0;

	// Cost is determined by distance to the object, modified by how "in line" it is with our target direction of travel
	// Use the distance to the player as the base cost for throwing an object (avoids pushing things too close to the player)
	float flLeastCost = ( criteria.bPreferObjectsAlongTargetVector ) ? ( criteria.pTarget->GetAbsOrigin() - GetAbsOrigin() ).LengthSqr() : Square( criteria.flRadius );
	float flCost;

	AISightIter_t iter = (AISightIter_t)(-1);
	CEntity *pObject = NULL;
	CEntity	*pNearest = NULL;
	Vector vecBestHitPosition = vec3_origin;

	// Look through the list of sensed objects for possible targets
	while( ( pObject = GetNextShoveTarget( pObject, iter ) ) != NULL )
	{
		// If we couldn't shove this object last time, don't try again
		if ( m_FailedPhysicsTargets.Find( pObject ) != m_FailedPhysicsTargets.InvalidIndex() )
			continue;

		// Ignore things less than half a foot in diameter
		if ( pObject->CollisionProp()->BoundingRadius() < 6.0f )
			continue;

		IPhysicsObject *pPhysObj = pObject->VPhysicsGetObject();
		if ( pPhysObj == NULL )
			continue;

		// Ignore motion disabled props
		if ( pPhysObj->IsMoveable() == false )
			continue;

		// Ignore things lighter than 5kg
		if ( pPhysObj->GetMass() < 5.0f )
			continue;

		// Ignore objects moving too quickly (they'll be too hard to catch otherwise)
		Vector	velocity;
		pPhysObj->GetVelocity( &velocity, NULL );
		if ( velocity.LengthSqr() > (16*16) )
			continue;

		// Get the direction from us to the physics object
		Vector vecDirToObject = pObject->WorldSpaceCenter() - GetAbsOrigin();
		VectorNormalize( vecDirToObject );
		vecDirToObject.z = 0;
		
		Vector vecObjCenter = pObject->WorldSpaceCenter();
		float flDistSqr = 0.0f;
		float flDot = 0.0f;

		// If we want to find things along the vector to the target, do so
		if ( criteria.bPreferObjectsAlongTargetVector )
		{
			// Object must be closer than our target
			if ( ( GetAbsOrigin() - vecObjCenter ).LengthSqr() > ( GetAbsOrigin() - criteria.pTarget->GetAbsOrigin() ).LengthSqr() )
				continue;

			// Calculate a "cost" to this object
			flDistSqr = ( GetAbsOrigin() - vecObjCenter ).LengthSqr();
			flDot = DotProduct( vecDirToTarget, vecDirToObject );
			
			// Ignore things outside our allowed cone
			if ( flDot < criteria.flTargetCone )
				continue;

			// The more perpendicular we are, the higher the cost
			float flCostScale = RemapValClamped( flDot, 1.0f, criteria.flTargetCone, 1.0f, 4.0f );
			flCost = flDistSqr * flCostScale;
		}
		else
		{
			// Straight distance cost
			flCost = ( criteria.vecCenter - vecObjCenter ).LengthSqr();
		}

		// This must be a less costly object to use
		if ( flCost >= flLeastCost )
		{
			continue;
		}

		// Check for a (roughly) clear trajectory path from the object to target
		trace_t	tr;
		UTIL_TraceLine( vecObjCenter, criteria.pTarget->BodyTarget( vecObjCenter ), MASK_SOLID_BRUSHONLY, BaseEntity(), COLLISION_GROUP_NONE, &tr );
		
		// See how close to our target we got (we still look good hurling things that won't necessarily hit)
		if ( ( tr.endpos - criteria.pTarget->WorldSpaceCenter() ).LengthSqr() > Square(criteria.flNearRadius) )
			continue;

		// Must be able to stand at a position to hit the object
		Vector vecHitPosition;
		if ( CanStandAtShoveTarget( pObject, criteria.pTarget, &vecHitPosition ) == false )
		{
			continue;
		}

		// Take this as the best object so far
		pNearest = pObject;
		flLeastCost = flCost;
		vecBestHitPosition = vecHitPosition;
	}

	// Set extra info if we've succeeded
	if ( pNearest != NULL )
	{
		m_vecPhysicsHitPosition = vecBestHitPosition;
	}

	return pNearest;
}

void CNPC_AntlionGuard::BuildScheduleTestBits( void )
{
	BaseClass::BuildScheduleTestBits();

	// Interrupt if we can shove something
	if ( IsCurSchedule( SCHED_ANTLIONGUARD_CHASE_ENEMY ) )
	{
		SetCustomInterruptCondition( COND_ANTLIONGUARD_PHYSICS_TARGET );
		SetCustomInterruptCondition( COND_ANTLIONGUARD_CAN_SUMMON );
	}

	// Interrupt if we've been given a charge target
	if ( IsCurSchedule( SCHED_ANTLIONGUARD_CHARGE ) == false )
	{
		SetCustomInterruptCondition( COND_ANTLIONGUARD_HAS_CHARGE_TARGET );
	}

	// Once we commit to doing this, just do it!
	if ( IsCurSchedule( SCHED_MELEE_ATTACK1 ) )
	{
		ClearCustomInterruptCondition( COND_ENEMY_OCCLUDED );
	}

	// Always take heavy damage
	SetCustomInterruptCondition( COND_HEAVY_DAMAGE );
}

void CNPC_AntlionGuard::ImpactShock( const Vector &origin, float radius, float magnitude, CEntity *pIgnored )
{
	// Also do a local physics explosion to push objects away
	float	adjustedDamage, flDist;
	Vector	vecSpot;
	float	falloff = 1.0f / 2.5f;

	CEntity *pEntity = NULL;

	// Find anything within our radius
	while ( ( pEntity = g_helpfunc.FindEntityInSphere( pEntity, origin, radius ) ) != NULL )
	{
		// Don't affect the ignored target
		if ( pEntity == pIgnored )
			continue;

		if ( pEntity == this )
			continue;

		// UNDONE: Ask the object if it should get force if it's not MOVETYPE_VPHYSICS?
		if ( pEntity->GetMoveType() == MOVETYPE_VPHYSICS || ( pEntity->VPhysicsGetObject() && pEntity->IsPlayer() == false ) )
		{
			vecSpot = pEntity->BodyTarget( GetAbsOrigin() );
			
			// decrease damage for an ent that's farther from the bomb.
			flDist = ( GetAbsOrigin() - vecSpot ).Length();

			if ( radius == 0 || flDist <= radius )
			{
				adjustedDamage = flDist * falloff;
				adjustedDamage = magnitude - adjustedDamage;
		
				if ( adjustedDamage < 1 )
				{
					adjustedDamage = 1;
				}

				CTakeDamageInfo info( BaseEntity(), BaseEntity(), adjustedDamage, DMG_BLAST );
				CalculateExplosiveDamageForce( &info, (vecSpot - GetAbsOrigin()), GetAbsOrigin() );

				pEntity->VPhysicsTakeDamage( info );
			}
		}
	}
}

void CNPC_AntlionGuard::ChargeDamage( CEntity *pTarget )
{
	if ( pTarget == NULL )
		return;

	CPlayer *pPlayer = ToBasePlayer( pTarget );

	if ( pPlayer != NULL )
	{
		//Kick the player angles
		pPlayer->ViewPunch( QAngle( 20, 20, -30 ) );	

		Vector	dir = pPlayer->WorldSpaceCenter() - WorldSpaceCenter();
		VectorNormalize( dir );
		dir.z = 0.0f;
		
		Vector vecNewVelocity = dir * 250.0f;
		vecNewVelocity[2] += 128.0f;
		pPlayer->SetAbsVelocity( vecNewVelocity );

		color32 red = {128,0,0,128};
		UTIL_ScreenFade( pPlayer, red, 1.0f, 0.1f, FFADE_IN );
	}
	
	// Player takes less damage
	float flDamage = ( pPlayer == NULL ) ? 250 : sk_antlionguard_dmg_charge.GetFloat();
	
	// If it's being held by the player, break that bond
	Pickup_ForcePlayerToDropThisObject( pTarget );

	// Calculate the physics force
	ApplyChargeDamage( this, pTarget, flDamage );
}

void CNPC_AntlionGuard::InputRagdoll( inputdata_t &inputdata )
{
	if ( IsAlive() == false )
		return;

	//Set us to nearly dead so the velocity from death is minimal
	SetHealth( 1 );

	CTakeDamageInfo info( BaseEntity(), BaseEntity(), GetHealth(), DMG_CRUSH );
	BaseClass::TakeDamage( info );
}

void CNPC_AntlionGuard::InputEnablePreferPhysicsAttack( inputdata_t &inputdata )
{
	m_bPreferPhysicsAttack = true;
}

void CNPC_AntlionGuard::InputDisablePreferPhysicsAttack( inputdata_t &inputdata )
{
	m_bPreferPhysicsAttack = false;
}

int CNPC_AntlionGuard::SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode )
{
	// Figure out what to do next
	if ( failedSchedule == SCHED_ANTLIONGUARD_CHASE_ENEMY && HasCondition( COND_ENEMY_UNREACHABLE ) )
		return SelectUnreachableSchedule();

	return BaseClass::SelectFailSchedule( failedSchedule,failedTask, taskFailCode );
}

int CNPC_AntlionGuard::TranslateSchedule( int scheduleType )
{
	switch( scheduleType )
	{
	case SCHED_CHASE_ENEMY:
		return SCHED_ANTLIONGUARD_CHASE_ENEMY;
		break;
	}

	return BaseClass::TranslateSchedule( scheduleType );
}

void CNPC_AntlionGuard::StartSounds( void )
{
	//Initialize the additive sound channels
	CPASAttenuationFilter filter( this );

	if ( m_pGrowlHighSound == NULL )
	{
		m_pGrowlHighSound = g_SoundController->SoundCreate( filter, entindex(), CHAN_VOICE, "NPC_AntlionGuard.GrowlHigh",	ATTN_NORM );
		
		if ( m_pGrowlHighSound )
		{
			g_SoundController->Play( m_pGrowlHighSound,0.0f, 100 );
		}
	}

	if ( m_pGrowlIdleSound == NULL )
	{
		m_pGrowlIdleSound = g_SoundController->SoundCreate( filter, entindex(), CHAN_STATIC, "NPC_AntlionGuard.GrowlIdle",	ATTN_NORM );

		if ( m_pGrowlIdleSound )
		{
			g_SoundController->Play( m_pGrowlIdleSound,0.0f, 100 );
		}
	}

	if ( m_pBreathSound == NULL )
	{
		m_pBreathSound	= g_SoundController->SoundCreate( filter, entindex(), CHAN_ITEM, "NPC_AntlionGuard.BreathSound",		ATTN_NORM );
		
		if ( m_pBreathSound )
		{
			g_SoundController->Play( m_pBreathSound,	0.0f, 100 );
		}
	}

	if ( m_pConfusedSound == NULL )
	{
		m_pConfusedSound = g_SoundController->SoundCreate( filter, entindex(), CHAN_WEAPON,"NPC_AntlionGuard.Confused",	ATTN_NORM );

		if ( m_pConfusedSound )
		{
			g_SoundController->Play( m_pConfusedSound,	0.0f, 100 );
		}
	}

}

void CNPC_AntlionGuard::InputEnableBark( inputdata_t &inputdata )
{
	m_bBarkEnabled = true;
}

void CNPC_AntlionGuard::InputDisableBark( inputdata_t &inputdata )
{
	m_bBarkEnabled = false;
}

void CNPC_AntlionGuard::DeathSound( const CTakeDamageInfo &info )
{
	EmitSound( "NPC_AntlionGuard.Die" );
}

void CNPC_AntlionGuard::Event_Killed( const CTakeDamageInfo &info )
{
	BaseClass::Event_Killed( info );

	// Tell all of my antlions to burrow away, 'cos they fear the Freeman
	if ( m_iNumLiveAntlions )
	{
		CEntity	*pSearch = NULL;

		// Iterate through all antlions and see if there are any orphans
		while ( ( pSearch = g_helpfunc.FindEntityByClassname( (pSearch) ? pSearch->BaseEntity() : NULL, "npc_antlion" ) ) != NULL )
		{
			CNPC_Antlion *pAntlion = assert_cast<CNPC_Antlion *>(pSearch);

			// See if it's a live orphan
			if ( pAntlion && pAntlion->GetOwnerEntity() == NULL && pAntlion->IsAlive() )
			{
				g_CEventQueue->AddEvent( pSearch->BaseEntity(), "BurrowAway", RandomFloat(0.1, 2.0), BaseEntity(), BaseEntity() );
			}
		}	
	}

	DestroyGlows();
}

bool CNPC_AntlionGuard::CanBecomeRagdoll( void )
{
	if ( IsCurSchedule( SCHED_DIE ) )
		return true;

	return hl2_episodic->GetBool();
}

bool CNPC_AntlionGuard::BecomeRagdollOnClient( const Vector &force )
{
	if ( !CanBecomeRagdoll() ) 
		return false;

	EmitSound( "NPC_AntlionGuard.Fallover" );

	// Become server-side ragdoll if we're flagged to do it
	if ( m_spawnflags & SF_ANTLIONGUARD_SERVERSIDE_RAGDOLL )
	{
		CTakeDamageInfo	info;

		// Fake the info
		info.SetDamageType( DMG_GENERIC );
		info.SetDamageForce( force );
		info.SetDamagePosition( WorldSpaceCenter() );

		CEntity *pRagdoll = g_helpfunc.CreateServerRagdoll( BaseEntity(), 0, info, COLLISION_GROUP_NONE );
		if(pRagdoll)
		{
			// Transfer our name to the new ragdoll
			pRagdoll->SetName( GetEntityName() );
			pRagdoll->SetCollisionGroup( COLLISION_GROUP_DEBRIS );
		}
		// Get rid of our old body
		UTIL_Remove(this);

		return true;
	}

	return BaseClass::BecomeRagdollOnClient( force );
}

bool CNPC_AntlionGuard::OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval )
{
  	Vector		vecFacePosition = vec3_origin;
	CEntity		*pFaceTarget = NULL;
	bool		bFaceTarget = false;

	// FIXME: this will break scripted sequences that walk when they have an enemy
	if ( m_hChargeTarget )
	{
		vecFacePosition = m_hChargeTarget->GetAbsOrigin();
		pFaceTarget = m_hChargeTarget;
		bFaceTarget = true;
	}
#ifdef HL2_EPISODIC
	else if ( GetEnemy() && IsCurSchedule( SCHED_ANTLIONGUARD_CANT_ATTACK ) )
	{
		// Always face our enemy when randomly patrolling around
		vecFacePosition = GetEnemy()->EyePosition();
		pFaceTarget = GetEnemy();
		bFaceTarget = true;
	}
#endif	// HL2_EPISODIC
	else if ( GetEnemy() && GetNavigator()->GetMovementActivity() == ACT_RUN )
  	{
		Vector vecEnemyLKP = GetEnemyLKP();
		
		// Only start facing when we're close enough
		if ( ( UTIL_DistApprox( vecEnemyLKP, GetAbsOrigin() ) < 512 ) || IsCurSchedule( SCHED_ANTLIONGUARD_PATROL_RUN ) )
		{
			vecFacePosition = vecEnemyLKP;
			pFaceTarget = GetEnemy();
			bFaceTarget = true;
		}
	}

	// Face
	if ( bFaceTarget )
	{
		AddFacingTarget_E_V_F_F_F( pFaceTarget->BaseEntity(), vecFacePosition, 1.0, 0.2 );
	}

	return BaseClass::OverrideMoveFacing( move, flInterval );
}

bool CNPC_AntlionGuard::IsHeavyDamage( const CTakeDamageInfo &info )
{
	// Struck by blast
	if ( info.GetDamageType() & DMG_BLAST )
	{
		if ( info.GetDamage() > MIN_BLAST_DAMAGE )
			return true;
	}

	// Struck by large object
	if ( info.GetDamageType() & DMG_CRUSH )
	{
		CEntity *cent_Inflictor = CEntity::Instance(info.GetInflictor());
		IPhysicsObject *pPhysObject = cent_Inflictor->VPhysicsGetObject();

		if ( ( pPhysObject != NULL ) && ( pPhysObject->GetGameFlags() & FVPHYSICS_WAS_THROWN ) )
		{
			// If we're under half health, stop being interrupted by heavy damage
			if ( GetHealth() < (GetMaxHealth() * 0.25) )
				return false;

			// Ignore physics damages that don't do much damage
			if ( info.GetDamage() < MIN_CRUSH_DAMAGE )
				return false;

			return true;
		}

		return false;
	}

	return false;
}

bool CNPC_AntlionGuard::IsLightDamage( const CTakeDamageInfo &info )
{
	return false;
}

void CNPC_AntlionGuard::InputSummonedAntlionDied( inputdata_t &inputdata )
{
	m_iNumLiveAntlions--;
	Assert( m_iNumLiveAntlions >= 0 );
}

bool CNPC_AntlionGuard::QueryHearSound( CSound *pSound )
{
	// Don't bother with danger sounds from antlions or other guards
	CEntity *pEntity = CEntity::Instance(pSound->m_hOwner);
	if ( pSound->SoundType() == SOUND_DANGER && ( pEntity != NULL && pEntity->Classify() == CLASS_ANTLION ) )
		return false;

	return BaseClass::QueryHearSound( pSound );
}

#if HL2_EPISODIC
//---------------------------------------------------------
// Prevent the cavern guard from using stopping paths, as it occasionally forces him off the navmesh.
//---------------------------------------------------------
bool CNPC_AntlionGuard::CNavigator::GetStoppingPath( CAI_WaypointList *pClippedWaypoints )
{
	if (GetOuter()->m_bInCavern)
	{
		return false;
	}
	else
	{
		return BaseClass::GetStoppingPath( pClippedWaypoints );
	}
}
#endif

bool CNPC_AntlionGuard::RememberFailedPhysicsTarget( CEntity *pTarget )
{
	// Already in the list?
	if ( m_FailedPhysicsTargets.Find( pTarget ) != m_FailedPhysicsTargets.InvalidIndex() )
		return true;

	// We're not holding on to any more
	if ( ( m_FailedPhysicsTargets.Count() + 1 ) > MAX_FAILED_PHYSOBJECTS )
		return false;

	m_FailedPhysicsTargets.AddToTail( pTarget );

	return true;
}

bool CNPC_AntlionGuard::HandleInteraction( int interactionType, void *data, CBaseEntity *sender )
{
	CEntity *pEntity = CEntity::Instance(sender);
	// Don't chase targets that other guards in our squad may be going after!
	if ( interactionType == g_interactionAntlionGuardFoundPhysicsObject )
	{
		RememberFailedPhysicsTarget( pEntity );
		return true;
	}

	// Mark a shoved object as being free to pursue again
	if ( interactionType == g_interactionAntlionGuardShovedPhysicsObject )
	{
		CBaseEntity *pObject = (CBaseEntity *) data;
		m_FailedPhysicsTargets.FindAndRemove( CEntity::Instance(pObject) );
		return true;
	}

	return BaseClass::HandleInteraction( interactionType, data, sender );
}

void CNPC_AntlionGuard::PopulatePoseParameters( void )
{
	m_poseThrow = LookupPoseParameter("throw");
	m_poseHead_Pitch = LookupPoseParameter("head_pitch");
	m_poseHead_Yaw   = LookupPoseParameter("head_yaw" );

	BaseClass::PopulatePoseParameters();
}


AI_BEGIN_CUSTOM_NPC( npc_antlionguard, CNPC_AntlionGuard )

	// Interactions	
	DECLARE_INTERACTION( g_interactionAntlionGuardFoundPhysicsObject )
	DECLARE_INTERACTION( g_interactionAntlionGuardShovedPhysicsObject )

	// Squadslots	
	DECLARE_SQUADSLOT( SQUAD_SLOT_ANTLIONGUARD_CHARGE )

	//Tasks
	DECLARE_TASK( TASK_ANTLIONGUARD_CHARGE )
	DECLARE_TASK( TASK_ANTLIONGUARD_GET_PATH_TO_PHYSOBJECT )
	DECLARE_TASK( TASK_ANTLIONGUARD_SHOVE_PHYSOBJECT )
	DECLARE_TASK( TASK_ANTLIONGUARD_SUMMON )
	DECLARE_TASK( TASK_ANTLIONGUARD_SET_FLINCH_ACTIVITY )
	DECLARE_TASK( TASK_ANTLIONGUARD_GET_PATH_TO_CHARGE_POSITION )
	DECLARE_TASK( TASK_ANTLIONGUARD_GET_PATH_TO_NEAREST_NODE )
	DECLARE_TASK( TASK_ANTLIONGUARD_GET_CHASE_PATH_ENEMY_TOLERANCE )
	DECLARE_TASK( TASK_ANTLIONGUARD_OPPORTUNITY_THROW )
	DECLARE_TASK( TASK_ANTLIONGUARD_FIND_PHYSOBJECT )

	//Activities
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_SEARCH )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_PEEK_FLINCH )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_PEEK_ENTER )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_PEEK_EXIT )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_PEEK1 )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_BARK )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_PEEK_SIGHTED )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_CHARGE_START )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_CHARGE_CANCEL )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_CHARGE_RUN )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_CHARGE_CRASH )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_CHARGE_STOP )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_CHARGE_HIT )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_CHARGE_ANTICIPATION )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_SHOVE_PHYSOBJECT )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_FLINCH_LIGHT )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_UNBURROW )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_ROAR )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_RUN_HURT )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_PHYSHIT_FR )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_PHYSHIT_FL )
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_PHYSHIT_RR )	
	DECLARE_ACTIVITY( ACT_ANTLIONGUARD_PHYSHIT_RL )		
	
	//Adrian: events go here
	DECLARE_ANIMEVENT( AE_ANTLIONGUARD_CHARGE_HIT )
	DECLARE_ANIMEVENT( AE_ANTLIONGUARD_SHOVE_PHYSOBJECT )
	DECLARE_ANIMEVENT( AE_ANTLIONGUARD_SHOVE )
	DECLARE_ANIMEVENT( AE_ANTLIONGUARD_FOOTSTEP_LIGHT )
	DECLARE_ANIMEVENT( AE_ANTLIONGUARD_FOOTSTEP_HEAVY )
	DECLARE_ANIMEVENT( AE_ANTLIONGUARD_CHARGE_EARLYOUT )
	DECLARE_ANIMEVENT( AE_ANTLIONGUARD_VOICE_GROWL )
	DECLARE_ANIMEVENT( AE_ANTLIONGUARD_VOICE_BARK )
	DECLARE_ANIMEVENT( AE_ANTLIONGUARD_VOICE_PAIN )
	DECLARE_ANIMEVENT( AE_ANTLIONGUARD_VOICE_SQUEEZE )
	DECLARE_ANIMEVENT( AE_ANTLIONGUARD_VOICE_SCRATCH )
	DECLARE_ANIMEVENT( AE_ANTLIONGUARD_VOICE_GRUNT )
	DECLARE_ANIMEVENT( AE_ANTLIONGUARD_BURROW_OUT )
	DECLARE_ANIMEVENT( AE_ANTLIONGUARD_VOICE_ROAR )

	DECLARE_CONDITION( COND_ANTLIONGUARD_PHYSICS_TARGET )
	DECLARE_CONDITION( COND_ANTLIONGUARD_PHYSICS_TARGET_INVALID )
	DECLARE_CONDITION( COND_ANTLIONGUARD_HAS_CHARGE_TARGET )
	DECLARE_CONDITION( COND_ANTLIONGUARD_CAN_SUMMON )
	DECLARE_CONDITION( COND_ANTLIONGUARD_CAN_CHARGE )

	//==================================================
	// SCHED_ANTLIONGUARD_SUMMON
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_SUMMON,

		"	Tasks"
		"		TASK_STOP_MOVING						0"
		"		TASK_FACE_ENEMY							0"
		"		TASK_PLAY_SEQUENCE						ACTIVITY:ACT_ANTLIONGUARD_BARK"
		"		TASK_ANTLIONGUARD_SUMMON				0"
		"		TASK_ANTLIONGUARD_OPPORTUNITY_THROW		0"
		"	"
		"	Interrupts"
		"		COND_HEAVY_DAMAGE"
	)

	//==================================================
	// SCHED_ANTLIONGUARD_CHARGE
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_CHARGE,

		"	Tasks"
		"		TASK_STOP_MOVING					0"
		"		TASK_SET_FAIL_SCHEDULE				SCHEDULE:SCHED_ANTLIONGUARD_CHASE_ENEMY"
		"		TASK_FACE_ENEMY						0"
		"		TASK_ANTLIONGUARD_CHARGE			0"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"
		"		COND_HEAVY_DAMAGE"

		// These are deliberately left out so they can be detected during the 
		// charge Task and correctly play the charge stop animation.
		//"		COND_NEW_ENEMY"
		//"		COND_ENEMY_DEAD"
		//"		COND_LOST_ENEMY"
	)

	//==================================================
	// SCHED_ANTLIONGUARD_CHARGE_TARGET
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_CHARGE_TARGET,

		"	Tasks"
		"		TASK_STOP_MOVING					0"
		"		TASK_FACE_ENEMY						0"
		"		TASK_ANTLIONGUARD_CHARGE			0"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"
		"		COND_ENEMY_DEAD"
		"		COND_HEAVY_DAMAGE"
	)

	//==================================================
	// SCHED_ANTLIONGUARD_CHARGE_SMASH
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_CHARGE_CRASH,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_ANTLIONGUARD_CHARGE_CRASH"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"
		"		COND_HEAVY_DAMAGE"
	)

	//==================================================
	// SCHED_ANTLIONGUARD_PHYSICS_ATTACK
	//==================================================

	DEFINE_SCHEDULE
	( 
		SCHED_ANTLIONGUARD_PHYSICS_ATTACK,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE						SCHEDULE:SCHED_ANTLIONGUARD_CHASE_ENEMY"
		"		TASK_ANTLIONGUARD_GET_PATH_TO_PHYSOBJECT	0"
		"		TASK_RUN_PATH								0"
		"		TASK_WAIT_FOR_MOVEMENT						0"
		"		TASK_FACE_ENEMY								0"
		"		TASK_ANTLIONGUARD_SHOVE_PHYSOBJECT			0"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"
		"		COND_ENEMY_DEAD"
		"		COND_LOST_ENEMY"
		"		COND_NEW_ENEMY"
		"		COND_ANTLIONGUARD_PHYSICS_TARGET_INVALID"
		"		COND_HEAVY_DAMAGE"
	)

	//==================================================
	// SCHED_FORCE_ANTLIONGUARD_PHYSICS_ATTACK
	//==================================================

	DEFINE_SCHEDULE
	( 
		SCHED_FORCE_ANTLIONGUARD_PHYSICS_ATTACK,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE						SCHEDULE:SCHED_ANTLIONGUARD_CANT_ATTACK"
		"		TASK_ANTLIONGUARD_FIND_PHYSOBJECT			0"
		"		TASK_SET_SCHEDULE							SCHEDULE:SCHED_ANTLIONGUARD_PHYSICS_ATTACK"
		""
		"	Interrupts"
		"		COND_ANTLIONGUARD_PHYSICS_TARGET"
		"		COND_HEAVY_DAMAGE"
	)

	//==================================================
	// SCHED_ANTLIONGUARD_CANT_ATTACK
	//		If we're here, the guard can't chase enemy, can't find a physobject to attack with, and can't summon
	//==================================================

#ifdef HL2_EPISODIC

	DEFINE_SCHEDULE
	( 
		SCHED_ANTLIONGUARD_CANT_ATTACK,

		"	Tasks"
		"		TASK_SET_ROUTE_SEARCH_TIME		2"	// Spend 5 seconds trying to build a path if stuck
		"		TASK_GET_PATH_TO_RANDOM_NODE	1024"
		"		TASK_WALK_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_WAIT_PVS					0"
		""
		"	Interrupts"
		"		COND_GIVE_WAY"
		"		COND_NEW_ENEMY"
		"		COND_ANTLIONGUARD_PHYSICS_TARGET"
		"		COND_HEAVY_DAMAGE"
	)

#else

	DEFINE_SCHEDULE
	( 
	SCHED_ANTLIONGUARD_CANT_ATTACK,

	"	Tasks"
	"		TASK_WAIT								5"
	""
	"	Interrupts"
	)

#endif

	//==================================================
	// SCHED_ANTLIONGUARD_PHYSICS_DAMAGE_HEAVY
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_PHYSICS_DAMAGE_HEAVY,

		"	Tasks"
		"		TASK_STOP_MOVING						0"
		"		TASK_RESET_ACTIVITY						0"
		"		TASK_ANTLIONGUARD_SET_FLINCH_ACTIVITY	0"
		""
		"	Interrupts"
	)

	//==================================================
	// SCHED_ANTLIONGUARD_UNBURROW
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_UNBURROW,

		"	Tasks"
		"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_ANTLIONGUARD_UNBURROW"
		""
		"	Interrupts"
	)

	//==================================================
	// SCHED_ANTLIONGUARD_CHARGE_CANCEL
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_CHARGE_CANCEL,

		"	Tasks"
		"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_ANTLIONGUARD_CHARGE_CANCEL"
		""
		"	Interrupts"
	)
	
	//==================================================
	// SCHED_ANTLIONGUARD_FIND_CHARGE_POSITION
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_FIND_CHARGE_POSITION,

		"	Tasks"
		"		TASK_ANTLIONGUARD_GET_PATH_TO_CHARGE_POSITION	0"
		"		TASK_RUN_PATH									0"
		"		TASK_WAIT_FOR_MOVEMENT							0"
		"	"
		"	Interrupts"
		"		COND_ENEMY_DEAD"
		"		COND_GIVE_WAY"
		"		COND_TASK_FAILED"
		"		COND_HEAVY_DAMAGE"
	)

	//=========================================================
	// > SCHED_ANTLIONGUARD_CHASE_ENEMY_TOLERANCE
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_CHASE_ENEMY_TOLERANCE,

		"	Tasks"
		"		TASK_STOP_MOVING									0"
		"		TASK_SET_FAIL_SCHEDULE								SCHEDULE:SCHED_ANTLIONGUARD_PATROL_RUN"
		"		TASK_ANTLIONGUARD_GET_PATH_TO_NEAREST_NODE			500"
		"		TASK_RUN_PATH										0"
		"		TASK_WAIT_FOR_MOVEMENT								0"
		"		TASK_FACE_ENEMY										0"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_GIVE_WAY"
		"		COND_NEW_ENEMY"
		"		COND_ANTLIONGUARD_CAN_SUMMON"
		"		COND_ANTLIONGUARD_PHYSICS_TARGET"
		"		COND_HEAVY_DAMAGE"
		"		COND_ANTLIONGUARD_CAN_CHARGE"
	);

	//=========================================================
	// > PATROL_RUN
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_PATROL_RUN,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE						SCHEDULE:SCHED_ANTLIONGUARD_CANT_ATTACK"
		"		TASK_SET_ROUTE_SEARCH_TIME					3"	// Spend 3 seconds trying to build a path if stuck
		"		TASK_ANTLIONGUARD_GET_PATH_TO_NEAREST_NODE	500"
		"		TASK_RUN_PATH								0"
		"		TASK_WAIT_FOR_MOVEMENT						0"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"
		"		COND_CAN_MELEE_ATTACK1"
		"		COND_GIVE_WAY"
		"		COND_NEW_ENEMY"
		"		COND_ANTLIONGUARD_PHYSICS_TARGET"
		"		COND_ANTLIONGUARD_CAN_SUMMON"
		"		COND_HEAVY_DAMAGE"
		"		COND_ANTLIONGUARD_CAN_CHARGE"
	);

	//==================================================
	// SCHED_ANTLIONGUARD_ROAR
	//==================================================

	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_ROAR,

		"	Tasks"
		"		TASK_STOP_MOVING			0"
		"		TASK_FACE_ENEMY				0"
		"		TASK_PLAY_SEQUENCE			ACTIVITY:ACT_ANTLIONGUARD_ROAR"
		"	"
		"	Interrupts"
		"		COND_HEAVY_DAMAGE"
	)

	//==================================================
	// SCHED_ANTLIONGUARD_TAKE_COVER_FROM_ENEMY
	//==================================================
	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_TAKE_COVER_FROM_ENEMY,

		"	Tasks"
		"		TASK_SET_FAIL_SCHEDULE			SCHEDULE:SCHED_ANTLIONGUARD_CANT_ATTACK"
		"		TASK_FIND_COVER_FROM_ENEMY		0"
		"		TASK_RUN_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_STOP_MOVING				0"
		""
		"	Interrupts"
		"		COND_TASK_FAILED"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_ANTLIONGUARD_PHYSICS_TARGET"
		"		COND_ANTLIONGUARD_CAN_SUMMON"
		"		COND_HEAVY_DAMAGE"
	)

	//=========================================================
	// SCHED_ANTLIONGUARD_CHASE_ENEMY
	//=========================================================
	DEFINE_SCHEDULE
	(
		SCHED_ANTLIONGUARD_CHASE_ENEMY,

		"	Tasks"
		"		TASK_STOP_MOVING				0"
		"		TASK_GET_CHASE_PATH_TO_ENEMY	300"
		"		TASK_RUN_PATH					0"
		"		TASK_WAIT_FOR_MOVEMENT			0"
		"		TASK_FACE_ENEMY			0"
		""
		"	Interrupts"
		"		COND_NEW_ENEMY"
		"		COND_ENEMY_DEAD"
		"		COND_ENEMY_UNREACHABLE"
		"		COND_CAN_RANGE_ATTACK1"
		// "		COND_CAN_MELEE_ATTACK1"
		"		COND_CAN_RANGE_ATTACK2"
		"		COND_CAN_MELEE_ATTACK2"
		"		COND_TOO_CLOSE_TO_ATTACK"
		"		COND_TASK_FAILED"
		"		COND_LOST_ENEMY"
		"		COND_HEAVY_DAMAGE"
		"		COND_ANTLIONGUARD_CAN_CHARGE"
	)

AI_END_CUSTOM_NPC()

