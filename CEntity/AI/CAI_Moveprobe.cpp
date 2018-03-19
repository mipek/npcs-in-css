
#include "CAI_NPC.h"
#include "CAI_moveprobe.h"
#include "ai_routedist.h"
#include "CAI_utils.h"
#include "CPhysicsProp.h"



// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



#undef LOCAL_STEP_SIZE
// FIXME: this should be based in their hull width
#define	LOCAL_STEP_SIZE	16.0 // 8 // 16

float MOVE_HEIGHT_EPSILON = 0.0625f;


extern ConVar *ai_moveprobe_usetracelist;


//-----------------------------------------------------------------------------
// Categorizes the blocker and sets the appropriate bits
//-----------------------------------------------------------------------------
AIMoveResult_t AIComputeBlockerMoveResult( CBaseEntity *pBlocker )
{
	CEntity *cent = CEntity::Instance(pBlocker);
	if (cent->MyNPCPointer())
		return AIMR_BLOCKED_NPC;
	else if (cent->entindex() == 0)
		return AIMR_BLOCKED_WORLD;
	return AIMR_BLOCKED_ENTITY;
}



bool CAI_MoveProbe::MoveLimit( Navigation_t navType, const Vector &vecStart, 
	const Vector &vecEnd, unsigned int collisionMask, const CEntity *pTarget, 
	float pctToCheckStandPositions, unsigned flags, AIMoveTrace_t* pTrace)
{
	return g_helpfunc.MoveLimit(this, navType, vecStart, vecEnd, collisionMask, (pTarget != NULL) ? pTarget->BaseEntity() : NULL, pctToCheckStandPositions, flags, pTrace);
}


bool CAI_MoveProbe::ShouldBrushBeIgnored( CBaseEntity *pEntity )
{
	return g_helpfunc.ShouldBrushBeIgnored(this, pEntity);
}



