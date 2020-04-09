/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ai.h"
#include "astar.h"
#include "player.h"

static const tMap *s_pMap;
static tAstarData *s_pAstar[4];

static void aiTransitToPlanning(tAi *pAi) {
	if(pAi->wasLastAggresive) {
		pAi->eState = AI_STATE_PLANNING_DEFENSIVE;
		logWrite("Replan to def\n");
	}
	else {
		pAi->eState = AI_STATE_PLANNING_AGGRESIVE;
		logWrite("Replan to aggro\n");
	}
	pAi->ubCurrNode = 0;
	pAi->wWeakestCharge = 0x7FFF;
	pAi->pTargetSrc = 0;
	pAi->pTarget = 0;
	pAi->eNeighborIdx = 0;
	pAi->wasLastAggresive = !pAi->wasLastAggresive;
}

static tDir aiProcessPlanningAggressive(tAi *pAi) {
	const tPlayer *pPlayer = playerFromIdx(pAi->ubPlayerIdx + 1);
	// Get next owned blob
	const tNode *pNode = &s_pMap->pNodes[pAi->ubCurrNode];
	if(pNode->pPlayer == pPlayer) {
		// Check directions for enemy neighbors
		if(pAi->eNeighborIdx < 4) {
			// Store the weakest enemy
			if(
				pNode->pNeighbors[pAi->eNeighborIdx]->pPlayer != pPlayer &&
				pNode->pNeighbors[pAi->eNeighborIdx]->wCharges < pAi->wWeakestCharge
			) {
				pAi->pTargetSrc = pNode;
				pAi->pTarget = pNode->pNeighbors[pAi->eNeighborIdx];
				pAi->wWeakestCharge = pAi->pTarget->wCharges;
			}
			++pAi->eNeighborIdx;
		}
		else {
			// Out of directions - next node
			if(++pAi->ubCurrNode >= s_pMap->ubNodeCount) {
				// Out of nodes - execute attack or plan defensive move
				if(pAi->pTarget) {
					logWrite("aggro: getting from %p to %p\n", pAi->pTargetSrc, pAi->pTarget);
					pAi->eState = AI_STATE_ROUTING;
				}
				else {
					aiTransitToPlanning(pAi);
				}
			}
		}
	}
	else {
		if(++pAi->ubCurrNode >= s_pMap->ubNodeCount) {
			// No player nodes - am I dead?
			aiTransitToPlanning(pAi);
		}
	}
	return DIR_COUNT;
}

static tDir aiProcessPlanningDefensive(tAi *pAi) {
	const tPlayer *pPlayer = playerFromIdx(pAi->ubPlayerIdx + 1);
	// Get next owned blob
	const tNode *pNode = &s_pMap->pNodes[pAi->ubCurrNode];
	if(pNode->pPlayer == pPlayer) {
		// Check directions for allied neighbors
		if(pAi->eNeighborIdx < 4) {
			// Store the weakest ally
			if(
				pNode->pNeighbors[pAi->eNeighborIdx]->pPlayer == pPlayer &&
				pNode->pNeighbors[pAi->eNeighborIdx]->wCharges < pAi->wWeakestCharge
			) {
				pAi->pTargetSrc = pNode;
				pAi->pTarget = pNode->pNeighbors[pAi->eNeighborIdx];
				pAi->wWeakestCharge = pAi->pTarget->wCharges;
			}
			++pAi->eNeighborIdx;
		}
		else {
			// Out of directions - next node
			if(++pAi->ubCurrNode >= s_pMap->ubNodeCount) {
				// Out of nodes - execute defense or plan aggresive move
				if(pAi->pTarget) {
					logWrite("def: getting from %p to %p\n", pAi->pTargetSrc, pAi->pTarget);
					pAi->eState = AI_STATE_ROUTING;
				}
				else {
					aiTransitToPlanning(pAi);
				}
			}
		}
	}
	else {
		if(++pAi->ubCurrNode >= s_pMap->ubNodeCount) {
			// No player nodes - am I dead?
			aiTransitToPlanning(pAi);
		}
	}
	return DIR_COUNT;
}

static tDir aiProcessRouting(tAi *pAi) {
	if(pAi->isAstarStarted) {
		if(astarProcess(s_pAstar[pAi->ubPlayerIdx])) {
			logWrite(
				"Found route with %hhu steps\n",
				s_pAstar[pAi->ubPlayerIdx]->sRoute.ubNodeCount
			);
			logWrite(
				"%p (%hhu,%hhu) -> ",
				pAi->pTargetSrc, pAi->pTargetSrc->ubTileX, pAi->pTargetSrc->ubTileY
			);
			for(UBYTE i = s_pAstar[pAi->ubPlayerIdx]->sRoute.ubNodeCount; i--;) {
				logWrite(
					"%p (%hhu,%hhu) -> ", s_pAstar[pAi->ubPlayerIdx]->sRoute.pNodes[i],
					s_pAstar[pAi->ubPlayerIdx]->sRoute.pNodes[i]->ubTileX,
					s_pAstar[pAi->ubPlayerIdx]->sRoute.pNodes[i]->ubTileY
				);
			}
			logWrite(
				"%p (%hhu,%hhu)\n",
				pAi->pTarget, pAi->pTarget->ubTileX, pAi->pTarget->ubTileY
			);
			pAi->eState = AI_STATE_GETTING_TO_SOURCE_BLOB;
			pAi->isAstarStarted = 0;
		}
	}
	else {
		const tPlayer *pPlayer = playerFromIdx(pAi->ubPlayerIdx + 1);
		astarStart(s_pAstar[pAi->ubPlayerIdx], pPlayer->pNodeCursor, pAi->pTargetSrc);
		pAi->isAstarStarted = 1;
	}
	return DIR_COUNT;
}

