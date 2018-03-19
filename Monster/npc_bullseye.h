#ifndef _INCLUDE_NPC_BULLSEYE_H_
#define _INCLUDE_NPC_BULLSEYE_H_

#include "CEntityManager.h"
#include "CEntity.h"
#include "CCycler_Fix.h"
#include "CAI_NPC.h"

class CNPC_Bullseye : public CE_Cycler_Fix
{
public:
	CE_DECLARE_CLASS( CNPC_Bullseye, CE_Cycler_Fix );

	CNPC_Bullseye();
	~CNPC_Bullseye();

	void PostConstructor();
	virtual void Precache( void );
	virtual void Spawn( void );
	virtual void Activate( void );
	virtual void OnRestore( void );

	virtual float GetAutoAimRadius() { return m_fAutoaimRadius; }

	Class_T Classify( void );
	void	Event_Killed( const CTakeDamageInfo &info );
	void	DecalTrace( trace_t *pTrace, char const *decalName );
	void	ImpactTrace( trace_t *pTrace, int iDamageType, const char *pCustomImpactName );
	bool	IsLightDamage( const CTakeDamageInfo &info );
	void	TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator );
	int		OnTakeDamage( const CTakeDamageInfo &info );

	//CE_TODO , need hack
	bool	UsePerfectAccuracy( void ) { return m_bPerfectAccuracy; }

	bool	TestHitboxes( const Ray_t &ray, unsigned int fContentsMask, trace_t& tr ) { return false; } // force traces to test against hull
	
	void	BullseyeThink( void );
	bool	CanBecomeRagdoll( void );

	void	SetPainPartner( CEntity *pOther );
	void	InputTargeted( inputdata_t &inputdata );
	void	InputReleased( inputdata_t &inputdata );

	bool	CanBecomeServerRagdoll( void ) { return false;	}

	bool	CanBeAnEnemyOf( CBaseEntity *pEnemy );


protected:
	float			_m_flFieldOfView;

	CFakeHandle		m_hPainPartner;	//Entity that the bullseye will pass any damage it take to
	COutputEvent	m_OnTargeted;
	COutputEvent	m_OnReleased;
	bool			m_bPerfectAccuracy;	// Entities that shoot at me should be perfectly accurate
	float			m_fAutoaimRadius;	// How much to influence player's autoaim.
	float			m_flMinDistValidEnemy;

	DECLARE_DATADESC();
};

int FindBullseyesInCone( CBaseEntity **pList, int listMax, const Vector &coneOrigin, const Vector &coneAxis, float coneAngleCos, float coneLength );

#define SF_BULLSEYE_NONSOLID		(1 << 16)
#define SF_BULLSEYE_NODAMAGE		(1 << 17)
#define	SF_BULLSEYE_ENEMYDAMAGEONLY	(1 << 18)
#define	SF_BULLSEYE_BLEED			(1 << 19)
#define SF_BULLSEYE_PERFECTACC		(1 << 20)
#define SF_BULLSEYE_VPHYSICSSHADOW  (1 << 21)

#endif
