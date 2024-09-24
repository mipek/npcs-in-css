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

#ifndef _INCLUDE_VEHICLES_H_
#define _INCLUDE_VEHICLES_H_

/* Modified from IVehicle/iservervehicle to be compilable without lots of sdk includes */
#include "shareddefs.h"
#include "usercmd.h"

class CMoveData;


#define VEHICLE_TYPE_CAR_WHEELS			(1<<0)
#define VEHICLE_TYPE_CAR_RAYCAST		(1<<1)
#define VEHICLE_TYPE_JETSKI_RAYCAST		(1<<2)
#define VEHICLE_TYPE_AIRBOAT_RAYCAST	(1<<3)

#define VEHICLE_MAX_AXLE_COUNT	4
#define VEHICLE_MAX_GEAR_COUNT	6
#define VEHICLE_MAX_WHEEL_COUNT	(2*VEHICLE_MAX_AXLE_COUNT)

#define VEHICLE_TIRE_NORMAL		0
#define VEHICLE_TIRE_BRAKING	1
#define VEHICLE_TIRE_POWERSLIDE	2

struct vehicle_controlparams_t
{
	float	throttle;
	float	steering;
	float	brake;
	float	boost;
	bool	handbrake;
	bool	handbrakeLeft;
	bool	handbrakeRight;
	bool	brakepedal;
	bool	bHasBrakePedal;
	bool	bAnalogSteering;
};

struct vehicle_operatingparams_t
{
	DECLARE_SIMPLE_DATADESC();

	float	speed;
	float	engineRPM;
	int		gear;
	float	boostDelay;
	int		boostTimeLeft;
	float	skidSpeed;
	int		skidMaterial;
	float	steeringAngle;
	int		wheelsNotInContact;
	int		wheelsInContact;
	bool	isTorqueBoosting;
};

// Iterator for queries
class CPassengerSeatTransition;
typedef CUtlVector< CPassengerSeatTransition> PassengerSeatAnims_t;


// Seat query types
enum VehicleSeatQuery_e
{
	VEHICLE_SEAT_ANY,			// Any available seat for our role
	VEHICLE_SEAT_NEAREST,		// Seat closest to our starting point
};

// Seat anim types for return
enum PassengerSeatAnimType_t
{
	PASSENGER_SEAT_ENTRY,
	PASSENGER_SEAT_EXIT
};

#define VEHICLE_SEAT_INVALID	-1		// An invalid seat

// Debug!
#define VEHICLE_DEBUGRENDERDATA_MAX_WHEELS		10
#define VEHICLE_DEBUGRENDERDATA_MAX_AXLES		3

struct vehicle_debugcarsystem_t
{
	Vector vecAxlePos[VEHICLE_DEBUGRENDERDATA_MAX_AXLES];

	Vector vecWheelPos[VEHICLE_DEBUGRENDERDATA_MAX_WHEELS];
	Vector vecWheelRaycasts[VEHICLE_DEBUGRENDERDATA_MAX_WHEELS][2];
	Vector vecWheelRaycastImpacts[VEHICLE_DEBUGRENDERDATA_MAX_WHEELS];
};


class IPhysicsVehicleController
{
public:
	virtual ~IPhysicsVehicleController() {}
	// call this from the game code with the control parameters
	virtual void Update( float dt, vehicle_controlparams_t &controls ) = 0;
	virtual const vehicle_operatingparams_t &GetOperatingParams() = 0;
	virtual const vehicleparams_t &GetVehicleParams() = 0;
	virtual vehicleparams_t &GetVehicleParamsForChange() = 0;
	virtual float UpdateBooster(float dt) = 0;
	virtual int GetWheelCount(void) = 0;
	virtual IPhysicsObject *GetWheel(int index) = 0;
	virtual bool GetWheelContactPoint( int index, Vector *pContactPoint, int *pSurfaceProps ) = 0;
	virtual void SetSpringLength(int wheelIndex, float length) = 0;
	virtual void SetWheelFriction(int wheelIndex, float friction) = 0;

	virtual void OnVehicleEnter( void ) = 0;
	virtual void OnVehicleExit( void ) = 0;

	virtual void SetEngineDisabled( bool bDisable ) = 0;
	virtual bool IsEngineDisabled( void ) = 0;

	// Debug
	virtual void GetCarSystemDebugData( vehicle_debugcarsystem_t &debugCarSystem ) = 0;
	virtual void VehicleDataReload() = 0;
};



