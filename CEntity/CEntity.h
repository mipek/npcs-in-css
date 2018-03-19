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

/**
* CEntity Entity Handling Framework version 2.0 by Matt "pRED*" Woodrow
*
* - Credits:
*		- This is largely (or entirely) based on a concept by voogru - http://voogru.com/
*		- The virtual function hooking is powered by the SourceHook library by Pavol "PM OnoTo" Marko.
*		- Contains code contributed by Brett "Brutal" Powell.
*		- Contains code contributed by Asher "asherkin" Baker.
*
* - About:
*		- CEntity is (and its derived classes are) designed to emulate the CBaseEntity class from the HL2 SDK.
*		- Valve code (like entire class definitions and CBaseEntity functions) from the SDK should almost just work when copied into this.
*			- References to CBaseEntity need to be changed to CEntity.
*			- Sendprops and datamaps are pointers to the actual values so references to these need to be dereferenced.
*				- Code that uses unexposed data members won't work - Though you could reimplement these manually.
*		- Virtual functions handle identically to ones in a real derived class.
*			- Calls from valve code to a virtual in CEntity (with no derived versions) fall back directly to the valve code.
*			- Calls from valve code to a virtual (with a derived version) will call that code, and the valve code can be optionally run using BaseClass::Function().
*
*			- Calls from your code to a virtual in CEntity (with no derived versions) will make a call to the valve code.
*			- Calls from your code to a virtual (with a derived version) will call that code, and that derived handler can run the valve code optionally using BaseClass::Function().
*			
*
* - Notes:
*		- If you inherit Init() or Destroy() in a derived class, I would highly recommend calling the BaseClass equivalent.
* 
* - TODO (in no particular order):
*		- Add handling of custom keyvalues commands
*			- Add datamapping to class values so keyvalues can parse to them
*		- Include more CEntity virtuals and props/datamaps
*		- Create more derived classes
*		- Include more Think/Touch etc handlers
*			- Valve code now has lists of thinks, can we access this?
*		- Forcibly deleting entities? - Implemented AcceptInput("Kill"...), UTIL_Remove sig scan would be cleaner.
*		- Support mods other than TF2 (CPlayer should only contain CBasePlayer sdk stuff and create optional CTFPlayer/CCSPlayer derives)
*
*	- Change log
*		- 1.0
*			- Initial import of basic CEntity and CPlayer
*		- 1.x
*			- Unlogged fixes/changes added after original version. TODO: Grab these from the hg changelog sometime.
*		- 2.0
*			- Improved CE_LINK_ENTITY_TO_CLASS to use DLL Classnames. tf_projectile_rocket changed to CTFProjectile_Rocket for example.
*			- Added the ability to handle entity Inputs/Outputs.
*			- Cleaned up Macros used for almost everything.
*			- Added many new hooks and props for CEntity and CPlayer.
*			- Support for custom classnames with LINK_ENTITY_TO_CUSTOM_CLASS.
*			- Added support for detours, needs CDetours folder in the parent directory.
*			- Added a helpers class that makes common functions easy (from CrimsonGT).
*			- CScriptCreatedItem and CScriptCreatedAttribute (from TF2Items).
*			- A new 'tracker', designed to get a generic pointer from a sig in a gamedata file.
*			- A lot of CPlayer defines, including PLAYERCONDs, WEAPONSLOTs and LOADOUTSLOTs.
*			- Added CAnimating with StudioFrameAdvance and Dissolve.
*			- Changed CPlayer to inherit from CAnimating.
*/

#ifndef _INCLUDE_CENTITY_H_
#define _INCLUDE_CENTITY_H_

//#define GAME_DLL


#include "extension.h"

#include "CE_Define.h"
#include "CEntityBase.h"
#include "CECollisionProperty.h"
#include "CTakeDamageInfo.h"
#include "IEntityFactory.h"
#include "string_t.h"
#include "gamestringpool.h"

inline int ENTINDEX( edict_t *pEdict)			
{ 
	return engine->IndexOfEdict(pEdict); 
}

#include "vector.h"
#include "server_class.h"

#include "macros.h"

#include "cutil.h"

//#include "CTakeDamageInfo.h"

#include <typeinfo>
#include <variant_t.h>
#include "EntityOutput.h"

#include "takedamageinfo.h"
#include "collisionproperty.h"
#include "ServerNetworkProperty.h"
#include "decals.h"
#include "vehicles.h"
#include "vcollide_parse.h"
#include "physics.h"

extern variant_t g_Variant;


// For CLASSIFY
enum Class_T
{
	CLASS_NONE=0,				
	CLASS_PLAYER,			
	CLASS_PLAYER_ALLY,
	CLASS_PLAYER_ALLY_VITAL,
	CLASS_ANTLION,
	CLASS_BARNACLE,
	CLASS_BULLSEYE,
	CLASS_BULLSQUID,	
	CLASS_CITIZEN_PASSIVE,	
	CLASS_CITIZEN_REBEL,
	CLASS_COMBINE,
	CLASS_COMBINE_GUNSHIP,
	CLASS_CONSCRIPT,
	CLASS_HEADCRAB,
	CLASS_HOUNDEYE,
	CLASS_MANHACK,
	CLASS_METROPOLICE,		
	CLASS_MILITARY,		
	CLASS_SCANNER,		
	CLASS_STALKER,		
	CLASS_VORTIGAUNT,
	CLASS_ZOMBIE,
	CLASS_PROTOSNIPER,
	CLASS_MISSILE,
	CLASS_FLARE,
	CLASS_EARTH_FAUNA,
	CLASS_HACKED_ROLLERMINE,
	CLASS_COMBINE_HUNTER,
	
	NUM_AI_CLASSES
};

// Things that toggle (buttons/triggers/doors) need this
enum TOGGLE_STATE
{
	TS_AT_TOP,
	TS_AT_BOTTOM,
	TS_GOING_UP,
	TS_GOING_DOWN
};

//
// Structure passed to input handlers.
//
struct inputdata_t
{
	CBaseEntity *pActivator;		// The entity that initially caused this chain of output events.
	CBaseEntity *pCaller;			// The entity that fired this particular output.
	variant_t value;				// The data parameter for this output.
	int nOutputID;					// The unique ID of the output that was fired.
};

class CEntity;
class CECollisionProperty;
class CAI_NPC;
class CE_CSkyCamera;
class IResponseSystem;

#define CONCEPT_WEIGHT 5.0f

#define VPHYSICS_MAX_OBJECT_LIST_COUNT	1024


extern CEntity *pEntityData[ENTITY_ARRAY_SIZE];

extern ConVar *sv_gravity;
extern ConVar *phys_pushscale;
extern ConVar *hl2_episodic;


