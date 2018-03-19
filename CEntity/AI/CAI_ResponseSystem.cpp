
#include "CAI_NPC.h"
#include "CAI_ResponseSystem.h"
#include "CAI_Criteria.h"
#include "GameSystem.h"



// memdbgon must be the last include file in a .cpp file!!!
#include "tier0/memdbgon.h"



struct Criteria;
struct Rule;
struct ResponseGroup;
struct Response;
class Matcher;
class CInstancedResponseSystem;



abstract_class CResponseSystem : public IResponseSystem 
{
public:
	CResponseSystem();
	~CResponseSystem();

	// IResponseSystem
	virtual bool FindBestResponse( const AI_CriteriaSet& set, AI_Response& response, IResponseFilter *pFilter = NULL );
	virtual void GetAllResponses( CUtlVector<AI_Response *> *pResponses );

	virtual void Release() = 0;

	virtual void DumpRules();

	virtual void Precache();

	virtual void PrecacheResponses( bool bEnable )
	{
	}

	bool		ShouldPrecache()	{ return m_bPrecache; }
	bool		IsCustomManagable()	{ return m_bCustomManagable; }

	void		Clear();

	void		DumpDictionary( const char *pszName );

protected:

	virtual const char *GetScriptFile( void ) = 0;
	void		LoadRuleSet( const char *setname );

	void		ResetResponseGroups();

	float		LookForCriteria( const AI_CriteriaSet &criteriaSet, int iCriteria );
	float		RecursiveLookForCriteria( const AI_CriteriaSet &criteriaSet, Criteria *pParent );

public:

	void		CopyRuleFrom( Rule *pSrcRule, int iRule, CResponseSystem *pCustomSystem );
	void		CopyCriteriaFrom( Rule *pSrcRule, Rule *pDstRule, CResponseSystem *pCustomSystem );
	void		CopyResponsesFrom( Rule *pSrcRule, Rule *pDstRule, CResponseSystem *pCustomSystem );
	void		CopyEnumerationsFrom( CResponseSystem *pCustomSystem );

//private:

	struct Enumeration
	{
		float		value;
	};

	struct ResponseSearchResult
	{
		ResponseSearchResult()
		{
			group = NULL;
			action = NULL;
		}

		ResponseGroup	*group;
		Response		*action;
	};

	inline bool ParseToken( void )
	{
		if ( m_bUnget )
		{
			m_bUnget = false;
			return true;
		}
		if ( m_ScriptStack.Count() <= 0 )
		{
			Assert( 0 );
			return false;
		}

		m_ScriptStack[ 0 ].currenttoken = engine->ParseFile( m_ScriptStack[ 0 ].currenttoken, token, sizeof( token ) );
		m_ScriptStack[ 0 ].tokencount++;
		return m_ScriptStack[ 0 ].currenttoken != NULL ? true : false;
	}

	inline void Unget()
	{
		m_bUnget = true;
	}

	inline bool TokenWaiting( void )
	{
		if ( m_ScriptStack.Count() <= 0 )
		{
			Assert( 0 );
			return false;
		}

		const char *p = m_ScriptStack[ 0 ].currenttoken;

		if ( !p )
		{
			Error( "AI_ResponseSystem:  Unxpected TokenWaiting() with NULL buffer in %s", m_ScriptStack[ 0 ].name );
			return false;
		}


		while ( *p && *p!='\n')
		{
			// Special handler for // comment blocks
			if ( *p == '/' && *(p+1) == '/' )
				return false;

			if ( !isspace( *p ) || isalnum( *p ) )
				return true;

			p++;
		}

		return false;
	}
	
	void		ParseOneResponse( const char *responseGroupName, ResponseGroup& group );

	void		ParseInclude( CStringPool &includedFiles );
	void		ParseResponse( void );
	void		ParseCriterion( void );
	void		ParseRule( void );
	void		ParseEnumeration( void );

	int			ParseOneCriterion( const char *criterionName );
	
	bool		Compare( const char *setValue, Criteria *c, bool verbose = false );
	bool		CompareUsingMatcher( const char *setValue, Matcher& m, bool verbose = false );
	void		ComputeMatcher( Criteria *c, Matcher& matcher );
	void		ResolveToken( Matcher& matcher, char *token, size_t bufsize, char const *rawtoken );
	float		LookupEnumeration( const char *name, bool& found );

	int			FindBestMatchingRule( const AI_CriteriaSet& set, bool verbose );

	float		ScoreCriteriaAgainstRule( const AI_CriteriaSet& set, int irule, bool verbose = false );
	float		RecursiveScoreSubcriteriaAgainstRule( const AI_CriteriaSet& set, Criteria *parent, bool& exclude, bool verbose /*=false*/ );
	float		ScoreCriteriaAgainstRuleCriteria( const AI_CriteriaSet& set, int icriterion, bool& exclude, bool verbose = false );
	bool		GetBestResponse( ResponseSearchResult& result, Rule *rule, bool verbose = false, IResponseFilter *pFilter = NULL );
	bool		ResolveResponse( ResponseSearchResult& result, int depth, const char *name, bool verbose = false, IResponseFilter *pFilter = NULL );
	int			SelectWeightedResponseFromResponseGroup( ResponseGroup *g, IResponseFilter *pFilter );
	void		DescribeResponseGroup( ResponseGroup *group, int selected, int depth );
	void		DebugPrint( int depth, const char *fmt, ... );

	void		LoadFromBuffer( const char *scriptfile, const char *buffer, CStringPool &includedFiles );

	void		GetCurrentScript( char *buf, size_t buflen );
	int			GetCurrentToken() const;
	void		SetCurrentScript( const char *script );
	bool		IsRootCommand();

	void		PushScript( const char *scriptfile, unsigned char *buffer );
	void		PopScript(void);

	void		ResponseWarning( const char *fmt, ... );

	CUtlDict< ResponseGroup, short >	m_Responses;
	CUtlDict< Criteria, short >	m_Criteria;
	CUtlDict< Rule, short >	m_Rules;
	CUtlDict< Enumeration, short > m_Enumerations;

	char		token[ 1204 ];

	bool		m_bUnget;
	bool		m_bPrecache;	

	bool		m_bCustomManagable;

	struct ScriptEntry
	{
		unsigned char	*buffer;
		FileNameHandle_t name;
		const char		*currenttoken;
		int				tokencount;
	};

	CUtlVector< ScriptEntry >		m_ScriptStack;

	friend class CDefaultResponseSystemSaveRestoreBlockHandler;
	friend class CResponseSystemSaveRestoreOps;
};



