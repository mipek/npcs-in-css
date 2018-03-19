
#ifndef _INCLUDE_CANIMATINGOVERLAY_H_
#define _INCLUDE_CANIMATINGOVERLAY_H_

#include "CAnimating.h"

class CAnimatingOverlay;

class CAnimationLayer
{
public:	
	DECLARE_CLASS_NOBASE( CAnimationLayer );
	
	CAnimationLayer( void );
	void	Init( CAnimatingOverlay *pOverlay );

	// float	SetBlending( int iBlender, float flValue, CBaseAnimating *pOwner );
	void	StudioFrameAdvance( float flInterval, CAnimating *pOwner );
	void	DispatchAnimEvents( CAnimating *eventHandler, CAnimating *pOwner );
	void	SetOrder( int nOrder );

	float GetFadeout( float flCurTime );

	// For CNetworkVars.
	void NetworkStateChanged();
	void NetworkStateChanged( void *pVar );

public:	

#define ANIM_LAYER_ACTIVE		0x0001
#define ANIM_LAYER_AUTOKILL		0x0002
#define ANIM_LAYER_KILLME		0x0004
#define ANIM_LAYER_DONTRESTORE	0x0008
#define ANIM_LAYER_CHECKACCESS	0x0010
#define ANIM_LAYER_DYING		0x0020

	int		m_fFlags;

	bool	m_bSequenceFinished;
	bool	m_bLooping;
	
	CNetworkVar( int, m_nSequence );
	CNetworkVar( float, m_flCycle );
	CNetworkVar( float, m_flPrevCycle );
	CNetworkVar( float, m_flWeight );
	
	float	m_flPlaybackRate;

	float	m_flBlendIn; // start and end blend frac (0.0 for now blend)
	float	m_flBlendOut; 

	float	m_flKillRate;
	float	m_flKillDelay;

	float	m_flLayerAnimtime;
	float	m_flLayerFadeOuttime;

	// For checking for duplicates
	Activity	m_nActivity;

	// order of layering on client
	int		m_nPriority;
	CNetworkVar( int, m_nOrder );

	bool	IsActive( void ) { return ((m_fFlags & ANIM_LAYER_ACTIVE) != 0); }
	bool	IsAutokill( void ) { return ((m_fFlags & ANIM_LAYER_AUTOKILL) != 0); }
	bool	IsKillMe( void ) { return ((m_fFlags & ANIM_LAYER_KILLME) != 0); }
	bool	IsAutoramp( void ) { return (m_flBlendIn != 0.0 || m_flBlendOut != 0.0); }
	void	KillMe( void ) { m_fFlags |= ANIM_LAYER_KILLME; }
	void	Dying( void ) { m_fFlags |= ANIM_LAYER_DYING; }
	bool	IsDying( void ) { return ((m_fFlags & ANIM_LAYER_DYING) != 0); }
	void	Dead( void ) { m_fFlags &= ~ANIM_LAYER_DYING; }

	bool	IsAbandoned( void );
	void	MarkActive( void );

	float	m_flLastEventCheck;

	float	m_flLastAccess;

	// Network state changes get forwarded here.
	CBaseEntity *m_pOwnerEntity;
	
	DECLARE_SIMPLE_DATADESC();
};

inline float CAnimationLayer::GetFadeout( float flCurTime )
{
	float s;

	if (m_flLayerFadeOuttime <= 0.0f)
	{
		s = 0;
	}
	else
	{
		// blend in over 0.2 seconds
		s = 1.0 - (flCurTime - m_flLayerAnimtime) / m_flLayerFadeOuttime;
		if (s > 0 && s <= 1.0)
		{
			// do a nice spline curve
			s = 3 * s * s - 2 * s * s * s;
		}
		else if ( s > 1.0f )
		{
			// Shouldn't happen, but maybe curtime is behind animtime?
			s = 1.0f;
		}
	}
	return s;
}


class CAnimatingOverlay : public CAnimating
{
public:
	CE_DECLARE_CLASS(CAnimatingOverlay, CAnimating);
	enum 
	{
		MAX_OVERLAYS = 15,
	};
public:
	int		AddGesture( Activity activity, bool autokill = true );
	int		AddLayeredSequence( int sequence, int iPriority );
	void	SetLayerPlaybackRate( int iLayer, float flPlaybackRate );
	void	SetLayerWeight( int iLayer, float flWeight );
	void	SetLayerNoRestore( int iLayer, bool bNoRestore );
	void	SetLayerCycle( int iLayer, float flCycle );
	void	SetLayerCycle( int iLayer, float flCycle, float flPrevCycle );
	void	RemoveLayer( int iLayer, float flKillRate = 0.2, float flKillDelay = 0.0 );
	bool	IsValidLayer( int iLayer );
	int		AddGestureSequence( int sequence, bool autokill = true );
	int		AddGestureSequence( int sequence, float flDuration, bool autokill = true );
	void	SetLayerPriority( int iLayer, int iPriority );
	void	SetLayerDuration( int iLayer, float flDuration );
	float	GetLayerDuration( int iLayer );
	void	SetLayerAutokill( int iLayer, bool bAutokill );
	int		FindGestureLayer( Activity activity );
	bool	IsPlayingGesture( Activity activity );
	void	RestartGesture( Activity activity, bool addifmissing = true, bool autokill = true );
	float	GetLayerCycle( int iLayer );
	int		GetLayerSequence( int iLayer );


private:
	int		AllocateLayer( int iPriority = 0 ); 

public:
	DECLARE_DEFAULTHEADER_DETOUR(AddGesture, int, (Activity activity, bool autokill));

protected:
	DECLARE_DATAMAP(CUtlVector< CAnimationLayer	> ,m_AnimOverlay);
	


};



// ------------------------------------------------------------------------------------------ //
// CAnimationLayer inlines.
// ------------------------------------------------------------------------------------------ //

inline void CAnimationLayer::SetOrder( int nOrder )
{
	m_nOrder = nOrder;
}

inline void CAnimationLayer::NetworkStateChanged()
{
	if ( m_pOwnerEntity )
		CEntity::Instance(m_pOwnerEntity)->NetworkStateChanged();
}

inline void CAnimationLayer::NetworkStateChanged( void *pVar )
{
	if ( m_pOwnerEntity )
		CEntity::Instance(m_pOwnerEntity)->NetworkStateChanged();
}




#endif
