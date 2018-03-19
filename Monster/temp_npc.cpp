
#include "CEntity.h"
#include "CPlayer.h"
#include "CCycler_Fix.h"


abstract_class CE_Temp_NPC : public CE_Cycler_Fix
{
public:
	CE_DECLARE_CLASS(CE_Temp_NPC, CE_Cycler_Fix);

	virtual void Spawn()
	{
		Precache();
		BaseClass::Spawn();
		SetModel(STRING(GetModelName()));

		SetHullType(HULL_HUMAN);
		SetHullSizeNormal();

		SetSolid( SOLID_BBOX );
		AddSolidFlags( FSOLID_NOT_STANDABLE );
		SetMoveType( MOVETYPE_STEP );
		SetBloodColor( DONT_BLEED );

		m_NPCState			= NPC_STATE_NONE;
		SetImpactEnergyScale( 0.0f ); 

		CapabilitiesClear();
		NPCInit();
	}

	virtual Class_T	Classify ( void )
	{
		return CLASS_PLAYER_ALLY_VITAL;
	}

	virtual int OnTakeDamage(const CTakeDamageInfo& info)
	{
		return 0;
	}

	virtual float MaxYawSpeed( void )
	{
		return 0.0f;
	}
	virtual bool CanBeAnEnemyOf( CBaseEntity *pEnemy )
	{
		return false;
	}

};

class NPC_Gman : public CE_Temp_NPC
{
public:
	CE_DECLARE_CLASS(NPC_Gman, CE_Temp_NPC);

	void Precache()
	{
		PrecacheModel("models/gman.mdl");
		SetModelName(AllocPooledString("models/gman.mdl"));
		BaseClass::Precache();
	}
};

LINK_ENTITY_TO_CUSTOM_CLASS( npc_gman, cycler, NPC_Gman);


class NPC_Alyx : public CE_Temp_NPC
{
public:
	CE_DECLARE_CLASS(NPC_Alyx, CE_Temp_NPC);

	void Precache()
	{
		PrecacheModel("models/alyx.mdl");
		SetModelName(AllocPooledString("models/alyx.mdl"));
		BaseClass::Precache();
	}
};

LINK_ENTITY_TO_CUSTOM_CLASS( npc_alyx, cycler, NPC_Alyx);

class NPC_Barney : public CE_Temp_NPC
{
public:
	CE_DECLARE_CLASS(NPC_Barney, CE_Temp_NPC);
	DECLARE_DATADESC();

	void Precache()
	{
		PrecacheModel("models/barney.mdl");
		SetModelName(AllocPooledString("models/barney.mdl"));
		BaseClass::Precache();
	}

	void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value )
	{
		BaseClass::Use(pActivator, pCaller, useType, value);
		m_OnPlayerUse.FireOutput( pActivator, pCaller );
	}

	COutputEvent				m_OnPlayerUse;
};

BEGIN_DATADESC( NPC_Barney )
	DEFINE_OUTPUT( m_OnPlayerUse, "OnPlayerUse" ),
END_DATADESC()

LINK_ENTITY_TO_CUSTOM_CLASS( npc_barney, cycler, NPC_Barney);

class NPC_Monk : public CE_Temp_NPC //set idle_slam?
{
public:
	CE_DECLARE_CLASS(NPC_Monk, CE_Temp_NPC);

	void Precache()
	{
		PrecacheModel("models/monk.mdl");
		SetModelName(AllocPooledString("models/monk.mdl"));
		BaseClass::Precache();
	}
};

LINK_ENTITY_TO_CUSTOM_CLASS( npc_monk, cycler, NPC_Monk);

