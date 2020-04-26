/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "player.h"

//---------------------------------------------------------------------- DEFINES

#define PLAYER_COUNT_MAX 4

//------------------------------------------------------------------------ TYPES

//----------------------------------------------------------------- PRIVATE VARS

tPlayer s_pPlayers[PLAYER_COUNT_MAX];
static tBitMap *s_pCursors, *s_pCursorsMask;

//------------------------------------------------------------------ PRIVATE FNS

static void playerSetCursorPos(tPlayer *pPlayer, tNode *pNode) {
		pPlayer->pNodeCursor = pNode;
		pPlayer->sBobCursor.sPos.uwX = pNode->ubTileX * 16;
		pPlayer->sBobCursor.sPos.uwY = pNode->ubTileY * 16;
}

static void playerTryMoveSelectionFromInDir(
	tPlayer *pPlayer, tNode *pNode, tDir eDir
) {
	if(eDir != DIR_COUNT && pNode->pNeighbors[eDir]) {
		playerSetCursorPos(pPlayer, pNode->pNeighbors[eDir]);
		pPlayer->eLastDir = eDir;
	}
}

static void playerSpawnPlep(tPlayer *pPlayer) {
	// TODO: store last index, add starting from it
	WORD wPlepCharges = pPlayer->pNodePlepSrc->wCharges / 2;
	if(
		!wPlepCharges || pPlayer->eLastDir >= DIR_FIRE ||
		!pPlayer->pNodePlepSrc->pNeighbors[pPlayer->eLastDir]
	) {
		return;
	}
	for(UBYTE i = 0; i < PLEPS_PER_PLAYER; ++i) {
		tPlep *pPlep = &pPlayer->pPleps[i];
		if(!pPlep->isActive && pPlayer->pNodePlepSrc->pPlayer == pPlayer) {
			plepSpawn(pPlep, wPlepCharges, pPlayer->eLastDir);
			pPlayer->eLastDir = DIR_COUNT;
			pPlayer->pNodePlepSrc->wCharges -= wPlepCharges;
			logWrite(
				"Spawned plep %hhu on player %hhu: blob %hhu,%hhu -> %hhu,%hhu\n",
				i, playerToIdx(pPlayer),
				pPlayer->pNodePlepSrc->ubTileX, pPlayer->pNodePlepSrc->ubTileY,
				pPlep->pDestination->ubTileX, pPlep->pDestination->ubTileY
			);
			break;
		}
	}
}

//------------------------------------------------------------------- PUBLIC FNS

void playerCreate(void) {
	logBlockBegin("playerCreate()");
	s_pCursors = bitmapCreateFromFile("data/cursors.bm", 0);
	s_pCursorsMask = bitmapCreateFromFile("data/cursors_mask.bm", 0);
	plepCreate();
	logBlockEnd("playerCreate()");
}

void playerDestroy(void) {
	bitmapDestroy(s_pCursors);
	bitmapDestroy(s_pCursorsMask);
	plepDestroy();
}

void playerReset(UBYTE ubIdx, tNode *pStartNode, tSteer sSteer) {
	tPlayer *pPlayer = &s_pPlayers[ubIdx];
	bobNewInit(
		&pPlayer->sBobCursor, 16, 16, 1, s_pCursors, s_pCursorsMask, 0, 0
	);
	for(UBYTE ubPlep = 0; ubPlep < PLEPS_PER_PLAYER; ++ubPlep) {
		plepReset(&pPlayer->pPleps[ubPlep], pPlayer);
	}
	bobNewSetBitMapOffset(&pPlayer->sBobCursor, ubIdx * 16);
	pPlayer->pNodeCursor = pStartNode;
	pPlayer->sBobCursor.sPos.uwX = pPlayer->pNodeCursor->ubTileX * 16;
	pPlayer->sBobCursor.sPos.uwY = pPlayer->pNodeCursor->ubTileY * 16;
	pPlayer->isSelectingDestination = 0;
	s_pPlayers[ubIdx].sSteer = sSteer;
	pPlayer->isDead = 0;
	pPlayer->bNodeCount = 0;
}

tTile playerToTile(const tPlayer *pPlayer) {
	tTile eTile = TILE_BLOB_NEUTRAL + playerToIdx(pPlayer);
	return eTile;
}

tPlayer *playerFromTile(tTile eTile) {
	tPlayer *pPlayer = 0;
	if(eTile >= TILE_BLOB_P1) {
		pPlayer = &s_pPlayers[eTile - TILE_BLOB_P1];
	}
	return pPlayer;
}

UBYTE playerToIdx(const tPlayer *pPlayer) {
	UBYTE ubPlayerIdx = 0;
	if(pPlayer) {
		ubPlayerIdx = 1 + (UBYTE)(pPlayer - &s_pPlayers[0]);
	}
	return ubPlayerIdx;
}

tPlayer *playerFromIdx(UBYTE ubIdx) {
	tPlayer *pPlayer = 0;
	if(ubIdx > 0) {
		pPlayer = &s_pPlayers[ubIdx - 1];
	}
	return pPlayer;
}

void playerProcess(void) {
	for(UBYTE i = 0; i < PLAYER_COUNT_MAX; ++i) {
		tPlayer *pPlayer = &s_pPlayers[i];
		if(pPlayer->isDead) {
			continue;
		}
		tDir eDir = steerProcess(&pPlayer->sSteer);
		if(pPlayer->isSelectingDestination) {
			if(eDir == DIR_FIRE) {
				playerSpawnPlep(pPlayer);
				playerSetCursorPos(pPlayer, pPlayer->pNodePlepSrc);
				pPlayer->isSelectingDestination = 0;
			}
			else {
				playerTryMoveSelectionFromInDir(pPlayer, pPlayer->pNodePlepSrc, eDir);
			}
		}
		else {
			if(eDir == DIR_FIRE) {
				if(pPlayer->pNodeCursor->pPlayer == pPlayer) {
					pPlayer->pNodePlepSrc = pPlayer->pNodeCursor;
					pPlayer->isSelectingDestination = 1;
				}
			}
			else {
				playerTryMoveSelectionFromInDir(pPlayer, pPlayer->pNodeCursor, eDir);
			}
		}

		for(UBYTE i = 0; i < PLEPS_PER_PLAYER; ++i) {
			if(pPlayer->pPleps[i].isActive) {
				plepProcess(&pPlayer->pPleps[i]);
			}
		}

		bobNewPush(&pPlayer->sBobCursor);
	}
}

void playerUpdateDead(tPlayer *pPlayer) {
	// Check if player has some blobs or is dead already
	logWrite("playerUpdateDead: %p has %hhd blobs\n", pPlayer, pPlayer->bNodeCount);
	if(pPlayer->isDead || pPlayer->bNodeCount > 0) {
		return;
	}

	// Check if player has any plep
	for(UBYTE i = 0; i < PLEPS_PER_PLAYER; ++i) {
		if(pPlayer->pPleps[i].isActive) {
			logWrite("has active pleps!\n");
			return;
		}
	}

	// Nothing more to check - player is dead
	logWrite("yep he ded\n");
	pPlayer->isDead = 1;
}
