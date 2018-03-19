
#include "CWorld.h"
#include "GameSystem.h"
#include "CESoundEnt.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



CE_LINK_ENTITY_TO_CLASS(CWorld, CE_CWorld);

void CE_CWorld::Precache()
{
	BaseClass::Precache();

	physenv = iphysics->GetActiveEnvironmentByIndex(0);

	CE_CSoundEnt::InitSoundEnt();

	g_Monster.Precache();
}

