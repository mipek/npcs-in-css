
#include "CAI_NPC.h"
#include "ai_movesolver.h"


// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



inline float round( float f )
{
	return (float)( (int)( f + 0.5 ) );
}


//-----------------------------------------------------------------------------
// CAI_MoveSolver
//-----------------------------------------------------------------------------

// The epsilon used by the solver
const float AIMS_EPS = 0.01;




//-------------------------------------
// Purpose: The actual solver function. Reweights according to type and sums
//			all the suggestions, identifying the best.
//-------------------------------------
bool CAI_MoveSolver::Solve( const AI_MoveSuggestion_t *pSuggestions, int nSuggestions, AI_MoveSolution_t *pResult)
{
	//---------------------------------
	//
	// Quick out
	//
	if ( !nSuggestions )
		return false;

	if ( nSuggestions == 1 && m_Regulations.Count() == 0 && pSuggestions->type == AIMST_MOVE )
	{
		pResult->dir = pSuggestions->arc.center;
		return true;
	}

	//---------------------------------
	//
	// Setup
	//
	CAI_MoveSuggestions suggestions;

	suggestions.EnsureCapacity( m_Regulations.Count() + nSuggestions );

	suggestions.CopyArray( pSuggestions, nSuggestions);
	suggestions.AddVectorToTail( m_Regulations );

	// Initialize the solver
	const int NUM_SOLUTIONS	= 120;
	const int SOLUTION_ANG	= 360 / NUM_SOLUTIONS;

	COMPILE_TIME_ASSERT( ( 360 % NUM_SOLUTIONS ) == 0 );

	struct Solution_t
	{
		// The sum bias
		float					   bias;
		float					   highBias;
		AI_MoveSuggestion_t *pHighSuggestion;
	};

	Solution_t 	solutions[NUM_SOLUTIONS]	= { {0.0f, 0.0f, NULL} };

	//---------------------------------

	// The first thing we do is reweight and normalize the weights into a range of [-1..1], where
	// a negative weight is a repulsion. This becomes a bias for the solver.
	// @TODO (toml 06-18-02): this can be made sligtly more optimal by precalculating regulation adjusted weights
	Assert( suggestions.Count() >= 1 );
	NormalizeSuggestions( &suggestions[0], (&suggestions[0]) + suggestions.Count() );

	//
	// Add the biased suggestions to the solutions
	//
	for ( int iSuggestion = 0; iSuggestion < suggestions.Count(); ++iSuggestion )
	{
		AI_MoveSuggestion_t &current = suggestions[iSuggestion];

		// Convert arc values to solution indices relative to right post. Right is angle down, left is angle up.
		float halfSpan	= current.arc.span * 0.5;
		int   center 	= (int)round( ( halfSpan * NUM_SOLUTIONS ) / 360 );
		int   left		= (int)( (current.arc.span * NUM_SOLUTIONS) / 360 );

		float angRight   = current.arc.center - halfSpan;

		if (angRight < 0.0)
			angRight += 360;

		int base = (int)( (angRight * NUM_SOLUTIONS) / 360 );

		// Sweep from left to right, summing the bias. For positive suggestions,
		// the bias is further weighted to favor the center of the arc.
		const float positiveDegradePer180 = 0.05; // i.e., lose 5% of weight by the time hit 180 degrees off center
		const float positiveDegrade       = ( positiveDegradePer180 / ( NUM_SOLUTIONS * 0.5 ) ); 

		for ( int i = 0; i < left + 1; ++i )
		{
			float bias = 0.0;

			if ( current.weight > 0)
			{
				int	iOffset = center - i;
				float degrade = abs( iOffset ) * positiveDegrade;

				if ( ( (current.flags & AIMS_FAVOR_LEFT ) && i > center ) || 
					 ( (current.flags & AIMS_FAVOR_RIGHT) && i < center ) )
				{
					degrade *= 0.9;
				}

				bias = current.weight - ( current.weight * degrade );
			}
			else
				bias = current.weight;

			int iCurSolution = (base + i) % NUM_SOLUTIONS;

			solutions[iCurSolution].bias += bias;
			if ( bias > solutions[iCurSolution].highBias )
			{
				solutions[iCurSolution].highBias        = bias;
				solutions[iCurSolution].pHighSuggestion = &current;
			}
		}
	}

	//
	// Find the best solution
	//
	int   best     = -1;
	float biasBest = 0;

	for ( int i = 0; i < NUM_SOLUTIONS; ++i )
	{
		if ( solutions[i].bias > biasBest )
		{
			best     = i;
			biasBest = solutions[i].bias;
		}
	}

	if ( best == -1 )
		return false; // no solution

	//
	// Construct the results
	//
	float result = best * SOLUTION_ANG;

	// If the matching suggestion is within the solution, use that as the result,
	// as it is valid and more precise.
	const float suggestionCenter = solutions[best].pHighSuggestion->arc.center;

	if ( suggestionCenter > result && suggestionCenter <= result + SOLUTION_ANG )
		result = suggestionCenter;

	pResult->dir = result;

	return true;
}




//-------------------------------------
// Purpose: Adjusts the suggestion weights according to the type of the suggestion,
//			apply the appropriate sign, ensure values are in expected ranges
//-------------------------------------

struct AI_MoveSuggWeights
{
	float min;
	float max;
};

static AI_MoveSuggWeights g_AI_MoveSuggWeights[] = // @TODO (toml 06-18-02): these numbers need tuning
{
	{  0.20,  1.00 },	// AIMST_MOVE
	{ -0.00, -0.25 },	// AIMST_AVOID_DANGER
	{ -0.00, -0.25 },	// AIMST_AVOID_OBJECT
	{ -0.00, -0.25 },	// AIMST_AVOID_NPC
	{ -0.00, -0.25 },	// AIMST_AVOID_WORLD
	{ -1.00, -1.00 },	// AIMST_NO_KNOWLEDGE
	{ -0.60, -0.60 },	// AIMST_OSCILLATION_DETERRANCE
	{  0.00,  0.00 },	// AIMST_INVALID
};

void CAI_MoveSolver::NormalizeSuggestions( AI_MoveSuggestion_t *pBegin, AI_MoveSuggestion_t *pEnd )
{
	while ( pBegin != pEnd )
	{
		const float min = g_AI_MoveSuggWeights[pBegin->type].min;
		const float max = g_AI_MoveSuggWeights[pBegin->type].max;

		Assert( pBegin->weight >= -AIMS_EPS && pBegin->weight <= 1.0 + AIMS_EPS );

		if ( pBegin->weight < AIMS_EPS ) // zero normalizes to zero
			pBegin->weight = 0.0;
		else
			pBegin->weight = ( ( max - min ) * pBegin->weight ) + min;

		while (pBegin->arc.center < 0)
			pBegin->arc.center += 360;

		while (pBegin->arc.center >= 360)
			pBegin->arc.center -= 360;

		++pBegin;
	}
}





