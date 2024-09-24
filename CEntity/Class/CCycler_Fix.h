#ifndef _INCLUDE_CCYCLER_FIX_H_
#define _INCLUDE_CCYCLER_FIX_H_

#include "CEntity.h"
//#include "CAI_NPC.h"
#include "CAI_NPC_AI_Patch.h"

class CTakeDamageInfo;

abstract_class CE_Cycler_Fix : public CAI_NPC_AI_Patch
{
public:
	CE_DECLARE_CLASS(CE_Cycler_Fix, CAI_NPC_AI_Patch);
	CE_CUSTOM_ENTITY();
	DECLARE_DATADESC();

public:
	virtual void PostConstructor();
	virtual void Spawn(void);
	virtual void Precache(void);
	virtual int ObjectCaps(void);
	virtual void Think(void);
	virtual int OnTakeDamage_Alive(const CTakeDamageInfo& info);
	virtual int OnTakeDamage(const CTakeDamageInfo& info);
	virtual bool IsAlive(void);
	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual Disposition_t IRelationType ( CBaseEntity *pTarget );
	virtual bool DispatchKeyValue( const char *szKeyName, const char *szValue );
	virtual void NPCInit();

private:
	int m_iNpchealth;
};

#endif

