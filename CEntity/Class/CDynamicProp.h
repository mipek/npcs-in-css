
#ifndef _INCLUDE_CDYNAMICPROP_H_
#define _INCLUDE_CDYNAMICPROP_H_

#include "CBreakable.h"

class CE_CDynamicProp : public CE_CBreakable
{
public:
	CE_DECLARE_CLASS( CE_CDynamicProp, CE_CBreakable );


public: // Sendprop
	DECLARE_DATAMAP(bool, m_bUseHitboxesForRenderBox);
};


#endif