//-----------------------------------------------------------------------------
// Entity events... targetted to a particular entity
// Each event has a well defined structure to use for parameters
//-----------------------------------------------------------------------------
enum EntityEvent_t
{
	ENTITY_EVENT_WATER_TOUCH = 0,		// No data needed
	ENTITY_EVENT_WATER_UNTOUCH,			// No data needed
	ENTITY_EVENT_PARENT_CHANGED,		// No data needed
};


struct ResponseContext_t
{
	DECLARE_SIMPLE_DATADESC();

	string_t		m_iszName;
	string_t		m_iszValue;
	float			m_fExpirationTime;		// when to expire context (0 == never)
};

struct hl_constraint_info_t
{
	hl_constraint_info_t() 
	{ 
		pObjects[0] = pObjects[1] = NULL;
		pGroup = NULL;
		anchorPosition[0].Init();
		anchorPosition[1].Init();
		swapped = false; 
		massScale[0] = massScale[1] = 1.0f;
	}
	Vector			anchorPosition[2];
	IPhysicsObject	*pObjects[2];
	IPhysicsConstraintGroup *pGroup;
	float			massScale[2];
	bool			swapped;
};

struct constraint_anchor_t
{
	Vector		localOrigin;
	EHANDLE		hEntity;
	int			parentAttachment;
	string_t	name;
	float		massScale;
};


typedef void (CEntity::*BASEPTR)(void);
typedef void (CEntity::*ENTITYFUNCPTR)(CEntity *pOther);
typedef void (CEntity::*USEPTR)(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);

typedef void (CBaseEntity::*VALVE_BASEPTR)(void);
typedef void (CBaseEntity::*VALVE_ENTITYFUNCPTR)(CBaseEntity *pOther);


#define DEFINE_THINKFUNC( function ) DEFINE_FUNCTION_RAW( function, BASEPTR )
#define DEFINE_ENTITYFUNC( function ) DEFINE_FUNCTION_RAW( function, ENTITYFUNCPTR )
#define DEFINE_USEFUNC( function ) DEFINE_FUNCTION_RAW( function, USEPTR )


//template< class T >
class CFakeHandle;
class AI_CriteriaSet;


#define DECLARE_DEFAULTHEADER(name, ret, params) \
	ret Internal##name params; \
	bool m_bIn##name;

