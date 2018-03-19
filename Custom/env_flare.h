#ifndef _include_env_flare_h_
#define _include_env_flare_h_

#include "CEntity.h"
#include "CSode_Fix.h"

#define	SF_FLARE_NO_DLIGHT	0x00000001
#define	SF_FLARE_NO_SMOKE	0x00000002
#define	SF_FLARE_INFINITE	0x00000004
#define	SF_FLARE_START_OFF	0x00000008

#define	FLARE_DURATION		30.0f
#define FLARE_DECAY_TIME	10.0f
#define FLARE_BLIND_TIME	6.0f

class CSoundPatch;

//---------------------
// Flare
//---------------------

class CFlare : public CSode_Fix
{
public:
	CE_DECLARE_CLASS( CFlare, CSode_Fix );

	CFlare();
	~CFlare();

	static CFlare *	GetActiveFlares( void );
	CFlare *		GetNextFlare( void ) const { return m_pNextFlare; }

	static CFlare *Create( Vector vecOrigin, QAngle vecAngles, CEntity *pOwner, float lifetime );

	virtual unsigned int PhysicsSolidMaskForEntity( void ) const;

	void	Spawn( void );
	void	Precache( void );
	int		Restore( IRestore &restore );
	void	Activate( void );

	void	StartBurnSound( void );

	void	Start( float lifeTime );
	void	Die( float fadeTime );
	void	Launch( const Vector &direction, float speed );

	Class_T Classify( void );

	void	FlareTouch( CEntity *pOther );
	void	FlareBurnTouch( CEntity *pOther );
	void	FlareThink( void );

	void	InputStart( inputdata_t &inputdata );
	void	InputDie( inputdata_t &inputdata );
	void	InputLaunch( inputdata_t &inputdata );

	//DECLARE_SERVERCLASS();
	DECLARE_DATADESC();

	static CFlare *activeFlares;

	CEntity *m_pOwner;
	int			m_nBounces;			// how many times has this flare bounced?
	float m_flTimeBurnOut;
	float m_flScale;
	float		m_flDuration;
	float		m_flNextDamage;
	
	CSoundPatch	*m_pBurnSound;
	bool		m_bFading;
	bool m_bLight;
	bool m_bSmoke;
	//bool m_bPropFlare;

	bool		m_bInActiveList;
	CFlare *	m_pNextFlare;

	void		RemoveFromActiveFlares( void );
	void		AddToActiveFlares( void );
};

#endif //_include_env_flare_h_