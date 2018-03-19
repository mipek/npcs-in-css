
#include "CPropVehicleDriveable.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



CE_LINK_ENTITY_TO_CLASS(CPropVehicleDriveable, CE_CPropVehicleDriveable);


// Sendprops
DEFINE_PROP(m_hPlayer, CE_CPropVehicleDriveable);
DEFINE_PROP(m_nRPM, CE_CPropVehicleDriveable);


//Datamaps
DEFINE_PROP(m_hNPCDriver, CE_CPropVehicleDriveable);

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
CEntity *CE_CPropVehicleDriveable::GetDriver() 
{ 
	if (m_hNPCDriver != NULL) 
		return m_hNPCDriver; 

	return m_hPlayer; 
}
