/**
* =============================================================================
* CEntity Entity Handling Framework
* Copyright (C) 2011 Matt Woodrow.  All rights reserved.
* =============================================================================
*
* This program is free software; you can redistribute it and/or modify it under
* the terms of the GNU General Public License, version 3.0, as published by the
* Free Software Foundation.
* 
* This program is distributed in the hope that it will be useful, but WITHOUT
* ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
* FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
* details.
*
* You should have received a copy of the GNU General Public License along with
* this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _INCLUDE_MACROS_H_
#define _INCLUDE_MACROS_H_

#include "CEntityBase.h"
#include "CEntity.h"
#include "../CDetour/detours.h"


#define CE_CUSTOM_ENTITY()\
	bool IsCustomEntity() { return true; }

#undef CE_DECLARE_CLASS
#define CE_DECLARE_CLASS( className, baseClassName ) \
	typedef baseClassName BaseClass; \
	typedef className ThisClass; \
	virtual bool IsBase() { return false; } \
	virtual void InitDataMap() \
	{ \
		if (BaseClass::IsBase()) \
		{ \
			ThisClass::m_DataMap.baseMap = NULL; \
		} \
		datamap_t *pMap = gamehelpers->GetDataMap(BaseEntity()); \
		if (eventFuncs == NULL) \
		{ \
			if (pMap) \
			{ \
				typedescription_t *typedesc = gamehelpers->FindInDataMap(pMap, "m_OnUser1"); \
				if (typedesc != NULL) \
					eventFuncs = typedesc->pSaveRestoreOps; \
			} \
		} \
		if (eventFuncs == NULL) \
			g_pSM->LogError(myself, "[CENTITY] Could not lookup ISaveRestoreOps for Outputs"); \
		UTIL_PatchOutputRestoreOps(ThisClass::GetDataDescMap()); \
		BaseClass::InitDataMap();\
	}

#undef CE_DECLARE_CLASS_NOBASE
#define CE_DECLARE_CLASS_NOBASE( className ) \
	typedef className ThisClass; \
	virtual bool IsBase() { return true; } \
	virtual void InitDataMap() {};


#undef DECLARE_PREDICTABLE
#define DECLARE_PREDICTABLE()

#define BEGIN_DATADESC_CENTITY( className ) \
datamap_t className::m_DataMap = { 0, 0, #className, NULL }; \
datamap_t *className::GetDataDescMap(void) { \
    m_DataMap.baseMap = BaseClass::GetDataDescMap(); \
    return &m_DataMap; \
} \
datamap_t *className::GetBaseMap() { datamap_t *pResult; DataMapAccess((BaseClass *)NULL, &pResult); return pResult; } \
BEGIN_DATADESC_GUTS(className)  

class IHookTracker
{
public:
	IHookTracker()
	{
		m_Next = m_Head;
		m_Head = this;
	}
	virtual void ReconfigureHook(IGameConfig *pConfig) =0;
	virtual void AddHook(CEntity *pEnt) =0;
	virtual void ClearFlag(CEntity *pEnt) =0;
public:
	static IHookTracker *m_Head;
	IHookTracker *m_Next;
public:
	int m_bShouldHook;
};

#define DECLARE_HOOK(name, cl) \
class name##cl##HookTracker : public IHookTracker \
{ \
public: \
	void ReconfigureHook(IGameConfig *pConfig) \
	{ \
		int offset; \
		if (!pConfig->GetOffset(#name, &offset)) \
		{ \
			g_pSM->LogError(myself, "[CENTITY] Failed to retrieve offset %s from gamedata file", #name); \
			m_bShouldHook = false; \
		} else { \
			SH_MANUALHOOK_RECONFIGURE(name, offset, 0, 0); \
			m_bShouldHook = true; \
		} \
	} \
	void AddHook(CEntity *pEnt) \
	{ \
		cl *pThisType = dynamic_cast<cl *>(pEnt); \
		if (pThisType && m_bShouldHook) \
		{ \
			SH_ADD_MANUALVPHOOK(name, pEnt->BaseEntity(), SH_MEMBER(pThisType, &cl::Internal##name), false); \
		} \
	} \
	void ClearFlag(CEntity *pEnt) \
	{ \
		cl *pThisType = dynamic_cast<cl *>(pEnt); \
		if (pThisType) \
		{ \
			pThisType->m_bIn##name = false; \
		} \
	} \
}; \
name##cl##HookTracker name##cl##HookTrackerObj;


#define DECLARE_HOOK_SUBCLASS(name, cl, c2) \
class name##cl##HookTracker : public IHookTracker \
{ \
public: \
	void ReconfigureHook(IGameConfig *pConfig) \
	{ \
		int offset; \
		if (!pConfig->GetOffset(#name, &offset)) \
		{ \
			g_pSM->LogError(myself, "[CENTITY] Failed to retrieve offset %s from gamedata file", #name); \
			m_bShouldHook = false; \
		} else { \
			SH_MANUALHOOK_RECONFIGURE(name, offset, 0, 0); \
			m_bShouldHook = true; \
		} \
	} \
	void AddHook(CEntity *pEnt) \
	{ \
		cl *pThisType = dynamic_cast<cl *>(pEnt); \
		if (pThisType && m_bShouldHook) \
		{ \
			c2 *pThisSubType = dynamic_cast<c2 *>(pEnt->BaseEntity()); \
			if(pThisSubType) \
			{ \
				SH_ADD_MANUALVPHOOK(name, pThisSubType, SH_STATIC(Static_##name), false); \
			} \
		} \
	} \
	void ClearFlag(CEntity *pEnt) \
	{ \
		cl *pThisType = dynamic_cast<cl *>(pEnt); \
		if (pThisType) \
		{ \
			c2 *pThisSubType = dynamic_cast<c2 *>(pEnt->BaseEntity()); \
			if(pThisSubType) \
			{ \
				pThisType->m_bIn##name = false; \
			} \
		} \
	} \
}; \
name##cl##HookTracker name##cl##HookTrackerObj;




class IDetourTracker
{
public:
	IDetourTracker()
	{
		m_Next = m_Head;
		m_Head = this;
	}

	virtual void AddHook() = 0;
	virtual void RemoveHook() = 0;

	static IDetourTracker *m_Head;
	IDetourTracker *m_Next;

public:
	CDetour *m_Detour; \
};

#define DECLARE_DETOUR(name, cl) \
class name##cl##DetourTracker : public IDetourTracker \
{ \
public: \
	void AddHook() \
	{ \
		void *callback = (void *)GetCodeAddress(&cl::Internal##name); \
		void **trampoline = (void **)(&cl::name##_Actual); \
		m_Detour = CDetourManager::CreateDetour(callback, trampoline, #name); \
		if (m_Detour) \
		{ \
			m_Detour->EnableDetour(); \
		} \
	} \
	void RemoveHook() \
	{ \
		if (m_Detour) \
		{ \
			m_Detour->Destroy(); \
			m_Detour = NULL; \
		} \
	} \
}; \
name##cl##DetourTracker name##cl##DetourTrackerObj;

class ISigOffsetTracker
{
public:
	ISigOffsetTracker()
	{
		m_Next = m_Head;
		m_Head = this;
	}

	virtual void FindSig(IGameConfig *pConfig) = 0;

	static ISigOffsetTracker *m_Head;
	ISigOffsetTracker *m_Next;

public:
	void *pPointer;
};

#define DECLARE_SIGOFFSET(name) \
class name##SigOffsetTracker : public ISigOffsetTracker \
{ \
public: \
	void FindSig(IGameConfig *pConfig) \
	{ \
		if (!pConfig->GetMemSig(#name, &pPointer)) \
		{ \
			g_pSM->LogError(myself, "[CENTITY] Failed to retrieve %s from gamedata file", #name); \
		} else if (!pPointer) { \
			g_pSM->LogError(myself, "[CENTITY] Failed to retrieve pointer from %s", #name); \
		} \
	} \
}; \
name##SigOffsetTracker name##SigOffsetTrackerObj;

#define GET_SIGOFFSET(name) name##SigOffsetTrackerObj.pPointer

#define PROP_SEND 0
#define PROP_DATA 1

#define DECLARE_SENDPROP(typeof, name) DECLARE_PROP_SEND(typeof, name)
#define DECLARE_DATAMAP(typeof, name) DECLARE_PROP(typeof, name)

#define DECLARE_DATAMAP_OFFSET(typeof, name) DECLARE_PROP_OFFSET(typeof, name)



#define DEFINE_PROP(name, cl)	cl::name##PropTracker cl::name##PropTrackerObj;

template <typename T>
class Redirect;



template <typename T>
class Redirect_Send
{
public:
	Redirect_Send& operator =(const Redirect<T>& input)
	{
		//Assert(0);
		*ptr = *(input.ptr);
		if(pEdict)
			pEdict->StateChanged(offset);
		return *this;
	}
	Redirect_Send& operator =(const Redirect_Send& input)
	{
		//Assert(0);
		*ptr = *(input.ptr);
		if(pEdict)
			pEdict->StateChanged(offset);
		return *this;
	}
	Redirect_Send& operator =(const T& input)
	{
		*ptr = input;
		if(pEdict)
			pEdict->StateChanged(offset);
		return *this;
	}
	operator T& () const
	{
		return *ptr;
	}
	operator T* () const
	{
		return ptr;
	}
	T* operator->() 
	{
		return ptr;
	}
	T& operator [] (unsigned i)
	{
		return ptr[i];
	}

public:
	edict_t *pEdict;
	T* ptr;
	unsigned int offset;
};


template <typename T>
class Redirect
{
public:
	Redirect& operator =(const Redirect_Send<T>& input)
	{
		//Assert(0);
		*ptr = *(input.ptr);
		return *this;
	}
	Redirect& operator =(const Redirect& input)
	{
		//Assert(0);
		*ptr = *(input.ptr);
		return *this;
	}
	Redirect& operator =(const T& input)
	{
		*ptr = input;
		return *this;
	}
	operator T& () const
	{
		return *ptr;
	}
	operator T* () const
	{
		return ptr;
	}
	T* operator->() 
	{
		return ptr;
	}
	T& operator [] (unsigned i)
	{
		return ptr[i];
	}

public:
	T* ptr;
	unsigned int offset;
};


class CFakeHandle : public CBaseHandle
{
public:
	CEntity* Get() const
	{
		return CEntityLookup::Instance(*this);
	}
	operator CEntity*()
	{
		return Get();
	}
	operator CEntity*() const
	{
		return Get();
	}
	CEntity* operator->() const
	{
		return Get();
	}
};

template <typename T>
class CEFakeHandle : public CBaseHandle
{
public:
	T* Get() const
	{
		return (T *)CEntityLookup::Instance(*this);
	}
	operator T*()
	{
		return Get();
	}
	operator T*() const
	{
		return Get();
	}
	T* operator->() const
	{
		return Get();
	}
};


template<>
class Redirect_Send <CFakeHandle>
{
public:
	Redirect_Send& operator =(const CFakeHandle& input)
	{
		*ptr = input;
		if(pEdict)
			pEdict->StateChanged(offset);
		return *this;
	}
	bool operator ==(const CEntity *lhs)
	{
		return (*ptr == lhs);
	}
	bool operator !=(const CEntity *lhs)
	{
		return (*ptr != lhs);
	}
	operator CFakeHandle& () const
	{
		return *ptr;
	}
	operator CFakeHandle* () const
	{
		return ptr;
	}
	operator CEntity* () const
	{
		return ptr->Get();
	}
	CEntity* operator->() 
	{
		return CEntityLookup::Instance(*ptr);
	}
	bool operator == (void *rhs )
	{
		return (ptr == rhs);
	}

public:
	edict_t *pEdict;
	CFakeHandle* ptr;
	unsigned int offset;
};


template<>
class Redirect <CFakeHandle>
{
public:
	Redirect& operator =(const CFakeHandle& input)
	{
		*ptr = input;
		return *this;
	}
	bool operator ==(const CEntity *lhs)
	{
		return (*ptr == lhs);
	}
	bool operator !=(const CEntity *lhs)
	{
		return (*ptr != lhs);
	}
	operator CFakeHandle& () const
	{
		return *ptr;
	}
	operator CFakeHandle* () const
	{
		return ptr;
	}
	operator CEntity* () const
	{
		return ptr->Get();
	}
	CEntity* operator->() 
	{
		return CEntityLookup::Instance(*ptr);
	}
	bool operator == (void *rhs )
	{
		return (ptr == rhs);
	}

public:
	CFakeHandle* ptr;
	unsigned int offset;
};


class IPropTracker
{
public:
	IPropTracker()
	{
		m_Next = m_Head;
		m_Head = this;
	}
	virtual void InitProp(CEntity *pEnt) =0;

	bool GetSendPropOffset(const char *classname, const char *name, unsigned int &offset)
	{
		sm_sendprop_info_t info;
		if (!gamehelpers->FindSendPropInfo(classname, name, &info))
		{
			return false;
		}

		offset = info.actual_offset;
		return true;
	}

	bool GetDataMapOffset(CBaseEntity *pEnt, const char *name, unsigned int &offset)
	{
		datamap_t *pMap = gamehelpers->GetDataMap(pEnt);
		if (!pMap)
		{
			return false;
		}
		
		typedescription_t *typedesc = gamehelpers->FindInDataMap(pMap, name);
		
		if (typedesc == NULL)
		{
			return false;
		}

		offset = typedesc->fieldOffset[TD_OFFSET_NORMAL];
		return true;
	}
public:
	static IPropTracker *m_Head;
	IPropTracker *m_Next;
};


#define DECLARE_PROP_SEND(typeof, name) \
Redirect_Send<typeof> name; \
friend class name##PropTracker; \
class name##PropTracker : public IPropTracker \
{ \
public: \
	name##PropTracker() \
	{ \
		lookup = false; \
		found = false; \
	} \
	void InitProp(CEntity *pEnt) \
	{ \
		ThisClass *pThisType = dynamic_cast<ThisClass *>(pEnt); \
		if (pThisType) \
		{ \
			pThisType->name.pEdict = NULL;\
			if (!lookup) \
			{ \
				found = GetSendPropOffset(pEnt->edict()->GetNetworkable()->GetServerClass()->GetName(), #name, offset); \
				if (!found) \
				{ \
					g_pSM->LogError(myself,"[CENTITY] Failed lookup of prop %s on entity", #name); \
				} \
				lookup = true; \
			} \
			if (found) \
			{ \
				pThisType->name.pEdict = pEnt->edict();\
				pThisType->name.offset = offset; \
				pThisType->name.ptr = (typeof *)(((uint8_t *)(pEnt->BaseEntity())) + offset); \
			} \
			else \
			{ \
				pThisType->name.ptr = NULL; \
			} \
		} \
	} \
private: \
	unsigned int offset; \
	bool lookup; \
	bool found; \
}; \
static name##PropTracker name##PropTrackerObj;



#define DECLARE_PROP(typeof, name) \
Redirect<typeof> name; \
friend class name##PropTracker; \
class name##PropTracker : public IPropTracker \
{ \
public: \
	name##PropTracker() \
	{ \
		lookup = false; \
		found = false; \
	} \
	void InitProp(CEntity *pEnt) \
	{ \
		ThisClass *pThisType = dynamic_cast<ThisClass *>(pEnt); \
		if (pThisType) \
		{ \
			if (!lookup) \
			{ \
				found = GetDataMapOffset(pEnt->BaseEntity(), #name, offset); \
				if (!found) \
				{ \
					g_pSM->LogError(myself,"[CENTITY] Failed lookup of prop %s on entity", #name); \
				} \
				lookup = true; \
			} \
			if (found) \
			{ \
				pThisType->name.offset = offset; \
				pThisType->name.ptr = (typeof *)(((uint8_t *)(pEnt->BaseEntity())) + offset); \
			} \
			else \
			{ \
				pThisType->name.ptr = NULL; \
			} \
		} \
	} \
private: \
	unsigned int offset; \
	bool lookup; \
	bool found; \
}; \
static name##PropTracker name##PropTrackerObj;





#define DECLARE_PROP_OFFSET(typeof, name) \
Redirect<typeof> name; \
friend class name##PropTracker; \
class name##PropTracker : public IPropTracker \
{ \
public: \
	name##PropTracker() \
	{ \
		lookup = false; \
		found = false; \
	} \
	void InitProp(CEntity *pEnt) \
	{ \
		ThisClass *pThisType = dynamic_cast<ThisClass *>(pEnt); \
		if (pThisType) \
		{ \
			if(!lookup) \
			{ \
				found = g_pGameConf->GetOffset(#name, &offset); \
				if(!found) \
				{ \
					g_pSM->LogError(myself,"[CENTITY] Can not find offset %s", #name); \
				} \
				lookup = true; \
			} \
			if(found) \
			{ \
				pThisType->name.offset = offset; \
				pThisType->name.ptr = (typeof *)(((uint8_t *)(pEnt->BaseEntity())) + offset); \
			} \
		} \
	} \
private: \
	int offset; \
	bool lookup; \
	bool found; \
}; \
static name##PropTracker name##PropTrackerObj;



#define DECLARE_DEFAULTHANDLER_SPECIAL(type, name, ret, params, paramscall, special_ret) \
ret type::name params \
{ \
	if (!m_bIn##name) \
		return SH_MCALL(BaseEntity(), name) paramscall; \
	SET_META_RESULT(MRES_IGNORED); \
	SH_GLOB_SHPTR->DoRecall(); \
	SourceHook::EmptyClass *thisptr = reinterpret_cast<SourceHook::EmptyClass*>(SH_GLOB_SHPTR->GetIfacePtr()); \
	if(thisptr != (void *)BaseEntity()) { \
		RETURN_META_VALUE(MRES_SUPERCEDE, SH_MCALL(BaseEntity(), name) paramscall); \
	} \
	RETURN_META_VALUE(MRES_SUPERCEDE, (thisptr->*(__SoureceHook_FHM_GetRecallMFP##name(thisptr))) paramscall); \
} \
ret type::Internal##name params \
{ \
	SET_META_RESULT(MRES_SUPERCEDE); \
	type *pEnt = (type *)CEntity::Instance(META_IFACEPTR(CBaseEntity)); \
	if (!pEnt) \
		RETURN_META_VALUE(MRES_IGNORED, special_ret); \
	int __index = pEnt->entindex_non_network(); \
	pEnt->m_bIn##name = true; \
	ret retvalue = pEnt->name paramscall; \
	pEnt = (type *)CEntity::Instance(__index); \
	if (pEnt) \
		pEnt->m_bIn##name = false; \
	return retvalue; \
}


#define DECLARE_DEFAULTHANDLER(type, name, ret, params, paramscall) \
ret type::name params \
{ \
	if (!m_bIn##name) \
		return SH_MCALL(BaseEntity(), name) paramscall; \
	SET_META_RESULT(MRES_IGNORED); \
	SH_GLOB_SHPTR->DoRecall(); \
	SourceHook::EmptyClass *thisptr = reinterpret_cast<SourceHook::EmptyClass*>(SH_GLOB_SHPTR->GetIfacePtr()); \
	if(thisptr != (void *)BaseEntity()) { \
		RETURN_META_VALUE(MRES_SUPERCEDE, SH_MCALL(BaseEntity(), name) paramscall); \
	} \
	RETURN_META_VALUE(MRES_SUPERCEDE, (thisptr->*(__SoureceHook_FHM_GetRecallMFP##name(thisptr))) paramscall); \
} \
ret type::Internal##name params \
{ \
	SET_META_RESULT(MRES_SUPERCEDE); \
	type *pEnt = (type *)CEntity::Instance(META_IFACEPTR(CBaseEntity)); \
	if (!pEnt) \
		RETURN_META_VALUE(MRES_IGNORED, (ret)0); \
	int __index = pEnt->entindex_non_network(); \
	pEnt->m_bIn##name = true; \
	ret retvalue = pEnt->name paramscall; \
	pEnt = (type *)CEntity::Instance(__index); \
	if (pEnt) \
		pEnt->m_bIn##name = false; \
	return retvalue; \
}


#define DECLARE_DEFAULTHANDLER_REFERENCE(type, name, ret, params, paramscall) \
ret type::name params \
{ \
	if (!m_bIn##name) \
		return SH_MCALL(BaseEntity(), name) paramscall; \
	SET_META_RESULT(MRES_IGNORED); \
	SH_GLOB_SHPTR->DoRecall(); \
	SourceHook::EmptyClass *thisptr = reinterpret_cast<SourceHook::EmptyClass*>(SH_GLOB_SHPTR->GetIfacePtr()); \
	if(thisptr != (void *)BaseEntity()) { \
		RETURN_META_VALUE(MRES_SUPERCEDE, SH_MCALL(BaseEntity(), name) paramscall); \
	} \
	RETURN_META_VALUE(MRES_SUPERCEDE, (thisptr->*(__SoureceHook_FHM_GetRecallMFP##name(thisptr))) paramscall); \
} \
ret type::Internal##name params \
{ \
	SET_META_RESULT(MRES_SUPERCEDE); \
	type *pEnt = (type *)CEntity::Instance(META_IFACEPTR(CBaseEntity)); \
	if (!pEnt) \
		RETURN_META_NOREF(MRES_IGNORED, ret); \
	int __index = pEnt->entindex_non_network(); \
	pEnt->m_bIn##name = true; \
	ret retvalue = pEnt->name paramscall; \
	pEnt = (type *)CEntity::Instance(__index); \
	if (pEnt) \
		pEnt->m_bIn##name = false; \
	return retvalue; \
}


#define DECLARE_DEFAULTHANDLER_void(type, name, params, paramscall) \
void type::name params \
{ \
	if (!m_bIn##name) \
	{ \
		SH_MCALL(BaseEntity(), name) paramscall; \
		return; \
	} \
	SET_META_RESULT(MRES_IGNORED); \
	SH_GLOB_SHPTR->DoRecall(); \
	SourceHook::EmptyClass *thisptr = reinterpret_cast<SourceHook::EmptyClass*>(SH_GLOB_SHPTR->GetIfacePtr()); \
	if(thisptr != (void *)BaseEntity()) { \
		SH_MCALL(BaseEntity(), name) paramscall; \
		RETURN_META(MRES_SUPERCEDE); \
	} \
	(thisptr->*(__SoureceHook_FHM_GetRecallMFP##name(thisptr))) paramscall; \
	RETURN_META(MRES_SUPERCEDE); \
} \
void type::Internal##name params \
{ \
	SET_META_RESULT(MRES_SUPERCEDE); \
	type *pEnt = (type *)CEntity::Instance(META_IFACEPTR(CBaseEntity)); \
	if (!pEnt) \
		RETURN_META(MRES_IGNORED); \
	int __index = pEnt->entindex_non_network(); \
	pEnt->m_bIn##name = true; \
	pEnt->name paramscall; \
	pEnt = (type *)CEntity::Instance(__index); \
	if (pEnt) \
		pEnt->m_bIn##name = false; \
}


#define DECLARE_DEFAULTHANDLER_SUBCLASS(type1, type2, name, ret, params, paramscall) \
ret type1::name params \
{ \
	type2 *pointer = dynamic_cast<type2*>(BaseEntity()); \
	if (!m_bIn##name) { \
		return SH_MCALL(pointer, name) paramscall; \
	} \
	SET_META_RESULT(MRES_IGNORED); \
	SH_GLOB_SHPTR->DoRecall(); \
	SourceHook::EmptyClass *thisptr = reinterpret_cast<SourceHook::EmptyClass*>(SH_GLOB_SHPTR->GetIfacePtr()); \
	if(thisptr != (void *)pointer) { \
		RETURN_META_VALUE(MRES_SUPERCEDE, SH_MCALL(pointer, name) paramscall); \
	} \
	RETURN_META_VALUE(MRES_SUPERCEDE, (thisptr->*(__SoureceHook_FHM_GetRecallMFP##name(thisptr))) paramscall); \
} \
ret Static_##name params \
{ \
	SET_META_RESULT(MRES_SUPERCEDE); \
	type2 *pointer = META_IFACEPTR(type2); \
	CBaseEntity *cbase = dynamic_cast<CBaseEntity*>(pointer); \
	Assert(cbase); \
	if(!cbase) \
		RETURN_META_VALUE(MRES_IGNORED, (ret)0); \
	type1 *pEnt = dynamic_cast<type1 *>(CEntity::Instance(cbase)); \
	Assert(pEnt); \
	if(!pEnt) \
		RETURN_META_VALUE(MRES_IGNORED, (ret)0); \
	int __index = pEnt->entindex_non_network(); \
	pEnt->m_bIn##name = true; \
	ret retvalue = pEnt->name paramscall; \
	pEnt = (type1 *)CEntity::Instance(__index); \
	if (pEnt) \
		pEnt->m_bIn##name = false; \
	return retvalue; \
}


#define DECLARE_DEFAULTHANDLER_SUBCLASS_void(type1, type2, name, params, paramscall) \
void type1::name params \
{ \
	type2 *pointer = dynamic_cast<type2*>(BaseEntity()); \
	if (!m_bIn##name) { \
		SH_MCALL(pointer, name) paramscall; \
		return; \
	} \
	SET_META_RESULT(MRES_IGNORED); \
	SH_GLOB_SHPTR->DoRecall(); \
	SourceHook::EmptyClass *thisptr = reinterpret_cast<SourceHook::EmptyClass*>(SH_GLOB_SHPTR->GetIfacePtr()); \
	if(thisptr != (void *)pointer) { \
		SH_MCALL(pointer, name) paramscall; \
		RETURN_META(MRES_SUPERCEDE); \
	} \
	(thisptr->*(__SoureceHook_FHM_GetRecallMFP##name(thisptr))) paramscall; \
	RETURN_META(MRES_SUPERCEDE); \
} \
void Static_##name params \
{ \
	SET_META_RESULT(MRES_SUPERCEDE); \
	type2 *pointer = META_IFACEPTR(type2); \
	CBaseEntity *cbase = dynamic_cast<CBaseEntity*>(pointer); \
	Assert(cbase); \
	if(!cbase) { \
		SET_META_RESULT(MRES_IGNORED); \
		return; \
	} \
	type1 *pEnt = dynamic_cast<type1 *>(CEntity::Instance(cbase)); \
	Assert(pEnt); \
	if(!pEnt) { \
		SET_META_RESULT(MRES_IGNORED); \
		return; \
	} \
	int __index = pEnt->entindex_non_network(); \
	pEnt->m_bIn##name = true; \
	pEnt->name paramscall; \
	pEnt = (type1 *)CEntity::Instance(__index); \
	if (pEnt) \
		pEnt->m_bIn##name = false; \
	return; \
}


#define DECLARE_DEFAULTHANDLER_DETOUR_void(type, name, params, paramscall) \
void type::name params \
{ \
	(((type *)BaseEntity())->*name##_Actual) paramscall; \
} \
void type::Internal##name params \
{ \
	type *pEnt = (type *)CEntity::Instance((CBaseEntity *)this); \
	if(!pEnt) {\
		(this->*name##_Actual) paramscall;\
		return;\
	}\
	assert(pEnt); \
	pEnt->name paramscall; \
} \
void (type::* type::name##_Actual) params = NULL; \

#define DECLARE_DEFAULTHANDLER_DETOUR(type, name, ret, params, paramscall) \
ret type::name params \
{ \
	return (((type *)BaseEntity())->*name##_Actual) paramscall; \
} \
ret type::Internal##name params \
{ \
	type *pEnt = (type *)CEntity::Instance((CBaseEntity *)this); \
	if(!pEnt) {\
		return (this->*name##_Actual) paramscall;\
	}\
	assert(pEnt); \
	return pEnt->name paramscall; \
} \
ret (type::* type::name##_Actual) params = NULL;



#define RegisterHook(name, hook)\
	if(!g_pGameConf->GetOffset(name, &offset))\
	{\
		return false;\
	}\
	SH_MANUALHOOK_RECONFIGURE(hook, offset, 0, 0);

#define RegisterHookNORET(name, hook)\
	if(!g_pGameConf->GetOffset(name, &offset))\
	{\
		return;\
	}\
	SH_MANUALHOOK_RECONFIGURE(hook, offset, 0, 0);


#define _GET_VARIABLE(name, type, ret)\
	char *name##_addr = NULL;\
	int name##_offset;\
	META_CONPRINTF("[%s] Getting %s - ",g_Monster.GetLogTag(),#name);\
	if(!g_pGameConf->GetMemSig(#name, (void **)&name##_addr) || name##_addr == NULL)\
		goto name##_fail;\
	if(!g_pGameConf->GetOffset(#name, &name##_offset))\
		goto name##_fail;\
	void *name##_final_addr = reinterpret_cast<char *>(name##_addr + name##_offset);\
	char *name##_data;\
	memcpy(&name##_data, reinterpret_cast<void*>(name##_final_addr), sizeof(char*));\
	name = (type)name##_data;\
		goto name##_success;\
	name##_fail:\
		META_CONPRINT("Fail\n");\
		g_pSM->LogError(myself,"Unable getting %s",#name);\
		Assert(0);\
		return ret;\
	name##_success:\
		META_CONPRINT("Success\n");


#define GET_VARIABLE(name, type)\
	_GET_VARIABLE(name, type, false);

#define GET_VARIABLE_NORET(name, type)\
	_GET_VARIABLE(name, type,);


#define _GET_VARIABLE_POINTER(name, type, ret)\
	unsigned char *name##_addr = NULL;\
	int name##_offset;\
	META_CONPRINTF("[%s] Getting %s - ",g_Monster.GetLogTag(),#name);\
	if(!g_pGameConf->GetMemSig(#name, (void **)&name##_addr) || name##_addr == NULL)\
		goto name##_fail;\
	if(!g_pGameConf->GetOffset(#name, &name##_offset))\
		goto name##_fail;\
	void **name##_final_addr = *reinterpret_cast<void ***>(name##_addr + name##_offset);\
	char *name##_data;\
	memcpy(&name##_data, reinterpret_cast<void*>(name##_final_addr), sizeof(char*));\
	name = (type)name##_data;\
		goto name##_success;\
	name##_fail:\
		META_CONPRINT("Fail\n");\
		g_pSM->LogError(myself,"Unable getting %s",#name);\
		return ret;\
	name##_success:\
		META_CONPRINT("Success\n");


#define GET_VARIABLE_POINTER(name, type)\
	_GET_VARIABLE_POINTER(name, type, false);

#define GET_VARIABLE_POINTER_NORET(name, type)\
	_GET_VARIABLE_POINTER(name, type,);


#define GET_DETOUR(name, func)\
	name##_CDetour = func;\
	if(name##_CDetour == NULL)\
	{\
		g_pSM->LogError(myself,"Unable getting %s",#name);\
		return false;\
	}\
	name##_CDetour->EnableDetour();


#define FindValveGameSystem(ptr, bclass, systemname) \
	META_CONPRINTF("[%s] Getting Valve System %s - ",g_Monster.GetLogTag(),#ptr);\
	ptr = NULL;\
	for(int i=0;i<s_GameSystems->Count();i++)\
	{\
		IValveGameSystem *vsystem = s_GameSystems->Element(i);\
		if(vsystem)\
		{\
			char const *name = vsystem->Name();\
			if(strcmp(name, systemname) == 0)\
			{\
				ptr = (bclass)(vsystem);\
			}\
		}\
	}\
	if(ptr == NULL) {\
		META_CONPRINT("Fail\n");\
		g_pSM->LogError(myself,"Unable getting Valve System %s", systemname);\
		return false;\
	}\
	META_CONPRINT("Success\n");\

// recycling FTYPEDESC_VIEW_NEVER to denote CEntity data.. no the best solution but FTYPEDESC_VIEW_NEVER seems to be only used on the client anyway
#define CE_DEFINE_INPUT( name, fieldtype, inputname ) { fieldtype, #name, { offsetof(classNameTypedef, name), 0 }, 1, FTYPEDESC_INPUT | FTYPEDESC_SAVE | FTYPEDESC_KEY | FTYPEDESC_VIEW_NEVER,	inputname, NULL, NULL, NULL, sizeof( ((classNameTypedef *)0)->name ) }

#endif // _INCLUDE_MACROS_H_
