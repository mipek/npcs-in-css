
#ifndef _INCLUDE_CECOLLISIONPROPERTY_H_
#define _INCLUDE_CECOLLISIONPROPERTY_H_

#include "extension.h"
#include "CEntity.h"
#include "collisionproperty.h"


class CECollisionProperty
{
public:
	CECollisionProperty(CEntity *centity);

	void				SetSolidFlags(int flags);
	void				SetSolid(SolidType_t val);
	void				AddSolidFlags(int flags);
	void				RemoveSolidFlags(int flags);
	bool				IsSolid();
	const Vector&		GetCollisionOrigin() const;
	const QAngle&		GetCollisionAngles() const;
	const matrix3x4_t&	CollisionToWorldTransform() const;
	void				RandomPointInBounds( const Vector &vecNormalizedMins, const Vector &vecNormalizedMaxs, Vector *pPoint) const;
	const Vector &		NormalizedToWorldSpace( const Vector &in, Vector *pResult ) const;
	const Vector &		NormalizedToCollisionSpace( const Vector &in, Vector *pResult ) const;
	const Vector &		CollisionToWorldSpace( const Vector &in, Vector *pResult ) const;
	bool				IsBoundsDefinedInEntitySpace() const;
	void				MarkSurroundingBoundsDirty();
	void				MarkPartitionHandleDirty();
	void				SetSurroundingBoundsType( SurroundingBoundsType_t type, const Vector *pMins = NULL, const Vector *pMaxs = NULL );
	float				BoundingRadius() const;
	float				CalcDistanceFromPoint( const Vector &vecWorldPt ) const;
	void				WorldSpaceAABB( Vector *pWorldMins, Vector *pWorldMaxs ) const;
	void				SetCollisionBounds( const Vector& mins, const Vector &maxs );
	void				UseTriggerBounds( bool bEnable, float flBloat = 0.0f );
	bool				IsSolidFlagSet( int flagMask ) const;
	void				CalcNearestPoint( const Vector &vecWorldPt, Vector *pVecNearestWorldPt ) const;
	const Vector &		WorldToCollisionSpace( const Vector &in, Vector *pResult ) const;
	const Vector&	OBBMins( ) const;
	const Vector&	OBBMaxs( ) const;
	inline const Vector&	OBBSize( ) const
	{
		Vector &temp = AllocTempVector();
		VectorSubtract( OBBMaxs(), OBBMins(), temp );
		return temp;
	}

private:
	void				CollisionAABBToWorldAABB( const Vector &entityMins, const Vector &entityMaxs, Vector *pWorldMins, Vector *pWorldMaxs ) const;

protected:
	CEntity *centity;

protected: //Sendprops
};



#endif

