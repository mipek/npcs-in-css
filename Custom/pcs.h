#ifndef _INCLUDE_CSERVERCOMMANDLOG_H_
#define _INCLUDE_CSERVERCOMMANDLOG_H_

#include "CEntityManager.h"
#include "CCommand.h"

class CServerCommandLog : public CServerCommand
{
public:
	CE_DECLARE_CLASS(CServerCommandLog, CServerCommand);

	virtual void InputCommand(inputdata_t *inputdata);
};

#endif // _INCLUDE_CSERVERCOMMANDLOG_H_
