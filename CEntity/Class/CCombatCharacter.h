
#ifndef _INCLUDE_CCOMBATCHARACTER_H_
#define _INCLUDE_CCOMBATCHARACTER_H_

#include "CFlex.h"
#include "ai_hull.h"
#include "weapon_proficiency.h"

enum Capability_t 
{
	bits_CAP_MOVE_GROUND			= 0x00000001, // walk/run
	bits_CAP_MOVE_JUMP				= 0x00000002, // jump/leap
	bits_CAP_MOVE_FLY				= 0x00000004, // can fly, move all around
	bits_CAP_MOVE_CLIMB				= 0x00000008, // climb ladders
	bits_CAP_MOVE_SWIM				= 0x00000010, // navigate in water			// UNDONE - not yet implemented
	bits_CAP_MOVE_CRAWL				= 0x00000020, // crawl						// UNDONE - not yet implemented
	bits_CAP_MOVE_SHOOT				= 0x00000040, // tries to shoot weapon while moving
	bits_CAP_SKIP_NAV_GROUND_CHECK	= 0x00000080, // optimization - skips ground tests while computing navigation
	bits_CAP_USE					= 0x00000100, // open doors/push buttons/pull levers
	//bits_CAP_HEAR					= 0x00000200, // can hear forced sounds
	bits_CAP_AUTO_DOORS				= 0x00000400, // can trigger auto doors
	bits_CAP_OPEN_DOORS				= 0x00000800, // can open manual doors
	bits_CAP_TURN_HEAD				= 0x00001000, // can turn head, always bone controller 0
	bits_CAP_WEAPON_RANGE_ATTACK1	= 0x00002000, // can do a weapon range attack 1
	bits_CAP_WEAPON_RANGE_ATTACK2	= 0x00004000, // can do a weapon range attack 2
	bits_CAP_WEAPON_MELEE_ATTACK1	= 0x00008000, // can do a weapon melee attack 1
	bits_CAP_WEAPON_MELEE_ATTACK2	= 0x00010000, // can do a weapon melee attack 2
	bits_CAP_INNATE_RANGE_ATTACK1	= 0x00020000, // can do a innate range attack 1
	bits_CAP_INNATE_RANGE_ATTACK2	= 0x00040000, // can do a innate range attack 1
	bits_CAP_INNATE_MELEE_ATTACK1	= 0x00080000, // can do a innate melee attack 1
	bits_CAP_INNATE_MELEE_ATTACK2	= 0x00100000, // can do a innate melee attack 1
	bits_CAP_USE_WEAPONS			= 0x00200000, // can use weapons (non-innate attacks)
	//bits_CAP_STRAFE					= 0x00400000, // strafe ( walk/run sideways)
	bits_CAP_ANIMATEDFACE			= 0x00800000, // has animated eyes/face
	bits_CAP_USE_SHOT_REGULATOR		= 0x01000000, // Uses the shot regulator for range attack1
	bits_CAP_FRIENDLY_DMG_IMMUNE	= 0x02000000, // don't take damage from npc's that are D_LI
	bits_CAP_SQUAD					= 0x04000000, // can form squads
	bits_CAP_DUCK					= 0x08000000, // cover and reload ducking
	bits_CAP_NO_HIT_PLAYER			= 0x10000000, // don't hit players
	bits_CAP_AIM_GUN				= 0x20000000, // Use arms to aim gun, not just body
	bits_CAP_NO_HIT_SQUADMATES		= 0x40000000, // none
	bits_CAP_SIMPLE_RADIUS_DAMAGE	= 0x80000000, // Do not use robust radius damage model on this character.
};

#define bits_CAP_DOORS_GROUP    (bits_CAP_AUTO_DOORS | bits_CAP_OPEN_DOORS)
#define bits_CAP_RANGE_ATTACK_GROUP	(bits_CAP_WEAPON_RANGE_ATTACK1 | bits_CAP_WEAPON_RANGE_ATTACK2)
#define bits_CAP_MELEE_ATTACK_GROUP	(bits_CAP_WEAPON_MELEE_ATTACK1 | bits_CAP_WEAPON_MELEE_ATTACK2)

