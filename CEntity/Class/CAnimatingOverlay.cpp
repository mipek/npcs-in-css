
#include "CAnimatingOverlay.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



CE_LINK_ENTITY_TO_CLASS(CBaseAnimatingOverlay, CAnimatingOverlay);


DECLARE_DETOUR(AddGesture, CAnimatingOverlay);
DECLARE_DEFAULTHANDLER_DETOUR(CAnimatingOverlay, AddGesture, int, (Activity activity, bool autokill),(activity, autokill));

//Datamaps
DEFINE_PROP(m_AnimOverlay, CAnimatingOverlay);



CAnimationLayer::CAnimationLayer( )
{
	Init( NULL );
}


void CAnimationLayer::Init( CAnimatingOverlay *pOverlay )
{
	m_pOwnerEntity = (pOverlay) ? pOverlay->BaseEntity() : NULL;
	m_fFlags = 0;
	m_flWeight = 0;
	m_flCycle = 0;
	m_flPrevCycle = 0;
	m_bSequenceFinished = false;
	m_nActivity = ACT_INVALID;
	m_nSequence = 0;
	m_nPriority = 0;
	m_nOrder.Set( CAnimatingOverlay::MAX_OVERLAYS );
	m_flKillRate = 100.0;
	m_flKillDelay = 0.0;
	m_flPlaybackRate = 1.0;
	m_flLastAccess = gpGlobals->curtime;
	m_flLayerAnimtime = 0;
	m_flLayerFadeOuttime = 0;
}



//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int	CAnimatingOverlay::AddLayeredSequence( int sequence, int iPriority )
{
	int i = AllocateLayer( iPriority );
	// No room?

	CAnimationLayer	*p_m_AnimOverlay = &m_AnimOverlay->Element(i);
	if ( IsValidLayer( i ) )
	{

		p_m_AnimOverlay->m_flCycle = 0;
		p_m_AnimOverlay->m_flPrevCycle = 0;
		p_m_AnimOverlay->m_flPlaybackRate = 1.0;
		p_m_AnimOverlay->m_nActivity = ACT_INVALID;
		p_m_AnimOverlay->m_nSequence = sequence;
		p_m_AnimOverlay->m_flWeight = 1.0f;
		p_m_AnimOverlay->m_flBlendIn = 0.0f;
		p_m_AnimOverlay->m_flBlendOut = 0.0f;
		p_m_AnimOverlay->m_bSequenceFinished = false;
		p_m_AnimOverlay->m_flLastEventCheck = 0;
		p_m_AnimOverlay->m_bLooping = ((GetSequenceFlags( GetModelPtr(), sequence ) & STUDIO_LOOPING) != 0);
	}

	return i;
}

