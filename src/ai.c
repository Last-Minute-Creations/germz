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
		// logWrite("\nReplan to def\n");
	}
	else {
		pAi->eState = AI_STATE_PLANNING_AGGRESIVE;
		// logWrite("\nReplan to aggro\n");
	}
	pAi->uwCurrNode = 0;
	pAi->wBiggestDelta = -32768;
	pAi->pTargetSrc = 0;
	pAi->pTarget = 0;
	pAi->eNeighborIdx = 0;
	pAi->wasLastAggresive = !pAi->wasLastAggresive;
	pAi->pTargetCurrNode = pAi->pTarget;
	pAi->wBiggestDeltaCurrNode = pAi->wBiggestDelta;
}

static tDirection aiProcessPlanningAggressive(tAi *pAi) {
	const tPlayer *pPlayer = playerFromIdx(pAi->ubPlayerIdx);
	// Get next owned blob
	const tNode *pNode = &s_pMap->pNodes[pAi->uwCurrNode];
	UBYTE isNextNode = 0;

	if(pNode->pPlayer == pPlayer && pNode->wCharges > 10) {
		// Check directions for enemy neighbors
		if(pAi->eNeighborIdx < 4) {
			// Store the weakest enemy
			if(
				pNode->pNeighbors[pAi->eNeighborIdx] &&
				pNode->pNeighbors[pAi->eNeighborIdx]->pPlayer != pPlayer
			) {
				WORD wDelta = pNode->wCharges - pNode->pNeighbors[pAi->eNeighborIdx]->wCharges;
				// logWrite(
				// 	"wDelta: %hd - %hd = %hd\n",
				// 	pNode->wCharges, pNode->pNeighbors[pAi->eNeighborIdx]->wCharges, wDelta
				// );
				if(wDelta > pAi->wBiggestDelta) {
					pAi->pTargetSrc = pNode;
					pAi->pTarget = pNode->pNeighbors[pAi->eNeighborIdx];
					pAi->wBiggestDelta = wDelta;
					// logWrite("aggro: stored next potential target: %p\n", pAi->pTarget);
				}
			}
			++pAi->eNeighborIdx;
		}
		else {
			// Out of directions - next node
			isNextNode = 1;
			// logWrite("aggro: out of directions\n");
		}
	}
	else {
		isNextNode = 1;
	}

	if(isNextNode) {
		if(++pAi->uwCurrNode >= s_pMap->uwNodeCount) {
			// Out of nodes - execute attack or plan defensive move
			if(pAi->pTarget) {
				// logWrite(
				// 	"aggro: cursor %p, getting to %p to fire into %p (%hhu,%hhu)\n",
				// 	pPlayer->pNodeCursor, pAi->pTargetSrc, pAi->pTarget,
				// 	pAi->pTarget->ubTileX, pAi->pTarget->ubTileY
				// );
				pAi->eState = AI_STATE_ROUTING;
			}
			else {
				aiTransitToPlanning(pAi);
			}
		}
	}
	return DIRECTION_COUNT;
}

