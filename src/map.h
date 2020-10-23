/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _GERMZ_MAP_H_
#define _GERMZ_MAP_H_

#include "direction.h"
#include "map_data.h"

#define MAP_NODES_MAX 255
#define MAP_TILE_SIZE 16
// #define MAP_TICK_RATE 25
#define MAP_TICK_RATE 5

typedef enum _tNodeType {
	NODE_TYPE_NORMAL,
	NODE_TYPE_SUPER,
} tNodeType;

typedef struct _tNode {
	UBYTE ubIdx;
	tUbCoordYX sPosTile;
	struct _tPlayer *pPlayer;
	struct _tNode *pNeighbors[DIRECTION_COUNT - 1];
	WORD wCharges;
	WORD wCapacity; ///< Max amount of charges.
	UBYTE ubChargeClock;
	UBYTE ubChargeRate; ///< How many ticks before gaining next charge, 1 unit is MAP_TICK_RATE frames.
	tNodeType eType;
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

tNodeType nodeTypeFromTile(tTile eTile);

tTile nodeToTile(const tNode *pNode);

extern tMapData g_sMapData;
extern tMap g_sMap;

#endif // _GERMZ_MAP_H_
