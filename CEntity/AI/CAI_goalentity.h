//========= Copyright ?1996-2005, Valve Corporation, All rights reserved. ============//
//
// Purpose:
//
//=============================================================================//

#ifndef AI_GOALENTITY_H
#define AI_GOALENTITY_H

#include "CAI_NPC.h"
#include "utlvector.h"

#if defined( _WIN32 )
#pragma once
#endif


class CE_AI_GoalEntity : public CEntity
{
public:
	CE_DECLARE_CLASS( CE_AI_GoalEntity, CEntity );

public:
	CEntity		*	GetGoalEntity();

protected:
	void			UpdateActors();

private:
	enum Flags_t
	{
		ACTIVE			= 0x01,
		RESOLVED_NAME 	= 0x02,
		DORMANT			= 0x04,
	};
	
	enum SearchType_t	
	{
		ST_ENTNAME,
		ST_CLASSNAME,
	};

	void PruneActors();
	void ResolveNames();

protected:
	DECLARE_DATAMAP(CFakeHandle, m_hGoalEntity);
	DECLARE_DATAMAP(unsigned int, m_flags);
	DECLARE_DATAMAP(CUtlVector<AIHANDLE>, m_actors);
	DECLARE_DATAMAP(SearchType_t, m_SearchType);
	DECLARE_DATAMAP(string_t, m_iszActor);
	DECLARE_DATAMAP(string_t, m_iszGoal);


};

inline void CE_AI_GoalEntity::UpdateActors()
{
	if ( !( m_flags & ACTIVE ) || !( m_flags & RESOLVED_NAME ) )
	{
		ResolveNames();
		m_flags |= RESOLVED_NAME;
	}
	else
		PruneActors();
}

inline CEntity *CE_AI_GoalEntity::GetGoalEntity()
{
	UpdateActors();
	return m_hGoalEntity;
}


#endif

