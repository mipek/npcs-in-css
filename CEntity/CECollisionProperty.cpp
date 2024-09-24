
#include "CECollisionProperty.h"
#include "CAnimating.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



CECollisionProperty::CECollisionProperty(CEntity *centity)
{
	this->centity = centity;
}

void CECollisionProperty::SetSolidFlags(int flags)
{
	g_helpfunc.SetSolidFlags(centity->CollisionProp_Actual(), flags);
}

void CECollisionProperty::SetSolid(SolidType_t val)
{
	g_helpfunc.SetSolid(centity->CollisionProp_Actual(), val);
}

void CECollisionProperty::AddSolidFlags(int flags)
{
	g_helpfunc.SetSolidFlags(centity->CollisionProp_Actual(), (centity->m_usSolidFlags | flags));
}

void CECollisionProperty::RemoveSolidFlags(int flags)
{
	g_helpfunc.SetSolidFlags(centity->CollisionProp_Actual(), (centity->m_usSolidFlags & ~flags));
}

bool CECollisionProperty::IsSolid()
{
	return ::IsSolid( (SolidType_t)(unsigned char)centity->m_nSolidType, centity->m_usSolidFlags );

}

const Vector& CECollisionProperty::GetCollisionOrigin() const
{
	return centity->CollisionProp_Actual()->GetCollisionOrigin();
}

const QAngle& CECollisionProperty::GetCollisionAngles() const
{
	return centity->CollisionProp_Actual()->GetCollisionAngles();
}

const matrix3x4_t&	CECollisionProperty::CollisionToWorldTransform() const
{
	return centity->CollisionProp_Actual()->CollisionToWorldTransform();
}

//-----------------------------------------------------------------------------
// Computes a "normalized" point (range 0,0,0 - 1,1,1) in collision space
//-----------------------------------------------------------------------------
const Vector & CECollisionProperty::NormalizedToCollisionSpace( const Vector &in, Vector *pResult ) const
{
	pResult->x = Lerp( in.x, centity->m_vecMins.ptr->x, centity->m_vecMaxs.ptr->x );
	pResult->y = Lerp( in.y, centity->m_vecMins.ptr->y, centity->m_vecMaxs.ptr->y );
	pResult->z = Lerp( in.z, centity->m_vecMins.ptr->z, centity->m_vecMaxs.ptr->z );
	return *pResult;
}


//-----------------------------------------------------------------------------
// Computes a "normalized" point (range 0,0,0 - 1,1,1) in world space
//-----------------------------------------------------------------------------
const Vector & CECollisionProperty::NormalizedToWorldSpace( const Vector &in, Vector *pResult ) const
{
	Vector vecCollisionSpace;
	NormalizedToCollisionSpace( in, &vecCollisionSpace );
	CollisionToWorldSpace( vecCollisionSpace, pResult );
	return *pResult;
}


//-----------------------------------------------------------------------------
// Selects a random point in the bounds given the normalized 0-1 bounds 
//-----------------------------------------------------------------------------
void CECollisionProperty::RandomPointInBounds( const Vector &vecNormalizedMins, const Vector &vecNormalizedMaxs, Vector *pPoint) const
{
	Vector vecNormalizedSpace;
	vecNormalizedSpace.x = enginerandom->RandomFloat( vecNormalizedMins.x, vecNormalizedMaxs.x );
	vecNormalizedSpace.y = enginerandom->RandomFloat( vecNormalizedMins.y, vecNormalizedMaxs.y );
	vecNormalizedSpace.z = enginerandom->RandomFloat( vecNormalizedMins.z, vecNormalizedMaxs.z );
	NormalizedToWorldSpace( vecNormalizedSpace, pPoint );
}



//-----------------------------------------------------------------------------
// Methods relating to solid flags
//-----------------------------------------------------------------------------
bool CECollisionProperty::IsBoundsDefinedInEntitySpace() const
{
	return (( centity->m_usSolidFlags & FSOLID_FORCE_WORLD_ALIGNED ) == 0 ) &&
			( centity->m_nSolidType != SOLID_BBOX ) && ( centity->m_nSolidType != SOLID_NONE );
}

//-----------------------------------------------------------------------------
// Transforms a point in OBB space to world space
//-----------------------------------------------------------------------------
const Vector &CECollisionProperty::CollisionToWorldSpace( const Vector &in, Vector *pResult ) const 
{
	// Makes sure we don't re-use the same temp twice
	if ( !IsBoundsDefinedInEntitySpace() || ( GetCollisionAngles() == vec3_angle ) )
	{
		VectorAdd( in, GetCollisionOrigin(), *pResult );
	}
	else
	{
		VectorTransform( in, CollisionToWorldTransform(), *pResult );
	}
	return *pResult;
}

