
#include "CECollisionProperty.h"


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
	centity->m_nSurroundType = type;
	if (type != USE_SPECIFIED_BOUNDS)
	{
		//Assert( !pMins && !pMaxs );
		MarkSurroundingBoundsDirty();
	}
	else
	{
		//Assert( pMins && pMaxs );
		centity->m_vecSpecifiedSurroundingMins = *pMins;
		centity->m_vecSpecifiedSurroundingMaxs = *pMaxs;
		centity->m_vecSurroundingMins = *pMins;
		centity->m_vecSurroundingMaxs = *pMaxs;

		//ASSERT_COORD( m_vecSurroundingMins );
		//ASSERT_COORD( m_vecSurroundingMaxs );
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
	if ( (*(centity->m_vecMins.ptr) == mins) && (*(centity->m_vecMaxs.ptr) == maxs) )
		return;
		
	centity->m_vecMins = mins;
	centity->m_vecMaxs = maxs;
	
	//ASSERT_COORD( mins );
	//ASSERT_COORD( maxs );

	Vector vecSize;
	VectorSubtract( maxs, mins, vecSize );
	centity->m_flRadius = vecSize.Length() * 0.5f;

	MarkSurroundingBoundsDirty();
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

const Vector& CECollisionProperty::OBBMins( ) const
{
	return centity->m_vecMins;
}

const Vector& CECollisionProperty::OBBMaxs( ) const
{
	return centity->m_vecMaxs;
}