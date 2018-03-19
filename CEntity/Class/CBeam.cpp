
#include "CBeam.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



CE_LINK_ENTITY_TO_CLASS(CBeam, CE_CBeam);

DEFINE_PROP(m_fWidth, CE_CBeam);
DEFINE_PROP(m_fEndWidth, CE_CBeam);
DEFINE_PROP(m_fFadeLength, CE_CBeam);
DEFINE_PROP(m_fAmplitude, CE_CBeam);
DEFINE_PROP(m_fStartFrame, CE_CBeam);
DEFINE_PROP(m_fSpeed, CE_CBeam);
DEFINE_PROP(m_hAttachEntity, CE_CBeam);
DEFINE_PROP(m_nAttachIndex, CE_CBeam);
DEFINE_PROP(m_nHaloIndex, CE_CBeam);
DEFINE_PROP(m_fHaloScale, CE_CBeam);
DEFINE_PROP(m_nBeamType, CE_CBeam);
DEFINE_PROP(m_nBeamFlags, CE_CBeam);
DEFINE_PROP(m_nNumBeamEnts, CE_CBeam);
DEFINE_PROP(m_vecEndPos, CE_CBeam);




DEFINE_PROP(m_hEndEntity, CE_CBeam);
DEFINE_PROP(m_flFireTime, CE_CBeam);
DEFINE_PROP(m_nDissolveType, CE_CBeam);
DEFINE_PROP(m_flDamage, CE_CBeam);



#define BEAM_DEFAULT_HALO_SCALE		10


CEntity *CE_CBeam::Get_m_hAttachEntity(int index)
{
	CBaseHandle &hndl = *(CBaseHandle *)(((uint8_t *)(BaseEntity())) + m_hAttachEntity.offset + (index*4));
	return CEntity::Instance(hndl);
}

void CE_CBeam::Set_m_hAttachEntity(int index, CBaseEntity *pEntity)
{
	edict_t *pEdict = servergameents->BaseEntityToEdict(pEntity);
	CBaseHandle &hndl = *(CBaseHandle *)(((uint8_t *)(BaseEntity())) + m_hAttachEntity.offset + (index*4));
	hndl.Set((pEdict) ? pEdict->GetIServerEntity() : NULL);
}

int CE_CBeam::Get_m_nAttachIndex(int index)
{
	return *(int *)(((uint8_t *)(BaseEntity())) + m_nAttachIndex.offset + (index*4));
}

void CE_CBeam::Set_m_nAttachIndex(int index, int value)
{
	*(int *)(((uint8_t *)(BaseEntity())) + m_nAttachIndex.offset + (index*4)) = value;
}


CE_CBeam *CE_CBeam::BeamCreate( const char *pSpriteName, float width )
{
	// Create a new entity with CBeam private data
	CE_CBeam *pBeam = (CE_CBeam *)CreateEntityByName("beam");
	pBeam->BeamInit( pSpriteName, width );

	return pBeam;
}

void CE_CBeam::BeamInit( const char *pSpriteName, float width )
{
	SetColor( 255, 255, 255 );
	SetBrightness( 255 );
	SetNoise( 0 );
	SetFrame( 0 );
	SetScrollRate( 0 );
	SetModelName( MAKE_STRING( pSpriteName ) );
	SetRenderMode( kRenderTransTexture );
	SetTexture( engine->PrecacheModel( pSpriteName ) );
	SetWidth( width );
	SetEndWidth( width );
	SetFadeLength( 0 );			// No fade

	for (int i=0;i<MAX_BEAM_ENTS;i++)
	{
		Set_m_hAttachEntity(i,NULL);
		Set_m_nAttachIndex(i, 0);
	}

	m_nHaloIndex	= 0;
	m_fHaloScale	= BEAM_DEFAULT_HALO_SCALE;
	m_nBeamType		= 0;
	m_nBeamFlags    = 0;
}

void CE_CBeam::PointEntInit( const Vector &start, CEntity *pEndEntity )
{
	SetType( BEAM_ENTPOINT );
	//SetType(BEAM_POINTS);
	m_nNumBeamEnts = 2;
	SetStartPos( start );
	SetEndEntity( pEndEntity );
	SetStartAttachment( 0 );
	SetEndAttachment( 0 );
	RelinkBeam();
}

void CE_CBeam::EntsInit( CEntity *pStartEntity, CEntity *pEndEntity )
{
	SetType( BEAM_ENTS );
	m_nNumBeamEnts = 2;
	SetStartEntity( pStartEntity );
	SetEndEntity( pEndEntity );
	SetStartAttachment( 0 );
	SetEndAttachment( 0 );
	RelinkBeam();
}

void CE_CBeam::SetType( int type )		
{ 
	Assert( type < NUM_BEAM_TYPES );
	m_nBeamType = type;
}

int CE_CBeam::GetType( void ) const 
{ 
	return m_nBeamType;
}


void CE_CBeam::SetStartEntity( CEntity *pEntity )
{ 
	Assert( m_nNumBeamEnts >= 2 );
	Set_m_hAttachEntity(0, pEntity->BaseEntity());
	SetOwnerEntity( pEntity->BaseEntity() );
	RelinkBeam();
	pEntity->AddEFlags( EFL_FORCE_CHECK_TRANSMIT );
}