#define SF_RAGDOLL_BOOGIE_ELECTRICAL	0x10000
#define SF_RAGDOLL_BOOGIE_ELECTRICAL_NARROW_BEAM	0x20000

enum Disposition_t 
{
	D_ER,		// Undefined - error
	D_HT,		// Hate
	D_FR,		// Fear
	D_LI,		// Like
	D_NU		// Neutral
};

struct Relationship_t
{
	EHANDLE			entity;			// Relationship to a particular entity
	Class_T			classType;		// Relationship to a class  CLASS_NONE = not class based (Def. in baseentity.h)
	Disposition_t	disposition;	// D_HT (Hate), D_FR (Fear), D_LI (Like), D_NT (Neutral)
	int				priority;		// Relative importance of this relationship (higher numbers mean more important)

	DECLARE_SIMPLE_DATADESC();
};


class CCombatWeapon;

class CCombatCharacter : public CFlex
{
public:
	CE_DECLARE_CLASS(CCombatCharacter, CFlex);

public:
	Hull_t				GetHullType() const				{ return m_eHull; }
	void				SetHullType( Hull_t hullType )	{ m_eHull = hullType; }
	void				SetImpactEnergyScale( float fScale ) { m_impactEnergyScale = fScale; }
	float				GetNextAttack() const { return m_flNextAttack; }
	void				SetNextAttack( float flWait ) { m_flNextAttack = flWait; }
	void				SetRelationshipString( string_t theString ) { m_RelationshipString = theString; }

	void SetBloodColor(int nBloodColor);

	Vector CalcDamageForceVector( const CTakeDamageInfo &info );

	IServerVehicle *GetVehicle();

	int GetAmmoCount( int iAmmoIndex ) const;
	void AddAmmo( int iAmmoIndex , int iAmmount);
	void RemoveAmmo( int iCount, int iAmmoIndex );
	void RemoveAmmo( int iCount, const char *szName );
	virtual bool GiveAmmo(int nCount, int nAmmoIndex);

	bool				IsAllowedToPickupWeapons( void ) { return !m_bPreventWeaponPickup; }
	void				SetPreventWeaponPickup( bool bPrevent ) { m_bPreventWeaponPickup = bPrevent; }

	int					WeaponCount() const;
	CCombatWeapon		*GetWeapon( int i ) const;
	CCombatWeapon		*GetActiveWeapon() const;

	void				Weapon_DropAll( bool bDisallowWeaponPickup = false );
	bool				Weapon_Detach( CCombatWeapon *pWeapon );

	void				SetActiveWeapon( CCombatWeapon *pNewWeapon );
	void				ClearActiveWeapon() { SetActiveWeapon( NULL ); }
		
	bool				HaveThisWeaponType(CCombatWeapon *pWeapon);
	
	int					FAKE_OnTakeDamage_Alive( const CTakeDamageInfo &info );
	
	CCombatWeapon		*Weapon_OwnsThisType( const char *pszWeapon, int iSubType = 0 ) const;

	bool				SwitchToNextBestWeapon(CCombatWeapon *pCurrent);

	int					LastHitGroup() const { return m_LastHitGroup; }
	void				SetLastHitGroup( int nHitGroup ) { m_LastHitGroup = nHitGroup; }
	
