/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/log.h>
#include "map_data.h"
#include "json/json.h"

static tTile tileFromChar(char c) {
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
		[TILE_PATH_X1] = '+',
	};

	for(tTile eTile = 0; eTile < TILE_COUNT; ++eTile) {
		if(c == pTileToChar[eTile]) {
			return eTile;
		}
	}
	return TILE_COUNT;
}

UBYTE mapDataInitFromFile(tMapData *pMapData, const char *szPath) {
	logBlockBegin("mapDataInitFromFile(szPath: '%s')", szPath);
	tJson *pJson = jsonCreate(szPath);

	UWORD uwTokName = jsonGetDom(pJson, "name");
	UWORD uwTokAuthor = jsonGetDom(pJson, "author");
	UWORD uwTokPlayers = jsonGetDom(pJson, "players");
	UWORD uwTokTiles = jsonGetDom(pJson, "tiles");
	UBYTE isOk = 0;

	if(!uwTokName || !uwTokAuthor || !uwTokPlayers || !uwTokTiles) {
		logWrite("ERR: couldn't find all tokens\n");
		goto end;
	}

	jsonTokStrCpy(pJson, 0, uwTokName, pMapData->szName, MAP_NAME_MAX);
	jsonTokStrCpy(pJson, 0, uwTokAuthor, pMapData->szAuthor, MAP_AUTHOR_MAX);
	logWrite("Name: '%s' by '%s'\n", pMapData->szName, pMapData->szAuthor);

	UBYTE ubCharsInRow = jsonStrLen(pJson, jsonGetElementInArray(pJson, uwTokTiles, 0));
	UBYTE ubPadX = (MAP_SIZE - ubCharsInRow) / 2;
	UBYTE ubRowCount = pJson->pTokens[uwTokTiles].size;
	UBYTE ubPadY = (MAP_SIZE - ubRowCount) / 2;
	logWrite(
		"PadX: %hhu, Pad Y: %hhu, row count: %hhu, line width: %hhu\n",
		ubPadX, ubPadY, ubRowCount, ubCharsInRow
	);

	pMapData->ubPlayerCount = 0;
	for(UBYTE y = 0; y < ubRowCount; ++y) {
		UWORD uwTokRow = jsonGetElementInArray(pJson, uwTokTiles, y);
		const char *pRow = &pJson->szData[pJson->pTokens[uwTokRow].start];

		UBYTE ubCurrentRowLength = jsonStrLen(pJson, uwTokRow);
		if(ubCurrentRowLength != ubCharsInRow) {
			logWrite("ERR: line length mismatch\n");
			goto end;
		}

		for(UBYTE x = 0; x < ubCharsInRow; ++x) {
			tTile eTile = tileFromChar(pRow[x]);
			if(eTile != TILE_COUNT) {
				UBYTE ubMapX = ubPadX + x;
				UBYTE ubMapY = ubPadY + y;
				pMapData->pTiles[ubMapX][ubMapY] = eTile;
			}
			else {
				logWrite("ERR: Can't find tile for '%c' at %hu,%hu\n", pRow[x], x, y);
				goto end;
			}
		}
	}

	mapDataRecalculateStuff(pMapData);
	isOk = 1;
end:
	if(pJson) {
		jsonDestroy(pJson);
	}
	logBlockEnd("mapDataInitFromFile()");
	return isOk;
}

void mapDataClear(tMapData *pMapData) {
	for(UBYTE x = 0; x < MAP_SIZE; ++x) {
		for(UBYTE y = 0; y < MAP_SIZE; ++y) {
			pMapData->pTiles[x][y] = TILE_PAD;
		}
	}
	strcpy(pMapData->szAuthor, "");
	strcpy(pMapData->szName, "");
	pMapData->ubPlayerCount = 0;
}

UBYTE tileIsMovable(tTile eTile) {
	UBYTE isMovable = (eTile >= TILE_PATH_H1);
	return isMovable;
}

UBYTE tileIsNode(tTile eTile) {
	UBYTE isNode = (TILE_BLOB_NEUTRAL <= eTile && eTile <= TILE_BLOB_P4);
	return isNode;
}

void mapDataRecalculateStuff(tMapData *pMapData) {
	for(UBYTE x = 0; x < MAP_SIZE; ++x) {
		for(UBYTE y = 0; y < MAP_SIZE; ++y) {
			tTile eTile = pMapData->pTiles[x][y];
			if(tileIsNode(eTile) && eTile != TILE_BLOB_NEUTRAL) {
				pMapData->ubPlayerCount = MAX(
					pMapData->ubPlayerCount, eTile - TILE_BLOB_P1 + 1
				);
			}
			else if(tileIsMovable(eTile) && (x + y) & 1) {
				++eTile;
			}
			pMapData->pTiles[x][y] = eTile;
		}
	}
}
