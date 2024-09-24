
#ifndef _INCLUDE_CRAGDOLLPROP_H_
#define _INCLUDE_CRAGDOLLPROP_H_

#include "CAnimating.h"
#include "player_pickup.h"
#include "ragdoll_shared.h"

class CE_CPhysicsProp;


class CE_CRagdollProp : public CAnimating
{
public:
	CE_DECLARE_CLASS( CE_CRagdollProp, CAnimating );

	CE_CRagdollProp();

	inline ragdoll_t *GetRagdoll( void ) { return &(*(m_ragdoll.ptr)); }

	void SetDamageEntity( CEntity *pEntity );
	void SetBlendWeight( float weight ) { m_flBlendWeight = weight; }
	void SetOverlaySequence( Activity activity );

	void DisableAutoFade();
	void RecheckCollisionFilter();

	void FadeOut( float flDelay = 0, float fadeTime = -1 );
	bool IsFading();

	void FadeOutThink();

protected: // Sendprop
	DECLARE_SENDPROP(float, m_flBlendWeight);
	DECLARE_SENDPROP(int, m_nOverlaySequence);

protected: // Datamaps
	DECLARE_DATAMAP_OFFSET(ragdoll_t, m_ragdoll);
	DECLARE_DATAMAP(CFakeHandle, m_hDamageEntity);
	DECLARE_DATAMAP(float, m_flDefaultFadeScale);
	DECLARE_DATAMAP(float, m_flFadeTime);
	DECLARE_DATAMAP(float, m_flFadeOutStartTime);



};


class CE_CRagdollPropAttached : public CE_CRagdollProp
{
public:
	CE_DECLARE_CLASS( CE_CRagdollPropAttached, CE_CRagdollProp );

	CE_CRagdollPropAttached();

	void DetachOnNextUpdate();

protected: // Datamaps
	DECLARE_DATAMAP(bool, m_bShouldDetach);
};

void DetachAttachedRagdoll( CEntity *pRagdollIn );

#endif