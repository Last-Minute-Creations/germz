/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "map.h"
#include <string.h>
#include <ace/managers/memory.h>
#include <ace/managers/log.h>
#include "json/json.h"

static void nodeAdd(tMap *pMap, UBYTE ubTrueX, UBYTE ubTrueY, tTile eTile) {
	if(pMap->ubNodeCount >= NODES_MAX) {
		logWrite("ERR: Can't add another node\n");
		return;
	}
	tNode *pNode = &pMap->pNodes[pMap->ubNodeCount];
	pNode->ubTileX = ubTrueX;
	pNode->ubTileY = ubTrueY;
	for(UBYTE i = 4; i--;) {
		pNode->pNeighbors[i] = 0;
	}
	pNode->pPlayer = playerFromTile(eTile);
	if(pNode->pPlayer) {
		logWrite("player at pos %hhu,%hhu: %p (%d)\n", ubTrueX, ubTrueY, pNode->pPlayer, eTile);
	}
	++pMap->ubNodeCount;
	pMap->pNodesOnTiles[ubTrueX][ubTrueY] = pNode;
	logWrite("Added node %p at %hhu,%hhu\n", pNode, ubTrueX, ubTrueY);
}

static UBYTE tileIsMovable(tTile eTile) {
	UBYTE isMovable = (eTile >= TILE_PATH_H1);
	return isMovable;
}

static UBYTE tileIsNode(tTile eTile) {
	UBYTE isNode = (TILE_BLOB_NEUTRAL <= eTile && eTile <= TILE_BLOB_P4);
	return isNode;
}

static tDir dirReverse(tDir eDir) {
	return (eDir ^ 1);
}

static void nodeFindNeighbor(tMap *pMap, UBYTE ubNodeIdx, tDir eDir) {
	static const tBCoordYX pDeltas[DIR_COUNT] = {
		[DIR_UP]    = {.bX =  0, .bY = -1},
		[DIR_DOWN]  = {.bX =  0, .bY =  1},
		[DIR_LEFT]  = {.bX = -1, .bY =  0},
		[DIR_RIGHT] = {.bX =  1, .bY =  0},
	};

	// Find next blob in line or give up
	tNode *pNodeSrc = &pMap->pNodes[ubNodeIdx];
	BYTE x = pNodeSrc->ubTileX;
	BYTE y = pNodeSrc->ubTileY;
	logWrite("Starting at %hhd,%hhd\n", x, y);
	do {
		x += pDeltas[eDir].bX;
		y += pDeltas[eDir].bY;
		if(x < 0 || MAP_SIZE < x || y < 0 || MAP_SIZE < y) {
			logWrite("No neighbor on dir %d\n", eDir);
			return;
		}
	} while(tileIsMovable(pMap->pTiles[x][y]));

	if(tileIsNode(pMap->pTiles[x][y])) {
		// Mark given node as neigbor
		tNode *pNeighbor = pMap->pNodesOnTiles[x][y];
		pNodeSrc->pNeighbors[eDir] = pNeighbor;
		pNeighbor->pNeighbors[dirReverse(eDir)] = pNodeSrc;
		logWrite("Connected to %p in dir %d\n", pNeighbor, eDir);
	}
}

static void nodeCalculateNeighbors(tMap *pMap) {
	for(UBYTE i = 0; i < pMap->ubNodeCount; ++i) {
		logWrite("Searching for neighbors for node %p\n", &pMap->pNodes[i]);
		for(tDir eDir = 0; eDir < DIR_COUNT; ++eDir) {
			if(!pMap->pNodes[i].pNeighbors[eDir]) {
				nodeFindNeighbor(pMap, i, eDir);
			}
		}
	}
}

