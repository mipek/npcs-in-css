
#ifndef _INCLUDE_CPATHTRACK_H_
#define _INCLUDE_CPATHTRACK_H_

#include "CEntity.h"

//-----------------------------------------------------------------------------
// Spawnflag for CPathTrack
//-----------------------------------------------------------------------------
#define SF_PATH_DISABLED		0x00000001
//#define SF_PATH_FIREONCE		0x00000002
#define SF_PATH_ALTREVERSE		0x00000004
#define SF_PATH_DISABLE_TRAIN	0x00000008
#define SF_PATH_TELEPORT		0x00000010
#define SF_PATH_UPHILL			0x00000020
#define SF_PATH_DOWNHILL		0x00000040
#define SF_PATH_ALTERNATE		0x00008000


enum TrackOrientationType_t
{
	TrackOrientation_Fixed = 0,
	TrackOrientation_FacePath,
	TrackOrientation_FacePathAngles,
};



class CE_CPathTrack : public CPointEntity
{
public:
	CE_DECLARE_CLASS( CE_CPathTrack, CPointEntity );

	static CE_CPathTrack	*ValidPath( CE_CPathTrack *ppath, int testFlag = true );
	static void BeginIteration();
	static void EndIteration();

	CE_CPathTrack	*GetNext( void );
	CE_CPathTrack	*GetPrevious( void );

	void		Visit();
	bool		HasBeenVisited() const;

	float GetRadius() const { return m_flRadius; }

public:
	static int	*s_nCurrIterVal;
	static bool *s_bIsIterating;


protected:
	DECLARE_DATAMAP(CEFakeHandle<CE_CPathTrack>, m_paltpath);
	DECLARE_DATAMAP(CEFakeHandle<CE_CPathTrack>, m_pprevious);
	DECLARE_DATAMAP(CEFakeHandle<CE_CPathTrack>, m_pnext);
	DECLARE_DATAMAP(float, m_flRadius);
	DECLARE_DATAMAP_OFFSET(int, m_nIterVal);

};


//-----------------------------------------------------------------------------
// Used to make sure circular iteration works all nice
//-----------------------------------------------------------------------------
#define BEGIN_PATH_TRACK_ITERATION() CPathTrackVisitor _visit

class CPathTrackVisitor
{
public:
	CPathTrackVisitor() { CE_CPathTrack::BeginIteration(); }
	~CPathTrackVisitor() { CE_CPathTrack::EndIteration(); }
};



#endif
