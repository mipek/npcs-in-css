
#include "CFilters.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"

CE_LINK_ENTITY_TO_CLASS(CBaseFilter, CE_CBaseFilter);

SH_DECL_MANUALHOOK2(PassesFilterImpl, 0, 0, 0, bool, CBaseEntity *, CBaseEntity *);
DECLARE_HOOK(PassesFilterImpl, CE_CBaseFilter);
DECLARE_DEFAULTHANDLER(CE_CBaseFilter, PassesFilterImpl, bool, (CBaseEntity *pCaller, CBaseEntity *pEntity), (pCaller, pEntity));




//Datamaps
DEFINE_PROP(m_bNegated, CE_CBaseFilter);




bool CE_CBaseFilter::PassesFilter( CBaseEntity *pCaller, CBaseEntity *pEntity )
{
	bool baseResult = PassesFilterImpl( pCaller, pEntity );
	return (m_bNegated) ? !baseResult : baseResult;
}