#define DECLARE_DEFAULTHEADER_DETOUR(name, ret, params) \
	ret Internal##name params; \
	static ret (ThisClass::* name##_Actual) params;

#define SetThink(a) ThinkSet(static_cast <void (CEntity::*)(void)> (a), 0, NULL)
#define SetContextThink( a, b, context ) ThinkSet( static_cast <void (CEntity::*)(void)> (a), (b), context )

#define SetTouch( a ) ce_m_pfnTouch = static_cast <void (CEntity::*)(CEntity *)> (a)
#define SetUse( a ) ce_m_pfnUse = static_cast <void (CEntity::*)(CBaseEntity *, CBaseEntity *, USE_TYPE , float)> (a)


struct thinkfunc_t
{
	BASEPTR	m_pfnThink;
	string_t		m_iszContext;
	int				m_nNextThinkTick;
	int				m_nLastThinkTick;

	DECLARE_SIMPLE_DATADESC();
};

// handling entity/edict transforms
inline CBaseEntity *GetContainingEntity( edict_t *pent )
{
	if ( pent && pent->GetUnknown() )
	{
		return pent->GetUnknown()->GetBaseEntity();
	}

	return NULL;
}

class CCombatCharacter;
class CAnimating;
class CDmgAccumulator;


class CEntity // : public CBaseEntity  - almost.
{
public: // CEntity
	CE_DECLARE_CLASS_NOBASE(CEntity);
	DECLARE_DATADESC();
	DECLARE_DEFAULTHEADER(GetDataDescMap_Real, datamap_t *, ());
	datamap_t *GetDataDescMap_Real();

	virtual ~CEntity();

	virtual void CE_Init(edict_t *pEdict, CBaseEntity *pBaseEntity);
	virtual void PostConstructor() {}

	void InitHooks();
	void InitProps();
	void ClearAllFlags();
	virtual void Destroy();
	inline CBaseEntity *BaseEntity() { return m_pEntity; }
	inline CBaseEntity *BaseEntity() const { return m_pEntity; }

	// memory handling
    void *operator new( size_t stAllocateBlock );
    void *operator new( size_t stAllocateBlock, int nBlockUse, const char *pFileName, int nLine );
	void operator delete( void *pMem );
	void operator delete( void *pMem, int nBlockUse, const char *pFileName, int nLine ) { operator delete(pMem); }


	operator CBaseEntity* ()
	{
		if (this == NULL)
		{
			return NULL;
		}

		return BaseEntity();
	}
	CEntity *operator=(CBaseEntity *rhs)
	{
		return CEntityLookup::Instance(rhs);
	}
	CEntity *operator=(CBaseHandle &rhs)
	{
		return CEntityLookup::Instance(rhs);
	}
	CEntity *operator=(CBaseHandle rhs)
	{
		return CEntityLookup::Instance(rhs);
	}
	CEntity *operator=(Redirect<CBaseHandle> &rhs)
	{
		return CEntityLookup::Instance(rhs);
	}
	CBaseEntity *operator=(CEntity *const pEnt)
	{
		return pEnt->BaseEntity();
	}

	/* Bcompat and it's just easier to refer to these as CEntity:: */
	static CEntity *Instance(const CBaseHandle &hEnt) { return CEntityLookup::Instance(hEnt); }
	static CEntity *Instance(const edict_t *pEnt)  { return CEntityLookup::Instance(pEnt); }
	static CEntity *Instance(edict_t *pEnt)  { return CEntityLookup::Instance(pEnt); }
	static CEntity* Instance(int iEnt)  { return CEntityLookup::Instance(iEnt); }
	static CEntity* Instance(CBaseEntity *pEnt)  { return CEntityLookup::Instance(pEnt); }
	static CEntity* Instance(const CBaseEntity *pEnt)  { return CEntityLookup::Instance((CBaseEntity *)pEnt); }

	static CEntity *Create( const char *szName, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL );
	static CEntity *CreateNoSpawn( const char *szName, const Vector &vecOrigin, const QAngle &vecAngles, CBaseEntity *pOwner = NULL );

	static HSOUNDSCRIPTHANDLE PrecacheScriptSound( const char *soundname );
	
	static soundlevel_t LookupSoundLevel( const char *soundname );
	static soundlevel_t LookupSoundLevel( const char *soundname, HSOUNDSCRIPTHANDLE& handle );

	static void EmitSound( IRecipientFilter& filter, int iEntIndex, const EmitSound_t & params );
	static void EmitSound( IRecipientFilter& filter, int iEntIndex, const char *soundname, const Vector *pOrigin = NULL, float soundtime = 0.0f, float *duration = NULL );

	static void StopSound( int iEntIndex, const char *soundname );
	static void StopSound( int iEntIndex, int iChannel, const char *pSample );

	static bool	GetParametersForSound( const char *soundname, CSoundParameters &params, const char *actormodel );
	static bool	GetParametersForSound( const char *soundname, HSOUNDSCRIPTHANDLE& handle, CSoundParameters &params, const char *actormodel );

	static int	PrecacheModel( const char *name ); 

	static void	EmitSentenceByIndex( IRecipientFilter& filter, int iEntIndex, int iChannel, int iSentenceIndex, 
		float flVolume, soundlevel_t iSoundlevel, int iFlags = 0, int iPitch = PITCH_NORM,
		const Vector *pOrigin = NULL, const Vector *pDirection = NULL, bool bUpdatePositions = true, float soundtime = 0.0f );

	static const trace_t *GetTouchTrace( void );

	static float GetSoundDuration( const char *soundname, char const *actormodel );

public: // CBaseEntity virtuals
	virtual bool IsCustomEntity() { return false; }

	virtual void SetModel(const char *model);
	virtual void Teleport(const Vector *origin, const QAngle* angles, const Vector *velocity);
	virtual void UpdateOnRemove();
	virtual void Spawn();
	virtual int OnTakeDamage(const CTakeDamageInfo &info);
	virtual void Think();
	virtual bool AcceptInput(const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t Value, int outputID);
	virtual void StartTouch(CEntity *pOther);
	virtual void Touch(CEntity *pOther); 
	virtual void EndTouch(CEntity *pOther);
	virtual Vector GetSoundEmissionOrigin();
	virtual int VPhysicsTakeDamage(const CTakeDamageInfo &inputInfo);
	virtual int	VPhysicsGetObjectList(IPhysicsObject **pList, int listMax);
	virtual ServerClass *GetServerClass();
	virtual Class_T Classify();
	virtual bool ShouldCollide(int collisionGroup, int contentsMask);
	virtual int ObjectCaps();
	virtual bool IsNPC();
	virtual void TakeDamage( const CTakeDamageInfo &inputInfo );
	virtual void SetParent(CBaseEntity* pNewParent, int iAttachment = -1);
	virtual void DecalTrace( trace_t *pTrace, char const *decalName );
	virtual void Event_Killed( const CTakeDamageInfo &info );
	virtual void TraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator );
	virtual Vector BodyTarget( const Vector &posSrc, bool bNoisy = true);
	virtual IServerVehicle *GetServerVehicle();
	virtual bool IsAlive();
	virtual const Vector &WorldSpaceCenter() const;
	virtual void PhysicsSimulate();
	virtual int	BloodColor();
	virtual void StopLoopingSounds();
	virtual void SetOwnerEntity( CBaseEntity* pOwner );
	virtual void Activate();
	virtual Vector HeadTarget( const Vector &posSrc );
	virtual float GetAutoAimRadius();
	virtual unsigned int PhysicsSolidMaskForEntity( void ) const;
	virtual bool CanStandOn(CBaseEntity *pSurface) const;
	virtual Vector GetSmoothedVelocity( void );
	virtual void Use( CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value );
	virtual bool FVisible_Entity(CBaseEntity *pEntity, int traceMask = MASK_BLOCKLOS, CBaseEntity **ppBlocker = NULL );
	virtual bool FVisible_Vector(const Vector &vecTarget, int traceMask = MASK_BLOCKLOS, CBaseEntity **ppBlocker = NULL );
	virtual Vector EarPosition( void ) const;
	virtual Vector GetAutoAimCenter();
	virtual Vector EyePosition();
	virtual void OnRestore( void );
	virtual void ImpactTrace( trace_t *pTrace, int iDamageType, const char *pCustomImpactName = NULL );
	virtual	bool TestHitboxes(const Ray_t &ray, unsigned int fContentsMask, trace_t& tr);
	virtual void VPhysicsCollision( int index, gamevcollisionevent_t *pEvent );
	virtual void VPhysicsShadowCollision( int index, gamevcollisionevent_t *pEvent );
	virtual void Splash();
	virtual bool ShouldSavePhysics();
	virtual bool CreateVPhysics( void );
	virtual bool IsNetClient( void ) const;
	virtual CBaseEntity *HasPhysicsAttacker( float dt );
	virtual const QAngle &EyeAngles( void );
	virtual int	Save( ISave &save );
	virtual int	Restore( IRestore &restore );
	virtual void ModifyOrAppendCriteria( AI_CriteriaSet& set );
	virtual void DeathNotice ( CBaseEntity *pVictim );
	virtual bool PassesDamageFilter( const CTakeDamageInfo &info );
	virtual void Precache();
	virtual bool DispatchKeyValue( const char *szKeyName, const char *szValue );
	virtual void OnEntityEvent( EntityEvent_t event, void *pEventData );
	virtual void VPhysicsDestroyObject();
	virtual int	TakeHealth( float flHealth, int bitsDamageType );
	virtual float GetAttackDamageScale( CBaseEntity *pVictim );
	virtual bool OnControls( CBaseEntity *pControls );
	virtual void VPhysicsUpdate( IPhysicsObject *pPhysics );
	virtual void VPhysicsShadowUpdate( IPhysicsObject *pPhysics );
	virtual bool IsTriggered( CBaseEntity *pActivator );
	virtual void FireBullets( const FireBulletsInfo_t &info );
	virtual const char *GetTracerType( void );
	virtual int	UpdateTransmitState();
	virtual void SetTransmit( CCheckTransmitInfo *pInfo, bool bAlways );
	virtual bool CanBeSeenBy( CBaseEntity *pNPC );
	virtual bool IsViewable();
	virtual IResponseSystem *GetResponseSystem();

public:
	void SetLocalOrigin(const Vector& origin);
	void PhysicsTouchTriggers(const Vector *pPrevAbsOrigin = NULL);
	void SetAbsOrigin(const Vector& absOrigin);
	void SetLocalAngles( const QAngle& angles );
	virtual bool IsMoving( void );
	bool AddStepDiscontinuity( float flTime, const Vector &vecOrigin, const QAngle &vecAngles );
	IPhysicsObject *VPhysicsInitShadow( bool allowPhysicsMovement, bool allowPhysicsRotation, solid_t *pSolid = NULL );
	void PhysicsMarkEntitiesAsTouching( CBaseEntity *other, trace_t &trace );
	IPhysicsObject *VPhysicsInitNormal( SolidType_t solidType, int nSolidFlags, bool createAsleep, solid_t *pSolid = NULL);
	IPhysicsObject *VPhysicsInitStatic( void );
	bool CBaseEntity_FVisible( const Vector &vecTarget, int traceMask = MASK_BLOCKLOS, CBaseEntity **ppBlocker = NULL );

public:
	virtual CCombatCharacter *MyCombatCharacterPointer( void ) { return NULL; }
	virtual CAnimating	*GetBaseAnimating() { return NULL; }

public: // CBaseEntity non virtual helpers
	void CBaseEntityThink();
	void CEntityThink();
	VALVE_BASEPTR GetCurrentThinkPointer();
	BASEPTR	ThinkSet(BASEPTR func, float thinkTime, const char *szContext);
	void SetNextThink(float thinkTime, const char *szContext = NULL);
	void CheckHasThinkFunction(bool isThinking);
	bool WillThink();

	void AddEFlags(int nEFlagMask);
	void RemoveEFlags(int nEFlagMask);
	bool IsEFlagSet(int nEFlagMask) const;

	const char *GetDebugName(void);
	const char* GetClassname();
	void SetClassname(const char *pClassName);
	const char* GetEntityName();
	string_t GetEntityName_String();
	void SetName(const char *pTargetName);
	CEntity *GetOwnerEntity();
	void SetOwner(CEntity *pOwnerEntity);

	int GetTeamNumber()  const;
	virtual void ChangeTeam(int iTeamNum);
	bool InSameTeam(CEntity *pEntity) const;

	MoveType_t GetMoveType() const;
	void SetMoveType( MoveType_t val, MoveCollide_t moveCollide = MOVECOLLIDE_DEFAULT );
	MoveCollide_t GetMoveCollide() const;
	void SetMoveCollide(MoveCollide_t MoveCollide);

	const Vector &GetAbsOrigin() const;
	const Vector &GetLocalOrigin() const;
	const Vector &GetAbsVelocity() const;
	const Vector &GetVelocity() const;

	CEntity *GetMoveParent();
	CEntity *GetRootMoveParent();
	CEntity *FirstMoveChild( void ) { return m_hMoveChild; }
	CEntity *NextMovePeer( void ) { return m_hMovePeer; }

	inline edict_t *edict() { return m_pEdict; }
	inline const edict_t *edict() const { return m_pEdict; }
	int entindex();
	int entindex_non_network();

	inline IPhysicsObject *VPhysicsGetObject(void) const { return m_pPhysicsObject; }
	inline CECollisionProperty *CollisionProp(void) const { return col_ptr; }
	inline CCollisionProperty *CollisionProp_Actual(void) const { return m_Collision; }
	inline ICollideable *GetCollideable() const { return m_Collision; }

	void SetCollisionGroup(int collisionGroup);
	void CollisionRulesChanged();

	inline int GetWaterLevel() const { return m_nWaterLevel; }
	inline void SetWaterLevel( int nLevel ) { m_nWaterLevel = nLevel; }
	
	bool IsPointSized() const;

	void SetWaterType( int nType );

	inline bool	IsWorld() { return entindex() == 0; }
	
	const Vector&			WorldAlignMins( ) const;
	const Vector&			WorldAlignMaxs( ) const;
	const Vector&			WorldAlignSize( ) const;

	float BoundingRadius() const;
	bool  IsSolidFlagSet( int flagMask ) const;

	virtual	bool IsPlayer();

	int GetTeam();

	const color32 GetRenderColor() const;
	void SetRenderColor( byte r, byte g, byte b );
	void SetRenderColor( byte r, byte g, byte b, byte a );
	void SetRenderColorR( byte r );
	void SetRenderColorG( byte g );
	void SetRenderColorB( byte b );
	void SetRenderColorA( byte a );

	int GetSpawnFlags() const;
	void AddSpawnFlags(int nFlags);
	void RemoveSpawnFlags(int nFlags) ;
	void ClearSpawnFlags();
	bool HasSpawnFlags( int nFlags ) const;

	void GetVectors(Vector* pForward, Vector* pRight, Vector* pUp) const;

	matrix3x4_t &EntityToWorldTransform() ;
	const matrix3x4_t &EntityToWorldTransform() const;

	void SetGravity(float flGravity);
	float GetGravity() const;

	int		GetMaxHealth()  const	{ return m_iMaxHealth; }
	void	SetMaxHealth( int amt )	{ m_iMaxHealth = amt; }

	int		GetHealth() const		{ return m_iHealth; }
	void	SetHealth( int amt )	{ m_iHealth = amt; }

	void ApplyAbsVelocityImpulse( const Vector &vecImpulse );
	void ApplyLocalAngularVelocityImpulse( const AngularImpulse &angImpulse );

	void EmitSound( const char *soundname, float soundtime = 0.0f, float *duration = NULL );
	void EmitSound( const char *soundname, HSOUNDSCRIPTHANDLE& handle, float soundtime = 0.0f, float *duration = NULL );  // Override for doing the general case of CPASAttenuationFilter filter( this ), and EmitSound( filter, entindex(), etc. );

	void RemoveDeferred( void );

	void PhysicsStepRecheckGround();

	void SetNavIgnore( float duration = FLT_MAX );
	void ClearNavIgnore();
	bool IsNavIgnored() const;

	void StopSound( const char *soundname );

	void PhysicsCheckWaterTransition( void );
	void UpdateWaterState();

	void FollowEntity( CBaseEntity *pBaseEntity, bool bBoneMerge = true );
	void StopFollowingEntity();
	bool IsFollowingEntity();
	
	void RemoveAllDecals();

	void ComputeAbsPosition( const Vector &vecLocalPosition, Vector *pAbsPosition );

public: //custom
	CECollisionProperty *col_ptr;

public: // custom
	SolidType_t GetSolid() const;
	void SetSolid(SolidType_t val);
	void SetSolidFlags(int flags);
	void AddSolidFlags(int nFlags );
	void RemoveSolidFlags(int nFlags);
	bool IsSolid() const;

	void SetViewOffset(const Vector& v);
	
	void AddEffects(int nEffects);
	void SetEffects( int nEffects );
	void RemoveEffects(int nEffects);
	void ClearEffects();
	bool IsEffectActive( int nEffects ) const;

	void AddFlag(int flags);
	void RemoveFlag(int flagsToRemove);
	void ToggleFlag(int flagToToggle);
	int GetFlags() const;
	void ClearFlags();
	
	int GetModelIndex( void ) const;
	void SetModelIndex( int index );

	bool IsTransparent() const;
	void SetRenderMode( RenderMode_t nRenderMode );

	int GetCollisionGroup() const;

	IHandleEntity *GetIHandle() const;

	model_t	*GetModel();

	int DispatchUpdateTransmitState();

	const QAngle& GetLocalAngles() const;
	const QAngle& GetAbsAngles() const;
	
	const Vector& GetViewOffset() const;

	void SetGroundEntity(CBaseEntity *ground);
	void SetAbsVelocity(const Vector &vecAbsVelocity);

	void SetAbsAngles(const QAngle& absAngles);

	CAI_NPC	*MyNPCPointer();

	CServerNetworkProperty *NetworkProp() const;
	
	CEntity *GetGroundEntity();
	CEntity *GetGroundEntity() const;

	string_t GetModelName( void ) const;
	void SetModelName( string_t name );

	bool IsBSPModel() const;

	float GetGroundChangeTime( void );

	float GetLastThink( const char *szContext = NULL );

	void NetworkStateChanged();

	void VelocityPunch( const Vector &vecForce );

	CEntity *GetNextTarget( void );

	bool		NameMatches( const char *pszNameOrWildcard );
	bool		ClassMatches( const char *pszClassOrWildcard );

	bool		NameMatches( string_t nameStr );
	bool		ClassMatches( string_t nameStr );

	void CalcAbsolutePosition();
	void CalcAbsoluteVelocity();

	bool PhysicsCheckWater( void );
	int GetWaterType() const;

	const Vector&	GetBaseVelocity() const;
	void			SetBaseVelocity( const Vector& v );

	void		TraceBleed( float flDamage, const Vector &vecDir, trace_t *ptr, int bitsDamageType );

	void		SUB_Remove();
	void		SUB_DoNothing();
	void		SUB_TouchNothing(CEntity *pOther);

	void		Remove();
	void		ViewPunch( const QAngle &angleOffset );

	bool		IsMarkedForDeletion( void ); 

	CEntity		*GetParent();

	void		SetCollisionBounds( const Vector& mins, const Vector &maxs );
	
	float		GetNextThink( const char *szContext = NULL );
	void		DispatchTraceAttack( const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr );

	inline int	GetSolidFlags( void ) const { return m_usSolidFlags; }

	bool		BlocksLOS( void );
	void		SetBlocksLOS( bool bBlocksLOS );
	
	bool		HasNPCsOnIt();
	void		*GetDataObject( int type );
	
	float		GetFriction( void ) const;
	void		SetFriction( float flFriction );

	void		SetLocalAngularVelocity( const QAngle &vecAngVelocity );
	const QAngle &GetLocalAngularVelocity() const;

	unsigned char GetParentAttachment() const;

	void		SetMoveDoneTime( float flTime );
	void		CheckHasGamePhysicsSimulation();

	float		GetLocalTime( void ) const;

	int			CBaseEntity_ObjectCaps( void );
	
	int			SetTransmitState( int nFlag);

	bool			DetectInSkybox();
	CE_CSkyCamera	*GetEntitySkybox();

	int			GetIndexForThinkContext( const char *pszContext );
	int			RegisterThinkContext( const char *szContext );
	int			GetNextThinkTick( const char *szContext = NULL );

	void		GenderExpandString( char const *in, char *out, int maxlen );
	
	void		AddContext( const char *nameandvalue );
	int			FindContextByName( const char *name ) const;
	const char *GetContextName( int index ) const;

	void FireBullets( int cShots, const Vector &vecSrc, const Vector &vecDirShooting, 
		const Vector &vecSpread, float flDistance, int iAmmoType, int iTracerFreq = 4, 
		int firingEntID = -1, int attachmentID = -1, float iDamage = 0.0f, 
		CBaseEntity *pAttacker = NULL, bool bFirstShotAccurate = false );


	bool		CustomDispatchKeyValue(const char *szKeyName, const char *szValue);
	bool		CustomAcceptInput(const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller, variant_t Value, int outputID);

	inline bool IsSimulatedEveryTick() const { return m_bSimulatedEveryTick; }
	inline void SetSimulatedEveryTick( bool sim ) { m_bSimulatedEveryTick = sim; }

private:
	bool		NameMatchesComplex( const char *pszNameOrWildcard );
	bool		ClassMatchesComplex( const char *pszClassOrWildcard );


public: // All the internal hook implementations for the above virtuals
	DECLARE_DEFAULTHEADER(SetModel, void, (const char *model));
	DECLARE_DEFAULTHEADER(Teleport, void, (const Vector *origin, const QAngle* angles, const Vector *velocity));
	DECLARE_DEFAULTHEADER(UpdateOnRemove, void, ());
	DECLARE_DEFAULTHEADER(Spawn, void, ());
	DECLARE_DEFAULTHEADER(OnTakeDamage, int, (const CTakeDamageInfo &info));
	DECLARE_DEFAULTHEADER(Think, void, ());
	DECLARE_DEFAULTHEADER(AcceptInput, bool, (const char *szInputName, CBaseEntity *pActivator, CBaseEntity *pCaller,variant_t Value, int outputID));
	DECLARE_DEFAULTHEADER(StartTouch, void, (CBaseEntity *pOther));
	DECLARE_DEFAULTHEADER(Touch, void, (CBaseEntity *pOther));
	DECLARE_DEFAULTHEADER(EndTouch, void, (CBaseEntity *pOther));
	DECLARE_DEFAULTHEADER(GetSoundEmissionOrigin, Vector, ());
	DECLARE_DEFAULTHEADER(VPhysicsTakeDamage, int, (const CTakeDamageInfo &inputInfo));
	DECLARE_DEFAULTHEADER(VPhysicsGetObjectList, int, (IPhysicsObject **pList, int listMax));
	DECLARE_DEFAULTHEADER(GetServerClass, ServerClass *, ());
	DECLARE_DEFAULTHEADER(Classify, Class_T, ());
	DECLARE_DEFAULTHEADER(ShouldCollide, bool, (int collisionGroup, int contentsMask));
	DECLARE_DEFAULTHEADER(ObjectCaps, int, ());
	DECLARE_DEFAULTHEADER(IsNPC, bool, ());
	DECLARE_DEFAULTHEADER(SetParent, void, (CBaseEntity* pNewParent, int iAttachment));
	DECLARE_DEFAULTHEADER(DecalTrace, void, (trace_t *pTrace, char const *decalName));
	DECLARE_DEFAULTHEADER(Event_Killed, void, (const CTakeDamageInfo &info));
	DECLARE_DEFAULTHEADER(TraceAttack, void, (const CTakeDamageInfo &info, const Vector &vecDir, trace_t *ptr, CDmgAccumulator *pAccumulator));
	DECLARE_DEFAULTHEADER(BodyTarget, Vector, (const Vector &posSrc, bool bNoisy));
	DECLARE_DEFAULTHEADER(IsAlive, bool, ());
	DECLARE_DEFAULTHEADER(WorldSpaceCenter, const Vector &, () const);
	DECLARE_DEFAULTHEADER(PhysicsSimulate, void, ());
	DECLARE_DEFAULTHEADER(BloodColor, int, ());
	DECLARE_DEFAULTHEADER(StopLoopingSounds, void, ());
	DECLARE_DEFAULTHEADER(SetOwnerEntity, void, (CBaseEntity* pOwner));
	DECLARE_DEFAULTHEADER(Activate, void, ());
	DECLARE_DEFAULTHEADER(HeadTarget, Vector, (const Vector &posSrc));
	DECLARE_DEFAULTHEADER(GetAutoAimRadius, float, ());
	DECLARE_DEFAULTHEADER(PhysicsSolidMaskForEntity, unsigned int, () const);
	DECLARE_DEFAULTHEADER(CanStandOn, bool, (CBaseEntity *pSurface) const);
	DECLARE_DEFAULTHEADER(IsMoving, bool, ());
	DECLARE_DEFAULTHEADER(GetSmoothedVelocity, Vector, ());
	DECLARE_DEFAULTHEADER(Use, void, (CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value));
	DECLARE_DEFAULTHEADER(FVisible_Entity, bool, (CBaseEntity *pEntity, int traceMask = MASK_BLOCKLOS, CBaseEntity **ppBlocker = NULL));
	DECLARE_DEFAULTHEADER(FVisible_Vector, bool, (const Vector &vecTarget, int traceMask = MASK_BLOCKLOS, CBaseEntity **ppBlocker = NULL));
	DECLARE_DEFAULTHEADER(EarPosition, Vector, () const);
	DECLARE_DEFAULTHEADER(GetAutoAimCenter, Vector, ());
	DECLARE_DEFAULTHEADER(EyePosition, Vector, ());
	DECLARE_DEFAULTHEADER(OnRestore, void, ());
	DECLARE_DEFAULTHEADER(ImpactTrace, void, ( trace_t *pTrace, int iDamageType, const char *pCustomImpactName));
	DECLARE_DEFAULTHEADER(TestHitboxes, bool, (const Ray_t &ray, unsigned int fContentsMask, trace_t& tr));
	DECLARE_DEFAULTHEADER(VPhysicsCollision, void, (int index, gamevcollisionevent_t *pEvent ));
	DECLARE_DEFAULTHEADER(VPhysicsShadowCollision, void, (int index, gamevcollisionevent_t *pEvent ));
	DECLARE_DEFAULTHEADER(Splash, void, ());
	DECLARE_DEFAULTHEADER(ShouldSavePhysics, bool, ());
	DECLARE_DEFAULTHEADER(CreateVPhysics, bool, ());
	DECLARE_DEFAULTHEADER(IsNetClient, bool, () const);
	DECLARE_DEFAULTHEADER(HasPhysicsAttacker, CBaseEntity *, (float dt));
	DECLARE_DEFAULTHEADER(EyeAngles, const QAngle &, ());
	DECLARE_DEFAULTHEADER(Save, int, (ISave &save));
	DECLARE_DEFAULTHEADER(Restore, int, (IRestore &restore));
	DECLARE_DEFAULTHEADER(ModifyOrAppendCriteria, void, (AI_CriteriaSet& set));
	DECLARE_DEFAULTHEADER(DeathNotice, void, ( CBaseEntity *pVictim ));
	DECLARE_DEFAULTHEADER(PassesDamageFilter, bool, ( const CTakeDamageInfo &info ));
	DECLARE_DEFAULTHEADER(Precache, void, ());
	DECLARE_DEFAULTHEADER(DispatchKeyValue, bool, ( const char *szKeyName, const char *szValue ));
	DECLARE_DEFAULTHEADER(OnEntityEvent, void, ( EntityEvent_t event, void *pEventData ));
	DECLARE_DEFAULTHEADER(VPhysicsDestroyObject, void, ());
	DECLARE_DEFAULTHEADER(TakeHealth, int, ( float flHealth, int bitsDamageType ));
	DECLARE_DEFAULTHEADER(GetAttackDamageScale, float,( CBaseEntity *pVictim ));
	DECLARE_DEFAULTHEADER(OnControls, bool, ( CBaseEntity *pControls ));
	DECLARE_DEFAULTHEADER(VPhysicsUpdate, void, ( IPhysicsObject *pPhysics ));
	DECLARE_DEFAULTHEADER(VPhysicsShadowUpdate, void, ( IPhysicsObject *pPhysics ));
	DECLARE_DEFAULTHEADER(IsTriggered, bool, ( CBaseEntity *pActivator ));
	DECLARE_DEFAULTHEADER(FireBullets, void, ( const FireBulletsInfo_t &info ));
	DECLARE_DEFAULTHEADER(GetTracerType, const char	*,());
	DECLARE_DEFAULTHEADER(UpdateTransmitState, int, ());
	DECLARE_DEFAULTHEADER(SetTransmit, void, ( CCheckTransmitInfo *pInfo, bool bAlways ));
	DECLARE_DEFAULTHEADER(CanBeSeenBy, bool, ( CBaseEntity *pNPC ));
	DECLARE_DEFAULTHEADER(IsViewable, bool, ());
	DECLARE_DEFAULTHEADER(GetResponseSystem, IResponseSystem *, ());

public:
	DECLARE_DEFAULTHEADER_DETOUR(SetLocalOrigin, void, (const Vector& origin));
	DECLARE_DEFAULTHEADER_DETOUR(PhysicsTouchTriggers, void, (const Vector *pPrevAbsOrigin));
	DECLARE_DEFAULTHEADER_DETOUR(TakeDamage, void, (const CTakeDamageInfo &inputInfo));
	DECLARE_DEFAULTHEADER_DETOUR(SetAbsOrigin, void, (const Vector& absOrigin));
	DECLARE_DEFAULTHEADER_DETOUR(SetLocalAngles, void, (const QAngle& angles));
	DECLARE_DEFAULTHEADER_DETOUR(AddStepDiscontinuity, bool, (float flTime, const Vector &vecOrigin, const QAngle &vecAngles));
	DECLARE_DEFAULTHEADER_DETOUR(VPhysicsInitShadow, IPhysicsObject *, ( bool allowPhysicsMovement, bool allowPhysicsRotation, solid_t *pSolid));
	DECLARE_DEFAULTHEADER_DETOUR(VPhysicsInitNormal, IPhysicsObject *, ( SolidType_t solidType, int nSolidFlags, bool createAsleep, solid_t *pSolid ));
	DECLARE_DEFAULTHEADER_DETOUR(VPhysicsInitStatic, IPhysicsObject *, ());

protected: // CEntity	
	CBaseEntity *m_pEntity;
	edict_t *m_pEdict;

protected: //Sendprops
	DECLARE_SENDPROP(uint8_t, m_iTeamNum);
	DECLARE_SENDPROP(Vector, m_vecOrigin);
	DECLARE_SENDPROP(int, m_CollisionGroup);
	DECLARE_SENDPROP(CFakeHandle, m_hOwnerEntity);
	DECLARE_SENDPROP(unsigned short, m_usSolidFlags);
	DECLARE_SENDPROP(color32, m_clrRender);
	DECLARE_SENDPROP(unsigned char, m_nSolidType);
	DECLARE_SENDPROP(Vector, m_vecMins);
	DECLARE_SENDPROP(Vector, m_vecMaxs);	
	DECLARE_SENDPROP(short, m_nModelIndex);
	DECLARE_SENDPROP(QAngle, m_angRotation);
	DECLARE_SENDPROP(unsigned char, m_iParentAttachment);

public:
	DECLARE_SENDPROP(unsigned char, m_nRenderMode);
	DECLARE_SENDPROP(unsigned char, m_nRenderFX);
	DECLARE_SENDPROP(unsigned char, m_nSurroundType);
	DECLARE_SENDPROP(Vector, m_vecSpecifiedSurroundingMins);
	DECLARE_SENDPROP(Vector, m_vecSpecifiedSurroundingMaxs);


protected: //Datamaps
	DECLARE_DATAMAP(Vector, m_vecAbsOrigin);
	DECLARE_DATAMAP(Vector, m_vecAbsVelocity);
	DECLARE_DATAMAP(matrix3x4_t, m_rgflCoordinateFrame);
	DECLARE_DATAMAP(Vector, m_vecVelocity);
	DECLARE_DATAMAP(QAngle, m_vecAngVelocity);
	DECLARE_DATAMAP(Vector, m_vecBaseVelocity);
	DECLARE_DATAMAP(CFakeHandle, m_hMoveParent);
	DECLARE_DATAMAP(CFakeHandle, m_hMoveChild);
	DECLARE_DATAMAP(CFakeHandle, m_hMovePeer);
	DECLARE_DATAMAP(int, m_iEFlags);
	DECLARE_DATAMAP(IPhysicsObject *, m_pPhysicsObject);
	DECLARE_DATAMAP(int, m_nNextThinkTick);
	DECLARE_DATAMAP(CFakeHandle, m_pParent);
	DECLARE_DATAMAP(unsigned char, m_MoveType);
	DECLARE_DATAMAP(unsigned char, m_MoveCollide);
	DECLARE_DATAMAP(string_t, m_iName);
	
	DECLARE_DATAMAP(CCollisionProperty, m_Collision);
	DECLARE_DATAMAP(Vector, m_vecViewOffset);
	DECLARE_DATAMAP(int, m_fFlags);
	DECLARE_DATAMAP(CServerNetworkProperty , m_Network);
	DECLARE_DATAMAP(unsigned char, m_nWaterLevel);
	DECLARE_DATAMAP(CFakeHandle, m_hGroundEntity);
	DECLARE_DATAMAP(QAngle, m_angAbsRotation);
	DECLARE_DATAMAP(float, m_flGravity);
	DECLARE_DATAMAP(int, m_iMaxHealth);
	DECLARE_DATAMAP(int, m_fEffects);
	DECLARE_DATAMAP(string_t, m_ModelName);
	DECLARE_DATAMAP(float, m_flGroundChangeTime);
	DECLARE_DATAMAP(int, m_nLastThinkTick);
	DECLARE_DATAMAP(float, m_flNavIgnoreUntilTime);
	DECLARE_DATAMAP(unsigned char, m_nWaterType);
	DECLARE_DATAMAP(float, m_flFriction);
	DECLARE_DATAMAP(float, m_flMoveDoneTime);
	DECLARE_DATAMAP(float, m_flLocalTime);
	DECLARE_DATAMAP(CUtlVector< thinkfunc_t >, m_aThinkFunctions);
	DECLARE_DATAMAP(CUtlVector< ResponseContext_t >, m_ResponseContexts);
	DECLARE_DATAMAP(bool, m_bSimulatedEveryTick);

public:
	DECLARE_DATAMAP(char, m_lifeState);
	DECLARE_DATAMAP(int, m_iHealth);
	DECLARE_DATAMAP(string_t, m_iClassname);
	DECLARE_DATAMAP(char, m_takedamage);
	DECLARE_DATAMAP(float , m_flAnimTime);
	DECLARE_DATAMAP(float , m_flPrevAnimTime);
	DECLARE_DATAMAP(float , m_flSpeed);
	DECLARE_DATAMAP(string_t , m_target);
	DECLARE_DATAMAP(Vector, m_vecSurroundingMins);
	DECLARE_DATAMAP(Vector, m_vecSurroundingMaxs);
	DECLARE_DATAMAP_OFFSET(float, m_flRadius);
	DECLARE_DATAMAP(VALVE_BASEPTR, m_pfnThink);
	DECLARE_DATAMAP(unsigned char, m_triggerBloat);
	DECLARE_DATAMAP_OFFSET(CBaseEntity *, m_pLink);
	DECLARE_DATAMAP(int, m_spawnflags);



	/* Thinking Stuff */
	void (CEntity::*ce_m_pfnThink)(void);
	void (CEntity::*ce_m_pfnTouch)(CEntity *pOther);
	void (CEntity::*ce_m_pfnUse)(CBaseEntity *pActivator, CBaseEntity *pCaller, USE_TYPE useType, float value);
	
	friend class CECollisionProperty;
};

