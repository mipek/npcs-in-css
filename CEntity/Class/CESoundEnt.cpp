
#include "CEntity.h"
#include "CESoundEnt.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



CE_LINK_ENTITY_TO_CLASS(CSoundEnt, CE_CSoundEnt);

static CE_CSoundEnt *g_pSoundEnt = NULL;

//Datamaps
DEFINE_PROP(m_iActiveSound,CE_CSoundEnt);
DEFINE_PROP(m_SoundPool,CE_CSoundEnt);


void CE_CSoundEnt::InitSoundEnt()
{
	g_pSoundEnt = dynamic_cast<CE_CSoundEnt *>(g_helpfunc.FindEntityByClassname((CBaseEntity *)NULL, "soundent"));
}

int CE_CSoundEnt::ActiveList ( void )
{
	if ( !g_pSoundEnt )
	{
		return SOUNDLIST_EMPTY;
	}

	return g_pSoundEnt->m_iActiveSound;
}

CSound*	CE_CSoundEnt::GetLoudestSoundOfType( int iType, const Vector &vecEarPosition )
{
	CSound *pLoudestSound = NULL;

	int iThisSound; 
	int	iBestSound = SOUNDLIST_EMPTY;
	float flBestDist = MAX_COORD_RANGE*MAX_COORD_RANGE;// so first nearby sound will become best so far.
	float flDist;
	CSound *pSound = NULL;

	iThisSound = ActiveList();

	while ( iThisSound != SOUNDLIST_EMPTY )
	{
		pSound = SoundPointerForIndex( iThisSound );

		if(pSound == NULL)
			break;

		if ( pSound->m_iType == iType && pSound->ValidateOwner() )
		{
			flDist = ( pSound->GetSoundOrigin() - vecEarPosition ).Length();

			//FIXME: This doesn't match what's in Listen()
			//flDist = UTIL_DistApprox( pSound->GetSoundOrigin(), vecEarPosition );

			if ( flDist <= pSound->m_iVolume && flDist < flBestDist )
			{
				pLoudestSound = pSound;

				iBestSound = iThisSound;
				flBestDist = flDist;
			}
		}

		iThisSound = pSound->m_iNext;
	}

	return pLoudestSound;
}


CSound*	CE_CSoundEnt::SoundPointerForIndex( int iIndex )
{
	if ( !g_pSoundEnt )
	{
		return NULL;
	}

	if ( iIndex > ( MAX_WORLD_SOUNDS_MP - 1 ) )
	{
		Msg( "SoundPointerForIndex() - Index too large!\n" );
		return NULL;
	}

	if ( iIndex < 0 )
	{
		Msg( "SoundPointerForIndex() - Index < 0!\n" );
		return NULL;
	}
	
	CSound &sound = *(CSound *)(((uint8_t *)(g_pSoundEnt->BaseEntity())) + g_pSoundEnt->m_SoundPool.offset + (sizeof(CSound) + iIndex));
	return &sound;
}


//---------------------------------------------------------
// This function returns the spot the listener should be
// interested in if he hears the sound. MOST of the time,
// this spot is the same as the sound's origin. But sometimes
// (like with bullet impacts) the entity that owns the 
// sound is more interesting than the actual location of the
// sound effect.
//---------------------------------------------------------
const Vector &CSound::GetSoundReactOrigin( void )
{
	CEntity *cent = CEntity::Instance(m_hOwner.Get());
	// Check pure types.
	switch( m_iType )
	{
	case SOUND_BULLET_IMPACT:
	case SOUND_PHYSICS_DANGER:
		if( cent != NULL )
		{
			// We really want the origin of this sound's 
			// owner.
			return cent->GetAbsOrigin();
		}
		else
		{
			// If the owner is somehow invalid, we'll settle
			// for the sound's origin rather than a crash.
			return GetSoundOrigin();
		}
		break;
	}

	if( m_iType & SOUND_CONTEXT_REACT_TO_SOURCE )
	{
		if( cent != NULL )
		{
			return cent->GetAbsOrigin();
		}
	}

	// Check for types with additional context.
	if( m_iType & SOUND_DANGER )
	{
		if( (m_iType & SOUND_CONTEXT_FROM_SNIPER) )
		{
			if( cent != NULL )
			{
				// Be afraid of the sniper's location, not where the bullet will hit.
				return cent->GetAbsOrigin();
			}
			else
			{
				return GetSoundOrigin();
			}
		}
	}


	return GetSoundOrigin();
}




