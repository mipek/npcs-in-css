
#include "CEntity.h"
#include "TemplateEntities.h"



// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"




struct TemplateEntityData_t
{
	const char	*pszName;
	char		*pszMapData;
	string_t	iszMapData;
	int			iMapDataLength;
	bool		bNeedsEntityIOFixup;	// If true, this template has entity I/O in its mapdata that needs fixup before spawning.
	char		*pszFixedMapData;		// A single copy of this template that we used to fix up the Entity I/O whenever someone wants a fixed version of this template

	DECLARE_SIMPLE_DATADESC();
};


CUtlVector<TemplateEntityData_t *> *g_Templates;

//-----------------------------------------------------------------------------
// Purpose: Looks up a template entity by name, returning the map data blob as
//			a null-terminated string containing key/value pairs.
//			NOTE: This can't handle multiple templates with the same targetname.
//-----------------------------------------------------------------------------
string_t Templates_FindByTargetName(const char *pszName)
{
	int nCount = g_Templates->Count();
	for (int i = 0; i < nCount; i++)
	{
		TemplateEntityData_t *pTemplate = g_Templates->Element(i);
		if ( !stricmp(pTemplate->pszName, pszName) )
			return Templates_FindByIndex( i );
	}

	return NULL_STRING;
}

//-----------------------------------------------------------------------------
// Purpose: Looks up a template entity by its index in the templates
//			Used by point_templates because they often have multiple templates with the same name
//-----------------------------------------------------------------------------
string_t Templates_FindByIndex( int iIndex )
{
	Assert( iIndex < g_Templates->Count() );

	TemplateEntityData_t *pTemplate = g_Templates->Element(iIndex);
	// First time through we alloc the mapdata onto the pool.
	// It's safe to do it now because this isn't called until post Entity I/O cleanup.
	if ( pTemplate->iszMapData == NULL_STRING )
	{
		pTemplate->iszMapData = MAKE_STRING( pTemplate->pszMapData );
	}

	return pTemplate->iszMapData;
}
