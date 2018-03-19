//========= Copyright © 1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose: 
//
// $NoKeywords: $
//=============================================================================//
#include "CEntity.h"
#include "CCombatWeapon.h"
#include "cutil.h"
#include "gameweaponmanager.h"
#include "saverestore_utlvector.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

//=========================================================
//=========================================================
class CE_CGameWeaponManager;
static CUtlVector<CE_CGameWeaponManager *> g_Managers;


//=========================================================
//=========================================================
class CE_CGameWeaponManager : public CEntity
{
	CE_DECLARE_CLASS( CE_CGameWeaponManager, CEntity );
	DECLARE_DATADESC();

public:
	void Spawn();
	CE_CGameWeaponManager()
	{
		m_flAmmoMod = 1.0f;
		m_bExpectingWeapon = false;
		g_Managers.AddToTail( this );
	}

	~CE_CGameWeaponManager()
	{
		g_Managers.FindAndRemove( this );
	}

	void Think();
	void InputSetMaxPieces( inputdata_t &inputdata );
	void InputSetAmmoModifier( inputdata_t &inputdata );

	string_t	m_iszWeaponName;
	int			m_iMaxPieces;
	float		m_flAmmoMod;
	bool		m_bExpectingWeapon;

	CUtlVector<EHANDLE> m_ManagedNonWeapons;

};

BEGIN_DATADESC( CE_CGameWeaponManager )

//fields	
	DEFINE_KEYFIELD( m_iszWeaponName, FIELD_STRING, "weaponname" ),
	DEFINE_KEYFIELD( m_iMaxPieces, FIELD_INTEGER, "maxpieces" ),
	DEFINE_KEYFIELD( m_flAmmoMod, FIELD_FLOAT, "ammomod" ),
	DEFINE_FIELD( m_bExpectingWeapon, FIELD_BOOLEAN ),
// funcs
	DEFINE_FUNCTION( Think ),
// inputs
	DEFINE_INPUTFUNC( FIELD_INTEGER, "SetMaxPieces", InputSetMaxPieces ),
	DEFINE_INPUTFUNC( FIELD_FLOAT, "SetAmmoModifier", InputSetAmmoModifier ),

	DEFINE_UTLVECTOR( m_ManagedNonWeapons, FIELD_EHANDLE ),

END_DATADESC()

CE_LINK_ENTITY_TO_CLASS( game_weapon_manager, CE_CGameWeaponManager );

void CE_CreateWeaponManager( const char *pWeaponName, int iMaxPieces )
{
	CE_CGameWeaponManager *pManager = (CE_CGameWeaponManager *)CreateEntityByName( "game_weapon_manager");

	if( pManager )
	{
		pManager->m_iszWeaponName = MAKE_STRING( pWeaponName );
		pManager->m_iMaxPieces = iMaxPieces;
		DispatchSpawn( pManager->BaseEntity() );
	}
}

void WeaponManager_AmmoMod( CCombatWeapon *pWeapon )
{
	for ( int i = 0; i < g_Managers.Count(); i++ )
	{
		if ( g_Managers[i]->m_iszWeaponName == pWeapon->m_iClassname )
		{
			int iNewClip = (int)(pWeapon->m_iClip1 * g_Managers[i]->m_flAmmoMod);
			int iNewRandomClip = iNewClip + RandomInt( -2, 2 );

			if ( iNewRandomClip > pWeapon->GetMaxClip1() )
			{
				iNewRandomClip = pWeapon->GetMaxClip1();
			}
			else if ( iNewRandomClip <= 0 )
			{
				//Drop at least one bullet.
				iNewRandomClip = 1;
			}

			pWeapon->m_iClip1 = iNewRandomClip;
		}
	}
}

void WeaponManager_AddManaged( CEntity *pWeapon )
{
	for ( int i = 0; i < g_Managers.Count(); i++ )
	{
		if ( g_Managers[i]->m_iszWeaponName == pWeapon->m_iClassname )
		{
			//Assert( g_Managers[i]->m_ManagedNonWeapons.Find( pWeapon ) == g_Managers[i]->m_ManagedNonWeapons.InvalidIndex() );
			g_Managers[i]->m_ManagedNonWeapons.AddToTail( pWeapon->BaseEntity() );
			break;
		}
	}
}

