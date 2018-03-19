#ifndef _INCLUDE_LADDER_H_
#define _INCLUDE_LADDER_H_

#include "CEntity.h"

class CPlayer;

class CFuncLadder : public CEntity
{
public:
	CE_DECLARE_CLASS( CFuncLadder, CEntity );

	void	GetTopPosition( Vector& org );
	void	GetBottomPosition( Vector& org );
	
	bool	IsPlayerOnLadder(CPlayer *pPlayer);

protected: // Sendprops
	DECLARE_SENDPROP(Vector, m_vecPlayerMountPositionTop);
	DECLARE_SENDPROP(Vector, m_vecPlayerMountPositionBottom);
	DECLARE_SENDPROP(Vector, m_vecLadderDir);


};



#endif
