
#ifndef _INCLUDE_CCOMBATWEAPON_H
#define _INCLUDE_CCOMBATWEAPON_H

#include "CEntity.h"
#include "CAnimating.h"
#include "CItem.h"
#include "weapon_proficiency.h"
#include "weapon_parse.h"
#include "weapon_replace.h"

class CPlayer;

//Start with a constraint in place (don't drop to floor)
#define	SF_WEAPON_START_CONSTRAINED	(1<<0)	
#define SF_WEAPON_NO_PLAYER_PICKUP	(1<<1)
#define SF_WEAPON_NO_PHYSCANNON_PUNT (1<<2)

// Put this in your derived class definition to declare it's activity table
#define DECLARE_ACTTABLE() \
	static acttable_t m_acttable[];\
	acttable_t *ActivityList( void );\
	int ActivityListCount( void );

// Put this after your derived class' definition to implement the accessors for the activity table.
#define IMPLEMENT_ACTTABLE(className) \
	acttable_t *className::ActivityList( void ) { return m_acttable; } \
	int className::ActivityListCount( void ) { return ARRAYSIZE(m_acttable); } \

typedef struct
{
	int			baseAct;
	int			weaponAct;
	bool		required;
} acttable_t;

abstract_class Template_CCombatWeapon : public CAnimating
{
public:
	CE_DECLARE_CLASS( Template_CCombatWeapon, CAnimating );

public:
	virtual void	Drop( const Vector &vecVelocity );
	virtual int		GetMaxClip1( void ) const;
	virtual int		GetSlot( void ) const;

	virtual bool	HasPrimaryAmmo( void );	
	virtual bool	UsesClipsForAmmo1( void ) const;
	virtual bool	HasSecondaryAmmo( void );
	virtual bool	UsesClipsForAmmo2( void ) const;

	virtual bool	Holster( CBaseEntity *pSwitchingTo = NULL );
	virtual bool	Deploy();
	virtual	bool	CanHolster();
	virtual	bool	Reload();

	virtual int		GetRandomBurst();
	virtual float	GetFireRate( void );
	virtual int		Weapon_CapabilitiesGet( void );
	virtual void	Operator_HandleAnimEvent( animevent_t *pEvent, CBaseEntity *pOperator );
	virtual void	WeaponSound( WeaponSound_t sound_type, float soundtime = 0.0f );
	virtual int		GetMaxClip2( void ) const;
	virtual void	Operator_ForceNPCFire( CBaseEntity  *pOperator, bool bSecondary );

	virtual void	Equip( CBaseEntity *pOwner );
	virtual	int		WeaponRangeAttack1Condition( float flDot, float flDist );
	virtual	int		WeaponRangeAttack2Condition( float flDot, float flDist );
	virtual	int		WeaponMeleeAttack1Condition( float flDot, float flDist );
	virtual	int		WeaponMeleeAttack2Condition( float flDot, float flDist );

	virtual void	PrimaryAttack();
	virtual void	SecondaryAttack();
	
	virtual void	SendWeaponAnim( int iAcvitity );
	virtual	void	ItemPostFrame();
	virtual void	Weapon_SetActivity( Activity activity, float flDuration );

	virtual Activity		ActivityOverride( Activity baseAct, bool *pRequired );
	virtual	acttable_t*		ActivityList( void );
	virtual int				ActivityListCount( void ) { return 0; }
	//virtual	int				ActivityListCount( void );

	virtual const char *GetWorldModel( void ) const;
	virtual const WeaponProficiencyInfo_t *GetProficiencyValues();

	virtual Activity		GetPrimaryAttackActivity( void );
	virtual Activity		GetSecondaryAttackActivity( void );

	virtual bool			HasAnyAmmo( void );
	virtual void			ItemPreFrame( void );
	virtual void			WeaponIdle( void );
	virtual const char		*GetViewModel( int viewmodelindex = 0 ) const;
	virtual const Vector	&GetBulletSpread( void );
	virtual int				GetMinBurst( void );
	virtual int				GetMaxBurst( void );
	virtual float			GetMinRestTime( void );
	virtual float			GetMaxRestTime( void );
	virtual bool			CanBePickedUpByNPCs( void );

public:
	DECLARE_DEFAULTHEADER(Drop, void, (const Vector &vecVelocity));
	DECLARE_DEFAULTHEADER(GetMaxClip1, int, () const);
	DECLARE_DEFAULTHEADER(GetSlot, int, () const);

	DECLARE_DEFAULTHEADER(HasPrimaryAmmo, bool, ());
	DECLARE_DEFAULTHEADER(UsesClipsForAmmo1, bool, () const);
	DECLARE_DEFAULTHEADER(HasSecondaryAmmo, bool, ());
	DECLARE_DEFAULTHEADER(UsesClipsForAmmo2, bool, () const);

	DECLARE_DEFAULTHEADER(Holster, bool, (CBaseEntity *pSwitchingTo));
	DECLARE_DEFAULTHEADER(Deploy, bool, ());
	DECLARE_DEFAULTHEADER(CanHolster, bool, ());
	DECLARE_DEFAULTHEADER(Reload, bool, ());

	DECLARE_DEFAULTHEADER(GetRandomBurst, int, ());
	DECLARE_DEFAULTHEADER(GetFireRate, float, ());
	DECLARE_DEFAULTHEADER(Weapon_CapabilitiesGet, int, ());
	DECLARE_DEFAULTHEADER(Operator_HandleAnimEvent, void, (animevent_t *pEvent, CBaseEntity *pOperator));
	DECLARE_DEFAULTHEADER(WeaponSound, void, (WeaponSound_t sound_type, float soundtime));
	DECLARE_DEFAULTHEADER(GetMaxClip2, int, () const);
	DECLARE_DEFAULTHEADER(Operator_ForceNPCFire, void, (CBaseEntity  *pOperator, bool bSecondary));

	DECLARE_DEFAULTHEADER(Equip, void, ( CBaseEntity *pOwner ));
	DECLARE_DEFAULTHEADER(WeaponRangeAttack1Condition, int, ( float flDot, float flDist ));
	DECLARE_DEFAULTHEADER(WeaponRangeAttack2Condition, int, ( float flDot, float flDist ));
	DECLARE_DEFAULTHEADER(WeaponMeleeAttack1Condition, int, ( float flDot, float flDist ));
	DECLARE_DEFAULTHEADER(WeaponMeleeAttack2Condition, int, ( float flDot, float flDist ));

	DECLARE_DEFAULTHEADER(PrimaryAttack, void, ());
	DECLARE_DEFAULTHEADER(SecondaryAttack, void, ());

	DECLARE_DEFAULTHEADER(SendWeaponAnim, void, ( int iActivity ));
	DECLARE_DEFAULTHEADER(ItemPostFrame, void, ());
	DECLARE_DEFAULTHEADER(Weapon_SetActivity, void, (Activity activity, float flDuration));

	DECLARE_DEFAULTHEADER(ActivityOverride, Activity, (Activity baseAct, bool *pRequired));
	DECLARE_DEFAULTHEADER(ActivityList, acttable_t*, ());
	//DECLARE_DEFAULTHEADER(ActivityListCount, int, ());

	DECLARE_DEFAULTHEADER(GetWorldModel, const char *, () const);
	DECLARE_DEFAULTHEADER(GetProficiencyValues, const WeaponProficiencyInfo_t *, ());

	DECLARE_DEFAULTHEADER(GetPrimaryAttackActivity, Activity, ());
	DECLARE_DEFAULTHEADER(GetSecondaryAttackActivity, Activity, ());

	DECLARE_DEFAULTHEADER(HasAnyAmmo, bool, ());
	DECLARE_DEFAULTHEADER(ItemPreFrame, void, ());
	DECLARE_DEFAULTHEADER(WeaponIdle, void, ());
	DECLARE_DEFAULTHEADER(GetViewModel, const char *, (int viewmodelindex) const);
	DECLARE_DEFAULTHEADER(GetBulletSpread, const Vector &,());

	DECLARE_DEFAULTHEADER(GetMinBurst, int, ());
	DECLARE_DEFAULTHEADER(GetMaxBurst, int, ());
	DECLARE_DEFAULTHEADER(GetMinRestTime, float, ());
	DECLARE_DEFAULTHEADER(GetMaxRestTime, float, ());

	DECLARE_DEFAULTHEADER(CanBePickedUpByNPCs, bool, ());
};