tMap *mapCreateFromFile(const char *szPath) {
	tJson *pJson = jsonCreate(szPath);

	UWORD uwTokName = jsonGetDom(pJson, "name");
	UWORD uwTokAuthor = jsonGetDom(pJson, "author");
	UWORD uwTokPlayers = jsonGetDom(pJson, "players");
	UWORD uwTokTiles = jsonGetDom(pJson, "tiles");

	if(!uwTokName || !uwTokAuthor || !uwTokPlayers || !uwTokTiles) {
		logWrite("ERR: couldn't find all tokens\n");
		jsonDestroy(pJson);
		return 0;
	}

	tMap *pMap = memAllocFastClear(sizeof(*pMap));
	jsonTokStrCpy(pJson, 0, uwTokName, pMap->szName, MAP_NAME_MAX);
	jsonTokStrCpy(pJson, 0, uwTokAuthor, pMap->szAuthor, MAP_AUTHOR_MAX);
	pMap->ubPlayerCount = jsonTokToUlong(pJson, uwTokPlayers);
	logWrite(
		"Name: '%s' by '%s', %hhu players\n",
		pMap->szName, pMap->szAuthor, pMap->ubPlayerCount
	);

	UBYTE ubCharsInRow = jsonStrLen(pJson, jsonGetElementInArray(pJson, uwTokTiles, 0));
	UBYTE ubPadX = (MAP_SIZE - ubCharsInRow) / 2;
	UBYTE ubRowCount = pJson->pTokens[uwTokTiles].size;
	UBYTE ubPadY = (MAP_SIZE - ubRowCount) / 2;
	logWrite(
		"PadX: %hhu, Pad Y: %hhu, row count: %hhu, line width: %hhu\n",
		ubPadX, ubPadY, ubRowCount, ubCharsInRow
	);

	static const char pTileToChar[TILE_COUNT] = {'\0',
		[TILE_PAD] = ' ',
		[TILE_BLANK] = '.',
		[TILE_BLOB_NEUTRAL] = 'N',
		[TILE_BLOB_P1] = '1',
		[TILE_BLOB_P2] = '2',
		[TILE_BLOB_P3] = '3',
		[TILE_BLOB_P4] = '4',
		[TILE_PATH_H1] = '-',
		[TILE_PATH_V1] = '|',
		[TILE_PATH_X] = '+',
	};

	pMap->ubNodeCount = 0;

	for(UBYTE y = 0; y < ubRowCount; ++y) {
		UWORD uwTokRow = jsonGetElementInArray(pJson, uwTokTiles, y);
		const char *pRow = &pJson->szData[pJson->pTokens[uwTokRow].start];

		UBYTE ubCurrentRowLength = jsonStrLen(pJson, uwTokRow);
		if(ubCurrentRowLength != ubCharsInRow) {
			logWrite("ERR: line length mismatch\n");
			memFree(pMap, sizeof(*pMap));
			jsonDestroy(pJson);
			return 0;
		}

		for(UBYTE x = 0; x < ubCharsInRow; ++x) {
			UBYTE isFound = 0;
			for(tTile eTile = 0; eTile < TILE_COUNT; ++eTile) {
				if(pRow[x] == pTileToChar[eTile]) {
					UBYTE ubTrueX = ubPadX + x;
					UBYTE ubTrueY = ubPadY + y;
					if(
						(eTile == TILE_PATH_H1 || eTile == TILE_PATH_V1) &&
						(ubTrueX + ubTrueY) & 1
					) {
						++eTile;
					}
					else if(TILE_BLOB_NEUTRAL <= eTile && eTile <= TILE_BLOB_P4) {
						nodeAdd(pMap, ubTrueX, ubTrueY, eTile);
					}
					isFound = 1;
					pMap->pTiles[ubTrueX][ubTrueY] = eTile;
					break;
				}
			}
			if(!isFound) {
				logWrite("ERR: Can't find tile for '%c' at %hu,%hu\n", pRow[x], x, y);
			}
		}
	}

	nodeCalculateNeighbors(pMap);
	jsonDestroy(pJson);
	return pMap;
}

void mapDestroy(tMap *pMap) {
	memFree(pMap, sizeof(*pMap));
}