void WeaponManager_RemoveManaged( CEntity *pWeapon )
{
	for ( int i = 0; i < g_Managers.Count(); i++ )
	{
		if ( g_Managers[i]->m_iszWeaponName == pWeapon->m_iClassname )
		{
			int j = g_Managers[i]->m_ManagedNonWeapons.Find( pWeapon->BaseEntity() );
			if ( j != g_Managers[i]->m_ManagedNonWeapons.InvalidIndex() )
			{
				g_Managers[i]->m_ManagedNonWeapons.FastRemove( j );
			}
		}
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CE_CGameWeaponManager::Spawn()
{
	SetThink( &CE_CGameWeaponManager::Think );
	SetNextThink( gpGlobals->curtime );
	CEntity *pEntity = CreateEntityByName( STRING(m_iszWeaponName) );
	if ( !pEntity )
	{
		DevMsg("%s removed itself!\n", GetDebugName() );
		UTIL_Remove(this);
	}
	else
	{
		m_bExpectingWeapon = ( dynamic_cast<CCombatWeapon *>(pEntity) != NULL );
		UTIL_Remove(pEntity);
	}
}

//---------------------------------------------------------
// Count of all the weapons in the world of my type and
// see if we have a surplus. If there is a surplus, try
// to find suitable candidates for removal.
//
// Right now we just remove the first weapons we find that
// are behind the player, or are out of the player's PVS. 
// Later, we may want to score the results so that we
// removed the farthest gun that's not in the player's 
// viewcone, etc.
//
// Some notes and thoughts:
//
// This code is designed NOT to remove weapons that are 
// hand-placed by level designers. It should only clean
// up weapons dropped by dead NPCs, which is useful in
// situations where enemies are spawned in for a sustained
// period of time.
//
// Right now we PREFER to remove weapons that are not in the
// player's PVS, but this could be opposite of what we 
// really want. We may only want to conduct the cleanup on
// weapons that are IN the player's PVS.
//---------------------------------------------------------
void CE_CGameWeaponManager::Think()
{
	int i;

	// Don't have to think all that often. 
	SetNextThink( gpGlobals->curtime + 2.0 );

	const char *pszWeaponName = STRING( m_iszWeaponName );

	CUtlVector<CEntity *> candidates( 0, 64 );

	if ( m_bExpectingWeapon )
	{
		// Firstly, count the total number of weapons of this type in the world.
		// Also count how many of those can potentially be removed.
		CCombatWeapon *pWeapon = dynamic_cast<CCombatWeapon *>(g_helpfunc.FindEntityByClassname( (CBaseEntity*)NULL, pszWeaponName ));

		while( pWeapon )
		{
			if( !pWeapon->IsEffectActive( EF_NODRAW ) && pWeapon->IsRemoveable() )
			{
				candidates.AddToTail( pWeapon );
			}

			pWeapon = dynamic_cast<CCombatWeapon *>(g_helpfunc.FindEntityByClassname( pWeapon, pszWeaponName ));
		}
	}
	else
	{
		for ( i = 0; i < m_ManagedNonWeapons.Count(); i++)
		{
			CEntity *pEntity = CEntity::Instance(m_ManagedNonWeapons[i]);
			if ( pEntity )
			{
				//Assert( pEntity->m_iClassname == m_iszWeaponName );
				if ( !pEntity->IsEffectActive( EF_NODRAW ) )
				{
					candidates.AddToTail( pEntity );
				}
			}
			else
			{
				m_ManagedNonWeapons.FastRemove( i-- );
			}
		}
	}

	// Calculate the surplus.
	int surplus = candidates.Count() - m_iMaxPieces;

	// Based on what the player can see, try to clean up the world by removing weapons that
	// the player cannot see right at the moment.
	CEntity *pCandidate;
	for ( i = 0; i < candidates.Count() && surplus > 0; i++ )
	{
		bool fRemovedOne = false;

		pCandidate = candidates[i];
		//Assert( !pCandidate->IsEffectActive( EF_NODRAW ) );

#if 0
		if ( gpGlobals->maxClients == 1 )
		{
			CBasePlayer *pPlayer = UTIL_GetLocalPlayer();
			// Nodraw serves as a flag that this weapon is already being removed since
			// all we're really doing inside this loop is marking them for removal by
			// the entity system. We don't want to count the same weapon as removed
			// more than once.
			if( !UTIL_FindClientInPVS( pCandidate->edict() ) )
			{
				fRemovedOne = true;
			}
			else if( !pPlayer->FInViewCone( pCandidate ) )
			{
				fRemovedOne = true;
			}
			else if ( UTIL_DistApprox( pPlayer->GetAbsOrigin(), pCandidate->GetAbsOrigin() ) > (30*12) )
			{
				fRemovedOne = true;
			}
		}
		else
#endif
		{
			fRemovedOne = true;
		}

		if( fRemovedOne )
		{
			pCandidate->AddEffects( EF_NODRAW );
			UTIL_Remove( pCandidate );

			DevMsg( 2, "Surplus %s removed\n", pszWeaponName);
			surplus--;
		}
	}
}

//---------------------------------------------------------
//---------------------------------------------------------
void CE_CGameWeaponManager::InputSetMaxPieces( inputdata_t &inputdata )
{
	m_iMaxPieces = inputdata.value.Int();
}

//---------------------------------------------------------
//---------------------------------------------------------
void CE_CGameWeaponManager::InputSetAmmoModifier( inputdata_t &inputdata )
{
	m_flAmmoMod = inputdata.value.Float();
}
