#include "CEntity.h"
#include "CCycler_Fix.h"
#include "CAI_NPC.h"

// Must be the last file included
#include "memdbgon.h"

class CNPC_Puppet : public CE_Cycler_Fix
{
public:
	CE_DECLARE_CLASS( CNPC_Puppet, CE_Cycler_Fix );

	virtual void Spawn( void );
	virtual void Precache( void );

	void	InputSetAnimationTarget( inputdata_t &inputdata );

private:

	string_t	m_sAnimTargetname;
	string_t	m_sAnimAttachmentName;

	CFakeHandle m_hAnimationTarget;	// NPC that will drive what animation we're playing
	int m_nTargetAttachment;	// Attachment point to match to on the target

	DECLARE_DATADESC();
};

LINK_ENTITY_TO_CUSTOM_CLASS( npc_puppet, cycler, CNPC_Puppet );

BEGIN_DATADESC( CNPC_Puppet )
					DEFINE_KEYFIELD( m_sAnimTargetname, FIELD_STRING,	"animationtarget" ),
					DEFINE_KEYFIELD( m_sAnimAttachmentName, FIELD_STRING,	"attachmentname" ),

					DEFINE_FIELD( m_nTargetAttachment, FIELD_INTEGER ),
					DEFINE_FIELD( m_hAnimationTarget, FIELD_EHANDLE ),
					DEFINE_INPUTFUNC( FIELD_STRING, "SetAnimationTarget", InputSetAnimationTarget ),
END_DATADESC()


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Puppet::Precache( void )
{
	BaseClass::Precache();
	PrecacheModel( STRING( GetModelName() ) );
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CNPC_Puppet::Spawn( void )
{
	BaseClass::Spawn();

	Precache();

	SetModel( STRING( GetModelName() ) );

	NPCInit();

	SetHealth( 100 );

	// Find our animation target
	CEntity *pTarget = g_helpfunc.FindEntityByName( (CBaseEntity *)NULL, m_sAnimTargetname );
	m_hAnimationTarget.Set((pTarget)?pTarget->BaseEntity():NULL);
	if ( pTarget )
	{
		CAnimating *pAnimating = pTarget->GetBaseAnimating();
		if ( pAnimating )
		{
			m_nTargetAttachment = pAnimating->LookupAttachment( STRING( m_sAnimAttachmentName ) );
		}
	}

	// Always be scripted
	SetInAScript( true );
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &inputdata - 
//-----------------------------------------------------------------------------
void CNPC_Puppet::InputSetAnimationTarget( inputdata_t &inputdata )
{
	// Take the new name
	m_sAnimTargetname = MAKE_STRING( inputdata.value.String() );

	// Find our animation target
	CEntity *pTarget = g_helpfunc.FindEntityByName( (CBaseEntity *)NULL, m_sAnimTargetname );
	if ( pTarget == NULL )
	{
		Warning("Failed to find animation target %s for npc_puppet (%s)\n", STRING( m_sAnimTargetname ), GetEntityName());
		return;
	}

	m_hAnimationTarget.Set(pTarget->BaseEntity());

	CAnimating *pAnimating = pTarget->GetBaseAnimating();
	if ( pAnimating )
	{
		// Cache off our target attachment
		m_nTargetAttachment = pAnimating->LookupAttachment( STRING( m_sAnimAttachmentName ) );
	}

	// Stuff us at the owner's core for visibility reasons
	SetParent( pTarget->BaseEntity() );
	SetLocalOrigin( vec3_origin );
	SetLocalAngles( vec3_angle );
}