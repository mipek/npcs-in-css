
#ifndef _INCLUDE_GRENADE_SPIT_H_
#define _INCLUDE_GRENADE_SPIT_H_

#include "CEntity.h"
#include "CGrenade.h"


class CSoundPatch;

enum SpitSize_e
{
	SPIT_SMALL,
	SPIT_MEDIUM,
	SPIT_LARGE,
};

#define SPIT_GRAVITY 600


class CGrenadeSpit : public CE_Grenade
{
public:
	CE_DECLARE_CLASS( CGrenadeSpit, CE_Grenade );

public:
	CGrenadeSpit( void );

	virtual void		Spawn( void );
	virtual void		Precache( void );
	virtual void		Event_Killed( const CTakeDamageInfo &info );

	virtual	unsigned int	PhysicsSolidMaskForEntity( void ) const { return ( BaseClass::PhysicsSolidMaskForEntity() | CONTENTS_WATER ); }

	void 				GrenadeSpitTouch( CEntity *pOther );
	void				SetSpitSize( int nSize );
	void				Detonate( void );
	void				Think( void );

private:
	DECLARE_DATADESC();
	
	void	InitHissSound( void );
	
	CFakeHandle		m_hSpitEffect;
	CSoundPatch		*m_pHissSound;
	bool			m_bPlaySound;
};



#endif

