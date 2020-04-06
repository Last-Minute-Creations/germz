/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef _GERMZ_PLAYER_H_
#define _GERMZ_PLAYER_H_

#include "bob_new.h"
#include "map.h"
#include "plep.h"

#define PLEPS_PER_PLAYER 3

typedef struct _tPlayer {
	tBobNew sBobCursor;
	UBYTE ubJoy;
	UBYTE isSelectingDestination;
	struct _tNode *pNodeCursor, *pNodePlepSrc;
	struct _tPlep pPleps[PLEPS_PER_PLAYER];
} tPlayer;

void playerCreate(void);

void playerDestroy(void);

void playerReset(struct _tNode *pNodeStart);

void playerProcess(void);

void playerSpawnPlep(tPlayer *pPlayer);

enum _tTile playerToTile(const tPlayer *pPlayer);

UBYTE playerToIdx(const tPlayer *pPlayer);

tPlayer *playerFromTile(enum _tTile eTile);

#endif // _GERMZ_PLAYER_H_
