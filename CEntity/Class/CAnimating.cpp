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

#include "CAnimating.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



CE_LINK_ENTITY_TO_CLASS(CBaseAnimating, CAnimating);

SH_DECL_MANUALHOOK0_void(StudioFrameAdvance, 0, 0, 0);
DECLARE_HOOK(StudioFrameAdvance, CAnimating);
DECLARE_DEFAULTHANDLER_void(CAnimating, StudioFrameAdvance, (), ());

SH_DECL_MANUALHOOK4_void(Ignite, 0, 0, 0, float, bool, float, bool);
DECLARE_HOOK(Ignite, CAnimating);
DECLARE_DEFAULTHANDLER_void(CAnimating, Ignite, (float flFlameLifetime, bool bNPCOnly, float flSize , bool bCalledByLevelDesigner), (flFlameLifetime, bNPCOnly, flSize, bCalledByLevelDesigner));

SH_DECL_MANUALHOOK2(GetAttachment, 0, 0, 0, bool, int , matrix3x4_t &);
DECLARE_HOOK(GetAttachment, CAnimating);
DECLARE_DEFAULTHANDLER(CAnimating, GetAttachment, bool, (int iAttachment, matrix3x4_t &attachmentToWorld), (iAttachment, attachmentToWorld));

SH_DECL_MANUALHOOK0_void(Extinguish, 0, 0, 0);
DECLARE_HOOK(Extinguish, CAnimating);
DECLARE_DEFAULTHANDLER_void(CAnimating, Extinguish, (), ());

SH_DECL_MANUALHOOK0(GetIdealSpeed, 0, 0, 0, float);
DECLARE_HOOK(GetIdealSpeed, CAnimating);
DECLARE_DEFAULTHANDLER(CAnimating, GetIdealSpeed, float, () const, ());

SH_DECL_MANUALHOOK0(GetIdealAccel, 0, 0, 0, float);
DECLARE_HOOK(GetIdealAccel, CAnimating);
DECLARE_DEFAULTHANDLER(CAnimating, GetIdealAccel, float, () const, ());

SH_DECL_MANUALHOOK2(GetSequenceGroundSpeed, 0, 0, 0, float, CStudioHdr *, int);
DECLARE_HOOK(GetSequenceGroundSpeed, CAnimating);
DECLARE_DEFAULTHANDLER(CAnimating, GetSequenceGroundSpeed, float, (CStudioHdr *pStudioHdr, int iSequence), (pStudioHdr, iSequence));

SH_DECL_MANUALHOOK0(CanBecomeRagdoll, 0, 0, 0, bool);
DECLARE_HOOK(CanBecomeRagdoll, CAnimating);
DECLARE_DEFAULTHANDLER(CAnimating, CanBecomeRagdoll, bool, (), ());

SH_DECL_MANUALHOOK2_void(GetVelocity, 0, 0, 0, Vector *, AngularImpulse *);
DECLARE_HOOK(GetVelocity, CAnimating);
DECLARE_DEFAULTHANDLER_void(CAnimating, GetVelocity, (Vector *vVelocity, AngularImpulse *vAngVelocity), (vVelocity, vAngVelocity));

SH_DECL_MANUALHOOK0_void(PopulatePoseParameters, 0, 0, 0);
DECLARE_HOOK(PopulatePoseParameters, CAnimating);
DECLARE_DEFAULTHANDLER_void(CAnimating, PopulatePoseParameters, (), ()); 

SH_DECL_MANUALHOOK0(GetGroundSpeedVelocity, 0, 0, 0,Vector);
DECLARE_HOOK(GetGroundSpeedVelocity, CAnimating);
DECLARE_DEFAULTHANDLER(CAnimating, GetGroundSpeedVelocity, Vector, (), ()); 

SH_DECL_MANUALHOOK2_void(ClampRagdollForce, 0, 0, 0, const Vector &, Vector *);
DECLARE_HOOK(ClampRagdollForce, CAnimating);
DECLARE_DEFAULTHANDLER_void(CAnimating, ClampRagdollForce, (const Vector &vecForceIn, Vector *vecForceOut), (vecForceIn, vecForceOut));

