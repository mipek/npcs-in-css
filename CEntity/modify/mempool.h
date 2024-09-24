//===== Copyright � 1996-2005, Valve Corporation, All rights reserved. ======//
//
// Purpose:
//
// $Workfile:     $
// $Date:         $
//
//-----------------------------------------------------------------------------
// $Log: $
//
// $NoKeywords: $
//===========================================================================//

#ifndef MEMPOOL_H
#define MEMPOOL_H

#ifdef _WIN32
#pragma once
#endif

#include "tier0/memalloc.h"
#include "tier0/tslist.h"
#include "tier0/platform.h"
#include "tier1/utlvector.h"
#include "tier1/utlrbtree.h"

//-----------------------------------------------------------------------------
// Purpose: Optimized pool memory allocator
//-----------------------------------------------------------------------------

typedef void (*MemoryPoolReportFunc_t)( char const* pMsg, ... );

class CEMemoryPool
{
public:
	// Ways the memory pool can grow when it needs to make a new blob.
	enum MemoryPoolGrowType_t
	{
		GROW_NONE=0,		// Don't allow new blobs.
		GROW_FAST=1,		// New blob size is numElements * (i+1)  (ie: the blocks it allocates
		// get larger and larger each time it allocates one).
		GROW_SLOW=2			// New blob size is numElements.
	};

	CEMemoryPool( int blockSize, int numElements, int growMode = GROW_FAST, const char *pszAllocOwner = NULL, int nAlignment = 0 );
	~CEMemoryPool();

	void*		Alloc();	// Allocate the element size you specified in the constructor.
	void*		Alloc( size_t amount );
	void*		AllocZero();	// Allocate the element size you specified in the constructor, zero the memory before construction
	void*		AllocZero( size_t amount );
	void		Free(void *pMem);

	// Frees everything
	void		Clear();

	// Error reporting...
	static void SetErrorReportFunc( MemoryPoolReportFunc_t func );

	// returns number of allocated blocks
	int Count() { return m_BlocksAllocated; }
	int PeakCount() { return m_PeakAlloc; }

protected:
	class CBlob
	{
	public:
		CBlob	*m_pPrev, *m_pNext;
		int		m_NumBytes;		// Number of bytes in this blob.
		char	m_Data[1];
	};

	// Resets the pool
	void		Init();
	void		AddNewBlob();
	void		ReportLeaks();

	int			m_BlockSize;
	int			m_BlocksPerBlob;

	int			m_GrowMode;	// GROW_ enum.

	int				m_BlocksAllocated;
	void			*m_pHeadOfFreeList;
	int				m_PeakAlloc;
	unsigned short	m_nAlignment;
	unsigned short	m_NumBlobs;
	const char *	m_pszAllocOwner;
	CBlob			m_BlobHead;
#if 0
	// FIXME: Change m_ppMemBlob into a growable array?
	CBlob			m_BlobHead;
	void			*m_pHeadOfFreeList;
	int				m_BlocksAllocated;
	int				m_PeakAlloc;
	unsigned short	m_nAlignment;
	unsigned short	m_NumBlobs;
	const char *	m_pszAllocOwner;
#endif

	static MemoryPoolReportFunc_t g_ReportFunc;
};

// CE
typedef CEMemoryPool CUtlMemoryPool;

#endif // MEMPOOL_H
