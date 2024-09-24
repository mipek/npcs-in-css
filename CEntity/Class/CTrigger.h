
#ifndef _INCLUDE_CTRIGGER_H_
#define _INCLUDE_CTRIGGER_H_

#include "CEntity.h"
#include "CToggle.h"

class CTrigger : public CToggle
{
public:
	CE_DECLARE_CLASS(CTrigger, CToggle);

	bool IsTouching( CEntity *pOther );
	void Enable( void );
	void Disable( void );

public:
	virtual bool PassesTriggerFilters(CBaseEntity *pOther);

public:
	DECLARE_DEFAULTHEADER(PassesTriggerFilters, bool, (CBaseEntity *pOther));


protected:
	DECLARE_DATAMAP(CUtlVector< EHANDLE >,m_hTouchingEntities);
	DECLARE_DATAMAP(bool ,m_bDisabled);



};

class CTriggerHurt : public CTrigger
{
public:
	CE_DECLARE_CLASS(CTriggerHurt, CTrigger);

	bool HurtEntity( CEntity *pOther, float damage );
	int HurtAllTouchers( float dt );

	enum
	{
		DAMAGEMODEL_NORMAL = 0,
		DAMAGEMODEL_DOUBLE_FORGIVENESS,
	};

protected:
	DECLARE_DATAMAP(float ,m_flLastDmgTime);
	DECLARE_DATAMAP(CUtlVector<EHANDLE> ,m_hurtEntities);
	DECLARE_DATAMAP(int ,m_damageModel);
	DECLARE_DATAMAP(float ,m_flDmgResetTime);
	DECLARE_DATAMAP(float ,m_flDamage);
	DECLARE_DATAMAP(float ,m_flOriginalDamage);
	DECLARE_DATAMAP(float ,m_flDamageCap);
	DECLARE_DATAMAP(int ,m_bitsDamageInflict);
	DECLARE_DATAMAP(bool ,m_bNoDmgForce);
	DECLARE_DATAMAP(COutputEvent ,m_OnHurtPlayer);
	DECLARE_DATAMAP(COutputEvent ,m_OnHurt);

};

#endif
