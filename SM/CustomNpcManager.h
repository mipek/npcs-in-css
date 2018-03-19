#ifndef _INCLUDE_CUSTOMNPCMANAGER_H
#define _INCLUDE_CUSTOMNPCMANAGER_H

#include "extension.h"
#include "IEntityFactory.h"
#include <sh_stack.h>


using namespace SourceHook;

class CE_NPC_Custom;

enum Entity_Function
{
	EF_NONE = 0,

	EF_Init,
	EF_PostInit,
	EF_Spawn,
	EF_Teleport,
	EF_UpdateOnRemove,
	EF_OnTakeDamage,


	EF_CENTITY,


	EF_CANIMATING,


	EF_CANIMATINGOVERLAY,


	EF_CFLEX,


	EF_CCOMBATCHARACTER,


	EF_CAI_NPC,

	EF_LAST
};

struct SM_CustomNPC
{
	IPluginRuntime *sp_runtime; // each .sp equal ONE NPC	
	IPluginFunction *sp_function[EF_LAST];
	unsigned int function_count;
	cell_t *g_pEntity;
	cell_t *g_pNPC_ID;
	cell_t *pNPC;
	unsigned int member_data_size;
	unsigned int npc_id;
	char npc_name[256];
	CStack<CE_NPC_Custom *> pNPC_stack;

	unsigned char *original_member_data;
	CE_NPC_Custom *current_pNPC;
};


class CustomNPCManager : public IEntityFactory_CE
{
public:
	CustomNPCManager();
	~CustomNPCManager();

	CEntity *Create(edict_t *pEdict, CBaseEntity *pEnt);
	void CustomNpcRemove(CE_NPC_Custom *npc);

	int AddCustomNPC(IPluginContext *pContext, unsigned int data_size);

	SM_CustomNPC *FindCustomNPC(const char *name);
	SM_CustomNPC *FindCustomNPC(IPluginRuntime *the_sp);

	void AddCustomNPC(SM_CustomNPC *npc);

	cell_t CallBaseFunction(IPluginContext *pContext, const cell_t *params);

private:
	void LinkCustomNPC(char *pClassName);


private:
	unsigned int npc_id;
};


extern CustomNPCManager g_CustomNPCManager;


#endif

