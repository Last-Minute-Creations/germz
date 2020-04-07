/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "player.h"
#include <ace/managers/joy.h>

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
	}
}

//------------------------------------------------------------------- PUBLIC FNS

void playerCreate(void) {
	s_pCursors = bitmapCreateFromFile("data/cursors.bm", 0);
	s_pCursorsMask = bitmapCreateFromFile("data/cursors_mask.bm", 0);
	plepCreate();
}

void playerDestroy(void) {
	bitmapDestroy(s_pCursors);
	bitmapDestroy(s_pCursorsMask);
	plepDestroy();
}

void playerReset(UBYTE ubIdx, tNode *pStartNode, UBYTE ubJoy) {
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
	s_pPlayers[ubIdx].ubJoy = ubJoy;
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
		ubPlayerIdx = 1 + (UBYTE)(pPlayer - s_pPlayers) / sizeof(pPlayer);
	}
	return ubPlayerIdx;
}

void playerProcess(void) {
	for(UBYTE i = 0; i < PLAYER_COUNT_MAX; ++i) {
		tPlayer *pPlayer = &s_pPlayers[i];
		tDir eDir = DIR_COUNT;
		if(joyUse(pPlayer->ubJoy + JOY_LEFT)) {
			eDir = DIR_LEFT;
		}
		else if(joyUse(pPlayer->ubJoy + JOY_RIGHT)) {
			eDir = DIR_RIGHT;
		}
		else if(joyUse(pPlayer->ubJoy + JOY_UP)) {
			eDir = DIR_UP;
		}
		else if(joyUse(pPlayer->ubJoy + JOY_DOWN)) {
			eDir = DIR_DOWN;
		}
		if(pPlayer->isSelectingDestination) {
			playerTryMoveSelectionFromInDir(pPlayer, pPlayer->pNodePlepSrc, eDir);
			if(joyUse(pPlayer->ubJoy + JOY_FIRE)) {
				playerSpawnPlep(pPlayer);
				playerSetCursorPos(pPlayer, pPlayer->pNodePlepSrc);
				pPlayer->isSelectingDestination = 0;
			}
		}
		else {
			playerTryMoveSelectionFromInDir(pPlayer, pPlayer->pNodeCursor, eDir);
			if(joyUse(pPlayer->ubJoy + JOY_FIRE)) {
				if(pPlayer->pNodeCursor->pPlayer == pPlayer) {
					pPlayer->pNodePlepSrc = pPlayer->pNodeCursor;
					pPlayer->isSelectingDestination = 1;
				}
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

void playerSpawnPlep(tPlayer *pPlayer) {
	// TODO: store last index, add starting from it
	for(UBYTE i = 0; i < PLEPS_PER_PLAYER; ++i) {
		tPlep *pPlep = &pPlayer->pPleps[i];
		if(!pPlep->isActive) {
			plepSpawn(pPlep);
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
