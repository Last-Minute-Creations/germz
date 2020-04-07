/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "plep.h"
#include "display.h"

//---------------------------------------------------------------------- DEFINES

#define PLEP_SIZE 16

//------------------------------------------------------------------------ TYPES

//----------------------------------------------------------------- PRIVATE VARS

static tBitMap *s_pBmPleps[4];
static tBitMap *s_pBmPlepMask;

//------------------------------------------------------------------ PRIVATE FNS

static void plepSinkInNode(tPlep *pPlep) {
	pPlep->isActive = 0;
	if(pPlep->pDestination->pPlayer != pPlep->pPlayer) {
		pPlep->pDestination->pPlayer = pPlep->pPlayer;
		displayAddNodeToQueue(pPlep->pDestination);
	}
}

//------------------------------------------------------------------- PUBLIC FNS

void plepCreate(void) {
	for(UBYTE i = 0; i < 4; ++i) {
		char szName[15];
		sprintf(szName, "data/plep%hhu.bm", i);
		s_pBmPleps[i] = bitmapCreateFromFile(szName, 0);
	}
	s_pBmPlepMask = bitmapCreateFromFile("data/plep_mask.bm", 0);
}

void plepDestroy(void) {
	for(UBYTE i = 0; i < 4; ++i) {
		bitmapDestroy(s_pBmPleps[i]);
	}
	bitmapDestroy(s_pBmPlepMask);
}

void plepReset(tPlep *pPlep, tPlayer *pPlayer) {
	pPlep->isActive = 0;
	pPlep->pPlayer = pPlayer;
	bobNewInit(
		&pPlep->sBob, PLEP_SIZE, PLEP_SIZE, 1, s_pBmPleps[playerToIdx(pPlayer)], s_pBmPlepMask, 0, 0
	);
}

void plepProcess(tPlep *pPlep) {
	if(pPlep->isActive) {
		pPlep->sBob.sPos.uwX += pPlep->bDeltaX;
		pPlep->sBob.sPos.uwY += pPlep->bDeltaY;
		if(
			pPlep->sBob.sPos.uwX == pPlep->pDestination->ubTileX * 16 &&
			pPlep->sBob.sPos.uwY == pPlep->pDestination->ubTileY * 16
		) {
			plepSinkInNode(pPlep);
		}
		else {
			bobNewPush(&pPlep->sBob);
		}
	}
}

void plepSpawn(tPlep *pPlep) {
	const tNode *pSrc = pPlep->pPlayer->pNodePlepSrc;
	pPlep->pDestination = pPlep->pPlayer->pNodeCursor;
	pPlep->isActive = 1;
	pPlep->sBob.sPos.uwX = pSrc->ubTileX * 16;
	pPlep->sBob.sPos.uwY = pSrc->ubTileY * 16;
	pPlep->bDeltaX = SGN(pPlep->pDestination->ubTileX - pSrc->ubTileX);
	pPlep->bDeltaY = SGN(pPlep->pDestination->ubTileY - pSrc->ubTileY);
	bobNewSetBitMapOffset(&pPlep->sBob, 0);
}
