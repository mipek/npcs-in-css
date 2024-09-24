#ifndef _INCLUDE_CAI_FALLOWGOAL_FIX_H_
#define _INCLUDE_CAI_FALLOWGOAL_FIX_H_

#include "CEntity.h"
#include "CAI_goalentity.h"

abstract_class CAI_FollowGoal_Fix : public CE_AI_GoalEntity
{
public:
	CE_DECLARE_CLASS(CAI_FollowGoal_Fix, CE_AI_GoalEntity);
	CE_CUSTOM_ENTITY();

public:
	virtual void EnableGoal( CBaseEntity *pAI );
	virtual void DisableGoal( CBaseEntity *pAI );

};


#endif