/**
 * Fake definition of CBaseEntity, as long as we don't declare any data member we should be fine with this.
 * Also gives us access to IServerEntity and below without explicit casting.
 */
class CBaseEntity : public IServerEntity
{
public:
	CBaseEntity* operator= (CEntity* const input)
	{
		return input->BaseEntity();
	}
	operator CEntity* ()
	{
		return CEntityLookup::Instance(this);
	}
};

inline const color32 CEntity::GetRenderColor() const
{
	return m_clrRender;
}

inline void CEntity::SetRenderColor( byte r, byte g, byte b )
{
	color32 clr = { r, g, b, m_clrRender->a };
	m_clrRender = clr;
}

inline void CEntity::SetRenderColor( byte r, byte g, byte b, byte a )
{
	color32 clr = { r, g, b, a };
	m_clrRender = clr;
}

inline void CEntity::SetRenderColorR( byte r )
{
	SetRenderColor( r, GetRenderColor().g, GetRenderColor().b );
}

inline void CEntity::SetRenderColorG( byte g )
{
	SetRenderColor( GetRenderColor().r, g, GetRenderColor().b );
}

inline void CEntity::SetRenderColorB( byte b )
{
	SetRenderColor( GetRenderColor().r, GetRenderColor().g, b );
}

inline void CEntity::SetRenderColorA( byte a )
{
	SetRenderColor( GetRenderColor().r, GetRenderColor().g, GetRenderColor().b, a );
}

