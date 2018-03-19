#ifndef _INCLUDE_NPC_CUSTOM_H_
#define _INCLUDE_NPC_CUSTOM_H_


#include "CEntity.h"
#include "CCycler_Fix.h"
#include "CAI_NPC.h"


struct SM_CustomNPC;
enum Entity_Function;


class CE_NPC_Custom : public CE_Cycler_Fix
{
public:
	CE_DECLARE_CLASS( CE_NPC_Custom, CE_Cycler_Fix );

public:
	CE_NPC_Custom();
	~CE_NPC_Custom();

	void CE_PostInit();

	void Spawn();

private:
	void StartCallSpFunction();
	void EndCallSpFunction();

public:
	/*AI_SchedLoadStatus_t 				gm_SchedLoadStatus; 
	static CAI_ClassScheduleIdSpace 	gm_ClassScheduleIdSpace; 
	static const char *					gm_pszErrorClassName;
	
	static CAI_ClassScheduleIdSpace &AccessClassScheduleIdSpaceDirect() 	{ return gm_ClassScheduleIdSpace; } 
	virtual CAI_ClassScheduleIdSpace *	GetClassScheduleIdSpace()			{ return &gm_ClassScheduleIdSpace; } 
	virtual const char *				GetSchedulingErrorName()			{ return gm_pszErrorClassName; } 
	
	static void							InitCustomSchedules(void);
	
	bool							LoadSchedules(void);
	virtual bool						LoadedSchedules(void); 
	
	friend class ScheduleLoadHelperImpl;	
	
	class CScheduleLoader 
	{ 
	} m_ScheduleLoader; 
	
	friend class CScheduleLoader;


	static CAI_LocalIdSpace gm_SquadSlotIdSpace;
	const char*				SquadSlotName	(int squadSlotID);

*/

private:
	SM_CustomNPC *sm_npc;
	unsigned char *member_data;

	friend class CustomNPCManager;
};


#endif

