
#include "extension.h"
#include "CustomNpcManager.h"

static cell_t sm_mm_createnpc(IPluginContext *pContext, const cell_t *params)
{
	return g_CustomNPCManager.AddCustomNPC(pContext,(unsigned int)params[1]);
}

static cell_t sm_mm_destorynpc(IPluginContext *pContext, const cell_t *params)
{

	return 1;
}

static cell_t sm_mm_callbase(IPluginContext *pContext, const cell_t *params)
{
	return g_CustomNPCManager.CallBaseFunction(pContext,params);
}

sp_nativeinfo_t g_MonsterNatives[] = 
{ 

	{"MM_CreateNPC",				sm_mm_createnpc},
	{"MM_DestoryNPC",				sm_mm_destorynpc},
	{"MM_CallBase",					sm_mm_callbase},

	{NULL,							NULL}
};


