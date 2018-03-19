
#ifndef	GRENADEBEAM_H
#define	GRENADEBEAM_H

#include "CEntity.h"
#include "CGrenade.h"
#include "CSode_Fix.h"


#define GRENADEBEAM_MAXBEAMS 2
#define GRENADEBEAM_MAXHITS  GRENADEBEAM_MAXBEAMS-1

class CGrenadeBeam;
class CE_CBeam;

// End of the grenade beam
class CGrenadeBeamChaser : public CSode_Fix
{
public:
	CE_DECLARE_CLASS( CGrenadeBeamChaser, CSode_Fix );
	DECLARE_DATADESC();

	static CGrenadeBeamChaser* ChaserCreate( CGrenadeBeam *pTarget );

	void			Spawn( void );
	void 			ChaserThink();

	CGrenadeBeam*	m_pTarget;
};

class CGrenadeBeam : public CE_Grenade
{
public:
	CE_DECLARE_CLASS( CGrenadeBeam, CE_Grenade );
	DECLARE_DATADESC();

	static CGrenadeBeam* Create( CEntity* pOwner, const Vector &vStart);

public:
	CGrenadeBeam();

	void		Spawn( void );
	void		Precache( void );
	void		Format( color32 clrColor, float flWidth);
	void 		GrenadeBeamTouch( CEntity *pOther );
	void 		KillBeam();
	void		CreateBeams(void);
	void		UpdateBeams(void);
	//void		DebugBeams(void);
	void		GetChaserTargetPos(Vector *vPosition);
	void		GetNextTargetPos(Vector *vPosition);
	int			UpdateTransmitState(void);
	void		Shoot(Vector vDirection, float flSpeed, float flLifetime, float flLag, float flDamage );

	Vector		m_vLaunchPos;
	float		m_flBeamWidth;
	float		m_flBeamSpeed;
	float		m_flBeamLag;
	float		m_flLaunchTime;
	float		m_flLastTouchTime;
	CFakeHandle	m_hBeamChaser;
	int			m_nNumHits;
	Vector		m_pHitLocation[GRENADEBEAM_MAXHITS];
	CE_CBeam*	m_pBeam[GRENADEBEAM_MAXBEAMS];
};

#endif	//GRENADEBEAM_H
