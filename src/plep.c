/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "plep.h"

//---------------------------------------------------------------------- DEFINES

#define PLEP_SIZE 16

//----------------------------------------------------------------- PRIVATE VARS

static tBitMap *s_pBmPleps[4][PLEP_ANIM_COUNT];
static tBitMap *s_pBmPlepMasks[4][PLEP_ANIM_COUNT];

// For right direction, left must have sign reversed
static const BYTE s_pAnimOffsets[PLEP_ANIM_COUNT][8] = {
	{6, 7, 9, 11},
	{-5, -4, -2, 0, 2, 4, 6, 10},
	{-5, -3, 0, 4},
	{-5, -2, 0, 1, 2, 3, 4, 5}
};

static const UBYTE s_pFramesPerAnim[PLEP_ANIM_COUNT] = {4, 8, 4, 8};
static const tBCoordYX s_pPlepMoveDelta[4] = {
	[DIR_UP] = {.bX = 0, .bY = -16},
	[DIR_DOWN] = {.bX = 0, .bY = 16},
	[DIR_LEFT] = {.bX = -16, .bY = 0},
	[DIR_RIGHT] = {.bX = 16, .bY = 0},
};

//------------------------------------------------------------------ PRIVATE FNS

static UBYTE plepSinkInNode(tPlep *pPlep) {
	tNode *pNode = pPlep->pDestination;
	if(pNode->pPlayer != pPlep->pPlayer) {
		// logWrite("Attacking blob %hd with plep %hd\n", pNode->wCharges, pPlep->wCharges);
		// Attack with plep's charges!
		pNode->wCharges -= pPlep->wCharges;
		if(pNode->wCharges == 0) {
			// Zero charges in blob - make it neutral
			if(pNode->pPlayer) {
				// logWrite("Draw! To neutral\n");
				nodeChangeOwnership(pNode, 0);
				// TODO: if player is selecting from that blob, remove selection
				// TODO: test it
			}
		}
		else if(pNode->wCharges < 0) {
			// Negative charge - capture blob!
			nodeChangeOwnership(pNode, pPlep->pPlayer);
			pNode->wCharges = -pNode->wCharges;
			return 1;
			// logWrite("Capture! %hd\n", pNode->wCharges);
		}
	}
	else {
		// logWrite("Power up! %hd %hd\n", pNode->wCharges, pPlep->wCharges);
		// Power up blob with plep's charges
		pNode->wCharges = pNode->wCharges + pPlep->wCharges;
		return 1;
	}

	// Plep capture / power up failed
	return 0;
}

//------------------------------------------------------------------- PUBLIC FNS

void plepCreate(void) {
	logBlockBegin("plepCreate()");
	static const char *szDirNames[] = {"up", "down", "left", "right"};
	static const char *szAnimNames[] = {"born", "move", "win", "lose"};
	for(UBYTE ubDir = 0; ubDir < 4; ++ubDir) {
		for(UBYTE ubAnim = 0; ubAnim < PLEP_ANIM_COUNT; ++ubAnim) {
			char szName[30];
			sprintf(szName, "data/%s_%s.bm", szAnimNames[ubAnim], szDirNames[ubDir]);
			s_pBmPleps[ubDir][ubAnim] = bitmapCreateFromFile(szName, 0);
			sprintf(szName, "data/%s_%s_mask.bm", szAnimNames[ubAnim], szDirNames[ubDir]);
			s_pBmPlepMasks[ubDir][ubAnim] = bitmapCreateFromFile(szName, 0);
		}
	}
	logBlockEnd("plepCreate()");
}

void plepInitBob(tPlep *pPlep) {
	bobNewInit(
		&pPlep->sBob, PLEP_SIZE, PLEP_SIZE, 1,
		s_pBmPleps[0][0], s_pBmPlepMasks[0][0], 0, 0
	);
}

void plepDestroy(void) {
	for(UBYTE ubDir = 0; ubDir < 4; ++ubDir) {
		for(UBYTE ubAnim = 0; ubAnim < PLEP_ANIM_COUNT; ++ubAnim) {
			bitmapDestroy(s_pBmPleps[ubDir][ubAnim]);
			bitmapDestroy(s_pBmPlepMasks[ubDir][ubAnim]);
		}
	}
}

void plepReset(tPlep *pPlep, tPlayer *pPlayer) {
	pPlep->isActive = 0;
	pPlep->pPlayer = pPlayer;
}

