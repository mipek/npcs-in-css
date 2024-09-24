#ifndef _INCLUDE_CAI_NPC_AI_PATCH_H
#define _INCLUDE_CAI_NPC_AI_PATCH_H

#include "CAI_NPC.h"

abstract_class CAI_NPC_AI_Patch : public CAI_NPC
{
public:
	CE_DECLARE_CLASS(CAI_NPC_AI_Patch, CAI_NPC);

public:
	virtual void RunTask( const Task_t *pTask );
	virtual void UpdateEfficiency( bool bInPVS );
	virtual CBaseEntity *FindNamedEntity( const char *name, IEntityFindFilter *pFilter = NULL);
	virtual bool IsPlayerAlly( CBaseEntity *pPlayer = NULL );
	virtual void CollectShotStats( const Vector &vecShootOrigin, const Vector &vecShootDir );
	virtual void FireBullets( const FireBulletsInfo_t &info );
	virtual bool HandleInteraction( int interactionType, void *data, CBaseEntity* sourceEnt );
	virtual	CBaseEntity *CreateCustomTarget( const Vector &vecOrigin, float duration = -1 );
	virtual int OnTakeDamage_Alive(const CTakeDamageInfo& info);

};

#endif