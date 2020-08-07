/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "map.h"
#include <string.h>
#include <ace/managers/memory.h>
#include <ace/managers/log.h>
#include "json/json.h"
#include "player.h"
#include "blob_anim.h"

static tNode *nodeAdd(UBYTE ubX, UBYTE ubY, tTile eTile) {
	if(g_sMap.uwNodeCount >= MAP_NODES_MAX) {
		logWrite("ERR: Can't add another node\n");
		return 0;
	}
	tNode *pNode = &g_sMap.pNodes[g_sMap.uwNodeCount];
	pNode->sPosTile.ubX = ubX;
	pNode->sPosTile.ubY = ubY;
	for(UBYTE i = 4; i--;) {
		pNode->pNeighbors[i] = 0;
	}
	pNode->pPlayer = playerFromTile(eTile);
	if(pNode->pPlayer) {
		pNode->wCharges = 60;
		logWrite("player at pos %hhu,%hhu: %p (%d)\n", ubX, ubY, pNode->pPlayer, eTile);
	}
	else {
		pNode->wCharges = 20;
	}
	pNode->ubIdx = g_sMap.uwNodeCount;
	++g_sMap.uwNodeCount;
	g_sMap.pNodesOnTiles[ubX][ubY] = pNode;
	logWrite("Added node %p (%hhu) at %hhu,%hhu\n", pNode, pNode->ubIdx, ubX, ubY);
	return pNode;
}

static tDirection dirReverse(tDirection eDir) {
	return (eDir ^ 1);
}

static void nodeFindNeighbor(tNode *pNode, tDirection eDir) {
	static const tBCoordYX pDeltas[DIRECTION_COUNT] = {
		[DIRECTION_UP]    = {.bX =  0, .bY = -1},
		[DIRECTION_DOWN]  = {.bX =  0, .bY =  1},
		[DIRECTION_LEFT]  = {.bX = -1, .bY =  0},
		[DIRECTION_RIGHT] = {.bX =  1, .bY =  0},
		[DIRECTION_FIRE]  = {.bX =  0, .bY =  0}, // SHOULDN'T HAPPEN!
	};

	// Find next blob in line or give up
	BYTE x = pNode->sPosTile.ubX;
	BYTE y = pNode->sPosTile.ubY;
	do {
		x += pDeltas[eDir].bX;
		y += pDeltas[eDir].bY;
		if(x < 0 || MAP_SIZE < x || y < 0 || MAP_SIZE < y) {
			// logWrite("No more tiles on dir %d\n", eDir);
			return;
		}
	} while(tileIsLink(g_sMapData.pTiles[x][y]));

	if(tileIsNode(g_sMapData.pTiles[x][y])) {
		// Mark given node as neigbor
		tNode *pNeighbor = g_sMap.pNodesOnTiles[x][y];
		pNode->pNeighbors[eDir] = pNeighbor;
		pNeighbor->pNeighbors[dirReverse(eDir)] = pNode;
		logWrite("Connected to %p in dir %d\n", pNeighbor, eDir);
	}
}

static void nodeCalculateNeighbors(void) {
	logBlockBegin("nodeCalculateNeighbors()");
	for(UBYTE i = 0; i < g_sMap.uwNodeCount; ++i) {
		logWrite(
			"Searching for neighbors for node %p (%hhu,%hhu)\n",
			&g_sMap.pNodes[i], g_sMap.pNodes[i].sPosTile.ubX, g_sMap.pNodes[i].sPosTile.ubY
		);
		for(tDirection eDir = 0; eDir < DIRECTION_FIRE; ++eDir) {
			if(!g_sMap.pNodes[i].pNeighbors[eDir]) {
				nodeFindNeighbor(&g_sMap.pNodes[i], eDir);
			}
		}
	}
	logBlockEnd("nodeCalculateNeighbors()");
}

void mapInitFromMapData(void) {
	logBlockBegin("mapInitFromMapData()");
	g_sMap.uwNodeCount = 0;
	for(UBYTE x = 0; x < MAP_SIZE; ++x) {
		for(UBYTE y = 0; y < MAP_SIZE; ++y) {
			g_sMap.pNodesOnTiles[x][y] = 0;
			tTile eTile = g_sMapData.pTiles[x][y];
			if(tileIsNode(eTile)) {
				tNode *pNode = nodeAdd(x, y, eTile);
				tPlayerIdx ePlayerIdx = playerToIdx(playerFromTile(eTile));
				if(ePlayerIdx != PLAYER_NONE) {
					g_sMap.pPlayerStartNodes[ePlayerIdx] = pNode;
					logWrite("Player %d start: %hhu,%hhu\n", ePlayerIdx, pNode->sPosTile.ubX, pNode->sPosTile.ubY);
				}
			}
		}
	}

	g_sMap.ubChargeClock = 0;
	nodeCalculateNeighbors();
	logBlockEnd("mapInitFromMapData()");
}

void mapProcessNodes(void) {
	// TODO: nodes could save some sort of "timestamps" for growth start
	// and deltas from them could be calculated when needed
	// provided that the growth time is constant for each blob
	++g_sMap.ubChargeClock;
	if(
		g_sMap.ubChargeClock == 50 || g_sMap.ubChargeClock == 100 ||
		g_sMap.ubChargeClock == 150
	) {
		UBYTE isNeutralCharge = 0;
		if(g_sMap.ubChargeClock >= 150) {
			g_sMap.ubChargeClock = 0;
			isNeutralCharge = 1;
		}
		for(UBYTE i = 0; i < g_sMap.uwNodeCount; ++i) {
			if(
				g_sMap.pNodes[i].wCharges < 100 &&
				(g_sMap.pNodes[i].pPlayer || isNeutralCharge)
			) {
				++g_sMap.pNodes[i].wCharges;
			}
			else if(isNeutralCharge && g_sMap.pNodes[i].wCharges > 100) {
				--g_sMap.pNodes[i].wCharges;
			}
		}
	}
}

void nodeChangeOwnership(tNode *pNode, tPlayer *pPlayer) {
	if(pNode->pPlayer) {
		--pNode->pPlayer->bNodeCount;
		playerUpdateDead(pNode->pPlayer);
	}
	pNode->pPlayer = pPlayer;
	if(pPlayer) {
		++pPlayer->bNodeCount;
	}
	blobAnimAddToQueue(pNode);
}

void mapUpdateNodeCountForPlayers(void) {
	for(UBYTE i = 0; i < g_sMap.uwNodeCount; ++i) {
		if(g_sMap.pNodes[i].pPlayer) {
			++g_sMap.pNodes[i].pPlayer->bNodeCount;
		}
	}
}

tMapData g_sMapData;
tMap g_sMap;