class CDefaultResponseSystem : public CResponseSystem, public CValveAutoGameSystem
{
	typedef CValveAutoGameSystem BaseClass;

public:
	CDefaultResponseSystem() : CValveAutoGameSystem( "CDefaultResponseSystem" )
	{
	}

	virtual const char *GetScriptFile( void ) 
	{
		return "scripts/talker/response_rules.txt";
	}

	// CAutoServerSystem
	virtual bool Init();
	virtual void Shutdown();

	virtual void LevelInitPostEntity()
	{
	}

	virtual void Release()
	{
		Assert( 0 );
	}

	void AddInstancedResponseSystem( const char *scriptfile, CInstancedResponseSystem *sys );

	CInstancedResponseSystem *FindResponseSystem( const char *scriptfile );

	IResponseSystem *PrecacheCustomResponseSystem( const char *scriptfile );

	IResponseSystem *BuildCustomResponseSystemGivenCriteria( const char *pszBaseFile, const char *pszCustomName, AI_CriteriaSet &criteriaSet, float flCriteriaScore );
	void DestroyCustomResponseSystems();

	virtual void LevelInitPreEntity()
	{
	}

	void ReloadAllResponseSystems();
private:

	void ClearInstanced();

	CUtlDict< CInstancedResponseSystem *, int > m_InstancedSystems;
};

IResponseSystem *g_pResponseSystem = NULL;
extern CUtlVector<IValveGameSystem*> *s_GameSystems;

bool SetResponseSystem()
{
	FindValveGameSystem(g_pResponseSystem, CDefaultResponseSystem *, "CDefaultResponseSystem");	

	return true;
}