SH_DECL_MANUALHOOK1_void(HandleAnimEvent, 0, 0, 0, animevent_t *);
DECLARE_HOOK(HandleAnimEvent, CAnimating);
DECLARE_DEFAULTHANDLER_void(CAnimating, HandleAnimEvent,(animevent_t *pEvent), (pEvent));

SH_DECL_MANUALHOOK1_void(DispatchAnimEvents, 0, 0, 0, CBaseEntity *);
DECLARE_HOOK(DispatchAnimEvents, CAnimating);
DECLARE_DEFAULTHANDLER_void(CAnimating, DispatchAnimEvents,(CBaseEntity *eventHandler), (eventHandler));




DECLARE_DETOUR(StudioFrameAdvanceManual, CAnimating);
DECLARE_DEFAULTHANDLER_DETOUR_void(CAnimating, StudioFrameAdvanceManual, (float flInterval),(flInterval));


//DECLARE_DETOUR(GetModelPtr, CAnimating);
//DECLARE_DEFAULTHANDLER_DETOUR(CAnimating, GetModelPtr, CStudioHdr *, (),());

DECLARE_DETOUR(InvalidateBoneCache, CAnimating);
DECLARE_DEFAULTHANDLER_DETOUR_void(CAnimating, InvalidateBoneCache, (),());

DECLARE_DETOUR(Dissolve, CAnimating);
DECLARE_DEFAULTHANDLER_DETOUR(CAnimating, Dissolve, bool, (const char *pMaterialName, float flStartTime, bool bNPCOnly, int nDissolveType, Vector vDissolverOrigin, int iMagnitude), (pMaterialName, flStartTime, bNPCOnly, nDissolveType, vDissolverOrigin, iMagnitude));



//Sendprops
DEFINE_PROP(m_flPlaybackRate,CAnimating);
DEFINE_PROP(m_flPoseParameter,CAnimating);
DEFINE_PROP(m_nBody,CAnimating);
DEFINE_PROP(m_nSkin,CAnimating);
DEFINE_PROP(m_nNewSequenceParity,CAnimating);
DEFINE_PROP(m_nResetEventsParity,CAnimating);
DEFINE_PROP(m_nHitboxSet,CAnimating);
DEFINE_PROP(m_nMuzzleFlashParity,CAnimating);



//Datamaps
DEFINE_PROP(m_flCycle,CAnimating);
DEFINE_PROP(m_nSequence,CAnimating);
DEFINE_PROP(m_bSequenceLoops,CAnimating);
DEFINE_PROP(m_flGroundSpeed,CAnimating);
DEFINE_PROP(m_bSequenceFinished,CAnimating);
DEFINE_PROP(m_flLastEventCheck,CAnimating);
DEFINE_PROP(m_fBoneCacheFlags,CAnimating);



#define MAX_ANIMTIME_INTERVAL 0.2f

CAnimating::CAnimating()
{

}

