
#include "ItemRespawnSystem.h"
#include "CAnimating.h"
#include "CCombatWeapon.h"
#include "CItem.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



ItemRespawnSystem g_ItemRespawnSystem("ItemRespawnSystem");

#define WEAPON_MAX_DISTANCE_FROM_SPAWN 64

void GameFrame(bool simulating)
{
	g_ItemRespawnSystem.Think();
}

ItemRespawnSystem::ItemRespawnSystem(const char *name) : CBaseGameSystem(name)
{

}


void ItemRespawnSystem::LevelInitPreEntity()
{
	g_pSM->AddGameFrameHook(&GameFrame);
}

void ItemRespawnSystem::LevelShutdownPreEntity()
{
	g_pSM->RemoveGameFrameHook(&GameFrame);
	m_Item.RemoveAll();
}

void ItemRespawnSystem::Think()
{
	ManageObjectRelocation();
}

void ItemRespawnSystem::AddItem( ICItem *pItem )
{
	m_Item.AddToTail( pItem ); 
}
void ItemRespawnSystem::RemoveItem( ICItem *pItem )
{
	int i = m_Item.Find( pItem );
	if ( i != -1 )
		m_Item.FastRemove( i );
}
bool ItemRespawnSystem::FindItem( ICItem *pItem )
{
	return ( m_Item.Find( pItem ) != m_Item.InvalidIndex() ); 
}


// Respawn System
void ItemRespawnSystem::ManageObjectRelocation()
{
	Vector null_vel(0.0f,0.0f,0.0f);
	int iTotal = m_Item.Count();
	if ( iTotal > 0 )
	{
		for ( int i = 0; i < iTotal; i++ )
		{
			ICItem *pObject = m_Item[i];
			if ( !pObject )
				continue;
			
			if(pObject->ShouldMaterialize())
			{
				pObject->Materialize();
				continue;
			}

			Vector vSpawOrigin;
			QAngle vSpawnAngles;
			if ( pObject->GetObjectsOriginalParameters( vSpawOrigin, vSpawnAngles ) == false )
				continue;
			
			CEntity *pEntity = pObject->GetOuter();
			float flDistanceFromSpawn = (pEntity->GetAbsOrigin() - vSpawOrigin ).Length();
			if ( flDistanceFromSpawn <= WEAPON_MAX_DISTANCE_FROM_SPAWN )
				continue;
			
			bool shouldReset = false;
			IPhysicsObject *pPhysics = pEntity->VPhysicsGetObject();

			if ( pPhysics )
			{
				shouldReset = pPhysics->IsAsleep();
			} else {
				shouldReset = (pEntity->GetFlags() & FL_ONGROUND) ? true : false;
			}

			if ( !shouldReset )
				continue;

			/*bool shouldReset = (pEntity->GetFlags() & FL_ONGROUND) ? true : false;
			if ( !shouldReset )
				continue;*/
			
			pEntity->Teleport( &vSpawOrigin, &vSpawnAngles, &null_vel );
			pEntity->EmitSound( "AlyxEmp.Charge" );
			IPhysicsObject *pPhys = pEntity->VPhysicsGetObject();

			if ( pPhys )
			{
				pPhys->Wake();
			}
		}
	}
}


