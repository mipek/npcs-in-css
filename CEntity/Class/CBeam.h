
#ifndef _INCLUDE_CBEAM_H_
#define _INCLUDE_CBEAM_H_

#include "CEntity.h"
#include "beam_flags.h"

#define MAX_BEAM_WIDTH			102.3f
#define MAX_BEAM_SCROLLSPEED	100.0f
#define MAX_BEAM_NOISEAMPLITUDE		64

#define SF_BEAM_STARTON			0x0001
#define SF_BEAM_TOGGLE			0x0002
#define SF_BEAM_RANDOM			0x0004
#define SF_BEAM_RING			0x0008
#define SF_BEAM_SPARKSTART		0x0010
#define SF_BEAM_SPARKEND		0x0020
#define SF_BEAM_DECALS			0x0040
#define SF_BEAM_SHADEIN			0x0080
#define SF_BEAM_SHADEOUT		0x0100
#define	SF_BEAM_TAPEROUT		0x0200	// Tapers to zero
#define SF_BEAM_TEMPORARY		0x8000

#define ATTACHMENT_INDEX_BITS	5
#define ATTACHMENT_INDEX_MASK	((1 << ATTACHMENT_INDEX_BITS) - 1)



class CE_CBeam : public CEntity
{
public:
	CE_DECLARE_CLASS(CE_CBeam, CEntity);

public:
	static CE_CBeam *BeamCreate( const char *pSpriteName, float width );

public:
	void BeamInit( const char *pSpriteName, float width );
	void PointEntInit( const Vector &start, CEntity *pEndEntity );
	
	void EntsInit( CEntity *pStartEntity, CEntity *pEndEntity );

	void LiveForTime( float time );
	void SetFireTime( float flFireTime );

	void SetColor( int r, int g, int b );
	void SetBrightness( int brightness );
	void SetNoise( float amplitude );
	void SetFrame( float frame );
	void SetScrollRate( int speed );
	void SetTexture( int spriteIndex );
	void SetHaloTexture( int spriteIndex );
	void SetHaloScale( float haloScale );
	void SetWidth( float width );
	void SetEndWidth( float endWidth );
	void SetFadeLength( float fadeLength );

	void SetType( int type );
	int GetType( void ) const;
	void SetStartPos( const Vector &pos );
	void SetEndPos( const Vector &pos );
	void SetStartEntity( CEntity *pEntity );
	void SetEndEntity( CEntity *pEntity );
	void SetStartAttachment( int attachment );
	void SetEndAttachment( int attachment );
	void SetBeamFlags( int flags );

	void RelinkBeam( void );

	const Vector &GetAbsStartPos( void );
	const Vector &GetAbsEndPos( void );

	CEntity	*GetStartEntity( void );
	CEntity *GetEndEntity( void );

	void SetBeamFlag( int flag );
	void PointsInit( const Vector &start, const Vector &end );

	void SetAbsEndPos( const Vector &pos );
	void BeamDamage( trace_t *ptr );
	void DoSparks( const Vector &start, const Vector &end );

	CEntity *RandomTargetname( const char *szName );

	float GetWidth() { return m_fWidth; }

public:
	// not going to hook this
	virtual const char *GetDecalName( void ) { return "BigShot"; }

private:
	CEntity *Get_m_hAttachEntity(int index);
	void Set_m_hAttachEntity(int index, CBaseEntity *pEntity);
	
	int Get_m_nAttachIndex(int index);
	void Set_m_nAttachIndex(int index, int value);

protected: //Sendprops
	DECLARE_SENDPROP(float, m_fWidth);
	DECLARE_SENDPROP(float, m_fEndWidth);
	DECLARE_SENDPROP(float, m_fFadeLength);
	DECLARE_SENDPROP(float, m_fAmplitude);
	DECLARE_SENDPROP(float, m_fStartFrame);
	DECLARE_SENDPROP(float, m_fSpeed);
	DECLARE_SENDPROP(CBaseEntity *, m_hAttachEntity);
	DECLARE_SENDPROP(int, m_nAttachIndex);
	DECLARE_SENDPROP(int, m_nHaloIndex);
	DECLARE_SENDPROP(float, m_fHaloScale);
	DECLARE_SENDPROP(int, m_nBeamType);
	DECLARE_SENDPROP(int, m_nBeamFlags);
	DECLARE_SENDPROP(int, m_nNumBeamEnts);
	DECLARE_SENDPROP(Vector, m_vecEndPos);


protected: //Datamaps
	DECLARE_DATAMAP(CFakeHandle, m_hEndEntity);
	DECLARE_DATAMAP(float, m_flFireTime);
	DECLARE_DATAMAP(int, m_nDissolveType);
	DECLARE_DATAMAP(float, m_flDamage);

	
};