//-----------------------------------------------------------------------------
// Checks a ground-based movement
// NOTE: The movement will be based on an *actual* start position and
// a *desired* end position; it works this way because the ground-based movement
// is 2 1/2D, and we may end up on a ledge above or below the actual desired endpoint.
//-----------------------------------------------------------------------------
bool CAI_MoveProbe::TestGroundMove( const Vector &vecActualStart, const Vector &vecDesiredEnd, 
	unsigned int collisionMask, float pctToCheckStandPositions, unsigned flags, AIMoveTrace_t *pMoveTrace ) const
{
	AIMoveTrace_t ignored;
	if ( !pMoveTrace )
		pMoveTrace = &ignored;

	// Set a reasonable default set of values
	pMoveTrace->flDistObstructed = 0.0f;
	pMoveTrace->pObstruction 	 = NULL;
	pMoveTrace->vHitNormal		 = vec3_origin;
	pMoveTrace->fStatus 		 = AIMR_OK;
	pMoveTrace->vEndPosition 	 = vecActualStart;
	pMoveTrace->flStepUpDistance = 0;

	Vector vecMoveDir;
	pMoveTrace->flTotalDist = ComputePathDirection( NAV_GROUND, vecActualStart, vecDesiredEnd, &vecMoveDir );
	if (pMoveTrace->flTotalDist == 0.0f)
	{
		return true;
	}
	
	// If it starts hanging over an edge, tough it out until it's not
	// This allows us to blow an NPC in an invalid region + allow him to walk out
	StepGroundTest_t groundTest;
	if ( (flags & AITGM_IGNORE_FLOOR) || pctToCheckStandPositions < 0.001 )
	{
		groundTest = STEP_DONT_CHECK_GROUND;
		pctToCheckStandPositions = 0; // AITGM_IGNORE_FLOOR always overrides pct
	}
	else
	{
		if ( pctToCheckStandPositions > 99.999 )
			pctToCheckStandPositions = 100;

		if ((flags & AITGM_IGNORE_INITIAL_STAND_POS) || CheckStandPosition(vecActualStart, collisionMask))
			groundTest = STEP_ON_VALID_GROUND;
		else
			groundTest = STEP_ON_INVALID_GROUND;
	}


	//  Take single steps towards the goal
	float distClear = 0;
	int i;

	CheckStepArgs_t checkStepArgs;
	CheckStepResult_t checkStepResult;

	checkStepArgs.vecStart				= vecActualStart;
	checkStepArgs.vecStepDir			= vecMoveDir;
	checkStepArgs.stepSize				= 0;
	checkStepArgs.stepHeight			= StepHeight();
	checkStepArgs.stepDownMultiplier	= GetOuter()->GetStepDownMultiplier();
	checkStepArgs.minStepLanding		= GetHullWidth() * 0.3333333;
	checkStepArgs.collisionMask			= collisionMask;
	checkStepArgs.groundTest			= groundTest;

	checkStepResult.endPoint = vecActualStart;
	checkStepResult.hitNormal = vec3_origin;
	checkStepResult.pBlocker = NULL;
	
	float distStartToIgnoreGround = (pctToCheckStandPositions == 100) ? pMoveTrace->flTotalDist : pMoveTrace->flTotalDist * ( pctToCheckStandPositions * 0.01);
	bool bTryNavIgnore = ( ( vecActualStart - GetLocalOrigin() ).Length2DSqr() < 0.1 && fabsf(vecActualStart.z - GetLocalOrigin().z) < checkStepArgs.stepHeight * 0.5 );

	CUtlVector<CBaseEntity *> ignoredEntities;

	for (;;)
	{
		float flStepSize = min( LOCAL_STEP_SIZE, pMoveTrace->flTotalDist - distClear );
		if ( flStepSize < 0.001 )
			break;

		checkStepArgs.stepSize = flStepSize;
		if ( distClear - distStartToIgnoreGround > 0.001 )
			checkStepArgs.groundTest = STEP_DONT_CHECK_GROUND;

		Assert( !m_pTraceListData || m_pTraceListData->IsEmpty() );
		SetupCheckStepTraceListData( checkStepArgs );
		
		for ( i = 0; i < 16; i++ )
		{
			CheckStep( checkStepArgs, &checkStepResult );

			if ( !bTryNavIgnore || !checkStepResult.pBlocker || !checkStepResult.fStartSolid )
				break;

			CEntity *cent = CEntity::Instance(checkStepResult.pBlocker);
			if ( cent->GetMoveType() != MOVETYPE_VPHYSICS && !cent->IsNPC() )
				break;

			// Only permit pass through of objects initially embedded in
			if ( vecActualStart != checkStepArgs.vecStart )
			{
				bTryNavIgnore = false;
				break;
			}

			// Only allow move away from physics objects
			if ( cent->GetMoveType() == MOVETYPE_VPHYSICS )
			{
				Vector vMoveDir = vecDesiredEnd - vecActualStart;
				VectorNormalize( vMoveDir );

				Vector vObstacleDir = (cent->WorldSpaceCenter() - GetOuter()->WorldSpaceCenter() );
				VectorNormalize( vObstacleDir );

				if ( vMoveDir.Dot( vObstacleDir ) >= 0 )
					break;
			}


			ignoredEntities.AddToTail( checkStepResult.pBlocker );
			cent->SetNavIgnore();
		}

		ResetTraceListData();
		
		// If we're being blocked by something, move as close as we can and stop
		if ( checkStepResult.pBlocker )
		{
			distClear += ( checkStepResult.endPoint - checkStepArgs.vecStart ).Length2D();
			break;
		}
		
		float dz = checkStepResult.endPoint.z - checkStepArgs.vecStart.z;
		if ( dz < 0 )
		{
			dz = 0;
		}
		
		pMoveTrace->flStepUpDistance += dz;
		distClear += flStepSize;
		checkStepArgs.vecStart = checkStepResult.endPoint;
	}

	for ( i = 0; i < ignoredEntities.Count(); i++ )
	{
		CEntity::Instance(ignoredEntities[i])->ClearNavIgnore();
	}

	pMoveTrace->vEndPosition = checkStepResult.endPoint;
	
	if (checkStepResult.pBlocker)
	{
		pMoveTrace->pObstruction	 = checkStepResult.pBlocker;
		pMoveTrace->vHitNormal		 = checkStepResult.hitNormal;
		pMoveTrace->fStatus			 = AIComputeBlockerMoveResult( checkStepResult.pBlocker );
		pMoveTrace->flDistObstructed = pMoveTrace->flTotalDist - distClear;

		return false;
	}

	// FIXME: If you started on a ledge and ended on a ledge, 
	// should it return an error condition (that you hit the world)?
	// Certainly not for Step(), but maybe for GroundMoveLimit()?
	
	// Make sure we actually made it to the target position 
	// and not a ledge above or below the target.
	if (!(flags & AITGM_2D))
	{
		float threshold = max(  0.5f * GetHullHeight(), StepHeight() + 0.1 );
		if (fabs(pMoveTrace->vEndPosition.z - vecDesiredEnd.z) > threshold)
		{
			// Ok, we ended up on a ledge above or below the desired destination
			pMoveTrace->pObstruction = GetContainingEntity( INDEXENT(0) );
			pMoveTrace->vHitNormal	 = vec3_origin;
			pMoveTrace->fStatus = AIMR_BLOCKED_WORLD;
			pMoveTrace->flDistObstructed = ComputePathDistance( NAV_GROUND, pMoveTrace->vEndPosition, vecDesiredEnd );
			return false;
		}
	}

	return true;
}

