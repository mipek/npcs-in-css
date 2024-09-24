
#include "CBoneFollower.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


CE_LINK_ENTITY_TO_CLASS(CBoneFollower, CE_CBoneFollower);

//Sendprops
DEFINE_PROP(m_modelIndex, CE_CBoneFollower);
DEFINE_PROP(m_solidIndex, CE_CBoneFollower);


//Datamaps
DEFINE_PROP(m_hitGroup, CE_CBoneFollower);
DEFINE_PROP(m_physicsBone, CE_CBoneFollower);



BEGIN_SIMPLE_DATADESC( physfollower_t )
					DEFINE_FIELD( boneIndex,			FIELD_INTEGER	),
					DEFINE_FIELD( hFollower,			FIELD_EHANDLE	),
END_DATADESC()

BEGIN_SIMPLE_DATADESC( CBoneFollowerManager )
					DEFINE_GLOBAL_FIELD( m_iNumBones,			FIELD_INTEGER	),
END_DATADESC()


static int HitGroupFromPhysicsBone( CAnimating *pAnim, int physicsBone )
{
	CStudioHdr *pStudioHdr = pAnim->GetModelPtr( );
	mstudiohitboxset_t *set = pStudioHdr->pHitboxSet( pAnim->m_nHitboxSet );
	for ( int i = 0; i < set->numhitboxes; i++ )
	{
		if ( pStudioHdr->pBone( set->pHitbox(i)->bone )->physicsbone == physicsBone )
		{
			return set->pHitbox(i)->group;
		}
	}

	return 0;
}


void CE_CBoneFollower::SetTraceData( int physicsBone, int hitGroup )
{
	m_hitGroup = hitGroup;
	m_physicsBone = physicsBone;
}

void CE_CBoneFollower::UpdateFollower( const Vector &position, const QAngle &orientation, float flInterval )
{
	// UNDONE: Shadow update needs timing info?
	VPhysicsGetObject()->UpdateShadow( position, orientation, false, flInterval );
}

CE_CBoneFollower *CE_CBoneFollower::Create( CEntity *pOwner, const char *pModelName, solid_t &solid, const Vector &position, const QAngle &orientation )
{
	CE_CBoneFollower *pFollower = (CE_CBoneFollower *)CreateEntityByName( "phys_bone_follower" );
	if ( pFollower )
	{
		pFollower->Init( pOwner, pModelName, solid, position, orientation );
	}
	return pFollower;
}

bool CE_CBoneFollower::Init( CEntity *pOwner, const char *pModelName, solid_t &solid, const Vector &position, const QAngle &orientation )
{
	SetOwnerEntity( pOwner->BaseEntity() );
	UTIL_SetModel( this, pModelName );

	AddEffects( EF_NODRAW ); // invisible

	m_modelIndex = modelinfo->GetModelIndex( pModelName );
	m_solidIndex = solid.index;
	SetAbsOrigin( position );
	SetAbsAngles( orientation );
	SetMoveType( MOVETYPE_PUSH );
	SetSolid( SOLID_VPHYSICS );
	SetCollisionGroup( pOwner->GetCollisionGroup() );
	AddSolidFlags( FSOLID_CUSTOMRAYTEST | FSOLID_CUSTOMBOXTEST );
	solid.params.pGameData = (void *)BaseEntity();
	IPhysicsObject *pPhysics = VPhysicsInitShadow( false, false, &solid );
	if ( !pPhysics )
		return false;

	// we can't use the default model bounds because each entity is only one bone of the model
	// so compute the OBB of the physics model and use that.
	Vector mins, maxs;
	physcollision->CollideGetAABB( &mins, &maxs, pPhysics->GetCollide(), vec3_origin, vec3_angle );
	SetCollisionBounds( mins, maxs );

	pPhysics->SetCallbackFlags( pPhysics->GetCallbackFlags() | CALLBACK_GLOBAL_TOUCH );
	pPhysics->EnableGravity( false );
	// This is not a normal shadow controller that is trying to go to a space occupied by an entity in the game physics
	// This entity is not running PhysicsPusher(), so Vphysics is supposed to move it
	// This line of code informs vphysics of that fact
	if ( pOwner->IsNPC() )
	{
		pPhysics->GetShadowController()->SetPhysicallyControlled( true );
	}

	return true;
}













