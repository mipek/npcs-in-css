
#ifndef _INCLUDE_CCBONEFOLOWER_H_
#define _INCLUDE_CCBONEFOLOWER_H_

#include "CEntity.h"
#include "CAnimating.h"



class CE_CBoneFollower : public CEntity
{
public:
	CE_DECLARE_CLASS( CE_CBoneFollower, CEntity );

	bool Init( CEntity *pOwner, const char *pModelName, solid_t &solid, const Vector &position, const QAngle &orientation );
	void SetTraceData( int physicsBone, int hitGroup );
	void UpdateFollower( const Vector &position, const QAngle &orientation, float flInterval );

public:
	static CE_CBoneFollower *Create( CEntity *pOwner, const char *pModelName, solid_t &solid, const Vector &position, const QAngle &orientation );

protected: //Sendprop
	DECLARE_SENDPROP(int, m_modelIndex);
	DECLARE_SENDPROP(int, m_solidIndex);

protected: //Datamaps
	DECLARE_DATAMAP(int, m_hitGroup);
	DECLARE_DATAMAP(int, m_physicsBone);

};



struct physfollower_t
{
	DECLARE_SIMPLE_DATADESC();
	int boneIndex;
	CEFakeHandle<CE_CBoneFollower> hFollower;
};

struct vcollide_t;

// create a manager and a list of followers directly from a ragdoll
void CreateBoneFollowersFromRagdoll( CAnimating *pEntity, class CBoneFollowerManager *pManager, vcollide_t *pCollide );



class CBoneFollowerManager
{
	DECLARE_SIMPLE_DATADESC();
public:
	CBoneFollowerManager();
	~CBoneFollowerManager();

	// Use either of these to create the bone followers in your entity's CreateVPhysics()
	void InitBoneFollowers( CAnimating *pParentEntity, int iNumBones, const char **pFollowerBoneNames );
	void AddBoneFollower( CAnimating *pParentEntity, const char *pFollowerBoneName, solid_t *pSolid = NULL );	// Adds a single bone follower

	// Call this after you move your bones
	void UpdateBoneFollowers( CAnimating *pParentEntity );

	// Call this when your entity's removed
	void DestroyBoneFollowers( void );

	physfollower_t *GetBoneFollower( int iFollowerIndex );
	int				GetBoneFollowerIndex( CE_CBoneFollower *pFollower );
	int				GetNumBoneFollowers( void ) const { return m_iNumBones; }

private:
	bool CreatePhysicsFollower( CAnimating *pParentEntity, physfollower_t &follow, const char *pBoneName, solid_t *pSolid );

private:
	int							m_iNumBones;
	CUtlVector<physfollower_t>	m_physBones;
};


#endif