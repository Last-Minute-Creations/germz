#include "value_ptr.h"

#define PACK_BIT (1 << ((sizeof(void*) << 3) - 1))

UBYTE isValuePtr(const void *pVal) {
	if((ULONG)pVal & PACK_BIT) {
		return 1;
	}
	return 0;
}

ULONG valuePtrUnpack(const void *pVal) {
	return (ULONG)pVal & ~PACK_BIT;
}

void *valuePtrPack(ULONG ulVal) {
	return (void*)(ulVal | PACK_BIT);
}
