#ifndef _INCLUDE_GAMESYSTEM_H_
#define _INCLUDE_GAMESYSTEM_H_

#ifdef _WIN32
#pragma once
#endif

#include "CEntity.h"

abstract_class IGameSystem
{
public:
	virtual bool SDKInit() =0;
	virtual void SDKInitPost() =0;
	virtual void SDKShutdown() =0;
	virtual void LevelInitPreEntity() =0;
	virtual void LevelInitPostEntity() =0;
	virtual void LevelShutdownPreEntity() =0;
	virtual void LevelShutdownPostEntity() =0;
	virtual char *GetSystemName();

public:
	static void Add(IGameSystem* pSys);
	static bool SDKInitAllSystems();
	static void SDKInitPostAllSystems();
	static void LevelInitPreEntityAllSystems();
	static void LevelInitPostEntityAllSystems();
	static void LevelShutdownPreEntityAllSystems();
	static void LevelShutdownPostEntityAllSystems();
	static void SDKShutdownAllSystems();
	static void InitAllSystems();
	static void HookValveSystem();

protected:
	char system_name[65];
};


class CBaseGameSystem : public IGameSystem
{
public:
	CBaseGameSystem(const char *name);
	virtual bool SDKInit() { return true; }
	virtual void SDKInitPost() {}
	virtual void SDKShutdown() {}
	virtual void LevelInitPreEntity() {}
	virtual void LevelInitPostEntity() {}
	virtual void LevelShutdownPreEntity() {}
	virtual void LevelShutdownPostEntity() {}

public:
	CBaseGameSystem		*m_pNext;

};






abstract_class IValveGameSystem
{
public:
	// GameSystems are expected to implement these methods.
	virtual char const *Name() = 0;

	// Init, shutdown
	// return true on success. false to abort DLL init!
	virtual bool Init() = 0;
	virtual void PostInit() = 0;
	virtual void Shutdown() = 0;

	// Level init, shutdown
	virtual void LevelInitPreEntity() = 0;
	// entities are created / spawned / precached here
	virtual void LevelInitPostEntity() = 0;

	virtual void LevelShutdownPreClearSteamAPIContext() =0;

	virtual void LevelShutdownPreEntity() = 0;
	// Entities are deleted / released here...
	virtual void LevelShutdownPostEntity() = 0;
	// end of level shutdown
	
	// Called during game save
	virtual void OnSave() = 0;

	// Called during game restore, after the local player has connected and entities have been fully restored
	virtual void OnRestore() = 0;

	// Called every frame. It's safe to remove an igamesystem from within this callback.
	virtual void SafeRemoveIfDesired() = 0;

	virtual bool	IsPerFrame() = 0;

	// destructor, cleans up automagically....
	virtual ~IValveGameSystem();

	// Client systems can use this to get at the map name
	static char const*	MapName();

	// These methods are used to add and remove server systems from the
	// main server loop. The systems are invoked in the order in which
	// they are added.
	static void Add ( IValveGameSystem* pSys );
	static void Remove ( IValveGameSystem* pSys );
	static void RemoveAll (  );

	// These methods are used to initialize, shutdown, etc all systems
	static bool InitAllSystems();
	static void PostInitAllSystems();
	static void ShutdownAllSystems();
	static void LevelInitPreEntityAllSystems( char const* pMapName );
	static void LevelInitPostEntityAllSystems();
	static void LevelShutdownPreEntityAllSystems();
	static void LevelShutdownPostEntityAllSystems();

	static void OnSaveAllSystems();
	static void OnRestoreAllSystems();

	static void SafeRemoveIfDesiredAllSystems();

	static void FrameUpdatePreEntityThinkAllSystems();
	static void FrameUpdatePostEntityThinkAllSystems();
	static void PreClientUpdateAllSystems();

	// Accessors for the above function
	static CBasePlayer *RunCommandPlayer();
	static CUserCmd *RunCommandUserCmd();
};



