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

#ifndef _INCLUDE_ENTITYOUTPUT_H_
#define _INCLUDE_ENTITYOUTPUT_H_

#include "CEntity.h"
#include "variant_t.h"
#include "datamap.h"


#define EVENT_FIRE_ALWAYS	-1

class CEventAction
{
public:
	CEventAction( const char *ActionData = NULL );

	string_t m_iTarget; // name of the entity(s) to cause the action in
	string_t m_iTargetInput; // the name of the action to fire
	string_t m_iParameter; // parameter to send, 0 if none
	float m_flDelay; // the number of seconds to wait before firing the action
	int m_nTimesToFire; // The number of times to fire this event, or EVENT_FIRE_ALWAYS.

	int m_iIDStamp;	// unique identifier stamp

	static int s_iNextIDStamp;

	CEventAction *m_pNext; 
/*
	// allocates memory from engine.MPool/g_EntityListPool
	static void *operator new( size_t stAllocateBlock );
	static void *operator new( size_t stAllocateBlock, int nBlockUse, const char *pFileName, int nLine );
	static void operator delete( void *pMem );
	static void operator delete( void *pMem , int nBlockUse, const char *pFileName, int nLine ) { operator delete(pMem); }
*/
	DECLARE_SIMPLE_DATADESC();

};

class CBaseEntityOutput
{
public:

	~CBaseEntityOutput();

	void ParseEventAction( const char *EventData );
	void AddEventAction( CEventAction *pEventAction );

	int Save( ISave &save );
	int Restore( IRestore &restore, int elementCount );

	int NumberOfElements( void );

	float GetMaxDelay( void );

	fieldtype_t ValueFieldType() { return m_Value.FieldType(); }

	void FireOutput( variant_t Value, CBaseEntity *pActivator, CBaseEntity *pCaller, float fDelay = 0 );

	/// Delete every single action in the action list. 
	void DeleteAllElements( void ) ;

protected:
	variant_t m_Value;
	CEventAction *m_ActionList;
	DECLARE_SIMPLE_DATADESC();
	
	CBaseEntityOutput() { m_ActionList = NULL; } // this class cannot be created, only it's children

private:
	CBaseEntityOutput( CBaseEntityOutput& ); // protect from accidental copying
};

class COutputEvent : public CBaseEntityOutput
{
public:
	// void Firing, no parameter
	void FireOutput(CBaseEntity *pActivator, CBaseEntity *pCaller, float fDelay = 0);
	void FireOutput(CEntity *pActivator, CEntity *pCaller, float fDelay = 0);
	void FireOutput(CBaseEntity *pActivator, CEntity *pCaller, float fDelay = 0);
	void FireOutput(CEntity *pActivator, CBaseEntity *pCaller, float fDelay = 0);
};

//-----------------------------------------------------------------------------
// Purpose: wraps variant_t data handling in convenient, compiler type-checked template
//-----------------------------------------------------------------------------
template< class Type, fieldtype_t fieldType >
class CEntityOutputTemplate : public CBaseEntityOutput
{
public:
	//
	// Sets an initial value without firing the output.
	//
	void Init( Type value ) 
	{
		m_Value.Set( fieldType, &value );
	}

	//
	// Sets a value and fires the output.
	//
	void Set( Type value, CBaseEntity *pActivator, CBaseEntity *pCaller ) 
	{
		m_Value.Set( fieldType, &value );
		FireOutput( m_Value, pActivator, pCaller );
	}

	//
	// Returns the current value.
	//
	Type Get( void )
	{
		return *((Type*)&m_Value);
	}
};


typedef CEntityOutputTemplate<variant_t,FIELD_INPUT>		COutputVariant;
typedef CEntityOutputTemplate<int,FIELD_INTEGER>			COutputInt;
typedef CEntityOutputTemplate<float,FIELD_FLOAT>			COutputFloat;
typedef CEntityOutputTemplate<string_t,FIELD_STRING>		COutputString;
typedef CEntityOutputTemplate<EHANDLE,FIELD_EHANDLE>		COutputEHANDLE;
typedef CEntityOutputTemplate<Vector,FIELD_VECTOR>			COutputVector;
typedef CEntityOutputTemplate<Vector,FIELD_POSITION_VECTOR>	COutputPositionVector;
typedef CEntityOutputTemplate<color32,FIELD_COLOR32>		COutputColor32;



abstract_class CBaseEntityClassList
{
public:
	CBaseEntityClassList();
	~CBaseEntityClassList();
	virtual void LevelShutdownPostEntity() = 0;

	CBaseEntityClassList *m_pNextClassList;
};

template< class T >
class CEntityClassList : public CBaseEntityClassList
{
public:
	virtual void LevelShutdownPostEntity()  { m_pClassList = NULL; }

	void Insert( T *pEntity )
	{
		pEntity->m_pNext = m_pClassList;
		m_pClassList = pEntity;
	}

	void Remove( T *pEntity )
	{
		T **pPrev = &m_pClassList;
		T *pCur = *pPrev;
		while ( pCur )
		{
			if ( pCur == pEntity )
			{
				*pPrev = pCur->m_pNext;
				return;
			}
			pPrev = &pCur->m_pNext;
			pCur = *pPrev;
		}
	}

	static T *m_pClassList;
};


#endif // _INCLUDE_ENTITYOUTPUT_H_