// GetModelPtr got inlined so we use this little hack
CStudioHdr *CAnimating::GetModelPtr()
{
	CBaseEntity *pBaseEntity = BaseEntity();
	assert(pBaseEntity);

#if 0
	CStudioHdr *m_pStudioHdr = *reinterpret_cast<CStudioHdr**>(pBaseEntity + 0x450);
	if( !m_pStudioHdr && GetModel() )
	{
		g_helpfunc.LockStudioHdr(pBaseEntity);
	}
#endif
	return g_helpfunc.GetModelPtr(pBaseEntity);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : float
//-----------------------------------------------------------------------------
float CAnimating::GetAnimTimeInterval() const
{
	float flInterval;
	if (m_flAnimTime < gpGlobals->curtime)
	{
		// estimate what it'll be this frame
		flInterval = clamp( gpGlobals->curtime - *m_flAnimTime, 0, MAX_ANIMTIME_INTERVAL );
	}
	else
	{
		// report actual
		flInterval = clamp( *m_flAnimTime - *m_flPrevAnimTime, 0, MAX_ANIMTIME_INTERVAL );
	}
	return flInterval;
}



//-----------------------------------------------------------------------------
// Purpose:
// Output :
//-----------------------------------------------------------------------------
/*bool CAnimating::GetIntervalMovement( float flIntervalUsed, bool &bMoveSeqFinished, Vector &newPosition, QAngle &newAngles )
{
	CStudioHdr *pstudiohdr = GetModelPtr( );
	if (! pstudiohdr || !pstudiohdr->SequencesAvailable())
		return false;

	float flComputedCycleRate = GetSequenceCycleRate( GetSequence() );
	
	float flNextCycle = GetCycle() + flIntervalUsed * flComputedCycleRate * m_flPlaybackRate;

	if ((!m_bSequenceLoops) && flNextCycle > 1.0)
	{
		flIntervalUsed = GetCycle() / (flComputedCycleRate * m_flPlaybackRate);
		flNextCycle = 1.0;
		bMoveSeqFinished = true;
	}
	else
	{
		bMoveSeqFinished = false;
	}

	Vector deltaPos;
	QAngle deltaAngles;

	if (Studio_SeqMovement( pstudiohdr, GetSequence(), GetCycle(), flNextCycle, GetPoseParameterArray(), deltaPos, deltaAngles ))
	{
		VectorYawRotate( deltaPos, GetLocalAngles().y, deltaPos );
		newPosition = GetLocalOrigin() + deltaPos;
		newAngles.Init();
		newAngles.y = GetLocalAngles().y + deltaAngles.y;
		return true;
	}
	else
	{
		newPosition = GetLocalOrigin();
		newAngles = GetLocalAngles();
		return false;
	}
}*/

/**
float CAnimating::GetSequenceCycleRate( CStudioHdr *pStudioHdr, int iSequence )
{
	float t = SequenceDuration( pStudioHdr, iSequence );

	if (t > 0.0f)
	{
		return 1.0f / t;
	}
	else
	{
		return 1.0f / 0.1f;
	}
}

//=========================================================
//=========================================================
float CAnimating::SequenceDuration( CStudioHdr *pStudioHdr, int iSequence )
{
	if ( !pStudioHdr )
	{
		DevWarning( 2, "CBaseAnimating::SequenceDuration( %d ) NULL pstudiohdr on %s!\n", iSequence, GetClassname() );
		return 0.1;
	}
	if ( !pStudioHdr->SequencesAvailable() )
	{
		return 0.1;
	}
	if (iSequence >= pStudioHdr->GetNumSeq() || iSequence < 0 )
	{
		DevWarning( 2, "CBaseAnimating::SequenceDuration( %d ) out of range\n", iSequence );
		return 0.1;
	}

	return Studio_Duration( pStudioHdr, iSequence, GetPoseParameterArray() );
}
*/

//=========================================================
//=========================================================
bool CAnimating::HasPoseParameter( int iSequence, int iParameter )
{
	CStudioHdr *pstudiohdr = GetModelPtr( );

	if ( !pstudiohdr )
	{
		return false;
	}

	if ( !pstudiohdr->SequencesAvailable() )
	{
		return false;
	}

	if (iSequence < 0 || iSequence >= pstudiohdr->GetNumSeq())
	{
		return false;
	}

	mstudioseqdesc_t &seqdesc = pstudiohdr->pSeqdesc( iSequence );
	if (pstudiohdr->GetSharedPoseParameter( iSequence, seqdesc.paramindex[0] ) == iParameter || 
		pstudiohdr->GetSharedPoseParameter( iSequence, seqdesc.paramindex[1] ) == iParameter)
	{
		return true;
	}
	return false;
}


//=========================================================
//=========================================================
bool CAnimating::HasPoseParameter( int iSequence, const char *szName )
{
	int iParameter = LookupPoseParameter( szName );
	if (iParameter == -1)
	{
		return false;
	}

	return HasPoseParameter( iSequence, iParameter );
}

//-----------------------------------------------------------------------------
// Purpose: 
//
// Input  : iSequence - 
//
// Output : float - 
//-----------------------------------------------------------------------------
float CAnimating::GetSequenceMoveYaw( int iSequence )
{
	Vector				vecReturn;
	
	Assert( GetModelPtr() );
	::GetSequenceLinearMotion( GetModelPtr(), iSequence, GetPoseParameterArray(), &vecReturn );

	if (vecReturn.Length() > 0)
	{
		return UTIL_VecToYaw( vecReturn );
	}

	return NOMOTION;
}

//=========================================================
//=========================================================
int CAnimating::LookupPoseParameter( CStudioHdr *pStudioHdr, const char *szName )
{
	if ( !pStudioHdr )
		return 0;

	if ( !pStudioHdr->SequencesAvailable() )
	{
		return 0;
	}

	for (int i = 0; i < pStudioHdr->GetNumPoseParameters(); i++)
	{
		if (Q_stricmp( pStudioHdr->pPoseParameter( i ).pszName(), szName ) == 0)
		{
			return i;
		}
	}

	// AssertMsg( 0, UTIL_VarArgs( "poseparameter %s couldn't be mapped!!!\n", szName ) );
	return -1; // Error
}

//-----------------------------------------------------------------------------
// Make a model look as though it's burning. 
//-----------------------------------------------------------------------------
void CAnimating::Scorch( int rate, int floor )
{
	color32 color = GetRenderColor();

	if( color.r > floor )
		color.r -= rate;

	if( color.g > floor )
		color.g -= rate;

	if( color.b > floor )
		color.b -= rate;

	SetRenderColor( color.r, color.g, color.b );
}


//=========================================================
// SelectWeightedSequence
//=========================================================
int CAnimating::SelectWeightedSequence ( Activity activity )
{
	Assert( activity != ACT_INVALID );
	Assert( GetModelPtr() );
	return ::SelectWeightedSequence(GetModelPtr(), activity, GetSequence());
}


int CAnimating::SelectWeightedSequence ( Activity activity, int curSequence )
{
	Assert( activity != ACT_INVALID );
	Assert( GetModelPtr() );
	return ::SelectWeightedSequence( GetModelPtr(), activity, curSequence );
}

//=========================================================
//=========================================================
float CAnimating::SetPoseParameter( CStudioHdr *pStudioHdr, const char *szName, float flValue )
{
	int poseParam = LookupPoseParameter( pStudioHdr, szName );
	//AssertMsg2(poseParam >= 0, "SetPoseParameter called with invalid argument %s by %s", szName, GetDebugName());
	return SetPoseParameter( pStudioHdr, poseParam, flValue );
}

float CAnimating::SetPoseParameter( CStudioHdr *pStudioHdr, int iParameter, float flValue )
{
	if ( !pStudioHdr )
	{
		return flValue;
	}

	if (iParameter >= 0)
	{
		float flNewValue;
		flValue = Studio_SetPoseParameter( pStudioHdr, iParameter, flValue, flNewValue );

		float *ptr = (float *)((unsigned char *)m_flPoseParameter.ptr);
		ptr[iParameter] = flNewValue;
	}

	return flValue;
}

float CAnimating::GetPoseParameter( int iParameter )
{
	CStudioHdr *pstudiohdr = GetModelPtr( );

	if ( !pstudiohdr )
	{
		Assert(!"CBaseAnimating::GetPoseParameter: model missing");
		return 0.0;
	}

	if ( !pstudiohdr->SequencesAvailable() )
	{
		return 0;
	}

	if (iParameter >= 0)
	{
		return Studio_GetPoseParameter( pstudiohdr, iParameter,  GetPoseParameterArray()[iParameter]);
	}

	return 0.0;
}

//-----------------------------------------------------------------------------
// Purpose: Returns index number of a given named attachment
// Input  : name of attachment
// Output :	attachment index number or -1 if attachment not found
//-----------------------------------------------------------------------------
int CAnimating::LookupAttachment( const char *szName )
{
	CStudioHdr *pStudioHdr = GetModelPtr( );
	if (!pStudioHdr)
	{
		Assert(!"CBaseAnimating::LookupAttachment: model missing");
		return 0;
	}

	// The +1 is to make attachment indices be 1-based (namely 0 == invalid or unused attachment)
	return Studio_FindAttachment( pStudioHdr, szName ) + 1;
}

//-----------------------------------------------------------------------------
// Purpose: Returns the world location of an attachment
// Input  : attachment index
// Output :	location and angles
//-----------------------------------------------------------------------------
bool CAnimating::GetAttachment( const char *szName, Vector &absOrigin, Vector *forward, Vector *right, Vector *up )
{																
	return GetAttachment( LookupAttachment( szName ), absOrigin, forward, right, up );
}

bool CAnimating::GetAttachment( int iAttachment, Vector &absOrigin, Vector *forward, Vector *right, Vector *up )
{
	matrix3x4_t attachmentToWorld;

	bool bRet = GetAttachment( iAttachment, attachmentToWorld );
	MatrixPosition( attachmentToWorld, absOrigin );
	if (forward)
	{
		MatrixGetColumn( attachmentToWorld, 0, forward );
	}
	if (right)
	{
		MatrixGetColumn( attachmentToWorld, 1, right );
	}
	if (up)
	{
		MatrixGetColumn( attachmentToWorld, 2, up );
	}
	return bRet;
}


//-----------------------------------------------------------------------------
// Purpose: Returns index number of a given named bone
// Input  : name of a bone
// Output :	Bone index number or -1 if bone not found
//-----------------------------------------------------------------------------
int CAnimating::LookupBone( const char *szName )
{
	Assert( GetModelPtr() );

	return Studio_BoneIndexByName( GetModelPtr(), szName );
}

//=========================================================
//=========================================================
void CAnimating::GetBonePosition ( int iBone, Vector &origin, QAngle &angles )
{
	CStudioHdr *pStudioHdr = GetModelPtr( );
	if (!pStudioHdr)
	{
		Assert(!"CBaseAnimating::GetBonePosition: model missing");
		return;
	}

	if (iBone < 0 || iBone >= pStudioHdr->numbones())
	{
		Assert(!"CBaseAnimating::GetBonePosition: invalid bone index");
		return;
	}

	matrix3x4_t bonetoworld;
	GetBoneTransform( iBone, bonetoworld );
	
	MatrixAngles( bonetoworld, angles, origin );
}



//=========================================================
//=========================================================

void CAnimating::GetBoneTransform( int iBone, matrix3x4_t &pBoneToWorld )
{
	CStudioHdr *pStudioHdr = GetModelPtr( );

	if (!pStudioHdr)
	{
		Assert(!"CBaseAnimating::GetBoneTransform: model missing");
		return;
	}

	if (iBone < 0 || iBone >= pStudioHdr->numbones())
	{
		Assert(!"CBaseAnimating::GetBoneTransform: invalid bone index");
		return;
	}

	CBoneCache *pcache = GetBoneCache( );

	matrix3x4_t *pmatrix = pcache->GetCachedBone( iBone );

	if ( !pmatrix )
	{
		MatrixCopy( EntityToWorldTransform(), pBoneToWorld );
		return;
	}

	Assert( pmatrix );
	
	// FIXME
	MatrixCopy( *pmatrix, pBoneToWorld );
}


//-----------------------------------------------------------------------------
// Purpose: return the index to the shared bone cache
// Output :
//-----------------------------------------------------------------------------
CBoneCache *CAnimating::GetBoneCache( void )
{
	return g_helpfunc.GetBoneCache(BaseEntity());
}

//-----------------------------------------------------------------------------
// Purpose: 
//
// Input  : iSequence - 
//
// Output : char
//-----------------------------------------------------------------------------
const char *CAnimating::GetSequenceName( int iSequence )
{
	if( iSequence == -1 )
	{
		return "Not Found!";
	}

	if ( !GetModelPtr() )
		return "No model!";

	return ::GetSequenceName( GetModelPtr(), iSequence );
}


//=========================================================
//=========================================================

void CAnimating::SetBodygroup( int iGroup, int iValue )
{
	Assert( GetModelPtr() );

	int newBody = m_nBody;
	::SetBodygroup( GetModelPtr( ), newBody, iGroup, iValue );
	m_nBody = newBody;
}


//-----------------------------------------------------------------------------
// Purpose: find frame where they animation has moved a given distance.
// Output :
//-----------------------------------------------------------------------------
float CAnimating::GetMovementFrame( float flDist )
{
	CStudioHdr *pstudiohdr = GetModelPtr( );
	if (! pstudiohdr)
		return 0;

	float t = Studio_FindSeqDistance( pstudiohdr, GetSequence(), GetPoseParameterArray(), flDist );

	return t;
}

//-----------------------------------------------------------------------------
// Purpose:
// Output :
//-----------------------------------------------------------------------------
float CAnimating::GetInstantaneousVelocity( float flInterval )
{
	CStudioHdr *pstudiohdr = GetModelPtr( );
	if (! pstudiohdr)
		return 0;

	// FIXME: someone needs to check for last frame, etc.
	float flNextCycle = GetCycle() + flInterval * GetSequenceCycleRate( GetSequence() ) * m_flPlaybackRate;

	Vector vecVelocity;
	Studio_SeqVelocity( pstudiohdr, GetSequence(), flNextCycle, GetPoseParameterArray(), vecVelocity );
	vecVelocity *= m_flPlaybackRate;

	return vecVelocity.Length();
}


float CAnimating::GetSequenceCycleRate( CStudioHdr *pStudioHdr, int iSequence )
{
	float t = SequenceDuration( pStudioHdr, iSequence );

	if (t > 0.0f)
	{
		return 1.0f / t;
	}
	else
	{
		return 1.0f / 0.1f;
	}
}


//=========================================================
//=========================================================
float CAnimating::SequenceDuration( CStudioHdr *pStudioHdr, int iSequence )
{
	if ( !pStudioHdr )
	{
		DevWarning( 2, "CBaseAnimating::SequenceDuration( %d ) NULL pstudiohdr on %s!\n", iSequence, GetClassname() );
		return 0.1;
	}
	if ( !pStudioHdr->SequencesAvailable() )
	{
		return 0.1;
	}
	if (iSequence >= pStudioHdr->GetNumSeq() || iSequence < 0 )
	{
		DevWarning( 2, "CBaseAnimating::SequenceDuration( %d ) out of range\n", iSequence );
		return 0.1;
	}

	return Studio_Duration( pStudioHdr, iSequence, GetPoseParameterArray() );
}


//-----------------------------------------------------------------------------
// Purpose: 
//
// Input  : iSequence - 
//			*pVec - 
//
//-----------------------------------------------------------------------------
void CAnimating::GetSequenceLinearMotion( int iSequence, Vector *pVec )
{
	Assert( GetModelPtr() );
	::GetSequenceLinearMotion( GetModelPtr(), iSequence, GetPoseParameterArray(), pVec );
}


//=========================================================
//=========================================================
int CAnimating::FindTransitionSequence( int iCurrentSequence, int iGoalSequence, int *piDir )
{
	Assert( GetModelPtr() );

	if (piDir == NULL)
	{
		int iDir = 1;
		int sequence = ::FindTransitionSequence( GetModelPtr(), iCurrentSequence, iGoalSequence, &iDir );
		if (iDir != 1)
			return -1;
		else
			return sequence;
	}

	return ::FindTransitionSequence( GetModelPtr(), iCurrentSequence, iGoalSequence, piDir );
}

//-----------------------------------------------------------------------------
// Purpose:
// Input  :
// Output :
//-----------------------------------------------------------------------------
KeyValues *CAnimating::GetSequenceKeyValues( int iSequence )
{
	const char *szText = Studio_GetKeyValueText( GetModelPtr(), iSequence );

	if (szText)
	{
		KeyValues *seqKeyValues = new KeyValues("");
		if ( seqKeyValues->LoadFromBuffer( modelinfo->GetModelName( GetModel() ), szText ) )
		{
			return seqKeyValues;
		}
		seqKeyValues->deleteThis();
	}
	return NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Looks up an activity by name.
// Input  : label - Name of the activity, ie "ACT_IDLE".
// Output : Returns the activity ID or ACT_INVALID.
//-----------------------------------------------------------------------------
int CAnimating::LookupActivity( const char *label )
{
	Assert( GetModelPtr() );
	return ::LookupActivity( GetModelPtr(), label );
}

//=========================================================
//=========================================================
int CAnimating::LookupSequence( const char *label )
{
	Assert( GetModelPtr() );
	return ::LookupSequence( GetModelPtr(), label );
}

//-----------------------------------------------------------------------------
// Purpose:
// Output :
//-----------------------------------------------------------------------------
float CAnimating::GetEntryVelocity( int iSequence )
{
	CStudioHdr *pstudiohdr = GetModelPtr( );
	if (! pstudiohdr)
		return 0;

	Vector vecVelocity;
	Studio_SeqVelocity( pstudiohdr, iSequence, 0.0, GetPoseParameterArray(), vecVelocity );

	return vecVelocity.Length();
}

int CAnimating::GetBodygroup( int iGroup )
{
	return ::GetBodygroup( GetModelPtr( ), m_nBody, iGroup );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the world location and world angles of an attachment
// Input  : attachment name
// Output :	location and angles
//-----------------------------------------------------------------------------
bool CAnimating::GetAttachment( const char *szName, Vector &absOrigin, QAngle &absAngles )
{																
	return GetAttachment( LookupAttachment( szName ), absOrigin, absAngles );
}

//-----------------------------------------------------------------------------
// Purpose: Returns the world location and world angles of an attachment
// Input  : attachment index
// Output :	location and angles
//-----------------------------------------------------------------------------
bool CAnimating::GetAttachment ( int iAttachment, Vector &absOrigin, QAngle &absAngles )
{
	matrix3x4_t attachmentToWorld;

	bool bRet = GetAttachment( iAttachment, attachmentToWorld );
	MatrixAngles( attachmentToWorld, absAngles, absOrigin );
	return bRet;
}

void CAnimating::SetSequence( int nSequence )
{
	Assert( GetModelPtr( ) && ( nSequence < GetModelPtr( )->GetNumSeq() ) && ( GetModelPtr( )->GetNumSeq() < (1 << ANIMATION_SEQUENCE_BITS) ) );
	m_nSequence = nSequence;
}

bool CAnimating::ComputeHitboxSurroundingBox( Vector *pVecWorldMins, Vector *pVecWorldMaxs )
{
	// Note that this currently should not be called during Relink because of IK.
	// The code below recomputes bones so as to get at the hitboxes,
	// which causes IK to trigger, which causes raycasts against the other entities to occur,
	// which is illegal to do while in the Relink phase.

	CStudioHdr *pStudioHdr = GetModelPtr();
	if (!pStudioHdr)
		return false;

	mstudiohitboxset_t *set = pStudioHdr->pHitboxSet( m_nHitboxSet );
	if ( !set || !set->numhitboxes )
		return false;

	CBoneCache *pCache = GetBoneCache();

	// Compute a box in world space that surrounds this entity
	pVecWorldMins->Init( FLT_MAX, FLT_MAX, FLT_MAX );
	pVecWorldMaxs->Init( -FLT_MAX, -FLT_MAX, -FLT_MAX );

	Vector vecBoxAbsMins, vecBoxAbsMaxs;
	for ( int i = 0; i < set->numhitboxes; i++ )
	{
		mstudiobbox_t *pbox = set->pHitbox(i);
		matrix3x4_t *pMatrix = pCache->GetCachedBone(pbox->bone);

		if ( pMatrix )
		{
			TransformAABB( *pMatrix, pbox->bbmin, pbox->bbmax, vecBoxAbsMins, vecBoxAbsMaxs );
			VectorMin( *pVecWorldMins, vecBoxAbsMins, *pVecWorldMins );
			VectorMax( *pVecWorldMaxs, vecBoxAbsMaxs, *pVecWorldMaxs );
		}
	}
	return true;
}


void CAnimating::DoMuzzleFlash()
{
	m_nMuzzleFlashParity = (*(m_nMuzzleFlashParity)+1) & ((1 << EF_MUZZLEFLASH_BITS) - 1);
}

void CAnimating::ResetSequence(int nSequence)
{
	if ( !SequenceLoops() )
	{
		SetCycle( 0 );
	}

	// Tracker 17868:  If the sequence number didn't actually change, but you call resetsequence info, it changes
	//  the newsequenceparity bit which causes the client to call m_flCycle.Reset() which causes a very slight 
	//  discontinuity in looping animations as they reset around to cycle 0.0.  This was causing the parentattached
	//  helmet on barney to hitch every time barney's idle cycled back around to its start.
	bool changed = nSequence != GetSequence() ? true : false;

	SetSequence( nSequence );
	if ( changed || !SequenceLoops() )
	{
		ResetSequenceInfo();
	}
}

void CAnimating::ResetSequenceInfo()
{
	if (GetSequence() == -1)
	{
		// This shouldn't happen.  Setting m_nSequence blindly is a horrible coding practice.
		SetSequence( 0 );
	}

	CStudioHdr *pStudioHdr = GetModelPtr();
	m_flGroundSpeed = GetSequenceGroundSpeed( pStudioHdr, GetSequence() );
	m_bSequenceLoops = ((GetSequenceFlags( pStudioHdr, GetSequence() ) & STUDIO_LOOPING) != 0);
	// m_flAnimTime = gpGlobals->time;
	m_flPlaybackRate = 1.0;
	m_bSequenceFinished = false;
	m_flLastEventCheck = 0;

	m_nNewSequenceParity = ( *(m_nNewSequenceParity)+1 ) & EF_PARITY_MASK;
	m_nResetEventsParity = ( *(m_nResetEventsParity)+1 ) & EF_PARITY_MASK;

	// FIXME: why is this called here?  Nothing should have changed to make this nessesary
	if ( pStudioHdr )
	{
		SetEventIndexForSequence( pStudioHdr->pSeqdesc( GetSequence() ) );
	}
}


LocalFlexController_t CAnimating::GetNumFlexControllers( void )
{
	CStudioHdr *pstudiohdr = GetModelPtr( );
	if (! pstudiohdr)
		return LocalFlexController_t(0);

	return pstudiohdr->numflexcontrollers();
}

const char *CAnimating::GetFlexControllerName( LocalFlexController_t iFlexController )
{
	CStudioHdr *pstudiohdr = GetModelPtr( );
	if (! pstudiohdr)
		return 0;

	mstudioflexcontroller_t *pflexcontroller = pstudiohdr->pFlexcontroller( iFlexController );

	return pflexcontroller->pszName( );
}

float CAnimating::EdgeLimitPoseParameter( int iParameter, float flValue, float flBase )
{
	CStudioHdr *pstudiohdr = GetModelPtr( );
	if ( !pstudiohdr )
	{
		return flValue;
	}

	if (iParameter < 0 || iParameter >= pstudiohdr->GetNumPoseParameters())
	{
		return flValue;
	}

	const mstudioposeparamdesc_t &Pose = pstudiohdr->pPoseParameter( iParameter );

	if (Pose.loop || Pose.start == Pose.end)
	{
		return flValue;
	}

	return RangeCompressor( flValue, Pose.start, Pose.end, flBase );
}

int CAnimating::GetHitboxSet()
{
	static int offs = -1;
	if(offs == -1)
	{
		if(!g_pGameConf->GetOffset("m_nHitboxSet", &offs))
		{
			assert(0);
			return 0;
		}
	}

	return *reinterpret_cast<int*>((unsigned long)BaseEntity() + offs);
}