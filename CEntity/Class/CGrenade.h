
#ifndef _INCLUDE_CGRENADE_H_
#define _INCLUDE_CGRENADE_H_

#include "CEntity.h"
#include "CAnimating.h"

class CE_Grenade : public CAnimating
{
public:
	CE_DECLARE_CLASS( CE_Grenade, CAnimating );

	void PostConstructor();

	CCombatCharacter *GetThrower( void );
	void DetonateUse( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	void SetThrower( CBaseEntity *pThrower );

public:
	virtual void Explode( trace_t *pTrace, int bitsDamageType );
	virtual void Detonate( void );
	virtual Vector GetBlastForce( void );
	virtual float GetShakeAmplitude( void );
	virtual float GetShakeRadius( void );
	virtual void BounceTouch(CBaseEntity *pOther);
	virtual void ExplodeTouch(CBaseEntity *pOther);
	virtual void ThumbleThink();
	virtual void DangerSoundThink();

public:
	float GetDamage()
	{
		return m_flDamage;
	}
	float GetDamageRadius()
	{
		return m_DmgRadius;
	}

	void SetDamage(float flDamage)
	{
		m_flDamage = flDamage;
	}

	void SetDamageRadius(float flDamageRadius)
	{
		m_DmgRadius = flDamageRadius;
	}

public:
	int ObjectCaps() 
	{ 
		return (BaseClass::ObjectCaps() | FCAP_IMPULSE_USE | FCAP_USE_IN_RADIUS);
	}

public:
	DECLARE_DEFAULTHEADER(Explode, void, (trace_t*, int));
	DECLARE_DEFAULTHEADER(Detonate, void, ());
	DECLARE_DEFAULTHEADER(GetBlastForce, Vector, ());
	DECLARE_DEFAULTHEADER(GetShakeAmplitude, float, ());
	DECLARE_DEFAULTHEADER(GetShakeRadius, float, ());
	DECLARE_DEFAULTHEADER_DETOUR(BounceTouch, void, (CBaseEntity *pOther));
	DECLARE_DEFAULTHEADER_DETOUR(ExplodeTouch, void, (CBaseEntity *pOther));
	DECLARE_DEFAULTHEADER_DETOUR(ThumbleThink, void, ());
	DECLARE_DEFAULTHEADER_DETOUR(DangerSoundThink, void, ());

protected: //Sendprops
	DECLARE_SENDPROP(float, m_flDamage);
	DECLARE_SENDPROP(float, m_DmgRadius);
	DECLARE_SENDPROP(CFakeHandle, m_hThrower);

public: //Datamaps
	DECLARE_DATAMAP_OFFSET(CFakeHandle,m_hOriginalThrower);
	DECLARE_DATAMAP(float,m_flDetonateTime);
	DECLARE_DATAMAP(float,m_flWarnAITime);
	DECLARE_DATAMAP(bool,m_bHasWarnedAI);

};


#endif