void CECollisionProperty::MarkSurroundingBoundsDirty()
{
	centity->AddEFlags( EFL_DIRTY_SURROUNDING_COLLISION_BOUNDS );
	MarkPartitionHandleDirty();
	centity->NetworkProp()->MarkPVSInformationDirty();
}

void CECollisionProperty::MarkPartitionHandleDirty()
{
	g_helpfunc.MarkPartitionHandleDirty(centity->CollisionProp_Actual());
}

void CECollisionProperty::SetSurroundingBoundsType( SurroundingBoundsType_t type, const Vector *pMins, const Vector *pMaxs )
{
	CCollisionProperty *collsion = centity->CollisionProp_Actual();

	centity->m_nSurroundType = type;
	if (type != USE_SPECIFIED_BOUNDS)
	{
		Assert( !pMins && !pMaxs );
		MarkSurroundingBoundsDirty();
	}
	else
	{
		Assert( pMins && pMaxs );
		centity->m_vecSpecifiedSurroundingMinsPreScaled = *pMins;
		centity->m_vecSpecifiedSurroundingMaxsPreScaled = *pMaxs;

		// Check if it's a scaled model
		CAnimating *pAnim = centity->GetBaseAnimating();
		if ( pAnim && pAnim->GetModelScale() != 1.0f )
		{
			// Do the scaling
			Vector vecNewMins = *pMins * pAnim->GetModelScale();
			Vector vecNewMaxs = *pMaxs * pAnim->GetModelScale();

			centity->m_vecSpecifiedSurroundingMins = vecNewMins;
			centity->m_vecSpecifiedSurroundingMaxs = vecNewMaxs;
			collsion->m_vecSurroundingMins = vecNewMins;
			collsion->m_vecSurroundingMaxs = vecNewMaxs;

		}
		else
		{
			// No scaling needed!
			centity->m_vecSpecifiedSurroundingMins = *pMins;
			centity->m_vecSpecifiedSurroundingMaxs = *pMaxs;
			collsion->m_vecSurroundingMins = *pMins;
			collsion->m_vecSurroundingMaxs = *pMaxs;

		}

		ASSERT_COORD( (collsion->m_vecSurroundingMins) );
		ASSERT_COORD( (collsion->m_vecSurroundingMaxs) );
	}
}

float CECollisionProperty::BoundingRadius() const
{
	return centity->m_flRadius;
}

float CECollisionProperty::CalcDistanceFromPoint( const Vector &vecWorldPt ) const
{
	// Calculate physics force
	Vector localPt, localClosestPt;
	centity->CollisionProp_Actual()->WorldToCollisionSpace( vecWorldPt, &localPt );
	CalcClosestPointOnAABB( centity->m_vecMins, centity->m_vecMaxs, localPt, localClosestPt );
	return localPt.DistTo( localClosestPt );
}

void CECollisionProperty::WorldSpaceAABB( Vector *pWorldMins, Vector *pWorldMaxs ) const
{
	CollisionAABBToWorldAABB( centity->m_vecMins, centity->m_vecMaxs, pWorldMins, pWorldMaxs );
}

void CECollisionProperty::CollisionAABBToWorldAABB( const Vector &entityMins, 
	const Vector &entityMaxs, Vector *pWorldMins, Vector *pWorldMaxs ) const
{
	if ( !IsBoundsDefinedInEntitySpace() || (GetCollisionAngles() == vec3_angle) )
	{
		VectorAdd( entityMins, GetCollisionOrigin(), *pWorldMins );
		VectorAdd( entityMaxs, GetCollisionOrigin(), *pWorldMaxs );
	}
	else
	{
		TransformAABB( CollisionToWorldTransform(), entityMins, entityMaxs, *pWorldMins, *pWorldMaxs );
	}
}

const Vector &CECollisionProperty::WorldToCollisionSpace( const Vector &in, Vector *pResult ) const
{
	if ( !IsBoundsDefinedInEntitySpace() || ( GetCollisionAngles() == vec3_angle ) )
	{
		VectorSubtract( in, GetCollisionOrigin(), *pResult );
	}
	else
	{
		VectorITransform( in, CollisionToWorldTransform(), *pResult );
	}
	return *pResult;
}


void CECollisionProperty::CalcNearestPoint( const Vector &vecWorldPt, Vector *pVecNearestWorldPt ) const
{
	// Calculate physics force
	Vector localPt, localClosestPt;
	WorldToCollisionSpace( vecWorldPt, &localPt );
	CalcClosestPointOnAABB( centity->m_vecMins, centity->m_vecMaxs, localPt, localClosestPt );
	CollisionToWorldSpace( localClosestPt, pVecNearestWorldPt );
}

