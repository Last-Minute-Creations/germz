#ifndef _GERMZ_VALUE_PTR_H_
#define _GERMZ_VALUE_PTR_H_

#include <ace/types.h>

UBYTE isValuePtr(const void *pVal);

ULONG valuePtrUnpack(const void *pVal);

void *valuePtrPack(ULONG ulVal);

#endif // _GERMZ_VALUE_PTR_H_