abstract_class IVehicle
{
public:
	// Get and set the current driver. Use PassengerRole_t enum in shareddefs.h for adding passengers
	virtual CBaseEntity*	GetPassenger( int nRole = VEHICLE_ROLE_DRIVER ) = 0;											/* returns CBaseCombatCharacter* */
	virtual int						GetPassengerRole( CBaseEntity *pPassenger ) = 0;										/* CBaseCombatCharacter* param */

	// Where is the passenger seeing from?
	virtual void			GetVehicleViewPosition( int nRole, Vector *pOrigin, QAngle *pAngles, float *pFOV = NULL ) = 0;

	// Does the player use his normal weapons while in this mode?
	virtual bool			IsPassengerUsingStandardWeapons( int nRole = VEHICLE_ROLE_DRIVER ) = 0;

	// Process movement
	virtual void			SetupMove( CBaseEntity *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move ) = 0;	/* First param is CBasePlayer */
	virtual void			ProcessMovement( CBaseEntity *pPlayer, CMoveData *pMoveData ) = 0;								/* First param is CBasePlayer */
	virtual void			FinishMove( CBaseEntity *player, CUserCmd *ucmd, CMoveData *move ) = 0;							/* First param is CBasePlayer */

	// Process input
	virtual void			ItemPostFrame( CBaseEntity *pPlayer ) = 0;														/* First param is CBasePlayer */
};

abstract_class IServerVehicle : public IVehicle
{
public:
	// Get the entity associated with the vehicle.
	virtual CBaseEntity*	GetVehicleEnt() = 0;

	// Get and set the current driver. Use PassengerRole_t enum in shareddefs.h for adding passengers
	virtual void			SetPassenger( int nRole, CBaseEntity *pPassenger ) = 0;											/* Second param is CBaseCombatCharacter */

	// Is the player visible while in the vehicle? (this is a constant the vehicle)
	virtual bool			IsPassengerVisible( int nRole = VEHICLE_ROLE_DRIVER ) = 0;

	// Can a given passenger take damage?
	virtual bool			IsPassengerDamagable( int nRole  = VEHICLE_ROLE_DRIVER ) = 0;
	virtual bool			PassengerShouldReceiveDamage( CTakeDamageInfo &info ) = 0;

	// Is the vehicle upright?
	virtual bool			IsVehicleUpright( void ) = 0;

	// Whether or not we're in a transitional phase
	virtual bool			IsPassengerEntering( void ) = 0;
	virtual bool			IsPassengerExiting( void ) = 0;

	// Get a position in *world space* inside the vehicle for the player to start at
	virtual void			GetPassengerSeatPoint( int nRole, Vector *pPoint, QAngle *pAngles ) = 0;

	virtual void			HandlePassengerEntry( CBaseEntity *pPassenger, bool bAllowEntryOutsideZone = false ) = 0;	/* First param is CBaseCombatCharacter */
	virtual bool			HandlePassengerExit( CBaseEntity *pPassenger ) = 0;									/* First param is CBaseCombatCharacter */

	// Get a point in *world space* to leave the vehicle from (may be in solid)
	virtual bool			GetPassengerExitPoint( int nRole, Vector *pPoint, QAngle *pAngles ) = 0;
	virtual int				GetEntryAnimForPoint( const Vector &vecPoint ) = 0;
	virtual int				GetExitAnimToUse( Vector &vecEyeExitEndpoint, bool &bAllPointsBlocked ) = 0;
	virtual void			HandleEntryExitFinish( bool bExitAnimOn, bool bResetAnim ) = 0;

	virtual int				ClassifyPassenger( CBaseEntity *pPassenger, int defaultClassification ) = 0;	/* First param is CBaseCombatCharacter, second param/ret is 'Class_T' enum */
	virtual float			PassengerDamageModifier( const CTakeDamageInfo &info ) = 0;

	// Get me the parameters for this vehicle
	virtual const vehicleparams_t	*GetVehicleParams( void ) = 0;
	// If I'm a physics vehicle, get the controller
	virtual IPhysicsVehicleController *GetVehicleController() = 0;


	virtual int				NPC_GetAvailableSeat( CBaseEntity *pPassenger, string_t strRoleName, VehicleSeatQuery_e nQueryType ) = 0;
	virtual bool			NPC_AddPassenger( CBaseEntity *pPassenger, string_t strRoleName, int nSeat ) = 0;
	virtual bool			NPC_RemovePassenger( CBaseEntity *pPassenger ) = 0;
	virtual bool			NPC_GetPassengerSeatPosition( CBaseEntity *pPassenger, Vector *vecResultPos, QAngle *vecResultAngle ) = 0;
	virtual bool			NPC_GetPassengerSeatPositionLocal( CBaseEntity *pPassenger, Vector *vecResultPos, QAngle *vecResultAngle ) = 0;
	virtual int				NPC_GetPassengerSeatAttachment( CBaseEntity *pPassenger ) = 0;
	virtual bool			NPC_HasAvailableSeat( string_t strRoleName ) = 0;

	virtual const PassengerSeatAnims_t	*NPC_GetPassengerSeatAnims( CBaseEntity *pPassenger, PassengerSeatAnimType_t nType ) = 0;
	virtual CBaseEntity					*NPC_GetPassengerInSeat( int nRoleID, int nSeatID ) = 0;

	virtual void			RestorePassengerInfo( void ) = 0;

	// NPC Driving
	virtual bool			NPC_CanDrive( void ) = 0;
	virtual void			NPC_SetDriver( CBaseEntity *pDriver ) = 0;
	virtual void			NPC_DriveVehicle( void ) = 0;
	virtual void			NPC_ThrottleCenter( void ) = 0;
	virtual void			NPC_ThrottleReverse( void ) = 0;
	virtual void			NPC_ThrottleForward( void ) = 0;
	virtual void			NPC_Brake( void ) = 0;
	virtual void			NPC_TurnLeft( float flDegrees ) = 0;
	virtual void			NPC_TurnRight( float flDegrees ) = 0;
	virtual void			NPC_TurnCenter( void ) = 0;
	virtual void			NPC_PrimaryFire( void ) = 0;
	virtual void			NPC_SecondaryFire( void ) = 0;
	virtual bool			NPC_HasPrimaryWeapon( void ) = 0;
	virtual bool			NPC_HasSecondaryWeapon( void ) = 0;
	virtual void			NPC_AimPrimaryWeapon( Vector vecTarget ) = 0;
	virtual void			NPC_AimSecondaryWeapon( Vector vecTarget ) = 0;

	// Weapon handling
	virtual void			Weapon_PrimaryRanges( float *flMinRange, float *flMaxRange ) = 0;
	virtual void			Weapon_SecondaryRanges( float *flMinRange, float *flMaxRange ) = 0;
	virtual float			Weapon_PrimaryCanFireAt( void ) = 0;	// Return the time at which this vehicle's primary weapon can fire again
	virtual float			Weapon_SecondaryCanFireAt( void ) = 0;	// Return the time at which this vehicle's secondary weapon can fire again

	// debugging, script file flushed
	virtual void			ReloadScript() = 0;

};