float CAI_MoveProbe::StepHeight() const
{
	return GetOuter()->StepHeight();
}


//-----------------------------------------------------------------------------


void CAI_MoveProbe::SetupCheckStepTraceListData( const CheckStepArgs_t &args ) const
{
	if ( ai_moveprobe_usetracelist->GetBool() )
	{
		Ray_t ray;
		Vector hullMin = WorldAlignMins();
		Vector hullMax = WorldAlignMaxs();

		hullMax.z += MOVE_HEIGHT_EPSILON;
		hullMin.z -= MOVE_HEIGHT_EPSILON;

		hullMax.z += args.stepHeight;
		hullMin.z -= args.stepHeight;

		if ( args.groundTest != STEP_DONT_CHECK_GROUND )
			hullMin.z -= args.stepHeight;

		hullMax.x += args.minStepLanding;
		hullMin.x -= args.minStepLanding;

		hullMax.y += args.minStepLanding;
		hullMin.y -= args.minStepLanding;

		Vector vecEnd;
		Vector2DMA( args.vecStart.AsVector2D(), args.stepSize, args.vecStepDir.AsVector2D(), vecEnd.AsVector2D() );
		vecEnd.z = args.vecStart.z;

		ray.Init( args.vecStart, vecEnd, hullMin, hullMax );

		if ( !m_pTraceListData )
		{
			const_cast<CAI_MoveProbe *>(this)->m_pTraceListData = new CTraceListData;
		}
		enginetrace->SetupLeafAndEntityListRay( ray, *(const_cast<CAI_MoveProbe *>(this)->m_pTraceListData) );
	}
}


