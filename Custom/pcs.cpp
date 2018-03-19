#include "pcs.h"
#include "CEntity.h"

CE_LINK_ENTITY_TO_CLASS(CPointServerCommand, CServerCommandLog);

ConVar sm_log_pcs("sm_log_pcs", "1", FCVAR_NONE, "If set, point_servercommand usages will be logged.", true, 0.0, true, 1.0);

void CServerCommandLog::InputCommand(inputdata_t *inputdata)
{
	if(sm_log_pcs.GetBool())
	{
		g_pSM->LogMessage(myself, "point_servercommand: \"%s\"", inputdata->value.String());
	}

	BaseClass::InputCommand(inputdata);
}