static void plepUpdateAnimFrame(tPlep *pPlep) {
	BYTE bDelta = s_pAnimOffsets[pPlep->eAnim][pPlep->ubAnimFrame];
	if(dirIsVertical(pPlep->eDir)) {
		if(pPlep->eDir == DIR_UP) {
			bDelta = -bDelta;
		}
		pPlep->sBob.sPos.uwX = pPlep->sAnimAnchor.uwX;
		pPlep->sBob.sPos.uwY = pPlep->sAnimAnchor.uwY + bDelta;
	}
	else {
		if(pPlep->eDir == DIR_LEFT) {
			bDelta = -bDelta;
		}
		pPlep->sBob.sPos.uwX = pPlep->sAnimAnchor.uwX + bDelta;
		pPlep->sBob.sPos.uwY = pPlep->sAnimAnchor.uwY;
	}
	bobNewSetBitMapOffset(&pPlep->sBob, pPlep->ubAnimFrame * 16);
}

void plepProcess(tPlep *pPlep) {
	if(!pPlep->isActive) {
		return;
	}

	if(++pPlep->ubAnimTick >= 5) {
		if(pPlep->ubAnimFrame >= s_pFramesPerAnim[pPlep->eAnim]) {
			// end of anim
			// logWrite(
			// 	"End of anim %d (%hu,%hu dir %d)\n",
			// 	pPlep->eAnim, pPlep->sAnimAnchor.uwX, pPlep->sAnimAnchor.uwY, pPlep->eDir
			// );
			switch(pPlep->eAnim) {
				case PLEP_ANIM_BORN:
				case PLEP_ANIM_MOVE:
					pPlep->sAnimAnchor.uwX += s_pPlepMoveDelta[pPlep->eDir].bX;
					pPlep->sAnimAnchor.uwY += s_pPlepMoveDelta[pPlep->eDir].bY;
					tTile eNextTile = g_sMapData.pTiles[pPlep->sAnimAnchor.uwX / 16][pPlep->sAnimAnchor.uwY / 16];
					if(eNextTile >= TILE_BLOB_COUNT) {
						// We're at non-blob tile
						pPlep->eAnim = PLEP_ANIM_MOVE;
					}
					else {
						// We're at blob tile - win or lose against blob
						if(plepSinkInNode(pPlep)) {
							pPlep->eAnim = PLEP_ANIM_WIN;
						}
						else {
							pPlep->eAnim = PLEP_ANIM_LOSE;
						}
					}
					break;
				case PLEP_ANIM_WIN:
				case PLEP_ANIM_LOSE:
				default:
					// end of anim - end of plep
					pPlep->isActive = 0;
					if(pPlep->eAnim == PLEP_ANIM_LOSE) {
						playerUpdateDead(pPlep->pPlayer);
					}
					break;
			}
			pPlep->sBob.pBitmap = s_pBmPleps[pPlep->eDir][pPlep->eAnim];
			pPlep->sBob.pMask = s_pBmPlepMasks[pPlep->eDir][pPlep->eAnim];
			pPlep->ubAnimFrame = 0;
			// logWrite(
			// 	"Next anim: %d (%hu,%hu), active: %hhu\n",
			// 	pPlep->eAnim, pPlep->sAnimAnchor.uwX, pPlep->sAnimAnchor.uwY, pPlep->isActive
			// );
		}
		// Advance frame & anim
		plepUpdateAnimFrame(pPlep);
		++pPlep->ubAnimFrame;
		pPlep->ubAnimTick = 0;
	}
	if(pPlep->isActive) {
		bobNewPush(&pPlep->sBob); // No bob changing past this point
	}
	// gameDumpFrame();
}

void plepSpawn(tPlep *pPlep, WORD wCharges, tDir eDir) {
	const tNode *pSrc = pPlep->pPlayer->pNodePlepSrc;
	pPlep->pDestination = pPlep->pPlayer->pNodeCursor;
	pPlep->isActive = 1;
	pPlep->wCharges = wCharges;
	pPlep->sBob.sPos.uwX = pSrc->ubTileX * MAP_TILE_SIZE;
	pPlep->sBob.sPos.uwY = pSrc->ubTileY * MAP_TILE_SIZE;
	pPlep->eDir = eDir;

	// Init anim
	pPlep->eAnim = PLEP_ANIM_BORN;
	pPlep->sBob.pBitmap = s_pBmPleps[pPlep->eDir][pPlep->eAnim];
	pPlep->sBob.pMask = s_pBmPlepMasks[pPlep->eDir][pPlep->eAnim];
	pPlep->ubAnimFrame = 0;
	pPlep->ubAnimTick = 0;
	pPlep->sAnimAnchor.uwX = pSrc->ubTileX * MAP_TILE_SIZE;
	pPlep->sAnimAnchor.uwY = pSrc->ubTileY * MAP_TILE_SIZE;
	plepUpdateAnimFrame(pPlep);

	bobNewSetBitMapOffset(&pPlep->sBob, 0);
}
