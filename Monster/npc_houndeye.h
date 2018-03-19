//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:		
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//=============================================================================//

#ifndef NPC_HOUNDEYE_H
#define NPC_HOUNDEYE_H
#pragma once

#include "CAI_NPC.h"
#include "CCombatCharacter.h"
#include "CCycler_Fix.h"

class CNPC_Houndeye : public CE_Cycler_Fix
{
	CE_DECLARE_CLASS( CNPC_Houndeye, CE_Cycler_Fix );
	
public:
	void			Spawn( void );
	void			Precache( void );
	Class_T			Classify ( void );
	void			HandleAnimEvent( animevent_t *pEvent );
	float			MaxYawSpeed ( void );
	void			WarmUpSound ( void );
	void			AlertSound( void );
	void			DeathSound( const CTakeDamageInfo &info );
	void			WarnSound( void );
	void			PainSound( const CTakeDamageInfo &info );
	void			IdleSound( void );
	void			StartTask( const Task_t *pTask );
	void			RunTask( const Task_t *pTask );
	int				GetSoundInterests( void );
	void			SonicAttack( void );
	void			PrescheduleThink( void );
	int				RangeAttack1Conditions ( float flDot, float flDist );
	bool			FCanActiveIdle ( void );
	virtual int		TranslateSchedule( int scheduleType );
	Activity		NPC_TranslateActivity( Activity eNewActivity );
	virtual int		SelectSchedule( void );
	bool			HandleInteraction(int interactionType, void *data, CCombatCharacter* sourceEnt);
	void			NPCThink(void);
	int				OnTakeDamage_Alive( const CTakeDamageInfo &info );
	void			Event_Killed( const CTakeDamageInfo &info );
	bool			IsAnyoneInSquadAttacking( void );
	void			SpeakSentence( int sentenceType );

	//float			m_flNextSecondaryAttack;
	float			m_flSoundWaitTime;

	bool			m_bLoopClockwise;
	bool			m_fAsleep;// some houndeyes sleep in idle mode if this is set, the houndeye is lying down
	bool			m_fDontBlink;// don't try to open/close eye if this bit is set!

	DEFINE_CUSTOM_AI;
	
	DECLARE_DATADESC();
	
private:
	void			GetBeamColor ( int &bRed, int &bGreen, int &bBlue );
};


#endif // NPC_HOUNDEYE_H