inline int CEntity::GetFlags() const
{
	return m_fFlags;
}

inline int CEntity::GetSpawnFlags() const
{ 
	return m_spawnflags; 
}

inline void CEntity::AddSpawnFlags(int nFlags) 
{ 
	m_spawnflags |= nFlags; 
}

inline void CEntity::RemoveSpawnFlags(int nFlags) 
{ 
	m_spawnflags &= ~nFlags; 
}

inline void CEntity::ClearSpawnFlags() 
{ 
	m_spawnflags = 0; 
}

inline bool CEntity::HasSpawnFlags( int nFlags ) const
{ 
	return (m_spawnflags & nFlags) != 0; 
}

inline int CEntity::GetCollisionGroup() const
{
	return m_CollisionGroup;
}

inline int CEntity::GetModelIndex( void ) const
{
	return m_nModelIndex;
}

inline void CEntity::SetModelIndex( int index )
{
	m_nModelIndex = index;
	DispatchUpdateTransmitState();
}

inline bool CEntity::IsTransparent() const
{
	return m_nRenderMode != kRenderNormal;
}

inline void CEntity::SetRenderMode( RenderMode_t nRenderMode )
{
	m_nRenderMode = nRenderMode;
}

inline const QAngle& CEntity::GetLocalAngles() const
{
	return m_angRotation;
}

