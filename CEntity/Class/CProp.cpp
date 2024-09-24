
#include "CProp.h"

CE_LINK_ENTITY_TO_CLASS(CBaseProp, CE_Prop);


SH_DECL_MANUALHOOK0(OverridePropdata, 0, 0, 0, bool);
DECLARE_HOOK(OverridePropdata, CE_Prop);
DECLARE_DEFAULTHANDLER(CE_Prop, OverridePropdata, bool, (), ());

void CE_Prop::Think()
{
	VALVE_BASEPTR original_think = m_pfnThink;
	if(original_think)
	{
		(BaseEntity()->*original_think)();
		return;
	}
}