	bool				DispatchInteraction( int interactionType, void *data, CBaseEntity* sourceEnt )	{ return ( interactionType > 0 ) ? HandleInteraction( interactionType, data, sourceEnt ) : false; }

private:
	void ThrowDirForWeaponStrip( CCombatWeapon *pWeapon, const Vector &vecForward, Vector *pVecThrowDir );
	void DropWeaponForWeaponStrip( CCombatWeapon *pWeapon, const Vector &vecForward, const QAngle &vecAngles, float flDiameter );


public:
	virtual Disposition_t IRelationType(CBaseEntity *pTarget);
	virtual int	IRelationPriority( CBaseEntity *pTarget );
	virtual bool FInAimCone_Entity(CBaseEntity *pEntity);
	virtual bool FVisible_Entity(CBaseEntity *pEntity, int traceMask = MASK_BLOCKLOS, CBaseEntity **ppBlocker = NULL );
	virtual Vector EyeDirection3D();
	virtual bool CorpseGib( const CTakeDamageInfo &info );
	virtual int	OnTakeDamage_Alive( const CTakeDamageInfo &info );
	virtual int	OnTakeDamage_Dying( const CTakeDamageInfo &info );
	virtual int	OnTakeDamage_Dead( const CTakeDamageInfo &info );
	virtual bool HandleInteraction( int interactionType, void *data, CBaseEntity* sourceEnt);
	virtual CCombatCharacter *MyCombatCharacterPointer( void ) { return this; }
	virtual CBaseEntity	*CheckTraceHullAttack_Float( float flDist, const Vector &mins, const Vector &maxs, int iDamage, int iDmgType, float forceScale = 1.0f, bool bDamageAnyNPC = false );
	virtual CBaseEntity	*CheckTraceHullAttack_Vector( const Vector &vStart, const Vector &vEnd, const Vector &mins, const Vector &maxs, int iDamage, int iDmgType, float flForceScale = 1.0f, bool bDamageAnyNPC = false );
	virtual bool BecomeRagdoll( const CTakeDamageInfo &info, const Vector &forceVector );
	virtual bool BecomeRagdollOnClient( const Vector &force );
	virtual bool IsInAVehicle()
	{
		return false;
	}
	virtual CBaseEntity *GetVehicleEntity(); 
	virtual bool FInViewCone_Entity( CBaseEntity *pEntity );
	virtual bool FInViewCone_Vector( const Vector &vecSpot );
	virtual bool CanBecomeServerRagdoll( void );
	virtual void Event_Dying(CTakeDamageInfo const& info);
	virtual bool ShouldGib( const CTakeDamageInfo &info );
	virtual QAngle BodyAngles();
	virtual Vector BodyDirection3D();
	virtual Vector BodyDirection2D();
	virtual void OnChangeActiveWeapon(CBaseEntity *pOldWeapon, CBaseEntity *pNewWeapon);
	virtual void OnFriendDamaged( CBaseEntity *pSquadmate, CBaseEntity *pAttacker );
	virtual void AddClassRelationship( Class_T nClass, Disposition_t nDisposition, int nPriority );
	virtual void AddEntityRelationship( CBaseEntity *pEntity, Disposition_t nDisposition, int nPriority );
	virtual bool RemoveEntityRelationship( CBaseEntity *pEntity );
	virtual Activity Weapon_TranslateActivity( Activity baseAct, bool *pRequired = NULL );
	virtual	void Weapon_Drop( CBaseEntity *pWeapon, const Vector *pvecTarget = NULL, const Vector *pVelocity = NULL );
	virtual bool Weapon_CanUse( CBaseEntity *pWeapon );
	virtual void Weapon_Equip( CBaseEntity *pWeapon );
	virtual CBaseEntity *Weapon_GetSlot( int slot ) const;
	virtual Vector Weapon_ShootPosition();
	virtual bool BecomeRagdollBoogie( CBaseEntity *pKiller, const Vector &forceVector, float duration, int flags );
	virtual void NotifyFriendsOfDamage( CBaseEntity *pAttackerEntity );
	virtual	bool Weapon_Switch( CBaseEntity *pWeapon, int viewmodelindex = 0 );
	virtual Vector GetAttackSpread( CBaseEntity *pWeapon, CBaseEntity *pTarget );
	virtual bool ShouldShootMissTarget( CBaseEntity *pAttacker );
	virtual CBaseEntity *FindMissTarget( void );
	virtual	float GetSpreadBias( CBaseEntity *pWeapon, CBaseEntity *pTarget );
	virtual Vector	HeadDirection2D();
	virtual Vector	HeadDirection3D();
	virtual Vector	EyeDirection2D();
	virtual WeaponProficiency_t CalcWeaponProficiency( CBaseEntity *pWeapon );

public:
	DECLARE_DEFAULTHEADER(IRelationType, Disposition_t, (CBaseEntity *pTarget));
	DECLARE_DEFAULTHEADER(IRelationPriority, int, ( CBaseEntity *pTarget ));
	DECLARE_DEFAULTHEADER(FInAimCone_Entity, bool, (CBaseEntity *pEntity));
	DECLARE_DEFAULTHEADER(EyeDirection3D, Vector, ());
	DECLARE_DEFAULTHEADER(CorpseGib, bool, (const CTakeDamageInfo &info));
	DECLARE_DEFAULTHEADER(OnTakeDamage_Alive, int, (const CTakeDamageInfo &info));
	DECLARE_DEFAULTHEADER(OnTakeDamage_Dying, int, (const CTakeDamageInfo &info));
	DECLARE_DEFAULTHEADER(OnTakeDamage_Dead, int, (const CTakeDamageInfo &info));
	DECLARE_DEFAULTHEADER(HandleInteraction, bool, (int interactionType, void *data, CBaseEntity* sourceEnt));
	DECLARE_DEFAULTHEADER(CheckTraceHullAttack_Float, CBaseEntity *,(float flDist, const Vector &mins, const Vector &maxs, int iDamage, int iDmgType, float forceScale, bool bDamageAnyNPC));
	DECLARE_DEFAULTHEADER(CheckTraceHullAttack_Vector, CBaseEntity *, (const Vector &vStart, const Vector &vEnd, const Vector &mins, const Vector &maxs, int iDamage, int iDmgType, float flForceScale, bool bDamageAnyNPC));
	DECLARE_DEFAULTHEADER(BecomeRagdoll, bool, (const CTakeDamageInfo &info, const Vector &forceVector));
	DECLARE_DEFAULTHEADER(BecomeRagdollOnClient, bool, (const Vector &force));
	//DECLARE_DEFAULTHEADER(IsInAVehicle, bool, ());
	DECLARE_DEFAULTHEADER(GetVehicleEntity, CBaseEntity *, ());
	DECLARE_DEFAULTHEADER(FInViewCone_Entity, bool, (CBaseEntity *));
	DECLARE_DEFAULTHEADER(FInViewCone_Vector, bool, (const Vector &vecSpot));
	DECLARE_DEFAULTHEADER(CanBecomeServerRagdoll, bool, ());
	DECLARE_DEFAULTHEADER(Event_Dying, void, (CTakeDamageInfo const& info));
	DECLARE_DEFAULTHEADER(ShouldGib, bool, (const CTakeDamageInfo &info ));
	DECLARE_DEFAULTHEADER(BodyAngles, QAngle, ());
	DECLARE_DEFAULTHEADER(BodyDirection3D, Vector, ());
	DECLARE_DEFAULTHEADER(BodyDirection2D, Vector, ());
	DECLARE_DEFAULTHEADER(OnChangeActiveWeapon, void, (CBaseEntity *pOldWeapon, CBaseEntity *pNewWeapon));
	DECLARE_DEFAULTHEADER(OnFriendDamaged, void,( CBaseEntity *pSquadmate, CBaseEntity *pAttacker ));
	DECLARE_DEFAULTHEADER(AddClassRelationship, void, ( Class_T nClass, Disposition_t nDisposition, int nPriority ));
	DECLARE_DEFAULTHEADER(AddEntityRelationship, void, ( CBaseEntity *pEntity, Disposition_t nDisposition, int nPriority ));
	DECLARE_DEFAULTHEADER(RemoveEntityRelationship, bool, ( CBaseEntity *pEntity ));
	DECLARE_DEFAULTHEADER(Weapon_TranslateActivity, Activity, ( Activity baseAct, bool *pRequired ));
	DECLARE_DEFAULTHEADER(Weapon_Drop, void, ( CBaseEntity *pWeapon, const Vector *pvecTarget , const Vector *pVelocity ));
	DECLARE_DEFAULTHEADER(Weapon_CanUse, bool, ( CBaseEntity *pWeapon ));
	DECLARE_DEFAULTHEADER(Weapon_Equip, void, ( CBaseEntity *pWeapon ));
	DECLARE_DEFAULTHEADER(Weapon_GetSlot, CBaseEntity *, (int slot) const);
	DECLARE_DEFAULTHEADER(Weapon_ShootPosition, Vector, ());
	DECLARE_DEFAULTHEADER(BecomeRagdollBoogie, bool, ( CBaseEntity *pKiller, const Vector &forceVector, float duration, int flags ));
	DECLARE_DEFAULTHEADER(NotifyFriendsOfDamage, void, ( CBaseEntity *pAttackerEntity ));
	DECLARE_DEFAULTHEADER(Weapon_Switch, bool,( CBaseEntity *pWeapon, int viewmodelindex ));
	DECLARE_DEFAULTHEADER(GetAttackSpread, Vector, ( CBaseEntity *pWeapon, CBaseEntity *pTarget ));
	DECLARE_DEFAULTHEADER(ShouldShootMissTarget, bool, ( CBaseEntity *pAttacker ));
	DECLARE_DEFAULTHEADER(FindMissTarget, CBaseEntity*, ( void ));
	DECLARE_DEFAULTHEADER(GetSpreadBias, float, ( CBaseEntity *pWeapon, CBaseEntity *pTarget ));
	DECLARE_DEFAULTHEADER(HeadDirection2D, Vector, ());
	DECLARE_DEFAULTHEADER(HeadDirection3D, Vector, ());
	DECLARE_DEFAULTHEADER(EyeDirection2D, Vector, ());
	DECLARE_DEFAULTHEADER(CalcWeaponProficiency, WeaponProficiency_t,( CBaseEntity *pWeapon ));

public:
	static void Shutdown();
	static void AllocateDefaultRelationships();
	static int *m_lastInteraction;
	static Relationship_t** *m_DefaultRelationship;
	static void	SetDefaultRelationship( Class_T nClass, Class_T nClassTarget,  Disposition_t nDisposition, int nPriority );
	static int GetInteractionID();

public: //Sendprops
	DECLARE_SENDPROP(float, m_flNextAttack);
	DECLARE_SENDPROP(CBaseEntity * , m_hMyWeapons);
	DECLARE_SENDPROP(CFakeHandle , m_hActiveWeapon);

protected: //Datamaps
	DECLARE_DATAMAP(Hull_t, m_eHull);
	DECLARE_DATAMAP(int, m_bloodColor);
	DECLARE_DATAMAP(float, m_flFieldOfView);
	DECLARE_DATAMAP(float, m_impactEnergyScale);
	DECLARE_DATAMAP(Vector, m_HackedGunPos);
	DECLARE_DATAMAP(string_t, m_RelationshipString);
	DECLARE_DATAMAP(int , m_iAmmo);
	DECLARE_DATAMAP(bool , m_bPreventWeaponPickup);
	DECLARE_DATAMAP(float , m_flDamageAccumulator);
	DECLARE_DATAMAP(int , m_LastHitGroup);

	
};

