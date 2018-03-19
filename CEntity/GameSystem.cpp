
#include "GameSystem.h"
#include "stringregistry.h"


static CUtlVector<IGameSystem*> s_my_GameSystems( 0, 4 );
static	CBaseGameSystem *s_my_pSystemList = NULL;
static bool s_bSystemsInitted = false; 

typedef void (IGameSystem::*GameSystemFunc_t)();

CUtlVector<IValveGameSystem*> *s_GameSystems = NULL;
CPropData *g_PropDataSystem = NULL;

class CHookedValveAutoSystem : public CValveBaseGameSystem
{
public:
	char const *Name() { return "HookedValveAutoSystem"; }

	void LevelInitPreEntity()
	{
		IGameSystem::LevelInitPreEntityAllSystems();
	}
	void LevelInitPostEntity()
	{
		IGameSystem::LevelInitPostEntityAllSystems();
	}
	void LevelShutdownPreEntity()
	{
		IGameSystem::LevelShutdownPreEntityAllSystems();
	}
	void LevelShutdownPostEntity()
	{
		IGameSystem::LevelShutdownPostEntityAllSystems();
	}
};

static CHookedValveAutoSystem g_CHookedValveAutoSystem;

void InvokeMethod( GameSystemFunc_t f )
{
	int i;
	int c = s_my_GameSystems.Count();
	for ( i = 0; i < c ; ++i )
	{
		IGameSystem *sys = s_my_GameSystems[i];
		(sys->*f)();
	}
}

void IGameSystem::InitAllSystems()
{
	{
		// first add any auto systems to the end
		CBaseGameSystem *pSystem = s_my_pSystemList;
		while ( pSystem )
		{
			if ( s_my_GameSystems.Find( pSystem ) == s_my_GameSystems.InvalidIndex() )
			{
				Add( pSystem );
			}
			pSystem = pSystem->m_pNext;
		}
		s_my_pSystemList = NULL;
	}
	s_bSystemsInitted = true;
}

void IGameSystem::HookValveSystem()
{
	s_GameSystems->AddToTail(&g_CHookedValveAutoSystem);
}

void IGameSystem::Add( IGameSystem* pSys )
{
	s_my_GameSystems.AddToTail( pSys );
}

bool IGameSystem::SDKInitAllSystems()
{
	int i;
	int c = s_my_GameSystems.Count();
	for ( i = 0; i < c ; ++i )
	{
		IGameSystem *sys = s_my_GameSystems[i];
		if(!sys->SDKInit())
		{
			META_CONPRINTF("[*] %s SDKInit failed to Initialize.",sys->GetSystemName());
			return false;
		}
	}
	return true;
}

void IGameSystem::SDKInitPostAllSystems()
{
	int i;
	int c = s_my_GameSystems.Count();
	for ( i = 0; i < c ; ++i )
	{
		IGameSystem *sys = s_my_GameSystems[i];
		sys->SDKInitPost();
	}
}

void IGameSystem::LevelInitPreEntityAllSystems()
{
	InvokeMethod(&IGameSystem::LevelInitPreEntity);
}

void IGameSystem::LevelInitPostEntityAllSystems()
{
	InvokeMethod(&IGameSystem::LevelInitPostEntity);
}

void IGameSystem::LevelShutdownPreEntityAllSystems()
{
	InvokeMethod(&IGameSystem::LevelShutdownPreEntity);
}

void IGameSystem::LevelShutdownPostEntityAllSystems()
{
	InvokeMethod(&IGameSystem::LevelShutdownPostEntity);
}

void IGameSystem::SDKShutdownAllSystems()
{
	InvokeMethod(&IGameSystem::SDKShutdown);
}

char *IGameSystem::GetSystemName()
{
	return system_name;
}

CBaseGameSystem::CBaseGameSystem(const char *name)
{
	memset(system_name,0,sizeof(system_name));
	strncpy(system_name, name, strlen(name));

	if ( s_bSystemsInitted )
	{
		Add( this );
	}
	else
	{
		m_pNext = s_my_pSystemList;
		s_my_pSystemList = this;
	}
}

IValveGameSystem::~IValveGameSystem()
{
	if(s_GameSystems) {
		s_GameSystems->FindAndRemove( this );
	}
}

const char *CPropData::GetRandomChunkModel( const char *pszBreakableSection, int iMaxSize )
{
	if ( !m_bPropDataLoaded )
		return NULL;

	// Find the right section
	int iCount = m_BreakableChunks.Count();
	int i;
	for ( i = 0; i < iCount; i++ )
	{
		if ( !Q_strncmp( STRING(m_BreakableChunks[i].iszChunkType), pszBreakableSection, strlen(pszBreakableSection) ) )
			break;
	}
	if ( i == iCount )
		return NULL;

	// Now pick a random one and return it
	int iRandom;
	if ( iMaxSize == -1 )
	{
		iRandom = RandomInt( 0, m_BreakableChunks[i].iszChunkModels.Count()-1 );
	}
	else
	{
		// Don't pick anything over the specified size
		iRandom = RandomInt( 0, MIN(iMaxSize, m_BreakableChunks[i].iszChunkModels.Count()-1) );
	}

	return STRING(m_BreakableChunks[i].iszChunkModels[iRandom]);
}


