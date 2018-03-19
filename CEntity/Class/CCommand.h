#ifndef _INCLUDE_CCOMMAND_H
#define _INCLUDE_CCOMMAND_H

#include "CEntityManager.h"
#include "CEntity.h"

class CServerCommand: public CPointEntity
{
public:
	CE_DECLARE_CLASS(CServerCommand, CPointEntity);
	
public:
	virtual void InputCommand(inputdata_t *inputdata);
	
public:
	DECLARE_DEFAULTHEADER_DETOUR(InputCommand, void, (inputdata_t *inputdata));
};

/*class CClientCommand: public CPointEntity
{
public:
	CE_DECLARE_CLASS(CClientCommand, CPointEntity);
	
public:
	virtual void InputCommand(inputdata_t *inputdata);
	
public:
	DECLARE_DEFAULTHEADER_DETOUR(InputCommand, void, (inputdata_t *inputdata));
};*/

#endif //_INCLUDE_CSERVERCOMMAND_H