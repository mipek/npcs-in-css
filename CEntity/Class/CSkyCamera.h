
#ifndef _INCLUDE_CSKYCAMERA_H_
#define _INCLUDE_CSKYCAMERA_H_

#include "CEntity.h"

struct fogparams_t
{
	DECLARE_CLASS_NOBASE( fogparams_t );
	DECLARE_EMBEDDED_NETWORKVAR();

#ifndef CLIENT_DLL
	DECLARE_SIMPLE_DATADESC();
#endif

	bool operator !=( const fogparams_t& other ) const;

	CNetworkVector( dirPrimary );
	CNetworkColor32( colorPrimary );
	CNetworkColor32( colorSecondary );
	CNetworkColor32( colorPrimaryLerpTo );
	CNetworkColor32( colorSecondaryLerpTo );
	CNetworkVar( float, start );
	CNetworkVar( float, end );
	CNetworkVar( float, farz );
	CNetworkVar( float, maxdensity );

	CNetworkVar( float, startLerpTo );
	CNetworkVar( float, endLerpTo );
	CNetworkVar( float, lerptime );
	CNetworkVar( float, duration );
	CNetworkVar( bool, enable );
	CNetworkVar( bool, blend );
};


struct sky3dparams_t
{
	DECLARE_CLASS_NOBASE( sky3dparams_t );
	DECLARE_EMBEDDED_NETWORKVAR();

	DECLARE_SIMPLE_DATADESC();

	// 3d skybox camera data
	CNetworkVar( int, scale );
	CNetworkVector( origin );
	CNetworkVar( int, area );

	// 3d skybox fog data
	CNetworkVarEmbedded( fogparams_t, fog );
};


class CE_CSkyCamera : public CEntity
{
public:
	CE_DECLARE_CLASS( CE_CSkyCamera, CEntity );
	
	CE_CSkyCamera();
	~CE_CSkyCamera();

public:
	CE_CSkyCamera		*m_pNext;

public: //Datamaps
	DECLARE_DATAMAP_OFFSET(sky3dparams_t, m_skyboxData);

};

CE_CSkyCamera*	GetCurrentSkyCamera();
CE_CSkyCamera*	GetSkyCameraList();

#endif
