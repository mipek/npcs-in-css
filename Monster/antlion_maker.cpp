#include "CEntity.h"
#include "npc_antlion.h"
#include "antlion_maker.h"
#include "CSmoke_trail.h"
#include "CPlayer.h"



CAntlionMakerManager g_AntlionMakerManager( "CAntlionMakerManager" );

static const char *s_pPoolThinkContext = "PoolThinkContext";
static const char *s_pBlockedEffectsThinkContext = "BlockedEffectsThinkContext";
static const char *s_pBlockedCheckContext = "BlockedCheckContext";



#define ANTLION_MAKER_PLAYER_DETECT_RADIUS	512
#define ANTLION_MAKER_BLOCKED_MASS			250.0f		// half the weight of a car
#define ANTLION_MAKE_SPORE_SPAWNRATE		25.0f

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &vFightGoal - 
//-----------------------------------------------------------------------------
void CAntlionMakerManager::BroadcastFightGoal( const Vector &vFightGoal )
{
	CAntlionTemplateMaker *pMaker;

	for ( int i=0; i < m_Makers.Count(); i++ )
	{
		pMaker = m_Makers[i];

		if ( pMaker && pMaker->ShouldHearBugbait() )
		{
			pMaker->SetFightTarget( vFightGoal );
			pMaker->SetChildMoveState( ANTLION_MOVE_FIGHT_TO_GOAL );
			pMaker->UpdateChildren();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFightGoal - 
//-----------------------------------------------------------------------------
void CAntlionMakerManager::BroadcastFightGoal( CEntity *pFightGoal )
{
	CAntlionTemplateMaker *pMaker;

	for ( int i=0; i < m_Makers.Count(); i++ )
	{
		pMaker = m_Makers[i];

		if ( pMaker && pMaker->ShouldHearBugbait() )
		{
			pMaker->SetFightTarget( pFightGoal );
			pMaker->SetChildMoveState( ANTLION_MOVE_FIGHT_TO_GOAL );
			pMaker->UpdateChildren();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pFightGoal - 
//-----------------------------------------------------------------------------
void CAntlionMakerManager::BroadcastFollowGoal( CEntity *pFollowGoal )
{
	CAntlionTemplateMaker *pMaker;

	for ( int i=0; i < m_Makers.Count(); i++ )
	{
		pMaker = m_Makers[i];

		if ( pMaker && pMaker->ShouldHearBugbait() )
		{
			//pMaker->SetFightTarget( NULL );
			pMaker->SetFollowTarget( pFollowGoal );
			pMaker->SetChildMoveState( ANTLION_MOVE_FOLLOW );
			pMaker->UpdateChildren();
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAntlionMakerManager::GatherMakers( void )
{
	CEntity				*pSearch = NULL;
	CAntlionTemplateMaker	*pMaker;

	m_Makers.Purge();

	// Find these all once
	while ( ( pSearch = g_helpfunc.FindEntityByClassname( pSearch, "npc_antlion_template_maker" ) ) != NULL )
	{
		pMaker = static_cast<CAntlionTemplateMaker *>(pSearch);

		CEFakeHandle< CAntlionTemplateMaker > fake;
		fake.Set(pMaker->BaseEntity());
		m_Makers.AddToTail(fake);
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAntlionMakerManager::LevelInitPostEntity( void )
{
	//Find all antlion makers
	GatherMakers();
}

//-----------------------------------------------------------------------------
// Antlion template maker
//-----------------------------------------------------------------------------

LINK_ENTITY_TO_CUSTOM_CLASS( npc_antlion_template_maker, info_target, CAntlionTemplateMaker );

//DT Definition
BEGIN_DATADESC( CAntlionTemplateMaker )

	DEFINE_KEYFIELD( m_strSpawnGroup,	FIELD_STRING,	"spawngroup" ),
	DEFINE_KEYFIELD( m_strSpawnTarget,	FIELD_STRING,	"spawntarget" ),
	DEFINE_KEYFIELD( m_flSpawnRadius,	FIELD_FLOAT,	"spawnradius" ),
	DEFINE_KEYFIELD( m_strFightTarget,	FIELD_STRING,	"fighttarget" ),
	DEFINE_KEYFIELD( m_strFollowTarget,	FIELD_STRING,	"followtarget" ),
	DEFINE_KEYFIELD( m_bIgnoreBugbait,	FIELD_BOOLEAN,	"ignorebugbait" ),
	DEFINE_KEYFIELD( m_flVehicleSpawnDistance,	FIELD_FLOAT,	"vehicledistance" ),
	DEFINE_KEYFIELD( m_flWorkerSpawnRate,	FIELD_FLOAT,	"workerspawnrate" ),

	DEFINE_FIELD( m_nChildMoveState,	FIELD_INTEGER ),
	DEFINE_FIELD( m_hFightTarget,		FIELD_EHANDLE ),
	DEFINE_FIELD( m_hProxyTarget,		FIELD_EHANDLE ),
	DEFINE_FIELD( m_hFollowTarget,		FIELD_EHANDLE ),
	DEFINE_FIELD( m_iSkinCount,			FIELD_INTEGER ),
	DEFINE_FIELD( m_flBlockedBumpTime,  FIELD_TIME ),
	DEFINE_FIELD( m_bBlocked,			FIELD_BOOLEAN ),

	//DEFINE_UTLVECTOR( m_Children,		FIELD_EHANDLE ),

	DEFINE_KEYFIELD( m_iPool,			FIELD_INTEGER,	"pool_start" ),
	DEFINE_KEYFIELD( m_iMaxPool,		FIELD_INTEGER,	"pool_max" ),
	DEFINE_KEYFIELD( m_iPoolRegenAmount,FIELD_INTEGER,	"pool_regen_amount" ),
	DEFINE_KEYFIELD( m_flPoolRegenTime,	FIELD_FLOAT,	"pool_regen_time" ),

	DEFINE_INPUTFUNC( FIELD_STRING, "SetFightTarget",		InputSetFightTarget ),
	DEFINE_INPUTFUNC( FIELD_STRING, "SetFollowTarget",		InputSetFollowTarget ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"ClearFollowTarget",	InputClearFollowTarget ),
	DEFINE_INPUTFUNC( FIELD_VOID,	"ClearFightTarget",		InputClearFightTarget ),
	DEFINE_INPUTFUNC( FIELD_FLOAT,	"SetSpawnRadius",		InputSetSpawnRadius ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "AddToPool",			InputAddToPool ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetMaxPool",			InputSetMaxPool ),
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetPoolRegenAmount",	InputSetPoolRegenAmount ),
	DEFINE_INPUTFUNC( FIELD_FLOAT,	 "SetPoolRegenTime",	InputSetPoolRegenTime ),
	DEFINE_INPUTFUNC( FIELD_STRING,	 "ChangeDestinationGroup",	InputChangeDestinationGroup ),
	DEFINE_OUTPUT( m_OnAllBlocked, "OnAllBlocked" ),

	DEFINE_KEYFIELD( m_bCreateSpores,			FIELD_BOOLEAN,	"createspores" ),

	//DEFINE_THINKFUNC( PoolRegenThink ),
	//DEFINE_THINKFUNC( FindNodesCloseToPlayer ),
	//DEFINE_THINKFUNC( BlockedCheckFunc ),

END_DATADESC()

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CAntlionTemplateMaker::CAntlionTemplateMaker( void )
{
	m_hFightTarget.Set(NULL);
	m_hProxyTarget.Set(NULL);
	m_hFollowTarget.Set(NULL);
	m_nChildMoveState = ANTLION_MOVE_FREE;
	m_iSkinCount = 0;
	m_flBlockedBumpTime = 0.0f;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CAntlionTemplateMaker::~CAntlionTemplateMaker( void )
{
	DestroyProxyTarget();
	m_Children.Purge();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pAnt - 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::AddChild( CNPC_Antlion *pAnt )
{
	CEFakeHandle< CNPC_Antlion > fake;
	fake.Set(pAnt->BaseEntity());
	m_Children.AddToTail( fake );
	m_nLiveChildren = m_Children.Count();

	pAnt->SetOwnerEntity(BaseEntity());
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pAnt - 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::RemoveChild( CNPC_Antlion *pAnt )
{
	CEFakeHandle< CNPC_Antlion > fake;
	fake.Set(pAnt->BaseEntity());
	m_Children.FindAndRemove( fake );
	m_nLiveChildren = m_Children.Count();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::FixupOrphans( void )
{
	CEntity		*pSearch = NULL;
	CNPC_Antlion	*pAntlion = NULL;

	// Iterate through all antlions and see if there are any orphans
	while ( ( pSearch = g_helpfunc.FindEntityByClassname( pSearch, "npc_antlion" ) ) != NULL )
	{
		pAntlion = dynamic_cast<CNPC_Antlion *>(pSearch);

		// See if it's a live orphan
		if ( pAntlion && pAntlion->GetOwnerEntity() == NULL && pAntlion->IsAlive() )
		{
			// See if its parent was named the same as we are
			if ( stricmp( pAntlion->GetParentSpawnerName(), GetEntityName() ) == 0 )
			{
				// Relink us to this antlion, he's come through a transition and was orphaned
				AddChild( pAntlion );
			}
		}
	}	
}


//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::PrecacheTemplateEntity( CEntity *pEntity )
{
	BaseClass::PrecacheTemplateEntity( pEntity );

	// If we can spawn workers, precache the worker as well.			
	if ( m_flWorkerSpawnRate != 0 )
	{
		pEntity->AddSpawnFlags( SF_ANTLION_WORKER );
		pEntity->Precache();
	}
}	


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::Activate( void )
{
	FixupOrphans();

	BaseClass::Activate();

	// Are we using the pool behavior for coast?
	if ( m_iMaxPool )
	{
		if ( !m_flPoolRegenTime )
		{
			Msg("%s using pool behavior without a specified pool regen time.\n", GetClassname() );
			m_flPoolRegenTime = 0.1;
		}

		// Start up our think cycle unless we're reloading this map (which would reset it)
		if ( m_bDisabled == false && gpGlobals->eLoadType != MapLoad_LoadGame )
		{
			// Start our pool regeneration cycle
			SetContextThink( &CAntlionTemplateMaker::PoolRegenThink_CBE, gpGlobals->curtime + m_flPoolRegenTime, s_pPoolThinkContext );

			// Start our blocked effects cycle
			if ( hl2_episodic->GetBool() == true && HasSpawnFlags( SF_ANTLIONMAKER_DO_BLOCKEDEFFECTS ) )
			{
				SetContextThink( &CAntlionTemplateMaker::FindNodesCloseToPlayer_CBE, gpGlobals->curtime + 1.0f, s_pBlockedEffectsThinkContext );
			}
		}
	}

	ActivateAllSpores();
}

void CAntlionTemplateMaker::ActivateSpore( const char* sporename, Vector vOrigin )
{
	if ( m_bCreateSpores == false )
		return;

	char szName[64];
	Q_snprintf( szName, sizeof( szName ), "%s_spore", sporename );

	CE_SporeExplosion *pSpore = (CE_SporeExplosion*)g_helpfunc.FindEntityByName( (CBaseEntity *)NULL, szName );

	//One already exists...
	if ( pSpore )
	{	
		if ( pSpore->m_bDisabled == true )
		{
			inputdata_t inputdata;
			pSpore->InputEnable( inputdata );
		}

		return;
	}

	CEntity *pEnt = CreateEntityByName( "env_sporeexplosion" );

	if ( pEnt )
	{
		pSpore = dynamic_cast<CE_SporeExplosion*>(pEnt);
		
		if ( pSpore )
		{
			pSpore->SetAbsOrigin( vOrigin );
			pSpore->SetName( szName );
			pSpore->m_flSpawnRate = ANTLION_MAKE_SPORE_SPAWNRATE;
		}
	}
}

void CAntlionTemplateMaker::DisableSpore( const char* sporename )
{
	if ( m_bCreateSpores == false )
		return;

	char szName[64];
	Q_snprintf( szName, sizeof( szName ), "%s_spore", sporename );

	CE_SporeExplosion *pSpore = (CE_SporeExplosion*)g_helpfunc.FindEntityByName( (CBaseEntity *)NULL, szName );

	if ( pSpore && pSpore->m_bDisabled == false )
	{	
		inputdata_t inputdata;
		pSpore->InputDisable( inputdata );
		return;
	}
}

void CAntlionTemplateMaker::ActivateAllSpores( void )
{
	if ( m_bDisabled == true )
		return;

	if ( m_bCreateSpores == false )
		return;

	CHintCriteria	hintCriteria;

	hintCriteria.SetGroup( m_strSpawnGroup );
	hintCriteria.SetHintType( HINT_ANTLION_BURROW_POINT );

	CUtlVector<CE_AI_Hint *> hintList;
	CAI_HintManager::FindAllHints( vec3_origin, hintCriteria, &hintList );

	for ( int i = 0; i < hintList.Count(); i++ )
	{
		CE_AI_Hint *pTestHint = hintList[i];

		if ( pTestHint )
		{
			bool bBlank;
			if ( !AllHintsFromClusterBlocked( pTestHint, bBlank ) )
			{
				ActivateSpore( pTestHint->GetEntityName() , pTestHint->GetAbsOrigin() );
			}
		}
	}
}

void CAntlionTemplateMaker::DisableAllSpores( void )
{
	CHintCriteria	hintCriteria;

	hintCriteria.SetGroup( m_strSpawnGroup );
	hintCriteria.SetHintType( HINT_ANTLION_BURROW_POINT );

	CUtlVector<CE_AI_Hint *> hintList;
	CAI_HintManager::FindAllHints( vec3_origin, hintCriteria, &hintList );

	for ( int i = 0; i < hintList.Count(); i++ )
	{
		CE_AI_Hint *pTestHint = hintList[i];

		if ( pTestHint )
		{
			DisableSpore( pTestHint->GetEntityName() );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CBaseEntity
//-----------------------------------------------------------------------------
CEntity *CAntlionTemplateMaker::GetFightTarget( void )
{
	if ( m_hFightTarget != NULL )
		return m_hFightTarget;

	return m_hProxyTarget;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : CBaseEntity
//-----------------------------------------------------------------------------
CEntity *CAntlionTemplateMaker::GetFollowTarget( void )
{
	return m_hFollowTarget;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::UpdateChildren( void )
{
	//Update all children
	CNPC_Antlion *pAntlion = NULL;

	// Move through our child list
	int i=0;
	for ( ; i < m_Children.Count(); i++ )
	{
		pAntlion = m_Children[i];
		
		//HACKHACK
		//Let's just fix this up.
		//This guy might have been killed in another level and we just came back.
		if ( pAntlion == NULL )
		{
			m_Children.Remove( i );
			i--;
			continue;
		}
		
		if ( pAntlion->m_lifeState != LIFE_ALIVE )
			 continue;

		pAntlion->SetFightTarget( GetFightTarget() );
		pAntlion->SetFollowTarget( GetFollowTarget() );
		pAntlion->SetMoveState( m_nChildMoveState );
	}

	m_nLiveChildren = i;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : strTarget - 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::SetFightTarget( string_t strTarget, CEntity *pActivator, CEntity *pCaller )
{
	if ( HasSpawnFlags( SF_ANTLIONMAKER_RANDOM_FIGHT_TARGET ) )
	{
		CEntity *pSearch = m_hFightTarget;

		for ( int i = enginerandom->RandomInt(1,5); i > 0; i-- )
			pSearch = g_helpfunc.FindEntityByName( pSearch, strTarget, BaseEntity(), (pActivator)?pActivator->BaseEntity():NULL, (pCaller)?pCaller->BaseEntity():NULL );

		if ( pSearch != NULL )
		{
			SetFightTarget( pSearch );
		}
		else
		{
			SetFightTarget( g_helpfunc.FindEntityByName( (CBaseEntity *)NULL, strTarget, BaseEntity(), (pActivator)?pActivator->BaseEntity():NULL, (pCaller)?pCaller->BaseEntity():NULL ) );
		}
	}
	else 
	{
		SetFightTarget( g_helpfunc.FindEntityByName( (CBaseEntity *)NULL, strTarget, BaseEntity(), (pActivator)?pActivator->BaseEntity():NULL, (pCaller)?pCaller->BaseEntity():NULL) );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pEntity - 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::SetFightTarget( CEntity *pEntity )
{
	m_hFightTarget.Set((pEntity)?pEntity->BaseEntity():NULL);
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &position - 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::SetFightTarget( const Vector &position )
{
	CreateProxyTarget( position );
	
	m_hFightTarget.Set(NULL);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTarget - 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::SetFollowTarget( CEntity *pTarget )
{
	m_hFollowTarget.Set((pTarget)?pTarget->BaseEntity():NULL);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pTarget - 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::SetFollowTarget( string_t strTarget, CEntity *pActivator, CEntity *pCaller )
{
	CEntity *pSearch = g_helpfunc.FindEntityByName( (CBaseEntity *)NULL, strTarget, NULL, (pActivator)?pActivator->BaseEntity():NULL, (pCaller)?pCaller->BaseEntity():NULL );

	if ( pSearch != NULL )
	{
		SetFollowTarget( pSearch );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : state - 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::SetChildMoveState( AntlionMoveState_e state )
{
	m_nChildMoveState = state;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &position - 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::CreateProxyTarget( const Vector &position )
{
	// Create if we don't have one
	if ( m_hProxyTarget == NULL )
	{
		CEntity *cent = CreateEntityByName( "info_target" );
		if(cent)
			m_hProxyTarget.Set(cent->BaseEntity());
	}

	// Update if we do
	if ( m_hProxyTarget != NULL )
	{
		m_hProxyTarget->SetAbsOrigin( position );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::DestroyProxyTarget( void )
{
	if ( m_hProxyTarget )
	{
		UTIL_Remove( m_hProxyTarget );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : bIgnoreSolidEntities - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAntlionTemplateMaker::CanMakeNPC( bool bIgnoreSolidEntities )
{
	if ( m_nMaxLiveChildren == 0 )
		 return false;

	if ( !HasSpawnFlags( SF_ANTLIONMAKER_SPAWN_CLOSE_TO_TARGET ) )
	{
		if ( m_strSpawnGroup == NULL_STRING )
			 return BaseClass::CanMakeNPC( bIgnoreSolidEntities );
	}

	if ( m_nMaxLiveChildren > 0 && m_nLiveChildren >= m_nMaxLiveChildren )
		return false;

	// If we're spawning from a pool, ensure the pool has an antlion in it
	if ( m_iMaxPool && !m_iPool )
		return false;

	return true;
}

void CAntlionTemplateMaker::Enable( void )
{
	BaseClass::Enable();

	if ( m_iMaxPool )
	{
		SetContextThink( &CAntlionTemplateMaker::PoolRegenThink_CBE, gpGlobals->curtime + m_flPoolRegenTime, s_pPoolThinkContext );
	}

	if ( hl2_episodic->GetBool() == true && HasSpawnFlags( SF_ANTLIONMAKER_DO_BLOCKEDEFFECTS ) )
	{
		SetContextThink( &CAntlionTemplateMaker::FindNodesCloseToPlayer_CBE, gpGlobals->curtime + 1.0f, s_pBlockedEffectsThinkContext );
	}

	ActivateAllSpores();
}

void CAntlionTemplateMaker::Disable( void )
{
	BaseClass::Disable();

	SetContextThink( NULL, gpGlobals->curtime, s_pPoolThinkContext );
	SetContextThink( NULL, gpGlobals->curtime, s_pBlockedEffectsThinkContext );

	DisableAllSpores();
}


//-----------------------------------------------------------------------------
// Randomly turn it into an antlion worker if that is enabled for this maker.
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::ChildPreSpawn( CAI_NPC *pChild )
{
	BaseClass::ChildPreSpawn( pChild );

	if ( ( m_flWorkerSpawnRate > 0 ) && ( enginerandom->RandomFloat( 0, 1 ) < m_flWorkerSpawnRate ) )
	{
		pChild->AddSpawnFlags( SF_ANTLION_WORKER );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::MakeNPC( void )
{
	// If we're not restricting to hint groups, spawn as normal
	if ( !HasSpawnFlags( SF_ANTLIONMAKER_SPAWN_CLOSE_TO_TARGET ) )
	{
		if ( m_strSpawnGroup == NULL_STRING )
		{
			BaseClass::MakeNPC();
			return;
		}
	}

	if ( CanMakeNPC( true ) == false )
		return;

	// Set our defaults
	Vector	targetOrigin = GetAbsOrigin();
	QAngle	targetAngles = GetAbsAngles();

	// Look for our target entity
	CEntity *pTarget = g_helpfunc.FindEntityByName( (CBaseEntity *)NULL, m_strSpawnTarget, BaseEntity() );

	// Take its position if it exists
	if ( pTarget != NULL )
	{
		UTIL_PredictedPosition( pTarget, 1.5f, &targetOrigin );
	}

	Vector	spawnOrigin = vec3_origin;

	CE_AI_Hint *pNode = NULL;

	bool bRandom = HasSpawnFlags( SF_ANTLIONMAKER_RANDOM_SPAWN_NODE );

	if ( HasSpawnFlags( SF_ANTLIONMAKER_SPAWN_CLOSE_TO_TARGET ) )
	{
		if ( FindNearTargetSpawnPosition( spawnOrigin, m_flSpawnRadius, pTarget ) == false )
			return;
	}
	else
	{
		// If we can't find a spawn position, then we can't spawn this time
		if ( FindHintSpawnPosition( targetOrigin, m_flSpawnRadius, m_strSpawnGroup, &pNode, bRandom ) == false )
			return;

		pNode->GetPosition( HULL_MEDIUM, &spawnOrigin );
	}
	
	// Point at the current position of the enemy
	if ( pTarget != NULL )
	{
		targetOrigin = pTarget->GetAbsOrigin();
	}	
 	
	// Create the entity via a template
	CAI_NPC	*pent = NULL;
	CEntity *pEntity = NULL;
	g_helpfunc.MapEntity_ParseEntity( pEntity, STRING(m_iszTemplateData), NULL );
	
	if ( pEntity != NULL )
	{
		pent = (CAI_NPC *) pEntity;
	}

	if ( pent == NULL )
	{
		Warning("NULL Ent in NPCMaker!\n" );
		return;
	}
	
	if ( !HasSpawnFlags( SF_ANTLIONMAKER_SPAWN_CLOSE_TO_TARGET ) )
	{
		// Lock this hint node
		pNode->Lock( pEntity->BaseEntity() );
		
		// Unlock it in two seconds, this forces subsequent antlions to 
		// reject this point as a spawn point to spread them out a bit
		pNode->Unlock( 2.0f );
	}

	m_OnSpawnNPC.Set( pEntity->BaseEntity(), pEntity->BaseEntity(), BaseEntity() );

	pent->AddSpawnFlags( SF_NPC_FALL_TO_GROUND );

	ChildPreSpawn( pent );

	// Put us at the desired location
	pent->SetLocalOrigin( spawnOrigin );

	QAngle	spawnAngles;

	if ( pTarget )
	{
		// Face our spawning direction
		Vector	spawnDir = ( targetOrigin - spawnOrigin );
		VectorNormalize( spawnDir );

		VectorAngles( spawnDir, spawnAngles );
		spawnAngles[PITCH] = 0.0f;
		spawnAngles[ROLL] = 0.0f;
	}
	else if ( pNode )
	{
		spawnAngles = QAngle( 0, pNode->Yaw(), 0 );
	}

	pent->SetLocalAngles( spawnAngles );	
	DispatchSpawn( pent->BaseEntity() );
	
	pent->Activate();

	m_iSkinCount = ( m_iSkinCount + 1 ) % ANTLION_SKIN_COUNT;
	pent->m_nSkin = m_iSkinCount; 

	ChildPostSpawn( pent );

	// Hold onto the child
	CNPC_Antlion *pAntlion = dynamic_cast<CNPC_Antlion *>(pent);

	AddChild( pAntlion );

	m_bBlocked = false;
	SetContextThink( NULL, -1, s_pBlockedCheckContext );

	pAntlion->ClearBurrowPoint( spawnOrigin );

	if (!(m_spawnflags & SF_NPCMAKER_INF_CHILD))
	{
		if ( m_iMaxPool )
		{
			m_iPool--;
		}
		else
		{
			m_nMaxNumNPCs--;
		}

		if ( IsDepleted() )
		{
			m_OnAllSpawned.FireOutput( this, this );

			// Disable this forever.  Don't kill it because it still gets death notices
			SetThink( NULL );
			SetUse( NULL );
		}
	}
}

bool CAntlionTemplateMaker::FindPositionOnFoot( Vector &origin, float radius, CEntity *pTarget )
{
	int iMaxTries = 10;
	Vector vSpawnOrigin = pTarget->GetAbsOrigin();

	while ( iMaxTries > 0 )
	{
		vSpawnOrigin.x += enginerandom->RandomFloat( -radius, radius );
		vSpawnOrigin.y += enginerandom->RandomFloat( -radius, radius );
		vSpawnOrigin.z += 96;

		if ( ValidateSpawnPosition( vSpawnOrigin, pTarget ) == false )
		{
			iMaxTries--;
			continue;
		}

		origin = vSpawnOrigin;
		return true;
	}

	return false;
}

bool CAntlionTemplateMaker::FindPositionOnVehicle( Vector &origin, float radius, CEntity *pTarget )
{
	int iMaxTries = 10;
	Vector vSpawnOrigin = pTarget->GetAbsOrigin();
	vSpawnOrigin.z += 96;

	if ( pTarget == NULL )
		 return false;

	while ( iMaxTries > 0 )
	{
		Vector vForward, vRight;
		
		pTarget->GetVectors( &vForward, &vRight, NULL );

		float flSpeed = (pTarget->GetSmoothedVelocity().Length() * m_flVehicleSpawnDistance) * enginerandom->RandomFloat( 1.0f, 1.5f );
	
		vSpawnOrigin = vSpawnOrigin + (vForward * flSpeed) + vRight * enginerandom->RandomFloat( -radius, radius );

		if ( ValidateSpawnPosition( vSpawnOrigin, pTarget ) == false )
		{
			iMaxTries--;
			continue;
		}

		origin = vSpawnOrigin;
		return true;
	}

	return false;
}

bool CAntlionTemplateMaker::ValidateSpawnPosition( Vector &vOrigin, CEntity *pTarget )
{
	trace_t	tr;
	UTIL_TraceLine( vOrigin, vOrigin - Vector( 0, 0, 1024 ), MASK_BLOCKLOS | CONTENTS_WATER, NULL, COLLISION_GROUP_NONE, &tr );

	// Make sure this point is clear 
	if ( tr.fraction != 1.0 )
	{
		if ( tr.contents & ( CONTENTS_WATER ) )
			 return false;

		const surfacedata_t *psurf = physprops->GetSurfaceData( tr.surface.surfaceProps );

		if ( psurf )
		{
			if ( psurf->game.material != CHAR_TEX_SAND )
				return false;
		}

		if ( CAntlionRepellant::IsPositionRepellantFree( tr.endpos ) == false )
			 return false;
	
		trace_t trCheck;
		UTIL_TraceHull( tr.endpos, tr.endpos + Vector(0,0,5), NAI_Hull::Mins( HULL_MEDIUM ), NAI_Hull::Maxs( HULL_MEDIUM ), MASK_NPCSOLID, NULL, COLLISION_GROUP_NONE, &trCheck );

		if ( trCheck.DidHit() == false )
		{
			if ( pTarget )
			{
				if ( pTarget->IsPlayer() )
				{
					CBaseEntity *pVehicle = NULL;
					CPlayer *pPlayer = dynamic_cast < CPlayer *> ( pTarget );

					if ( pPlayer && pPlayer->GetVehicle() )
						pVehicle = ((CPlayer *)pTarget)->GetVehicle()->GetVehicleEnt();

					CTraceFilterSkipTwoEntities traceFilter( (pPlayer)?pPlayer->BaseEntity():NULL, pVehicle, COLLISION_GROUP_NONE );

					trace_t trVerify;
					
					Vector vVerifyOrigin = pPlayer->GetAbsOrigin() + pPlayer->GetViewOffset();
					float flZOffset = NAI_Hull::Maxs( HULL_MEDIUM ).z;
					UTIL_TraceLine( vVerifyOrigin, tr.endpos + Vector( 0, 0, flZOffset ), MASK_BLOCKLOS | CONTENTS_WATER, &traceFilter, &trVerify );

					if ( trVerify.fraction != 1.0f )
					{
						const surfacedata_t *psurf = physprops->GetSurfaceData( trVerify.surface.surfaceProps );

						if ( psurf )
						{
							if ( psurf->game.material == CHAR_TEX_DIRT )
							{
								return false;
							}
						}
					}
				}
			}

	
			vOrigin = trCheck.endpos + Vector(0,0,5);
			return true;
		}
		else
		{
			return false;
		}
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Find a position near the player to spawn the new antlion at
// Input  : &origin - search origin
//			radius - search radius
//			*retOrigin - found origin (if any)
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAntlionTemplateMaker::FindNearTargetSpawnPosition( Vector &origin, float radius, CEntity *pTarget )
{
	if ( pTarget )
	{
		CEntity *pVehicle = NULL;

		if ( pTarget->IsPlayer() )
		{
			CPlayer *pPlayer = ((CPlayer *)pTarget);

			if ( pPlayer->GetVehicle() )
				pVehicle = CEntity::Instance(((CPlayer *)pTarget)->GetVehicle()->GetVehicleEnt());
		}

		if ( pVehicle )
		     return FindPositionOnVehicle( origin, radius, pVehicle );
		else 
			 return FindPositionOnFoot( origin, radius, pTarget );
	}

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: Find a hint position to spawn the new antlion at
// Input  : &origin - search origin
//			radius - search radius
//			hintGroupName - search hint group name
//			*retOrigin - found origin (if any)
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAntlionTemplateMaker::FindHintSpawnPosition( const Vector &origin, float radius, string_t hintGroupName, CE_AI_Hint **pHint, bool bRandom )
{
	CE_AI_Hint *pChosenHint = NULL;

	CHintCriteria	hintCriteria;

	hintCriteria.SetGroup( hintGroupName );
	hintCriteria.SetHintType( HINT_ANTLION_BURROW_POINT );

	if ( bRandom )
	{
		hintCriteria.SetFlag( bits_HINT_NODE_RANDOM );
	}
	else
	{
		hintCriteria.SetFlag( bits_HINT_NODE_NEAREST );
	}
	
	// If requested, deny nodes that can be seen by the player
	if ( m_spawnflags & SF_NPCMAKER_HIDEFROMPLAYER )
	{
		hintCriteria.SetFlag( bits_HINT_NODE_NOT_VISIBLE_TO_PLAYER );
	}

	hintCriteria.AddIncludePosition( origin, radius );

	if ( bRandom == true )
	{
		pChosenHint = CAI_HintManager::FindHintRandom( NULL, origin, hintCriteria );
	}
	else
	{
		pChosenHint = CAI_HintManager::FindHint( origin, hintCriteria );
	}

	if ( pChosenHint != NULL )
	{
		bool bChosenHintBlocked = false;

		if ( AllHintsFromClusterBlocked( pChosenHint, bChosenHintBlocked ) )
		{
			if ( ( GetIndexForThinkContext( s_pBlockedCheckContext ) == NO_THINK_CONTEXT ) ||
				( GetNextThinkTick( s_pBlockedCheckContext ) == TICK_NEVER_THINK ) )
			{
				SetContextThink( &CAntlionTemplateMaker::BlockedCheckFunc_CBE, gpGlobals->curtime + 2.0f, s_pBlockedCheckContext );
			}

			return false;
		}
		
		if ( bChosenHintBlocked == true )
		{
			return false;
		}

		*pHint = pChosenHint;
		return true;
	}

	return false;
}

void CAntlionTemplateMaker::DoBlockedEffects( CEntity *pBlocker, Vector vOrigin )
{
	// If the object blocking the hole is a physics object, wobble it a bit.
	if( pBlocker )
	{
		IPhysicsObject *pPhysObj = pBlocker->VPhysicsGetObject();

		if( pPhysObj && pPhysObj->IsAsleep() )
		{
			// Don't bonk the object unless it is at rest.
			float x = RandomFloat( -5000, 5000 );
			float y = RandomFloat( -5000, 5000 );

			Vector vecForce = Vector( x, y, RandomFloat(10000, 15000) );
			pPhysObj->ApplyForceCenter( vecForce );

			UTIL_CreateDust(vOrigin, vec3_angle, 6.0f, 10.0f);
			//UTIL_CreateAntlionDust( vOrigin, vec3_angle, true );
			pBlocker->EmitSound( "NPC_Antlion.MeleeAttackSingle_Muffled" );
			pBlocker->EmitSound( "NPC_Antlion.TrappedMetal" );


			m_flBlockedBumpTime = gpGlobals->curtime + enginerandom->RandomFloat( 1.75, 2.75 );
		}
	}
}

CEntity *CAntlionTemplateMaker::AllHintsFromClusterBlocked( CE_AI_Hint *pNode, bool &bChosenHintBlocked )
{
	// Only do this for episodic content!
	if ( hl2_episodic->GetBool() == false )
		return NULL;

	CEntity *pBlocker = NULL;

	if ( pNode != NULL )
	{
		int iNumBlocked = 0;
		int iNumNodes = 0;

		CHintCriteria	hintCriteria;

		hintCriteria.SetGroup( m_strSpawnGroup );
		hintCriteria.SetHintType( HINT_ANTLION_BURROW_POINT );

		CUtlVector<CE_AI_Hint *> hintList;
		CAI_HintManager::FindAllHints( vec3_origin, hintCriteria, &hintList );
	
		for ( int i = 0; i < hintList.Count(); i++ )
		{
			CE_AI_Hint *pTestHint = hintList[i];

			if ( pTestHint )
			{
				if ( pTestHint->NameMatches( pNode->GetEntityName() ) )
				{
					bool bBlocked;

					iNumNodes++;

					Vector spawnOrigin;
					pTestHint->GetPosition( HULL_MEDIUM, &spawnOrigin );

					bBlocked = false;

					CEntity*	pList[20];
				
					int count = UTIL_EntitiesInBox( pList, 20, spawnOrigin + NAI_Hull::Mins( HULL_MEDIUM ), spawnOrigin + NAI_Hull::Maxs( HULL_MEDIUM ), 0 );

					//Iterate over all the possible targets
					for ( int i = 0; i < count; i++ )
					{
						if ( pList[i]->GetMoveType() != MOVETYPE_VPHYSICS )
							continue;

						if ( PhysGetEntityMass( pList[i] ) > ANTLION_MAKER_BLOCKED_MASS )
						{
							bBlocked = true;
							iNumBlocked++;
							pBlocker = pList[i];

							if ( pTestHint == pNode )
							{
								bChosenHintBlocked = true;
							}

							break;
						}
					}
				}
			}
		}

		//All the nodes from this cluster are blocked so start playing the effects.
		if ( iNumNodes > 0 && iNumBlocked == iNumNodes )
		{
			return pBlocker;
		}
	}

	return NULL;
}

void CAntlionTemplateMaker::FindNodesCloseToPlayer_CBE( void )
{
	CAntlionTemplateMaker *cent = (CAntlionTemplateMaker *)CEntity::Instance(reinterpret_cast<CBaseEntity *>(this));
	if(cent)
	{
		cent->FindNodesCloseToPlayer();
	}
}

void CAntlionTemplateMaker::FindNodesCloseToPlayer( void )
{
	SetContextThink( &CAntlionTemplateMaker::FindNodesCloseToPlayer_CBE, gpGlobals->curtime + enginerandom->RandomFloat( 0.75, 1.75 ), s_pBlockedEffectsThinkContext );

	CPlayer *pPlayer = UTIL_GetNearestPlayer(GetAbsOrigin());

	if ( pPlayer == NULL )
		 return;

	CHintCriteria hintCriteria;

	float flRadius = ANTLION_MAKER_PLAYER_DETECT_RADIUS;

	hintCriteria.SetGroup( m_strSpawnGroup );
	hintCriteria.SetHintType( HINT_ANTLION_BURROW_POINT );
	hintCriteria.AddIncludePosition( pPlayer->GetAbsOrigin(), ANTLION_MAKER_PLAYER_DETECT_RADIUS );

	CUtlVector<CE_AI_Hint *> hintList;

	if ( CAI_HintManager::FindAllHints( vec3_origin, hintCriteria, &hintList ) <= 0 )
		return;

	CUtlVector<string_t> m_BlockedNames;

	//----
	//What we do here is find all hints of the same name (cluster name) and figure out if all of them are blocked.
	//If they are then we only need to play the blocked effects once
	//---
	for ( int i = 0; i < hintList.Count(); i++ )
	{
		CE_AI_Hint *pNode = hintList[i];

		if ( pNode && pNode->HintMatchesCriteria( NULL, hintCriteria, pPlayer->GetAbsOrigin(), &flRadius ) )
		{
			bool bClusterAlreadyBlocked = false;

			//Have one of the nodes from this cluster been checked for blockage? If so then there's no need to do block checks again for this cluster.
			for ( int iStringCount = 0; iStringCount < m_BlockedNames.Count(); iStringCount++ )
			{
				if ( pNode->NameMatches( m_BlockedNames[iStringCount] ) )
				{
					bClusterAlreadyBlocked = true;
					break;
				}
			}

			if ( bClusterAlreadyBlocked == true )
				continue;

			Vector vHintPos;
			pNode->GetPosition( HULL_MEDIUM, &vHintPos );
		
			bool bBlank;
			if ( CEntity *pBlocker = AllHintsFromClusterBlocked( pNode, bBlank ) )
			{
				DisableSpore( pNode->GetEntityName() );
				DoBlockedEffects( pBlocker, vHintPos );
				m_BlockedNames.AddToTail( pNode->GetEntityName_String() );
			}
			else
			{
				ActivateSpore( pNode->GetEntityName() , pNode->GetAbsOrigin() );
			}
		}
	}
}

void CAntlionTemplateMaker::BlockedCheckFunc_CBE( void )
{
	CAntlionTemplateMaker *cent = (CAntlionTemplateMaker *)CEntity::Instance(reinterpret_cast<CBaseEntity *>(this));
	if(cent)
	{
		cent->BlockedCheckFunc();
	}
}

void CAntlionTemplateMaker::BlockedCheckFunc( void )
{
	SetContextThink( &CAntlionTemplateMaker::BlockedCheckFunc_CBE, -1, s_pBlockedCheckContext );

	if ( m_bBlocked == true )
		 return;

	CUtlVector<CE_AI_Hint *> hintList;
	int iBlocked = 0;

	CHintCriteria	hintCriteria;

	hintCriteria.SetGroup( m_strSpawnGroup );
	hintCriteria.SetHintType( HINT_ANTLION_BURROW_POINT );

	if ( CAI_HintManager::FindAllHints( vec3_origin, hintCriteria, &hintList ) > 0 )
	{
		for ( int i = 0; i < hintList.Count(); i++ )
		{
			CE_AI_Hint *pNode = hintList[i];

			if ( pNode )
			{
				Vector vHintPos;
				pNode->GetPosition( UTIL_GetNearestPlayer(GetAbsOrigin()), &vHintPos );

				CEntity*	pList[20];
				int count = UTIL_EntitiesInBox( pList, 20, vHintPos + NAI_Hull::Mins( HULL_MEDIUM ), vHintPos + NAI_Hull::Maxs( HULL_MEDIUM ), 0 );

				//Iterate over all the possible targets
				for ( int i = 0; i < count; i++ )
				{
					if ( pList[i]->GetMoveType() != MOVETYPE_VPHYSICS )
						continue;

					if ( PhysGetEntityMass( pList[i] ) > ANTLION_MAKER_BLOCKED_MASS )
					{
						iBlocked++;
						break;
					}
				}
			}
		}
	}

	if ( iBlocked > 0 && hintList.Count() == iBlocked )
	{
		m_bBlocked = true;
		m_OnAllBlocked.FireOutput( this, this );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Makes the antlion immediatley unburrow if it started burrowed
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::ChildPostSpawn( CAI_NPC *pChild )
{
	CNPC_Antlion *pAntlion = static_cast<CNPC_Antlion*>(pChild);

	if ( pAntlion == NULL )
		return;

	// Unburrow the spawned antlion immediately
	if ( pAntlion->m_bStartBurrowed )
	{
		pAntlion->BurrowUse( BaseEntity(), BaseEntity(), USE_ON, 0.0f );
	}

	// Set us to a follow target, if we have one
	if ( GetFollowTarget() )
	{
		pAntlion->SetFollowTarget( GetFollowTarget() );
	}
	else if ( ( m_strFollowTarget != NULL_STRING ) )
	{
		// If we don't already have a fight target, set it up
		SetFollowTarget( m_strFollowTarget );

		if ( GetFightTarget() == NULL )
		{
			SetChildMoveState( ANTLION_MOVE_FOLLOW );

			// If it's valid, fight there
			if ( GetFollowTarget() != NULL )
			{
				pAntlion->SetFollowTarget( GetFollowTarget() );
			}
		}
	}
	// See if we need to send them on their way to a fight goal
	if ( GetFightTarget() && !HasSpawnFlags( SF_ANTLIONMAKER_RANDOM_FIGHT_TARGET ) )
	{
		pAntlion->SetFightTarget( GetFightTarget() );
	}
	else if ( m_strFightTarget != NULL_STRING )
	{
		// If we don't already have a fight target, set it up
		SetFightTarget( m_strFightTarget );	
		SetChildMoveState( ANTLION_MOVE_FIGHT_TO_GOAL );

		// If it's valid, fight there
		if ( GetFightTarget() != NULL )
		{
			pAntlion->SetFightTarget( GetFightTarget() );
		}
	}

	// Set us to the desired movement state
	pAntlion->SetMoveState( m_nChildMoveState );

	// Save our name for level transitions
	pAntlion->SetParentSpawnerName( GetEntityName() );

	if ( m_hIgnoreEntity != NULL )
	{
		pChild->SetOwnerEntity( m_hIgnoreEntity->BaseEntity() );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::InputSetFightTarget( inputdata_t &inputdata )
{
	// Set our new goal
	m_strFightTarget = MAKE_STRING( inputdata.value.String() );

	SetFightTarget( m_strFightTarget, CEntity::Instance(inputdata.pActivator), CEntity::Instance(inputdata.pCaller) );
	SetChildMoveState( ANTLION_MOVE_FIGHT_TO_GOAL );
	
	UpdateChildren();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::InputSetFollowTarget( inputdata_t &inputdata )
{
	// Set our new goal
	m_strFollowTarget = MAKE_STRING( inputdata.value.String() );

	SetFollowTarget( m_strFollowTarget, CEntity::Instance(inputdata.pActivator), CEntity::Instance(inputdata.pCaller) );
	SetChildMoveState( ANTLION_MOVE_FOLLOW );
	
	UpdateChildren();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::InputClearFightTarget( inputdata_t &inputdata )
{
	SetFightTarget( NULL );
	SetChildMoveState( ANTLION_MOVE_FOLLOW );

	UpdateChildren();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::InputClearFollowTarget( inputdata_t &inputdata )
{
	SetFollowTarget( NULL );
	SetChildMoveState( ANTLION_MOVE_FIGHT_TO_GOAL );

	UpdateChildren();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::InputSetSpawnRadius( inputdata_t &inputdata )
{
	m_flSpawnRadius = inputdata.value.Float();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::InputAddToPool( inputdata_t &inputdata )
{
	PoolAdd( inputdata.value.Int() );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::InputSetMaxPool( inputdata_t &inputdata )
{
	m_iMaxPool = inputdata.value.Int();
	if ( m_iPool > m_iMaxPool )
	{
		m_iPool = m_iMaxPool;
	}

	// Stop regenerating if we're supposed to stop using the pool
	if ( !m_iMaxPool )
	{
		SetContextThink( NULL, gpGlobals->curtime, s_pPoolThinkContext );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::InputSetPoolRegenAmount( inputdata_t &inputdata )
{
	m_iPoolRegenAmount = inputdata.value.Int();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::InputSetPoolRegenTime( inputdata_t &inputdata )
{
	m_flPoolRegenTime = inputdata.value.Float();

	if ( m_flPoolRegenTime != 0.0f )
	{
		SetContextThink( &CAntlionTemplateMaker::PoolRegenThink_CBE, gpGlobals->curtime + m_flPoolRegenTime, s_pPoolThinkContext );
	}
	else
	{
		SetContextThink( NULL, gpGlobals->curtime, s_pPoolThinkContext );
	}
}

//-----------------------------------------------------------------------------
// Purpose: Pool behavior for coast
// Input  : iNumToAdd - 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::PoolAdd( int iNumToAdd )
{
	m_iPool = clamp( m_iPool + iNumToAdd, 0, m_iMaxPool );
}

void CAntlionTemplateMaker::PoolRegenThink_CBE( void )
{
	CAntlionTemplateMaker *cent = (CAntlionTemplateMaker *)CEntity::Instance(reinterpret_cast<CBaseEntity *>(this));
	if(cent)
	{
		cent->PoolRegenThink();
	}
}

//-----------------------------------------------------------------------------
// Purpose: Regenerate the pool
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::PoolRegenThink( void )
{
	if ( m_iPool < m_iMaxPool )
	{
		m_iPool = clamp( m_iPool + m_iPoolRegenAmount, 0, m_iMaxPool );
	}

	SetContextThink( &CAntlionTemplateMaker::PoolRegenThink_CBE, gpGlobals->curtime + m_flPoolRegenTime, s_pPoolThinkContext );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pVictim - 
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::DeathNotice( CBaseEntity *pVictim )
{
	CNPC_Antlion *pAnt = dynamic_cast<CNPC_Antlion *>(pVictim);
	if ( pAnt == NULL )
		return;

	// Take it out of our list
	RemoveChild( pAnt );

	// Check if all live children are now dead
	if ( m_nLiveChildren <= 0 )
	{
		// Fire the output for this case
		m_OnAllLiveChildrenDead.FireOutput( this, this );

		bool bPoolDepleted = ( m_iMaxPool != 0 && m_iPool == 0 );
		if ( bPoolDepleted || IsDepleted() )
		{
			// Signal that all our children have been spawned and are now dead
			m_OnAllSpawnedDead.FireOutput( this, this );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: If this had a finite number of children, return true if they've all
//			been created.
//-----------------------------------------------------------------------------
bool CAntlionTemplateMaker::IsDepleted( void )
{
	// If we're running pool behavior, we're never depleted
	if ( m_iMaxPool )
		return false;

	return BaseClass::IsDepleted();
}

//-----------------------------------------------------------------------------
// Purpose: Change the spawn group the maker is using
//-----------------------------------------------------------------------------
void CAntlionTemplateMaker::InputChangeDestinationGroup( inputdata_t &inputdata )
{
	// FIXME: This function is redundant to the base class version, remove the m_strSpawnGroup
	m_strSpawnGroup = inputdata.value.StringID();
}


