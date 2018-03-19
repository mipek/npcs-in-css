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

#ifndef _INCLUDE_CTAKEDAMAGEINFO_H_
#define _INCLUDE_CTAKEDAMAGEINFO_H_

#include "CEntity.h"

/**
 * TODO: Rewrite all CBE* input/outputs to be CEntity* and handle the conversion internally
 * Also find some realistic damage events to attach to and make sure the members line up. Up to maxdamage seems correct
 */

#define BASEDAMAGE_NOT_SPECIFIED	FLT_MAX

#if 0
class CEntityTakeDamageInfo
{
public:
#if 1
	CEntityTakeDamageInfo();
	CEntityTakeDamageInfo(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType, int iKillType = 0);
	CEntityTakeDamageInfo(CBaseEntity *pInflictor, CBaseEntity *pAttacker, CBaseEntity *pWeapon, float flDamage, int bitsDamageType, int iKillType = 0);
	CEntityTakeDamageInfo(CBaseEntity *pInflictor, CBaseEntity *pAttacker, const Vector &damageForce, const Vector &damagePosition, float flDamage, int bitsDamageType, int iKillType = 0, Vector *reportedPosition = NULL);
	CEntityTakeDamageInfo(CBaseEntity *pInflictor, CBaseEntity *pAttacker, CBaseEntity *pWeapon, const Vector &damageForce, const Vector &damagePosition, float flDamage, int bitsDamageType, int iKillType = 0, Vector *reportedPosition = NULL);


	// Inflictor is the weapon or rocket (or player) that is dealing the damage.
	CBaseEntity*	GetInflictor() const;
	void			SetInflictor(CBaseEntity *pInflictor);

	// Weapon is the weapon that did the attack.
	// For hitscan weapons, it'll be the same as the inflictor. For projectile weapons, the projectile 
	// is the inflictor, and this contains the weapon that created the projectile.
	CBaseEntity*	GetWeapon() const;
	void			SetWeapon(CBaseEntity *pWeapon);
#endif
	// Attacker is the character who originated the attack (like a player or an AI).
	CEntity*	GetAttacker() const;
#if 0
	void			SetAttacker(CBaseEntity *pAttacker);
#endif
	float			GetDamage() const;
	void			SetDamage(float flDamage);

	float			GetMaxDamage() const;
	void			SetMaxDamage(float flMaxDamage);

	void			ScaleDamage(float flScaleAmount);
	void			AddDamage(float flAddAmount);
	void			SubtractDamage(float flSubtractAmount);
	float			GetBaseDamage() const;
	bool			BaseDamageIsValid() const;
	Vector			GetDamageForce() const;
	void			SetDamageForce(const Vector &damageForce);
	void			ScaleDamageForce(float flScaleAmount);

	Vector			GetDamagePosition() const;
	void			SetDamagePosition(const Vector &damagePosition);

	Vector			GetReportedPosition() const;
	void			SetReportedPosition(const Vector &reportedPosition);
	int				GetDamageType() const;
	void			SetDamageType(int bitsDamageType);
	void			AddDamageType(int bitsDamageType);
	int				GetDamageCustom(void) const;
	void			SetDamageCustom(int iDamageCustom);
	int				GetDamageStats(void) const;
	void			SetDamageStats(int iDamageStats);

	int				GetAmmoType() const;
	void			SetAmmoType(int iAmmoType);
#if 1
	const char *	GetAmmoName() const;

	void			Set(CBaseEntity *pInflictor, CBaseEntity *pAttacker, float flDamage, int bitsDamageType, int iKillType = 0);
	void			Set(CBaseEntity *pInflictor, CBaseEntity *pAttacker, CBaseEntity *pWeapon, float flDamage, int bitsDamageType, int iKillType = 0);
	void			Set(CBaseEntity *pInflictor, CBaseEntity *pAttacker, const Vector &damageForce, const Vector &damagePosition, float flDamage, int bitsDamageType, int iKillType = 0, Vector *reportedPosition = NULL);
	void			Set(CBaseEntity *pInflictor, CBaseEntity *pAttacker, CBaseEntity *pWeapon, const Vector &damageForce, const Vector &damagePosition, float flDamage, int bitsDamageType, int iKillType = 0, Vector *reportedPosition = NULL);

	void			AdjustPlayerDamageInflictedForSkillLevel();
	void			AdjustPlayerDamageTakenForSkillLevel();

	// Given a damage type (composed of the #defines above), fill out a string with the appropriate text.
	// For designer debug output.
	static void		DebugGetDamageTypeString(unsigned int DamageType, char *outbuf, int outbuflength);


	//private:
	void			CopyDamageToBaseDamage();

protected:
	void			Init(CBaseEntity *pInflictor, CBaseEntity *pAttacker, CBaseEntity *pWeapon, const Vector &damageForce, const Vector &damagePosition, const Vector &reportedPosition, float flDamage, int bitsDamageType, int iKillType);
#endif
	Vector			m_vecDamageForce;
	Vector			m_vecDamagePosition;
	Vector			m_vecReportedPosition;	// Position players are told damage is coming from
	EHANDLE			m_hInflictor;
	EHANDLE			m_hAttacker;
	EHANDLE			m_hWeapon;
	float			m_flDamage;
	float			m_flMaxDamage;
	float			m_flBaseDamage;			// The damage amount before skill level adjustments are made. Used to get uniform damage forces.
	int				m_bitsDamageType;
	int				m_iDamageCustom;
	int				m_iDamageStats;
	int				m_iAmmoType;			// AmmoType of the weapon used to cause this damage, if any
};

