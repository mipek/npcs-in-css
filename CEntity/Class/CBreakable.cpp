
#include "CBreakable.h"
#include "CE_recipientfilter.h"

// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


CE_LINK_ENTITY_TO_CLASS(CBreakable, CE_CBreakable);


//Datamaps
//DEFINE_PROP(m_Material,CE_CBreakable);




const char *CE_CBreakable::MaterialSound( Materials precacheMaterial )
{
	switch ( precacheMaterial )
	{
		case matWood:
			return "Breakable.MatWood";
		case matFlesh:
		case matWeb:
			return "Breakable.MatFlesh";
		case matComputer:
			return "Breakable.Computer";
		case matUnbreakableGlass:
		case matGlass:
			return "Breakable.MatGlass";
		case matMetal:
			return "Breakable.MatMetal";
		case matCinderBlock:
		case matRocks:
			return "Breakable.MatConcrete";
		case matCeilingTile:
		case matNone:
		default:
			break;
	}

	return NULL;
}

void CE_CBreakable::MaterialSoundRandom( int entindex, Materials soundMaterial, float volume )
{
	const char	*soundname;
	soundname = MaterialSound( soundMaterial );
	if ( !soundname )
		return;

	CSoundParameters params;
	if ( !GetParametersForSound( soundname, params, NULL ) )
		return;

	CPASAttenuationFilter filter( CEntity::Instance( entindex ), params.soundlevel );


	EmitSound_t ep;
	ep.m_nChannel = params.channel;
	ep.m_pSoundName = params.soundname;
	ep.m_flVolume = volume;
	ep.m_SoundLevel = params.soundlevel;

	EmitSound( filter, entindex, ep );
}

/*bool CE_CBreakable::IsBreakable( void )
{
	return m_Material != matUnbreakableGlass;
}*/

