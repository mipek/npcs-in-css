
#include "CEntity.h"
#include "CAI_goalentity.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



CE_LINK_ENTITY_TO_CLASS(CAI_GoalEntity, CE_AI_GoalEntity);


//Datamaps
DEFINE_PROP(m_hGoalEntity, CE_AI_GoalEntity);
DEFINE_PROP(m_flags, CE_AI_GoalEntity);
DEFINE_PROP(m_actors, CE_AI_GoalEntity);
DEFINE_PROP(m_SearchType, CE_AI_GoalEntity);
DEFINE_PROP(m_iszActor, CE_AI_GoalEntity);
DEFINE_PROP(m_iszGoal, CE_AI_GoalEntity);



void CE_AI_GoalEntity::ResolveNames()
{
	m_actors->SetCount( 0 );
	
	CEntity *pEntity = NULL;
	for (;;)
	{
		switch ( m_SearchType )
		{
			case ST_ENTNAME:
			{
				pEntity = g_helpfunc.FindEntityByName( (pEntity)?pEntity->BaseEntity():NULL, m_iszActor.ptr->ToCStr());
				break;
			}
			
			case ST_CLASSNAME:
			{
				pEntity = g_helpfunc.FindEntityByClassname( (pEntity)?pEntity->BaseEntity():NULL, m_iszActor.ptr->ToCStr());
				break;
			}
		}
		
		if ( !pEntity )
			break;
			
		CAI_NPC *pActor = pEntity->MyNPCPointer();
		
		if ( pActor  && pActor->GetState() != NPC_STATE_DEAD )
		{
			AIHANDLE temp;
			temp = pActor->BaseEntity();
			m_actors->AddToTail( temp );
		}
	}
		
	pEntity = g_helpfunc.FindEntityByName( (CBaseEntity *)NULL, m_iszGoal.ptr->ToCStr());
	m_hGoalEntity.ptr->Set((pEntity)?pEntity->BaseEntity():NULL);
}


void CE_AI_GoalEntity::PruneActors()
{
	for ( int i = m_actors->Count() - 1; i >= 0; i-- )
	{
		CAI_NPC *ptr = (CAI_NPC *)CEntity::Instance(m_actors->Element(i));
		if ( ptr == NULL || ptr->IsMarkedForDeletion() || ptr->GetState() == NPC_STATE_DEAD )
			m_actors->FastRemove( i );
	}
}