bool CAI_MoveProbe::CheckStep( const CheckStepArgs_t &args, CheckStepResult_t *pResult ) const
{
	Vector vecEnd;
	unsigned collisionMask = args.collisionMask;
	VectorMA( args.vecStart, args.stepSize, args.vecStepDir, vecEnd );
	
	pResult->endPoint = args.vecStart;
	pResult->fStartSolid = false;
	pResult->hitNormal = vec3_origin;
	pResult->pBlocker = NULL;

	// This is fundamentally a 2D operation; we just want the end
	// position in Z to be no more than a step height from the start position
	Vector stepStart( args.vecStart.x, args.vecStart.y, args.vecStart.z + MOVE_HEIGHT_EPSILON );
	Vector stepEnd( vecEnd.x, vecEnd.y, args.vecStart.z + MOVE_HEIGHT_EPSILON );


	trace_t trace;

	TraceHull( stepStart, stepEnd, collisionMask, &trace );

	if (trace.startsolid || (trace.fraction < 1))
	{
		// Either the entity is starting embedded in the world, or it hit something.
		// Raise the box by the step height and try again
		trace_t stepTrace;

		if ( !trace.startsolid )
		{
			// Advance to first obstruction point
			stepStart = trace.endpos;

			// Trace up to locate the maximum step up in the space
			Vector stepUp( stepStart );
			stepUp.z += args.stepHeight;
			TraceHull( stepStart, stepUp, collisionMask, &stepTrace );

			stepStart = stepTrace.endpos;
		}
		else
			stepStart.z += args.stepHeight;

		// Now move forward
 		stepEnd.z = stepStart.z;

		TraceHull( stepStart, stepEnd, collisionMask, &stepTrace );
		bool bRejectStep = false;

		// Ok, raising it didn't work; we're obstructed
		if (stepTrace.startsolid || stepTrace.fraction <= 0.01 )
		{
			// If started in solid, and never escaped from solid, bail
			if ( trace.startsolid )
			{
				pResult->fStartSolid = true;
				pResult->pBlocker = trace.m_pEnt;
				pResult->hitNormal = trace.plane.normal;
				return false;
			}

			bRejectStep = true;
		}
		else
		{
			// If didn't step forward enough to qualify as a step, try as if stepped forward to
			// confirm there's potentially enough space to "land"
			float landingDistSq = ( stepEnd.AsVector2D() - stepStart.AsVector2D() ).LengthSqr();
			float requiredLandingDistSq = args.minStepLanding*args.minStepLanding;
			if ( landingDistSq < requiredLandingDistSq )
			{
				trace_t landingTrace;
				Vector stepEndWithLanding;

				VectorMA( stepStart, args.minStepLanding, args.vecStepDir, stepEndWithLanding );
				TraceHull( stepStart, stepEndWithLanding, collisionMask, &landingTrace );
				if ( landingTrace.fraction < 1 )
				{
					bRejectStep = true;
					if ( landingTrace.m_pEnt )
						pResult->pBlocker = landingTrace.m_pEnt;
				}
			}
			else if ( ( stepTrace.endpos.AsVector2D() - stepStart.AsVector2D() ).LengthSqr() < requiredLandingDistSq )
			{
				bRejectStep = true;
			}
		}

		// If trace.fraction == 0, we fall through and check the position
		// we moved up to for suitability. This allows for sub-step
		// traces if the position ends up being suitable
		if ( !bRejectStep )
			trace = stepTrace;

		if ( trace.fraction < 1.0 )
		{
			if ( !pResult->pBlocker )
				pResult->pBlocker = trace.m_pEnt;
			pResult->hitNormal = trace.plane.normal;
		}

		stepEnd = trace.endpos;
	}


	// seems okay, now find the ground
	// The ground is only valid if it's within a step height of the original position
	Assert( VectorsAreEqual( trace.endpos, stepEnd, 1e-3 ) );
	stepStart = stepEnd; 
	stepEnd.z = args.vecStart.z - args.stepHeight * args.stepDownMultiplier - MOVE_HEIGHT_EPSILON;

	TraceHull( stepStart, stepEnd, collisionMask, &trace );

	// in empty space, lie and say we hit the world
	if (trace.fraction == 1.0f)
	{
		Assert( pResult->endPoint == args.vecStart );
		if ( const_cast<CAI_MoveProbe *>(this)->GetOuter()->GetGroundEntity() )
		{
			CEntity *ground = const_cast<CAI_MoveProbe *>(this)->GetOuter()->GetGroundEntity();
			pResult->pBlocker = (ground) ? ground->BaseEntity() : NULL;
		}
		else
		{
			pResult->pBlocker = GetContainingEntity( INDEXENT(0) );
		}
		return false;
	}

	// Checks to see if the thing we're on is a *type* of thing we
	// are capable of standing on. Always true ffor our current ground ent
	// otherwise we'll be stuck forever
	CBaseEntity *pFloor = trace.m_pEnt;
	CEntity *ground = GetOuter()->GetGroundEntity();
	CEntity *c_pFloor = CEntity::Instance(pFloor);
	if ( c_pFloor != ground && !CanStandOn( pFloor ) )
	{
		Assert( pResult->endPoint == args.vecStart );
		pResult->pBlocker = pFloor;
		return false;
	}

	// Don't step up onto an odd slope
	if ( trace.endpos.z - args.vecStart.z > args.stepHeight * 0.5 &&
		 ( ( c_pFloor->IsWorld() && trace.hitbox > 0 ) ||
		   dynamic_cast<CE_CPhysicsProp *>( c_pFloor ) ) )
	{
		if ( fabsf( trace.plane.normal.Dot( Vector(1, 0, 0) ) ) > .4 )
		{
			Assert( pResult->endPoint == args.vecStart );
			pResult->pBlocker = pFloor;

			return false;
		}
	}

	if (args.groundTest != STEP_DONT_CHECK_GROUND)
	{

		// Next, check to see if we can *geometrically* stand on the floor
		bool bIsFloorFlat = CheckStandPosition( trace.endpos, collisionMask );
		if (args.groundTest != STEP_ON_INVALID_GROUND && !bIsFloorFlat)
		{
			pResult->pBlocker = pFloor;

			return false;
		}
		// If we started on shaky ground (namely, it's not geometrically ok),
		// then we can continue going even if we remain on shaky ground.
		// This allows NPCs who have been blown into an invalid area to get out
		// of that invalid area and into a valid area. As soon as we're in
		// a valid area, though, we're not allowed to leave it.
	}

	// Return a point that is *on the ground*
	// We'll raise it by an epsilon in check step again
	pResult->endPoint = trace.endpos;
	pResult->endPoint.z += MOVE_HEIGHT_EPSILON; // always safe because always stepped down at least by epsilon

	return ( pResult->pBlocker == NULL ); // totally clear if pBlocker is NULL, partial blockage otherwise
}

