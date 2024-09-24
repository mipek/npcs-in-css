#include "mempool.h"
#include "extension.h"

void* CEMemoryPool::Alloc(size_t amount)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("CUtlMemoryPool::Alloc_uint", &func))
		{
			assert(0);
			return NULL;
		}
	}

	typedef void* (*_func)(void*, size_t);
	_func thisfunc = (_func)func;
	return thisfunc(this, amount);
}

void CEMemoryPool::Free(void *pMem)
{
	static void *func = NULL;
	if(!func)
	{
		if(!g_pGameConf->GetMemSig("CUtlMemoryPool::Free", &func))
		{
			assert(0);
			return;
		}
	}

	typedef void* (*_func)(void*, void*);
	_func thisfunc = (_func)func;
	thisfunc(this, pMem);
}