void CE_CBeam::SetEndEntity( CEntity *pEntity ) 
{ 
	Assert( m_nNumBeamEnts >= 2 );
	int v = m_nNumBeamEnts;
	Set_m_hAttachEntity(v-1, pEntity->BaseEntity());
	m_hEndEntity.ptr->Set(pEntity->BaseEntity());
	RelinkBeam();
	pEntity->AddEFlags( EFL_FORCE_CHECK_TRANSMIT );
}

void CE_CBeam::RelinkBeam( void )
{
	// FIXME: Why doesn't this just define the absbox too?
	// It seems that we don't need to recompute the absbox
	// in CBaseEntity::SetObjectCollisionBox, in fact the absbox
	// computed there seems way too big
	Vector startPos = GetAbsStartPos(), endPos = GetAbsEndPos();

	Vector vecAbsExtra1, vecAbsExtra2;
	bool bUseExtraPoints = false;

	// UNDONE: Should we do this to make the boxes smaller?
	//SetAbsOrigin( startPos );

	Vector vecBeamMin, vecBeamMax;
	VectorMin( startPos, endPos, vecBeamMin );
	VectorMax( startPos, endPos, vecBeamMax );

	if ( bUseExtraPoints )
	{
		VectorMin( vecBeamMin, vecAbsExtra1, vecBeamMin );
		VectorMin( vecBeamMin, vecAbsExtra2, vecBeamMin );
		VectorMax( vecBeamMax, vecAbsExtra1, vecBeamMax );
		VectorMax( vecBeamMax, vecAbsExtra2, vecBeamMax );
	}

	SetCollisionBounds( vecBeamMin - GetAbsOrigin(), vecBeamMax - GetAbsOrigin() );
}

const Vector &CE_CBeam::GetAbsStartPos( void )
{
	CEntity *ent = GetStartEntity();
	if ( GetType() == BEAM_ENTS && ent )
	{
		return ent->GetAbsOrigin();
	}
	return GetAbsOrigin();
}


const Vector &CE_CBeam::GetAbsEndPos( void )
{
	CEntity *ent = GetEndEntity();
	if ( GetType() != BEAM_POINTS && GetType() != BEAM_HOSE && ent ) 
	{
		return ent->GetAbsOrigin();
	}

	if (!const_cast<CE_CBeam*>(this)->GetMoveParent())
		return m_vecEndPos;

	// FIXME: Cache this off?
	static Vector vecAbsPos;
	VectorTransform( m_vecEndPos, EntityToWorldTransform(), vecAbsPos );
	return vecAbsPos;
}

void CE_CBeam::SetBeamFlag( int flag )		
{ 
	m_nBeamFlags |= flag;
}

void CE_CBeam::PointsInit( const Vector &start, const Vector &end )
{
	SetType( BEAM_POINTS );
	m_nNumBeamEnts = 2;
	SetStartPos( start );
	SetEndPos( end );
	SetStartAttachment( 0 );
	SetEndAttachment( 0 );
	RelinkBeam();
}

void CE_CBeam::SetAbsEndPos( const Vector &pos )
{
	if (!GetMoveParent())
	{
		SetEndPos( pos );
		return;
	}

	Vector vecLocalPos;
	matrix3x4_t worldToBeam;
	MatrixInvert( EntityToWorldTransform(), worldToBeam );
	VectorTransform( pos, worldToBeam, vecLocalPos );
	SetEndPos( vecLocalPos );
}

void CE_CBeam::BeamDamage( trace_t *ptr )
{
	RelinkBeam();
	if ( ptr->fraction != 1.0 && ptr->m_pEnt != NULL )
	{
		CEntity *pHit = CEntity::Instance(ptr->m_pEnt);
		if ( pHit )
		{
			ClearMultiDamage();
			Vector dir = ptr->endpos - GetAbsOrigin();
			VectorNormalize( dir );
			int nDamageType = DMG_ENERGYBEAM;

			if (m_nDissolveType == 0)
			{
				nDamageType = DMG_DISSOLVE;
			}
			else if ( m_nDissolveType > 0 )
			{
				nDamageType = DMG_DISSOLVE | DMG_SHOCK; 
			}

			CTakeDamageInfo info( BaseEntity(), BaseEntity(), m_flDamage * (gpGlobals->curtime - m_flFireTime), nDamageType );
			CalculateMeleeDamageForce( &info, dir, ptr->endpos );
			pHit->DispatchTraceAttack( info, dir, ptr );
			ApplyMultiDamage();
			if ( HasSpawnFlags( SF_BEAM_DECALS ) )
			{
				if ( pHit->IsBSPModel() )
				{
					UTIL_DecalTrace( ptr, GetDecalName() );
				}
			}
		}
	}
	m_flFireTime = gpGlobals->curtime;
}


void CE_CBeam::DoSparks( const Vector &start, const Vector &end )
{
	if ( HasSpawnFlags(SF_BEAM_SPARKSTART|SF_BEAM_SPARKEND) )
	{
		if ( HasSpawnFlags( SF_BEAM_SPARKSTART ) )
		{
			g_pEffects->Sparks( start );
		}
		if ( HasSpawnFlags( SF_BEAM_SPARKEND ) )
		{
			g_pEffects->Sparks( end );
		}
	}
}

CEntity *CE_CBeam::RandomTargetname( const char *szName )
{
	int total = 0;

	CEntity *pEntity = NULL;
	CEntity *pNewEntity = NULL;
	while ((pNewEntity = g_helpfunc.FindEntityByName( pNewEntity, szName )) != NULL)
	{
		total++;
		if (enginerandom->RandomInt(0,total-1) < 1)
			pEntity = pNewEntity;
	}
	return pEntity;
}

