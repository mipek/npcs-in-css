
#include "CSode_Fix.h"



// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



void CSode_Fix::Spawn(void)
{

}

void CSode_Fix::Precache(void)
{

}

void CSode_Fix::Think(void)
{
	VALVE_BASEPTR original_think = m_pfnThink;
	if(original_think != NULL)
	{
		(BaseEntity()->*original_think)();
		return;
	}
}