class CCombatWeapon : public CItem<Template_CCombatWeapon>
{
public:
	CE_DECLARE_CLASS( CCombatWeapon, CItem<Template_CCombatWeapon> );
	DECLARE_DATADESC();
	CE_CUSTOM_ENTITY();
	CCombatWeapon();
	virtual ~CCombatWeapon();

	void PostConstructor();

	void		Spawn();
	CEntity*	MyTouch( CPlayer *pPlayer );
	bool		IsRemoveable() { return m_bRemoveable; }
	void		SetRemoveable( bool value ) { m_bRemoveable = value; }
	
	int GetPrimaryAmmoType() const { return m_iPrimaryAmmoType; }

	void OnWeaponDrop( CPlayer *pOwner);
	void OnWeaponEquip( CPlayer *pOwner );

	bool GetObjectsOriginalParameters(Vector &vOriginalOrigin, QAngle &vOriginalAngles);

	bool		UsesPrimaryAmmo( void ) const
	{
		if ( m_iPrimaryAmmoType < 0 )
			return false;
		return true;
	}
	bool		UsesSecondaryAmmo( void ) const
	{
		if ( m_iSecondaryAmmoType < 0 )
			return false;
		return true;
	}

	int			Clip1() const { return m_iClip1; }
	int			Clip2() const { return m_iClip2; }
	int			GetSubType( void ) { return m_iSubType; }

