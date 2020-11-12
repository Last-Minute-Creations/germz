/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _GERMZ_PLAYER_H_
#define _GERMZ_PLAYER_H_

#include "bob_new.h"
#include "map.h"
#include "plep.h"
#include "steer.h"

#define PLEPS_PER_PLAYER 3

typedef enum _tPlayerIdx {
	PLAYER_1,
	PLAYER_2,
	PLAYER_3,
	PLAYER_4,
	PLAYER_NONE,
} tPlayerIdx;

typedef struct _tPlayer {
	tBobNew *pBobCursor;
	ULONG ulRepeatCounter;
	struct _tPlayer *pTeamMate;
	struct _tSteer *pSteer;
	struct _tNode *pNodeCursor;
	struct _tNode *pNodePlepSrc;
	struct _tPlep pPleps[PLEPS_PER_PLAYER];
	BYTE bNodeCount;
	UBYTE isSelectingDestination;
	UBYTE isDead;
	const tMapPlayerData *pMapData;
} tPlayer;

void playerCreate(void);

void playerDestroy(void);

void playerReset(tPlayerIdx eIdx, struct _tNode *pStartNode);

/**
 * @brief
 *
 * @return Number of players alive.
 */
UBYTE playerProcess(void);

tPlayer *playerFromTile(enum _tTile eTile);

tPlayerIdx playerToIdx(const tPlayer *pPlayer);

tPlayerIdx playerIdxFromTile(tTile eTile);

tPlayer *playerFromIdx(tPlayerIdx eIdx);

void playerUpdateDead(tPlayer *pPlayer);

void playerAllDead(void);

void playerPushCursors(void);

UBYTE playerHasFreePlep(const tPlayer *pPlayer);

#endif // _GERMZ_PLAYER_H_
