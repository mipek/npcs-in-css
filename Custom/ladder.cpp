
#include "ladder.h"
#include "CPlayer.h"
#include "GameSystem.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CE_LINK_ENTITY_TO_CLASS( func_useableladder, CFuncLadder );

// Sendprops
DEFINE_PROP(m_vecPlayerMountPositionTop, CFuncLadder);
DEFINE_PROP(m_vecPlayerMountPositionBottom, CFuncLadder);
DEFINE_PROP(m_vecLadderDir, CFuncLadder);


void CFuncLadder::GetTopPosition( Vector& org )
{
	ComputeAbsPosition( *(m_vecPlayerMountPositionTop) + GetLocalOrigin(), &org );
}

void CFuncLadder::GetBottomPosition( Vector& org )
{
	ComputeAbsPosition( *(m_vecPlayerMountPositionBottom) + GetLocalOrigin(), &org );
}

class CLadderTraceFilterSimple : public CTraceFilter
{
public:
	DECLARE_CLASS_NOBASE( CLadderTraceFilterSimple );
	CLadderTraceFilterSimple(CPlayer *pPlayer)
	{
		pTarget = pPlayer;
		didhit = false;
	}

	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask )
	{
		CEntity *pEntity = CE_EntityFromEntityHandle( pHandleEntity );
		if(pEntity == pTarget)
		{
			didhit = true;
			return true;
		}
		return false;
	}
public:
	CEntity *pTarget;
	bool didhit;

};

bool CFuncLadder::IsPlayerOnLadder(CPlayer *pPlayer)
{
	Vector start_pos, end_pos;
	GetBottomPosition(start_pos);
	GetTopPosition(end_pos);

	Vector playerMins = Vector(-16, -16, 0 );
	Vector playerMaxs = Vector( 16,  16,  64 );

	trace_t result;
	Ray_t ray;
	ray.Init( start_pos, end_pos, playerMins, playerMaxs );
	CLadderTraceFilterSimple traceFilter(pPlayer);

	enginetrace->TraceRay( ray, 0, &traceFilter, &result );

	return traceFilter.didhit;
}