// This is an interface to derive from if your class contains an IServerVehicle
// handler (i.e. something derived CBaseServerVehicle.

abstract_class IDrivableVehicle
{
public:
	virtual CBaseEntity		*GetDriver( void ) = 0;

	// Process movement
	virtual void			ItemPostFrame( CBaseEntity *pPlayer ) = 0;
	virtual void			SetupMove( CBaseEntity *player, CUserCmd *ucmd, IMoveHelper *pHelper, CMoveData *move ) = 0;
	virtual void			ProcessMovement( CBaseEntity *pPlayer, CMoveData *pMoveData ) = 0;
	virtual void			FinishMove( CBaseEntity *player, CUserCmd *ucmd, CMoveData *move ) = 0;

	// Entering / Exiting
	virtual bool			CanEnterVehicle( CBaseEntity *pEntity ) = 0;
	virtual bool			CanExitVehicle( CBaseEntity *pEntity ) = 0;
	virtual void			SetVehicleEntryAnim( bool bOn ) = 0;
	virtual void			SetVehicleExitAnim( bool bOn, Vector vecEyeExitEndpoint ) = 0;
	virtual void			EnterVehicle( CBaseEntity *pPassenger ) = 0;

	virtual void			PreExitVehicle( CBaseEntity *pPassenger, int nRole ) = 0;
	virtual void			ExitVehicle( int nRole ) = 0;
	virtual bool			AllowBlockedExit( CBaseEntity *pPassenger, int nRole ) = 0;
	virtual bool			AllowMidairExit( CBaseEntity *pPassenger, int nRole ) = 0;
	virtual string_t		GetVehicleScriptName() = 0;

	virtual bool			PassengerShouldReceiveDamage( CTakeDamageInfo &info ) = 0;
};




// parameters for the body object control of the vehicle
struct vehicle_bodyparams_t
{
	DECLARE_SIMPLE_DATADESC();

	Vector		massCenterOverride;		// leave at vec3_origin for no override
	float		massOverride;			// leave at 0 for no override
	float		addGravity;				// keeps car down
	float		tiltForce;				// keeps car down when not on flat ground
	float		tiltForceHeight;		// where the tilt force pulls relative to center of mass
	float		counterTorqueFactor;
	float		keepUprightTorque;
	float		maxAngularVelocity;		// clamp the car angular velocity separately from other objects to keep stable
};

// wheel objects are created by vphysics, these are the parameters for those objects
// NOTE: They are paired, so only one set of parameters is necessary per axle
struct vehicle_wheelparams_t
{
	DECLARE_SIMPLE_DATADESC();

