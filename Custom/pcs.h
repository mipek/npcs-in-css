#ifndef _INCLUDE_CSERVERCOMMANDLOG_H_
#define _INCLUDE_CSERVERCOMMANDLOG_H_

#include "CEntityManager.h"
#include "CCommand.h"

class CEPointServerCommand : public CServerCommand
{
public:
	CE_DECLARE_CLASS(CEPointServerCommand, CServerCommand);

	void InputCommand(inputdata_t *inputdata) override;
};

#endif // _INCLUDE_CSERVERCOMMANDLOG_H_