void CECollisionProperty::SetCollisionBounds( const Vector& mins, const Vector &maxs )
{
	if ( ( *(centity->m_vecMinsPreScaled) != mins ) || ( *(centity->m_vecMaxsPreScaled) != maxs ) )
	{
		centity->m_vecMinsPreScaled = mins;
		centity->m_vecMaxsPreScaled = maxs;
	}

	bool bDirty = false;

	// Check if it's a scaled model
	CAnimating *pAnim = centity->GetBaseAnimating();
	if ( pAnim && pAnim->GetModelScale() != 1.0f )
	{
		// Do the scaling
		Vector vecNewMins = mins * pAnim->GetModelScale();
		Vector vecNewMaxs = maxs * pAnim->GetModelScale();

		if ( ( *(centity->m_vecMins) != vecNewMins ) || ( *(centity->m_vecMaxs) != vecNewMaxs ) )
		{
			centity->m_vecMins = vecNewMins;
			centity->m_vecMaxs = vecNewMaxs;
			bDirty = true;
		}
	}
	else
	{
		// No scaling needed!
		if ( ( *(centity->m_vecMins) != mins ) || ( *(centity->m_vecMaxs) != maxs ) )
		{
			centity->m_vecMins = mins;
			centity->m_vecMaxs = maxs;
			bDirty = true;
		}
	}

	if ( bDirty )
	{
		//ASSERT_COORD( m_vecMins.Get() );
		//ASSERT_COORD( m_vecMaxs.Get() );

		Vector vecSize;
		VectorSubtract( centity->m_vecMaxs, centity->m_vecMins, vecSize );
		CCollisionProperty *collsion = centity->CollisionProp_Actual();
		collsion->m_flRadius = vecSize.Length() * 0.5f;

		MarkSurroundingBoundsDirty();
	}
}

void CECollisionProperty::UseTriggerBounds( bool bEnable, float flBloat )
{
	Assert( flBloat <= 127.0f );
	centity->m_triggerBloat = (char )flBloat;
	if ( bEnable )
	{
		centity->AddSolidFlags( FSOLID_USE_TRIGGER_BOUNDS );
		Assert( flBloat > 0.0f );
	}
	else
	{
		centity->RemoveSolidFlags( FSOLID_USE_TRIGGER_BOUNDS );
	}
}

bool CECollisionProperty::IsSolidFlagSet( int flagMask ) const
{
	return (centity->m_usSolidFlags & flagMask) != 0;
}

const Vector & CECollisionProperty::WorldDirectionToCollisionSpace( const Vector &in, Vector *pResult ) const
{
	if ( !IsBoundsDefinedInEntitySpace() || ( GetCollisionAngles() == vec3_angle ) )
	{
		*pResult = in;
	}
	else
	{
		VectorIRotate( in, CollisionToWorldTransform(), *pResult );
	}
	return *pResult;
}

bool CECollisionProperty::IsPointInBounds( const Vector &vecWorldPt ) const
{
	Vector vecLocalSpace;
	WorldToCollisionSpace( vecWorldPt, &vecLocalSpace );
	return ( ( vecLocalSpace.x >= centity->m_vecMins->x && vecLocalSpace.x <=centity->m_vecMaxs->x ) &&
			 ( vecLocalSpace.y >= centity->m_vecMins->y && vecLocalSpace.y <= centity->m_vecMaxs->y ) &&
			 ( vecLocalSpace.z >= centity->m_vecMins->z && vecLocalSpace.z <= centity->m_vecMaxs->z ) );
}

const Vector & CECollisionProperty::WorldToNormalizedSpace( const Vector &in, Vector *pResult ) const
{
	Vector vecCollisionSpace;
	WorldToCollisionSpace( in, &vecCollisionSpace );
	CollisionToNormalizedSpace( vecCollisionSpace, pResult );
	return *pResult;
}

const Vector &	CECollisionProperty::CollisionToNormalizedSpace( const Vector &in, Vector *pResult ) const
{
	Vector vecSize = centity->CollisionProp_Actual()->OBBSize( );
	Vector m_vecMins = centity->m_vecMins;
	pResult->x = ( vecSize.x != 0.0f ) ? ( in.x - m_vecMins.x ) / vecSize.x : 0.5f;
	pResult->y = ( vecSize.y != 0.0f ) ? ( in.y - m_vecMins.y ) / vecSize.y : 0.5f;
	pResult->z = ( vecSize.z != 0.0f ) ? ( in.z - m_vecMins.z ) / vecSize.z : 0.5f;
	return *pResult;
}

const Vector& CECollisionProperty::OBBMins( ) const
{
	return centity->m_vecMins;
}

const Vector& CECollisionProperty::OBBMaxs( ) const
{
	return centity->m_vecMaxs;
}