inline const Vector& CEntity::GetViewOffset() const 
{ 
	return m_vecViewOffset; 
}

inline const Vector& CEntity::WorldAlignMins( ) const
{
	return CollisionProp_Actual()->OBBMins();
}

inline const Vector& CEntity::WorldAlignMaxs( ) const
{
	return CollisionProp_Actual()->OBBMaxs();
}

inline const Vector& CEntity::WorldAlignSize( ) const
{
	return CollisionProp_Actual()->OBBSize();
}

inline float CEntity::BoundingRadius() const
{
	return CollisionProp_Actual()->BoundingRadius();
}

inline void CEntity::SetGravity(float flGravity) 
{ 
	m_flGravity = flGravity; 
}

inline float CEntity::GetGravity() const 
{ 
	return m_flGravity; 
}

inline CEntity *GetWorldEntity()
{
	return my_g_WorldEntity;
}

inline void CEntity::SetModelName( string_t name )
{
	m_ModelName = name;
	DispatchUpdateTransmitState();
}

inline string_t CEntity::GetModelName( void ) const
{
	return m_ModelName;
}

inline void	CEntity::SetNavIgnore( float duration )
{
	float flNavIgnoreUntilTime = ( duration == FLT_MAX ) ? FLT_MAX : gpGlobals->curtime + duration;
	if ( flNavIgnoreUntilTime > m_flNavIgnoreUntilTime )
		m_flNavIgnoreUntilTime = flNavIgnoreUntilTime;
}

