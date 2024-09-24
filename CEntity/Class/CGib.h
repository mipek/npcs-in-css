#ifndef _INCLUDE_CGIB_H_
#define _INCLUDE_CGIB_H_

#include "CEntity.h"
#include "CAnimating.h"


class CE_CGib : public CAnimating
{
public:
	CE_DECLARE_CLASS( CE_CGib, CAnimating );

	void Spawn( const char *szGibModel );
	void Spawn( const char *szGibModel, float flLifetime );

	void BounceGibTouch ( CEntity *pOther );

	void SetBloodColor( int nBloodColor );

	void InitGib( CEntity *pVictim, float fMaxVelocity, float fMinVelocity );
	void LimitVelocity();


public:
	static	void SpawnSpecificGibs( CBaseEntity *pVictim, int nNumGibs, float fMaxVelocity, float fMinVelocity, const char* cModelName, float flLifetime = 25);

private:
	void AdjustVelocityBasedOnHealth( int nHealth, Vector &vecVelocity );

public:
	DECLARE_DATAMAP_OFFSET(float, m_lifeTime);
	DECLARE_DATAMAP_OFFSET(bool, m_bForceRemove);
	DECLARE_DATAMAP_OFFSET(int, m_material);
	DECLARE_DATAMAP_OFFSET(int, m_cBloodDecals);
	DECLARE_DATAMAP_OFFSET(int, m_bloodColor);

};


#endif