bool CAI_MoveProbe::CanStandOn( CBaseEntity *pSurface ) const
{
	return GetOuter()->CanStandOn( pSurface );
}


//-----------------------------------------------------------------------------

void CAI_MoveProbe::TraceHull( 
	const Vector &vecStart, const Vector &vecEnd, const Vector &hullMin, 
	const Vector &hullMax, unsigned int mask, trace_t *pResult ) const
{
	CTraceFilterNav traceFilter( const_cast<CAI_NPC *>(GetOuter()), m_bIgnoreTransientEntities, GetOuter()->BaseEntity(), GetCollisionGroup() );

	Ray_t ray;
	ray.Init( vecStart, vecEnd, hullMin, hullMax );

	if ( !m_pTraceListData || m_pTraceListData->IsEmpty() )
		enginetrace->TraceRay( ray, mask, &traceFilter, pResult );
	else
	{
		enginetrace->TraceRayAgainstLeafAndEntityList( ray, *(const_cast<CAI_MoveProbe *>(this)->m_pTraceListData), mask, &traceFilter, pResult );
	}

	//NDebugOverlay::SweptBox( vecStart, vecEnd, hullMin, hullMax, vec3_angle, 255, 255, 0, 0, 10 );
	// Just to make sure; I'm not sure that this is always the case but it should be
	Assert( !pResult->allsolid || pResult->startsolid );
}

//-----------------------------------------------------------------------------

void CAI_MoveProbe::TraceHull( const Vector &vecStart, const Vector &vecEnd, 
	unsigned int mask, trace_t *pResult ) const
{
	TraceHull( vecStart, vecEnd, WorldAlignMins(), WorldAlignMaxs(), mask, pResult);
}


//-----------------------------------------------------------------------------

bool CAI_MoveProbe::CheckStandPosition( const Vector &vecStart, unsigned int collisionMask ) const
{
	// If we're not supposed to do ground checks, always say we can stand there
	if ( (GetOuter()->CapabilitiesGet() & bits_CAP_SKIP_NAV_GROUND_CHECK) )
		return true;

	Vector contactMin, contactMax;

	// this should assume the model is already standing
	Vector vecUp	= Vector( vecStart.x, vecStart.y, vecStart.z + 0.1 );
	Vector vecDown	= Vector( vecStart.x, vecStart.y, vecStart.z - StepHeight() * GetOuter()->GetStepDownMultiplier() );

	// check a half sized box centered around the foot
	Vector vHullMins = WorldAlignMins();
	Vector vHullMaxs = WorldAlignMaxs();

	if ( vHullMaxs == vec3_origin && vHullMins == vHullMaxs )
	{
		// "Test hulls" have no collision property
		vHullMins = GetHullMins();
		vHullMaxs = GetHullMaxs();
	}

	contactMin.x = vHullMins.x * 0.75 + vHullMaxs.x * 0.25;
	contactMax.x = vHullMins.x * 0.25 + vHullMaxs.x * 0.75;
	contactMin.y = vHullMins.y * 0.75 + vHullMaxs.y * 0.25;
	contactMax.y = vHullMins.y * 0.25 + vHullMaxs.y * 0.75;
	contactMin.z = vHullMins.z;
	contactMax.z = vHullMins.z;

	trace_t trace1, trace2;
	
	if ( !GetOuter()->IsFlaggedEfficient() )
	{
	
		Vector vHullBottomCenter;
		vHullBottomCenter.Init( 0, 0, vHullMins.z );

		// Try diagonal from lower left to upper right
		TraceHull( vecUp, vecDown, contactMin, vHullBottomCenter, collisionMask, &trace1 );
		if ( trace1.fraction != 1.0 && CanStandOn( trace1.m_pEnt ) )
		{
			TraceHull( vecUp, vecDown, vHullBottomCenter, contactMax, collisionMask, &trace2 );
			if ( trace2.fraction != 1.0 && ( trace1.m_pEnt == trace2.m_pEnt || CanStandOn( trace2.m_pEnt ) ) )
			{
				return true;
			}
		}

		// Okay, try the other one
		Vector testMin;
		Vector testMax;
		testMin.Init(contactMin.x, 0, vHullMins.z);
		testMax.Init(0, contactMax.y, vHullMins.z);

		TraceHull( vecUp, vecDown, testMin, testMax, collisionMask, &trace1 );
		if ( trace1.fraction != 1.0 && CanStandOn( trace1.m_pEnt ) )
		{
			testMin.Init(0, contactMin.y, vHullMins.z);
			testMax.Init(contactMax.x, 0, vHullMins.z);
			TraceHull( vecUp, vecDown, testMin, testMax, collisionMask, &trace2 );
			if ( trace2.fraction != 1.0 && ( trace1.m_pEnt == trace2.m_pEnt || CanStandOn( trace2.m_pEnt ) ) )
			{
				return true;
			}
		}
	}
	else
	{
		TraceHull( vecUp, vecDown, contactMin, contactMax, collisionMask, &trace1 );
		if ( trace1.fraction != 1.0 && CanStandOn( trace1.m_pEnt ) )
			return true;
	}

	return false;
}

