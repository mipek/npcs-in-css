#ifndef INCLUDE_VTABLEHOOK_H_
#define INCLUDE_VTABLEHOOK_H_

// Performs a vtable hook.
// Do not use this, this is not using SourceHook!
void* VTableHook(void *classptr, int offset, void *callback);

#endif //INCLUDE_VTABLEHOOK_H_