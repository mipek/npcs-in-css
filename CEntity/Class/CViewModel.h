#ifndef _INCLUDE_CVIEWMODEL_H
#define _INCLUDE_CVIEWMODEL_H

#include "CEntity.h"
#include "CAnimating.h"

class CViewModel: public CAnimating
{
public:
	CE_DECLARE_CLASS( CViewModel, CAnimating );
	DECLARE_DATADESC();

protected:
	DECLARE_SENDPROP(int, m_nViewModelIndex);
};

#endif //_INCLUDE_CVIEWMODEL_H