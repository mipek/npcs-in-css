
#ifndef _INCLUDE_CENVPLAYERSURFACETRIGGER_H
#define _INCLUDE_CENVPLAYERSURFACETRIGGER_H

#include "CEntity.h"

class CEnvPlayerSurfaceTrigger : public CEntity
{
public:
	CE_DECLARE_CLASS( CEnvPlayerSurfaceTrigger, CEntity );

	~CEnvPlayerSurfaceTrigger( void );
	void	Spawn( void );
	void	OnRestore( void );

	static void	SetPlayerSurface( CPlayer *pPlayer, char gameMaterial );

	void	UpdateMaterialThink( void );

private:
	void	PlayerSurfaceChanged( CPlayer *pPlayer, char gameMaterial );

protected: //Datamaps
	DECLARE_DATAMAP(int, m_iTargetGameMaterial);
	DECLARE_DATAMAP(int, m_iCurrentGameMaterial);
	DECLARE_DATAMAP(bool, m_bDisabled);
	DECLARE_DATAMAP(COutputEvent, m_OnSurfaceChangedToTarget);
	DECLARE_DATAMAP(COutputEvent, m_OnSurfaceChangedFromTarget);


};


#endif