inline void	CEntity::ClearNavIgnore()
{
	m_flNavIgnoreUntilTime = 0;
}

inline bool	CEntity::IsNavIgnored() const
{
	return ( gpGlobals->curtime <= m_flNavIgnoreUntilTime );
}

inline void	CEntity::NetworkStateChanged()
{
	NetworkProp()->NetworkStateChanged();
}

inline bool CEntity::NameMatches( const char *pszNameOrWildcard )
{
	if ( IDENT_STRINGS(m_iName, pszNameOrWildcard) )
		return true;
	return NameMatchesComplex( pszNameOrWildcard );
}

inline bool CEntity::NameMatches( string_t nameStr )
{
	if ( IDENT_STRINGS(m_iName, nameStr) )
		return true;
	return NameMatchesComplex( STRING(nameStr) );
}

inline bool CEntity::ClassMatches( const char *pszClassOrWildcard )
{
	if ( IDENT_STRINGS(m_iClassname, pszClassOrWildcard ) )
		return true;
	return ClassMatchesComplex( pszClassOrWildcard );
}

inline bool CEntity::ClassMatches( string_t nameStr )
{
	if ( IDENT_STRINGS(m_iClassname, nameStr ) )
		return true;
	return ClassMatchesComplex( STRING(nameStr) );
}


