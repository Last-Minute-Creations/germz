/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _GERMZ_MAP_H_
#define _GERMZ_MAP_H_

#define MAP_NAME_MAX 20
#define MAP_AUTHOR_MAX 20
#define MAP_SIZE 16
#define NODES_MAX 32

#include <ace/types.h>
#include "player.h"
#include "dir.h"

typedef struct _tNode {
	UBYTE ubTileX;
	UBYTE ubTileY;
	struct _tPlayer *pPlayer;
	struct _tNode *pNeighbors[DIR_COUNT - 1];
	BYTE bCharges;
} tNode;

typedef enum _tTile {
	TILE_PAD,
	TILE_BLANK,
	TILE_BLOB_NEUTRAL,
	TILE_BLOB_P1,
	TILE_BLOB_P2,
	TILE_BLOB_P3,
	TILE_BLOB_P4,
	TILE_BLOB_COUNT,
	TILE_PATH_H1 = TILE_BLOB_COUNT,
	TILE_PATH_H2,
	TILE_PATH_V1,
	TILE_PATH_V2,
	TILE_PATH_X,
	TILE_COUNT
} tTile;

typedef struct _tMap {
	char szName[MAP_NAME_MAX];
	char szAuthor[MAP_AUTHOR_MAX];
	UBYTE ubPlayerCount;
	tTile pTiles[MAP_SIZE][MAP_SIZE];
	tNode pNodes[NODES_MAX];
	tNode *pNodesOnTiles[MAP_SIZE][MAP_SIZE];
	UBYTE ubNodeCount;
	tNode *pPlayerStartNodes[4];
	UBYTE ubChargeClock;
} tMap;

tMap *mapCreateFromFile(const char *szPath);

void mapDestroy(tMap *pMap);

void mapProcessNodes(tMap *pMap);

#endif // _GERMZ_MAP_H_
