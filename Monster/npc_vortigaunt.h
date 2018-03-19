#ifndef _INCLUDE_NPC_VORTIGAUNT_H_
#define _INCLUDE_NPC_VORTIGAUNT_H_

#include "CEntityManager.h"
#include "CEntity.h"
#include "CCycler_Fix.h"
#include "CAI_NPC.h"
#include "CAI_Behavior.h"
#include "CAI_Blended_Movement.h"
#include "CAI_basehumanoid.h"

#define		VORTIGAUNT_MAX_BEAMS				8

#define VORTIGAUNT_BEAM_ALL		-1
#define	VORTIGAUNT_BEAM_ZAP		0
#define VORTIGAUNT_BEAM_DISPEL	1

class CE_CSprite;

extern ConVar sk_vortigaunt_zap_range;

//=========================================================
//	>> CNPC_Vortigaunt
//=========================================================
class CNPC_Vortigaunt : public CAI_BaseHumanoid
{
public:
	CE_DECLARE_CLASS( CNPC_Vortigaunt, CAI_BaseHumanoid );

public:
	CNPC_Vortigaunt( void );

	virtual void	Spawn( void );
	virtual void	Precache( void );
	virtual float	MaxYawSpeed( void );

	virtual	Vector  FacingPosition( void );
	virtual Vector	BodyTarget( const Vector &posSrc, bool bNoisy = true );

	virtual void	PrescheduleThink( void );
	virtual void	BuildScheduleTestBits( void );
	virtual void	OnScheduleChange( void );

	virtual int		RangeAttack1Conditions( float flDot, float flDist );	// Primary zap
	virtual int		RangeAttack2Conditions( float flDot, float flDist );	// Concussive zap (larger)
	virtual bool	InnateWeaponLOSCondition( const Vector &ownerPos, const Vector &targetPos, bool bSetConditions );
	virtual int		MeleeAttack1Conditions( float flDot, float flDist );	// Dispel
	virtual float	InnateRange1MinRange( void ) { return 0.0f; }
	virtual float	InnateRange1MaxRange( void ) { return sk_vortigaunt_zap_range.GetFloat()*12; }
	virtual int		OnTakeDamage_Alive( const CTakeDamageInfo &info );
	virtual bool	FInViewCone_Entity( CBaseEntity *pEntity );
	virtual bool	ShouldMoveAndShoot( void );

	// vorts have a very long head/neck swing, so debounce heavily
	virtual	float	GetHeadDebounce( void ) { return 0.7; } // how much of previous head turn to use

	virtual void		Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual void		AlertSound( void );
	virtual Class_T		Classify ( void ) { return CLASS_VORTIGAUNT; }
	virtual void		HandleAnimEvent( animevent_t *pEvent );
	virtual Activity	NPC_TranslateActivity( Activity eNewActivity );

	virtual void	UpdateOnRemove( void );
	virtual void	Event_Killed( const CTakeDamageInfo &info );
	virtual	void	GatherConditions( void );
	virtual void	RunTask( const Task_t *pTask );
	virtual void	StartTask( const Task_t *pTask );
	virtual void	ClearSchedule( const char *szReason );

	virtual void	DeclineFollowing( void );
	virtual bool	CanBeUsedAsAFriend( void );
	virtual bool	IsPlayerAlly( void ) { return true; }

	// Override these to set behavior
	virtual int		TranslateSchedule( int scheduleType );
	virtual int		SelectSchedule( void );
	virtual int		SelectFailSchedule( int failedSchedule, int failedTask, AI_TaskFailureCode_t taskFailCode );
	virtual bool	IsValidEnemy( CBaseEntity *pEnemy );
	bool			IsLeading( void ) { return false; }

	void			DeathSound( const CTakeDamageInfo &info );
	void			PainSound( const CTakeDamageInfo &info );
	
