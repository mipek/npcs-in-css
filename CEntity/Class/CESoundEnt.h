
#ifndef _INCLUDE_CE_SOUNDENT_H_
#define _INCLUDE_CE_SOUNDENT_H_

#include "CEntity.h"

class CE_CSoundEnt : public CEntity
{
public:
	CE_DECLARE_CLASS(CE_CSoundEnt, CEntity);

public:
	static void InitSoundEnt();

	static CSound*	GetLoudestSoundOfType( int iType, const Vector &vecEarPosition );
	static int		ActiveList( void );
	static CSound*	SoundPointerForIndex( int iIndex );

		
protected: //Datamaps
	DECLARE_DATAMAP(int, m_iActiveSound);
	DECLARE_DATAMAP(CSound, m_SoundPool);

};




#endif

