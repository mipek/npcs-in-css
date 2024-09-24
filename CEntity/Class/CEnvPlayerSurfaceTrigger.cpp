
#include "CEnvPlayerSurfaceTrigger.h"
#include "CPlayer.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CE_LINK_ENTITY_TO_CLASS(env_player_surface_trigger, CEnvPlayerSurfaceTrigger);

CUtlVector<EHANDLE> g_PlayerSurfaceTriggers;

//Datamaps
DEFINE_PROP(m_iTargetGameMaterial, CEnvPlayerSurfaceTrigger);
DEFINE_PROP(m_iCurrentGameMaterial, CEnvPlayerSurfaceTrigger);
DEFINE_PROP(m_bDisabled, CEnvPlayerSurfaceTrigger);
DEFINE_PROP(m_OnSurfaceChangedToTarget, CEnvPlayerSurfaceTrigger);
DEFINE_PROP(m_OnSurfaceChangedFromTarget, CEnvPlayerSurfaceTrigger);


CEnvPlayerSurfaceTrigger::~CEnvPlayerSurfaceTrigger( void )
{
	g_PlayerSurfaceTriggers.FindAndRemove( BaseEntity() );
}

void CEnvPlayerSurfaceTrigger::Spawn( void )
{
	BaseClass::Spawn();

	g_PlayerSurfaceTriggers.AddToTail( BaseEntity() );
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CEnvPlayerSurfaceTrigger::OnRestore( void )
{
	BaseClass::OnRestore();

	g_PlayerSurfaceTriggers.AddToTail( BaseEntity() );
}


void CEnvPlayerSurfaceTrigger::SetPlayerSurface( CPlayer *pPlayer, char gameMaterial )
{
	// Ignore players in the air (stops bunny hoppers escaping triggers)
	if ( gameMaterial == 0 )
		return;

	// Loop through the surface triggers and tell them all about the change
	int iCount = g_PlayerSurfaceTriggers.Count();
	for ( int i = 0; i < iCount; i++ )
	{
		CEnvPlayerSurfaceTrigger *cent = (CEnvPlayerSurfaceTrigger *)CEntity::Instance(g_PlayerSurfaceTriggers[i]);
		cent->PlayerSurfaceChanged( pPlayer, gameMaterial );
	}
}

void CEnvPlayerSurfaceTrigger::PlayerSurfaceChanged( CPlayer *pPlayer, char gameMaterial )
{
	if ( m_bDisabled )
		return;

	// Fire the output if we've changed, but only if it involves the target material
	if ( gameMaterial != (char)(*(m_iCurrentGameMaterial)) &&
		 ( gameMaterial == m_iTargetGameMaterial || *(m_iCurrentGameMaterial) == *(m_iTargetGameMaterial) ) )
	{
		DevMsg( 2, "Player changed material to %d (was %d)\n", gameMaterial, m_iCurrentGameMaterial );

		m_iCurrentGameMaterial = (int)gameMaterial;

		SetThink( &CEnvPlayerSurfaceTrigger::UpdateMaterialThink );
		SetNextThink( gpGlobals->curtime );
	}
}

void CEnvPlayerSurfaceTrigger::UpdateMaterialThink( void )
{
	if ( *(m_iCurrentGameMaterial) == *(m_iTargetGameMaterial) )
	{
		m_OnSurfaceChangedToTarget->FireOutput( (CBaseEntity *)NULL, BaseEntity() );
	}
	else
	{
		m_OnSurfaceChangedFromTarget->FireOutput( (CBaseEntity *)NULL, BaseEntity() );
	}
}
