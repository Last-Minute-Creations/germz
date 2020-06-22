/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <ace/managers/log.h>
#include <ace/managers/system.h>
#include "map_data.h"
#include "json/json.h"

static const char s_pTileToChar[TILE_COUNT] = {'\0',
	[TILE_BLANK] = '.',
	[TILE_BLOB_NEUTRAL] = 'N',
	[TILE_BLOB_P1] = '1',
	[TILE_BLOB_P2] = '2',
	[TILE_BLOB_P3] = '3',
	[TILE_BLOB_P4] = '4',
	[TILE_PATH_H1...TILE_PATH_H4] = '-',
	[TILE_PATH_V1...TILE_PATH_V4] = '|',
	[TILE_PATH_X1...TILE_PATH_X4] = '+',
};

static tTile tileFromChar(char c) {

	for(tTile eTile = 0; eTile < TILE_COUNT; ++eTile) {
		if(c == s_pTileToChar[eTile]) {
			return eTile;
		}
	}
	return TILE_COUNT;
}

UBYTE mapDataInitFromFile(tMapData *pMapData, const char *szPath) {
	systemUse();
	logBlockBegin("mapDataInitFromFile(szPath: '%s')", szPath);
	mapDataClear(pMapData);
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
	systemUnuse();
	return isOk;
}

void mapDataClear(tMapData *pMapData) {
	for(UBYTE x = 0; x < MAP_SIZE; ++x) {
		for(UBYTE y = 0; y < MAP_SIZE; ++y) {
			pMapData->pTiles[x][y] = TILE_BLANK;
		}
	}
	strcpy(pMapData->szAuthor, "");
	strcpy(pMapData->szName, "");
	pMapData->ubPlayerCount = 0;
}

UBYTE tileIsLink(tTile eTile) {
	UBYTE isMovable = (eTile >= TILE_PATH_H1);
	return isMovable;
}

UBYTE tileIsNode(tTile eTile) {
	UBYTE isNode = (eTile < TILE_BLOB_COUNT);
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
		}
	}
}

UBYTE mapDataSaveToFile(const tMapData *pMapData, const char *szPath) {
	systemUse();
	logBlockBegin("mapDataSaveToFile(pMapData: %p, szPath: '%s')", pMapData, szPath);
	tFile *pFile = fileOpen(szPath, "wb");
	if(!pFile) {
		logWrite("ERR: File doesn't exist\n");
		logBlockEnd("mapDataSaveToFile()");
		systemUnuse();
		return 0;
	}
	fileWriteStr(pFile, "{\n");

	fileWriteStr(pFile, "\t\"name\": \"");
	fileWriteStr(pFile, pMapData->szName);
	fileWriteStr(pFile, "\",\n");

	fileWriteStr(pFile, "\t\"author\": \"");
	fileWriteStr(pFile, pMapData->szAuthor);
	fileWriteStr(pFile, "\",\n");

	char szBfr[2];
	sprintf(szBfr, "%hhu", pMapData->ubPlayerCount);
	fileWriteStr(pFile, "\t\"players\": ");
	fileWriteStr(pFile, szBfr);
	fileWriteStr(pFile, ",\n");

	fileWriteStr(pFile, "\t\"tiles\": [\n");
	for(UBYTE ubY = 0; ubY < MAP_SIZE; ++ubY) {
		fileWriteStr(pFile, "\t\t\"");
		for(UBYTE ubX = 0; ubX < MAP_SIZE; ++ubX) {
			char c = s_pTileToChar[pMapData->pTiles[ubX][ubY]];
			fileWrite(pFile, &c, 1);
		}
		fileWriteStr(pFile, "\",\n");
	}
	fileWriteStr(pFile, "\t]\n}\n");

	fileClose(pFile);
	logBlockEnd("mapDataSaveToFile()");
	systemUnuse();
	return 1;
}

void mapDataRecalculateLinkTileAt(tMapData *pMapData, UBYTE ubX, UBYTE ubY) {
	UBYTE isHorizontal = 0, isVertical = 0;
	tTile eTile;
	if(ubX > 0) {
		eTile = pMapData->pTiles[ubX - 1][ubY];
		if(tileIsLink(eTile) || tileIsNode(eTile)) {
			isHorizontal = 1;
		}
	}
	if(ubX < MAP_SIZE - 1) {
		eTile = pMapData->pTiles[ubX + 1][ubY];
		if(tileIsLink(eTile) || tileIsNode(eTile)) {
			isHorizontal = 1;
		}
	}

	if(ubY > 0) {
		eTile = pMapData->pTiles[ubX][ubY - 1];
		if(tileIsLink(eTile) || tileIsNode(eTile)) {
			isVertical = 1;
		}
	}

	if(ubY < MAP_SIZE - 1) {
		eTile = pMapData->pTiles[ubX][ubY + 1];
		if(tileIsLink(eTile) || tileIsNode(eTile)) {
			isVertical = 1;
		}
	}

	if(isHorizontal && isVertical) {
		eTile = TILE_PATH_X1;
	}
	else if(isHorizontal) {
		eTile = TILE_PATH_H1;
	}
	else { // isVertical
		eTile = TILE_PATH_V1;
	}
	eTile += ((ubX + ubY) & 3);
	pMapData->pTiles[ubX][ubY] = eTile;
}
