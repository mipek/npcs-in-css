
#include "CRagdollProp.h"
#include "CEntity.h"
#include "cutil.h"

class CRagdollProp;
CE_LINK_ENTITY_TO_CLASS(CRagdollProp, CE_CRagdollProp);

//SH_DECL_MANUALHOOK1_void(SetDamageEntity, 0, 0, 0, CBaseEntity *);
//DECLARE_HOOK(SetDamageEntity, CE_CRagdollProp);
//DECLARE_DEFAULTHANDLER_void(CE_CRagdollProp, SetDamageEntity, (CBaseEntity *pent), (pent));

void CE_CRagdollProp::SetDamageEntity ( CBaseEntity *pent )
{
	static int offset = NULL;
	if(!offset)
	{
		if(!g_pGameConf->GetOffset("SetDamageEntity", &offset))
		{
			assert(0);
			return;
		}
	}

	CBaseEntity *pThisBase = BaseEntity();
	void **this_ptr = *reinterpret_cast<void ***>(&pThisBase);
	void **vtable = *reinterpret_cast<void ***>(pThisBase);
	void *vfunc = vtable[offset];

	union
	{
		void (VEmptyClass::*mfpnew)(CBaseEntity*);
		void *addr;
	} u;
	u.addr = vfunc;

	(reinterpret_cast<VEmptyClass *>(this_ptr)->*u.mfpnew)(pent);
}