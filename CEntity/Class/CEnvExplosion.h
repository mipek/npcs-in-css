
#ifndef _INCLUDE_CENVEXPLOSION_H_
#define _INCLUDE_CENVEXPLOSION_H_

#include "CEntity.h"


#define	SF_ENVEXPLOSION_NODAMAGE	0x00000001 // when set, ENV_EXPLOSION will not actually inflict damage
#define	SF_ENVEXPLOSION_REPEATABLE	0x00000002 // can this entity be refired?
#define SF_ENVEXPLOSION_NOFIREBALL	0x00000004 // don't draw the fireball
#define SF_ENVEXPLOSION_NOSMOKE		0x00000008 // don't draw the smoke
#define SF_ENVEXPLOSION_NODECAL		0x00000010 // don't make a scorch mark
#define SF_ENVEXPLOSION_NOSPARKS	0x00000020 // don't make sparks
#define SF_ENVEXPLOSION_NOSOUND		0x00000040 // don't play explosion sound.
#define SF_ENVEXPLOSION_RND_ORIENT	0x00000080	// randomly oriented sprites
#define SF_ENVEXPLOSION_NOFIREBALLSMOKE 0x0100
#define SF_ENVEXPLOSION_NOPARTICLES 0x00000200
#define SF_ENVEXPLOSION_NODLIGHTS	0x00000400
#define SF_ENVEXPLOSION_NOCLAMPMIN	0x00000800 // don't clamp the minimum size of the fireball sprite
#define SF_ENVEXPLOSION_NOCLAMPMAX	0x00001000 // don't clamp the maximum size of the fireball sprite
#define SF_ENVEXPLOSION_SURFACEONLY	0x00002000 // don't damage the player if he's underwater.
#define SF_ENVEXPLOSION_GENERIC_DAMAGE	0x00004000 // don't do BLAST damage


class CE_CEnvExplosion : public CPointEntity
{
public:
	CE_DECLARE_CLASS( CE_CEnvExplosion, CPointEntity );

	void SetCustomDamageType( int iType ) { m_iCustomDamageType = iType; }

protected: //Sendprops


public: //Datamaps
	DECLARE_DATAMAP(CFakeHandle, m_hInflictor);
	DECLARE_DATAMAP(int, m_iCustomDamageType);
	DECLARE_DATAMAP(CFakeHandle, m_hEntityIgnore);
	DECLARE_DATAMAP(int, m_iClassIgnore);


};


void ExplosionCreate( const Vector &center, const QAngle &angles, 
	CBaseEntity *pOwner, int magnitude, int radius, bool doDamage, float flExplosionForce = 0.0f, bool bSurfaceOnly = false, bool bSilent = false, int iCustomDamageType = -1 );

void ExplosionCreate( const Vector &center, const QAngle &angles, 
					 CBaseEntity *pOwner, int magnitude, int radius, int nSpawnFlags, 
					 float flExplosionForce = 0.0f, CBaseEntity *pInflictor = NULL, int iCustomDamageType = -1,  const EHANDLE *ignoredEntity = NULL, Class_T ignoredClass = CLASS_NONE);

// this version lets you specify classes or entities to be ignored
void ExplosionCreate( const Vector &center, const QAngle &angles, 
					 CBaseEntity *pOwner, int magnitude, int radius, bool doDamage, 
					 const EHANDLE *ignoredEntity, Class_T ignoredClass,
					 float flExplosionForce = 0.0f, bool bSurfaceOnly = false, bool bSilent = false, int iCustomDamageType = -1 );



#endif