inline const Vector& CEntity::GetBaseVelocity() const 
{ 
	return m_vecBaseVelocity; 
}

inline void CEntity::SetBaseVelocity( const Vector& v ) 
{ 
	m_vecBaseVelocity = v; 
}

inline bool CEntity::IsPointSized() const
{
	return CollisionProp_Actual()->BoundingRadius() == 0.0f;
}

inline bool CEntity::IsEffectActive( int nEffects ) const
{ 
	return (*(m_fEffects.ptr) & nEffects) != 0; 
}

inline bool CEntity::IsMarkedForDeletion( void ) 
{ 
	return (m_iEFlags & EFL_KILLME); 
}

inline CEntity* CEntity::GetParent()
{
	return m_pParent;
}

inline float CEntity::GetFriction( void ) const
{ 
	return m_flFriction; 
}

inline void CEntity::SetFriction( float flFriction )
{ 
	m_flFriction = flFriction; 
}

inline unsigned char CEntity::GetParentAttachment() const
{
	return m_iParentAttachment;
}

inline float CEntity::GetLocalTime( void ) const
{ 
	return m_flLocalTime; 
}




void UTIL_PatchOutputRestoreOps(datamap_t *pMap);

inline bool FClassnameIs( CEntity *pEntity, const char *szClassname )
{ 
	Assert( pEntity );
	if ( pEntity == NULL )
		return false;

	return !strcmp( pEntity->GetClassname(), szClassname ) ? true : false; 
}


inline CCombatCharacter *ToBaseCombatCharacter( CEntity *pEntity )
{
	if ( !pEntity )
		return NULL;

	return pEntity->MyCombatCharacterPointer();
}

inline bool IsEntityAngularVelocityReasonable( const Vector &q )
{
	const float k_flMaxAngularVelocity = 360.0f * 10.0f;
	const float k_flMaxEntitySpinRate = k_flMaxAngularVelocity * 10.0f;

	float r = k_flMaxEntitySpinRate;
	return
		q.x > -r && q.x < r &&
		q.y > -r && q.y < r &&
		q.z > -r && q.z < r;
}

class CPointEntity : public CEntity
{
public:
	CE_DECLARE_CLASS( CPointEntity, CEntity );

	//void	Spawn( void );
	//virtual int	ObjectCaps( void ) { return BaseClass::ObjectCaps() & ~FCAP_ACROSS_TRANSITION; }
private:

};


#endif // _INCLUDE_CENTITY_H_
