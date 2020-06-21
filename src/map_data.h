/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _GERMZ_MAP_DATA_H_
#define _GERMZ_MAP_DATA_H_

#include <ace/types.h>

#define MAP_NAME_MAX 20
#define MAP_AUTHOR_MAX 20
#define MAP_SIZE 16

typedef enum _tTile {
	TILE_BLOB_P1,
	TILE_BLOB_P2,
	TILE_BLOB_P3,
	TILE_BLOB_P4,
	TILE_BLOB_NEUTRAL,
	TILE_BLOB_COUNT,
	TILE_BLANK = TILE_BLOB_COUNT,
	TILE_EDITOR_BLANK,
	TILE_PATH_H1,
	TILE_PATH_H2,
	TILE_PATH_H3,
	TILE_PATH_H4,
	TILE_PATH_V1,
	TILE_PATH_V2,
	TILE_PATH_V3,
	TILE_PATH_V4,
	TILE_PATH_X1,
	TILE_PATH_X2,
	TILE_PATH_X3,
	TILE_PATH_X4,
	TILE_COUNT
} tTile;

typedef struct _tMapData {
	char szName[MAP_NAME_MAX];
	char szAuthor[MAP_AUTHOR_MAX];
	UBYTE ubPlayerCount;
	tTile pTiles[MAP_SIZE][MAP_SIZE];
} tMapData;

UBYTE mapDataInitFromFile(tMapData *pMapData, const char *szPath);

UBYTE mapDataSaveToFile(const tMapData *pMapData, const char *szPath);

void mapDataClear(tMapData *pMapData);

UBYTE tileIsLink(tTile eTile);

UBYTE tileIsNode(tTile eTile);

void mapDataRecalculateStuff(tMapData *pMapData);

void mapDataRecalculateLinkTileAt(tMapData *pMapData, UBYTE ubX, UBYTE ubY);

#endif // _GERMZ_MAP_DATA_H_
