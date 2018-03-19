#include "CViewModel.h"

class CBaseViewModel;
CE_LINK_ENTITY_TO_CLASS(CBaseViewModel, CViewModel);

// Sendprops
DEFINE_PROP(m_nViewModelIndex, CViewModel);

BEGIN_DATADESC( CViewModel )
	DEFINE_FIELD( m_nViewModelIndex, FIELD_INTEGER ),
END_DATADESC()