static tDir aiProcessGettingToSourceBlob(tAi *pAi) {
	const tPlayer *pPlayer = playerFromIdx(pAi->ubPlayerIdx + 1);
	tAstarData *pNav = s_pAstar[pAi->ubPlayerIdx];
	const tNode *pNode = pNav->sRoute.pNodes[pNav->sRoute.ubCurrNode];
	logWrite("Node %p: %hhu,%hhu\n", pNode, pNode->ubTileX, pNode->ubTileY);
	if(pNode == pAi->pTargetSrc) {
		// We're at last on source
		if(pAi->pTargetSrc->pPlayer == pPlayer) {
			// It's still ours so let's continue
			logWrite("We're at source, fire!\n");
			pAi->eNeighborIdx = 0;
			pAi->eState = AI_STATE_TARGETING_TARGET;
			return DIR_FIRE;
		}
		else {
			// It's no longer ours - start again
			aiTransitToPlanning(pAi);
		}
	}
	else if(pNav->sRoute.ubCurrNode == 0) {
		logWrite("ERR: End of road and no destination! WTF\n");
		aiTransitToPlanning(pAi);
	}
	else {
		for(tDir i = 0; i < 4; ++i) {
			if(pPlayer->pNodeCursor->pNeighbors[i] == pNode) {
				--pNav->sRoute.ubCurrNode;
				return i;
			}
		}
		logWrite("ERR: Couldn't navigate from node %p to %p\n", pPlayer->pNodeCursor, pNode);
		aiTransitToPlanning(pAi);
		// Go to source node by pushing buttons
	}
	return DIR_COUNT;
}

static tDir aiProcessTargetingTarget(tAi *pAi) {
	// we've pressed fire button on source and now we'll attack weakest neighbor
	logWrite(
		"Checking if neighbor %p of node %p at dir %d is node %p\n",
		pAi->pTargetSrc->pNeighbors[pAi->eNeighborIdx], pAi->pTargetSrc,
		pAi->eNeighborIdx, pAi->pTarget
	);
	if(pAi->pTargetSrc->pNeighbors[pAi->eNeighborIdx] == pAi->pTarget) {
		// That's our neighbor
		logWrite("Confirming target!\n");
		pAi->eState = AI_STATE_CONFIRMING_TARGET;
		return pAi->eNeighborIdx;
	}
	else {
		if(++pAi->eNeighborIdx >= 3) {
			logWrite("AI lost target node!\n");
			aiTransitToPlanning(pAi);
		}
	}
	return DIR_COUNT;
}

static tDir aiProcessConfirmingTarget(tAi *pAi) {
	// FIRE!
	aiTransitToPlanning(pAi);
	return DIR_FIRE;
}

tDir aiProcess(tAi *pAi) {
	switch(pAi->eState) {
		case AI_STATE_PLANNING_AGGRESIVE:
			return aiProcessPlanningAggressive(pAi);
		case AI_STATE_PLANNING_DEFENSIVE:
			return aiProcessPlanningDefensive(pAi);
		case AI_STATE_ROUTING:
			return aiProcessRouting(pAi);
		case AI_STATE_GETTING_TO_SOURCE_BLOB:
			return aiProcessGettingToSourceBlob(pAi);
		case AI_STATE_TARGETING_TARGET:
			return aiProcessTargetingTarget(pAi);
		case AI_STATE_CONFIRMING_TARGET:
			return aiProcessConfirmingTarget(pAi);
	}
	return DIR_COUNT;
}

void aiCreate(const tMap *pMap) {
	for(UBYTE i = 0; i < 4; ++i) {
		s_pAstar[i] = astarCreate(pMap);
	}
	s_pMap = pMap;
}

void aiDestroy(void) {
	for(UBYTE i = 0; i < 4; ++i) {
		astarDestroy(s_pAstar[i]);
	}
}

void aiReset(tAi *pAi) {
	logWrite("AI Reset for player %hhu\n", pAi->ubPlayerIdx);
	pAi->wasLastAggresive = 0;
	pAi->eState = AI_STATE_PLANNING_DEFENSIVE;
	aiTransitToPlanning(pAi);
}
