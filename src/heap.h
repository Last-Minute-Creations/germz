/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _GERMZ_HEAP_H_
#define _GERMZ_HEAP_H_

#include <ace/types.h>

typedef struct _tHeapEntry {
	UWORD uwPriority;
	const void *pData;
} tHeapEntry;

typedef struct _tHeap {
	UWORD uwMaxEntries;
	UWORD uwCount;
	tHeapEntry *pEntries;
} tHeap;

tHeap *heapCreate(UWORD uwMaxEntries);

void heapDestroy(tHeap *pHeap);

void heapPush(tHeap *pHeap, const void *pData, UWORD uwPriority);

const void *heapPop(tHeap *pHeap);

static inline void heapClear(tHeap *pHeap) {
	pHeap->uwCount = 0;
}

#endif // _GERMZ_HEAP_H_