	virtual void	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator );

	virtual int		IRelationPriority( CBaseEntity *pTarget );
	virtual Disposition_t 	IRelationType( CBaseEntity *pTarget );
	virtual bool	IsReadinessCapable( void ) { return true; }
	virtual float	GetReadinessDecay() { return 30.0f; }
	virtual bool	CanRunAScriptedNPCInteraction( bool bForced = false );
	virtual void	AimGun( void );
	virtual void	OnUpdateShotRegulator( void );

	void	InputDispel( inputdata_t &data );
	void	InputBeginCarryNPC( inputdata_t &indputdata );
	void	InputEndCarryNPC( inputdata_t &indputdata );

	// color
	void	InputTurnBlue( inputdata_t &data );
	void	InputTurnBlack( inputdata_t &data );

	virtual void	SetScriptedScheduleIgnoreConditions( Interruptability_t interrupt );
	virtual void    OnRestore( void );
	virtual bool	OverrideMoveFacing( const AILocalMoveGoal_t &move, float flInterval );
	virtual void	OnStartScene( void );
	virtual bool	IsInterruptable( void );
	virtual bool	CanFlinch( void );

	// used so a grub can notify me that I stepped on it. Says a line.
	void	OnSquishedGrub( const CBaseEntity *pGrub );

private:

	int		NumAntlionsInRadius( float flRadius );
	void	DispelAntlions( const Vector &vecOrigin, float flRadius, bool bDispel = true );

	void	CreateBeamBlast( const Vector &vecOrigin );

private:
	//=========================================================
	// Vortigaunt schedules
	//=========================================================
	enum
	{
		SCHED_VORTIGAUNT_STAND = BaseClass::NEXT_SCHEDULE,
		SCHED_VORTIGAUNT_RANGE_ATTACK,
		SCHED_VORTIGAUNT_EXTRACT_BUGBAIT,
		SCHED_VORTIGAUNT_FACE_PLAYER,
		SCHED_VORTIGAUNT_RUN_TO_PLAYER,
		SCHED_VORTIGAUNT_DISPEL_ANTLIONS,
		SCHED_VORT_FLEE_FROM_BEST_SOUND,
		SCHED_VORT_ALERT_FACE_BESTSOUND,
	};

	//=========================================================
	// Vortigaunt Tasks 
	//=========================================================
	enum 
	{
		TASK_VORTIGAUNT_EXTRACT_WARMUP = BaseClass::NEXT_TASK,
		TASK_VORTIGAUNT_EXTRACT,
		TASK_VORTIGAUNT_EXTRACT_COOLDOWN,
		TASK_VORTIGAUNT_FIRE_EXTRACT_OUTPUT,
		TASK_VORTIGAUNT_WAIT_FOR_PLAYER,
		TASK_VORTIGAUNT_DISPEL_ANTLIONS
	};

	//=========================================================
	// Vortigaunt Conditions
	//=========================================================
	enum
	{
		COND_VORTIGAUNT_DISPEL_ANTLIONS = BaseClass::NEXT_CONDITION,		// Repulse all antlions around us
	};

	// ------------
	// Beams
	// ------------
	inline bool		InAttackSequence( void );
	void			ClearBeams( void );
	void			ArmBeam( int beamType, int nHand );
	void			ZapBeam( int nHand );
	int				m_nLightningSprite;

	int				m_nBeamLaser;

	// ---------------
	//  Glow
	// ----------------
	void			ClearHandGlow( void );
	float			m_fGlowAge;
	float			m_fGlowChangeTime;
	bool			m_bGlowTurningOn;
	int				m_nCurGlowIndex;
		
	CE_CSprite*		m_pHandGlow[2];

	void			StartHandGlow( int beamType, int nHand );
	void			EndHandGlow( int beamType = VORTIGAUNT_BEAM_ALL );
	void			MaintainGlows( void );


	int				m_nNumTokensToSpawn;
	float			m_flPainTime;
	float			m_nextLineFireTime;

	float			m_flDispelTestTime;
		
	bool			IsCarryingNPC( void ) const { return m_bCarryingNPC; }
	bool			m_bCarryingNPC;

	COutputEvent	m_OnPlayerUse;
	
	//Adrian: Let's do it the right way!
	int				m_iLeftHandAttachment;
	int				m_iRightHandAttachment;
	bool			m_bStopLoopingSounds;
	float			m_flAimDelay;			// Amount of time to suppress aiming

	// used for fading from green vort to blue vort
	bool  m_bIsBlue;
	float m_flBlueEndFadeTime;

	// used for fading to black
	bool m_bIsBlack;

public:
	//DECLARE_SERVERCLASS();
	DECLARE_DATADESC();
	DEFINE_CUSTOM_AI;
};




#endif