static tDirection aiProcessPlanningDefensive(tAi *pAi) {
	const tPlayer *pPlayer = playerFromIdx(pAi->ubPlayerIdx);
	// Get next owned blob
	const tNode *pNode = &s_pMap->pNodes[pAi->uwCurrNode];
	UBYTE isNextNode = 0;

	if(pNode->pPlayer == pPlayer && pNode->wCharges > 10) {
		// Check directions for allied neighbors
		if(pAi->eNeighborIdx < 4) {
			// Store the weakest ally
			if(pNode->pNeighbors[pAi->eNeighborIdx]) {
				if(pNode->pNeighbors[pAi->eNeighborIdx]->pPlayer != pPlayer) {
					// Colliding with enemy - skip node entirely
					// logWrite(
					// 	"def: skipping node %p (%hhu,%hhu) since it's in border\n",
					// 	pNode, pNode->ubTileX, pNode->ubTileY
					// );
					isNextNode = 1;
				}
				else if(
					pNode->pNeighbors[pAi->eNeighborIdx]->pPlayer == pPlayer
				) {
					WORD wDelta = pNode->wCharges - pNode->pNeighbors[pAi->eNeighborIdx]->wCharges;
					// logWrite(
					// 	"wDelta: %hd - %hd = %hd\n",
					// 	pNode->wCharges, pNode->pNeighbors[pAi->eNeighborIdx]->wCharges, wDelta
					// );
					if(wDelta > pAi->wBiggestDeltaCurrNode) {
						// Store target/delta for current node in case it gets discarded
						// because of being on border
						pAi->pTargetCurrNode = pNode->pNeighbors[pAi->eNeighborIdx];
						pAi->wBiggestDeltaCurrNode = wDelta;
						// logWrite("def: stored next potential target for current node %p: %p\n", pNode, pAi->pTargetCurrNode);
					}
				}
			}
			++pAi->eNeighborIdx;
		}
		else {
			// Out of directions - next node, but store target if there is new
			if(pAi->pTarget != pAi->pTargetCurrNode) {
				pAi->pTarget = pAi->pTargetCurrNode;
				pAi->pTargetSrc = pNode;
				pAi->wBiggestDelta = pAi->wBiggestDeltaCurrNode;
				// logWrite("def: stored global potential target: %p\n", pAi->pTarget);
			}
			isNextNode = 1;
		}
	}
	else {
		isNextNode = 1;
	}

	if(isNextNode) {
		pAi->pTargetCurrNode = pAi->pTarget;
		pAi->wBiggestDeltaCurrNode = pAi->wBiggestDelta;
		if(++pAi->uwCurrNode >= s_pMap->uwNodeCount) {
			// Out of nodes - execute defense or plan aggresive move
			if(pAi->pTarget) {
				// logWrite(
				// 	"def: cursor %p, getting to %p (%hhu,%hhu) to fire into %p (%hhu,%hhu)\n",
				// 	pPlayer->pNodeCursor,
				// 	pAi->pTargetSrc, pAi->pTargetSrc->ubTileX, pAi->pTargetSrc->ubTileY,
				// 	pAi->pTarget, pAi->pTarget->ubTileX, pAi->pTarget->ubTileY
				// );
				pAi->eState = AI_STATE_ROUTING;
			}
			else {
				aiTransitToPlanning(pAi);
			}
		}
	}
	return DIRECTION_COUNT;
}

static tDirection aiProcessRouting(tAi *pAi) {
	if(pAi->isAstarStarted) {
		if(astarProcess(s_pAstar[pAi->ubPlayerIdx])) {
			// logWrite(
			// 	"Found route with %hhu steps\n",
			// 	s_pAstar[pAi->ubPlayerIdx]->sRoute.bNodeCount
			// );
			// for(UBYTE i = s_pAstar[pAi->ubPlayerIdx]->sRoute.bNodeCount; i--;) {
			// 	logWrite(
			// 		"-> %hhu@%p (%hhu,%hhu)", i, s_pAstar[pAi->ubPlayerIdx]->sRoute.pNodes[i],
			// 		s_pAstar[pAi->ubPlayerIdx]->sRoute.pNodes[i]->ubTileX,
			// 		s_pAstar[pAi->ubPlayerIdx]->sRoute.pNodes[i]->ubTileY
			// 	);
			// }
			// logWrite("\n");
			pAi->eState = AI_STATE_GETTING_TO_SOURCE_BLOB;
			pAi->isAstarStarted = 0;
		}
	}
	else {
		const tPlayer *pPlayer = playerFromIdx(pAi->ubPlayerIdx);
		astarStart(s_pAstar[pAi->ubPlayerIdx], pPlayer->pNodeCursor, pAi->pTargetSrc);
		pAi->isAstarStarted = 1;
	}
	return DIRECTION_COUNT;
}