#endif

#if 0
inline CBaseEntity* CEntityTakeDamageInfo::GetInflictor() const
{
	return m_hInflictor;
}


inline void CEntityTakeDamageInfo::SetInflictor(CBaseEntity *pInflictor)
{
	m_hInflictor = pInflictor;
}

#endif
/*inline CEntity* CEntityTakeDamageInfo::GetAttacker() const
{
	return CEntity::Instance(m_hAttacker);
}*/

#if 0
inline void CEntityTakeDamageInfo::SetAttacker(CBaseEntity *pAttacker)
{
	m_hAttacker = pAttacker;
}

inline CBaseEntity* CEntityTakeDamageInfo::GetWeapon() const
{
	return m_hWeapon;
}


inline void CEntityTakeDamageInfo::SetWeapon(CBaseEntity *pWeapon)
{
	m_hWeapon = pWeapon;
}

#endif

#if 0
inline float CEntityTakeDamageInfo::GetDamage() const
{
	return m_flDamage;
}

inline void CEntityTakeDamageInfo::SetDamage(float flDamage)
{
	m_flDamage = flDamage;
}

inline float CEntityTakeDamageInfo::GetMaxDamage() const
{
	return m_flMaxDamage;
}

inline void CEntityTakeDamageInfo::SetMaxDamage(float flMaxDamage)
{
	m_flMaxDamage = flMaxDamage;
}

inline void CEntityTakeDamageInfo::ScaleDamage(float flScaleAmount)
{
	m_flDamage *= flScaleAmount;
}

inline void CEntityTakeDamageInfo::AddDamage(float flAddAmount)
{
	m_flDamage += flAddAmount;
}

inline void CEntityTakeDamageInfo::SubtractDamage(float flSubtractAmount)
{
	m_flDamage -= flSubtractAmount;
}

inline float CEntityTakeDamageInfo::GetBaseDamage() const
{
	if( BaseDamageIsValid() )
		return m_flBaseDamage;

	// No one ever specified a base damage, so just return damage.
	return m_flDamage;
}

inline bool CEntityTakeDamageInfo::BaseDamageIsValid() const
{
	return (m_flBaseDamage != BASEDAMAGE_NOT_SPECIFIED);
}

inline Vector CEntityTakeDamageInfo::GetDamageForce() const
{
	return m_vecDamageForce;
}

inline void CEntityTakeDamageInfo::SetDamageForce(const Vector &damageForce)
{
	m_vecDamageForce = damageForce;
}

inline void	CEntityTakeDamageInfo::ScaleDamageForce(float flScaleAmount)
{
	m_vecDamageForce *= flScaleAmount;
}

inline Vector CEntityTakeDamageInfo::GetDamagePosition() const
{
	return m_vecDamagePosition;
}


inline void CEntityTakeDamageInfo::SetDamagePosition(const Vector &damagePosition)
{
	m_vecDamagePosition = damagePosition;
}

inline Vector CEntityTakeDamageInfo::GetReportedPosition() const
{
	return m_vecReportedPosition;
}


inline void CEntityTakeDamageInfo::SetReportedPosition(const Vector &reportedPosition)
{
	m_vecReportedPosition = reportedPosition;
}

inline int CEntityTakeDamageInfo::GetDamageType() const
{
	return m_bitsDamageType;
}


inline void CEntityTakeDamageInfo::SetDamageType(int bitsDamageType)
{
	m_bitsDamageType = bitsDamageType;
}

inline void	CEntityTakeDamageInfo::AddDamageType(int bitsDamageType)
{
	m_bitsDamageType |= bitsDamageType;
}

inline int CEntityTakeDamageInfo::GetDamageCustom() const
{
	return m_iDamageCustom;
}

inline void CEntityTakeDamageInfo::SetDamageCustom(int iDamageCustom)
{
	m_iDamageCustom = iDamageCustom;
}

inline int CEntityTakeDamageInfo::GetDamageStats() const
{
	return m_iDamageCustom;
}

inline void CEntityTakeDamageInfo::SetDamageStats(int iDamageCustom)
{
	m_iDamageCustom = iDamageCustom;
}

inline int CEntityTakeDamageInfo::GetAmmoType() const
{
	return m_iAmmoType;
}

inline void CEntityTakeDamageInfo::SetAmmoType(int iAmmoType)
{
	m_iAmmoType = iAmmoType;
}

#endif

#if 0
inline void CEntityTakeDamageInfo::CopyDamageToBaseDamage()
{ 
	m_flBaseDamage = m_flDamage;
}
#endif

#endif // _INCLUDE_CTAKEDAMAGEINFO_H_