	float		radius;
	float		mass;
	float		inertia;
	float		damping;		// usually 0
	float		rotdamping;		// usually 0
	float		frictionScale;	// 1.5 front, 1.8 rear
	int			materialIndex;
	int			brakeMaterialIndex;
	int			skidMaterialIndex;
	float		springAdditionalLength;	// 0 means the spring is at it's rest length
};

struct vehicle_suspensionparams_t
{
	DECLARE_SIMPLE_DATADESC();

	float		springConstant;
	float		springDamping;
	float		stabilizerConstant;
	float		springDampingCompression;
	float		maxBodyForce;
};

// NOTE: both raytrace and wheel data here because jetski uses both.
struct vehicle_axleparams_t
{
	DECLARE_SIMPLE_DATADESC();

	Vector						offset;					// center of this axle in vehicle object space
	Vector						wheelOffset;			// offset to wheel (assume other wheel is symmetric at -wheelOffset) from axle center
	Vector						raytraceCenterOffset;	// offset to center of axle for the raytrace data.
	Vector						raytraceOffset;			// offset to raytrace for non-wheel (some wheeled) vehicles
	vehicle_wheelparams_t		wheels;
	vehicle_suspensionparams_t	suspension;
	float						torqueFactor;		// normalized to 1 across all axles
	// e.g. 0,1 for rear wheel drive - 0.5,0.5 for 4 wheel drive
	float						brakeFactor;		// normalized to 1 across all axles
};

struct vehicle_steeringparams_t
{
	DECLARE_SIMPLE_DATADESC();

	float		degreesSlow;			// angle in degrees of steering at slow speed
	float		degreesFast;			// angle in degrees of steering at fast speed
	float		degreesBoost;			// angle in degrees of steering at fast speed
	float		steeringRateSlow;		// this is the speed the wheels are steered when the vehicle is slow
	float		steeringRateFast;		// this is the speed the wheels are steered when the vehicle is "fast"
	float		steeringRestRateSlow;	// this is the speed at which the wheels move toward their resting state (straight ahead) at slow speed
	float		steeringRestRateFast;	// this is the speed at which the wheels move toward their resting state (straight ahead) at fast speed
	float		speedSlow;				// this is the max speed of "slow"
	float		speedFast;				// this is the min speed of "fast"
	float		turnThrottleReduceSlow;		// this is the amount of throttle reduction to apply at the maximum steering angle
	float		turnThrottleReduceFast;		// this is the amount of throttle reduction to apply at the maximum steering angle
	float		brakeSteeringRateFactor;	// this scales the steering rate when the brake/handbrake is down
	float		throttleSteeringRestRateFactor;	// this scales the steering rest rate when the throttle is down
	float		powerSlideAccel;		// scale of speed to acceleration
	float		boostSteeringRestRateFactor;	// this scales the steering rest rate when boosting
	float		boostSteeringRateFactor;	// this scales the steering rest rate when boosting
	float		steeringExponent;		// this makes the steering response non-linear.  The steering function is linear, then raised to this power

	bool		isSkidAllowed;			// true/false skid flag
	bool		dustCloud;				// flag for creating a dustcloud behind vehicle
};

struct vehicle_engineparams_t
{
	DECLARE_SIMPLE_DATADESC();

	float		horsepower;
	float		maxSpeed;
	float		maxRevSpeed;
	float		maxRPM;					// redline RPM limit
	float		axleRatio;				// ratio of engine rev to axle rev
	float		throttleTime;			// time to reach full throttle in seconds

	// transmission
	int			gearCount;				// gear count - max 10
	float		gearRatio[VEHICLE_MAX_GEAR_COUNT];	// ratio for each gear

	// automatic transmission (simple auto-shifter - switches at fixed RPM limits)
	float		shiftUpRPM;				// max RPMs to switch to a higher gear
	float		shiftDownRPM;			// min RPMs to switch to a lower gear
	float		boostForce;
	float		boostDuration;
	float		boostDelay;
	float		boostMaxSpeed;
	float		autobrakeSpeedGain;
	float		autobrakeSpeedFactor;
	bool		torqueBoost;
	bool		isAutoTransmission;		// true for auto, false for manual
};

struct vehicleparams_t
{
	DECLARE_SIMPLE_DATADESC();

	int							axleCount;
	int							wheelsPerAxle;
	vehicle_bodyparams_t		body;
	vehicle_axleparams_t		axles[VEHICLE_MAX_AXLE_COUNT];
	vehicle_engineparams_t		engine;
	vehicle_steeringparams_t	steering;
};


#endif // _INCLUDE_VEHICLES_H_