class CValveBaseGameSystem : public IValveGameSystem
{
public:

	virtual char const *Name() { return "unnamed"; }

	// Init, shutdown
	// return true on success. false to abort DLL init!
	virtual bool Init() { return true; }
	virtual void PostInit() {}
	virtual void Shutdown() {}

	// Level init, shutdown
	virtual void LevelInitPreEntity() {}
	virtual void LevelInitPostEntity() {}
	virtual void LevelShutdownPreClearSteamAPIContext() { }
	virtual void LevelShutdownPreEntity() {}
	virtual void LevelShutdownPostEntity() {}

	virtual void OnSave() {}
	virtual void OnRestore() {}
	virtual void SafeRemoveIfDesired() {}

	virtual bool	IsPerFrame() { return false; }
private:

	// Prevent anyone derived from CBaseGameSystem from implementing these, they need
	//  to derive from CBaseGameSystemPerFrame below!!!

	// Called each frame before entities think
	virtual void FrameUpdatePreEntityThink() {}
	// called after entities think
	virtual void FrameUpdatePostEntityThink() {}
	virtual void PreClientUpdate() {}
};


class CValveAutoGameSystem : public CValveBaseGameSystem
{
public:
	CValveAutoGameSystem( char const *name = NULL );	// hooks in at startup, no need to explicitly add
	CValveAutoGameSystem		*m_pNext;

	virtual char const *Name() { return m_pszName ? m_pszName : "unnamed"; }

private:
	char const *m_pszName;
};



class IValveGameSystemPerFrame : public IValveGameSystem
{
public:
	// destructor, cleans up automagically....
	virtual ~IValveGameSystemPerFrame();

	// Called each frame before entities think
	virtual void FrameUpdatePreEntityThink() = 0;
	// called after entities think
	virtual void FrameUpdatePostEntityThink() = 0;
	virtual void PreClientUpdate() = 0;
};

class CValveBaseGameSystemPerFrame : public IValveGameSystemPerFrame
{
public:
	virtual char const *Name() { return "unnamed"; }

	// Init, shutdown
	// return true on success. false to abort DLL init!
	virtual bool Init() { return true; }
	virtual void PostInit() {}
	virtual void Shutdown() {}

	// Level init, shutdown
	virtual void LevelInitPreEntity() {}
	virtual void LevelInitPostEntity() {}
	virtual void LevelShutdownPreEntity() {}
	virtual void LevelShutdownPostEntity() {}

	virtual void OnSave() {}
	virtual void OnRestore() {}
	virtual void SafeRemoveIfDesired() {}

	virtual bool	IsPerFrame() { return true; }

	// Called each frame before entities think
	virtual void FrameUpdatePreEntityThink() { }
	// called after entities think
	virtual void FrameUpdatePostEntityThink() { }
	virtual void PreClientUpdate() { }
};













class CPropData : public CValveAutoGameSystem
{
public:
	CPropData( void );

	// Inherited from IAutoServerSystem
	virtual void LevelInitPreEntity( void );
	virtual void LevelShutdownPostEntity( void );

	// Read in the data from the prop data file
	void ParsePropDataFile( void );

	// Parse a keyvalues section into the prop
	int ParsePropFromKV( CBaseEntity *pProp, KeyValues *pSection, KeyValues *pInteractionSection );

	// Fill out a prop's with base data parsed from the propdata file
	int ParsePropFromBase( CBaseEntity *pProp, const char *pszPropData );

	// Get a random chunk in the specified breakable section
	const char *GetRandomChunkModel( const char *pszBreakableSection, int iMaxSize = -1 );

protected:
	KeyValues	*m_pKVPropData;
	bool		m_bPropDataLoaded;

	struct propdata_breakablechunk_t
	{
		string_t				iszChunkType;
		CUtlVector<string_t>	iszChunkModels;
	};
	CUtlVector<propdata_breakablechunk_t>	m_BreakableChunks;
};

extern CPropData *g_PropDataSystem;



#endif

