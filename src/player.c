/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "player.h"
#include "game.h"
#include "game_assets.h"

//---------------------------------------------------------------------- DEFINES

#define PLAYER_COUNT_MAX 4

//------------------------------------------------------------------------ TYPES

typedef struct _tCursorField {
	tUbCoordYX sPosTile;
	UBYTE ubCurrentCursor;
	UBYTE ubCursorCount;
	UBYTE ubFrameCounter;
	tBobNew *pBobs[PLAYER_COUNT_MAX];
} tCursorField;

//----------------------------------------------------------------- PRIVATE VARS

tPlayer s_pPlayers[PLAYER_COUNT_MAX];
static tCursorField s_pCursorFields[4];

//------------------------------------------------------------------ PRIVATE FNS

static void playerSetCursorPos(tPlayer *pPlayer, tNode *pNode) {
		pPlayer->pNodeCursor = pNode;
		pPlayer->pBobCursor->sPos.uwX = pNode->sPosTile.ubX * MAP_TILE_SIZE;
		pPlayer->pBobCursor->sPos.uwY = pNode->sPosTile.ubY * MAP_TILE_SIZE;
}

static void playerTryMoveSelectionFromInDir(
	tPlayer *pPlayer, tNode *pNode, tDirection eDir
) {
	if(eDir != DIRECTION_COUNT && pNode->pNeighbors[eDir]) {
		playerSetCursorPos(pPlayer, pNode->pNeighbors[eDir]);
		pPlayer->eLastDir = eDir;
	}
}

static void playerSpawnPlep(tPlayer *pPlayer) {
	// TODO: store last index, add starting from it
	WORD wPlepCharges = pPlayer->pNodePlepSrc->wCharges / 2;
	if(
		!wPlepCharges || pPlayer->eLastDir >= DIRECTION_FIRE ||
		!pPlayer->pNodePlepSrc->pNeighbors[pPlayer->eLastDir]
	) {
		return;
	}
	for(UBYTE i = 0; i < PLEPS_PER_PLAYER; ++i) {
		tPlep *pPlep = &pPlayer->pPleps[i];
		if(!pPlep->isActive && pPlayer->pNodePlepSrc->pPlayer == pPlayer) {
			plepSpawn(pPlep, wPlepCharges, pPlayer->eLastDir);
			pPlayer->eLastDir = DIRECTION_COUNT;
			pPlayer->pNodePlepSrc->wCharges -= wPlepCharges;
			ptplayerSfxPlay(g_pSfxPlep2, 3, 64, 2);
			logWrite(
				"Spawned plep %hhu on player %d: blob %hhu,%hhu -> %hhu,%hhu\n",
				i, playerToIdx(pPlayer),
				pPlayer->pNodePlepSrc->sPosTile.ubX, pPlayer->pNodePlepSrc->sPosTile.ubY,
				pPlep->pDestination->sPosTile.ubX, pPlep->pDestination->sPosTile.ubY
			);
			break;
		}
	}
}

//------------------------------------------------------------------- PUBLIC FNS

void playerCreate(void) {
	logBlockBegin("playerCreate()");
	plepCreate();
	logBlockEnd("playerCreate()");
}

void playerDestroy(void) {
	plepDestroy();
}

void playerReset(tPlayerIdx eIdx, tNode *pStartNode) {
	tPlayer *pPlayer = playerFromIdx(eIdx);
	pPlayer->pBobCursor = gameGetCursorBob(eIdx);
	for(UBYTE ubPlep = 0; ubPlep < PLEPS_PER_PLAYER; ++ubPlep) {
		plepReset(&pPlayer->pPleps[ubPlep], pPlayer);
	}
	logWrite(
		"Player %d start pos: %hhu,%hhu\n",
		eIdx, pStartNode->sPosTile.ubX, pStartNode->sPosTile.ubY
	);
	playerSetCursorPos(pPlayer, pStartNode);
	pPlayer->isSelectingDestination = 0;
	pPlayer->pSteer = gameGetSteerForPlayer(eIdx);
	pPlayer->isDead = 0;
	pPlayer->bNodeCount = 0;
	pPlayer->eLastDir = DIRECTION_COUNT;

	for(UBYTE ubPlep = 0; ubPlep < PLEPS_PER_PLAYER; ++ubPlep) {
		plepInitBob(&pPlayer->pPleps[ubPlep]);
	}
}

