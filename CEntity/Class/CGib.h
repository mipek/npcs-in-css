
#ifndef _INCLUDE_CGIB_H_
#define _INCLUDE_CGIB_H_

#include "CEntity.h"
#include "CAnimating.h"

#define	HUMAN_GIB_COUNT			6
#define ALIEN_GIB_COUNT			4

enum GibType_e
{
	GIB_HUMAN,
	GIB_ALIEN,
};

class CE_CGib : public CAnimating
{
public:
	CE_DECLARE_CLASS( CE_CGib, CAnimating );
};


#endif
