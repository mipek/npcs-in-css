/**
* =============================================================================
* CEntity Entity Handling Framework
* Copyright (C) 2011 Matt Woodrow.  All rights reserved.
* =============================================================================
*
* This program is free software; you can redistribute it and/or modify it under
* the terms of the GNU General Public License, version 3.0, as published by the
* Free Software Foundation.
* 
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
* FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
* details.
*
* You should have received a copy of the GNU General Public License along with
* this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _INCLUDE_CANIMATING_H_
#define _INCLUDE_CANIMATING_H_

#include "CEntity.h"
#include "bone_setup.h"
#include "animation.h"
#include "npcevent.h"


#define	BCF_NO_ANIMATION_SKIP	( 1 << 0 )	// Do not allow PVS animation skipping (mostly for attachments being critical to an entity)
#define	BCF_IS_IN_SPAWN			( 1 << 1 )	// Is currently inside of spawn, always evaluate animations

class CAnimating : public CEntity
{
public:
	CE_DECLARE_CLASS(CAnimating, CEntity);
	CAnimating();

public:
	virtual void StudioFrameAdvance();
	virtual void StudioFrameAdvanceManual(float flInterval);
	virtual void Ignite( float flFlameLifetime, bool bNPCOnly = true, float flSize = 0.0f, bool bCalledByLevelDesigner = false );
	virtual bool GetAttachment( int iAttachment, matrix3x4_t &attachmentToWorld );
	virtual void Extinguish();
	virtual void InvalidateBoneCache();
	virtual float GetIdealSpeed() const;
	virtual float GetIdealAccel() const;
	virtual float GetSequenceGroundSpeed( CStudioHdr *pStudioHdr, int iSequence );
	inline float GetSequenceGroundSpeed( int iSequence ) { return GetSequenceGroundSpeed(GetModelPtr(), iSequence); }
	virtual bool CanBecomeRagdoll( void );
	virtual void GetVelocity(Vector *vVelocity, AngularImpulse *vAngVelocity);
	virtual void PopulatePoseParameters( void );
	virtual	Vector GetGroundSpeedVelocity( void );
	virtual bool Dissolve(const char *pMaterialName, float flStartTime, bool bNPCOnly = true, int nDissolveType = 0, Vector vDissolverOrigin = vec3_origin, int iMagnitude = 0 );
	virtual void ClampRagdollForce( const Vector &vecForceIn, Vector *vecForceOut );
	virtual void HandleAnimEvent(animevent_t *pEvent);
	virtual	void DispatchAnimEvents ( CBaseEntity *eventHandler ); 

public:
	virtual CAnimating*	GetBaseAnimating() { return this; }

public:
	float			GetCycle() const;
	void			SetCycle(float flCycle);
	float			GetAnimTimeInterval() const;
	//bool			GetIntervalMovement( float flIntervalUsed, bool &bMoveSeqFinished, Vector &newPosition, QAngle &newAngles );
	//float			GetSequenceCycleRate( CStudioHdr *pStudioHdr, int iSequence );
	//inline float	GetSequenceCycleRate( int iSequence ) { return GetSequenceCycleRate(GetModelPtr(),iSequence); }
	//float			SequenceDuration( CStudioHdr *pStudioHdr, int iSequence );
	const float*	GetPoseParameterArray() { return  (float *)((unsigned char *)m_flPoseParameter.ptr); }
	bool			IsOnFire() { return ( (GetFlags() & FL_ONFIRE) != 0 ); }
	void Scorch( int rate, int floor );

	int		SelectWeightedSequence ( Activity activity );
	int		SelectWeightedSequence ( Activity activity, int curSequence );

	bool	HasPoseParameter( int iSequence, int iParameter );
	bool	HasPoseParameter( int iSequence, const char *szName );

	int		LookupPoseParameter( CStudioHdr *pStudioHdr, const char *szName );
	inline int	LookupPoseParameter( const char *szName ) { return LookupPoseParameter(GetModelPtr(), szName); }

	float	SetPoseParameter( CStudioHdr *pStudioHdr, const char *szName, float flValue );
	inline float SetPoseParameter( const char *szName, float flValue ) { return SetPoseParameter( GetModelPtr(), szName, flValue ); }
	float	SetPoseParameter( CStudioHdr *pStudioHdr, int iParameter, float flValue );
	inline float SetPoseParameter( int iParameter, float flValue ) { return SetPoseParameter( GetModelPtr(), iParameter, flValue ); }

	float	GetPoseParameter( int iPoseParameter );

	float GetSequenceMoveYaw( int iSequence );

	inline bool IsSequenceFinished( void ) { return m_bSequenceFinished; }

	int LookupAttachment( const char *szName );

	bool GetAttachment( const char *szName, Vector &absOrigin, QAngle &absAngles );
	bool GetAttachment( int iAttachment, Vector &absOrigin, QAngle &absAngles );

	bool GetAttachment(  const char *szName, Vector &absOrigin, Vector *forward = NULL, Vector *right = NULL, Vector *up = NULL );
	bool GetAttachment( int iAttachment, Vector &absOrigin, Vector *forward = NULL, Vector *right = NULL, Vector *up = NULL );

	int  LookupBone( const char *szName );

	void GetBonePosition ( int iBone, Vector &origin, QAngle &angles );
	void GetBoneTransform( int iBone, matrix3x4_t &pBoneToWorld );

	class CBoneCache *GetBoneCache( void );

	const char *GetSequenceName( int iSequence );

	void SetBodygroup( int iGroup, int iValue );

	float GetInstantaneousVelocity( float flInterval = 0.0 );
	float GetMovementFrame( float flDist );

	float	GetSequenceCycleRate( CStudioHdr *pStudioHdr, int iSequence );
	inline float	GetSequenceCycleRate( int iSequence ) { return GetSequenceCycleRate(GetModelPtr(),iSequence); }

	inline float SequenceDuration( void ) { return SequenceDuration( m_nSequence ); }
	float	SequenceDuration( CStudioHdr *pStudioHdr, int iSequence );
	inline float SequenceDuration( int iSequence ) { return SequenceDuration(GetModelPtr(), iSequence); }

	void  GetSequenceLinearMotion( int iSequence, Vector *pVec );

	int  FindTransitionSequence( int iCurrentSequence, int iGoalSequence, int *piDir );

	void ResetSequence(int nSequence);
	void ResetSequenceInfo();

	inline float	GetPlaybackRate();
	inline void		SetPlaybackRate( float rate );

	KeyValues	*GetSequenceKeyValues( int iSequence );

	int			LookupActivity( const char *label );
	int			LookupSequence ( const char *label );

	float GetEntryVelocity( int iSequence );

	int GetBodygroup( int iGroup );
	
	void					SetSequence(int nSequence);

	inline bool SequenceLoops( void ) { return m_bSequenceLoops; }

	bool ComputeHitboxSurroundingBox( Vector *pVecWorldMins, Vector *pVecWorldMaxs );

	void DoMuzzleFlash();
	
	int		GetBoneCacheFlags( void ) { return m_fBoneCacheFlags; }
	inline void	SetBoneCacheFlags( unsigned short fFlag ) { m_fBoneCacheFlags |= fFlag; }
	inline void	ClearBoneCacheFlags( unsigned short fFlag ) { m_fBoneCacheFlags &= ~fFlag; }

	LocalFlexController_t GetNumFlexControllers( void );
	const char *GetFlexControllerName( LocalFlexController_t iFlexController );

	float	EdgeLimitPoseParameter( int iParameter, float flValue, float flBase = 0.0f );

	int GetHitboxSet();
	inline void StopAnimation() { m_flPlaybackRate = 0; }

public:
	CStudioHdr *		GetModelPtr();
	inline int			GetSequence() { return m_nSequence; }

	
public:
	DECLARE_DEFAULTHEADER(StudioFrameAdvance, void, ());
	DECLARE_DEFAULTHEADER(Ignite, void,( float flFlameLifetime, bool bNPCOnly, float flSize , bool bCalledByLevelDesigner));
	DECLARE_DEFAULTHEADER(GetAttachment, bool, (int iAttachment, matrix3x4_t &attachmentToWorld));
	DECLARE_DEFAULTHEADER(Extinguish, void, ());
	DECLARE_DEFAULTHEADER(GetIdealSpeed, float, () const);
	DECLARE_DEFAULTHEADER(GetIdealAccel, float, () const);
	DECLARE_DEFAULTHEADER(GetSequenceGroundSpeed, float, (CStudioHdr *pStudioHdr, int iSequence));
	DECLARE_DEFAULTHEADER(CanBecomeRagdoll, bool, ());
	DECLARE_DEFAULTHEADER(GetVelocity, void, (Vector *vVelocity, AngularImpulse *vAngVelocity));
	DECLARE_DEFAULTHEADER(PopulatePoseParameters, void, ());
	DECLARE_DEFAULTHEADER(GetGroundSpeedVelocity, Vector, ());
	DECLARE_DEFAULTHEADER(ClampRagdollForce, void, ( const Vector &vecForceIn, Vector *vecForceOut ));
	DECLARE_DEFAULTHEADER(HandleAnimEvent, void, (animevent_t *pEvent));
	DECLARE_DEFAULTHEADER(DispatchAnimEvents, void, ( CBaseEntity *eventHandler )); 

public:
	DECLARE_DEFAULTHEADER_DETOUR(StudioFrameAdvanceManual, void, (float flInterval));
	//DECLARE_DEFAULTHEADER_DETOUR(GetModelPtr, CStudioHdr *, ());
	DECLARE_DEFAULTHEADER_DETOUR(InvalidateBoneCache, void, ());
	DECLARE_DEFAULTHEADER_DETOUR(Dissolve, bool, (const char *pMaterialName, float flStartTime, bool bNPCOnly, int nDissolveType, Vector vDissolverOrigin, int iMagnitude));


protected: //Sendprops
	DECLARE_SENDPROP(float,m_flPlaybackRate);
	DECLARE_SENDPROP(float *,m_flPoseParameter);
	DECLARE_SENDPROP(int,m_nBody);
	DECLARE_SENDPROP(int,m_nNewSequenceParity);
	DECLARE_SENDPROP(int,m_nResetEventsParity);
	DECLARE_SENDPROP(int,m_nHitboxSet);
	DECLARE_SENDPROP(unsigned char,m_nMuzzleFlashParity);

public:
	DECLARE_SENDPROP(int,m_nSkin);

	
protected: //Datamaps
	DECLARE_DATAMAP(float, m_flCycle);
	DECLARE_DATAMAP(int, m_nSequence);
	DECLARE_DATAMAP(bool, m_bSequenceLoops);
	DECLARE_DATAMAP(bool, m_bSequenceFinished);
	DECLARE_DATAMAP(float, m_flLastEventCheck);
	DECLARE_DATAMAP(unsigned short, m_fBoneCacheFlags);

public:
	DECLARE_DATAMAP(float, m_flGroundSpeed);

};



inline float CAnimating::GetCycle() const
{
	return m_flCycle;
}

inline void CAnimating::SetCycle(float flCycle)
{
	m_flCycle = flCycle;
}

inline float CAnimating::GetPlaybackRate()
{
	return m_flPlaybackRate;
}

inline void CAnimating::SetPlaybackRate( float rate )
{
	m_flPlaybackRate = rate;
}




#define ANIMATION_SEQUENCE_BITS			12	// 4096 sequences
#define ANIMATION_SKIN_BITS				10	// 1024 body skin selections FIXME: this seems way high
#define ANIMATION_BODY_BITS				32	// body combinations
#define ANIMATION_HITBOXSET_BITS		2	// hit box sets 
#if defined( TF_DLL )
#define ANIMATION_POSEPARAMETER_BITS	8	// pose parameter resolution
#else
#define ANIMATION_POSEPARAMETER_BITS	11	// pose parameter resolution
#endif
#define ANIMATION_PLAYBACKRATE_BITS		8	// default playback rate, only used on leading edge detect sequence changes


#endif // _INCLUDE_CANIMATING_H_