bool CBoneFollowerManager::CreatePhysicsFollower( CAnimating *pParentEntity, physfollower_t &follow, const char *pBoneName, solid_t *pSolid )
{
	CStudioHdr *pStudioHdr = pParentEntity->GetModelPtr();
	matrix3x4_t boneToWorld;
	solid_t solidTmp;

	Vector bonePosition;
	QAngle boneAngles;

	int boneIndex = Studio_BoneIndexByName( pStudioHdr, pBoneName );

	if ( boneIndex >= 0 )
	{
		mstudiobone_t *pBone = pStudioHdr->pBone( boneIndex );

		int physicsBone = pBone->physicsbone;
		if ( !pSolid )
		{
			if ( !PhysModelParseSolidByIndex( solidTmp, pParentEntity->BaseEntity(), pParentEntity->GetModelIndex(), physicsBone ) )
				return false;
			pSolid = &solidTmp;
		}

		// fixup in case ragdoll is assigned to a parent of the requested follower bone
		follow.boneIndex = Studio_BoneIndexByName( pStudioHdr, pSolid->name );
		if ( follow.boneIndex < 0 )
		{
			follow.boneIndex = boneIndex;
		}

		pParentEntity->GetBoneTransform( follow.boneIndex, boneToWorld );
		MatrixAngles( boneToWorld, boneAngles, bonePosition );

		CE_CBoneFollower *follower =CE_CBoneFollower::Create( pParentEntity, STRING(pParentEntity->GetModelName()), *pSolid, bonePosition, boneAngles );
		follow.hFollower.Set(follower->BaseEntity());
		follower->SetTraceData( physicsBone, HitGroupFromPhysicsBone( pParentEntity, physicsBone ) );
		follower->SetBlocksLOS( pParentEntity->BlocksLOS() );
		return true;
	}
	else
	{
		Warning( "ERROR: Tried to create bone follower on invalid bone %s\n", pBoneName );
	}

	return false;
}



void CBoneFollowerManager::InitBoneFollowers( CAnimating *pParentEntity, int iNumBones, const char **pFollowerBoneNames )
{
	m_iNumBones = iNumBones;
	m_physBones.EnsureCount( iNumBones );

	// Now init all the bones
	for ( int i = 0; i < iNumBones; i++ )
	{
		CreatePhysicsFollower( pParentEntity, m_physBones[i], pFollowerBoneNames[i], NULL );
	}
}

void CBoneFollowerManager::DestroyBoneFollowers( void )
{
	for ( int i = 0; i < m_iNumBones; i++ )
	{
		if ( !m_physBones[i].hFollower )
			continue;

		UTIL_Remove( m_physBones[i].hFollower );
		m_physBones[i].hFollower.Set(NULL);
	}

	m_physBones.Purge();
	m_iNumBones = 0;
}

int CBoneFollowerManager::GetBoneFollowerIndex( CE_CBoneFollower *pFollower )
{
	if ( pFollower == NULL )
		return -1;

	for ( int i = 0; i < m_iNumBones; i++ )
	{
		if ( !m_physBones[i].hFollower )
			continue;

		if ( m_physBones[i].hFollower == pFollower )
			return i;
	}

	return -1;
}

CBoneFollowerManager::CBoneFollowerManager()
{
	m_iNumBones = 0;
}

CBoneFollowerManager::~CBoneFollowerManager()
{
	// if this fires then someone isn't destroying their bonefollowers in UpdateOnRemove
	Assert(m_iNumBones==0);
	DestroyBoneFollowers();
}

void CBoneFollowerManager::UpdateBoneFollowers( CAnimating *pParentEntity )
{
	if ( m_iNumBones )
	{
		matrix3x4_t boneToWorld;
		Vector bonePosition;
		QAngle boneAngles;
		for ( int i = 0; i < m_iNumBones; i++ )
		{
			if ( !m_physBones[i].hFollower )
				continue;

			pParentEntity->GetBoneTransform( m_physBones[i].boneIndex, boneToWorld );
			MatrixAngles( boneToWorld, boneAngles, bonePosition );
			m_physBones[i].hFollower->UpdateFollower( bonePosition, boneAngles, 0.1 );
		}
	}
}

physfollower_t *CBoneFollowerManager::GetBoneFollower( int iFollowerIndex )
{
	Assert( iFollowerIndex >= 0 && iFollowerIndex < m_iNumBones );
	if ( iFollowerIndex >= 0 && iFollowerIndex < m_iNumBones )
		return &m_physBones[iFollowerIndex];
	return NULL;
}
