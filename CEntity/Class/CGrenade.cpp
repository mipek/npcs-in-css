
#include "CGrenade.h"



// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"


CE_LINK_ENTITY_TO_CLASS(CBaseGrenade, CE_Grenade);

SH_DECL_MANUALHOOK2_void(Explode, 0, 0, 0, trace_t*, int);
DECLARE_HOOK(Explode, CE_Grenade);
DECLARE_DEFAULTHANDLER_void(CE_Grenade, Explode, (trace_t *pTrace, int dmgType), (pTrace, dmgType));

SH_DECL_MANUALHOOK0_void(Detonate, 0, 0, 0);
DECLARE_HOOK(Detonate, CE_Grenade);
DECLARE_DEFAULTHANDLER_void(CE_Grenade, Detonate, (), ());

SH_DECL_MANUALHOOK0(GetBlastForce, 0, 0, 0, Vector);
DECLARE_HOOK(GetBlastForce, CE_Grenade);
DECLARE_DEFAULTHANDLER(CE_Grenade, GetBlastForce, Vector, (), ());

SH_DECL_MANUALHOOK0(GetShakeAmplitude, 0, 0, 0, float);
DECLARE_HOOK(GetShakeAmplitude, CE_Grenade);
DECLARE_DEFAULTHANDLER(CE_Grenade, GetShakeAmplitude, float, (), ());

SH_DECL_MANUALHOOK0(GetShakeRadius, 0, 0, 0, float);
DECLARE_HOOK(GetShakeRadius, CE_Grenade);
DECLARE_DEFAULTHANDLER(CE_Grenade, GetShakeRadius, float, (), ());

DECLARE_DETOUR(BounceTouch, CE_Grenade);
DECLARE_DEFAULTHANDLER_DETOUR_void(CE_Grenade, BounceTouch, (CBaseEntity *pOther),(pOther));

DECLARE_DETOUR(ExplodeTouch, CE_Grenade);
DECLARE_DEFAULTHANDLER_DETOUR_void(CE_Grenade, ExplodeTouch, (CBaseEntity *pOther),(pOther));

DECLARE_DETOUR(ThumbleThink, CE_Grenade);
DECLARE_DEFAULTHANDLER_DETOUR_void(CE_Grenade, ThumbleThink, (), ());

DECLARE_DETOUR(DangerSoundThink, CE_Grenade);
DECLARE_DEFAULTHANDLER_DETOUR_void(CE_Grenade, DangerSoundThink, (), ());

// Sendprops
DEFINE_PROP(m_flDamage, CE_Grenade);
DEFINE_PROP(m_DmgRadius, CE_Grenade);
DEFINE_PROP(m_hThrower, CE_Grenade);

// Datamaps
DEFINE_PROP(m_flDetonateTime, CE_Grenade);
DEFINE_PROP(m_flWarnAITime, CE_Grenade);
DEFINE_PROP(m_bHasWarnedAI, CE_Grenade);

void CE_Grenade::PostConstructor()
{
	BaseClass::PostConstructor();
	m_hOriginalThrower.offset = m_hThrower.offset + 4;
	m_hOriginalThrower.ptr = (CFakeHandle *)(((uint8_t *)(BaseEntity())) + m_hOriginalThrower.offset);
}

CCombatCharacter *CE_Grenade::GetThrower( void )
{
	CCombatCharacter *pResult = ToBaseCombatCharacter( m_hThrower );
	if ( !pResult && GetOwnerEntity() != NULL )
	{
		pResult = ToBaseCombatCharacter( GetOwnerEntity() );
	}
	return pResult;
}

void CE_Grenade::DetonateUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
{
	SetThink( &CE_Grenade::Detonate );
	SetNextThink( gpGlobals->curtime );
}


void CE_Grenade::SetThrower( CBaseEntity *pThrower )
{
	m_hThrower.ptr->Set(pThrower);

	// if this is the first thrower, set it as the original thrower
	if ( m_hOriginalThrower.ptr->Get() == NULL)
	{
		m_hOriginalThrower.ptr->Set(pThrower);
	}
}

void CE_HEGrenade_Fix::Explode( trace_t *pTrace, int bitsDamageType )
{
	BaseClass::Explode(pTrace, bitsDamageType);

	SetNextThink(gpGlobals->curtime + 0.1f);
	SetThink(&CEntity::SUB_Remove);
}

// from basegrenade_contact.cpp
class CBaseGrenadeContact : public CE_Grenade
{
public:
	CE_DECLARE_CLASS(CBaseGrenadeContact, CE_Grenade);

	void Spawn( void );
	void Precache( void );

private:
	void ExplodeTouchCENT(CEntity *pOther)
	{
		ExplodeTouch(pOther->BaseEntity());
	}
};

extern ConVar sk_plr_dmg_grenade; //grenade_frag.cpp
LINK_ENTITY_TO_CUSTOM_CLASS( npc_contactgrenade, hegrenade_projectile, CBaseGrenadeContact );

void CBaseGrenadeContact::Spawn( void )
{
	// point sized, solid, bouncing
	SetMoveType( MOVETYPE_FLYGRAVITY, MOVECOLLIDE_FLY_BOUNCE );
	SetSolid( SOLID_BBOX );
	SetCollisionGroup( COLLISION_GROUP_PROJECTILE );
	SetModel( "models/weapons/w_grenade.mdl" );	// BUG: wrong model

	UTIL_SetSize(BaseEntity(), vec3_origin, vec3_origin);

	// contact grenades arc lower
	SetGravity( UTIL_ScaleForGravity( 400 ) );	// use a lower gravity for grenades to make them easier to see

	QAngle angles;
	VectorAngles(GetAbsVelocity(), angles);
	SetLocalAngles( angles );
	
	// make NPCs afaid of it while in the air
	SetThink( &CE_Grenade::DangerSoundThink );
	SetNextThink( gpGlobals->curtime );
	
	// Tumble in air
	QAngle vecAngVelocity( enginerandom->RandomFloat ( -100, -500 ), 0, 0 );
	SetLocalAngularVelocity( vecAngVelocity );

	// Explode on contact
	SetTouch( &CBaseGrenadeContact::ExplodeTouchCENT );

	m_flDamage = sk_plr_dmg_grenade.GetFloat();

	// Allow player to blow this puppy up in the air
	m_takedamage	= DAMAGE_YES;

	//m_iszBounceSound = NULL_STRING; CE_TODO: add CGrenade datamap?
}


void CBaseGrenadeContact::Precache( void )
{
	BaseClass::Precache( );

	PrecacheModel("models/weapons/w_grenade.mdl");
}