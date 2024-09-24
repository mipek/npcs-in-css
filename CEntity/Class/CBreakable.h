#ifndef _INCLUDE_CBREAKABLE_H_
#define _INCLUDE_CBREAKABLE_H_

#include "CEntity.h"


class CE_CBreakable : public CEntity
{
public:
	CE_DECLARE_CLASS( CE_CBreakable, CEntity );

	//Materials GetMaterialType( void ) { return m_Material; }

	//bool IsBreakable( void );

public:
	static void MaterialSoundRandom( int entindex, Materials soundMaterial, float volume );
	static const char *MaterialSound( Materials precacheMaterial );

protected: //Datamaps
	//DECLARE_DATAMAP(Materials, m_Material);

};


#endif