tTile playerToTile(const tPlayer *pPlayer) {
	tTile eTile = TILE_BLOB_P1 + playerToIdx(pPlayer) - PLAYER_1;
	return eTile;
}

tPlayer *playerFromTile(tTile eTile) {
	tPlayer *pPlayer = 0;
	if(eTile < TILE_BLOB_NEUTRAL) {
		pPlayer = &s_pPlayers[eTile - TILE_BLOB_P1];
	}
	return pPlayer;
}

tPlayerIdx playerToIdx(const tPlayer *pPlayer) {
	tPlayerIdx eIdx = PLAYER_NONE;
	if(pPlayer) {
		eIdx = pPlayer - &s_pPlayers[0];
	}
	return eIdx;
}

tPlayer *playerFromIdx(tPlayerIdx eIdx) {
	tPlayer *pPlayer = 0;
	if(eIdx != PLAYER_NONE) {
		pPlayer = &s_pPlayers[eIdx];
	}
	return pPlayer;
}

UBYTE playerProcess(void) {
	for(UBYTE i = 0; i < PLAYER_COUNT_MAX; ++i) {
		s_pCursorFields[i].ubCursorCount = 0;
		s_pCursorFields[i].sPosTile.ubX = 0;
		s_pCursorFields[i].sPosTile.ubY = 0;
	}

	UBYTE ubAliveCount = 0;
	for(UBYTE i = 0; i < PLAYER_COUNT_MAX; ++i) {
		tPlayer *pPlayer = &s_pPlayers[i];
		if(pPlayer->isDead) {
			continue;
		}
		++ubAliveCount;

		tDirection eDir = steerProcess(pPlayer->pSteer);
		if(pPlayer->isSelectingDestination) {
			if(eDir == DIRECTION_FIRE) {
				playerSpawnPlep(pPlayer);
				playerSetCursorPos(pPlayer, pPlayer->pNodePlepSrc);
				pPlayer->isSelectingDestination = 0;
			}
			else {
				playerTryMoveSelectionFromInDir(pPlayer, pPlayer->pNodePlepSrc, eDir);
			}
		}
		else {
			if(eDir == DIRECTION_FIRE) {
				if(pPlayer->pNodeCursor->pPlayer == pPlayer) {
					pPlayer->pNodePlepSrc = pPlayer->pNodeCursor;
					pPlayer->isSelectingDestination = 1;
					ptplayerSfxPlay(g_pSfxPlep1, 3, 64, 1);
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

		tCursorField *pField;
		for(
			pField = &s_pCursorFields[0];
			pField < &s_pCursorFields[PLAYER_COUNT_MAX - 1]; ++pField
		) {
			if(pField->sPosTile.uwYX == 0) {
				pField->sPosTile.uwYX = pPlayer->pNodeCursor->sPosTile.uwYX;
				break;
			}
			else if(pField->sPosTile.uwYX == pPlayer->pNodeCursor->sPosTile.uwYX) {
				break;
			}
		}
		pField->pBobs[pField->ubCursorCount++] = pPlayer->pBobCursor;
	}
	return ubAliveCount;
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

void playerAllDead(void) {
	for(UBYTE i = 0; i < PLAYER_COUNT_MAX; ++i) {
		s_pPlayers[i].isDead = 1;
	}
}

void playerPushCursors(void) {
	for(UBYTE i = 0; i < 4; ++i) {
		tCursorField *pField = &s_pCursorFields[i];
		if(pField->ubCursorCount) {
			UBYTE ubTickLimit = 40 / pField->ubCursorCount;
			if(++pField->ubFrameCounter >= ubTickLimit) {
				++pField->ubCurrentCursor;
				pField->ubFrameCounter = 0;
			}
			if(pField->ubCurrentCursor >= pField->ubCursorCount) {
				pField->ubCurrentCursor = 0;
			}
			bobNewPush(pField->pBobs[pField->ubCurrentCursor]);
		}
	}
}
