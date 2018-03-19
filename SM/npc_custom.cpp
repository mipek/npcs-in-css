
#include "npc_custom.h"
#include "CustomNpcManager.h"


CE_NPC_Custom::CE_NPC_Custom()
{
	sm_npc = NULL;
	member_data = NULL;

	/*gm_SchedLoadStatus.fValid = true;
	gm_SchedLoadStatus.signature = -1; 

	LoadSchedules();*/
}

CE_NPC_Custom::~CE_NPC_Custom()
{
	g_CustomNPCManager.CustomNpcRemove(this);
	if(member_data)
	{
		delete [] member_data;
		member_data = NULL;
	}

	sm_npc = NULL;
}


void CE_NPC_Custom::StartCallSpFunction()
{
	assert(sm_npc);
	if(sm_npc->member_data_size == 0)
		return;
	
	if(sm_npc->current_pNPC != this)
	{
		// save sp data to current npc->member_data
		if(sm_npc->current_pNPC == NULL)
			memcpy(sm_npc->original_member_data, sm_npc->pNPC, sm_npc->member_data_size);
		else
			memcpy(sm_npc->current_pNPC->member_data, sm_npc->pNPC, sm_npc->member_data_size);

		// save current using npc->member_data addr
		sm_npc->pNPC_stack.push(sm_npc->current_pNPC);

		// copy this npc->member_data to sp data
		memcpy(sm_npc->pNPC, member_data, sm_npc->member_data_size);

		sm_npc->current_pNPC = this;
	}
}

void CE_NPC_Custom::EndCallSpFunction()
{
	assert(sm_npc);
	if(sm_npc->member_data_size == 0)
		return;
	
	if(!sm_npc->pNPC_stack.empty())
	{
		CE_NPC_Custom *front = sm_npc->pNPC_stack.front();
		if(front == this)
			return;

		// save sp data to this npc->member_data
		memcpy(member_data, sm_npc->pNPC, sm_npc->member_data_size);

		// get stack npc->member_data
		sm_npc->current_pNPC = front;

		// copy stack member_data to sp data
		if(sm_npc->current_pNPC == NULL)
			memcpy(sm_npc->pNPC, sm_npc->original_member_data, sm_npc->member_data_size);
		else
			memcpy(sm_npc->pNPC, sm_npc->current_pNPC->member_data, sm_npc->member_data_size);

		sm_npc->pNPC_stack.pop();
	}
}

void CE_NPC_Custom::CE_PostInit()
{
	BaseClass::CE_PostInit();

	sm_npc = g_CustomNPCManager.FindCustomNPC(GetClassname());
	if(sm_npc == NULL)
	{
		g_pSM->LogError(myself, "Custom failed to Initialize.",GetClassname());
		return;
	}

	if(sm_npc->member_data_size > 0)
	{
		member_data = new unsigned char[sm_npc->member_data_size];
		memset(member_data, 0, sm_npc->original_member_data);
	}
	
	if(sm_npc->sp_function[EF_PostInit] != NULL)
	{
		StartCallSpFunction();
		sm_npc->sp_function[EF_PostInit]->Execute(NULL);
		EndCallSpFunction();
	}
}

void CE_NPC_Custom::Spawn()
{
	if(sm_npc == NULL)
	{
		BaseClass::Spawn();
		return;
	}

	if(sm_npc->sp_function[EF_Spawn] != NULL)
	{
		StartCallSpFunction();
		sm_npc->sp_function[EF_Spawn]->Execute(NULL);
		EndCallSpFunction();
	}
}



#if 0

//AI_SchedLoadStatus_t		CE_NPC_Custom::gm_SchedLoadStatus = { true, -1 }; 
CAI_ClassScheduleIdSpace 	CE_NPC_Custom::gm_ClassScheduleIdSpace; 
const char *				CE_NPC_Custom::gm_pszErrorClassName = "npc_custom"; 
CAI_LocalIdSpace 	CE_NPC_Custom::gm_SquadSlotIdSpace; 

	
/* --------------------------------------------- */ 
/* Load schedules for this type of NPC           */ 
/* --------------------------------------------- */ 
bool CE_NPC_Custom::LoadSchedules(void)
{
	/*return AI_DoLoadSchedules( CE_NPC_Custom::BaseClass::LoadSchedules, 
							   CE_NPC_Custom::InitCustomSchedules, 
						   &CE_NPC_Custom::gm_SchedLoadStatus ); */

	return AI_DoLoadSchedules( CE_NPC_Custom::BaseClass::LoadSchedules, 
							   CE_NPC_Custom::InitCustomSchedules, 
						   &gm_SchedLoadStatus ); 

	//return true;
}

bool CE_NPC_Custom::LoadedSchedules(void) 
{ 
	return CE_NPC_Custom::gm_SchedLoadStatus.fValid;
}

	
/* -------------------------------------------------- */ 
/* Given squadSlot enumeration return squadSlot name  */ 
/* -------------------------------------------------- */ 
const char* CE_NPC_Custom::SquadSlotName(int slotEN)
{
	return "";
}

void CE_NPC_Custom::InitCustomSchedules( void ) 
{ 
	typedef CE_NPC_Custom CNpc; 
	const char *pszClassName = "npc_custom"; 
		
	CUtlVector<char *> schedulesToLoad; 
	CUtlVector<AIScheduleLoadFunc_t> reqiredOthers; 
	CAI_NamespaceInfos scheduleIds; 
	CAI_NamespaceInfos taskIds; 
	CAI_NamespaceInfos conditionIds; 
	CAI_NamespaceInfos squadSlotIds;


}

#endif
