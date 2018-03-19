#ifndef _INCLUDE_CAI_ACTOR_H
#define _INCLUDE_CAI_ACTOR_H

#include "CEntity.h"
#include "CAI_basehumanoid.h"

#if defined( _WIN32 )
#pragma once
#endif

class CAI_Actor : public CAI_ExpresserHost<CAI_BaseHumanoid>
{
public:
	CE_DECLARE_CLASS( CAI_Actor, CAI_ExpresserHost<CAI_BaseHumanoid> );

	//DECLARE_DEFAULTHEADER(OnEndMoveAndShoot, void,( void ));
};

//-----------------------------------------------------------------------------
#endif // _INCLUDE_CAI_ACTOR_H