inline void CE_CBeam::SetWidth( float width )				
{
	Assert( width <= MAX_BEAM_WIDTH );
	m_fWidth = MIN( MAX_BEAM_WIDTH, width );
}

inline void CE_CBeam::SetEndWidth( float endWidth )		
{ 
	Assert( endWidth <= MAX_BEAM_WIDTH );
	m_fEndWidth	= MIN( MAX_BEAM_WIDTH, endWidth );
}

inline void CE_CBeam::SetFadeLength( float fadeLength )	
{ 
	m_fFadeLength = fadeLength; 
}

inline void CE_CBeam::SetNoise( float amplitude )			
{ 
	m_fAmplitude = amplitude; 
}

inline void CE_CBeam::SetColor( int r, int g, int b )		
{ 
	SetRenderColor( r, g, b, GetRenderColor().a );
}

inline void CE_CBeam::SetBrightness( int brightness )		
{ 
	SetRenderColorA( brightness ); 
}

inline void CE_CBeam::SetFrame( float frame )				
{ 
	m_fStartFrame = frame; 
}

inline void CE_CBeam::SetScrollRate( int speed )			
{ 
	m_fSpeed = speed; 
}

inline void CE_CBeam::SetTexture( int spriteIndex )		
{ 
	SetModelIndex( spriteIndex ); 
}

inline void CE_CBeam::SetHaloTexture( int spriteIndex )	
{ 
	m_nHaloIndex = spriteIndex; 
}

inline void CE_CBeam::SetHaloScale( float haloScale )		
{ 
	m_fHaloScale = haloScale; 
}

inline void CE_CBeam::SetStartPos( const Vector &pos ) 
{ 
	SetLocalOrigin( pos );
}

inline void CE_CBeam::SetEndPos( const Vector &pos ) 
{ 
	m_vecEndPos = pos; 
}

inline void CE_CBeam::SetStartAttachment( int attachment )	
{
	Assert( (attachment & ~ATTACHMENT_INDEX_MASK) == 0 );
	Set_m_nAttachIndex(0, attachment);
}

inline void CE_CBeam::SetEndAttachment( int attachment )		
{ 
	Assert( (attachment & ~ATTACHMENT_INDEX_MASK) == 0 );
	int v = m_nNumBeamEnts;
	Set_m_nAttachIndex(v-1, attachment);
}

inline CEntity *CE_CBeam::GetStartEntity( void ) 
{ 
	CEntity *handle = Get_m_hAttachEntity(0);
	return handle;
}

inline CEntity *CE_CBeam::GetEndEntity( void )	
{ 
	int v = m_nNumBeamEnts;
	CEntity *handle = Get_m_hAttachEntity(v-1);
	return handle;
}

inline void CE_CBeam::LiveForTime( float time ) 
{ 
	SetThink(&CE_CBeam::SUB_Remove); 
	SetNextThink( gpGlobals->curtime + time ); 
}

inline void	CE_CBeam::SetFireTime( float flFireTime )		
{ 
	m_flFireTime = flFireTime; 
}

inline void CE_CBeam::SetBeamFlags( int flags )	
{ 
	Assert( flags < (1 << NUM_BEAM_FLAGS) );
	m_nBeamFlags = flags;
}

// Start/End Entity is encoded as 12 bits of entity index, and 4 bits of attachment (4:12)
#define BEAMENT_ENTITY(x)		((x)&0xFFF)
#define BEAMENT_ATTACHMENT(x)	(((x)>>12)&0xF)


// Beam types, encoded as a byte
enum 
{
	BEAM_POINTS = 0,
	BEAM_ENTPOINT,
	BEAM_ENTS,
	BEAM_HOSE,
	BEAM_SPLINE,
	BEAM_LASER,
	NUM_BEAM_TYPES
};



#endif
