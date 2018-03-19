
#ifndef _INCLUDE_CPROPDOOR_H_
#define _INCLUDE_CPROPDOOR_H_

#include "CEntity.h"
#include "CAI_NPC.h"
#include "CDynamicProp.h"

class CPropDoor : public CE_CDynamicProp
{
public:
	CE_DECLARE_CLASS( CPropDoor, CE_CDynamicProp );
	bool NPCOpenDoor(CAI_NPC *pNPC);
	bool IsDoorClosed();

protected:
	enum DoorState_t
	{
		DOOR_STATE_CLOSED = 0,
		DOOR_STATE_OPENING,
		DOOR_STATE_OPEN,
		DOOR_STATE_CLOSING,
		DOOR_STATE_AJAR,
	};


public:
	virtual float GetOpenInterval(void);

	

public:
	DECLARE_DEFAULTHEADER(GetOpenInterval, float, ());

protected: //Sendprops


protected: //Datamaps
	DECLARE_DATAMAP(DoorState_t, m_eDoorState);

};




#endif
