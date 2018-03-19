
#include "CSkyCamera.h"



// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



CE_LINK_ENTITY_TO_CLASS(CSkyCamera, CE_CSkyCamera);


// Datamaps
DEFINE_PROP(m_skyboxData, CE_CSkyCamera);


CEntityClassList<CE_CSkyCamera> g_SkyList;
template <> CE_CSkyCamera *CEntityClassList<CE_CSkyCamera>::m_pClassList = NULL;


CE_CSkyCamera*	GetCurrentSkyCamera()
{
	return g_SkyList.m_pClassList;
}

CE_CSkyCamera*	GetSkyCameraList()
{
	return g_SkyList.m_pClassList;
}


CE_CSkyCamera::CE_CSkyCamera()
{
	g_SkyList.Insert( this );
}

CE_CSkyCamera::~CE_CSkyCamera()
{
	g_SkyList.Remove( this );
}

