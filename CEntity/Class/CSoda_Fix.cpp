
#include "CSoda_Fix.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


void CSoda_Fix::Spawn(void)
{

}

void CSoda_Fix::Think(void)
{
	VALVE_BASEPTR original_think = m_pfnThink;
	if(original_think != NULL)
	{
		(BaseEntity()->*original_think)();
		return;
	}
}