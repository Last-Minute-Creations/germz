/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _GERMZ_AI_H_
#define _GERMZ_AI_H_

#include <ace/types.h>
#include "direction.h"
#include "map.h"

typedef enum _tAiState {
	AI_STATE_PLANNING_AGGRESIVE,
	AI_STATE_PLANNING_DEFENSIVE,
	AI_STATE_ROUTING,
	AI_STATE_GETTING_TO_SOURCE_BLOB,
	AI_STATE_TARGETING_TARGET,
	AI_STATE_CONFIRMING_TARGET,
} tAiState;

typedef struct _tAi {
	UBYTE ubPlayerIdx;
	UWORD uwCurrNode;
	UBYTE wasLastAggresive;
	UBYTE isAstarStarted;
	//
	WORD wBiggestDelta;
	const struct _tNode *pTargetSrc, *pTarget;
	//
	WORD wBiggestDeltaCurrNode;
	const struct _tNode *pTargetCurrNode;
	tAiState eState;
	tDirection eNeighborIdx;
	UBYTE ubCnt;
	UBYTE isPressingFire;
	tDirection ePrevDir;
} tAi;

void aiCreate(const tMap *pMap);

void aiDestroy(void);

void aiSetNodeCount(void);

void aiInit(tAi *pAi, UBYTE ubPlayerIdx);

tDirection aiProcess(tAi *pAi);

#endif // _GERMZ_AI_H_
