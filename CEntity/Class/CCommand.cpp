#include "CCommand.h"
#include "CEntity.h"

//CE_LINK_ENTITY_TO_CLASS(CPointServerCommand, CServerCommand);

DECLARE_DETOUR(InputCommand, CServerCommand);

DECLARE_DEFAULTHANDLER_DETOUR_void(CServerCommand, InputCommand, (inputdata_t *inputdata), (inputdata));