	Activity	GetIdealActivity( void ) { return m_IdealActivity; }
	void		SetActivity( Activity eActivity ) { m_Activity = eActivity; }
	Activity	GetActivity( void ) { return m_Activity; }

	virtual Activity GetPrimaryAttackActivity( void );
	virtual Activity GetSecondaryAttackActivity( void );


	virtual void SetWeaponIdleTime( float time );
	virtual float GetWeaponIdleTime( void );

	virtual int CapabilitiesGet()
	{
		return 0;
	}

	CCombatCharacter *GetOwner();

	void		Lock( float lockTime, CEntity *pLocker );
	bool		IsLocked( CEntity *pAsker );

	bool		IsConstrained() const { return *(m_pConstraint.ptr) != NULL; }

	void		SaveMinMaxRange();
	void		RestoreMinMaxRange();

	void		WeaponReplaceGiveAmmo(CPlayer *pPlayer);
	void		WeaponReplaceCreate(CCombatWeapon *pWeapon);

	void		CustomWeaponSound(int index, const Vector &origin, const char *sound);
	void		CustomWeaponSound(const Vector &origin, const char *sound, WeaponSound_t sound_type);

public:
	static const	WeaponProficiencyInfo_t *GetDefaultProficiencyValues();

	static int GetAvailableWeaponsInBox( CCombatWeapon **pList, int listMax, const Vector &mins, const Vector &maxs );

protected:
	float m_flRaiseTime;
	float m_flHolsterTime;
	int m_iPrimaryAttacks;
	int m_iSecondaryAttacks;
	bool m_bRemoveOnDrop;
	bool m_bAltFiresUnderwater;

public:
	DECLARE_SENDPROP(int, m_iPrimaryAmmoType);
	DECLARE_SENDPROP(int, m_iSecondaryAmmoType);
	DECLARE_SENDPROP(int, m_iClip1);
	DECLARE_SENDPROP(int, m_iClip2);
	DECLARE_SENDPROP(float, m_flNextPrimaryAttack);
	DECLARE_SENDPROP(float, m_flNextSecondaryAttack);
	DECLARE_DATAMAP(float, m_fMinRange1);
	DECLARE_DATAMAP(float, m_fMinRange2);
	DECLARE_DATAMAP(float, m_fMaxRange1);
	DECLARE_DATAMAP(float, m_fMaxRange2);
	DECLARE_SENDPROP(float, m_flTimeWeaponIdle);
	DECLARE_SENDPROP(int, m_iState);
	DECLARE_SENDPROP(int, m_iWorldModelIndex);
	DECLARE_SENDPROP(int, m_iViewModelIndex);
	DECLARE_SENDPROP(CEFakeHandle<CCombatCharacter>, m_hOwner);
	
protected: //Datamaps
	DECLARE_DATAMAP(bool, m_bRemoveable);
	DECLARE_DATAMAP(bool, m_bFireOnEmpty);
	DECLARE_DATAMAP(bool, m_bFiresUnderwater);
	//DECLARE_DATAMAP(bool, m_bAltFiresUnderwater);
	DECLARE_DATAMAP(IPhysicsConstraint *, m_pConstraint);
	DECLARE_DATAMAP(int, m_iSubType);
	DECLARE_DATAMAP(Activity, m_IdealActivity);
	DECLARE_DATAMAP_OFFSET(Activity, m_Activity);
	DECLARE_DATAMAP(float, m_fFireDuration);
	DECLARE_DATAMAP(float, m_flUnlockTime);
	DECLARE_DATAMAP(CFakeHandle, m_hLocker);

	//DECLARE_DATAMAP_OFFSET(bool, m_bFireOnEmpty);
	//DECLARE_DATAMAP_OFFSET(bool, m_iPrimaryAmmoType);
	//DECLARE_DATAMAP_OFFSET(bool, m_iSecondaryAmmoType);

private:
	float backup_m_fMinRange1;
	float backup_m_fMinRange2;
	float backup_m_fMaxRange1;
	float backup_m_fMaxRange2;
};

inline CCombatCharacter *CCombatWeapon::GetOwner()
{
	CCombatCharacter *owner = m_hOwner->Get();
	return owner;
}

#endif