static tDirection aiProcessGettingToSourceBlob(tAi *pAi) {
	const tPlayer *pPlayer = playerFromIdx(pAi->ubPlayerIdx);
	tAstarData *pNav = s_pAstar[pAi->ubPlayerIdx];

	if(pNav->sRoute.bCurrNode < 0) {
		// End of the road
		if(pPlayer->pNodeCursor == pAi->pTargetSrc) {
			// We're at last on source
			if(pAi->pTargetSrc->pPlayer == pPlayer) {
				// It's still ours so let's continue
				// logWrite("We're at source, fire!\n");
				pAi->eNeighborIdx = 0;
				pAi->eState = AI_STATE_TARGETING_TARGET;
				return DIRECTION_FIRE;
			}
			else {
				// It's no longer ours - start again
				aiTransitToPlanning(pAi);
			}
		}
		else {
			logWrite("ERR: End of road and no destination! WTF\n");
			aiTransitToPlanning(pAi);
		}
	}
	else {
		const tNode *pNode = pNav->sRoute.pNodes[pNav->sRoute.bCurrNode];
		// logWrite(
		// 	"We're at %p: %hhu,%hhu, next route node: %p: %hhu,%hhu\n",
		// 	pPlayer->pNodeCursor, pPlayer->pNodeCursor->ubTileX, pPlayer->pNodeCursor->ubTileY,
		// 	pNode, pNode->ubTileX, pNode->ubTileY
		// );
		if(pNode == pPlayer->pNodeCursor) {
			// We're here, skip
			--pNav->sRoute.bCurrNode;
			// logWrite("Skipping movement\n");
		}
		else {
			for(tDirection i = 0; i < 4; ++i) {
				if(pPlayer->pNodeCursor->pNeighbors[i] == pNode) {
					--pNav->sRoute.bCurrNode;
					// logWrite("Moving in dir %d\n", i);
					return i;
				}
			}
			logWrite("ERR: Couldn't navigate from node %p to %p\n", pPlayer->pNodeCursor, pNode);
			aiTransitToPlanning(pAi);
			// Go to source node by pushing buttons
		}
	}
	return DIRECTION_COUNT;
}

static tDirection aiProcessTargetingTarget(tAi *pAi) {
	// we've pressed fire button on source and now we'll attack weakest neighbor
	if(pAi->pTargetSrc->pNeighbors[pAi->eNeighborIdx] == pAi->pTarget) {
		// That's our neighbor
		// logWrite("Confirming target!\n");
		pAi->eState = AI_STATE_CONFIRMING_TARGET;
		return pAi->eNeighborIdx;
	}
	else {
		if(++pAi->eNeighborIdx >= 4) {
			logWrite(
				"ERR: AI lost target node! While checking if target src %p (%hhu,%hhu) neighbors with target %p (%hhu,%hhu)\n",
				pAi->pTargetSrc, pAi->pTargetSrc->ubTileX, pAi->pTargetSrc->ubTileY,
				pAi->pTarget, pAi->pTarget->ubTileX, pAi->pTarget->ubTileY
			);
			logPushIndent();
			for(UBYTE i = 0; i < 4; ++i) {
				logWrite("Neighbor at dir %hhu: %p\n", i, pAi->pTargetSrc->pNeighbors[i]);
			}
			logPopIndent();
			aiTransitToPlanning(pAi);
		}
	}
	return DIRECTION_COUNT;
}

static tDirection aiProcessConfirmingTarget(tAi *pAi) {
	// FIRE!
	aiTransitToPlanning(pAi);
	return DIRECTION_FIRE;
}

tDirection aiProcess(tAi *pAi) {
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
	return DIRECTION_COUNT;
}

void aiCreate(const tMap *pMap) {
	for(UBYTE i = 0; i < 4; ++i) {
		s_pAstar[i] = astarCreate();
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
