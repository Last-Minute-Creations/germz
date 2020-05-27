/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _GERMZ_MAP_H_
#define _GERMZ_MAP_H_

#define MAP_NODES_MAX 255
#define MAP_TILE_SIZE 16

#include "dir.h"
#include "map_data.h"

typedef struct _tNode {
	UBYTE ubIdx;
	UBYTE ubTileX;
	UBYTE ubTileY;
	struct _tPlayer *pPlayer;
	struct _tNode *pNeighbors[DIR_COUNT - 1];
	WORD wCharges;
} tNode;

typedef struct _tMap {
	tNode pNodes[MAP_NODES_MAX];
	tNode *pNodesOnTiles[MAP_SIZE][MAP_SIZE];
	UWORD uwNodeCount;
	tNode *pPlayerStartNodes[4];
	UBYTE ubChargeClock;
} tMap;

void mapInitFromMapData(void);

void mapProcessNodes(void);

void nodeChangeOwnership(tNode *pNode, struct _tPlayer *pPlayer);

void mapUpdateNodeCountForPlayers(void);

extern tMapData g_sMapData;
extern tMap g_sMap;

#endif // _GERMZ_MAP_H_
