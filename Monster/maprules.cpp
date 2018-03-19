//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: Contains entities for implementing/changing game rules dynamically within each BSP.
//
//=============================================================================//

#include "CEntity.h"
#include "CPlayer.h"
#include "CAI_NPC.h"
#include "CInfoTarget_Fix.h"

class CERuleEntity : public CE_InfoTarget_Fix
{
public:
	CE_DECLARE_CLASS( CERuleEntity, CE_InfoTarget_Fix );

	void	Spawn( void );

	DECLARE_DATADESC();

	void	SetMaster( string_t iszMaster ) { m_iszMaster = iszMaster; }

protected:
	bool	CanFireForActivator( CBaseEntity *pActivator );

private:
	string_t	m_iszMaster;
};

BEGIN_DATADESC( CERuleEntity )

	DEFINE_KEYFIELD( m_iszMaster, FIELD_STRING, "master" ),

END_DATADESC()



void CERuleEntity::Spawn( void )
{
	SetSolid( SOLID_NONE );
	SetMoveType( MOVETYPE_NONE );
	AddEffects( EF_NODRAW );
}


bool CERuleEntity::CanFireForActivator( CBaseEntity *pActivator )
{
	if ( m_iszMaster != NULL_STRING )
	{
		if ( UTIL_IsMasterTriggered( m_iszMaster, CEntity::Instance(pActivator) ) )
			return true;
		else
			return false;
	}
	
	return true;
}

// 
// CRulePointEntity -- base class for all rule "point" entities (not brushes)
//
class CERulePointEntity : public CERuleEntity
{
public:
	DECLARE_DATADESC();
	CE_DECLARE_CLASS( CERulePointEntity, CERuleEntity );

	int		m_Score;
	void		Spawn( void );
};

//---------------------------------------------------------
// Save/Restore
//---------------------------------------------------------
BEGIN_DATADESC( CERulePointEntity )

	DEFINE_FIELD( m_Score,	FIELD_INTEGER ),

END_DATADESC()


void CERulePointEntity::Spawn( void )
{
	BaseClass::Spawn();
	SetModelName( NULL_STRING );
	m_Score = 0;
}

class CEGameEnd : public CERulePointEntity
{
	CE_DECLARE_CLASS( CGameEnd, CERulePointEntity );

public:
	DECLARE_DATADESC();

	void	InputGameEnd( inputdata_t &inputdata );
	void	Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
private:
};

BEGIN_DATADESC( CEGameEnd )

	// inputs
	DEFINE_INPUTFUNC( FIELD_VOID, "EndGame", InputGameEnd ),

END_DATADESC()

LINK_ENTITY_TO_CLASS( game_end, CGameEnd );


void CEGameEnd::InputGameEnd( inputdata_t &inputdata )
{
	g_pGameRules->EndMultiplayerGame();
}

void CEGameEnd::Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	if ( !CanFireForActivator( pActivator ) )
		return;

	g_pGameRules->EndMultiplayerGame();
}

