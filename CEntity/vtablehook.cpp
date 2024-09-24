#include "vtablehook.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#include <cstdint>
#endif

void* VTableHook(void *classptr, int offset, void *new_method)
{
	void **vtable = *(void ***) classptr;
	void *old_method = vtable[offset];

#ifdef _WIN32
	DWORD oldProtect;
	VirtualProtect(vtable, offset * sizeof(void*), PAGE_EXECUTE_READWRITE, &oldProtect);
#else
	long page_size = sysconf(_SC_PAGESIZE);
	void *page_start = (void *)((uintptr_t)vtable & ~(page_size - 1));
	mprotect(page_start, page_size, PROT_READ | PROT_WRITE);
#endif

	vtable[offset] = new_method;

#ifdef _WIN32
	DWORD dummy;
	VirtualProtect(vtable, offset * sizeof(void*), oldProtect, &dummy);
#else
	mprotect(page_start, page_size, PROT_READ);
#endif

	return old_method;
}