inline int CCombatCharacter::WeaponCount() const
{
	return MAX_WEAPONS;
}


class CTraceFilterMelee : public CTraceFilterEntitiesOnly
{
public:
	// It does have a base, but we'll never network anything below here..
	DECLARE_CLASS_NOBASE( CTraceFilterMelee );
	
	CTraceFilterMelee( const IHandleEntity *passentity, int collisionGroup, CTakeDamageInfo *dmgInfo, float flForceScale, bool bDamageAnyNPC )
		: m_pPassEnt(passentity), m_collisionGroup(collisionGroup), m_dmgInfo(dmgInfo), m_pHit(NULL), m_flForceScale(flForceScale), m_bDamageAnyNPC(bDamageAnyNPC)
	{
	}
	
	virtual bool ShouldHitEntity( IHandleEntity *pHandleEntity, int contentsMask );

public:
	const IHandleEntity *m_pPassEnt;
	int					m_collisionGroup;
	CTakeDamageInfo		*m_dmgInfo;
	CEntity				*m_pHit;
	float				m_flForceScale;
	bool				m_bDamageAnyNPC;
};


void RadiusDamage( const CTakeDamageInfo &info, const Vector &vecSrc, float flRadius, int iClassIgnore, CBaseEntity *pEntityIgnore );

#endif