void CAnimationLayer::MarkActive( void )
{ 
	m_flLastAccess = gpGlobals->curtime;
}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAnimatingOverlay::SetLayerWeight( int iLayer, float flWeight )
{
	if (!IsValidLayer( iLayer ))
		return;

	CAnimationLayer	*p_m_AnimOverlay = &m_AnimOverlay->Element(iLayer);
	flWeight = clamp( flWeight, 0.0f, 1.0f );
	p_m_AnimOverlay->m_flWeight = flWeight;
	p_m_AnimOverlay->MarkActive( );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAnimatingOverlay::SetLayerPlaybackRate( int iLayer, float flPlaybackRate )
{
	if (!IsValidLayer( iLayer ))
		return;

	Assert( flPlaybackRate > -1.0 && flPlaybackRate < 40.0);

	CAnimationLayer	*p_m_AnimOverlay = &m_AnimOverlay->Element(iLayer);
	p_m_AnimOverlay->m_flPlaybackRate = flPlaybackRate;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAnimatingOverlay::SetLayerNoRestore( int iLayer, bool bNoRestore )
{
	if (!IsValidLayer( iLayer ))
		return;

	CAnimationLayer	*p_m_AnimOverlay = &m_AnimOverlay->Element(iLayer);
	if (bNoRestore)
	{
		p_m_AnimOverlay->m_fFlags |= ANIM_LAYER_DONTRESTORE;
	}
	else
	{
		p_m_AnimOverlay->m_fFlags &= ~ANIM_LAYER_DONTRESTORE;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAnimatingOverlay::SetLayerCycle( int iLayer, float flCycle, float flPrevCycle )
{
	if (!IsValidLayer( iLayer ))
		return;

	CAnimationLayer	*p_m_AnimOverlay = &m_AnimOverlay->Element(iLayer);
	if (!p_m_AnimOverlay->m_bLooping)
	{
		flCycle = clamp( flCycle, 0.0, 1.0 );
		flPrevCycle = clamp( flPrevCycle, 0.0, 1.0 );
	}
	p_m_AnimOverlay->m_flCycle = flCycle;
	p_m_AnimOverlay->m_flPrevCycle = flPrevCycle;
	p_m_AnimOverlay->m_flLastEventCheck = flPrevCycle;
	p_m_AnimOverlay->MarkActive( );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAnimatingOverlay::RemoveLayer( int iLayer, float flKillRate, float flKillDelay )
{
	if (!IsValidLayer( iLayer ))
		return;

	CAnimationLayer	*p_m_AnimOverlay = &m_AnimOverlay->Element(iLayer);
	if (flKillRate > 0)
	{
		p_m_AnimOverlay->m_flKillRate = p_m_AnimOverlay->m_flWeight / flKillRate;
	}
	else
	{
		p_m_AnimOverlay->m_flKillRate = 100;
	}

	p_m_AnimOverlay->m_flKillDelay = flKillDelay;

	p_m_AnimOverlay->KillMe();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CAnimatingOverlay::AllocateLayer( int iPriority )
{
	int i;

	// look for an open slot and for existing layers that are lower priority
	int iNewOrder = 0;
	int iOpenLayer = -1;
	int iNumOpen = 0;
	for (i = 0; i < m_AnimOverlay->Count(); i++)
	{
		CAnimationLayer	*p_m_AnimOverlay = &m_AnimOverlay->Element(i);
		if ( p_m_AnimOverlay->IsActive() )
		{
			if (p_m_AnimOverlay->m_nPriority <= iPriority)
			{
				iNewOrder = max( iNewOrder, p_m_AnimOverlay->m_nOrder + 1 );
			}
		}
		else if (p_m_AnimOverlay->IsDying())
		{
			// skip
		}
		else if (iOpenLayer == -1)
		{
			iOpenLayer = i;
		}
		else
		{
			iNumOpen++;
		}
	}

	if (iOpenLayer == -1)
	{
		if (m_AnimOverlay->Count() >= MAX_OVERLAYS)
		{
			return -1;
		}

		iOpenLayer = m_AnimOverlay->AddToTail();
		CAnimationLayer	*p_m_AnimOverlay = &m_AnimOverlay->Element(iOpenLayer);
		p_m_AnimOverlay->Init( this );
	}

	// make sure there's always an empty unused layer so that history slots will be available on the client when it is used
	if (iNumOpen == 0)
	{
		if (m_AnimOverlay->Count() < MAX_OVERLAYS)
		{
			i = m_AnimOverlay->AddToTail();
			CAnimationLayer	*p_m_AnimOverlay = &m_AnimOverlay->Element(i);
			p_m_AnimOverlay->Init( this );
		}
	}

	for (i = 0; i < m_AnimOverlay->Count(); i++)
	{
		CAnimationLayer	*p_m_AnimOverlay = &m_AnimOverlay->Element(i);
		if ( p_m_AnimOverlay->m_nOrder >= iNewOrder && p_m_AnimOverlay->m_nOrder < MAX_OVERLAYS)
		{
			p_m_AnimOverlay->m_nOrder++;
		}
	}

	CAnimationLayer	*p_m_AnimOverlay = &m_AnimOverlay->Element(iOpenLayer);
	p_m_AnimOverlay->m_fFlags = ANIM_LAYER_ACTIVE;
	p_m_AnimOverlay->m_nOrder = iNewOrder;
	p_m_AnimOverlay->m_nPriority = iPriority;

	p_m_AnimOverlay->MarkActive();

	return iOpenLayer;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
bool CAnimatingOverlay::IsValidLayer( int iLayer )
{
	return (iLayer >= 0 && iLayer < m_AnimOverlay->Count() && (&m_AnimOverlay->Element(iLayer))->IsActive());
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CAnimatingOverlay::AddGestureSequence( int sequence, bool autokill /*= true*/ )
{
	int i = AddLayeredSequence( sequence, 0 );
	// No room?
	if ( IsValidLayer( i ) )
	{
		SetLayerAutokill( i, autokill );
	}

	return i;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CAnimatingOverlay::AddGestureSequence( int nSequence, float flDuration, bool autokill /*= true*/ )
{
	int iLayer = AddGestureSequence( nSequence, autokill );
	Assert( iLayer != -1 );

	if (iLayer >= 0 && flDuration > 0)
	{
		CAnimationLayer	*p_m_AnimOverlay = &m_AnimOverlay->Element(iLayer);
		p_m_AnimOverlay->m_flPlaybackRate = SequenceDuration( nSequence ) / flDuration;
	}
	return iLayer;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
void CAnimatingOverlay::SetLayerPriority( int iLayer, int iPriority )
{
	if (!IsValidLayer( iLayer ))
	{
		return;
	}

	CAnimationLayer	*p_m_AnimOverlay = &m_AnimOverlay->Element(iLayer);
	if (p_m_AnimOverlay->m_nPriority == iPriority)
	{
		return;
	}

	// look for an open slot and for existing layers that are lower priority
	int i;
	for (i = 0; i < m_AnimOverlay->Count(); i++)
	{
		p_m_AnimOverlay = &m_AnimOverlay->Element(i);
		if ( p_m_AnimOverlay->IsActive() )
		{
			if (p_m_AnimOverlay->m_nOrder > p_m_AnimOverlay->m_nOrder)
			{
				p_m_AnimOverlay->m_nOrder--;
			}
		}
	}

	int iNewOrder = 0;
	for (i = 0; i < m_AnimOverlay->Count(); i++)
	{
		p_m_AnimOverlay = &m_AnimOverlay->Element(i);
		if ( i != iLayer && p_m_AnimOverlay->IsActive() )
		{
			if (p_m_AnimOverlay->m_nPriority <= iPriority)
			{
				iNewOrder = max( iNewOrder, p_m_AnimOverlay->m_nOrder + 1 );
			}
		}
	}

	for (i = 0; i < m_AnimOverlay->Count(); i++)
	{
		p_m_AnimOverlay = &m_AnimOverlay->Element(i);
		if ( i != iLayer && p_m_AnimOverlay->IsActive() )
		{
			if ( p_m_AnimOverlay->m_nOrder >= iNewOrder)
			{
				p_m_AnimOverlay->m_nOrder++;
			}
		}
	}
	
	p_m_AnimOverlay = &m_AnimOverlay->Element(iLayer);
	p_m_AnimOverlay->m_nOrder = iNewOrder;
	p_m_AnimOverlay->m_nPriority = iPriority;
	p_m_AnimOverlay->MarkActive( );
	return;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------

float CAnimatingOverlay::GetLayerDuration( int iLayer )
{
	if (IsValidLayer( iLayer ))
	{
		CAnimationLayer	*p_m_AnimOverlay = &m_AnimOverlay->Element(iLayer);
		if (p_m_AnimOverlay->m_flPlaybackRate != 0.0f)
		{
			return (1.0 - p_m_AnimOverlay->m_flCycle) * SequenceDuration( p_m_AnimOverlay->m_nSequence ) / p_m_AnimOverlay->m_flPlaybackRate;
		}
		return SequenceDuration( p_m_AnimOverlay->m_nSequence );
	}
	return 0.0;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CAnimatingOverlay::SetLayerAutokill( int iLayer, bool bAutokill )
{
	if (!IsValidLayer( iLayer ))
		return;

	CAnimationLayer	*p_m_AnimOverlay = &m_AnimOverlay->Element(iLayer);
	if (bAutokill)
	{
		p_m_AnimOverlay->m_fFlags |= ANIM_LAYER_AUTOKILL;
	}
	else
	{
		p_m_AnimOverlay->m_fFlags &= ~ANIM_LAYER_AUTOKILL;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------

void CAnimatingOverlay::SetLayerDuration( int iLayer, float flDuration )
{
	if (IsValidLayer( iLayer ) && flDuration > 0)
	{
		CAnimationLayer	*p_m_AnimOverlay = &m_AnimOverlay->Element(iLayer);
		p_m_AnimOverlay->m_flPlaybackRate = SequenceDuration( p_m_AnimOverlay->m_nSequence ) / flDuration;
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : activity - 
//-----------------------------------------------------------------------------
int	CAnimatingOverlay::FindGestureLayer( Activity activity )
{
	for (int i = 0; i < m_AnimOverlay->Count(); i++)
	{
		CAnimationLayer	*p_m_AnimOverlay = &m_AnimOverlay->Element(i);
		if ( !(p_m_AnimOverlay->IsActive()) )
			continue;

		if ( p_m_AnimOverlay->IsKillMe() )
			continue;

		if ( p_m_AnimOverlay->m_nActivity == ACT_INVALID )
			continue;

		if ( p_m_AnimOverlay->m_nActivity == activity )
			return i;
	}

	return -1;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : activity - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool CAnimatingOverlay::IsPlayingGesture( Activity activity )
{
	return FindGestureLayer( activity ) != -1 ? true : false;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : activity - 
//-----------------------------------------------------------------------------
void CAnimatingOverlay::RestartGesture( Activity activity, bool addifmissing /*=true*/, bool autokill /*=true*/ )
{
	int idx = FindGestureLayer( activity );
	if ( idx == -1 )
	{
		if ( addifmissing )
		{
			AddGesture( activity, autokill );
		}
		return;
	}

	CAnimationLayer	*p_m_AnimOverlay = &m_AnimOverlay->Element(idx);
	p_m_AnimOverlay->m_flCycle = 0.0f;
	p_m_AnimOverlay->m_flPrevCycle = 0.0f;
	p_m_AnimOverlay->m_flLastEventCheck = 0.0f;
}


float CAnimatingOverlay::GetLayerCycle( int iLayer )
{
	if (!IsValidLayer( iLayer ))
		return 0.0;

	CAnimationLayer	*p_m_AnimOverlay = &m_AnimOverlay->Element(iLayer);
	return p_m_AnimOverlay->m_flCycle;
}


int CAnimatingOverlay::GetLayerSequence( int iLayer )
{
	if (!IsValidLayer( iLayer ))
	{
		return ACT_INVALID;
	}
	CAnimationLayer	*p_m_AnimOverlay = &m_AnimOverlay->Element(iLayer);
	return p_m_AnimOverlay->m_nSequence;
}

