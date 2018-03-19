
#include "CEnvGunfire.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



CE_LINK_ENTITY_TO_CLASS(CEnvGunfire, CE_CEnvGunfire);

DEFINE_PROP(m_vecTargetPosition, CE_CEnvGunfire);
DEFINE_PROP(m_target, CE_CEnvGunfire);
DEFINE_PROP(m_hTarget, CE_CEnvGunfire);
DEFINE_PROP(m_iszTracerType, CE_CEnvGunfire);



void CE_CEnvGunfire::Spawn(void)
{
	BaseClass::Spawn();

	m_iszTracerType = AllocPooledString("Tracer");
}