//-----------------------------------------------------------------------------
// Purpose: Returns a jump lauch velocity for the current target entity
// Input  :
// Output :
//-----------------------------------------------------------------------------
Vector CAI_MoveProbe::CalcJumpLaunchVelocity(const Vector &startPos, const Vector &endPos, float flGravity, float *pminHeight, float maxHorzVelocity, Vector *pvecApex ) const
{
	// Get the height I have to jump to get to the target
	float	stepHeight = endPos.z - startPos.z;

	// get horizontal distance to target
	Vector targetDir2D	= endPos - startPos;
	targetDir2D.z = 0;
	float distance = VectorNormalize(targetDir2D);

	Assert( maxHorzVelocity > 0 );

	// get minimum times and heights to meet ideal horz velocity
	float minHorzTime = distance / maxHorzVelocity;
	float minHorzHeight = 0.5 * flGravity * (minHorzTime * 0.5) * (minHorzTime * 0.5);

	// jump height must be enough to hang in the air
	*pminHeight = max( *pminHeight, minHorzHeight );
	// jump height must be enough to cover the step up
	*pminHeight = max( *pminHeight, stepHeight );

	// time from start to apex
	float t0 = sqrt( ( 2.0 * *pminHeight) / flGravity );
	// time from apex to end
	float t1 = sqrt( ( 2.0 * fabs( *pminHeight - stepHeight) ) / flGravity );

	float velHorz = distance / (t0 + t1);

	Vector jumpVel = targetDir2D * velHorz;

	jumpVel.z = (float)sqrt(2.0f * flGravity * (*pminHeight));

	if (pvecApex)
	{
		*pvecApex = startPos + targetDir2D * velHorz * t0 + Vector( 0, 0, *pminHeight );
	}

	// -----------------------------------------------------------
	// Make the horizontal jump vector and add vertical component
	// -----------------------------------------------------------

	return jumpVel;
}

//-----------------------------------------------------------------------------
// Computes a point on the floor below the start point, somewhere
// between vecStart.z + flStartZ and vecStart.z + flEndZ
//-----------------------------------------------------------------------------
bool CAI_MoveProbe::FloorPoint( const Vector &vecStart, unsigned int collisionMask, 
						   float flStartZ, float flEndZ, Vector *pVecResult ) const
{
	// make a pizzabox shaped bounding hull
	Vector mins = WorldAlignMins();
	Vector maxs( WorldAlignMaxs().x, WorldAlignMaxs().y, mins.z );

	// trace down step height and a bit more
	Vector vecUp( vecStart.x, vecStart.y, vecStart.z + flStartZ + MOVE_HEIGHT_EPSILON );
	Vector vecDown( vecStart.x, vecStart.y, vecStart.z + flEndZ );
	
	trace_t trace;
	TraceHull( vecUp, vecDown, mins, maxs, collisionMask, &trace );

	bool fStartedInObject = false;

	if (trace.startsolid)
	{
		CEntity *cent = CEntity::Instance(trace.m_pEnt);
		if ( trace.m_pEnt && cent &&
			 ( cent->GetMoveType() == MOVETYPE_VPHYSICS || cent->IsNPC() ) &&
			 ( vecStart - GetLocalOrigin() ).Length() < 0.1 )
		{
			fStartedInObject = true;
		}

		vecUp.z = vecStart.z + MOVE_HEIGHT_EPSILON;
		TraceHull( vecUp, vecDown, mins, maxs, collisionMask, &trace );
	}

	// this should have hit a solid surface by now
	if (trace.fraction == 1 || trace.allsolid || ( fStartedInObject && trace.startsolid ) )
	{
		// set result to start position if it doesn't work
		*pVecResult = vecStart;
		if ( fStartedInObject )
			return true; // in this case, probably got intruded on by a physics object. Try ignoring it...
		return false;
	}

	*pVecResult = trace.endpos;
	return true;
}

